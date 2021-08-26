#include "provisioning.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>                // bool
#include <stdint.h>                 // uint*_t
#include <string.h>                 // memset

// NRF
#include "sdk_errors.h"             // ret_code_t

// NRF APPS
#include "app_error.h"              // APP_ERROR_CHECK

// MESH SDK
#include "device_state_manager.h"   // dsm_*
#include "mesh_config.h"            // mesh_config_*, MESH_CONFIG_*
#include "mesh_stack.h"             // mesh_stack_*
#include "nrf_mesh.h"               // nrf_mesh_*
#include "nrf_mesh_prov.h"          /* nrf_mesh_prov_*,
                                    ** NRF_MESH_PROV_*
                                    */

// MESH MODELS
#include "config_server.h"          // config_server_*
#include "config_server_events.h"   // config_server_evt_t

// CUSTOM
#include "luos_mesh_common.h"       /* _provisioning_init,
                                    ** encryption_keys_generate,
                                    ** auth_data_provide,
                                    ** LUOS_MESH_NETWORK_MAX_NODES
                                    */
#include "mesh_init.h"              // g_device_provisioned
#include "network_ctx.h"            // network_ctx_*
#include "node_config.h"            // node_config_start
#include "provisioner_config.h"     // prov_conf_*

#ifdef DEBUG
#include "nrf_log.h"                // NRF_LOG_INFO
#else /* ! DEBUG */
#include "boards.h"                 /* bsp_board_led_off,
                                    ** bsp_board_led_on
                                    */
#endif /* DEBUG */

/*      TYPEDEFS                                                    */

// Enum representing the current provisioning state.
typedef enum
{
    // Neither scanning nor provisioning.
    PROV_STATE_IDLE,

    // Scanning for unprovisioned devices.
    PROV_STATE_SCANNING,

    // Provisioning a device.
    PROV_STATE_PROVISIONING,

    // Waiting for link closure after successful provisioning.
    PROV_STATE_COMPLETE,

} prov_state_t;

/*      STATIC VARIABLES & CONSTANTS                                */

// Provisioning context.
static nrf_mesh_prov_ctx_t  s_prov_ctx;

// Unprovisioned device attention duration, in seconds.
static const uint32_t       ATTENTION_DURATION      = 5;

// Information regarding the current state and provisioned device.
static struct
{
    // Current provisioning state.
    prov_state_t                    prov_state;

    // Current provisioning configuration header state.
    prov_conf_header_entry_live_t   prov_conf_header;

    // Current node being approvisioned.
    prov_conf_node_entry_live_t     curr_conf_node;

    // Device key and address handles for each approvisioned node.
    dsm_handle_t                    devkey_handles[LUOS_MESH_NETWORK_MAX_NODES];
    dsm_handle_t                    address_handles[LUOS_MESH_NETWORK_MAX_NODES];

}                           s_prov_curr_state;

/*      INITIALIZATIONS                                             */

// Provisioner configuration file.
MESH_CONFIG_FILE(
    s_prov_conf_file,               // Config file name.
    PROV_CONF_FILE_ID,              // ID of the configuration file.
    MESH_CONFIG_STRATEGY_CONTINUOUS // Configuration storage strategy.
);

// Provisioner configuration header entry.
MESH_CONFIG_ENTRY(
    s_prov_conf_header_entry,               // Config entry name.
    PROV_CONF_HEADER_ENTRY_ID,              // ID of the configuration entry.
    1,                                      // Just one configuration entry.
    sizeof(prov_conf_header_entry_live_t),  // Size of a configuration entry.
    prov_conf_header_set_cb,                // Configuration entry setter.
    prov_conf_header_get_cb,                // Configuration entry getter.
    prov_conf_header_delete_cb,             // Configuration entry deleter.
    false                                   // Default value does not exist.
);

// Provisioner configuration node entries.
MESH_CONFIG_ENTRY(
    s_prov_conf_first_node_entry,           // Config entry name.
    PROV_CONF_NODE_ENTRY_ID(0),             // ID of the configuration entry.
    LUOS_MESH_NETWORK_MAX_NODES,            // Number of configuration entries.
    sizeof(prov_conf_node_entry_live_t),    // Size of a configuration entry.
    prov_conf_node_set_cb,                  // Configuration entry setter.
    prov_conf_node_get_cb,                  // Configuration entry getter.
    prov_conf_node_delete_cb,               // Configuration entry deleter.
    false                                   // Default value does not exist.
);

#ifndef DEBUG
// LED toggled to match scanning state.
static const uint32_t   DEFAULT_SCAN_LED_IDX    = 0;

// LED toggled to match provisioning state.
static const uint32_t   DEFAULT_PROV_LED_IDX    = 1;
#endif /* ! DEBUG */

/*      STATIC FUNCTIONS                                            */

#ifndef DEBUG
// Visually show scanning state.
void indicate_scanning_start(void);
void indicate_scanning_stop(void);

// Visually show provisioning start and completion.
void indicate_provisioning_begin(void);
void indicate_provisioning_end(void);
#endif /* ! DEBUG */

// Fetches persistent configuration.
static void prov_conf_fetch(void);

// Fetches persistent configuration regarding one node.
static void prov_node_conf_fetch(uint16_t node_entry_idx);

// Updates static context and persistent configuration.
static void prov_on_complete_event(const nrf_mesh_prov_evt_complete_t* event);

// Updates static context depending on current state.
static void prov_on_link_closed_event(void);

/*      CALLBACKS                                                   */

/* Link established:        Logs message.
** Caps received:           Uses received capacities for
**                          OOB authentication.
** Static request:          Provides static authentication data.
** Failed:                  Logs message.
** Complete:                Prepares remote config server setup.
** Link closed:             Adds node to static context if provisioning
**                          was successful.
*/
static void mesh_prov_event_cb(const nrf_mesh_prov_evt_t* event);

// Unprovisioned device:    Start provisioning the advertising device.
static void mesh_unprov_event_cb(const nrf_mesh_prov_evt_t* event);

void provisioning_init(void)
{
    // Initialize static context with given callback.
    _provisioning_init(&s_prov_ctx, mesh_prov_event_cb);

    // State starts as Idle.
    s_prov_curr_state.prov_state = PROV_STATE_IDLE;
}

void prov_conf_init(void)
{
    if (g_device_provisioned)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Retrieving persistent configuration!");
        #endif /* DEBUG */

        // Fetch persistent configuration.
        prov_conf_fetch();
    }
    else
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Generating persistent configuration!");
        #endif /* DEBUG */

        // Reset static context.
        s_prov_curr_state.prov_conf_header.nb_prov_nodes    = 0;
        s_prov_curr_state.prov_conf_header.nb_conf_nodes    = 0;
        s_prov_curr_state.prov_conf_header.next_address     = PROV_ELM_ADDRESS + ACCESS_ELEMENT_COUNT;

        // Store previously computed appkey from global network context.
        memcpy(s_prov_curr_state.prov_conf_header.appkey,
               g_network_ctx.appkey, NRF_MESH_KEY_SIZE);

        // Store static header config in persistent memory.
        mesh_config_entry_set(PROV_CONF_HEADER_ENTRY_ID,
                              &(s_prov_curr_state.prov_conf_header));
    }
}

bool prov_scan_start(void)
{
    if (s_prov_curr_state.prov_conf_header.nb_prov_nodes >= LUOS_MESH_NETWORK_MAX_NODES)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Cannot start scanning: max number of devices in Mesh network reached!");
        #endif /* DEBUG */

        return false;
    }

    ret_code_t  err_code;

    // Start scanning with the given callback for unprovisioned events.
    err_code    = nrf_mesh_prov_scan_start(mesh_unprov_event_cb);
    APP_ERROR_CHECK(err_code);

    // Switch state to Scanning.
    s_prov_curr_state.prov_state = PROV_STATE_SCANNING;

    #ifdef DEBUG
    NRF_LOG_INFO("Start scanning for unprovisioned devices!");
    #else /* ! DEBUG */
    indicate_scanning_start();
    #endif /* DEBUG */

    return true;
}

void prov_scan_stop(void)
{
    // Stop scanning for unprovisioned devices.
    nrf_mesh_prov_scan_stop();

    if (s_prov_curr_state.prov_state == PROV_STATE_SCANNING)
    {
        // Switch back to idle state.
        s_prov_curr_state.prov_state = PROV_STATE_IDLE;
    }

    #ifdef DEBUG
    NRF_LOG_INFO("Stop scanning for unprovisioned devices!");
    #else /* ! DEBUG */
    indicate_scanning_stop();
    #endif /* DEBUG */
}

#ifndef DEBUG
__attribute__((weak)) void indicate_scanning_start(void)
{
    bsp_board_led_on(DEFAULT_SCAN_LED_IDX);
}

__attribute__((weak)) void indicate_scanning_stop(void)
{
    bsp_board_led_off(DEFAULT_SCAN_LED_IDX);
}

__attribute__((weak)) void indicate_provisioning_begin(void)
{
    bsp_board_led_on(DEFAULT_PROV_LED_IDX);
}

__attribute__((weak)) void indicate_provisioning_end(void)
{
    bsp_board_led_off(DEFAULT_PROV_LED_IDX);
}
#endif /* ! DEBUG */

void prov_conf_fetch(void)
{
    // Store provisioning configuration header in static context.
    mesh_config_entry_get(PROV_CONF_HEADER_ENTRY_ID,
                          &(s_prov_curr_state.prov_conf_header));

    // Retrieved number of provisioned nodes.
    uint16_t                    nb_prov_nodes;
    nb_prov_nodes   = s_prov_curr_state.prov_conf_header.nb_prov_nodes;

    #ifdef DEBUG
    NRF_LOG_INFO("Number of provisioned nodes: %u!", nb_prov_nodes);
    #endif /* DEBUG */

    for (uint16_t node_entry_idx = 0; node_entry_idx < nb_prov_nodes;
         node_entry_idx++)
    {
        // Fetch node configuration entry for the given index.
        prov_node_conf_fetch(node_entry_idx);
    }

    if (nb_prov_nodes > s_prov_curr_state.prov_conf_header.nb_conf_nodes)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Last provisioned node must still be configured!");
        #endif /* DEBUG */

        /* One node has been provisioned but not configured: store
        ** it in current context so that it can be configured right
        ** away.
        */
        /* FIXME Store last node entry in static context and start with
        ** its configuration.
        */
    }

    /* Copy appkey from persistent configuration in global context:
    ** needed since appkey can not be retrieved from DSM.
    */
    memcpy(g_network_ctx.appkey,
           s_prov_curr_state.prov_conf_header.appkey,
           NRF_MESH_KEY_SIZE);
}

void prov_node_conf_fetch(uint16_t node_entry_idx)
{
    #ifdef DEBUG
    NRF_LOG_INFO("Retrieving config for node %u!", node_entry_idx);
    #endif /* DEBUG */

    // Store node configuration entry in buffer variable.
    prov_conf_node_entry_live_t node_conf_buffer;
    mesh_config_entry_get(PROV_CONF_NODE_ENTRY_ID(node_entry_idx),
                          &node_conf_buffer);

    // Unicast address of the current node configuration entry.
    uint16_t    node_unicast_addr   = node_conf_buffer.first_addr;

    ret_code_t                  err_code;

    /* Retrieve corresponding devkey handle from DSM and store
    ** it in static context.
    */
    err_code    = dsm_devkey_handle_get(
                    node_conf_buffer.first_addr,
                    s_prov_curr_state.devkey_handles + node_entry_idx
                  );
    APP_ERROR_CHECK(err_code);

    // Device address for address handle retrieval.
    nrf_mesh_address_t device_address;
    memset(&device_address, 0, sizeof(nrf_mesh_address_t));
    device_address.type             = NRF_MESH_ADDRESS_TYPE_UNICAST;    // Device address: unicast.
    device_address.value            = node_unicast_addr;                // Retrieved device address.

    /* Retrieve device address handle from DSM and store it in
    ** static context.
    */
    err_code    = dsm_address_handle_get(
                    &device_address,
                    s_prov_curr_state.address_handles + node_entry_idx
                  );
    APP_ERROR_CHECK(err_code);

    #ifdef DEBUG
    NRF_LOG_INFO("Unicast address: 0x%x, devkey handle: 0x%x, address handle: 0x%x!",
                 node_unicast_addr,
                 s_prov_curr_state.devkey_handles[node_entry_idx],
                 s_prov_curr_state.address_handles[node_entry_idx]);
    #endif /* DEBUG */
}

void prov_on_complete_event(const nrf_mesh_prov_evt_complete_t* event)
{

    // Switch state to Complete.
    s_prov_curr_state.prov_state    = PROV_STATE_COMPLETE;

    // Store static node index in variable for easier writing.
    uint16_t        node_idx                = s_prov_curr_state.prov_conf_header.nb_prov_nodes;

    // First unicast address of remote device.
    uint16_t        device_first_address    = event->p_prov_data->address;

    // Number of elements in the remote device.
    uint16_t        node_nb_elm             = s_prov_curr_state.curr_conf_node.nb_elm;

    // Remote device devkey.
    const uint8_t*  device_devkey           = event->p_devkey;

    #ifdef DEBUG
    NRF_LOG_INFO("Provisioning process complete for device 0x%x!",
                 device_first_address);
    #endif /* DEBUG */

    ret_code_t      err_code;

    /* Add first address of remote device as publication address in
    ** DSM.
    */
    err_code                        = dsm_address_publish_add(
                                        device_first_address,
                                        s_prov_curr_state.address_handles + node_idx
                                      );
    APP_ERROR_CHECK(err_code);

    // Add remote device devkey to subnet through DSM.
    err_code                        = dsm_devkey_add(
                                        device_first_address,
                                        g_network_ctx.netkey_handle,
                                        device_devkey,
                                        s_prov_curr_state.devkey_handles + node_idx
                                      );
    APP_ERROR_CHECK(err_code);

    // Temporary variable to ease writing.
    uint16_t*   next_address;
    next_address                    = &(s_prov_curr_state.prov_conf_header.next_address);

    // Update static provisioning configuration header.
    *next_address                   = device_first_address + node_nb_elm;
    s_prov_curr_state.prov_conf_header.nb_prov_nodes++;

    // Update persistent provisioning configuration header.
    mesh_config_entry_set(PROV_CONF_HEADER_ENTRY_ID,
                          &(s_prov_curr_state.prov_conf_header));

    // Temporary variable to ease writing.
    uint16_t*   curr_first_addr;
    curr_first_addr                 = &(s_prov_curr_state.curr_conf_node.first_addr);

    // Update static node configuration for given index.
    *curr_first_addr                = device_first_address;

    // Update persistent node configuration for given index.
    mesh_config_entry_set(PROV_CONF_NODE_ENTRY_ID(node_idx),
                          &(s_prov_curr_state.curr_conf_node));
}

// Updates static context depending on current state.
void prov_on_link_closed_event(void)
{
    switch (s_prov_curr_state.prov_state)
    {
    case PROV_STATE_COMPLETE:
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Provisioning link closed after successful procedure!");
        #endif /* DEBUG */

        // Switch state back to Idle.
        s_prov_curr_state.prov_state    = PROV_STATE_IDLE;

        /* Node index is number provisioned nodes - 1, since indexes
        ** start at O.
        */
        uint16_t    node_idx   = s_prov_curr_state.prov_conf_header.nb_prov_nodes - 1;

        // Start node configuration for given node and handles.
        node_config_start(s_prov_curr_state.curr_conf_node.first_addr,
                          s_prov_curr_state.devkey_handles[node_idx],
                          s_prov_curr_state.address_handles[node_idx]);
    }
        break;

    case PROV_STATE_PROVISIONING:
        #ifdef DEBUG
        NRF_LOG_INFO("Provisioning procedure was aborted!");
        #endif /* DEBUG */

        // Start scanning again, no need to look at the result.
        prov_scan_start();

        break;

    default:
        #ifdef DEBUG
        NRF_LOG_INFO("Provisioning link closed event received in inadequate state!");
        #endif /* DEBUG */

        break;
    }
}

static void mesh_prov_event_cb(const nrf_mesh_prov_evt_t* event)
{
    ret_code_t              err_code;

    // Remote device provisioning context.
    nrf_mesh_prov_ctx_t*    device_prov_ctx;

    switch (event->type)
    {
    case NRF_MESH_PROV_EVT_LINK_ESTABLISHED:
        if (s_prov_curr_state.prov_state != PROV_STATE_PROVISIONING)
        {
            #ifdef DEBUG
            NRF_LOG_INFO("Provisioning link established while not provisioning!");
            #endif /* DEBUG */

            return;
        }

        #ifdef DEBUG
        NRF_LOG_INFO("Provisioning link established with unprovisioned device!");
        #else /* ! DEBUG */
        indicate_provisioning_begin();
        #endif /* DEBUG */

        break;

    case NRF_MESH_PROV_EVT_CAPS_RECEIVED:
    {
        if (s_prov_curr_state.prov_state != PROV_STATE_PROVISIONING)
        {
            #ifdef DEBUG
            NRF_LOG_INFO("OOB capacities received while not provisioning!");
            #endif /* DEBUG */

            return;
        }

        #ifdef DEBUG
        NRF_LOG_INFO("Capacities received from unprovisioned device!");
        #endif /* DEBUG */

        // Store remote context from message.
        nrf_mesh_prov_evt_caps_received_t   oob_caps_received;
        oob_caps_received                       = event->params.oob_caps_received;
        device_prov_ctx                         = oob_caps_received.p_context;

        // Store received number of elements in static context.
        s_prov_curr_state.curr_conf_node.nb_elm = oob_caps_received.oob_caps.num_elements;

        // Use received out-of-band method (static).
        err_code                                = nrf_mesh_prov_oob_use(
                                                    device_prov_ctx,
                                                    NRF_MESH_PROV_OOB_METHOD_STATIC,
                                                    0, NRF_MESH_KEY_SIZE
                                                  );
        APP_ERROR_CHECK(err_code);
    }
        break;

    case NRF_MESH_PROV_EVT_STATIC_REQUEST:
        if (s_prov_curr_state.prov_state != PROV_STATE_PROVISIONING)
        {
            #ifdef DEBUG
            NRF_LOG_INFO("Static authentication data request received while not provisioning!");
            #endif /* DEBUG */

            return;
        }

        #ifdef DEBUG
        NRF_LOG_INFO("Static authentication data requested by unprovisioned device!");
        #endif /* DEBUG */

        // Store remote context from message.
        device_prov_ctx = event->params.static_request.p_context;

        // Provide internal authentication data to remote context.
        auth_data_provide(device_prov_ctx);

        break;

    case NRF_MESH_PROV_EVT_FAILED:
        if (s_prov_curr_state.prov_state != PROV_STATE_PROVISIONING)
        {
            #ifdef DEBUG
            NRF_LOG_INFO("Provisioning failed event received while not provisioning!");
            #endif /* DEBUG */

            return;
        }

        #ifdef DEBUG
        NRF_LOG_INFO("Provisioning procedure failed! Reason code: %u!",
                     event->params.failed.failure_code);
        #endif /* DEBUG */

        break;

    case NRF_MESH_PROV_EVT_COMPLETE:
    {
        if (s_prov_curr_state.prov_state != PROV_STATE_PROVISIONING)
        {
            #ifdef DEBUG
            NRF_LOG_INFO("Provisioning complete event received while not provisioning!");
            #endif /* DEBUG */

            return;
        }

        const nrf_mesh_prov_evt_complete_t* prov_complete;
        prov_complete   = &(event->params.complete);

        prov_on_complete_event(prov_complete);
    }

        break;

    case NRF_MESH_PROV_EVT_LINK_CLOSED:
        #ifndef DEBUG
        indicate_provisioning_end();
        #endif /* ! DEBUG */

        prov_on_link_closed_event();

        break;

    default:
        #ifdef DEBUG
        NRF_LOG_INFO("Mesh provisioning event received: type %u!",
                     event->type);
        #endif /* DEBUG */

        break;
    }
}

static void mesh_unprov_event_cb(const nrf_mesh_prov_evt_t* event)
{
    if (event->type != NRF_MESH_PROV_EVT_UNPROVISIONED_RECEIVED)
    {
        // Only manage unprovisioned device events.
        return;
    }

    if (s_prov_curr_state.prov_state != PROV_STATE_SCANNING)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Unprovisioned device detected while not scanning!");
        #endif /* DEBUG */

        return;
    }

    #ifdef DEBUG
    NRF_LOG_INFO("Start provisioning detected device!");
    #endif /* DEBUG */

    // Data to send to unprovisioned device.
    nrf_mesh_prov_provisioning_data_t   prov_data;
    memset(&prov_data, 0, sizeof(nrf_mesh_prov_provisioning_data_t));
    prov_data.netkey_index  = PROV_NETKEY_IDX;                                  // Index at which netkey shall be stored.
    prov_data.address       = s_prov_curr_state.prov_conf_header.next_address;  // First address of device.
    // Copy netkey in data from global network context.
    memcpy(prov_data.netkey, g_network_ctx.netkey, NRF_MESH_KEY_SIZE);

    // Generate internal encryption keys.
    encryption_keys_generate();

    // UUID of the unprovisioned device.
    const uint8_t*                      target_uuid = event->params.unprov.device_uuid;

    ret_code_t                          err_code;

    // Start provisioning unprovisioned device.
    err_code    = nrf_mesh_prov_provision(&s_prov_ctx, target_uuid,
                                          ATTENTION_DURATION,
                                          &prov_data,
                                          NRF_MESH_PROV_BEARER_ADV);
    APP_ERROR_CHECK(err_code);

    // Stop scanning for unprovisioned devices while provisioning.
    prov_scan_stop();

    // Switch state to Provisioning.
    s_prov_curr_state.prov_state = PROV_STATE_PROVISIONING;
}
