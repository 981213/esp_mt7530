#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mdio.h"

int cmd_mdio_main(int argc, char **argv)
{
	int phy, addr, ret;
	u16 val;
	if (argc == 3) {
		phy = (int)(strtol(argv[1], NULL, 0));
		addr = (int)(strtol(argv[2], NULL, 0));
		ret = mdio_read(phy, addr);
		printf("%d:0x%02X: 0x%04X\n", phy, (unsigned)addr,
		       (unsigned)ret);
	} else if (argc == 4) {
		phy = (int)(strtol(argv[1], NULL, 0));
		addr = (int)(strtol(argv[2], NULL, 0));
		val = (u16)(strtol(argv[3], NULL, 0));
		ret = mdio_write(phy, addr, val);
		printf("%d:0x%02X: 0x%04X\n", phy, addr, (unsigned)val);
	} else {
		printf("Usage:\n"
		       "read: mdio {PHY} {ADDR}\n"
		       "write:  mdio {PHY} {ADDR} {VAL}\n");
	}
    return 0;
}