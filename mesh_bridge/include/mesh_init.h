#ifndef MESH_INIT_H
#define MESH_INIT_H

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>    // bool
#include <stdint.h>     // uint16_t

// Initializes the Mesh stack with predefined parameters.
void mesh_init(void);

/* Sets the element addresses of the internal model instances according
** to the given device address.
*/
void mesh_models_set_addresses(uint16_t device_address);

// Sends a RTB GET request to remote devices.
void mesh_rtb_get(void);

// Describes if the device is provisioned.
extern bool g_device_provisioned;

#endif /* ! MESH_INIT_H */
