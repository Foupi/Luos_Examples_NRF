#include "app_luos_msg_model.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>                 // uint16_t
#include <string.h>                 // memset

// LUOS
#include "robus_struct.h"           // msg_t
#include "routing_table.h"          // routing_table_t

// CUSTOM
#include "local_container_table.h"  // local_container_table_*
#include "luos_mesh_msg.h"          // luos_mesh_msg_t
#include "luos_msg_model.h"         // luos_msg_model_*
#include "mesh_msg_queue_manager.h" // tx_queue_*, luos_mesh_msg_prepare
#include "remote_container_table.h" // remote_container_table_*

// NRF
#ifdef DEBUG
#include "nrf_log.h"                // NRF_LOG_INFO
#endif /* DEBUG */

/*      STATIC VARIABLES & CONSTANTS                                */

// Static Luos MSG model instance.
static luos_msg_model_t s_msg_model;

/*      CALLBACKS                                                   */

// Prepares the queue element and enqueues it.
static void msg_model_set_send(luos_msg_model_t* instance,
                               const luos_msg_model_set_t* set_cmd);

// Translates received coordinates and sends the message.
static void msg_model_set_cb(uint16_t src_addr,
                             const luos_mesh_msg_t* recv_msg);

void app_luos_msg_model_init(void)
{
    luos_msg_model_init_params_t params;
    memset(&params, 0 , sizeof(luos_msg_model_init_params_t));
    params.set_send = msg_model_set_send;
    params.set_cb   = msg_model_set_cb;

    luos_msg_model_init(&s_msg_model, &params);
}

void app_luos_msg_model_address_set(uint16_t device_address)
{
    luos_msg_model_set_address(&s_msg_model, device_address);
}

void app_luos_msg_model_send_msg(const msg_t* msg)
{
    uint16_t            src_id          = msg->header.source;
    uint16_t            target_id       = msg->header.target;

    #ifdef DEBUG
    NRF_LOG_INFO("Message received by container %u, from container %u!",
                 target_id, src_id);
    #endif /* DEBUG */

    routing_table_t*    exposed_entry   = local_container_table_get_entry_from_local_id(src_id);
    if (exposed_entry == NULL)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Local container entry not found!");
        #endif /* DEBUG */

        return;
    }

    uint16_t            new_src         = exposed_entry->id;

    remote_container_t* remote_entry    = remote_container_table_get_entry_from_local_id(target_id);
    if (remote_entry == NULL)
    {

        #ifdef DEBUG
        NRF_LOG_INFO("Remote container entry not found!");
        #endif /* DEBUG */

        return;
    }

    uint16_t            node_addr       = remote_entry->node_addr;
    uint16_t            remote_id       = remote_entry->remote_rtb_entry.id;

    #ifdef DEBUG
    NRF_LOG_INFO("Sending message to container %u on node 0x%x with source ID %u!",
                 remote_id, node_addr, new_src);
    #endif /* DEBUG */

    uint16_t            data_size       = msg->header.size;

    luos_mesh_msg_t mesh_msg;
    memset(&mesh_msg, 0, sizeof(luos_mesh_msg_t));
    mesh_msg.header.target      = remote_id;
    mesh_msg.header.target_mode = ID; // FIXME Only supported mode.
    mesh_msg.header.source      = new_src;
    mesh_msg.header.cmd         = msg->header.cmd;
    mesh_msg.header.size        = data_size;
    memcpy(mesh_msg.data, msg->data, LUOS_MESH_MSG_MAX_DATA_SIZE);

    luos_msg_model_set(&s_msg_model, node_addr, &mesh_msg);
}

static void msg_model_set_send(luos_msg_model_t* instance,
                               const luos_msg_model_set_t* set_cmd)
{
    tx_queue_luos_msg_model_elm_t   msg_model_msg;
    memset(&msg_model_msg, 0, sizeof(tx_queue_luos_msg_model_elm_t));
    msg_model_msg.cmd                   = TX_QUEUE_CMD_SET;
    memcpy(&(msg_model_msg.content.set), set_cmd,
           sizeof(luos_msg_model_set_t));

    tx_queue_elm_t                  new_msg;
    memset(&new_msg, 0, sizeof(tx_queue_elm_t));
    new_msg.model                       = TX_QUEUE_MODEL_LUOS_MSG;
    new_msg.model_handle                = instance->handle;
    memcpy(&(new_msg.content.luos_msg_model_msg), &msg_model_msg,
           sizeof(tx_queue_luos_msg_model_elm_t));

    luos_mesh_msg_prepare(&new_msg);
}

static void msg_model_set_cb(uint16_t src_addr,
                             const luos_mesh_msg_t* recv_msg)
{
    luos_mesh_header_t  recv_header = recv_msg->header;
    uint16_t            data_size   = recv_header.size;
    uint16_t            msg_src     = recv_header.source;
    uint16_t            msg_dst     = recv_header.target;

    #ifdef DEBUG
    NRF_LOG_INFO("Command 0x%x for container %u received from container %u on node 0x%x!",
                 recv_header.cmd, msg_dst, msg_src, src_addr);
    if (data_size > 0)
    {
        NRF_LOG_INFO("Message payload size is %u bytes:");
        NRF_LOG_HEXDUMP_INFO(recv_msg->data, data_size);
    }
    #endif /* DEBUG */

    remote_container_t* remote_entry   = remote_container_table_get_entry_from_addr_and_remote_id(src_addr, msg_src);
    if (remote_entry == NULL)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Remote container entry not found!");
        #endif /* DEBUG */

        return;
    }

    local_container_t*  local_entry     = local_container_table_get_entry_from_exposed_id(msg_dst);
    if (local_entry == NULL)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Local container entry not found!");
        #endif /* DEBUG */

        return;
    }

    uint16_t    local_src   = remote_entry->local_id;
    uint16_t    local_dst   = local_entry->local_id;

    #ifdef DEBUG
    NRF_LOG_INFO("Local IDs retrieved! source: %u; target: %u!",
                 local_src, local_dst);
    #endif /* DEBUG */

    msg_t local_msg;
    memset(&local_msg, 0, sizeof(local_msg));
    local_msg.header.target_mode    = ID;
    local_msg.header.target         = local_dst;
    local_msg.header.cmd            = recv_msg->header.cmd;
    if (data_size > 0)
    {
        local_msg.header.size       = data_size;
        memcpy(local_msg.data, recv_msg->data, data_size);
    }

    Luos_SendMsg(remote_entry->local_instance, &local_msg);
}
