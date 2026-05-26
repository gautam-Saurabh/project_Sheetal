#include "thingspeakhandler.h"

#include "config.h"
#include "temperaturemonitoring.h"
#include "tec_fan_controller.h"
#include "pumpcontroller.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_timer.h"

#include <string.h>
#include <stdio.h>

static const char *TAG = "THINGSPEAK";

/************************************************************
                    WIFI STATUS
************************************************************/

static bool s_wifi_connected = false;

static uint64_t last_upload_time = 0;

static uint64_t last_reconnect_attempt = 0;

/************************************************************
                    WIFI EVENT HANDLER
************************************************************/

static void wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
    /******************************************************
                    WIFI START
    ******************************************************/

    if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }

    /******************************************************
                WIFI DISCONNECTED
    ******************************************************/

    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        s_wifi_connected = false;

        ESP_LOGW(TAG, "WiFi disconnected");
    }

    /******************************************************
                    GOT IP
    ******************************************************/

    else if (event_base == IP_EVENT &&
             event_id == IP_EVENT_STA_GOT_IP)
    {
        s_wifi_connected = true;

        ESP_LOGI(TAG, "WiFi connected");
    }
}

/************************************************************
                    WIFI INIT
************************************************************/

static void wifi_init(void)
{
    ESP_ERROR_CHECK(
        esp_netif_init()
    );

    ESP_ERROR_CHECK(
        esp_event_loop_create_default()
    );

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg =
        WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(
        esp_wifi_init(&cfg)
    );

    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            NULL,
            NULL
        )
    );

    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &wifi_event_handler,
            NULL,
            NULL
        )
    );

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    strcpy(
        (char *)wifi_config.sta.ssid,
        WIFI_SSID
    );

    strcpy(
        (char *)wifi_config.sta.password,
        WIFI_PASSWORD
    );

    ESP_ERROR_CHECK(
        esp_wifi_set_mode(WIFI_MODE_STA)
    );

    ESP_ERROR_CHECK(
        esp_wifi_set_config(
            WIFI_IF_STA,
            &wifi_config
        )
    );

    ESP_ERROR_CHECK(
        esp_wifi_start()
    );

    ESP_LOGI(TAG, "WiFi started");
}

/************************************************************
                THINGSPEAK UPLOAD
************************************************************/

static void thingspeak_upload(void)
{
    char url[512];

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

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 10000,
    };

    esp_http_client_handle_t client =
        esp_http_client_init(&config);

    esp_err_t err =
        esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        ESP_LOGI(
            TAG,
            "ThingSpeak upload successful"
        );
    }
    else
    {
        ESP_LOGE(
            TAG,
            "ThingSpeak upload failed: %s",
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
    esp_err_t ret =
        nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(
            nvs_flash_erase()
        );

        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    wifi_init();

    last_upload_time =
        esp_timer_get_time() / 1000ULL;

    last_reconnect_attempt =
        esp_timer_get_time() / 1000ULL;

    ESP_LOGI(
        TAG,
        "ThingSpeak handler initialized"
    );
}

/************************************************************
                    UPDATE
************************************************************/

void thingspeak_handler_update(void)
{
    uint64_t now =
        esp_timer_get_time() / 1000ULL;

    /******************************************************
                WIFI RECONNECT
    ******************************************************/

    if (!s_wifi_connected &&
        (now - last_reconnect_attempt >
         RECONNECT_INTERVAL_MS))
    {
        last_reconnect_attempt = now;

        ESP_LOGI(
            TAG,
            "Reconnecting WiFi..."
        );

        esp_wifi_connect();
    }

    /******************************************************
                THINGSPEAK UPLOAD
    ******************************************************/

    if (s_wifi_connected &&
        (now - last_upload_time >
         UPLOAD_INTERVAL_MS))
    {
        last_upload_time = now;

        thingspeak_upload();
    }
}

/************************************************************
                WIFI STATUS
************************************************************/

bool thingspeak_handler_is_wifi_connected(void)
{
    return s_wifi_connected;
}
