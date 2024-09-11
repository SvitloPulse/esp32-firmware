#include <math.h>
#include <esp_wifi.h>
#include <esp_timer.h>
#include <esp_log.h>

#include "sb_web_server.hpp"

ESP_EVENT_DEFINE_BASE(SB_STATE_CHANGE_EVENTS);

extern const uint8_t webpage_start[] asm("_binary_webpage_bin_start");
extern const uint8_t webpage_end[] asm("_binary_webpage_bin_end");

static httpd_handle_t server = NULL;
static const char *LOG_TAG = "sb_web_server";

static sb_server_state_t state = {
    .status = "pending",
    .ssid = "",
    .lastPing = 0,
    .temperature = 0,
    .rssi = 0};

char response_buffer[1024];

static esp_err_t index_html_get_handler(httpd_req_t *req)
{
    httpd_resp_set_status(req, "307 Temporary Redirect");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t get_index_handler(httpd_req_t *req)
{
    const size_t response_size = (webpage_end - webpage_start);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    httpd_resp_send(req, (const char *)webpage_start, response_size);
    return ESP_OK;
}

static esp_err_t get_state_handler(httpd_req_t *req)
{
    snprintf(response_buffer, sizeof(response_buffer),
             "{\"status\":\"%s\",\"ssid\":\"%s\",\"lastPing\":%" PRIu64 ",\"temperature\":%lu,\"version\":\"" PROJECT_VER "\", \"rssi\":%d}",
             state.status, state.ssid, state.lastPing, state.temperature, state.rssi);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, (const char *)response_buffer, strlen(response_buffer));
    return ESP_OK;
}

httpd_uri_t uri_index_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = get_index_handler,
    .user_ctx = NULL};

httpd_uri_t uri_index_html_get = {
    .uri = "/index.html",
    .method = HTTP_GET,
    .handler = index_html_get_handler,
    .user_ctx = NULL};

httpd_uri_t uri_status_get = {
    .uri = "/api/state",
    .method = HTTP_GET,
    .handler = get_state_handler,
    .user_ctx = NULL};

static void on_temperature_measured(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data)
{
    state.temperature = (uint32_t)roundf(*(float *)event_data);
}

static void on_ssid_changed(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data)
{
    wifi_config_t config;
    bzero(&config, sizeof(wifi_config_t));
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &config));
    bzero(state.ssid, sizeof(state.ssid));
    strncpy(state.ssid, (const char *)config.sta.ssid, sizeof(state.ssid) - 1);
}

static void on_rssi_measured(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data)
{
    state.rssi = *(int8_t *)event_data;
    ESP_LOGI(LOG_TAG, "RSSI received: %d", *(int8_t *)event_data);
}

void sb_web_server_init(void)
{
    ESP_ERROR_CHECK(esp_event_handler_register(SB_STATE_CHANGE_EVENTS, SB_TEMPERATURE_MEASURED, on_temperature_measured, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(SB_STATE_CHANGE_EVENTS, SB_SSID_CHANGED, on_ssid_changed, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(SB_STATE_CHANGE_EVENTS, SB_RSSI_MEASURED, on_rssi_measured, NULL));
}

httpd_handle_t sb_web_server_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    response_buffer[0] = 0;

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &uri_index_get);
        httpd_register_uri_handler(server, &uri_index_html_get);
        httpd_register_uri_handler(server, &uri_status_get);
    }
    return server;
}

void sb_web_server_stop(void)
{
    if (server)
    {
        httpd_stop(server);
    }
}