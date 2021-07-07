#include "provisioning.h"

/*      INCLUDES                                                    */

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

// MESH MODELS
#include "config_server_events.h"   // config_server_evt_t

// CUSTOM
#include "luos_mesh_common.h"       // _mesh_init, _provisioning_init

/*      STATIC VARIABLES & CONSTANTS                                */

// Describes if the device is provisioned.
static bool                 s_device_provisioned    = false;

// Static provisioning context.
static nrf_mesh_prov_ctx_t  s_prov_ctx;

// Advertised device URI.
static const char           LUOS_DEVICE_URI[]       = "Luos Bridge";

/*      CALLBACKS                                                   */

// On node reset event, erases persistent data and resets the board.
static void config_server_event_cb(const config_server_evt_t* event);

// FIXME Initialize the models present on the node.
static void models_init_cb(void);

/* Link established:    Turn on LED 4.
** Invite received:     Turn on LED 3 for the received amount of
**                      seconds.
** Start received:      Turn off LED 3, Turn on LED 2.
** Static request:      Send authentication data.
** Complete:            Store received data and toggles provisioned
**                      bool, turn off LED 2.
** Failed:              Turn off LED 2.
** Link closed:         If provisioned bool is true turn on LED 1, turn
**                      off LED 4.
*/
static void mesh_prov_event_cb(const nrf_mesh_prov_evt_t* event);

void mesh_init(void)
{
    _mesh_init(config_server_event_cb, models_init_cb,
               &s_device_provisioned);
}

void provisioning_init(void)
{
    _provisioning_init(&s_prov_ctx, mesh_prov_event_cb);
}

void persistent_conf_init(void)
{
    if (s_device_provisioned)
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
}

static void config_server_event_cb(const config_server_evt_t* event)
{
    if (event->type == CONFIG_SERVER_EVT_NODE_RESET)
    {
        // FIXME Erase persistent data.

        // FIXME Reset board.
    }
}

static void models_init_cb(void)
{
    // FIXME Initializations.
}

static void mesh_prov_event_cb(const nrf_mesh_prov_evt_t* event)
{
    NRF_LOG_INFO("Mesh provisioning event received: type %u!",
                 event->type);
}
