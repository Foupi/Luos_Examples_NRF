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
container_t* s_mesh_bridge_instance;

/*      CALLBACKS                                                   */

// Starts the Ext-RTB procedure on the right command.
static void MeshBridge_MsgHandler(container_t* container, msg_t* msg);

void MeshBridge_Init(void)
{
    mesh_init();
    provisioning_init();
    mesh_start();

    if (!g_device_provisioned)
    {
        prov_listening_start();
    }
    else
    {
        uint16_t device_address = mesh_device_get_address();

        #ifdef DEBUG
        NRF_LOG_INFO("Device address: 0x%x!", device_address);
        #endif /* DEBUG */

        mesh_models_set_addresses(device_address);
    }

    revision_t      revision = { .unmap = REV };

    container_t*    container;
    container = Luos_CreateContainer(MeshBridge_MsgHandler,
                                     MESH_BRIDGE_TYPE,
                                     MESH_BRIDGE_ALIAS, revision);

    s_mesh_bridge_instance  = container;
    app_luos_rtb_model_container_set(container);
}

void MeshBridge_Loop(void)
{}

static void MeshBridge_MsgHandler(container_t* container, msg_t* msg)
{
    msg_t response;
    memset(&response, 0, sizeof(msg_t));
    response.header.target_mode = ID;
    response.header.target      = msg->header.source;

    switch(msg->header.cmd)
    {
    case MESH_BRIDGE_EXT_RTB_CMD:
        app_luos_rtb_model_engage_ext_rtb(msg->header.source,
                                          msg->header.target);
        return;

    case MESH_BRIDGE_PRINT_INTERNAL_TABLES:
        local_container_table_print();
        remote_container_table_print();
        return;

    case MESH_BRIDGE_CLEAR_INTERNAL_TABLES:
        local_container_table_clear();
        remote_container_table_clear();

        response.header.cmd     = MESH_BRIDGE_INTERNAL_TABLES_CLEARED;

        break;

    case MESH_BRIDGE_FILL_LOCAL_CONTAINER_TABLE:
    {
        uint16_t nb_local_containers = local_container_table_fill();

        response.header.cmd     = MESH_BRIDGE_LOCAL_CONTAINER_TABLE_FILLED;
        response.header.size    = sizeof(uint16_t);
        memcpy(response.data, &nb_local_containers, sizeof(uint16_t));
    }
        break;

    case MESH_BRIDGE_UPDATE_INTERNAL_TABLES:
    {
        uint16_t dtx_container_id;
        memcpy(&dtx_container_id, msg->data, sizeof(uint16_t));

        local_container_table_update_local_ids(dtx_container_id);
        remote_container_table_update_local_ids(dtx_container_id);

        response.header.cmd     = MESH_BRIDGE_INTERNAL_TABLES_UPDATED;
    }
        break;

    default:
        return;
    }

    Luos_SendMsg(s_mesh_bridge_instance, &response);
}
