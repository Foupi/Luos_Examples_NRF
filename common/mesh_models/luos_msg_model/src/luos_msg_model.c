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

// LUOS
#include "luos_utils.h"             // LUOS_ASSERT

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

/* Verifies the SET command address, then calls the given instance's.
** SET callback.
*/
static void luos_msg_model_set_cb(access_model_handle_t handle,
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
};

void luos_msg_model_init(luos_msg_model_t* instance,
                         const luos_msg_model_init_params_t* params)
{
    // Check parameters.
    LUOS_ASSERT(instance != NULL);
    LUOS_ASSERT(params != NULL);
    LUOS_ASSERT(params->set_send != NULL);
    LUOS_ASSERT(params->set_cb != NULL);

    // Fill instance information.
    memset(instance, 0, sizeof(luos_msg_model_t));
    // Set as default: it shall be set later.
    instance->element_address       = LUOS_MSG_MODEL_DEFAULT_ELM_ADDR;
    // Copy params.
    instance->set_send              = params->set_send;
    instance->set_cb                = params->set_cb;

    ret_code_t                  err_code;
    access_model_id_t           luos_msg_model_id   = LUOS_MSG_MODEL_ACCESS_ID;
    uint16_t                    nb_opcodes;

    nb_opcodes  = sizeof(LUOS_MSG_MODEL_OPCODE_HANDLERS) / sizeof(access_opcode_handler_t);

    // Parameters to add the model in the access layer.
    access_model_add_params_t   add_params;
    memset(&add_params, 0, sizeof(access_model_add_params_t));
    add_params.model_id             = luos_msg_model_id;                // Added model has Luos MSG model ID.
    add_params.element_index        = LUOS_MSG_MODEL_ELM_IDX;           // Model is added on ELM_IDX.
    add_params.p_opcode_handlers    = LUOS_MSG_MODEL_OPCODE_HANDLERS;   // Opcode handlers for this model.
    add_params.opcode_count         = nb_opcodes;                       // Number of opcodes in the previous array.
    add_params.p_args               = instance;                         /* Argument given to opcode handlers: the instance address, for
                                                                        ** context (model handle, element address...).
                                                                        */

    // Add model in access layer and update instance handle.
    err_code    = access_model_add(&add_params, &(instance->handle));
    APP_ERROR_CHECK(err_code);

    /* Allocates space for a subscription address to the model
    ** corresponding to the given handle.
    */
    err_code = access_model_subscription_list_alloc(instance->handle);
    APP_ERROR_CHECK(err_code);
}

void luos_msg_model_set_address(luos_msg_model_t* instance,
                                uint16_t device_address)
{
    // Check parameter.
    LUOS_ASSERT(instance != NULL);

    // Set the instance element address for future message management.
    instance->element_address   = device_address + LUOS_MSG_MODEL_ELM_IDX;
}

void luos_msg_model_set(luos_msg_model_t* instance, uint16_t dst_addr,
                        const luos_mesh_msg_t* msg)
{
    // Check parameter.
    LUOS_ASSERT(instance != NULL);
    LUOS_ASSERT(instance->set_send != NULL);

    // Engaging new transaction.
    s_curr_transaction_id++;

    // SET request payload.
    luos_msg_model_set_t    set_cmd;
    memset(&set_cmd, 0, sizeof(luos_msg_model_set_t));
    set_cmd.transaction_id  = s_curr_transaction_id;
    set_cmd.dst_addr        = dst_addr;
    memcpy(&(set_cmd.msg), msg, sizeof(luos_mesh_msg_t));

    // Send request through user-defined function.
    instance->set_send(instance, &set_cmd);
}

static void luos_msg_model_set_cb(access_model_handle_t handle,
                                  const access_message_rx_t* msg,
                                  void* arg)
{
    // An instance was stored in context in `luos_msg_model_init`.
    luos_msg_model_t*           instance    = (luos_msg_model_t*)arg;

    // Check parameters.
    LUOS_ASSERT(instance != NULL);
    LUOS_ASSERT(instance->set_cb != NULL);
    LUOS_ASSERT(msg != NULL);

    // Unicast address of the node which sent the command.
    uint16_t                    src_addr    = msg->meta_data.src.value;

    if (instance->element_address == LUOS_MSG_MODEL_DEFAULT_ELM_ADDR
        || src_addr == instance->element_address)
    {
        // Either model is not ready, or this is a localhost message.
        return;
    }

    // The actual command.
    const luos_msg_model_set_t* set_cmd     = (luos_msg_model_set_t*)(msg->p_data);

    if (set_cmd->transaction_id <= s_curr_transaction_id)
    {
        // Transaction either already occured or is currently occuring.
        return;
    }

    /* Update transaction ID even if the node is not targeted: this ID
    ** will progress with all transactions.
    */
    s_curr_transaction_id       = set_cmd->transaction_id;

    if (set_cmd->dst_addr != instance->element_address)
    {
        // Model instance is not the message destination.
        return;
    }

    // The payload message.
    const   luos_mesh_msg_t*    luos_msg    = &(set_cmd->msg);

    instance->set_cb(src_addr, luos_msg);
}
