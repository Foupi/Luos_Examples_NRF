#ifndef NODE_CONFIG_H
#define NODE_CONFIG_H

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>                 // uint16_t

// MESH SDK
#include "device_state_manager.h"   // dsm_handle_t

// MESH MODELS
#include "config_client.h"          // config_client_*

// LUOS
#include "luos.h"                   // container_t

// Initializes the internal container to send messages.
void node_config_container_set(container_t* container);

/* Sets up configuration context and starts configuration process for
** the given values.
*/
void node_config_start(uint16_t device_first_addr,
                       dsm_handle_t devkey_handle,
                       dsm_handle_t address_handle);

// Handles the received message based on the current context.
void config_client_msg_handler(const config_client_event_t* event);

#endif /* ! NODE_CONFIG_H */
