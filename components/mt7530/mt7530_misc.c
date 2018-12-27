#include <stdio.h>
#include <mt7530.h>
#include "freertos/task.h"
#include "rom/ets_sys.h"
#include <esp_log.h>

static inline void mt7530_atc_wait_busy(void)
{
	u32 atc_val;
	atc_val = mt7530_r32(MT7530_ATC);
	while (atc_val & MT7530_ATC_BUSY) {
		atc_val = mt7530_r32(MT7530_ATC);
	}
}

void mt7530_dump_mac_table(void)
{
	u32 reg_val;
	u32 atc_val;
	u8 mac_addr[6];
	u8 i;

	puts("MAC table:");
	mt7530_w32(MT7530_ATC, MT7530_ATC_BUSY | MT7530_ATC_CMD_MAC_FIRST);
	mt7530_atc_wait_busy();
	atc_val = mt7530_r32(MT7530_ATC);
	while (!(atc_val & MT7530_ATC_SRCH_END)) {
		reg_val = mt7530_r32(MT7530_TSRA1);
		for (i = 0; i < 4; i++) {
			mac_addr[i] = (reg_val >> ((3 - i) * 8)) & 0xff;
		}
		reg_val = mt7530_r32(MT7530_TSRA2);
		mac_addr[4] = reg_val >> 24;
		mac_addr[5] = (reg_val >> 16) & 0xff;
		printf("CVID: %u MAC: " MACSTR "\n", reg_val & 0x1f,
		       MAC2STR(mac_addr));
		mt7530_w32(MT7530_ATC,
			   MT7530_ATC_BUSY | MT7530_ATC_CMD_MAC_NEXT);
		mt7530_atc_wait_busy();
		atc_val = mt7530_r32(MT7530_ATC);
	}
}

static int mt7530_get_port_link(int port, struct switch_port_link *link)
{
	u32 speed, pmsr;

	if (port < 0 || port >= MT7530_NUM_PORTS)
		return -1;

	pmsr = mt7530_r32(0x3008 + (0x100 * port));

	link->link = pmsr & 1;
	link->duplex = (pmsr >> 1) & 1;
	speed = (pmsr >> 2) & 3;

	switch (speed) {
	case 0:
		link->speed = SWITCH_PORT_SPEED_10;
		break;
	case 1:
		link->speed = SWITCH_PORT_SPEED_100;
		break;
	case 2:
	case 3: /* forced gige speed can be 2 or 3 */
		link->speed = SWITCH_PORT_SPEED_1000;
		break;
	default:
		link->speed = SWITCH_PORT_SPEED_UNKNOWN;
		break;
	}

	return 0;
}

bool mt7530_check_init(void)
{
	u32 reg;
	reg = mt7530_r32(MT7530_SYSC);
	return reg != 0;
}

void mt7530_chip_reset(void)
{
	const TickType_t wait_delay = 500 / portTICK_PERIOD_MS;
	printf("mt7530: resetting...");
	mt7530_w32(MT7530_SYSC, MT7530_SYSC_CHIP_RESET);
	vTaskDelay(wait_delay);
	while (!mt7530_check_init())
		vTaskDelay(wait_delay);
	puts("done.");
}

static void mt7530_link_polling(void *pvParameters)
{
	static u32 link_reg;
	static struct switch_port_link plink;
	const TickType_t wait_delay = 1000 / portTICK_PERIOD_MS;
	while (1) {
		link_reg = mt7530_r32(MT7530_SYS_INT_STS);
		mt7530_w32(MT7530_SYS_INT_STS, 0x7f);
		for (int i = 0; i < MT7530_NUM_PORTS; i++) {
			if (link_reg & BIT(i) &&
			    (mt7530_get_port_link(i, &plink) == 0)) {
				if (plink.link)
					ESP_LOGI(
						"mt7530",
						"port %d: link up (%dMbps/%s duplex)",
						i, plink.speed,
						plink.duplex ? "full" : "half");
				else
					ESP_LOGI("mt7530", "port %d: link down",
						 i);
			}
		}
		vTaskDelay(wait_delay);
	}
	vTaskDelete(NULL);
}

void mt7530_init(void)
{
	mt7530_chip_reset();
	mt7530_vlan_load();
	mt7530_vlan_apply();
	xTaskCreate(mt7530_link_polling, "mt7530_link", 2048, NULL, 14, NULL);
}