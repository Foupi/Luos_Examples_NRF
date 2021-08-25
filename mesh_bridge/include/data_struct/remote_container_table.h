#ifndef REMOTE_CONTAINER_TABLE_H
#define REMOTE_CONTAINER_TABLE_H

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>                // bool
#include <stdint.h>                 // uint16_t

// LUOS
#include "routing_table.h"          // routing_table_t

// CUSTOM
#include "luos_mesh_common.h"       // LUOS_MESH_NETWORK_MAX_NODES
#include "luos_rtb_model_common.h"  // LUOS_RTB_MODEL_MAX_RTB_ENTRY

/*      DEFINES                                                     */

/* Maximum number of remote container entries in the remote containers
** table: defined on max exposed entry by node multiplied by max number
** of other nodes.
*/
#define REMOTE_CONTAINER_TABLE_MAX_NB_ENTRIES   \
    ((LUOS_MESH_NETWORK_MAX_NODES - 1) * LUOS_RTB_MODEL_MAX_RTB_ENTRY)

/*      TYPEDEFS                                                    */

// Information regarding a remote container.
typedef struct
{
    // Exposed routing table entry.
    routing_table_t remote_rtb_entry;

    // Unicast address of the Mesh node hosting the remote container.
    uint16_t        node_addr;

    // ID of the remote container instance in the local RTB.
    uint16_t        local_id;

    // Instance of the remote container on the local system.
    container_t*    local_instance;

} remote_container_t;

/* Adds a remote routing table entry to the remote container table:
** returns true if the insertion succeeded, false otherwise.
*/
bool remote_container_table_add_entry(uint16_t node_address,
                                      const routing_table_t* entry);

// Clears the remote container table.
void remote_container_table_clear(void);

/* Removes the entries corresponding to containers hosted by the given
** unicast address.
*/
void remote_container_table_clear_address(uint16_t node_address);

// FIXME A function could be added to reset number of local nodes.

/* FIXME All search functions could be refactored in one iterator and a
** bool function...
*/

/* Returns the remote container entry corresponding to the given local
** ID, or NULL if it does not exist.
*/
remote_container_t* remote_container_table_get_entry_from_local_id(uint16_t local_id);

/* Returns the remote container entry corresponding to the given unicast
** address and remote ID, or NULL if it does not exist.
*/
remote_container_t* remote_container_table_get_entry_from_addr_and_remote_id(
    uint16_t unicast_addr, uint16_t remote_id
);

// Updates the local IDs of each entry in the remote container table.
void remote_container_table_update_local_ids(uint16_t dtx_container_id);

// Displays the entries in the remote container table.
void remote_container_table_print(void);

#endif /* ! REMOTE_CONTAINER_TABLE_H */
