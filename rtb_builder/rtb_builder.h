#ifndef RTB_BUILDER_H
#define RTB_BUILDER_H

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>    // bool

// LUOS
#include "luos.h"       // container_t
#include "luos_list.h"  // LUOS_PROTOCOL_NB, VOID_MOD

/*      DEFINES                                                     */

// Start index of RTB messages (default: end of Luos cmds)
#ifndef RTB_MSG_BEGIN
#define RTB_MSG_BEGIN   LUOS_PROTOCOL_NB
#endif

typedef enum
{
    // Build the routing table.
    RTB_BUILD   = RTB_MSG_BEGIN,

    // Log the routing table.
    RTB_PRINT,

    // Start index for next messages.
    RTB_MSG_END,

} rtb_builder_cmd_t;

// Type of the RTB container.
#define RTB_TYPE        VOID_MOD

// Default alias of the RTB container.
#define RTB_ALIAS       "rtb_builder"

// Creates the container.
void RTBBuilder_Init(void);

// If a detection was asked, prepare the routing table.
void RTBBuilder_Loop(void);

// Describes if a detection was asked.
extern bool g_detection_asked;

#endif /* RTB_BUILDER_H */
