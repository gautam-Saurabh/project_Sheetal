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

// Globals from your temperature file
extern float currentTemp;
extern float sensor2Temp;
extern float sensor3Temp;

static uint64_t last_upload_time = 0;

/************************************************************
                    THINGSPEAK UPLOAD
************************************************************/

static void thingspeak_upload(void)
{
    /******************************************************
                VALIDATION GUARDRAIL
    ******************************************************/
    if (!temperature_monitoring_is_valid())
    {
        ESP_LOGW(TAG, "Invalid Temperature - Skipping Upload");
        return;
    }

    /******************************************************
                BUILD THE URL (THE FIX!)
    ******************************************************/
    char url[256];

    // The variables at the bottom MUST match the field1, field2, field3 order!
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
        currentTemp,                              // Field 1 (Main Sensor)
        sensor2Temp,                              // Field 2
        sensor3Temp,                              // Field 3
        tec_fan_controller_is_cooling() ? 1 : 0,  // Field 4 (TEC Status)
        pump_controller_is_on() ? 1 : 0           // Field 5 (Pump Status)
    );

    /******************************************************
                HTTP CONFIG
    ******************************************************/
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    if (client == NULL)
    {
        ESP_LOGE(TAG, "HTTP Client Init Failed");
        return;
    }

    /******************************************************
                SEND REQUEST & CLEANUP
    ******************************************************/
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200)
        {
            ESP_LOGI(TAG, "Upload Successful");
        }
        else
        {
            ESP_LOGW(TAG, "HTTP Status = %d", status_code);
        }
    }
    else
    {
        ESP_LOGE(TAG, "Upload Failed: %s", esp_err_to_name(err));
    }

    // CRITICAL: Prevents memory leaks so the ESP32 never crashes!
    esp_http_client_cleanup(client);
}

/************************************************************
                        INIT
************************************************************/

void thingspeak_handler_init(void)
{
    last_upload_time = esp_timer_get_time() / 1000ULL;
    ESP_LOGI(TAG, "ThingSpeak Initialized");
}

/************************************************************
                        UPDATE
************************************************************/

void thingspeak_handler_update(void)
{
    // Do not attempt upload if Wi-Fi is down or in Setup Mode
    if (!wifi_manager_is_connected())
    {
        return;
    }

    uint64_t now = esp_timer_get_time() / 1000ULL;

    // UPLOAD_INTERVAL_MS is pulled from your config.h (usually 15000)
    if ((now - last_upload_time) >= UPLOAD_INTERVAL_MS)
    {
        last_upload_time = now;
        thingspeak_upload();
    }
}