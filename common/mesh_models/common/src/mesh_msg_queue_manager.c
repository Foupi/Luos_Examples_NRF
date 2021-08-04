#include "mesh_msg_queue_manager.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>                // bool

// NRF
#include "nrf_log.h"                // NRF_LOG_INFO
#include "sdk_errors.h"             // ret_code_t

// NRF APPS
#include "app_error.h"              // APP_ERROR_CHECK

// MESH SDK
#include "access.h"                 // access_*
#include "nrf_mesh_events.h"        // nrf_mesh_evt_*
#include "nrf_mesh.h"               // NRF_MESH_TRANSMIC_SIZE_DEFAULT

// CUSTOM
#include "luos_mesh_msg_queue.h"    // tx_queue_elm_t
#include "luos_rtb_model.h"         // luos_rtb_model_*
#include "luos_rtb_model_common.h"  // LUOS_RTB_MODEL_*_ACCESS_OPCODE

/*      STATIC VARIABLES & CONSTANTS                                */

// Describes if a message can be sent.
static bool                 s_is_possible_to_send   = true;

// Token of the currently sent message.
static nrf_mesh_tx_token_t  s_curr_tx_token         = 0x0000;

/*      STATIC FUNCTIONS                                            */

// Publishes or replies the last queue element if there is one.
static void send_mesh_msg(void);

// Prepares the given message with the given characteristics.
static void send_luos_rtb_model_msg(const tx_queue_elm_t* elm,
                                    access_message_tx_t* msg);

/*      CALLBACKS                                                   */

// Toggles the send boolean and tries to pop the next message.
static void mesh_tx_complete_event_cb(const nrf_mesh_evt_t* event);

/*      INITIALIZATIONS                                             */

static nrf_mesh_evt_handler_t   mesh_tx_complete_event_handler  =
{
    .evt_cb = mesh_tx_complete_event_cb,
};

void luos_mesh_msg_queue_manager_init(void)
{
    nrf_mesh_evt_handler_add(&mesh_tx_complete_event_handler);
}

void luos_mesh_msg_prepare(const tx_queue_elm_t* message)
{
    bool insert_success = luos_mesh_msg_queue_enqueue(message);
    if (!insert_success)
    {
        // FIXME Manage error.
        while (true);
    }

    if (s_is_possible_to_send)
    {
        send_mesh_msg();
    }
}

static void send_mesh_msg(void)
{
    tx_queue_elm_t* last_queue_elm  = luos_mesh_msg_queue_peek();
    if (last_queue_elm == NULL)
    {
        // Queue is empty: nothing to send.
        return;
    }

    access_message_tx_t message;
    memset(&message, 0, sizeof(access_message_tx_t));
    message.transmic_size   = NRF_MESH_TRANSMIC_SIZE_DEFAULT;
    message.access_token    = nrf_mesh_unique_token_get();

    switch (last_queue_elm->model)
    {
    case TX_QUEUE_MODEL_LUOS_RTB:
        send_luos_rtb_model_msg(last_queue_elm, &message);
        break;

    default:
        // Unknown type.
        return;
    }

    luos_mesh_msg_queue_pop();
    s_curr_tx_token = message.access_token;

    NRF_LOG_INFO("Message sent: current token is 0x%x!", s_curr_tx_token);

    s_is_possible_to_send = false;
}

static void send_luos_rtb_model_msg(const tx_queue_elm_t* elm,
                                    access_message_tx_t* msg)
{
    ret_code_t                      err_code;
    tx_queue_luos_rtb_model_elm_t   rtb_model_msg = elm->content.luos_rtb_model_msg;

    switch (rtb_model_msg.cmd)
    {
    case TX_QUEUE_CMD_GET:
    {
        access_opcode_t opcode  = LUOS_RTB_MODEL_GET_ACCESS_OPCODE;

        msg->opcode             = opcode;
        msg->p_buffer           = (uint8_t*)(&(rtb_model_msg.content.get));
        msg->length             = sizeof(luos_rtb_model_get_t);

        err_code                = access_model_publish(elm->model_handle,
                                                       msg);
        APP_ERROR_CHECK(err_code);
    }
        break;

    case TX_QUEUE_CMD_STATUS:
    {
        access_opcode_t opcode  = LUOS_RTB_MODEL_STATUS_ACCESS_OPCODE;

        msg->opcode             = opcode;
        msg->p_buffer           = (uint8_t*)(&(rtb_model_msg.content.status));
        msg->length             = sizeof(luos_rtb_model_status_t);

        err_code                = access_model_publish(elm->model_handle,
                                                       msg);
        APP_ERROR_CHECK(err_code);
    }
        break;

    case TX_QUEUE_CMD_STATUS_REPLY:
    {
        access_opcode_t opcode  = LUOS_RTB_MODEL_STATUS_ACCESS_OPCODE;

        msg->opcode             = opcode;
        msg->p_buffer           = (uint8_t*)(&(rtb_model_msg.content.status_reply.status));
        msg->length             = sizeof(luos_rtb_model_status_t);

        err_code                = access_model_reply(elm->model_handle,
            &(rtb_model_msg.content.status_reply.src_msg), msg);
        APP_ERROR_CHECK(err_code);
    }
        break;

    default:
        // Unknown command.
        return;
    }
}

static void mesh_tx_complete_event_cb(const nrf_mesh_evt_t* event)
{
    nrf_mesh_tx_token_t token;

    switch (event->type)
    {
    case NRF_MESH_EVT_TX_COMPLETE:
        token = event->params.tx_complete.token;

        if (token == s_curr_tx_token)
        {
            NRF_LOG_INFO("Transmission complete for token 0x%x!", token);

            s_is_possible_to_send = true;
            send_mesh_msg();

            s_curr_tx_token = 0x0000;
        }

        break;

    default:
        return;
    }
}
