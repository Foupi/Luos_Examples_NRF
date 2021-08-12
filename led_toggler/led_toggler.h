#ifndef LED_TOGGLER_H
#define LED_TOGGLER_H

/*      INCLUDES                                                    */

// LUOS
#include "luos_list.h"  // STATE_MOD

/*      DEFINES                                                     */

// Type and default alias for the LED Toggler container.
#define LED_TOGGLER_TYPE    STATE_MOD
#define LED_TOGGLER_ALIAS   "led_toggler"

// Initializes the LED Toggler container.
void LedToggler_Init(void);

// Does nothing: this container reacts on message reception.
void LedToggler_Loop(void);

#endif /* ! LED_TOGGLER_H */
