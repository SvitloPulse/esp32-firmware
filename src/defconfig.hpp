#pragma once

#include <stdint.h>
#include "config.hpp"

#ifndef SB_DEFAULT_SVITLOBOT_API_URL
    #define SB_DEFAULT_SVITLOBOT_API_URL "https://api.svitlobot.in.ua/channelPing?channel_key="
#endif

#if 0
    #ifndef SB_SVITLOBOT_API
        #define SB_SVITLOBOT_API "https://api.svitlobot.in.ua/channelPing?channel_key="
    #endif
#endif

// Replace 0 with 1 to enable hardcoded Svitlobot key
#if 0
    #define SB_SVITLOBOT_KEY "XXXX" // put your key here instead of XXXX
#endif

// Replace 0 below with 1 to hardcode wifi credentials
// When WiFi credentials are hardcoded, the device will not enter SmartConfig mode
#if 0
    #define SB_WIFI_SSID "ssid"    // put your ssid here
    #define SB_WIFI_PWD "pwd"      // put your password here
#endif

// Replace 0 below with 1 to hardcode ping address
#if 0
    #define SB_PING_TARGET "192.168.0.1"
#endif

#if 0
    #ifndef LED_PIN
        #if BOARD_ESP32C3_SUPERMINI
            #define LED_PIN GPIO_NUM_8
            #define LED_ACTIVE_LOW 0
        #elif BOARD_ESP32C6_ZERO
            #define LED_PIN GPIO_NUM_8
            #define USE_LED_STRIP 1
        #elif CONFIG_IDF_TARGET_ESP32
            #define LED_PIN GPIO_NUM_10
            #define LED_ACTIVE_LOW 1
        #endif 
    #endif
#endif

void sb_led_set_level(uint32_t level);