#include "thingspeakhandler.h"

#include "temperaturemonitoring.h"
#include "tec_fan_controller.h"
#include "pumpcontroller.h"
#include "wifi_manager.h"
#include "config.h"

#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_timer.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

static const char *TAG = "THINGSPEAK";

static uint64_t last_upload_time = 0;

/************************************************************
                VALID TEMPERATURE CHECK
************************************************************/

static bool is_temperature_valid(float temp)
{
    if (isnan(temp))
    {
        return false;
    }

    if (temp <= -100.0f)
    {
        return false;
    }

    if (temp >= 100.0f)
    {
        return false;
    }

    if (fabs(temp - 85.0f) < 0.01f)
    {
        return false;
    }

    return true;
}

/************************************************************
                    THINGSPEAK UPLOAD
************************************************************/

static void thingspeak_upload(void)
{
    /******************************************************
                VALIDATION
    ******************************************************/

    if (!is_temperature_valid(currentTemp))
    {
        ESP_LOGW(
            TAG,
            "Invalid Temperature"
        );

        return;
    }

    /******************************************************
                    URL
    ******************************************************/

    char url[256];

    snprintf(
        url,
        sizeof(url),

        "http://api.thingspeak.com/update?"
        "api_key=%s"
        "&field1=%.2f"
        "&field2=%.2f"
        "&field3=%.2f"
        "&field4=%d"
        "&field5=%d",

        THINGSPEAK_WRITE_API_KEY,

        currentTemp,
        sensor2Temp,
        sensor3Temp,

        tec_fan_controller_is_cooling() ? 1 : 0,

        pump_controller_is_on() ? 1 : 0
    );

    /******************************************************
                HTTP CONFIG
    ******************************************************/

    esp_http_client_config_t config = {

        .url = url,

        .method = HTTP_METHOD_GET,

        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client =
        esp_http_client_init(&config);

    if (client == NULL)
    {
        ESP_LOGE(
            TAG,
            "HTTP Client Init Failed"
        );

        return;
    }

    /******************************************************
                SEND REQUEST
    ******************************************************/

    esp_err_t err =
        esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        int status_code =
            esp_http_client_get_status_code(client);

        if (status_code == 200)
        {
            ESP_LOGI(
                TAG,
                "Upload Successful"
            );
        }
        else
        {
            ESP_LOGW(
                TAG,
                "HTTP Status = %d",
                status_code
            );
        }
    }
    else
    {
        ESP_LOGE(
            TAG,
            "Upload Failed: %s",
            esp_err_to_name(err)
        );
    }

    esp_http_client_cleanup(client);
}

/************************************************************
                        INIT
************************************************************/

void thingspeak_handler_init(void)
{
    last_upload_time =
        esp_timer_get_time() / 1000ULL;

    ESP_LOGI(
        TAG,
        "ThingSpeak Initialized"
    );
}

/************************************************************
                        UPDATE
************************************************************/

void thingspeak_handler_update(void)
{
    /******************************************************
                WIFI CHECK
    ******************************************************/

    if (!wifi_manager_is_connected())
    {
        return;
    }

    /******************************************************
                TIME CHECK
    ******************************************************/

    uint64_t now =
        esp_timer_get_time() / 1000ULL;

    if ((now - last_upload_time) >=
        UPLOAD_INTERVAL_MS)
    {
        last_upload_time = now;

        thingspeak_upload();
    }
}