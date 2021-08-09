#ifndef RTB_BUILDER_H
#define RTB_BUILDER_H

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>    // bool

// LUOS
#include "luos.h"       // container_t
#include "luos_list.h"  // LUOS_PROTOCOL_NB

/*      DEFINES                                                     */

// Index of RTB Builder type among Luos types.
#ifndef RTB_MOD
#define RTB_MOD         LUOS_LAST_TYPE
#endif  /* ! RTB_MOD */

// Start index of RTB messages (default: end of Luos cmds)
#ifndef RTB_MSG_BEGIN
#define RTB_MSG_BEGIN   LUOS_PROTOCOL_NB
#endif /* ! RTB_MSG_BEGIN */

// Type and default alias for the RTB container.
#define RTB_TYPE        RTB_MOD
#define RTB_ALIAS       "rtb_builder"

/*      TYPEDEFS                                                    */

typedef enum
{
    // Received commands:
    // Build the routing table.
    RTB_BUILD   = RTB_MSG_BEGIN,

    // Log the routing table.
    RTB_PRINT,

    // Sent messages:
    // Routing table built.
    RTB_COMPLETE,

    // Start index for next messages.
    RTB_MSG_END,

} rtb_builder_cmd_t;

// Creates the container.
void RTBBuilder_Init(void);

// If a detection was asked, prepare the routing table.
void RTBBuilder_Loop(void);

// Describes if a detection was asked.
extern bool g_detection_asked;

#endif /* RTB_BUILDER_H */
