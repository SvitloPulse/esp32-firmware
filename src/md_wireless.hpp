#pragma once

#include <stdlib.h>
#include "esp_wifi.h"
#include "esp_err.h"
#include "driver/gpio.h"

struct md_wireless_config_t
{
    int8_t smart_config_key[17];
    uint64_t wifi_connection_timeout_us;
    uint64_t smart_config_timeout_us;
    uint32_t config_blink_interval_ms;
    gpio_num_t led_gpio;
    gpio_num_t sense_gpio;
};

void md_wireless_init(const md_wireless_config_t *config);
esp_err_t md_wireless_ensure_connected(void);
bool md_wireless_has_ssid_pwd_set(void);
void md_wireless_reset_ssid_pwd(void);