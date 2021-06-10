#include "led_toggler.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>        // bool
#include <stdint.h>         // uint8_t

// NRF
#include "boards.h"         // bsp_board_led_off, bsp_board_led_on

// LUOS
#include "luos.h"           /* container_t, msg_t, revision_t,
                            ** Luos_CreateContainer, STATE_MOD,
                            ** IO_STATE, ASK_PUB_CMD, Luos_SendMsg
                            */

/*      CONSTANTS & STATIC VARIABLES                                */

#ifndef REV
#define REV {0,0,1}
#endif

// Index of the controlled LED (LED 1 has index 0).
static const uint32_t LED_IDX = 0;

/*      STATIC FUNCTIONS                                            */

/* If the message contains an IO state, sets the LEDs on this state;
** if it is a status request, answers with the current LEDs state.
*/
static void LedToggler_MsgHandler(container_t* container, msg_t* msg);

/*      IMPLEMENTATION                                              */

void LedToggler_Init(void)
{
    revision_t revision = {.unmap = REV};
    Luos_CreateContainer(LedToggler_MsgHandler, STATE_MOD,
                         "led_toggler", revision);
}

void LedToggler_Loop(void)
{
    // Nothing to do.
}

static void LedToggler_MsgHandler(container_t* container, msg_t* msg)
{
    luos_cmd_t cmd = msg->header.cmd;

    if (cmd == IO_STATE)
    {
        uint8_t new_val = msg->data[0];

        LUOS_ASSERT((new_val == 0) || (new_val == 1));

        if (new_val == 0)
        {
            bsp_board_led_off(LED_IDX);
        }
        else
        {
            bsp_board_led_on(LED_IDX);
        }
    }
    else if (msg->header.cmd == ASK_PUB_CMD)
    {
        // Answer LED status.
        msg_t answer;

        // Payload is a simple boolean.
        answer.header.cmd = IO_STATE;
        answer.header.size = sizeof(uint8_t);
        // Answer to a precise container...
        answer.header.target_mode = ID;
        // ... Which is the one asking for the LED status.
        answer.header.target = msg->header.source;

        bool led_state = bsp_board_led_state_get(LED_IDX);
        uint8_t answer_state;
        if (led_state)
        {
            answer_state = 1;
        }
        else
        {
            answer_state = 0;
        }
        answer.data[0] = answer_state;

        Luos_SendMsg(container, &answer);
    }
}
