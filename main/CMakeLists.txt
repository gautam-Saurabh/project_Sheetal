idf_component_register(

    SRCS

        "main.c"
        "wifi_manager.c"
        "temperaturemonitoring.c"
        "tec_fan_controller.c"
        "pumpcontroller.c"
        "thingspeakhandler.c"
        "oled_display.c"
        "battery_monitor.c"
        "button_handler.c"

        "../components/u8g2/csrc/u8g2_bitmap.c"
        "../components/u8g2/csrc/u8g2_buffer.c"
        "../components/u8g2/csrc/u8g2_cleardisplay.c"
        "../components/u8g2/csrc/u8g2_d_memory.c"
        "../components/u8g2/csrc/u8g2_d_setup.c"
        "../components/u8g2/csrc/u8g2_font.c"
        "../components/u8g2/csrc/u8g2_fonts.c"
        "../components/u8g2/csrc/u8g2_hvline.c"
        "../components/u8g2/csrc/u8g2_line.c"
        "../components/u8g2/csrc/u8g2_ll_hvline.c"
        "../components/u8g2/csrc/u8g2_setup.c"

        "../components/u8g2/csrc/u8x8_byte.c"
        "../components/u8g2/csrc/u8x8_cad.c"
        "../components/u8g2/csrc/u8x8_display.c"
        "../components/u8g2/csrc/u8x8_fonts.c"
        "../components/u8g2/csrc/u8x8_gpio.c"
        "../components/u8g2/csrc/u8x8_setup.c"
        "../components/u8g2/csrc/u8x8_string.c"

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
        wifi_provisioning
        onewire_bus
        ds18b20
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Os)