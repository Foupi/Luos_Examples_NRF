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

// Describes if scanning was asked by the user.
bool                g_prov_scan_req         = false;

// Describes if the container is currently scanning for devices.
static bool         s_prov_scanning_state   = false;

/*      CALLBACKS                                                   */

// Does nothing as changes are triggered by a boolean toggle.
static void MeshProvisioner_MsgHandler(container_t* container,
                                       msg_t* msg);

void MeshProvisioner_Init(void)
{
    mesh_init();

    NRF_LOG_INFO("Mesh stack initialized!");

    provisioning_init();

    NRF_LOG_INFO("Provisioning module initialized!");

    network_ctx_init();

    NRF_LOG_INFO("Netkey handle: 0x%x; Appkey handle: 0x%x; Self devkey handle: 0x%x!",
                 g_network_ctx.netkey_handle,
                 g_network_ctx.appkey_handle,
                 g_network_ctx.self_devkey_handle);

    prov_conf_init();

    NRF_LOG_INFO("Provisioning configuration initialized!");

    mesh_start();

    NRF_LOG_INFO("Mesh stack started!");

    revision_t revision = { .unmap = REV };

    Luos_CreateContainer(MeshProvisioner_MsgHandler,
                         MESH_PROVISIONER_TYPE,
                         MESH_PROVISIONER_ALIAS,
                         revision);
}

void MeshProvisioner_Loop(void)
{
    if (g_prov_scan_req != s_prov_scanning_state)
    {
        /* Difference between these variables means a command was
        ** received.
        */

        if (g_prov_scan_req)
        {
            prov_scan_start();
        }
        else
        {
            prov_scan_stop();
        }

        s_prov_scanning_state = g_prov_scan_req;
    }
}

static void MeshProvisioner_MsgHandler(container_t* container,
                                       msg_t* msg)
{
}
