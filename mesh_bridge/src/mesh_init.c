#include "mesh_init.h"

/*      INCLUDES                                                    */

// MESH MODELS
#include "config_server_events.h"   // config_server_evt_t

// CUSTOM
#include "luos_mesh_common.h"       // _mesh_init
#include "luos_rtb_model.h"         // luos_rtb_model_*

/*      STATIC VARIABLES & CONSTANTS                                */

// Describes if the device is provisioned.
bool                    g_device_provisioned   = false;

// Luos RTB model instance.
static luos_rtb_model_t s_luos_rtb_model;

/*      CALLBACKS                                                   */

// On node reset event, erases persistent data and resets the board.
static void config_server_event_cb(const config_server_evt_t* event);

// Initialize the Luos RTB model present on the node.
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
    luos_rtb_model_init_params_t    init_params;
    // FIXME Fill parameters.

    luos_rtb_model_init(&s_luos_rtb_model, &init_params);
}
