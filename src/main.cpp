#include <string.h>
#include <stdlib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <esp_netif.h>
#include <esp_event.h>
#include <driver/gpio.h>

#include "defconfig.hpp"
#include "sb_config.hpp"
#include "sb_wireless.hpp"
#include "sb_state_sender.hpp"

static const char *LOG_TAG = "main";

static const uint64_t WIFI_CONNECTION_TIMEOUT_US = 10000000;  // 10 seconds
static const uint64_t SMART_CONFIG_TIMEOUT_US = 5 * 60000000; // 5 minutes
static const uint32_t CONFIG_BLINK_INTERVAL_MS = 300;
static const uint32_t POWER_ON_BLINK_INTERVAL_MS = 100;
static const uint32_t PING_INTERVAL_MS = 60 * 1000; // 1 minute

static void app_task(void *param)
{
    ESP_LOGI(LOG_TAG, "App task started. Waiting for WiFi connection.");
    sb_wireless_ensure_connected();
    while(1)
    {
        ESP_LOGI(LOG_TAG, "Waiting %lu seconds before next ping...", PING_INTERVAL_MS / 1000);
        vTaskDelay(PING_INTERVAL_MS / portTICK_PERIOD_MS);
        ESP_LOGI(LOG_TAG, "Sending ping to Svitlobot BE.");
        gpio_set_level(LED_PIN, 1);
        sb_wireless_ensure_connected();
        esp_err_t err = sb_sender_send_ping();
        if (err != ESP_OK)
        {
            gpio_set_level(LED_PIN, 1);
            ESP_LOGE(LOG_TAG, "Error sending state: %s", esp_err_to_name(err));
        } else {
            gpio_set_level(LED_PIN, 0);
            ESP_LOGI(LOG_TAG, "Ping sent successfully.");
        }
    }
}

static void power_on_blink_task(void *param)
{
    for (uint8_t i = 0; i < 5; i++)
    {
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(POWER_ON_BLINK_INTERVAL_MS / portTICK_PERIOD_MS);
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(POWER_ON_BLINK_INTERVAL_MS / portTICK_PERIOD_MS);
    }
    gpio_set_level(LED_PIN, 1);
    vTaskDelete(NULL);
}

static esp_err_t configure_gpio(void)
{
    gpio_config_t led_io_conf = {};
    led_io_conf.intr_type = GPIO_INTR_DISABLE;
    led_io_conf.mode = GPIO_MODE_OUTPUT;
    led_io_conf.pin_bit_mask = (1ULL << LED_PIN);
    led_io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    led_io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

    ESP_ERROR_CHECK(gpio_config(&led_io_conf));
    return ESP_OK;
}

static sb_wireless_config_t wireless_config = {
    .smart_config_key = {0},
    .wifi_connection_timeout_us = WIFI_CONNECTION_TIMEOUT_US,
    .smart_config_timeout_us = SMART_CONFIG_TIMEOUT_US,
    .config_blink_interval_ms = CONFIG_BLINK_INTERVAL_MS,
    .led_gpio = LED_PIN,
};

extern "C" void app_main()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(configure_gpio());
    gpio_set_level(LED_PIN, 1);

    xTaskCreate(power_on_blink_task, "power_on_blink", 4096, NULL, 3, NULL);

    sb_config_init();

#ifdef SB_SMARTCONFIG_KEY
    int8_t smart_config_key[17] = SB_SMARTCONFIG_KEY;
    bzero(wireless_config.smart_config_key, 17);
    memcpy(wireless_config.smart_config_key, smart_config_key, 16);
#endif

    sb_wireless_init(&wireless_config);
    sb_sender_init();

    xTaskCreate(app_task, "app_task", 4096, NULL, 3, NULL);
}