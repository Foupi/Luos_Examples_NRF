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

#endif /* ! REMOTE_CONTAINER_TABLE_H */
