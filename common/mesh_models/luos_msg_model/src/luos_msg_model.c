#include "luos_msg_model.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>         // uint16_t
#include <string.h>         // memset

// MESH SDK
#include "access.h"         // access_model_handle_t

// LUOS
#include "robus_struct.h"   // msg_t

// CUSTOM
#include "luos_msg_model_common.h"

void luos_msg_model_init(luos_msg_model_t* instance,
                         const luos_msg_model_init_params_t* params)
{
    memset(instance, 0, sizeof(luos_msg_model_t));
    instance->set_cb            = params->set_cb;
    instance->status_cb         = params->status_cb;

    instance->element_address   = LUOS_MSG_MODEL_DEFAULT_ELM_ADDR;

    // FIXME Add model in access layer.
}

void luos_msg_model_set_address(luos_msg_model_t* instance,
                                uint16_t device_address)
{
    instance->element_address   = device_address + LUOS_MSG_MODEL_ELM_IDX;
}

void luos_msg_model_set(luos_msg_model_t* instance, uint16_t dst_addr,
                        const msg_t* msg)
{
    // FIXME Create and enqueue set request.
}
