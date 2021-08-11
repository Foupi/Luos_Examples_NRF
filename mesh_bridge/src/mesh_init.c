#include "mesh_init.h"

/*      INCLUDES                                                    */

// MESH MODELS
#include "config_server_events.h"   // config_server_evt_t

// CUSTOM
#include "app_luos_msg_model.h"     // app_luos_msg_model_*
#include "app_luos_rtb_model.h"     // app_luos_rtb_model_*
#include "luos_mesh_common.h"       // _mesh_init
#include "mesh_msg_queue_manager.h" // luos_mesh_msg_queue_manager_init

/*      STATIC VARIABLES & CONSTANTS                                */

// Describes if the device is provisioned.
bool                    g_device_provisioned   = false;

/*      CALLBACKS                                                   */

// On node reset event, erases persistent data and resets the board.
static void config_server_event_cb(const config_server_evt_t* event);

// Initialize the Luos RTB model present on the node.
static void models_init_cb(void);

void mesh_init(void)
{
    _mesh_init(config_server_event_cb, models_init_cb,
               &g_device_provisioned);

    luos_mesh_msg_queue_manager_init();
}

void mesh_models_set_addresses(uint16_t device_address)
{
    app_luos_msg_model_address_set(device_address);
    app_luos_rtb_model_address_set(device_address);
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
        break;
    }
}

static void models_init_cb(void)
{
    app_luos_rtb_model_init();
    app_luos_msg_model_init();
}
