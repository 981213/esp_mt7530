#include <lwip/apps/httpd.h>
#include <esp_log.h>
extern int httpd_spiffs_init(void);

void lwhttpd_init(void)
{
	int ret;
	ret = httpd_spiffs_init();
	if (ret < 0) {
		ESP_LOGE("lwhttpd", "spiffs init failed. return %d", ret);
	}
	httpd_init();
}
