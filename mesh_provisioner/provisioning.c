#include "provisioning.h"

/*      INCLUDES                                                    */

// NRF
#include "nrf_log.h"    // NRF_LOG_INFO

void mesh_init(void)
{
    NRF_LOG_INFO("Initializing Mesh stack!");
}

void provisioning_init(void)
{
    NRF_LOG_INFO("Initializing provisioning module!");
}

void mesh_start(void)
{
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
