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
#include "routing_table.h"          // routing_table_t

// CUSTOM
#include "luos_mesh_common.h"       // LUOS_MESH_NETWORK_MAX_NODES
#include "luos_rtb_model_common.h"  // LUOS_RTB_MODEL_MAX_RTB_ENTRY

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

/*      CALLBACKS                                                   */

// FIXME Does nothing for now.
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

    remote_container_t* insertion_entry = s_remote_container_table.remote_containers + s_remote_container_table.nb_remote_containers;
    insertion_entry->node_addr  = node_address;
    memcpy(&(insertion_entry->remote_rtb_entry), entry,
           sizeof(routing_table_t));

    insertion_entry->local_instance = Luos_CreateContainer(
        RemoteContainer_MsgHandler, entry->type, entry->alias, revision
    );

    s_remote_container_table.nb_remote_containers++;

    return true;
}

void remote_container_table_clear(void)
{
    memset(&s_remote_container_table, 0,
           sizeof(s_remote_container_table));
}

static void RemoteContainer_MsgHandler(container_t* container,
                                       msg_t* msg)
{}
