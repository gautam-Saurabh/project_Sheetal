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

extern float currentTemp;
extern int setTemp;

void app_main(void)
{
    // Upgraded to LOGW (Warning) so your terminal actually prints them!
    ESP_LOGW(TAG, "========================================");
    ESP_LOGW(TAG, "TEC Cooling Jacket Firmware - BOOTING");
    ESP_LOGW(TAG, "========================================");

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BOOT_BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&io_conf);

    ESP_LOGW(TAG, "1. Starting OLED Init...");
    oled_display_init();
    oled_show_message("Cooling Jacket", "Starting...");
    vTaskDelay(pdMS_TO_TICKS(1200));

    ESP_LOGW(TAG, "2. Starting Hardware Init...");
    battery_monitor_init();
    button_handler_init();
    temperature_monitoring_init();
    tec_fan_controller_init();
    pump_controller_init();
    thingspeak_handler_init();

    for (int i = 0; i < 3; i++)
    {
        oled_show_message("Cooling Jacket", i == 0 ? "Initializing." : i == 1 ? "Initializing.." : "Initializing...");
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    // THE MAGIC POWER FIX: Give the USB port 2 seconds to stabilize before the Wi-Fi spike!
    ESP_LOGW(TAG, "3. Power Stabilizing... Wi-Fi starting in 2s...");
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    wifi_manager_init();
    ESP_LOGW(TAG, "4. Wi-Fi Init Complete! Entering Main Loop.");

    uint32_t boot_press_start_time = 0;
    bool is_boot_pressed = false;
    bool reset_triggered = false;

    while (1)
    {
        if (gpio_get_level(BOOT_BUTTON_PIN) == 0)
        {
            if (!is_boot_pressed)
            {
                is_boot_pressed = true;
                boot_press_start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
                reset_triggered = false;
                
                wifi_manager_lock_screen(true); 
                oled_show_message("Keep Holding", "Factory Reset");
            }
            else if (!reset_triggered)
            {
                uint32_t hold_duration = (xTaskGetTickCount() * portTICK_PERIOD_MS) - boot_press_start_time;
                if (hold_duration >= 3000) 
                {
                    reset_triggered = true; 
                    wifi_manager_factory_reset(); 
                }
            }
        }
        else
        {
            if (is_boot_pressed) wifi_manager_lock_screen(false);
            is_boot_pressed = false; 
        }

        temperature_monitoring_update();
        button_handler_update();
        tec_fan_controller_update(currentTemp);
        float battery_percent = battery_monitor_get_percent();
        bool wifi_connected = wifi_manager_is_connected();

        if (!wifi_manager_is_provisioning() && !is_boot_pressed && !wifi_manager_is_factory_resetting()) 
        {
            oled_display_update(currentTemp, setTemp, battery_percent, wifi_connected);
        }

        thingspeak_handler_update();

        // Print the real-time status as a Warning so it punches through the filter!
        ESP_LOGW(TAG, "Temp=%.2f Set=%d Battery=%.1f%% WiFi=%s", currentTemp, setTemp, battery_percent, wifi_connected ? "CONNECTED" : "DISCONNECTED");

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}