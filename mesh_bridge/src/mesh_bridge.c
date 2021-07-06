#include "mesh_bridge.h"

/*      INCLUDES                                                    */

// NRF
#include "nrf_log.h"    // NRF_LOG_INFO

// LUOS
#include "luos.h"       // container_t, Luos_CreateContainer, msg_t

/*      STATIC/GLOBAL VARIABLES & CONSTANTS                         */

#ifndef REV
#define REV {0,0,1}
#endif

/*      CALLBACKS                                                   */

// FIXME Does nothing for now.
static void MeshBridge_MsgHandler(container_t* container, msg_t* msg);

void MeshBridge_Init(void)
{
    revision_t revision = { .unmap = REV };

    Luos_CreateContainer(MeshBridge_MsgHandler, MESH_BRIDGE_TYPE,
                         MESH_BRIDGE_ALIAS, revision);
}

void MeshBridge_Loop(void)
{}

static void MeshBridge_MsgHandler(container_t* container, msg_t* msg)
{}
