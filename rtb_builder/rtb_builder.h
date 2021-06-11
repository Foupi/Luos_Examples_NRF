#ifndef RTB_BUILDER_H
#define RTB_BUILDER_H

/*      INCLUDES                                                    */

// LUOS
#include "luos_list.h"  // LUOS_PROTOCOL_NB, VOID_MOD

// Type of the RTB container.
#define RTB_TYPE        VOID_MOD

// Default alias of the RTB container.
#define RTB_ALIAS       "rtb_builder"

// Creates the container.
void RTBBuilder_Init(void);

// If a detection was asked, prepare the routing table.
void RTBBuilder_Loop(void);

#endif /* RTB_BUILDER_H */
