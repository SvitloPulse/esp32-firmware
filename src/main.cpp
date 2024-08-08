#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_event.h"
#include "esp_sleep.h"
#include "driver/gpio.h"

#include "config.hpp"
#include "md_config.hpp"
#include "md_wireless.hpp"
#include "md_state_sender.hpp"

#define LED_PIN GPIO_NUM_1
#define SENSE_PIN GPIO_NUM_3

static const char *LOG_TAG = "main";

static const uint64_t WIFI_CONNECTION_TIMEOUT_US = 30000000;  // 30 seconds
static const uint64_t SMART_CONFIG_TIMEOUT_US = 5 * 60000000; // 5 minutes
static const uint32_t CONFIG_BLINK_INTERVAL_MS = 300;
static const uint32_t HTTP_RETRY_INTERVAL_MS = 10000; // 10 seconds
static const uint32_t GO_TO_SLEEP_TIMEOUT_MS = 30000; // 30 seconds
static const uint8_t HTTP_RETRIES_COUNT = 10;
static const uint64_t DEEP_SLEEP_TIMEOUT = 5 * 60 * 1000000; // 5 minutes

static void enter_deep_sleep_wake_on_pin(void)
{
    esp_deepsleep_gpio_wake_up_mode_t mode = gpio_get_level(SENSE_PIN) ? ESP_GPIO_WAKEUP_GPIO_LOW : ESP_GPIO_WAKEUP_GPIO_HIGH;
    ESP_ERROR_CHECK(esp_deep_sleep_enable_gpio_wakeup(1 << SENSE_PIN, mode));
    esp_deep_sleep_start();
}

static void enter_deep_sleep_wake_on_timer(void)
{
    esp_sleep_enable_timer_wakeup(DEEP_SLEEP_TIMEOUT);
    esp_deep_sleep_start();
}

static void app_task(void *param)
{
    static uint8_t retries = 0;
    ESP_LOGI(LOG_TAG, "App task started. Waiting for WiFi connection.");
    while (retries < HTTP_RETRIES_COUNT)
    {
        md_wireless_ensure_connected();
        ESP_LOGI(LOG_TAG, "Trying to send state to BE.");
        bool state = !!gpio_get_level(SENSE_PIN);
        esp_err_t err = md_state_sender_send_state(state);
        if (err != ESP_OK)
        {
            ESP_LOGE(LOG_TAG, "Error sending state: %s", esp_err_to_name(err));
            ESP_LOGI(LOG_TAG, "Retrying in 10 seconds.");
            vTaskDelay(HTTP_RETRY_INTERVAL_MS / portTICK_PERIOD_MS);
            if (state == false)
            {
                // Count retries only if there is no electricity
                retries++;
            }
            continue;
        }
        ESP_LOGI(LOG_TAG, "State sent successfully.");
        break;
    }
    if (retries > 0)
    {
        ESP_LOGW(LOG_TAG, "Failed to send state %d times.Going to sleep for 5 minutes", retries);
        retries = 0;
        enter_deep_sleep_wake_on_timer();
    }
    else
    {
        vTaskDelay(GO_TO_SLEEP_TIMEOUT_MS / portTICK_PERIOD_MS);
    }
    ESP_LOGI(LOG_TAG, "Going to deep sleep.");
    enter_deep_sleep_wake_on_pin();
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

    gpio_config_t sense_io_conf = {
        .pin_bit_mask = (1ULL << SENSE_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };

    ESP_ERROR_CHECK(gpio_config(&sense_io_conf));
    return ESP_OK;
}

static md_wireless_config_t wireless_config = {
    .smart_config_key = {0},
    .wifi_connection_timeout_us = WIFI_CONNECTION_TIMEOUT_US,
    .smart_config_timeout_us = SMART_CONFIG_TIMEOUT_US,
    .config_blink_interval_ms = CONFIG_BLINK_INTERVAL_MS,
    .led_gpio = LED_PIN,
    .sense_gpio = SENSE_PIN,
};

extern "C" void app_main()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(configure_gpio());

    md_config_init();
    int8_t smart_config_key[17] = MD_SMARTCONFIG_KEY;
    bzero(wireless_config.smart_config_key, 17);
    memcpy(wireless_config.smart_config_key, smart_config_key, 16);
    md_wireless_init(&wireless_config);
    md_state_sender_init();

    xTaskCreate(app_task, "app_task", 4096, NULL, 3, NULL);
}