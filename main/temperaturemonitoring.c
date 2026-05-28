#include "temperaturemonitoring.h"
#include "config.h" 

#include "esp_log.h"
#include "esp_err.h"
#include "esp_random.h" 

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <math.h>

static const char *TAG = "TEMP";

// Global variables that ThingSpeak reads
float currentTemp = 0.0f;
float sensor2Temp = 0.0f;
float sensor3Temp = 0.0f;

static bool s_temp_valid = false;

// ====================================================================
//                       SIMULATION / CHAOS MODE
// ====================================================================
#if SIMULATION_MODE == 1

void temperature_monitoring_init(void)
{
    currentTemp = 25.0f;
    sensor2Temp = 25.0f;
    sensor3Temp = 25.0f;
    s_temp_valid = true;
    ESP_LOGW(TAG, "FULL CHAOS SIMULATION ACTIVE - Simulating 3 Sensors");
}

void temperature_monitoring_update(void)
{
    static uint32_t last_update = 0;
    uint32_t now = esp_log_timestamp();

    if ((now - last_update) < TEMP_UPDATE_INTERVAL_MS) return;
    last_update = now;

    // Generate 3 completely different random temperatures every second!
    // Sensor 1 swings wildly between 20.0 and 35.0
    currentTemp = 20.0f + ((float)(esp_random() % 150) / 10.0f);
    
    // Sensor 2 stays hot, between 35.0 and 45.0 (Simulating a hot TEC side)
    sensor2Temp = 35.0f + ((float)(esp_random() % 100) / 10.0f);
    
    // Sensor 3 stays cool, between 15.0 and 25.0 (Simulating outside ambient air)
    sensor3Temp = 15.0f + ((float)(esp_random() % 100) / 10.0f);
    
    s_temp_valid = true;
    
    // Log all three so you can watch them in the terminal
    ESP_LOGI(TAG, "CHAOS T1:%.1f | T2:%.1f | T3:%.1f", currentTemp, sensor2Temp, sensor3Temp);
}

bool temperature_monitoring_is_valid(void) { return s_temp_valid; }

// ====================================================================
//                       REAL HARDWARE MODE
// ====================================================================
#else

static onewire_bus_handle_t bus1 = NULL, bus2 = NULL, bus3 = NULL;
static ds18b20_device_handle_t sensor1 = NULL, sensor2 = NULL, sensor3 = NULL;

static bool is_valid_temp(float t)
{
    if (isnan(t) || t <= -100.0f || t >= 100.0f || t == 85.0f) return false;
    return true;
}

void temperature_monitoring_init(void)
{
    onewire_bus_rmt_config_t rmt_config = { .max_rx_bytes = 10, };
    ds18b20_config_t ds_cfg = {};
    onewire_bus_config_t bus_config = {};

    ESP_LOGI(TAG, "Scanning for physical temperature sensors...");

    // Sensor 1
    bus_config.bus_gpio_num = TEMP1_PIN;
    if (onewire_new_bus_rmt(&bus_config, &rmt_config, &bus1) == ESP_OK) {
        if (ds18b20_new_device_from_bus(bus1, &ds_cfg, &sensor1) == ESP_OK) {
            ds18b20_set_resolution(sensor1, DS18B20_RESOLUTION_12B);
        }
    }
    if (!sensor1) ESP_LOGW(TAG, "Sensor 1: MISSING"); else ESP_LOGI(TAG, "Sensor 1: CONNECTED");

    // Sensor 2
    bus_config.bus_gpio_num = TEMP2_PIN;
    if (onewire_new_bus_rmt(&bus_config, &rmt_config, &bus2) == ESP_OK) {
        if (ds18b20_new_device_from_bus(bus2, &ds_cfg, &sensor2) == ESP_OK) {
            ds18b20_set_resolution(sensor2, DS18B20_RESOLUTION_12B);
        }
    }
    
    // Sensor 3
    bus_config.bus_gpio_num = TEMP3_PIN;
    if (onewire_new_bus_rmt(&bus_config, &rmt_config, &bus3) == ESP_OK) {
        if (ds18b20_new_device_from_bus(bus3, &ds_cfg, &sensor3) == ESP_OK) {
            ds18b20_set_resolution(sensor3, DS18B20_RESOLUTION_12B);
        }
    }
}

void temperature_monitoring_update(void)
{
    static uint32_t last_update = 0;
    uint32_t now = esp_log_timestamp();

    if ((now - last_update) < TEMP_UPDATE_INTERVAL_MS) return;
    last_update = now;

    if (sensor1) ds18b20_trigger_temperature_conversion(sensor1);
    if (sensor2) ds18b20_trigger_temperature_conversion(sensor2);
    if (sensor3) ds18b20_trigger_temperature_conversion(sensor3);
    
    vTaskDelay(pdMS_TO_TICKS(800));

    currentTemp = 0.0f;
    sensor2Temp = 0.0f;
    sensor3Temp = 0.0f;
    s_temp_valid = false;

    if (sensor1) {
        float t1 = 0.0f;
        if (ds18b20_get_temperature(sensor1, &t1) == ESP_OK && is_valid_temp(t1)) {
            currentTemp = t1;
            s_temp_valid = true;
        }
    }
    if (sensor2) {
        float t2 = 0.0f;
        if (ds18b20_get_temperature(sensor2, &t2) == ESP_OK && is_valid_temp(t2)) sensor2Temp = t2;
    }
    if (sensor3) {
        float t3 = 0.0f;
        if (ds18b20_get_temperature(sensor3, &t3) == ESP_OK && is_valid_temp(t3)) sensor3Temp = t3;
    }
    
    ESP_LOGI(TAG, "HARDWARE T1=%.2f T2=%.2f T3=%.2f", currentTemp, sensor2Temp, sensor3Temp);
}

bool temperature_monitoring_is_valid(void) { return s_temp_valid; }

#endif