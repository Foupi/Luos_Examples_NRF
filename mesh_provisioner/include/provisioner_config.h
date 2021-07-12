#ifndef PROVISIONER_CONFIG_H
#define PROVISIONER_CONFIG_H

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>         // uint*_t

// NRF MESH
#include "mesh_config.h"    // MESH_CONFIG_ENTRY_ID
#include "mesh_opt.h"       // MESH_OPT_FIRST_FREE_ID

/*      DEFINES                                                     */

// Provisioner configuration file ID.
#define PROV_CONF_FILE_ID                   MESH_OPT_FIRST_FREE_ID

// Provisioner configuration header record and entry IDs.
#define PROV_CONF_HEADER_RECORD_ID          0x0001
#define PROV_CONF_HEADER_ENTRY_ID                                       \
    MESH_CONFIG_ENTRY_ID(PROV_CONF_FILE_ID, PROV_CONF_HEADER_RECORD_ID)

// Provisioner configuration node record and entry IDs.
#define PROV_CONF_FIRST_NODE_RECORD_ID      PROV_CONF_HEADER_RECORD_ID + 1
#define PROV_CONF_NODE_ENTRY_ID(__node_idx)                             \
    MESH_CONFIG_ENTRY_ID(PROV_CONF_FILE_ID,                             \
                         PROV_CONF_FIRST_NODE_RECORD_ID + (__node_idx))

// Maximum number of node configuration records.
#define MAX_PROV_CONF_NODE_RECORDS          8

/*      TYPEDEFS                                                    */

// Live representation of a header config entry.
typedef struct
{
    // Number of approvisioned nodes.
    uint8_t     nb_prov_nodes;

    // Number of configured nodes.
    uint8_t     nb_conf_nodes;

    // Target address for the next unprovisioned device.
    uint16_t    next_address;

    /* Application key (has to be copied because not retrievable through
    ** DSM)
    */
    uint8_t     appkey[NRF_MESH_KEY_SIZE];

} prov_conf_header_entry_live_t;

// Live representation of a node config entry.
typedef struct
{
    // First unicast address of the node.
    uint16_t    first_addr;

    // Number of elements in the node.
    uint16_t    nb_elm;

} prov_conf_node_entry_live_t;

// Provisioning configuration header callbacks.
uint32_t prov_conf_header_set_cb(mesh_config_entry_id_t id,
                                 const void* new_val);
void     prov_conf_header_get_cb(mesh_config_entry_id_t id, void* buf);
void     prov_conf_header_delete_cb(mesh_config_entry_id_t id);

// Provisioning configuration node callbacks.
uint32_t prov_conf_node_set_cb(mesh_config_entry_id_t id,
                               const void* new_val);
void     prov_conf_node_get_cb(mesh_config_entry_id_t id, void* buf);
void     prov_conf_node_delete_cb(mesh_config_entry_id_t id);

#endif /* PROVISIONER_CONFIG_H */
