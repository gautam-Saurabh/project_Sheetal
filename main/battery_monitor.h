#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

void battery_monitor_init(void);

float battery_monitor_get_percent(void);

#ifdef __cplusplus
}
#endif

#endif // BATTERY_MONITOR_H