#include "mesh_init.h"

/*      INCLUDES                                                    */

// MESH MODELS
#include "config_server_events.h"   // config_server_evt_t

// CUSTOM
#include "luos_mesh_common.h"       // _mesh_init

/*      STATIC VARIABLES & CONSTANTS                                */

// Describes if the device is provisioned.
bool g_device_provisioned   = false;

/*      CALLBACKS                                                   */

// On node reset event, erases persistent data and resets the board.
static void config_server_event_cb(const config_server_evt_t* event);

// FIXME Initialize the models present on the node.
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
    // FIXME Initializations.
}
