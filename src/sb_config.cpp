#include "sb_config.hpp"
#include "esp_log.h"
#include "nvs_flash.h"
#include "defconfig.hpp"

const char* sb_config::KEY = "key";
const char* sb_config::LED_PIN = "led_pin";
const char* sb_config::LED_ACTIVE_LOW = "led_act_low";
const char* sb_config::USE_LED_STRIP = "led_str_en";
const char* sb_config::ICMP_PINGER_ENABLE = "icmp_en";
const char* sb_config::ICMP_PINGER_TARGET = "icmp_tgt";
const char* sb_config::SVITLOBOT_API_URL = "sb_url";

static const char *LOG_TAG = "sb_config";
static const char nvs_namespace[] = "sb_cfg";
static nvs_handle_t s_nvs_handle = NULL;

sb_config_t g_sb_config;

esp_err_t sb_config_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGI(LOG_TAG, "New version found, erasing nvs flash...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err == ESP_OK)
    {
        err = nvs_open(nvs_namespace, NVS_READWRITE, &s_nvs_handle);
    }
    memset(&g_sb_config, 0, sizeof(g_sb_config));
    size_t size = sizeof(g_sb_config.svitlobot_key);
    #ifdef SB_SVITLOBOT_KEY
        strncpy(g_sb_config.svitlobot_key, SB_SVITLOBOT_KEY, size);
    #else
        err |= sb_config_get(sb_config::KEY, g_sb_config.svitlobot_key, &size);
    #endif
    size = sizeof(g_sb_config.icmp_pinger_target);
    err |= sb_config_get(sb_config::ICMP_PINGER_TARGET, g_sb_config.icmp_pinger_target, &size);
    err |= sb_config_get(sb_config::ICMP_PINGER_ENABLE, &g_sb_config.icmp_pinger_enable, false);
    size = sizeof(g_sb_config.sb_api_url);
    #ifdef SB_SVITLOBOT_API
        strncpy(g_sb_config.sb_api_url, SB_SVITLOBOT_API, size);
    #else
        err |= sb_config_get(sb_config::SVITLOBOT_API_URL, g_sb_config.sb_api_url, &size);
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            strncpy(g_sb_config.sb_api_url, SB_DEFAULT_SVITLOBOT_API_URL, size);
            err = ESP_OK;
        }
    #endif
    err |= sb_config_get(sb_config::LED_PIN, &g_sb_config.led_pin, 8);
    err |= sb_config_get(sb_config::LED_ACTIVE_LOW, &g_sb_config.led_active_low, true);
    err |= sb_config_get(sb_config::USE_LED_STRIP, &g_sb_config.use_led_strip, false);
    return err;
}

esp_err_t sb_config_get(const char *key, char *value, size_t *size)
{
    return nvs_get_str(s_nvs_handle, key, value, size);
}

esp_err_t sb_config_get(const char *key, bool *value, bool default_value)
{
    esp_err_t err = nvs_get_u8(s_nvs_handle, key, (uint8_t *)value);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        *value = default_value;
    }
    return err;
}

esp_err_t sb_config_get(const char *key, uint8_t *value, uint8_t default_value)
{
    esp_err_t err = nvs_get_u8(s_nvs_handle, key, value);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        *value = default_value;
    }
    return err;
}

esp_err_t sb_config_set(const char *key, const void *value)
{
    return nvs_set_str(s_nvs_handle, key, (const char *)value);
}

esp_err_t sb_config_commit(void)
{
    return nvs_commit(s_nvs_handle);
}

void sb_config_print_all(void)
{
    ESP_LOGI(LOG_TAG, "Current configuration:");
    ESP_LOGI(LOG_TAG, "  Svitlobot Key: %s", g_sb_config.svitlobot_key);
    ESP_LOGI(LOG_TAG, "  ICMP Pinger Target: %s", g_sb_config.icmp_pinger_target);
    ESP_LOGI(LOG_TAG, "  ICMP Pinger Enable: %s", g_sb_config.icmp_pinger_enable ? "true" : "false");
    ESP_LOGI(LOG_TAG, "  Svitlobot API URL: %s", g_sb_config.sb_api_url);
    ESP_LOGI(LOG_TAG, "  LED Pin: %d", g_sb_config.led_pin);
    ESP_LOGI(LOG_TAG, "  LED Active Low: %s", g_sb_config.led_active_low ? "true" : "false");
    ESP_LOGI(LOG_TAG, "  Use LED Strip: %s", g_sb_config.use_led_strip ? "true" : "false");
}