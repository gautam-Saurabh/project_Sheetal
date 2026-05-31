#ifndef TEC_FAN_CONTROLLER_H
#define TEC_FAN_CONTROLLER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************
                    PUBLIC API
************************************************************/

void tec_fan_controller_init(void);

void tec_fan_controller_update(float current_temp);

// --- NEW: CLOUD OVERRIDE FUNCTION ---
// state: 1 (Force ON), 0 (Force OFF), -1 (Return to Auto)
void tec_fan_controller_set_override(int state); 

/************************************************************
                COOLING STATUS
************************************************************/

bool tec_fan_controller_is_cooling(void);

#ifdef __cplusplus
}
#endif

#endif // TEC_FAN_CONTROLLER_H