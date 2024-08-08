#include "esp_log.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"

#include "config.hpp"
#include "md_state_sender.hpp"
#include "md_config.hpp"


static const char *TAG = "md_state_sender";
static char buffer[1024];
static char group[32];
static size_t group_size = sizeof(group);

esp_err_t md_state_sender_init(void) {
    group[0] = 0;
    return ESP_OK;
}

esp_err_t md_state_sender_send_state(bool state) {
    esp_http_client_config_t config = {
        .url = "https://" MD_API_ENDPOINT_HOST MD_API_ENDPOINT_PATH,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 30000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    ESP_ERROR_CHECK(md_config_get(md_config::GROUP, group, &group_size));

    buffer[0] = 0;
    sprintf(buffer, "{\"group\": \"%s\", \"state\": \"%s\", \"devId\": \"%s\"}", group, state ? "ON" : "OFF", MD_DEVICE_ID);

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, buffer, strlen(buffer));
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