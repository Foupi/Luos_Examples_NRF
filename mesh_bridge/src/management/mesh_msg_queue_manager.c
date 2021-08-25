#include "mesh_msg_queue_manager.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>                // bool

// NRF
#include "sdk_errors.h"             // ret_code_t

// NRF APPS
#include "app_error.h"              // APP_ERROR_CHECK
#include "app_timer.h"              // app_timer_*

// MESH SDK
#include "access.h"                 // access_*
#include "nrf_mesh_events.h"        // nrf_mesh_evt_*
#include "nrf_mesh.h"               // NRF_MESH_TRANSMIC_SIZE_DEFAULT

// LUOS
#include "luos_utils.h"             // LUOS_ASSERT

// CUSTOM
#include "app_luos_rtb_model.h"     // app_luos_rtb_model_publication_end
#include "local_container_table.h"  // local_container_table_get_nb_entries
#include "luos_mesh_msg.h"          // LUOS_MESH_MSG_MAX_DATA_SIZE
#include "luos_mesh_msg_queue.h"    // tx_queue_elm_t
#include "luos_msg_model.h"         // luos_msg_model_*
#include "luos_msg_model_common.h"  // LUOS_MSG_MODEL_*_ACCESS_OPCODE
#include "luos_rtb_model.h"         // luos_rtb_model_*
#include "luos_rtb_model_common.h"  // LUOS_RTB_MODEL_*_ACCESS_OPCODE

#include "nrf_log.h"

/*      STATIC VARIABLES & CONSTANTS                                */

// Describes if a message can be sent.
static bool                 s_is_possible_to_send   = true;

// Default value for currently sent message token.
#define                     DEFAULT_STATIC_TOKEN    0xFFFF;

// Token of the currently sent message.
static nrf_mesh_tx_token_t  s_curr_tx_token         = DEFAULT_STATIC_TOKEN;

// Wait time between TX complete event and next message sent.
#define                     WAIT_TIME_MS            5
static const uint32_t       WAIT_TIME_TICKS         = APP_TIMER_TICKS(WAIT_TIME_MS);

/*      STATIC FUNCTIONS                                            */

// Publishes or replies the last queue element if there is one.
static void send_mesh_msg(void);

// Prepares the given Luos RTB message with the given characteristics.
static void send_luos_rtb_model_msg(const tx_queue_elm_t* elm,
                                    access_message_tx_t* msg);

// Prepares the given Luos MSG message with the given characteristics.
static void send_luos_msg_model_msg(const tx_queue_elm_t* elm,
                                    access_message_tx_t* msg);

/* Returns true if the given queue element is the last published RTB entry,
** false otherwise.
*/
/* FIXME    This responsibility should not be put on the message queue
**          manager...
*/
static bool is_last_published_rtb_entry(const tx_queue_elm_t* elm);

/*      CALLBACKS                                                   */

/* If the event token matches the current one, toggles the send boolean
** and starts the timer to send the next message.
*/
static void mesh_tx_complete_event_cb(const nrf_mesh_evt_t* event);

// Sends the first message in the queue.
static void timer_to_send_event_cb(void* context);

/*      INITIALIZATIONS                                             */

// TX complete event handler struct, to add to the Mesh stack.
static nrf_mesh_evt_handler_t   mesh_tx_complete_event_handler  =
{
    .evt_cb = mesh_tx_complete_event_cb,
};

// Timer waiting between TX event and message sending.
APP_TIMER_DEF(s_timer_to_send);

void luos_mesh_msg_queue_manager_init(void)
{
    ret_code_t  err_code;

    // Add TX complete event handler to the Mesh stack.
    nrf_mesh_evt_handler_add(&mesh_tx_complete_event_handler);

    // Create sending wait timer.
    err_code    = app_timer_create(&s_timer_to_send,
                                   APP_TIMER_MODE_SINGLE_SHOT,
                                   timer_to_send_event_cb);
    APP_ERROR_CHECK(err_code);
}

void luos_mesh_msg_prepare(const tx_queue_elm_t* message)
{
    // Check parameter.
    LUOS_ASSERT(message != NULL);

    // Enqueue given TX queue element.
    bool    insert_success  = luos_mesh_msg_queue_enqueue(message);

    // Check insertion result.
    LUOS_ASSERT(insert_success);

    if (s_is_possible_to_send)
    {
        // Send last message in queue if sending is possible.
        send_mesh_msg();
    }
}

static void send_mesh_msg(void)
{
    // Fetch last queue element.
    tx_queue_elm_t*     last_queue_elm  = luos_mesh_msg_queue_peek();
    if (last_queue_elm == NULL)
    {
        // Queue is empty: nothing to send.
        return;
    }

    // Message to send on Access layer.
    access_message_tx_t message;
    memset(&message, 0, sizeof(access_message_tx_t));
    message.transmic_size   = NRF_MESH_TRANSMIC_SIZE_DEFAULT;   // Size of transmission check data.
    message.access_token    = nrf_mesh_unique_token_get();      // Token identifying Mesh stack transaction.
    switch (last_queue_elm->model)
    {
    case TX_QUEUE_MODEL_LUOS_RTB:
        // Complete message with Luos RTB message data and send it.
        send_luos_rtb_model_msg(last_queue_elm, &message);
        break;

    case TX_QUEUE_MODEL_LUOS_MSG:
        // Complete message with Luos MSG message data and send it.
        send_luos_msg_model_msg(last_queue_elm, &message);
        break;

    default:
        // Unknown type: break down.
        LUOS_ASSERT(false);
        return;
    }

    /* Set current Mesh stack transaction identifier for TX complete
    ** ID check.
    */
    s_curr_tx_token         = message.access_token;

    // Send operation is occuring: impossible to send a new message.
    s_is_possible_to_send   = false;
}

static void send_luos_rtb_model_msg(const tx_queue_elm_t* elm,
                                    access_message_tx_t* msg)
{
    // Check parameters.
    LUOS_ASSERT(elm != NULL);
    LUOS_ASSERT(msg != NULL);

    ret_code_t                              err_code;

    // Luos RTB message encapsulated in TX queue element.
    const tx_queue_luos_rtb_model_elm_t*    rtb_model_msg;
    rtb_model_msg   = &(elm->content.luos_rtb_model_msg);

    switch (rtb_model_msg->cmd)
    {
    case TX_QUEUE_CMD_GET:
    {
        // Luos RTB GET request.

        // Luos RTB model GET complete access opcode.
        access_opcode_t opcode  = LUOS_RTB_MODEL_GET_ACCESS_OPCODE;

        // Fill message data with Luos RTB GET request.
        msg->opcode     = opcode;
        msg->p_buffer   = (uint8_t*)(&(rtb_model_msg->content.get));
        msg->length     = sizeof(luos_rtb_model_get_t);

        // Publish Luos RTB GET request.
        err_code        = access_model_publish(elm->model_handle, msg);
    }
        break;

    case TX_QUEUE_CMD_STATUS:
    {
        // Luos RTB STATUS message.

        // Luos RTB model STATUS complete access opcode.
        access_opcode_t opcode  = LUOS_RTB_MODEL_STATUS_ACCESS_OPCODE;

        // Fill message data with Luos RTB STATUS message.
        msg->opcode     = opcode;
        msg->p_buffer   = (uint8_t*)(&(rtb_model_msg->content.status));
        msg->length     = sizeof(luos_rtb_model_status_t);

        // Publish Luos RTB STATUS message.
        err_code        = access_model_publish(elm->model_handle, msg);
    }
        break;

    case TX_QUEUE_CMD_STATUS_REPLY:
    {
        // Luos RTB STATUS reply.

        // Luos RTB model STATUS complete access opcode.
        access_opcode_t             opcode  = LUOS_RTB_MODEL_STATUS_ACCESS_OPCODE;

        // Fill message data with Luos RTB STATUS reply.
        msg->opcode     = opcode;
        msg->p_buffer   = (uint8_t*)(&(rtb_model_msg->content.status_reply.status));
        msg->length     = sizeof(luos_rtb_model_status_t);

        const access_message_rx_t*  src_msg;
        src_msg         = &(rtb_model_msg->content.status_reply.src_msg);

        // Reply to source message with Luos RTB STATUS reply.
        err_code        = access_model_reply(elm->model_handle, src_msg,
                                             msg);
    }
        break;

    default:
        // Unknown command: break down.
        LUOS_ASSERT(false);
        return;
    }

    // Check sending status.
    APP_ERROR_CHECK(err_code);
}

static void send_luos_msg_model_msg(const tx_queue_elm_t* elm,
                                    access_message_tx_t* msg)
{
    // Check parameters.
    LUOS_ASSERT(elm != NULL);
    LUOS_ASSERT(msg != NULL);

    ret_code_t                              err_code;

    // Luos MSG message encapsulated in TX queue element.
    const tx_queue_luos_msg_model_elm_t*    msg_model_msg;
    msg_model_msg   = &(elm->content.luos_msg_model_msg);

    switch (msg_model_msg->cmd)
    {
    case TX_QUEUE_CMD_SET:
    {
        // Luos MSG SET command.

        // Luos MSG model SET complete access opcode.
        access_opcode_t opcode      = LUOS_MSG_MODEL_SET_ACCESS_OPCODE;

        NRF_LOG_INFO("Max message payload size: %u", LUOS_MESH_MSG_MAX_DATA_SIZE);

        // Fill message data with Luos MSG SET command.
        msg->opcode     = opcode;
        msg->p_buffer   = (uint8_t*)(&(msg_model_msg->content.set));
        msg->length     = sizeof(luos_msg_model_set_t);

        // Publish Luos MSG SET command.
        err_code        = access_model_publish(elm->model_handle, msg);
    }
        break;

    default:
        // Unknown command: break down.
        LUOS_ASSERT(false);
        return;
    }

    // Check sending status.
    APP_ERROR_CHECK(err_code);
}

static bool is_last_published_rtb_entry(const tx_queue_elm_t* elm)
{
    // Check parameter.
    LUOS_ASSERT(elm != NULL);

    if (elm->model == TX_QUEUE_MODEL_LUOS_RTB)
    {
        // Luos RTB message encapsulated in TX queue element.
        const tx_queue_luos_rtb_model_elm_t*    rtb_model_msg;
        rtb_model_msg   = &(elm->content.luos_rtb_model_msg);

        if (rtb_model_msg->cmd == TX_QUEUE_CMD_STATUS)
        {
            // Published STATUS message.

            /* Index of the published entry in the local container
            ** table.
            */
            uint16_t    entry_idx;
            entry_idx           = rtb_model_msg->content.status.entry_idx;

            // Number of entries in the local container table.
            uint16_t    nb_exposed_entries;
            nb_exposed_entries  = local_container_table_get_nb_entries();

            // Check to avoid underflow...
            LUOS_ASSERT(nb_exposed_entries > 0);

            /* Index of the last entry in the local container table:
            ** number of entries - 1, since indexes start at 0.
            */
            uint16_t    last_entry_idx;
            last_entry_idx      = nb_exposed_entries - 1;

            if (entry_idx == last_entry_idx)
            {
                // Currently exposed entry is the last exposed entry.
                return true;
            }
        }
    }

    return false;
}

static void mesh_tx_complete_event_cb(const nrf_mesh_evt_t* event)
{
    switch (event->type)
    {
    case NRF_MESH_EVT_TX_COMPLETE:
    {
        // Token identifying the completed transaction.
        nrf_mesh_tx_token_t token   = event->params.tx_complete.token;

        if (token == s_curr_tx_token)
        {
            // Completed transaction is the last sent message.

            // Reset current transaction token.
            s_curr_tx_token = DEFAULT_STATIC_TOKEN;

            // Last sent message.
            tx_queue_elm_t* sent_elm        = luos_mesh_msg_queue_peek();

            // Check result.
            LUOS_ASSERT(sent_elm != NULL);

            // Check if sent message was the last exposed entry.
            bool            is_last_entry   = is_last_published_rtb_entry(sent_elm);
            if (is_last_entry)
            {
                /* Signal RTB publication end to Luos RTB model
                ** management module.
                */
                app_luos_rtb_model_publication_end();
            }

            // Destroy last message, as it is not needed anymore.
            luos_mesh_msg_queue_pop();

            ret_code_t      err_code;

            /* SAR session not being freed, next message has to be sent
            ** later: start timer to that effect.
            */
            err_code        = app_timer_start(s_timer_to_send,
                                              WAIT_TIME_TICKS,
                                              NULL);
            APP_ERROR_CHECK(err_code);
        }
    }
        break;

    default:
        // Nothing to do.
        return;
    }
}

static void timer_to_send_event_cb(void* context)
{
    // It is now possible to send a new message.
    s_is_possible_to_send = true;

    // Try sending a new message.
    send_mesh_msg();
}
