#include "oled_display.h"
#include "config.h" // Pulls in SCREEN_WIDTH, OLED_SDA, OLED_SCL, etc.
#include "tec_fan_controller.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "../components/u8g2/csrc/u8g2.h"
#include "../components/u8g2/csrc/u8x8.h"

#include "driver/i2c_master.h"

#include <stdio.h>
#include <stdbool.h>

/************************************************************
                        U8G2
************************************************************/

static u8g2_t u8g2;

/************************************************************
                    GPIO + DELAY CALLBACK
************************************************************/

uint8_t u8x8_gpio_and_delay_esp32(
    u8x8_t *u8x8,
    uint8_t msg,
    uint8_t arg_int,
    void *arg_ptr)
{
    switch (msg)
    {
        case U8X8_MSG_DELAY_MILLI:
            vTaskDelay(pdMS_TO_TICKS(arg_int));
            break;
    }

    return 0;
}

/************************************************************
                    I2C BYTE CALLBACK
************************************************************/

uint8_t u8x8_byte_esp32_hw_i2c(
    u8x8_t *u8x8,
    uint8_t msg,
    uint8_t arg_int,
    void *arg_ptr)
{
    static uint8_t buffer[128];
    static uint8_t buf_idx;
    static i2c_master_dev_handle_t dev_handle;

    switch (msg)
    {
        case U8X8_MSG_BYTE_INIT:
        {
            i2c_master_bus_config_t bus_config = {
                .i2c_port = I2C_NUM_0,
                .sda_io_num = OLED_SDA,
                .scl_io_num = OLED_SCL,
                .clk_source = I2C_CLK_SRC_DEFAULT,
                .glitch_ignore_cnt = 7,
                .flags.enable_internal_pullup = true
            };

            i2c_master_bus_handle_t bus_handle;

            i2c_new_master_bus(
                &bus_config,
                &bus_handle
            );

            i2c_device_config_t dev_config = {
                .dev_addr_length = I2C_ADDR_BIT_LEN_7,
                .device_address = OLED_I2C_ADDRESS,
                .scl_speed_hz = 400000
            };

            i2c_master_bus_add_device(
                bus_handle,
                &dev_config,
                &dev_handle
            );

            break;
        }

        case U8X8_MSG_BYTE_START_TRANSFER:
            buf_idx = 0;
            break;

        case U8X8_MSG_BYTE_SEND:
        {
            uint8_t *data = (uint8_t *)arg_ptr;

            while (arg_int > 0)
            {
                buffer[buf_idx++] = *data++;
                arg_int--;
            }

            break;
        }

        case U8X8_MSG_BYTE_END_TRANSFER:
        {
            i2c_master_transmit(
                dev_handle,
                buffer,
                buf_idx,
                -1
            );

            break;
        }
    }

    return 0;
}

/************************************************************
                    OLED INIT
************************************************************/

void oled_display_init(void)
{
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(
        &u8g2,
        U8G2_R0,
        u8x8_byte_esp32_hw_i2c,
        u8x8_gpio_and_delay_esp32
    );

    u8g2_InitDisplay(&u8g2);

    u8g2_SetPowerSave(&u8g2, 0);
}

/************************************************************
                    OLED UPDATE
************************************************************/
void oled_display_update(
    float current_temp,
    int set_temp,
    float battery_percent,
    bool wifi_connected)
{
    char buffer[32];

    u8g2_ClearBuffer(&u8g2);

    /************************************************
                    TOP STATUS
    ************************************************/

    u8g2_SetFont(
        &u8g2,
        u8g2_font_6x12_tf
    );

    sprintf(
        buffer,
        "BAT:%d %%",
        (int)battery_percent
    );

    u8g2_DrawStr(
        &u8g2,
        2,
        8,
        buffer
    );

    if (wifi_connected)
    {
        u8g2_DrawStr(
            &u8g2,
            85,
            8,
            "SYNC"
        );
    }
    else
    {
        u8g2_DrawStr(
            &u8g2,
            65,
            8,
            "Searching...."
        );
    }

    // Swapped 128 for SCREEN_WIDTH
    u8g2_DrawHLine(
        &u8g2,
        0,
        12,
        SCREEN_WIDTH
    );

    /************************************************
                    SET TEMP
    ************************************************/

    u8g2_SetFont(
        &u8g2,
        u8g2_font_6x12_tf
    );

    u8g2_DrawStr(
        &u8g2,
        2,
        26,
        "SET.TEMP:"
    );

    u8g2_SetFont(
        &u8g2,
        u8g2_font_logisoso16_tf
    );

    sprintf(
        buffer,
        "%dC",
        set_temp
    );

    u8g2_DrawStr(
        &u8g2,
        62,
        30,
        buffer
    );

    /************************************************
                    CURRENT TEMP
    ************************************************/

    u8g2_SetFont(
        &u8g2,
        u8g2_font_6x12_tf
    );

    u8g2_DrawStr(
        &u8g2,
        2,
        44,
        "CUR.TEMP:"
    );

    u8g2_SetFont(
        &u8g2,
        u8g2_font_logisoso16_tf
    );

    sprintf(
        buffer,
        "%.1fC",
        current_temp
    );

    u8g2_DrawStr(
        &u8g2,
        58,
        48,
        buffer
    );

    /************************************************
                    COOLING
    ************************************************/

    // Swapped 128 for SCREEN_WIDTH
    u8g2_DrawHLine(
        &u8g2,
        0,
        52,
        SCREEN_WIDTH
    );

    u8g2_SetFont(
        &u8g2,
        u8g2_font_6x12_tf
    );

    if (tec_fan_controller_is_cooling())
    {
        u8g2_DrawStr(
            &u8g2,
            2,
            63,
            "COOLING : ON"
        );
    }
    else
    {
        u8g2_DrawStr(
            &u8g2,
            2,
            63,
            "COOLING : OFF"
        );
    }

    u8g2_SendBuffer(&u8g2);
}

/************************************************
                OLED MESSAGE
************************************************/

void oled_show_message(
    const char *line1,
    const char *line2
)
{
    u8g2_ClearBuffer(&u8g2);

    u8g2_SetFont(
        &u8g2,
        u8g2_font_6x12_tf
    );

    int w1 =
        u8g2_GetStrWidth(
            &u8g2,
            line1
        );

    int w2 =
        u8g2_GetStrWidth(
            &u8g2,
            line2
        );

    // Swapped 128 for SCREEN_WIDTH to automatically center!
    u8g2_DrawStr(
        &u8g2,
        (SCREEN_WIDTH - w1) / 2,
        28,
        line1
    );

    u8g2_DrawStr(
        &u8g2,
        (SCREEN_WIDTH - w2) / 2,
        48,
        line2
    );

    u8g2_SendBuffer(&u8g2);
}