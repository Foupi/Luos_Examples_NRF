#ifndef PROVISIONING_H
#define PROVISIONING_H

/*      DEFINES                                                     */

// Provisioner element address.
#define PROV_ELM_ADDRESS    0x0001

// Key indexes.
#define PROV_NETKEY_IDX     0x0000
#define PROV_APPKEY_IDX     0x0000

// Initializes the Mesh stack with predefined parameters.
void mesh_init(void);

// Initializes the Mesh provisioning module with predefined parameters.
void provisioning_init(void);

// Initializes the provisioning persistent configuration.
void prov_conf_init(void);

// Start scanning for unprovisioned devices.
void prov_scan_start(void);

// Stop scanning for unprovisioned devices.
void prov_scan_stop(void);

#include <stdbool.h>

extern bool g_device_provisioned;

#endif /* ! PROVISIONING_H */
