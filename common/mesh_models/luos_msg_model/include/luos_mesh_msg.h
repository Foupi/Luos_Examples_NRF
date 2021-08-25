#ifndef LUOS_MESH_MSG_H
#define LUOS_MESH_MSG_H

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>             // uint*_t

// MESH SDK
#include "access.h"             // access_opcode_t
#include "nrf_mesh_defines.h"   // NRF_MESH_UNSEG_PAYLOAD_SIZE_MAX

/*      TYPEDEFS                                                    */

// Size of the protocol field in a Luos Mesh message header.
#define LUOS_MESH_MSG_HEADER_PROTOCOL_BITS      1

// Size of the target field in a Luos Mesh message header.
#define LUOS_MESH_MSG_HEADER_TARGET_BITS        3

// Maximum value of the target field in a Luos Mesh message header.
#define LUOS_MESH_MSG_HEADER_TARGET_MAX_VAL     \
    ((1 << LUOS_MESH_MSG_HEADER_TARGET_BITS) - 1)

// Size of the target mode field in a Luos Mesh message header.
#define LUOS_MESH_MSG_HEADER_TARGET_MODE_BITS   1

// Size of the source field in a Luos Mesh message header.
#define LUOS_MESH_MSG_HEADER_SOURCE_BITS        3

// Maximum value of the source field in a Luos Mesh message header.
#define LUOS_MESH_MSG_HEADER_SOURCE_MAX_VAL     \
    ((1 << LUOS_MESH_MSG_HEADER_SOURCE_BITS) - 1)

// Shorter version of a Luos message, to be sent over Mesh.
typedef struct __attribute__((__packed__))
{
    // Protocol version (only 2 versions can be supported...).
    uint8_t protocol    : LUOS_MESH_MSG_HEADER_PROTOCOL_BITS;

    /* Target ID (restricted since a limited amount of exposed
    ** containers is allowed).
    */
    uint8_t target      : LUOS_MESH_MSG_HEADER_TARGET_BITS;

    // Target mode (1 bit: ID or IDACK).
    uint8_t target_mode : LUOS_MESH_MSG_HEADER_TARGET_MODE_BITS;

    /* Source ID (restricted since a limited amount of exposed
    ** containers is allowed).
    */
    uint8_t source      : LUOS_MESH_MSG_HEADER_SOURCE_BITS;

    // Sent command.
    uint8_t cmd;

    // Payload size (restricted due to Mesh throughput limitations).
    uint8_t size;

} luos_mesh_header_t;

/* Luos MSG model SET access opcode size:
**  *   Size of company ID (2 bytes).
**  +   Size of vendor-specific opcodes (1 byte).
*/
#define LUOS_MSG_MODEL_SET_ACCESS_OPCODE_SIZE   \
    (sizeof(uint16_t) + sizeof(uint8_t))

/* Maximum Luos Mesh message payload size in bytes:
**  *   Maximum payload size for an unsegmented Mesh message.
**  -   Size of the opcode (it apparently is counted as part of the
**      payload.)
**  -   Size of additional data in Luos MSG SET command (FIXME a struct
**      should be defined instead of hardcoding an uint16_t...).
**  -   Size of Luos Mesh message header.
*/
#define LUOS_MESH_MSG_MAX_DATA_SIZE         \
    (NRF_MESH_UNSEG_PAYLOAD_SIZE_MAX        \
    - LUOS_MSG_MODEL_SET_ACCESS_OPCODE_SIZE \
    - sizeof(uint16_t)                      \
    - sizeof(luos_mesh_header_t)            \
    )

typedef struct __attribute__((__packed__))
{
    // Message header.
    luos_mesh_header_t  header;

    // Message payload.
    uint8_t             data[LUOS_MESH_MSG_MAX_DATA_SIZE];

} luos_mesh_msg_t;

#endif /* ! LUOS_MESH_MSG_H */
