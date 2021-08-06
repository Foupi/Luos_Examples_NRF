#include "app_luos_rtb_model.h"

/*      INCLUDES                                                    */

// NRF
#include "nrf_log.h"                // NRF_LOG_INFO
#include "sdk_errors.h"             // ret_code_t

// NRF APPS
#include "app_error.h"              // APP_ERROR_CHECK
#include "app_timer.h"              // app_timer_*

// C STANDARD
#include <stdint.h>                 // uint16_t
#include <string.h>                 // memset

// LUOS
#include "routing_table.h"          // routing_table_t

// CUSTOM
#include "luos_rtb_model.h"         // luos_rtb_model_*
#include "luos_rtb_model_common.h"  // LUOS_RTB_MODEL_MAX_RTB_ENTRY
#include "remote_container_table.h" // remote_container_*

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
#define                 WAIT_NEXT_ENTRY_PUBLISH_DELAY_MS    4000    // FIXME Publication takes a very long time...
static const uint32_t   WAIT_NEXT_ENTRY_PUBLISH_DELAY_TICKS = APP_TIMER_TICKS(WAIT_NEXT_ENTRY_PUBLISH_DELAY_MS);

static struct
{
    // Current state.
    luos_rtb_model_state_t  curr_state;

    // Table of local containers.
    struct
    {
        // Number of local containers.
        uint16_t        nb_local_containers;

        // Table of entries.
        routing_table_t local_containers[LUOS_RTB_MODEL_MAX_RTB_ENTRY];

    }                       local_container_table;

}                       s_luos_rtb_model_ctx;

/*      STATIC FUNCTIONS                                            */

// Retrieves local RTB and stores it in the given arguments.
static bool get_rtb_entries(routing_table_t* rtb_entries,
                            uint16_t* nb_entries);

// Stores routing table inside the static local containers table.
static bool retrieve_local_rtb(void);

/*      CALLBACKS                                                   */

/* Clears the remote containers table for the given address and switches
** state to REPLYING.
*/
static void rtb_model_get_cb(uint16_t src_addr);

/* Stores the received entry in the remote containers table and restarts
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

    luos_rtb_model_init_params_t    init_params;
    memset(&init_params, 0, sizeof(luos_rtb_model_init_params_t));
    init_params.get_cb                      = rtb_model_get_cb;
    init_params.local_rtb_entries_get_cb    = get_rtb_entries;
    init_params.status_cb                   = rtb_model_status_cb;

    luos_rtb_model_init(&s_luos_rtb_model, &init_params);

    err_code = app_timer_create(&s_entries_reception_timer,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                entries_reception_timeout_cb);
    APP_ERROR_CHECK(err_code);

    s_luos_rtb_model_ctx.curr_state = LUOS_RTB_MODEL_STATE_IDLE;
    memset(&(s_luos_rtb_model_ctx.local_container_table), 0,
           sizeof(s_luos_rtb_model_ctx.local_container_table));

    remote_container_table_clear();
}

void app_luos_rtb_model_address_set(uint16_t device_address)
{
    luos_rtb_model_set_address(&s_luos_rtb_model, device_address);
}

void app_luos_rtb_model_get(void)
{
    if (s_luos_rtb_model_ctx.curr_state != LUOS_RTB_MODEL_STATE_IDLE)
    {
        NRF_LOG_INFO("Cannot send Luos RTB GET requests out of IDLE mode!");
        return;
    }

    ret_code_t err_code;

    remote_container_table_clear();

    luos_rtb_model_get(&s_luos_rtb_model);

    err_code = app_timer_start(s_entries_reception_timer,
                               WAIT_FIRST_ENTRY_DELAY_TICKS, NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_INFO("Engaging ext-RTB procedure: switch to GETTING state!");
    s_luos_rtb_model_ctx.curr_state = LUOS_RTB_MODEL_STATE_GETTING;
}

static bool get_rtb_entries(routing_table_t* rtb_entries,
                            uint16_t* nb_entries)
{
    ret_code_t          err_code;

    if (s_luos_rtb_model_ctx.local_container_table.nb_local_containers == 0)
    {
        // Local routing table has not been stored in static context.
        bool    detection_complete = retrieve_local_rtb();
        if (!detection_complete)
        {
            return false;
        }
    }

    memcpy(rtb_entries, s_luos_rtb_model_ctx.local_container_table.local_containers,
           s_luos_rtb_model_ctx.local_container_table.nb_local_containers * sizeof(routing_table_t));
    *nb_entries = s_luos_rtb_model_ctx.local_container_table.nb_local_containers;

    // FIXME Create callback for end of RTB entries enqueuing?
    if (s_luos_rtb_model_ctx.curr_state == LUOS_RTB_MODEL_STATE_REPLYING)
    {
        NRF_LOG_INFO("Replied with local RTB: switch to RECEIVING mode!");
        s_luos_rtb_model_ctx.curr_state = LUOS_RTB_MODEL_STATE_RECEIVING;

        err_code = app_timer_start(s_entries_reception_timer,
                                   WAIT_FIRST_ENTRY_DELAY_TICKS, NULL);
        APP_ERROR_CHECK(err_code);
    }
    else if (s_luos_rtb_model_ctx.curr_state == LUOS_RTB_MODEL_STATE_PUBLISHING)
    {
        NRF_LOG_INFO("Published local RTB, ext-RTB procedure complete: switch back to IDLE mode!");
        s_luos_rtb_model_ctx.curr_state = LUOS_RTB_MODEL_STATE_IDLE;
    }

    return true;
}

static bool retrieve_local_rtb(void)
{
    uint16_t            local_nb_entries    = RoutingTB_GetLastEntry();
    if (local_nb_entries == 0)
    {
        // Local routing table is empty: detection was not performed.
        return false;
    }

    routing_table_t*    local_rtb           = RoutingTB_Get();
    for (uint16_t entry_idx = 0; entry_idx < local_nb_entries;
         entry_idx++)
    {
        if (local_rtb[entry_idx].mode != CONTAINER)
        {
            // Only export containers.
            continue;
        }

        memcpy(s_luos_rtb_model_ctx.local_container_table.local_containers + s_luos_rtb_model_ctx.local_container_table.nb_local_containers,
               local_rtb + entry_idx, sizeof(routing_table_t));
        s_luos_rtb_model_ctx.local_container_table.nb_local_containers++;

        if (s_luos_rtb_model_ctx.local_container_table.nb_local_containers >= LUOS_RTB_MODEL_MAX_RTB_ENTRY)
        {
            // Do not export any more entries.
            break;
        }
    }

    return true;
}

static void rtb_model_get_cb(uint16_t src_addr)
{
    if (s_luos_rtb_model_ctx.curr_state != LUOS_RTB_MODEL_STATE_IDLE)
    {
        NRF_LOG_INFO("Luos RTB GET request received while not in IDLE mode!");
        return;
    }

    remote_container_table_clear_address(src_addr);

    NRF_LOG_INFO("Luos RTB GET request received: switch to REPLYING mode!");
    s_luos_rtb_model_ctx.curr_state = LUOS_RTB_MODEL_STATE_REPLYING;
}

static void rtb_model_status_cb(uint16_t src_addr,
                                const routing_table_t* entry,
                                uint16_t entry_idx)
{
    if ((s_luos_rtb_model_ctx.curr_state != LUOS_RTB_MODEL_STATE_GETTING)
        && (s_luos_rtb_model_ctx.curr_state != LUOS_RTB_MODEL_STATE_RECEIVING))
    {
        NRF_LOG_INFO("Luos RTB STATUS message received while not in a reception mode!");
        return;
    }

    ret_code_t  err_code;

    err_code = app_timer_stop(s_entries_reception_timer);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_INFO("Luos RTB STATUS message received from node 0x%x: entry %u has ID 0x%x, type %s, alias %s!",
                 src_addr, entry_idx, entry->id,
                 RoutingTB_StringFromType(entry->type), entry->alias);

    bool        insertion_complete;
    insertion_complete = remote_container_table_add_entry(src_addr, entry);
    if (!insertion_complete)
    {
        NRF_LOG_INFO("Remote container table full: could not insert received entry!");
        // FIXME Change state to stop receiving unstorable entries?
    }

    if (s_luos_rtb_model_ctx.curr_state == LUOS_RTB_MODEL_STATE_GETTING)
    {
        err_code = app_timer_start(s_entries_reception_timer,
                                   WAIT_NEXT_ENTRY_REPLY_DELAY_TICKS,
                                   NULL);
    }
    else
    {
        err_code = app_timer_start(s_entries_reception_timer,
                                   WAIT_NEXT_ENTRY_PUBLISH_DELAY_TICKS,
                                   NULL);
    }
    APP_ERROR_CHECK(err_code);
}

static void entries_reception_timeout_cb(void* context)
{
    if (s_luos_rtb_model_ctx.curr_state == LUOS_RTB_MODEL_STATE_GETTING)
    {
        NRF_LOG_INFO("Reception timeout for remote nodes entries: switch to PUBLISHING mode!");
        s_luos_rtb_model_ctx.curr_state = LUOS_RTB_MODEL_STATE_PUBLISHING;

        routing_table_t local_rtb_entries[LUOS_RTB_MODEL_MAX_RTB_ENTRY];
        uint16_t        nb_local_entries;
        bool            detection_complete;

        memset(local_rtb_entries, 0, LUOS_RTB_MODEL_MAX_RTB_ENTRY * sizeof(routing_table_t));

        detection_complete = get_rtb_entries(local_rtb_entries,
                                             &nb_local_entries);
        if (!detection_complete)
        {
            NRF_LOG_INFO("Local RTB cannot be retrieved: detection not complete!");
            return;
        }

        luos_rtb_model_publish_entries(&s_luos_rtb_model,
                                       local_rtb_entries,
                                       nb_local_entries);
    }
    else if (s_luos_rtb_model_ctx.curr_state == LUOS_RTB_MODEL_STATE_RECEIVING)
    {
        NRF_LOG_INFO("Reception timeout for remote node entries, end of ext-RTB procedure: switch back to IDLE mode!");
        s_luos_rtb_model_ctx.curr_state = LUOS_RTB_MODEL_STATE_IDLE;
    }
}
