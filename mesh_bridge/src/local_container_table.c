#include "local_container_table.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>                // bool
#include <stdint.h>                 // uint16_t
#include <string.h>                 // memcpy

// LUOS
#include "config.h"                 // MAX_ALIAS_SIZE
#include "routing_table.h"          // routing_table_t

// CUSTOM
#include "mesh_bridge.h"            // MESH_BRIDGE_MOD
#include "luos_rtb_model_common.h"  // LUOS_RTB_MODEL_MAX_RTB_ENTRY

/*      STATIC VARIABLES & CONSTANTS                                */

// The internal table of local containers.
static struct
{
    // Number of entries contained in the local containers table.
    uint16_t            nb_local_containers;

    // Array of local containers;
    local_container_t   local_containers[LUOS_RTB_MODEL_MAX_RTB_ENTRY];

} s_local_container_table   = { 0 };

/*      STATIC FUNCTIONS                                            */

// Returns true if the entry must be stored in the local table.
static bool is_entry_to_store(routing_table_t* entry);

uint16_t local_container_table_fill(void)
{
    routing_table_t*    local_rtb           = RoutingTB_Get();
    uint16_t            nb_local_entries    = RoutingTB_GetLastEntry();

    if (nb_local_entries == 0)
    {
        // Detection was not performed yet.
        return 0;
    }

    for (uint16_t entry_idx = 0; entry_idx < nb_local_entries;
         entry_idx++)
    {
        routing_table_t*    entry       = local_rtb + entry_idx;

        if (entry->mode != CONTAINER)
        {
            // Only export container entries.
            continue;
        }

        bool must_store = is_entry_to_store(entry);
        if (!must_store)
        {
            continue;
        }

        local_container_t*  local_entry;
        local_entry = s_local_container_table.local_containers + s_local_container_table.nb_local_containers;

        local_entry->local_id           = entry->id;
        local_entry->exposed_entry.mode = CONTAINER;
        local_entry->exposed_entry.id   = s_local_container_table.nb_local_containers;
        local_entry->exposed_entry.type = entry->type;
        memcpy(local_entry->exposed_entry.alias, entry->alias,
               MAX_ALIAS_SIZE * sizeof(char));

        s_local_container_table.nb_local_containers++;
        if (s_local_container_table.nb_local_containers >= LUOS_RTB_MODEL_MAX_RTB_ENTRY)
        {
            // Do not store more RTB entries.
            break;
        }
    }

    return s_local_container_table.nb_local_containers;
}

uint16_t local_container_table_get_nb_entries(void)
{
    return s_local_container_table.nb_local_containers;
}

routing_table_t* local_container_table_get_entry(uint16_t entry_idx)
{
    if (entry_idx >= s_local_container_table.nb_local_containers)
    {
        // Invalid index.
        return NULL;
    }

    local_container_t*  local_container = s_local_container_table.local_containers + entry_idx;
    return &(local_container->exposed_entry);
}

void local_container_table_update_local_ids(uint16_t bridge_id)
{
    for (uint16_t entry_idx = 0;
         entry_idx < s_local_container_table.nb_local_containers;
         entry_idx++)
    {
        local_container_t*  entry;
        uint16_t            new_id;

        entry   = s_local_container_table.local_containers + entry_idx;
        new_id  = RoutingTB_FindFutureContainerID(entry->local_id,
                                                  bridge_id);

        entry->local_id = new_id;
    }
}

static bool is_entry_to_store(routing_table_t* entry)
{
    /* This format might not be the shortest but adding conditions is
    ** easier this way...
    */

    if (entry->type == MESH_BRIDGE_MOD)
    {
        // Do not export Mesh Bridge container.
        return false;
    }

    // Add conditions.

    return true;
}
