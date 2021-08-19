/******************************************************************************
 * @file gate
 * @brief Container gate
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "gate.h"
#include "json_mnger.h"
#include <stdio.h>

#include <unistd.h>             // read

// NRF
#include "sdk_errors.h"         // ret_code_t

// NRF APPS
#include "app_error.h"          // APP_ERROR_CHECK
#include "app_timer.h"          // app_timer_*
#include "app_uart.h"           // app_uart_evt_t

// LUOS
#include "luos_hal_config.h"    // MAX_SYSTICK_MS_VAL

// CUSTOM
#include "uart_helpers.h"       // uart_init

#ifdef LUOS_MESH_BRIDGE
#include "mesh_bridge.h"        // MESH_BRIDGE_*
#include "mesh_bridge_utils.h"  // find_mesh_bridge_container_id
#endif /* LUOS_MESH_BRIDGE */

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#ifndef REV
#define REV {1,0,0}
#endif

// Size in bytes of each read operation.
static const uint32_t   READ_SIZE               = 64;

// Time in ms between two Gate refreshes.
static const uint32_t   GATE_REFRESH_RATE_MS    = 2000;
static const uint32_t   GATE_REFRESH_RATE_TICKS = APP_TIMER_TICKS(GATE_REFRESH_RATE_MS);

// Gate refresh timer
APP_TIMER_DEF(s_gate_refresh_timer);

/*******************************************************************************
 * Variables
 ******************************************************************************/
container_t *container;
msg_t msg;
uint8_t RxData;
container_t *container_pointer;
volatile msg_t pub_msg;
volatile int pub = LUOS_PROTOCOL_NB;

// Data reception management.
static char* s_reception_buffer;
static uint32_t s_reception_index = 0;
/*******************************************************************************
 * Function
 ******************************************************************************/

// Changes JSON buffer if a complete command was received.
static void Gate_ManageReceivedData(void);

// Manages received data on data ready event, stops on error.
static void Gate_UartEvtHandler(app_uart_evt_t* event);

// Prints the routing table on UART.
static void print_rtb(const routing_table_t* rtb, uint16_t nb_entries);

// FIXME Logs message.
static void gate_refresh_timer_cb(void* context);

/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/

void Gate_Init(void)
{
    uart_init(Gate_UartEvtHandler);

    revision_t revision = {.unmap = REV};
    container = Luos_CreateContainer(0, GATE_MOD, "gate", revision);

    s_reception_buffer = get_json_buf();

    ret_code_t err_code = app_timer_create(&s_gate_refresh_timer,
                                           APP_TIMER_MODE_REPEATED,
                                           gate_refresh_timer_cb);
    APP_ERROR_CHECK(err_code);
}

__attribute__((weak)) void json_send(char *json)
{
    // Sends data on the UART link with the pilot.
    printf(json);
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void Gate_Loop(void)
{
    static unsigned int keepAlive = 0;
    static volatile uint8_t detection_done = 0;
    static char state = 0;
    uint32_t tickstart = 0;

    // Check if there is a dead container
    if (container->ll_container->dead_container_spotted)
    {
        char json[JSON_BUFF_SIZE] = {0};
        exclude_container_to_json(container->ll_container->dead_container_spotted, json);
        json_send(json);
        container->ll_container->dead_container_spotted = 0;
    }
    if (detection_done)
    {
        char json[JSON_BUFF_SIZE] = {0};
        state = !state;
        format_data(container, json);
        if (json[0] != '\0')
        {
//            json_send(json);
            keepAlive = 0;
        }
        else
        {
            if (keepAlive > 200)
            {
                sprintf(json, "{}\n");
//                json_send(json);
            }
            else
            {
                keepAlive++;
            }
        }
        collect_data(container);
    }
    if (pub != LUOS_PROTOCOL_NB)
    {
        Luos_SendMsg(container_pointer, (msg_t *)&pub_msg);
        pub = LUOS_PROTOCOL_NB;
    }
    // check if serial input messages ready and convert it into a luos message
    send_cmds(container);
    if (detection_ask)
    {
        char json[JSON_BUFF_SIZE * 2] = {0};
        RoutingTB_DetectContainers(container);
        routing_table_to_json(json);
        json_send(json);

        if (!detection_done)
        {
            ret_code_t err_code = app_timer_start(s_gate_refresh_timer,
                GATE_REFRESH_RATE_TICKS, NULL);
            APP_ERROR_CHECK(err_code);
        }

        #ifdef LUOS_MESH_BRIDGE
        routing_table_t* rtb    = RoutingTB_Get();
        uint16_t nb_entries     = RoutingTB_GetLastEntry();

        print_rtb(rtb, nb_entries);

        if (!detection_done)    // Only fill local container table once.
        {
            uint16_t mesh_bridge_id = find_mesh_bridge_container_id(rtb,
                nb_entries);

            msg_t   fill_local;
            memset(&fill_local, 0, sizeof(msg_t));
            fill_local.header.target_mode   = ID;
            fill_local.header.target        = mesh_bridge_id;
            fill_local.header.cmd           = MESH_BRIDGE_FILL_LOCAL_CONTAINER_TABLE;

            Luos_SendMsg(container, &fill_local);
        }
        #endif /* LUOS_MESH_BRIDGE */

        detection_done = 1;
        detection_ask = 0;
    }

    tickstart = Luos_GetSystick();
    uint32_t curr_tick = tickstart;
    uint32_t waited_ticks = 0;
    while(waited_ticks < GATE_REFRESH_RATE_MS)
    {
        uint32_t old_val = curr_tick;
        curr_tick = Luos_GetSystick();
        if (curr_tick < old_val)
        {
            // There was an overflow.
            uint32_t remaining;
            if (old_val >= MAX_SYSTICK_MS_VAL)
            {
                // Since MAX_SYSTICK_MS_VAL is not an exact value...
                remaining = 0;
            }
            else
            {
                remaining = MAX_SYSTICK_MS_VAL - old_val;
            }
            waited_ticks += (remaining + curr_tick);
        }
        else
        {
            waited_ticks += (curr_tick - old_val);
        }
    }
}

static void Gate_ManageReceivedData(void)
{
    ssize_t read_bytes;
    do
    {
        read_bytes = read(0, s_reception_buffer + s_reception_index,
                          READ_SIZE);

        if (read_bytes == -1)
            while (true);

        if (read_bytes == 0)
            break;

        s_reception_index += read_bytes;
    } while (s_reception_index < JSON_BUFF_SIZE
             && read_bytes == READ_SIZE);

    bool complete = check_json(s_reception_index - 1);
    if (complete)
    {
        s_reception_index = 0;
    }

    s_reception_buffer = get_json_buf();
}

static void Gate_UartEvtHandler(app_uart_evt_t* event)
{
    switch (event->evt_type)
    {
    case APP_UART_DATA_READY:
        Gate_ManageReceivedData();
        break;
    case APP_UART_TX_EMPTY:
        break;
    case APP_UART_DATA:
    case APP_UART_FIFO_ERROR:
    case APP_UART_COMMUNICATION_ERROR:
        while (true);
    }
}

static void print_rtb(const routing_table_t* rtb, uint16_t nb_entries)
{
    printf("Routing table contains %u entries!\n",
                 nb_entries);

    for (uint16_t entry_idx = 0; entry_idx < nb_entries; entry_idx++)
    {
        const routing_table_t entry = rtb[entry_idx];
        switch (entry.mode)
        {
        case NODE:
            printf("Entry %u: Node!\n", entry_idx);
            printf("ID: %u ; %s!\n", entry.node_id,
                entry.certified ? "certified" : "not certified");
            for (uint8_t port_idx = 0; port_idx < NBR_PORT; port_idx++)
            {
                printf("Port %u: %u!\n", port_idx,
                             entry.port_table[port_idx]);
            }
            break;
        case CONTAINER:
            printf("Entry %u: Container!\n", entry_idx);
            printf("ID: %u ; type: %u (%s) ; alias: %s!\n", entry.id,
                entry.type, RoutingTB_StringFromType(entry.type),
                (char*)entry.alias);
            break;
        default:
            printf("Unknown entry mode: leaving!\n");
            return;
        }
    }
}

static void gate_refresh_timer_cb(void* context)
{
    printf("Gate refresh!\n");
}
