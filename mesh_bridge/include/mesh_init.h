#ifndef MESH_INIT_H
#define MESH_INIT_H

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>    // bool

// Initializes the Mesh stack with predefined parameters.
void mesh_init(void);

// Sends a RTB GET request to remote devices.
void mesh_rtb_get(void);

// Describes if the device is provisioned.
extern bool g_device_provisioned;

#endif /* ! MESH_INIT_H */
