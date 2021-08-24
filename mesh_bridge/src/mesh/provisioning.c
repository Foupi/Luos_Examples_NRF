#include "provisioning.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>                // bool
#include <stdint.h>                 // uint*_t

// NRF
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

#ifdef DEBUG
#include "nrf_log.h"                // NRF_LOG_INFO
#endif /* DEBUG */

/*      STATIC VARIABLES & CONSTANTS                                */

// Static provisioning context.
static nrf_mesh_prov_ctx_t  s_prov_ctx;

// Advertised device URI.
static const char           LUOS_DEVICE_URI[]       = "Luos Bridge";

/*      CALLBACKS                                                   */

/* Link established:    Logs message.
** Invite received:     FIXME Logs message.
** Start received:      FIXME Logs message.
** Static request:      Send authentication data.
** Complete:            Store received data and toggles provisioned
**                      bool.
*/
static void mesh_prov_event_cb(const nrf_mesh_prov_evt_t* event);

void provisioning_init(void)
{
    // Initialize static context with given callback.
    _provisioning_init(&s_prov_ctx, mesh_prov_event_cb);
}

void prov_listening_start(void)
{
    // Initialize internal encryption keys.
    encryption_keys_generate();

    /* Listen to an incoming provisioning link on the given bearer type
    ** while exposing the given URI.
    */
    ret_code_t  err_code    = nrf_mesh_prov_listen(&s_prov_ctx,
        LUOS_DEVICE_URI, 0, NRF_MESH_PROV_BEARER_ADV);
    APP_ERROR_CHECK(err_code);

    #ifdef DEBUG
    NRF_LOG_INFO("Started listening for incoming links!");
    #endif /* DEBUG */
}

static void mesh_prov_event_cb(const nrf_mesh_prov_evt_t* event)
{
    ret_code_t err_code;

    switch (event->type)
    {
    case NRF_MESH_PROV_EVT_LINK_ESTABLISHED:
        #ifdef DEBUG
        NRF_LOG_INFO("Provisioning link established!");
        #endif /* DEBUG */

        break;

    case NRF_MESH_PROV_EVT_INVITE_RECEIVED:
    {
        uint32_t attention_duration = event->params.invite_received.attention_duration_s;

        #ifdef DEBUG
        NRF_LOG_INFO("Invitation received: attention interval is %us!",
                     attention_duration);
        #else /* ! DEBUG */
        // FIXME Turn on LEDs for given duration.
        #endif /* DEBUG */
    }
        break;

    case NRF_MESH_PROV_EVT_START_RECEIVED:
        #ifdef DEBUG
        NRF_LOG_INFO("Provisioning procedure started!");
        #else /* ! DEBUG */
        // FIXME Turn off LEDs.
        #endif /* DEBUG */

        break;

    case NRF_MESH_PROV_EVT_STATIC_REQUEST:
        #ifdef DEBUG
        NRF_LOG_INFO("Static authentication data requested!");
        #endif /* DEBUG */

        // Provide internal authentication data through given context.
        auth_data_provide(&s_prov_ctx);

        break;

    case NRF_MESH_PROV_EVT_COMPLETE:
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Provisioning procedure complete!");
        #endif /* DEBUG */

        nrf_mesh_prov_evt_complete_t                complete_evt    = event->params.complete;

        // Provisioning data provided by the Provisioner node.
        const nrf_mesh_prov_provisioning_data_t*    prov_data       = complete_evt.p_prov_data;

        /* Device key provided by the Provisioner node (used for
        ** later communication with the Provisioner).
        */
        const uint8_t*                              device_key      = complete_evt.p_devkey;

        /* Store provisioning data and device key in context and
        ** persistent memory.
        */
        err_code = mesh_stack_provisioning_data_store(prov_data,
                                                      device_key);
        APP_ERROR_CHECK(err_code);

        /* Set internal model addresses using the unicast device address
        ** provided by the Provisioner node.
        */
        mesh_models_set_addresses(prov_data->address);

        // Device is now provisioned.
        g_device_provisioned = true;
    }
        break;

    case NRF_MESH_PROV_EVT_LINK_CLOSED:
        #ifdef DEBUG
        NRF_LOG_INFO("Provisioning link closed!");
        #endif /* DEBUG */

        if (!g_device_provisioned)
        {
            #ifdef DEBUG
            NRF_LOG_INFO("Provisioning procedure was aborted: listening again!");
            #endif /* DEBUG */

            prov_listening_start();
        }

        /* Else there is nothing left to do, everything was managed at
        ** Provisioning Complete event.
        */

        break;

    default:
        #ifdef DEBUG
        NRF_LOG_INFO("Mesh provisioning event received: type %u!",
                     event->type);
        #endif /* DEBUG */

        break;
    }
}
