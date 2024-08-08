#include "md_config.hpp"
#include "esp_log.h"
#include "nvs_flash.h"

const char* md_config::GROUP = "group";
static const char *LOG_TAG = "md_config";
static const char nvs_namespace[] = "md_cfg";
static nvs_handle_t s_nvs_handle = NULL;

esp_err_t md_config_init(void)
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
    return err;
}

esp_err_t md_config_get(const char *key, char *value, size_t *size)
{
    return nvs_get_str(s_nvs_handle, key, value, size);
}

esp_err_t md_config_set(const char *key, const void *value)
{
    return nvs_set_str(s_nvs_handle, key, (const char *)value);
}

esp_err_t md_config_commit(void)
{
    return nvs_commit(s_nvs_handle);
}