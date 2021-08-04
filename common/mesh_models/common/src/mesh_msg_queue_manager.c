#include "mesh_msg_queue_manager.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>                // bool

// CUSTOM
#include "luos_mesh_msg_queue.h"    // tx_queue_elm_t

/*      STATIC VARIABLES & CONSTANTS                                */

// Describes if a message can be sent.
static bool s_is_possible_to_send   = true;

/*      CALLBACKS                                                   */

/*      INITIALIZATIONS                                             */

void luos_mesh_msg_queue_manager_init(void)
{

}

void luos_mesh_msg_prepare(const tx_queue_elm_t* message)
{

}
