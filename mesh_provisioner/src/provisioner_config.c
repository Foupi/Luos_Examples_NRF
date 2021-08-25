#include "provisioner_config.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>             // uint*_t
#include <string.h>             // memset, memcpy

// NRF
#include "sdk_errors.h"         // NRF_ERROR_*

// MESH SDK
#include "mesh_config.h"        // mesh_config_*, MESH_CONFIG_*

// LUOS
#include "luos_utils.h"         // LUOS_ASSERT

// CUSTOM
#include "luos_mesh_common.h"   // LUOS_MESH_NETWORK_MAX_NODES

#ifdef DEBUG
#include "nrf_log.h"            // NRF_LOG_INFO
#endif /* DEBUG */

/*      STATIC VARIABLES & CONSTANTS                                */

// Live representation of the provisioner configuration.
static struct
{
    // Provisioner configuration header.
    prov_conf_header_entry_live_t   header;

    // Provisioner node data.
    prov_conf_node_entry_live_t     nodes[LUOS_MESH_NETWORK_MAX_NODES];

} s_prov_conf_live;

uint32_t prov_conf_header_set_cb(mesh_config_entry_id_t id,
                                 const void* new_val)
{
    #ifdef DEBUG
    NRF_LOG_INFO("Provisioner configuration header setter!");
    #endif /* DEBUG */

    // Set live value to given value.
    memcpy(&(s_prov_conf_live.header), new_val,
           sizeof(prov_conf_header_entry_live_t));

    // Always succeeds.
    return NRF_SUCCESS;
}

void prov_conf_header_get_cb(mesh_config_entry_id_t id, void* buf)
{
    #ifdef DEBUG
    NRF_LOG_INFO("Provisioner configuration header getter!");
    #endif /* DEBUG */

    // Copy live value into given buffer.
    memcpy(buf, &(s_prov_conf_live.header),
           sizeof(prov_conf_header_entry_live_t));
}

void prov_conf_header_delete_cb(mesh_config_entry_id_t id)
{
    #ifdef DEBUG
    NRF_LOG_INFO("Provisioner configuration header deleter!");
    #endif /* DEBUG */

    // Erases live value.
    memset(&(s_prov_conf_live.header), 0,
           sizeof(prov_conf_header_entry_live_t));
}

uint32_t prov_conf_node_set_cb(mesh_config_entry_id_t id,
                               const void* new_val)
{
    // Record index in config file.
    uint16_t    node_idx;
    node_idx    = id.record - PROV_CONF_FIRST_NODE_RECORD_ID;

    if (node_idx >= LUOS_MESH_NETWORK_MAX_NODES)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Setter called on invalid provisioner configuration node index!");
        #endif /* DEBUG */

        return NRF_ERROR_INVALID_PARAM;
    }

    #ifdef DEBUG
    NRF_LOG_INFO("Provisioner configuration node setter on index %u!",
                 node_idx);
    #endif /* DEBUG */

    // Set live value of given entry to given value.
    memcpy(s_prov_conf_live.nodes + node_idx, new_val,
           sizeof(prov_conf_node_entry_live_t));

    return NRF_SUCCESS;
}

void prov_conf_node_get_cb(mesh_config_entry_id_t id, void* buf)
{
    // Record index in config file.
    uint16_t    node_idx;
    node_idx    = id.record - PROV_CONF_FIRST_NODE_RECORD_ID;

    // Check index.
    LUOS_ASSERT(node_idx < LUOS_MESH_NETWORK_MAX_NODES);

    #ifdef DEBUG
    NRF_LOG_INFO("Provisioner configuration node getter on index %u!",
                 node_idx);
    #endif /* DEBUG */

    // Copy live value of given entry into given buffer.
    memcpy(buf, s_prov_conf_live.nodes + node_idx,
           sizeof(prov_conf_node_entry_live_t));
}

void prov_conf_node_delete_cb(mesh_config_entry_id_t id)
{
    // Record index in config file.
    uint16_t    node_idx;
    node_idx    = id.record - PROV_CONF_FIRST_NODE_RECORD_ID;

    // Check index.
    LUOS_ASSERT(node_idx < LUOS_MESH_NETWORK_MAX_NODES);

    #ifdef DEBUG
    NRF_LOG_INFO("Provisioner configuration node deleter on index %u!",
                 node_idx);
    #endif /* DEBUG */

    // Erases live value at given entry.
    memset(s_prov_conf_live.nodes + node_idx, 0,
           sizeof(prov_conf_node_entry_live_t));
}
