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
#include <mdns.h>
#if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32C6
#include <driver/temperature_sensor.h>
#endif
#include <math.h>
#include <esp_netif_sntp.h>
#include <esp_sntp.h>

#include "defconfig.hpp"
#include "sb_config.hpp"
#include "sb_wireless.hpp"
#include "sb_state_sender.hpp"
#include "sb_web_server.hpp"

#ifdef USE_LED_STRIP
#include "led_strip.h"

static led_strip_handle_t led_strip;
#endif

static const char *LOG_TAG = "main";

#if CONFIG_IDF_TARGET_ESP32
extern "C" {
uint8_t temprature_sens_read();
}
#endif

void sb_led_set_level(uint32_t level)
{
#ifdef USE_LED_STRIP
    if (level == 1) {
        led_strip_set_pixel(led_strip, 0, 0, 0, 16); // Blue to match C3 Supermini
    } else {
        led_strip_clear(led_strip);
    }
    led_strip_refresh(led_strip);
#else
    #ifdef LED_ACTIVE_LOW
        gpio_set_level(LED_PIN, level > 0 ? 0 : 1);
    #else
        gpio_set_level(LED_PIN, level > 0 ? 1 : 0);
    #endif
#endif
}

static const uint64_t WIFI_CONNECTION_TIMEOUT_US = 10000000;  // 10 seconds
static const uint64_t SMART_CONFIG_TIMEOUT_US = 5 * 60000000; // 5 minutes
static const uint32_t CONFIG_BLINK_INTERVAL_MS = 300;
static const uint32_t POWER_ON_BLINK_INTERVAL_MS = 100;
static const uint32_t PING_INTERVAL_MS = 50 * 1000; // 50 seconds

static void app_task(void *param)
{
    ESP_LOGI(LOG_TAG, "App task started. Waiting for WiFi connection.");
    sb_wireless_ensure_connected();
    while(1)
    {
        ESP_LOGI(LOG_TAG, "Sending ping to Svitlobot BE.");
        sb_led_set_level(0);
        sb_wireless_ensure_connected();
        esp_err_t err = sb_sender_send_ping();
        if (err != ESP_OK)
        {
            sb_led_set_level(0);
            ESP_LOGE(LOG_TAG, "Error sending state: %s", esp_err_to_name(err));
        } else {
            sb_led_set_level(1);
            ESP_LOGI(LOG_TAG, "Ping sent successfully.");
        }
        ESP_LOGI(LOG_TAG, "Waiting %lu seconds before next ping...", PING_INTERVAL_MS / 1000);
        vTaskDelay(PING_INTERVAL_MS / portTICK_PERIOD_MS);
    }
}

static void power_on_blink_task(void *param)
{
    for (uint8_t i = 0; i < 5; i++)
    {
        sb_led_set_level(1);
        vTaskDelay(POWER_ON_BLINK_INTERVAL_MS / portTICK_PERIOD_MS);
        sb_led_set_level(0);
        vTaskDelay(POWER_ON_BLINK_INTERVAL_MS / portTICK_PERIOD_MS);
    }
    sb_led_set_level(1);
    vTaskDelete(NULL);
}

static void temperature_sensor_reading_task(void *param)
{
    ESP_LOGI(LOG_TAG, "Enable temperature sensor");
    float tsens_value = 0.0;
    
#if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32C6
    temperature_sensor_handle_t temp_handle = NULL;
    temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(20, 100);
    ESP_ERROR_CHECK(temperature_sensor_install(&temp_sensor_config, &temp_handle));
    ESP_ERROR_CHECK(temperature_sensor_enable(temp_handle));
#endif

    uint8_t log_delay_counter = 0;
    while(1)
    {
#if CONFIG_IDF_TARGET_ESP32
        // temprature_sens_read returns raw value, formula: (raw - 32) / 1.8
        uint8_t raw = temprature_sens_read();
        tsens_value = (float)(raw - 32) / 1.8f;
#elif CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32C6
        ESP_ERROR_CHECK(temperature_sensor_get_celsius(temp_handle, &tsens_value));
#endif

        ESP_ERROR_CHECK(esp_event_post(SB_STATE_CHANGE_EVENTS, SB_TEMPERATURE_MEASURED, &tsens_value, sizeof(tsens_value), portMAX_DELAY));
        if (log_delay_counter % 6 == 0) {
            ESP_LOGI(LOG_TAG, "Temperature value %.02f C", tsens_value);
            log_delay_counter = 0;
        }
        log_delay_counter++;
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

static void rssi_measurement_task(void *param)
{
    uint8_t log_delay_counter = 0;
    while(1)
    {
        sb_wireless_ensure_connected();
        wifi_ap_record_t ap_info;
        ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&ap_info));
        if (log_delay_counter % 6 == 0)
        {
            ESP_LOGI(LOG_TAG, "SSID: %s, RSSI: %d", ap_info.ssid, ap_info.rssi);
            log_delay_counter = 0;
        }
        log_delay_counter++;
        ESP_ERROR_CHECK(esp_event_post(SB_STATE_CHANGE_EVENTS, SB_RSSI_MEASURED, &ap_info.rssi, sizeof(ap_info.rssi), portMAX_DELAY));
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

#ifdef USE_LED_STRIP
static void configure_led_strip(void)
{
    /* LED strip initialization */
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_PIN,
        .max_leds = 1, // at least one LED on board
    };
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags = {
            .with_dma = false,
        }
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    led_strip_clear(led_strip);
}
#endif

static esp_err_t configure_gpio(void)
{
    gpio_config_t led_io_conf = {};
    led_io_conf.intr_type = GPIO_INTR_DISABLE;
    led_io_conf.mode = GPIO_MODE_OUTPUT;
    led_io_conf.pin_bit_mask = (1ULL << LED_PIN);
    led_io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    led_io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

    ESP_ERROR_CHECK(gpio_config(&led_io_conf));
    sb_led_set_level(0); // Initialize to OFF
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
#ifdef USE_LED_STRIP
    configure_led_strip();
#else
    ESP_ERROR_CHECK(configure_gpio());
#endif

    esp_sntp_config_t sntp_config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    ESP_ERROR_CHECK(esp_netif_sntp_init(&sntp_config));

    // Initialize mDNS
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set(PROJECT_NAME_LOWER));
    ESP_ERROR_CHECK(mdns_instance_name_set(PROJECT_NAME " " PROJECT_VER));
    ESP_ERROR_CHECK(mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0));
    ESP_ERROR_CHECK(mdns_service_instance_name_set("_http", "_tcp", PROJECT_NAME " " PROJECT_VER " Web Server"));

    xTaskCreate(power_on_blink_task, "power_on_blink", 4096, NULL, 3, NULL);

    sb_config_init();
    sb_web_server_init();

#ifdef SB_SMARTCONFIG_KEY
    int8_t smart_config_key[17] = SB_SMARTCONFIG_KEY;
    bzero(wireless_config.smart_config_key, 17);
    memcpy(wireless_config.smart_config_key, smart_config_key, 16);
#endif

    sb_wireless_init(&wireless_config);
    sb_sender_init();

    xTaskCreate(app_task, "app_task", 4096, NULL, 3, NULL);
    xTaskCreate(temperature_sensor_reading_task, "temperature", 4096, NULL, 3, NULL);
    xTaskCreate(rssi_measurement_task, "rssi", 4096, NULL, 3, NULL);

    // Start web server
    sb_wireless_ensure_connected();
    sb_web_server_start();
}
