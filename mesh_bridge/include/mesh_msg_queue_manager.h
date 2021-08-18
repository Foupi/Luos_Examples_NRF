#ifndef MESH_MSG_QUEUE_MANAGER_H
#define MESH_MSG_QUEUE_MANAGER_H

/*      INCLUDES                                                    */

// CUSTOM
#include "luos_mesh_msg_queue.h"    // tx_queue_elm_t

// Adds the TX complete Mesh event callback to the Mesh stack.
void luos_mesh_msg_queue_manager_init(void);

// If a message can be sent, sends the given message; else enqueue it.
void luos_mesh_msg_prepare(const tx_queue_elm_t* message);

#endif /* ! MESH_MSG_QUEUE_MANAGER_H */
