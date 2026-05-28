#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void wifi_manager_init(void);
bool wifi_manager_is_connected(void);
bool wifi_manager_is_provisioning(void);
void wifi_manager_reset_provisioning(void);
void wifi_manager_factory_reset(void);

void wifi_manager_lock_screen(bool lock);
bool wifi_manager_is_factory_resetting(void);

#ifdef __cplusplus
}
#endif

#endif