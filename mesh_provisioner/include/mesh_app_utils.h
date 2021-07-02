#ifndef MESH_APP_UTILS_H
#define MESH_APP_UTILS_H

/* DISCLAIMER:  This file was created solely because Nordic devs thought
**              that using example code INSIDE THE SDK CODE was a neat
**              practice. Props to them.
*/

/*      INCLUDES                                                    */

// NRF
#include "sdk_errors.h" // ret_code_t

// NRF APPS
#include "app_error.h"  // APP_ERROR_CHECK

/*      DEFINES                                                     */

/* Used in the SDK code despite belonging in example code. All this for
** a stupid wrapper.
*/
#define ERROR_CHECK(fun_call)           \
    do                                  \
    {                                   \
        ret_code_t err_code = fun_call; \
        APP_ERROR_CHECK(err_code);      \
    } while (0)

#endif /* ! MESH_APP_UTILS_H */
