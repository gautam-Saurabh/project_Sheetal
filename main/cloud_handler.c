#include "cloud_handler.h"
#include "config.h"
#include "temperaturemonitoring.h"
#include "tec_fan_controller.h"
#include "pumpcontroller.h"
#include "wifi_manager.h"
#include "battery_monitor.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "cJSON.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "CLOUD";

// --- EXTERNAL VARIABLES ---
extern float currentTemp;
extern float sensor2Temp;
extern float sensor3Temp;
extern int setTemp; // <--- IMPORTANT: Change "targetTemp" to whatever variable name you actually use in your switch logic!

static uint64_t last_upload_time = 0;

void cloud_handler_init(void) {
#if USE_ANEDYA == 1
    ESP_LOGI(TAG, "Initializing Cloud Handler (Anedya HTTP POST Mode)");
#else
    ESP_LOGI(TAG, "Initializing Cloud Handler (ThingSpeak HTTP GET Mode)");
#endif
    last_upload_time = esp_timer_get_time() / 1000ULL;
}

void cloud_handler_update(void) {
    if (!wifi_manager_is_connected()) return;

    uint64_t now = esp_timer_get_time() / 1000ULL;
    if ((now - last_upload_time) < UPLOAD_INTERVAL_MS) return;
    last_upload_time = now;

    if (!temperature_monitoring_is_valid()) return;

#if USE_ANEDYA == 1
    // ========================================================================
    //                           ANEDYA HTTP POST ENGINE
    // ========================================================================
    cJSON *root = cJSON_CreateObject();
    cJSON *data_array = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "data", data_array);

    #define ADD_NUM_VAR(n, v) do { \
        cJSON *node = cJSON_CreateObject(); \
        cJSON_AddStringToObject(node, "variable", n); \
        cJSON_AddNumberToObject(node, "value", v); \
        cJSON_AddItemToArray(data_array, node); \
    } while(0)

    // Your 7 Dashboard Variables (Now 100% dynamic!)
    ADD_NUM_VAR("temp-main", currentTemp);
    ADD_NUM_VAR("temp-sensor2", sensor2Temp);
    ADD_NUM_VAR("temp-sensor3", sensor3Temp);
    ADD_NUM_VAR("tec-status", tec_fan_controller_is_cooling() ? 1.0 : 0.0);
    ADD_NUM_VAR("pump-status", pump_controller_is_on() ? 1.0 : 0.0);
    ADD_NUM_VAR("battery", battery_monitor_get_percent());
    ADD_NUM_VAR("set-temp", setTemp); // <--- Using the live dynamic variable!
    
    char *json_string = cJSON_PrintUnformatted(root);
    
    char url[256];
    snprintf(url, sizeof(url), "https://device.%s.anedya.io/v1/submitData", ANEDYA_REGION);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Auth-mode", "key");
    esp_http_client_set_header(client, "Authorization", ANEDYA_CONNECTION_KEY);

    esp_http_client_set_post_field(client, json_string, strlen(json_string));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        if (status == 200) {
            ESP_LOGI(TAG, "Anedya Upload Successful!");
        } else {
            ESP_LOGW(TAG, "Anedya Upload Failed. HTTP Status: %d", status);
        }
    } else {
        ESP_LOGE(TAG, "Anedya HTTP POST Error: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    cJSON_Delete(root);
    free(json_string);

#else
    // ========================================================================
    //                           THINGSPEAK HTTP GET ENGINE
    // ========================================================================
    char url[300]; 
    snprintf(
        url,
        sizeof(url),
        "http://api.thingspeak.com/update?api_key=%s&field1=%.2f&field2=%.2f&field3=%.2f&field4=%d&field5=%d&field6=%.1f&field7=%d",
        THINGSPEAK_WRITE_API_KEY, 
        currentTemp, 
        sensor2Temp, 
        sensor3Temp, 
        tec_fan_controller_is_cooling() ? 1 : 0, 
        pump_controller_is_on() ? 1 : 0,
        battery_monitor_get_percent(),
        setTemp // <--- Using the live dynamic variable!
    );

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200) {
            ESP_LOGI(TAG, "ThingSpeak Upload Successful!");
        } else {
            ESP_LOGW(TAG, "ThingSpeak HTTP Status = %d", status_code);
        }
    } else {
        ESP_LOGE(TAG, "ThingSpeak Upload Failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
#endif
}