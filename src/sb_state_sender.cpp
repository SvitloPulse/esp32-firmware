#include <esp_log.h>
#include <esp_crt_bundle.h>
#include <esp_http_client.h>

#include "defconfig.hpp"
#include "sb_state_sender.hpp"
#include "sb_config.hpp"

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
        .method = HTTP_METHOD_GET,
        .timeout_ms = 30000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    snprintf(buffer, sizeof(buffer), SB_SVITLOBOT_API "%s", key);
    config.url = buffer;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    ESP_LOGI(TAG, "Sending ping to %s", config.url);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %"PRId64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    return err;
}