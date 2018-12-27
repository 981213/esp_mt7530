/*
 * Bitbanged MDIO support.
 * Based on the linux kernel one
 * 
 * Author: Scott Wood <scottwood@freescale.com>
 * Copyright (c) 2007 Freescale Semiconductor
 *
 * Based on CPM2 MDIO code which is:
 *
 * Copyright (c) 2003 Intracom S.A.
 *  by Pantelis Antoniou <panto@intracom.gr>
 *
 * 2005 (c) MontaVista Software, Inc.
 * Vitaly Bordug <vbordug@ru.mvista.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include "mdio.h"
#include "esp_log.h"
#include "freertos/semphr.h"

#define MDIO_SETUP_TIME 10
#define MDIO_HOLD_TIME 10

/* Minimum MDC period is 400 ns, plus some margin for error.  MDIO_DELAY
 * is done twice per period.
 */
#define MDIO_DELAY 250

/* The PHY may take up to 300 ns to produce data, plus some margin
 * for error.
 */
#define MDIO_READ_DELAY 350

static inline void ndelay(int times)
{
	times /= 13;
	unsigned r, rt;
	asm volatile("rsr %0, ccount" : "=r"(r));
	rt = r;
	while (rt - r > 0 && rt - r < times) {
		asm volatile("rsr %0, ccount" : "=r"(r));
	}
}

/* MDIO must already be configured as output. */
static void mdiobb_send_bit(int val)
{
	set_mdio_data(val);
	ndelay(MDIO_DELAY);
	set_mdc(1);
	ndelay(MDIO_DELAY);
	set_mdc(0);
}

/* MDIO must already be configured as input. */
static int mdiobb_get_bit()
{
	ndelay(MDIO_DELAY);
	set_mdc(1);
	ndelay(MDIO_READ_DELAY);
	set_mdc(0);

	return get_mdio_data();
}

/* MDIO must already be configured as output. */
static void mdiobb_send_num(u16 val, int bits)
{
	int i;

	for (i = bits - 1; i >= 0; i--)
		mdiobb_send_bit((val >> i) & 1);
}

/* MDIO must already be configured as input. */
static u16 mdiobb_get_num(int bits)
{
	int i;
	u16 ret = 0;

	for (i = bits - 1; i >= 0; i--) {
		ret <<= 1;
		ret |= mdiobb_get_bit();
	}

	return ret;
}

/* Utility to send the preamble, address, and
 * register (common to read and write).
 */
static void mdiobb_cmd(int op, u8 phy, u8 reg)
{
	int i;

	set_mdio_dir(1);

	/*
	 * Send a 32 bit preamble ('1's) with an extra '1' bit for good
	 * measure.  The IEEE spec says this is a PHY optional
	 * requirement.  The AMD 79C874 requires one after power up and
	 * one after a MII communications error.  This means that we are
	 * doing more preambles than we need, but it is safer and will be
	 * much more robust.
	 */

	for (i = 0; i < 32; i++)
		mdiobb_send_bit(1);

	/* send the start bit (01) and the read opcode (10) or write (01).
	   Clause 45 operation uses 00 for the start and 11, 10 for
	   read/write */
	mdiobb_send_bit(0);
	if (op & MDIO_C45)
		mdiobb_send_bit(0);
	else
		mdiobb_send_bit(1);
	mdiobb_send_bit((op >> 1) & 1);
	mdiobb_send_bit((op >> 0) & 1);

	mdiobb_send_num(phy, 5);
	mdiobb_send_num(reg, 5);
}

/* In clause 45 mode all commands are prefixed by MDIO_ADDR to specify the
   lower 16 bits of the 21 bit address. This transfer is done identically to a
   MDIO_WRITE except for a different code. To enable clause 45 mode or
   MII_ADDR_C45 into the address. Theoretically clause 45 and normal devices
   can exist on the same bus. Normal devices should ignore the MDIO_ADDR
   phase. */
static int mdiobb_cmd_addr(int phy, u32 addr)
{
	unsigned int dev_addr = (addr >> 16) & 0x1F;
	unsigned int reg = addr & 0xFFFF;
	mdiobb_cmd(MDIO_C45_ADDR, phy, dev_addr);

	/* send the turnaround (10) */
	mdiobb_send_bit(1);
	mdiobb_send_bit(0);

	mdiobb_send_num(reg, 16);

	set_mdio_dir(0);
	mdiobb_get_bit();

	return dev_addr;
}

static SemaphoreHandle_t mdio_mutex = NULL;

static void mdio_mutex_get(void)
{
	if (!mdio_mutex) {
		mdio_mutex = xSemaphoreCreateMutex();
	}
	while (xSemaphoreTake(mdio_mutex, (TickType_t)100) != pdTRUE) {
		ESP_LOGW("mdio_gpio", "mutex timeout.");
	}
}

static void mdio_mutex_put(void)
{
	xSemaphoreGive(mdio_mutex);
}

int mdio_read(int phy, int reg)
{
	int ret, i;
	mdio_mutex_get();
	if (reg & MII_ADDR_C45) {
		reg = mdiobb_cmd_addr(phy, reg);
		mdiobb_cmd(MDIO_C45_READ, phy, reg);
	} else
		mdiobb_cmd(MDIO_READ, phy, reg);

	set_mdio_dir(0);

	/* check the turnaround bit: the PHY should be driving it to zero, if this
	 * PHY is listed in phy_ignore_ta_mask as having broken TA, skip that
	 */
	if (mdiobb_get_bit() != 0) {
		/* PHY didn't drive TA low -- flush any bits it
		 * may be trying to send.
		 */
		for (i = 0; i < 32; i++)
			mdiobb_get_bit();

		mdio_mutex_put();
		return 0xffff;
	}

	ret = mdiobb_get_num(16);
	mdiobb_get_bit();
	mdio_mutex_put();
	return ret;
}

int mdio_write(int phy, int reg, u16 val)
{
	mdio_mutex_get();
	if (reg & MII_ADDR_C45) {
		reg = mdiobb_cmd_addr(phy, reg);
		mdiobb_cmd(MDIO_C45_WRITE, phy, reg);
	} else
		mdiobb_cmd(MDIO_WRITE, phy, reg);

	/* send the turnaround (10) */
	mdiobb_send_bit(1);
	mdiobb_send_bit(0);

	mdiobb_send_num(val, 16);

	set_mdio_dir(0);
	mdiobb_get_bit();
	mdio_mutex_put();
	return 0;
}
