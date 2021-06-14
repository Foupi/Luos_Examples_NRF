#include "rtb_builder.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>        // bool
#include <stdint.h>         // uint*_t

// NRF
#include "boards.h"         // BSP_BUTTON_0, BUTTON_PULL
#include "sdk_errors.h"     // ret_code_t

#ifdef DEBUG
#include "nrf_log.h"        // NRF_LOG_INFO
#endif /* DEBUG */

// NRF APPS
#include "app_button.h"     // app_button_*
#include "app_error.h"      // APP_ERROR_CHECK
#include "app_timer.h"      // APP_TIMER_TICKS

// LUOS
#include "luos.h"           // Luos_CreateContainer, container_t
#include "routing_table.h"  // routing_table_t, RoutingTB_*

/*      STATIC VARIABLES & CONSTANTS                                */

#ifndef REV
#define REV {0,0,1}
#endif

// The container instance.
static  container_t*    s_rtb_builder       = NULL;

// Describes if a detection was asked.
static  bool            s_detection_asked   = false;

// Button to press.
#define                 BUTTON_IDX  BSP_BUTTON_0

// Button detection delay.
static const uint32_t BTN_DTX_DELAY = APP_TIMER_TICKS(50);

/*      STATIC FUNCTIONS                                            */

// Initializes the button functionality.
static void init_button(void);

// Logs each entry from the given routing table.
static void print_rtb(const routing_table_t* rtb,
                      uint16_t last_entry_index);

/*      CALLBACKS                                                   */

// Does nothing: only commands are performed through button IT.
static void RTBBuilder_MsgHandler(container_t* container, msg_t* msg);

// If the index is the defined one, toggles the detection boolean.
static void button_evt_handler(uint8_t btn_idx, uint8_t event);

void RTBBuilder_Init(void)
{
    init_button();

    revision_t revision = { .unmap = REV };
    s_rtb_builder = Luos_CreateContainer(RTBBuilder_MsgHandler,
                                         RTB_TYPE, RTB_ALIAS, revision);

    app_button_enable();
}

void RTBBuilder_Loop(void)
{
    if (s_detection_asked)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Detection asked!");
        #endif /* DEBUG */

        // Run detection
        RoutingTB_DetectContainers(s_rtb_builder);

        #ifdef DEBUG
        NRF_LOG_INFO("Detection complete!");
        #endif /* DEBUG */

        // Log RTB
        routing_table_t* rtb = RoutingTB_Get();
        uint16_t last_entry_index = RoutingTB_GetLastEntry();
        print_rtb(rtb, last_entry_index);

        s_detection_asked = false;
    }
}

static void init_button(void)
{
    static app_button_cfg_t buttons[] =
    {
        {
            BUTTON_IDX,
            APP_BUTTON_ACTIVE_LOW,
            BUTTON_PULL,
            button_evt_handler,
        },
    };

    ret_code_t err_code = app_button_init(buttons, 1, BTN_DTX_DELAY);
    APP_ERROR_CHECK(err_code);
}

static void print_rtb(const routing_table_t* rtb,
                      uint16_t last_entry_index)
{
    #ifdef DEBUG
    for (uint16_t entry_idx = 0; entry_idx < last_entry_index;
         entry_idx++)
    {
        NRF_LOG_INFO("Entry %u!", entry_idx);

        const routing_table_t entry = rtb[entry_idx];
        switch (entry.mode)
        {
        case NODE:
            NRF_LOG_INFO("Mode: node!");
            NRF_LOG_INFO("Node ID: %u!", entry.node_id);
            NRF_LOG_INFO("Certified: %s!",
                         entry.certified ? "true" : "false");
            for (uint8_t port_idx = 0; port_idx < NBR_PORT; port_idx++)
            {
                NRF_LOG_INFO("Port %u: %u!", port_idx,
                             entry.port_table[port_idx]);
            }
            break;
        case CONTAINER:
            NRF_LOG_INFO("Mode: container!");
            NRF_LOG_INFO("ID: %u!", entry.id);
            NRF_LOG_INFO("Type: %u (%s)!", entry.type,
                         RoutingTB_StringFromType(entry.type));
            NRF_LOG_INFO("Alias: %s!", entry.alias);
            break;
        default:
            NRF_LOG_INFO("Unknown entry mode: leaving!");
            return;
        }
    }
    #endif /* DEBUG */
}

static void RTBBuilder_MsgHandler(container_t* container, msg_t* msg)
{}

static void button_evt_handler(uint8_t btn_idx, uint8_t event)
{
    if (btn_idx != BUTTON_IDX)
    {
        return;
    }

    s_detection_asked = (event == APP_BUTTON_PUSH);
}
