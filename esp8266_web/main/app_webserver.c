#include "sdkconfig.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lwip/apps/netbiosns.h"
#include <fcntl.h>
#include <string.h>

const char* TAG = "app_webserver";

esp_err_t start_rest_server(const char *base_path);

esp_err_t init_fs(void)
{
  esp_vfs_spiffs_conf_t conf = {
    .base_path = "/www",
    .partition_label = NULL,
    .max_files = 5,
    .format_if_mount_failed = true
  };
  esp_err_t ret = esp_vfs_spiffs_register(&conf);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(TAG, "Failed to mount or format filesystem");
    } else if (ret == ESP_ERR_NOT_FOUND) {
      ESP_LOGE(TAG, "Failed to find SPIFFS partition");
    } else {
      ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
    }
    return ESP_FAIL;
  }

  size_t total = 0, used = 0;
  ret = esp_spiffs_info(NULL, &total, &used);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
  } else {
    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
  }
  return ESP_OK;
}

void
app_webserver_init(void)
{
  ESP_ERROR_CHECK(init_fs());
  ESP_ERROR_CHECK(start_rest_server("/www"));
}
