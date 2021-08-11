#include "app_luos_msg_model.h"

/*      INCLUDES                                                    */

#include "luos_msg_model.h" // luos_msg_model_*

/*      STATIC VARIABLES & CONSTANTS                                */

// Static Luos MSG model instance.
static luos_msg_model_t s_msg_model;

void app_luos_msg_model_init(void)
{
    luos_msg_model_init_params_t params;
    // FIXME set params.

    luos_msg_model_init(&s_msg_model, &params);
}
