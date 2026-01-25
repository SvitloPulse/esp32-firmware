#pragma once

#include <stdlib.h>
#include "esp_err.h"

struct sb_config_t
{
    char svitlobot_key[17];
    char icmp_pinger_target[65];
    bool icmp_pinger_enable;
    char sb_api_url[129];
    uint8_t led_pin;
    bool led_active_low;
    bool use_led_strip;
};

namespace sb_config
{
    extern const char *KEY; // Svitlobot channel key
    extern const char *LED_PIN;
    extern const char *LED_ACTIVE_LOW;
    extern const char *USE_LED_STRIP;
    extern const char *ICMP_PINGER_ENABLE;
    extern const char *ICMP_PINGER_TARGET;
    extern const char *SVITLOBOT_API_URL;
}

extern sb_config_t g_sb_config;

esp_err_t sb_config_init(void);
esp_err_t sb_config_get(const char *key, char *value, size_t *size);
esp_err_t sb_config_get(const char *key, bool *value, bool default_value);
esp_err_t sb_config_get(const char *key, uint8_t *value, uint8_t default_value);

esp_err_t sb_config_set(const char *key, const void *value);
esp_err_t sb_config_commit(void);
void sb_config_print_all(void);