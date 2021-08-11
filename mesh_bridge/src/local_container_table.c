#include "local_container_table.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>                // bool
#include <stdint.h>                 // uint16_t

// LUOS
#include "routing_table.h"          // routing_table_t

// CUSTOM
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

uint16_t local_container_table_fill(void)
{
    // FIXME
    return 0;
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

void local_container_table_update_local_ids(void)
{
    // FIXME for loop
}
