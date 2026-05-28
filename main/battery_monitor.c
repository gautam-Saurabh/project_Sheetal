#include "battery_monitor.h"
#include "config.h" 
#include "tec_fan_controller.h" // Needed to check TEC status for Load Compensation
#include "driver/adc.h"
#include "esp_log.h"

static const char *TAG = "BATTERY";

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
    // Initialize your specific ADC channel here based on your hardware layout
    adc1_config_width(ADC_WIDTH_BIT_12); // Assuming standard 12-bit
    adc1_config_channel_atten(BATTERY_ADC_CHANNEL, ADC_ATTEN_DB_11); 
    
    ESP_LOGI(TAG, "Battery monitor initialized with Load Compensation & Dynamic Curve");
}

float battery_monitor_get_percent(void)
{
    // 1. Read Raw ADC
    int raw = adc1_get_raw(BATTERY_ADC_CHANNEL);
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