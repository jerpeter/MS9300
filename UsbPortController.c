// SPDX-License-Identifier: GPL-2.0
/*
 * Driver for TI TPS25750 USB Power Delivery controller family
 *
 * Copyright (C) 2023, Geotab Inc.
 * Author: Abdel Alkuor <abdelalkuor@geotab.com>
 */

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Typedefs.h"
#include "Common.h"
#include "mxc_errors.h"
#include "i2c.h"
#include "tmr.h"
#include "mxc_delay.h"

#include "UsbPortController.h"
#include "PowerManagement.h"
#include "usb.h"
#include "Menu.h"

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
#include "Globals.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
/* Register offsets */
#define TPS_REG_MODE			0x03
#define TPS_REG_TYPE			0x04
#define TPS_REG_CUSTUSE			0x06
#define TPS_REG_CMD1			0x08
#define TPS_REG_DATA1			0x09
#define TPS_REG_DEVICE_CAPABILITIES	0x0D
#define TPS_REG_VERSION			0x0F
#define TPS_REG_INT_EVENT1		0x14
#define TPS_REG_INT_MASK1		0x16
#define TPS_REG_INT_CLEAR1		0x18
#define TPS_REG_STATUS			0x1A
#define TPS_REG_POWER_PATH_STATUS	0x26
#define TPS_REG_PORT_CONTROL		0x29
#define TPS_REG_BOOT_STATUS		0x2D
#define TPS_REG_BUILD_DESCRIPTION	0x2E
#define TPS_REG_DEVICE_INFO		0x2F
#define TPS_REG_RX_SOURCE_CAPS		0x30
#define TPS_REG_RX_SINK_CAPS		0x31
#define TPS_REG_TX_SOURCE_CAPS		0x32
#define TPS_REG_TX_SINK_CAPS		0x33
#define TPS_REG_ACTIVE_CONTRACT_PDO	0x34
#define TPS_REG_ACTIVE_CONTRACT_RDO	0x35
#define TPS_REG_POWER_STATUS		0x3F
#define TPS_REG_PD_STATUS		0x40
#define TPS_REG_TYPEC_STATE		0x69
#define TPS_REG_SLEEP_CONFIG	0x70
#define TPS_REG_GPIO_STATUS		0x72
#define TPS_REG_MAX			TPS_REG_GPIO_STATUS

#define TPS_MAX_LEN			64

/* 4CC (4 Char Command) Task return code */
#define TPS_TASK_COMPLETED_SUCCESSFULLY		0x0
#define TPS_TASK_TIMEOUT_OR_ABRT			0x1
#define TPS_TASK_REJECTED					0x3
#define TPS_TASK_REJECTED_RX_BUF_LOCKED		0x4

#define TPS_TASK_BPMS_INVALID_BUNDLE_SIZE	0x4
#define TPS_TASK_BPMS_INVALID_SLAVE_ADDR	0x5
#define TPS_TASK_BPMS_INVALID_TIMEOUT		0x6

/* PBMc data out */
#define TPS_PBMC_RC	0 /* Return code */
#define TPS_PBMC_DPCS	2 /* device patch complete status */

/* invalid cmd == !CMD */
#define INVALID_CMD(_cmd_)		(_cmd_ == 0x444d4321)

/* 4 Characters Commands (4CC)*/
#define TPS_4CC_PBMS	"PBMs" /* Start Patch Burst Mode Download Sequence */
#define TPS_4CC_PBMC	"PBMc" /* Patch Burst Mode Download Complete */
#define TPS_4CC_PBME	"PBMe" /* End Patch Burst Mode Download Sequence */
#define TPS_4CC_GO2P	"GO2P" /* Go to Patch mode */
#define TPS_4CC_SWSK	"SWSk" /* Swap to sink power role */
#define TPS_4CC_SWSR	"SWSr" /* Swap to source power role */
#define TPS_4CC_SWUF	"SWUF" /* Swap to up facing stream (device) */
#define TPS_4CC_SWDF	"SWDF" /* Swap to down facing stream (host) */
#define TPS_4CC_DBFG	"DBfg" /* Clear dead battery flag */
#define TPS_4CC_GAID	"GAID" /* Cold reset */
#define TPS_4CC_GSKC	"GSkC" /* Get sink capabilities */
#define TPS_4CC_GSRC	"GSrC" /* Get source capabilities */
#define TPS_4CC_SSRC	"SSrC" /* Send source capabilities */

/*
 * Address used in PBMs command where address would be invalid when
 * 0x00 or I2C client slave address based on ADCINx.
 * pg.48 TPS2575 Host Interface Technical Reference
 * Manual (Rev. A)
 */
#define TPS_BUNDLE_SLAVE_ADDR	0x40 // Poorly documented, needs to be one of the possible TPS2575 I2C device addresses, but not the one configured

/*
 * BPMs task timeout, recommended 5 seconds
 * pg.48 TPS2575 Host Interface Technical Reference
 * Manual (Rev. A)
 */
#define TPS_BUNDLE_TIMEOUT	0x32

enum {
	TPS_MODE_APP,
	TPS_MODE_BOOT,
	TPS_MODE_PTCH,
};

static const char *const tps25750_modes[] = {
	[TPS_MODE_APP]	= "APP ",
	[TPS_MODE_BOOT]	= "BOOT",
	[TPS_MODE_PTCH]	= "PTCH",
};

struct tps25750 {
	struct device *dev;
	//struct i2c_client *client;
	//struct regmap *regmap;
	//struct mutex lock;

	//struct typec_port *port;
	//struct typec_partner *partner;
	//struct usb_role_switch *role_sw;

	//struct power_supply *psy;

	uint32_t status_reg;
	uint32_t max_source_current;
	enum typec_data_role role;
};

enum power_supply_property tps25750_psy_props[] = {
	POWER_SUPPLY_PROP_USB_TYPE,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_CURRENT_MAX
};

#if 0
enum power_supply_usb_type tps25750_psy_usb_types[] = {
	POWER_SUPPLY_USB_TYPE_C,
	POWER_SUPPLY_USB_TYPE_PD
};
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static int tps25750_block_write_raw(struct tps25750 *tps, uint8_t *data, size_t len)
{
	return (WriteI2CDevice(MXC_I2C0, I2C_ADDR_USBC_PORT_CONTROLLER, data, len, NULL, 0));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static int tps25750_block_read(struct tps25750 *tps, uint8_t reg, void *val, size_t len)
{
	int ret;
	uint8_t data[TPS_MAX_LEN + 1];

	if (len + 1 > TPS_MAX_LEN)
		return E_INVALID;

	// Read in the desired bytes plus the byte count (+1)
	ret = WriteI2CDevice(MXC_I2C0, I2C_ADDR_USBC_PORT_CONTROLLER, &reg, sizeof(uint8_t), data, (len + 1));

	if (ret)
		return ret;

#if 0 /* Don't like this logic, can read shorter than the byte count */
	if (data[0] < len)
		return -EIO;
#endif

	// Copy over data minus the byte count
	memcpy(val, &data[1], len);
	return 0;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static int tps25750_block_write(struct tps25750 *tps, uint8_t reg, const void *val, size_t len)
{
	uint8_t data[TPS_MAX_LEN + 1];

	if (len + 1 > TPS_MAX_LEN)
		return E_INVALID;

	data[0] = reg;
	data[1] = len;
	memcpy(&data[2], val, len);

	return (WriteI2CDevice(MXC_I2C0, I2C_ADDR_USBC_PORT_CONTROLLER, &data[0], (len + 2), NULL, 0));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static int tps25750_read(struct tps25750 *tps, uint8_t reg, uint8_t *val)
{
	return tps25750_block_read(tps, reg, val, sizeof(uint8_t));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int tps25750_read16(struct tps25750 *tps, uint8_t reg, uint16_t *val)
{
	return tps25750_block_read(tps, reg, val, sizeof(uint16_t));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static int tps25750_read32(struct tps25750 *tps, uint8_t reg, uint32_t *val)
{
	return tps25750_block_read(tps, reg, val, sizeof(uint32_t));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static int tps25750_wait_cmd_complete(struct tps25750 *tps, unsigned long timeout)
{
	int ret;
	uint32_t val;

	//timeout = jiffies + msecs_to_jiffies(timeout);

	do {
		ret = tps25750_read32(tps, TPS_REG_CMD1, &val);
		if (ret)
			return ret;

		if (INVALID_CMD(val))
			return E_INVALID;

#if 0
		if (time_is_before_jiffies(timeout))
			return -ETIMEDOUT;
#endif

#if 0 /* Test */
		debugRaw("<%d>", val);
#endif

		// Delay 10ms
		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(10));
	} while (val);

	return (0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static int tps25750_exec_cmd(struct tps25750 *tps, const char *cmd, size_t in_len, uint8_t *in_data, size_t out_len, uint8_t *out_data, uint8_t response_delay_ms, uint32_t cmd_timeout_ms)
{
	int ret;
	uint8_t dummy[3] = { };

	if (!out_len || !out_data)
		return E_INVALID;

	if (in_len) {
		ret = tps25750_block_write(tps, TPS_REG_DATA1, in_data, in_len);
		if (ret)
			return ret;
	}
#if 1 /* Original */
	else {
		/*
		 * For some reason, if no data is written to
		 * TPS_REG_DATA1 before sending 4CC, then 4CC would fail
		 */
		dummy[0] = TPS_REG_DATA1;
#if 0 /* Test */
		dummy[1] = 0;
		dummy[2] = 0;
#endif
		ret = tps25750_block_write_raw(tps, dummy, sizeof(dummy));
		if (ret)
			return ret;
	}
#else
	UNUSED(dummy);
#endif

	ret = tps25750_block_write(tps, TPS_REG_CMD1, cmd, 4);
	if (ret)
		return ret;

	ret = tps25750_wait_cmd_complete(tps, cmd_timeout_ms);

	if (ret)
		return ret;

	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(response_delay_ms));

	ret = tps25750_block_read(tps, TPS_REG_DATA1, out_data, out_len);
	if (ret)
		return ret;

	return (0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static int tps25750_exec_normal_cmd(struct tps25750 *tps, const char *cmd)
{
	int ret;
	uint8_t rc;

	ret = tps25750_exec_cmd(tps, cmd, 0, NULL, 1, &rc, 0, 1000);
	if (ret)
		return ret;

	switch (rc) {
	case TPS_TASK_COMPLETED_SUCCESSFULLY:
		return E_SUCCESS;
	case TPS_TASK_TIMEOUT_OR_ABRT:
		return E_TIME_OUT;
	case TPS_TASK_REJECTED:
		return E_NONE_AVAIL;
	case TPS_TASK_REJECTED_RX_BUF_LOCKED:
		return E_BUSY;
	default:
		break;
	}

	return (E_COMM_ERR);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int tps25750_exec_patch_cmd_pbms(struct tps25750 *tps, uint8_t *in_data, size_t in_len)
{
	int ret;
	uint8_t rc;

	ret = tps25750_exec_cmd(tps, TPS_4CC_PBMS, in_len, in_data, 1, &rc, 0, TPS_BUNDLE_TIMEOUT * 100);
	if (ret)
		return ret;

	switch (rc) {
	case TPS_TASK_BPMS_INVALID_BUNDLE_SIZE:
		debugErr("USB Port Controller: Invalid fw size\r\n");
		return E_INVALID;
	case TPS_TASK_BPMS_INVALID_SLAVE_ADDR:
		debugErr("USB Port Controller: Invalid slave address\r\n");
		return E_INVALID;
	case TPS_TASK_BPMS_INVALID_TIMEOUT:
		debugErr("USB Port Controller: Timed out\r\n");
		return E_TIME_OUT;
	default:
		break;
	}

	return (0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static int tps25750_complete_patch_process(struct tps25750 *tps)
{
	int ret;
	uint8_t out_data[40];

	ret = tps25750_exec_cmd(tps, TPS_4CC_PBMC, 0, NULL, sizeof(out_data), out_data, 20, 2000);
	if (ret)
		return ret;

	if (out_data[TPS_PBMC_RC]) {
		debugErr("USB Port Controller: PBMc failed: %u\r\n", out_data[TPS_PBMC_RC]);
		debugRaw("\r\nUSB Port Controller: PBMc Outout data: ");
		for (uint8_t i = 0; i < 40; i++) { debugRaw("%02x ", out_data[i]); }
		debugRaw("\r\n");

#if 0 /* Test to read status */
//extern uint8_t usbIsrActive;
	//if (usbIsrActive)
	{
		//usbIsrActive = NO;
		tps25750_block_read(tps, TPS_REG_INT_EVENT1, g_debugBuffer, 11); debug("USB Port Controller: Int Event1 Register is 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3], g_debugBuffer[4], g_debugBuffer[5], g_debugBuffer[6], g_debugBuffer[7], g_debugBuffer[8], g_debugBuffer[9], g_debugBuffer[10]);
		tps25750_block_read(tps, TPS_REG_STATUS, g_debugBuffer, 5); debug("USB Port Controller: Status Register is 0x%x 0x%x 0x%x 0x%x 0x%x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3], g_debugBuffer[4]);
		memset(g_debugBuffer, 0xFF, 11); tps25750_block_write(tps, TPS_REG_INT_CLEAR1, g_debugBuffer, 11);
		tps25750_block_read(tps, TPS_REG_INT_EVENT1, g_debugBuffer, 11); debug("USB Port Controller: Int Event1 Register is 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3], g_debugBuffer[4], g_debugBuffer[5], g_debugBuffer[6], g_debugBuffer[7], g_debugBuffer[8], g_debugBuffer[9], g_debugBuffer[10]);
	}
#endif

#if 1 /* Normal */
		return E_COMM_ERR;
#else /* Test */
	debug("USB Port Controller: Sending PBMe command...\r\n");
	ret = tps25750_exec_normal_cmd(tps, TPS_4CC_PBME);
	if (ret)
		return ret;
#endif
	}

	if (out_data[TPS_PBMC_DPCS]) {
		debugErr("USB Port Controller: Failed device patch complete status: %u\r\n", out_data[TPS_PBMC_DPCS]);
		return E_COMM_ERR;
	}
	return (0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static int tps25750_get_reg_boot_status(struct tps25750 *tps, uint64_t *status)
{
	int ret;

	ret = tps25750_block_read(tps, TPS_REG_BOOT_STATUS, status, 5);
	if (ret) {
		debugErr("USB Port Controller: failed to get boot status %d\r\n", ret);
		return ret;
	}

	return (0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static int tps25750_get_mode(struct tps25750 *tps)
{
	char mode[5] = { };
	int ret;

	ret = tps25750_read32(tps, TPS_REG_MODE, (void *)mode);
	if (ret)
		return ret;

#if 0 /* original logic doesn't look to work correctly */
	//ret = match_string(tps25750_modes, ARRAY_SIZE(tps25750_modes), mode);
	ret = strcmp(tps25750_modes[TPS_MODE_APP], mode);
	if (ret) { ret = strcmp(tps25750_modes[TPS_MODE_BOOT], mode); }
	if (ret) { ret = strcmp(tps25750_modes[TPS_MODE_PTCH], mode); }
	if (ret) { ret = E_INVALID; }
#else
	ret = -1;
	if (strcmp(tps25750_modes[TPS_MODE_APP], mode) == 0) { ret = TPS_MODE_APP; }
	else if (strcmp(tps25750_modes[TPS_MODE_BOOT], mode) == 0) { ret = TPS_MODE_BOOT; }
	else if (strcmp(tps25750_modes[TPS_MODE_PTCH], mode) == 0) { ret = TPS_MODE_PTCH; }
#endif

	if (ret < 0) {
		debugErr("USB Port Controller: unsupported mode \"%s\"\r\n", mode);
		return E_NO_DEVICE;
	}

	return (ret);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static int tps2750_is_mode(struct tps25750 *tps, uint8_t mode)
{
	int ret;

	ret = tps25750_get_mode(tps);
	if (ret < 0)
		return ret;

	return (ret == mode);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static int tps25750_abort_patch_process(struct tps25750 *tps)
{
	int ret;

	ret = tps25750_exec_normal_cmd(tps, TPS_4CC_PBME);
	if (ret)
		return ret;

	ret = tps2750_is_mode(tps, TPS_MODE_PTCH);
	if (ret != 1) {
		debugErr("USB Port Controller: failed to switch to \"PTCH\" mode\r\n");
		if (ret < 0)
			return ret;
		return E_FAIL;
	}

	return (0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void tps25750_int_status_and_clear(void)
{
	struct tps25750 tps;

	tps25750_block_read(&tps, TPS_REG_INT_EVENT1, g_debugBuffer, 11); debug("USB Port Controller: Int Event1 Register is %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3], g_debugBuffer[4], g_debugBuffer[5], g_debugBuffer[6], g_debugBuffer[7], g_debugBuffer[8], g_debugBuffer[9], g_debugBuffer[10]);
	memset(g_debugBuffer, 0xFF, 11); tps25750_block_write(&tps, TPS_REG_INT_CLEAR1, g_debugBuffer, 11);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void tps25750_disable_all_int(void)
{
	struct tps25750 tps;

	memset(g_debugBuffer, 0x00, 11); tps25750_block_write(&tps, TPS_REG_INT_MASK1, g_debugBuffer, 11);
	debug("USB Port Controller: Disable all Interrupts\r\n");
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void tps25750_enable_all_int(void)
{
	struct tps25750 tps;

	memset(g_debugBuffer, 0xFF, 11); tps25750_block_write(&tps, TPS_REG_INT_MASK1, g_debugBuffer, 11);
	debug("USB Port Controller: Enable all Interrupts\r\n");
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int tps25750_issue_get_sink_cap(void)
{
	struct tps25750 tps;
	int ret;

	ret = tps25750_exec_normal_cmd(&tps, TPS_4CC_GSKC);

	return (ret);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t* tps25750_get_rx_sink_cap(void)
{
	struct tps25750 tps;

	tps25750_block_read(&tps, TPS_REG_RX_SINK_CAPS, &g_spareBuffer, 31);
	return (g_spareBuffer);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void tps25750_set_data_role(struct tps25750 *tps, enum typec_data_role role, bool connected)
{
	if (role == TYPEC_HOST)
		tps->role = USB_ROLE_HOST;
	else
		tps->role = USB_ROLE_DEVICE;

	if (!connected)
		tps->role = USB_ROLE_NONE;

#if 0 /* Todo: fill in equivalent */
	usb_role_switch_set_role(tps->role_sw, tps->role);
	typec_set_data_role(tps->port, role);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static int tps25750_connect(struct tps25750 *tps, uint32_t status)
{
#if 0 /* Todo: complete connect operation */
	struct typec_partner_desc desc;
	enum typec_pwr_opmode mode;
	uint16_t pwr_status;
	int ret;

	if (!IS_ERR(tps->partner))
		typec_unregister_partner(tps->partner);

	ret = tps25750_read16(tps, TPS_REG_POWER_STATUS, &pwr_status);
	if (ret < 0)
		return ret;

	mode = TPS_REG_POWER_STATUS_TYPEC_CURRENT(pwr_status);

	desc.usb_pd = mode == TYPEC_PWR_MODE_PD;
	desc.accessory = TYPEC_ACCESSORY_NONE;
	desc.identity = NULL;

	typec_set_pwr_opmode(tps->port, mode);
	typec_set_pwr_role(tps->port, TPS_REG_STATUS_PORT_ROLE(status));
	tps25750_set_data_role(tps, TPS_REG_STATUS_DATA_ROLE(status), true);

	tps->partner = typec_register_partner(tps->port, &desc);
	if (IS_ERR(tps->partner))
		return PTR_ERR(tps->partner);

	power_supply_changed(tps->psy);
#endif

	return (0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static void tps25750_disconnect(struct tps25750 *tps, uint32_t status)
{
#if 0 /* Todo: complete disconnect operation */
	if (!IS_ERR(tps->partner))
		typec_unregister_partner(tps->partner);
	tps->partner = NULL;
	typec_set_pwr_opmode(tps->port, TYPEC_PWR_MODE_USB);
	typec_set_pwr_role(tps->port, TPS_REG_STATUS_PORT_ROLE(status));
	tps25750_set_data_role(tps, TPS_REG_STATUS_DATA_ROLE(status), false);
	power_supply_changed(tps->psy);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static void tps25750_handle_plug_event(struct tps25750 *tps, uint32_t status)
{
	int ret;

	if (TPS_REG_STATUS_PLUG_PRESENT(status)) {
		ret = tps25750_connect(tps, status);
		if (ret)
			debugErr("USB Port Controller: failed to register partner\r\n");
	} else {
		tps25750_disconnect(tps, status);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static bool tps25750_has_role_changed(struct tps25750 *tps, uint32_t status)
{
	status ^= tps->status_reg;

	return (TPS_REG_STATUS_PORT_ROLE(status) || TPS_REG_STATUS_DATA_ROLE(status));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
//irqreturn_t tps25750_interrupt(int irq, void *data)
int tps25750_interrupt(int irq, void *data)
{
	struct tps25750 *tps = data;
	uint32_t status;
	int ret;
	uint32_t event[3] = { };
	//uint8_t no_events = 0;

	//mutex_lock(&tps->lock);

	/* events reg size is 11 bytes */
	ret = tps25750_block_read(tps, TPS_REG_INT_EVENT1, event,
				  sizeof(event) - 1);

	if (ret) {
		debugErr("USB Port Controller: failed to read events ret: %d\r\n", ret);
		goto err_unlock;
	}

	if (!event[0] && !event[1] && !event[2]) {
		//no_events = 1;
		goto err_unlock;
	}

	if (!(TPS_REG_EVENT1_STATUS_UPDATE & event[0]))
		goto clear_events;

	ret = tps25750_read32(tps, TPS_REG_STATUS, &status);
	if (ret) {
		debugErr("USB Port Controller: failed to read status\r\n");
		goto clear_events;
	}

	/*
	 * data/port roles could be updated independently after
	 * a plug event. Therefore, we need to check
	 * for pr/dr status change to set TypeC dr/pr accordingly.
	 */
	if (TPS_REG_EVENT1_PLUG_INSERT_OR_REMOVAL & event[0] ||
		tps25750_has_role_changed(tps, status))
		tps25750_handle_plug_event(tps, status);

	tps->status_reg = status;
clear_events:
	/* clear reg size is 11 bytes */
	tps25750_block_write(tps, TPS_REG_INT_CLEAR1, event, sizeof(event) - 1);

err_unlock:
	//mutex_unlock(&tps->lock);

#if 0
	if (no_events)
		return IRQ_NONE;

	return IRQ_HANDLED;
#else
	return (0);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
//static int tps25750_dr_set(struct typec_port *port, enum typec_data_role role)
int tps25750_dr_set(struct tps25750* tps, enum typec_data_role role)
{
	int ret;
	uint32_t status;
	const char *cmd = (role == TYPEC_DEVICE) ? TPS_4CC_SWUF : TPS_4CC_SWDF;

	//mutex_lock(&tps->lock);

	// Attempt to change data role
	ret = tps25750_exec_normal_cmd(tps, cmd);
	if (ret) { goto release_lock; }

	// Read status to see if the controller changed
	ret = tps25750_read32(tps, TPS_REG_STATUS, &status);
	if (ret) { goto release_lock; }

	// Check if the data role did not change
	if (role != TPS_REG_STATUS_DATA_ROLE(status))
	{
		ret = E_FAIL;
		debugErr("USBC Port Controller: Failed to change data role to <%s>\r\n", (role == TYPEC_DEVICE) ? "Device" : "Host");
		goto release_lock;
	}

	tps25750_set_data_role(tps, role, true);

release_lock:
	//mutex_unlock(&tps->lock);

	return (ret);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int tps25750_write_firmware(struct tps25750 *tps, uint8_t *data, size_t len)
{
	int ret = 0;
	//uint8_t addr;
	//int timeout;

	//addr = tps->client->addr;
	//timeout = tps->client->adapter->timeout;

	/*
	 * Writing the patch might take some time as we are
	 * writing the whole patch at once.
	 * Tested using 5 seconds at 100kHz seems to work
	 */
	//tps->client->adapter->timeout = 5000; //msecs_to_jiffies(5000);
	//tps->client->addr = TPS_BUNDLE_SLAVE_ADDR;

#if 0 /* Original */
	ret = tps25750_block_write_raw(tps, data, len);
#elif 0 /* Test chopped up segment write */
	while (len > 32)
	{
		// Write 32 byte block length
		ret = tps25750_block_write_raw(tps, data, 32);
		if (ret) { debugErr("USB Port Controller: Write firmware blocks failed\r\n"); return (ret); }
		data += 32;
		len -= 32;
	}

	if (len)
	{
		// Write remaining length
		ret = tps25750_block_write_raw(tps, data, len);
		if (ret) { debugErr("USB Port Controller: Write firmware blocks failed\r\n"); return (ret); }
	}
#elif 1 /* Test Alt whole */
	ret = WriteI2CDevice(MXC_I2C0, TPS_BUNDLE_SLAVE_ADDR, data, len, NULL, 0);
#else
	while (len)
	{
		// Write 32 byte block length
		//ret = tps25750_block_write_raw(tps, data, 1);
		ret = WriteI2CDevice(MXC_I2C0, TPS_BUNDLE_SLAVE_ADDR, data, len, NULL, 0);
		if (ret) { debugErr("USB Port Controller: Write firmware blocks failed\r\n"); return (ret); }
		data++;
		len--;
	}
#endif

	//tps->client->addr = addr;
	//tps->client->adapter->timeout = timeout;

	return (ret);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
extern const char tps25750x_lowRegion_i2c_array[];
extern int gSizeLowRegionArray;
static int tps25750_start_patch_burst_mode(struct tps25750 *tps)
{
#if 0 /* Skip until patch ready */
	int ret = 0;
#else /* Todo: fill in equivalent */
	int ret;
	struct
	{
		uint32_t fw_size;
		uint8_t i2c_slave_addr;
		uint8_t timeout;
	} __packed pbms_in_data;

	pbms_in_data.fw_size = gSizeLowRegionArray;
	pbms_in_data.i2c_slave_addr = TPS_BUNDLE_SLAVE_ADDR;
	pbms_in_data.timeout = TPS_BUNDLE_TIMEOUT;

#if 0 /* Test to read status */
extern uint8_t usbIsrActive;
	if (usbIsrActive)
	{
		usbIsrActive = NO;
		tps25750_block_read(tps, TPS_REG_INT_EVENT1, g_debugBuffer, 11); debug("USB Port Controller: Int Event1 Register is 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3], g_debugBuffer[4], g_debugBuffer[5], g_debugBuffer[6], g_debugBuffer[7], g_debugBuffer[8], g_debugBuffer[9], g_debugBuffer[10]);
		tps25750_block_read(tps, TPS_REG_STATUS, g_debugBuffer, 5); debug("USB Port Controller: Status Register is 0x%x 0x%x 0x%x 0x%x 0x%x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3], g_debugBuffer[4]);
		memset(g_debugBuffer, 0xFF, 11); tps25750_block_write(tps, TPS_REG_INT_CLEAR1, g_debugBuffer, 11);
		tps25750_block_read(tps, TPS_REG_INT_EVENT1, g_debugBuffer, 11); debug("USB Port Controller: Int Event1 Register is 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3], g_debugBuffer[4], g_debugBuffer[5], g_debugBuffer[6], g_debugBuffer[7], g_debugBuffer[8], g_debugBuffer[9], g_debugBuffer[10]);
	}
#endif

	ret = tps25750_exec_patch_cmd_pbms(tps, (uint8_t *)&pbms_in_data, sizeof(pbms_in_data));
	if (ret)
	{
		debugErr("USB Port Controller: Failed Patch Start process with code %d\r\n", ret);
		return (ret);
	}
	else { debug("USB Port Controller: Patch Start process success\r\n"); }

#if 0 /* Test to read status */
extern uint8_t usbIsrActive;
	if (usbIsrActive)
	{
		usbIsrActive = NO;
		tps25750_block_read(tps, TPS_REG_INT_EVENT1, g_debugBuffer, 11); debug("USB Port Controller: Int Event1 Register is 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3], g_debugBuffer[4], g_debugBuffer[5], g_debugBuffer[6], g_debugBuffer[7], g_debugBuffer[8], g_debugBuffer[9], g_debugBuffer[10]);
		tps25750_block_read(tps, TPS_REG_STATUS, g_debugBuffer, 5); debug("USB Port Controller: Status Register is 0x%x 0x%x 0x%x 0x%x 0x%x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3], g_debugBuffer[4]);
		memset(g_debugBuffer, 0xFF, 11); tps25750_block_write(tps, TPS_REG_INT_CLEAR1, g_debugBuffer, 11);
		tps25750_block_read(tps, TPS_REG_INT_EVENT1, g_debugBuffer, 11); debug("USB Port Controller: Int Event1 Register is 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3], g_debugBuffer[4], g_debugBuffer[5], g_debugBuffer[6], g_debugBuffer[7], g_debugBuffer[8], g_debugBuffer[9], g_debugBuffer[10]);
	}
#endif

	ret = tps25750_write_firmware(tps, (uint8_t*)tps25750x_lowRegion_i2c_array, gSizeLowRegionArray);
	if (ret) {
		debugErr("USB Port Controller: Failed to write low region patch of %lu bytes\r\n", gSizeLowRegionArray);
	} else {
		/*
		 * A delay of 500us is required after the firmware is written
		 * based on pg.62 in TPS25750 Host Interface Technical
		 * Reference Manual
		 */
		MXC_TMR_Delay(MXC_TMR0, 500);
		ret = 0;
		debug("USB Port Controller: Low region patch write success (%d bytes)\r\n", gSizeLowRegionArray);
	}
#endif

	return (ret);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static int tps25750_apply_patch(struct tps25750 *tps)
{
	int ret;
	//unsigned long timeout;

#if 0 /* Skip until patch is verified (unitl then the path file will not be in source control) */
	return (-1);
#endif

#if 1 /* Added due to datasheet flowchart */
	tps25750_block_read(tps, TPS_REG_INT_EVENT1, g_debugBuffer, 11);
	if (g_debugBuffer[10] & 0x02)
	{
		debug("USB Port Controller: Validated Ready for Patch flag set\r\n");
	}
	else
	{
		debugErr("USB Port Controller: Ready for Patch flag not set\r\n");
	}
#endif

	ret = tps2750_is_mode(tps, TPS_MODE_PTCH);
	if (ret != 1)
		return ret;

#if 0 /* Skip for now, purpose? */
	uint64_t boot_status;
	ret = tps25750_get_reg_boot_status(tps, &boot_status);
	if (ret)
		return ret;
#endif

#if 0 /* No EEPROM, skip */
	// Nothing to be done if the configuration is being loaded from EERPOM
	if (TPS_REG_BOOT_STATUS_I2C_EEPROM_PRESENT(boot_status))
		goto wait_for_app;
#endif

	debug("USB Port Controller: Start Patch Burst Mode...\r\n");
	ret = tps25750_start_patch_burst_mode(tps);
	if (ret) {
		tps25750_abort_patch_process(tps);
		return ret;
	}

#if 0 /* Test to read status */
extern uint8_t usbIsrActive;
	if (usbIsrActive)
	{
		usbIsrActive = NO;
		tps25750_block_read(tps, TPS_REG_INT_EVENT1, g_debugBuffer, 11); debug("USB Port Controller: Int Event1 Register is 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3], g_debugBuffer[4], g_debugBuffer[5], g_debugBuffer[6], g_debugBuffer[7], g_debugBuffer[8], g_debugBuffer[9], g_debugBuffer[10]);
		tps25750_block_read(tps, TPS_REG_STATUS, g_debugBuffer, 5); debug("USB Port Controller: Status Register is 0x%x 0x%x 0x%x 0x%x 0x%x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3], g_debugBuffer[4]);
		memset(g_debugBuffer, 0xFF, 11); tps25750_block_write(tps, TPS_REG_INT_CLEAR1, g_debugBuffer, 11);
		tps25750_block_read(tps, TPS_REG_INT_EVENT1, g_debugBuffer, 11); debug("USB Port Controller: Int Event1 Register is 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3], g_debugBuffer[4], g_debugBuffer[5], g_debugBuffer[6], g_debugBuffer[7], g_debugBuffer[8], g_debugBuffer[9], g_debugBuffer[10]);
	}
#endif

	debug("USB Port Controller: Complete Patch Burst Mode...\r\n");
	ret = tps25750_complete_patch_process(tps);
	if (ret)
		return ret;

#if 0 /* No EEPROM, skip */
wait_for_app:
	//timeout = 1000; jiffies + msecs_to_jiffies(1000);
#endif

	do {
		ret = tps2750_is_mode(tps, TPS_MODE_APP);
		if (ret < 0)
			return ret;

#if 0 /* Todo: fill in equivalent */
		if (time_is_before_jiffies(timeout))
			return E_TIME_OUT;
#endif

		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(10));

	} while (ret != 1);

	if (ret == 0)
	{
		debugErr("USB Port Controller: Controller mode switch failed after patch\r\n");
		return E_FAIL;
	}

	debug("USB Port Controller: Controller switched to \"APP\" mode\r\n");

	return (0);
};

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
//static int tps25750_pr_set(struct typec_port *port, enum typec_role role)
int tps25750_pr_set(struct tps25750* tps, enum typec_role role)
{
	int ret;
	uint32_t status;
	const char *cmd = (role == TYPEC_SINK) ? TPS_4CC_SWSK : TPS_4CC_SWSR;

#if /* New board */ ((HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_PRODUCTION) || (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_RELEASE))
extern void USBHostControllerSetMuxAndSource(uint8_t state);
	if (role == TYPEC_SINK) { USBHostControllerSetMuxAndSource(OFF); }
	else /* (role == TYPEC_SOURCE) */ { USBHostControllerSetMuxAndSource(ON); }
#elif /* Old board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN)
	// Determine role for sourcing VBUS from the 5V Buck and set before swapping mode
	if (role == TYPEC_SOURCE) { PowerControl(USB_SOURCE_ENABLE, ON); } // Delay needed to let power settle?
	else { PowerControl(USB_SOURCE_ENABLE, OFF); }
#endif

	//mutex_lock(&tps->lock);

	// Attempt to change power role
	ret = tps25750_exec_normal_cmd(tps, cmd);
	if (ret) { goto release_lock; }

	// Read status to see if controller updated
	ret = tps25750_read32(tps, TPS_REG_STATUS, &status);
	if (ret) { goto release_lock; }

	// Check if the power role did not change
	if (role != TPS_REG_STATUS_PORT_ROLE(status))
	{
		ret = E_FAIL;
		debugErr("USBC Port Controller: Failed to change power role to <%s>\r\n", (role == TYPEC_SINK) ? "Sink" : "Source");
		goto release_lock;
	}

	// Todo: fill in equivalent
	//typec_set_pwr_role(tps->port, role);

release_lock:
	//mutex_unlock(&tps->lock);

	return (ret);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int tps25750_find_max_source_curr(struct tps25750 *tps)
{
	int ret;
	int i;
	uint8_t buf[31];
	uint8_t npdo;
	uint32_t pdo;

	ret = tps25750_block_read(tps, TPS_REG_TX_SOURCE_CAPS, buf, sizeof(buf));
	if (ret)
		return ret;

	/*
	 * The first byte is header which contains number of
	 * valid PDOs. Up to 7.
	 */
	npdo = TPS_PDO_NUM_VALID_PDOS(buf[0]);

	/*
	 * Each PDO is 4 byte in length where each PDO starts
	 * as following:
	 * PDO1: byte 3
	 * PDO2: byte 7
	 *	...
	 * PDO7: byte 27
	 * See pg.28 in TPS25750 Host Interface Technical Reference
	 * Manual (Rev. A)
	 */
	for (i = 1; i <= npdo; i++)
	{
		memcpy(&pdo, &buf[i*4 - 1], sizeof(pdo));

		// Todo: fill in equivalent
		//tps->max_source_current = max_t(u32, tps->max_source_current, TPS_PDO_MAX_CURRENT(pdo));
	}

	return (0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static int tps25750_clear_dead_battery(struct tps25750 *tps)
{
	int ret;

	ret = tps25750_exec_normal_cmd(tps, TPS_4CC_DBFG);
	if (ret) {
		debugErr("USB Port Controller: failed to clear dead battery %d\r\n", ret);
		return ret;
	}

	return (0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static int tps25750_init(struct tps25750 *tps)
{
	int ret;
	uint64_t boot_status;

	tps->status_reg = 0;

	ret = tps2750_is_mode(tps, TPS_MODE_BOOT);
	if (ret == 1) {
		debugWarn("USB Port Controller: Device booting in dead battery\r\n");
		return 0;
	}

	ret = tps25750_apply_patch(tps);
	if (ret)
		return ret;

	/*
	 * The dead battery flag may be triggered when the controller
	 * port is connected to a device that can source power and
	 * attempts to power up both the controller and the board it is on.
	 * To restore controller functionality, it is necessary to clear
	 * this flag
	 */
	ret = tps25750_get_reg_boot_status(tps, &boot_status);
	if (ret)
		return ret;

	if (TPS_REG_BOOT_STATUS_DEAD_BATTERY_FLAG(boot_status))
		return tps25750_clear_dead_battery(tps);

	return (0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0
static int tps25750_psy_get_prop(struct power_supply *psy, enum power_supply_property psp, union power_supply_propval *val)
{
	// Todo: fill in equivalent
	struct tps25750 *tps = NULL; //power_supply_get_drvdata(psy);
	uint16_t pwr_status;
	int ret;

	ret = tps25750_read16(tps, TPS_REG_POWER_STATUS, &pwr_status);
	if (ret)
		return ret;

	switch (psp) {
	case POWER_SUPPLY_PROP_USB_TYPE:
		if (TPS_REG_POWER_STATUS_TYPEC_CURRENT(pwr_status) == TPS_TYPEC_CURRENT_PD)
			val->intval = POWER_SUPPLY_USB_TYPE_PD;
		else
			val->intval = POWER_SUPPLY_USB_TYPE_C;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = TPS_REG_POWER_STATUS_POWER_CONNECTION(pwr_status);
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		/*
		 * maximum possible pdo source current is
		 * 1023 (10.23) A.
		 */
		val->intval = tps->max_source_current * 10000;
		break;
	default:
		ret = E_FAIL;
	}

	return ret;
}
#endif

#if 0
static const struct power_supply_desc tps25750_psy_desc = {
	.name = "tps25750-psy",
	.type = POWER_SUPPLY_TYPE_USB,
	.usb_types = tps25750_psy_usb_types,
	.num_usb_types = ARRAY_SIZE(tps25750_psy_usb_types),
	.properties = tps25750_psy_props,
	.num_properties = ARRAY_SIZE(tps25750_psy_props),
	.get_property = tps25750_psy_get_prop,
};

static const struct typec_operations tps25750_ops = {
	.dr_set = tps25750_dr_set,
	.pr_set = tps25750_pr_set,
};
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
//static int tps25750_probe(struct i2c_client *client)
int tps25750_probe(void)
{
	//struct power_supply_config psy_cfg = { };
	//struct typec_capability typec_cap = { };
	struct tps25750 tpsMem;
	struct tps25750 *tps = &tpsMem; //NULL;
	//struct fwnode_handle *fwnode;
	int ret;
	uint8_t pd_status;
	//const char *data_role;

	//mutex_init(&tps->lock);

	//tps->client = client;
	//tps->dev = &client->dev;

	// Make sure I2C is initialized

	ret = tps25750_init(tps);
	if (ret)
		return ret;

	ret = tps25750_read(tps, TPS_REG_PD_STATUS, &pd_status);
	if (ret)
		goto err_remove_patch;

#if 0 /* Todo: fill in equivalent */
	fwnode = device_get_named_child_node(&client->dev, "connector");
	if (!fwnode) {
		ret = -ENODEV;
		goto err_remove_patch;
	}

	tps->role_sw = fwnode_usb_role_switch_get(fwnode);
	if (IS_ERR(tps->role_sw)) {
		ret = PTR_ERR(tps->role_sw);
		goto err_fwnode_put;
	}

	ret = fwnode_property_read_string(fwnode, "data-role", &data_role);
	if (ret) {
		dev_err(tps->dev, "data-role not found: %d\r\n", ret);
		goto err_role_put;
	}

	ret = typec_find_port_data_role(data_role);
	if (ret < 0) {
		dev_err(tps->dev, "unknown data-role: %s\r\n", data_role);
		goto err_role_put;
	}

	typec_cap.data = ret;
	typec_cap.revision = USB_TYPEC_REV_1_3;
	typec_cap.pd_revision = 0x300;
	typec_cap.driver_data = tps;
	typec_cap.ops = &tps25750_ops;
	typec_cap.fwnode = fwnode;
	typec_cap.prefer_role = TYPEC_NO_PREFERRED_ROLE;

	switch (TPS_REG_PD_STATUS_PORT_TYPE(pd_status)) {
	case TPS_PORT_TYPE_SINK_SOURCE:
	case TPS_PORT_TYPE_SOURCE_SINK:
		typec_cap.type = TYPEC_PORT_DRP;
		break;
	case TPS_PORT_TYPE_SINK:
		typec_cap.type = TYPEC_PORT_SNK;
		break;
	case TPS_PORT_TYPE_SOURCE:
		typec_cap.type = TYPEC_PORT_SRC;
		break;
	default:
		ret = -ENODEV;
		goto err_role_put;
	}

	psy_cfg.fwnode = dev_fwnode(tps->dev);
	psy_cfg.drv_data = tps;

	tps->psy = devm_power_supply_register(tps->dev,
						  &tps25750_psy_desc,
						  &psy_cfg);
	if (IS_ERR(tps->psy)) {
		ret = PTR_ERR(tps->psy);
		goto err_role_put;
	}

	tps->port = typec_register_port(&client->dev, &typec_cap);
	if (IS_ERR(tps->port)) {
		ret = PTR_ERR(tps->port);
		goto err_role_put;
	}


	ret = devm_request_threaded_irq(&client->dev, client->irq, NULL,
					tps25750_interrupt,
					IRQF_SHARED | IRQF_ONESHOT,
					dev_name(&client->dev), tps);
	if (ret)
		goto err_port_unregister;

	tps25750_find_max_source_curr(tps);
	i2c_set_clientdata(client, tps);
	fwnode_handle_put(fwnode);

	return (0);

err_port_unregister:
	//typec_unregister_port(tps->port);
err_role_put:
	//usb_role_switch_put(tps->role_sw);
err_fwnode_put:
	//fwnode_handle_put(fwnode);
#endif

err_remove_patch:
	tps25750_exec_normal_cmd(tps, TPS_4CC_GAID);

	return (ret);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void tps25750_remove(struct tps25750 *tps)
{
#if 0 /* Todo: fill in equivalent */
	struct tps25750 *tps = i2c_get_clientdata(client);

	tps25750_disconnect(tps, 0);
	typec_unregister_port(tps->port);
	usb_role_switch_put(tps->role_sw);
#endif

	/* clear the patch by a hard reset */
	tps25750_exec_normal_cmd(tps, TPS_4CC_GAID);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestUSBCPortController(void)
{
	struct tps25750 tps;
	uint8_t mode;
	char commType[5];
	uint32_t version;
	char deviceInfo[41];
	uint8_t scratch;

	debug("USBC Port Controller: Test device access...\r\n");

	/*
		The host must not read or write most registers while the device is in the 'BOOT' or 'PTCH' mode. Only the following registers are available	in 'BOOT' and 'PTCH' modes:
		the 4CC patch commands
		MODE (0x03)
		TYPE (0x04), VERSION (0x0F)
		CMD1 (0x08), DATA1 (0x09)
		DEVICE_CAPABILITIES (0x0D)
		INT_EVENT1 (0x14), INT_MASK1 (0x16), and INT_CLEAR1 (0x18)
		BOOT_STATUS (0x2D)
		DEVICE_INFO (0x2F)
	*/

	mode = tps25750_get_mode(&tps);
	if (mode < 0) { debugErr("USBC Port Controller: Startup mode is unknown\r\n"); }
	else { debug("USBC Port Controller: Startup mode is %s\r\n", tps25750_modes[mode]); }

	memset(commType, 0, sizeof(commType));
	if (tps25750_read32(&tps, TPS_REG_TYPE, (void*)commType)) { debugErr("USBC Port Controller: I2C read error\r\n"); }
	else { debug("USBC Port Controller: Type is %s\r\n", (char*)commType); }

	memset(&version, 0, sizeof(version));
	if (tps25750_read32(&tps, TPS_REG_VERSION, (void*)&version)) { debugErr("USBC Port Controller: I2C read error\r\n"); }
	else { debug("USBC Port Controller: Type is %lu (%d.%d.%d)\r\n", version, (((version & 0xFF) << 8) | ((version >> 8) & 0xFF)), ((version >> 16) & 0x00FF), (version >> 24)); }

	memset(deviceInfo, 0, sizeof(deviceInfo));
	if (tps25750_block_read(&tps, TPS_REG_DEVICE_INFO, (void*)deviceInfo, 40)) { debugErr("USBC Port Controller: I2C read error\r\n"); }
	else { debug("USBC Port Controller: Device type info: %s\r\n", (char*)deviceInfo); }

	if (mode == TPS_MODE_APP)
	{
		scratch = 0xAA; tps25750_block_write(&tps, TPS_REG_CUSTUSE, &scratch, 1);
		scratch = 0x00; tps25750_read(&tps, TPS_REG_CUSTUSE, &scratch);
		debug("USBC Port Controller: 1st Custom use byte (scratchpad) test %s\r\n", (scratch == 0xAA) ? "Passed" : "Failed");

		scratch = 0x55; tps25750_block_write(&tps, TPS_REG_CUSTUSE, &scratch, 1);
		scratch = 0x00; tps25750_read(&tps, TPS_REG_CUSTUSE, &scratch);
		debug("USBC Port Controller: 2nd Custom use byte (scratchpad) test %s\r\n", (scratch == 0x55) ? "Passed" : "Failed");
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint32_t USBCPortControllerStatus(void)
{
	struct tps25750 tps;
	uint32_t status;

	tps25750_read32(&tps, TPS_REG_STATUS, &status);
	return ((uint16_t)status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16_t USBCPortControllerPStatus(void)
{
	struct tps25750 tps;
	uint16_t status;

	tps25750_read16(&tps, TPS_REG_POWER_STATUS, &status);
	return (status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint32_t USBCPortControllerPDStatus(void)
{
	struct tps25750 tps;
	uint32_t status;

	tps25750_read32(&tps, TPS_REG_PD_STATUS, &status);
	return (status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void USBCPortControllerClearIntFlags(void)
{
	struct tps25750 tps;
	uint8_t clearIntFlags[11];

	memset(clearIntFlags, 0xFF, 11);
	tps25750_block_write(&tps, TPS_REG_INT_CLEAR1, clearIntFlags, 11);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void USBCPortControllerReadAndClearInt(void)
{
	struct tps25750 tps;

extern uint8_t usbIsrActive;
	if (usbIsrActive)
	{
		usbIsrActive = NO;
		tps25750_block_read(&tps, TPS_REG_INT_EVENT1, g_debugBuffer, 11); debug("USB Port Controller: Int Event1 Register is 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3], g_debugBuffer[4], g_debugBuffer[5], g_debugBuffer[6], g_debugBuffer[7], g_debugBuffer[8], g_debugBuffer[9], g_debugBuffer[10]);
		memset(g_debugBuffer, 0xFF, 11); tps25750_block_write(&tps, TPS_REG_INT_CLEAR1, g_debugBuffer, 11);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void USBCPortControllerSwapToHost(void)
{
	struct tps25750 tps;

	debug("USB Port Controller: Swap to Host/Source\r\n");
	tps25750_pr_set(&tps, TYPEC_SOURCE);
	tps25750_dr_set(&tps, TYPEC_HOST);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void USBCPortControllerSwapToDevice(void)
{
	struct tps25750 tps;

	debug("USB Port Controller: Swap to Device/Sink\r\n");
	tps25750_pr_set(&tps, TYPEC_SINK);
	tps25750_dr_set(&tps, TYPEC_DEVICE);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int USBCPortControllerGetRole(void)
{
	uint8_t ret;
	uint32_t val;

	// Read status to see if controller updated
	ret = tps25750_read32(NULL, TPS_REG_STATUS, &val);
	if (ret) { return (-1); }

	// Return the role, TYPEC_SINK or TYPEC_SOURCE
	return (TPS_REG_STATUS_PORT_ROLE(val));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void USBCPortControllerInit(void)
{
	// Todo: Initial setup?
	struct tps25750 tps;
	uint64_t bootStatus;
	uint16_t powerStatus;
	uint32_t status;
	uint8_t fullAccess = YES;

	// In relation to VBUS charging (supplied externally through VBUS), what purpose does Aux Power Enable have?
	// In order to set the Aux Power Enable, external VBUS must be present

#if 0 /* Test Interrupts */
	debug("USB Port Controller: Setting Interrupt mask...\r\n");
	memset(g_debugBuffer, 0xFF, 11);
	tps25750_block_write(&tps, TPS_REG_INT_MASK1, g_debugBuffer, 11);
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(500));
	memset(g_debugBuffer, 0x00, 11);
	tps25750_block_write(&tps, TPS_REG_INT_MASK1, g_debugBuffer, 11);
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(1500));
	memset(g_debugBuffer, 0xFF, 11);
	tps25750_block_write(&tps, TPS_REG_INT_MASK1, g_debugBuffer, 11);
#endif

	// Check mode
	if (tps2750_is_mode(&tps, TPS_MODE_BOOT) == 1) { fullAccess = NO; debugWarn("USB Port Controller: In Boot mode, likely Device booting in dead battery\r\n"); }
	if (tps2750_is_mode(&tps, TPS_MODE_PTCH) == 1) { fullAccess = NO; debug("USB Port Controller: In Patch mode, applying patch...\r\n"); tps25750_apply_patch(&tps); }
	if (tps2750_is_mode(&tps, TPS_MODE_APP) == 1) { fullAccess = YES; debug("USB Port Controller: In App mode\r\n"); }

#if 0 /* Test going back into Patch mode */
	if (fullAccess)
	{
		uint8_t rc, ret;
		//ret = tps25750_exec_cmd(&tps, TPS_4CC_GO2P, 0, NULL, 1, &rc, 0, TPS_BUNDLE_TIMEOUT * 100);
		ret = tps25750_block_write(&tps, TPS_REG_CMD1, TPS_4CC_GO2P, 4);
		if (ret) { debugWarn("USB Port Controller: Failed to write Go2P (0x%x)\r\n", ret); }
		ret = tps25750_wait_cmd_complete(&tps, 5000);
		if (ret) { debugWarn("USB Port Controller: Failed to wait for commmand complete (0x%x)\r\n", ret); }
		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(20));
		ret = tps25750_block_read(&tps, TPS_REG_DATA1, &rc, sizeof(rc));
		if (ret) { debugWarn("USB Port Controller: Failed to read status (0x%x)\r\n", ret); }

		if (!ret && !rc)
		{
			MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(1000));
			if (tps2750_is_mode(&tps, TPS_MODE_PTCH) == 1) { fullAccess = NO; debugWarn("USB Port Controller: In Patch mode, applying patch...\r\n"); tps25750_apply_patch(&tps); }
			if (tps2750_is_mode(&tps, TPS_MODE_APP) == 1) { fullAccess = YES; debug("USB Port Controller: In App mode\r\n"); }
		}
		else { debugWarn("USB Port Controller: Failed to go to patch mode (0x%x)\r\n", rc); }
	}
#endif

	// Check and clear Dead Battery flag if set
	tps25750_get_reg_boot_status(&tps, &bootStatus);

	if (TPS_REG_BOOT_STATUS_DEAD_BATTERY_FLAG(bootStatus))
	{
		debugWarn("USB Port Controller: Dead Battery flag set, must clear to continue\r\n");
		tps25750_clear_dead_battery(&tps);
	}

	if (fullAccess)
	{
		tps25750_read32(&tps, TPS_REG_STATUS, &status);
		debug("USB Port Controller: Status Register is 0x%x\r\n", status);
		tps25750_read32(&tps, TPS_REG_PD_STATUS, &status);
		debug("USB Port Controller: PD Status Register is 0x%x\r\n", status);

		// Read power status to get connection state
		tps25750_read16(&tps, TPS_REG_POWER_STATUS, &powerStatus);
		debug("USB Port Controller: Power status is 0x%x\r\n", powerStatus);

		// Check if there is a connection present
		if (TPS_REG_POWER_STATUS_POWER_CONNECTION(powerStatus))
		{
			// Determine if connection provides power (Sink mode)
			if (TPS_REG_POWER_STATUS_SOURCE_SINK(powerStatus))
			{
				debug("USB Port Controller: Connection found, provides power, attempting Sink/Device mode\r\n");

				// Attempt to set Power role to Sink and Data role to Device
				tps25750_pr_set(&tps, TYPEC_SINK);
				tps25750_dr_set(&tps, TYPEC_DEVICE);
			}
			else // Connection requests power (Source mode)
			{
				debug("USB Port Controller: Connection found, requests power, attempting Source/Host mode\r\n");

				// Attempt to set Power role to Source and Data tole to Host
				tps25750_pr_set(&tps, TYPEC_SOURCE);
				tps25750_dr_set(&tps, TYPEC_HOST);
			}
		}
		else // No conneciton present
		{
			debug("USB Port Controller: No connection present (attempting Sink/Device mode as default)\r\n");

			// Attempt to set Power role to Sink and Data role to Device
			tps25750_pr_set(&tps, TYPEC_SINK);
			tps25750_dr_set(&tps, TYPEC_DEVICE);
		}

		tps25750_read32(&tps, TPS_REG_STATUS, &status);
		debug("USB Port Controller: Status Register is 0x%x\r\n", status);
		tps25750_read32(&tps, TPS_REG_PD_STATUS, &status);
		debug("USB Port Controller: PD Status Register is 0x%x\r\n", status);
	}
	else
	{
		debugWarn("USB Port Controller: Not in App mode, register access restricted, skipping device init\r\n");
	}

	tps25750_block_read(&tps, TPS_REG_MODE, g_debugBuffer, 4);
	debug("USB Port Controller: Mode Register is <%c%c%c%c>\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3]);
	tps25750_block_read(&tps, TPS_REG_TYPE, g_debugBuffer, 4);
	debug("USB Port Controller: Type Register is <%c%c%c%c>\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3]);
	//tps25750_block_read(&tps, TPS_REG_CUSTUSE, g_debugBuffer, 8);
	//debug("USB Port Controller: Custom Use Register is 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3], g_debugBuffer[4], g_debugBuffer[5], g_debugBuffer[6], g_debugBuffer[7]);
	tps25750_block_read(&tps, TPS_REG_DEVICE_CAPABILITIES, g_debugBuffer, 4);
	debug("USB Port Controller: Device Cap Register is 0x%x 0x%x 0x%x 0x%x 0x%x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3]);
	tps25750_block_read(&tps, TPS_REG_VERSION, g_debugBuffer, 4);
	debug("USB Port Controller: Version Register is 0x%x 0x%x 0x%x 0x%x 0x%x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3]);
	//tps25750_block_read(&tps, TPS_REG_STATUS, g_debugBuffer, 5);
	//debug("USB Port Controller: Status Register is 0x%x 0x%x 0x%x 0x%x 0x%x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3], g_debugBuffer[4]);
	//tps25750_block_read(&tps, TPS_REG_POWER_PATH_STATUS, g_debugBuffer, 5);
	//debug("USB Port Controller: Power Path Status Register is 0x%x 0x%x 0x%x 0x%x 0x%x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3], g_debugBuffer[4]);
	//tps25750_block_read(&tps, TPS_REG_PORT_CONTROL, g_debugBuffer, 4);
	//debug("USB Port Controller: Port Control Register is 0x%x 0x%x 0x%x 0x%x 0x%x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3]);
	tps25750_block_read(&tps, TPS_REG_BOOT_STATUS, g_debugBuffer, 5);
	debug("USB Port Controller: Boot Status Register is 0x%x 0x%x 0x%x 0x%x 0x%x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3], g_debugBuffer[4]);
	//tps25750_block_read(&tps, TPS_REG_BUILD_DESCRIPTION, g_debugBuffer, 49);
	//debug("USB Port Controller: Build Desc Register is <%s>\r\n", (char*)g_debugBuffer);
	tps25750_block_read(&tps, TPS_REG_DEVICE_INFO, g_debugBuffer, 40);
	debug("USB Port Controller: Device Info Register is <%s>\r\n", (char*)g_debugBuffer);
	//tps25750_block_read(&tps, TPS_REG_ACTIVE_CONTRACT_PDO, g_debugBuffer, 6);
	//debug("USB Port Controller: Active Contract PDO Register is 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3], g_debugBuffer[4], g_debugBuffer[5]);
	//tps25750_block_read(&tps, TPS_REG_ACTIVE_CONTRACT_RDO, g_debugBuffer, 4);
	//debug("USB Port Controller: Active Contract RDO Register is 0x%x 0x%x 0x%x 0x%x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3]);
	//tps25750_block_read(&tps, TPS_REG_POWER_STATUS, g_debugBuffer, 2);
	//debug("USB Port Controller: Power Status Register is 0x%x 0x%x\r\n", g_debugBuffer[0], g_debugBuffer[1]);
	//tps25750_block_read(&tps, TPS_REG_PD_STATUS, g_debugBuffer, 4);
	//debug("USB Port Controller: PD Status Register is 0x%x 0x%x 0x%x 0x%x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3]);
	//tps25750_block_read(&tps, TPS_REG_TYPEC_STATE, g_debugBuffer, 4);
	//debug("USB Port Controller: TypeC State Register is 0x%x 0x%x 0x%x 0x%x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3]);
	//tps25750_block_read(&tps, TPS_REG_GPIO_STATUS, g_debugBuffer, 8);
	//debug("USB Port Controller: GPIO Status Register is 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3], g_debugBuffer[4], g_debugBuffer[5], g_debugBuffer[6], g_debugBuffer[7]);

	tps25750_block_read(&tps, TPS_REG_TX_SOURCE_CAPS, g_debugBuffer, 7);
	debug("USB Port Controller: TX Source caps is %02x %02x %02x %02x %02x %02x %02x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3], g_debugBuffer[4], g_debugBuffer[5], g_debugBuffer[6]);
	tps25750_block_read(&tps, TPS_REG_TX_SINK_CAPS, g_debugBuffer, 5);
	debug("USB Port Controller: TX Sink caps is %02x %02x %02x %02x %02x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3], g_debugBuffer[4]);

	tps25750_block_read(&tps, TPS_REG_INT_EVENT1, g_debugBuffer, 11);
	debug("USB Port Controller: Int Event1 Register is %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3], g_debugBuffer[4], g_debugBuffer[5], g_debugBuffer[6], g_debugBuffer[7], g_debugBuffer[8], g_debugBuffer[9], g_debugBuffer[10]);
	tps25750_block_read(&tps, TPS_REG_INT_MASK1, g_debugBuffer, 11);
	debug("USB Port Controller: Int Mask1 Register is %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3], g_debugBuffer[4], g_debugBuffer[5], g_debugBuffer[6], g_debugBuffer[7], g_debugBuffer[8], g_debugBuffer[9], g_debugBuffer[10]);
#if 0 /* Test for r/w success */
	g_debugBuffer[0] |= 0x02; g_debugBuffer[1] |= 0x40; g_debugBuffer[2] |= 0x02; g_debugBuffer[3] |= 0x01; g_debugBuffer[4] |= 0x01; g_debugBuffer[5] |= 0x04; g_debugBuffer[8] |= 0x02; g_debugBuffer[10] |= 0x01;
	debug("USB Port Controller: Write to Int Mask1 is %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3], g_debugBuffer[4], g_debugBuffer[5], g_debugBuffer[6], g_debugBuffer[7], g_debugBuffer[8], g_debugBuffer[9], g_debugBuffer[10]);
	tps25750_block_write(&tps, TPS_REG_INT_MASK1, g_debugBuffer, 11);
	memset(g_debugBuffer, 0, 11);
	tps25750_block_read(&tps, TPS_REG_INT_MASK1, g_debugBuffer, 11);
	debug("USB Port Controller: Int Mask1 aft Write is %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n", g_debugBuffer[0], g_debugBuffer[1], g_debugBuffer[2], g_debugBuffer[3], g_debugBuffer[4], g_debugBuffer[5], g_debugBuffer[6], g_debugBuffer[7], g_debugBuffer[8], g_debugBuffer[9], g_debugBuffer[10]);
#endif

	USBCPortControllerClearIntFlags();
}

///--------------------------------------------------------------------------------------------------------------------------------------------------------
///--------------------------------------------------------------------------------------------------------------------------------------------------------
/// USB Host Controller
///--------------------------------------------------------------------------------------------------------------------------------------------------------
///--------------------------------------------------------------------------------------------------------------------------------------------------------

#include "lcd.h"

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int GetUsbHCRegister(uint8_t registerAddress, uint8_t* registerData, uint8_t dataSize)
{
	int status = E_SUCCESS;

	if (dataSize > 64) { return (E_BAD_PARAM); }
	uint8_t readData[65];
	uint8_t writeData[65];
	writeData[0] = (registerAddress << 3); // Set the address for the upper 5 bits, and read bit (bit 1) is a 0
	memset(&writeData[1], 0, dataSize);

	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { MXC_GPIO_OutClr(GPIO_SPI2_SS2_USB_PORT, GPIO_SPI2_SS2_USB_PIN); }
	//SoftUsecWait(5 * SOFT_MSECS);
	//SpiTransaction(SPI_USBHC, SPI_8_BIT_DATA_SIZE, YES, &writeData[0], sizeof(writeData), readData, sizeof(readData), BLOCKING);
	SpiTransaction(SPI_USBHC, SPI_8_BIT_DATA_SIZE, YES, &writeData[0], (dataSize + 1), readData, (dataSize + 1), BLOCKING);
	//SoftUsecWait(5 * SOFT_MSECS);
	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { MXC_GPIO_OutSet(GPIO_SPI2_SS2_USB_PORT, GPIO_SPI2_SS2_USB_PIN); }

	memcpy(registerData, &readData[1], dataSize);

	return (status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int SetUsbHCRegister(uint8_t registerAddress, uint8_t registerData)
{
	int status = E_SUCCESS;

	uint8_t writeData[2];
	writeData[0] = (registerAddress << 3) | 0x02; // Set the address for the upper 5 bits, and write bit (bit 1) is a 1
	writeData[1] = registerData;

	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { MXC_GPIO_OutClr(GPIO_SPI2_SS2_USB_PORT, GPIO_SPI2_SS2_USB_PIN); }
	//SoftUsecWait(5 * SOFT_MSECS);
	SpiTransaction(SPI_USBHC, SPI_8_BIT_DATA_SIZE, YES, &writeData[0], sizeof(writeData), NULL, 0, BLOCKING);
	//SoftUsecWait(5 * SOFT_MSECS);
	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { MXC_GPIO_OutSet(GPIO_SPI2_SS2_USB_PORT, GPIO_SPI2_SS2_USB_PIN); }

	return (status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int SetUsbHCRegisterMulti(uint8_t registerAddress, uint8_t* registerData, uint8 dataSize)
{
	int status = E_SUCCESS;

	if (dataSize > 64) { debugErr("USB Host Controller: Register Multi write size too large\r\n"); return (E_BAD_PARAM); }

	uint8_t writeData[65];
	writeData[0] = (registerAddress << 3) | 0x02; // Set the address for the upper 5 bits, and write bit (bit 1) is a 1
	memcpy(&writeData[1], registerData, dataSize);

	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { MXC_GPIO_OutClr(GPIO_SPI2_SS2_USB_PORT, GPIO_SPI2_SS2_USB_PIN); }
	//SoftUsecWait(5 * SOFT_MSECS);
	SpiTransaction(SPI_USBHC, SPI_8_BIT_DATA_SIZE, YES, &writeData[0], (dataSize + 1), NULL, 0, BLOCKING);
	//SoftUsecWait(5 * SOFT_MSECS);
	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { MXC_GPIO_OutSet(GPIO_SPI2_SS2_USB_PORT, GPIO_SPI2_SS2_USB_PIN); }

	return (status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* Unlikely to work with Rd/Wr bit location changed from bit 7 to bit 2 for this part */
int SetAndReadUsbHCRegister(uint8_t registerAddress, uint8_t registerData, uint8_t* readData, uint8_t dataSize)
{
	uint8_t writeData[2];
	int status = E_SUCCESS;

	writeData[0] = registerAddress;
	writeData[1] = registerData;

	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { MXC_GPIO_OutClr(GPIO_SPI2_SS2_USB_PORT, GPIO_SPI2_SS2_USB_PIN); }
	SpiTransaction(SPI_USBHC, SPI_8_BIT_DATA_SIZE, YES, &writeData[0], sizeof(writeData), readData, dataSize, BLOCKING);
	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { MXC_GPIO_OutSet(GPIO_SPI2_SS2_USB_PORT, GPIO_SPI2_SS2_USB_PIN); }

	return (status);
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void USBHostControllerPowerOn(void)
{
	debug("USB Host Controller: Reset off\r\n");
	PowerControl(USB_RESET, OFF);
	SoftUsecWait(25 * SOFT_MSECS);

	uint8_t reg;
	debug("USB Host Controller: Setting SPI Full Duplex Mode\r\n");
	reg = 0x10;
	SetUsbHCRegister(17, reg);

	debug("USB Host Controller: Setting Mode register to Host\r\n");
	reg = 0x01;
	SetUsbHCRegister(27, reg);

	GetUsbHCRegister(18, &reg, 1);
	debug("USB Host Controller: Revision register is 0x%x\r\n", reg);

	if (reg == 0x13) { debug("USB Host Controller: Device verified\r\n"); }
	else { debugWarn("USB Host Controller: Device not verified\r\n"); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void USBHostControllerPowerOff(void)
{
	debug("USB Host Controller: Reset on\r\n");
	PowerControl(USB_RESET, ON);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void USBHostControllerSetMuxAndSource(uint8_t state)
{
	uint8_t reg = 0xF0;
	debug("USB Host Controller: Setting SPI GP Out 0 & 1 (%d)\r\n", state);

#if 0 /* Normal */
	if (state) { reg |= 0x03; }
#else
	// Do not enable USB Source Enable, testing external power source
	//if (state) { reg |= 0x01; }
extern uint8_t g_usbSourceExternalPower;
	if (g_usbSourceExternalPower)
	{
		if (state) { reg |= 0x01; }
	}
	else // Use internal 5V buck
	{
		if (state) { reg |= 0x03; }
	}
#endif

	SetUsbHCRegister(20, reg);

	reg = 0;
	GetUsbHCRegister(20, &reg, 1);
	if (state)
	{
#if 0 /* Normal */
		if (reg == 0xF3) { debug("USB Host Controller: Mux and Source enabled\r\n"); }
		else { debug("USB Host Controller: Mux and Source enable failed (0x%x)\r\n", state); }
#else
		//if (reg == 0xF1) { debug("USB Host Controller: Mux enabled and Source disabled\r\n"); }
		//else { debug("USB Host Controller: Mux enable and Source disable failed (0x%x)\r\n", state); }
		if (g_usbSourceExternalPower)
		{
			if (reg == 0xF1) { debug("USB Host Controller: Mux enabled and Source disabled\r\n"); }
			else { debug("USB Host Controller: Mux enable and Source disable failed (0x%x)\r\n", state); }
		}
		else // Use internal 5V buck
		{
			if (reg == 0xF3) { debug("USB Host Controller: Mux and Source enabled\r\n"); }
			else { debug("USB Host Controller: Mux and Source enable failed (0x%x)\r\n", state); }
		}
#endif
	}
	else
	{
		if (reg == 0xF0) { debug("USB Host Controller: Mux and Source disabled\r\n"); }
		else { debug("USB Host Controller: Mux and Source disable failed (0x%x)\r\n", state); }
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void USBHostControllerMuxControl(uint8_t muxState)
{
	uint8_t reg = 0xF0;
	debug("USB Host Controller: Setting SPI GP Out 0 (%d)\r\n", muxState);
	if (muxState) { reg |= 0x01; }
	SetUsbHCRegister(20, reg);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void USBHostControllerSourceEnable(uint8_t sourceEnable)
{
	uint8_t reg = 0xF0;
	debug("USB Host Controller: Setting SPI GP Out 1 (%d)\r\n", sourceEnable);
	if (sourceEnable) { reg |= 0x02; }
	SetUsbHCRegister(20, reg);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void USBHostControllerInit(void)
{
	debug("USB Host Controller: Init\r\n");

	debug("USB Host Controller: Reset off\r\n");
	PowerControl(USB_RESET, OFF);

	SoftUsecWait(25 * SOFT_MSECS);

	uint8_t reg;
	debug("USB Host Controller: Setting SPI Full Duplex Mode\r\n");
	reg = 0x10;
	SetUsbHCRegister(17, reg);

	debug("USB Host Controller: Setting Mode register to Host\r\n");
	reg = 0x01;
	SetUsbHCRegister(27, reg);

	GetUsbHCRegister(18, &reg, 1);
	debug("USB Host Controller: Revision register is 0x%x\r\n", reg);

	if (reg == 0x13) { debug("USB Host Controller: Device verified\r\n"); }
	else { debugWarn("USB Host Controller: Device not verified\r\n"); }

	//USBHostControllerSetMuxAndSource(OFF);
	reg = 0xF0;
	SetUsbHCRegister(20, reg);
	debug("USB Host Controller: Mux and Source disabled\r\n");

#if 0 /* Test */
	while (1)
	{
		GetUsbHCRegister(18, &reg, 1);
		debug("USB Host Controller: Revision register is 0x%x\r\n", reg);
		if (ScanKeypad() != KEY_NONE) { break; }
		SoftUsecWait(1 * SOFT_SECS);
	}
#endif

#if 0 /* Test control register map */
	GetUsbHCRegister(13, &reg, 1); debug("USB Host Controller: IRQ register  is 0x%x\r\n", reg);
	GetUsbHCRegister(14, &reg, 1); debug("USB Host Controller: IEN register  is 0x%x\r\n", reg);
	GetUsbHCRegister(15, &reg, 1); debug("USB Host Controller: CTL register  is 0x%x\r\n", reg);
	GetUsbHCRegister(16, &reg, 1); debug("USB Host Controller: CPU CTL reg   is 0x%x\r\n", reg);
	GetUsbHCRegister(17, &reg, 1); debug("USB Host Controller: PIN CTL reg   is 0x%x\r\n", reg);
	GetUsbHCRegister(18, &reg, 1); debug("USB Host Controller: Rev register  is 0x%x\r\n", reg);

	GetUsbHCRegister(20, &reg, 1); debug("USB Host Controller: IO PINS 1 reg is 0x%x\r\n", reg);
	GetUsbHCRegister(21, &reg, 1); debug("USB Host Controller: IO PINS 2 reg is 0x%x\r\n", reg);
	GetUsbHCRegister(22, &reg, 1); debug("USB Host Controller: GP IN IRQ reg is 0x%x\r\n", reg);
	GetUsbHCRegister(23, &reg, 1); debug("USB Host Controller: GP IN IEN reg is 0x%x\r\n", reg);
	GetUsbHCRegister(24, &reg, 1); debug("USB Host Controller: GP IN POL reg is 0x%x\r\n", reg);
	GetUsbHCRegister(25, &reg, 1); debug("USB Host Controller: H IRQ reg     is 0x%x\r\n", reg);
	GetUsbHCRegister(26, &reg, 1); debug("USB Host Controller: H IEN reg     is 0x%x\r\n", reg);
	GetUsbHCRegister(27, &reg, 1); debug("USB Host Controller: MODE register is 0x%x\r\n", reg);
	GetUsbHCRegister(28, &reg, 1); debug("USB Host Controller: PER ADDR reg  is 0x%x\r\n", reg);
	GetUsbHCRegister(29, &reg, 1); debug("USB Host Controller: H CTL reg     is 0x%x\r\n", reg);
	GetUsbHCRegister(30, &reg, 1); debug("USB Host Controller: H XFR reg     is 0x%x\r\n", reg);
	GetUsbHCRegister(31, &reg, 1); debug("USB Host Controller: H RSL reg     is 0x%x\r\n", reg);
#endif

#if 0 /* Test skipping disable */
	debug("USB Host Controller: Reset on\r\n");
	PowerControl(USB_RESET, ON);
#endif
}


///----------------------------------------------------------------------------
///	USB Host Controller Defines
///----------------------------------------------------------------------------
#define DIR_WRITE 1
#define DIR_READ  0

#define BUFFER_SIZE 64

#define MAX_IRQ_BUSEVENT    BIT0
#define MAX_IRQ_RWU         BIT1
#define MAX_IRQ_RCVDAV      BIT2
#define MAX_IRQ_SNDBAV      BIT3
#define MAX_IRQ_SUSDN       BIT4
#define MAX_IRQ_CONDET      BIT5
#define MAX_IRQ_FRAME       BIT6
#define MAX_IRQ_HXFRDN      BIT7

#define MAX_IRQ_OSCOK       BIT0
/* MAX_IRQ_RWU */
#define MAX_IRQ_BUSACT      BIT2
#define MAX_IRQ_URES        BIT3
#define MAX_IRQ_SUSP        BIT4
#define MAX_IRQ_NOVBUS      BIT5
#define MAX_IRQ_VBUS        BIT6
#define MAX_IRQ_URESDN      BIT7

/* the End Point interrupts (EPIRQ) */
#define MAX_IRQ_IN0BAV      BIT0
#define MAX_IRQ_OUT0DAV     BIT1
#define MAX_IRQ_OUT1DAV     BIT2
#define MAX_IRQ_IN2BAV      BIT3
#define MAX_IRQ_IN3BAV      BIT4
#define MAX_IRQ_SUDAV       BIT5

// Data toggle bits
#define MAX_SNDTOG1			BIT7
#define MAX_SNDTOG0			BIT6
#define MAX_RCVTOG1			BIT5
#define MAX_RCVTOG0			BIT4

#define MAX_SNDTOGRD		BIT5
#define MAX_RCVTOGRD		BIT4

#define MODE_PERIPH         0
#define MODE_HOST           1

#define rEP0FIFO        0
#define rEP1OUTFIFO     1
#define rEP2INFIFO      2
#define rEP3INFIFO      3
#define rSUDFIFO        4
#define rEP0BC          5
#define rEP1OUTBC       6
#define rEP2INBC        7
#define rEP3INBC        8
#define rEPSTALLS       9
#define rCLRTOGS        10
#define rEPIRQ          11
#define rEPIEN          12
#define rUSBIRQ         13
#define rUSBIEN         14
#define rUSBCTL         15
#define rCPUCTL         16
#define rPINCTL         17
#define rREVISION       18
#define rFNADDR         19
#define rIOPINS         20

#define rRCVFIFO        1
#define rSNDFIFO        2
#define rSUDFIFO        4
#define rRCVBC          6
#define rSNDBC          7
#define rIOPINS1        20
#define rIOPINS2        21
#define rGPINIRQ        22
#define rGPINIEN        23
#define rGPINPOL        24
#define rHIRQ           25
#define rHIEN           26
#define rMODE           27
#define rPERADDR        28
#define rHCTL           29
#define rHXFR           30
#define rHRSL           31

#define BIT0	0x01
#define BIT1	0x02
#define BIT2	0x04
#define BIT3	0x08
#define BIT4	0x10
#define BIT5	0x20
#define BIT6	0x40
#define BIT7	0x80

//-----

#define PERIPHERAL_ADDRESS 8

#define DIR_OUT      0
#define DIR_IN      1

/* Transfer Tokens */
#define xfrSETUP    0x10
#define xfrIN       0x00
#define xfrOUT      0x20
#define xfrINHS     0x80
#define xfrOUTHS    0xA0
#define xfrISOIN    0x40
#define xfrISOOUT   0x60

/* Result Codes */
#define rslSUCCES   0x00
#define rslBUSY     0x01
#define rslBADREQ   0x02
#define rslUNDEF    0x03
#define rslNAK      0x04
#define rslSTALL    0x05
#define rslTOGERR   0x06
#define rslWRONGPID 0x07
#define rslBADBC    0x08
#define rslPIDERR   0x09
#define rslPKTERR   0x0A
#define rslCRCERR   0x0B
#define rslKERR     0x0C
#define rslJERR     0x0D
#define rslTIMEOUT  0x0E
#define rslBABBLE   0x0F

#define rslSUCCES_name   "Success"
#define rslBUSY_name     "Busy"
#define rslBADREQ_name   "Bad request"
#define rslUNDEF_name    "Undefined"
#define rslNAK_name      "Device returned NAK"
#define rslSTALL_name    "Device returned Stall"
#define rslTOGERR_name   "Toggle error"
#define rslWRONGPID_name "Rx wrong PID"
#define rslBADBC_name    "Bad byte count"
#define rslPIDERR_name   "Rx PID error"
#define rslPKTERR_name   "Packet error"
#define rslCRCERR_name   "CRC error"
#define rslKERR_name     "K-state instead of response"
#define rslJERR_name     "J-state instead of response"
#define rslTIMEOUT_name  "Device timeout"
#define rslBABBLE_name   "Device babbling"

#define USB_DEVICE_DESCRIPTOR		0x01
#define USB_CONFIG_DESCRIPTOR		0x02
#define USB_STRING_DESCRIPTOR		0x03
#define USB_INTERFACE_DESCRIPTOR	0x04
#define USB_ENDPOINT_DESCRIPTOR		0x05

#define USB_ADC_CLASS		0x01 // Audio Device Class (ADC)
#define USB_CDC_CLASS		0x02 // Communication Device Class (CDC)
#define USB_HID_CLASS		0x03 // Human Interface Device (HID)
#define USB_PRT_CLASS		0x07 // Printer Class
#define USB_MSC_CLASS		0x08 // Mass Storage Class (MSC)

#define USB_UNKNOWN_NAME	"Unknown"

#define USB_ADC_CLASS_NAME	"Audio"
#define USB_CDC_CLASS_NAME	"Communiation"
#define USB_HID_CLASS_NAME	"Human Interface"
#define USB_PRT_CLASS_NAME	"Printer"
#define USB_MSC_CLASS_NAME	"Mass Storage"

#define USB_TRANSFER_CONTROL		0x00
#define USB_TRANSFER_ISOCHRONOUS	0x01
#define USB_TRANSFER_BULK			0x02
#define USB_TRANSFER_INTERRUPT		0x03

#define USB_TRANSFER_CONTROL_NAME		"Control"
#define USB_TRANSFER_ISOCHRONOUS_NAME	"Isochronous"
#define USB_TRANSFER_BULK_NAME			"Bulk"
#define USB_TRANSFER_INTERRUPT_NAME		"Interrupt"

/* Standard Requests */
#define reqGET_STATUS           0x00
#define reqCLEAR_FEATURE        0x01
#define reqSET_FEATURE          0x03
#define reqSET_ADDRESS          0x05
#define reqGET_DESCRIPTOR       0x06
#define reqSET_DESCRIPTOR       0x07
#define reqGET_CONFIGURATION    0x08
#define reqSET_CONFIGURATION    0x09

#define MAX_TIMER_CONVERSION	20 //40

typedef struct {
	uint8_t perAddress;
	uint8_t type;
	uint8_t endPoint;
	uint8_t bmRequestType;
	uint8_t bRequest;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
	uint8_t direction;
} ControlPacket;

/*
typedef enum {
  USB_REQ_GET_STATUS        = 0  ,
  USB_REQ_CLEAR_FEATURE     = 1  ,
  USB_REQ_RESERVED          = 2  ,
  USB_REQ_SET_FEATURE       = 3  ,
  USB_REQ_RESERVED2         = 4  ,
  USB_REQ_SET_ADDRESS       = 5  ,
  USB_REQ_GET_DESCRIPTOR    = 6  ,
  USB_REQ_SET_DESCRIPTOR    = 7  ,
  USB_REQ_GET_CONFIGURATION = 8  ,
  USB_REQ_SET_CONFIGURATION = 9  ,
  USB_REQ_GET_INTERFACE     = 10 ,
  USB_REQ_SET_INTERFACE     = 11 ,
  USB_REQ_SYNCH_FRAME       = 12
} USB_REQUEST_CODE_TYPE;

typedef enum {
  USB_REQ_FEATURE_EDPT_HALT     = 0,
  USB_REQ_FEATURE_REMOTE_WAKEUP = 1,
  USB_REQ_FEATURE_TEST_MODE     = 2
} USB_REQUEST_FEATURE_SELECTOR_TYPE;

typedef enum {
  USB_REQ_TYPE_STANDARD = 0,
  USB_REQ_TYPE_CLASS,
  USB_REQ_TYPE_VENDOR,
  USB_REQ_TYPE_INVALID
} USB_REQUEST_TYPE;

typedef enum {
  USB_REQ_RCPT_DEVICE =0,
  USB_REQ_RCPT_INTERFACE,
  USB_REQ_RCPT_ENDPOINT,
  USB_REQ_RCPT_OTHER
} USB_REQUEST_RECIPIENT_TYPE;
*/
/*
typedef enum {
  USB_DIR_OUT = 0,
  USB_DIR_IN  = 1,

  USB_DIR_IN_MASK = 0x80
} USB_DIR_TYPE;
*/
/// SCSI Command Operation Code
typedef enum
{
  SCSI_CMD_TEST_UNIT_READY              = 0x00, ///< The SCSI Test Unit Ready command is used to determine if a device is ready to transfer data (read/write), i.e. if a disk has spun up, if a tape is loaded and ready etc. The device does not perform a self-test operation.
  SCSI_CMD_INQUIRY                      = 0x12, ///< The SCSI Inquiry command is used to obtain basic information from a target device.
  SCSI_CMD_MODE_SELECT_6                = 0x15, ///<  provides a means for the application client to specify medium, logical unit, or peripheral device parameters to the device server. Device servers that implement the MODE SELECT(6) command shall also implement the MODE SENSE(6) command. Application clients should issue MODE SENSE(6) prior to each MODE SELECT(6) to determine supported mode pages, page lengths, and other parameters.
  SCSI_CMD_MODE_SENSE_6                 = 0x1A, ///< provides a means for a device server to report parameters to an application client. It is a complementary command to the MODE SELECT(6) command. Device servers that implement the MODE SENSE(6) command shall also implement the MODE SELECT(6) command.
  SCSI_CMD_START_STOP_UNIT              = 0x1B,
  SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL = 0x1E,
  SCSI_CMD_READ_CAPACITY_10             = 0x25, ///< The SCSI Read Capacity command is used to obtain data capacity information from a target device.
  SCSI_CMD_REQUEST_SENSE                = 0x03, ///< The SCSI Request Sense command is part of the SCSI computer protocol standard. This command is used to obtain sense data -- status/error information -- from a target device.
  SCSI_CMD_READ_FORMAT_CAPACITY         = 0x23, ///< The command allows the Host to request a list of the possible format capacities for an installed writable media. This command also has the capability to report the writable capacity for a media when it is installed
  SCSI_CMD_READ_10                      = 0x28, ///< The READ (10) command requests that the device server read the specified logical block(s) and transfer them to the data-in buffer.
  SCSI_CMD_WRITE_10                     = 0x2A, ///< The WRITE (10) command requests that the device server transfer the specified logical block(s) from the data-out buffer and write them.
} SCSI_CMD_TYPE;

#if 1 /* New code not ready for compile yet */

///----------------------------------------------------------------------------
///	USB Host Controller Globals
///----------------------------------------------------------------------------
volatile bool peripheralAvailable;
volatile bool ACKSTAT;
volatile uint8_t enabledIRQ;
volatile uint8_t enabledEPIRQ;
volatile uint8_t peripheralConnected;
volatile uint8_t lastTransferResult;
volatile uint8_t lastReadSize;
volatile uint8_t RXData[BUFFER_SIZE];
volatile uint8_t TXData[BUFFER_SIZE];
volatile uint8_t ControlBuffer[64];
uint8_t usbMscConfigID = 0;
uint8_t usbMscInterfaceID = 0;
uint8_t usbMscBulkInEP = 0;
uint8_t usbMscBulkOutEP = 0;

static uint32_t s_usbTag = 0;

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t MAX_writeRegister(uint8_t addr, uint8_t data)
{
	return (SetUsbHCRegister(addr, data));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t MAX_readRegister(uint8_t addr)
{
	uint8_t data;
	GetUsbHCRegister(addr, &data, 1);
	return (data);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MAX_multiReadRegister(uint8_t address, uint8_t* buffer, uint8_t length)
{
#if 0
	/* Start the transaction by pulling the CS low */
	SET_CS_LOW

	/* Transmit the command byte */
	SIMSPI_transmitByte(_getCommandByte(address, DIR_READ));

	/* Transmit 0s, as we don't actually care about what's written but we do about the response */
	SIMSPI_readBytes(buffer, length);

	/* End the transaction by pulling the CS back to high */
	SET_CS_HIGH
#endif

	GetUsbHCRegister(address, buffer, length);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t MAX_multiWriteRegister(uint8_t address, uint8_t* values, uint8_t length)
{
#if 0
	/* Start the transaction by pulling the CS low */
	SET_CS_LOW

	/* Build and transmit the command byte */
	SIMSPI_transmitByte(_getCommandByte(address, DIR_WRITE));
	/* Transmit the data */
	result = SIMSPI_transmitBytes(values, length);

	/* End the transaction by pulling the CS back to high */
	SET_CS_HIGH
#endif

	return (SetUsbHCRegisterMulti(address, values, length));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int MAX_writeRegisterWithMask(uint8_t addr, uint8_t bits, uint8_t mask)
{
	/* Read the current state of the register */
	uint8_t regVal = MAX_readRegister(addr);

	/* Disable the mask bits */
	regVal &= ~mask;

	/* Enable the given bits within the mask */
	regVal |= (bits & mask);

	return (SetUsbHCRegister(addr, regVal));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MAX_enableOptions(uint8_t address, uint8_t flags)
{
	/* Read the current state of the register */
	uint8_t regVal = MAX_readRegister(address);

	/* Enable the given bits */
	regVal |= flags;

	/* Write the new register value back to the module */
	MAX_writeRegister(address, regVal);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MAX_disableOptions(uint8_t address, uint8_t flags)
{
	/* Read the current state of the register */
	uint8_t regVal = MAX_readRegister(address);

	/* Disable the given bits */
	regVal &= ~flags;

	/* Write the new register value back to the module */
	MAX_writeRegister(address, regVal);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MAX_enableInterrupts(uint8_t flags)
{
	/* Enable the interrupts */
	MAX_enableOptions(26, flags);

	enabledIRQ |= flags;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MAX_enableInterruptsMaster(void)
{
	/* Set IE to 1 */
	MAX_writeRegister(16, BIT0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MAX_disableInterruptsMaster(void)
{
	/* Set IE to 0 */
	MAX_disableOptions(16, BIT0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MAX_clearInterruptStatus(uint8_t flags)
{
	/* Clear the specified interrupts */
	MAX_enableOptions(25, flags);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t MAX_scanBus(void)
{
	/* Enable SAMPLEBUS */
	MAX_enableOptions(rHCTL, BIT2);

	while (!(MAX_readRegister(rHCTL) & BIT2))
	{
		//SysCtlDelay(200);
		SoftUsecWait((200 / MAX_TIMER_CONVERSION));
	}

	/* Return the J/K state bits */
	return (MAX_readRegister(rHRSL) & 0xC0) >> 6;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MAX_checkBusState(uint8_t debug)
{
	uint8_t result = MAX_scanBus();

	if (result == 0x01 || result == 0x02)
	{
		peripheralAvailable = true;
		if (debug) { debug("USB Host Controller: *** Peripheral found ***\r\n"); }
		if (debug) { OverlayMessage(getLangText(STATUS_TEXT), "FOUND USB DEVICE", (2 * SOFT_SECS)); }
	}
	else
	{
		peripheralAvailable = false;
		debugWarn("USB Host Controller: --- No peripheral available ---\r\n");
		OverlayMessage(getLangText(WARNING_TEXT), "NO USB DEVICE DETECTED", (2 * SOFT_SECS));
	}

	if (debug) { debug("USB Host Controller: Sample bus returns J-state %d, K-state %d\r\n", (result == 0x02), (result == 0x01)); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MAX_reset(void)
{
	/* Enable the reset */
	MAX_writeRegister(15, BIT5);

	/* Immediately clear the reset */
	MAX_writeRegister(15, 0);

	/* Wait a short while until the oscillator is stable */
	//DELAY_WITH_TIMEOUT(!(MAX_readRegister(13) & BIT0));
	while (1)
	{
		uint32_t oscDelayForStable = 10000;
		if (MAX_readRegister(13) & BIT0) { break; }
		if (oscDelayForStable == 0) { debugErr("USB Host Controller: Oscillator failed to stabilize after reset\r\n"); }
	}

	/* Reset the interrupt state */
	MAX_disableInterruptsMaster();

	/* Host */
	MAX_writeRegister(rHIEN, 0);
	MAX_writeRegister(rHIRQ, 0xFF);

	enabledIRQ = 0;
	enabledEPIRQ = 0;
	ACKSTAT = false;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void USB_busReset(uint8_t debug)
{
	if (debug) { debug("USB Host Controller: Perform bus reset\r\n"); }

	/* First disable the SOF generator */
	MAX_disableOptions(rMODE, BIT3);

	/* Perform the reset */
	MAX_enableOptions(rHCTL, BIT0);

	while (!MAX_readRegister(rHCTL) & BIT0)
	{
		//SysCtlDelay(10000);
		SoftUsecWait((10000 / MAX_TIMER_CONVERSION));
	}

	/* Restart the SOF generator */
	MAX_enableOptions(rMODE, BIT3);

	/* Wait until the first SOF is transmitted */
	while (!(MAX_readRegister(rHIRQ) & BIT6))
	{
		//SysCtlDelay(100);
		SoftUsecWait((100 / MAX_TIMER_CONVERSION));
	}

	if (debug) { debug("USB Host Controller: Bus reset done\r\n"); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MAX_start(void)
{
	/* Start SPI */
	//SIMSPI_startSPI( );
	// Done in Init Hardware

	/* Set the SPI configuration to 4-wire and IRQ mode to pulldown */
	MAX_writeRegister(17, 0x18);

	/* Make sure everything is reset (note: this does NOT reset the SPI config) */
	MAX_reset();

	/* Enable the dedicated INT pin (active low) */
	//MAP_GPIO_setAsInputPin(USBINT_PORT, USBINT_PIN);
	//MAP_GPIO_interruptEdgeSelect(USBINT_PORT, USBINT_PIN, GPIO_HIGH_TO_LOW_TRANSITION);
	//MAP_GPIO_clearInterruptFlag(USBINT_PORT, USBINT_PIN);
	//MAP_GPIO_enableInterrupt(USBINT_PORT, USBINT_PIN);
	//MAP_Interrupt_enableInterrupt(INT_PORT2);
	// Done in Init Hardware

	/* Enabling MASTER interrupts */
	//MAP_Interrupt_enableMaster( );
	// Done in Init Hardware

	/* We're starting as a USB host/master */
#if 1 /* Need to resetup Host mode after Max reset? */
	USBCPortControllerSwapToHost();
	SoftUsecWait(500 * SOFT_MSECS);
#endif

#if 1 /* Normal */
	/* Enable HOST, DMPULLDN and DPPULLDN */
	MAX_enableOptions(rMODE, BIT0 | BIT6 | BIT7);
#else /* Test without pulldowns, seemed to produce the same results */
	/* Enable HOST without pulldowns */
	MAX_enableOptions(rMODE, BIT0 | BIT6 | BIT7);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
char* ResultCodeName(uint8_t resultCode)
{
	char* resultName = NULL;

	switch (resultCode)
	{
		case rslSUCCES: resultName = rslSUCCES_name; break;
		case rslBUSY: resultName = rslBUSY_name; break;
		case rslBADREQ: resultName = rslBADREQ_name; break;
		case rslUNDEF: resultName = rslUNDEF_name; break;
		case rslNAK: resultName = rslNAK_name; break;
		case rslSTALL: resultName = rslSTALL_name; break;
		case rslTOGERR: resultName = rslTOGERR_name; break;
		case rslWRONGPID: resultName = rslWRONGPID_name; break;
		case rslBADBC: resultName = rslBADBC_name; break;
		case rslPIDERR: resultName = rslPIDERR_name; break;
		case rslPKTERR: resultName = rslPKTERR_name; break;
		case rslCRCERR: resultName = rslCRCERR_name; break;
		case rslKERR: resultName = rslKERR_name; break;
		case rslJERR: resultName = rslJERR_name; break;
		case rslTIMEOUT: resultName = rslTIMEOUT_name; break;
		case rslBABBLE: resultName = rslBABBLE_name; break;
	}

	return (resultName);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
//uint8_t g_usbEndpointDataToggle[3] = { 0xFF, 0xFF, 0xFF };
uint8_t g_usbEndpointDataToggle[3] = { 0, 0, 0 };
uint8_t transmitPacket(uint8_t token, uint8_t ep)
{
	uint8_t regval;
	uint32_t timeout;
	//static uint8_t s_lastEp = 0;

#if 0 /* Move endpoint check higher up */
	if (ep > 2)
	{
		debugErr("USB Host Controller: Endpoint out of range (%d)\r\n", ep);
		return (rslUNDEF);
	}
	else //if (ep != s_lastEp)
	{
#if 0 /* Test skipping data toggles at this level */
		uint8_t dataToggles;

		// Save the current toggle state of the endpoint
		//g_usbEndpointDataToggle[ep] = MAX_readRegister(rHRSL);

		// Check if default and set to using Data0
		if (g_usbEndpointDataToggle[ep] == 0xFF) { g_usbEndpointDataToggle[ep] = 0x30; }

		// Data toggles need to be inverted
		//dataToggles = (((g_usbEndpointDataToggle[ep] & BIT5) ? 0x80 : 0x40) | ((g_usbEndpointDataToggle[ep] & BIT4) ? 0x20 : 0x10));
		dataToggles = (((g_usbEndpointDataToggle[ep] & BIT5) ? 0x40 : 0x80) | ((g_usbEndpointDataToggle[ep] & BIT4) ? 0x10 : 0x20));
#if 1 /* Method 1 */
		debug("USB Host Controller: Updating EP%d data toggles to Snd %d and Rcv %d (0x%x)\r\n", ep, ((dataToggles & BIT7) ? 1 : 0), ((dataToggles & BIT5) ? 1 : 0), dataToggles);
		MAX_writeRegisterWithMask(rHCTL, dataToggles, 0x30);
		MAX_writeRegisterWithMask(rHCTL, dataToggles, 0xC0);
#elif 0 /* Method 2 */
		// Max datasheet says both Snd and Rcv should be set separate
		//if ((token == xfrSETUP) || (token == xfrIN) || (token == xfrINHS) || (token == xfrIN) || (token == xfrISOIN))
		if ((token == xfrIN) || (token == xfrINHS) || (token == xfrIN) || (token == xfrISOIN))
		{
			// Set the Rcv toggle
			debug("USB Host Controller: EP%d Rcv data toggle needs to be set %d, updating current Rcv toggle (0x%x)\r\n", ep, ((dataToggles & BIT5) ? 1 : 0), (dataToggles & 0x30));
			MAX_writeRegisterWithMask(rHCTL, dataToggles, 0x30);
		}
		else if ((token == xfrSETUP) || (token == xfrOUT) || (token == xfrOUTHS) || (token == xfrIN) || (token == xfrISOOUT))
		{
			// Set the Snd toggle
			debug("USB Host Controller: EP%d Snd data toggle needs to be set %d, updating current Snd toggle (0x%x)\r\n", ep, ((dataToggles & BIT7) ? 1 : 0), (dataToggles & 0xC0));
			MAX_writeRegisterWithMask(rHCTL, dataToggles, 0xC0);
		}
#endif
#endif
	}
#endif

#if 1 /* New add toggle setting here */
	if ((token == xfrIN) || (token == xfrINHS))
	{
		MAX_writeRegister(rHCTL, ((g_usbEndpointDataToggle[ep]) ? MAX_RCVTOG1 : MAX_RCVTOG0));
	}
	else if ((token == xfrOUT) || (token == xfrOUTHS))
	{
		MAX_writeRegister(rHCTL, ((g_usbEndpointDataToggle[ep]) ? MAX_SNDTOG1 : MAX_SNDTOG0));
	}
#endif

	/* Instruct the module to send the data as the specified type */
	MAX_writeRegister(rHXFR, token | (ep & 0x0F));

	//SysCtlDelay(1000);
	SoftUsecWait((1000 / MAX_TIMER_CONVERSION));

	timeout = 0xFFF; //0xFFFF; //0x3FFFF;
	while (timeout)
	{
		regval = MAX_readRegister(rHRSL) & 0x0F;

		if (regval == rslBUSY)
		{
			//SysCtlDelay(100);
			SoftUsecWait((100 / MAX_TIMER_CONVERSION));
		}
		else if (regval == rslNAK)
		{
			//debugRaw("<TP-N>");

			timeout--;
			MAX_writeRegister(rHXFR, token | ep);

			//SysCtlDelay(200);
			SoftUsecWait((200 / MAX_TIMER_CONVERSION));
		}
		else { break; }
	}

	//regval = MAX_readRegister(rHRSL) & 0x0F;
	if (regval)
	{
		debugErr("USB Host Controller: Error or timeout: %s (0x%x)\r\n", ResultCodeName(regval), regval);
	}

#if 0 /* Original */
	g_usbEndpointDataToggle[ep] = MAX_readRegister(rHRSL);
	debug("USB Host Controller: EP%d data toggles finished as Snd %d, Rcv %d (0x%x)\r\n", ep, ((g_usbEndpointDataToggle[ep] & BIT5) ? 1 : 0), ((g_usbEndpointDataToggle[ep] & BIT4) ? 1 : 0), (g_usbEndpointDataToggle[ep] & 0x30));
	//dataToggles = MAX_readRegister(rHRSL);
	//debug("USB Host Controller: EP%d data toggles finished as Snd %d, Rcv %d (0x%x)\r\n", ep, ((dataToggles & BIT5) ? 1 : 0), ((dataToggles & BIT4) ? 1 : 0), (dataToggles & 0x30));
#else
/*
	// save data toggle
	if (ep->ep_dir) {
	ep->data_toggle = (hrsl & HRSL_RCVTOGRD) ? 1u : 0u;
	}else {
	ep->data_toggle = (hrsl & HRSL_SNDTOGRD) ? 1u : 0u;
	}
*/
	if ((token == xfrIN) || (token == xfrINHS))
	{
		g_usbEndpointDataToggle[ep] = ((MAX_readRegister(rHRSL) & MAX_RCVTOGRD) ? 1u : 0u);
	}
	else if ((token == xfrOUT) || (token == xfrOUTHS))
	{
		g_usbEndpointDataToggle[ep] = ((MAX_readRegister(rHRSL) & MAX_SNDTOGRD) ? 1u : 0u);
	}
#endif

	return regval;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t transmitData(uint8_t token, uint8_t ep)
{
	uint8_t regval;
	uint32_t timeout;
	uint8_t stallEncountered = 0;

#if 0 /* New add toggle setting here */
	if ((token == xfrIN) || (token == xfrINHS))
	{
		MAX_writeRegister(rHCTL, ((g_usbEndpointDataToggle[ep]) ? MAX_RCVTOG1 : MAX_RCVTOG0));
	}
	else if ((token == xfrOUT) || (token == xfrOUTHS))
	{
		MAX_writeRegister(rHCTL, ((g_usbEndpointDataToggle[ep]) ? MAX_SNDTOG1 : MAX_SNDTOG0));
	}
#endif

	/* Instruct the module to send the data as the specified type */
	MAX_writeRegister(rHXFR, token | (ep & 0x0F));

	SoftUsecWait((1000 / MAX_TIMER_CONVERSION));

	timeout = 0xFFF; //0xFFFF; //0x3FFFF;
	while (timeout)
	{
		regval = MAX_readRegister(rHRSL) & 0x0F;

		if (regval == rslBUSY)
		{
			SoftUsecWait((100 / MAX_TIMER_CONVERSION));
		}
#if 0 /* Normal handle NAK by retrying */
		else if (regval == rslNAK)
#else
		else if ((regval == rslNAK) || (regval == rslSTALL))
#endif
		{
			if (regval == rslSTALL) { stallEncountered = 1; }
			//debugRaw("<TP-N>");

			timeout--;
			MAX_writeRegister(rHXFR, token | ep);

			//SysCtlDelay(200);
			SoftUsecWait((200 / MAX_TIMER_CONVERSION));
		}
		else { break; }
	}

	if (regval)
	{
		debugErr("USB Host Controller: Error or timeout: %s (0x%x)\r\n", ResultCodeName(regval), regval);
	}
	else if (stallEncountered) { debugWarn("USB Host Controller: Stall overcome with additional token send\r\n"); }

	if ((token == xfrIN) || (token == xfrINHS))
	{
		g_usbEndpointDataToggle[ep] = ((MAX_readRegister(rHRSL) & MAX_RCVTOGRD) ? 1u : 0u);
	}
	else if ((token == xfrOUT) || (token == xfrOUTHS))
	{
		g_usbEndpointDataToggle[ep] = ((MAX_readRegister(rHRSL) & MAX_SNDTOGRD) ? 1u : 0u);
	}

	return regval;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static uint32_t s_usbSuccess = 0;
static uint32_t s_usbFail = 0;
uint8_t requestData(uint8_t direction, uint8_t* rxbuffer, uint16_t nbytes, uint8_t performDataToggle, uint8_t debug)
{
	uint8_t readlength, result;
	uint32_t timeout = 0;
	uint16_t busyCount = 0;
	uint16_t nakCount = 0;
	uint16_t timeoutCount = 0;

#if 0 /* Original */
	/* Send a BULK-IN request packet */
	transmitPacket(xfrIN, usbMscBulkInEP); // Using the Out EP results in J-state error
#elif 0 /* Test the opposite */
	/* Send a BULK-OUT request packet */
	transmitPacket(xfrOUT, usbMscBulkOutEP);

	//if (direction == DIR_OUT) {	MAX_writeRegister(rHXFR, xfrOUT | (usbMscBulkOutEP & 0x0F)); debug("USB Host Controller: Bulk Out request\r\n"); }
	//else { MAX_writeRegister(rHXFR, xfrIN | (usbMscBulkInEP & 0x0F)); debug("USB Host Controller: Bulk In request\r\n"); }
#endif

#if 1 /* Test */
	if (!(MAX_readRegister(rHIRQ) & MAX_IRQ_HXFRDN))
	{
		debugWarn("USB Host Controller: BDI prior host transfer not done...\r\n");
		uint16 i = 1000;
		while (((MAX_readRegister(rHIRQ) & MAX_IRQ_HXFRDN) == 0) && (--i)) { SoftUsecWait(100); }

		if (MAX_readRegister(rHIRQ) & MAX_IRQ_HXFRDN) { debug("USB Host Controller: BDI is now available\r\n"); }
		else { debugErr("USB Host Controller: BDI not flagged available (timed out)\r\n"); }
	}
#endif

#if 0 /* Test */
	MAX_writeRegister(rPERADDR, PERIPHERAL_ADDRESS); // Shouldn't be needed since it's already set

	// Doesn't work
	//debug("USB Host Controller: Setting SNDBC to zero\r\n");
	//MAX_writeRegister(rSNDBC, 0);
#endif

	if (performDataToggle)
	//if (1)
	{
		// Set data toggle
		MAX_writeRegister(rHCTL, ((g_usbEndpointDataToggle[usbMscBulkInEP]) ? MAX_RCVTOG1 : MAX_RCVTOG0));
	}

	if (debug) { debug("USB Host Controller: Bulk In request %s(%d, %d)\r\n", ((nbytes == 13) ? "status " : ""), busyCount, nakCount); }

	SoftUsecWait(1 * SOFT_MSECS);

	while ((busyCount < 10) && (nakCount < 0xFF) && (timeoutCount < 50))
	{
		// Start the Bulk In request
		MAX_writeRegister(rHXFR, xfrIN | (usbMscBulkInEP & 0x0F));

		SoftUsecWait(1 * SOFT_MSECS);

		// Debug output delay seems to help here, adjust with delay if removed
		//if ((busyCount) || (nakCount)) { debugRaw(" (%d, %d)", busyCount, nakCount); }
		//SoftUsecWait(5000);

		/* Wait until we have a reply, or timeout */
		timeout = 100; //0xFFF; //0xFFFF;
		while ((!(MAX_readRegister(rHIRQ) & MAX_IRQ_RCVDAV)) && timeout)
		//while ((!(MAX_readRegister(rHIRQ) & MAX_IRQ_HXFRDN)) && timeout)
		{
			//SysCtlDelay(300);
			//SoftUsecWait((300 / MAX_TIMER_CONVERSION));
			SoftUsecWait(10);

			timeout--;
		}

#if 0 /* Moved lower */
		/* Quit if we got a timeout */
		if (!timeout)
		{
			debugWarn("USB Host Controller: Timeout out looking for a reply\r\n");
		}
		else
		{
			debug("USB Host Controller: Delay for data was %d us\r\n", ((0xFFFF - timeout) * 15));
		}
#endif
		/* Check the transfer result code: if it's an error, then quit */
		result = MAX_readRegister(rHRSL) & 0x0F;

		if (result == rslSUCCES)
		{
			// Done and ready for incoming data
			break;
		}
		else if (result == rslNAK)
		{
			//debugWarn("USB Host Controller: device NAK'ed\r\n");
			nakCount++;
			if (nbytes != 13) { return result; } // Only return immediantely on a NAK if not looking for status
		}
		else if (result == rslBUSY)
		{
			//debugWarn("USB Host Controller: Timed out while still busy\r\n");
			busyCount++;
		}
		else // Some other error code
		{
			debugErr("USB Host Controller: Error result %s (%d)\r\n", ResultCodeName(result), result);
			return (result);
		}

#if 1 /* Moved */
		/* Quit if we got a timeout */
		if (!timeout)
		{
			//debugWarn("USB Host Controller: Timeout out looking for a reply\r\n");
			//return result;
			timeoutCount++;
		}
		else
		{
			//debug("USB Host Controller: Delay for data was %d us\r\n", ((0xFFFF - timeout) * 15));
		}
#endif
	}

	if ((busyCount) || (nakCount) || (timeoutCount))
	{
		if (debug) { debugWarn("USB Host Controller: BDI not clean, B: %d, NAK: %d, TO: %d, Delay for data was %d us\r\n", busyCount, nakCount, timeoutCount, (((100 - timeout) + (timeoutCount * 100)) * 10)); }
	}

	// Test if this flag represents ACK
	//if ((MAX_readRegister(rHIRQ) & MAX_IRQ_SNDBAV) == 0) { debug("USB Host Controller: Peripheral answered with an ACK\r\n"); }
	//else { debugWarn("USB Host Controller: Send bytes flag not cleared, peripheral did not acknoledge request\r\n"); }

	if (MAX_readRegister(rHIRQ) & MAX_IRQ_RCVDAV) { } //{ debug("USB Host Controller: Data waiting to be read...\r\n"); }
	//else if (MAX_readRegister(rRCVBC)) { debugWarn("USB Host Controller: No rcv flag, but data bytes available...\r\n"); }
	else { debugWarn("USB Host Controller: No data received after request\r\n"); }

	if (MAX_readRegister(rHIRQ) & MAX_IRQ_RCVDAV)
	//if ((MAX_readRegister(rHIRQ) & MAX_IRQ_RCVDAV) || (MAX_readRegister(rRCVBC)))
	{
		/* This delay is apparently necessary, as the RX FIFO isn't directly ready (first byte will randomly corrupt) */
		//SysCtlDelay(500);
		//SoftUsecWait((500 / MAX_TIMER_CONVERSION));
		SoftUsecWait(1 * SOFT_MSECS);

		/* Get the length of the received data (should be the same as nbytes) */
		readlength = MAX_readRegister(rRCVBC);

		if (readlength != nbytes)
		{
#if 0 /* Original */
			debugErr("USB Host Controller: Error: expected %d bytes, but got %d\r\n", nbytes, readlength);
			return 0xF0;
#else
			debugWarn("USB Host Controller: Warning: expected %d bytes, but got %d\r\n", nbytes, readlength);
#endif
		}
		//totalRcvd += readlength;

		/* No error, so read the actual data */
		if (readlength <= nbytes)
		{
			MAX_multiReadRegister(rRCVFIFO, rxbuffer, readlength);
		}
		else // Incoming data is larger than the buffer size allocated
		{
			// Out of sync, flush the RX data
			uint8_t trashCan[64];
			MAX_multiReadRegister(rRCVFIFO, trashCan, readlength);
		}

#if 0 /* Unknown purpose */
		/* A simple test for now */
		/* TODO remove! */
		uint8_t it;
		for (it = 0; it < 64; it++)
		{
			if ( rxbuffer[it] > 9 )
			{
				debugErr("USB Host Controller: Byte error: %d (%d)...\r\n", rxbuffer[it], it);
			}
		}
#else
		if ((nbytes == 36) || (nbytes == 8)) // Filter for RX data output for all but Inquiry and Read capacity
		//if (1)
		{
			uint16_t i;
			//debugRaw("\r\nUSB Host Controller: RX Buffer: ");
			debug("USB Host Controller: RX Buffer: ");
			for (i = 0; i < readlength; i++)
			{
					debugRaw("%02x ", rxbuffer[i]);
			}
			debugRaw("<end>\r\n");
		}
#endif

#if 1 /* Test */
		if (readlength == 13)
		{
			uint32_t statusTag, dataResidue;
			memcpy(&statusTag, &rxbuffer[4], 4);
			memcpy(&dataResidue, &rxbuffer[8], 4);

			if ((statusTag == s_usbTag) && (dataResidue == 0)) { if (debug) { debug("USB Host Controller: Status verified (clean), tag: %d (total: %d)\r\n", statusTag, ++s_usbSuccess); } }
			else { debugWarn("USB Host Controller: Problem with status, tag %lu not %lu, data residue: %d (total err: %d)\r\n", statusTag, s_usbTag, dataResidue, ++s_usbFail); }
		}
#endif

		/* Clear the interrupt */
		MAX_writeRegister(rHIRQ, MAX_IRQ_RCVDAV);

		SoftUsecWait(1 * SOFT_MSECS);

		if (MAX_readRegister(rHIRQ) & MAX_IRQ_RCVDAV)
		{
			debugWarn("USB Host Controller: More data to receive...\r\n");

			readlength = MAX_readRegister(rRCVBC);
			MAX_multiReadRegister(rRCVFIFO, rxbuffer, readlength);
			uint16_t j;
			debugRaw("\r\nUSB Host Controller: RX Buffer: ");
			for (j = 0; j < readlength; j++)
			{
					debugRaw("%02x ", rxbuffer[j]);
			}
			debugRaw("<end>\r\n");

			/* Clear the interrupt */
			MAX_writeRegister(rHIRQ, MAX_IRQ_RCVDAV);
		}
	}
#if 0 /* Test peeking into the rcv buffer */
	else
	{
		uint8_t dryReadSize = 64;
		memset(rxbuffer, 0, dryReadSize);
		MAX_multiReadRegister(rRCVFIFO, rxbuffer, dryReadSize);
		debugRaw("\r\nUSB Host Controller: Dry read %d bytes of RX Buffer: ", dryReadSize);
		for (uint16_t i = 0; i < dryReadSize; i++)
		{
				debugRaw("%02x ", rxbuffer[i]);
		}
		debugRaw("<end>\r\n");

		/* Clear the interrupt */
		MAX_writeRegister(rHIRQ, MAX_IRQ_RCVDAV);
	}
#endif

#if 0 /* Test wild thought to add in handshake, doesn't seem to work, not supposed to handshake Bulk transfers */
	/* Send an HS-IN or HS-OUT. */
	if (direction == DIR_OUT)
	{
		debug("USB Host Controller: Bulk send handshake (In)\r\n");
		result = transmitPacket(xfrINHS, usbMscBulkInEP);
		//debug("USB Host Controller: Transmit packet HS-IN (response code %0x)\r\n", result);
		//result = transmitPacket(xfrOUTHS, usbMscBulkOutEP);
		//debug("USB Host Controller: Transmit packet HS-OUT (response code %0x)\r\n", result);
		//MAX_writeRegister(rHXFR, xfrINHS | (usbMscBulkInEP & 0x0F));
	}
	else
	{
		debug("USB Host Controller: Bulk send handshake (Out)\r\n");
		result = transmitPacket(xfrOUTHS, usbMscBulkOutEP);
		//debug("USB Host Controller: Transmit packet HS-OUT (response code %0x)\r\n", result);
		//result = transmitPacket(xfrINHS, usbMscBulkInEP);
		//debug("USB Host Controller: Transmit packet HS-IN (response code %0x)\r\n", result);
		//MAX_writeRegister(rHXFR, xfrOUTHS | (usbMscBulkOutEP & 0x0F));
	}
#endif

	result = MAX_readRegister(rHRSL);
#if 0 /* Original */
	g_usbEndpointDataToggle[usbMscBulkInEP] = result;
	debug("USB Host Controller: EP%d data toggles finished as Snd %d, Rcv %d (0x%x)\r\n", usbMscBulkInEP, ((g_usbEndpointDataToggle[usbMscBulkInEP] & BIT5) ? 1 : 0), ((g_usbEndpointDataToggle[usbMscBulkInEP] & BIT4) ? 1 : 0), (g_usbEndpointDataToggle[usbMscBulkInEP] & 0x30));
#else
	// Save data toggle
	g_usbEndpointDataToggle[usbMscBulkInEP] = (result & MAX_RCVTOGRD) ? 1u : 0u;
#endif

	return (result & 0x0F);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t MAX_getInterruptStatus(void)
{
	return MAX_readRegister(rHIRQ);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t MAX_getEnabledInterruptStatus(void)
{
	uint8_t result = MAX_getInterruptStatus();
	return result & enabledIRQ;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t MAX_getEPInterruptStatus(void)
{
	return MAX_readRegister(rEPIRQ);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t MAX_getEnabledEPInterruptStatus(void)
{
	uint8_t result = MAX_getEPInterruptStatus();
	return result & enabledEPIRQ;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MAX_disableEPInterrupts(uint8_t flags)
{
	/* Disable the interrupts */
	return;

	enabledEPIRQ &= ~flags;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t sendControl(ControlPacket* packet, uint8_t debug)
{
	uint8_t rescode;
	uint32_t timeout;

#if 1 /* Original */
	/* Make sure the peripheral address is correct */
	MAX_writeRegister(rPERADDR, packet->perAddress);
	if (debug) { debug("USB Host Controller: Peripheral Addr set (%d)\r\n", packet->perAddress); }
#else /* Test without setting addr for Device Descriptor but that shouldn't be valid */
	if (packet->bRequest != 6)
	{
		/* Make sure the peripheral address is correct */
		MAX_writeRegister(rPERADDR, packet->perAddress);
		debug("USB Host Controller: Peripheral Addr set\r\n");
	}
	else
	{
		debug("USB Host Controller: Skipping peripheral addr set for Get Descriptor\r\n");
	}
#endif

#if 1 /* Test setting data toggle for control to start at 1 */
/*
	if ( ep_num == 0 ) {
		ep->data_toggle = 1;
	}
*/
	g_usbEndpointDataToggle[0] = 1;
#endif

	/* Load the contents from the given packet and send this as a Control packet, should be setup little endian */
	TXData[0] = packet->bmRequestType;
	TXData[1] = packet->bRequest;
	TXData[2] = (uint8_t) ((packet->wValue & 0xFF));
	TXData[3] = (uint8_t) (packet->wValue >> 8);
	TXData[4] = (uint8_t) ((packet->wIndex & 0xFF));
	TXData[5] = (uint8_t) (packet->wIndex >> 8);
	TXData[6] = (uint8_t) ((packet->wLength & 0xFF));
	TXData[7] = (uint8_t) (packet->wLength >> 8);

	if (debug) { debug("USB Host Controller: TX packet %02x %02x %02x %02x %02x %02x %02x %02x\r\n", TXData[0], TXData[1], TXData[2], TXData[3], TXData[4], TXData[5], TXData[6], TXData[7]); }

	/* Write the data into the SUPFIFO */
	MAX_multiWriteRegister(rSUDFIFO, (uint8_t*)TXData, 8);

	if (debug) { debug("USB Host Controller: Sending bRequest: 0x%x. (addr %d)\r\n", packet->bRequest, packet->perAddress); }

	/* Start the transaction */
	rescode = transmitPacket(xfrSETUP, 0);
	if (rescode) { debugErr("USB Host Controller: Transmit packet (Setup) retrn code (%d)\r\n", rescode); return rescode; }
	else { if (debug) { debug("USB Host Controller: Transmit packed (Setup) success\r\n"); } }

	/* Check whether we need a data stage (request only at the moment) and perform */
	if ((packet->wLength > 0) && (packet->direction == DIR_IN))
	{
		//if (packet->bRequest == reqGET_STATUS) { rescode = transmitPacket(xfrIN, 0); }
		//else { rescode = transmitPacket(xfrIN, 0); }
		rescode = transmitPacket(xfrIN, 0);

		if (rescode) { debugErr("USB Host Controller: Transmit packet (In) retrn code (%d)\r\n", rescode); return rescode; }
		else { if (debug) { debug("USB Host Controller: Transmit packed (In) success\r\n"); } }

		timeout = 0x1FFFF;
		while (!(MAX_readRegister(rHIRQ) & MAX_IRQ_RCVDAV) && timeout)
		{
			timeout--;
			//SysCtlDelay(100);
			SoftUsecWait((100 / MAX_TIMER_CONVERSION));
			if ((timeout % 6000) == 0) { debugRaw("."); }
		}

		if (timeout == 0)
		{
			debugErr("USB Host Controller: Timeout hit, HIRQ: 0x%x\r\n", MAX_readRegister(rHIRQ));
		}

		/* Check if we got data and read if available */
		if (MAX_readRegister(rHIRQ) & MAX_IRQ_RCVDAV)
		{
			memset((uint8_t*)ControlBuffer, 0, sizeof(ControlBuffer));

			lastReadSize = MAX_readRegister(rRCVBC);
			if (debug) { debug("USB Host Controller: Read length %d bytes\r\n", lastReadSize); }

			MAX_multiReadRegister(rRCVFIFO, (uint8_t*)ControlBuffer, lastReadSize);

			if (lastReadSize)
			{
				//debugRaw("\r\nUSB Host Controller: Got control data: ");
				if (debug)
				{
					debug("USB Host Controller: Got control data: ");
					uint8_t i = 0;
					while (i < lastReadSize)
					{
						debugRaw("%02x ", ControlBuffer[i++]);
					}
					debugRaw("\r\n");
				}
			}

			MAX_writeRegister(rHIRQ, MAX_IRQ_RCVDAV);

#if 1 /* Test if there's data in the second buffer */
			SoftUsecWait(1 * SOFT_MSECS);

			if (MAX_readRegister(rHIRQ) & MAX_IRQ_RCVDAV)
			{
				debugWarn("USB Host Controller: More data to receive...\r\n");

				lastReadSize = MAX_readRegister(rRCVBC);
				debugWarn("USB Host Controller: Second Read length %d bytes\r\n", lastReadSize);

				MAX_multiReadRegister(rRCVFIFO, (uint8_t*)ControlBuffer, lastReadSize);

				if (lastReadSize)
				{
					//debugRaw("\r\nUSB Host Controller: Got second control data: ");
					debugWarn("USB Host Controller: Got second control data: ");
					uint8_t i = 0;
					while (i < lastReadSize)
					{
						debugRaw("%02x ", ControlBuffer[i++]);
					}
					debugRaw("\r\n");
				}

				/* Clear the interrupt */
				MAX_writeRegister(rHIRQ, MAX_IRQ_RCVDAV);
			}
#endif
		}
	}

	/* Send an HS-IN or HS-OUT. */
	if (packet->direction == DIR_OUT)
	{
		rescode = transmitPacket(xfrINHS, 0);
		if (debug) { debug("USB Host Controller: Transmit packet HS-IN (response code %0x)\r\n", rescode); }
	}
	else
	{
		rescode = transmitPacket(xfrOUTHS, 0);
		if (debug) { debug("USB Host Controller: Transmit packet HS-OUT (response code %0x)\r\n", rescode); }
	}

	return rescode;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_getDeviceDescriptorShortRequest(uint8_t peraddress, uint8_t debug)
{
	ControlPacket addrPacket =
	{
		peraddress, /* perAddress */
		0x10, /* type */
		0, /* endPoint */
		0x80, /* bmRequestType */
		reqGET_DESCRIPTOR, /* bRequest */
		0x0100, /* wValue */
		0x0000, /* wIndex */
		0x0008, /* wLength */
		DIR_IN /* direction */
	};

	return sendControl(&addrPacket, debug);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_getDeviceDescriptorRequest(uint8_t peraddress, uint8_t debug)
{
	ControlPacket addrPacket =
	{
		peraddress, /* perAddress */
		0x10, /* type */
		0, /* endPoint */
		0x80, /* bmRequestType */
		reqGET_DESCRIPTOR, /* bRequest */
		0x0100, /* wValue */
		0x0000, /* wIndex */
		0x0040, /* wLength */
		DIR_IN /* direction */
	};

	return sendControl(&addrPacket, debug);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_getConfigurationDescriptorRequest(uint8_t peraddress, uint8_t debug)
{
	ControlPacket addrPacket =
	{
		peraddress, /* perAddress */
		0x10, /* type */
		0, /* endPoint */
		0x80, /* bmRequestType */
		reqGET_DESCRIPTOR, /* bRequest */
		0x0200, /* wValue */
		0x0000, /* wIndex */
		0x0009, /* wLength */
		DIR_IN /* direction */
	};

	return sendControl(&addrPacket, debug);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_getConfigurationDescriptorFullRequest(uint8_t peraddress, uint8_t length, uint8_t debug)
{
	ControlPacket addrPacket =
	{
		peraddress, /* perAddress */
		0x10, /* type */
		0, /* endPoint */
		0x80, /* bmRequestType */
		reqGET_DESCRIPTOR, /* bRequest */
		0x0200, /* wValue */
		0x0000, /* wIndex */
		length, /* wLength */
		DIR_IN /* direction */
	};

	return sendControl(&addrPacket, debug);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_getStringDescriptorRequest(uint8_t peraddress, uint8_t debug)
{
	ControlPacket addrPacket =
	{
		peraddress, /* perAddress */
		0x10, /* type */
		0, /* endPoint */
		0x80, /* bmRequestType */
		reqGET_DESCRIPTOR, /* bRequest */
		0x0300, /* wValue */
		0x0000, /* wIndex */
		0x0010, /* wLength */
		DIR_IN /* direction */
	};

	return sendControl(&addrPacket, debug);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_clearStallRequest(uint8_t peraddress, uint8_t endpoint, uint8_t debug)
{
	ControlPacket addrPacket =
	{
		peraddress, /* perAddress */
		0x10, /* type */
		endpoint, /* endPoint */
		0x00, /* bmRequestType */
#if 1 /* Original */
		reqCLEAR_FEATURE, /* bRequest */
#if 1 /* Original */
		0x0001, /* wValue */
#else /* Test 0 */
		0x0000, /* wValue */
#endif
#else /* Test */
		reqSET_FEATURE, /* bRequest */
		0x0000, /* wValue */
#endif
		endpoint, /* wIndex */
		0x0, /* wLength */
		DIR_IN /* direction */
	};

	if (debug) { debug("USB Host Controller: Clear Stall Request for endpoint %d (%d)\r\n", endpoint, addrPacket.wIndex); }

	return sendControl(&addrPacket, debug);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#define MSC_REQ_GET_MAX_LUN	254
uint8_t USB_getMaxLunRequest(uint8_t peraddress, uint8_t debug)
{
	ControlPacket addrPacket =
	{
		peraddress, /* perAddress */
		0x10, /* type */
		0, /* endPoint */
		0xA1, /* bmRequestType */
		MSC_REQ_GET_MAX_LUN, /* bRequest */
		0x0, /* wValue */
		usbMscInterfaceID, /* wIndex */
		0x1, /* wLength */
		DIR_IN /* direction */
	};

	return sendControl(&addrPacket, debug);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_setNewPeripheralAddress(uint8_t peraddress, uint8_t debug)
{
	ControlPacket addrPacket =
	{
		0, /* perAddress */
		0x10, /* type */
		0, /* endPoint */
		0, /* bmRequestType */
		reqSET_ADDRESS, /* bRequest */
		peraddress, /* wValue */
		0, /* wIndex */
		0, /* wLength */
		DIR_OUT /* direction */
	};

	return sendControl(&addrPacket, debug);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_setDeviceConfiguration(uint8_t peraddress, uint8_t config, uint8_t debug)
{
	ControlPacket packet =
	{
		peraddress, /* perAddress */
		0x10, /* type */
		0, /* endPoint */
		0, /* bmRequestType */ /* recipient = USB_REQ_RCPT_DEVICE, type = USB_REQ_TYPE_STANDARD, direction = USB_DIR_OUT */
		reqSET_CONFIGURATION, /* bRequest */
		config, /* wValue */
		0, /* wIndex */
		0, /* wLength */
		DIR_OUT /* direction */
	};

	return sendControl(&packet, debug);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_requestStatus(uint8_t* resultBuffer, uint8_t debug)
{
	ControlPacket packet =
	{
		PERIPHERAL_ADDRESS, /* perAddress */
		0x10, /* type */
		0, /* endPoint */
		0x80, /* bmRequestType */
		reqGET_STATUS, /* bRequest */
		0, /* wValue */
		0, /* wIndex */
		2, /* wLength */
		DIR_IN
	};

	return sendControl(&packet, debug);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint32_t usbStallsEncountered = 0;
uint8_t USB_handleStallwithClear(uint8_t endpoint, uint8_t debug)
{
	uint8_t rescode;

#if 0 /* Original */
	debugWarn("USB Host Controller: Stall detected on EP%d, attempting to clean...\r\n", endpoint);
	SoftUsecWait(50 * SOFT_MSECS);

#if 1 /* Test */
extern uint8_t USB_getStatus(uint8_t debug);
extern uint8_t USB_testUnitReady(uint8_t debug);
extern uint8_t USB_readSector(uint32_t sector, uint8_t* rxBuffer, uint8_t debug);
extern uint8_t USB_getMaxLun(uint8_t debug);
extern uint8_t USB_getStringDescriptor(uint8_t debug);
	uint16_t i;
	for (i = 0; i < 8; i++) { USB_getStatus(YES); }
	USB_getMaxLun(YES);
	USB_getStringDescriptor(YES);
	//for (i = 0; i < 8; i++) { USB_testUnitReady(YES); }
	//USB_readSector(0, &g_spareBuffer[1000], YES);
#endif

//extern void SetDataToggleForEndpoint(uint8_t endpoint);
	//SetDataToggleForEndpoint(0);
	rescode = USB_clearStallRequest(PERIPHERAL_ADDRESS, endpoint);

#if 1 /* Test */
	for (i = 0; i < 8; i++) { USB_getStatus(YES); }
	USB_getMaxLun(YES);
	USB_getStringDescriptor(YES);
#endif

	if (rescode == rslSUCCES)
	{
		debug("USB Host Controller: Clear request success, resetting toggles for EP%d\r\n", endpoint);
		g_usbEndpointDataToggle[endpoint] = 0x00;
	}
	else
	{
		debugWarn("USB Host Controller: Clear request failed\r\n");
	}

	SoftUsecWait(50 * SOFT_MSECS);
#else /* Test full bus reset and re-enumerate device */
#if 0 /* First method */
	uint16_t i = 0;
	peripheralAvailable = 0;

	while ((!peripheralAvailable) && (i++ < 50))
	{
		USB_busReset(YES);
		MAX_checkBusState(YES);
		if (!peripheralAvailable) { debugWarn("USB Host Controller: Bus reset %d did not find a peripheral\r\n", i); }
		SoftUsecWait(18 * i * SOFT_MSECS); // Cumulative time gets close to 1 second
	}

	/* Perform a bus reset to reconnect after a power down */
	if (!peripheralAvailable)
	{
		debugErr("USB Host Controller: Stall recovery through re-enumeration failed\r\n");
		return (rslSTALL);
	}
#else /* Second method */
	// Prevent handling Stalls on the Control endpoint since this can be recursive and the Control endpoint doesn't seem to stall out
	if (endpoint == 0) { return (0); }

	uint16_t i = 1;
	rescode = rslSTALL;
	while (rescode == rslSTALL)
	{
		usbStallsEncountered++;

		if (debug)
		{
			debug("USB Host Controller: ----------------\r\n");
			debug("USB Host Controller: Stall recovery with Bus Reset and Re-Enumeration (Attempt %d)...\r\n", i++);
			debug("USB Host Controller: ----------------\r\n");
		}

		USB_busReset(debug);
#endif
extern void MAX_processNewDevice(uint8_t);
		MAX_processNewDevice(debug);

		// Reset to initial toggle state if test is run multiple times
		memset(&g_usbEndpointDataToggle[0], 0, 3);

		// Need a slight delay after process new device before send buffer is flagged available

extern uint8_t USB_setConfiguration(uint8_t debug);
		rescode = USB_setConfiguration(debug); if (rescode) { debugWarn("USB Host Controller: Recovery Set Config returned code %s (%d)\r\n", ResultCodeName(rescode), rescode); }
#endif

extern uint8_t USB_testUnitReady(uint8_t debug);
		rescode = USB_testUnitReady(debug); if (rescode) { debugWarn("USB Host Controller: First TUR returned code %s (%d)\r\n", ResultCodeName(rescode), rescode); }
		rescode = USB_testUnitReady(debug);
	}

	if (rescode) { debugWarn("USB Host Controller: Stall recovery not cleared %s (%d)\r\n", ResultCodeName(rescode), rescode); }
	else { debug("USB Host Controller: Stall recovery cleared, device ready\r\n"); }

	return (rescode);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_doEnumeration(uint8_t debug)
{
	uint16_t tries = 0;

#if 1 /* Test removal to transmit packet level */
#if 1 /* Original */
	MAX_enableOptions(rHCTL, BIT7); MAX_disableOptions(rHCTL, BIT6);
	MAX_enableOptions(rHCTL, BIT5); MAX_disableOptions(rHCTL, BIT4);
#else
	//MAX_writeRegisterWithMask(rHCTL, 0x50, 0xF0);
	MAX_writeRegisterWithMask(rHCTL, 0xA0, 0xF0);
#endif
#endif

	while (tries < 20)
	{
		if (tries)
		{
			debugErr("USB Host Controller: Enumeration failed. Retrying...\r\n");
			USB_busReset(YES);

			//SysCtlDelay(4000000);
			SoftUsecWait((4000000 / MAX_TIMER_CONVERSION));
		}

		tries++;
		MAX_writeRegister(rPERADDR, 0);

		if (!USB_setNewPeripheralAddress(PERIPHERAL_ADDRESS, debug))
		{
			MAX_writeRegister(rPERADDR, PERIPHERAL_ADDRESS);

			//SysCtlDelay(500000);
			SoftUsecWait((500000 / MAX_TIMER_CONVERSION));
		}
		else
		{
			continue;
		}

		if (!USB_requestStatus(0, debug))
		{
			break;
		}
	}

	if (tries < 20)
	{
#if 0 /* Test removal to transmit packet level */
#if 1 /* Original */
		MAX_enableOptions(rHCTL, BIT6);
		MAX_disableOptions(rHCTL, BIT7);
		MAX_enableOptions(rHCTL, BIT4);
		MAX_disableOptions(rHCTL, BIT5);
#else
		//MAX_writeRegisterWithMask(rHCTL, 0xA0, 0xF0);
		MAX_writeRegisterWithMask(rHCTL, 0x50, 0xF0);
#endif

		g_usbEndpointDataToggle[0] = MAX_readRegister(rHRSL);
#endif
		return 0;
	}
	else
	{
		return 1;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MAX_ISR(void)
{
	debugRaw("-USBHC ISR-");

	uint8_t regval, USBStatus;

	/* Get the IQR status */
	USBStatus = MAX_getEnabledInterruptStatus();

#if 0 /* Device mode */
	uint8_t USBEPStatus;
	USBEPStatus = MAX_getEnabledEPInterruptStatus();

	/* Peripheral: we got a setup package */
	if (USBEPStatus & MAX_IRQ_SUDAV)
	{
		MAX_writeRegister(rEPIRQ, BIT5);
		MAX_multiReadRegister(4, (uint_fast8_t *) RXData, 8);

		switch (RXData[1])
		{
			case reqSET_ADDRESS:
				ACKSTAT = true;
				MAX_readRegister(19);
				break;
			case reqGET_STATUS:
				USB_respondStatus((uint_fast8_t *) RXData);
				break;
		}
	}

	/* Peripheral: the buffer is available again */
	if (USBEPStatus & MAX_IRQ_IN2BAV)
	{
		MAX_disableEPInterrupts(MAX_IRQ_IN2BAV);
	}

	/* Peripheral: a bus reset was commanded */
	if ( !mode && USBStatus & MAX_IRQ_URESDN ) {
		MAX_writeRegister(rUSBIRQ, MAX_IRQ_URESDN);

		/* Reconfigure the interrupts after a reset */
		MAX_enableEPInterrupts(MAX_IRQ_SUDAV);
		MAX_clearEPInterruptStatus(MAX_IRQ_SUDAV);
		MAX_enableInterrupts(MAX_IRQ_URESDN);
		MAX_clearInterruptStatus(MAX_IRQ_URESDN);

		/* Re-enable EP2 */
		MAX_writeRegister(rEP2INBC, 64);
	}
#endif

	/* Host: a peripheral connected or disconnected */
	if (USBStatus & MAX_IRQ_CONDET)
	{
		regval = MAX_readRegister(rHRSL);

		if (regval & 0xC0)
		{
			peripheralConnected = 1;

			/* Enable the SOF generator */
			MAX_enableOptions(rMODE, BIT3);
			while (!(MAX_readRegister(rHIRQ) & MAX_IRQ_FRAME)) { ; }

			USB_busReset(YES);

			//SysCtlDelay(4000000);
			SoftUsecWait((4000000 / MAX_TIMER_CONVERSION));

			if (USB_doEnumeration(NO))
				debugErr("USB Host Controller ISR: Enumeration Failed...\r\n");
			else
				debug("USB Host Controller ISR: Done with enumeration\r\n");

			/* Add a delay to stabilise the bus */
			//SysCtlDelay(8000000);
			SoftUsecWait((8000000 / MAX_TIMER_CONVERSION));
		}
		else
		{
			peripheralConnected = 0;
			/* Disable the SOF generator */
			MAX_disableOptions(rMODE, BIT3);
		}

		//if (handlePtr != 0) { handlePtr((uint_fast8_t) peripheralConnected); }
		MAX_checkBusState(NO);

		MAX_writeRegister(rHIRQ, BIT5);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MAX_processNewDevice(uint8_t debug)
{
	uint8_t regval;

	regval = MAX_readRegister(rHRSL);

	if (regval & 0xC0)
	{
		if (debug) { debug("USB Host Controller: Peripheral connected\r\n"); }
		peripheralConnected = 1;

		/* Enable the SOF generator */
		MAX_enableOptions(rMODE, BIT3);
		while (!(MAX_readRegister(rHIRQ) & MAX_IRQ_FRAME)) { ; }

#if 1 /* Original */
		USB_busReset(debug);
#else /* Try to see operation without an additional bus reset */
#endif
		//SysCtlDelay(4000000);
		SoftUsecWait((4000000 / MAX_TIMER_CONVERSION));

		if (USB_doEnumeration(debug))
			debugErr("USB Host Controller: Enumeration Failed...\r\n");
		else if (debug) { debug("USB Host Controller: Done with enumeration\r\n"); }

		/* Add a delay to stabilise the bus */
		//SysCtlDelay(8000000);
		SoftUsecWait((8000000 / MAX_TIMER_CONVERSION));
	}
	else
	{
		debugWarn("USB Host Controller: Peripheral not found\r\n");
		peripheralConnected = 0;
		/* Disable the SOF generator */
		MAX_disableOptions(rMODE, BIT3);
	}

	//if (handlePtr != 0) { handlePtr((uint_fast8_t) peripheralConnected); }
	MAX_checkBusState(NO);

	MAX_writeRegister(rHIRQ, BIT5);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetDataToggles(uint8_t snd, uint8_t rcv)
{
	uint8_t setToggles = 0;

#if 0 /* Straight */
	if (snd) { setToggles |= BIT7; } else { setToggles |= BIT6; }
	if (rcv) { setToggles |= BIT5; } else { setToggles |= BIT4; }
#else /* Flip */
	if (snd) { setToggles |= BIT6; } else { setToggles |= BIT7; }
	if (rcv) { setToggles |= BIT4; } else { setToggles |= BIT5; }
#endif
	MAX_writeRegisterWithMask(rHCTL, setToggles, 0xF0);
	debug("USB Host Controller: setting HCTL to 0x%x\r\n", setToggles);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t GetDataToggles(void)
{
	uint8_t getToggles = MAX_readRegister(rHCTL);
	debug("USB Host Controller: Getting data toggles, Snd: %d, Rcv: %d\r\n", (getToggles & BIT5), (getToggles & BIT4));
	return (getToggles);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void CheckDataToggleAndSet(void)
{
	uint8_t readToggles = MAX_readRegister(rHRSL);

	if ((readToggles & BIT5) && (readToggles & BIT4)) { debug("USB Host Controller: SND and RCV toggles both 1 (0x%0x), setting as such\r\n", readToggles); SetDataToggles(1, 1); }
	else if (readToggles & BIT5) { debug("USB Host Controller: SND is 1, RCV is 0 (0x%0x), setting toggles as such\r\n", readToggles); SetDataToggles(1, 0); }
	else if (readToggles & BIT4) { debug("USB Host Controller: SND is 0, RCV is 1 (0x%0x), setting toggles as such\r\n", readToggles); SetDataToggles(0, 1);}
	else /* readToggles are zero */ { debug("USB Host Controller: SND and RCV toggles both 0 (0x%0x), setting as such\r\n", readToggles); SetDataToggles(0, 0); }

}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetDataToggleForEndpoint(uint8_t endpoint)
{
	uint8_t dataToggles;
	if (g_usbEndpointDataToggle[endpoint] == 0xFF) { g_usbEndpointDataToggle[endpoint] = 0x30; }
#if 1 /* Flip toggles */
	dataToggles = (((g_usbEndpointDataToggle[endpoint] & BIT5) ? 0x40 : 0x80) | ((g_usbEndpointDataToggle[endpoint] & BIT4) ? 0x10 : 0x20));
#else /* Keep same produces lots of toggle errors */
	dataToggles = (((g_usbEndpointDataToggle[endpoint] & BIT5) ? 0x80 : 0x40) | ((g_usbEndpointDataToggle[endpoint] & BIT4) ? 0x20 : 0x10));
#endif
	debug("USB Host Controller: Updating EP%d data toggles to Snd %d and Rcv %d (0x%x)\r\n", endpoint, ((dataToggles & BIT7) ? 1 : 0), ((dataToggles & BIT5) ? 1 : 0), dataToggles);

#if 0 /* Original set separate since datasheet says only supposed to set one pair at a time */
	MAX_writeRegisterWithMask(rHCTL, dataToggles, 0x30);
	MAX_writeRegisterWithMask(rHCTL, dataToggles, 0xC0);
#else /* Attempt both toggles set in one write */
	MAX_writeRegisterWithMask(rHCTL, dataToggles, 0xF0);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
char* GetUsbClassName(uint8_t class)
{
	char* className = NULL;

	switch (class)
	{
		case USB_ADC_CLASS: className = USB_ADC_CLASS_NAME; break;
		case USB_CDC_CLASS: className = USB_CDC_CLASS_NAME; break;
		case USB_HID_CLASS: className = USB_HID_CLASS_NAME; break;
		case USB_PRT_CLASS: className = USB_PRT_CLASS_NAME; break;
		case USB_MSC_CLASS: className = USB_MSC_CLASS_NAME; break;
		default: className = USB_UNKNOWN_NAME; break;
	}

	return (className);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
char* GetUsbTransferTypeName(uint8_t transferType)
{
	char* transferTypeName = NULL;

	switch (transferType)
	{
		case USB_TRANSFER_CONTROL: transferTypeName = USB_TRANSFER_CONTROL_NAME; break;
		case USB_TRANSFER_ISOCHRONOUS: transferTypeName = USB_TRANSFER_ISOCHRONOUS_NAME; break;
		case USB_TRANSFER_BULK: transferTypeName = USB_TRANSFER_BULK_NAME; break;
		case USB_TRANSFER_INTERRUPT: transferTypeName = USB_TRANSFER_INTERRUPT_NAME; break;
		default: transferTypeName = USB_UNKNOWN_NAME; break;
	}

	return (transferTypeName);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DecodeFullConfigDescriptor(uint8_t fullConfigLength)
{
	/*
		Example: USB Host Controller: Got control data: 09 02 20 00 01 01 00 80 32 09 04 00 00 02 08 06 50 00 07 05 01 02 40 00 00 07 05 82 02 40 00 00 00

		---------------- Configuration Descriptor -----------------
		0 bLength                  : 0x09 (9 bytes)
		1 bDescriptorType          : 0x02 (Configuration Descriptor)
		2 wTotalLength             : 0x0020 (32 bytes)
		4 bNumInterfaces           : 0x01 (1 Interface)
		5 bConfigurationValue      : 0x01 (Configuration 1)
		6 iConfiguration           : 0x00 (No String Descriptor)
		7 bmAttributes             : 0x80
		8 MaxPower                 : 0x32 (100 mA)

		---------------- Interface Descriptor -----------------
		0 bLength                  : 0x09 (9 bytes)
		1 bDescriptorType          : 0x04 (Interface Descriptor)
		2 bInterfaceNumber         : 0x00 (Interface 0)
		3 bAlternateSetting        : 0x00
		4 bNumEndpoints            : 0x02 (2 Endpoints)
		5 bInterfaceClass          : 0x08 (Mass Storage)
		6 bInterfaceSubClass       : 0x06 (SCSI transparent command set)
		7 bInterfaceProtocol       : 0x50 (Bulk-Only Transport)
		8 iInterface               : 0x00 (No String Descriptor)

		----------------- Endpoint Descriptor -----------------
		0 bLength                  : 0x07 (7 bytes)
		1 bDescriptorType          : 0x05 (Endpoint Descriptor)
		2 bEndpointAddress         : 0x01 (Direction=OUT EndpointID=1)
		3 bmAttributes             : 0x02 (TransferType=Bulk)
		4 wMaxPacketSize           : 0x0200 (max 512 bytes)
		5 bInterval                : 0x00 (never NAKs)
	*/

	uint8_t* configPayload = (uint8_t*)&ControlBuffer[0];

	debug("USB Host Controller: -------- Configuration --------\r\n");
	while (fullConfigLength)
	{
		if (configPayload[1] == 0x02) // Configuration descriptor
		{
			sprintf((char*)g_debugBuffer, "Configuration %d, Num of Interfaces: %d, Max power: %d mA", configPayload[5], configPayload[4], (configPayload[8] * 2));
			debug("USB Host Controller: %s\r\n", (char*)g_debugBuffer);
#if 0 /* Disable for faster testing */
			OverlayMessage(getLangText(STATUS_TEXT), (char*)g_debugBuffer, (3 * SOFT_SECS));
#endif
			usbMscConfigID = configPayload[5];
		}
		else if (configPayload[1] == 0x04) // Interface descriptor
		{
			sprintf((char*)g_debugBuffer, "Interface %d, Num of Endpoints: %d, Class: %s", configPayload[2], configPayload[4], GetUsbClassName(configPayload[5]));
			debug("USB Host Controller: %s\r\n", (char*)g_debugBuffer);
#if 0 /* Disable for faster testing */
			OverlayMessage(getLangText(STATUS_TEXT), (char*)g_debugBuffer, (3 * SOFT_SECS));
#endif
			usbMscInterfaceID = configPayload[2];
		}
		else if (configPayload[1] == 0x05) // Endpoint descriptor
		{
			sprintf((char*)g_debugBuffer, "Endpoint %d, %s, %s, Max size %d", (configPayload[2] & 0x0F), ((configPayload[2] & 0x80) ? "In" : "Out"), GetUsbTransferTypeName(configPayload[3]), configPayload[4]);
			debug("USB Host Controller: %s\r\n", (char*)g_debugBuffer);
#if 0 /* Disable for faster testing */
			OverlayMessage(getLangText(STATUS_TEXT), (char*)g_debugBuffer, (2 * SOFT_SECS));
#endif
			if (configPayload[3] == USB_TRANSFER_BULK)
			{
				if (configPayload[2] & 0x80) { usbMscBulkInEP = (configPayload[2] & 0x0F); }
				else { usbMscBulkOutEP = configPayload[2]; }
			}
		}

		fullConfigLength -= configPayload[0];
		configPayload += configPayload[0];
	}
}

/*
	Notes: A Command Block Wrapper (CBW) in USB Bulk-Only Transport (BOT) is a 31-byte structure sent from the host to the device to initiate a command.
	Structure of CBW

	The CBW consists of several fields that define the command and its parameters. Here’s a breakdown of its components:
	Field Name	Size (Bytes)	Description
	Signature				4	A unique identifier for the CBW, typically set to "USBC"
	Tag						4	A unique identifier for the command, used for matching with the Command Status Wrapper (CSW)
	Data Transfer Length	4	The total number of bytes to be transferred
	Flags					1	Indicates the direction of data transfer (IN or OUT)
	LUN						1	Logical Unit Number, identifying the specific device
	Command Length			1	The length of the command block that follows
	Command Block			16	The actual command to be executed on the mass storage device

	Example of a CBW : An example of a CBW might look like this:
	Signature: 				0x43425355 (ASCII for "USBC")
	Tag: 					0x00000001 (Unique command identifier)
	Data Transfer Length: 	0x00000010 (16 bytes to be transferred)
	Flags: 					0x00 (Data transfer from host to device)
	LUN: 					0x00 (First logical unit)
	Command Length: 		0x0A (10 bytes for the command)
	Command Block: 			0x28 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 (Example command for a read operation)
*/

/*
	Notes: A Command Status Wrapper (CSW) in USB Bulk-Only Transfer (BOT) is a 13-byte packet sent by the device to the host to indicate the status of a command.
	Structure of the CSW

	The CSW consists of several fields that provide essential information about the command execution. Below is a breakdown of its structure:
	Field Name	Size (Bytes)	Description
	Signature		4	A unique identifier for the CSW, typically set to "CSW"
	Tag				4	A unique identifier that matches the Command Block Wrapper (CBW)
	Data Residue	4	The number of bytes not transferred in the data phase
	Status			1	Indicates the success or failure of the command execution

	Example of a CSW Packet : Here is an example of a CSW packet with hypothetical values:
	Field Name	Value	Description
	Signature		0x53425355	"CSW" in ASCII (Little Endian)
	Tag				0x00000001	Matches the corresponding CBW tag
	Data Residue	0x00000004	4 bytes not transferred
	Status			0x00	Command executed successfully
*/

#define CBWFLAGS_DIR_IN         0x80 // For data-in operation, Indicates that data is being sent from the device to the host
#define CBWFLAGS_DIR_OUT        0x00 // For data-out operation, Indicates that data is being sent from the host to the device

// Command Descriptor Block for 6-byte command
typedef struct __attribute__((packed)) {
	uint8_t opCode;			// Operation code
	uint8_t lunAndLbaMsb;	// Logical unit number (first 3 bits, last 5 bits are LBA)
	uint8_t lbaMsb;			// Logical block address, MSB (Big Endian)
	uint8_t lbaLsb;			// Logical block address, LSB (Big Endian)
	uint8_t xferLength;		// Transfer length
	uint8_t control;		// Control
	uint8_t pad[10];		// Padding for 16-byte command block
} USB_CBW_SCSI_6;

// Command Descriptor Block for 10-byte command
typedef struct __attribute__((packed)) {
	uint8_t opCode;			// Operation code
	uint8_t lun;			// Logical unit number (first 3 bits)
	uint8_t lba1;			// Logical block address, MSB (Big Endian)
	uint8_t lba2;			// Logical block address, 2nd MSB (Big Endian)
	uint8_t lba3;			// Logical block address, 2nd LSB (Big Endian)
	uint8_t lba4;			// Logical block address, LSB (Big Endian)
	uint8_t reserved;		// Reserved
	uint8_t xferLen1;		// Transfer length, MSB (Big Endian)
	uint8_t xferLen2;		// Transfer length, LSB (Big Endian)
	uint8_t control;		// Control
	uint8_t pad[6];			// Padding for 16-byte command block
} USB_CBW_SCSI_10;

// Command Descriptor Block for 12-byte command
typedef struct __attribute__((packed)) {
	uint8_t opCode;			// Operation code
	uint8_t lun;			// Logical unit number (first 3 bits)
	uint8_t lba1;			// Logical block address, MSB (Big Endian)
	uint8_t lba2;			// Logical block address, 2nd MSB (Big Endian)
	uint8_t lba3;			// Logical block address, 2nd LSB (Big Endian)
	uint8_t lba4;			// Logical block address, LSB (Big Endian)
	uint8_t xferLen1;		// Transfer length, MSB (Big Endian)
	uint8_t xferLen2;		// Transfer length, 2nd MSB (Big Endian)
	uint8_t xferLen3;		// Transfer length, 2nd LSB (Big Endian)
	uint8_t xferLen4;		// Transfer length, LSB (Big Endian)
	uint8_t reserved;		// Reserved
	uint8_t control;		// Control
	uint8_t pad[4];			// Padding for 16-byte command block
} USB_CBW_SCSI_12;

typedef struct __attribute__((packed)) {
	uint8_t opCode;			// Operation code
	uint8_t reserved1;		// Reserved
	uint32_t lba;			// Logical block address
	uint16_t reverved2;		// Reserved
	uint8_t pmi;			// Partial Medium Indicator
	uint8_t control;		// Control
	uint8_t pad[6];			// Padding for 16-byte command block
} USB_CBW_SCSI_READ_CAPACITY_10;

typedef struct __attribute__((packed)) {
	uint8_t opCode;			// Operation code
	uint8_t reserved1;		// Reserved
	uint32_t lba;			// Logical block address
	uint8_t reverved2;		// Reserved
	uint16_t blockCount;	// Number of Blocks used by this command
	uint8_t control;		// Control
	uint8_t pad[6];			// Padding for 16-byte command block
} USB_CBW_SCSI_READ_10, USB_CBW_SCSI_WRITE_10;

typedef struct __attribute__((packed)) {
	uint8_t opCode;			// Operation code
	uint8_t lun;			// Logical unit number
	uint8_t reserved[3];	// Reserved
	uint8_t control;		// Control
	uint8_t pad[10];		// Padding for 16-byte command block
} USB_CBW_SCSI_TEST_UNIT_READY;

typedef struct __attribute__((packed)) {
	uint8_t opCode;			// Operation code
	uint8_t reserved1;		// Reserved
	uint8_t pageCode;		//
	uint8_t reverved2;		// Reserved
	uint8_t allocLen;		//
	uint8_t control;		// Control
	uint8_t pad[10];		// Padding for 16-byte command block
} USB_CBW_SCSI_INQUIRY, USB_CBW_SCSI_REQUEST_SENSE;

typedef struct __attribute__((packed)) {
	uint32_t dCBWSignature;  // Signature identifying the CBW
	uint32_t dCBWTag;        // Unique tag for the command
	uint32_t dCBWDataTransferLength; // Length of data to transfer
	uint8_t bmCBWFlags;      // Flags indicating data direction
	uint8_t bCBWLUN;         // Logical Unit Number
	uint8_t cbCBWLength;     // Length of the command
	union {
		USB_CBW_SCSI_6 six;
		USB_CBW_SCSI_10 ten;
		USB_CBW_SCSI_12 twelve;
		USB_CBW_SCSI_TEST_UNIT_READY testUnitReady;
		USB_CBW_SCSI_INQUIRY inquiry;
		USB_CBW_SCSI_REQUEST_SENSE reqSense;
		USB_CBW_SCSI_READ_CAPACITY_10 readCap10;
		USB_CBW_SCSI_READ_10 read10;
		USB_CBW_SCSI_WRITE_10 write10;
	} scsiCmd;
} USB_CBW;

/*
// Command Block Wrapper
typedef struct
{
  uint32_t signature;   ///< Signature that helps identify this data packet as a CBW. The signature field shall contain the value 43425355h (little endian), indicating a CBW.
  uint32_t tag;         ///< Tag sent by the host. The device shall echo the contents of this field back to the host in the dCSWTagfield of the associated CSW. The dCSWTagpositively associates a CSW with the corresponding CBW.
  uint32_t total_bytes; ///< The number of bytes of data that the host expects to transfer on the Bulk-In or Bulk-Out endpoint (as indicated by the Direction bit) during the execution of this command. If this field is zero, the device and the host shall transfer no data between the CBW and the associated CSW, and the device shall ignore the value of the Direction bit in bmCBWFlags.
  uint8_t dir;          ///< Bit 7 of this field define transfer direction \n - 0 : Data-Out from host to the device. \n - 1 : Data-In from the device to the host.
  uint8_t lun;          ///< The device Logical Unit Number (LUN) to which the command block is being sent. For devices that support multiple LUNs, the host shall place into this field the LUN to which this command block is addressed. Otherwise, the host shall set this field to zero.
  uint8_t cmd_len;      ///< The valid length of the CBWCBin bytes. This defines the valid length of the command block. The only legal values are 1 through 16
  uint8_t command[16];  ///< The command block to be executed by the device. The device shall interpret the first cmd_len bytes in this field as a command block
} MSC_CBW_STRUCT;
*/
#if 0 /* Alt structure */
struct command_block_wrapper {
	uint8_t dCBWSignature[4];
	uint32_t dCBWTag;
	uint32_t dCBWDataTransferLength;
	uint8_t bmCBWFlags;
	uint8_t bCBWLUN;
	uint8_t bCBWCBLength;
	uint8_t CBWCB[16];
};
#endif

// Command Status Wrapper (CSW)
typedef struct command_status_wrapper {
	uint8_t dCSWSignature[4];
	uint32_t dCSWTag;
	uint32_t dCSWDataResidue;
	uint8_t bCSWStatus;
} USB_CSW;

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void USB_setupBulkCbw(uint8_t scsiCmd, uint32_t lba, uint16_t blockCount, uint8_t debug)
{
	/*
		The CPU programs this similarly to a BULK transfer. The CPU loads bytes into the SNDFIFO,
		and writes the byte count into SNDBC. Then it writes the HXFR register with 0110eeee (Table
		4) to launch the transfer.
	*/

	// Setup CBW (done)
	// Set sector address in command block (done, BE)
	// Select read or write (done)
	// Write CBW into SNDFIFO (done)
	// Write byte count into SNDBC (done)
	// Write HXFR to initiate transfer
	// Immediately read or write data
	// After data complete, read CSW

	/*
		Example of a CBW : An example of a CBW might look like this:
		Signature: 				0x43425355 (ASCII for "USBC")
		Tag: 					0x00000001 (Unique command identifier)
		Data Transfer Length: 	0x00000010 (16 bytes to be transferred)
		Flags: 					0x00 (Data transfer from host to device)
		LUN: 					0x00 (First logical unit)
		Command Length: 		0x0A (10 bytes for the command)
		Command Block: 			0x28 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 (Example command for a read operation)
	*/

	/* The Command Block Wrapper (CBW) size for USB bulk transfers is typically 31 bytes */

	USB_CBW cbwRequest;

	//debug("USB Host Controller: CBW setup (Size is %d)\r\n", sizeof(cbwRequest));
	memset(&cbwRequest, 0, sizeof(cbwRequest));

#if 0 /* Big endian */
	cbwRequest.dCBWSignature = __builtin_bswap32(0x43425355);
	cbwRequest.dCBWTag = __builtin_bswap32(0x00000001);
	cbwRequest.dCBWDataTransferLength = __builtin_bswap32(0x00000010);
#else /* Little endian */
	cbwRequest.dCBWSignature = 0x43425355; // "USBC"
	cbwRequest.dCBWTag = ++s_usbTag;

	//s_usbTag += 3; cbwRequest.dCBWTag = s_usbTag;

	// Seemed to work, test flag?
	//cbwRequest.dCBWTag = 0x54555342; // "TUSB"

	//static uint32_t s_usbTag = 0;
	//if (s_usbTag++ == 0) { cbwRequest.dCBWTag = 0x54555342; } else { cbwRequest.dCBWTag = s_usbTag; }
#endif
	cbwRequest.bCBWLUN = 0x00;

	// Test 12-byte read command block
	if (scsiCmd == SCSI_CMD_TEST_UNIT_READY)
	{
		cbwRequest.dCBWDataTransferLength = 0;
		cbwRequest.bmCBWFlags = CBWFLAGS_DIR_OUT;
		cbwRequest.cbCBWLength = 6;

		cbwRequest.scsiCmd.testUnitReady.opCode = SCSI_CMD_TEST_UNIT_READY;
		cbwRequest.scsiCmd.testUnitReady.lun = 0;
	}
	else if (scsiCmd == SCSI_CMD_INQUIRY)
	{
		cbwRequest.dCBWDataTransferLength = 36;
		cbwRequest.bmCBWFlags = CBWFLAGS_DIR_IN;
		cbwRequest.cbCBWLength = 6;

		cbwRequest.scsiCmd.inquiry.opCode = SCSI_CMD_INQUIRY;
		cbwRequest.scsiCmd.inquiry.allocLen = 36;
	}
	else if (scsiCmd == SCSI_CMD_MODE_SENSE_6)
	{
	}
	else if (scsiCmd == SCSI_CMD_REQUEST_SENSE)
	{
	}
	else if (scsiCmd == SCSI_CMD_READ_CAPACITY_10)
	{
		cbwRequest.dCBWDataTransferLength = 8;
		cbwRequest.bmCBWFlags = CBWFLAGS_DIR_IN;
		cbwRequest.cbCBWLength = 10;

		cbwRequest.scsiCmd.readCap10.opCode = SCSI_CMD_READ_CAPACITY_10;
	}
	else if (scsiCmd == SCSI_CMD_READ_FORMAT_CAPACITY)
	{
	}
	else if (scsiCmd == SCSI_CMD_READ_10)
	{
		cbwRequest.dCBWDataTransferLength = (blockCount * 512); // Block count * block size (sector size, 512)
		cbwRequest.bmCBWFlags = CBWFLAGS_DIR_IN;
		cbwRequest.cbCBWLength = 10;

		cbwRequest.scsiCmd.read10.opCode = SCSI_CMD_READ_10;
		cbwRequest.scsiCmd.read10.lba = __builtin_bswap32(lba);
		cbwRequest.scsiCmd.read10.blockCount = __builtin_bswap16(blockCount);
	}
	else if (scsiCmd == SCSI_CMD_WRITE_10)
	{
		cbwRequest.dCBWDataTransferLength = (blockCount * 512); // Block count * block size (sector size, 512)
		cbwRequest.bmCBWFlags = CBWFLAGS_DIR_OUT;
		cbwRequest.cbCBWLength = 10;

		cbwRequest.scsiCmd.write10.opCode = SCSI_CMD_WRITE_10;
		cbwRequest.scsiCmd.write10.lba = __builtin_bswap32(lba);
		cbwRequest.scsiCmd.write10.blockCount = __builtin_bswap16(blockCount);
	}

	if (debug)
	{
		if (scsiCmd == SCSI_CMD_TEST_UNIT_READY) { debug("USB Host Controller: CBW Request, SCSI Test unit ready: "); } else
		if (scsiCmd == SCSI_CMD_INQUIRY) { debug("USB Host Controller: CBW Request, SCSI Inquiry: "); } else
		if (scsiCmd == SCSI_CMD_MODE_SENSE_6) { debug("USB Host Controller: CBW Request, SCSI Mode sense: "); } else
		if (scsiCmd == SCSI_CMD_REQUEST_SENSE) { debug("USB Host Controller: CBW Request, SCSI Request sense: "); } else
		if (scsiCmd == SCSI_CMD_READ_CAPACITY_10) { debug("USB Host Controller: CBW Request, SCSI Read capacity: "); } else
		if (scsiCmd == SCSI_CMD_READ_FORMAT_CAPACITY) { debug("USB Host Controller: CBW Request, SCSI Read format capacity: "); } else
		if (scsiCmd == SCSI_CMD_READ_10) { debug("USB Host Controller: CBW Request, SCSI Read 10: "); } else
		if (scsiCmd == SCSI_CMD_WRITE_10) { debug("USB Host Controller: CBW Request, SCSI Write 10: "); } else
		debug("USB Host Controller: CBW Request, SCSI Cmd %d: ", scsiCmd);

		if ((scsiCmd == SCSI_CMD_READ_10) || (scsiCmd == SCSI_CMD_WRITE_10))
		{
			debugRaw("Tag: %d, Sector: %lu\r\n", s_usbTag, lba);
		}
		else // Raw output of command
		{
			uint16_t i = 0;
			while (i < sizeof(cbwRequest))
			{
				debugRaw("%02x ", ((uint8_t*)&cbwRequest)[i++]);
			}
			debugRaw("\r\n");
		}
	}

#if 1 /* Test Checking Send Bytes Available flag and handling data toggles here */
	//if (MAX_readRegister(rHIRQ) & MAX_IRQ_SNDBAV) { debug("USB Host Controller: Send buffer is available\r\n"); }
	if (MAX_readRegister(rHIRQ) & MAX_IRQ_SNDBAV) { }
	else
	{
		debugWarn("USB Host Controller: Send buffer is not flagged available (starting retry check)\r\n");
		uint16 i = 5000;
		while (((MAX_readRegister(rHIRQ) & MAX_IRQ_SNDBAV) == 0) && (--i)) { SoftUsecWait(100); }

		if (MAX_readRegister(rHIRQ) & MAX_IRQ_SNDBAV) { debug("USB Host Controller: Send buffer is now available\r\n"); }
		else { debugErr("USB Host Controller: Send buffer is not flagged available (timed out)\r\n"); }
	}

	MAX_writeRegister(rPERADDR, PERIPHERAL_ADDRESS); // Shouldn't be needed since it's already set

#if 0 /* Original */
	SetDataToggleForEndpoint(usbMscBulkOutEP);
	//if (cbwRequest.bmCBWFlags == CBWFLAGS_DIR_OUT) { SetDataToggleForEndpoint(usbMscBulkOutEP); }
	//else { SetDataToggleForEndpoint(usbMscBulkInEP); }
#else
	// Skip here in favor of handling at transmit packet
#endif

#endif

#if 1 /* Original */
	if (debug) { debug("USB Host Controller: CBW write to SNDFIFO (cbwRequest size is %d)\r\n", sizeof(cbwRequest)); }
	MAX_multiWriteRegister(rSNDFIFO, (uint8_t*)&cbwRequest, sizeof(cbwRequest));

	if (debug) { debug("USB Host Controller: Byte count write to SNDBC (Count is %d)\r\n", sizeof(cbwRequest)); }
	MAX_writeRegister(rSNDBC, sizeof(cbwRequest));
#elif 1 /* Try individual writes */
	debug("USB Host Controller: CBW write bytes individually to SNDFIFO (cbwRequest size is %d)\r\n", sizeof(cbwRequest));
	uint8_t j = 0;
	uint8_t* cbwRequestPtr = (uint8_t*)&cbwRequest;
	while (j < sizeof(cbwRequest))
	{
		MAX_writeRegister(rSNDFIFO, cbwRequestPtr[j++]);
	}

	debug("USB Host Controller: Byte count write to SNDBC (Count is %d)\r\n", sizeof(cbwRequest));
	MAX_writeRegister(rSNDBC, sizeof(cbwRequest));
#else
	debug("USB Host Controller: Skipping CBW write\r\n");
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void USB_setupBulkData(void* usbData, uint16_t dataSize, uint8_t debug)
{
	//debug("USB Host Controller: Bulk Data Out (Size is %d)\r\n", dataSize);

	if (MAX_readRegister(rHIRQ) & MAX_IRQ_SNDBAV) { if (debug) { debug("USB Host Controller: BDO Send buffer is available (send size %d)\r\n", dataSize); } else { SoftUsecWait(200); } }
	//if (MAX_readRegister(rHIRQ) & MAX_IRQ_SNDBAV) { } // Seem to get instant write failure without debug delay
	else
	{
		debugWarn("USB Host Controller: BDO Send buffer is not flagged available (starting retry check)\r\n");
		uint16 i = 1000;
		while (((MAX_readRegister(rHIRQ) & MAX_IRQ_SNDBAV) == 0) && (--i)) { SoftUsecWait(100); }

		if (MAX_readRegister(rHIRQ) & MAX_IRQ_SNDBAV) { if (debug) { debug("USB Host Controller: BDO Send buffer is now available\r\n"); } }
		else { debugErr("USB Host Controller: BDO Send buffer is not flagged available (timed out)\r\n"); }
	}

	//MAX_writeRegister(rPERADDR, PERIPHERAL_ADDRESS); // Shouldn't be needed since it's already set

	//debug("USB Host Controller: Bulk Data Out write to SNDFIFO (size is %d)\r\n", dataSize);
	MAX_multiWriteRegister(rSNDFIFO, (uint8_t*)usbData, dataSize);

	//debug("USB Host Controller: BDO Byte count write to SNDBC (Count is %d)\r\n", dataSize);
	MAX_writeRegister(rSNDBC, dataSize);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_getDeviceDescriptor(uint8_t debug)
{
	uint8_t responseCode;

	//___Get Device Descriptor
	if (debug)
	{
		debug("USB Host Controller: ----------------\r\n");
		debug("USB Host Controller: Requesting Device Descriptor to addr %d...\r\n", PERIPHERAL_ADDRESS);
		debug("USB Host Controller: ----------------\r\n");
	}

	responseCode = USB_getDeviceDescriptorRequest(PERIPHERAL_ADDRESS, debug);

	if (debug) { debug("USB Host Controller: Response code %02x\r\n", responseCode); }
	if (responseCode == rslSTALL) { USB_handleStallwithClear(0, debug); }

	return (responseCode);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_getBaseConfigDescriptor(uint8_t* fullConfigLength, uint8_t debug)
{
	uint8_t responseCode;

	//___Get Base Config Descriptor
	if (debug)
	{
		debug("USB Host Controller: ----------------\r\n");
		debug("USB Host Controller: Requesting Configuration (Base) Descriptor to addr %d...\r\n", PERIPHERAL_ADDRESS);
		debug("USB Host Controller: ----------------\r\n");
	}

	responseCode = USB_getConfigurationDescriptorRequest(PERIPHERAL_ADDRESS, debug);

	if (debug) { debug("USB Host Controller: Response code %02x\r\n", responseCode); }
	if (responseCode == rslSTALL) { USB_handleStallwithClear(0, debug); }

	if (responseCode == rslSUCCES)
	{
		if (ControlBuffer[1] == USB_CONFIG_DESCRIPTOR)
		{
			*fullConfigLength = ControlBuffer[2];
			if (debug) { debug("USB Host Controller: Config full length is %d\r\n", *fullConfigLength); }
		}
		else
		{
			debugErr("USB Host Controller: Wrong descriptor (%d)\r\n", ControlBuffer[1]);
		}
	}
	else
	{
		debugErr("USB Host Controller: Bad response code, Configuraiton descriptor full length not available\r\n");
	}

	return (responseCode);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_getStringDescriptor(uint8_t debug)
{
	uint8_t responseCode;

	//___Get String Descriptor
	if (debug)
	{
		debug("USB Host Controller: ----------------\r\n");
		debug("USB Host Controller: Requesting String Descriptor to addr %d...\r\n", PERIPHERAL_ADDRESS);
		debug("USB Host Controller: ----------------\r\n");
	}

	responseCode = USB_getStringDescriptorRequest(PERIPHERAL_ADDRESS, debug);

	if (debug) { debug("USB Host Controller: Response code %02x\r\n", responseCode); }
	if (responseCode == rslSTALL) { USB_handleStallwithClear(0, debug); }

	return (responseCode);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_getFullConfigDescriptor(uint8_t fullConfigLength, uint8_t debug)
{
	uint8_t responseCode = 0;

	//___Get Full Config Descriptor
	if (fullConfigLength)
	{
		if (debug)
		{
			debug("USB Host Controller: ----------------\r\n");
			debug("USB Host Controller: Requesting Configuration (Full) Descriptor to addr %d...\r\n", PERIPHERAL_ADDRESS);
			debug("USB Host Controller: ----------------\r\n");
		}

		responseCode = USB_getConfigurationDescriptorFullRequest(PERIPHERAL_ADDRESS, fullConfigLength, debug);

		if (debug) { debug("USB Host Controller: Response code %02x\r\n", responseCode); }
		if (responseCode == rslSTALL) { USB_handleStallwithClear(0, debug); }

		if (responseCode == rslSUCCES)
		{
			if (lastReadSize == fullConfigLength)
			{
				DecodeFullConfigDescriptor(fullConfigLength);
			}
			else
			{
				debugWarn("USB Host Controller: Read size did not match full Configuration Descriptor\r\n");
				//OverlayMessage(getLangText(WARNING_TEXT), "Read size did not match full Configuration Descriptor", (3 * SOFT_SECS));
				responseCode = rslUNDEF;
			}
		}
		else { debugErr("USB Host Controller: Error getting full Configuration Descriptor\r\n"); }
	}
	else
	{
		debugErr("USB Host Controller: Full configuraiton descriptor not requested due to no valid length\r\n");
	}

	if (debug)
	{
		debug("USB Host Controller: -------- Bulk Endpoints --------\r\n");
		debug("USB Host Controller: USB MSC Bulk In EP is %d\r\n", usbMscBulkInEP);
		debug("USB Host Controller: USB MSC Bulk Out EP is %d\r\n", usbMscBulkOutEP);
		OverlayMessage(getLangText(STATUS_TEXT), "USB Flash device configured", 0);
	}

	return (responseCode);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_setConfiguration(uint8_t debug)
{
	uint8_t responseCode;

	//___Set Configuration
	if (debug)
	{
		debug("USB Host Controller: ----------------\r\n");
		debug("USB Host Controller: Sending Set Configuraiton to addr %d...\r\n", PERIPHERAL_ADDRESS);
		debug("USB Host Controller: ----------------\r\n");
	}

	responseCode = USB_setDeviceConfiguration(PERIPHERAL_ADDRESS, usbMscConfigID, debug);

	if (debug) { debug("USB Host Controller: Response code %02x\r\n", responseCode); }
	if (responseCode == rslSTALL) { USB_handleStallwithClear(0, debug); }

	return (responseCode);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_getMaxLun(uint8_t debug)
{
	uint8_t responseCode;

	//___Get Max LUN
	if (debug)
	{
		debug("USB Host Controller: ----------------\r\n");
		debug("USB Host Controller: Requesting Max LUN to addr %d...\r\n", PERIPHERAL_ADDRESS);
		debug("USB Host Controller: ----------------\r\n");
	}

	responseCode = USB_getMaxLunRequest(PERIPHERAL_ADDRESS, debug);

	if (debug) { debug("USB Host Controller: Response code %02x\r\n", responseCode); }
	if (responseCode == rslSTALL) { USB_handleStallwithClear(0, debug); }

	return (responseCode);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_getStatus(uint8_t debug)
{
	uint8_t responseCode;

	//___Get Status
	if (debug)
	{
		debug("USB Host Controller: ----------------\r\n");
		debug("USB Host Controller: Requesting status...\r\n");
		debug("USB Host Controller: ----------------\r\n");
	}

	responseCode = USB_requestStatus(0, debug);

	if (debug) { debug("USB Host Controller: Response code %02x\r\n", responseCode); }
	if (responseCode == rslSTALL) { USB_handleStallwithClear(0, debug); }

	return (responseCode);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_testUnitReady(uint8_t debug)
{
	uint8_t responseCode;
	USB_CSW statusPacket;

	//___Test Unit Ready
	if (debug)
	{
		debug("USB Host Controller: ----------------\r\n");
		debug("USB Host Controller: Checking MSC Test unit ready...\r\n");
		debug("USB Host Controller: ----------------\r\n");
	}

	USB_setupBulkCbw(SCSI_CMD_TEST_UNIT_READY, 0, 0, debug);
	transmitPacket(xfrOUT, usbMscBulkOutEP);
	responseCode = requestData(DIR_IN, (uint8_t*)&statusPacket, 13, YES, debug);

	if (debug) { debug("USB Host Controller: TUR Host result: %s (%d)\r\n", ResultCodeName(responseCode), responseCode); }

	if (responseCode == rslNAK)
	{
		SoftUsecWait(500 * SOFT_MSECS);
		debugWarn("USB Host Controller: Re-trying TUR request...\r\n");
		USB_setupBulkCbw(SCSI_CMD_TEST_UNIT_READY, 0, 0, debug);
		transmitPacket(xfrOUT, usbMscBulkOutEP);
		responseCode = requestData(DIR_IN, (uint8_t*)&statusPacket, 13, YES, debug);

		if (debug) { debug("USB Host Controller: Host result: %s (%d)\r\n", ResultCodeName(responseCode), responseCode); }

		if (responseCode == rslNAK)
		{
			SoftUsecWait(500 * SOFT_MSECS);
			debugWarn("USB Host Controller: Re-trying TUR request (again)...\r\n");
			USB_setupBulkCbw(SCSI_CMD_TEST_UNIT_READY, 0, 0, debug);
			transmitPacket(xfrOUT, usbMscBulkOutEP);
			responseCode = requestData(DIR_IN, (uint8_t*)&statusPacket, 13, YES, debug);

			if (debug) { debug("USB Host Controller: Host result: %s (%d)\r\n", ResultCodeName(responseCode), responseCode); }

			if (responseCode == rslNAK)
			{
				debugErr("USB Host Controller: 3rd TUR request failed\r\n");
			}
		}
	}

	return (responseCode);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_mscInquiry(uint8_t* rxBuffer, uint8_t debug)
{
	uint8_t responseCode;
	USB_CSW statusPacket;

	//___MSC Inquiry
	if (debug)
	{
		debug("USB Host Controller: ----------------\r\n");
		debug("USB Host Controller: Requesting MSC Inquiry...\r\n");
		debug("USB Host Controller: ----------------\r\n");
	}

	USB_setupBulkCbw(SCSI_CMD_INQUIRY, 0, 0, debug);
	transmitPacket(xfrOUT, usbMscBulkOutEP);
	responseCode = requestData(DIR_IN, rxBuffer, 36, YES, debug);
	responseCode = requestData(DIR_IN, (uint8_t*)&statusPacket, 13, NO, debug);

	if (debug) { debug("USB Host Controller: Host result: %s (%d)\r\n", ResultCodeName(responseCode), responseCode); }

	return (responseCode);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_readCapacity(uint8_t* rxBuffer, uint8_t debug)
{
	uint8_t responseCode;
	USB_CSW statusPacket;

	//___Read Capacity
	if (debug)
	{
		debug("USB Host Controller: ----------------\r\n");
		debug("USB Host Controller: Attempting Read Capacity 10\r\n");
		debug("USB Host Controller: ----------------\r\n");
	}

	USB_setupBulkCbw(SCSI_CMD_READ_CAPACITY_10, 0, 0, debug);
	transmitPacket(xfrOUT, usbMscBulkOutEP);
	responseCode = requestData(DIR_IN, rxBuffer, 8, YES, debug);
	responseCode = requestData(DIR_IN, (uint8_t*)&statusPacket, 13, NO, debug);

	if (responseCode == rslNAK)
	{
		USB_setupBulkCbw(SCSI_CMD_READ_CAPACITY_10, 0, 0, debug);
		transmitPacket(xfrOUT, usbMscBulkOutEP);
		responseCode = requestData(DIR_IN, rxBuffer, 8, YES, debug);
		responseCode = requestData(DIR_IN, (uint8_t*)&statusPacket, 13, NO, debug);
	}

	if ((responseCode == rslNAK) || (responseCode == rslSTALL))
	{
		USB_handleStallwithClear(usbMscBulkOutEP, debug);
		return (rslSTALL);
	}

	if (debug) { debug("USB Host Controller: Total sectors is %lu\r\n", __builtin_bswap32(*(uint32_t*)&rxBuffer[0])); }
	if (debug) { debug("USB Host Controller: Sectors size is %lu\r\n", __builtin_bswap32(*(uint32_t*)&rxBuffer[4])); }

	if (debug) { debug("USB Host Controller: Host result: %s (%d)\r\n", ResultCodeName(responseCode), responseCode); }

	return (responseCode);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_readSector(uint32_t sector, uint8_t* rxBuffer, uint8_t debug)
{
	uint8_t responseCode;
	uint16_t i;
	USB_CSW statusPacket;

	if (debug)
	{
		debug("USB Host Controller: ----------------\r\n");
		debug("USB Host Controller: Attempting Read Data 10 (Sector %d)...\r\n", sector);
		debug("USB Host Controller: ----------------\r\n");
	}

#if 0 /* Original */
	USB_setupBulkCbw(SCSI_CMD_READ_10, sector, 1, debug);
	responseCode = transmitPacket(xfrOUT, usbMscBulkOutEP);
	if (responseCode != rslSUCCES)
	{
		debugWarn("USB Host Controller: Read Sector, Transmit Bulk Out failed: %s (%d), retrying...\r\n", ResultCodeName(responseCode), responseCode);
		USB_setupBulkCbw(SCSI_CMD_READ_10, sector, 1, debug);
		responseCode = transmitPacket(xfrOUT, usbMscBulkOutEP);
		if (responseCode != rslSUCCES) { debugErr("USB Host Controller: Read Sector, Transmit Bulk Out failed again: %s (%d)\r\n", ResultCodeName(responseCode), responseCode); }
	}

	i = 0;
	responseCode = requestData(DIR_IN, rxBuffer, 64, YES, debug);
	if (responseCode == rslNAK)
	{
		debugWarn("USB Host Controller: Read Sector, Request Bulk In was NAK'ed, retrying initial request...\r\n");
		USB_setupBulkCbw(SCSI_CMD_READ_10, sector, 1, debug);
		responseCode = transmitPacket(xfrOUT, usbMscBulkOutEP);
		if (responseCode != rslSUCCES) { debugErr("USB Host Controller: Read Sector, Transmit Bulk Out after NAK failed: %s (%d)\r\n", ResultCodeName(responseCode), responseCode); USB_handleStallwithClear(usbMscBulkInEP, debug); return (rslSTALL); }

		responseCode = requestData(DIR_IN, rxBuffer, 64, YES, debug);
		if (responseCode == rslNAK)	{ debugErr("USB Host Controller: Read Sector, Request Bulk In was NAK'ed, unsure how to proceed (Sector: %d)\r\n", sector); USB_handleStallwithClear(usbMscBulkInEP, debug); return (rslSTALL); }
		else { i = 1; }
	}
	else { i = 1; }

	for (; i < 8; i++)
	{
		responseCode = requestData(DIR_IN, (rxBuffer + (i * 64)), 64, NO, debug);
		if (responseCode == rslNAK) { debugErr("USB Host Controller: Read Sector, Request data was NAK'ed, handling as stall (Sector: %d)\r\n", sector); USB_handleStallwithClear(usbMscBulkInEP, debug); return (rslSTALL); }
		if (responseCode == rslSTALL) { debugErr("USB Host Controller: Read Sector, Stall encountered (Sector: %d)\r\n", sector); USB_handleStallwithClear(usbMscBulkInEP, debug); return (rslSTALL); }
	}

	responseCode = requestData(DIR_IN, (uint8_t*)&statusPacket, 13, NO, debug);
	if (responseCode == rslNAK) { debugErr("USB Host Controller: Read Sector, Request CSW was NAK'ed, handling as stall (Sector: %d)\r\n", sector); USB_handleStallwithClear(usbMscBulkInEP, debug); return (rslSTALL); }
	if (responseCode == rslSTALL) { debugErr("USB Host Controller: Read Sector, Request CSW stalled (Sector: %d)\r\n", sector); USB_handleStallwithClear(usbMscBulkInEP, debug); return (rslSTALL); }

	if (statusPacket.dCSWDataResidue != 0) { debugWarn("USB Host Controller: Read Sector, Residual data to be read\r\n"); }

	//if (debug)
	if (0)
	{
		debug("USB Host Controller: Host result: %s (%d)\r\n", ResultCodeName(responseCode), responseCode);

		// Dump sector
		debug("USB Host Controller: Read sector %lu:", sector); for (i = 0; i < 512; i++) { debugRaw(" %02x", g_spareBuffer[(1000 + i)]); } debugRaw(" <end>\r\n");
		memset(&g_spareBuffer[1000], 0, 512);
	}

	if (debug) { SoftUsecWait(500 * SOFT_MSECS); }
#else /* Try different error recovery method */
	uint8_t stage = 0;

	// Stage 1.0
	USB_setupBulkCbw(SCSI_CMD_READ_10, sector, 1, debug);
	responseCode = transmitPacket(xfrOUT, usbMscBulkOutEP);

	if (responseCode == rslSUCCES)
	{
		// Stage 2.x
		responseCode = requestData(DIR_IN, rxBuffer, 64, YES, debug);
		if (responseCode != rslSUCCES) { stage = 20; }
		i = 1;

#if 0 // Seems to cause more problems
		// Check if NAK
		if (responseCode == rslNAK)
		{
			// One attempt at retrying initial request
			USB_setupBulkCbw(SCSI_CMD_READ_10, sector, 1, debug);
			responseCode = transmitPacket(xfrOUT, usbMscBulkOutEP);
			if (responseCode != rslSUCCES) { stage = 29; }
			i = 0;
		}
#endif
		// Check if first data packet sent successfully or initial request retry was successful
		if (responseCode == rslSUCCES)
		{
			// Stage 2.x
			for (; i < 8; i++)
			{
				responseCode = requestData(DIR_IN, (rxBuffer + (i * 64)), 64, ((i == 0) ? YES : NO), debug); // Index check for first request which will perform toggle, otherwise no
				if (responseCode != rslSUCCES) { stage = 20 + i; break; }
			}
		}
	}
	else { stage = 10; }

	if (responseCode == rslSUCCES)
	{
		// Stage 3.0
		responseCode = requestData(DIR_IN, (uint8_t*)&statusPacket, 13, NO, debug);
		if (responseCode != rslSUCCES) { stage = 30; }
	}

	if (responseCode != rslSUCCES)
	{
		debugErr("USB Host Controller: Read Sector %d, failed stage %d with %s (%d)\r\n", sector, stage, ResultCodeName(responseCode), responseCode);
		USB_handleStallwithClear(usbMscBulkInEP, debug);
		responseCode = rslSTALL;
	}
#endif

	return (responseCode);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_writeSector(uint32_t sector, uint8_t* txBuffer, uint8_t debug)
{
	uint8_t responseCode;
	uint16_t i;
	USB_CSW statusPacket;

	if (debug)
	{
		debug("USB Host Controller: ----------------\r\n");
		debug("USB Host Controller: Attempting Write Data 10 (Sector %d)...\r\n", sector);
		debug("USB Host Controller: ----------------\r\n");
	}

#if 0 /* Original */
	USB_setupBulkCbw(SCSI_CMD_WRITE_10, sector, 1, debug);
	responseCode = transmitPacket(xfrOUT, usbMscBulkOutEP);
	if (responseCode != rslSUCCES)
	{
		debugWarn("USB Host Controller: Write Sector, Transmit Bulk Out failed: %s (%d), retrying...\r\n", ResultCodeName(responseCode), responseCode);
		USB_setupBulkCbw(SCSI_CMD_WRITE_10, sector, 1, debug);
		responseCode = transmitPacket(xfrOUT, usbMscBulkOutEP);
		if (responseCode != rslSUCCES) { debugErr("USB Host Controller: Write Sector, Transmit Bulk Out failed again: %s (%d)\r\n", ResultCodeName(responseCode), responseCode); }
		if (responseCode == rslSTALL) { USB_handleStallwithClear(usbMscBulkOutEP, debug); return (rslSTALL); }
	}

	for (i = 0; i < 8; i++)
	{
		//SoftUsecWait(10 * SOFT_MSECS);
		USB_setupBulkData((txBuffer + (i * 64)), 64, debug);
		responseCode = transmitData(xfrOUT, usbMscBulkOutEP);
		if (responseCode != rslSUCCES) { debugErr("USB Host Controller: Write Sector, Transmit Bulk Out failed: %s (%d)\r\n", ResultCodeName(responseCode), responseCode); }
		if (responseCode == rslSTALL) { USB_handleStallwithClear(usbMscBulkOutEP, debug); return (rslSTALL); }
	}

	responseCode = requestData(DIR_IN, (uint8_t*)&statusPacket, 13, YES, debug);

	if (debug) { debug("USB Host Controller: Host result: %s (%d)\r\n", ResultCodeName(responseCode), responseCode); }

	if (responseCode == rslNAK)
	{
		debugWarn("USB Host Controller: Write Sector, Re-trying status request...\r\n");
		responseCode = requestData(DIR_IN, (uint8_t*)&statusPacket, 13, NO, debug);
	}

	if (responseCode != rslSUCCES)
	{
		debugErr("USB Host Controller: Write Sector, failed to get status\r\n");
	}

	if (statusPacket.dCSWDataResidue != 0) { debugWarn("USB Host Controller: Write Sector, Residual data to be read\r\n"); }

	if (debug) { SoftUsecWait(500 * SOFT_MSECS); }
#else /* Try different error recovery method */
	uint8_t stage = 0;

	// Stage 1.0
	USB_setupBulkCbw(SCSI_CMD_WRITE_10, sector, 1, debug);
	responseCode = transmitPacket(xfrOUT, usbMscBulkOutEP);

	if (responseCode == rslSUCCES)
	{
		// Stage 2.x
		for (i = 0; i < 8; i++)
		{
			USB_setupBulkData((txBuffer + (i * 64)), 64, debug);
			responseCode = transmitData(xfrOUT, usbMscBulkOutEP);
			if (responseCode != rslSUCCES) { stage = 20 + i; break; }
		}
	}
	else { stage = 10; }

	if (responseCode == rslSUCCES)
	{
		// Stage 3.0
		responseCode = requestData(DIR_IN, (uint8_t*)&statusPacket, 13, YES, debug);

		if (responseCode != rslSUCCES) { stage = 30; }
	}

	if (responseCode != rslSUCCES)
	{
		debugErr("USB Host Controller: Write Sector %d, failed stage %d with %s (%d)\r\n", sector, stage, ResultCodeName(responseCode), responseCode);
		USB_handleStallwithClear(usbMscBulkOutEP, debug);
		responseCode = rslSTALL;
	}
#endif

	return (responseCode);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void USBHostControllerShutdown(void)
{
	debug("USB Host Controller: ----------------\r\n");
	debug("USB Host Controller: Done with access, powering down\r\n");
	debug("USB Host Controller: ----------------\r\n");
	OverlayMessage(getLangText(STATUS_TEXT), "USB Host powering down", 0);

	// Use Power button to start or as escape mechanism, reset here since combo key used for actication
	ClearSoftTimer(POWER_OFF_TIMER_NUM);
	g_powerOffAttempted = NO;

	USBCPortControllerSwapToDevice();

	/* Enable the Max reset */
	MAX_writeRegister(15, BIT5);

#if 1 /* Test */
	SetBattChargerChargeState(ON);

	// Re-enable Aux power
	PowerControl(USB_AUX_POWER_ENABLE, ON);

#if 0 /* Re-enable the MCU USB Device drivers */
	// Re-enable the MCU USB
extern void SetupUSBComposite(uint8_t);
	SetupUSBComposite(USB_COMPOSITE_OPTION_FLAG);
#endif
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void USBHostControllerTest(void)
{
	uint16_t i;

#if 0 /* Test removing power from the LCD */
	// Check if the LCD is currently powered
	if (g_lcdPowerFlag == ENABLED)
	{
		ClearSoftTimer(LCD_BACKLIGHT_ON_OFF_TIMER_NUM);
		ClearSoftTimer(LCD_POWER_ON_OFF_TIMER_NUM);
		LcdPwTimerCallBack();
	}
#endif

#if 1 /* Test */
#if 0 /* Option to shutdown the MCU USB Device driver */
	// Disable MCU USB to make sure there is no interference
	MXC_USB_Shutdown();
#endif
	debug("Fuel Gauge: %s, BC charge current: %u mA\r\n", FuelGaugeDebugString(), GetBattChargerBatteryChargeCurrent());

	// Disable Aux power to prevent fake cahrging
	PowerControl(USB_AUX_POWER_ENABLE, OFF);

	SetBattChargerChargeState(OFF);

	SoftUsecWait(2 * SOFT_SECS);
	debug("Fuel Gauge: %s, BC charge current: %u mA\r\n", FuelGaugeDebugString(), GetBattChargerBatteryChargeCurrent());
#endif

	USBCPortControllerSwapToHost();
#if 0 /* Normal */
	debug("USB Host Controller: Delay for USB Device power to stabilize\r\n");
	SoftUsecWait(2 * SOFT_SECS);
#else
extern uint8_t g_usbSourceExternalPower;
	if (g_usbSourceExternalPower)
	{
		debug("USB Host Controller: Turn on external power supply for USB 5V supply (press Power On when complete)...\r\n");
		OverlayMessage(getLangText(STATUS_TEXT), "TURN ON EXTERNAL POWER FOR USB 5V, THEN PRESS POWER ON BUTTON", 0);
		while (1)
		{
			if (GetPowerOnButtonState() == ON) { debug("\r\n\r\nKeypad: Loop break\r\n\r\n"); break; }
		}
	}
	else // Use internal 5V buck
	{
		debug("USB Host Controller: Delay for USB Device power to stabilize\r\n");
		SoftUsecWait(2 * SOFT_SECS);
	}
#endif

	debug("Fuel Gauge: %s, BC charge current: %u mA\r\n", FuelGaugeDebugString(), GetBattChargerBatteryChargeCurrent());

	MAX_start();

	debug("Fuel Gauge: %s, BC charge current: %u mA\r\n", FuelGaugeDebugString(), GetBattChargerBatteryChargeCurrent());

	//debug("USB Host Controller: ** 5 second pause **\r\n"); SoftUsecWait(5 * SOFT_SECS);

	MAX_checkBusState(YES);

	/* Enable interrupts */
	MAX_enableInterrupts(MAX_IRQ_CONDET);
	MAX_clearInterruptStatus(MAX_IRQ_CONDET);
	MAX_enableInterruptsMaster();

	uint8_t regval = MAX_readRegister(rREVISION);
	debug("USB Host Controller: Revision: 0x%x\r\n", regval);

#if 0 /* Test low speed - did not work, get K-state errors */
	// Enable low speed
	debug("USB Host Controller: Trying Low Speed option\r\n");
	MAX_enableOptions(rMODE, BIT1);
#endif

	/* Perform a bus reset to reconnect after a power down */
	if (!peripheralAvailable)
	{
		debugWarn("USB Host Controller: Perform a bus reset to reconnect after a power down\r\n");
		USB_busReset(YES);
	}

	//debug("USB Host Controller: ** 5 second pause **\r\n"); SoftUsecWait(5 * SOFT_SECS);

	MAX_processNewDevice(YES);

	if (peripheralAvailable == false) { USBHostControllerShutdown(); return; }

	//debug("USB Host Controller: ** 5 second pause **\r\n"); SoftUsecWait(5 * SOFT_SECS);

	uint8_t fullConfigLength = 0;

	// Reset to initial toggle state if test is run multiple times
	//memset(&g_usbEndpointDataToggle[0], 0xFF, 3);
	memset(&g_usbEndpointDataToggle[0], 0, 3);

	usbStallsEncountered = 0;

	//___________________________________________________________________________________________
	//___Get Device Descriptor
	USB_getDeviceDescriptor(YES);

	//___________________________________________________________________________________________
	//___Get Base Config Descriptor
	USB_getBaseConfigDescriptor(&fullConfigLength, YES);

	//___________________________________________________________________________________________
	//___Get String Descriptor
	USB_getStringDescriptor(YES);

	//___________________________________________________________________________________________
	//___Get Full Config Descriptor
	if (USB_getFullConfigDescriptor(fullConfigLength, YES) != rslSUCCES) { USBHostControllerShutdown(); return; }

	//___________________________________________________________________________________________
	//___Set Configuration
	USB_setConfiguration(YES);

	//___________________________________________________________________________________________
	//___Get Max LUN
	USB_getMaxLun(YES);

	//___________________________________________________________________________________________
	//___Get Status (loop)
	for (i = 0; i < 8; i++) { USB_getStatus(YES); }

	SoftUsecWait(1 * SOFT_SECS);

	//___________________________________________________________________________________________
	//___Stall recovery test (works)
#if 0 /* Test Bus reset and re-enumerate device */
	USB_handleStallwithClear(0, YES);
	USB_getMaxLun(YES);
	for (i = 0; i < 8; i++) { USB_getStatus(YES); }
#endif

	//___________________________________________________________________________________________
	//___Bulk Transfers

#if 0 /* Test read and write sectors */
		for (i = 0; i < 1; i++)
		{
			USB_testUnitReady(YES);
			SoftUsecWait(1 * SOFT_SECS);
		}

		for (i = 0; i < 1; i++)
		{
			USB_mscInquiry(g_spareBuffer, YES);
			SoftUsecWait(1 * SOFT_SECS);
		}

		for (i = 0; i < 1; i++)
		{
			USB_readCapacity(YES);
			SoftUsecWait(1 * SOFT_SECS);
		}

		//___________________________________________________________________________________________
		for (i = 0; i < 3; i++)
		{
			USB_readSector(60000000, g_spareBuffer, YES);
		}

		//___________________________________________________________________________________________
		for (uint16_t k = 0; k < 512; k++) { g_spareBuffer[k] = k; } // Write consecutive numbers to data buffer
		USB_writeSector(60000000, g_spareBuffer, YES);

		//___________________________________________________________________________________________
		for (i = 0; i < 3; i++)
		{
			USB_readSector(60000000, g_spareBuffer, YES);
		}

		//___________________________________________________________________________________________
		for (uint16_t k = 0; k < 512; k++) { g_spareBuffer[k] = 0xFF; } // Write erased flash to data buffer
		USB_writeSector(60000000, g_spareBuffer, YES);

		//___________________________________________________________________________________________
		for (i = 0; i < 3; i++)
		{
			USB_readSector(60000000, g_spareBuffer, YES);
		}
#endif

#if 1 /* Loop test of Unit Ready, Inquiry, Read Capacity, Read Data */
	for (i = 0; i < 8; i++) { USB_testUnitReady(YES); }
	USB_mscInquiry(g_spareBuffer, YES);
	SoftUsecWait(50 * SOFT_MSECS);
	USB_readCapacity(g_spareBuffer, YES);

	uint32_t testSectorNumber = 7777777; //80000; //0;

	testSectorNumber = 0;
	SoftUsecWait(1000 * SOFT_MSECS);
	USB_readSector(testSectorNumber, &g_spareBuffer[1000], YES);
	SoftUsecWait(1000 * SOFT_MSECS);

	//--- While loop read test ---
#if 0 /* Test read loop */
	SoftUsecWait(1000 * SOFT_MSECS);
	USB_readSector(0, &g_spareBuffer[1000], YES);
	SoftUsecWait(1000 * SOFT_MSECS);

	testSectorNumber = 0;
	debug("USB Host Controller: Loop read sector test, starting at 0\r\n");
	while (1)
	{
		if (GetPowerOnButtonState() == ON) { debug("\r\n\r\nKeypad: Loop break\r\n\r\n"); break; }
		//USB_readSector(testSectorNumber, &g_spareBuffer[1000], NO);
		if (USB_readSector(testSectorNumber, &g_spareBuffer[1000], NO) == rslSTALL) { debugRaw("(S)"); }
		//SoftUsecWait(2 * SOFT_MSECS);
		testSectorNumber++;
		if ((testSectorNumber % 100) == 0) { debugRaw("-"); }
		if ((testSectorNumber) && (testSectorNumber % 10000) == 0) { debugRaw("(%d)", testSectorNumber); }
	}
	SoftUsecWait(1500 * SOFT_MSECS);
#endif

#if 0 /* Test Read/Write-Clear/Read/Write-Unique/Read */
	uint8_t loops = 5;
	uint16_t j;
	//--- Read base sector (Fat table) ---
	for (i = 0; i < loops; i++) { USB_readSector(0, &g_spareBuffer[1000], YES); }

	//--- Read sectors to see current data ---
	//testSectorNumber = 80000;
	testSectorNumber = 7777777;
	for (i = 0; i < loops; i++) { USB_readSector(testSectorNumber, &g_spareBuffer[1000], YES);
		for (j = 0; j < 512; j++) { if (g_spareBuffer[1000 + j] != 0) { break; } } if (j == 512) { debug("USB Host Controller: Sector %d is empty\r\n", testSectorNumber); } else {
		for (j = 0; j < 512; j++) { if (g_spareBuffer[1000 + j] != g_spareBuffer[1000]) { break; } } if (j == 512) { debug("USB Host Controller: Sector %d contains unique number %d\r\n", testSectorNumber, g_spareBuffer[1000]); }
		else { debug("USB Host Controller: Sector %d is random\r\n", testSectorNumber); } }
		testSectorNumber++; }

	//--- Wite sectors clear ---
	//testSectorNumber = 80000;
	testSectorNumber = 7777777;
	for (i = 0; i < loops; i++)
	{
		debug("USB Host Controller: Write sector clear %d\r\n", testSectorNumber);
		memset(&g_spareBuffer[1000], 0, 512);
		if (USB_writeSector(testSectorNumber, &g_spareBuffer[1000], YES) == rslSTALL) { USB_writeSector(testSectorNumber, &g_spareBuffer[1000], YES); }
		USB_testUnitReady(YES);
		SoftUsecWait(1000 * SOFT_MSECS);
		testSectorNumber++;
	}

	//--- Read sectors are clear ---
	//testSectorNumber = 80000;
	testSectorNumber = 7777777;
	for (i = 0; i < loops; i++) { USB_readSector(testSectorNumber, &g_spareBuffer[1000], YES);
		for (j = 0; j < 512; j++) { if (g_spareBuffer[1000 + j] != 0) { break; } } if (j == 512) { debug("USB Host Controller: Sector %d is empty\r\n", testSectorNumber); } else {
		for (j = 0; j < 512; j++) { if (g_spareBuffer[1000 + j] != g_spareBuffer[1000]) { break; } } if (j == 512) { debug("USB Host Controller: Sector %d contains unique number %d\r\n", testSectorNumber, g_spareBuffer[1000]); }
		else { debug("USB Host Controller: Sector %d is random\r\n", testSectorNumber); } }
		testSectorNumber++; }

	//--- Wite sectors with unique number ---
	//testSectorNumber = 80000;
	testSectorNumber = 7777777;
	for (i = 0; i < loops; i++)
	{
		debug("USB Host Controller: Write sector %d with unique number %d\r\n", testSectorNumber, (testSectorNumber - 80000 + 1));
		memset(&g_spareBuffer[1000], (testSectorNumber - 80000 + 1), 512);
		if (USB_writeSector(testSectorNumber, &g_spareBuffer[1000], YES) == rslSTALL) { USB_writeSector(testSectorNumber, &g_spareBuffer[1000], YES); }
		USB_testUnitReady(YES);
		SoftUsecWait(1000 * SOFT_MSECS);
		testSectorNumber++;
	}

	//--- Read sectors have unique number stored ---
	//testSectorNumber = 80000;
	testSectorNumber = 7777777;
	for (i = 0; i < loops; i++) { USB_readSector(testSectorNumber, &g_spareBuffer[1000], YES);
		for (j = 0; j < 512; j++) { if (g_spareBuffer[1000 + j] != 0) { break; } } if (j == 512) { debug("USB Host Controller: Sector %d is empty\r\n", testSectorNumber); } else {
		for (j = 0; j < 512; j++) { if (g_spareBuffer[1000 + j] != g_spareBuffer[1000]) { break; } } if (j == 512) { debug("USB Host Controller: Sector %d contains unique number %d\r\n", testSectorNumber, g_spareBuffer[1000]); }
		else { debug("USB Host Controller: Sector %d is random\r\n", testSectorNumber); } }
		testSectorNumber++; }
#endif

#if 0 /* Test Write-Unique/Read */
	//--- While loop test ---
	uint16_t j;
	uint8_t stallFound = 0;
	uint16_t sectorsWritten = 0;
	uint8_t showDebug = 0;
	testSectorNumber = 7777777;

	SoftUsecWait(1000 * SOFT_MSECS);
	USB_readSector(0, &g_spareBuffer[1000], YES);
	SoftUsecWait(1000 * SOFT_MSECS);

	debug("USB Host Controller: ----------------\r\n");
	debug("USB Host Controller: Write unique and Read test start...\r\n");
	debug("USB Host Controller: ----------------\r\n");
	while (1)
	{
		if (GetPowerOnButtonState() == ON) { debug("\r\n\r\nKeypad: Loop break\r\n\r\n"); break; }

		//--- Wite sectors with unique number ---
		if (showDebug) { debug("USB Host Controller: (Setup) Write sector %d with unique number %d\r\n", testSectorNumber, (testSectorNumber - 7777777 + 1)); }
		memset(&g_spareBuffer[1000], (testSectorNumber - 7777777 + 1), 512);
#if 0 /* Method 1 */
		if (USB_writeSector(testSectorNumber, &g_spareBuffer[1000], YES) == rslSTALL) { stallFound = 1; if (USB_writeSector(testSectorNumber, &g_spareBuffer[1000], YES) == rslSUCCES) { stallFound = 0; } }
		//SoftUsecWait(500 * SOFT_MSECS);
		USB_testUnitReady(YES);
#else /* Method 2 */
		while (USB_writeSector(testSectorNumber, &g_spareBuffer[1000], NO) == rslSTALL) { debugWarn("USB Host Controller: Write sector %d, retry after stall\r\n", testSectorNumber); }
		//USB_testUnitReady(YES);
#endif
		//debug("Fuel Gauge: %s, BC charge current: %u mA\r\n", FuelGaugeDebugString(), GetBattChargerBatteryChargeCurrent());
		if (!stallFound) { sectorsWritten++; }
		if (showDebug) { debug("USB Host Controller: Total sectors written before stall is %d (stall found: %s)\r\n", sectorsWritten, ((stallFound == 0) ? "No" : "Yes")); }
		//SoftUsecWait(500 * SOFT_MSECS);

		//--- Read sectors have unique number stored ---
		memset(&g_spareBuffer[1000], 0, 512);
#if 0 /* Method 1 */
		USB_readSector(testSectorNumber, &g_spareBuffer[1000], YES);
#else /* Method 2 */
		while (USB_readSector(testSectorNumber, &g_spareBuffer[1000], NO) == rslSTALL) { debugWarn("USB Host Controller: Read sector %d, retry after stall\r\n", testSectorNumber); }
#endif
		if (showDebug)
		{
			for (j = 0; j < 512; j++) { if (g_spareBuffer[1000 + j] != 0) { break; } } if (j == 512) { debug("USB Host Controller: Sector %d is empty\r\n", testSectorNumber); } else {
			for (j = 0; j < 512; j++) { if (g_spareBuffer[1000 + j] != g_spareBuffer[1000]) { break; } } if (j == 512) { debug("USB Host Controller: Sector %d contains unique number %u\r\n", testSectorNumber, g_spareBuffer[1000]); }
			else { debug("USB Host Controller: Sector %d is random\r\n", testSectorNumber); } }
		}
		else // Test for negative result
		{
			for (j = 0; j < 512; j++) { if (g_spareBuffer[1000 + j] != g_spareBuffer[1000]) { break; } } if (j != 512) { debugWarn("USB Host Controller: Sector %d is not consistent\r\n", testSectorNumber); }
		}

		if (showDebug) { debug("Fuel Gauge: %s, BC charge current: %u mA\r\n", FuelGaugeDebugString(), GetBattChargerBatteryChargeCurrent()); }
		//SoftUsecWait(500 * SOFT_MSECS);

		testSectorNumber++;
		if ((testSectorNumber % 10) == 0) { debugRaw("."); }
	}
#endif

#if 0 /* Test */
	//--- While loop test ---
	while (1)
	{
		if (GetPowerOnButtonState() == ON) { debug("\r\n\r\nKeypad: Loop break\r\n\r\n"); break; }

		for (i = 0; i < 0; i++)
		{
			USB_testUnitReady(YES);
			SoftUsecWait(1 * SOFT_SECS);
		}

		for (i = 0; i < 0; i++)
		{
			USB_mscInquiry(g_spareBuffer, YES);
			SoftUsecWait(1 * SOFT_SECS);
		}

		for (i = 0; i < 0; i++)
		{
			USB_readCapacity(g_spareBuffer, YES);
			SoftUsecWait(1 * SOFT_SECS);
		}

		for (i = 0; i < 2; i++) { USB_readSector(testSectorNumber, &g_spareBuffer[1000], YES); }

		uint8_t tempOffset = ((testSectorNumber % 100) + 8);
		if (g_spareBuffer[1000] == tempOffset) { tempOffset += 7; }
		USB_writeSector(testSectorNumber, &g_spareBuffer[1000], YES);
		USB_testUnitReady(YES);

		for (i = 0; i < 2; i++) { USB_readSector(testSectorNumber, &g_spareBuffer[1000], YES); }

		testSectorNumber++;

		SoftUsecWait(1 * SOFT_SECS);
	}
#endif
#endif

#if 1 /* Test individual loop tests for Status, Unit Ready, Inquiry */
	for (i = 0; i < 1; i++)
	{
		USB_getStatus(YES);
		SoftUsecWait(1 * SOFT_SECS);
	}

	for (i = 0; i < 1; i++)
	{
		USB_testUnitReady(YES);
		SoftUsecWait(1 * SOFT_SECS);
	}

	for (i = 0; i < 1; i++)
	{
		USB_mscInquiry(&g_spareBuffer[1000], YES);
		SoftUsecWait(1 * SOFT_SECS);
	}
#endif

#if 0 /* Test disabled for now until above Bulk working */
	for (i = 0; i < 1; i++)
	{
		USB_mscInquiry(g_spareBuffer, YES);
		SoftUsecWait(1 * SOFT_SECS);
	}

	for (i = 0; i < 1; i++)
	{
		USB_getStatus(YES);
	}

	for (i = 0; i < 1; i++)
	{
		USB_mscInquiry(g_spareBuffer, YES);
		SoftUsecWait(1 * SOFT_SECS);
	}

	for (i = 0; i < 3; i++)
	{
		USB_getStatus(YES);
	}

	USB_readCapacity(YES);
	SoftUsecWait(1 * SOFT_SECS);

	for (i = 0; i < 3; i++)
	{
		USB_getStatus(YES);
	}

	USB_readSector(0, g_spareBuffer, YES);
	SoftUsecWait(1 * SOFT_SECS);

	for (i = 0; i < 3; i++)
	{
		USB_getStatus(YES);
	}

	USB_readSector(0, g_spareBuffer, YES);
	SoftUsecWait(1 * SOFT_SECS);
#endif

#if 1 /* File test */
	debug("Calling SetupUsbMscFlashDriveAndFilesystem...\r\n");
extern void SetupUsbMscFlashDriveAndFilesystem(void);
	SetupUsbMscFlashDriveAndFilesystem();
	SoftUsecWait(1 * SOFT_SECS);

	g_syncFileExistsAction = 0;
extern FRESULT USB_RecursiveSyncEventsDirectory(char* path);
	USB_RecursiveSyncEventsDirectory(EVENTS_PATH);

	debug("UsbMscFlashTestFile: Unmounting USB MSC Flash filesystem\r\n");
	f_mount(NULL, "1:", 0);

	sprintf((char*)g_spareBuffer, "USB Flash drive sync done (Device stalls overcome: %d)", usbStallsEncountered);
	MessageBox(getLangText(STATUS_TEXT), (char*)g_spareBuffer, MB_OK);
#endif

#if 1 /* Normal (shutdown) */
	debug("USB Host Controller: ----------------\r\n");
	debug("USB Host Controller: Done with access, powering down\r\n");
	debug("USB Host Controller: ----------------\r\n");

	// Use Power button to start or as escape mechanism, reset here since combo key used for actication
	ClearSoftTimer(POWER_OFF_TIMER_NUM);
	g_powerOffAttempted = NO;

	USBCPortControllerSwapToDevice();

	/* Enable the Max reset */
	MAX_writeRegister(15, BIT5);

#if 1 /* Test */
	SetBattChargerChargeState(ON);

	// Re-enable Aux power
	PowerControl(USB_AUX_POWER_ENABLE, ON);

#if 0 /* Test re-init to see if second call to swap to Device mode works, didn't work */
	USBCPortControllerSwapToDevice();
	SoftUsecWait(2 * SOFT_SECS);
#endif

#if 0 /* Option to shutdown the MCU USB Device driver */
	// Re-enable the MCU USB
extern void SetupUSBComposite(uint8_t);
	SetupUSBComposite(USB_COMPOSITE_OPTION_FLAG);
#endif
#endif
#else /* Skip shutdown */
	debug("USB Host Controller: ----------------\r\n");
	debug("USB Host Controller: Done with init and test, keeping alive...\r\n");
	debug("USB Host Controller: ----------------\r\n");

	// Use Power button to start or as escape mechanism, reset here since combo key used for actication
	ClearSoftTimer(POWER_OFF_TIMER_NUM);
	g_powerOffAttempted = NO;

#if 1 /* File test */
	debug("Calling SetupUsbMscFlashDriveAndFilesystem...\r\n");
extern void SetupUsbMscFlashDriveAndFilesystem(void);
	SetupUsbMscFlashDriveAndFilesystem();
	SoftUsecWait(1 * SOFT_SECS);

extern void UsbMscFlashTestFile(void);
	UsbMscFlashTestFile();

	sprintf((char*)g_spareBuffer, "USB Flash drive file test done (Device stalls overcome: %d)", usbStallsEncountered);
	MessageBox(getLangText(STATUS_TEXT), (char*)g_spareBuffer, MB_OK);
#endif
#endif
}
#endif

#if 1 /* Test */
uint8_t g_usbDebug = OFF;
uint8_t g_usbSourceExternalPower = NO;
#endif

#if 1
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
// EVENTS_PATH
FRESULT USB_RecursiveSyncEventsDirectory(char* path)
{
	FRESULT res;
	DIR dir;
	UINT i;
	static FILINFO fno; //, fcopy;
	//uint16_t eventNumber;
	char workingPath[80];
	char copyPath[80];
	char fileExtension[10];
	char* fileExtensionStartPtr;
	uint8_t skipFileCopy;
	uint8_t targetExists;
	uint16_t duplicateCount;
	uint8_t lastSyncAction;
	INPUT_MSG_STRUCT mn_msg;

	uint16_t filesCopied = 0;
	uint16_t filesSkipped = 0;
	uint16_t filesReplaced = 0;
	uint16_t filesDuplicated = 0;

	strcpy(workingPath, path);

	// Open directory
	res = f_opendir(&dir, workingPath);

	if (res == FR_OK)
	{
		debug("USB Event Sync: Working directory %s\r\n", path);

		// Copy working directory path to mirror on USB Flash drive
		strcpy(copyPath, path); copyPath[0] = '1';

		// Check for and make base working directory (/Events) on USB Flash drive if it does not exist
		if ((f_stat((const TCHAR*)copyPath, NULL)) == FR_OK) { debug("USB Event Sync: Base working directory %s already exists\r\n", copyPath); }
		else { debug("USB Event Sync: Making directory %s\r\n", copyPath); if (f_mkdir(copyPath) != FR_OK) { debugErr("USB Event Sync: Failed to make directory %s\r\n", copyPath); } }

		while (1)
		{
			// Find next directory item
			res = f_readdir(&dir, &fno);

			// Check if failed or at end of directory contents
			if (res != FR_OK || fno.fname[0] == 0) { debug("USB Event Sync: End of directory %s\r\n", path); break; }
			else { debug("USB Event Sync: Working on item %s\r\n", fno.fname); }

			// Check if a directory
			if (fno.fattrib & AM_DIR)
			{
				// Mark current path
				i = strlen(workingPath);

				// Add sub-directory to path
				sprintf(&workingPath[i], "%s", fno.fname);

				// Copy sub-directory path to mirror on USB Flash drive
				strcpy(copyPath, workingPath); copyPath[0] = '1';

				// Check for and make sub-directory on USB Flash drive if it does not exist
				if ((f_stat((const TCHAR*)copyPath, NULL)) == FR_OK) { debug("USB Event Sync: Directory %s already exists\r\n", copyPath); }
				else { debug("USB Event Sync: Making directory %s\r\n", copyPath); if (f_mkdir(copyPath) != FR_OK) { debugErr("USB Event Sync: Failed to make directory %s\r\n", copyPath); } }

				//sprintf((char*)g_debugBuffer, "%s %s (%s %s)", getLangText(EVENT_TEXT), getLangText(SYNC_IN_PROGRESS_TEXT), getLangText(DIR_TEXT), workingPath);
				//OverlayMessage(getLangText(SUMMARY_LIST_TEXT), (char*)g_debugBuffer, 0);

				// Enter sub-directory to repeat process
				debug("USB Event Sync: Entering sub-directory %s\r\n", workingPath);
				res = USB_RecursiveSyncEventsDirectory(workingPath);
				if (res != FR_OK) { break; }

				// Cut off added sub-directory to path
				workingPath[i] = '\0';
			}
			else // File
			{
				// Check if the file has the correct event file extension
				if(strstr(fno.fname, "ns8"))
				{
					skipFileCopy = NO;
					duplicateCount = 0;
					targetExists = NO;

					// Make USB flash drive path and filename
					sprintf(copyPath, "%s/%s", workingPath, fno.fname);
					copyPath[0] = '1';

					// Check if USB event exists
					debug("USB Event Sync: Checking if %s exists...\r\n", copyPath);
					if ((f_stat((const TCHAR*)copyPath, NULL)) == FR_OK) { targetExists = YES; }

					// prompt, skip, replace, duplicate?

					//=========================================================================
					// Destination file already exists
					//-------------------------------------------------------------------------
					if (targetExists)
					{
						debug("USB Event Sync: Destination %s file exists\r\n", fno.fname);

						// Silent option to force skipping all options
						if (0)
						{
							g_syncFileExistsAction = SKIP_ALL_OPTION;
						}

						//=========================================================================
						// New method to prompt the user what action for a file that already exists
						//-------------------------------------------------------------------------
						// Offer options the first time when the file already exists on the destination (skip, replace, duplicate, skip all, replace all, duplicate all)
						if ((g_syncFileExistsAction < SKIP_ALL_OPTION) && (!duplicateCount))
						{
							lastSyncAction = g_syncFileExistsAction;
							g_syncFileExistsAction = 0;

							memset(g_menuTags[FILENAME_TAG].text, 0, MENU_TAGS_MAX_CHARS);
							strncpy(g_menuTags[FILENAME_TAG].text, (char*)fno.fname, (MENU_TAGS_MAX_CHARS - 1));

							SoftUsecWait(750 * SOFT_MSECS);

extern USER_MENU_STRUCT syncFileExistsMenu[];
							SETUP_USER_MENU_MSG(&syncFileExistsMenu, lastSyncAction);
							JUMP_TO_ACTIVE_MENU();

							// Wait for user input, need key input
							while (g_syncFileExistsAction == 0)
							{
								HandleSystemEvents();
							}
						}

						//=========================================================================
						// Skip option
						//-------------------------------------------------------------------------
						if ((g_syncFileExistsAction == SKIP_OPTION) || (g_syncFileExistsAction == SKIP_ALL_OPTION))
						{
							//if (g_syncFileExistsAction == SKIP_OPTION)
							{
								memset(g_spareBuffer, 0, MAX_FILE_NAME_CHARS);
								sprintf((char*)g_spareBuffer, "%s: %s %s, %s", getLangText(FILE_TEXT), (char*)fno.fname, getLangText(EXISTS_TEXT), getLangText(SKIPPED_TEXT));
								OverlayMessage(getLangText(SYNC_PROGRESS_TEXT), (char*)g_spareBuffer, (0 * SOFT_SECS));
							}

							debug("USB Event Sync: Skipping %s copy\r\n", fno.fname);
							skipFileCopy = YES;
							filesSkipped += 1;
						}
						//=========================================================================
						// Replace option
						//-------------------------------------------------------------------------
						else if ((g_syncFileExistsAction == REPLACE_OPTION) || (g_syncFileExistsAction == REPLACE_ALL_OPTION))
						{
							memset(g_spareBuffer, 0, MAX_FILE_NAME_CHARS);
							sprintf((char*)g_spareBuffer, "%s: %s %s", getLangText(FILE_TEXT), (char*)fno.fname, getLangText(REPLACED_TEXT));
							OverlayMessage(getLangText(SYNC_PROGRESS_TEXT), (char*)g_spareBuffer, 0);

							debug("USB Event Sync: Replacing %s\r\n", fno.fname);

							filesReplaced += 1;
						}
						//=========================================================================
						// Duplicate option
						//-------------------------------------------------------------------------
						else if ((g_syncFileExistsAction == DUPLICATE_OPTION) || (g_syncFileExistsAction == DUPLICATE_ALL_OPTION))
						{
							memset(g_spareBuffer, 0, MAX_FILE_NAME_CHARS);
							sprintf((char*)g_spareBuffer, "%s: %s %s", getLangText(FILE_TEXT), (char*)fno.fname, getLangText(DUPLICATED_TEXT));
							OverlayMessage(getLangText(SYNC_PROGRESS_TEXT), (char*)g_spareBuffer, 0);

							duplicateCount = 1;
							memset(fileExtension, 0, sizeof(fileExtension));
							fileExtensionStartPtr = strstr(copyPath, ".");

							// Check if the '.' separator was found
							if (fileExtensionStartPtr)
							{
								// Skip past '.' separator
								sscanf((fileExtensionStartPtr + 1), "%s", fileExtension);

								// Terminate the string at the '.' separator
								*fileExtensionStartPtr = '\0';
							}

							// Copy the file name (either up to the '.' separator or the entire string if no separator was found)
							strcpy(g_spareFileName, copyPath);

							// fileExtension = ns8
							// g_spareFileName = 1:Events/Evts 1-99/Evt33

							while (duplicateCount < 9)
							{
								if (strlen(fileExtension)) { sprintf((char*)copyPath, "%s (%d).%s", g_spareFileName, duplicateCount, fileExtension); }
								else { sprintf((char*)copyPath, "%s (%d)", g_spareFileName, duplicateCount); }

								if ((f_stat((const TCHAR*)copyPath, NULL)) == FR_OK)
								{
									debug("USB Event Sync: Destination %s already exists\r\n", copyPath);
									duplicateCount++;
								}
								else
								{
									debug("USB Event Sync: Destination %s is free to use\r\n", copyPath);
									break;
								}
							}

							filesDuplicated += 1;
							debug("USB Event Sync: Duplicating %s to %s\r\n", fno.fname, copyPath);
						}
					}
					else
					{
						memset(g_spareBuffer, 0, MAX_FILE_NAME_CHARS);
						sprintf((char*)g_spareBuffer, "%s: %s %s", getLangText(FILE_TEXT), (char*)fno.fname, getLangText(COPY_TEXT));
						OverlayMessage(getLangText(SYNC_PROGRESS_TEXT), (char*)g_spareBuffer, (0 * SOFT_SECS));

						debug("USB Event Sync: Destination %s filename is available\r\n", fno.fname);
					}

					// Copy time
					if (skipFileCopy == NO)
					{
						// Perform copy
						sprintf(g_spareFileName, "%s/%s", workingPath, fno.fname);
						debug("USB Event Sync: Copying %s to %s\r\n", g_spareFileName, copyPath);

extern void UsbMscFlashTestCopyFile(char* sourceFile, char* destFile);
						UsbMscFlashTestCopyFile(g_spareFileName, copyPath);

						filesCopied += 1;
					}

				} // End of source file existing
			}
		}

		f_closedir(&dir);
	}

	// Check if the base recursive call is the original Events directory
	if (strncmp(path, EVENTS_PATH, strlen(EVENTS_PATH)) == 0)
	{
		debug("USB Event Sync: Complete, Copied: %d, Replaced: %d, Duplicated: %d, Skipped: %d\r\n", filesCopied, filesReplaced, filesDuplicated, filesSkipped);

		// Jump to the main menu
		debug("Jumping to Main Menu\r\n");
		SETUP_MENU_MSG(MAIN_MENU);
		JUMP_TO_ACTIVE_MENU();
	}

	return res;
}
#endif
