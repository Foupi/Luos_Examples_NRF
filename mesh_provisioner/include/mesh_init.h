#ifndef MESH_INIT_H
#define MESH_INIT_H

/*      INCLUDES                                                    */

#include <stdbool.h>    // bool

// Initializes the Mesh stack with predefined parameters.
void mesh_init(void);

// Describes if the device is provisioned.
extern bool g_device_provisioned;

#endif /* ! MESH_INIT_H */
