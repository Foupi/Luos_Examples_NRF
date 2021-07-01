#ifndef MESH_PROVISIONER_H
#define MESH_PROVISIONER_H

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>    // bool

// LUOS
#include "luos_list.h"  // *_MOD

/*      DEFINES                                                     */

// Type of the Mesh Provisioner container.
#define MESH_PROVISIONER_TYPE   VOID_MOD

// Default alias of the Mesh Provisioner container.
#define MESH_PROVISIONER_ALIAS  "mesh_prov"

/* Initializes and starts the Mesh stack, and initializes the low-level
** container.
*/
void MeshProvisioner_Init(void);

// Enables or disables provisioning depending on state and user request.
void MeshProvisioner_Loop(void);

// Describes the expected provisioning scan state.
extern bool g_prov_scan_req;

#endif /* MESH_PROVISIONER_H */
