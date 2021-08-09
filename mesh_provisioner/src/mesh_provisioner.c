#include "mesh_provisioner.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>            // bool

// NRF
#include "nrf_log.h"            // NRF_LOG_INFO

// LUOS
#include "luos.h"               /* container_t, Luos_CreateContainer,
                                ** msg_t
                                */

// CUSTOM
#include "luos_mesh_common.h"   // mesh_start
#include "mesh_init.h"          // mesh_init
#include "network_ctx.h"        // network_ctx_init, g_network_ctx
#include "provisioning.h"       // mesh_*, prov*

/*      STATIC/GLOBAL VARIABLES & CONSTANTS                         */

#ifndef REV
#define REV {0,0,1}
#endif

/*      CALLBACKS                                                   */

// Does nothing as changes are triggered by a boolean toggle.
static void MeshProvisioner_MsgHandler(container_t* container,
                                       msg_t* msg);

void MeshProvisioner_Init(void)
{
    mesh_init();
    provisioning_init();
    network_ctx_init();
    NRF_LOG_INFO("Netkey handle: 0x%x; Appkey handle: 0x%x; Self devkey handle: 0x%x!",
                 g_network_ctx.netkey_handle,
                 g_network_ctx.appkey_handle,
                 g_network_ctx.self_devkey_handle);

    prov_conf_init();
    mesh_start();

    revision_t revision = { .unmap = REV };

    Luos_CreateContainer(MeshProvisioner_MsgHandler,
                         MESH_PROVISIONER_TYPE,
                         MESH_PROVISIONER_ALIAS,
                         revision);
}

void MeshProvisioner_Loop(void)
{}

static void MeshProvisioner_MsgHandler(container_t* container,
                                       msg_t* msg)
{
    switch (msg->header.cmd)
    {
    case IO_STATE:
    {
        bool req = msg->data[0];

        if (req)
        {
            prov_scan_start();
        }
        else
        {
            prov_scan_stop();
        }
    }
        break;

    default:
        break;
    }
}
