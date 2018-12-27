#include <mt7530_mib.h>
#include <mt7530.h>
#include <stdio.h>

#define MIB_DESC(_s, _o, _n)                                                   \
	{                                                                      \
		.size = (_s), .offset = (_o), .name = (_n),                    \
	}
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

struct mt7xxx_mib_desc {
	unsigned int size;
	unsigned int offset;
	const char *name;
};

static const struct mt7xxx_mib_desc mt7530_mibs[] = {
	MIB_DESC(1, MT7530_STATS_TDPC, "TxDrop"),
	MIB_DESC(1, MT7530_STATS_TCRC, "TxCRC"),
	MIB_DESC(1, MT7530_STATS_TUPC, "TxUni"),
	MIB_DESC(1, MT7530_STATS_TMPC, "TxMulti"),
	MIB_DESC(1, MT7530_STATS_TBPC, "TxBroad"),
	MIB_DESC(1, MT7530_STATS_TCEC, "TxCollision"),
	MIB_DESC(1, MT7530_STATS_TSCEC, "TxSingleCol"),
	MIB_DESC(1, MT7530_STATS_TMCEC, "TxMultiCol"),
	MIB_DESC(1, MT7530_STATS_TDEC, "TxDefer"),
	MIB_DESC(1, MT7530_STATS_TLCEC, "TxLateCol"),
	MIB_DESC(1, MT7530_STATS_TXCEC, "TxExcCol"),
	MIB_DESC(1, MT7530_STATS_TPPC, "TxPause"),
	MIB_DESC(1, MT7530_STATS_TL64PC, "Tx64Byte"),
	MIB_DESC(1, MT7530_STATS_TL65PC, "Tx65Byte"),
	MIB_DESC(1, MT7530_STATS_TL128PC, "Tx128Byte"),
	MIB_DESC(1, MT7530_STATS_TL256PC, "Tx256Byte"),
	MIB_DESC(1, MT7530_STATS_TL512PC, "Tx512Byte"),
	MIB_DESC(1, MT7530_STATS_TL1024PC, "Tx1024Byte"),
	MIB_DESC(2, MT7530_STATS_TOC, "TxByte"),
	MIB_DESC(1, MT7530_STATS_RDPC, "RxDrop"),
	MIB_DESC(1, MT7530_STATS_RFPC, "RxFiltered"),
	MIB_DESC(1, MT7530_STATS_RUPC, "RxUni"),
	MIB_DESC(1, MT7530_STATS_RMPC, "RxMulti"),
	MIB_DESC(1, MT7530_STATS_RBPC, "RxBroad"),
	MIB_DESC(1, MT7530_STATS_RAEPC, "RxAlignErr"),
	MIB_DESC(1, MT7530_STATS_RCEPC, "RxCRC"),
	MIB_DESC(1, MT7530_STATS_RUSPC, "RxUnderSize"),
	MIB_DESC(1, MT7530_STATS_RFEPC, "RxFragment"),
	MIB_DESC(1, MT7530_STATS_ROSPC, "RxOverSize"),
	MIB_DESC(1, MT7530_STATS_RJEPC, "RxJabber"),
	MIB_DESC(1, MT7530_STATS_RPPC, "RxPause"),
	MIB_DESC(1, MT7530_STATS_RL64PC, "Rx64Byte"),
	MIB_DESC(1, MT7530_STATS_RL65PC, "Rx65Byte"),
	MIB_DESC(1, MT7530_STATS_RL128PC, "Rx128Byte"),
	MIB_DESC(1, MT7530_STATS_RL256PC, "Rx256Byte"),
	MIB_DESC(1, MT7530_STATS_RL512PC, "Rx512Byte"),
	MIB_DESC(1, MT7530_STATS_RL1024PC, "Rx1024Byte"),
	MIB_DESC(2, MT7530_STATS_ROC, "RxByte"),
	MIB_DESC(1, MT7530_STATS_RDPC_CTRL, "RxCtrlDrop"),
	MIB_DESC(1, MT7530_STATS_RDPC_ING, "RxIngDrop"),
	MIB_DESC(1, MT7530_STATS_RDPC_ARL, "RxARLDrop")
};

static u64 get_mib_counter(int i, int port)
{
	unsigned int port_base;
	u64 lo;

	port_base =
		MT7530_MIB_COUNTER_BASE + MT7530_MIB_COUNTER_PORT_OFFSET * port;

	lo = mt7530_r32(port_base + mt7530_mibs[i].offset);
	if (mt7530_mibs[i].size == 2) {
		u64 hi;

		hi = mt7530_r32(port_base + mt7530_mibs[i].offset + 4);
		lo |= hi << 32;
	}

	return lo;
}

int mt7530_dump_port_mib(int port)
{
	int i;
	u64 counter;

	if (port >= MT7530_NUM_PORTS)
		return -1;

	printf("Port %d MIB counters\n", port);

	for (i = 0; i < ARRAY_SIZE(mt7530_mibs); ++i) {
		printf("%-11s: ", mt7530_mibs[i].name);
		counter = get_mib_counter(i, port);
		printf("%u\n", (u32)counter);
	}

	return 0;
}
