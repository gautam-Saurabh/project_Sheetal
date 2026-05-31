#include "tec_fan_controller.h"
#include "button_handler.h"
#include "config.h" // Pulls in HYSTERESIS, TEC_PIN, FAN_PIN, and OVERRIDE states!

#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "TEC_FAN";

// Cooling state & Override state
static bool cooling_state = false;
static int cloud_override_state = OVERRIDE_AUTO; // Default to automatic mode (-1)

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
    /******************************************************
            1. CHECK CLOUD OVERRIDE FIRST
    ******************************************************/
    if (cloud_override_state == OVERRIDE_ON) {
        cooling_state = true;
        gpio_set_level(TEC_PIN, 1);
        gpio_set_level(FAN_PIN, 1);
        return; // Exit early! Ignore the temperature sensors entirely.
    } 
    else if (cloud_override_state == OVERRIDE_OFF) {
        cooling_state = false;
        gpio_set_level(TEC_PIN, 0);
        gpio_set_level(FAN_PIN, 0);
        return; // Exit early! Ignore the temperature sensors entirely.
    }

    /******************************************************
            2. AUTOMATIC HYSTERESIS MODE
    ******************************************************/
    
    // TURN ON COOLING (e.g., SET=25, ON at 26)
    if (current_temp >= (setTemp + (HYSTERESIS / 2.0f)))
    {
        cooling_state = true;
    }

    // TURN OFF COOLING (e.g., SET=25, OFF at 24)
    if (current_temp <= (setTemp - (HYSTERESIS / 2.0f)))
    {
        cooling_state = false;
    }

    /************************************************
                OUTPUT CONTROL
    ************************************************/

    gpio_set_level(TEC_PIN, cooling_state);
    gpio_set_level(FAN_PIN, cooling_state);

    // Limit log spam by only logging on state change or occasionally if needed,
    // but leaving your original log here for debugging.
    ESP_LOGI(
        TAG,
        "Temp=%.1f Set=%d Cooling=%s",
        current_temp,
        setTemp,
        cooling_state ? "ON" : "OFF"
    );
}

/************************************************************
                CLOUD OVERRIDE CONTROL
************************************************************/

void tec_fan_controller_set_override(int state)
{
    cloud_override_state = state;
    ESP_LOGI(TAG, "Cloud Override State Changed to: %d", state);
}

/************************************************************
            COOLING STATUS FOR OLED
************************************************************/

bool tec_fan_controller_is_cooling(void)
{
    return cooling_state;
}