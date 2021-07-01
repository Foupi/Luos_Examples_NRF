#ifndef PROVISIONING_H
#define PROVISIONING_H

// Initializes the Mesh stack with predefined parameters.
void mesh_init(void);

// Initializes the Mesh provisioning module with predefined parameters.
void provisioning_init(void);

// Starts the Mesh stack.
void mesh_start(void);

// Start scanning for unprovisioned devices.
void prov_scan_start(void);

// Stop scanning for unprovisioned devices.
void prov_scan_stop(void);

#endif /* PROVISIONING_H */
