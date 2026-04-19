#include "wifi_app.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "lcd.h"
#include <string.h>
#include <stdio.h>

#define DEFAULT_SSID        "password"
#define DEFAULT_PWD         "asd12345"
#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1

static EventGroupHandle_t   wifi_event;
static const char *TAG = "WIFI_APP";
static bool s_wifi_connected = false;
static bool s_handlers_registered = false;
static bool s_wifi_driver_inited = false;
static esp_netif_t *s_sta_netif = NULL;

bool wifi_is_connected(void) { return s_wifi_connected; }

static void connet_display(uint8_t flag) {
    /* LCD在LVGL启动前未初始化，WiFi状态由日志输出，不在屏幕显示 */
    (void)flag;
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    static int s_retry_num = 0;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        connet_display(0);
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        connet_display(2);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        s_wifi_connected = false;
        wifi_event_sta_disconnected_t *disc = (wifi_event_sta_disconnected_t *)event_data;
        ESP_LOGW(TAG, "Disconnected, reason=%d (2=auth fail, 15=4-way handshake, 201=no AP found)", disc->reason);
        if (s_retry_num < 20) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry %d/20...", s_retry_num);
        } else {
            xEventGroupSetBits(wifi_event, WIFI_FAIL_BIT);
        }
    } else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        esp_netif_t *netif = event->esp_netif;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Gateway: " IPSTR, IP2STR(&event->ip_info.gw));

        /* 无条件强制设置DNS，热点DHCP经常不分配或分配的DNS不通 */
        esp_netif_dns_info_t dns1 = {0};
        dns1.ip.type = ESP_IPADDR_TYPE_V4;
        dns1.ip.u_addr.ip4.addr = ESP_IP4TOADDR(114, 114, 114, 114);
        esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns1);

        esp_netif_dns_info_t dns2 = {0};
        dns2.ip.type = ESP_IPADDR_TYPE_V4;
        dns2.ip.u_addr.ip4.addr = ESP_IP4TOADDR(223, 5, 5, 5);
        esp_netif_set_dns_info(netif, ESP_NETIF_DNS_BACKUP, &dns2);

        ESP_LOGI(TAG, "DNS set to 114.114.114.114 / 223.5.5.5");
        s_wifi_connected = true;
        s_retry_num = 0;
        xEventGroupSetBits(wifi_event, WIFI_CONNECTED_BIT);
    }
}

void wifi_sta_init(void) {
    esp_err_t ret;
    EventBits_t bits;

    if (wifi_event == NULL) {
        wifi_event = xEventGroupCreate();
    }

    if (strcmp(DEFAULT_SSID, "password") == 0) {
        ESP_LOGW(TAG, "WiFi SSID is still default placeholder. Please update DEFAULT_SSID/DEFAULT_PWD.");
    }

    /* 先清旧事件位，避免复用初始化时读到陈旧状态 */
    xEventGroupClearBits(wifi_event, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

    ret = esp_netif_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_netif_init failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_event_loop_create_default failed: %s", esp_err_to_name(ret));
        return;
    }

    if (s_sta_netif == NULL) {
        s_sta_netif = esp_netif_create_default_wifi_sta();
        if (s_sta_netif == NULL) {
            ESP_LOGE(TAG, "esp_netif_create_default_wifi_sta failed");
            return;
        }
    }

    if (!s_handlers_registered) {
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
        s_handlers_registered = true;
    }

    if (!s_wifi_driver_inited) {
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ret = esp_wifi_init(&cfg);
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "esp_wifi_init failed: %s", esp_err_to_name(ret));
            return;
        }

        wifi_config_t wifi_config = {
            .sta = {
                .ssid = DEFAULT_SSID,
                .password = DEFAULT_PWD,
                .threshold.authmode = WIFI_AUTH_WPA2_PSK,  /* WPA/WPA2/WPA3 均可连接 */
            },
        };

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

        ret = esp_wifi_start();
        if (ret != ESP_OK && ret != ESP_ERR_WIFI_CONN) {
            ESP_LOGE(TAG, "esp_wifi_start failed: %s", esp_err_to_name(ret));
            return;
        }

        s_wifi_driver_inited = true;
    } else {
        ret = esp_wifi_connect();
        if (ret != ESP_OK && ret != ESP_ERR_WIFI_CONN) {
            ESP_LOGW(TAG, "esp_wifi_connect returned: %s", esp_err_to_name(ret));
        }
    }

    bits = xEventGroupWaitBits(wifi_event,
                               WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                               pdFALSE,
                               pdFALSE,
                               pdMS_TO_TICKS(30000));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi connected");
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGW(TAG, "WiFi connect failed after retries");
    } else {
        ESP_LOGW(TAG, "WiFi connect timeout (no CONNECTED/FAIL event in 30s)");
    }
}