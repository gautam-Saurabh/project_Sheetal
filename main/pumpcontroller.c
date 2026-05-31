#include "pumpcontroller.h"
#include "config.h" // Pulls in PUMP_PIN, PUMP_ENABLED, and OVERRIDE states!

#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "PUMP";

/************************************************************
                    PUMP STATE & OVERRIDE
************************************************************/

static bool pump_state = false; 
static int cloud_override_state = OVERRIDE_AUTO; // Default to automatic mode (-1)

/************************************************************
                    INIT
************************************************************/

void pump_controller_init(void)
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pin_bit_mask = (1ULL << PUMP_PIN)
    };

    ESP_ERROR_CHECK(gpio_config(&io_conf));

    /******************************************************
            SET PUMP BASED ON MASTER CONFIG
    ******************************************************/
    
    gpio_set_level(PUMP_PIN, PUMP_ENABLED);
    
    // Convert the 1 or 0 into a boolean true or false
    pump_state = (PUMP_ENABLED == 1);

    ESP_LOGI(TAG, "Pump initialized: %s", pump_state ? "ON" : "OFF (Battery Saver)");
}

/************************************************************
                    UPDATE
************************************************************/

void pump_controller_update(void)
{
    /******************************************************
            1. CHECK CLOUD OVERRIDE FIRST
    ******************************************************/
    if (cloud_override_state == OVERRIDE_ON) {
        gpio_set_level(PUMP_PIN, 1);
        pump_state = true;
        return; // Exit early! Ignore the automatic config below.
    } 
    else if (cloud_override_state == OVERRIDE_OFF) {
        gpio_set_level(PUMP_PIN, 0);
        pump_state = false;
        return; // Exit early! Ignore the automatic config below.
    }

    /******************************************************
            2. AUTOMATIC MODE (If no cloud override)
    ******************************************************/
    // Lock pump to whatever the config dictates
    gpio_set_level(PUMP_PIN, PUMP_ENABLED);
    pump_state = (PUMP_ENABLED == 1);
}

/************************************************************
                    CLOUD OVERRIDE CONTROL
************************************************************/

void pump_controller_set_override(int state)
{
    cloud_override_state = state;
    ESP_LOGI(TAG, "Cloud Override State Changed to: %d", state);
}

/************************************************************
                    STATUS
************************************************************/

bool pump_controller_is_on(void)
{
    return pump_state;
}