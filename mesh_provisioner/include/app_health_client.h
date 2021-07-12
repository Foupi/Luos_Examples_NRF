#ifndef APP_HEALTH_CLIENT
#define APP_HEALTH_CLIENT

/*      INCLUDES                                                    */

// MESH SDK
#include "device_state_manager.h"       // dsm_handle_t

// Initializes the Health Client instance.
void app_health_client_init(void);

/* Binds and sets publication for the Health Client instance to the
** application key correspondig to the given handle.
*/
void app_health_client_bind(dsm_handle_t appkey_handle);

#endif /* ! APP_HEALTH_CLIENT */
