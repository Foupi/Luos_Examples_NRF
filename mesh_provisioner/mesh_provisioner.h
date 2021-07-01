#ifndef MESH_PROVISIONER_H
#define MESH_PROVISIONER_H

/* Initializes and starts the Mesh stack, and initializes the low-level
** container.
*/
void MeshProvisioner_Init(void);

// Enables or disables provisioning depending on state and user request.
void MeshProvisioner_Loop(void);

#endif /* MESH_PROVISIONER_H */
