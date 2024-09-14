#pragma once

#include <stdint.h>
#include <time.h>
#include <esp_http_server.h>
#include <esp_event.h>

struct sb_server_state_t {
    char status[16];
    char ssid[32];
    uint64_t lastPing;
    uint32_t temperature;
    int8_t rssi;
    uint8_t pings_failed;
};

ESP_EVENT_DECLARE_BASE(SB_STATE_CHANGE_EVENTS);

enum sb_state_change_event_id_t {
    SB_SSID_CHANGED,
    SB_PING_SENT,
    SB_PING_FAILED,
    SB_TEMPERATURE_MEASURED,
    SB_RSSI_MEASURED,
};

void sb_web_server_init(void);
httpd_handle_t sb_web_server_start(void);
void sb_web_server_stop(void);