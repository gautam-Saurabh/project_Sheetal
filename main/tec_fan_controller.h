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

/************************************************************
                COOLING STATUS
************************************************************/

bool tec_fan_controller_is_cooling(void);

#ifdef __cplusplus
}
#endif

#endif // TEC_FAN_CONTROLLER_H