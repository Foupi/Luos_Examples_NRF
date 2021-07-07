#ifndef LUOS_MESH_COMMON_H
#define LUOS_MESH_COMMON_H

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>                // bool

// MESH SDK
#include "mesh_stack.h"             // mesh_stack_models_init_cb_t
#include "nrf_mesh_prov.h"          // nrf_mesh_prov_ctx_t
#include "nrf_mesh_prov_events.h"   // nrf_mesh_prov_evt_handler_cb_t

// MESH MODELS
#include "config_server_events.h"   // config_server_evt_cb_t

/* Initializes the Mesh stack with predefined parameters and the given
** callbacks, and describes in the given boolean if the device is
** configured or not.
*/
void _mesh_init(config_server_evt_cb_t cfg_srv_cb,
                mesh_stack_models_init_cb_t models_init_cb,
                bool* device_provisioned);

/* Initializes the given provisioning context with predefined values and
** the given event handler.
*/
void _provisioning_init(nrf_mesh_prov_ctx_t* prov_ctx,
                        nrf_mesh_prov_evt_handler_cb_t event_handler);

// Generates the public and private encryption keys.
void encryption_keys_generate(void);

#endif /* LUOS_MESH_COMMON_H */
