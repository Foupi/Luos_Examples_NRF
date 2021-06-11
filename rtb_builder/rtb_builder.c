#include "rtb_builder.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>        // bool
#include <stdint.h>         // uint*_t

// NRF
#include "boards.h"         // bsp_board_led_*
#include "nrf_log.h"        // NRF_LOG_INFO
#include "sdk_errors.h"     // ret_code_t

// NRF APPS
#include "app_button.h"     // app_button_*
#include "app_error.h"      // APP_ERROR_CHECK
#include "app_timer.h"      // APP_TIMER_TICKS

// LUOS
#include "luos.h"           // Luos_CreateContainer, container_t

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

/*      CALLBACKS                                                   */

// Does nothing: only commands are performed through button IT.
static void RTBBuilder_MsgHandler(container_t* container, msg_t* msg);

// If the index is the defined one, toggles the detection boolean.
static void button_evt_handler(uint8_t btn_idx, uint8_t event);

void RTBBuilder_Init(void)
{
    init_button();

    revision_t revision = { .unmap = REV };
    s_rtb_builder = Luos_CreateContainer(NULL, RTB_TYPE, RTB_ALIAS,
                                         revision);

    app_button_enable();
}

void RTBBuilder_Loop(void)
{
    // if detection asked, run detection.
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

static void button_evt_handler(uint8_t btn_idx, uint8_t event)
{
    #ifdef DEBUG
    NRF_LOG_INFO("Button event %u on btn %u!", event, btn_idx);
    #endif /* DEBUG */

    if (btn_idx != BUTTON_IDX)
    {
        return;
    }

    s_detection_asked = (event == APP_BUTTON_PUSH);
}
