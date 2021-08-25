#include "mesh_bridge.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>                 // uint16_t

// LUOS
#include "luos.h"                   /* container_t,
                                    ** Luos_CreateContainer, msg_t
                                    */

// CUSTOM
#include "app_luos_msg_model.h"     // app_luos_msg_model_send_msg
#include "app_luos_rtb_model.h"     // app_luos_rtb_model_get
#include "local_container_table.h"  // local_container_table_*
#include "luos_mesh_common.h"       // mesh_start
#include "mesh_init.h"              // mesh_init, g_device_provisioned
#include "provisioning.h"           /* provisioning_init,
                                    ** persistent_conf_init,
                                    ** prov_listening_start
                                    */
#include "remote_container_table.h" // remote_container_table_print

// NRF
#ifdef DEBUG
#include "nrf_log.h"                // NRF_LOG_INFO
#endif /* DEBUG */

/*      STATIC/GLOBAL VARIABLES & CONSTANTS                         */

#ifndef REV
#define REV {0,0,1}
#endif

/*      STATIC VARIABLE & CONSTANTS                                 */

// Static container instance.
container_t*    s_mesh_bridge_instance;

/*      CALLBACKS                                                   */

/* EXT-RTB:             Engages EXT-RTB procedure.
** Print tables:        Prints the internal tables.
** Clear tables:        Clears the internal tables.
** Fill local table:    Fills the local container table with RTB
**                      entries.
** Update tables:       Update the local IDs of entries from internal
**                      tables.
*/
static void MeshBridge_MsgHandler(container_t* container, msg_t* msg);

void MeshBridge_Init(void)
{
    // Initialize Mesh stack.
    mesh_init();

    // Initialize provisioning module.
    provisioning_init();

    // Start Mesh stack.
    mesh_start();

    if (!g_device_provisioned)
    {
        // Listen for provisioning link.
        prov_listening_start();
    }
    else
    {
        /* Device already provisioned: fetch device address from
        ** persistent storage.
        */

        uint16_t device_address = mesh_device_get_address();

        #ifdef DEBUG
        NRF_LOG_INFO("Device address: 0x%x!", device_address);
        #endif /* DEBUG */

        // Set internal addresses of model instances.
        mesh_models_set_addresses(device_address);
    }

    revision_t      revision = { .unmap = REV };

    // Create Mesh Bridge container instance.
    container_t*    container;
    container = Luos_CreateContainer(MeshBridge_MsgHandler,
                                     MESH_BRIDGE_TYPE,
                                     MESH_BRIDGE_ALIAS, revision);

    // Set internal container instance for message sending.
    s_mesh_bridge_instance  = container;

    // Set internal container instance for Ext-RTB complete message.
    app_luos_rtb_model_container_set(container);
}

void MeshBridge_Loop(void)
{}

static void MeshBridge_MsgHandler(container_t* container, msg_t* msg)
{
    // Start response message creation.
    msg_t response;
    memset(&response, 0, sizeof(msg_t));
    response.header.target_mode = ID;
    response.header.target      = msg->header.source;
    switch(msg->header.cmd)
    {
    case MESH_BRIDGE_EXT_RTB_CMD:
        // Engage Ext-RTB procedure.
        app_luos_rtb_model_engage_ext_rtb(msg->header.source,
                                          msg->header.target);

        // No answer to send: return.
        return;

    case MESH_BRIDGE_PRINT_INTERNAL_TABLES:
        // Print internal tables.
        local_container_table_print();
        remote_container_table_print();

        // No answer to send: return.
        return;

    case MESH_BRIDGE_CLEAR_INTERNAL_TABLES:
        // Clear internal tables.
        local_container_table_clear();
        remote_container_table_clear();

        response.header.cmd     = MESH_BRIDGE_INTERNAL_TABLES_CLEARED;

        break;

    case MESH_BRIDGE_FILL_LOCAL_CONTAINER_TABLE:
    {
        // Fill local container table.
        uint16_t nb_local_containers = local_container_table_fill();

        // Answer with number of local entries.
        response.header.cmd     = MESH_BRIDGE_LOCAL_CONTAINER_TABLE_FILLED;
        response.header.size    = sizeof(uint16_t);
        memcpy(response.data, &nb_local_containers, sizeof(uint16_t));
    }
        break;

    case MESH_BRIDGE_UPDATE_INTERNAL_TABLES:
    {
        // Fetch message payload.
        uint16_t dtx_container_id;
        memcpy(&dtx_container_id, msg->data, sizeof(uint16_t));

        // Update internal tables.
        local_container_table_update_local_ids(dtx_container_id);
        remote_container_table_update_local_ids(dtx_container_id);

        response.header.cmd     = MESH_BRIDGE_INTERNAL_TABLES_UPDATED;
    }
        break;

    default:
        // Nothing to do: return.
        return;
    }

    // Send answer.
    Luos_SendMsg(s_mesh_bridge_instance, &response);
}
