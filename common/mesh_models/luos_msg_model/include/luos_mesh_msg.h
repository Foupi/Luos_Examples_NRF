#ifndef LUOS_MESH_MSG_H
#define LUOS_MESH_MSG_H

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>             // uint*_t

// MESH SDK
#include "nrf_mesh_defines.h"   // NRF_MESH_UNSEG_PAYLOAD_SIZE_MAX

/*      TYPEDEFS                                                    */

// Shorter version of a Luos message, to be sent over Mesh.
typedef struct __attribute__((__packed__))
{
    // Protocol version.
    uint8_t protocol    : 4;

    // Target (restricted since a limited amount of nodes is allowed).
    uint8_t target      : 4;

    // Target mode.
    uint8_t target_mode : 4;

    // Source (restricted since a limited amount of nodes is allowed).
    uint8_t source      : 4;

    // Sent command.
    uint8_t cmd;

    // Payload size (restricted due to Mesh throughput limitations).
    uint8_t size;

} luos_mesh_header_t;

// Maximum payload size in bytes.
// FIXME Define struct for Luos MSG SET non-message part.
#define LUOS_MESH_MSG_MAX_DATA_SIZE (NRF_MESH_UNSEG_PAYLOAD_SIZE_MAX - sizeof(luos_mesh_header_t) - (sizeof(uint16_t)))

typedef struct __attribute__((__packed__))
{
    // Message header.
    luos_mesh_header_t  header;

    // Message payload.
    uint8_t             data[LUOS_MESH_MSG_MAX_DATA_SIZE];

} luos_mesh_msg_t;

#endif /* ! LUOS_MESH_MSG_H */
