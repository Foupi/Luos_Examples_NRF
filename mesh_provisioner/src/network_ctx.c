#include "network_ctx.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>                 // uint*_t
#include <string.h>                 // memset

// NRF
#include "nrf_log.h"                // NRF_LOG_INFO
#include "sdk_errors.h"             // ret_code_t

// NRF APPS
#include "app_error.h"              // APP_ERROR_CHECK

// MESH SDK
#include "device_state_manager.h"   // dsm_*
#include "rand.h"                   // rand_hw_rng_get

// MESH MODELS
#include "config_server.h"          // config_server_*

// CUSTOM
#include "app_health_client.h"      // app_health_client_*
#include "mesh_init.h"              // g_device_provisioned
#include "provisioning.h"           // PROV_*

/*      STATIC/GLOBAL VARIABLES & CONSTANTS                         */

network_ctx_t   g_network_ctx;

// Amounts of expected key indexes when fetching from persistent data.
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
        NRF_LOG_INFO("Fetching network context from persistent storage!");
        network_ctx_fetch();
    }
    else
    {
        NRF_LOG_INFO("Generating network context!");
        network_ctx_generate();
    }
}

static void models_bind(dsm_handle_t devkey_handle,
                        dsm_handle_t appkey_handle)
{
    ret_code_t err_code = config_server_bind(devkey_handle);
    APP_ERROR_CHECK(err_code);

    app_health_client_bind(appkey_handle);
}

static void network_ctx_generate(void)
{
    ret_code_t                  err_code;

    dsm_local_unicast_address_t local_addr_range;
    memset(&local_addr_range, 0, sizeof(dsm_local_unicast_address_t));
    local_addr_range.address_start  = PROV_ELM_ADDRESS;
    local_addr_range.count          = ACCESS_ELEMENT_COUNT;

    err_code = dsm_local_unicast_addresses_set(&local_addr_range);
    APP_ERROR_CHECK(err_code);

    rand_hw_rng_get(g_network_ctx.netkey, NRF_MESH_KEY_SIZE);
    err_code = dsm_subnet_add(PROV_NETKEY_IDX, g_network_ctx.netkey,
                              &(g_network_ctx.netkey_handle));
    APP_ERROR_CHECK(err_code);

    rand_hw_rng_get(g_network_ctx.appkey, NRF_MESH_KEY_SIZE);
    err_code = dsm_appkey_add(PROV_APPKEY_IDX,
                              g_network_ctx.netkey_handle,
                              g_network_ctx.appkey,
                              &(g_network_ctx.appkey_handle));
    APP_ERROR_CHECK(err_code);

    rand_hw_rng_get(g_network_ctx.self_devkey, NRF_MESH_KEY_SIZE);
    err_code = dsm_devkey_add(PROV_ELM_ADDRESS,
                              g_network_ctx.netkey_handle,
                              g_network_ctx.self_devkey,
                              &(g_network_ctx.self_devkey_handle));
    APP_ERROR_CHECK(err_code);

    models_bind(g_network_ctx.self_devkey_handle,
                g_network_ctx.appkey_handle);
}

static void network_ctx_fetch(void)
{
    ret_code_t                  err_code;
    dsm_local_unicast_address_t local_addr_range;
    dsm_local_unicast_addresses_get(&local_addr_range);

    // Temporary variables to ease writing.
    uint32_t                    nb_indexes          = 1;
    mesh_key_index_t            key_index_buffer;
    dsm_handle_t                curr_handle;

    err_code = dsm_subnet_get_all(&key_index_buffer, &nb_indexes);
    APP_ERROR_CHECK(err_code);
    if (nb_indexes != NB_NETKEY_IDX)
    {
        // FIXME Manage error.
        NRF_LOG_INFO("Number of indexes: %u! (Expected %u).",
                     nb_indexes, NB_NETKEY_IDX);
    }

    curr_handle = dsm_net_key_index_to_subnet_handle(key_index_buffer);
    if (curr_handle == DSM_HANDLE_INVALID)
    {
        // FIXME Manage error.
    }

    err_code = dsm_subnet_key_get(curr_handle, g_network_ctx.netkey);
    APP_ERROR_CHECK(err_code);

    g_network_ctx.netkey_handle = curr_handle;

    err_code = dsm_appkey_get_all(g_network_ctx.netkey_handle,
                                  &key_index_buffer, &nb_indexes);
    APP_ERROR_CHECK(err_code);
    if (nb_indexes != NB_APPKEY_IDX)
    {
        // FIXME Manage error.
    }

    curr_handle = dsm_appkey_index_to_appkey_handle(key_index_buffer);
    if (curr_handle == DSM_HANDLE_INVALID)
    {
        // FIXME Manage error.
    }

    g_network_ctx.appkey_handle = curr_handle;

    err_code = dsm_devkey_handle_get(local_addr_range.address_start,
                                     &curr_handle);
    APP_ERROR_CHECK(err_code);
    if (curr_handle == DSM_HANDLE_INVALID)
    {
        // FIXME Manage error.
    }

    g_network_ctx.self_devkey_handle = curr_handle;
}
