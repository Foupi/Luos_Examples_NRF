#include "luos_rtb_model.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <string.h>                 // memset

// NRF
#include "sdk_errors.h"             // ret_code_t

// NRF APPS
#include "app_error.h"              // APP_ERROR_CHECK

// MESH SDK
#include "access.h"                 // access_*
#include "access_config.h"          // access_model_subscription_list_alloc
#include "nrf_mesh.h"               // NRF_MESH_TRANSMIC_SIZE_DEFAULT

// LUOS
#include "luos_utils.h"             // LUOS_ASSERT
#include "routing_table.h"          // routing_table_t

// CUSTOM
#include "luos_rtb_model_common.h"  // LUOS_RTB_MODEL_*

#ifdef DEBUG
#include "nrf_log.h"                // NRF_LOG_INFO
#endif /* DEBUG */

/*      STATIC VARIABLES & CONSTANTS                                */

// Index of the current transaction
static uint16_t s_curr_transaction_id   = 0;

/*      STATIC FUNCTIONS                                            */

/* Replies the given RTB entries to the given message through the given
** instance.
*/
static void luos_rtb_model_reply_entries(luos_rtb_model_t* instance,
    const routing_table_t* entries, uint16_t nb_entries,
    const access_message_rx_t* msg);

/*      CALLBACKS                                                   */

/* Calls the given instance's GET callback, then retrieves and replies
** RTB entries.
*/
static void luos_rtb_model_get_cb(access_model_handle_t handle,
                                  const access_message_rx_t* msg,
                                  void* arg);

// Calls the given instance's STATUS callback.
static void luos_rtb_model_status_cb(access_model_handle_t handle,
                                     const access_message_rx_t* msg,
                                     void* arg);

/*      INITIALIZATIONS                                             */

// Luos RTB model opcode handlers table.
static const access_opcode_handler_t    LUOS_RTB_MODEL_OPCODE_HANDLERS[]    =
{
    {
        LUOS_RTB_MODEL_GET_ACCESS_OPCODE,
        luos_rtb_model_get_cb,
    },
    {
        LUOS_RTB_MODEL_STATUS_ACCESS_OPCODE,
        luos_rtb_model_status_cb,
    },
};

void luos_rtb_model_init(luos_rtb_model_t* instance,
                         const luos_rtb_model_init_params_t* params)
{
    // Check parameters.
    LUOS_ASSERT(instance != NULL);
    LUOS_ASSERT(params != NULL);
    LUOS_ASSERT(params->get_send != NULL);
    LUOS_ASSERT(params->status_send != NULL);
    LUOS_ASSERT(params->status_reply != NULL);
    LUOS_ASSERT(params->rtb_entries_get_cb != NULL);
    // GET and STATUS callbacks do not matter too much.

    // Fill instance information.
    memset(instance, 0, sizeof(luos_rtb_model_t));
    // Set as default: it shall be set later.
    instance->element_address       = LUOS_RTB_MODEL_DEFAULT_ELM_ADDR;
    // Copy params.
    instance->get_send              = params->get_send;
    instance->status_send           = params->status_send;
    instance->status_reply          = params->status_reply;
    instance->get_cb                = params->get_cb;
    instance->rtb_entries_get_cb    = params->rtb_entries_get_cb;
    instance->status_cb             = params->status_cb;

    ret_code_t                  err_code;
    access_model_id_t           luos_rtb_model_id   = LUOS_RTB_MODEL_ACCESS_ID;
    uint16_t                    nb_opcodes;

    nb_opcodes  = sizeof(LUOS_RTB_MODEL_OPCODE_HANDLERS) / sizeof(access_opcode_handler_t);

    // Parameters to add the model in the access layer.
    access_model_add_params_t   add_params;
    memset(&add_params, 0, sizeof(access_model_add_params_t));
    add_params.model_id             = luos_rtb_model_id;                // Added model has Luos RTB model ID.
    add_params.element_index        = LUOS_RTB_MODEL_ELM_IDX;           // Model is added on element ELM_IDX.
    add_params.p_opcode_handlers    = LUOS_RTB_MODEL_OPCODE_HANDLERS;   // Opcode handlers for this model.
    add_params.opcode_count         = nb_opcodes;                       // Number of opcodes in the previous array.
    add_params.p_args               = instance;                         /* Argument given to opcode handlers: the instance address, for
                                                                        ** context (model handle, element address...).
                                                                        */

    // Add model in access layer and update instance handle.
    err_code = access_model_add(&add_params, &(instance->handle));
    APP_ERROR_CHECK(err_code);

    /* Allocates space for a subscription address to the model
    ** corresponding to the given handle.
    */
    err_code = access_model_subscription_list_alloc(instance->handle);
    APP_ERROR_CHECK(err_code);
}

void luos_rtb_model_set_address(luos_rtb_model_t* instance,
                                uint16_t device_address)
{
    // Check parameter.
    LUOS_ASSERT(instance != NULL);

    // Set the instance element address for future message management.
    instance->element_address = device_address + LUOS_RTB_MODEL_ELM_IDX;
}

void luos_rtb_model_get(luos_rtb_model_t* instance)
{
    // Check parameter.
    LUOS_ASSERT(instance != NULL);
    LUOS_ASSERT(instance->get_send != NULL);

    // Engaging new transaction.
    s_curr_transaction_id++;

    // GET request payload.
    luos_rtb_model_get_t    get_msg;
    memset(&get_msg, 0, sizeof(luos_rtb_model_get_t));
    get_msg.transaction_id  = s_curr_transaction_id;

    // Send request through user-defined function.
    instance->get_send(instance, &get_msg);
}

void luos_rtb_model_publish_entries(luos_rtb_model_t* instance,
                                    const routing_table_t* entries,
                                    uint16_t nb_entries)
{
    // Check parameters.
    LUOS_ASSERT(instance != NULL);
    LUOS_ASSERT(instance->status_send != NULL);
    LUOS_ASSERT(entries != NULL);

    for (uint16_t entry_idx = 0; entry_idx < nb_entries; entry_idx++)
    {
        // STATUS message payload.
        luos_rtb_model_status_t publish_status;
        memset(&publish_status, 0, sizeof(luos_rtb_model_status_t));
        publish_status.transaction_id   = s_curr_transaction_id;
        publish_status.entry_idx        = entry_idx;
        // Copy current RTB entry in message.
        memcpy(&(publish_status.entry), entries + entry_idx,
               sizeof(routing_table_t));

        // Send message through user-defined function.
        instance->status_send(instance, &publish_status);
    }
}

static void luos_rtb_model_reply_entries(luos_rtb_model_t* instance,
    const routing_table_t* entries, uint16_t nb_entries,
    const access_message_rx_t* msg)
{
    // Check parameters.
    LUOS_ASSERT(instance != NULL);
    LUOS_ASSERT(instance->status_reply != NULL);
    LUOS_ASSERT(entries != NULL);
    LUOS_ASSERT(msg != NULL);

    for (uint16_t entry_idx = 0; entry_idx < nb_entries; entry_idx++)
    {
        // STATUS reply payload.
        luos_rtb_model_status_t reply_status;
        memset(&reply_status, 0, sizeof(luos_rtb_model_status_t));
        reply_status.transaction_id = s_curr_transaction_id;
        reply_status.entry_idx      = entry_idx;
        // Copy current RTB entry in message.
        memcpy(&(reply_status.entry), entries + entry_idx,
               sizeof(routing_table_t));

        // Reply message through user-defined function.
        instance->status_reply(instance, &reply_status, msg);
    }
}

static void luos_rtb_model_get_cb(access_model_handle_t handle,
                                  const access_message_rx_t* msg,
                                  void* arg)
{
    // An instance was stored in context in `luos_rtb_model_init`.
    luos_rtb_model_t*           instance    = (luos_rtb_model_t*)arg;

    // Check parameters.
    LUOS_ASSERT(instance != NULL);
    LUOS_ASSERT(instance->rtb_entries_get_cb != NULL);
    LUOS_ASSERT(instance->status_reply != NULL);
    LUOS_ASSERT(msg != NULL);

    // Unicast address of the node which sent the request.
    uint16_t                    src_addr    = msg->meta_data.src.value;

    if (instance->element_address == LUOS_RTB_MODEL_DEFAULT_ELM_ADDR
        || src_addr == instance->element_address)
    {
        // Either model is not ready, or this is a localhost message.
        return;
    }

    // The actual request.
    const luos_rtb_model_get_t* get_req     = (luos_rtb_model_get_t*)(msg->p_data);

    if (get_req->transaction_id <= s_curr_transaction_id)
    {
        // Transaction either already occured or is currently occuring.
        return;
    }

    // Update current transaction ID: new transaction ongiong.
    s_curr_transaction_id   = get_req->transaction_id;

    if (instance->get_cb != NULL)
    {
        instance->get_cb(src_addr);
    }
    #ifdef DEBUG
    else
    {
        NRF_LOG_INFO("No user-defined callback for GET requests on Luos RTB model!");
    }
    #endif /* DEBUG */

    // RTB entries buffer.
    routing_table_t             rtb_entries[LUOS_RTB_MODEL_MAX_RTB_ENTRY];
    uint16_t                    nb_entries;
    memset(rtb_entries, 0,
           LUOS_RTB_MODEL_MAX_RTB_ENTRY * sizeof(routing_table_t));

    // Retrieve RTB entries.
    bool                        entries_retrieved;
    entries_retrieved       = instance->rtb_entries_get_cb(rtb_entries,
                                                           &nb_entries);
    if (!entries_retrieved)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("RTB entries could not be retrieved!");
        #endif /* DEBUG */

        return;
    }

    #ifdef DEBUG
    NRF_LOG_INFO("Local RTB contains %u entries!", nb_entries);
    #endif /* DEBUG */

    // Reply with retrieved entries.
    luos_rtb_model_reply_entries(instance, rtb_entries,
                                 nb_entries, msg);
}

static void luos_rtb_model_status_cb(access_model_handle_t handle,
                                     const access_message_rx_t* msg,
                                     void* arg)
{
    // An instance was stored in context in `luos_rtb_model_init`.
    luos_rtb_model_t*   instance    = (luos_rtb_model_t*)arg;

    // Check parameters.
    LUOS_ASSERT(instance != NULL);
    LUOS_ASSERT(msg != NULL);

    // Unicast address of the node sending the message.
    uint16_t            src_addr    = msg->meta_data.src.value;

    if (instance->element_address == LUOS_RTB_MODEL_DEFAULT_ELM_ADDR
        || src_addr == instance->element_address)
    {
        // Either model is not ready, or this is a localhost message.
        return;
    }

    // The actual message.
    const luos_rtb_model_status_t*  status_msg  = (luos_rtb_model_status_t*)(msg->p_data);

    if (status_msg->transaction_id < s_curr_transaction_id)
    {
        // This STATUS message is meant for a previous transaction.
        return;
    }

    if (instance->status_cb != NULL)
    {
        instance->status_cb(src_addr, &(status_msg->entry),
                            status_msg->entry_idx);
    }
    #ifdef DEBUG
    else
    {
        NRF_LOG_INFO("No user-defined callback for STATUS messages on Luos RTB model!");
    }
    #endif /* DEBUG */
}
