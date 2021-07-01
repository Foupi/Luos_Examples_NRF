#ifndef MESH_PROVISIONER_H
#define MESH_PROVISIONER_H

/*      INCLUDES                                                    */

// LUOS
#include "luos_list.h"  // STATE_MOD

/*      DEFINES                                                     */

// Type of the Mesh Provisioner container.
#define MESH_PROVISIONER_TYPE   STATE_MOD

// Default alias of the Mesh Provisioner container.
#define MESH_PROVISIONER_ALIAS  "mesh_prov"

/* Initializes and starts the Mesh stack, and initializes the low-level
** container.
*/
void MeshProvisioner_Init(void);

// Enables or disables provisioning depending on state and user request.
void MeshProvisioner_Loop(void);

#endif /* MESH_PROVISIONER_H */
