#include "mesh_bridge.h"

/*      INCLUDES                                                    */

// NRF
#include "nrf_log.h"            // NRF_LOG_INFO

// LUOS
#include "luos.h"               /* container_t, Luos_CreateContainer,
                                ** msg_t
                                */

// CUSTOM
#include "app_luos_msg_model.h" // app_luos_msg_model_send_msg
#include "app_luos_rtb_model.h" // app_luos_rtb_model_get
#include "local_container_table.h"  // local_container_table_*
#include "luos_mesh_common.h"   // mesh_start
#include "mesh_init.h"          // mesh_init
#include "provisioning.h"       /* provisioning_init,
                                ** persistent_conf_init,
                                ** prov_listening_start
                                */

/*      STATIC/GLOBAL VARIABLES & CONSTANTS                         */

#ifndef REV
#define REV {0,0,1}
#endif

/*      CALLBACKS                                                   */

// Starts the Ext-RTB procedure on the right command.
static void MeshBridge_MsgHandler(container_t* container, msg_t* msg);

void MeshBridge_Init(void)
{
    mesh_init();
    provisioning_init();
    persistent_conf_init();
    mesh_start();
    prov_listening_start();

    revision_t      revision = { .unmap = REV };

    container_t*    container;
    container = Luos_CreateContainer(MeshBridge_MsgHandler,
                                     MESH_BRIDGE_TYPE,
                                     MESH_BRIDGE_ALIAS, revision);

    app_luos_rtb_model_container_set(container);
}

void MeshBridge_Loop(void)
{}

static void MeshBridge_MsgHandler(container_t* container, msg_t* msg)
{
    switch(msg->header.cmd)
    {
    case MESH_BRIDGE_EXT_RTB_CMD:
        app_luos_rtb_model_engage_ext_rtb(msg->header.source,
                                          msg->header.target);
        break;

    case MESH_BRIDGE_PRINT_INTERNAL_TABLES:
        local_container_table_print();
        // FIXME Print remote container table.
        break;

    case MESH_BRIDGE_RESET_INTERNAL_TABLES:
        NRF_LOG_INFO("Received request to reset internal tables!");
        break;

    case MESH_BRIDGE_UPDATE_INTERNAL_TABLES:
        NRF_LOG_INFO("Received request to update entries of internal tables!");
        break;

    default:
        break;
    }
}
