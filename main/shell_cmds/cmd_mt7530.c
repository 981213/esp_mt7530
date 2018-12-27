#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uart_shell.h"
#include "mt7530.h"
#include <nvram.h>
void mt7530_vlan_load(void);
void mt7530_vlan_save(void);

static int cmd_mt7530_reg(int argc, char **argv)
{
	u32 addr, val;

	if (argc == 2) {
		addr = strtol(argv[1], NULL, 0);
		val = mt7530_r32(addr);
		printf("0x%08X: 0x%08X\n", addr, val);
	} else if (argc == 3) {
		addr = strtol(argv[1], NULL, 0);
		val = strtol(argv[2], NULL, 0);
		mt7530_w32(addr, val);
		printf("0x%08X: 0x%08X\n", addr, val);
	} else {
		printf("Usage:\n"
		       "read: mt7530 {ADDR}\n"
		       "write: mt7530 {ADDR} {VAL}\n");
	}
	return 0;
}

static int cmd_mt7530_mactable(int argc, char **argv)
{
	mt7530_dump_mac_table();
	return 0;
}

static int cmd_mt7530_dumpmib(int argc, char **argv)
{
	int port;
	if (argc != 2)
		puts("usage: mt7530 dump_mib {port number}");
	port = (int)strtol(argv[1], NULL, 0);
	mt7530_dump_port_mib(port);
	return 0;
}

static int cmd_mt7530_reset(int argc, char **argv)
{
	mt7530_chip_reset();
	return 0;
}

static int cmd_mt7530_vlan(int argc, char **argv)
{
	if (argc < 2) {
		puts("invalid arguments.");
		return 0;
	}

	if (*argv[1] == 'e' && argc == 3) {
		mt7530_vlan_set_enable(strtol(argv[2], NULL, 0) != 0);
	} else if (*argv[1] == 's' && argc == 5) {
		mt7530_vlan_set_entry((u16)strtol(argv[2], NULL, 0),
				      (u8)strtol(argv[3], NULL, 0),
				      (u8)strtol(argv[4], NULL, 0));
	} else if (*argv[1] == 'r') {
		mt7530_vlan_conf_reset();
	} else if (*argv[1] == 'a') {
		mt7530_vlan_apply();
	} else if (*argv[1] == 'c') {
		mt7530_vlan_save();
	} else if (*argv[1] == 'l') {
		mt7530_vlan_load();
	}
	return 0;
}

static const uart_cmd_entry mt7530_cmds[] = {
	{ "dump_mib", "dump mib counter", cmd_mt7530_dumpmib },
	{ "mactable", "dump mac table", cmd_mt7530_mactable },
	{ "reg", "read/write mt7530 register", cmd_mt7530_reg },
	{ "reset", "reset mt7530 chip", cmd_mt7530_reset },
	{ "vlan", "vlan operations", cmd_mt7530_vlan },
};

int cmd_mt7530_main(int argc, char **argv)
{
	int i;
	if (argc < 2) {
		puts("mt7530 usage:");
		for (i = 0; i < ARRAY_SIZE(mt7530_cmds); i++) {
			printf("mt7530 %s - %s\n", mt7530_cmds[i].cmd,
			       mt7530_cmds[i].desc);
		}
		return 1;
	}
	for (i = 0; i < ARRAY_SIZE(mt7530_cmds); i++) {
		if (!strcmp(mt7530_cmds[i].cmd, argv[1])) {
			return mt7530_cmds[i].func_main(argc - 1, argv + 1);
		}
	}
	printf("mt7530: %s: unknown command\n", argv[1]);
	return 1;
}
