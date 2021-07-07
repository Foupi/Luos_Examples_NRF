#include "provisioning.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>                     // uint*_t
#include <string.h>                     // memset

// NRF
#include "nrf_log.h"                    // NRF_LOG_INFO
#include "sdk_errors.h"                 // ret_code_t

// NRF APPS
#include "app_error.h"                  // APP_ERROR_CHECK

// MESH SDK
#include "device_state_manager.h"       // dsm_*
#include "mesh_config.h"                // mesh_config_*, MESH_CONFIG_*
#include "mesh_stack.h"                 // mesh_stack_*
#include "nrf_mesh.h"                   // nrf_mesh_*
#include "nrf_mesh_prov.h"              /* nrf_mesh_prov_*,
                                        ** NRF_MESH_PROV_*
                                        */
#include "nrf_mesh_prov_bearer.h"       // prov_bearer_t
#include "nrf_mesh_prov_bearer_adv.h"   // nrf_mesh_prov_bearer_adv_*
#include "rand.h"                       // rand_hw_rng_get

// MESH MODELS
#include "config_server.h"              // config_server_*
#include "config_server_events.h"       // config_server_evt_t

// CUSTOM
#include "luos_mesh_common.h"           /* _mesh_init, _provisioning_init,
                                        ** encryption_keys_generate,
                                        */
#include "provisioner_config.h"         // prov_conf_*

/*      TYPEDEFS                                                    */

// Enum representing the current provisioning state.
typedef enum
{
    // Neither scanning nor provisioning.
    PROV_STATE_IDLE,

    // Scanning for unprovisioned devices.
    PROV_STATE_SCANNING,

    // Provisioning a device.
    PROV_STATE_PROVISIONING,

    // Waiting for link closure after successful provisioning.
    PROV_STATE_COMPLETE,

} prov_state_t;

/*      STATIC VARIABLES & CONSTANTS                                */

// Describes if the device is provisioned.
static bool                         s_device_provisioned    = false;

// Provisioning context.
static nrf_mesh_prov_ctx_t          s_prov_ctx;

// Provisioner element address.
static const uint16_t               PROV_ELM_ADDRESS        = 0x0001;

// Key indexes.
static const mesh_key_index_t       PROV_NETKEY_IDX         = 0x0000;
static const mesh_key_index_t       PROV_APPKEY_IDX         = 0x0000;

// Amounts of expected key indexes when fetching from persistent data.
static const uint32_t               NB_NETKEY_IDX           = 1;
static const uint32_t               NB_APPKEY_IDX           = 1;

// Unprovisioned device attention duration, in seconds.
static const uint32_t               ATTENTION_DURATION      = 5;

// Static authentication data (FIXME copied from Mesh examples).
static const uint8_t                AUTH_DATA[]             = {
    0x6E, 0x6F, 0x72, 0x64, 0x69, 0x63, 0x5F, 0x65,
    0x78, 0x61, 0x6D, 0x70, 0x6C, 0x65, 0x5F, 0x31,
};

// Information regarding the current state and provisioned device.
static struct
{
    // Current provisioning state.
    prov_state_t                    prov_state;

    // Current provisioning configuration header state.
    prov_conf_header_entry_live_t   prov_conf_header;

    // Current node being approvisioned.
    prov_conf_node_entry_live_t     curr_conf_node;

    // Device key and address handles for each approvisioned node.
    dsm_handle_t                    devkey_handles[MAX_PROV_CONF_NODE_RECORDS];
    dsm_handle_t                    address_handles[MAX_PROV_CONF_NODE_RECORDS];

}                                   s_prov_curr_state;

// Network context: keys and handles.
static struct
{
    // Key handles.
    dsm_handle_t    netkey_handle;
    dsm_handle_t    appkey_handle;
    dsm_handle_t    self_devkey_handle;

    // Keys values.
    uint8_t         netkey[NRF_MESH_KEY_SIZE];
    uint8_t         appkey[NRF_MESH_KEY_SIZE];
    uint8_t         self_devkey[NRF_MESH_KEY_SIZE];

}                                   s_network_ctx;

/*      INITIALIZATIONS                                             */

// Provisioner configuration file.
MESH_CONFIG_FILE(
    s_prov_conf_file,
    PROV_CONF_FILE_ID,
    MESH_CONFIG_STRATEGY_CONTINUOUS
);

// Provisioner configuration header entry.
MESH_CONFIG_ENTRY(
    s_prov_conf_header_entry,
    PROV_CONF_HEADER_ENTRY_ID,
    1,
    sizeof(prov_conf_header_entry_live_t),
    prov_conf_header_set_cb,
    prov_conf_header_get_cb,
    prov_conf_header_delete_cb,
    false
);

// Provisioner connfiguration node entries.
MESH_CONFIG_ENTRY(
    s_prov_conf_first_node_entry,
    PROV_CONF_NODE_ENTRY_ID(0),
    MAX_PROV_CONF_NODE_RECORDS,
    sizeof(prov_conf_node_entry_live_t),
    prov_conf_node_set_cb,
    prov_conf_node_get_cb,
    prov_conf_node_delete_cb,
    false
);

/*      STATIC FUNCTIONS                                            */

/* Generates keys and handles, stores them in the DSM and in the static
** context.
*/
static void network_ctx_generate(void);

/* Fetches the handles stored in the DSM and keeps them in the static
** context.
*/
static void network_ctx_fetch(void);

/*      CALLBACKS                                                   */

// On node reset event, erases persistent data and resets the board.
static void config_server_event_cb(const config_server_evt_t* event);

// Initializes the config client and health client models.
static void models_init_cb(void);

/* Caps received:           Use received capacities for
**                          OOB authentication.
** Static request:          Provide static authentication data.
** Complete:                Prepare remote config server setup.
** Link closed:             Add node to static context if provisioning
**                          was successful.
*/
static void mesh_prov_event_cb(const nrf_mesh_prov_evt_t* event);

// Unprovisioned device:    Start provisioning the advertising device.
static void mesh_unprov_event_cb(const nrf_mesh_prov_evt_t* event);

void mesh_init(void)
{
    _mesh_init(config_server_event_cb, models_init_cb,
               &s_device_provisioned);
}

void provisioning_init(void)
{
    _provisioning_init(&s_prov_ctx, mesh_prov_event_cb);

    s_prov_curr_state.prov_state = PROV_STATE_IDLE;
}

void prov_conf_init(void)
{
    if (s_device_provisioned)
    {
        NRF_LOG_INFO("Retrieving persistent configuration!");

        mesh_config_entry_get(PROV_CONF_HEADER_ENTRY_ID,
                              &(s_prov_curr_state.prov_conf_header));

        uint32_t                    err_code;
        prov_conf_node_entry_live_t node_conf_buffer;
        uint16_t                    nb_prov_nodes       = s_prov_curr_state.prov_conf_header.nb_prov_nodes;

        NRF_LOG_INFO("Number of provisioned nodes: %u!", nb_prov_nodes);

        for (uint16_t node_entry_idx = 0; node_entry_idx < nb_prov_nodes;
             node_entry_idx++)
        {
            NRF_LOG_INFO("Retrieving config for node %u!",
                         node_entry_idx);

            mesh_config_entry_get(PROV_CONF_NODE_ENTRY_ID(node_entry_idx),
                                  &node_conf_buffer);

            uint16_t    node_unicast_addr   = node_conf_buffer.first_addr;

            NRF_LOG_INFO("Unicast address: %u!", node_unicast_addr);

            err_code = dsm_devkey_handle_get(node_conf_buffer.first_addr,
                s_prov_curr_state.devkey_handles + node_entry_idx);
            APP_ERROR_CHECK(err_code);

            NRF_LOG_INFO("Devkey handle: 0x%x!",
                s_prov_curr_state.devkey_handles[node_entry_idx]);

            nrf_mesh_address_t device_address;
            memset(&device_address, 0, sizeof(nrf_mesh_address_t));
            device_address.type             = NRF_MESH_ADDRESS_TYPE_UNICAST;
            device_address.value            = node_unicast_addr;

            err_code = dsm_address_handle_get(&device_address,
                s_prov_curr_state.address_handles + node_entry_idx);
            APP_ERROR_CHECK(err_code);

            NRF_LOG_INFO("Address handle: 0x%x!",
                s_prov_curr_state.address_handles[node_entry_idx]);
        }

        if (nb_prov_nodes > s_prov_curr_state.prov_conf_header.nb_conf_nodes)
        {
            NRF_LOG_INFO("Last provisioned node must still be configured!");

            /* One node has been provisioned but not configured: store
            ** it in current context so that it can be configured right
            ** away.
            */
            s_prov_curr_state.curr_conf_node = node_conf_buffer;
        }
    }
    else
    {
        s_prov_curr_state.prov_conf_header.nb_prov_nodes    = 0;
        s_prov_curr_state.prov_conf_header.nb_conf_nodes    = 0;
        s_prov_curr_state.prov_conf_header.next_address     = PROV_ELM_ADDRESS + ACCESS_ELEMENT_COUNT;

        mesh_config_entry_set(PROV_CONF_HEADER_ENTRY_ID,
                              &(s_prov_curr_state.prov_conf_header));

        NRF_LOG_INFO("Generating persistent configuration!");
    }
}

void network_ctx_init(void)
{
    if (s_device_provisioned)
    {
        NRF_LOG_INFO("Fetching network context from persistent storage!");
        network_ctx_fetch();
    }
    else
    {
        NRF_LOG_INFO("Generating network context!");
        network_ctx_generate();
    }

    NRF_LOG_INFO("Netkey handle: 0x%x; Appkey handle: 0x%x; Self devkey handle: 0x%x!",
                 s_network_ctx.netkey_handle,
                 s_network_ctx.appkey_handle,
                 s_network_ctx.self_devkey_handle);
}

void mesh_start(void)
{
    ret_code_t err_code = mesh_stack_start();
    APP_ERROR_CHECK(err_code);
}

void prov_scan_start(void)
{
    ret_code_t err_code = nrf_mesh_prov_scan_start(mesh_unprov_event_cb);
    APP_ERROR_CHECK(err_code);

    s_prov_curr_state.prov_state = PROV_STATE_SCANNING;

    NRF_LOG_INFO("Start scanning for unprovisioned devices!");
}

void prov_scan_stop(void)
{
    nrf_mesh_prov_scan_stop();

    if (s_prov_curr_state.prov_state == PROV_STATE_SCANNING)
    {
        s_prov_curr_state.prov_state = PROV_STATE_IDLE;
    }

    NRF_LOG_INFO("Stop scanning for unprovisioned devices!");
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

    rand_hw_rng_get(s_network_ctx.netkey, NRF_MESH_KEY_SIZE);
    err_code = dsm_subnet_add(PROV_NETKEY_IDX, s_network_ctx.netkey,
                              &(s_network_ctx.netkey_handle));
    APP_ERROR_CHECK(err_code);

    rand_hw_rng_get(s_network_ctx.appkey, NRF_MESH_KEY_SIZE);
    err_code = dsm_appkey_add(PROV_APPKEY_IDX,
                              s_network_ctx.netkey_handle,
                              s_network_ctx.appkey,
                              &(s_network_ctx.appkey_handle));
    APP_ERROR_CHECK(err_code);

    rand_hw_rng_get(s_network_ctx.self_devkey, NRF_MESH_KEY_SIZE);
    err_code = dsm_devkey_add(PROV_ELM_ADDRESS,
                              s_network_ctx.netkey_handle,
                              s_network_ctx.self_devkey,
                              &(s_network_ctx.self_devkey_handle));
    APP_ERROR_CHECK(err_code);

    err_code = config_server_bind(s_network_ctx.self_devkey_handle);
    APP_ERROR_CHECK(err_code);
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

    err_code = dsm_subnet_key_get(curr_handle, s_network_ctx.netkey);
    APP_ERROR_CHECK(err_code);

    s_network_ctx.netkey_handle = curr_handle;

    err_code = dsm_appkey_get_all(s_network_ctx.netkey_handle,
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

    s_network_ctx.appkey_handle = curr_handle;

    err_code = dsm_devkey_handle_get(local_addr_range.address_start,
                                     &curr_handle);
    APP_ERROR_CHECK(err_code);
    if (curr_handle == DSM_HANDLE_INVALID)
    {
        // FIXME Manage error.
    }

    s_network_ctx.self_devkey_handle = curr_handle;
}

static void config_server_event_cb(const config_server_evt_t* event)
{
    if (event->type == CONFIG_SERVER_EVT_NODE_RESET)
    {
        // FIXME Erase persistent data.

        // FIXME Reset board.
    }
}

static void models_init_cb(void)
{
    s_network_ctx.netkey_handle         = DSM_HANDLE_INVALID;
    s_network_ctx.appkey_handle         = DSM_HANDLE_INVALID;
    s_network_ctx.self_devkey_handle    = DSM_HANDLE_INVALID;

    NRF_LOG_INFO("Initializing %u models!", ACCESS_MODEL_COUNT);
}

static void mesh_prov_event_cb(const nrf_mesh_prov_evt_t* event)
{
    ret_code_t              err_code;
    nrf_mesh_prov_ctx_t*    device_prov_ctx;

    switch (event->type)
    {
    case NRF_MESH_PROV_EVT_LINK_ESTABLISHED:
        if (s_prov_curr_state.prov_state != PROV_STATE_PROVISIONING)
        {
            NRF_LOG_INFO("Provisioning link established while not provisioning!");
            return;
        }

        NRF_LOG_INFO("Provisioning link established with unprovisioned device!");
        break;

    case NRF_MESH_PROV_EVT_CAPS_RECEIVED:
        if (s_prov_curr_state.prov_state != PROV_STATE_PROVISIONING)
        {
            NRF_LOG_INFO("OOB capacities received while not provisioning!");
            return;
        }

    {
        NRF_LOG_INFO("Capacities received from unprovisioned device!");

        nrf_mesh_prov_evt_caps_received_t   oob_caps_received   = event->params.oob_caps_received;
        device_prov_ctx                                         = oob_caps_received.p_context;

        s_prov_curr_state.curr_conf_node.nb_elm                 = oob_caps_received.oob_caps.num_elements;

        err_code = nrf_mesh_prov_oob_use(device_prov_ctx,
                                         NRF_MESH_PROV_OOB_METHOD_STATIC,
                                         0,
                                         NRF_MESH_KEY_SIZE);
        APP_ERROR_CHECK(err_code);
    }
        break;

    case NRF_MESH_PROV_EVT_STATIC_REQUEST:
        if (s_prov_curr_state.prov_state != PROV_STATE_PROVISIONING)
        {
            NRF_LOG_INFO("Static authentication data request received while not provisioning!");
            return;
        }

        NRF_LOG_INFO("Static authentication data requested by unprovisioned device!");

        device_prov_ctx = event->params.static_request.p_context;

        err_code = nrf_mesh_prov_auth_data_provide(device_prov_ctx,
                                                   AUTH_DATA,
                                                   NRF_MESH_KEY_SIZE);
        APP_ERROR_CHECK(err_code);
        break;

    case NRF_MESH_PROV_EVT_FAILED:
        if (s_prov_curr_state.prov_state != PROV_STATE_PROVISIONING)
        {
            NRF_LOG_INFO("Provisioning failed event received while not provisioning!");
            return;
        }

        NRF_LOG_INFO("Provisioning procedure failed! Reason code: %u!",
                     event->params.failed.failure_code);

        break;

    case NRF_MESH_PROV_EVT_COMPLETE:
        if (s_prov_curr_state.prov_state != PROV_STATE_PROVISIONING)
        {
            NRF_LOG_INFO("Provisioning complete event received while not provisioning!");
            return;
        }

    {
        s_prov_curr_state.prov_state                            = PROV_STATE_COMPLETE;

        nrf_mesh_prov_evt_complete_t    prov_complete           = event->params.complete;
        uint16_t                        device_first_address    = prov_complete.p_prov_data->address;
        const uint8_t*                  device_devkey           = prov_complete.p_devkey;
        uint16_t                        node_idx                = s_prov_curr_state.prov_conf_header.nb_prov_nodes;
        uint16_t                        node_nb_elm             = s_prov_curr_state.curr_conf_node.nb_elm;

        NRF_LOG_INFO("Provisioning process complete for device 0x%x!",
                     device_first_address);


        err_code = dsm_address_publish_add(device_first_address,
            s_prov_curr_state.address_handles + node_idx);
        APP_ERROR_CHECK(err_code);

        err_code = dsm_devkey_add(device_first_address,
            s_network_ctx.netkey_handle, device_devkey,
            s_prov_curr_state.devkey_handles + node_idx);
        APP_ERROR_CHECK(err_code);

        s_prov_curr_state.prov_conf_header.next_address         = device_first_address + node_nb_elm;
        s_prov_curr_state.prov_conf_header.nb_prov_nodes++;
        mesh_config_entry_set(PROV_CONF_HEADER_ENTRY_ID,
                              &(s_prov_curr_state.prov_conf_header));

        s_prov_curr_state.curr_conf_node.first_addr             = device_first_address;
        mesh_config_entry_set(PROV_CONF_NODE_ENTRY_ID(node_idx),
                              &(s_prov_curr_state.curr_conf_node));

        // FIXME Start configuration.
    }

        break;

    case NRF_MESH_PROV_EVT_LINK_CLOSED:
        switch (s_prov_curr_state.prov_state)
        {
        case PROV_STATE_COMPLETE:
            NRF_LOG_INFO("Provisioning link closed after successful procedure!");

            // FIXME Start configuration.

            prov_scan_start();

            break;

        case PROV_STATE_PROVISIONING:
            NRF_LOG_INFO("Provisioning procedure was aborted!");

            prov_scan_start();

            break;

        default:
            NRF_LOG_INFO("Provisioning link closed event received in inadequate state!");
            break;
        }

        break;

    default:
        NRF_LOG_INFO("Mesh provisioning event received: type %u!",
                     event->type);
        break;
    }
}

static void mesh_unprov_event_cb(const nrf_mesh_prov_evt_t* event)
{
    if (event->type != NRF_MESH_PROV_EVT_UNPROVISIONED_RECEIVED)
    {
        // Only manage unprovisioned device events.
        return;
    }

    if (s_prov_curr_state.prov_state != PROV_STATE_SCANNING)
    {
        NRF_LOG_INFO("Unprovisioned device detected while not scanning!");

        return;
    }

    NRF_LOG_INFO("Start provisioning detected device!");

    nrf_mesh_prov_provisioning_data_t   prov_data;
    memset(&prov_data, 0, sizeof(nrf_mesh_prov_provisioning_data_t));
    prov_data.netkey_index  = PROV_NETKEY_IDX;
    prov_data.address       = s_prov_curr_state.prov_conf_header.next_address;
    memcpy(prov_data.netkey, s_network_ctx.netkey, NRF_MESH_KEY_SIZE);

    encryption_keys_generate();

    const uint8_t*                      target_uuid = event->params.unprov.device_uuid;
    ret_code_t                          err_code;
    err_code = nrf_mesh_prov_provision(&s_prov_ctx, target_uuid,
                                       ATTENTION_DURATION, &prov_data,
                                       NRF_MESH_PROV_BEARER_ADV);
    APP_ERROR_CHECK(err_code);

    prov_scan_stop();

    s_prov_curr_state.prov_state = PROV_STATE_PROVISIONING;
}
