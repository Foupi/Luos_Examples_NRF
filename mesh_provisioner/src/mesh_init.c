#include "mesh_init.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>                // bool

// NRF
#include "nrf_log.h"                // NRF_LOG_INFO
#include "sdk_errors.h"             // ret_code_t

// NRF APPS
#include "app_error.h"              // APP_ERROR_CHECK

// MESH SDK
#include "device_state_manager.h"   // DSM_HANDLE_INVALID

// MESH MODELS
#include "config_client.h"          // config_client_init
#include "config_server_events.h"   // config_server_evt_t

// CUSTOM
#include "luos_mesh_common.h"       // _mesh_init
#include "network_ctx.h"            // g_network_ctx

/*      STATIC/GLOBAL VARIABLES AND CONSTANTS                       */

bool    g_device_provisioned    = false;

/*      CALLBACKS                                                   */

// On node reset event, erases persistent data and resets the board.
static void config_server_event_cb(const config_server_evt_t* event);

// Initializes the config client and health client models.
static void models_init_cb(void);

void mesh_init(void)
{
    _mesh_init(config_server_event_cb, models_init_cb,
               &g_device_provisioned);
}

static void config_server_event_cb(const config_server_evt_t* event)
{
    if (event->type == CONFIG_SERVER_EVT_NODE_RESET)
    {
        // FIXME Erase persistent data.

        // FIXME Reset board.
    }
}

static void models_init_cb(void)
{
    g_network_ctx.netkey_handle         = DSM_HANDLE_INVALID;
    g_network_ctx.appkey_handle         = DSM_HANDLE_INVALID;
    g_network_ctx.self_devkey_handle    = DSM_HANDLE_INVALID;

    NRF_LOG_INFO("Initializing %u models!", ACCESS_MODEL_COUNT);

    ret_code_t err_code = config_client_init(/* FIXME */ NULL);
    APP_ERROR_CHECK(err_code);

    // FIXME Init health client.
}
