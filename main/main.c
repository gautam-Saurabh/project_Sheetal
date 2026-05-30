#include "main.h"
#include "config.h" 
#include "oled_display.h"
#include "wifi_manager.h"
#include "temperaturemonitoring.h"
#include "tec_fan_controller.h"
#include "pumpcontroller.h"
#include "button_handler.h"
#include "battery_monitor.h"
#include "cloud_handler.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include <stdio.h> 

static const char *TAG = "MAIN";

extern float currentTemp;
extern int setTemp;

void app_main(void)
{
    ESP_LOGW(TAG, "========================================");
    ESP_LOGW(TAG, "TEC Cooling Jacket Firmware - BOOTING");
    ESP_LOGW(TAG, "========================================");

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BOOT_BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&io_conf);

    ESP_LOGW(TAG, "Starting OLED Init...");
    oled_display_init();
    oled_show_message("Cooling Jacket", "Starting...");
    vTaskDelay(pdMS_TO_TICKS(1200));

    ESP_LOGW(TAG, "Starting Hardware Init...");
    battery_monitor_init();
    button_handler_init();
    temperature_monitoring_init();
    tec_fan_controller_init();
    pump_controller_init();
    
    for (int i = 0; i < 3; i++) {
        oled_show_message("Cooling Jacket", i == 0 ? "Initializing." : i == 1 ? "Initializing.." : "Initializing...");
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    ESP_LOGW(TAG, "Power Stabilizing... Wi-Fi starting in 2s...");
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    wifi_manager_init();
    ESP_LOGW(TAG, "Wi-Fi Init Complete! Entering Main Loop.");

    // INITIALIZE CLOUD HANDLER AFTER WIFI
    cloud_handler_init(); 

    uint32_t boot_press_start_time = 0;
    bool is_boot_pressed = false;
    bool reset_triggered = false;

    while (1)
    {
        if (gpio_get_level(BOOT_BUTTON_PIN) == 0) {
            if (!is_boot_pressed) {
                is_boot_pressed = true;
                boot_press_start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
                reset_triggered = false;
                wifi_manager_lock_screen(true); 
                oled_show_message("Keep Holding", "Factory Reset");
            } else if (!reset_triggered) {
                uint32_t hold_duration = (xTaskGetTickCount() * portTICK_PERIOD_MS) - boot_press_start_time;
                if (hold_duration >= 3000) {
                    reset_triggered = true; 
                    wifi_manager_factory_reset(); 
                }
            }
        } else {
            if (is_boot_pressed) wifi_manager_lock_screen(false);
            is_boot_pressed = false; 
        }

        temperature_monitoring_update();
        button_handler_update();
        tec_fan_controller_update(currentTemp);
        
        float battery_percent = battery_monitor_get_percent();
        bool wifi_connected = wifi_manager_is_connected();

        if (!wifi_manager_is_provisioning() && !is_boot_pressed && !wifi_manager_is_factory_resetting()) {
            
#if ENABLE_HARDWARE_ALERTS == 1
            int active_errors[10]; 
            int error_count = 0;

            if (setTemp < MIN_SET_TEMP || setTemp > MAX_SET_TEMP) {
                active_errors[error_count++] = 0;
            }
            if (!temperature_monitoring_is_valid()) {
                active_errors[error_count++] = 1; 
            } 
            if (sensor2Temp <= -100.0f) {
                active_errors[error_count++] = 2;
            }
            if (sensor3Temp <= -100.0f) {
                active_errors[error_count++] = 3;
            }
            if (!wifi_connected) {
                active_errors[error_count++] = 4;
            }

            if (error_count > 0) {
                uint32_t current_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
                int current_error_index = (current_ms / 1000) % error_count;
                int error_to_show = active_errors[current_error_index];

                if (current_ms % 1000 < 500) {
                    char err_code_str[16];
                    snprintf(err_code_str, sizeof(err_code_str), "CODE: E%02d", error_to_show); 
                    oled_show_message("HARDWARE FAULT", err_code_str);
                } else {
                    oled_show_message("", ""); 
                }
            } else {
                oled_display_update(currentTemp, setTemp, battery_percent, wifi_connected);
            }
#else
            oled_display_update(currentTemp, setTemp, battery_percent, wifi_connected);
#endif
        }

        // CLOUD SYNC
        cloud_handler_update();

        ESP_LOGW(TAG, "Temp=%.2f Set=%d Battery=%.1f%% WiFi=%s", 
                 currentTemp, setTemp, battery_percent, wifi_connected ? "CONNECTED" : "DISCONNECTED");

        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}