#include "button_handler.h"
#include "config.h"

#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "BUTTON";

int setTemp = 25;

// Timing
static uint64_t last_repeat_time = 0;

/************************************************************
                        INIT
************************************************************/

void button_handler_init(void)
{
    gpio_config_t io_conf = {

        .intr_type = GPIO_INTR_DISABLE,

        .mode = GPIO_MODE_INPUT,

        .pull_up_en = GPIO_PULLUP_ENABLE,

        .pull_down_en = GPIO_PULLDOWN_DISABLE,

        .pin_bit_mask =
            (1ULL << SW1_PIN) |
            (1ULL << SW4_PIN)
    };

    ESP_ERROR_CHECK(
        gpio_config(&io_conf)
    );

    setTemp = 25;

    ESP_LOGI(TAG, "Button handler initialized");
}

/************************************************************
                        UPDATE
************************************************************/

void button_handler_update(void)
{
    uint64_t now =
        esp_timer_get_time() / 1000ULL;

    /************************************************
            BUTTON REPEAT SPEED
    ************************************************/

    // Faster response while holding
    if ((now - last_repeat_time) < 120)
    {
        return;
    }

    /************************************************
                INCREASE BUTTON
    ************************************************/

    if (gpio_get_level(SW1_PIN) == 0)
    {
        setTemp++;

        if (setTemp > 30)
        {
            setTemp = 30;
        }

        last_repeat_time = now;

        ESP_LOGI(
            TAG,
            "SetTemp = %d",
            setTemp
        );
    }

    /************************************************
                DECREASE BUTTON
    ************************************************/

    if (gpio_get_level(SW4_PIN) == 0)
    {
        setTemp--;

        if (setTemp < 25)
        {
            setTemp = 25;
        }

        last_repeat_time = now;

        ESP_LOGI(
            TAG,
            "SetTemp = %d",
            setTemp
        );
    }
}