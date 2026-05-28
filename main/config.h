#ifndef CONFIG_H
#define CONFIG_H

// ================================================================
// CONFIG.H
// TEC Cooling Jacket Firmware
// ESP-IDF v5.3
// ================================================================

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

// 1 = Test OLED/WiFi with fake data (No sensors needed), 0 = Real Hardware Mode
#define SIMULATION_MODE            0

/************************************************************
                        PUMP CONFIGURATION
************************************************************/

// 1 = Pump stays ON, 0 = Pump stays OFF (Battery Saver Mode)
#define PUMP_ENABLED               0

/************************************************************
                    TEMPERATURE SENSOR PINS
************************************************************/

#define TEMP1_PIN                  GPIO_NUM_4
#define TEMP2_PIN                  GPIO_NUM_16
#define TEMP3_PIN                  GPIO_NUM_19

/************************************************************
                        OUTPUT PINS
************************************************************/

#define FAN_PIN                    GPIO_NUM_32
#define TEC_PIN                    GPIO_NUM_33
#define PUMP_PIN                   GPIO_NUM_25

/************************************************************
                        BUTTON PINS
************************************************************/

#define SW1_PIN                    GPIO_NUM_5
#define SW4_PIN                    GPIO_NUM_17

#define BOOT_BUTTON_PIN            GPIO_NUM_0

/************************************************************
                        WIFI
************************************************************/

#define WIFI_LED_PIN               GPIO_NUM_2

#define WIFI_PROV_TIMEOUT_SEC      60

#define WIFI_LED_BLINK_MS          500UL

// === NEW: Wi-Fi Provisioning Settings ===
#define PROV_AP_SSID               "COOLING_JACKET"
#define PROV_AP_PASS               "00000000"
#define PROV_POP                   "abcd1234"
#define MAX_WIFI_RETRY             5

/************************************************************
                        OLED DISPLAY
************************************************************/

#define OLED_SDA                   GPIO_NUM_21
#define OLED_SCL                   GPIO_NUM_22

#define SCREEN_WIDTH               128
#define SCREEN_HEIGHT              64

#define OLED_I2C_ADDRESS           0x3C

/************************************************************
                        BATTERY ADC
************************************************************/

// GPIO36 = ADC1_CHANNEL_0

#define BATTERY_ADC_CHANNEL        ADC_CHANNEL_0

/************************************************************
                        THINGSPEAK
************************************************************/

#define THINGSPEAK_CHANNEL_ID      3254497UL

#define THINGSPEAK_WRITE_API_KEY   "XPINKA2S866KH06D"

#define UPLOAD_INTERVAL_MS         15000UL

/************************************************************
                    TEMPERATURE CONTROL
************************************************************/

#define DEFAULT_SET_TEMP           25

#define MIN_SET_TEMP               15

#define MAX_SET_TEMP               30

#define HYSTERESIS                 2.0f

#define TEMP_UPDATE_INTERVAL_MS    1000UL

/************************************************************
                        BUTTONS
************************************************************/

#define BUTTON_DEBOUNCE_MS         200UL

#define BUTTON_HOLD_SPEED_MS       120UL

/************************************************************
                    BATTERY MONITOR & CALIBRATION
************************************************************/

// Voltage Divider: R1 = 1M, R2 = 470K
#define BAT_DIVIDER_RATIO          (1470.0f / 470.0f)

// Tweak this multiplier if the ESP32 voltage doesn't match your multimeter
// This completely fixes op-amp tolerances and resistor variations!
#define BATTERY_CALIBRATION_RATIO   3.23f 

// True measured voltages for 100% and 0% (Adjust based on your BMS cutoff)
#define BATTERY_MAX_VOLTAGE         8.2f
#define BATTERY_MIN_VOLTAGE         6.00f

/************************************************************
                            ADC
************************************************************/

#define ADC_ATTEN                  ADC_ATTEN_DB_12

#define ADC_WIDTH                  ADC_BITWIDTH_12

/************************************************************
                            TASKS
************************************************************/

#define TASK_STACK_SIZE            6144

#define TASK_PRIORITY              5

#endif // CONFIG_H