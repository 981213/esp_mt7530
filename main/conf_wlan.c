#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nvram.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "rom/ets_sys.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

static EventGroupHandle_t wifi_event_group;

#define TAG "wlan"

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
	switch (event->event_id) {
	case SYSTEM_EVENT_STA_START:
		esp_wifi_connect();
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		ESP_LOGI(TAG, "got ip:%s",
			 ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
		xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
		break;
	case SYSTEM_EVENT_AP_STACONNECTED:
		ESP_LOGI(TAG, "station:" MACSTR " join, AID=%d",
			 MAC2STR(event->event_info.sta_connected.mac),
			 event->event_info.sta_connected.aid);
		break;
	case SYSTEM_EVENT_AP_STADISCONNECTED:
		ESP_LOGI(TAG, "station:" MACSTR "leave, AID=%d",
			 MAC2STR(event->event_info.sta_disconnected.mac),
			 event->event_info.sta_disconnected.aid);
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		esp_wifi_connect();
		xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
		break;
	default:
		break;
	}
	return ESP_OK;
}
void wifi_init_ap(wifi_config_t *devcfg)
{
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, devcfg));
	ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_init_sta(wifi_config_t *devcfg)
{
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, devcfg));
	ESP_ERROR_CHECK(esp_wifi_start());
}

bool wifi_load_sta_conf(wifi_config_t *cfg)
{
	size_t len = 32;
	memset(cfg, 0, sizeof(wifi_config_t));
	if (nvs_get_str(nvram_handler, "wlan_ssid", (char *)cfg->sta.ssid,
			&len) != ESP_OK)
		return false;
	len = 64;
	if (nvs_get_str(nvram_handler, "wlan_password",
			(char *)cfg->sta.password, &len) != ESP_OK)
		return false;
	return true;
}

void wifi_load_ap_conf(wifi_config_t *cfg)
{
	memset(cfg, 0, sizeof(wifi_config_t));
	strncpy((char *)cfg->ap.ssid, "ESP MT7530", sizeof(cfg->ap.ssid));
	cfg->ap.ssid_len = strlen((char *)cfg->ap.ssid);
	cfg->ap.max_connection = 5;
	cfg->ap.authmode = WIFI_AUTH_OPEN;
}

void wifi_init(void)
{
	wifi_config_t wifi_config;
	wifi_event_group = xEventGroupCreate();
	tcpip_adapter_init();
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	if (wifi_load_sta_conf(&wifi_config)) {
		wifi_init_sta(&wifi_config);
	} else {
		wifi_load_ap_conf(&wifi_config);
		wifi_init_ap(&wifi_config);
	}
}