#include "remote_container_table.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>                // bool
#include <stdint.h>                 // uint16_t
#include <string.h>                 // memcpy, memset

// NRF
#include "nrf_log.h"                // NRF_LOG_INFO

// LUOS
#include "luos.h"                   /* container_t,
                                    ** Luos_CreateContainer, msg_t,
                                    ** revision_t
                                    */
#include "routing_table.h"          // routing_table_t

// CUSTOM
#include "app_luos_msg_model.h"     // app_luos_msg_model_send_msg
#include "local_container_table.h"  // local_container_table_*
#include "luos_mesh_common.h"       // LUOS_MESH_NETWORK_MAX_NODES
#include "luos_rtb_model_common.h"  // LUOS_RTB_MODEL_MAX_RTB_ENTRY
#include "mesh_bridge_utils.h"      // find_mesh_bridge_node_id

/*      STATIC VARIABLES & CONSTANTS                                */

/* Maximum number of remote container entries in the remote containers
** table.
*/
#define REMOTE_CONTAINER_TABLE_MAX_NB_ENTRIES   \
    LUOS_MESH_NETWORK_MAX_NODES * LUOS_RTB_MODEL_MAX_RTB_ENTRY

// The internal table of remote containers.
static struct
{
    // Number of entries contained in the remote containers table.
    uint16_t            nb_remote_containers;

    // Array of remote containers.
    remote_container_t  remote_containers[REMOTE_CONTAINER_TABLE_MAX_NB_ENTRIES];

}       s_remote_container_table                = { 0 };

#ifndef REV
#define REV {0,0,1}
#endif

// Number of local containers hosted by the Mesh Bridge node.
static uint16_t s_nb_local_containers   = 0;

/*      STATIC FUNCTIONS                                            */

/* Returns the number of non-remote containers in the node hosting the
** Mesh Bridge container.
*/
static uint16_t             get_nb_local_containers_in_mesh_bridge_node(void);

/*      CALLBACKS                                                   */

// FIXME Logs message.
static void RemoteContainer_MsgHandler(container_t* container,
                                       msg_t* msg);

bool remote_container_table_add_entry(uint16_t node_address,
                                      const routing_table_t* entry)
{
    revision_t revision = { .unmap = REV };

    if (s_remote_container_table.nb_remote_containers >= REMOTE_CONTAINER_TABLE_MAX_NB_ENTRIES)
    {
        // Table is full: insertion is not possible.
        return false;
    }

    remote_container_t* insertion_entry     = s_remote_container_table.remote_containers + s_remote_container_table.nb_remote_containers;
    insertion_entry->node_addr  = node_address;
    memcpy(&(insertion_entry->remote_rtb_entry), entry,
           sizeof(routing_table_t));

    if (s_nb_local_containers == 0)
    {
        s_nb_local_containers = get_nb_local_containers_in_mesh_bridge_node();
    }
    uint16_t            first_available_id  = s_nb_local_containers + 1; // Since container indexes start at 1.

    insertion_entry->local_id       = first_available_id + s_remote_container_table.nb_remote_containers;
    insertion_entry->local_instance = Luos_CreateContainer(
        RemoteContainer_MsgHandler, entry->type, entry->alias, revision
    );

    NRF_LOG_INFO("Remote container %s received ID %u!",
                 entry->alias, insertion_entry->local_id);

    s_remote_container_table.nb_remote_containers++;

    return true;
}

void remote_container_table_clear(void)
{
    for (uint16_t entry_idx = 0;
         entry_idx < s_remote_container_table.nb_remote_containers;
         entry_idx++)
    {
        remote_container_t  curr_entry;
        curr_entry = s_remote_container_table.remote_containers[entry_idx];

        Luos_DestroyContainer(curr_entry.local_instance);
    }

    memset(&s_remote_container_table, 0,
           sizeof(s_remote_container_table));
}

void remote_container_table_clear_address(uint16_t node_address)
{
    uint16_t    entry_idx   = 0;
    while (entry_idx < s_remote_container_table.nb_remote_containers)
    {
        remote_container_t  curr_entry;
        curr_entry = s_remote_container_table.remote_containers[entry_idx];

        if (curr_entry.node_addr != node_address)
        {
            entry_idx++;
            continue;
        }

        Luos_DestroyContainer(curr_entry.local_instance);
        uint16_t    last_entry_idx = s_remote_container_table.nb_remote_containers - 1;
        for (uint16_t replacement_idx = entry_idx;
             replacement_idx < last_entry_idx; replacement_idx++)
        {
            remote_container_t* replaced_entry;
            remote_container_t* replacing_entry;

            replaced_entry  = s_remote_container_table.remote_containers + replacement_idx;
            replacing_entry = replaced_entry + 1;
            replacing_entry->local_id--;
            memcpy(replaced_entry, replacing_entry, sizeof(remote_container_t));
        }
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

static uint16_t get_nb_local_containers_in_mesh_bridge_node(void)
{
    routing_table_t*    rtb         = RoutingTB_Get();
    uint16_t            last_entry  = RoutingTB_GetLastEntry();

    uint16_t            node_id     = find_mesh_bridge_node_id(rtb, last_entry);
    if (node_id == 0)
    {
        NRF_LOG_INFO("Node ID of Mesh Bridge container not found!");
        return 0;
    }

    routing_table_t*    node_entry  = NULL;

    for (uint16_t entry_idx = 0; entry_idx < last_entry; entry_idx++)
    {
        routing_table_t*    curr_entry  = rtb + entry_idx;

        if (curr_entry->mode == NODE && curr_entry->node_id == node_id)
        {
            node_entry = curr_entry;
            break;
        }
    }

    if (node_entry == NULL)
    {
        // Not found, for some reason...
        return 0;
    }

    uint16_t nb_local_containers = 0;
    node_entry++;   // To match first container entry.
    while (node_entry[nb_local_containers].mode == CONTAINER)
    {
        nb_local_containers++;
    }

    return nb_local_containers;
}

static void RemoteContainer_MsgHandler(container_t* container,
                                       msg_t* msg)
{
    if (msg->header.target_mode != ID)
    {
        // Let's let the complicated cases behind for now...
        return;
    }

    app_luos_msg_model_send_msg(msg);
}
