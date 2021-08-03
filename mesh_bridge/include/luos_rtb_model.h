#ifndef LUOS_RTB_MODEL_H
#define LUOS_RTB_MODEL_H

/*      INCLUDES                                                    */

// MESH SDK
#include "access.h" // access_model_handle_t

/*      DEFINES                                                     */

// Default element address.
#define LUOS_RTB_MODEL_DEFAULT_ELM_ADDR 0xFFFF

/*      TYPEDEFS                                                    */

typedef struct luos_rtb_model_s luos_rtb_model_t;

typedef struct
{
    // FIXME Fields necessary for the instance initialization.

} luos_rtb_model_init_params_t;

// A Luos RTB model instance.
struct luos_rtb_model_s
{
    // Handle for the model instance.
    access_model_handle_t   handle;

    // Unicast address of the element hosting the model instance.
    uint16_t                element_address;

    // FIXME Fields necessary for the model's good behaviour.

};

// Payload type for a Luos RTB model GET message.
typedef struct
{
    // Current transaction index.
    uint16_t    transaction_id;

} luos_rtb_get_t;

// Initialize the given instance with the given parameters.
void luos_rtb_model_init(luos_rtb_model_t* instance,
                         const luos_rtb_model_init_params_t* params);

/* Sets the given instance's element address according to the given
** device address.
*/
void luos_rtb_model_set_address(luos_rtb_model_t* instance,
                                uint16_t device_address);

// Sends a Luos RTB GET request through the given model instance.
void luos_rtb_model_get(luos_rtb_model_t* instance);

#endif /* ! LUOS_RTB_MODEL_H */
