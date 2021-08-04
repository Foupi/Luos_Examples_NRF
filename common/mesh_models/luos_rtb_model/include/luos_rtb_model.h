#ifndef LUOS_RTB_MODEL_H
#define LUOS_RTB_MODEL_H

/*      INCLUDES                                                    */

// MESH SDK
#include "access.h"         // access_model_handle_t

// LUOS
#include "routing_table.h"  // routing_table_t

/*      DEFINES                                                     */

// Default element address.
#define LUOS_RTB_MODEL_DEFAULT_ELM_ADDR 0xFFFF

/*      TYPEDEFS                                                    */

// Forward declaration
typedef struct luos_rtb_model_s luos_rtb_model_t;

// Callback called on GET request.
typedef void (*luos_rtb_model_get_cb_t)(void);

/* Callback called on GET request to get local RTB entries.
** Returns true and updates parameters with local RTB information if the
** detection process is complete, returns false otherwise.
*/
typedef bool (*luos_rtb_model_get_rtb_entries_cb_t)(routing_table_t* rtb_entries,
                uint16_t* nb_entries);

// Callback called on STATUS message.
typedef void (*luos_rtb_model_status_cb_t)(uint16_t src_addr,
    const routing_table_t* entry, uint16_t entry_idx);

// Parameters to initialize a Luos RTB model instance.
typedef struct
{
    // User callback called on GET request.
    luos_rtb_model_get_cb_t             get_cb;

    // Callback to retrieve local RTB entries on GET request.
    luos_rtb_model_get_rtb_entries_cb_t local_rtb_entries_get_cb;

    // User callback called on STATUS message.
    luos_rtb_model_status_cb_t          status_cb;

} luos_rtb_model_init_params_t;

// A Luos RTB model instance.
struct luos_rtb_model_s
{
    // Handle for the model instance.
    access_model_handle_t               handle;

    // Unicast address of the element hosting the model instance.
    uint16_t                            element_address;

    // User callback called on GET request.
    luos_rtb_model_get_cb_t             get_cb;

    // Callback to retrieve local RTB entries on GET request.
    luos_rtb_model_get_rtb_entries_cb_t local_rtb_entries_get_cb;

    // User callback called on STATUS message.
    luos_rtb_model_status_cb_t          status_cb;
};

// Payload type for a Luos RTB model GET message.
typedef struct
{
    // Current transaction index.
    uint16_t                            transaction_id;

} luos_rtb_model_get_t;

// Payload type for a Luos RTB model STATUS message.
typedef struct
{
    // Current transaction index.
    uint16_t                            transaction_id;

    // Current RTB entry.
    routing_table_t                     entry;

    // Current RTB entry index.
    uint16_t                            entry_idx;

} luos_rtb_model_status_t;

// Initialize the given instance with the given parameters.
void luos_rtb_model_init(luos_rtb_model_t* instance,
                         const luos_rtb_model_init_params_t* params);

/* Sets the given instance's element address according to the given
** device address.
*/
void luos_rtb_model_set_address(luos_rtb_model_t* instance,
                                uint16_t device_address);

// Sends a Luos RTB GET request through the given model instance.
void luos_rtb_model_get(luos_rtb_model_t* instance);

/* Publishes the given RTB entries on the given instance's publish
** address.
*/
void luos_rtb_model_publish_entries(luos_rtb_model_t* instance,
                                    const routing_table_t* entries,
                                    uint16_t nb_entries);

#endif /* ! LUOS_RTB_MODEL_H */
