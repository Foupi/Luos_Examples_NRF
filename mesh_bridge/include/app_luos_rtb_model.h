#ifndef APP_LUOS_RTB_MODEL_H
#define APP_LUOS_RTB_MODEL_H

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>         // uint16_t

// LUOS
#include "luos.h"           // container_t

/* Initializes the internal Luos RTB model instance with predefined
** callbacks.
*/
void app_luos_rtb_model_init(void);

/* Sets the element address of the internal model instance according
** to the given device address.
*/
void app_luos_rtb_model_address_set(uint16_t device_address);

// Sends a Luos RTB model GET request to remote devices.
void app_luos_rtb_model_engage_ext_rtb(container_t* mesh_bridge_container,
                                       uint16_t src_id,
                                       uint16_t mesh_bridge_id);

#endif /* ! APP_LUOS_RTB_MODEL_H */
