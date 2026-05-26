#include "temperaturemonitoring.h"
#include "config.h"

#include "esp_log.h"
#include "esp_err.h"

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
                    INIT
************************************************************/

void temperature_monitoring_init(void)
{
    onewire_bus_config_t bus_config = {
        .bus_gpio_num = TEMP1_PIN,
    };

    onewire_bus_rmt_config_t rmt_config = {
        .max_rx_bytes = 10,
    };

    /******************************************************
                    SENSOR 1 BUS
    ******************************************************/

    ESP_ERROR_CHECK(
        onewire_new_bus_rmt(
            &bus_config,
            &rmt_config,
            &bus1
        )
    );

    /******************************************************
                    SENSOR 2 BUS
    ******************************************************/

    bus_config.bus_gpio_num = TEMP2_PIN;

    ESP_ERROR_CHECK(
        onewire_new_bus_rmt(
            &bus_config,
            &rmt_config,
            &bus2
        )
    );

    /******************************************************
                    SENSOR 3 BUS
    ******************************************************/

    bus_config.bus_gpio_num = TEMP3_PIN;

    ESP_ERROR_CHECK(
        onewire_new_bus_rmt(
            &bus_config,
            &rmt_config,
            &bus3
        )
    );

    /******************************************************
                    DS18B20 CONFIG
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

    ESP_LOGI(TAG, "DS18B20 sensors initialized");
}

/************************************************************
                    UPDATE
************************************************************/

void temperature_monitoring_update(void)
{
    /******************************************************
                START CONVERSION
    ******************************************************/

    ds18b20_trigger_temperature_conversion(sensor1);

    ds18b20_trigger_temperature_conversion(sensor2);

    ds18b20_trigger_temperature_conversion(sensor3);

    /******************************************************
                READ TEMPERATURES
    ******************************************************/

    float t1 = 0.0f;
    float t2 = 0.0f;
    float t3 = 0.0f;

    ds18b20_get_temperature(sensor1, &t1);

    ds18b20_get_temperature(sensor2, &t2);

    ds18b20_get_temperature(sensor3, &t3);

    /******************************************************
                VALIDITY CHECK
    ******************************************************/

    if (t1 != -127.0f)
        currentTemp = t1;

    if (t2 != -127.0f)
        sensor2Temp = t2;

    if (t3 != -127.0f)
        sensor3Temp = t3;

    /******************************************************
                    DEBUG LOG
    ******************************************************/

    ESP_LOGI(
        TAG,
        "T1=%.2f  T2=%.2f  T3=%.2f",
        currentTemp,
        sensor2Temp,
        sensor3Temp
    );
}