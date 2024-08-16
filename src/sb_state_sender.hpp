#pragma once

#include <stdlib.h>
#include "esp_err.h"

esp_err_t sb_sender_init(void);
esp_err_t sb_sender_send_ping(void);