#include "node_config.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdint.h>                 // uint16_t

// NRF
#include "nrf_log.h"                // NRF_LOG_INFO
#include "sdk_errors.h"             // ret_code_t

// NRF APPS
#include "app_error.h"              // APP_ERROR_CHECK

// MESH SDK
#include "device_state_manager.h"   // dsm_handle_t

// MESH MODELS
#include "config_client.h"          // config_client_*
#include "config_opcodes.h"         // CONFIG_OPCODE_*

// CUSTOM
#include "network_ctx.h"            // PROV_*_IDX, g_network_ctx

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

    // FIXME Binding remote health server instance to appkey.
    NODE_CONFIG_STEP_APPKEY_BIND_HEALTH,

    /* FIXME    Setting publish address of remote health server instance
    **          to address of the element hosting the health client
    **          instance.
    */
    NODE_CONFIG_STEP_PUBLISH_HEALTH,

    // FIXME Add steps for custom models.

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

/*      STATIC FUNCTIONS                                            */

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

/*      INITIALIZATIONS                                             */

// Transition function for each configuration step.
static const node_config_step_transition_t  STEP_TRANSITIONS[]  =
{
    [NODE_CONFIG_STEP_IDLE]                 = node_config_idle_to_composition_get,
    [NODE_CONFIG_STEP_COMPOSITION_GET]      = node_config_composition_get_to_network_transmit,
    [NODE_CONFIG_STEP_NETWORK_TRANSMIT]     = node_config_network_transmit_to_appkey_add,
    [NODE_CONFIG_STEP_APPKEY_ADD]           = /* FIXME */ NULL,
    [NODE_CONFIG_STEP_APPKEY_BIND_HEALTH]   = /* FIXME */ NULL,
    [NODE_CONFIG_STEP_PUBLISH_HEALTH]       = /* FIXME */ NULL,
};


void node_config_start(uint16_t device_first_addr,
                       dsm_handle_t devkey_handle,
                       dsm_handle_t address_handle)
{
    if (s_node_config_curr_state.config_step != NODE_CONFIG_STEP_IDLE)
    {
        NRF_LOG_INFO("Configuration start requested for node 0x%x while still configuring element 0x%x!",
                     device_first_addr,
                     s_node_config_curr_state.elm_address);
    }

    NRF_LOG_INFO("Start configuration for node 0x%x! (devkey handle: 0x%x; address handle: 0x%x)",
                 device_first_addr, devkey_handle, address_handle);

    ret_code_t err_code;

    err_code = config_client_server_bind(devkey_handle);
    APP_ERROR_CHECK(err_code);

    err_code = config_client_server_set(devkey_handle, address_handle);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_INFO("Config Client bound with and set on remote Config Server!");

    s_node_config_curr_state.elm_address    = device_first_addr;

    node_config_idle_to_composition_get();
}

void config_client_msg_handler(const config_client_event_t* event)
{
    if (s_node_config_curr_state.config_step == NODE_CONFIG_STEP_IDLE)
    {
        NRF_LOG_INFO("Config client message received whild no configuration is occuring!");
        return;
    }

    config_opcode_t expected_opcode = EXPECTED_OPCODES[s_node_config_curr_state.config_step];
    config_opcode_t                 received_opcode = event->opcode;

    if (received_opcode != expected_opcode)
    {
        NRF_LOG_INFO("Wrong opcode received: expected 0x%x, got 0x%x!",
                     expected_opcode, received_opcode);

        return;
    }

    node_config_step_transition_t   next_transition = STEP_TRANSITIONS[s_node_config_curr_state.config_step];

    if (next_transition == NULL)
    {
        NRF_LOG_INFO("Opcode 0x%x received but not managed yet!");

        return;
    }

    next_transition();
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
    NRF_LOG_INFO("Composition data received!");

    s_node_config_curr_state.config_step    = NODE_CONFIG_STEP_NETWORK_TRANSMIT;

    ret_code_t err_code;

    err_code = config_client_network_transmit_set(NETWORK_TRANSMIT_COUNT,
        NETWORK_TRANSMIT_INTERVAL_STEPS);
    APP_ERROR_CHECK(err_code);
}

static void node_config_network_transmit_to_appkey_add(void)
{
    NRF_LOG_INFO("Network transmit parameters set!");

    s_node_config_curr_state.config_step    = NODE_CONFIG_STEP_APPKEY_ADD;

    ret_code_t err_code;

    err_code = config_client_appkey_add(PROV_NETKEY_IDX,
                                        PROV_APPKEY_IDX,
                                        g_network_ctx.appkey);
    APP_ERROR_CHECK(err_code);
}
