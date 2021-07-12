#include "app_health_client.h"

/*      INCLUDES                                                    */

// NRF
#include "nrf_log.h"                // NRF_LOG_INFO
#include "sdk_errors.h"             // ret_code_t

// NRF APPS
#include "app_error.h"              // APP_ERROR_CHECK

// MESH SDK
#include "access_config.h"          // access_model_*
#include "device_state_manager.h"   // dsm_handle_t

// MESH MODELS
#include "health_client.h"          // health_client_*

/*      STATIC VARIABLES & CONSTANTS                                */

// Health Client model instance.
static health_client_t              s_health_client;

/*      CALLBACKS                                                   */

// On current status reception, logs a message.
static void health_client_event_cb(const health_client_t* instance,
                                   const health_client_evt_t* event);

void app_health_client_init(void)
{
    ret_code_t err_code = health_client_init(&s_health_client, 0,
                                             health_client_event_cb);
    APP_ERROR_CHECK(err_code);
}

void app_health_client_bind(dsm_handle_t appkey_handle)
{
    ret_code_t err_code;

    err_code = access_model_application_bind(s_health_client.model_handle,
                                             appkey_handle);
    APP_ERROR_CHECK(err_code);

    err_code = access_model_publish_application_set(s_health_client.model_handle,
                                                    appkey_handle);
    APP_ERROR_CHECK(err_code);
}

static void health_client_event_cb(const health_client_t* instance,
                                   const health_client_evt_t* event)
{
    switch (event->type)
    {
    case HEALTH_CLIENT_EVT_TYPE_CURRENT_STATUS_RECEIVED:
        NRF_LOG_INFO("Node 0x%x alive with %u active faults!",
                     event->p_meta_data->src.value,
                     event->data.fault_status.fault_array_length);

        break;
    default:
        NRF_LOG_INFO("Health client event received: type 0x%x!",
                     event->type);
        break;
    }
}
