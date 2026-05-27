#ifndef TEMPERATURE_MONITORING_H
#define TEMPERATURE_MONITORING_H

#include "onewire_bus.h"
#include "ds18b20.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************
                    GLOBAL TEMPERATURES
************************************************************/

// Main control sensor

extern float currentTemp;

// Secondary sensor

extern float sensor2Temp;

// Third sensor

extern float sensor3Temp;

/************************************************************
                    PUBLIC API
************************************************************/

void temperature_monitoring_init(void);

void temperature_monitoring_update(void);

/************************************************************
                TEMPERATURE STATUS
************************************************************/

bool temperature_monitoring_is_valid(void);

#ifdef __cplusplus
}
#endif

#endif // TEMPERATURE_MONITORING_H