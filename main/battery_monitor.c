#include "battery_monitor.h"
#include "config.h" 
#include "tec_fan_controller.h" // Needed to check TEC status for Load Compensation
#include "esp_adc/adc_oneshot.h" // MODERN ESP-IDF v5 API!
#include "esp_log.h"

static const char *TAG = "BATTERY";

// Modern ADC Handle
static adc_oneshot_unit_handle_t adc1_handle;

// ==========================================
// CALIBRATION CONSTANTS
// ==========================================
// The exact Op-Amp voltage drop measured when TEC/Fan turn ON
#define LOAD_COMPENSATION_V     0.14f 

// Using your Op-Amp boundaries instead of calculating the raw 8.4V limit
#define OPAMP_VOLTAGE_MAX       2.46f // Represents 8.4V (100%)
#define OPAMP_VOLTAGE_MIN       2.06f // Represents 6.2V (0%)

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
    
    float opamp_voltage = ((float)raw / 4095.0f) * 3.3f;
    
    // 2. APPLY LOAD COMPENSATION (The Engineer's Fix)
    // If the heavy TEC load is running, add the sag back to the raw reading!
    if (tec_fan_controller_is_cooling() == true) 
    {
        opamp_voltage += LOAD_COMPENSATION_V;
    }

    // Convert Op-Amp voltage back to estimated Pack Voltage so your curve works
    // (2.46V op-amp = 8.4V pack, 2.06V op-amp = 6.2V pack)
    float pack_voltage = ((opamp_voltage - OPAMP_VOLTAGE_MIN) / (OPAMP_VOLTAGE_MAX - OPAMP_VOLTAGE_MIN)) * (8.4f - 6.2f) + 6.2f;

    // 3. APPLY DYNAMIC DISCHARGE CURVE (Your Mathematician's Fix)
    float percent = 0.0f;
    float v = pack_voltage;

    if (v >= 8.4f) {
        percent = 100.0f;
    } 
    else if (v >= 7.8f) {
        percent = 80.0f + 20.0f * ((v - 7.8f) / (8.4f - 7.8f));
    } 
    else if (v >= 7.4f) {
        percent = 50.0f + 30.0f * ((v - 7.4f) / (7.8f - 7.4f));
    } 
    else if (v >= 6.8f) {
        percent = 10.0f + 40.0f * ((v - 6.8f) / (7.4f - 6.8f));
    } 
    else if (v >= 6.2f) {
        percent = 0.0f + 10.0f * ((v - 6.2f) / (6.8f - 6.2f));
    } 
    else {
        percent = 0.0f;
    }

    // 4. Safety Limits
    if (percent > 100.0f) percent = 100.0f;
    if (percent < 0.0f) percent = 0.0f;

    ESP_LOGI(TAG, "ADC=%.2fV PACK=%.2fV BATTERY=%.1f%% (TEC: %s)", 
             opamp_voltage, pack_voltage, percent, tec_fan_controller_is_cooling() ? "ON" : "OFF");

    return percent;
}