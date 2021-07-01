#include "mesh_provisioner.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>    // bool

// LUOS
#include "luos.h"       // container_t, Luos_CreateContainer, msg_t

/*      STATIC VARIABLES & CONSTANTS                                */

#ifndef REV
#define REV {0,0,1}
#endif

// Describes if the container is currently scanning for devices.
static bool         s_prov_scanning_state   = false;

// Describes if scanning was asked by the user.
static bool         s_prov_scanning_asked   = false;

/*      CALLBACKS                                                   */

/* Ask pub:     Send the current scanning state as an IO_STATE message.
** IO state:    Sets the current scanning state as the received one.
*/
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
    if (s_prov_scanning_asked != s_prov_scanning_state)
    {
        /* Difference between these variables means a command was
        ** received.
        */

        if (s_prov_scanning_asked)
        {
            // FIXME Start scanning.
        }
        else
        {
            // FIXME Stop scanning.
        }

        s_prov_scanning_state = s_prov_scanning_asked;
    }
}

static void MeshProvisioner_MsgHandler(container_t* container,
                                       msg_t* msg)
{
    switch (msg->header.cmd)
    {
    case ASK_PUB_CMD:
        // FIXME Build IO_STATE response with current scanning state.
        break;
    case IO_STATE:
        // FIXME Set scanning asked state.
        break;
    default:
        break;
    }
}
