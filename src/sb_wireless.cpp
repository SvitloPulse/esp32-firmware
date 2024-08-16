#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <esp_netif.h>
#include <esp_smartconfig.h>
#include <esp_mac.h>
#include <esp_timer.h>
#include <esp_sleep.h>

#include "sb_wireless.hpp"
#include "defconfig.hpp"
#include "sb_config.hpp"

static const char *LOG_TAG = "sb_wireless";

static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;
static const int WIFI_STA_STARTED_BIT = BIT2;

static EventGroupHandle_t s_wifi_event_group;
static esp_timer_handle_t s_wifi_connection_timeout_timer;
static esp_timer_handle_t s_smart_config_timeout_timer;
static TaskHandle_t blinker_task_handle = NULL;
static const int MAX_CONNECTION_RETRIES = 10;
static int connection_retries = 0;
static const uint64_t DEEP_SLEEP_TIMEOUT = 5 * 60 * 1000000; // 5 minutes

#ifndef SB_WIFI_SSID
    static void _smart_config_task(void *param);
#endif

static void _handle_wifi_events(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    sb_wireless_config_t *config = (sb_wireless_config_t *)arg;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(LOG_TAG, "ESP32 station mode started");
        xEventGroupSetBits(s_wifi_event_group, WIFI_STA_STARTED_BIT);
#ifdef SB_WIFI_SSID
        wifi_config_t config;
        bzero(&config, sizeof(wifi_config_t));
        ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &config));
        ESP_LOGI(LOG_TAG, "Setting hardcoded SSID and password");
        strcpy((char *)config.sta.ssid, SB_WIFI_SSID);
        strcpy((char *)config.sta.password, SB_WIFI_PWD);
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));
#endif
#ifndef SB_WIFI_SSID
        if (!sb_wireless_has_ssid_pwd_set())
        {
            ESP_LOGW(LOG_TAG, "No SSID and password set. Starting smart config.");
            xTaskCreate(_smart_config_task, "smart_config_task", 4096, arg, 3, NULL);
        } else {
#endif
            ESP_LOGI(LOG_TAG, "SSID and password set. Trying to connect...");
#ifndef SB_WIFI_SSID
            wifi_config_t config;
            bzero(&config, sizeof(wifi_config_t));
            ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &config));
            ESP_LOGI(LOG_TAG, "SSID: %s", (char *)config.sta.ssid);
            ESP_LOGI(LOG_TAG, "PWD: %s", (char *)config.sta.password);
#endif
            ESP_ERROR_CHECK(esp_wifi_connect());
#ifndef SB_WIFI_SSID
        }
#endif
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(LOG_TAG, "Disconnected from AP. Trying to reconnect...");
        xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT | WIFI_STA_STARTED_BIT);
        if (connection_retries >= MAX_CONNECTION_RETRIES)
        {
#ifndef SB_WIFI_SSID
            ESP_LOGE(LOG_TAG, "Max connection retries reached. Start Smart Config.");
            xEventGroupSetBits(s_wifi_event_group, WIFI_STA_STARTED_BIT);
            xTaskCreate(_smart_config_task, "smart_config_task", 4096, arg, 3, NULL);
#else
            ESP_LOGE(LOG_TAG, "Max connection retries reached. Rebooting...");
            esp_restart();
#endif
        } else {
#ifndef SB_WIFI_SSID
            wifi_config_t config;
            bzero(&config, sizeof(wifi_config_t));
            ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &config));
            ESP_LOGI(LOG_TAG, "SSID: %s", (char *)config.sta.ssid);
            ESP_LOGI(LOG_TAG, "PWD: %s", (char *)config.sta.password);
            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));
#endif
            esp_wifi_connect();
            connection_retries++;
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ESP_LOGI(LOG_TAG, "Connected to AP and got IP address");
        xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
        connection_retries = 0;
    }
}

#ifndef SB_WIFI_SSID
static void _handle_smart_config_events(void *arg, esp_event_base_t event_base,
                                       int32_t event_id, void *event_data)
{
    if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD)
    {
        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        if (evt->type != SC_TYPE_ESPTOUCH_V2)
        {
            ESP_LOGW(LOG_TAG, "Smart Config type is not supported. Ignoring.");
            return;
        }

        ESP_LOGI(LOG_TAG, "Got SSID and password");

        wifi_config_t wifi_config;
        uint8_t ssid[33] = {0};
        uint8_t password[65] = {0};
        uint8_t rvd_data[33] = {0};

        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));

        memcpy(ssid, evt->ssid, sizeof(evt->ssid));
        memcpy(password, evt->password, sizeof(evt->password));
        ESP_LOGI(LOG_TAG, "SSID:%s", ssid);
        ESP_LOGI(LOG_TAG, "PASSWORD:%s", "******");
        ESP_ERROR_CHECK(esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)));
        sb_config_set(sb_config::KEY, rvd_data);
        sb_config_commit();
        ESP_LOGI(LOG_TAG, "RVD_DATA: %s", rvd_data);
        ESP_ERROR_CHECK(esp_wifi_disconnect());
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        esp_wifi_connect();
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE)
    {
        ESP_LOGI(LOG_TAG, "Scan done");
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL)
    {
        ESP_LOGI(LOG_TAG, "Found channel");
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE)
    {
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
    }
}

static void _blinker_task(void *param)
{
    const sb_wireless_config_t *config = (sb_wireless_config_t *)param;
    for (;;)
    {
        gpio_set_level(config->led_gpio, 0);
        vTaskDelay(config->config_blink_interval_ms / portTICK_PERIOD_MS);
        gpio_set_level(config->led_gpio, 1);
        vTaskDelay(config->config_blink_interval_ms / portTICK_PERIOD_MS);
    }
}

static void _smart_config_task(void *param)
{
    const sb_wireless_config_t *config = (sb_wireless_config_t *)param;
    esp_event_handler_instance_t smart_config_handler = NULL;
    ESP_LOGI(LOG_TAG, "Starting Smart Config Task");
    ESP_ERROR_CHECK(esp_event_handler_instance_register(SC_EVENT, ESP_EVENT_ANY_ID, &_handle_smart_config_events, param, &smart_config_handler));
    ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_V2));
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();

    if (strlen((char *)config->smart_config_key) > 0) {
        cfg.esp_touch_v2_enable_crypt = true;
        cfg.esp_touch_v2_key = (char *)config->smart_config_key;
    } else {
        cfg.esp_touch_v2_enable_crypt = false;
    }
    
    const esp_timer_create_args_t timeout_args = {
        .callback = [](void *arg)
        {
            const sb_wireless_config_t *config = (sb_wireless_config_t *)arg;
            ESP_LOGW(LOG_TAG, "Smart Config Timeout. Rebooting...");
            esp_restart();
        },
        .arg = (void *)config,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "smart_config_timeout_timer",
        .skip_unhandled_events = true,
    };
    ESP_ERROR_CHECK(esp_timer_create(&timeout_args, &s_smart_config_timeout_timer));

    for (;;)
    {
        EventBits_t uxBits = xEventGroupWaitBits(s_wifi_event_group, WIFI_STA_STARTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
        if (uxBits & WIFI_STA_STARTED_BIT)
        {
            ESP_LOGI(LOG_TAG, "WiFi station started. Starting smart config.");
            ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));
            xTaskCreate(_blinker_task, "blinker_task", 1024, param, 3, &blinker_task_handle);
            ESP_ERROR_CHECK(esp_timer_start_once(s_smart_config_timeout_timer, config->smart_config_timeout_us));
        }
        if (uxBits & ESPTOUCH_DONE_BIT)
        {
            ESP_LOGI(LOG_TAG, "Smart config finished. Stopping smart config.");
            esp_smartconfig_stop();
            esp_timer_stop(s_smart_config_timeout_timer);
            s_smart_config_timeout_timer = NULL;
            vTaskDelete(blinker_task_handle);
            blinker_task_handle = NULL;
            gpio_set_level(config->led_gpio, 1);
            break;
        }
    }
    esp_event_handler_instance_unregister(SC_EVENT, ESP_EVENT_ANY_ID, smart_config_handler);
    ESP_LOGI(LOG_TAG, "Smart Config Task finished");
}
#endif

void sb_wireless_init(const sb_wireless_config_t *config) {
#ifndef SB_WIFI_SSID
    const esp_timer_create_args_t args = {
        .callback = [](void *arg)
        {
            xTaskCreate(_smart_config_task, "smart_config_task", 4096, arg, 3, NULL);
        },
        .arg = (void *)config,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "wifi_connection_timeout_timer",
        .skip_unhandled_events = true,
    };
    ESP_ERROR_CHECK(esp_timer_create(&args, &s_wifi_connection_timeout_timer));
#endif
    s_wifi_event_group = xEventGroupCreate();
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    // Persist the WiFi configuration
    cfg.nvs_enable = true;
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &_handle_wifi_events, (void *)config, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &_handle_wifi_events, (void *)config, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

esp_err_t sb_wireless_ensure_connected(void) {
    for (;;)
    {
        EventBits_t uxBits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
        if (uxBits & CONNECTED_BIT)
        {
            return ESP_OK;
        }
    }
}

bool sb_wireless_has_ssid_pwd_set(void) {
    wifi_config_t config;
    bzero(&config, sizeof(wifi_config_t));
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &config));
    return strlen((char *)config.sta.ssid) > 0 && strlen((char *)config.sta.password) > 0;
}

void sb_wireless_reset_ssid_pwd(void) {
    wifi_config_t config;
    bzero(&config, sizeof(wifi_config_t));
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &config));
    config.sta.ssid[0] = '\0';
    config.sta.password[0] = '\0';
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));
}