#ifndef LUOS_RTB_MODEL_H
#define LUOS_RTB_MODEL_H

/*      TYPEDEFS                                                    */

typedef struct luos_rtb_model_s luos_rtb_model_t;

typedef struct
{
    // FIXME Fields necessary for the instance initialization.

} luos_rtb_model_init_params_t;

// A Luos RTB model instance.
struct luos_rtb_model_s
{
    // FIXME Fields necessary for the model's good behaviour.

};

// Initialize the given instance with the given parameters.
void luos_rtb_model_init(luos_rtb_model_t* instance,
                         const luos_rtb_model_init_params_t* params);

// Sends a Luos RTB GET request through the given model instance.
void luos_rtb_model_get(luos_rtb_model_t* instance);

#endif /* ! LUOS_RTB_MODEL_H */
