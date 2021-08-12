#ifndef LUOS_MESH_MSG_QUEUE_H
#define LUOS_MESH_MSG_QUEUE_H

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>        // bool

// MESH SDK
#include "access.h"         // access_*

// CUSTOM
#include "luos_msg_model.h" // luos_msg_model_*
#include "luos_rtb_model.h" // luos_rtb_model_*

/*      TYPEDEFS                                                    */

typedef enum
{
    // Empty element.
    TX_QUEUE_MODEL_EMPTY             = 0,

    // Luos RTB model.
    TX_QUEUE_MODEL_LUOS_RTB,

    // Luos MSG model.
    TX_QUEUE_MODEL_LUOS_MSG,

} tx_queue_elm_model_t;

typedef enum
{
    // GET message.
    TX_QUEUE_CMD_GET,

    // STATUS message.
    TX_QUEUE_CMD_STATUS,

    // STATUS message in reply to a GET or SET message.
    TX_QUEUE_CMD_STATUS_REPLY,

    // SET message.
    TX_QUEUE_CMD_SET,

} tx_queue_cmd_t;

// Element of the TX queue corresponding to a Luos RTB model message.
typedef struct
{
    // Corresponding command.
    tx_queue_cmd_t  cmd;

    // Union of message types.
    union
    {
        // Corresponding to a Luos RTB model GET request.
        luos_rtb_model_get_t    get;

        // Corresponding to a Luos RTB model STATUS message.
        luos_rtb_model_status_t status;

        // Corresponding to a Luos RTB model STATUS reply.
        struct
        {
            // Received message.
            access_message_rx_t     src_msg;

            // Replied status.
            luos_rtb_model_status_t status;

        }                       status_reply;

    }               content;

} tx_queue_luos_rtb_model_elm_t;

// Element of the TX queue corresponding to a Luos RTB model message.
typedef struct
{
    // Corresponding command.
    tx_queue_cmd_t  cmd;

    // Union of message types.
    union
    {
        // Corresponding to a Luos MSG SET command.
        luos_msg_model_set_t    set;

    }               content;

} tx_queue_luos_msg_model_elm_t;

// Element of the TX queue.
typedef struct
{
    // Corresponding model.
    tx_queue_elm_model_t    model;

    access_model_handle_t   model_handle;

    // Union of model messages.
    union
    {
        // Content corresponding to a Luos RTB model message.
        tx_queue_luos_rtb_model_elm_t   luos_rtb_model_msg;

        // Content corresponding to a Luos MSG model message.
        tx_queue_luos_msg_model_elm_t   luos_msg_model_msg;

    }                       content;

} tx_queue_elm_t;

/* Stores the given element in the internal message queue. Returns false
** if the queue is full, true otherwise.
*/
bool luos_mesh_msg_queue_enqueue(const tx_queue_elm_t* elm);

// Returns the last queue element, or NULL if the queue is empty.
tx_queue_elm_t* luos_mesh_msg_queue_peek(void);

// Pops the last queue element from the queue.
void luos_mesh_msg_queue_pop(void);

#endif /* ! LUOS_MESH_MSG_QUEUE_H */
