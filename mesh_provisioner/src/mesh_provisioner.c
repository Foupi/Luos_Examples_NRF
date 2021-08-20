#include "mesh_provisioner.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>            // bool
#include <string.h>             // memset

// LUOS
#include "luos.h"               /* container_t, Luos_CreateContainer,
                                ** msg_t
                                */

// CUSTOM
#include "luos_mesh_common.h"   // mesh_start
#include "mesh_init.h"          // mesh_init
#include "network_ctx.h"        // network_ctx_init, g_network_ctx
#include "node_config.h"        // node_config_container_set
#include "provisioning.h"       // mesh_*, prov*

// NRF
#ifdef DEBUG
#include "nrf_log.h"            // NRF_LOG_INFO
#endif /* DEBUG */

/*      STATIC/GLOBAL VARIABLES & CONSTANTS                         */

#ifndef REV
#define REV {0,0,1}
#endif

/*      CALLBACKS                                                   */

// Switches scanning state depending on the given IO state.
static void MeshProvisioner_MsgHandler(container_t* container,
                                       msg_t* msg);

void MeshProvisioner_Init(void)
{
    mesh_init();
    provisioning_init();
    network_ctx_init();

    #ifdef DEBUG
    NRF_LOG_INFO("Netkey handle: 0x%x; Appkey handle: 0x%x; Self devkey handle: 0x%x!",
                 g_network_ctx.netkey_handle,
                 g_network_ctx.appkey_handle,
                 g_network_ctx.self_devkey_handle);
    #endif /* DEBUG */

    prov_conf_init();
    mesh_start();

    revision_t revision = { .unmap = REV };

    container_t* container;
    container   = Luos_CreateContainer(MeshProvisioner_MsgHandler,
                                       MESH_PROVISIONER_TYPE,
                                       MESH_PROVISIONER_ALIAS,
                                       revision);
    node_config_container_set(container);
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

        if (!req)
        {
            prov_scan_stop();
            break;
        }

        bool    can_scan    = prov_scan_start();
        if (!can_scan)
        {
            // Max nb nodes reached: signal it to source.
            msg_t   cannot_scan;
            memset(&cannot_scan, 0, sizeof(msg_t));
            cannot_scan.header.target_mode  = ID;
            cannot_scan.header.target       = msg->header.source;
            cannot_scan.header.cmd          = IO_STATE;
            cannot_scan.header.size         = sizeof(uint8_t);
            cannot_scan.data[0]             = 0x00; // False.

            // Send msg with instance which received the message.
            Luos_SendMsg(container, &cannot_scan);
        }
    }
        break;

    default:
        break;
    }
}
