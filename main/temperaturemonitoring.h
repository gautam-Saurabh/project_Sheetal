#ifndef TEMPERATURE_MONITORING_H
#define TEMPERATURE_MONITORING_H

#include "onewire_bus.h"
#include "ds18b20.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************
                    GLOBAL TEMPERATURES
************************************************************/

extern float currentTemp;

extern float sensor2Temp;

extern float sensor3Temp;

/************************************************************
                    PUBLIC API
************************************************************/

void temperature_monitoring_init(void);

void temperature_monitoring_update(void);

#ifdef __cplusplus
}
#endif

#endif // TEMPERATURE_MONITORING_H