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
#include <stdio.h>
#include <string.h>
#include <time.h>

// --- SECURE MQTT INCLUDES ---
#include "mqtt_client.h"
#include "esp_sntp.h"
#include "esp_netif_sntp.h"

static const char *TAG = "CLOUD_ANEDYA";

// --- EXTERNAL VARIABLES ---
extern float currentTemp;
extern float sensor2Temp;
extern float sensor3Temp;
extern int setTemp; 

// --- TIMERS (Now accessible globally for instant-reset) ---
static uint64_t last_upload_time = 0;
static uint64_t last_heartbeat_time = 0;

static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool mqtt_connected = false;
static bool mqtt_started = false;

// Embedded ECC Certificate
extern const uint8_t anedyaeca3_crt_start[] asm("_binary_anedyaeca3_crt_start");
extern const uint8_t anedyaeca3_crt_end[]   asm("_binary_anedyaeca3_crt_end");

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    if (event_id == MQTT_EVENT_CONNECTED) {
        ESP_LOGI(TAG, "Secure MQTT Connected - Heartbeat Active! 💓");
        mqtt_connected = true;
        
        // --- BOOSTER FIX: FORCE INSTANT UPLOAD ---
        // Setting these to 0 guarantees the next loop fires immediately!
        last_upload_time = 0;
        last_heartbeat_time = 0;
        
    } else if (event_id == MQTT_EVENT_DISCONNECTED) {
        ESP_LOGW(TAG, "Secure MQTT Disconnected.");
        mqtt_connected = false;
    } else if (event_id == MQTT_EVENT_ERROR) {
        ESP_LOGE(TAG, "MQTT ERROR OCCURRED!");
    }
}

void cloud_handler_init(void) {
    ESP_LOGI(TAG, "Initializing Cloud Handler (Anedya SECURE MQTT Mode)");
    
    // 1. Initialize NTP using Anedya's dedicated Time Server
    esp_sntp_config_t sntp_config = ESP_NETIF_SNTP_DEFAULT_CONFIG("time.anedya.io");
    esp_netif_sntp_init(&sntp_config);

    // 2. Safely copy and null-terminate the ECC certificate
    size_t cert_len = anedyaeca3_crt_end - anedyaeca3_crt_start;
    char *cert_buffer = malloc(cert_len + 1);
    memcpy(cert_buffer, anedyaeca3_crt_start, cert_len);
    cert_buffer[cert_len] = '\0';

    // 3. Configure MQTT parameters
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = ANEDYA_MQTT_URI,
        .broker.verification.certificate = cert_buffer, 
        .broker.verification.skip_cert_common_name_check = true,
        
        .credentials.client_id = ANEDYA_DEVICE_ID,
        .credentials.username = ANEDYA_DEVICE_ID,
        .credentials.authentication.password = ANEDYA_CONNECTION_KEY,
    };
    
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
}

void cloud_handler_update(void) {
    if (!wifi_manager_is_connected()) return;

    // --- TIME SYNC GATEWAY ---
    if (!mqtt_started) {
        time_t now_sec;
        time(&now_sec);
        if (now_sec > 1700000000) { 
            ESP_LOGI(TAG, "Time Sync OK! Epoch: %lld. Starting MQTT Handshake...", (long long)now_sec);
            esp_mqtt_client_start(mqtt_client);
            mqtt_started = true;
        } else {
            return; 
        }
    }

    if (mqtt_connected) {
        
        uint64_t current_ms = esp_timer_get_time() / 1000ULL;
        
        // =========================================================
        //                 HEARTBEAT ENGINE (Every 30s)
        // =========================================================
        if ((current_ms - last_heartbeat_time) >= 30000 || last_heartbeat_time == 0) {
            char hb_topic[128];
            snprintf(hb_topic, sizeof(hb_topic), "$anedya/device/%s/heartbeat/json", ANEDYA_DEVICE_ID);
            
            int hb_msg = esp_mqtt_client_publish(mqtt_client, hb_topic, "{}", 0, 1, 0);
            if(hb_msg != -1) {
                ESP_LOGI(TAG, "💓 Heartbeat Sent to Anedya!");
            }
            last_heartbeat_time = current_ms;
        }
        
        // =========================================================
        //                 TELEMETRY ENGINE (Every 15s)
        // =========================================================
        if ((current_ms - last_upload_time) < UPLOAD_INTERVAL_MS && last_upload_time != 0) return;
        last_upload_time = current_ms;

        if (!temperature_monitoring_is_valid()) return;

        cJSON *root = cJSON_CreateObject();
        cJSON *data_array = cJSON_CreateArray();
        cJSON_AddItemToObject(root, "data", data_array);

        time_t now_sec;
        time(&now_sec);
        uint64_t current_epoch_ms = (uint64_t)now_sec * 1000ULL;

        #define ADD_NUM_VAR(n, v) do { \
            cJSON *node = cJSON_CreateObject(); \
            cJSON_AddStringToObject(node, "variable", n); \
            cJSON_AddNumberToObject(node, "value", (double)(v)); \
            cJSON_AddNumberToObject(node, "timestamp", (double)current_epoch_ms); \
            cJSON_AddItemToArray(data_array, node); \
        } while(0)

        // Sending the 5 recognized variables
        ADD_NUM_VAR("temp-main", currentTemp);
        ADD_NUM_VAR("temp-sensor2", sensor2Temp);
        ADD_NUM_VAR("temp-sensor3", sensor3Temp);
        ADD_NUM_VAR("tec-status", tec_fan_controller_is_cooling() ? 1.0 : 0.0);
        ADD_NUM_VAR("pump-status", pump_controller_is_on() ? 1.0 : 0.0);
        
        char *json_string = cJSON_PrintUnformatted(root);
        char topic[128];
        snprintf(topic, sizeof(topic), "$anedya/device/%s/submitdata/json", ANEDYA_DEVICE_ID);
        
        int msg_id = esp_mqtt_client_publish(mqtt_client, topic, json_string, 0, 1, 0);
        if(msg_id != -1) {
            ESP_LOGI(TAG, "Telemetry Uploaded -> %s", json_string);
        } else {
            ESP_LOGW(TAG, "Telemetry Upload Failed.");
        }
        
        cJSON_Delete(root);
        free(json_string);
    }
}