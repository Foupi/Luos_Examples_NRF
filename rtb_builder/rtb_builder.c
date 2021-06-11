#include "rtb_builder.h"

/*      INCLUDES                                                    */

// C STANDARD
#include <stdbool.h>    // bool

// LUOS
#include "luos.h"       // Luos_CreateContainer, container_t

/*      STATIC VARIABLES & CONSTANTS                                */

#ifndef REV
#define REV {0,0,1}
#endif

// The container instance.
static  container_t*    s_rtb_builder       = NULL;

// Describes if a detection was asked.
static  bool            s_detection_asked   = false;

void RTBBuilder_Init(void)
{
    revision_t revision = { .unmap = REV };
    s_rtb_builder = Luos_CreateContainer(NULL, RTB_TYPE, RTB_ALIAS,
                                         revision);
}

void RTBBuilder_Loop(void)
{
    // if detection asked, run detection.
}
