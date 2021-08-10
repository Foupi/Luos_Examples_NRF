#include "rtb_builder.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>        // bool
#include <stdint.h>         // uint*_t

// NRF

#ifdef DEBUG
#include "nrf_log.h"        // NRF_LOG_INFO
#endif /* DEBUG */

// LUOS
#include "context.h"        // ctx
#include "luos.h"           // Luos_CreateContainer, container_t
#include "routing_table.h"  // routing_table_t, RoutingTB_*

/*      STATIC/GLOBAL VARIABLES & CONSTANTS                         */

#ifndef REV
#define REV {0,0,1}
#endif

// Global detection boolean.
bool                g_detection_asked   = false;

// Static container instance.
static container_t* s_rtb_builder       = NULL;

/*      STATIC FUNCTIONS                                            */

// Logs each entry from the given routing table.
static void     print_rtb(const routing_table_t* rtb,
                          uint16_t last_entry_index);

/* Finds the ID the container corresponding to the given ID will have
** after a detection process is completed by this container.
*/
static uint16_t find_future_container_id(uint16_t container_id,
                                         uint16_t self_id);

// Finds in the routing table the ID of the node hosting the given ID.
static uint16_t find_node_id(uint16_t container_id);

// Compute future container ID if both containers are on the same node.
static uint16_t find_future_container_id_same_node(uint16_t container_id,
                                                   uint16_t self_id);

/*      CALLBACKS                                                   */

// Does nothing: only commands are performed through button IT.
static void RTBBuilder_MsgHandler(container_t* container, msg_t* msg);

void RTBBuilder_Init(void)
{
    revision_t revision = { .unmap = REV };
    s_rtb_builder = Luos_CreateContainer(RTBBuilder_MsgHandler,
                                         RTB_TYPE, RTB_ALIAS, revision);
}

void RTBBuilder_Loop(void)
{
    if (g_detection_asked)
    {
        // Run detection
        RoutingTB_DetectContainers(s_rtb_builder);

        // Log RTB
        routing_table_t* rtb = RoutingTB_Get();
        uint16_t last_entry_index = RoutingTB_GetLastEntry();

        print_rtb(rtb, last_entry_index);
        g_detection_asked       = false;
    }
}

static void print_rtb(const routing_table_t* rtb,
                      uint16_t last_entry_index)
{
    #ifdef DEBUG
    NRF_LOG_INFO("Routing table contains %u entries!",
                 last_entry_index);

    for (uint16_t entry_idx = 0; entry_idx < last_entry_index;
         entry_idx++)
    {
        const routing_table_t entry = rtb[entry_idx];
        switch (entry.mode)
        {
        case NODE:
            NRF_LOG_INFO("Entry %u: Node!", entry_idx);
            NRF_LOG_INFO("ID: %u ; %s!", entry.node_id,
                entry.certified ? "certified" : "not certified");
            for (uint8_t port_idx = 0; port_idx < NBR_PORT; port_idx++)
            {
                NRF_LOG_INFO("Port %u: %u!", port_idx,
                             entry.port_table[port_idx]);
            }
            break;
        case CONTAINER:
            NRF_LOG_INFO("Entry %u: Container!", entry_idx);
            NRF_LOG_INFO("ID: %u ; type: %u (%s) ; alias: %s!", entry.id,
                entry.type, RoutingTB_StringFromType(entry.type),
                (char*)entry.alias);
            break;
        default:
            NRF_LOG_INFO("Unknown entry mode: leaving!");
            return;
        }
    }
    #endif /* DEBUG */
}

static uint16_t find_future_container_id(uint16_t container_id,
                                         uint16_t self_id)
{
    if (container_id == self_id)
    {
        // Edge-case...
        return 1;
    }

    uint16_t    host_node = find_node_id(container_id);
    if (host_node == ctx.node.node_id)
    {
        return find_future_container_id_same_node(container_id,
                                                  self_id);
    }
    else
    {
        // FIXME Iterate over RTB.
        return 0;
    }
}

static uint16_t find_node_id(uint16_t container_id)
{
    routing_table_t* rtb    = RoutingTB_Get();
    uint16_t last_entry_idx = RoutingTB_GetLastEntry();

    uint16_t node_id = 1;
    uint16_t entry_idx = 1; // Index 0 is entry for Node 1.
    while (entry_idx < last_entry_idx)
    {
        routing_table_t entry   = rtb[entry_idx];

        if (entry.mode == NODE)
        {
            node_id = entry.node_id;
        }
        else if (entry.mode == CONTAINER)
        {
            if (entry.id == container_id)
            {
                return node_id;
            }
        }
        else
        {
            break;
        }

        entry_idx++;
    }
    // Not found.
    return 0;
}

static uint16_t find_future_container_id_same_node(uint16_t container_id,
                                                   uint16_t self_id)
{
    routing_table_t*    rtb             = RoutingTB_Get();
    uint16_t            entry_idx       = 0;
    uint16_t            last_entry_idx  = RoutingTB_GetLastEntry();

    while (entry_idx < last_entry_idx)
    {
        routing_table_t entry = rtb[entry_idx];

        if ((entry.mode == NODE) && (entry.node_id == ctx.node.node_id))
        {
            break;
        }

        entry_idx++;
    }

    uint16_t            node_idx       = entry_idx;
    uint16_t            container_idx  = 0;
    uint16_t            self_idx       = 0;
    entry_idx++;

    while (entry_idx < last_entry_idx)
    {
        routing_table_t entry = rtb[entry_idx];

        if (entry.mode != CONTAINER)
        {
            break;
        }

        if (entry.id == container_id)
        {
            container_idx   = entry_idx - node_idx; // Node relative index.
        }
        else if (entry.id == self_id)
        {
            self_idx        = entry_idx - node_idx;
        }

        entry_idx++;
    }

    if (container_idx == 0)
    {
        // Container not found in node: error.
        return 0;
    }

    if (self_idx > container_idx)
    {
        return container_idx + 1;
    }
    return container_idx;
}

static void RTBBuilder_MsgHandler(container_t* container, msg_t* msg)
{
    switch (msg->header.cmd)
    {
    case RTB_BUILD:
    {
        uint16_t    msg_src = msg->header.source;
        uint16_t    msg_dst = msg->header.target;
        uint16_t    new_src = find_future_container_id(msg_src,
                                                       msg_dst);

        RoutingTB_DetectContainers(s_rtb_builder);

        msg_t answer;
        memset(&answer, 0, sizeof(msg_t));
        answer.header.target_mode   = ID;
        answer.header.target        = new_src;
        answer.header.cmd           = RTB_COMPLETE;

        Luos_SendMsg(s_rtb_builder, &answer);
    }
        break;

    case RTB_PRINT:
    {
        // Log RTB
        routing_table_t* rtb = RoutingTB_Get();
        uint16_t last_entry_index = RoutingTB_GetLastEntry();

        print_rtb(rtb, last_entry_index);
    }
        break;
    default:
        break;
    }
}
