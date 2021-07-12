#ifndef NODE_CONFIG_H
#define NODE_CONFIG_H

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>                 // uint16_t

// MESH SDK
#include "device_state_manager.h"   // dsm_handle_t

/* Sets up configuration context and starts configuration process for
** the given values.
*/
void node_config_start(uint16_t device_first_addr,
                       dsm_handle_t devkey_handle,
                       dsm_handle_t address_handle);

#endif /* ! NODE_CONFIG_H */
