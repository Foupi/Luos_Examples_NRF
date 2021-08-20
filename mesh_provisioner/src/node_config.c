#include "node_config.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>                 // uint16_t
#include <string.h>                 // memset

// NRF
#include "sdk_errors.h"             // ret_code_t

// NRF APPS
#include "app_error.h"              // APP_ERROR_CHECK

// MESH SDK
#include "access_status.h"          // ACCESS_STATUS_*
#include "device_state_manager.h"   // dsm_handle_t
#include "mesh_config.h"            // mesh_config_entry_*

// MESH MODELS
#include "config_client.h"          // config_client_*
#include "config_opcodes.h"         // CONFIG_OPCODE_*
#include "health_common.h"          // HEALTH_SERVER_MODEL_ID

// LUOS
#include "luos.h"                   // container_t, msg_t, Luos_SendMsg
#include "luos_utils.h"             // LUOS_ASSERT

// CUSTOM
#include "example_network_config.h" // NETWORK_*, *KEY_IDX
#include "network_ctx.h"            // PROV_*_IDX, g_network_ctx
#include "luos_msg_model_common.h"  // LUOS_MSG_MODEL_*
#include "luos_rtb_model_common.h"  // LUOS_RTB_MODEL_*
#include "provisioner_config.h"     // prov_conf_*
#include "provisioning.h"           // prov_scan_start

#ifdef DEBUG
#include "nrf_log.h"                // NRF_LOG_INFO
#endif /* DEBUG */

/*      TYPEDEFS                                                    */

typedef enum
{
    // Not configuring a node.
    NODE_CONFIG_STEP_IDLE,

    // Getting node composition data from remote node.
    NODE_CONFIG_STEP_COMPOSITION_GET,

    // Setting network transmission parameters for remote node.
    NODE_CONFIG_STEP_NETWORK_TRANSMIT,

    // Adding appkey to remode node.
    NODE_CONFIG_STEP_APPKEY_ADD,

    // Binding remote health server instance to appkey.
    NODE_CONFIG_STEP_APPKEY_BIND_HEALTH,

    /* Setting publish address of remote health server instance to
    ** address of the element hosting the health client instance.
    */
    NODE_CONFIG_STEP_PUBLISH_HEALTH,

    // Binding remote Luos RTB model instance to appkey.
    NODE_CONFIG_STEP_APPKEY_BIND_LUOS_RTB,

    /* Setting publish address of remote Luos RTB model instance to
    ** Luos models group address.
    */
    NODE_CONFIG_STEP_PUBLISH_LUOS_RTB,

    /* Adding Luos models group address to remote Luos RTB model
    ** instance subscription addresses.
    */
    NODE_CONFIG_STEP_SUBSCRIBE_LUOS_RTB,

    // Binding remote Luos MSG instance to appkey.
    NODE_CONFIG_STEP_APPKEY_BIND_LUOS_MSG,

    /* Setting publish address of remote Luos MSG model instance to
    ** Luos models group address.
    */
    NODE_CONFIG_STEP_PUBLISH_LUOS_MSG,

    /* Adding Luos models group address to remote Luos MSG model
    ** instance subscription addresses.
    */
    NODE_CONFIG_STEP_SUBSCRIBE_LUOS_MSG,

} node_config_step_t;

// Configuration step transition function.
typedef void (*node_config_step_transition_t)(void);

/*      STATIC VARIABLES & CONSTANTS                                */

// Mapping between configuration steps and expected config opcodes.
static const config_opcode_t                EXPECTED_OPCODES[]  =
{
    [NODE_CONFIG_STEP_COMPOSITION_GET]      = CONFIG_OPCODE_COMPOSITION_DATA_STATUS,
    [NODE_CONFIG_STEP_NETWORK_TRANSMIT]     = CONFIG_OPCODE_NETWORK_TRANSMIT_STATUS,
    [NODE_CONFIG_STEP_APPKEY_ADD]           = CONFIG_OPCODE_APPKEY_STATUS,
    [NODE_CONFIG_STEP_APPKEY_BIND_HEALTH]   = CONFIG_OPCODE_MODEL_APP_STATUS,
    [NODE_CONFIG_STEP_PUBLISH_HEALTH]       = CONFIG_OPCODE_MODEL_PUBLICATION_STATUS,
    [NODE_CONFIG_STEP_APPKEY_BIND_LUOS_RTB] = CONFIG_OPCODE_MODEL_APP_STATUS,
    [NODE_CONFIG_STEP_PUBLISH_LUOS_RTB]     = CONFIG_OPCODE_MODEL_PUBLICATION_STATUS,
    [NODE_CONFIG_STEP_SUBSCRIBE_LUOS_RTB]   = CONFIG_OPCODE_MODEL_SUBSCRIPTION_STATUS,
    [NODE_CONFIG_STEP_APPKEY_BIND_LUOS_MSG] = CONFIG_OPCODE_MODEL_APP_STATUS,
    [NODE_CONFIG_STEP_PUBLISH_LUOS_MSG]     = CONFIG_OPCODE_MODEL_PUBLICATION_STATUS,
    [NODE_CONFIG_STEP_SUBSCRIBE_LUOS_MSG]   = CONFIG_OPCODE_MODEL_SUBSCRIPTION_STATUS,
};

// Node configuration state.
static struct
{
    // Current configuration step.
    node_config_step_t  config_step;

    // Address of the currently configured element.
    uint16_t            elm_address;

}                               s_node_config_curr_state    =
{
    // Initialization to prevent problems on first configuration.
    .config_step    = NODE_CONFIG_STEP_IDLE,
};

// Remote device first composition page index.
static const uint8_t            FIRST_COMP_PAGE_IDX         = 0x00;

// Static provisioner instance for message sending.
static container_t*             s_mesh_provisioner_instance = NULL;

/*      STATIC FUNCTIONS                                            */

/* Sets the publication parameters for the instance of the given model
** ID located on the current configured element with the given address
** and publish period.
*/
static void publish_set(const access_model_id_t* target_model_id,
    const nrf_mesh_address_t* publish_address,
    const access_publish_period_t* publish_period);

/* Set current context on Composition Get step, then requests
** composition data.
*/
static void node_config_idle_to_composition_get(void);

/* Set current context on Network Transmit, then sets network transmit
** parameters.
*/
static void node_config_composition_get_to_network_transmit(void);

/* Set current context on Appkey Add, then adds the appkey to remote
** node.
*/
static void node_config_network_transmit_to_appkey_add(void);

/* Sets current context on Appkey Bind to Health Server, then binds the
** remote Health Server instance to the appkey.
*/
static void node_config_appkey_add_to_appkey_bind_health(void);

/* Sets current context on Publish Health Server, then defines the
** publish parameters of the remote Health Server instance to the
** Provisioner element address.
*/
static void node_config_appkey_bind_health_to_publish_health(void);

/* Sets current context on Appkey Bind to Luos RTB model, then binds the
** remote Luos RTB model instance to the appkey.
*/
static void node_config_publish_health_to_appkey_bind_luos_rtb(void);

/* Sets current context on Publish Luos RTB model, then defines the
** publish parameters of the remote Luos RTB model instance to the Luos
** models group address.
*/
static void node_config_appkey_bind_luos_rtb_to_publish_luos_rtb(void);

/* Sets current context on Subscribe Luos RTB model, then adds the Luos
** models group address to the subscription list of the remote Luos RTB
** model instance.
*/
static void node_config_publish_luos_rtb_to_subscribe_luos_rtb(void);

/* Sets current context on Appkey Bind to Luos MSG model, then binds the
** remote Luos MSG model instance to the appkey.
*/
static void node_config_subscribe_luos_rtb_to_appkey_bind_luos_msg(void);

/* Sets current context on Publish Luos MSG model, then defines the
** publish parameters of the remote Luos MSG model instance to the Luos
** models group address.
*/
static void node_config_appkey_bind_luos_msg_to_publish_luos_msg(void);

/* Sets current context on Subscribe Luos MSG model, then adds the Luos
** models group address to the subscription list of the remote Luos MSG
** model instance.
*/
static void node_config_publish_luos_msg_to_subscribe_luos_msg(void);

/* Increases the number of configured nodes in the persistent
** configuration, then sets the configuration state to Idle and starts
** scanning.
*/
static void node_config_success(void);

/*      INITIALIZATIONS                                             */

// Transition function for each configuration step.
static const node_config_step_transition_t  STEP_TRANSITIONS[]              =
{
    [NODE_CONFIG_STEP_IDLE]                 = node_config_idle_to_composition_get,
    [NODE_CONFIG_STEP_COMPOSITION_GET]      = node_config_composition_get_to_network_transmit,
    [NODE_CONFIG_STEP_NETWORK_TRANSMIT]     = node_config_network_transmit_to_appkey_add,
    [NODE_CONFIG_STEP_APPKEY_ADD]           = node_config_appkey_add_to_appkey_bind_health,
    [NODE_CONFIG_STEP_APPKEY_BIND_HEALTH]   = node_config_appkey_bind_health_to_publish_health,
    [NODE_CONFIG_STEP_PUBLISH_HEALTH]       = node_config_publish_health_to_appkey_bind_luos_rtb,
    [NODE_CONFIG_STEP_APPKEY_BIND_LUOS_RTB] = node_config_appkey_bind_luos_rtb_to_publish_luos_rtb,
    [NODE_CONFIG_STEP_PUBLISH_LUOS_RTB]     = node_config_publish_luos_rtb_to_subscribe_luos_rtb,
    [NODE_CONFIG_STEP_SUBSCRIBE_LUOS_RTB]   = node_config_subscribe_luos_rtb_to_appkey_bind_luos_msg,
    [NODE_CONFIG_STEP_APPKEY_BIND_LUOS_MSG] = node_config_appkey_bind_luos_msg_to_publish_luos_msg,
    [NODE_CONFIG_STEP_PUBLISH_LUOS_MSG]     = node_config_publish_luos_msg_to_subscribe_luos_msg,
    [NODE_CONFIG_STEP_SUBSCRIBE_LUOS_MSG]   = node_config_success,
};

static const access_model_id_t              HEALTH_SERVER_ACCESS_MODEL_ID   =
{
    .company_id = ACCESS_COMPANY_ID_NONE,
    .model_id   = HEALTH_SERVER_MODEL_ID,
};

void node_config_container_set(container_t* container)
{
    s_mesh_provisioner_instance = container;
}

void node_config_start(uint16_t device_first_addr,
                       dsm_handle_t devkey_handle,
                       dsm_handle_t address_handle)
{
    if (s_node_config_curr_state.config_step != NODE_CONFIG_STEP_IDLE)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Configuration start requested for node 0x%x while still configuring element 0x%x!",
                     device_first_addr,
                     s_node_config_curr_state.elm_address);
        #endif /* DEBUG */

        return;
    }

    #ifdef DEBUG
    NRF_LOG_INFO("Start configuration for node 0x%x! (devkey handle: 0x%x; address handle: 0x%x)",
                 device_first_addr, devkey_handle, address_handle);
    #endif /* DEBUG */

    ret_code_t err_code;

    err_code = config_client_server_bind(devkey_handle);
    APP_ERROR_CHECK(err_code);

    err_code = config_client_server_set(devkey_handle, address_handle);
    APP_ERROR_CHECK(err_code);

    s_node_config_curr_state.elm_address    = device_first_addr;

    node_config_idle_to_composition_get();
}

void config_client_msg_handler(const config_client_event_t* event)
{
    if (s_node_config_curr_state.config_step == NODE_CONFIG_STEP_IDLE)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Config client message received whild no configuration is occuring!");
        #endif /* DEBUG */

        return;
    }

    config_opcode_t expected_opcode = EXPECTED_OPCODES[s_node_config_curr_state.config_step];
    config_opcode_t                 received_opcode = event->opcode;

    if (received_opcode != expected_opcode)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Wrong opcode received: expected 0x%x, got 0x%x!",
                     expected_opcode, received_opcode);
        #endif /* DEBUG */

        return;
    }

    uint8_t                         status;

    switch (received_opcode)
    {
    case CONFIG_OPCODE_APPKEY_STATUS:
        status = event->p_msg->appkey_status.status;
        LUOS_ASSERT((status == ACCESS_STATUS_SUCCESS)
            || (status == ACCESS_STATUS_KEY_INDEX_ALREADY_STORED));

        break;

    case CONFIG_OPCODE_MODEL_APP_STATUS:
        status = event->p_msg->app_status.status;
        LUOS_ASSERT(status == ACCESS_STATUS_SUCCESS);

        break;

    case CONFIG_OPCODE_MODEL_PUBLICATION_STATUS:
        status = event->p_msg->publication_status.status;
        LUOS_ASSERT(status == ACCESS_STATUS_SUCCESS);

        break;

    default:
        // No status to check.
        break;
    }

    node_config_step_transition_t   next_transition = STEP_TRANSITIONS[s_node_config_curr_state.config_step];

    if (next_transition == NULL)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Opcode 0x%x received but not managed yet!",
                     received_opcode);
        #endif /* DEBUG */

        return;
    }

    next_transition();
}

static void publish_set(const access_model_id_t* target_model_id,
    const nrf_mesh_address_t* publish_address,
    const access_publish_period_t* publish_period)
{
    ret_code_t                  err_code;

    config_publication_state_t  pub_state;
    memset(&pub_state, 0, sizeof(config_publication_state_t));
    pub_state.element_address   = s_node_config_curr_state.elm_address;
    pub_state.publish_address   = *publish_address;
    pub_state.appkey_index      = PROV_APPKEY_IDX;
    pub_state.publish_ttl       = ACCESS_DEFAULT_TTL;
    pub_state.publish_period    = *publish_period;
    pub_state.retransmit_count  = 1;
    pub_state.model_id          = *target_model_id;

    err_code = config_client_model_publication_set(&pub_state);
    APP_ERROR_CHECK(err_code);
}

static void node_config_idle_to_composition_get(void)
{
    s_node_config_curr_state.config_step    = NODE_CONFIG_STEP_COMPOSITION_GET;

    ret_code_t err_code;

    err_code = config_client_composition_data_get(FIRST_COMP_PAGE_IDX);
    APP_ERROR_CHECK(err_code);
}

static void node_config_composition_get_to_network_transmit(void)
{
    #ifdef DEBUG
    NRF_LOG_INFO("Composition data received!");
    #endif /* DEBUG */

    s_node_config_curr_state.config_step    = NODE_CONFIG_STEP_NETWORK_TRANSMIT;

    ret_code_t err_code;

    err_code = config_client_network_transmit_set(NETWORK_TRANSMIT_COUNT,
        NETWORK_TRANSMIT_INTERVAL_STEPS);
    APP_ERROR_CHECK(err_code);
}

static void node_config_network_transmit_to_appkey_add(void)
{
    #ifdef DEBUG
    NRF_LOG_INFO("Network transmit parameters set!");
    #endif /* DEBUG */

    s_node_config_curr_state.config_step    = NODE_CONFIG_STEP_APPKEY_ADD;

    ret_code_t err_code;

    err_code = config_client_appkey_add(PROV_NETKEY_IDX,
                                        PROV_APPKEY_IDX,
                                        g_network_ctx.appkey);
    APP_ERROR_CHECK(err_code);
}

static void node_config_appkey_add_to_appkey_bind_health(void)
{
    #ifdef DEBUG
    NRF_LOG_INFO("Appkey added to device!");
    #endif /* DEBUG */

    s_node_config_curr_state.config_step    = NODE_CONFIG_STEP_APPKEY_BIND_HEALTH;

    ret_code_t err_code;

    err_code = config_client_model_app_bind(s_node_config_curr_state.elm_address,
        PROV_APPKEY_IDX, HEALTH_SERVER_ACCESS_MODEL_ID);
    APP_ERROR_CHECK(err_code);
}

static void node_config_appkey_bind_health_to_publish_health(void)
{
    #ifdef DEBUG
    NRF_LOG_INFO("Appkey bound to remote Health Server Instance!");
    #endif /* DEBUG */

    s_node_config_curr_state.config_step    = NODE_CONFIG_STEP_PUBLISH_HEALTH;

    nrf_mesh_address_t          health_client_address;
    memset(&health_client_address, 0, sizeof(nrf_mesh_address_t));
    health_client_address.type  = NRF_MESH_ADDRESS_TYPE_UNICAST;
    health_client_address.value = PROV_ELM_ADDRESS;

    access_publish_period_t     publish_period;
    memset(&publish_period, 0, sizeof(access_publish_period_t));
    publish_period.step_num     = 1;
    publish_period.step_res     = ACCESS_PUBLISH_RESOLUTION_10S;

    publish_set(&HEALTH_SERVER_ACCESS_MODEL_ID, &health_client_address,
                &publish_period);
}

static void node_config_publish_health_to_appkey_bind_luos_rtb(void)
{
    #ifdef DEBUG
    NRF_LOG_INFO("Publish address set for remote Health Server!");
    #endif /* DEBUG */

    s_node_config_curr_state.config_step    = NODE_CONFIG_STEP_APPKEY_BIND_LUOS_RTB;

    ret_code_t          err_code;
    access_model_id_t   luos_rtb_model_id   = LUOS_RTB_MODEL_ACCESS_ID;

    err_code = config_client_model_app_bind(s_node_config_curr_state.elm_address,
                                            PROV_APPKEY_IDX,
                                            luos_rtb_model_id);
    APP_ERROR_CHECK(err_code);
}

static void node_config_appkey_bind_luos_rtb_to_publish_luos_rtb(void)
{
    #ifdef DEBUG
    NRF_LOG_INFO("Appkey bound to remote Luos RTB model instance!");
    #endif /* DEBUG */

    s_node_config_curr_state.config_step    = NODE_CONFIG_STEP_PUBLISH_LUOS_RTB;

    access_model_id_t   luos_rtb_model_id   = LUOS_RTB_MODEL_ACCESS_ID;

    nrf_mesh_address_t      luos_group_address;
    memset(&luos_group_address, 0, sizeof(nrf_mesh_address_t));
    luos_group_address.type     = NRF_MESH_ADDRESS_TYPE_GROUP;
    luos_group_address.value    = LUOS_GROUP_ADDRESS;

    access_publish_period_t publish_period;
    memset(&publish_period, 0, sizeof(access_publish_period_t));
    publish_period.step_res     = ACCESS_PUBLISH_RESOLUTION_100MS;

    publish_set(&luos_rtb_model_id, &luos_group_address,
                &publish_period);
}

static void node_config_publish_luos_rtb_to_subscribe_luos_rtb(void)
{
    #ifdef DEBUG
    NRF_LOG_INFO("Publish address set for remote Luos RTB model instance!");
    #endif /* DEBUG */

    s_node_config_curr_state.config_step    = NODE_CONFIG_STEP_SUBSCRIBE_LUOS_RTB;

    ret_code_t          err_code;
    access_model_id_t   luos_rtb_model_id   = LUOS_RTB_MODEL_ACCESS_ID;

    nrf_mesh_address_t  luos_group_address;
    memset(&luos_group_address, 0, sizeof(nrf_mesh_address_t));
    luos_group_address.type     = NRF_MESH_ADDRESS_TYPE_GROUP;
    luos_group_address.value    = LUOS_GROUP_ADDRESS;

    err_code = config_client_model_subscription_add(s_node_config_curr_state.elm_address,
        luos_group_address, luos_rtb_model_id);
    APP_ERROR_CHECK(err_code);
}

static void node_config_subscribe_luos_rtb_to_appkey_bind_luos_msg(void)
{
    #ifdef DEBUG
    NRF_LOG_INFO("Remote Luos RTB model instance subsribed to Luos group address!");
    #endif /* DEBUG */

    s_node_config_curr_state.config_step    = NODE_CONFIG_STEP_APPKEY_BIND_LUOS_MSG;

    ret_code_t          err_code;
    access_model_id_t   luos_msg_model_id   = LUOS_MSG_MODEL_ACCESS_ID;

    err_code = config_client_model_app_bind(s_node_config_curr_state.elm_address,
                                            PROV_APPKEY_IDX,
                                            luos_msg_model_id);
    APP_ERROR_CHECK(err_code);
}

static void node_config_appkey_bind_luos_msg_to_publish_luos_msg(void)
{
    #ifdef DEBUG
    NRF_LOG_INFO("Appkey bound to remote Luos MSG model instance!");
    #endif /* DEBUG */

    s_node_config_curr_state.config_step    = NODE_CONFIG_STEP_PUBLISH_LUOS_MSG;

    access_model_id_t   luos_msg_model_id   = LUOS_MSG_MODEL_ACCESS_ID;

    nrf_mesh_address_t      luos_group_address;
    memset(&luos_group_address, 0, sizeof(nrf_mesh_address_t));
    luos_group_address.type     = NRF_MESH_ADDRESS_TYPE_GROUP;
    luos_group_address.value    = LUOS_GROUP_ADDRESS;

    access_publish_period_t publish_period;
    memset(&publish_period, 0, sizeof(access_publish_period_t));
    publish_period.step_res     = ACCESS_PUBLISH_RESOLUTION_100MS;

    publish_set(&luos_msg_model_id, &luos_group_address,
                &publish_period);
}

static void node_config_publish_luos_msg_to_subscribe_luos_msg(void)
{
    #ifdef DEBUG
    NRF_LOG_INFO("Publish address set for remote Luos MSG model instance!");
    #endif /* DEBUG */

    s_node_config_curr_state.config_step    = NODE_CONFIG_STEP_SUBSCRIBE_LUOS_MSG;

    ret_code_t          err_code;
    access_model_id_t   luos_msg_model_id   = LUOS_MSG_MODEL_ACCESS_ID;

    nrf_mesh_address_t  luos_group_address;
    memset(&luos_group_address, 0, sizeof(nrf_mesh_address_t));
    luos_group_address.type     = NRF_MESH_ADDRESS_TYPE_GROUP;
    luos_group_address.value    = LUOS_GROUP_ADDRESS;

    err_code = config_client_model_subscription_add(s_node_config_curr_state.elm_address,
        luos_group_address, luos_msg_model_id);
    APP_ERROR_CHECK(err_code);
}

static void node_config_success(void)
{
    #ifdef DEBUG
    NRF_LOG_INFO("Configuration process successfully completed!");
    #endif /* DEBUG */

    prov_conf_header_entry_live_t   tmp_conf_header;
    mesh_config_entry_get(PROV_CONF_HEADER_ENTRY_ID, &tmp_conf_header);

    tmp_conf_header.nb_conf_nodes++;

    mesh_config_entry_set(PROV_CONF_HEADER_ENTRY_ID, &tmp_conf_header);

    s_node_config_curr_state.config_step    = NODE_CONFIG_STEP_IDLE;

    bool can_scan   = prov_scan_start();
    if ((!can_scan) && (s_mesh_provisioner_instance != NULL))
    {
        msg_t   cannot_scan;
        memset(&cannot_scan, 0, sizeof(msg_t));
        cannot_scan.header.target_mode  = BROADCAST;
        cannot_scan.header.target       = BROADCAST_VAL;
        cannot_scan.header.cmd          = IO_STATE;
        cannot_scan.header.size         = sizeof(uint8_t);
        cannot_scan.data[0]             = 0x00;

        Luos_SendMsg(s_mesh_provisioner_instance, &cannot_scan);
    }
}
