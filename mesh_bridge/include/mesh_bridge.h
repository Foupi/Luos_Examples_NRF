#ifndef MESH_BRIDGE_H
#define MESH_BRIDGE_H

/* Initializes and starts the Mesh stack, and initializes the low-level
** container, then starts listening for a provisioning link.
*/
void MeshBridge_Init(void);

// FIXME Does nothing for now.
void MeshBridge_Loop(void);

#endif /* MESH_BRIDGE_H */
