#include "luos_rtb_model.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <string.h>                 // memset

// NRF
#include "nrf_log.h"                // NRF_LOG_INFO
#include "sdk_errors.h"             // ret_code_t

// NRF APPS
#include "app_error.h"              // APP_ERROR_CHECK

// MESH SDK
#include "access.h"                 // access_*
#include "access_config.h"          // access_model_subscription_list_alloc
#include "nrf_mesh.h"               // NRF_MESH_TRANSMIC_SIZE_DEFAULT

// LUOS
#include "routing_table.h"          // routing_table_t

// CUSTOM
#include "luos_rtb_model_common.h"  // LUOS_RTB_MODEL_*

/*      STATIC VARIABLES & CONSTANTS                                */

// Index of the current transaction
uint16_t                                s_curr_transaction_id               = 0;

/*      STATIC FUNCTIONS                                            */

// Reply the given status to the given message using the given instance.
static void luos_rtb_model_reply_entry(luos_rtb_model_t* instance,
    const access_message_rx_t* msg,
    const luos_rtb_model_status_t* reply_status);

/*      CALLBACKS                                                   */

// Manage a Luos RTB model GET request.
static void luos_rtb_model_get_cb(access_model_handle_t handle,
                                  const access_message_rx_t* msg,
                                  void* arg);

// Manage a Luos RTB model STATUS message.
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
    memset(instance, 0, sizeof(luos_rtb_model_t));
    instance->element_address           = LUOS_RTB_MODEL_DEFAULT_ELM_ADDR;
    instance->get_cb                    = params->get_cb;
    instance->local_rtb_entries_get_cb  = params->local_rtb_entries_get_cb;
    instance->status_cb                 = params->status_cb;

    ret_code_t                  err_code;
    access_model_id_t           luos_rtb_model_id   = LUOS_RTB_MODEL_ACCESS_ID;

    access_model_add_params_t   add_params;
    memset(&add_params, 0, sizeof(access_model_add_params_t));
    add_params.model_id             = luos_rtb_model_id;
    add_params.element_index        = LUOS_RTB_MODEL_ELM_IDX;
    add_params.p_opcode_handlers    = LUOS_RTB_MODEL_OPCODE_HANDLERS;
    add_params.opcode_count         = sizeof(LUOS_RTB_MODEL_OPCODE_HANDLERS) / sizeof(access_opcode_handler_t);
    add_params.p_args               = instance;

    err_code = access_model_add(&add_params, &(instance->handle));
    APP_ERROR_CHECK(err_code);

    err_code = access_model_subscription_list_alloc(instance->handle);
    APP_ERROR_CHECK(err_code);
}

void luos_rtb_model_set_address(luos_rtb_model_t* instance,
                                uint16_t device_address)
{
    instance->element_address = device_address + LUOS_RTB_MODEL_ELM_IDX;
}

void luos_rtb_model_get(luos_rtb_model_t* instance)
{
    ret_code_t          err_code;
    access_opcode_t     get_opcode  = LUOS_RTB_MODEL_GET_ACCESS_OPCODE;

    s_curr_transaction_id++;

    luos_rtb_model_get_t      get_msg;
    memset(&get_msg, 0, sizeof(luos_rtb_model_get_t));
    get_msg.transaction_id  = s_curr_transaction_id;

    access_message_tx_t get_req;
    memset(&get_req, 0, sizeof(access_message_tx_t));
    get_req.opcode          = get_opcode;
    get_req.p_buffer        = (uint8_t*)(&get_msg);
    get_req.length          = sizeof(luos_rtb_model_get_t);
    get_req.transmic_size   = NRF_MESH_TRANSMIC_SIZE_DEFAULT;
    get_req.access_token    = nrf_mesh_unique_token_get();

    err_code = access_model_publish(instance->handle, &get_req);
    APP_ERROR_CHECK(err_code);
}

static void luos_rtb_model_reply_entry(luos_rtb_model_t* instance,
    const access_message_rx_t* msg,
    const luos_rtb_model_status_t* reply_status)
{
    ret_code_t          err_code;
    access_opcode_t     status_opcode   = LUOS_RTB_MODEL_STATUS_ACCESS_OPCODE;

    access_message_tx_t reply_msg;
    memset(&reply_msg, 0, sizeof(access_message_tx_t));
    reply_msg.opcode        = status_opcode;
    reply_msg.p_buffer      = (uint8_t*)reply_status;
    reply_msg.length        = sizeof(luos_rtb_model_status_t);
    reply_msg.transmic_size = NRF_MESH_TRANSMIC_SIZE_DEFAULT;
    reply_msg.access_token  = nrf_mesh_unique_token_get();

    err_code = access_model_reply(instance->handle, msg, &reply_msg);
    APP_ERROR_CHECK(err_code);
}

static void luos_rtb_model_get_cb(access_model_handle_t handle,
                                  const access_message_rx_t* msg,
                                  void* arg)
{
    luos_rtb_model_t*   instance    = (luos_rtb_model_t*)arg;
    uint16_t            src_addr    = msg->meta_data.src.value;

    if (instance->element_address == LUOS_RTB_MODEL_DEFAULT_ELM_ADDR
        || src_addr == instance->element_address)
    {
        // Either model is not ready, or this is a localhost message.
        return;
    }

    const luos_rtb_model_get_t* get_req = (luos_rtb_model_get_t*)(msg->p_data);

    if (get_req->transaction_id <= s_curr_transaction_id)
    {
        // Transaction either already occured or is currently occuring.
        return;
    }

    s_curr_transaction_id = get_req->transaction_id;

    if (instance->get_cb == NULL)
    {
        NRF_LOG_INFO("No user-defined callback for GET requests on Luos RTB model!");
    }
    else
    {
        instance->get_cb();
    }

    if ((instance->local_rtb_entries_get_cb) == NULL)
    {
        NRF_LOG_INFO("No callback to retrieve local RTB entries: leaving!");
        return;
    }

    routing_table_t     local_rtb_entries[LUOS_RTB_MODEL_MAX_RTB_ENTRY];
    uint16_t            nb_local_entries;
    bool                detection_complete;

    memset(local_rtb_entries, 0, LUOS_RTB_MODEL_MAX_RTB_ENTRY * sizeof(routing_table_t));

    detection_complete  = instance->local_rtb_entries_get_cb(local_rtb_entries,
        &nb_local_entries);
    if (!detection_complete)
    {
        NRF_LOG_INFO("Local RTB cannot be retrieved: detection not complete!");
        return;
    }

    NRF_LOG_INFO("Local RTB contains %u entries!", nb_local_entries);

    for (uint16_t entry_idx = 0; entry_idx < nb_local_entries;
         entry_idx++)
    {
        routing_table_t         entry   = local_rtb_entries[entry_idx];

        luos_rtb_model_status_t reply_status;
        memset(&reply_status, 0, sizeof(reply_status));
        reply_status.transaction_id = s_curr_transaction_id;
        reply_status.entry_idx      = entry_idx;
        memcpy(&(reply_status.entry), &entry, sizeof(routing_table_t));

        luos_rtb_model_reply_entry(instance, msg, &reply_status);
    }
}

static void luos_rtb_model_status_cb(access_model_handle_t handle,
                                     const access_message_rx_t* msg,
                                     void* arg)
{
    luos_rtb_model_t*   instance    = (luos_rtb_model_t*)arg;
    uint16_t            src_addr    = msg->meta_data.src.value;

    if (instance->element_address == LUOS_RTB_MODEL_DEFAULT_ELM_ADDR
        || src_addr == instance->element_address)
    {
        // Either model is not ready, or this is a localhost message.
        return;
    }

    const luos_rtb_model_status_t*  status_msg  = (luos_rtb_model_status_t*)(msg->p_data);

    if (status_msg->transaction_id < s_curr_transaction_id)
    {
        // This STATUS message is meant for a previous transaction.
        return;
    }

    if (instance->status_cb == NULL)
    {
        NRF_LOG_INFO("No user-defined callback for STATUS messages on Luos RTB model!");
    }
    else
    {
        instance->status_cb(src_addr, &(status_msg->entry),
                            status_msg->entry_idx);
    }
}
