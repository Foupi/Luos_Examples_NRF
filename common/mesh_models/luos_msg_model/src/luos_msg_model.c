#include "luos_msg_model.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>         // uint16_t

// MESH SDK
#include "access.h"         // access_model_handle_t

// LUOS
#include "robus_struct.h"   // msg_t

void luos_msg_model_init(luos_msg_model_t* instance,
                         const luos_msg_model_init_params_t* params)
{}

void luos_msg_model_set(luos_msg_model_t* instance, uint16_t dst_addr,
                        const msg_t* msg)
{}
