#include "luos_rtb_model.h"

/*      INCLUDES                                                    */

// NRF
#include "nrf_log.h"    // NRF_LOG_INFO

void luos_rtb_model_init(luos_rtb_model_t* instance,
                         const luos_rtb_model_init_params_t* params)
{
    NRF_LOG_INFO("Initializing Luos RTB model instance!");
}

void luos_rtb_model_get(luos_rtb_model_t* instance)
{
    NRF_LOG_INFO("Luos RTB GET request sent!");
}
