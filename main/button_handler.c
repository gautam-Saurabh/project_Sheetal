#include "button_handler.h"
#include "config.h" // Pulls in your DEFAULT, MAX, MIN temperatures, and BUTTON timings!

#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "BUTTON";

int setTemp = DEFAULT_SET_TEMP;

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
        .pin_bit_mask = (1ULL << SW1_PIN) | (1ULL << SW4_PIN)
    };

    ESP_ERROR_CHECK(gpio_config(&io_conf));

    setTemp = DEFAULT_SET_TEMP;

    ESP_LOGI(TAG, "Button handler initialized");
}

/************************************************************
                        UPDATE
************************************************************/

void button_handler_update(void)
{
    uint64_t now = esp_timer_get_time() / 1000ULL;

    /************************************************
            BUTTON REPEAT SPEED
    ************************************************/
    
    // Now pulling the speed directly from config.h!
    if ((now - last_repeat_time) < BUTTON_HOLD_SPEED_MS)
    {
        return;
    }

    /************************************************
                INCREASE BUTTON
    ************************************************/
    if (gpio_get_level(SW1_PIN) == 0)
    {
        setTemp++;

        if (setTemp > MAX_SET_TEMP)
        {
            setTemp = MAX_SET_TEMP;
        }

        last_repeat_time = now;
        ESP_LOGI(TAG, "SetTemp = %d", setTemp);
    }

    /************************************************
                DECREASE BUTTON
    ************************************************/
    if (gpio_get_level(SW4_PIN) == 0)
    {
        setTemp--;

        if (setTemp < MIN_SET_TEMP)
        {
            setTemp = MIN_SET_TEMP;
        }

        last_repeat_time = now;
        ESP_LOGI(TAG, "SetTemp = %d", setTemp);
    }
}