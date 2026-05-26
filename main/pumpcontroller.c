#include "pumpcontroller.h"
#include "config.h"

#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "PUMP";

/************************************************************
                    PUMP STATE
************************************************************/

static bool pump_state = true;

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

    ESP_ERROR_CHECK(
        gpio_config(&io_conf)
    );

    /******************************************************
                    ALWAYS ON
    ******************************************************/

    gpio_set_level(PUMP_PIN, 1);

    pump_state = true;

    ESP_LOGI(
        TAG,
        "Pump initialized and ON"
    );
}

/************************************************************
                    UPDATE
************************************************************/

void pump_controller_update(void)
{
    /******************************************************
            Pump stays permanently ON
    ******************************************************/

    gpio_set_level(PUMP_PIN, 1);

    pump_state = true;
}

/************************************************************
                    STATUS
************************************************************/

bool pump_controller_is_on(void)
{
    return pump_state;
}