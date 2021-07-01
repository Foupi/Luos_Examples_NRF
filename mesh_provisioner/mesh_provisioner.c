#include "mesh_provisioner.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>    // bool

// NRF
#include "nrf_log.h"    // NRF_LOG_INFO

// LUOS
#include "luos.h"       // container_t, Luos_CreateContainer, msg_t

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
    // FIXME Initialize Mesh stack.

    // FIXME Initialize provisioning module.

    // FIXME Start Mesh stack.

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
            // FIXME Start scanning.
        }
        else
        {
            // FIXME Stop scanning.
        }

        s_prov_scanning_state = g_prov_scan_req;
    }
}

static void MeshProvisioner_MsgHandler(container_t* container,
                                       msg_t* msg)
{
}
