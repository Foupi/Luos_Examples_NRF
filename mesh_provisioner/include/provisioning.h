#ifndef PROVISIONING_H
#define PROVISIONING_H

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>                // bool

// Initializes the Mesh provisioning module with predefined parameters.
void provisioning_init(void);

// Initializes the provisioning persistent configuration.
void prov_conf_init(void);

/* Starts scanning for unprovisioned devices and returns true if
** possible, returns false otherwise.
*/
bool prov_scan_start(void);

// Stops scanning for unprovisioned devices.
void prov_scan_stop(void);

#endif /* ! PROVISIONING_H */
