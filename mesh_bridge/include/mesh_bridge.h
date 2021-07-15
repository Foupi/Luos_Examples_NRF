#ifndef MESH_BRIDGE_H
#define MESH_BRIDGE_H

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>    // g_rtb_get_asked

// LUOS
#include "luos_list.h"  // *_MOD

/*      DEFINES                                                     */

// Type of the Mesh Bridge container.
#define MESH_BRIDGE_TYPE    VOID_MOD

// Default alias of the Mesh Bridge container.
#define MESH_BRIDGE_ALIAS   "mesh_bridge"

/* Initializes and starts the Mesh stack, and initializes the low-level
** container, then starts listening for a provisioning link.
*/
void MeshBridge_Init(void);

// FIXME Does nothing for now.
void MeshBridge_Loop(void);

// Describes if a RTB GET request was asked by user.
extern bool g_rtb_get_asked;

#endif /* MESH_BRIDGE_H */
