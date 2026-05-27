#include "temperaturemonitoring.h"
#include "config.h"

#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <math.h>

static const char *TAG = "TEMP";

/************************************************************
                    ONEWIRE BUS
************************************************************/

static onewire_bus_handle_t bus1 = NULL;
static onewire_bus_handle_t bus2 = NULL;
static onewire_bus_handle_t bus3 = NULL;

/************************************************************
                    SENSOR HANDLES
************************************************************/

static ds18b20_device_handle_t sensor1 = NULL;
static ds18b20_device_handle_t sensor2 = NULL;
static ds18b20_device_handle_t sensor3 = NULL;

/************************************************************
                    GLOBAL TEMPERATURES
************************************************************/

float currentTemp = 0.0f;

float sensor2Temp = 0.0f;

float sensor3Temp = 0.0f;

/************************************************************
                TEMPERATURE VALID FLAG
************************************************************/

static bool s_temp_valid = false;

/************************************************************
                    INIT
************************************************************/

void temperature_monitoring_init(void)
{
    onewire_bus_rmt_config_t rmt_config = {

        .max_rx_bytes = 10,
    };

    onewire_bus_config_t bus_config = {

        .bus_gpio_num = TEMP1_PIN,
    };

    /******************************************************
                    SENSOR 1
    ******************************************************/

    ESP_ERROR_CHECK(
        onewire_new_bus_rmt(
            &bus_config,
            &rmt_config,
            &bus1
        )
    );

    /******************************************************
                    SENSOR 2
    ******************************************************/

    bus_config.bus_gpio_num =
        TEMP2_PIN;

    ESP_ERROR_CHECK(
        onewire_new_bus_rmt(
            &bus_config,
            &rmt_config,
            &bus2
        )
    );

    /******************************************************
                    SENSOR 3
    ******************************************************/

    bus_config.bus_gpio_num =
        TEMP3_PIN;

    ESP_ERROR_CHECK(
        onewire_new_bus_rmt(
            &bus_config,
            &rmt_config,
            &bus3
        )
    );

    /******************************************************
                    SENSOR CONFIG
    ******************************************************/

    ds18b20_config_t ds_cfg = {};

    ESP_ERROR_CHECK(
        ds18b20_new_device_from_bus(
            bus1,
            &ds_cfg,
            &sensor1
        )
    );

    ESP_ERROR_CHECK(
        ds18b20_new_device_from_bus(
            bus2,
            &ds_cfg,
            &sensor2
        )
    );

    ESP_ERROR_CHECK(
        ds18b20_new_device_from_bus(
            bus3,
            &ds_cfg,
            &sensor3
        )
    );

    /******************************************************
                    RESOLUTION
    ******************************************************/

    ds18b20_set_resolution(
        sensor1,
        DS18B20_RESOLUTION_12B
    );

    ds18b20_set_resolution(
        sensor2,
        DS18B20_RESOLUTION_12B
    );

    ds18b20_set_resolution(
        sensor3,
        DS18B20_RESOLUTION_12B
    );

    ESP_LOGI(
        TAG,
        "DS18B20 Initialized"
    );
}

/************************************************************
                VALID TEMP CHECK
************************************************************/

static bool is_valid_temp(float t)
{
    if (isnan(t))
    {
        return false;
    }

    if (t <= -100.0f)
    {
        return false;
    }

    if (t >= 100.0f)
    {
        return false;
    }

    if (t == 85.0f)
    {
        return false;
    }

    return true;
}

/************************************************************
                    UPDATE
************************************************************/

void temperature_monitoring_update(void)
{
    static uint32_t last_update = 0;

    uint32_t now =
        esp_log_timestamp();

    /******************************************************
                UPDATE EVERY 1 SECOND
    ******************************************************/

    if ((now - last_update) < 1000)
    {
        return;
    }

    last_update = now;

    /******************************************************
                START CONVERSION
    ******************************************************/

    ds18b20_trigger_temperature_conversion(
        sensor1
    );

    ds18b20_trigger_temperature_conversion(
        sensor2
    );

    ds18b20_trigger_temperature_conversion(
        sensor3
    );

    /******************************************************
                WAIT FOR CONVERSION
    ******************************************************/

    vTaskDelay(
        pdMS_TO_TICKS(800)
    );

    /******************************************************
                READ TEMPERATURES
    ******************************************************/

    float t1 = 0.0f;
    float t2 = 0.0f;
    float t3 = 0.0f;

    esp_err_t err1 =
        ds18b20_get_temperature(
            sensor1,
            &t1
        );

    esp_err_t err2 =
        ds18b20_get_temperature(
            sensor2,
            &t2
        );

    esp_err_t err3 =
        ds18b20_get_temperature(
            sensor3,
            &t3
        );

    /******************************************************
                VALIDATION
    ******************************************************/

    bool valid1 =
        (err1 == ESP_OK) &&
        is_valid_temp(t1);

    bool valid2 =
        (err2 == ESP_OK) &&
        is_valid_temp(t2);

    bool valid3 =
        (err3 == ESP_OK) &&
        is_valid_temp(t3);

    if (valid1)
    {
        currentTemp = t1;
    }

    if (valid2)
    {
        sensor2Temp = t2;
    }

    if (valid3)
    {
        sensor3Temp = t3;
    }

    s_temp_valid =
        valid1 || valid2 || valid3;

    /******************************************************
                    DEBUG
    ******************************************************/

    ESP_LOGI(
        TAG,
        "T1=%.2f T2=%.2f T3=%.2f",
        currentTemp,
        sensor2Temp,
        sensor3Temp
    );
}

/************************************************************
                TEMPERATURE STATUS
************************************************************/

bool temperature_monitoring_is_valid(void)
{
    return s_temp_valid;
}