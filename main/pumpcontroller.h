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

// --- NEW: CLOUD OVERRIDE FUNCTION ---
// state: 1 (Force ON), 0 (Force OFF), -1 (Return to Auto)
void pump_controller_set_override(int state); 

/************************************************************
                    PUMP STATUS
************************************************************/

bool pump_controller_is_on(void);

#ifdef __cplusplus
}
#endif

#endif // PUMP_CONTROLLER_H