#ifndef MESH_PROVISIONER_H
#define MESH_PROVISIONER_H

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>    // bool

// LUOS
#include "luos_list.h"  // *_MOD

/*      DEFINES                                                     */

// Type and default alias for the Mesh Provisioner container.
#define MESH_PROVISIONER_TYPE   STATE_MOD
#define MESH_PROVISIONER_ALIAS  "mesh_prov"

/* Initializes and starts the Mesh stack, and initializes the low-level
** container.
*/
void MeshProvisioner_Init(void);

// Enables or disables provisioning depending on state and user request.
void MeshProvisioner_Loop(void);

#endif /* ! MESH_PROVISIONER_H */
