#ifndef MESH_BRIDGE_UTILS_H
#define MESH_BRIDGE_UTILS_H

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>         // uint16_t

// LUOS
#include "routing_table.h"  // routing_table_t

/* Returns the ID of the Mesh Bridge container in the given routing
** table, or 0 if it is not found.
*/
uint16_t find_mesh_bridge_container_id(routing_table_t* routing_table,
                                       uint16_t nb_entries);

/* Returns the ID of the node hosting the Mesh Bridge container in the
** given routing table, or 0 if it is not found.
*/
uint16_t find_mesh_bridge_node_id(routing_table_t* routing_table,
                                  uint16_t nb_entries);

// Visually show the start and end of Ext-RTB procedure.
void indicate_ext_rtb_engaged(void);
void indicate_ext_rtb_complete(void);

#endif /* ! MESH_BRIDGE_UTILS_H */
