#ifndef LUOS_MESH_COMMON_H
#define LUOS_MESH_COMMON_H

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>                // bool

// MESH SDK
#include "mesh_stack.h"             // mesh_stack_models_init_cb_t

// MESH MODELS
#include "config_server_events.h"   // config_server_evt_cb_t

/* Initializes the Mesh stack with predefined parameters and the given
** callbacks, and describes in the given boolean if the device is
** configured or not.
*/
void _mesh_init(config_server_evt_cb_t cfg_srv_cb,
                mesh_stack_models_init_cb_t models_init_cb,
                bool* device_provisioned);

#endif /* LUOS_MESH_COMMON_H */
