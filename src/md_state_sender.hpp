#pragma once

#include <stdlib.h>
#include "esp_err.h"

esp_err_t md_state_sender_init(void);
esp_err_t md_state_sender_send_state(bool state);