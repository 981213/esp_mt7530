#include "driver/gpio.h"
#include "mdio.h"

#define MDIO_GPIO_MDC GPIO_NUM_5
#define MDIO_GPIO_MDIO GPIO_NUM_4
void set_mdc(int val)
{
	gpio_set_level(MDIO_GPIO_MDC, val);
}

void set_mdio_data(int val)
{
	gpio_set_level(MDIO_GPIO_MDIO, val);
}

void set_mdio_dir(int dir)
{
	if (dir) {
		gpio_pullup_dis(MDIO_GPIO_MDIO);
		gpio_set_direction(MDIO_GPIO_MDIO, GPIO_MODE_OUTPUT);
	} else {
		gpio_set_direction(MDIO_GPIO_MDIO, GPIO_MODE_INPUT);
		gpio_pullup_en(MDIO_GPIO_MDIO);
	}
}

int get_mdio_data(void)
{
	return gpio_get_level(MDIO_GPIO_MDIO);
}

void mdio_gpio_init(void)
{
	gpio_config_t gpio_conf;
	gpio_conf.intr_type = GPIO_INTR_DISABLE;
	gpio_conf.mode = GPIO_MODE_OUTPUT;
	gpio_conf.pin_bit_mask = 1 << MDIO_GPIO_MDC;
	gpio_conf.pull_down_en = 0;
	gpio_conf.pull_up_en = 0;
	gpio_config(&gpio_conf);
	gpio_conf.mode = GPIO_MODE_INPUT;
	gpio_conf.pin_bit_mask = 1 << MDIO_GPIO_MDIO;
	gpio_conf.pull_up_en = 1;
	gpio_config(&gpio_conf);
	gpio_set_level(MDIO_GPIO_MDC, 0);
}
