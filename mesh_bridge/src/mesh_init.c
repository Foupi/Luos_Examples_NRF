#include "mesh_init.h"

/*      INCLUDES                                                    */

// NRF
#include "nrf_log.h"                // NRF_LOG_INFO

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

void mesh_models_set_addresses(uint16_t device_address)
{
    luos_rtb_model_set_address(&s_luos_rtb_model, device_address);
}

void mesh_rtb_get(void)
{
    luos_rtb_model_get(&s_luos_rtb_model);
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
    luos_rtb_model_init_params_t    init_params;
    // FIXME Fill parameters.

    luos_rtb_model_init(&s_luos_rtb_model, &init_params);
}
