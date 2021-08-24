#include "remote_container_table.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>                // bool
#include <stdint.h>                 // uint16_t
#include <string.h>                 // memcpy, memset

// LUOS
#include "luos.h"                   /* container_t,
                                    ** Luos_CreateContainer, msg_t,
                                    ** revision_t
                                    */
#include "luos_utils.h"             // LUOS_ASSERT
#include "routing_table.h"          // routing_table_t

// CUSTOM
#include "app_luos_msg_model.h"     // app_luos_msg_model_send_msg
#include "local_container_table.h"  // local_container_table_*
#include "mesh_bridge_utils.h"      // find_mesh_bridge_node_id

// NRF
#ifdef DEBUG
#include "nrf_log.h"                // NRF_LOG_INFO
#endif /* DEBUG */

/*      STATIC VARIABLES & CONSTANTS                                */

// The internal table of remote containers.
static struct
{
    // Number of entries contained in the remote containers table.
    uint16_t            nb_remote_containers;

    // Array of remote containers.
    remote_container_t  remote_containers[REMOTE_CONTAINER_TABLE_MAX_NB_ENTRIES];

}               s_remote_container_table    =
{
    // Table starts as empty.
    0
};

#ifndef REV
#define REV {0,0,1}
#endif

// Number of local containers hosted by the Mesh Bridge node.
static uint16_t s_nb_local_containers       = 0;

/*      STATIC FUNCTIONS                                            */

/* Returns the number of non-remote containers in the node hosting the
** Mesh Bridge container.
*/
static uint16_t get_nb_local_containers_in_mesh_bridge_node(void);

/*      CALLBACKS                                                   */

// Sends message through Luos MSG Mesh model.
static void RemoteContainer_MsgHandler(container_t* container,
                                       msg_t* msg);

bool remote_container_table_add_entry(uint16_t node_address,
                                      const routing_table_t* entry)
{
    // Check parameter.
    LUOS_ASSERT(entry != NULL);

    if (s_remote_container_table.nb_remote_containers >= REMOTE_CONTAINER_TABLE_MAX_NB_ENTRIES)
    {
        // Table is full: insertion is not possible.
        return false;
    }

    /* Since containers are stacked, the first index available for a
    ** remote container entry is the last local container index + 1;
    ** which is why we need to compute the number of local containers.
    */
    if (s_nb_local_containers == 0)
    {
        // Number has not been computed yet: compute it.
        s_nb_local_containers = get_nb_local_containers_in_mesh_bridge_node();
    }

    // Since container indexes start at 1.
    uint16_t            first_available_id  = s_nb_local_containers + 1;

    revision_t          revision            = { .unmap = REV };

    // Instance of remote container on local network.
    container_t*        local_instance;
    local_instance  = Luos_CreateContainer(RemoteContainer_MsgHandler,
                                           entry->type, entry->alias,
                                           revision);

    // ID of remote container on local routing table.
    uint16_t            local_id;
    local_id        = first_available_id + s_remote_container_table.nb_remote_containers;

    // Current remote container table entry.
    remote_container_t* insertion_entry;
    insertion_entry = s_remote_container_table.remote_containers + s_remote_container_table.nb_remote_containers;

    // Fill insertion spot.
    insertion_entry->node_addr      = node_address;
    insertion_entry->local_id       = local_id;
    insertion_entry->local_instance = local_instance;
    memcpy(&(insertion_entry->remote_rtb_entry), entry,
           sizeof(routing_table_t));

    // Increase number of remote containers.
    s_remote_container_table.nb_remote_containers++;

    return true;
}

void remote_container_table_clear(void)
{
    /* Loop is necessary to destroy all local instances of remote
    ** containers.
    */
    for (uint16_t entry_idx = 0;
         entry_idx < s_remote_container_table.nb_remote_containers;
         entry_idx++)
    {
        remote_container_t  curr_entry;
        curr_entry  = s_remote_container_table.remote_containers[entry_idx];

        Luos_DestroyContainer(curr_entry.local_instance);
    }

    // Empty table.
    memset(&s_remote_container_table, 0,
           sizeof(s_remote_container_table));

    /* Reset number of local containers: table clear might mean new
    ** local containers will be added.
    */
    s_nb_local_containers = 0;
}

void remote_container_table_clear_address(uint16_t node_address)
{
    uint16_t    entry_idx   = 0;

    // While loop as upper bound is not constant.
    while (entry_idx < s_remote_container_table.nb_remote_containers)
    {
        remote_container_t  curr_entry;
        curr_entry = s_remote_container_table.remote_containers[entry_idx];

        if (curr_entry.node_addr != node_address)
        {
            // Not an entry to erase: continue.
            entry_idx++;
            continue;
        }

        // Destroy local instance of remote container entry.
        Luos_DestroyContainer(curr_entry.local_instance);

        /* Loop to shift next containers at index - 1. As source and
        ** destination memory zones overlap, a `memcpy` cannot be
        ** executed here...
        */
        uint16_t    last_entry_idx = s_remote_container_table.nb_remote_containers - 1;
        for (uint16_t replacement_idx = entry_idx;
             replacement_idx < last_entry_idx; replacement_idx++)
        {
            // The current entry to replace by its successor.
            remote_container_t* replaced_entry;
            replaced_entry  = s_remote_container_table.remote_containers + replacement_idx;
            // The entry replacing the previous one.
            remote_container_t* replacing_entry;
            replacing_entry = replaced_entry + 1;

            /* The local ID is always decreased, as containers are
            ** stacked in the same order in the remote container table
            ** as they are in the context container table.
            */
            replacing_entry->local_id--;

            // Rewrite replaced entry.
            memcpy(replaced_entry, replacing_entry, sizeof(remote_container_t));
        }

        // Decrease number of remote containers.
        s_remote_container_table.nb_remote_containers--;
    }
}

remote_container_t* remote_container_table_get_entry_from_local_id(uint16_t local_id)
{
    for (uint16_t entry_idx = 0;
         entry_idx < s_remote_container_table.nb_remote_containers;
         entry_idx++)
    {
        remote_container_t* entry   = s_remote_container_table.remote_containers + entry_idx;

        if (entry->local_id == local_id)
        {
            return entry;
        }
    }

    return NULL;
}

remote_container_t* remote_container_table_get_entry_from_addr_and_remote_id(uint16_t unicast_addr,
    uint16_t remote_id)
{
    for (uint16_t entry_idx = 0;
         entry_idx < s_remote_container_table.nb_remote_containers;
         entry_idx++)
    {
        remote_container_t* entry   = s_remote_container_table.remote_containers + entry_idx;

        if (entry->node_addr == unicast_addr && entry->remote_rtb_entry.id == remote_id)
        {
            return entry;
        }
    }

    return NULL;
}

void remote_container_table_update_local_ids(uint16_t dtx_container_id)
{
    for (uint16_t entry_idx = 0;
         entry_idx < s_remote_container_table.nb_remote_containers;
         entry_idx++)
    {
        remote_container_t* entry;
        uint16_t            new_id;

        entry           = s_remote_container_table.remote_containers + entry_idx;

        // Compute next local ID after detection by given ID.
        new_id          = RoutingTB_FindFutureContainerID(
                            entry->local_id,
                            dtx_container_id
                          );

        // Update local ID.
        entry->local_id = new_id;
    }
}

void remote_container_table_print(void)
{
    #ifdef DEBUG
    if (s_remote_container_table.nb_remote_containers == 0)
    {
        NRF_LOG_INFO("Remote containers table is empty!");
        return;
    }

    NRF_LOG_INFO("Remote containers table contains %u entries:",
                 s_remote_container_table.nb_remote_containers);

    for (uint16_t entry_idx = 0;
         entry_idx < s_remote_container_table.nb_remote_containers;
         entry_idx++)
    {
        remote_container_t entry = s_remote_container_table.remote_containers[entry_idx];

        NRF_LOG_INFO("Entry %u: Type = %s, Alias = %s, Remote ID = %u, Local ID = %u, Node address = 0x%x!",
                     entry_idx,
                     RoutingTB_StringFromType(entry.remote_rtb_entry.type),
                     entry.remote_rtb_entry.alias,
                     entry.remote_rtb_entry.id, entry.local_id,
                     entry.node_addr);
    }
    #endif /* DEBUG */
}

static uint16_t get_nb_local_containers_in_mesh_bridge_node(void)
{
    routing_table_t*    rtb                 = RoutingTB_Get();
    uint16_t            last_entry          = RoutingTB_GetLastEntry();

    // Find local node ID.
    uint16_t            node_id             = find_mesh_bridge_node_id(
                                                rtb,
                                                last_entry
                                              );

    // Check result.
    LUOS_ASSERT(node_id != 0);

    // Find corresponding routing table entry.
    routing_table_t*    node_entry          = NULL;
    for (uint16_t entry_idx = 0; entry_idx < last_entry; entry_idx++)
    {
        routing_table_t*    curr_entry  = rtb + entry_idx;

        if (curr_entry->mode == NODE && curr_entry->node_id == node_id)
        {
            node_entry = curr_entry;
            break;
        }
    }

    // Check search result.
    LUOS_ASSERT(node_entry != NULL);

    uint16_t            nb_local_containers = 0;

    // To match first container entry in the node.
    node_entry++;

    // Loop over container entries.
    while (node_entry[nb_local_containers].mode == CONTAINER)
    {
        nb_local_containers++;
    }

    return nb_local_containers;
}

static void RemoteContainer_MsgHandler(container_t* container,
                                       msg_t* msg)
{
    if ((msg->header.target_mode != ID)
        && (msg->header.target_mode != IDACK))
    {
        // Let's let the complicated cases behind for now...
        return;
    }

    // Send message through Luos MSG model.
    app_luos_msg_model_send_msg(msg);
}
