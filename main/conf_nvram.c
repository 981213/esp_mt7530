#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_system.h"

#include <nvram.h>

nvs_handle nvram_handler;

esp_err_t nvram_init()
{
	esp_err_t esp_err;
	esp_err = nvs_flash_init_partition("esp_nvs");
	if (esp_err != ESP_OK) {
		ESP_LOGE("nvram", "nvram partition init failed");
		return esp_err;
	}
	esp_err = nvs_open_from_partition("esp_nvs", "mtmgr", NVS_READWRITE,
					  &nvram_handler);
	if (esp_err != ESP_OK) {
		ESP_LOGE("nvram", "nvram init failed");
		return esp_err;
	}
	return ESP_OK;
}
