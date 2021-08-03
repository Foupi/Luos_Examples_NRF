#ifndef LUOS_RTB_MODEL_COMMON_H
#define LUOS_RTB_MODEL_COMMON_H

/*      INCLUDES                                                    */

// MESH SDK
#include "access.h"             // ACCESS_*_VENDOR

// CUSTOM
#include "luos_mesh_common.h"   // ACCESS_COMPANY_ID_LUOS

/*      DEFINES                                                     */

// Luos RTB model ID.
#define LUOS_RTB_MODEL_ID                               0x0000

// Luos RTB model complete access ID.
#define LUOS_RTB_MODEL_ACCESS_ID                        \
    ACCESS_MODEL_VENDOR(LUOS_RTB_MODEL_ID, ACCESS_COMPANY_ID_LUOS)

// Luos RTB model opcodes.
#define LUOS_RTB_MODEL_GET_OPCODE                       0xc0
#define LUOS_RTB_MODEL_STATUS_OPCODE                    0xc1

// Luos RTB model complete access opcodes.
#define LUOS_RTB_MODEL_GET_ACCESS_OPCODE                \
    ACCESS_OPCODE_VENDOR(LUOS_RTB_MODEL_GET_OPCODE,     \
                         ACCESS_COMPANY_ID_LUOS)
#define LUOS_RTB_MODEL_STATUS_ACCESS_OPCODE             \
    ACCESS_OPCODE_VENDOR(LUOS_RTB_MODEL_STATUS_OPCODE,  \
                         ACCESS_COMPANY_ID_LUOS)

// Index of the element hosting the Luos RTB model instance.
#define LUOS_RTB_MODEL_ELM_IDX                          0

// Maximum number of exposed RTB entries for a Luos network.
#define LUOS_RTB_MODEL_MAX_RTB_ENTRY                    5

#endif /* ! LUOS_RTB_MODEL_COMMON_H */
