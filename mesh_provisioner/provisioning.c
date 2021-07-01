#include "provisioning.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>         // uint*_t
#include <string.h>         // memset

// NRF
#include "nrf_log.h"        // NRF_LOG_INFO
#include "nrf_sdh_ble.h"    // nrf_sdh_ble_*
#include "nrf_sdh_soc.h"    // NRF_SDH_SOC_OBSERVER
#include "sdk_errors.h"     // ret_code_t

// NRF APPS
#include "app_error.h"      // APP_ERROR_CHECK

// MESH SDK
#include "nrf_mesh.h"       // nrf_mesh_on_sd_evt

/*      STATIC VARIABLES & CONSTANTS                                */

// Configuration tag.
static const uint8_t    CONN_CFG_TAG    = 1;

/*      STATIC FUNCTIONS                                            */

// Initializes the SoftDevice and BLE stack with predefined parameters.
static void ble_stack_init(void);

// Registers the BLE callback in the BLE stack.
static void ble_observers_register(void);


/*      CALLBACKS                                                   */

// Transmits the event to the Mesh stack.
static void sd_soc_event_cb(uint32_t event, void* context);

void mesh_init(void)
{
    ble_stack_init();

    NRF_LOG_INFO("BLE stack enabled!");

    ble_observers_register();

    NRF_LOG_INFO("BLE observers registered!");

    // FIXME Create init params.

    // FIXME Call to `mesh_stack_init`.

    NRF_LOG_INFO("Initializing Mesh stack!");
}

void provisioning_init(void)
{
    // FIXME Define OOB capacities.

    // FIXME Call to `nrf_mesh_prov_init`.

    // FIXME Add bearer interface.

    // FIXME Initialize network context (from persistent data).

    NRF_LOG_INFO("Initializing provisioning module!");
}

void mesh_start(void)
{
    // FIXME Call to `mesh_stack_start`.

    NRF_LOG_INFO("Starting Mesh stack!");
}

void prov_scan_start(void)
{
    NRF_LOG_INFO("Start scanning for unprovisioned devices!");
}

void prov_scan_stop(void)
{
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

static void sd_soc_event_cb(uint32_t event, void* context)
{
    nrf_mesh_on_sd_evt(event);
}
