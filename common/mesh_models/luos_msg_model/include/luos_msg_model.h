#ifndef LUOS_MSG_MODEL_H
#define LUOS_MSG_MODEL_H

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>         // uint16_t

// MESH SDK
#include "access.h"         // access_model_handle_t

// LUOS
#include "robus_struct.h"   // msg_t

/*      DEFINES                                                     */

// Default element address.
#define LUOS_MSG_MODEL_DEFAULT_ELM_ADDR 0xFFFF

/*      TYPEDEFS                                                    */

// Forward declaration.
typedef struct luos_msg_model_s luos_msg_model_t;

// Callback called on SET command.
typedef void (*luos_msg_model_set_cb_t)(uint16_t src_addr,
                                        const msg_t* recv_msg);

// Callback called on STATUS message.
typedef void (*luos_msg_model_status_cb_t)(const msg_t* recv_msg);

// Parameters to initialize a Luos MSG model instance.
typedef struct
{
    // User callback called on SET command.
    luos_msg_model_set_cb_t     set_cb;

    // User callback called on STATUS command.
    luos_msg_model_status_cb_t  status_cb;

} luos_msg_model_init_params_t;

// A Luos MSG model instance.
struct luos_msg_model_s
{
    // Handle for the model instance.
    access_model_handle_t       handle;

    // Unicast address of the element hosting the instance.
    uint16_t                    element_address;

    // User callback called on SET command.
    luos_msg_model_set_cb_t     set_cb;

    // User callback called on STATUS command.
    luos_msg_model_status_cb_t  status_cb;
};

// Payload type for a Luos MSG model SET command.
typedef struct 
{
    // Current transaction index.
    uint16_t    transaction_id;

    // Unicast address of the destination element.
    uint16_t    dst_addr;

    // Luos message.
    msg_t       msg;

} luos_msg_model_set_t;

// Initialize the given instance with the given parameters.
void luos_msg_model_init(luos_msg_model_t* instance,
                         const luos_msg_model_init_params_t* params);

/* Sets the given instance's element address according to the given
** device address.
*/
void luos_msg_model_set_address(luos_msg_model_t* instance,
                                uint16_t device_address);

// Sends a Luos MSG SET commabd through the given model instance.
void luos_msg_model_set(luos_msg_model_t* instance, uint16_t dst_addr,
                        const msg_t* msg);

#endif /* ! LUOS_MSG_MODEL_H */
