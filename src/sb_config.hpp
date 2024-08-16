#pragma once

#include <stdlib.h>
#include "esp_err.h"

namespace sb_config {
    extern const char* KEY;
}

esp_err_t sb_config_init(void);
esp_err_t sb_config_get(const char *key, char *value, size_t* size);
esp_err_t sb_config_set(const char *key, const void *value);
esp_err_t sb_config_commit(void);