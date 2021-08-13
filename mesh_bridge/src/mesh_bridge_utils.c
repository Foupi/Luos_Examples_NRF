#include "mesh_bridge_utils.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>         // uint16_t

// LUOS
#include "routing_table.h"  // routing_table_t, entry_mode_t

// CUSTOM
#include "mesh_bridge.h"    // MESH_BRIDGE_MOD

/*      STATIC FUNCTIONS                                            */

// Finds the number of containers in the given node RTB entry.
static uint16_t find_nb_containers(routing_table_t* node_entry);

uint16_t find_mesh_bridge_container_id(routing_table_t* routing_table,
                                       uint16_t nb_entries)
{
    for (uint16_t entry_idx = 0; entry_idx < nb_entries; entry_idx++)
    {
        routing_table_t entry   = routing_table[entry_idx];

        if (entry.mode == CONTAINER && entry.type == MESH_BRIDGE_MOD)
        {
            return entry.id;
        }
    }

    // Not found.
    return 0;
}

uint16_t find_mesh_bridge_node_id(routing_table_t* routing_table,
                                  uint16_t nb_entries)
{
    for (uint16_t entry_idx = 0; entry_idx < nb_entries; entry_idx++)
    {
        routing_table_t*    entry           = routing_table + entry_idx;

        if (entry->mode != NODE)
        {
            continue;
        }

        uint16_t            nb_containers   = find_nb_containers(entry);

        // Find Mesh Bridge ID in node.
        uint16_t            mesh_bridge_id  = find_mesh_bridge_container_id(entry,
                                                nb_containers);
        if (mesh_bridge_id != 0)
        {
            // Mesh Bridge container was found in the node.
            return entry->node_id;
        }
    }

    // Not found.
    return 0;
}

static uint16_t find_nb_containers(routing_table_t* node_entry)
{
    uint16_t            nb_containers           = 0;
    routing_table_t*    first_container_entry   = node_entry + 1;

    while (first_container_entry[nb_containers].mode == CONTAINER)
    {
        nb_containers++;
    }

    return nb_containers;
}
