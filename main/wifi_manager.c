#include "wifi_manager.h"
#include "config.h" 

#include <stdbool.h>
#include <stdio.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_softap.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "oled_display.h"
    
/************************************************************
                        GLOBALS
************************************************************/

static const char *TAG = "WIFI";

static bool s_wifi_connected = false;
static bool s_is_provisioning = false;
static int s_retry_count = 0;

static bool s_factory_resetting = false; 
static bool s_screen_locked_by_main = false; 

// ONE STRIKE TRIGGER FOR WRONG PASSWORD
static bool s_wrong_password_triggered = false; 
static bool s_failed_connection_triggered = false;

/************************************************************
                SCREEN LOCK CONTROLLERS
************************************************************/
void wifi_manager_lock_screen(bool lock)
{
    s_screen_locked_by_main = lock;
}

bool wifi_manager_is_factory_resetting(void)
{
    return s_factory_resetting || s_wrong_password_triggered || s_failed_connection_triggered;
}

/************************************************************
                TOTAL FACTORY RESET (NUCLEAR WIPE)
************************************************************/

void wifi_manager_factory_reset(void)
{
    if (s_factory_resetting) return;
    s_factory_resetting = true;

    ESP_LOGW(TAG, "EXECUTING TOTAL HARDWARE FACTORY RESET");
    
    // Lock the screen so main.c stays away
    wifi_manager_lock_screen(true);

    const char* erasing_anim[] = {"Erasing Data", "Erasing Data.", "Erasing Data..", "Erasing Data..."};
    
    // Show first part of animation
    oled_show_message("Factory Reset", erasing_anim[0]);
    vTaskDelay(pdMS_TO_TICKS(500));

    // 1. SAFELY DISCONNECT: Tell the phone the connection is dead, then give the phone 500ms to realize it!
    esp_wifi_disconnect();
    esp_wifi_stop();
    oled_show_message("Factory Reset", erasing_anim[1]);
    vTaskDelay(pdMS_TO_TICKS(500)); // Crucial delay to stop phone hanging
    
    // 2. SAFELY WIPE MEMORY: Give the chip 500ms to delete the files so it doesn't corrupt!
    esp_wifi_restore();
    wifi_prov_mgr_reset_provisioning();
    oled_show_message("Factory Reset", erasing_anim[2]);
    vTaskDelay(pdMS_TO_TICKS(500)); 
    
    // 3. FINAL NUKE
    nvs_flash_erase(); 
    oled_show_message("Factory Reset", erasing_anim[3]);
    vTaskDelay(pdMS_TO_TICKS(500));

    // 4. REBOOT
    esp_restart();
}

/************************************************************
                        LED TASK
************************************************************/

static void led_task(void *pv)
{
    bool blink_state = false;
    int dot_counter = 0;

    while (1)
    {
        /**************************************************
             ONE-STRIKE WRONG PASSWORD SEQUENCE
        **************************************************/
        if (s_wrong_password_triggered)
        {
            // Lock out main.c so the normal screen doesn't overlap
            wifi_manager_lock_screen(true);
            gpio_set_level(WIFI_LED_PIN, 1);

            const char* try_again_anim[] = {"Try Again", "Try Again.", "Try Again..", "Try Again..."};
            
            // Show "Wrong Password / Try Again..." for exactly 3 seconds
            for (int i = 0; i < 6; i++) {
                oled_show_message("Wrong Password", try_again_anim[i % 4]);
                vTaskDelay(pdMS_TO_TICKS(500));
            }

            // Once the message is done, carefully execute the reset with safe delays!
            wifi_manager_factory_reset();
            continue; 
        }

        /**************************************************
             FAILED CONNECTION SEQUENCE (5 DROPS)
        **************************************************/
        if (s_failed_connection_triggered)
        {
            wifi_manager_factory_reset();
            continue;
        }

        // ANTI-FLICKER CRITICAL CHECK: If button is held, freeze this task completely!
        if (s_screen_locked_by_main)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        dot_counter++;
        int dots = dot_counter % 4; 
        const char* waiting_anim[] = {"- Waiting -", "- Waiting .", "- Waiting ..", "- Waiting ..."};

        /**************************************************
                    WIFI CONNECTED
        **************************************************/
        if (s_wifi_connected)
        {
            gpio_set_level(WIFI_LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        /**************************************************
                    PROVISIONING (NORMAL WAITING)
        **************************************************/
        else if (s_is_provisioning)
        {
            blink_state = !blink_state;
            gpio_set_level(WIFI_LED_PIN, blink_state ? 1 : 0);

            if (blink_state) 
            {
                oled_show_message("WiFi Setup Mode", "Connect Using App");
            } 
            else 
            {
                oled_show_message("WiFi Setup Mode", waiting_anim[dots]);
            }
            
            // Now dynamically linked to your config file!
            vTaskDelay(pdMS_TO_TICKS(WIFI_LED_BLINK_MS));
        }

        /**************************************************
                    DISCONNECTED
        **************************************************/
        else
        {
            gpio_set_level(WIFI_LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

/************************************************************
                START SOFTAP PROVISIONING
************************************************************/

static void start_provisioning(void)
{
    s_is_provisioning = true;
    s_factory_resetting = false;
    s_wrong_password_triggered = false;
    s_failed_connection_triggered = false;

    wifi_prov_security_t security = WIFI_PROV_SECURITY_1;
    
    // Variables pull directly from config.h now
    const char *pop = PROV_POP;
    const char *service_name = PROV_AP_SSID;
    const char *service_key = PROV_AP_PASS;

    ESP_LOGI(TAG, "================================");
    ESP_LOGI(TAG, "SOFTAP PROVISIONING STARTED");
    ESP_LOGI(TAG, "AP NAME : %s", service_name);
    ESP_LOGI(TAG, "AP PASS : %s", service_key);
    ESP_LOGI(TAG, "POP     : %s", pop);
    ESP_LOGI(TAG, "================================");

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_start());
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, pop, service_name, service_key));
}

/************************************************************
                    EVENT HANDLER
************************************************************/

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "STA Started");
                break;

            case WIFI_EVENT_STA_DISCONNECTED:
                s_wifi_connected = false;

                if (s_is_provisioning) break;

                // NORMAL BOOT ONLY: Try to connect to saved router
                s_retry_count++;
                
                if (s_retry_count >= MAX_WIFI_RETRY)
                {
                    ESP_LOGE(TAG, "Saved network connection failed. Triggering Reset.");
                    s_failed_connection_triggered = true; 
                }
                else
                {
                    char retry_msg[32];
                    const char* dots_str = (s_retry_count % 3 == 1) ? "." : (s_retry_count % 3 == 2) ? ".." : "...";
                    sprintf(retry_msg, "Retrying%s %d/%d", dots_str, s_retry_count, MAX_WIFI_RETRY);
                    
                    oled_show_message("WiFi Connection", retry_msg);
                    ESP_LOGW(TAG, "Reconnect Retry %d/%d", s_retry_count, MAX_WIFI_RETRY);
                    
                    esp_wifi_connect();
                }
                break;

            default:
                break;
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        s_wifi_connected = true;
        s_is_provisioning = false;
        s_retry_count = 0;
        s_factory_resetting = false;
        s_wrong_password_triggered = false;
        s_failed_connection_triggered = false;

        oled_show_message("WiFi Connected", "Setup Complete");
        vTaskDelay(pdMS_TO_TICKS(1500));
        ESP_LOGI(TAG, "WiFi Connected");
    }
    else if (event_base == WIFI_PROV_EVENT)
    {
        switch (event_id)
        {
            case WIFI_PROV_START:
                ESP_LOGI(TAG, "Provisioning Started");
                break;
            case WIFI_PROV_CRED_RECV:
                ESP_LOGI(TAG, "Credentials Received");
                break;
            case WIFI_PROV_CRED_SUCCESS:
                ESP_LOGI(TAG, "Provisioning Success");
                esp_wifi_connect();
                break;
            case WIFI_PROV_CRED_FAIL:
                ESP_LOGE(TAG, "Provisioning Failed - Wrong Password");
                
                // INSTANT 1-STRIKE FAILURE FOR WRONG PASSWORD
                // Triggers the LED task to show the 3-second message, then safe wipe.
                s_wrong_password_triggered = true;
                break;
            case WIFI_PROV_END:
                ESP_LOGI(TAG, "Provisioning Finished");
                s_is_provisioning = false;
                wifi_prov_mgr_deinit();
                break;
            default:
                break;
        }
    }
}

/************************************************************
                    WIFI INIT
************************************************************/

void wifi_manager_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << WIFI_LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);

    xTaskCreate(led_task, "led_task", 2048, NULL, 1, NULL);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

    wifi_prov_mgr_config_t prov_cfg = {
        .scheme = wifi_prov_scheme_softap,
        .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE
    };
    ESP_ERROR_CHECK(wifi_prov_mgr_init(prov_cfg));

    bool provisioned = false;
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
    ESP_LOGI(TAG, "Provisioned = %d", provisioned);

    if (!provisioned)
    {
        start_provisioning();
    }
    else
    {
        ESP_LOGI(TAG, "Connecting Saved WiFi");
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_start());
        esp_wifi_connect();
    }
}

bool wifi_manager_is_connected(void) { return s_wifi_connected; }
bool wifi_manager_is_provisioning(void) { return s_is_provisioning; }