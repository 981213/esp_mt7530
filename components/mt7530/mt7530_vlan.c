#include <stdio.h>
#include <mt7530.h>
#include <mt7530_vlan.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <nvram.h>

static void mt7530_vtcr(struct mt7530_priv *priv, u32 cmd, u32 val)
{
	int i;
	const TickType_t wait_delay = 1 / portTICK_PERIOD_MS;

	mt7530_w32(REG_ESW_VLAN_VTCR, BIT(31) | (cmd << 12) | val);

	for (i = 0; i < 20; i++) {
		u32 val = mt7530_r32(REG_ESW_VLAN_VTCR);

		if ((val & BIT(31)) == 0)
			break;

		vTaskDelay(wait_delay);
	}
	if (i == 20)
		ESP_LOGW("mt7530", "vtcr timeout");
}

static void mt7530_write_vlan_entry(struct mt7530_priv *priv, int vlan, u16 vid,
				    u8 ports, u8 etags)
{
	int port;
	u32 val;

	/* vlan port membership */
	if (ports)
		mt7530_w32(REG_ESW_VLAN_VAWD1,
			   REG_ESW_VLAN_VAWD1_IVL_MAC |
				   REG_ESW_VLAN_VAWD1_VTAG_EN | (ports << 16) |
				   REG_ESW_VLAN_VAWD1_VALID);
	else
		mt7530_w32(REG_ESW_VLAN_VAWD1, 0);

	/* egress mode */
	val = 0;
	for (port = 0; port < MT7530_NUM_PORTS; port++) {
		if (etags & BIT(port))
			val |= ETAG_CTRL_TAG << (port * 2);
		else
			val |= ETAG_CTRL_UNTAG << (port * 2);
	}
	mt7530_w32(REG_ESW_VLAN_VAWD2, val);

	/* write to vlan table */
	mt7530_vtcr(priv, 1, vid);
}

static int mt7530_apply_config(struct mt7530_priv *priv)
{
	int i, j;
	u8 tag_ports;
	u8 untag_ports;

	if (!priv->global_vlan_enable) {
		/*
         * instead of owrt which isolates all ports we need this
         * to be a dumb switch so set port matrix to ff0000
         * instead of 400000.
         */
		ESP_LOGI("mt7530", "VLAN is disabled.");
		for (i = 0; i < MT7530_NUM_PORTS; i++)
			mt7530_w32(REG_ESW_PORT_PCR(i), 0x00ff0000);

		//mt7530_w32(REG_ESW_PORT_PCR(MT7530_CPU_PORT), 0x00ff0000);

		for (i = 0; i < MT7530_NUM_PORTS; i++)
			mt7530_w32(REG_ESW_PORT_PVC(i), 0x810000c0);

		return 0;
	}

	/* set all ports as security mode */
	for (i = 0; i < MT7530_NUM_PORTS; i++)
		mt7530_w32(REG_ESW_PORT_PCR(i), 0x00ff0003);

	/* check if a port is used in tag/untag vlan egress mode */
	tag_ports = 0;
	untag_ports = 0;

	for (i = 0; i < MT7530_NUM_VLANS; i++) {
		u8 member = priv->vlan_entries[i].member;
		u8 etags = priv->vlan_entries[i].etags;

		if (!member)
			continue;

		for (j = 0; j < MT7530_NUM_PORTS; j++) {
			if (!(member & BIT(j)))
				continue;

			if (etags & BIT(j))
				tag_ports |= 1u << j;
			else
				untag_ports |= 1u << j;
		}
	}

	/* set all untag-only ports as transparent and the rest as user port */
	for (i = 0; i < MT7530_NUM_PORTS; i++) {
		u32 pvc_mode = 0x81000000;

		if (untag_ports & BIT(i) && !(tag_ports & BIT(i)))
			pvc_mode = 0x810000c0;

		mt7530_w32(REG_ESW_PORT_PVC(i), pvc_mode);
	}

	/* first clear the swtich vlan table */
	for (i = 0; i < MT7530_MAX_VID; i++)
		mt7530_write_vlan_entry(priv, i, i, 0, 0);

	/* now program only vlans with members to avoid
	   clobbering remapped entries in later iterations */
	for (i = 0; i < priv->vlan_count; i++) {
		u16 vid = priv->vlan_entries[i].vid;
		u8 member = priv->vlan_entries[i].member;
		u8 etags = priv->vlan_entries[i].etags;

		if (member) {
			mt7530_write_vlan_entry(priv, i, vid, member, etags);
			ESP_LOGI("mt7530",
				 "VLAN ID %d members 0x%02X tags 0x%02X",
				 priv->vlan_entries[i].vid,
				 priv->vlan_entries[i].member,
				 priv->vlan_entries[i].etags);
		}
	}

	/* Port Default PVID */
	for (i = 0; i < MT7530_NUM_PORTS; i++) {
		u32 val;

		val = mt7530_r32(REG_ESW_PORT_PPBV1(i));
		val &= ~0xfff;
		val |= priv->port_entries[i].pvid;
		mt7530_w32(REG_ESW_PORT_PPBV1(i), val);
		ESP_LOGI("mt7530", "Port %d VID %d", i,
			 priv->port_entries[i].pvid);
	}

	return 0;
}

static struct mt7530_priv sw_priv;

void mt7530_vlan_conf_reset(void)
{
	memset(&sw_priv, 0, sizeof(sw_priv));
}

void mt7530_vlan_set_enable(bool enabled)
{
	sw_priv.global_vlan_enable = enabled;
}

void mt7530_vlan_set_entry(u16 vid, u8 member, u8 etags)
{
	int i;
	u8 untag_members;
	for (i = 0; i < sw_priv.vlan_count; i++)
		if (sw_priv.vlan_entries[i].vid == vid)
			break;
	if (i >= MT7530_NUM_VLANS) {
		ESP_LOGE("mt7530", "too many vlan entries.");
		return;
	}
	if (vid >= MT7530_MAX_VID) {
		ESP_LOGE("mt7530", "invalid vlan id.");
		return;
	}
	sw_priv.vlan_entries[i].vid = vid;
	sw_priv.vlan_entries[i].member = member;
	sw_priv.vlan_entries[i].etags = etags;
	if (i == sw_priv.vlan_count)
		sw_priv.vlan_count++;

	untag_members = member & (~etags);

	for (i = 0; i < MT7530_NUM_PORTS; i++) {
		if (untag_members & BIT(i)) {
			sw_priv.port_entries[i].pvid = vid;
		}
	}
}

void mt7530_vlan_apply(void)
{
	mt7530_apply_config(&sw_priv);
}

void mt7530_vlan_load(void)
{
	size_t len = sizeof(sw_priv);
	if (nvs_get_blob(nvram_handler, "mt7530_vlan", (void *)&sw_priv,
			 &len) != ESP_OK) {
		ESP_LOGW(
			"mt7530",
			"Loading VLAN configuration failed. VLAN will be disabled.");
		mt7530_vlan_conf_reset();
	}
}

void mt7530_vlan_save(void)
{
	if (nvs_set_blob(nvram_handler, "mt7530_vlan", (void *)&sw_priv,
			 sizeof(sw_priv)) != ESP_OK) {
		ESP_LOGE("mt7530", "Failed to save VLAN configuration.");
	}
	nvs_commit(nvram_handler);
}
