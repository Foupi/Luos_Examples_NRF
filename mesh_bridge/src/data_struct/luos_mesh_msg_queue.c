#include "luos_mesh_msg_queue.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>                // bool
#include <string.h>                 // memcpy

// LUOS
#include "luos_utils.h"             // LUOS_ASSERT

// CUSTOM
#include "luos_msg_model.h"         // luos_msg_model_*
#include "luos_rtb_model.h"         // luos_rtb_model_*
#include "remote_container_table.h" // REMOTE_CONTAINER_TABLE_MAX_NB_ENTRIES

/*      STATIC VARIABLES & CONSTANTS                                */

// Max queue size: at most one message for each remote container.
#define MSG_QUEUE_MAX_SIZE  REMOTE_CONTAINER_TABLE_MAX_NB_ENTRIES

// The message queue.
static struct
{
    // Insertion index in the queue.
    uint16_t        insertion_index;

    // Peek/pop index in the queue.
    uint16_t        peek_index;

    tx_queue_elm_t  elements[MSG_QUEUE_MAX_SIZE];

} s_msg_queue   =
{
    // Queue starts as empty.
    0
};

bool luos_mesh_msg_queue_enqueue(const tx_queue_elm_t* elm)
{
    // Check parameter.
    LUOS_ASSERT(elm != NULL);

    // Insertion spot in the queue.
    tx_queue_elm_t* insert_spot = s_msg_queue.elements + s_msg_queue.insertion_index;

    if (insert_spot->model != TX_QUEUE_MODEL_EMPTY)
    {
        // Insertion spot is occupied: queue is full.
        return false;
    }

    // Copy element in insertion spot.
    memcpy(insert_spot, elm, sizeof(tx_queue_elm_t));

    // Increase insertion index and loop if necessary.
    s_msg_queue.insertion_index++;
    s_msg_queue.insertion_index %= MSG_QUEUE_MAX_SIZE;

    return true;
}

tx_queue_elm_t* luos_mesh_msg_queue_peek(void)
{
    // Peek spot in the queue.
    tx_queue_elm_t* peek_spot;
    peek_spot   = s_msg_queue.elements + s_msg_queue.peek_index;

    if (peek_spot->model == TX_QUEUE_MODEL_EMPTY)
    {
        // Peek spot is empty: queue is empty.
        return NULL;
    }

    return peek_spot;
}

void luos_mesh_msg_queue_pop(void)
{
    // Pop spot in the queue.
    tx_queue_elm_t* pop_spot;
    pop_spot                = s_msg_queue.elements + s_msg_queue.peek_index;

    if (pop_spot->model == TX_QUEUE_MODEL_EMPTY)
    {
        // Pop spot is empty: no need to continue.
        return;
    }

    // Just mark the last index as empty.
    pop_spot->model = TX_QUEUE_MODEL_EMPTY;

    // Increase peek index and loop if necessary.
    s_msg_queue.peek_index++;
    s_msg_queue.peek_index  %= MSG_QUEUE_MAX_SIZE;
}
