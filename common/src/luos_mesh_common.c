#include "luos_mesh_common.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>                // bool
#include <stdint.h>                 // uint*_t
#include <string.h>                 // memset

// NRF
#include "nrf_log.h"                // NRF_LOG_INFO
#include "nrf_sdh_ble.h"            // nrf_sdh_ble_*
#include "nrf_sdh_soc.h"            // NRF_SDH_SOC_OBSERVER
#include "sdk_config.h"             // NRF_SDH_CLOCK_LF_*
#include "sdk_errors.h"             // ret_code_t

// NRF APPS
#include "app_error.h"              // APP_ERROR_CHECK

// SOFTDEVICE
#include "nrf_sdm.h"                // nrf_clock_lf_cfg_t

// MESH SDK
#include "mesh_stack.h"             // mesh_stack_*
#include "nrf_mesh.h"               // nrf_mesh_*

// MESH MODELS
#include "config_server_events.h"   // config_server_evt_cb_t

/*      STATIC VARIABLES & CONSTANTS                                */

// Configuration tag.
static const uint8_t                CONN_CFG_TAG            = 1;

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
    init_params.models.config_server_cb = cfg_srv_cb;

    ret_code_t err_code;
    err_code = mesh_stack_init(&init_params, device_provisioned);
    APP_ERROR_CHECK(err_code);
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

static void sd_soc_event_cb(uint32_t event, void* context)
{
    nrf_mesh_on_sd_evt(event);
}

