#include "provisioning.h"

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
#include "mesh_config.h"            // mesh_config_*, MESH_CONFIG_*
#include "mesh_stack.h"             // mesh_stack_*
#include "nrf_mesh.h"               // nrf_mesh_*
#include "nrf_mesh_prov.h"          /* nrf_mesh_prov_*,
                                    ** NRF_MESH_PROV_*
                                    */

// MESH MODELS
#include "config_server.h"          // config_server_*
#include "config_server_events.h"   // config_server_evt_t

// CUSTOM
#include "luos_mesh_common.h"       /* _provisioning_init,
                                    ** encryption_keys_generate,
                                    ** auth_data_provide,
                                    ** LUOS_MESH_NETWORK_MAX_NODES
                                    */
#include "mesh_init.h"              // g_device_provisioned
#include "network_ctx.h"            // network_ctx_*
#include "node_config.h"            // node_config_start
#include "provisioner_config.h"     // prov_conf_*

#ifdef DEBUG
#include "nrf_log.h"                // NRF_LOG_INFO
#endif /* DEBUG */

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

// Provisioning context.
static nrf_mesh_prov_ctx_t  s_prov_ctx;

// Unprovisioned device attention duration, in seconds.
static const uint32_t       ATTENTION_DURATION      = 5;

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
    dsm_handle_t                    devkey_handles[LUOS_MESH_NETWORK_MAX_NODES];
    dsm_handle_t                    address_handles[LUOS_MESH_NETWORK_MAX_NODES];

}                           s_prov_curr_state;

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
    LUOS_MESH_NETWORK_MAX_NODES,
    sizeof(prov_conf_node_entry_live_t),
    prov_conf_node_set_cb,
    prov_conf_node_get_cb,
    prov_conf_node_delete_cb,
    false
);

/*      CALLBACKS                                                   */

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

void provisioning_init(void)
{
    _provisioning_init(&s_prov_ctx, mesh_prov_event_cb);

    s_prov_curr_state.prov_state = PROV_STATE_IDLE;
}

void prov_conf_init(void)
{
    if (g_device_provisioned)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Retrieving persistent configuration!");
        #endif /* DEBUG */

        mesh_config_entry_get(PROV_CONF_HEADER_ENTRY_ID,
                              &(s_prov_curr_state.prov_conf_header));

        uint32_t                    err_code;
        prov_conf_node_entry_live_t node_conf_buffer;
        uint16_t                    nb_prov_nodes       = s_prov_curr_state.prov_conf_header.nb_prov_nodes;

        #ifdef DEBUG
        NRF_LOG_INFO("Number of provisioned nodes: %u!", nb_prov_nodes);
        #endif /* DEBUG */

        for (uint16_t node_entry_idx = 0; node_entry_idx < nb_prov_nodes;
             node_entry_idx++)
        {
            #ifdef DEBUG
            NRF_LOG_INFO("Retrieving config for node %u!",
                         node_entry_idx);
            #endif /* DEBUG */

            mesh_config_entry_get(PROV_CONF_NODE_ENTRY_ID(node_entry_idx),
                                  &node_conf_buffer);

            uint16_t    node_unicast_addr   = node_conf_buffer.first_addr;

            err_code = dsm_devkey_handle_get(node_conf_buffer.first_addr,
                s_prov_curr_state.devkey_handles + node_entry_idx);
            APP_ERROR_CHECK(err_code);

            nrf_mesh_address_t device_address;
            memset(&device_address, 0, sizeof(nrf_mesh_address_t));
            device_address.type             = NRF_MESH_ADDRESS_TYPE_UNICAST;
            device_address.value            = node_unicast_addr;

            err_code = dsm_address_handle_get(&device_address,
                s_prov_curr_state.address_handles + node_entry_idx);
            APP_ERROR_CHECK(err_code);

            #ifdef DEBUG
            NRF_LOG_INFO("Unicast address: 0x%x, devkey handle: 0x%x, address handle: 0x%x!",
                         node_unicast_addr,
                         s_prov_curr_state.devkey_handles[node_entry_idx],
                         s_prov_curr_state.address_handles[node_entry_idx]);
            #endif /* DEBUG */
        }

        if (nb_prov_nodes > s_prov_curr_state.prov_conf_header.nb_conf_nodes)
        {
            #ifdef DEBUG
            NRF_LOG_INFO("Last provisioned node must still be configured!");
            #endif /* DEBUG */

            /* One node has been provisioned but not configured: store
            ** it in current context so that it can be configured right
            ** away.
            */
            s_prov_curr_state.curr_conf_node = node_conf_buffer;
        }

        // Needed since appkey can not be retrieved from DSM.
        memcpy(g_network_ctx.appkey,
               s_prov_curr_state.prov_conf_header.appkey,
               NRF_MESH_KEY_SIZE);
    }
    else
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Generating persistent configuration!");
        #endif /* DEBUG */

        s_prov_curr_state.prov_conf_header.nb_prov_nodes    = 0;
        s_prov_curr_state.prov_conf_header.nb_conf_nodes    = 0;
        s_prov_curr_state.prov_conf_header.next_address     = PROV_ELM_ADDRESS + ACCESS_ELEMENT_COUNT;

        // Store previously computed appkey.
        memcpy(s_prov_curr_state.prov_conf_header.appkey,
               g_network_ctx.appkey, NRF_MESH_KEY_SIZE);

        mesh_config_entry_set(PROV_CONF_HEADER_ENTRY_ID,
                              &(s_prov_curr_state.prov_conf_header));
    }
}

bool prov_scan_start(void)
{
    if (s_prov_curr_state.prov_conf_header.nb_prov_nodes >= LUOS_MESH_NETWORK_MAX_NODES)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Cannot start scanning: max number of devices in Mesh network reached!");
        #endif /* DEBUG */

        return false;
    }

    ret_code_t err_code = nrf_mesh_prov_scan_start(mesh_unprov_event_cb);
    APP_ERROR_CHECK(err_code);

    s_prov_curr_state.prov_state = PROV_STATE_SCANNING;

    #ifdef DEBUG
    NRF_LOG_INFO("Start scanning for unprovisioned devices!");
    #endif /* DEBUG */

    return true;
}

void prov_scan_stop(void)
{
    nrf_mesh_prov_scan_stop();

    if (s_prov_curr_state.prov_state == PROV_STATE_SCANNING)
    {
        s_prov_curr_state.prov_state = PROV_STATE_IDLE;
    }

    #ifdef DEBUG
    NRF_LOG_INFO("Stop scanning for unprovisioned devices!");
    #endif /* DEBUG */
}

static void mesh_prov_event_cb(const nrf_mesh_prov_evt_t* event)
{
    ret_code_t              err_code;
    nrf_mesh_prov_ctx_t*    device_prov_ctx;
    uint16_t                node_idx;

    switch (event->type)
    {
    case NRF_MESH_PROV_EVT_LINK_ESTABLISHED:
        if (s_prov_curr_state.prov_state != PROV_STATE_PROVISIONING)
        {
            #ifdef DEBUG
            NRF_LOG_INFO("Provisioning link established while not provisioning!");
            #endif /* DEBUG */

            return;
        }

        #ifdef DEBUG
        NRF_LOG_INFO("Provisioning link established with unprovisioned device!");
        #endif /* DEBUG */
        break;

    case NRF_MESH_PROV_EVT_CAPS_RECEIVED:
    {
        if (s_prov_curr_state.prov_state != PROV_STATE_PROVISIONING)
        {
            #ifdef DEBUG
            NRF_LOG_INFO("OOB capacities received while not provisioning!");
            #endif /* DEBUG */

            return;
        }

        #ifdef DEBUG
        NRF_LOG_INFO("Capacities received from unprovisioned device!");
        #endif /* DEBUG */

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
            #ifdef DEBUG
            NRF_LOG_INFO("Static authentication data request received while not provisioning!");
            #endif /* DEBUG */

            return;
        }

        #ifdef DEBUG
        NRF_LOG_INFO("Static authentication data requested by unprovisioned device!");
        #endif /* DEBUG */

        device_prov_ctx = event->params.static_request.p_context;
        auth_data_provide(device_prov_ctx);

        break;

    case NRF_MESH_PROV_EVT_FAILED:
        if (s_prov_curr_state.prov_state != PROV_STATE_PROVISIONING)
        {
            #ifdef DEBUG
            NRF_LOG_INFO("Provisioning failed event received while not provisioning!");
            #endif /* DEBUG */

            return;
        }

        #ifdef DEBUG
        NRF_LOG_INFO("Provisioning procedure failed! Reason code: %u!",
                     event->params.failed.failure_code);
        #endif /* DEBUG */

        break;

    case NRF_MESH_PROV_EVT_COMPLETE:
    {
        if (s_prov_curr_state.prov_state != PROV_STATE_PROVISIONING)
        {
            #ifdef DEBUG
            NRF_LOG_INFO("Provisioning complete event received while not provisioning!");
            #endif /* DEBUG */

            return;
        }

        s_prov_curr_state.prov_state                            = PROV_STATE_COMPLETE;
        node_idx                                                = s_prov_curr_state.prov_conf_header.nb_prov_nodes;

        nrf_mesh_prov_evt_complete_t    prov_complete           = event->params.complete;
        uint16_t                        device_first_address    = prov_complete.p_prov_data->address;
        const uint8_t*                  device_devkey           = prov_complete.p_devkey;
        uint16_t                        node_nb_elm             = s_prov_curr_state.curr_conf_node.nb_elm;

        #ifdef DEBUG
        NRF_LOG_INFO("Provisioning process complete for device 0x%x!",
                     device_first_address);
        #endif /* DEBUG */


        err_code = dsm_address_publish_add(device_first_address,
            s_prov_curr_state.address_handles + node_idx);
        APP_ERROR_CHECK(err_code);

        err_code = dsm_devkey_add(device_first_address,
            g_network_ctx.netkey_handle, device_devkey,
            s_prov_curr_state.devkey_handles + node_idx);
        APP_ERROR_CHECK(err_code);

        s_prov_curr_state.prov_conf_header.next_address         = device_first_address + node_nb_elm;
        s_prov_curr_state.prov_conf_header.nb_prov_nodes++;
        mesh_config_entry_set(PROV_CONF_HEADER_ENTRY_ID,
                              &(s_prov_curr_state.prov_conf_header));

        s_prov_curr_state.curr_conf_node.first_addr             = device_first_address;
        mesh_config_entry_set(PROV_CONF_NODE_ENTRY_ID(node_idx),
                              &(s_prov_curr_state.curr_conf_node));
    }

        break;

    case NRF_MESH_PROV_EVT_LINK_CLOSED:
        switch (s_prov_curr_state.prov_state)
        {
        case PROV_STATE_COMPLETE:
            #ifdef DEBUG
            NRF_LOG_INFO("Provisioning link closed after successful procedure!");
            #endif /* DEBUG */

            s_prov_curr_state.prov_state    = PROV_STATE_IDLE;
            node_idx                        = s_prov_curr_state.prov_conf_header.nb_prov_nodes - 1;

            node_config_start(s_prov_curr_state.curr_conf_node.first_addr,
                              s_prov_curr_state.devkey_handles[node_idx],
                              s_prov_curr_state.address_handles[node_idx]);

            break;

        case PROV_STATE_PROVISIONING:
            #ifdef DEBUG
            NRF_LOG_INFO("Provisioning procedure was aborted!");
            #endif /* DEBUG */

            prov_scan_start();

            break;

        default:
            #ifdef DEBUG
            NRF_LOG_INFO("Provisioning link closed event received in inadequate state!");
            #endif /* DEBUG */

            break;
        }

        break;

    default:
        #ifdef DEBUG
        NRF_LOG_INFO("Mesh provisioning event received: type %u!",
                     event->type);
        #endif /* DEBUG */

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
        #ifdef DEBUG
        NRF_LOG_INFO("Unprovisioned device detected while not scanning!");
        #endif /* DEBUG */

        return;
    }

    #ifdef DEBUG
    NRF_LOG_INFO("Start provisioning detected device!");
    #endif /* DEBUG */

    nrf_mesh_prov_provisioning_data_t   prov_data;
    memset(&prov_data, 0, sizeof(nrf_mesh_prov_provisioning_data_t));
    prov_data.netkey_index  = PROV_NETKEY_IDX;
    prov_data.address       = s_prov_curr_state.prov_conf_header.next_address;
    memcpy(prov_data.netkey, g_network_ctx.netkey, NRF_MESH_KEY_SIZE);

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
