#include "mt7530.h"
#include "esp_log.h"
#include "freertos/semphr.h"

static SemaphoreHandle_t mt7530_reg_mutex = NULL;

static void mt7530_reg_mutex_get(void)
{
	if (!mt7530_reg_mutex) {
		mt7530_reg_mutex = xSemaphoreCreateMutex();
	}
	while (xSemaphoreTake(mt7530_reg_mutex, (TickType_t)100) != pdTRUE) {
		ESP_LOGW("mt7530", "mutex timeout.");
	}
}

static void mt7530_reg_mutex_put(void)
{
	xSemaphoreGive(mt7530_reg_mutex);
}

u32 mt7530_r32(u32 reg)
{
	u16 high, low;
	mt7530_reg_mutex_get();
	mdio_write(0x1f, 0x1f, (reg >> 6) & 0x3ff);
	low = mdio_read(0x1f, (reg >> 2) & 0xf);
	high = mdio_read(0x1f, 0x10);
	mt7530_reg_mutex_put();
	return (high << 16) | (low & 0xffff);
}

void mt7530_w32(u32 reg, u32 val)
{
	mt7530_reg_mutex_get();
	mdio_write(0x1f, 0x1f, (reg >> 6) & 0x3ff);
	mdio_write(0x1f, (reg >> 2) & 0xf, val & 0xffff);
	mdio_write(0x1f, 0x10, val >> 16);
	mt7530_reg_mutex_put();
}
