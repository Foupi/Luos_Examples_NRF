#include "mesh_init.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>                // bool

// NRF
#include "sdk_errors.h"             // ret_code_t

// NRF APPS
#include "app_error.h"              // APP_ERROR_CHECK

// MESH SDK
#include "device_state_manager.h"   // DSM_HANDLE_INVALID

// MESH MODELS
#include "config_client.h"          // config_client_init
#include "config_server_events.h"   // config_server_evt_t

// CUSTOM
#include "app_health_client.h"      // app_health_client_init
#include "luos_mesh_common.h"       // _mesh_init
#include "network_ctx.h"            // g_network_ctx
#include "node_config.h"            // config_client_msg_handler

#ifdef DEBUG
#include "nrf_log.h"                // NRF_LOG_INFO
#endif /* DEBUG */

/*      STATIC/GLOBAL VARIABLES AND CONSTANTS                       */

// Describes if the device is provisioned.
bool    g_device_provisioned    = false;

/*      CALLBACKS                                                   */

/* FIXME    On node reset event, erases persistent data and resets the
**          board.
*/
static void config_server_event_cb(const config_server_evt_t* event);

/* Timeout: FIXME Call timeout function.
** Cancel:  FIXME Call cancel function.
** Message: Send event to message handler.
*/
static void config_client_event_cb(config_client_event_type_t type,
                                   const config_client_event_t* event,
                                   uint16_t len);

// Initializes the config client and health client models.
static void models_init_cb(void);

void mesh_init(void)
{
    // Initialize Mesh stack with given callbacks and global boolean.
    _mesh_init(config_server_event_cb, models_init_cb,
               &g_device_provisioned);
}

static void config_server_event_cb(const config_server_evt_t* event)
{
    switch (event->type)
    {
    case CONFIG_SERVER_EVT_NODE_RESET:
        // FIXME Erase persistent data.

        // FIXME Reset board.
        break;

    default:
        // Nothing to do.
        break;
    }
}

static void config_client_event_cb(config_client_event_type_t type,
                                   const config_client_event_t* event,
                                   uint16_t len)
{
    switch (type)
    {
    case CONFIG_CLIENT_EVENT_TYPE_MSG:
        // Call message handler implemented in Node Config module.
        config_client_msg_handler(event);

        break;

    default:
        #ifdef DEBUG
        NRF_LOG_INFO("Config client event received: type %u!", type);
        #endif /* DEBUG */

        // Nothing to do.
        break;
    }
}

static void models_init_cb(void)
{
    // Reset network context.
    g_network_ctx.netkey_handle         = DSM_HANDLE_INVALID;
    g_network_ctx.appkey_handle         = DSM_HANDLE_INVALID;
    g_network_ctx.self_devkey_handle    = DSM_HANDLE_INVALID;

    // Initialize Config Client module with the given callback.
    ret_code_t  err_code    = config_client_init(config_client_event_cb);
    APP_ERROR_CHECK(err_code);

    // Initialize Health Client module.
    app_health_client_init();
}
