#include "app_luos_rtb_model.h"

/*      INCLUDES                                                    */

// NRF
#include "nrf_log.h"        // NRF_LOG_INFO

// C STANDARD
#include <stdint.h>         // uint16_t
#include <string.h>         // memset

// LUOS
#include "luos_list.h"      // FIXME VOID_MOD
#include "routing_table.h"  // routing_table_t

// CUSTOM
#include "luos_rtb_model.h" // luos_rtb_model_*

/*      STATIC VARIABLES & CONSTANTS                                */

// Luos RTB model instance.
static luos_rtb_model_t s_luos_rtb_model;

/*      CALLBACKS                                                   */

// FIXME Logs message.
static void rtb_model_get_cb(void);

// FIXME Fills the table with one hardcoded RTB entry.
static bool get_rtb_entries(routing_table_t* rtb_entries,
                            uint16_t* nb_entries);

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

static const char   STUB_ENTRY1_ALIAS[] = "coucou";
static const char   STUB_ENTRY2_ALIAS[] = "Pierre";

static bool get_rtb_entries(routing_table_t* rtb_entries,
                            uint16_t* nb_entries)
{
    // FIXME Stub function!

    routing_table_t entry_1;
    memset(&entry_1, 0, sizeof(routing_table_t));
    entry_1.mode  = CONTAINER;
    entry_1.id    = 1;
    entry_1.type  = VOID_MOD;
    memcpy(entry_1.alias, STUB_ENTRY1_ALIAS, sizeof(STUB_ENTRY1_ALIAS));
    memcpy(rtb_entries, &entry_1, sizeof(routing_table_t));

    routing_table_t entry_2;
    memset(&entry_2, 0, sizeof(routing_table_t));
    entry_2.mode  = CONTAINER;
    entry_2.id    = 8;
    entry_2.type  = VOID_MOD;
    memcpy(entry_2.alias, STUB_ENTRY2_ALIAS, sizeof(STUB_ENTRY2_ALIAS));
    memcpy(rtb_entries + 1, &entry_2, sizeof(routing_table_t));

    *nb_entries = 2;
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
