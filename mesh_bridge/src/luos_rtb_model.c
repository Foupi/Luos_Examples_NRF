#include "luos_rtb_model.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <string.h>                 // memset

// NRF
#include "nrf_log.h"                // NRF_LOG_INFO
#include "sdk_errors.h"             // ret_code_t

// NRF APPS
#include "app_error.h"              // APP_ERROR_CHECK

// MESH SDK
#include "access.h"                 // access_*

// CUSTOM
#include "luos_rtb_model_common.h"  // LUOS_RTB_MODEL_*

/*      CALLBACKS                                                   */

// FIXME Manage a Luos RTB model GET request.
static void luos_rtb_model_get_cb(access_model_handle_t handle,
                                  const access_message_rx_t* msg,
                                  void* arg);

// FIXME Manage a Luos RTB model STATUS message.
static void luos_rtb_model_status_cb(access_model_handle_t handle,
                                     const access_message_rx_t* msg,
                                     void* arg);

/*      INITIALIZATIONS                                             */

// Luos RTB model opcode handlers table.
static const access_opcode_handler_t    LUOS_RTB_MODEL_OPCODE_HANDLERS[]    =
{
    {
        LUOS_RTB_MODEL_GET_ACCESS_OPCODE,
        luos_rtb_model_get_cb,
    },
    {
        LUOS_RTB_MODEL_STATUS_ACCESS_OPCODE,
        luos_rtb_model_status_cb,
    },
};

void luos_rtb_model_init(luos_rtb_model_t* instance,
                         const luos_rtb_model_init_params_t* params)
{
    NRF_LOG_INFO("Initializing Luos RTB model instance!");

    ret_code_t                  err_code;
    access_model_id_t           luos_rtb_model_id   = LUOS_RTB_MODEL_ACCESS_ID;

    access_model_add_params_t   add_params;
    memset(&add_params, 0, sizeof(access_model_add_params_t));
    add_params.model_id             = luos_rtb_model_id;
    add_params.element_index        = LUOS_RTB_MODEL_ELM_IDX;
    add_params.p_opcode_handlers    = LUOS_RTB_MODEL_OPCODE_HANDLERS;
    add_params.opcode_count         = sizeof(LUOS_RTB_MODEL_OPCODE_HANDLERS) / sizeof(access_opcode_handler_t);
    add_params.p_args               = instance;

    err_code = access_model_add(&add_params, &(instance->handle));
    APP_ERROR_CHECK(err_code);
}

void luos_rtb_model_get(luos_rtb_model_t* instance)
{
    NRF_LOG_INFO("Luos RTB GET request sent!");
}

static void luos_rtb_model_get_cb(access_model_handle_t handle,
                                  const access_message_rx_t* msg,
                                  void* arg)
{
    NRF_LOG_INFO("Luos RTB GET request received!");
}

static void luos_rtb_model_status_cb(access_model_handle_t handle,
                                     const access_message_rx_t* msg,
                                     void* arg)
{
    NRF_LOG_INFO("Luos RTB STATUS message received!");
}
