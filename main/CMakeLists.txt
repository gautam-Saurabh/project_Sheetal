idf_component_register(

    SRCS

        "main.c"
        "temperaturemonitoring.c"
        "tec_fan_controller.c"
        "pumpcontroller.c"
        "thingspeakhandler.c"
        "oled_display.c"
        "battery_monitor.c"
        "button_handler.c"

        "../components/u8g2/csrc/u8g2_arc.c"
        "../components/u8g2/csrc/u8g2_bitmap.c"
        "../components/u8g2/csrc/u8g2_box.c"
        "../components/u8g2/csrc/u8g2_buffer.c"
        "../components/u8g2/csrc/u8g2_circle.c"
        "../components/u8g2/csrc/u8g2_cleardisplay.c"
        "../components/u8g2/csrc/u8g2_d_memory.c"
        "../components/u8g2/csrc/u8g2_d_setup.c"
        "../components/u8g2/csrc/u8g2_font.c"
        "../components/u8g2/csrc/u8g2_fonts.c"
        "../components/u8g2/csrc/u8g2_hvline.c"
        "../components/u8g2/csrc/u8g2_input_value.c"
        "../components/u8g2/csrc/u8g2_intersection.c"
        "../components/u8g2/csrc/u8g2_kerning.c"
        "../components/u8g2/csrc/u8g2_line.c"
        "../components/u8g2/csrc/u8g2_ll_hvline.c"
        "../components/u8g2/csrc/u8g2_message.c"
        "../components/u8g2/csrc/u8g2_polygon.c"
        "../components/u8g2/csrc/u8g2_selection_list.c"
        "../components/u8g2/csrc/u8g2_setup.c"

        "../components/u8g2/csrc/u8x8_8x8.c"
        "../components/u8g2/csrc/u8x8_byte.c"
        "../components/u8g2/csrc/u8x8_cad.c"
        "../components/u8g2/csrc/u8x8_display.c"
        "../components/u8g2/csrc/u8x8_fonts.c"
        "../components/u8g2/csrc/u8x8_gpio.c"
        "../components/u8g2/csrc/u8x8_input_value.c"
        "../components/u8g2/csrc/u8x8_message.c"
        "../components/u8g2/csrc/u8x8_selection_list.c"
        "../components/u8g2/csrc/u8x8_setup.c"
        "../components/u8g2/csrc/u8x8_string.c"
        "../components/u8g2/csrc/u8x8_u16toa.c"
        "../components/u8g2/csrc/u8x8_u8toa.c"

        "../components/u8g2/csrc/u8x8_d_ssd1306_128x64_noname.c"

    INCLUDE_DIRS
        "."
        "../components/u8g2/csrc"

    REQUIRES
        driver
        esp_wifi
        esp_event
        esp_netif
        nvs_flash
        esp_http_client
        esp_timer
        esp_adc
        onewire_bus
        ds18b20
)