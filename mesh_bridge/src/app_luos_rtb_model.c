#include "app_luos_rtb_model.h"

/*      INCLUDES                                                    */

// NRF
#include "nrf_log.h"                // NRF_LOG_INFO

// C STANDARD
#include <stdint.h>                 // uint16_t
#include <string.h>                 // memset

// LUOS
#include "luos_list.h"              // FIXME VOID_MOD
#include "routing_table.h"          // routing_table_t

// CUSTOM
#include "luos_rtb_model.h"         // luos_rtb_model_*
#include "luos_rtb_model_common.h"  // LUOS_RTB_MODEL_MAX_RTB_ENTRY

/*      STATIC VARIABLES & CONSTANTS                                */

// Luos RTB model instance.
static luos_rtb_model_t s_luos_rtb_model;

/*      STATIC FUNCTIONS                                            */

// FIXME Retrieves local RTB and stores it in the given arguments.
static bool get_rtb_entries(routing_table_t* rtb_entries,
                            uint16_t* nb_entries);

/*      CALLBACKS                                                   */

// FIXME Logs message.
static void rtb_model_get_cb(void);

// FIXME Logs message.
static void rtb_model_status_cb(uint16_t src_addr,
                                const routing_table_t* entry,
                                uint16_t entry_idx);

void app_luos_rtb_model_init(void)
{
    luos_rtb_model_init_params_t    init_params;
    memset(&init_params, 0, sizeof(luos_rtb_model_init_params_t));
    init_params.get_cb                      = rtb_model_get_cb;
    init_params.local_rtb_entries_get_cb    = get_rtb_entries;
    init_params.status_cb                   = rtb_model_status_cb;

    luos_rtb_model_init(&s_luos_rtb_model, &init_params);
}

void app_luos_rtb_model_address_set(uint16_t device_address)
{
    luos_rtb_model_set_address(&s_luos_rtb_model, device_address);
}

void app_luos_rtb_model_get(void)
{
    luos_rtb_model_get(&s_luos_rtb_model);
}

static void rtb_model_get_cb(void)
{
    NRF_LOG_INFO("Luos RTB GET request received!");
}

static bool get_rtb_entries(routing_table_t* rtb_entries,
                            uint16_t* nb_entries)
{
    uint16_t            local_nb_entries    = RoutingTB_GetLastEntry();
    if (local_nb_entries == 0)
    {
        // Local routing table is empty: detection was not performed.
        return false;
    }

    routing_table_t*    local_rtb           = RoutingTB_Get();
    uint16_t            returned_nb_entries = 0;
    for (uint16_t entry_idx = 0; entry_idx < local_nb_entries;
         entry_idx++)
    {
        if (local_rtb[entry_idx].mode != CONTAINER)
        {
            // Only export containers.
            continue;
        }

        memcpy(rtb_entries + returned_nb_entries, local_rtb + entry_idx,
               sizeof(routing_table_t));
        returned_nb_entries++;

        if (returned_nb_entries >= LUOS_RTB_MODEL_MAX_RTB_ENTRY)
        {
            // Do not export any more entries.
            break;
        }
    }

    *nb_entries                             = returned_nb_entries;
    return true;
}

static void rtb_model_status_cb(uint16_t src_addr,
                                const routing_table_t* entry,
                                uint16_t entry_idx)
{
    NRF_LOG_INFO("Luos RTB STATUS message received from node 0x%x: entry %u has ID 0x%x, type %s, alias %s!",
                 src_addr, entry_idx, entry->id,
                 RoutingTB_StringFromType(entry->type), entry->alias);
}
