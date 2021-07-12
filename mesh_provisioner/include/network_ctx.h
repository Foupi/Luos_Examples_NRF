#ifndef NETWORK_CTX_H
#define NETWORK_CTX_H

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>                 // uint8_t

// MESH SDK
#include "device_state_manager.h"   // dsm_handle_t

/*      TYPEDEFS                                                    */

// Network context: keys and handles.
typedef struct
{
    // Key handles.
    dsm_handle_t    netkey_handle;
    dsm_handle_t    appkey_handle;
    dsm_handle_t    self_devkey_handle;

    // Keys values.
    uint8_t         netkey[NRF_MESH_KEY_SIZE];
    uint8_t         appkey[NRF_MESH_KEY_SIZE];
    uint8_t         self_devkey[NRF_MESH_KEY_SIZE];

} network_ctx_t;

// Initializes network context (generating or fetching persistent data).
void network_ctx_init(void);

// Global network context.
extern network_ctx_t    g_network_ctx;

#endif /* ! NETWORK_CTX_H */
