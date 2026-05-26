#ifndef CONFIG_H
#define CONFIG_H

// ================================================================
// CONFIG.H
// Central configuration file for TEC Cooling Jacket Firmware
// ESP-IDF v5.3
// ================================================================

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

// ======================= PIN DEFINITIONS =======================

#define TEMP1_PIN       GPIO_NUM_4
#define TEMP2_PIN       GPIO_NUM_16
#define TEMP3_PIN       GPIO_NUM_19

#define FAN_PIN         GPIO_NUM_32
#define TEC_PIN         GPIO_NUM_33
#define PUMP_PIN        GPIO_NUM_25

#define SW1_PIN         GPIO_NUM_17
#define SW4_PIN         GPIO_NUM_5

// GPIO36 = ADC1_CHANNEL_0
#define BATTERY_ADC_CHANNEL ADC_CHANNEL_0

#define OLED_SDA        GPIO_NUM_21
#define OLED_SCL        GPIO_NUM_22

// ======================= OLED =======================

#define SCREEN_WIDTH        128
#define SCREEN_HEIGHT       64
#define OLED_I2C_ADDRESS    0x3C

// ======================= WIFI =======================

#define WIFI_SSID       "Project_213"
#define WIFI_PASSWORD   "da@2020@"

// ======================= THINGSPEAK =======================

#define THINGSPEAK_CHANNEL_ID      3254497UL
#define THINGSPEAK_WRITE_API_KEY   "XPINKA2S866KH06D"

// ======================= TEMPERATURE CONTROL =======================

#define DEFAULT_SET_TEMP    20
#define HYSTERESIS          2.0f

// ======================= TIMING =======================

#define UPLOAD_INTERVAL_MS      15000UL
#define RECONNECT_INTERVAL_MS   60000UL
#define BUTTON_DEBOUNCE_MS      200UL

// ======================= BATTERY MONITOR =======================

#define BAT_DIVIDER_RATIO       (1470.0f / 470.0f)

#define BAT_MIN_VOLTAGE         6.0f
#define BAT_MAX_VOLTAGE         8.4f

// ======================= ADC =======================

#define ADC_ATTEN           ADC_ATTEN_DB_12
#define ADC_WIDTH           ADC_BITWIDTH_12

// ======================= MISC =======================

#define TASK_STACK_SIZE     4096
#define TASK_PRIORITY       5

#endif // CONFIG_H