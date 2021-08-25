#include "network_ctx.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>                 // uint*_t
#include <string.h>                 // memset

// NRF
#include "sdk_errors.h"             // ret_code_t

// NRF APPS
#include "app_error.h"              // APP_ERROR_CHECK

// MESH SDK
#include "device_state_manager.h"   // dsm_*
#include "rand.h"                   // rand_hw_rng_get

// MESH MODELS
#include "config_server.h"          // config_server_*

// LUOS
#include "luos_utils.h"             // LUOS_ASSERT

// CUSTOM
#include "app_health_client.h"      // app_health_client_*
#include "mesh_init.h"              // g_device_provisioned
#include "provisioning.h"           // PROV_*

#ifdef DEBUG
#include "nrf_log.h"                // NRF_LOG_INFO
#endif /* DEBUG */

/*      STATIC/GLOBAL VARIABLES & CONSTANTS                         */

// Global network context.
network_ctx_t                       g_network_ctx;

// Amounts of key indexes expected when fetching from persistent data.
static const uint32_t               NB_NETKEY_IDX           = 1;
static const uint32_t               NB_APPKEY_IDX           = 1;

/*      STATIC FUNCTIONS                                            */

// Binds the models to the given handles.
static void models_bind(dsm_handle_t devkey_handle,
                        dsm_handle_t appkey_handle);

/* Generates keys and handles, stores them in the DSM and in the static
** context.
*/
static void network_ctx_generate(void);

/* Fetches the handles stored in the DSM and keeps them in the static
** context.
*/
static void network_ctx_fetch(void);

void network_ctx_init(void)
{
    if (g_device_provisioned)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Fetching network context from persistent storage!");
        #endif /* DEBUG */

        // Fetch context if device is provisioned.
        network_ctx_fetch();
    }
    else
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Generating network context!");
        #endif /* DEBUG */

        // Generate context if device is not provisioned.
        network_ctx_generate();
    }
}

static void models_bind(dsm_handle_t devkey_handle,
                        dsm_handle_t appkey_handle)
{
    // Bind internal Config Server instance to the given devkey.
    ret_code_t err_code = config_server_bind(devkey_handle);
    APP_ERROR_CHECK(err_code);

    // Bind internal Health Client instance to the given appkey.
    app_health_client_bind(appkey_handle);
}

static void network_ctx_generate(void)
{
    ret_code_t                  err_code;

    // Range of unicast addresses hosted on this node.
    dsm_local_unicast_address_t local_addr_range;
    memset(&local_addr_range, 0, sizeof(dsm_local_unicast_address_t));
    local_addr_range.address_start  = PROV_ELM_ADDRESS;     // First address on this node.
    local_addr_range.count          = ACCESS_ELEMENT_COUNT; // Number of elements on this node.

    // Set local addresses range in DSM.
    err_code    = dsm_local_unicast_addresses_set(&local_addr_range);
    APP_ERROR_CHECK(err_code);

    // Generate random netkey.
    rand_hw_rng_get(g_network_ctx.netkey, NRF_MESH_KEY_SIZE);

    // Add a new sub-network in DSM using the given netkey.
    err_code    = dsm_subnet_add(PROV_NETKEY_IDX, g_network_ctx.netkey,
                                 &(g_network_ctx.netkey_handle));
    APP_ERROR_CHECK(err_code);

    // Generate random appkey.
    rand_hw_rng_get(g_network_ctx.appkey, NRF_MESH_KEY_SIZE);

    /* Add a new application attached to the given netkey in DSM using
    ** the given appkey.
    */
    err_code    = dsm_appkey_add(PROV_APPKEY_IDX,
                                 g_network_ctx.netkey_handle,
                                 g_network_ctx.appkey,
                                 &(g_network_ctx.appkey_handle));
    APP_ERROR_CHECK(err_code);

    // Generate random devkey.
    rand_hw_rng_get(g_network_ctx.self_devkey, NRF_MESH_KEY_SIZE);

    /* Add a new device correspoding to the given address and attached
    ** to the given netkey in DSM using the given devkey.
    */
    err_code    = dsm_devkey_add(PROV_ELM_ADDRESS,
                                 g_network_ctx.netkey_handle,
                                 g_network_ctx.self_devkey,
                                 &(g_network_ctx.self_devkey_handle));
    APP_ERROR_CHECK(err_code);

    // Bind internal model instances to devkey and appkey.
    models_bind(g_network_ctx.self_devkey_handle,
                g_network_ctx.appkey_handle);
}

static void network_ctx_fetch(void)
{
    ret_code_t                  err_code;

    // Get range of addresses hosted on device.
    dsm_local_unicast_address_t local_addr_range;
    dsm_local_unicast_addresses_get(&local_addr_range);

    // Temporary variables to ease writing.
    uint32_t                    nb_indexes          = 1;
    mesh_key_index_t            key_index_buffer;
    dsm_handle_t                curr_handle;

    // Get all netkey indexes from DSM.
    err_code    = dsm_subnet_get_all(&key_index_buffer, &nb_indexes);
    APP_ERROR_CHECK(err_code);

    // Check received number of netkeys.
    LUOS_ASSERT(nb_indexes == NB_NETKEY_IDX);

    // Retrieve netkey handle from DSM using given netkey index.
    curr_handle = dsm_net_key_index_to_subnet_handle(key_index_buffer);

    // Check result.
    LUOS_ASSERT(curr_handle != DSM_HANDLE_INVALID);

    // Retrieve netkey from DSM using given netkey handle.
    err_code    = dsm_subnet_key_get(curr_handle, g_network_ctx.netkey);
    APP_ERROR_CHECK(err_code);

    // Store netkey handle in global context.
    g_network_ctx.netkey_handle = curr_handle;

    // Get all appkey indexes attached to the given netkey from DSM.
    err_code    = dsm_appkey_get_all(g_network_ctx.netkey_handle,
                                     &key_index_buffer, &nb_indexes);
    APP_ERROR_CHECK(err_code);

    // Check received number of appkeys.
    LUOS_ASSERT(nb_indexes == NB_APPKEY_IDX);

    // Retrieve appkey handle from DSM using given appkey index.
    curr_handle = dsm_appkey_index_to_appkey_handle(key_index_buffer);

    // Check result.
    LUOS_ASSERT(curr_handle != DSM_HANDLE_INVALID);

    // Store appkey handle in global context.
    g_network_ctx.appkey_handle = curr_handle;

    // Retrieve devkey from DSM using given local addresses range start.
    err_code    = dsm_devkey_handle_get(local_addr_range.address_start,
                                        &curr_handle);
    APP_ERROR_CHECK(err_code);

    // Check result.
    LUOS_ASSERT(curr_handle != DSM_HANDLE_INVALID);

    // Store devkey handle in global context.
    g_network_ctx.self_devkey_handle = curr_handle;
}
