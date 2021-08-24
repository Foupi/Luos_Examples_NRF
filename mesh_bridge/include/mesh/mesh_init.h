#ifndef MESH_INIT_H
#define MESH_INIT_H

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>                // bool
#include <stdint.h>                 // uint16_t

// Initializes the Mesh stack with predefined parameters.
void mesh_init(void);

// Fetches the local device address from DSM.
uint16_t mesh_device_get_address(void);

/* Sets the element addresses of the internal model instances according
** to the given device address.
*/
void mesh_models_set_addresses(uint16_t device_address);

// Describes if the device is provisioned.
extern bool g_device_provisioned;

#endif /* ! MESH_INIT_H */
