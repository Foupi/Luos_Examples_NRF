#ifndef LUOS_MSG_MODEL_COMMON_H
#define LUOS_MSG_MODEL_COMMON_H

/*      INCLUDES                                                    */

// MESH SDK
#include "access.h"             // ACCESS_*_VENDOR

// CUSTOM
#include "luos_mesh_common.h"   // ACCESS_COMPANY_ID_LUOS

/*      DEFINES                                                     */

// Luos MSG model ID (among Luos models).
#define LUOS_MSG_MODEL_ID                           0x0001

// Luos MSG model complete access ID (combined with Luos company ID).
#define LUOS_MSG_MODEL_ACCESS_ID                    \
    ACCESS_MODEL_VENDOR(LUOS_MSG_MODEL_ID, ACCESS_COMPANY_ID_LUOS)

// Luos MSG model SET opcode (IDs among Luos opcodes).
#define LUOS_MSG_MODEL_SET_OPCODE                   0xc2

/* Luos MSG model SET complete access opcode (combined with Luos company
** ID).
*/
#define LUOS_MSG_MODEL_SET_ACCESS_OPCODE            \
    ACCESS_OPCODE_VENDOR(LUOS_MSG_MODEL_SET_OPCODE, \
                         ACCESS_COMPANY_ID_LUOS)

// Index of the element hosting the Luos MSG model instance.
#define LUOS_MSG_MODEL_ELM_IDX                      0

#endif /* ! LUOS_MSG_MODEL_COMMON_H */
