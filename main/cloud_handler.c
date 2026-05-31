#include "cloud_handler.h"
#include "config.h"
#include "temperaturemonitoring.h"
#include "tec_fan_controller.h"
#include "pumpcontroller.h"
#include "wifi_manager.h"
#include "battery_monitor.h"
#include "oled_display.h" 
#include "esp_log.h"
#include "esp_timer.h"
#include "cJSON.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

// --- FreeRTOS INCLUDES (For vTaskDelay) ---
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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

// --- TIMERS ---
static uint64_t last_upload_time = 0;
static uint64_t last_heartbeat_time = 0;

static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool mqtt_connected = false;
static bool mqtt_started = false;

// Embedded ECC Certificate
extern const uint8_t anedyaeca3_crt_start[] asm("_binary_anedyaeca3_crt_start");
extern const uint8_t anedyaeca3_crt_end[]   asm("_binary_anedyaeca3_crt_end");

// =========================================================
//         HARDWARE DIAGNOSTIC ENGINE (E00 - E05)
// =========================================================
static void execute_hardware_diagnostics(void) {
    char error_code[16] = "SYSTEM HEALTHY";
    
    // Check E00: System Memory Fault (setTemp corrupted)
    if (setTemp < 10 || setTemp > 45) {
        strcpy(error_code, "ERR: E00");
    }
    // Check E01-E03: Temperature Sensors Lost
    else if (!temperature_monitoring_is_valid()) {
        strcpy(error_code, "ERR: E01-E03");
    }
    // Check E04: Wi-Fi Network Lost
    else if (!wifi_manager_is_connected()) {
        strcpy(error_code, "ERR: E04");
    }
    
    // Log it to the serial monitor
    ESP_LOGW(TAG, "DIAGNOSTIC RESULT: %s", error_code);
    
    // Flash it on the OLED Screen!
    oled_show_message("DIAGNOSTICS", error_code);
    
    // Hold the message on screen for 3 seconds so the user can read it
    vTaskDelay(3000 / portTICK_PERIOD_MS); 
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    
    if (event_id == MQTT_EVENT_CONNECTED) {
        ESP_LOGI(TAG, "Secure MQTT Connected - Heartbeat Active! 💓");
        mqtt_connected = true;
        
        last_upload_time = 0;
        last_heartbeat_time = 0;
        
        char cmd_topic[128];
        snprintf(cmd_topic, sizeof(cmd_topic), "$anedya/device/%s/commands", ANEDYA_DEVICE_ID);
        esp_mqtt_client_subscribe(mqtt_client, cmd_topic, 1); 
        ESP_LOGI(TAG, "Subscribed to Commands Topic: %s", cmd_topic);
        
    } else if (event_id == MQTT_EVENT_DISCONNECTED) {
        ESP_LOGW(TAG, "Secure MQTT Disconnected.");
        mqtt_connected = false;
        
    } else if (event_id == MQTT_EVENT_ERROR) {
        ESP_LOGE(TAG, "MQTT ERROR OCCURRED!");
        
    // =========================================================
    //         THE MASTER COMMAND SWITCHBOARD
    // =========================================================
    } else if (event_id == MQTT_EVENT_DATA) {
        ESP_LOGI(TAG, "Incoming Cloud Message Received!");
        
        char *payload = malloc(event->data_len + 1);
        if (payload) {
            strncpy(payload, event->data, event->data_len);
            payload[event->data_len] = '\0';
            
            cJSON *json = cJSON_Parse(payload);
            if (json != NULL) {
                cJSON *cmd_node = cJSON_GetObjectItemCaseSensitive(json, "command");
                cJSON *data_node = cJSON_GetObjectItemCaseSensitive(json, "data");
                cJSON *id_node = cJSON_GetObjectItemCaseSensitive(json, "commandId");
                
                if (cJSON_IsString(cmd_node) && cJSON_IsString(id_node)) {
                    bool command_success = false;
                    const char* command_name = cmd_node->valuestring;

                    // Relaxed parsing: extract integer safely from string or number nodes
                    int cmd_val = 0;
                    if (data_node != NULL) {
                        if (cJSON_IsNumber(data_node)) {
                            cmd_val = data_node->valueint;
                        } else if (cJSON_IsString(data_node)) {
                            cmd_val = atoi(data_node->valuestring);
                        }
                    }
                    
                    // --- 1. SET TEMPERATURE ---
                    if (strcmp(command_name, "set-temp") == 0) {
                        if (cmd_val >= 10 && cmd_val <= 40) { 
                            setTemp = cmd_val;
                            ESP_LOGW(TAG, "CLOUD: Target Temp set to %d C", setTemp);
                            command_success = true;
                        }
                    }
                    // --- 2. PUMP OVERRIDE (1 = ON, 0 = OFF, -1 = AUTO) ---
                    else if (strcmp(command_name, "pump-override") == 0) {
                        ESP_LOGW(TAG, "CLOUD: Pump Override State -> %d", cmd_val);
                        pump_controller_set_override(cmd_val);
                        command_success = true;
                    }
                    // --- 3. TEC COOLER OVERRIDE (1 = ON, 0 = OFF, -1 = AUTO) ---
                    else if (strcmp(command_name, "tec-override") == 0) {
                        ESP_LOGW(TAG, "CLOUD: TEC Override State -> %d", cmd_val);
                        tec_fan_controller_set_override(cmd_val);
                        command_success = true;
                    }
                    // --- 4. RUN DIAGNOSTICS ---
                    else if (strcmp(command_name, "run-diagnostic") == 0) {
                        ESP_LOGW(TAG, "CLOUD: Running Hardware Diagnostics...");
                        execute_hardware_diagnostics(); 
                        command_success = true;
                    }
                    // --- 5. SYSTEM REBOOT ---
                    else if (strcmp(command_name, "system-reboot") == 0) {
                        ESP_LOGW(TAG, "CLOUD: Initiating System Reboot...");
                        command_success = true;
                    }
                    // --- 6. WIFI RESET ---
                    else if (strcmp(command_name, "wifi-reset") == 0) {
                        ESP_LOGW(TAG, "CLOUD: Erasing Wi-Fi Credentials & Rebooting...");
                        command_success = true;
                    }

                    // =========================================================
                    //         SEND ACKNOWLEDGMENT TO CLEAR "PENDING"
                    // =========================================================
                    if (command_success) {
                        // FIXED: Corrected token casing to updateStatus per Anedya specification
                        char ack_topic[128];
                        snprintf(ack_topic, sizeof(ack_topic), "$anedya/device/%s/commands/updateStatus/json", ANEDYA_DEVICE_ID);
                        
                        cJSON *ack_json = cJSON_CreateObject();
                        cJSON_AddStringToObject(ack_json, "commandId", id_node->valuestring);
                        cJSON_AddStringToObject(ack_json, "status", "success");
                        cJSON_AddStringToObject(ack_json, "ackdata", "Executed by ESP32");
                        cJSON_AddStringToObject(ack_json, "ackdatatype", "string");
                        
                        char *ack_str = cJSON_PrintUnformatted(ack_json);
                        esp_mqtt_client_publish(mqtt_client, ack_topic, ack_str, 0, 1, 0);
                        ESP_LOGI(TAG, "Command ACK Sent -> %s", ack_str);
                        
                        cJSON_Delete(ack_json);
                        free(ack_str);

                        // --- EXECUTE DELAYED HARDWARE ACTIONS ---
                        // Executed after ACK submission to maintain session stability
                        if (strcmp(command_name, "system-reboot") == 0) {
                            vTaskDelay(2000 / portTICK_PERIOD_MS); 
                            esp_restart(); 
                        } 
                        else if (strcmp(command_name, "wifi-reset") == 0) {
                            vTaskDelay(2000 / portTICK_PERIOD_MS);
                            wifi_manager_factory_reset(); 
                        }
                    } else {
                        ESP_LOGW(TAG, "Command Rejected or Unknown.");
                    }
                }
                cJSON_Delete(json); 
            }
            free(payload);
        }
    }
}

void cloud_handler_init(void) {
    ESP_LOGI(TAG, "Initializing Cloud Handler (Anedya STABLE Mode)");
    
    esp_sntp_config_t sntp_config = ESP_NETIF_SNTP_DEFAULT_CONFIG("time.anedya.io");
    esp_netif_sntp_init(&sntp_config);

    size_t cert_len = anedyaeca3_crt_end - anedyaeca3_crt_start;
    char *cert_buffer = malloc(cert_len + 1);
    memcpy(cert_buffer, anedyaeca3_crt_start, cert_len);
    cert_buffer[cert_len] = '\0';

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = ANEDYA_MQTT_URI,
        .broker.verification.certificate = cert_buffer, 
        .broker.verification.skip_cert_common_name_check = true,
        
        .credentials.client_id = ANEDYA_DEVICE_ID,
        .credentials.username = ANEDYA_DEVICE_ID,
        .credentials.authentication.password = ANEDYA_CONNECTION_KEY,
        
        .session.keepalive = 60,
    };
    
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
}

void cloud_handler_update(void) {
    if (!wifi_manager_is_connected()) return;

    if (!mqtt_started) {
        time_t now_sec;
        time(&now_sec);
        if (now_sec > 1700000000) { 
            ESP_LOGI(TAG, "Time Sync OK! Starting Stable Handshake...");
            esp_mqtt_client_start(mqtt_client);
            mqtt_started = true;
        } else {
            return; 
        }
    }

    if (mqtt_connected) {
        uint64_t current_ms = esp_timer_get_time() / 1000ULL;
        
        // =========================================================
        //         STABLE HEARTBEAT ENGINE (Every 30s)
        // =========================================================
        if ((current_ms - last_heartbeat_time) >= 30000 || last_heartbeat_time == 0) {
            char hb_topic[128];
            snprintf(hb_topic, sizeof(hb_topic), "$anedya/device/%s/heartbeat/json", ANEDYA_DEVICE_ID);
            
            int hb_msg = esp_mqtt_client_publish(mqtt_client, hb_topic, "{}", 0, 1, 0);
            if(hb_msg != -1) {
                ESP_LOGI(TAG, "💓 Stable Heartbeat Sent!");
            }
            last_heartbeat_time = current_ms;
        }
        
        // =========================================================
        //         STABLE TELEMETRY ENGINE (Every 10s)
        // =========================================================
        if ((current_ms - last_upload_time) < 10000 && last_upload_time != 0) return;
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

        ADD_NUM_VAR("temp-main", currentTemp);
        ADD_NUM_VAR("temp-sensor2", sensor2Temp);
        ADD_NUM_VAR("temp-sensor3", sensor3Temp);
        ADD_NUM_VAR("tec-status", tec_fan_controller_is_cooling() ? 1.0 : 0.0);
        ADD_NUM_VAR("pump-status", pump_controller_is_on() ? 1.0 : 0.0);
        ADD_NUM_VAR("battery", battery_monitor_get_percent()); 
        ADD_NUM_VAR("set-temp", setTemp); 
        
        char *json_string = cJSON_PrintUnformatted(root);
        char topic[128];
        snprintf(topic, sizeof(topic), "$anedya/device/%s/submitdata/json", ANEDYA_DEVICE_ID);
        
        int msg_id = esp_mqtt_client_publish(mqtt_client, topic, json_string, 0, 1, 0);
        if(msg_id != -1) {
            ESP_LOGI(TAG, "Stable Telemetry Sent -> %s", json_string);
        } else {
            ESP_LOGW(TAG, "Stable Telemetry Failed.");
        }
        
        cJSON_Delete(root);
        free(json_string);
    }
}