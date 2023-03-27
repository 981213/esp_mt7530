#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

static void heap_mon(void *pvParameters)
{
	const TickType_t wait_delay = 2000 / portTICK_PERIOD_MS;
	unsigned int pfmem, cfmem;
	pfmem = esp_get_free_heap_size();
	while (1) {
		cfmem = esp_get_free_heap_size();
		if (pfmem > cfmem) {
			ESP_LOGI("heapmon", "Heap memory decreased %u bytes.",
				 pfmem - cfmem);
		} else if (pfmem < cfmem) {
			ESP_LOGI("heapmon", "Heap memory increased %u bytes.",
				 cfmem - pfmem);
		} else {
			vTaskDelay(wait_delay);
			continue;
		}
		pfmem = cfmem;
		ESP_LOGI("heapmon", "Current free memory %u bytes.", cfmem);
	}
	vTaskDelete(NULL);
}

void heapmon_init(void)
{
	xTaskCreate(heap_mon, "heap_mon", 2048, NULL, 14, NULL);
}