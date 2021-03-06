#ifndef LOCAL_CONTAINER_TABLE_H
#define LOCAL_CONTAINER_TABLE_H

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>        // bool
#include <stdint.h>         // uint16_t

// LUOS
#include "routing_table.h"  // routing_table_t

/*      TYPEDEFS                                                    */

// Information regarding a local container.
typedef struct
{
    // RTB entry exposed to remote nodes.
    routing_table_t exposed_entry;

    // Local RTB ID of the container.
    uint16_t        local_id;

} local_container_t;

/* Fills the local container table, and returns its new number of local
** entries.
*/
uint16_t local_container_table_fill(void);

// Returns the number of local container entries.
uint16_t local_container_table_get_nb_entries(void);

// Returns the entry located at the given index, or NULL if it is empty.
routing_table_t* local_container_table_get_entry_from_idx(uint16_t entry_idx);

/* FIXME All search functions could be refactored in one iterator and a
** bool function...
*/

/* Returns the entry corresponding to the given local ID, or NULL if it
** is not found.
*/
routing_table_t* local_container_table_get_entry_from_local_id(uint16_t id);

/* Returns the entry corresponding to the given exposed ID, or NULL if
** it is not found.
*/
local_container_t* local_container_table_get_entry_from_exposed_id(uint16_t id);

// Updates the local IDs of each entry in the local container table.
void local_container_table_update_local_ids(uint16_t dtx_container_id);

// Clears the local container table.
void local_container_table_clear(void);

// Displays the entries in the local container table.
void local_container_table_print(void);

#endif /* ! LOCAL_CONTAINER_TABLE_H */
