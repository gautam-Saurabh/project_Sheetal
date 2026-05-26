#include "battery_monitor.h"
#include "config.h"

#include "driver/adc.h"
#include "esp_log.h"

static const char *TAG = "BATTERY";

/************************************************************
                    INIT
************************************************************/

void battery_monitor_init(void)
{
    adc1_config_width(ADC_WIDTH);

    adc1_config_channel_atten(
        ADC_CHANNEL_0,
        ADC_ATTEN
    );

    ESP_LOGI(
        TAG,
        "Battery monitor initialized"
    );
}

/************************************************************
                GET BATTERY PERCENT
************************************************************/

float battery_monitor_get_percent(void)
{
    /************************************************
                    READ ADC
    ************************************************/

    int raw =
        adc1_get_raw(ADC_CHANNEL_0);

    /************************************************
                ADC VOLTAGE
    ************************************************/

    float adc_voltage =
        ((float)raw / 4095.0f) * 3.3f;

    /************************************************
            CALIBRATED BATTERY VOLTAGE

        Measured values:

        8.4V -> 2.60V ADC
        7.4V -> 2.29V ADC
        6.2V -> 1.92V ADC

        Ratio:
        8.4 / 2.60 = 3.23
    ************************************************/

    float battery_voltage =
        adc_voltage * 3.23f;

    /************************************************
                BATTERY PERCENT
    ************************************************/

    float percent =
        ((battery_voltage - 6.2f)
        / (8.4f - 6.2f))
        * 100.0f;

    /************************************************
                    LIMITS
    ************************************************/

    if (percent > 100.0f)
        percent = 100.0f;

    if (percent < 0.0f)
        percent = 0.0f;

    /************************************************
                    DEBUG
    ************************************************/

    ESP_LOGI(
        TAG,
        "RAW=%d ADC=%.2fV BAT=%.2fV BATTERY=%.1f%%",
        raw,
        adc_voltage,
        battery_voltage,
        percent
    );

    return percent;
}