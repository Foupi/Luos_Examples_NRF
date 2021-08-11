#include "luos_msg_model.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>                 // uint16_t
#include <string.h>                 // memset

// NRF
#include "nrf_log.h"                // NRF_LOG_INFO
#include "sdk_errors.h"             // ret_code_t

// NRF APPS
#include "app_error.h"              // APP_ERROR_CHECK

// MESH SDK
#include "access.h"                 // access_*
#include "access_config.h"          // access_model_subscription_list_alloc

// LUOS
#include "robus_struct.h"           // msg_t

// CUSTOM
#include "luos_msg_model_common.h"  // LUOS_MSG_MODEL_*

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
                        const msg_t* msg)
{
    // FIXME Create and enqueue set request.
}

static void luos_msg_model_set_cb(access_model_handle_t handle,
                                  const access_message_rx_t* msg,
                                  void* arg)
{
    NRF_LOG_INFO("Luos MSG SET received!");
}

static void luos_msg_model_status_cb(access_model_handle_t handle,
                                     const access_message_rx_t* msg,
                                     void* arg)
{
    NRF_LOG_INFO("Luos MSG STATUS received!");
}
