#include "app_luos_msg_model.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>                 // uint16_t
#include <string.h>                 // memset

// NRF
#include "nrf_log.h"                // NRF_LOG_INFO

// LUOS
#include "robus_struct.h"           // msg_t
#include "routing_table.h"          // routing_table_t

// CUSTOM
#include "local_container_table.h"  // local_container_table_*
#include "luos_msg_model.h"         // luos_msg_model_*
#include "remote_container_table.h" // remote_container_table_*

/*      STATIC VARIABLES & CONSTANTS                                */

// Static Luos MSG model instance.
static luos_msg_model_t s_msg_model;

/*      CALLBACKS                                                   */

// FIXME Logs message.
static void msg_model_set_cb(uint16_t src_addr, const msg_t* recv_msg);

void app_luos_msg_model_init(void)
{
    luos_msg_model_init_params_t params;
    memset(&params, 0 , sizeof(luos_msg_model_init_params_t));
    params.set_cb   = msg_model_set_cb;

    luos_msg_model_init(&s_msg_model, &params);
}

void app_luos_msg_model_address_set(uint16_t device_address)
{
    luos_msg_model_set_address(&s_msg_model, device_address);
}

void app_luos_msg_model_send_msg(msg_t* msg)
{
    uint16_t            src_id          = msg->header.source;
    uint16_t            target_id       = msg->header.target;

    NRF_LOG_INFO("Message received by container %u, from container %u!",
                 target_id, src_id);

    routing_table_t*    exposed_entry   = local_container_table_get_entry_from_local_id(src_id);
    if (exposed_entry == NULL)
    {
        NRF_LOG_INFO("Local container entry not found!");
        return;
    }

    uint16_t            new_src         = exposed_entry->id;

    remote_container_t* remote_entry    = remote_container_table_get_entry_from_local_id(target_id);
    if (remote_entry == NULL)
    {
        NRF_LOG_INFO("Remote container entry not found!");
        return;
    }

    uint16_t            node_addr       = remote_entry->node_addr;
    uint16_t            remote_id       = remote_entry->remote_rtb_entry.id;

    NRF_LOG_INFO("Sending message to container %u on node 0x%x with source ID %u!",
                 remote_id, node_addr, new_src);

    msg->header.source  = new_src;
    msg->header.target  = remote_id;

    luos_msg_model_set(&s_msg_model, node_addr, msg);
}

static void msg_model_set_cb(uint16_t src_addr, const msg_t* recv_msg)
{
    NRF_LOG_INFO("Luos MSG SET command received from node 0x%x!",
                 src_addr);

    header_t    recv_header = recv_msg->header;
    uint16_t    data_size   = recv_header.size;

    NRF_LOG_INFO("Received message target: %u (mode 0x%x), from container %u!",
                 recv_header.target, recv_header.target_mode,
                 recv_header.source);
    NRF_LOG_INFO("Message contains the command 0x%x, of size %u!",
                 recv_header.cmd, data_size);
    if (data_size > 0)
    {
        NRF_LOG_HEXDUMP_INFO(recv_msg->data, data_size);
    }
}
