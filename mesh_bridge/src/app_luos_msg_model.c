#include "app_luos_msg_model.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>         // uint16_t
#include <string.h>         // memset

// NRF
#include "nrf_log.h"        // NRF_LOG_INFO

// LUOS
#include "robus_struct.h"   // msg_t

// CUSTOM
#include "luos_msg_model.h" // luos_msg_model_*

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
    NRF_LOG_INFO("Device address is 0x%x!", device_address);
    luos_msg_model_set_address(&s_msg_model, device_address);
}

void app_luos_msg_model_send_msg(msg_t* msg)
{
    // FIXME Translate addresses and find node address.

    luos_msg_model_set(&s_msg_model, 0x0003, msg);
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
