#include "tec_fan_controller.h"
#include "button_handler.h"
#include "config.h" // Pulls in HYSTERESIS, TEC_PIN, and FAN_PIN!

#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "TEC_FAN";

// Cooling state
static bool cooling_state = false;

/************************************************************
                        INIT
************************************************************/

void tec_fan_controller_init(void)
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pin_bit_mask = (1ULL << TEC_PIN) | (1ULL << FAN_PIN)
    };

    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Initial OFF state
    gpio_set_level(TEC_PIN, 0);
    gpio_set_level(FAN_PIN, 0);

    cooling_state = false;

    ESP_LOGI(TAG, "TEC + FAN initialized");
}

/************************************************************
                        UPDATE
************************************************************/

void tec_fan_controller_update(float current_temp)
{
    /************************************************
            TURN ON COOLING

        Example with HYSTERESIS of 2.0:
        SET = 25
        ON at 26 (25 + 1.0)
    ************************************************/
    
    if (current_temp >= (setTemp + (HYSTERESIS / 2.0f)))
    {
        cooling_state = true;
    }

    /************************************************
            TURN OFF COOLING

        Example with HYSTERESIS of 2.0:
        SET = 25
        OFF at 24 (25 - 1.0)
    ************************************************/
    
    if (current_temp <= (setTemp - (HYSTERESIS / 2.0f)))
    {
        cooling_state = false;
    }

    /************************************************
                OUTPUT CONTROL
    ************************************************/

    gpio_set_level(TEC_PIN, cooling_state);
    gpio_set_level(FAN_PIN, cooling_state);

    // Note: Since this runs in your main while(1) loop every 100ms, 
    // it will print to the serial monitor very fast!
    ESP_LOGI(
        TAG,
        "Temp=%.1f Set=%d Cooling=%s",
        current_temp,
        setTemp,
        cooling_state ? "ON" : "OFF"
    );
}

/************************************************************
            COOLING STATUS FOR OLED
************************************************************/

bool tec_fan_controller_is_cooling(void)
{
    return cooling_state;
}