#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void oled_display_init(void);

void oled_display_update(
    float current_temp,
    int set_temp,
    float battery_percent,
    bool wifi_connected
);

#ifdef __cplusplus
}
#endif

#endif // OLED_DISPLAY_H