#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "rom/ets_sys.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#define IMU_WIFI_AP_SSID            "imu"
#define IMU_WIFI_AP_PASSWORD        "password"
#define IMU_WIFI_AP_MAX_CONN        4

#define IMU_WIFI_STA_SSID           "hkim-rnd"
#define IMU_WIFI_STA_PASSWORD       "hkrnd1234"

const int WIFI_CONNECTED_BIT = BIT0;

static const char *TAG = "app_wifi";

static esp_err_t
event_handler(void *ctx, system_event_t *event)
{
  /* For accessing reason codes in case of disconnection */
  system_event_info_t *info = &event->event_info;

  switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
      esp_wifi_connect();
      break;

    case SYSTEM_EVENT_STA_GOT_IP:
      ESP_LOGI(TAG, "got ip:%s",
          ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
      break;

    case SYSTEM_EVENT_AP_STACONNECTED:
      ESP_LOGI(TAG, "station:"MACSTR" join, AID=%d",
          MAC2STR(event->event_info.sta_connected.mac),
          event->event_info.sta_connected.aid);
      break;

    case SYSTEM_EVENT_AP_STADISCONNECTED:
      ESP_LOGI(TAG, "station:"MACSTR"leave, AID=%d",
          MAC2STR(event->event_info.sta_disconnected.mac),
          event->event_info.sta_disconnected.aid);
      break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
      ESP_LOGE(TAG, "Disconnect reason : %d", info->disconnected.reason);
      if (info->disconnected.reason == WIFI_REASON_BASIC_RATE_NOT_SUPPORT) {
        /*Switch to 802.11 bgn mode */
        esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
      }
      esp_wifi_connect();
      break;

    case SYSTEM_EVENT_AP_STAIPASSIGNED:
      ESP_LOGI(TAG, "assigned ip:%s",
          ip4addr_ntoa(&event->event_info.ap_staipassigned.ip));
      break;

    default:
      break;
  }
  return ESP_OK;
}

static void
wifi_init_sta_softap(void)
{
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  wifi_config_t ap_config = {
    .ap = {
      .ssid = IMU_WIFI_AP_SSID,
      .ssid_len = strlen(IMU_WIFI_AP_SSID),
      .password = IMU_WIFI_AP_PASSWORD,
      .max_connection = IMU_WIFI_AP_MAX_CONN,
      .authmode = WIFI_AUTH_WPA_WPA2_PSK
    },
  };

  if (strlen(IMU_WIFI_AP_PASSWORD) == 0) {
    ap_config.ap.authmode = WIFI_AUTH_OPEN;
  }

  wifi_config_t sta_config = {
    .sta = {
      .ssid = IMU_WIFI_STA_SSID,
      .password = IMU_WIFI_STA_PASSWORD
    },
  };

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_config));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));
  ESP_ERROR_CHECK(esp_wifi_start());
}

void
app_wifi_init(void)
{
  tcpip_adapter_init();
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

  wifi_init_sta_softap();
}
