#ifndef _MT7530_H_
#define _MT7530_H_
#include <mdio.h>

#define MT7530_NUM_PORTS 7

#define MT7530_ATC 0x80
#define MT7530_TSRA1 0x84
#define MT7530_TSRA2 0x88
#define MT7530_ATC_BUSY BIT(15)
#define MT7530_ATC_SRCH_END BIT(14)
#define MT7530_ATC_SRCH_HIT BIT(13)
#define MT7530_ATC_ADDR_INVLD BIT(12)
#define MT7530_ATC_CMD_CLR 0x2
#define MT7530_ATC_CMD_MAC_FIRST 0x4
#define MT7530_ATC_CMD_MAC_NEXT 0x5

#define MT7530_SYSC 0x7000
#define MT7530_SYSC_CHIP_RESET 0x3

#define MT7530_SYS_INT_STS 0x700c
#define MT7530_SYS_IDENT 0x7ffc

/* OpenWrt swconfig struct */

/* port nested attributes */
enum {
	SWITCH_PORT_UNSPEC,
	SWITCH_PORT_ID,
	SWITCH_PORT_FLAG_TAGGED,
	SWITCH_PORT_ATTR_MAX
};

enum switch_port_speed {
	SWITCH_PORT_SPEED_UNKNOWN = 0,
	SWITCH_PORT_SPEED_10 = 10,
	SWITCH_PORT_SPEED_100 = 100,
	SWITCH_PORT_SPEED_1000 = 1000,
};

struct switch_port_link {
	bool link;
	bool duplex;
	bool aneg;
	bool tx_flow;
	bool rx_flow;
	enum switch_port_speed speed;
	/* in ethtool adv_t format */
	u32 eee;
};

struct switch_port {
	u32 id;
	u32 flags;
};

struct switch_val {
	unsigned int port_vlan;
	unsigned int len;
	union {
		const char *s;
		u32 i;
		struct switch_port *ports;
		struct switch_port_link *link;
	} value;
};

u32 mt7530_r32(u32 reg);
void mt7530_w32(u32 reg, u32 val);

void mt7530_chip_reset(void);
bool mt7530_check_init(void);
void mt7530_init(void);

void mt7530_dump_mac_table(void);
int mt7530_dump_port_mib(int port);
int mt7530_get_port_link(int port, struct switch_port_link *link);

void mt7530_vlan_conf_reset(void);
void mt7530_vlan_set_enable(bool enabled);
void mt7530_vlan_set_entry(u16 vid, u8 member, u8 etags);
void mt7530_vlan_apply(void);
void mt7530_vlan_load(void);
void mt7530_vlan_save(void);
#endif
