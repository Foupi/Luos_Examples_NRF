#ifndef PROVISIONING_H
#define PROVISIONING_H

// Initializes the Mesh provisioning module with predefined parameters.
void provisioning_init(void);

// Initializes or fetches persistent data.
void persistent_conf_init(void);

// Starts listening for a provisioning link.
void prov_listening_start(void);

#endif /* ! PROVISIONING_H */
