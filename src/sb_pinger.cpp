#include "sb_pinger.hpp"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <lwip/inet.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include <ping/ping_sock.h>
#include <string.h>

static const char *TAG = "sb_pinger";

struct ping_result_t {
  bool success;
  SemaphoreHandle_t done_sem;
};

bool sb_pinger_check(const char *target_ip) {
  esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
  ip_addr_t target_addr;
  memset(&target_addr, 0, sizeof(target_addr));

  if (ipaddr_aton(target_ip, &target_addr) == 0) {
    ESP_LOGE(TAG, "Invalid IP address: %s", target_ip);
    return false;
  }

  ping_config.target_addr = target_addr;
  ping_config.count = 3; // Try 3 pings for better reliability
  ping_config.timeout_ms = 1000;

  ping_result_t res = {
      .success = false,
      .done_sem = xSemaphoreCreateBinary(),
  };

  esp_ping_callbacks_t cbs = {0};
  cbs.cb_args = &res;
  cbs.on_ping_success = [](esp_ping_handle_t hdl, void *args) {
    ping_result_t *r = (ping_result_t *)args;
    r->success = true;
  };
  cbs.on_ping_end = [](esp_ping_handle_t hdl, void *args) {
    ping_result_t *r = (ping_result_t *)args;
    xSemaphoreGive(r->done_sem);
  };

  esp_ping_handle_t ping;
  esp_err_t err = esp_ping_new_session(&ping_config, &cbs, &ping);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to create ping session: %s", esp_err_to_name(err));
    vSemaphoreDelete(res.done_sem);
    return false;
  }

  esp_ping_start(ping);

  // Wait for all pings to finish (max 5 seconds)
  if (xSemaphoreTake(res.done_sem, pdMS_TO_TICKS(5000)) == pdFALSE) {
    ESP_LOGE(TAG, "Ping timed out");
  }

  esp_ping_stop(ping);
  esp_ping_delete_session(ping);
  vSemaphoreDelete(res.done_sem);

  if (res.success) {
    ESP_LOGI(TAG, "Ping to %s success.", target_ip);
  } else {
    ESP_LOGW(TAG, "Ping to %s failed.", target_ip);
  }

  return res.success;
}
