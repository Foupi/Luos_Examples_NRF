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
#include "luos_mesh_common.h"       // LUOS_GROUP_ADDRESS
#include "luos_msg_model_common.h"  // LUOS_MSG_MODEL_*
#include "luos_rtb_model_common.h"  // LUOS_RTB_MODEL_*
#include "provisioner_config.h"     // prov_conf_*
#include "provisioning.h"           // prov_scan_start

#ifdef DEBUG
#include "nrf_log.h"                // NRF_LOG_INFO
#else /* ! DEBUG */
#include "boards.h"                 /* bsp_board_led_off,
                                    ** bsp_board_led_on
                                    */
#endif /* DEBUG */

/*      TYPEDEFS                                                    */

// Steps in the configuration of a node.
typedef enum
{
    // Not configuring a node.
    NODE_CONFIG_STEP_IDLE,

    // Getting node composition data from remote node.
    NODE_CONFIG_STEP_COMPOSITION_GET,

    // Setting network transmission parameters for remote node.
    NODE_CONFIG_STEP_NETWORK_TRANSMIT,

    // Adding appkey to remote node.
    NODE_CONFIG_STEP_APPKEY_ADD,

    // Binding remote Health Server instance to appkey.
    NODE_CONFIG_STEP_APPKEY_BIND_HEALTH,

    /* Setting publish address of remote Health Server instance to
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

    // Binding remote Luos MSG model instance to appkey.
    NODE_CONFIG_STEP_APPKEY_BIND_LUOS_MSG,

    /* Setting publish address of remote Luos MSG model instance to
    ** Luos models group address.
    */
    NODE_CONFIG_STEP_PUBLISH_LUOS_MSG,

    /* Adding Luos models group address to remote Luos MSG model
    ** instance subscription addresses.
    */
    NODE_CONFIG_STEP_SUBSCRIBE_LUOS_MSG,

    // Last config step.
    NODE_CONFIG_STEP_END,

} node_config_step_t;

// Configuration step transition function.
typedef void (*node_config_step_transition_t)(void);

/*      STATIC VARIABLES & CONSTANTS                                */

/* Mapping between configuration steps and expected Config model
** opcodes.
*/
static const config_opcode_t    EXPECTED_OPCODES[]          =
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

#ifdef DEBUG
// Debug message printed at completion of each step.
static const char*              DEBUG_MESSAGES[]            =
{
    [NODE_CONFIG_STEP_COMPOSITION_GET]      = "Composition data received!",
    [NODE_CONFIG_STEP_NETWORK_TRANSMIT]     = "Network transmit parameters set!",
    [NODE_CONFIG_STEP_APPKEY_ADD]           = "Appkey added to device!",
    [NODE_CONFIG_STEP_APPKEY_BIND_HEALTH]   = "Appkey bound to remote Health Server Instance!",
    [NODE_CONFIG_STEP_PUBLISH_HEALTH]       = "Publish address set for remote Health Server!",
    [NODE_CONFIG_STEP_APPKEY_BIND_LUOS_RTB] = "Appkey bound to remote Luos RTB model instance!",
    [NODE_CONFIG_STEP_PUBLISH_LUOS_RTB]     = "Publish address set for remote Luos RTB model instance!",
    [NODE_CONFIG_STEP_SUBSCRIBE_LUOS_RTB]   = "Remote Luos RTB model instance subsribed to Luos group address!",
    [NODE_CONFIG_STEP_APPKEY_BIND_LUOS_MSG] = "Appkey bound to remote Luos MSG model instance!",
    [NODE_CONFIG_STEP_PUBLISH_LUOS_MSG]     = "Publish address set for remote Luos MSG model instance!",
    [NODE_CONFIG_STEP_SUBSCRIBE_LUOS_MSG]   = "Remote Luos MSG model instance subscribed to Luos group address!",
};
#endif /* DEBUG */

// Default address for configuration element.
#define                         DEFAULT_CONF_ELM_ADDR       0xFFFF

// Node configuration state.
static struct
{
    // Current node configuration step.
    node_config_step_t  config_step;

    // Address of the currently configured element.
    uint16_t            elm_address;

}                               s_node_config_curr_state    =
{
    // Initialization to prevent problems on first configuration.
    .config_step    = NODE_CONFIG_STEP_IDLE,

    // Start with default element address.
    .elm_address    = DEFAULT_CONF_ELM_ADDR,
};

// Remote device first composition page index.
static const uint8_t            FIRST_COMP_PAGE_IDX         = 0x00;

// NRF-formatted Luos Group address.
static const nrf_mesh_address_t LUOS_GROUP_NRF_ADDRESS      =
{
    // Group address.
    .type   = NRF_MESH_ADDRESS_TYPE_GROUP,

    // Value defined in `luos_mesh_common.h`
    .value  = LUOS_GROUP_ADDRESS,
};

// Static provisioner instance for message sending.
static container_t*             s_mesh_provisioner_instance = NULL;

#ifndef DEBUG
// LED toggled to match configuration state.
static const uint32_t           DEFAULT_NODE_CONF_LED       = 2;
#endif /* ! DEBUG */

/*      STATIC FUNCTIONS                                            */

#ifndef DEBUG
// Visually show node configuration start and completion.
void indicate_configuration_begin(void);
void indicate_configuration_end(void);
#endif /* ! DEBUG */

/* Sets the publication parameters for the instance of the given model
** ID located on the current configured element with the given address
** and publish period.
*/
static void publish_set(const access_model_id_t* target_model_id,
    const nrf_mesh_address_t* publish_address,
    const access_publish_period_t* publish_period);

// Requests composition data from remote node.
static void node_config_idle_to_composition_get(void);

// Sets network transmit parameters with remote node.
static void node_config_composition_get_to_network_transmit(void);

// Adds the appkey to remote node.
static void node_config_network_transmit_to_appkey_add(void);

// Binds the remote Health Server instance to the appkey.
static void node_config_appkey_add_to_appkey_bind_health(void);

/* Defines the publish parameters of the remote Health Server instance
** to the Provisioner element address.
*/
static void node_config_appkey_bind_health_to_publish_health(void);

// Binds the remote Luos RTB model instance to the appkey.
static void node_config_publish_health_to_appkey_bind_luos_rtb(void);

/* Defines the publish parameters of the remote Luos RTB model instance
** to the Luos models group address.
*/
static void node_config_appkey_bind_luos_rtb_to_publish_luos_rtb(void);

/* Adds the Luos models group address to the subscription list of the
** remote Luos RTB model instance.
*/
static void node_config_publish_luos_rtb_to_subscribe_luos_rtb(void);

// Binds the remote Luos MSG model instance to the appkey.
static void node_config_subscribe_luos_rtb_to_appkey_bind_luos_msg(void);

/* Defines the publish parameters of the remote Luos MSG model instance
** to the Luos models group address.
*/
static void node_config_appkey_bind_luos_msg_to_publish_luos_msg(void);

/* Adds the Luos models group address to the subscription list of the
** remote Luos MSG model instance.
*/
static void node_config_publish_luos_msg_to_subscribe_luos_msg(void);

/* Increases the number of configured nodes in the persistent
** configuration, then sets the configuration state to Idle and tries
** to start scanning again.
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

// Instanciation of Health Server complete access ID.
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
    #else /* ! DEBUG */
    indicate_configuration_begin();
    #endif /* DEBUG */

    ret_code_t  err_code;

    // Bind remote Config Server instance to given devkey.
    err_code                                = config_client_server_bind(
                                                devkey_handle
                                              );
    APP_ERROR_CHECK(err_code);

    // Set remote Config Server instance to publish with given appkey.
    err_code                                = config_client_server_set(
                                                devkey_handle,
                                                address_handle
                                              );
    APP_ERROR_CHECK(err_code);

    // Store given address in static context.
    s_node_config_curr_state.elm_address    = device_first_addr;

    // Configuration start.
    s_node_config_curr_state.config_step    = NODE_CONFIG_STEP_COMPOSITION_GET;
    node_config_idle_to_composition_get();
}

void config_client_msg_handler(const config_client_event_t* event)
{
    // Check parameter.
    LUOS_ASSERT(event != NULL);

    if (s_node_config_curr_state.config_step == NODE_CONFIG_STEP_IDLE)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Config client message received while no configuration is occuring!");
        #endif /* DEBUG */

        return;
    }

    // Expected opcode depending on current configuration step.
    config_opcode_t                 expected_opcode;
    expected_opcode = EXPECTED_OPCODES[s_node_config_curr_state.config_step];

    // Opcode received in event.
    config_opcode_t                 received_opcode;
    received_opcode = event->opcode;

    if (received_opcode != expected_opcode)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Wrong opcode received: expected 0x%x, got 0x%x!",
                     expected_opcode, received_opcode);
        #endif /* DEBUG */

        return;
    }

    // Status received in event.
    uint8_t                         status;

    switch (received_opcode)
    {
    case CONFIG_OPCODE_APPKEY_STATUS:
        // Response to an Appkey Add command.

        status  = event->p_msg->appkey_status.status;
        LUOS_ASSERT((status == ACCESS_STATUS_SUCCESS)
            || (status == ACCESS_STATUS_KEY_INDEX_ALREADY_STORED));

        break;

    case CONFIG_OPCODE_MODEL_APP_STATUS:
        // Response to an Appkey Bind command.

        status  = event->p_msg->app_status.status;
        LUOS_ASSERT(status == ACCESS_STATUS_SUCCESS);

        break;

    case CONFIG_OPCODE_MODEL_PUBLICATION_STATUS:
        // Response to a Publish Set command.

        status  = event->p_msg->publication_status.status;
        LUOS_ASSERT(status == ACCESS_STATUS_SUCCESS);

        break;

    case CONFIG_OPCODE_MODEL_SUBSCRIPTION_STATUS:
        // Response to a Subscription Add command.

        status  = event->p_msg->subscription_status.status;
        LUOS_ASSERT(status == ACCESS_STATUS_SUCCESS);

        break;

    default:
        // No status to check.
        break;
    }

    #ifdef DEBUG
    // Debug message depending on current configuration step.
    const char*                     debug_msg;
    debug_msg       = DEBUG_MESSAGES[s_node_config_curr_state.config_step];

    if (debug_msg != NULL)
    {
        NRF_LOG_INFO("%s", debug_msg);
    }
    #endif /* DEBUG */

    // Step transition function depending on current configuration step.
    node_config_step_transition_t   next_transition;
    next_transition = STEP_TRANSITIONS[s_node_config_curr_state.config_step];

    if (next_transition == NULL)
    {
        #ifdef DEBUG
        NRF_LOG_INFO("Opcode 0x%x received but not managed yet!",
                     received_opcode);
        #endif /* DEBUG */

        return;
    }

    // Switch step.
    s_node_config_curr_state.config_step++;

    // Call step transition function.
    next_transition();
}

#ifndef DEBUG
__attribute__((weak)) void indicate_configuration_begin(void)
{
    bsp_board_led_on(DEFAULT_NODE_CONF_LED);
}

__attribute__((weak)) void indicate_configuration_end(void)
{
    bsp_board_led_off(DEFAULT_NODE_CONF_LED);
}
#endif /* ! DEBUG */

static void publish_set(const access_model_id_t* target_model_id,
    const nrf_mesh_address_t* publish_address,
    const access_publish_period_t* publish_period)
{
    // Publication state to set for the remote node.
    config_publication_state_t  pub_state;
    memset(&pub_state, 0, sizeof(config_publication_state_t));
    pub_state.element_address   = s_node_config_curr_state.elm_address; // Element of the state to set.
    pub_state.publish_address   = *publish_address;                     // Address to publish on.
    pub_state.appkey_index      = PROV_APPKEY_IDX;                      // Index of the appkey to use for publication.
    pub_state.publish_ttl       = ACCESS_DEFAULT_TTL;                   // Default Time-To-Live value for published messages.
    pub_state.publish_period    = *publish_period;                      // Period between retransmissions of a published message.
    pub_state.retransmit_count  = 1;                                    // Number of retransmissions for a published message.
    pub_state.model_id          = *target_model_id;                     // Model which publish address shall be set.

    ret_code_t                  err_code;

    // Set publication state with the given parameters.
    err_code    = config_client_model_publication_set(&pub_state);
    APP_ERROR_CHECK(err_code);
}

static void node_config_idle_to_composition_get(void)
{
    ret_code_t err_code;

    // Request composition data from remote device.
    err_code    = config_client_composition_data_get(
                    FIRST_COMP_PAGE_IDX
                  );
    APP_ERROR_CHECK(err_code);
}

static void node_config_composition_get_to_network_transmit(void)
{
    ret_code_t err_code;

    // Define network transmission parameters.
    err_code    = config_client_network_transmit_set(
                    NETWORK_TRANSMIT_COUNT,         // Number of times a packet shall be re-transmitted.
                    NETWORK_TRANSMIT_INTERVAL_STEPS // Interval between two packet retransmissions.
                  );
    APP_ERROR_CHECK(err_code);
}

static void node_config_network_transmit_to_appkey_add(void)
{
    ret_code_t err_code;

    // Add appkey to remote device with the given indexes.
    err_code    = config_client_appkey_add(PROV_NETKEY_IDX,
                                           PROV_APPKEY_IDX,
                                           g_network_ctx.appkey);
    APP_ERROR_CHECK(err_code);
}

static void node_config_appkey_add_to_appkey_bind_health(void)
{
    ret_code_t err_code;

    // Bind appkey to remote instance of Health Server model.
    err_code    = config_client_model_app_bind(
                    s_node_config_curr_state.elm_address,
                    PROV_APPKEY_IDX, HEALTH_SERVER_ACCESS_MODEL_ID
                  );
    APP_ERROR_CHECK(err_code);
}

static void node_config_appkey_bind_health_to_publish_health(void)
{
    // Address of the element hosting the Health Client instance.
    nrf_mesh_address_t          health_client_address;
    memset(&health_client_address, 0, sizeof(nrf_mesh_address_t));
    health_client_address.type  = NRF_MESH_ADDRESS_TYPE_UNICAST;    // Element address: Unicast.
    health_client_address.value = PROV_ELM_ADDRESS;                 // All model instances are on element 0x0001.

    // Publication period of Health model messages.
    access_publish_period_t     publish_period;
    memset(&publish_period, 0, sizeof(access_publish_period_t));
    publish_period.step_num     = 1;                                // Messages only sent once.
    publish_period.step_res     = ACCESS_PUBLISH_RESOLUTION_10S;    // Messages sent every 10s.

    // Set publication parameters.
    publish_set(&HEALTH_SERVER_ACCESS_MODEL_ID, &health_client_address,
                &publish_period);
}

static void node_config_publish_health_to_appkey_bind_luos_rtb(void)
{
    // Luos RTB model complete access ID.
    access_model_id_t   luos_rtb_model_id   = LUOS_RTB_MODEL_ACCESS_ID;

    ret_code_t          err_code;

    // Bind appkey to remote instance of Luos RTB model.
    err_code    = config_client_model_app_bind(
                    s_node_config_curr_state.elm_address,
                    PROV_APPKEY_IDX, luos_rtb_model_id
                  );
    APP_ERROR_CHECK(err_code);
}

static void node_config_appkey_bind_luos_rtb_to_publish_luos_rtb(void)
{
    // Luos RTB model complete access ID.
    access_model_id_t   luos_rtb_model_id   = LUOS_RTB_MODEL_ACCESS_ID;

    // Publication period of Luos RTB model messages.
    access_publish_period_t publish_period;
    memset(&publish_period, 0, sizeof(access_publish_period_t));
    publish_period.step_res     = ACCESS_PUBLISH_RESOLUTION_100MS;  // Messages sent every 100ms.

    // Set publication parameters.
    publish_set(&luos_rtb_model_id, &LUOS_GROUP_NRF_ADDRESS,
                &publish_period);
}

static void node_config_publish_luos_rtb_to_subscribe_luos_rtb(void)
{
    // Luos RTB model complete access ID.
    access_model_id_t   luos_rtb_model_id   = LUOS_RTB_MODEL_ACCESS_ID;

    ret_code_t          err_code;

    // Add subscription address to given model.
    err_code    = config_client_model_subscription_add(
                    s_node_config_curr_state.elm_address,
                    LUOS_GROUP_NRF_ADDRESS, luos_rtb_model_id
                  );
    APP_ERROR_CHECK(err_code);
}

static void node_config_subscribe_luos_rtb_to_appkey_bind_luos_msg(void)
{
    // Luos MSG model complete access ID.
    access_model_id_t   luos_msg_model_id   = LUOS_MSG_MODEL_ACCESS_ID;

    ret_code_t          err_code;

    // Bind appkey to remote instance of Luos MSG model.
    err_code    = config_client_model_app_bind(
                    s_node_config_curr_state.elm_address,
                    PROV_APPKEY_IDX, luos_msg_model_id
                  );
    APP_ERROR_CHECK(err_code);
}

static void node_config_appkey_bind_luos_msg_to_publish_luos_msg(void)
{
    // Luos MSG model complete access ID.
    access_model_id_t   luos_msg_model_id   = LUOS_MSG_MODEL_ACCESS_ID;

    // Publication period of Luos MSG model messages.
    access_publish_period_t publish_period;
    memset(&publish_period, 0, sizeof(access_publish_period_t));
    publish_period.step_res     = ACCESS_PUBLISH_RESOLUTION_100MS;  // Messages sent every 100ms.

    // Set publication parameters.
    publish_set(&luos_msg_model_id, &LUOS_GROUP_NRF_ADDRESS,
                &publish_period);
}

static void node_config_publish_luos_msg_to_subscribe_luos_msg(void)
{
    // Luos MSG model complete access ID.
    access_model_id_t   luos_msg_model_id   = LUOS_MSG_MODEL_ACCESS_ID;

    ret_code_t          err_code;

    // Add subscription address to given model.
    err_code    = config_client_model_subscription_add(
                    s_node_config_curr_state.elm_address,
                    LUOS_GROUP_NRF_ADDRESS, luos_msg_model_id
                  );
    APP_ERROR_CHECK(err_code);
}

static void node_config_success(void)
{
    #ifdef DEBUG
    NRF_LOG_INFO("Configuration process successfully completed!");
    #else /* ! DEBUG */
    indicate_configuration_end();
    #endif /* DEBUG */

    // Retrieve persistent provisioning configuration header.
    prov_conf_header_entry_live_t   tmp_conf_header;
    mesh_config_entry_get(PROV_CONF_HEADER_ENTRY_ID, &tmp_conf_header);

    // Increase number of configured nodes.
    tmp_conf_header.nb_conf_nodes++;

    // Rewrite persistent configuration with this new information.
    mesh_config_entry_set(PROV_CONF_HEADER_ENTRY_ID, &tmp_conf_header);

    // Reset configuration step.
    s_node_config_curr_state.config_step    = NODE_CONFIG_STEP_IDLE;

    // Try scanning again.
    bool can_scan   = prov_scan_start();
    if ((!can_scan) && (s_mesh_provisioner_instance != NULL))
    {
        /* Scan impossible: maximum number of provisioned nodes has been
        ** reached.
        */

        // Prepare message signaling impossibility to scan.
        msg_t   cannot_scan;
        memset(&cannot_scan, 0, sizeof(msg_t));
        cannot_scan.header.target_mode  = BROADCAST;        // Broadcast: not an answer.
        cannot_scan.header.target       = BROADCAST_VAL;
        cannot_scan.header.cmd          = IO_STATE;
        cannot_scan.header.size         = sizeof(uint8_t);
        cannot_scan.data[0]             = 0x00;             // false

        // Send message.
        Luos_SendMsg(s_mesh_provisioner_instance, &cannot_scan);
    }
}
