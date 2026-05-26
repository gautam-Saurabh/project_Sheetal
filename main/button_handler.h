#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************
                    GLOBAL SET TEMP
************************************************************/

extern int setTemp;

/************************************************************
                    PUBLIC API
************************************************************/

void button_handler_init(void);

void button_handler_update(void);

#ifdef __cplusplus
}
#endif

#endif // BUTTON_HANDLER_H