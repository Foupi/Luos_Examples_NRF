#include "provisioning.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>                     // uint*_t
#include <string.h>                     // memset

// NRF
#include "nrf_log.h"                    // NRF_LOG_INFO
#include "nrf_sdh_ble.h"                // nrf_sdh_ble_*
#include "nrf_sdh_soc.h"                // NRF_SDH_SOC_OBSERVER
#include "sdk_config.h"                 // NRF_SDH_CLOCK_LF_*
#include "sdk_errors.h"                 // ret_code_t

// NRF APPS
#include "app_error.h"                  // APP_ERROR_CHECK

// SOFTDEVICE
#include "nrf_sdm.h"                    // nrf_clock_lf_cfg_t

// MESH SDK
#include "device_state_manager.h"       // dsm_*
#include "mesh_stack.h"                 // mesh_stack_*
#include "nrf_mesh.h"                   // nrf_mesh_on_sd_evt
#include "nrf_mesh_prov.h"              /* nrf_mesh_prov_*,
                                        ** NRF_MESH_PROV_*
                                        */
#include "nrf_mesh_prov_bearer.h"       // prov_bearer_t
#include "nrf_mesh_prov_bearer_adv.h"   // nrf_mesh_prov_bearer_adv_*
#include "rand.h"                       // rand_hw_rng_get

// MESH MODELS
#include "config_server.h"              // config_server_*
#include "config_server_events.h"       // config_server_evt_t

/*      STATIC VARIABLES & CONSTANTS                                */

// Configuration tag.
static const uint8_t                CONN_CFG_TAG            = 1;

// Describes if the device is provisioned.
static bool                         s_device_provisioned    = false;

// Provisioning context.
static nrf_mesh_prov_ctx_t          s_prov_ctx;

// Provisioning encryption keys (public/private)
static uint8_t                      s_pubkey[NRF_MESH_PROV_PUBKEY_SIZE];
static uint8_t                      s_privkey[NRF_MESH_PROV_PRIVKEY_SIZE];

// Provisioning bearer (ADV).
static nrf_mesh_prov_bearer_adv_t   s_prov_bearer;

// Provisioner element address.
static const uint16_t               PROV_ELM_ADDRESS        = 0x0001;

// Key indexes.
static const mesh_key_index_t       PROV_NETKEY_IDX         = 0x0000;
static const mesh_key_index_t       PROV_APPKEY_IDX         = 0x0000;

// Amounts of expected key indexes when fetching from persistent data.
static const uint32_t               NB_NETKEY_IDX           = 1;
static const uint32_t               NB_APPKEY_IDX           = 1;

static struct
{
    // Key handles.
    dsm_handle_t    netkey_handle;
    dsm_handle_t    appkey_handle;
    dsm_handle_t    self_devkey_handle;

    // Keys values.
    uint8_t         netkey[NRF_MESH_KEY_SIZE];
    uint8_t         appkey[NRF_MESH_KEY_SIZE];
    uint8_t         self_devkey[NRF_MESH_KEY_SIZE];

}                                   s_network_ctx;

/*      STATIC FUNCTIONS                                            */

// Initializes the SoftDevice and BLE stack with predefined parameters.
static void ble_stack_init(void);

// Registers the BLE callback in the BLE stack.
static void ble_observers_register(void);

/* Generates keys and handles, stores them in the DSM and in the static
** context.
*/
static void network_ctx_generate(void);

/* Fetches the handles stored in the DSM and keeps them in the static
** context.
*/
static void network_ctx_fetch(void);

/*      CALLBACKS                                                   */

// Transmits the event to the Mesh stack.
static void sd_soc_event_cb(uint32_t event, void* context);

// On node reset event, erases persistent data and resets the board.
static void config_server_event_cb(const config_server_evt_t* event);

// Initializes the config client and health client models.
static void models_init_cb(void);

/* Caps received:           Use received capacities for
**                          OOB authentication.
** Static request:          Provide static authentication data.
** Complete:                Prepare remote config server setup.
** Link closed:             Add node to static context if provisioning
**                          was successful.
*/
static void mesh_prov_event_cb(const nrf_mesh_prov_evt_t* event);

// Unprovisioned device:    Start provisioning the advertising device.
static void mesh_unprov_event_cb(const nrf_mesh_prov_evt_t* event);

void mesh_init(void)
{
    ble_stack_init();

    ble_observers_register();

    nrf_clock_lf_cfg_t lf_clk_src;
    memset(&lf_clk_src, 0, sizeof(nrf_clock_lf_cfg_t));
    lf_clk_src.source       = NRF_SDH_CLOCK_LF_SRC;
    lf_clk_src.rc_ctiv      = NRF_SDH_CLOCK_LF_RC_CTIV;
    lf_clk_src.rc_temp_ctiv = NRF_SDH_CLOCK_LF_RC_TEMP_CTIV;
    lf_clk_src.accuracy     = NRF_SDH_CLOCK_LF_ACCURACY;

    mesh_stack_init_params_t init_params;
    memset(&init_params, 0, sizeof(mesh_stack_init_params_t));
    init_params.core.irq_priority       = NRF_MESH_IRQ_PRIORITY_LOWEST;
    init_params.core.lfclksrc           = lf_clk_src;
    init_params.models.models_init_cb   = models_init_cb;
    init_params.models.config_server_cb = config_server_event_cb;

    ret_code_t err_code = mesh_stack_init(&init_params,
                                          &s_device_provisioned);
    APP_ERROR_CHECK(err_code);
}

void provisioning_init(void)
{
    ret_code_t                      err_code;

    const nrf_mesh_prov_oob_caps_t  oob_caps = NRF_MESH_PROV_OOB_CAPS_DEFAULT(ACCESS_ELEMENT_COUNT);

    err_code = nrf_mesh_prov_init(&s_prov_ctx, s_pubkey, s_privkey,
                                  &oob_caps, mesh_prov_event_cb);
    APP_ERROR_CHECK(err_code);

    prov_bearer_t* generic_bearer = nrf_mesh_prov_bearer_adv_interface_get(&s_prov_bearer);
    err_code = nrf_mesh_prov_bearer_add(&s_prov_ctx, generic_bearer);
    APP_ERROR_CHECK(err_code);
}

void network_ctx_init(void)
{
    if (s_device_provisioned)
    {
        NRF_LOG_INFO("Fetching network context from persistent storage!");
        network_ctx_fetch();
    }
    else
    {
        NRF_LOG_INFO("Generating network context!");
        network_ctx_generate();
    }

    NRF_LOG_INFO("Netkey handle: 0x%x; Appkey handle: 0x%x; Self devkey handle: 0x%x!",
                 s_network_ctx.netkey_handle,
                 s_network_ctx.appkey_handle,
                 s_network_ctx.self_devkey_handle);
}

void mesh_start(void)
{
    ret_code_t err_code = mesh_stack_start();
    APP_ERROR_CHECK(err_code);
}

void prov_scan_start(void)
{
    ret_code_t err_code = nrf_mesh_prov_scan_start(mesh_unprov_event_cb);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_INFO("Start scanning for unprovisioned devices!");
}

void prov_scan_stop(void)
{
    nrf_mesh_prov_scan_stop();

    NRF_LOG_INFO("Stop scanning for unprovisioned devices!");
}

// Logs instruction pointer.
void mesh_assertion_handler(uint32_t pc)
{
    NRF_LOG_INFO("Assertion failed at address 0x%x!", pc);
}

static void ble_stack_init(void)
{
    // SDH is already enabled in Luos HAL.

    uint32_t    ram_start = 0;
    ret_code_t  err_code;

    // FIXME Maybe default cfg is not the best.
    err_code = nrf_sdh_ble_default_cfg_set(CONN_CFG_TAG,
                                           &ram_start);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);
}

static void ble_observers_register(void)
{
    NRF_SDH_SOC_OBSERVER(s_mesh_soc_obs,
                         NRF_SDH_BLE_STACK_OBSERVER_PRIO,
                         sd_soc_event_cb,
                         NULL);
}

static void network_ctx_generate(void)
{
    ret_code_t                  err_code;

    dsm_local_unicast_address_t local_addr_range;
    memset(&local_addr_range, 0, sizeof(dsm_local_unicast_address_t));
    local_addr_range.address_start  = PROV_ELM_ADDRESS;
    local_addr_range.count          = ACCESS_ELEMENT_COUNT;

    err_code = dsm_local_unicast_addresses_set(&local_addr_range);
    APP_ERROR_CHECK(err_code);

    rand_hw_rng_get(s_network_ctx.netkey, NRF_MESH_KEY_SIZE);
    err_code = dsm_subnet_add(PROV_NETKEY_IDX, s_network_ctx.netkey,
                              &(s_network_ctx.netkey_handle));
    APP_ERROR_CHECK(err_code);

    rand_hw_rng_get(s_network_ctx.appkey, NRF_MESH_KEY_SIZE);
    err_code = dsm_appkey_add(PROV_APPKEY_IDX,
                              s_network_ctx.netkey_handle,
                              s_network_ctx.appkey,
                              &(s_network_ctx.appkey_handle));
    APP_ERROR_CHECK(err_code);

    rand_hw_rng_get(s_network_ctx.self_devkey, NRF_MESH_KEY_SIZE);
    err_code = dsm_devkey_add(PROV_ELM_ADDRESS,
                              s_network_ctx.netkey_handle,
                              s_network_ctx.self_devkey,
                              &(s_network_ctx.self_devkey_handle));
    APP_ERROR_CHECK(err_code);

    err_code = config_server_bind(s_network_ctx.self_devkey_handle);
    APP_ERROR_CHECK(err_code);
}

static void network_ctx_fetch(void)
{
    ret_code_t                  err_code;
    dsm_local_unicast_address_t local_addr_range;
    dsm_local_unicast_addresses_get(&local_addr_range);

    // Temporary variables to ease writing.
    uint32_t                    nb_indexes;
    mesh_key_index_t            key_index_buffer;
    dsm_handle_t                curr_handle;

    err_code = dsm_subnet_get_all(&key_index_buffer, &nb_indexes);
    APP_ERROR_CHECK(err_code);
    if (nb_indexes != NB_NETKEY_IDX)
    {
        // FIXME Manage error.
    }

    curr_handle = dsm_net_key_index_to_subnet_handle(key_index_buffer);
    if (curr_handle == DSM_HANDLE_INVALID)
    {
        // FIXME Manage error.
    }
    s_network_ctx.netkey_handle = curr_handle;

    err_code = dsm_appkey_get_all(s_network_ctx.netkey_handle,
                                  &key_index_buffer, &nb_indexes);
    APP_ERROR_CHECK(err_code);
    if (nb_indexes != NB_APPKEY_IDX)
    {
        // FIXME Manage error.
    }

    curr_handle = dsm_appkey_index_to_appkey_handle(key_index_buffer);
    if (curr_handle == DSM_HANDLE_INVALID)
    {
        // FIXME Manage error.
    }
    s_network_ctx.appkey_handle = curr_handle;

    err_code = dsm_devkey_handle_get(local_addr_range.address_start,
                                     &curr_handle);
    APP_ERROR_CHECK(err_code);
    if (curr_handle == DSM_HANDLE_INVALID)
    {
        // FIXME Manage error.
    }
    s_network_ctx.self_devkey_handle = curr_handle;
}

static void sd_soc_event_cb(uint32_t event, void* context)
{
    nrf_mesh_on_sd_evt(event);
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
    s_network_ctx.netkey_handle         = DSM_HANDLE_INVALID;
    s_network_ctx.appkey_handle         = DSM_HANDLE_INVALID;
    s_network_ctx.self_devkey_handle    = DSM_HANDLE_INVALID;

    NRF_LOG_INFO("Initializing %u models!", ACCESS_MODEL_COUNT);
}

static void mesh_prov_event_cb(const nrf_mesh_prov_evt_t* event)
{
    NRF_LOG_INFO("Mesh provisioning event received: type %u!",
                 event->type);
}

static void mesh_unprov_event_cb(const nrf_mesh_prov_evt_t* event)
{
    if (event->type != NRF_MESH_PROV_EVT_UNPROVISIONED_RECEIVED)
    {
        // Only manage unprovisioned device events.
        return;
    }

    NRF_LOG_INFO("Unprovisioned device detected!");
}
