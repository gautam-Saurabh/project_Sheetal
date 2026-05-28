#include "pumpcontroller.h"
#include "config.h" // Pulls in PUMP_PIN and PUMP_ENABLED!

#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "PUMP";

/************************************************************
                    PUMP STATE
************************************************************/

static bool pump_state = false; 

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
            Lock pump to whatever the config dictates
    ******************************************************/
    
    gpio_set_level(PUMP_PIN, PUMP_ENABLED);
    pump_state = (PUMP_ENABLED == 1);
}

/************************************************************
                    STATUS
************************************************************/

bool pump_controller_is_on(void)
{
    return pump_state;
}