#include <esp_log.h>
#include <esp_crt_bundle.h>
#include <esp_http_client.h>
#include <esp_event.h>

#include "defconfig.hpp"
#include "sb_state_sender.hpp"
#include "sb_config.hpp"
#include "sb_web_server.hpp"

static const char *TAG = "sb_state_sender";
static char buffer[1024];
static char key[32];
static size_t key_size = sizeof(key);

esp_err_t sb_sender_init(void) {
    key[0] = 0;
    buffer[0] = 0;
#ifdef SB_SVITLOBOT_KEY
    strcpy(key, SB_SVITLOBOT_KEY);
#endif
    return ESP_OK;
}

esp_err_t sb_sender_send_ping(void) {
#ifndef SB_SVITLOBOT_KEY
    ESP_ERROR_CHECK(sb_config_get(sb_config::KEY, key, &key_size));
#endif
    esp_http_client_config_t config = {
        .url = SB_SVITLOBOT_API,
        .user_agent = PROJECT_NAME "/" PROJECT_VER,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 10000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    snprintf(buffer, sizeof(buffer), SB_SVITLOBOT_API "%s", key);
    config.url = buffer;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    ESP_LOGI(TAG, "Sending ping to %s", config.url);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        auto status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %"PRId64,
                status_code,
                esp_http_client_get_content_length(client));
        if (status_code == 200) {
            ESP_LOGI(TAG, "Ping sent successfully.");
            ESP_ERROR_CHECK(esp_event_post(SB_STATE_CHANGE_EVENTS, SB_PING_SENT, NULL, 0, portMAX_DELAY));
        } else {
            ESP_LOGE(TAG, "Error sending ping: %d", status_code);
            ESP_ERROR_CHECK(esp_event_post(SB_STATE_CHANGE_EVENTS, SB_PING_FAILED, NULL, 0, portMAX_DELAY));
            err = ESP_ERR_INVALID_RESPONSE;
        }
    } else {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    return err;
}