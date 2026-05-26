#ifndef THINGSPEAK_HANDLER_H
#define THINGSPEAK_HANDLER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************
                    PUBLIC API
************************************************************/

void thingspeak_handler_init(void);

void thingspeak_handler_update(void);

/************************************************************
                    WIFI STATUS
************************************************************/

bool thingspeak_handler_is_wifi_connected(void);

#ifdef __cplusplus
}
#endif

#endif // THINGSPEAK_HANDLER_H