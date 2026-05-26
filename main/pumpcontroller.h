#ifndef PUMP_CONTROLLER_H
#define PUMP_CONTROLLER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************
                    PUBLIC API
************************************************************/

void pump_controller_init(void);

void pump_controller_update(void);

/************************************************************
                    PUMP STATUS
************************************************************/

bool pump_controller_is_on(void);

#ifdef __cplusplus
}
#endif

#endif // PUMP_CONTROLLER_H
