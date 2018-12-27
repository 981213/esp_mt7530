#ifndef _MT7530_VLAN_H_
#define _MT7530_VLAN_H_
#include <mdio.h>

#define MT7530_CPU_PORT 6
/* for a 5 port switch 4095 entries are too much. */
#define MT7530_NUM_VLANS 16
#define MT7530_MAX_VID 15
#define MT7530_MIN_VID 0

/* registers */
#define REG_ESW_VLAN_VTCR 0x90
#define REG_ESW_VLAN_VAWD1 0x94
#define REG_ESW_VLAN_VAWD2 0x98
#define REG_ESW_VLAN_VTIM(x) (0x100 + 4 * ((x) / 2))

#define REG_ESW_VLAN_VAWD1_IVL_MAC BIT(30)
#define REG_ESW_VLAN_VAWD1_VTAG_EN BIT(28)
#define REG_ESW_VLAN_VAWD1_VALID BIT(0)

/* vlan egress mode */
enum { ETAG_CTRL_UNTAG = 0,
       ETAG_CTRL_TAG = 2,
       ETAG_CTRL_SWAP = 1,
       ETAG_CTRL_STACK = 3,
};

#define REG_ESW_PORT_PCR(x) (0x2004 | ((x) << 8))
#define REG_ESW_PORT_PVC(x) (0x2010 | ((x) << 8))
#define REG_ESW_PORT_PPBV1(x) (0x2014 | ((x) << 8))

struct mt7530_port_entry {
	u16 pvid;
};

struct mt7530_vlan_entry {
	u16 vid;
	u8 member;
	u8 etags;
};

struct mt7530_priv {
	bool global_vlan_enable;
	u8 vlan_count;
	struct mt7530_vlan_entry vlan_entries[MT7530_NUM_VLANS];
	struct mt7530_port_entry port_entries[MT7530_NUM_PORTS];
};

#endif // _MT7530_VLAN_H_
