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

/*      DEFINES                                                     */

// Static authentication data (FIXME copied from Mesh examples).
#define LUOS_STATIC_AUTH_DATA                       \
{                                                   \
    0x6E, 0x6F, 0x72, 0x64, 0x69, 0x63, 0x5F, 0x65, \
    0x78, 0x61, 0x6D, 0x70, 0x6C, 0x65, 0x5F, 0x31, \
}

// Luos Company ID. FIXME Find how to generate an unique one.
#define ACCESS_COMPANY_ID_LUOS                      0xCAFE

// Group address for Luos models communication.
#define LUOS_GROUP_ADDRESS                          0xF00D

// Maximum number of nodes in the Luos Mesh network.
#define LUOS_MESH_NETWORK_MAX_NODES                 5

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

// Starts the Mesh stack.
void mesh_start(void);

// Generates the public and private encryption keys.
void encryption_keys_generate(void);

// Provides the Luos static authentication data to the given context.
void auth_data_provide(nrf_mesh_prov_ctx_t* prov_ctx);

#endif /* LUOS_MESH_COMMON_H */
