#include "luos_mesh_common.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>                    // bool
#include <stdint.h>                     // uint*_t
#include <string.h>                     // memset

// NRF
#include "nrf_sdh_ble.h"                // nrf_sdh_ble_*
#include "nrf_sdh_soc.h"                // NRF_SDH_SOC_OBSERVER
#include "sdk_config.h"                 // NRF_SDH_CLOCK_LF_*
#include "sdk_errors.h"                 // ret_code_t

// NRF APPS
#include "app_error.h"                  // APP_ERROR_CHECK

// SOFTDEVICE
#include "nrf_sdm.h"                    // nrf_clock_lf_cfg_t

// MESH SDK
#include "mesh_stack.h"                 // mesh_stack_*
#include "nrf_mesh.h"                   // nrf_mesh_*
#include "nrf_mesh_prov.h"              /* nrf_mesh_prov_*,
                                        ** NRF_MESH_PROV_*
                                        */
#include "nrf_mesh_prov_bearer.h"       // prov_bearer_t
#include "nrf_mesh_prov_bearer_adv.h"   // nrf_mesh_prov_bearer_adv_*
#include "nrf_mesh_prov_events.h"       // nrf_mesh_prov_evt_handler_cb_t

// MESH MODELS
#include "config_server_events.h"       // config_server_evt_cb_t

// LUOS
#include "luos_utils.h"                 // LUOS_ASSERT

#ifdef DEBUG
#include "nrf_log.h"                    // NRF_LOG_INFO
#endif /* DEBUG */

/*      STATIC VARIABLES & CONSTANTS                                */

// Connection configuration tag.
static const uint8_t                CONN_CFG_TAG    = 1;

// Provisioning encryption keys (public/private)
static uint8_t                      s_pubkey[NRF_MESH_PROV_PUBKEY_SIZE];
static uint8_t                      s_privkey[NRF_MESH_PROV_PRIVKEY_SIZE];

/* Provisioning bearer: provisioning channel (chosen one is ADV for
** ADVertisement, but it could be done over GATT).
*/
static nrf_mesh_prov_bearer_adv_t   s_prov_bearer;

// Static authentication data for provisioning process.
static const uint8_t                AUTH_DATA[]     = LUOS_STATIC_AUTH_DATA;

/*      STATIC FUNCTIONS                                            */

// Initializes the SoftDevice and BLE stack with predefined parameters.
static void ble_stack_init(void);

// Registers the BLE callback in the BLE stack.
static void ble_observers_register(void);

/*      CALLBACKS                                                   */

// Transmits the event to the Mesh stack.
static void sd_soc_event_cb(uint32_t event, void* context);

void _mesh_init(config_server_evt_cb_t cfg_srv_cb,
                mesh_stack_models_init_cb_t models_init_cb,
                bool* device_provisioned)
{
    // Check parameters.
    LUOS_ASSERT(cfg_srv_cb != NULL);
    LUOS_ASSERT(device_provisioned != NULL);
    // Models init callback can be NULL.

    // Initialize the BLE stack first.
    ble_stack_init();

    // Register the BLE observer in the initialized stack.
    ble_observers_register();

    // Clock source and configuration.
    nrf_clock_lf_cfg_t          lf_clk_src;
    memset(&lf_clk_src, 0, sizeof(nrf_clock_lf_cfg_t));
    lf_clk_src.source                   = NRF_SDH_CLOCK_LF_SRC;
    lf_clk_src.rc_ctiv                  = NRF_SDH_CLOCK_LF_RC_CTIV;
    lf_clk_src.rc_temp_ctiv             = NRF_SDH_CLOCK_LF_RC_TEMP_CTIV;
    lf_clk_src.accuracy                 = NRF_SDH_CLOCK_LF_ACCURACY;

    // Mesh stack initialization parameters.
    mesh_stack_init_params_t    init_params;
    memset(&init_params, 0, sizeof(mesh_stack_init_params_t));
    init_params.core.irq_priority       = NRF_MESH_IRQ_PRIORITY_LOWEST; // Priority of Mesh stack interruptions.
    init_params.core.lfclksrc           = lf_clk_src;                   // Mesh stack clock source and configuration.
    init_params.models.models_init_cb   = models_init_cb;               // Mesh models will be initialized by this function.
    init_params.models.config_server_cb = cfg_srv_cb;                   // Mesh stack always contains a config server: user must define a callback.

    ret_code_t                  err_code;

    /* Initialize the Mesh stack with the given parameters. Given
    ** boolean is updated.
    */
    err_code    = mesh_stack_init(&init_params, device_provisioned);
    APP_ERROR_CHECK(err_code);
}

void _provisioning_init(nrf_mesh_prov_ctx_t* prov_ctx,
                        nrf_mesh_prov_evt_handler_cb_t event_handler)
{
    // Check parameters.
    LUOS_ASSERT(prov_ctx != NULL);
    LUOS_ASSERT(event_handler != NULL);

    ret_code_t                      err_code;

    // Out-of-band provisioning capacities.
    const nrf_mesh_prov_oob_caps_t  oob_caps        = NRF_MESH_PROV_OOB_CAPS_DEFAULT(ACCESS_ELEMENT_COUNT);

    /* Initialize provisioning context with public and private key
    ** addresses (they are not initialized yet!), out-of-band capacities
    ** and given event handler.
    */
    err_code    = nrf_mesh_prov_init(prov_ctx, s_pubkey, s_privkey,
                                  &oob_caps, event_handler);
    APP_ERROR_CHECK(err_code);

    // Generic bearer interface gathered from ADV bearer.
    prov_bearer_t*                  generic_bearer  = nrf_mesh_prov_bearer_adv_interface_get(&s_prov_bearer);

    // Check result.
    LUOS_ASSERT(generic_bearer != NULL);

    // Add bearer interface to provisioning context.
    err_code    = nrf_mesh_prov_bearer_add(prov_ctx, generic_bearer);
    APP_ERROR_CHECK(err_code);
}

void mesh_start(void)
{
    /* Start Mesh stack and check error (this function's purpose is to
    ** mask the Mesh SDK API).
    */
    ret_code_t  err_code    = mesh_stack_start();
    APP_ERROR_CHECK(err_code);
}

void encryption_keys_generate(void)
{
    ret_code_t  err_code;

    // Generate public and private keys at the given addresses.
    err_code    = nrf_mesh_prov_generate_keys(s_pubkey, s_privkey);
    APP_ERROR_CHECK(err_code);
}

void auth_data_provide(nrf_mesh_prov_ctx_t* prov_ctx)
{
    ret_code_t  err_code;

    // Provide authentication data address and size to given context.
    err_code    = nrf_mesh_prov_auth_data_provide(prov_ctx, AUTH_DATA,
                                                  NRF_MESH_KEY_SIZE);
    APP_ERROR_CHECK(err_code);
}

// Logs instruction pointer.
void mesh_assertion_handler(uint32_t pc)
{
    #ifdef DEBUG
    NRF_LOG_INFO("Assertion failed at address 0x%x!", pc);
    #endif /* DEBUG */
}

static void ble_stack_init(void)
{
    // SDH is already enabled in Luos HAL.

    // RAM start provided to start the BLE SoftDevice.
    uint32_t    ram_start   = 0;
    ret_code_t  err_code;

    // Sets the connection config at the given tag to the default.
    err_code = nrf_sdh_ble_default_cfg_set(CONN_CFG_TAG,
                                           &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enables the BLE SoftDevice at the given RAM start.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);
}

static void ble_observers_register(void)
{
    // Register a System-On-Chip SoftDevice event observer.
    NRF_SDH_SOC_OBSERVER(s_mesh_soc_obs,                    // Observer name.
                         NRF_SDH_BLE_STACK_OBSERVER_PRIO,   // Observer priority.
                         sd_soc_event_cb,                   // Observer event handler.
                         NULL);                             // Observer context (given to event handler).
}

static void sd_soc_event_cb(uint32_t event, void* context)
{
    // Just send event to Mesh stack.
    nrf_mesh_on_sd_evt(event);
}
