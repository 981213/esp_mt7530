#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nvram.h>
#include <uart_shell.h>

static int cmd_nvram_get(int argc, char **argv)
{
	char buf[128];
    size_t len = sizeof(buf);
	esp_err_t esp_err;
	if (argc == 2) {
		esp_err =
			nvs_get_str(nvram_handler, argv[1], buf, &len);
		if (esp_err == ESP_OK) {
			puts(buf);
		} else {
			puts("get value failed.");
		}
	}
	return 0;
}

static int cmd_nvram_set(int argc, char **argv)
{
	esp_err_t esp_err;
	if (argc == 3) {
		esp_err = nvs_set_str(nvram_handler, argv[1], argv[2]);
		if (esp_err != ESP_OK) {
			puts("set value failed.");
		}
	}
	return 0;
}

static int cmd_nvram_commit(int argc, char **argv)
{
	nvs_commit(nvram_handler);
	return 0;
}

static int cmd_nvram_del(int argc, char **argv)
{
	esp_err_t esp_err;
	if (argc == 2) {
		esp_err = nvs_erase_key(nvram_handler, argv[1]);
		if (esp_err != ESP_OK) {
			puts("delete value failed.");
		}
	}
	return 0;
}

static int cmd_nvram_erase(int argc, char **argv)
{
	nvs_erase_all(nvram_handler);
	return 0;
}

static const uart_cmd_entry nvram_cmds[] = {
	{ "set", "set key to string value", cmd_nvram_set },
	{ "get", "get key value", cmd_nvram_get },
	{ "commit", "write to flash", cmd_nvram_commit },
	{ "del", "delete key", cmd_nvram_del },
	{ "erase", "erase nvram", cmd_nvram_erase },
};

int cmd_nvram_main(int argc, char **argv)
{
	int i;
	if (argc < 2) {
		puts("nvram usage:");
		for (i = 0; i < ARRAY_SIZE(nvram_cmds); i++) {
			printf("nvram %s - %s\n", nvram_cmds[i].cmd,
			       nvram_cmds[i].desc);
		}
		return 1;
	}
	for (i = 0; i < ARRAY_SIZE(nvram_cmds); i++) {
		if (!strcmp(nvram_cmds[i].cmd, argv[1])) {
			return nvram_cmds[i].func_main(argc - 1, argv + 1);
		}
	}
	printf("nvram: %s: unknown command\n", argv[1]);
	return 1;
}
