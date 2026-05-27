#include "main.h"
#include "config.h"

#include "wifi_manager.h"

#include "temperaturemonitoring.h"
#include "tec_fan_controller.h"
#include "pumpcontroller.h"
#include "button_handler.h"
#include "oled_display.h"
#include "battery_monitor.h"
#include "thingspeakhandler.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "driver/gpio.h"

static const char *TAG = "MAIN";

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
                        WIFI INIT
    ********************************************************/

    wifi_manager_init();

    /********************************************************
                            INIT
    ********************************************************/

    battery_monitor_init();

    button_handler_init();

    temperature_monitoring_init();

    tec_fan_controller_init();

    pump_controller_init();

    oled_display_init();

    thingspeak_handler_init();

    ESP_LOGI(TAG, "All modules initialized");

    /********************************************************
                            LOOP
    ********************************************************/

    while (1)
    {
        /****************************************************
                    RESET PROVISIONING
        ****************************************************/

        if (gpio_get_level(BOOT_BUTTON) == 0)
        {
            vTaskDelay(
                pdMS_TO_TICKS(3000)
            );

            if (gpio_get_level(BOOT_BUTTON) == 0)
            {
                ESP_LOGW(
                    TAG,
                    "Reset WiFi Provisioning"
                );

                wifi_manager_reset_provisioning();
            }
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

        oled_display_update(
            currentTemp,
            setTemp,
            battery_percent,
            wifi_connected
        );

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