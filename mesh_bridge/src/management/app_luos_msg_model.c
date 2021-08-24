#include "app_luos_msg_model.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>                 // uint16_t
#include <string.h>                 // memset

// LUOS
#include "luos_utils.h"             // LUOS_ASSERT
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

/*      STATIC FUNCTIONS                                            */

// Translates the given Luos message into the given lightweight message.
static void msg_to_luos_mesh_msg(const msg_t* msg,
                                 luos_mesh_msg_t* mesh_msg,
                                 uint16_t source, uint16_t target);

// Translates the given lightweight message into the given Luos message.
static void luos_mesh_msg_to_msg(const luos_mesh_msg_t* mesh_msg,
                                 msg_t* msg, uint16_t target);

/*      CALLBACKS                                                   */

// Prepares the queue element and enqueues it.
static void msg_model_set_send(luos_msg_model_t* instance,
                               const luos_msg_model_set_t* set_cmd);

// Translates received coordinates and sends the message.
static void msg_model_set_cb(uint16_t src_addr,
                             const luos_mesh_msg_t* recv_msg);

void app_luos_msg_model_init(void)
{
    // Parameters to initialize the internal Luos MSG model.
    luos_msg_model_init_params_t params;
    memset(&params, 0 , sizeof(luos_msg_model_init_params_t));
    params.set_send = msg_model_set_send;
    params.set_cb   = msg_model_set_cb;

    // Initialize the model instance.
    luos_msg_model_init(&s_msg_model, &params);
}

void app_luos_msg_model_address_set(uint16_t device_address)
{
    luos_msg_model_set_address(&s_msg_model, device_address);
}

void app_luos_msg_model_send_msg(const msg_t* msg)
{
    // Check parameter.
    LUOS_ASSERT(msg != NULL);

    // Local source and target IDs.
    uint16_t            src_id          = msg->header.source;
    uint16_t            target_id       = msg->header.target;

    #ifdef DEBUG
    NRF_LOG_INFO("Message received by container %u, from container %u!",
                 target_id, src_id);
    #endif /* DEBUG */

    // Exposed entry corresponding to the local source container.
    routing_table_t*    exposed_entry;
    exposed_entry   = local_container_table_get_entry_from_local_id(src_id);

    // Only send messages to remote containers from exposed containers.
    LUOS_ASSERT(exposed_entry != NULL);

    // Exposed ID of the source container.
    uint16_t            exposed_src     = exposed_entry->id;

    // Remote container table entry corresponding to the message target.
    remote_container_t* remote_entry;
    remote_entry    = remote_container_table_get_entry_from_local_id(target_id);

    // Check result.
    LUOS_ASSERT(remote_entry != NULL);

    // Unicast address of the network hosting the target container.
    uint16_t            node_addr       = remote_entry->node_addr;

    // ID of the target container as exposed by its network.
    uint16_t            remote_id;
    remote_id       = remote_entry->remote_rtb_entry.id;

    #ifdef DEBUG
    NRF_LOG_INFO("Sending message to container %u on node 0x%x with source ID %u!",
                 remote_id, node_addr, exposed_src);
    #endif /* DEBUG */

    // Translate the Luos message into a lightweight Luos Mesh message.
    luos_mesh_msg_t mesh_msg;
    msg_to_luos_mesh_msg(msg, &mesh_msg, exposed_src, remote_id);

    // Send message as Luos MSG SET command.
    luos_msg_model_set(&s_msg_model, node_addr, &mesh_msg);
}

static void msg_to_luos_mesh_msg(const msg_t* msg,
                                 luos_mesh_msg_t* mesh_msg,
                                 uint16_t source, uint16_t target)
{
    // Check parameters.
    LUOS_ASSERT(msg != NULL);
    LUOS_ASSERT(mesh_msg != NULL);

    uint8_t     target_mode = msg->header.target_mode;
    uint16_t    data_size   = msg->header.size;

    LUOS_ASSERT((target_mode == ID) || (target_mode == IDACK));
    LUOS_ASSERT(data_size <= LUOS_MESH_MSG_MAX_DATA_SIZE);
    LUOS_ASSERT(source <= ((1 << 4) - 1));
    LUOS_ASSERT(target <= ((1 << 4) - 1));

    memset(mesh_msg, 0, sizeof(luos_mesh_msg_t));
    // Fill target and source with parameters.
    mesh_msg->header.target         = target;
    mesh_msg->header.source         = source;
    // Copy msg header.
    mesh_msg->header.target_mode    = target_mode;
    mesh_msg->header.cmd            = msg->header.cmd;
    mesh_msg->header.size           = data_size;

    if (data_size > 0)
    {
        // Copy message payload.
        memcpy(mesh_msg->data, msg->data, data_size);
    }
}

static void luos_mesh_msg_to_msg(const luos_mesh_msg_t* mesh_msg,
                                 msg_t* msg, uint16_t target)
{
    // Check parameters.
    LUOS_ASSERT(mesh_msg != NULL);
    LUOS_ASSERT(msg != NULL);

    uint8_t data_size   = mesh_msg->header.size;

    memset(msg, 0, sizeof(msg_t));
    /* Fill target with parameter. No need for source: autofilled at
    ** send.
    */
    msg->header.target      = target;
    // Copy Mesh msg header.
    msg->header.target_mode = mesh_msg->header.target_mode;
    msg->header.cmd         = mesh_msg->header.cmd;
    msg->header.size        = data_size;

    if (data_size > 0)
    {
        // Copy message payload.
        memcpy(msg->data, mesh_msg->data, data_size);
    }
}

static void msg_model_set_send(luos_msg_model_t* instance,
                               const luos_msg_model_set_t* set_cmd)
{
    // Check parameters.
    LUOS_ASSERT(instance != NULL);
    LUOS_ASSERT(set_cmd != NULL);

    // Create Luos MSG SET TX queue message.
    tx_queue_luos_msg_model_elm_t   msg_model_msg;
    memset(&msg_model_msg, 0, sizeof(tx_queue_luos_msg_model_elm_t));
    msg_model_msg.cmd                   = TX_QUEUE_CMD_SET;
    memcpy(&(msg_model_msg.content.set), set_cmd,
           sizeof(luos_msg_model_set_t));

    // Encapsulate message in TX queue element.
    tx_queue_elm_t                  new_msg;
    memset(&new_msg, 0, sizeof(tx_queue_elm_t));
    new_msg.model                       = TX_QUEUE_MODEL_LUOS_MSG;
    new_msg.model_handle                = instance->handle;
    memcpy(&(new_msg.content.luos_msg_model_msg), &msg_model_msg,
           sizeof(tx_queue_luos_msg_model_elm_t));

    // Enqueue given element.
    luos_mesh_msg_prepare(&new_msg);
}

static void msg_model_set_cb(uint16_t src_addr,
                             const luos_mesh_msg_t* recv_msg)
{
    // Check parameter.
    LUOS_ASSERT(recv_msg != NULL);

    // Received message header.
    luos_mesh_header_t  recv_header = recv_msg->header;

    // Exposed source and destination IDs.
    uint16_t            msg_src     = recv_header.source;
    uint16_t            msg_dst     = recv_header.target;

    #ifdef DEBUG
    uint16_t            data_size   = recv_header.size;

    NRF_LOG_INFO("Command 0x%x for container %u received from container %u on node 0x%x!",
                 recv_header.cmd, msg_dst, msg_src, src_addr);
    if (data_size > 0)
    {
        NRF_LOG_INFO("Message payload size is %u bytes:", data_size);
        NRF_LOG_HEXDUMP_INFO(recv_msg->data, data_size);
    }
    #endif /* DEBUG */

    /* Remote container table entry corresponding to the source node
    ** unicast address and exposed ID.
    */
    remote_container_t* remote_entry;
    remote_entry    = remote_container_table_get_entry_from_addr_and_remote_id(
                        src_addr, msg_src
                      );

    // Check result.
    LUOS_ASSERT(remote_entry != NULL);

    /* Local container table entry corresponding to the exposed target
    ** ID.
    */
    local_container_t*  local_entry;
    local_entry     = local_container_table_get_entry_from_exposed_id(msg_dst);

    // Check result.
    LUOS_ASSERT(local_entry != NULL);

    // Local target ID.
    uint16_t            local_dst   = local_entry->local_id;

    #ifdef DEBUG
    // Local source ID.
    uint16_t            local_src   = remote_entry->local_id;

    NRF_LOG_INFO("Local IDs retrieved! source: %u; target: %u!",
                 local_src, local_dst);
    #endif /* DEBUG */

    // Translate the lightweight Luos Mesh message into a Luos message.
    msg_t               local_msg;
    luos_mesh_msg_to_msg(recv_msg, &local_msg, local_dst);

    /* Send message in network through local instance of remote
    ** container.
    */
    Luos_SendMsg(remote_entry->local_instance, &local_msg);
}
