#include "luos_mesh_msg_queue.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>        // bool
#include <string.h>         // memcpy

// CUSTOM
#include "luos_rtb_model.h" // luos_rtb_model_*

/*      STATIC VARIABLES & CONSTANTS                                */

// Max queue size.
#define MSG_QUEUE_MAX_SIZE  8

// The message queue.
static tx_queue_elm_t   s_msg_queue[MSG_QUEUE_MAX_SIZE] = { 0 };

// Insertion index in the queue.
static uint16_t         s_msg_queue_insertion_index     = 0;

// Peek/pop index in the queue.
static uint16_t         s_msg_queue_peek_index          = 0;

bool luos_mesh_msg_queue_enqueue(const tx_queue_elm_t* elm)
{
    tx_queue_elm_t* insert_spot = s_msg_queue + s_msg_queue_insertion_index;
    if (insert_spot->model != TX_QUEUE_MODEL_EMPTY)
    {
        // Insertion spot is occupied: queue is full!
        return false;
    }

    memcpy(insert_spot, elm, sizeof(tx_queue_elm_t));

    s_msg_queue_insertion_index++;
    s_msg_queue_insertion_index %= MSG_QUEUE_MAX_SIZE;

    return true;
}

tx_queue_elm_t* luos_mesh_msg_queue_peek(void)
{
    tx_queue_elm_t* peek_spot   = s_msg_queue + s_msg_queue_peek_index;

    if (peek_spot->model == TX_QUEUE_MODEL_EMPTY)
    {
        // Peek spot is empty: queue is empty.
        return NULL;
    }

    return peek_spot;
}

void luos_mesh_msg_queue_pop(void)
{
    // Just mark the last index as empty.
    s_msg_queue[s_msg_queue_peek_index].model   = TX_QUEUE_MODEL_EMPTY;

    s_msg_queue_peek_index++;
    s_msg_queue_peek_index %= MSG_QUEUE_MAX_SIZE;
}
