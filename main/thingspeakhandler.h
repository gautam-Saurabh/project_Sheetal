#ifndef THINGSPEAK_HANDLER_H
#define THINGSPEAK_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************
                    PUBLIC API
************************************************************/

void thingspeak_handler_init(void);

void thingspeak_handler_update(void);

#ifdef __cplusplus
}
#endif

#endif // THINGSPEAK_HANDLER_H