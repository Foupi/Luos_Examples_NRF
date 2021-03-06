#include "app_luos_rtb_model.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>                 // uint16_t
#include <string.h>                 // memset

// NRF
#include "sdk_errors.h"             // ret_code_t

// NRF APPS
#include "app_error.h"              // APP_ERROR_CHECK
#include "app_timer.h"              // app_timer_*

// MESH SDK
#include "access.h"                 // access_message_rx_t

// LUOS
#include "config.h"                 // BROADCAST_VAL
#include "luos.h"                   // container_t
#include "luos_utils.h"             // LUOS_ASSERT
#include "routing_table.h"          // routing_table_t

// CUSTOM
#include "local_container_table.h"  // local_container_*
#include "luos_rtb_model.h"         // luos_rtb_model_*
#include "luos_rtb_model_common.h"  // LUOS_RTB_MODEL_MAX_RTB_ENTRY
#include "mesh_bridge.h"            // MESH_BRIDGE_*
#include "mesh_bridge_utils.h"      /* find_mesh_bridge_container_id,
                                    ** indicate_ext_rtb_*
                                    */
#include "mesh_init.h"              // g_device_provisioned
#include "mesh_msg_queue_manager.h" // tx_queue_*, luos_mesh_msg_prepare
#include "remote_container_table.h" // remote_container_*

#ifdef DEBUG
#include "nrf_log.h"                // NRF_LOG_INFO
#endif /* DEBUG */

/*      TYPEDEFS                                                    */

// Luos RTB model management states.
typedef enum
{
    // Idle state.
    LUOS_RTB_MODEL_STATE_IDLE,

    // Node initiating procedure:

    // Getting remote RTB.
    LUOS_RTB_MODEL_STATE_GETTING,

    // Publishing local RTB.
    LUOS_RTB_MODEL_STATE_PUBLISHING,

    // Node responding to the procedure:

    // Replying to request with local RTB.
    LUOS_RTB_MODEL_STATE_REPLYING,

    // Receiving remote RTB.
    LUOS_RTB_MODEL_STATE_RECEIVING,

} luos_rtb_model_state_t;

/*      STATIC VARIABLES & CONSTANTS                                */

// Luos RTB model instance.
static luos_rtb_model_t s_luos_rtb_model;

// Timer used to detect end of RTB reception.
APP_TIMER_DEF(s_entries_reception_timer);

// Delay to wait for first RTB entry.
#define                 WAIT_FIRST_ENTRY_DELAY_MS           2000
static const uint32_t   WAIT_FIRST_ENTRY_DELAY_TICKS        = APP_TIMER_TICKS(WAIT_FIRST_ENTRY_DELAY_MS);

// Delay to wait for next replied RTB entry.
#define                 WAIT_NEXT_ENTRY_REPLY_DELAY_MS      500
static const uint32_t   WAIT_NEXT_ENTRY_REPLY_DELAY_TICKS   = APP_TIMER_TICKS(WAIT_NEXT_ENTRY_REPLY_DELAY_MS);

// Delay to wait for next published RTB entry.
#define                 WAIT_NEXT_ENTRY_PUBLISH_DELAY_MS    4000
static const uint32_t   WAIT_NEXT_ENTRY_PUBLISH_DELAY_TICKS = APP_TIMER_TICKS(WAIT_NEXT_ENTRY_PUBLISH_DELAY_MS);

static struct
{
    // Current state.
    luos_rtb_model_state_t  curr_state;

    // Container from which Ext-RTB complete msg shall be sent.
    container_t*            mesh_bridge_container;

    // ID of the Mesh Bridge container.
    uint16_t                curr_mesh_bridge_id;

    // ID of the container requesting the Ext-RTB procedure.
    uint16_t                curr_ext_rtb_src_id;

}                       s_luos_rtb_model_ctx;

/*      STATIC FUNCTIONS                                            */

/* Retrieves local RTB and stores it in the given arguments. Returns
** true if the local container table was filled, false otherwise.
*/
static bool get_rtb_entries(routing_table_t* rtb_entries,
                            uint16_t* nb_entries);

// Resets state, updates tables, runs detection, sends ext-RTB complete.
static void ext_rtb_complete(void);

/*      CALLBACKS                                                   */

// Prepares the queue element and enqueues it.
static void rtb_model_get_send(luos_rtb_model_t* instance,
                               const luos_rtb_model_get_t* get_req);

// Prepares the queue element and enqueues it.
static void rtb_model_status_send(luos_rtb_model_t* instance,
    const luos_rtb_model_status_t* status_msg);

// Prepares the queue element and enqueues it.
static void rtb_model_status_reply(luos_rtb_model_t* instance,
    const luos_rtb_model_status_t* status_reply,
    const access_message_rx_t* msg);

/* Clears the remote containers table for the given address and switches
** state to REPLYING.
*/
static void rtb_model_get_cb(uint16_t src_addr);

/* Stores the received entry in the remote container table and restarts
** the timer.
*/
static void rtb_model_status_cb(uint16_t src_addr,
                                const routing_table_t* entry,
                                uint16_t entry_idx);

// Switches state depending on the current state.
static void entries_reception_timeout_cb(void* context);

void app_luos_rtb_model_init(void)
{
    ret_code_t                      err_code;

    // Parameters to initialize the internal Luos RTB model.
    luos_rtb_model_init_params_t    init_params;
    memset(&init_params, 0, sizeof(luos_rtb_model_init_params_t));
    init_params.get_send            = rtb_model_get_send;
    init_params.status_send         = rtb_model_status_send;
    init_params.status_reply        = rtb_model_status_reply;
    init_params.get_cb              = rtb_model_get_cb;
    init_params.rtb_entries_get_cb  = get_rtb_entries;
    init_params.status_cb           = rtb_model_status_cb;

    // Initialize the internal model instance.
    luos_rtb_model_init(&s_luos_rtb_model, &init_params);

    // Create timer instance for RTB entries reception.
    err_code                        = app_timer_create(
                                        &s_entries_reception_timer,
                                        APP_TIMER_MODE_SINGLE_SHOT,
                                        entries_reception_timeout_cb
                                      );
    APP_ERROR_CHECK(err_code);

    // Start in Idle state.
    s_luos_rtb_model_ctx.curr_state = LUOS_RTB_MODEL_STATE_IDLE;

    // Clear internal tables.
    /* FIXME Maybe this should be put somewhere else, as internal tables
    ** are shared?
    */
    local_container_table_clear();
    remote_container_table_clear();
}

void app_luos_rtb_model_address_set(uint16_t device_address)
{
    luos_rtb_model_set_address(&s_luos_rtb_model, device_address);
}

void app_luos_rtb_model_container_set(container_t* mesh_bridge_container)
{
    s_luos_rtb_model_ctx.mesh_bridge_container  = mesh_bridge_container;
}

void app_luos_rtb_model_engage_ext_rtb(uint16_t src_id,
                                       uint16_t mesh_bridge_id)
{
    if (!g_device_provisioned)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Device not provisioned yet!");
        #endif /* DEBUG */

        return;
    }

    if (s_luos_rtb_model_ctx.curr_state != LUOS_RTB_MODEL_STATE_IDLE)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Cannot send Luos RTB GET requests out of IDLE mode!");
        #endif /* DEBUG */

        return;
    }

    // This operation resets the remote container table.
    remote_container_table_clear();

    // Send Luos RTB GET request through internal model instance.
    luos_rtb_model_get(&s_luos_rtb_model);

    ret_code_t  err_code;

    // Start timer for entries reception.
    err_code                                    = app_timer_start(
                                                    s_entries_reception_timer,
                                                    WAIT_FIRST_ENTRY_DELAY_TICKS,
                                                    NULL
                                                  );
    APP_ERROR_CHECK(err_code);

    #ifdef DEBUG
    NRF_LOG_INFO("Engaging ext-RTB procedure: switch to GETTING state!");
    #else /* ! DEBUG */
    indicate_ext_rtb_engaged();
    #endif /* DEBUG */

    // Update current state.
    s_luos_rtb_model_ctx.curr_state             = LUOS_RTB_MODEL_STATE_GETTING;

    /* Mesh Bridge container ID, for internal tables update at
    ** detection.
    */
    s_luos_rtb_model_ctx.curr_mesh_bridge_id    = mesh_bridge_id;

    // ID of container requesting the Ext-RTB procedure, for response.
    s_luos_rtb_model_ctx.curr_ext_rtb_src_id    = src_id;
}

void app_luos_rtb_model_publication_end(void)
{
    #ifdef DEBUG
    NRF_LOG_INFO("Published local RTB, ext-RTB procedure complete: switch back to IDLE mode!");
    #else /* ! DEBUG */
    indicate_ext_rtb_complete();
    #endif /* DEBUG */

    ext_rtb_complete();
}

static bool get_rtb_entries(routing_table_t* rtb_entries,
                            uint16_t* nb_entries)
{
    // Retrieve number of entries in the local container table.
    uint16_t    nb_local_entries;
    nb_local_entries    = local_container_table_get_nb_entries();
    if (nb_local_entries == 0)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Local container table not filled!");
        #endif /* DEBUG */

        // Loop will be skipped.
    }

    // Copy each exposed entry in arguments.
    for (uint16_t entry_idx = 0; entry_idx < nb_local_entries;
         entry_idx++)
    {
        // Exposed entry at current index in the local container table.
        routing_table_t*    entry;
        entry   = local_container_table_get_entry_from_idx(entry_idx);

        // Check result.
        LUOS_ASSERT(entry != NULL);

        // Copy entry in argument.
        memcpy(rtb_entries + entry_idx, entry, sizeof(routing_table_t));
    }

    // Write number of local entries in argument.
    *nb_entries         = nb_local_entries;

    /* FIXME    Put this responsibility in
    **          `app_luos_rtb_model_publication_end`.
    */
    if (s_luos_rtb_model_ctx.curr_state == LUOS_RTB_MODEL_STATE_REPLYING)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Replied with local RTB: switch to RECEIVING mode!");
        #endif /* DEBUG */

        s_luos_rtb_model_ctx.curr_state = LUOS_RTB_MODEL_STATE_RECEIVING;

        // Start timer for first received entry.
        ret_code_t  err_code;
        err_code                        = app_timer_start(
                                            s_entries_reception_timer,
                                            WAIT_FIRST_ENTRY_DELAY_TICKS,
                                            NULL
                                          );
        APP_ERROR_CHECK(err_code);
    }

    // True if local container table was filled, false otherwise...
    return (nb_local_entries != 0);
}

static void ext_rtb_complete(void)
{
    bool    is_status_publishing;
    is_status_publishing    = (s_luos_rtb_model_ctx.curr_state == LUOS_RTB_MODEL_STATE_PUBLISHING);

    bool    is_status_receiving;
    is_status_receiving     = (s_luos_rtb_model_ctx.curr_state == LUOS_RTB_MODEL_STATE_RECEIVING);

    // Checking context.
    LUOS_ASSERT(is_status_publishing || is_status_receiving);

    if (is_status_publishing)
    {
        /* Publishing state means that Ext-RTB procedure was initiated
        ** due to a container message. As a detection is going to be
        ** run, its future ID must be found for sending it the Ext-RTB
        ** complete message.
        */
        uint16_t    new_src_id;
        new_src_id                                  = RoutingTB_FindFutureContainerID(
                                                        s_luos_rtb_model_ctx.curr_ext_rtb_src_id,
                                                        s_luos_rtb_model_ctx.curr_mesh_bridge_id
                                                      );

        s_luos_rtb_model_ctx.curr_ext_rtb_src_id    = new_src_id;
    }

    /* Update local container table entries local IDs, as a detection
    ** will be run.
    */
    local_container_table_update_local_ids(s_luos_rtb_model_ctx.curr_mesh_bridge_id);
    // No need to update IDs of remote containers, as they were inserted with correct IDs.

    // Run detection to add new remote containers to RTB.
    RoutingTB_DetectContainers(s_luos_rtb_model_ctx.mesh_bridge_container);

    // Message signaling completion of the Ext-RTB procedure.
    msg_t   ext_rtb_complete_msg;
    memset(&ext_rtb_complete_msg, 0, sizeof(msg_t));
    if (is_status_publishing)
    {
        // Procedure was initiated by a source container.
        ext_rtb_complete_msg.header.target_mode = ID;
        ext_rtb_complete_msg.header.target      = s_luos_rtb_model_ctx.curr_ext_rtb_src_id;
    }
    else
    {
        // Receiving mode: no source container. Broadcast message.
        ext_rtb_complete_msg.header.target_mode = BROADCAST;
        ext_rtb_complete_msg.header.target      = BROADCAST_VAL;
    }
    ext_rtb_complete_msg.header.cmd = MESH_BRIDGE_EXT_RTB_COMPLETE;

    // Send message through internal container instance.
    Luos_SendMsg(s_luos_rtb_model_ctx.mesh_bridge_container,
                 &ext_rtb_complete_msg);

    // Switch status back to Idle.
    s_luos_rtb_model_ctx.curr_state = LUOS_RTB_MODEL_STATE_IDLE;
}

static void rtb_model_get_send(luos_rtb_model_t* instance,
                               const luos_rtb_model_get_t* get_req)
{
    // Check parameters.
    LUOS_ASSERT(instance != NULL);
    LUOS_ASSERT(get_req != NULL);

    // Create Luos RTB GET TX queue message.
    tx_queue_luos_rtb_model_elm_t   rtb_model_msg;
    memset(&rtb_model_msg, 0, sizeof(rtb_model_msg));
    rtb_model_msg.cmd                   = TX_QUEUE_CMD_GET;
    rtb_model_msg.content.get           = *get_req;         // No array type: no memcpy needed.

    // Encapsulate message in TX queue element.
    tx_queue_elm_t          new_msg;
    memset(&new_msg, 0, sizeof(tx_queue_elm_t));
    new_msg.model                       = TX_QUEUE_MODEL_LUOS_RTB;
    new_msg.model_handle                = instance->handle;
    new_msg.content.luos_rtb_model_msg  = rtb_model_msg;

    // Enqueue given element.
    luos_mesh_msg_prepare(&new_msg);
}

static void rtb_model_status_send(luos_rtb_model_t* instance,
    const luos_rtb_model_status_t* status_msg)
{
    // Check parameters.
    LUOS_ASSERT(instance != NULL);
    LUOS_ASSERT(status_msg != NULL);

    // Create Luos RTB STATUS TX queue message.
    tx_queue_luos_rtb_model_elm_t   rtb_model_msg;
    memset(&rtb_model_msg, 0, sizeof(tx_queue_luos_rtb_model_elm_t));
    rtb_model_msg.cmd       = TX_QUEUE_CMD_STATUS;
    memcpy(&(rtb_model_msg.content.status), status_msg,
           sizeof(luos_rtb_model_status_t));

    // Encapsulate message in TX queue element.
    tx_queue_elm_t                  new_msg;
    memset(&new_msg, 0, sizeof(tx_queue_elm_t));
    new_msg.model           = TX_QUEUE_MODEL_LUOS_RTB;
    new_msg.model_handle    = instance->handle;
    memcpy(&(new_msg.content.luos_rtb_model_msg), &rtb_model_msg,
           sizeof(tx_queue_luos_rtb_model_elm_t));

    // Enqueue given element.
    luos_mesh_msg_prepare(&new_msg);
}

static void rtb_model_status_reply(luos_rtb_model_t* instance,
    const luos_rtb_model_status_t* status_reply,
    const access_message_rx_t* msg)
{
    // Check parameters.
    LUOS_ASSERT(instance != NULL);
    LUOS_ASSERT(status_reply != NULL);
    LUOS_ASSERT(msg != NULL);

    // Create Luos RTB STATUS REPLY TX queue message.
    tx_queue_luos_rtb_model_elm_t   rtb_model_msg;
    memset(&rtb_model_msg, 0, sizeof(tx_queue_luos_rtb_model_elm_t));
    rtb_model_msg.cmd       = TX_QUEUE_CMD_STATUS_REPLY;
    memcpy(&(rtb_model_msg.content.status_reply.src_msg), msg,
           sizeof(access_message_rx_t));
    memcpy(&(rtb_model_msg.content.status_reply.status), status_reply,
           sizeof(luos_rtb_model_status_t));

    // Encapsulate message in TX queue element.
    tx_queue_elm_t                  new_msg;
    memset(&new_msg, 0, sizeof(tx_queue_elm_t));
    new_msg.model           = TX_QUEUE_MODEL_LUOS_RTB;
    new_msg.model_handle    = instance->handle;
    memcpy(&(new_msg.content.luos_rtb_model_msg),
           &rtb_model_msg, sizeof(tx_queue_luos_rtb_model_elm_t));

    // Enqueue given element.
    luos_mesh_msg_prepare(&new_msg);
}

static void rtb_model_get_cb(uint16_t src_addr)
{
    if (s_luos_rtb_model_ctx.curr_state != LUOS_RTB_MODEL_STATE_IDLE)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Luos RTB GET request received while not in IDLE mode!");
        #endif /* DEBUG */

        return;
    }

    routing_table_t*    rtb         = RoutingTB_Get();
    uint16_t            nb_entries  = RoutingTB_GetLastEntry();

    // Retrieve Mesh Bridge container ID in routing table.
    s_luos_rtb_model_ctx.curr_mesh_bridge_id    = find_mesh_bridge_container_id(
                                                    rtb, nb_entries
                                                  );

    /* Clear remote container table entries corresponding to the node
    ** sending the Luos RTB GET request.
    */
    remote_container_table_clear_address(src_addr);

    /* Update local IDs of remaining entries in prevision of the
    ** detection to come at the end of the procedure.
    */
    remote_container_table_update_local_ids(s_luos_rtb_model_ctx.curr_mesh_bridge_id);

    #ifdef DEBUG
    NRF_LOG_INFO("Luos RTB GET request received: switch to REPLYING mode!");
    #else /* ! DEBUG */
    indicate_ext_rtb_engaged();
    #endif /* DEBUG */

    s_luos_rtb_model_ctx.curr_state = LUOS_RTB_MODEL_STATE_REPLYING;
}

static void rtb_model_status_cb(uint16_t src_addr,
                                const routing_table_t* entry,
                                uint16_t entry_idx)
{
    bool        is_status_getting;
    is_status_getting   = (s_luos_rtb_model_ctx.curr_state == LUOS_RTB_MODEL_STATE_GETTING);

    if ((!is_status_getting) && (s_luos_rtb_model_ctx.curr_state != LUOS_RTB_MODEL_STATE_RECEIVING))
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Luos RTB STATUS message received while not in a reception mode!");
        #endif /* DEBUG */

        return;
    }

    ret_code_t  err_code;

    // Stop entries reception timer.
    err_code            = app_timer_stop(s_entries_reception_timer);
    APP_ERROR_CHECK(err_code);

    #ifdef DEBUG
    NRF_LOG_INFO("Luos RTB STATUS message received from node 0x%x: entry %u has ID 0x%x, type %s, alias %s!",
                 src_addr, entry_idx, entry->id,
                 RoutingTB_StringFromType(entry->type), entry->alias);
    #endif /* DEBUG */

    // Insert received entry in the remote container table.
    bool        insertion_complete;
    insertion_complete  = remote_container_table_add_entry(src_addr,
                                                           entry);
    if (!insertion_complete)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Remote container table full: could not insert received entry!");
        #endif /* DEBUG */

        // FIXME Change state to stop receiving unstorable entries?
    }

    // Find out next timeout duration from context.
    uint32_t    timeout_ticks;
    if (is_status_getting)
    {
        timeout_ticks   = WAIT_NEXT_ENTRY_REPLY_DELAY_TICKS;
    }
    else
    {
        // Receiving.
        timeout_ticks   = WAIT_NEXT_ENTRY_PUBLISH_DELAY_TICKS;
    }

    // Reset timer.
    err_code            = app_timer_start(s_entries_reception_timer,
                                          timeout_ticks, NULL);
    APP_ERROR_CHECK(err_code);
}

static void entries_reception_timeout_cb(void* context)
{
    bool            is_status_getting;
    is_status_getting               = (s_luos_rtb_model_ctx.curr_state == LUOS_RTB_MODEL_STATE_GETTING);

    bool            is_status_receiving;
    is_status_receiving             = (s_luos_rtb_model_ctx.curr_state == LUOS_RTB_MODEL_STATE_RECEIVING);

    // Check context.
    LUOS_ASSERT(is_status_getting || is_status_receiving);

    if (is_status_receiving)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Reception timeout for remote node entries, end of ext-RTB procedure: switch back to IDLE mode!");
        #else /* ! DEBUG */
        indicate_ext_rtb_complete();
        #endif /* DEBUG */

        ext_rtb_complete();

        return;
    }

    // Getting.

    #ifdef DEBUG
    NRF_LOG_INFO("Reception timeout for remote nodes entries: switch to PUBLISHING mode!");
    #endif /* DEBUG */

    s_luos_rtb_model_ctx.curr_state = LUOS_RTB_MODEL_STATE_PUBLISHING;

    routing_table_t local_rtb_entries[LUOS_RTB_MODEL_MAX_RTB_ENTRY];
    memset(local_rtb_entries, 0,
           LUOS_RTB_MODEL_MAX_RTB_ENTRY * sizeof(routing_table_t));

    bool            detection_complete;
    uint16_t        nb_local_entries;

    // Retrieve local container entries.
    detection_complete              = get_rtb_entries(local_rtb_entries,
                                        &nb_local_entries);
    if (!detection_complete)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Local container table not filled!");
        #endif /* DEBUG */

        return;
    }

    // Publish retrieved entries.
    luos_rtb_model_publish_entries(&s_luos_rtb_model,
                                   local_rtb_entries,
                                   nb_local_entries);
}
