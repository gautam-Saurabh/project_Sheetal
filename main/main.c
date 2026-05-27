#include "main.h"
#include "config.h"
#include "oled_display.h"

#include "wifi_manager.h"

#include "temperaturemonitoring.h"
#include "tec_fan_controller.h"
#include "pumpcontroller.h"
#include "button_handler.h"

#include "battery_monitor.h"
#include "thingspeakhandler.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "driver/gpio.h"

static const char *TAG = "MAIN";

/************************************************************
                    EXTERNAL VARIABLES
************************************************************/
// This tells the compiler to look for the real values in your sensor files
extern float currentTemp;
extern int setTemp;

/************************************************************
                    BOOT BUTTON
************************************************************/

#define BOOT_BUTTON GPIO_NUM_0

/************************************************************
                        APP MAIN
************************************************************/

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "TEC Cooling Jacket Firmware");
    ESP_LOGI(TAG, "ESP-IDF v5.3");
    ESP_LOGI(TAG, "========================================");

    /********************************************************
                    BOOT BUTTON CONFIG
    ********************************************************/

    gpio_config_t io_conf = {

        .pin_bit_mask =
            (1ULL << BOOT_BUTTON),

        .mode =
            GPIO_MODE_INPUT,

        .pull_up_en =
            GPIO_PULLUP_ENABLE,
    };

    gpio_config(&io_conf);

    /********************************************************
                            INIT
    ********************************************************/

    // 1. Initialize OLED FIRST so the I2C bus is ready!
    oled_display_init();

    /******************************************************
                    BOOT ANIMATION
    ******************************************************/

    oled_show_message(
        "Cooling Jacket",
        "Starting..."
    );

    vTaskDelay(
        pdMS_TO_TICKS(1000)
    );

    // 2. NOW initialize WiFi (which can safely use the OLED)
    wifi_manager_init();

    battery_monitor_init();

    button_handler_init();

    temperature_monitoring_init();

    tec_fan_controller_init();

    pump_controller_init();

    /******************************************************
                    INITIALIZING
    ******************************************************/

    for (int i = 0; i < 3; i++)
    {
        oled_show_message(
            "COOLING Jacket",

            i == 0 ? "Initializing." :
            i == 1 ? "Initializing.." :
                     "Initializing..."
        );

        vTaskDelay(
            pdMS_TO_TICKS(500)
        );
    }

    thingspeak_handler_init();

    ESP_LOGI(TAG, "All modules initialized");

    /********************************************************
                    LOOP VARIABLES
    ********************************************************/
    
    uint32_t boot_press_start_time = 0;
    bool is_boot_pressed = false;
    bool reset_triggered = false;

    /********************************************************
                            LOOP
    ********************************************************/

    while (1)
    {
        /****************************************************
                    RESET PROVISIONING (TIME-BASED)
        ****************************************************/

        if (gpio_get_level(BOOT_BUTTON) == 0)
        {
            if (!is_boot_pressed)
            {
                is_boot_pressed = true;
                boot_press_start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
                reset_triggered = false;
            }
            else if (!reset_triggered)
            {
                uint32_t hold_duration = (xTaskGetTickCount() * portTICK_PERIOD_MS) - boot_press_start_time;

                if (hold_duration >= 500 && hold_duration < 3000) 
                {
                    oled_show_message(
                        "Keep Holding",
                        "To Reset WiFi"
                    );
                }
                else if (hold_duration >= 3000) // Held for 3 real seconds
                {
                    reset_triggered = true; // Stop it from triggering over and over
                    
                    ESP_LOGW(
                        TAG,
                        "Reset WiFi Provisioning"
                    );

                    oled_show_message(
                        "Resetting WiFi",
                        "Entering Setup"
                    );

                    vTaskDelay(
                        pdMS_TO_TICKS(2000)
                    );

                    wifi_manager_reset_provisioning();
                }
            }
        }
        else
        {
            is_boot_pressed = false; 
        }

        /****************************************************
                        TEMPERATURE
        ****************************************************/

        temperature_monitoring_update();

        /****************************************************
                            BUTTONS
        ****************************************************/

        button_handler_update();

        /****************************************************
                        TEC + FAN
        ****************************************************/

        tec_fan_controller_update(
            currentTemp
        );

        /****************************************************
                            BATTERY
        ****************************************************/

        float battery_percent =
            battery_monitor_get_percent();

        /****************************************************
                            WIFI
        ****************************************************/

        bool wifi_connected =
            wifi_manager_is_connected();

        /****************************************************
                            OLED
        ****************************************************/

        // Only update standard display if button is NOT held for more than 500ms
        if (!is_boot_pressed || ((xTaskGetTickCount() * portTICK_PERIOD_MS) - boot_press_start_time) < 500)
        {
            oled_display_update(
                currentTemp,
                setTemp,
                battery_percent,
                wifi_connected
            );
        }

        /****************************************************
                        THINGSPEAK
        ****************************************************/

        thingspeak_handler_update();

        /****************************************************
                            DEBUG
        ****************************************************/

        ESP_LOGI(
            TAG,
            "Temp=%.2f Set=%d Battery=%.1f%% WiFi=%s",

            currentTemp,

            setTemp,

            battery_percent,

            wifi_connected ?
            "CONNECTED" :
            "DISCONNECTED"
        );

        vTaskDelay(
            pdMS_TO_TICKS(100)
        );
    }
}