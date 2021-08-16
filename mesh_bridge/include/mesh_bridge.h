#ifndef MESH_BRIDGE_H
#define MESH_BRIDGE_H

/*      INCLUDES                                                    */

// LUOS
#include "luos_list.h"      // LUOS_PROTOCOL_NB, LUOS_LAST_TYPE

// CUSTOM
#include "app_luos_list.h"  // MESH_BRIDGE_*

/*      DEFINES                                                     */

// Index of Mesh Bridge type among Luos types.
#ifndef MESH_BRIDGE_MOD
#define MESH_BRIDGE_MOD         LUOS_LAST_TYPE
#endif /* ! MESH_BRIDGE_MOD */

// Start index of Mesh Bridge messages.
#ifndef MESH_BRIDGE_MSG_BEGIN
#define MESH_BRIDGE_MSG_BEGIN   LUOS_PROTOCOL_NB
#endif /* ! MESH_BRIDGE_MSG_BEGIN */

// Type and default alias for the Mesh Bridge container.
#define MESH_BRIDGE_TYPE        MESH_BRIDGE_MOD
#define MESH_BRIDGE_ALIAS       "mesh_bridge"

/*      TYPEDEFS                                                    */

typedef enum
{
    // Received commands:
    // Request for extended RTB procedure start.
    MESH_BRIDGE_EXT_RTB_CMD = MESH_BRIDGE_MSG_BEGIN,

    // Request to print internal remote and local container tables.
    MESH_BRIDGE_PRINT_INTERNAL_TABLES,

    // Request to reset internal remote and local container tables.
    MESH_BRIDGE_RESET_INTERNAL_TABLES,

    /* Request to update the local IDs of each entry of the internal
    ** remote and local container tables.
    */
    MESH_BRIDGE_UPDATE_INTERNAL_TABLES,

    // Sent messages:
    // Extended RTB procedure complete.
    MESH_BRIDGE_EXT_RTB_COMPLETE,

    // Start index for next messages.
    MESH_BRIDDGE_MSG_END,

} mesh_bridge_msg_t;

/* Initializes and starts the Mesh stack, and initializes the low-level
** container, then starts listening for a provisioning link.
*/
void MeshBridge_Init(void);

// Does nothing.
void MeshBridge_Loop(void);

#endif /* MESH_BRIDGE_H */
