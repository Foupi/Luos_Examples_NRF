#ifndef REMOTE_CONTAINER_TABLE_H
#define REMOTE_CONTAINER_TABLE_H

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>        // bool
#include <stdint.h>         // uint16_t

// LUOS
#include "routing_table.h"  // routing_table_t

/*      TYPEDEFS                                                    */

// Information regarding a remote container.
typedef struct
{
    // Remote routing table entry.
    routing_table_t remote_rtb_entry;

    // Unicast address of the Mesh node hosting the remote container.
    uint16_t        node_addr;

    // ID of the remote container in the local RTB.
    uint16_t        local_id;

    // Instance of the remote container on the local system.
    container_t*    local_instance;

} remote_container_t;

/* Adds a remote routing table entry to the remote containers map:
** returns true if the insertion succeeded, false otherwise.
*/
bool remote_container_table_add_entry(uint16_t node_address,
                                      const routing_table_t* entry);

// Clears the remote containers table.
void remote_container_table_clear(void);

/* Removes the entries corresponding to containers hosted by the given
** unicast address.
*/
void remote_container_table_clear_address(uint16_t node_address);

/* Returns the remote container entry corresponding to the given local
** ID, or NULL if it does not exist.
*/
remote_container_t* remote_container_table_get_entry_from_local_id(uint16_t local_id);

/* Returns the remote container entry corresponding to the given unicast
** address and remote ID, or NULL if it does not exist.
*/
remote_container_t* remote_container_table_get_entry_from_addr_and_remote_id(uint16_t unicast_addr,
    uint16_t remote_id);

// Displays the entries in the remote container table.
void remote_container_table_print(void);

#endif /* ! REMOTE_CONTAINER_TABLE_H */
