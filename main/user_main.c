#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include <mdio.h>
#include <uart_shell.h>
#include <nvram.h>
void wifi_init_sta(void);
void mt7530_init(void);
void app_main()
{
	esp_err_t esp_ret;
	printf("ESP8266 MT7530 Manager v0.1\n");
	printf("Copyright (c) 2018 GuoGuo <gch981213@gmail.com>\n");
	printf("ESP SDK version:%s\n", esp_get_idf_version());
	esp_ret = nvram_init();
	if (esp_ret != ESP_OK) {
		ESP_LOGE("main", "nvram init failed");
		return;
	}
	wifi_init_sta();
	mdio_gpio_init();
	mt7530_init();
	uart_shell_init();
}
