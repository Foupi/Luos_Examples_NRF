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
#include "luos.h"           // Luos_CreateContainer, container_t
#include "routing_table.h"  // routing_table_t, RoutingTB_*

/*      STATIC/GLOBAL VARIABLES & CONSTANTS                         */

#ifndef REV
#define REV {0,0,1}
#endif

// Static container instance.
static container_t* s_rtb_builder       = NULL;

/*      STATIC FUNCTIONS                                            */

// Logs each entry from the given routing table.
static void     print_rtb(const routing_table_t* rtb,
                          uint16_t last_entry_index);

/*      CALLBACKS                                                   */

// Does nothing: only commands are performed through button IT.
static void RTBBuilder_MsgHandler(container_t* container, msg_t* msg);

void RTBBuilder_Init(void)
{
    #ifdef DEBUG
    NRF_LOG_INFO("RTB Mod: 0x%x!", RTB_MOD);
    NRF_LOG_INFO("RTB MSG begin: 0x%x!", RTB_MSG_BEGIN);
    #endif /* DEBUG */

    revision_t revision = { .unmap = REV };
    s_rtb_builder = Luos_CreateContainer(RTBBuilder_MsgHandler,
                                         RTB_TYPE, RTB_ALIAS, revision);
}

void RTBBuilder_Loop(void)
{}

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

static void RTBBuilder_MsgHandler(container_t* container, msg_t* msg)
{
    switch (msg->header.cmd)
    {
    case RTB_BUILD:
    {
        uint16_t    msg_src = msg->header.source;
        uint16_t    msg_dst = msg->header.target;
        uint16_t    new_src = RoutingTB_FindFutureContainerID(msg_src,
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
