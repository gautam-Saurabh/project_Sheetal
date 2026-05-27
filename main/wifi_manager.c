#include "wifi_manager.h"

#include <stdbool.h>

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

/************************************************************
                        DEFINES
************************************************************/

#define WIFI_LED_PIN      GPIO_NUM_2

#define MAX_WIFI_RETRY    5

/************************************************************
                        GLOBALS
************************************************************/

static const char *TAG = "WIFI";

static bool s_wifi_connected = false;

static bool s_is_provisioning = false;

static int s_retry_count = 0;

/************************************************************
                        LED TASK
************************************************************/

static void led_task(void *pv)
{
    while (1)
    {
        /**************************************************
                    WIFI CONNECTED
        **************************************************/

        if (s_wifi_connected)
        {
            gpio_set_level(
                WIFI_LED_PIN,
                0
            );

            vTaskDelay(
                pdMS_TO_TICKS(1000)
            );
        }

        /**************************************************
                    PROVISIONING
        **************************************************/

        else if (s_is_provisioning)
        {
            gpio_set_level(
                WIFI_LED_PIN,
                0
            );

            vTaskDelay(
                pdMS_TO_TICKS(150)
            );

            gpio_set_level(
                WIFI_LED_PIN,
                1
            );

            vTaskDelay(
                pdMS_TO_TICKS(150)
            );
        }

        /**************************************************
                    DISCONNECTED
        **************************************************/

        else
        {
            gpio_set_level(
                WIFI_LED_PIN,
                1
            );

            vTaskDelay(
                pdMS_TO_TICKS(1000)
            );
        }
    }
}

/************************************************************
                START SOFTAP PROVISIONING
************************************************************/

static void start_provisioning(void)
{
    s_is_provisioning = true;

    wifi_prov_security_t security =
        WIFI_PROV_SECURITY_1;

    const char *pop =
        "abcd1234";

    const char *service_name =
        "TEC_JACKET_SETUP";

    const char *service_key =
        "12345678";

    ESP_LOGI(TAG, "================================");
    ESP_LOGI(TAG, "SOFTAP PROVISIONING STARTED");
    ESP_LOGI(TAG, "AP NAME : %s", service_name);
    ESP_LOGI(TAG, "AP PASS : %s", service_key);
    ESP_LOGI(TAG, "POP     : %s", pop);
    ESP_LOGI(TAG, "================================");

    ESP_ERROR_CHECK(
        esp_wifi_set_mode(
            WIFI_MODE_APSTA
        )
    );

    ESP_ERROR_CHECK(
        esp_wifi_start()
    );

    vTaskDelay(
        pdMS_TO_TICKS(2000)
    );

    ESP_ERROR_CHECK(
        wifi_prov_mgr_start_provisioning(
            security,
            pop,
            service_name,
            service_key
        )
    );
}

/************************************************************
                    EVENT HANDLER
************************************************************/

static void event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
    /******************************************************
                        WIFI EVENTS
    ******************************************************/

    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
            case WIFI_EVENT_STA_START:

                ESP_LOGI(
                    TAG,
                    "STA Started"
                );

                break;

            case WIFI_EVENT_STA_DISCONNECTED:

                s_wifi_connected = false;

                /******************************************
                    IGNORE RETRIES DURING PROVISION
                ******************************************/

                if (s_is_provisioning)
                {
                    break;
                }

                s_retry_count++;

                ESP_LOGW(
                    TAG,
                    "Reconnect Retry %d/%d",
                    s_retry_count,
                    MAX_WIFI_RETRY
                );

                /******************************************
                        RECONNECT
                ******************************************/

                if (s_retry_count < MAX_WIFI_RETRY)
                {
                    esp_wifi_connect();
                }

                /******************************************
                    RESET PROVISIONING
                ******************************************/

                else
                {
                    ESP_LOGE(
                        TAG,
                        "WiFi Failed"
                    );

                    ESP_LOGE(
                        TAG,
                        "Reset Provisioning"
                    );

                    wifi_prov_mgr_reset_provisioning();

                    vTaskDelay(
                        pdMS_TO_TICKS(2000)
                    );

                    esp_restart();
                }

                break;

            default:
                break;
        }
    }

    /******************************************************
                        IP EVENT
    ******************************************************/

    else if (event_base == IP_EVENT &&
             event_id == IP_EVENT_STA_GOT_IP)
    {
        s_wifi_connected = true;

        s_is_provisioning = false;

        s_retry_count = 0;

        ESP_LOGI(
            TAG,
            "WiFi Connected"
        );
    }

    /******************************************************
                    PROVISION EVENTS
    ******************************************************/

    else if (event_base == WIFI_PROV_EVENT)
    {
        switch (event_id)
        {
            case WIFI_PROV_START:

                ESP_LOGI(
                    TAG,
                    "Provisioning Started"
                );

                break;

            case WIFI_PROV_CRED_RECV:

                ESP_LOGI(
                    TAG,
                    "Credentials Received"
                );

                break;

            case WIFI_PROV_CRED_SUCCESS:

                ESP_LOGI(
                    TAG,
                    "Provisioning Success"
                );

                esp_wifi_connect();

                break;

            case WIFI_PROV_CRED_FAIL:

                ESP_LOGE(
                    TAG,
                    "Provisioning Failed"
                );

                break;

            case WIFI_PROV_END:

                ESP_LOGI(
                    TAG,
                    "Provisioning Finished"
                );

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
    /******************************************************
                        NVS
    ******************************************************/

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

    /******************************************************
                        LED
    ******************************************************/

    gpio_config_t io_conf = {

        .pin_bit_mask =
            (1ULL << WIFI_LED_PIN),

        .mode =
            GPIO_MODE_OUTPUT,
    };

    gpio_config(&io_conf);

    xTaskCreate(
        led_task,
        "led_task",
        2048,
        NULL,
        1,
        NULL
    );

    /******************************************************
                    NETWORK STACK
    ******************************************************/

    ESP_ERROR_CHECK(
        esp_netif_init()
    );

    ESP_ERROR_CHECK(
        esp_event_loop_create_default()
    );

    esp_netif_create_default_wifi_ap();

    esp_netif_create_default_wifi_sta();

    /******************************************************
                        WIFI INIT
    ******************************************************/

    wifi_init_config_t cfg =
        WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(
        esp_wifi_init(&cfg)
    );

    ESP_ERROR_CHECK(
        esp_wifi_set_storage(
            WIFI_STORAGE_FLASH
        )
    );

    /******************************************************
                    EVENT HANDLERS
    ******************************************************/

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &event_handler,
            NULL
        )
    );

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &event_handler,
            NULL
        )
    );

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            WIFI_PROV_EVENT,
            ESP_EVENT_ANY_ID,
            &event_handler,
            NULL
        )
    );

    /******************************************************
                PROVISION MANAGER
    ******************************************************/

    wifi_prov_mgr_config_t prov_cfg = {

        .scheme = wifi_prov_scheme_softap,

        .scheme_event_handler =
            WIFI_PROV_EVENT_HANDLER_NONE
    };

    ESP_ERROR_CHECK(
        wifi_prov_mgr_init(
            prov_cfg
        )
    );

    /******************************************************
                CHECK PROVISIONED
    ******************************************************/

    bool provisioned = false;

    ESP_ERROR_CHECK(
        wifi_prov_mgr_is_provisioned(
            &provisioned
        )
    );

    ESP_LOGI(
        TAG,
        "Provisioned = %d",
        provisioned
    );

    /******************************************************
                    START FLOW
    ******************************************************/

    if (!provisioned)
    {
        start_provisioning();
    }
    else
    {
        ESP_LOGI(
            TAG,
            "Connecting Saved WiFi"
        );

        ESP_ERROR_CHECK(
            esp_wifi_set_mode(
                WIFI_MODE_STA
            )
        );

        ESP_ERROR_CHECK(
            esp_wifi_start()
        );

        esp_wifi_connect();
    }
}

/************************************************************
                WIFI CONNECT STATUS
************************************************************/

bool wifi_manager_is_connected(void)
{
    return s_wifi_connected;
}

/************************************************************
                PROVISION STATUS
************************************************************/

bool wifi_manager_is_provisioning(void)
{
    return s_is_provisioning;
}

/************************************************************
            RESET WIFI PROVISIONING
************************************************************/

void wifi_manager_reset_provisioning(void)
{
    ESP_LOGW(
        TAG,
        "Factory Reset WiFi"
    );

    wifi_prov_mgr_reset_provisioning();

    vTaskDelay(
        pdMS_TO_TICKS(1000)
    );

    esp_restart();
}