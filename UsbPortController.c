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

#if /* New board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_PRODUCTION)
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
	if (state) { reg |= 0x03; }
	SetUsbHCRegister(20, reg);

	reg = 0;
	GetUsbHCRegister(20, &reg, 1);
	if (state)
	{
		if (reg == 0xF3) { debug("USB Host Controller: Mux and Source enabled\r\n"); }
		else { debug("USB Host Controller: Mux and Source enable failed (0x%x)\r\n", state); }
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
uint8_t usbMscBulkInEP = 0;
uint8_t usbMscBulkOutEP = 0;

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
void MAX_checkBusState(void)
{
    uint8_t result = MAX_scanBus();

    if (result == 0x01 || result == 0x02) { peripheralAvailable = true; debug("USB Host Controller: *** Peripheral found ***\r\n"); OverlayMessage(getLangText(STATUS_TEXT), "FOUND USB DEVICE", (2 * SOFT_SECS)); }
    else { peripheralAvailable = false; debugWarn("USB Host Controller: --- No peripheral available ---\r\n"); OverlayMessage(getLangText(WARNING_TEXT), "NO USB DEVICE DETECTED", (2 * SOFT_SECS)); }
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
void USB_busReset(void)
{
	debug("USB Host Controller: Perform bus reset\r\n");

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

	debug("USB Host Controller: Bus reset done\r\n");
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

	/* Enable HOST, DMPULLDN and DPPULLDN */
	MAX_enableOptions(rMODE, BIT0 | BIT6 | BIT7);
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
uint8_t transmitPacket(uint8_t token, uint8_t ep)
{
    uint8_t regval;
    uint16_t timeout;

    /* Instruct the module to send the data as the specified type */
    MAX_writeRegister(rHXFR, token | ep);

    //SysCtlDelay(1000);
	SoftUsecWait((1000 / MAX_TIMER_CONVERSION));

    timeout = 0xFFFF;
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

    return regval;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t requestData(uint8_t* rxbuffer, uint16_t nbytes)
{
    uint8_t it, timeout, readlength, result;

#if 1 /* Original */
    /* Send a BULK-IN request packet */
    transmitPacket(xfrIN, usbMscBulkInEP);
#else /* Test the opposite */
    /* Send a BULK-OUT request packet */
    transmitPacket(xfrOUT, usbMscBulkOutEP);
#endif

    /* Wait until we have a reply, or timeout */
    timeout = 0xFF;
    while ((!(MAX_readRegister(rHIRQ) & MAX_IRQ_RCVDAV)) && timeout)
	{
        //SysCtlDelay(300);
		SoftUsecWait((300 / MAX_TIMER_CONVERSION));

        timeout--;
    }

    /* Quit if we got a timeout */
    if (!timeout) { return 0xFF; }

    /* This delay is apparently necessary, as the RX FIFO isn't directly ready (first byte will randomly corrupt) */
    //SysCtlDelay(500);
	SoftUsecWait((500 / MAX_TIMER_CONVERSION));

    /* Get the length of the received data (should be the same as nbytes) */
    readlength = MAX_readRegister(rRCVBC);

    if (readlength != nbytes)
	{
        debugErr("USB Host Controller: Error: expected %d bytes, but got %d!\r\n", nbytes, readlength);
        return 0xF0;
    }
    //totalRcvd += readlength;

    /* Check the transfer result code: if it's an error, then quit */
    result = MAX_readRegister(rHRSL) & 0x0F;

    if (result && result != rslBUSY)
	{
        return result;
    }

    /* No error, so read the actual data */
    MAX_multiReadRegister(rRCVFIFO, rxbuffer, readlength);

    /* A simple test for now */
    /* TODO remove! */
    for (it = 0; it < 64; it++)
	{
        if ( rxbuffer[it] > 9 )
		{
            debugErr("USB Host Controller: Byte error: %d (%d)...\r\n", rxbuffer[it], it);
		}
    }

    /* Clear the interrupt */
    MAX_writeRegister(rHIRQ, MAX_IRQ_RCVDAV);

    return 0;
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
uint8_t sendControl(ControlPacket* packet)
{
    uint8_t rescode;
    uint32_t timeout;

#if 1 /* Original */
	/* Make sure the peripheral address is correct */
	MAX_writeRegister(rPERADDR, packet->perAddress);
	debug("USB Host Controller: Peripheral Addr set\r\n");
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

    /* Load the contents from the given packet and send this as a Control packet, should be setup little endian */
    TXData[0] = packet->bmRequestType;
    TXData[1] = packet->bRequest;
    TXData[2] = (uint8_t) ((packet->wValue & 0xFF));
    TXData[3] = (uint8_t) (packet->wValue >> 8);
    TXData[4] = (uint8_t) ((packet->wIndex & 0xFF));
    TXData[5] = (uint8_t) (packet->wIndex >> 8);
    TXData[6] = (uint8_t) ((packet->wLength & 0xFF));
    TXData[7] = (uint8_t) (packet->wLength >> 8);

	debug("USB Host Controller: TX packet %02x %02x %02x %02x %02x %02x %02x %02x\r\n", TXData[0], TXData[1], TXData[2], TXData[3], TXData[4], TXData[5], TXData[6], TXData[7]);

    /* Write the data into the SUPFIFO */
    MAX_multiWriteRegister(rSUDFIFO, (uint8_t*)TXData, 8);

    debug("USB Host Controller: Sending bRequest: 0x%x. (addr %d)\r\n", packet->bRequest, packet->perAddress);

    /* Start the transaction */
    rescode = transmitPacket(0x10, 0);
    if (rescode) { debugErr("USB Host Controller: Transmit packet retrn code (%d)\r\n", rescode); return rescode; }
	else { debug("USB Host Controller: Transmit packed (Setup) success\r\n"); }

    /* Check whether we need a data stage (request only at the moment) and perform */
    if ((packet->wLength > 0) && (packet->direction == DIR_IN))
	{
        rescode = transmitPacket(xfrIN, 0);
        if (rescode) { debugErr("USB Host Controller: Transmit packet retrn code (%d)\r\n", rescode); return rescode; }
		else { debug("USB Host Controller: Transmit packed (In) success\r\n"); }

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
			debug("USB Host Controller: Read length %d bytes\r\n", lastReadSize);

            MAX_multiReadRegister(rRCVFIFO, (uint8_t*)ControlBuffer, lastReadSize);

			if (lastReadSize)
			{
	            debugRaw("\r\nUSB Host Controller: Got control data: ");
				uint8_t i = 0;
				while (i < lastReadSize)
				{
					debugRaw("%02x ", ControlBuffer[i++]);
				}
				debugRaw("\r\n");
			}

            MAX_writeRegister(rHIRQ, MAX_IRQ_RCVDAV);
        }
    }

    /* Send an HS-IN or HS-OUT. */
    if (packet->direction == DIR_OUT)
	{
        rescode = transmitPacket(0x80, 0);
		debug("USB Host Controller: Transmit packet HS-IN (response code %0x)\r\n");
    }
	else
	{
        rescode = transmitPacket(0xA0, 0);
		debug("USB Host Controller: Transmit packet HS-OUT (response code %0x)\r\n");
    }

    return rescode;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_getDeviceDescriptorShortRequest(uint8_t peraddress)
{
    ControlPacket addrPacket =
	{
		peraddress, /* perAddress */
		0x10, /* type */
		0, /* endPoint */
		0x80, /*bmRequestType*/
		0x06, /* bRequest */
		0x0100, /* wValue */
		0x0000, /* wIndex */
		0x0008, /* wLength */
		DIR_IN /* direction */
    };

    return sendControl(&addrPacket);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_getDeviceDescriptorRequest(uint8_t peraddress)
{
    ControlPacket addrPacket =
	{
		peraddress, /* perAddress */
		0x10, /* type */
		0, /* endPoint */
		0x80, /*bmRequestType*/
		0x06, /* bRequest */
		0x0100, /* wValue */
		0x0000, /* wIndex */
		0x0040, /* wLength */
		DIR_IN /* direction */
    };

    return sendControl(&addrPacket);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_getConfigurationDescriptorRequest(uint8_t peraddress)
{
    ControlPacket addrPacket =
	{
		peraddress, /* perAddress */
		0x10, /* type */
		0, /* endPoint */
		0x80, /*bmRequestType*/
		0x06, /* bRequest */
		0x0200, /* wValue */
		0x0000, /* wIndex */
		0x0009, /* wLength */
		DIR_IN /* direction */
    };

    return sendControl(&addrPacket);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_getConfigurationDescriptorFullRequest(uint8_t peraddress, uint8_t length)
{
    ControlPacket addrPacket =
	{
		peraddress, /* perAddress */
		0x10, /* type */
		0, /* endPoint */
		0x80, /*bmRequestType*/
		0x06, /* bRequest */
		0x0200, /* wValue */
		0x0000, /* wIndex */
		length, /* wLength */
		DIR_IN /* direction */
    };

    return sendControl(&addrPacket);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_getStringDescriptorRequest(uint8_t peraddress)
{
    ControlPacket addrPacket =
	{
		peraddress, /* perAddress */
		0x10, /* type */
		0, /* endPoint */
		0x80, /*bmRequestType*/
		0x06, /* bRequest */
		0x0300, /* wValue */
		0x0000, /* wIndex */
		0x0010, /* wLength */
		DIR_IN /* direction */
    };

    return sendControl(&addrPacket);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_setNewPeripheralAddress(uint8_t peraddress)
{
    ControlPacket addrPacket =
	{
		0, /* perAddress */
		0x10, /* type */
		0, /* endPoint */
		0, /*bmRequestType*/
		reqSET_ADDRESS, /* bRequest */
		peraddress, /* wValue */
		0, /* wIndex */
		0, /* wLength */
		DIR_OUT /* direction */
    };

    return sendControl(&addrPacket);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_setConfiguration(uint8_t peraddress, uint8_t config)
{
    ControlPacket addrPacket =
	{
		peraddress, /* perAddress */
		0x10, /* type */
		0, /* endPoint */
		0, /*bmRequestType*/
		reqSET_CONFIGURATION, /* bRequest */
		config, /* wValue */
		0, /* wIndex */
		0, /* wLength */
		DIR_OUT /* direction */
    };

    return sendControl(&addrPacket);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_requestStatus(uint8_t* resultBuffer)
{
    ControlPacket packet =
	{
		PERIPHERAL_ADDRESS, /* perAddress */
		0x10, /* type */
		0, /* endPoint */
		0x80, /*bmRequestType*/
		reqGET_STATUS, /* bRequest */
		0, /* wValue */
		0, /* wIndex */
		2, /* wLength */
		DIR_IN
	};

    return sendControl(&packet);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t USB_doEnumeration(void)
{
    uint16_t tries = 0;

    MAX_enableOptions(rHCTL, BIT7);
    MAX_disableOptions(rHCTL, BIT6);
    MAX_enableOptions(rHCTL, BIT5);
    MAX_disableOptions(rHCTL, BIT4);

    while (tries < 20)
	{
        if (tries)
		{
            debugErr("USB Host Controller: Enumeration failed. Retrying...\r\n");
            USB_busReset();

            //SysCtlDelay(4000000);
			SoftUsecWait((4000000 / MAX_TIMER_CONVERSION));
        }

        tries++;
        MAX_writeRegister(rPERADDR, 0);

        if (!USB_setNewPeripheralAddress(PERIPHERAL_ADDRESS))
		{
            MAX_writeRegister(rPERADDR, PERIPHERAL_ADDRESS);

            //SysCtlDelay(500000);
			SoftUsecWait((500000 / MAX_TIMER_CONVERSION));
        }
		else
		{
            continue;
        }

        if (!USB_requestStatus(0))
		{
            break;
		}
    }

    if (tries < 20)
	{
        MAX_enableOptions(rHCTL, BIT6);
        MAX_disableOptions(rHCTL, BIT7);
        MAX_enableOptions(rHCTL, BIT4);
        MAX_disableOptions(rHCTL, BIT5);

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
	debugRaw("-USBHC-");

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

            USB_busReset();

            //SysCtlDelay(4000000);
			SoftUsecWait((4000000 / MAX_TIMER_CONVERSION));

            if (USB_doEnumeration())
                debugErr("USB Host Controller ISR: Enumeration Failed...\r\n");
            else
                debug("USB Host Controller ISR: Done with enumeration!\r\n");

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
		MAX_checkBusState();

        MAX_writeRegister(rHIRQ, BIT5);
    }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MAX_processNewDevice(void)
{
    uint8_t regval;

	regval = MAX_readRegister(rHRSL);

	if (regval & 0xC0)
	{
		debug("USB Host Controller: Peripheral connected\r\n");
		peripheralConnected = 1;

		/* Enable the SOF generator */
		MAX_enableOptions(rMODE, BIT3);
		while (!(MAX_readRegister(rHIRQ) & MAX_IRQ_FRAME)) { ; }

		USB_busReset();

		//SysCtlDelay(4000000);
		SoftUsecWait((4000000 / MAX_TIMER_CONVERSION));

		if (USB_doEnumeration())
			debugErr("USB Host Controller: Enumeration Failed...\r\n");
		else
			debug("USB Host Controller: Done with enumeration\r\n");

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
	MAX_checkBusState();

	MAX_writeRegister(rHIRQ, BIT5);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetDataToggles(uint8_t snd, uint8_t rcv)
{
	uint8_t setToggles = MAX_readRegister(rHCTL);

	setToggles &= 0x0F;
#if 0 /* Straight */
	if (snd) { setToggles |= BIT7; } else { setToggles |= BIT6; }
	if (rcv) { setToggles |= BIT5; } else { setToggles |= BIT4; }
#else /* Flip */
	if (snd) { setToggles |= BIT6; } else { setToggles |= BIT7; }
	if (rcv) { setToggles |= BIT5; } else { setToggles |= BIT5; }
#endif
	MAX_writeRegister(rHCTL, setToggles);
	debug("USB Host Controller: setting HCTL to 0x%x\r\n", setToggles);
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
			OverlayMessage(getLangText(STATUS_TEXT), (char*)g_debugBuffer, (3 * SOFT_SECS));

			usbMscConfigID = configPayload[5];
		}
		else if (configPayload[1] == 0x04) // Interface descriptor
		{
			sprintf((char*)g_debugBuffer, "Interface %d, Num of Endpoints: %d, Class: %s", configPayload[2], configPayload[4], GetUsbClassName(configPayload[5]));
			debug("USB Host Controller: %s\r\n", (char*)g_debugBuffer);
			OverlayMessage(getLangText(STATUS_TEXT), (char*)g_debugBuffer, (3 * SOFT_SECS));
		}
		else if (configPayload[1] == 0x05) // Endpoint descriptor
		{
			sprintf((char*)g_debugBuffer, "Endpoint %d, %s, %s, Max size %d", (configPayload[2] & 0x0F), ((configPayload[2] & 0x80) ? "In" : "Out"), GetUsbTransferTypeName(configPayload[3]), configPayload[4]);
			debug("USB Host Controller: %s\r\n", (char*)g_debugBuffer);
			OverlayMessage(getLangText(STATUS_TEXT), (char*)g_debugBuffer, (2 * SOFT_SECS));

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

#define CBWFLAGS_DIR_IN         0x80 // For data-in operation, Indicates that data is being sent from the device to the host
#define CBWFLAGS_DIR_OUT        0x00 // For data-out operation, Indicates that data is being sent from the host to the device

// Command Descriptor Block for 6-byte command
typedef struct {
    uint8_t opCode;			// Operation code
    uint8_t lun;			// Logical unit number
    uint8_t lbaMsb;			// Logical block address, MSB (Big Endian)
    uint8_t lbaLsb;			// Logical block address, LSB (Big Endian)
    uint8_t xferLength;		// Transfer length
    uint8_t control;		// Control
	uint8_t pad[10];		// Padding for 16-byte command block
} USB_CBW_SCSI_6;

// Command Descriptor Block for 10-byte command
typedef struct {
    uint8_t opCode;			// Operation code
    uint8_t lun;			// Logical unit number
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
typedef struct {
    uint8_t opCode;			// Operation code
    uint8_t lun;			// Logical unit number
    uint8_t lba1;			// Logical block address, MSB (Big Endian)
    uint8_t lba2;			// Logical block address, 2nd MSB (Big Endian)
    uint8_t lba3;			// Logical block address, 2nd LSB (Big Endian)
    uint8_t lba4;			// Logical block address, LSB (Big Endian)
    uint8_t xferLen1;		// Transfer length, MSB (Big Endian)
    uint8_t xferLen2;		// Transfer length, 2nd MSB (Big Endian)
    uint8_t xferLen3;		// Transfer length, 2nd LSB (Big Endian)
    uint8_t xferLen4;		// Transfer length, LSB (Big Endian)
    uint8_t control;		// Control
	uint8_t pad[4];			// Padding for 16-byte command block
} USB_CBW_SCSI_12;

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
	} scsiCmd;
} USB_CBW;

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void USB_setupBulkCbw(void)
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

	USB_CBW cbwRequest;

	debug("USB Host Controller: CBW setup (Size is %d)\r\n", sizeof(cbwRequest));
	memset(&cbwRequest, 0, sizeof(cbwRequest));

	cbwRequest.dCBWSignature = __builtin_bswap32(0x43425355);
	cbwRequest.dCBWTag = __builtin_bswap32(0x00000001);
	cbwRequest.dCBWDataTransferLength = __builtin_bswap32(0x00000010);
	cbwRequest.bmCBWFlags = CBWFLAGS_DIR_IN;
	cbwRequest.bCBWLUN = 0x00;

	cbwRequest.cbCBWLength = 12;
	cbwRequest.scsiCmd.twelve.opCode = 0x28;
	cbwRequest.scsiCmd.twelve.lun = 0;
	cbwRequest.scsiCmd.twelve.lba4 = 0;
	cbwRequest.scsiCmd.twelve.xferLen4 = 1;
	cbwRequest.scsiCmd.twelve.control = 0;

	uint8_t i;
	for (i = 0; i < sizeof(cbwRequest); i++)
	{
		debug("USB Host Controller: CBW Request[%d]	= 0x%x\r\n", i, ((uint8_t*)&cbwRequest)[i]);
	}

	debug("USB Host Controller: CBW write to SNDFIFO (cbwRequest size is %d)\r\n", sizeof(cbwRequest));
	MAX_multiWriteRegister(rSNDFIFO, (uint8_t*)&cbwRequest, sizeof(cbwRequest));

	debug("USB Host Controller: Byte count write to SNDBC (Count is %d)\r\n", sizeof(cbwRequest));
	MAX_writeRegister(rSNDBC, sizeof(cbwRequest));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void USBHostControllerTest(void)
{
	USBCPortControllerSwapToHost();
    debug("USB Host Controller: Delay for USB Device power to stabilize\r\n");
	SoftUsecWait(2 * SOFT_SECS);

    MAX_start();

    MAX_checkBusState();

    /* Enable interrupts */
    MAX_enableInterrupts(MAX_IRQ_CONDET);
    MAX_clearInterruptStatus(MAX_IRQ_CONDET);
    MAX_enableInterruptsMaster();

    uint8_t regval = MAX_readRegister(rREVISION);
    debug("USB Host Controller: Revision: 0x%x\r\n", regval);

    /* Perform a bus reset to reconnect after a power down */
    if (!peripheralAvailable)
	{
        debugWarn("USB Host Controller: Perform a bus reset to reconnect after a power down\r\n");
		USB_busReset();
	}

	MAX_processNewDevice();

	uint8_t responseCode;
	uint8_t fullConfigLength = 0;
	debug("USB Host Controller: ----------------\r\n"); debug("USB Host Controller: Requesting Device Descriptor to addr %d...\r\n", PERIPHERAL_ADDRESS); CheckDataToggleAndSet(); responseCode = USB_getDeviceDescriptorRequest(PERIPHERAL_ADDRESS);
	debug("USB Host Controller: Response code %02x\r\n", responseCode);

	debug("USB Host Controller: ----------------\r\n"); debug("USB Host Controller: Requesting Configuration (Base) Descriptor to addr %d...\r\n", PERIPHERAL_ADDRESS); CheckDataToggleAndSet(); responseCode = USB_getConfigurationDescriptorRequest(PERIPHERAL_ADDRESS);
	debug("USB Host Controller: Response code %02x\r\n", responseCode);
	if (responseCode == rslSUCCES)
	{
#if 0 /* Test */
        debugRaw("\r\nUSB Host Controller: Check Control data: ");
		uint8_t i = 0;
		while (i < 9)
		{
			debugRaw("%02x ", ControlBuffer[i++]);
		}
		debugRaw("\r\n");

		debug("USB Host Controller: Config check (%d) (%d)\r\n", ControlBuffer[0], USB_CONFIG_DESCRIPTOR);
#endif
		if (ControlBuffer[1] == USB_CONFIG_DESCRIPTOR)
		{
			fullConfigLength = ControlBuffer[2];
			debug("USB Host Controller: Config full length is %d\r\n", fullConfigLength);
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

	debug("USB Host Controller: ----------------\r\n"); debug("USB Host Controller: Requesting String Descriptor to addr %d...\r\n", PERIPHERAL_ADDRESS); CheckDataToggleAndSet(); responseCode = USB_getStringDescriptorRequest(PERIPHERAL_ADDRESS);
	debug("USB Host Controller: Response code %02x\r\n", responseCode);

	if (fullConfigLength)
	{
		debug("USB Host Controller: ----------------\r\n"); debug("USB Host Controller: Requesting Configuration (Full) Descriptor to addr %d...\r\n", PERIPHERAL_ADDRESS);
		CheckDataToggleAndSet(); responseCode = USB_getConfigurationDescriptorFullRequest(PERIPHERAL_ADDRESS, fullConfigLength);
		debug("USB Host Controller: Response code %02x\r\n", responseCode);

		if (responseCode == rslSUCCES)
		{
			if (lastReadSize == fullConfigLength)
			{
				DecodeFullConfigDescriptor(fullConfigLength);
			}
			else
			{
				debugWarn("USB Host Controller: Read size did not match full Configuration Descriptor\r\n");
				OverlayMessage(getLangText(WARNING_TEXT), "Read size did not match full Configuration Descriptor", (3 * SOFT_SECS));
			}
		}
		else { debugErr("USB Host Controller: Error getting full Configuration Descriptor\r\n"); }
	}
	else
	{
		debugErr("USB Host Controller: Full configuraiton descriptor not requested due to no valid length\r\n");
	}

	debug("USB Host Controller: ----------------\r\n"); debug("USB Host Controller: USB MSC Bulk Out EP is %d, USB MSC Bulk In EP is %d\r\n", usbMscBulkOutEP, usbMscBulkInEP);

#if 0 /* Test? */
	debug("USB Host Controller: ----------------\r\n"); debug("USB Host Controller: Sending Set Configuraiton to addr %d...\r\n", PERIPHERAL_ADDRESS); CheckDataToggleAndSet(); responseCode = USB_setConfiguration(PERIPHERAL_ADDRESS, 0);
	debug("USB Host Controller: Response code %02x\r\n", responseCode);

	debug("USB Host Controller: ----------------\r\n"); debug("USB Host Controller: Sending Set Configuraiton to addr %d...\r\n", PERIPHERAL_ADDRESS); CheckDataToggleAndSet(); responseCode = USB_setConfiguration(PERIPHERAL_ADDRESS, 1);
	debug("USB Host Controller: Response code %02x\r\n", responseCode);
#endif

#if 1 /* Working */
	debug("USB Host Controller: ----------------\r\n"); debug("USB Host Controller: Sending Set Configuraiton to addr %d...\r\n", PERIPHERAL_ADDRESS); CheckDataToggleAndSet(); responseCode = USB_setConfiguration(PERIPHERAL_ADDRESS, usbMscConfigID);
	debug("USB Host Controller: Response code %02x\r\n", responseCode);
#else /* Test incorrect config number, showed success */
	debug("USB Host Controller: ----------------\r\n"); debug("USB Host Controller: Sending Set Configuraiton to addr %d...\r\n", PERIPHERAL_ADDRESS); CheckDataToggleAndSet(); responseCode = USB_setConfiguration(PERIPHERAL_ADDRESS, 7);
	debug("USB Host Controller: Response code %02x\r\n", responseCode);
#endif

	responseCode = USB_requestStatus(0);
	debug("USB Host Controller: Response code %02x\r\n", responseCode);

#if 0 /* Incorrect method to initiate bulk transfer */
	debug("USB Host Controller: ----------------\r\n"); debug("USB Host Controller: Requesting data...\r\n"); MAX_writeRegister(rHIRQ, BIT2); CheckDataToggleAndSet(); responseCode = requestData((uint8_t*) RXData, 64);
	if (responseCode != 0) { debugErr("USB Host Controller: Request result error: %s (0x%x)\r\n", ResultCodeName(responseCode), responseCode); } else { debug("USB Host Controller: Read 64 bytes\r\n"); }
#endif

	/*
		Steps to Perform a Bulk Only Transfer
		1. Sending the Command Block Wrapper (CBW)
			The host initiates the transfer by sending a Command Block Wrapper (CBW) to the device.
			This CBW is sent via a Bulk-Out endpoint and contains the command that the device needs to execute.

		2. Device Processing
			Upon receiving the CBW, the device processes the command.
			It checks the validity of the CBW and prepares to execute the requested operation.

		3. Receiving the Command Status Wrapper (CSW)
			After processing the command, the device sends back a Command Status Wrapper (CSW) to the host.
			This CSW is transmitted via a Bulk-In endpoint and indicates the status of the command execution (success or failure).

	*/

	USB_setupBulkCbw();
	requestData(g_spareBuffer, 512);

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

	//while (1)
	// Skip while loop for now
	if (0)
	{
        if (peripheralAvailable)
		{
            /* Make sure the RX buffer is free */
            MAX_writeRegister(rHIRQ, BIT2);

            uint8_t i, result;
            uint32_t totalRcvd = 0;

            debug("USB Host Controller: ----------------\r\n");
			debug("USB Host Controller: Requesting data...\r\n");
            debug("USB Host Controller: (Address: %d)\r\n", MAX_readRegister(rPERADDR));

            MAX_writeRegister(rHIRQ, BIT2);

            // Use Power button as escape mechanism, reset here since combo key used for actication
			g_powerOffAttempted = NO;

			//for (i = 0; i < 1000; i++)
			for (i = 0; i < 10; i++)
			{
				CheckDataToggleAndSet();
				result = requestData((uint8_t*) RXData, 64);
                if (result != 0)
				{
                    debugErr("USB Host Controller: Request result error: %s (0x%x) (%d)\r\n", ResultCodeName(result), result, i);
                }
				else
				{
                    debug("USB Host Controller: Read 64 bytes\r\n");
					totalRcvd += 64;
                }

				if (g_powerOffAttempted == YES) { break; }
            }

            debug("USB Host Controller: Received %d bytes\r\n", totalRcvd);
        }
    }

	// Use Power button to start or as escape mechanism, reset here since combo key used for actication
	ClearSoftTimer(POWER_OFF_TIMER_NUM);
	g_powerOffAttempted = NO;

	USBCPortControllerSwapToDevice();
}
#endif
