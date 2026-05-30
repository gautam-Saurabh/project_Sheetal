#include "battery_monitor.h"
#include "config.h" 
#include "tec_fan_controller.h" 
#include "esp_adc/adc_oneshot.h" 
#include "esp_log.h"

static const char *TAG = "BATTERY";

// Modern ADC Handle
static adc_oneshot_unit_handle_t adc1_handle;

// ==========================================
// SIMPLE CALIBRATION CONSTANTS
// ==========================================
// The exact Op-Amp voltage drop measured when TEC/Fan turn ON
#define LOAD_COMPENSATION_V     0.14f 

// Raw ADC pin voltages representing 100% and 0%
#define ADC_VOLTAGE_MAX         2.40f // Represents 100% (7.8V Pack)
#define ADC_VOLTAGE_MIN         2.00f // Represents 0% (6.2V Pack)

void battery_monitor_init(void)
{
    // 1. Initialize the ADC Unit
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1, // Battery is on ADC1
    };
    adc_oneshot_new_unit(&init_config, &adc1_handle);

    // 2. Configure the specific channel (Using the modern ADC_ATTEN_DB_12)
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12,
    };
    adc_oneshot_config_channel(adc1_handle, BATTERY_ADC_CHANNEL, &config);
    
    ESP_LOGI(TAG, "Battery monitor initialized (ESP-IDF v5 API)");
}

float battery_monitor_get_percent(void)
{
    // 1. Read Raw ADC using the modern API
    int raw = 0;
    adc_oneshot_read(adc1_handle, BATTERY_ADC_CHANNEL, &raw);
    
    // Convert to voltage (assuming 3.3V reference)
    float opamp_voltage = ((float)raw / 4095.0f) * 3.3f;
    
    // 2. APPLY LOAD COMPENSATION
    // If the heavy TEC load is running, add the sag back to the raw reading
    if (tec_fan_controller_is_cooling() == true) 
    {
        opamp_voltage += LOAD_COMPENSATION_V;
    }

    // 3. ULTRA-SIMPLE LINEAR MAPPING (No pack voltage math, no curves)
    float percent = ((opamp_voltage - ADC_VOLTAGE_MIN) / (ADC_VOLTAGE_MAX - ADC_VOLTAGE_MIN)) * 100.0f;

    // 4. Safety Limits (Clamp to 0-100%)
    if (percent > 100.0f) percent = 100.0f;
    if (percent < 0.0f)  percent = 0.0f;

    ESP_LOGI(TAG, "ADC=%.2fV BATTERY=%.1f%% (TEC: %s)", 
             opamp_voltage, percent, tec_fan_controller_is_cooling() ? "ON" : "OFF");

    return percent;
}