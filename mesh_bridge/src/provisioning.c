#include "provisioning.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>                // bool
#include <stdint.h>                 // uint*_t

// NRF
#include "nrf_log.h"                // NRF_LOG_INFO
#include "sdk_errors.h"             // ret_code_t

// NRF APPS
#include "app_error.h"              // APP_ERROR_CHECK

// MESH SDK
#include "nrf_mesh_prov.h"          // nrf_mesh_prov_*, NRF_MESH_PROV_*
#include "nrf_mesh_prov_events.h"   /* nrf_mesh_prov_evt_*,
                                    ** NRF_MESH_PROV_EVT_*
                                    */

// CUSTOM
#include "luos_mesh_common.h"       // _provisioning_init
#include "mesh_init.h"              /* g_device_provisioned,
                                    ** mesh_models_set_addresses
                                    */

/*      STATIC VARIABLES & CONSTANTS                                */

// Static provisioning context.
static nrf_mesh_prov_ctx_t  s_prov_ctx;

// Advertised device URI.
static const char           LUOS_DEVICE_URI[]       = "Luos Bridge";

/*      CALLBACKS                                                   */

/* Invite received:     Turn on LEDs for the received amount of
**                      seconds.
** Start received:      Turn off LEDs.
** Static request:      Send authentication data.
** Complete:            Store received data and toggles provisioned
**                      bool.
*/
static void mesh_prov_event_cb(const nrf_mesh_prov_evt_t* event);

void provisioning_init(void)
{
    _provisioning_init(&s_prov_ctx, mesh_prov_event_cb);
}

void persistent_conf_init(void)
{
    if (g_device_provisioned)
    {
        NRF_LOG_INFO("Fetching persistent configuration!");

        // FIXME Fetch persistent configuration.
    }
    else
    {
        NRF_LOG_INFO("Generating persistent configuration!");

        // FIXME Generate persistent configuration.
    }
}

void prov_listening_start(void)
{
    encryption_keys_generate();

    ret_code_t err_code = nrf_mesh_prov_listen(&s_prov_ctx,
        LUOS_DEVICE_URI, 0, NRF_MESH_PROV_BEARER_ADV);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_INFO("Started listening for incoming links!");
}

static void mesh_prov_event_cb(const nrf_mesh_prov_evt_t* event)
{
    ret_code_t err_code;

    switch (event->type)
    {
    case NRF_MESH_PROV_EVT_LINK_ESTABLISHED:
        NRF_LOG_INFO("Provisioning link established!");

        break;

    case NRF_MESH_PROV_EVT_INVITE_RECEIVED:
    {
        uint32_t attention_duration = event->params.invite_received.attention_duration_s;

        NRF_LOG_INFO("Invitation received: attention interval is %us!",
                     attention_duration);

        // FIXME Turn on LEDs for the given duration.
    }
        break;

    case NRF_MESH_PROV_EVT_START_RECEIVED:
        NRF_LOG_INFO("Provisioning procedure started!");

        // FIXME Turn off LEDs.

        break;

    case NRF_MESH_PROV_EVT_STATIC_REQUEST:
        NRF_LOG_INFO("Static authentication data requested!");

        auth_data_provide(&s_prov_ctx);

        break;

    case NRF_MESH_PROV_EVT_COMPLETE:
    {
        NRF_LOG_INFO("Provisioning procedure complete!");

        nrf_mesh_prov_evt_complete_t                complete_evt    = event->params.complete;
        const nrf_mesh_prov_provisioning_data_t*    prov_data       = complete_evt.p_prov_data;
        const uint8_t*                              device_key      = complete_evt.p_devkey;

        err_code = mesh_stack_provisioning_data_store(prov_data,
                                                      device_key);
        APP_ERROR_CHECK(err_code);

        mesh_models_set_addresses(prov_data->address);

        // Device is now provisioned.
        g_device_provisioned = true;
    }
        break;

    case NRF_MESH_PROV_EVT_LINK_CLOSED:
        NRF_LOG_INFO("Provisioning link closed!");

        if (!g_device_provisioned)
        {
            NRF_LOG_INFO("Provisioning procedure was aborted: listening again!");

            prov_listening_start();
        }

        break;

    default:
        NRF_LOG_INFO("Mesh provisioning event received: type %u!",
                     event->type);
        break;
    }
}
