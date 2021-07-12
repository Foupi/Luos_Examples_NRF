#include "node_config.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>                 // uint16_t

// NRF
#include "nrf_log.h"                // NRF_LOG_INFO

// MESH SDK
#include "device_state_manager.h"   // dsm_handle_t

void node_config_start(uint16_t device_first_addr,
                       dsm_handle_t devkey_handle,
                       dsm_handle_t address_handle)
{
    NRF_LOG_INFO("Start configuration for node 0x%x! (devkey handle: 0x%x; address handle: 0x%x)",
                 device_first_addr, devkey_handle, address_handle);
}
