#ifndef APP_LUOS_MSG_MODEL_H
#define APP_LUOS_MSG_MODEL_H

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h> // uint16_t

// Initializes the internal Luos MSG model instance.
void app_luos_msg_model_init(void);

/* Sets the element address of the internal model instance according
** to the given device address.
*/
void app_luos_msg_model_address_set(uint16_t device_address);

#endif /* ! APP_LUOS_MSG_MODEL_H */
