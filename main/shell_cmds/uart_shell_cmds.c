#include <stdio.h>
#include <string.h>
#include "uart_shell.h"
#include <esp_system.h>

int cmd_mdio_main(int argc, char **argv);
int cmd_mt7530_main(int argc, char **argv);
int cmd_nvram_main(int argc, char **argv);

static int help_main(int argc, char **argv)
{
	int i;
	for (i = 0; i < CMD_CNT; i++) {
		printf("%s - %s\n", uart_cmds[i].cmd, uart_cmds[i].desc);
	}
	return 0;
}

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

const uart_cmd_entry uart_cmds[CMD_CNT] = {
	{ "free", "free memory size", free_main },
	{ "help", "print this help", help_main },
	{ "mdio", "mdio operations", cmd_mdio_main },
	{ "mt7530", "mt7530 operations", cmd_mt7530_main },
	{ "nvram", "nvram operations", cmd_nvram_main },
	{ "reset", "chip reset", reset_main },
};

void uart_cmd_exec(int argc, char **argv)
{
	int i;
	for (i = 0; i < CMD_CNT; i++) {
		if (!strcmp(uart_cmds[i].cmd, argv[0])) {
			uart_cmds[i].func_main(argc, argv);
			return;
		}
	}
	printf("esp_cmd: %s: unknown command\n", argv[0]);
}
