#pragma once

#include <stdlib.h>
#include "esp_err.h"

namespace md_config {
    extern const char* GROUP;
}

esp_err_t md_config_init(void);
esp_err_t md_config_get(const char *key, char *value, size_t* size);
esp_err_t md_config_set(const char *key, const void *value);
esp_err_t md_config_commit(void);