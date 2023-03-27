#include <stdio.h>
#include <string.h>
#include "uart_shell.h"
#include <esp_system.h>
#include <esp_console.h>

int cmd_mdio_main(int argc, char **argv);
int cmd_mt7530_main(int argc, char **argv);
int cmd_nvram_main(int argc, char **argv);

static int free_main(int argc, char **argv)
{
	printf("Free heap memory: %u bytes\n", esp_get_free_heap_size());
	return 0;
}

static int reset_main(int argc, char **argv)
{
	esp_restart();
	return 0;
}

const esp_console_cmd_t uart_cmds[CMD_CNT] = {
	{ "free", "free memory size", NULL, free_main, NULL },
	{ "mdio", "mdio operations", NULL, cmd_mdio_main, NULL },
	{ "mt7530", "mt7530 operations", NULL, cmd_mt7530_main, NULL },
	{ "nvram", "nvram operations", NULL, cmd_nvram_main, NULL },
	{ "reset", "chip reset", NULL, reset_main, NULL },
};

void uart_shell_reg_cmds(void) {
    int i;
    esp_console_register_help_command();
    for (i = 0; i < CMD_CNT; i++) {
        ESP_ERROR_CHECK(esp_console_cmd_register(&uart_cmds[i]));
    }
}
