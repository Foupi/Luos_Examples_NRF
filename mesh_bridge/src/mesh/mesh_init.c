#include "mesh_init.h"

/*      INCLUDES                                                    */

// MESH SDK
#include "device_state_manager.h"   // dsm_*

// MESH MODELS
#include "config_server_events.h"   // config_server_evt_t

// CUSTOM
#include "app_luos_msg_model.h"     // app_luos_msg_model_*
#include "app_luos_rtb_model.h"     // app_luos_rtb_model_*
#include "luos_mesh_common.h"       // _mesh_init
#include "mesh_msg_queue_manager.h" // luos_mesh_msg_queue_manager_init

/*      STATIC VARIABLES & CONSTANTS                                */

// Describes if the device is provisioned.
bool    g_device_provisioned   = false;

/*      CALLBACKS                                                   */

// On node reset event, erases persistent data and resets the board.
static void config_server_event_cb(const config_server_evt_t* event);

// Initialize the Luos RTB and Luos MSG models present on the node.
static void models_init_cb(void);

void mesh_init(void)
{
    // Initialize Mesh stack with given callbacks and global boolean.
    _mesh_init(config_server_event_cb, models_init_cb,
               &g_device_provisioned);
}

uint16_t mesh_device_get_address(void)
{
    /* Fetch the range of unicast addresses on this device from Device
    ** State Manager (persistent memory).
    */
    dsm_local_unicast_address_t local_addr_range;
    dsm_local_unicast_addresses_get(&local_addr_range);

    // Device unicast address is the first of this range.
    return local_addr_range.address_start;
}

void mesh_models_set_addresses(uint16_t device_address)
{
    // Set internal addresses of Luos RTB and Luos MSG models.
    app_luos_msg_model_address_set(device_address);
    app_luos_rtb_model_address_set(device_address);
}

static void config_server_event_cb(const config_server_evt_t* event)
{
    switch (event->type)
    {
    case CONFIG_SERVER_EVT_NODE_RESET:
        // FIXME Reset board.
        break;

    default:
        // Nothing to do.
        break;
    }
}

static void models_init_cb(void)
{
    // Initialize the message queue manager.
    // FIXME Maybe this should be done somewhere else?
    luos_mesh_msg_queue_manager_init();

    // Initialize Luos RTB and Luos MSG models.
    app_luos_rtb_model_init();
    app_luos_msg_model_init();
}
