#include <stdio.h>
#include <stdbool.h>
#include "json_mnger.h"
#include "cmd.h"
#include "convert.h"
#include "gate.h"

#ifdef LUOS_MESH_BRIDGE
#include "routing_table.h"  // routing_table_t
#include "mesh_bridge.h"    // MESH_BRIDGE_*
#endif /* LUOS_MESH_BRIDGE */

static unsigned int delayms = 1;

#ifdef LUOS_MESH_BRIDGE
/*      STATIC FUNCTIONS                                             */

/* Applies and returns true if the given message is a Mesh Bridge
** command, returns false otherwise.
*/
static bool is_mesh_bridge_cmd(container_t* container,
                               const msg_t* msg);

// Finds the ID of the Gate container in the routing table.
static uint16_t find_gate_container_id(const routing_table_t* rtb,
                                       uint16_t nb_entries);
#endif /* LUOS_MESH_BRIDGE */

//******************* sensor update ****************************
// This function will gather data from sensors and create a json string for you
void collect_data(container_t *container)
{
    msg_t json_msg;
    json_msg.header.target_mode = ID;
    json_msg.header.cmd = ASK_PUB_CMD;
    json_msg.header.size = 0;
    // ask containers to publish datas
    for (uint8_t i = 1; i <= RoutingTB_GetLastContainer(); i++)
    {
        // Check if this container is a sensor
        if ((RoutingTB_ContainerIsSensor(RoutingTB_TypeFromID(i))) || (RoutingTB_TypeFromID(i) >= LUOS_LAST_TYPE))
        {
            // This container is a sensor so create a msg and send it
            json_msg.header.target = i;
            Luos_SendMsg(container, &json_msg);
#ifdef GATE_TIMEOUT
            // Get the current number of message available
            int back_nbr_msg = Luos_NbrAvailableMsg();
            // Get the current time
            uint32_t send_time = Luos_GetSystick();
            // Wait for a reply before continuing
            while ((back_nbr_msg == Luos_NbrAvailableMsg()) & (send_time == Luos_GetSystick()))
            {
                Luos_Loop();
            }
#endif
        }
    }
}

// This function will create a json string for containers datas
void format_data(container_t *container, char *json)
{
    msg_t *json_msg = 0;
    uint8_t json_ok = false;
    if ((Luos_NbrAvailableMsg() > 0))
    {
        // Init the json string
        sprintf(json, "{\"containers\":{");
        // loop into containers.
        uint16_t i = 1;
        // get the oldest message
        while (Luos_ReadMsg(container, &json_msg) == SUCCEED)
        {
            // check if this is an assert
            if (json_msg->header.cmd == ASSERT)
            {
                char error_json[256] = "\0";
                luos_assert_t assertion;
                memcpy(assertion.unmap, json_msg->data, json_msg->header.size);
                assertion.unmap[json_msg->header.size] = '\0';
                sprintf(error_json, "{\"assert\":{\"node_id\":%d,\"file\":\"%s\",\"line\":%d}}\n", json_msg->header.source, assertion.file, (unsigned int)assertion.line);
                json_send(error_json);
                continue;
            }

            #ifdef LUOS_MESH_BRIDGE
            bool mesh_bridge_cmd = is_mesh_bridge_cmd(container,
                                                      json_msg);
            if (mesh_bridge_cmd)
            {
                json_ok = true;
                continue;
            }
            #endif /* LUOS_MESH_BRIDGE */

            // get the source of this message
            i = json_msg->header.source;
            // Create container description
            char *alias;
            alias = RoutingTB_AliasFromId(i);
            if (alias != 0)
            {
                json_ok = true;
                sprintf(json, "%s\"%s\":{", json, alias);
                // now add json data from container
                msg_to_json(json_msg, &json[strlen(json)]);
                // Check if we receive other messages from this container
                while (Luos_ReadFromContainer(container, i, &json_msg) == SUCCEED)
                {
                    // we receive some, add it to the Json
                    msg_to_json(json_msg, &json[strlen(json)]);
                }
                if (json[strlen(json) - 1] != '{')
                {
                    // remove the last "," char
                    json[strlen(json) - 1] = '\0';
                }
                // End the container section
                sprintf(json, "%s},", json);
            }
        }
        if (json_ok)
        {
            // remove the last "," char
            json[strlen(json) - 1] = '\0';
            // End the Json message
            sprintf(json, "%s}}\n", json);
        }
        else
        {
            //create a void string
            *json = '\0';
        }
    }
    else
    {
        //create a void string
        *json = '\0';
    }
}

unsigned int get_delay(void)
{
    return delayms;
}

void set_delay(unsigned int new_delayms)
{
    delayms = new_delayms;
}

#ifdef LUOS_MESH_BRIDGE
static bool is_mesh_bridge_cmd(container_t* container, const msg_t* msg)
{
    switch (msg->header.cmd)
    {
    case MESH_BRIDGE_LOCAL_CONTAINER_TABLE_FILLED:
        printf("Mesh Bridge local container table filled!\n");
        break;

    case MESH_BRIDGE_EXT_RTB_COMPLETE:
    {
        printf("RTB extension complete!\n");

        routing_table_t* rtb    = RoutingTB_Get();
        uint16_t nb_entries     = RoutingTB_GetLastEntry();

        uint16_t    gate_id = find_gate_container_id(rtb,
            nb_entries);

        msg_t update_tables;
        memset(&update_tables, 0, sizeof(msg_t));
        update_tables.header.target_mode    = ID;
        update_tables.header.target         = 1; // Detection at end of Ext-RTB.
        update_tables.header.cmd            = MESH_BRIDGE_UPDATE_INTERNAL_TABLES;
        update_tables.header.size           = sizeof(uint16_t);
        memcpy(update_tables.data, &gate_id, sizeof(uint16_t));

        Luos_SendMsg(container, &update_tables);
    }
        break;

    case MESH_BRIDGE_INTERNAL_TABLES_UPDATED:
        printf("Mesh Bridge internal tables updated!\n");
        detection_ask = 1;
        break;

    default:
        return false;
    }

    return true;
}

static uint16_t find_gate_container_id(const routing_table_t* rtb,
                                       uint16_t nb_entries)
{
    for (uint16_t entry_idx = 0; entry_idx < nb_entries; entry_idx++)
    {
        routing_table_t entry = rtb[entry_idx];

        if ((entry.mode == CONTAINER) && (entry.type == GATE_MOD))
        {
            return entry.id;
        }
    }

    return 0;
}
#endif /* LUOS_MESH_BRIDGE */
