#pragma once
#include "config.hpp"

#ifndef SB_SVITLOBOT_API
    #define SB_SVITLOBOT_API "https://api.svitlobot.in.ua/channelPing?channel_key="
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

#ifndef LED_PIN
    // Valid for ESP32-C3 Super Mini Board, adjust if needed
    #define LED_PIN GPIO_NUM_8
#endif