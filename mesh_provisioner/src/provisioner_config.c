#include "provisioner_config.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>         // uint*_t
#include <string.h>         // memset, memcpy

// NRF
#include "nrf_log.h"        // NRF_LOG_INFO
#include "sdk_errors.h"     // NRF_ERROR_*

// NRF MESH
#include "mesh_config.h"    // mesh_config_*, MESH_CONFIG_*

/*      STATIC VARIABLES & CONSTANTS                                */

// Live representation of the provisioner configuration.
static struct
{
    // Provisioner configuration header.
    prov_conf_header_entry_live_t   header;

    // Provisioner node data.
    prov_conf_node_entry_live_t     nodes[MAX_PROV_CONF_NODE_RECORDS];

} s_prov_conf_live;

uint32_t prov_conf_header_set_cb(mesh_config_entry_id_t id,
                                        const void* new_val)
{
    NRF_LOG_INFO("Provisioner configuration header setter!");

    memcpy(&(s_prov_conf_live.header), new_val,
           sizeof(prov_conf_header_entry_live_t));

    return NRF_SUCCESS;
}

void prov_conf_header_get_cb(mesh_config_entry_id_t id, void* buf)
{
    NRF_LOG_INFO("Provisioner configuration header getter!");

    memcpy(buf, &(s_prov_conf_live.header),
           sizeof(prov_conf_header_entry_live_t));
}

void prov_conf_header_delete_cb(mesh_config_entry_id_t id)
{
    NRF_LOG_INFO("Provisioner configuration header deleter!");

    memset(&(s_prov_conf_live.header), 0,
           sizeof(prov_conf_header_entry_live_t));
}

uint32_t prov_conf_node_set_cb(mesh_config_entry_id_t id,
                               const void* new_val)
{
    uint16_t node_idx = id.record - PROV_CONF_FIRST_NODE_RECORD_ID;

    if (node_idx >= MAX_PROV_CONF_NODE_RECORDS)
    {
        NRF_LOG_INFO("Setter called on invalid provisioner configuration node index!");
        return NRF_ERROR_INVALID_PARAM;
    }

    NRF_LOG_INFO("Provisioner configuration node setter on index %u!",
                 node_idx);

    memcpy(s_prov_conf_live.nodes + node_idx, new_val,
           sizeof(prov_conf_node_entry_live_t));

    return NRF_SUCCESS;
}

void prov_conf_node_get_cb(mesh_config_entry_id_t id, void* buf)
{
    uint16_t node_idx = id.record - PROV_CONF_FIRST_NODE_RECORD_ID;

    if (node_idx >= MAX_PROV_CONF_NODE_RECORDS)
    {
        NRF_LOG_INFO("Getter called on invalid provisioner configuration node index!");
        return;
    }

    NRF_LOG_INFO("Provisioner configuration node getter on index %u!",
                 node_idx);

    memcpy(buf, s_prov_conf_live.nodes + node_idx,
           sizeof(prov_conf_node_entry_live_t));
}

void prov_conf_node_delete_cb(mesh_config_entry_id_t id)
{
    uint16_t node_idx = id.record - PROV_CONF_FIRST_NODE_RECORD_ID;

    if (node_idx >= MAX_PROV_CONF_NODE_RECORDS)
    {
        NRF_LOG_INFO("Deleter called on invalid provisioner configuration node index!");
        return;
    }

    NRF_LOG_INFO("Provisioner configuration node deleter on index %u!",
                 node_idx);

    memset(s_prov_conf_live.nodes + node_idx, 0,
           sizeof(prov_conf_node_entry_live_t));
}
