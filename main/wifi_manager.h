#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************
                    WIFI INIT
************************************************************/

void wifi_manager_init(void);

/************************************************************
                WIFI STATUS
************************************************************/

bool wifi_manager_is_connected(void);

/************************************************************
            PROVISIONING STATUS
************************************************************/

bool wifi_manager_is_provisioning(void);

/************************************************************
        RESET WIFI PROVISIONING
************************************************************/

void wifi_manager_reset_provisioning(void);

#ifdef __cplusplus
}
#endif

#endif