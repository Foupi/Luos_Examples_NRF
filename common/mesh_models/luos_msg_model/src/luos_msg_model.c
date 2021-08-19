#include "luos_msg_model.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>                 // uint16_t
#include <string.h>                 // memset

// NRF
#include "sdk_errors.h"             // ret_code_t

// NRF APPS
#include "app_error.h"              // APP_ERROR_CHECK

// MESH SDK
#include "access.h"                 // access_*
#include "access_config.h"          // access_model_subscription_list_alloc

// CUSTOM
#include "luos_mesh_msg.h"          // luos_mesh_msg_t
#include "luos_msg_model_common.h"  // LUOS_MSG_MODEL_*

#ifdef DEBUG
#include "nrf_log.h"                // NRF_LOG_INFO
#endif /* DEBUG */

/*      STATIC VARIABLES & CONSTANTS                                */

// Index of the current transaction.
static uint16_t s_curr_transaction_id   = 0;

/*      CALLBACKS                                                   */

// Manage a Luos MSG SET command.
static void luos_msg_model_set_cb(access_model_handle_t handle,
                                  const access_message_rx_t* msg,
                                  void* arg);

// Manage a Luos MSG STATUS message.
static void luos_msg_model_status_cb(access_model_handle_t handle,
                                     const access_message_rx_t* msg,
                                     void* arg);

/*      INITIALIZATIONS                                             */

// Luos MSG model opcode handlers table.
static const access_opcode_handler_t    LUOS_MSG_MODEL_OPCODE_HANDLERS[]    =
{
    {
        LUOS_MSG_MODEL_SET_ACCESS_OPCODE,
        luos_msg_model_set_cb,
    },
    {
        LUOS_MSG_MODEL_STATUS_ACCESS_OPCODE,
        luos_msg_model_status_cb,
    },
};

void luos_msg_model_init(luos_msg_model_t* instance,
                         const luos_msg_model_init_params_t* params)
{
    memset(instance, 0, sizeof(luos_msg_model_t));
    instance->set_send              = params->set_send;
    instance->set_cb                = params->set_cb;
    instance->status_cb             = params->status_cb;

    instance->element_address       = LUOS_MSG_MODEL_DEFAULT_ELM_ADDR;

    ret_code_t                  err_code;
    access_model_id_t           luos_msg_model_id   = LUOS_MSG_MODEL_ACCESS_ID;

    access_model_add_params_t   add_params;
    memset(&add_params, 0, sizeof(access_model_add_params_t));
    add_params.model_id             = luos_msg_model_id;
    add_params.element_index        = LUOS_MSG_MODEL_ELM_IDX;
    add_params.p_opcode_handlers    = LUOS_MSG_MODEL_OPCODE_HANDLERS;
    add_params.opcode_count         = sizeof(LUOS_MSG_MODEL_OPCODE_HANDLERS) / sizeof(access_opcode_handler_t);
    add_params.p_args               = instance;

    err_code    = access_model_add(&add_params, &(instance->handle));
    APP_ERROR_CHECK(err_code);

    err_code = access_model_subscription_list_alloc(instance->handle);
    APP_ERROR_CHECK(err_code);
}

void luos_msg_model_set_address(luos_msg_model_t* instance,
                                uint16_t device_address)
{
    instance->element_address   = device_address + LUOS_MSG_MODEL_ELM_IDX;
}

void luos_msg_model_set(luos_msg_model_t* instance, uint16_t dst_addr,
                        const luos_mesh_msg_t* msg)
{
    if (instance->set_send == NULL)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("No user-defined function to sent SET requests!");
        #endif /* DEBUG */

        return;
    }

    s_curr_transaction_id++;

    luos_msg_model_set_t            set_cmd;
    set_cmd.transaction_id          = s_curr_transaction_id;
    set_cmd.dst_addr                = dst_addr;
    memcpy(&(set_cmd.msg), msg, sizeof(luos_mesh_msg_t));

    instance->set_send(instance, &set_cmd);
}

static void luos_msg_model_set_cb(access_model_handle_t handle,
                                  const access_message_rx_t* msg,
                                  void* arg)
{
    luos_msg_model_t*           instance  = (luos_msg_model_t*)arg;
    uint16_t                    src_addr    = msg->meta_data.src.value;

    if (instance->element_address == LUOS_MSG_MODEL_DEFAULT_ELM_ADDR
        || src_addr == instance->element_address)
    {
        // Either model is not ready, or this is a localhost message.
        return;
    }

    const luos_msg_model_set_t* set_cmd = (luos_msg_model_set_t*)(msg->p_data);

    if (set_cmd->transaction_id <= s_curr_transaction_id)
    {
        // Transaction either already occured or is currently occuring.
        return;
    }

    s_curr_transaction_id       = set_cmd->transaction_id;

    if (set_cmd->dst_addr != instance->element_address)
    {
        // Model instance is not the message destination.
        return;
    }

    const   luos_mesh_msg_t*    luos_msg    = &(set_cmd->msg);

    if (instance->set_cb == NULL)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("No user-defined callback for SET commands on Luos MSG model!");
        #endif /* DEBUG */
    }
    else
    {
        instance->set_cb(src_addr, luos_msg);
    }

    // FIXME Send status reply.
}

static void luos_msg_model_status_cb(access_model_handle_t handle,
                                     const access_message_rx_t* msg,
                                     void* arg)
{
    #ifdef DEBUG
    NRF_LOG_INFO("Luos MSG STATUS received!");
    #endif /* DEBUG */
}
