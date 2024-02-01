// SPDX-License-Identifier: GPL-2.0
/*
 * Driver for TI TPS25750 USB Power Delivery controller family
 *
 * Copyright (C) 2023, Geotab Inc.
 * Author: Abdel Alkuor <abdelalkuor@geotab.com>
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Typedefs.h"
#include "Common.h"
#include "mxc_errors.h"
#include "i2c.h"
#include "mxc_delay.h"

#include "UsbPortController.h"
#include "PowerManagement.h"

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
#define TPS_BUNDLE_SLAVE_ADDR	0x0F

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

	data[0] = len;
	memcpy(&data[1], val, len);

	return (WriteI2CDevice(MXC_I2C0, I2C_ADDR_USBC_PORT_CONTROLLER, &reg, sizeof(uint8_t), data, (len + 1)));
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

		// Delay 10ms
		MXC_Delay(MXC_DELAY_MSEC(10));
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
		ret = tps25750_block_write(tps, TPS_REG_DATA1,
				     in_data, in_len);
		if (ret)
			return ret;
	} else {
		/*
		 * For some reason, if no data is written to
		 * TPS_REG_DATA1 before sending 4CC, then 4CC would fail
		 */
		dummy[0] = TPS_REG_DATA1;
		ret = tps25750_block_write_raw(tps, dummy, sizeof(dummy));
		if (ret)
			return ret;
	}

	ret = tps25750_block_write(tps, TPS_REG_CMD1, cmd, 4);
	if (ret)
		return ret;

	ret = tps25750_wait_cmd_complete(tps, cmd_timeout_ms);

	if (ret)
		return ret;

	MXC_Delay(MXC_DELAY_MSEC(response_delay_ms));

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
		debugErr("USB Port Controller: invalid fw size\r\n");
		return E_INVALID;
	case TPS_TASK_BPMS_INVALID_SLAVE_ADDR:
		debugErr("USB Port Controller: invalid slave address\r\n");
		return E_INVALID;
	case TPS_TASK_BPMS_INVALID_TIMEOUT:
		debugErr("USB Port Controller: timed out\r\n");
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
		debugErr("USB Port Controller: pbmc failed: %u\r\n", out_data[TPS_PBMC_RC]);
		return E_COMM_ERR;
	}

	if (out_data[TPS_PBMC_DPCS]) {
		debugErr("USB Port Controller: failed device patch complete status: %u\r\n", out_data[TPS_PBMC_DPCS]);
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
	int ret;
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

	ret = tps25750_block_write_raw(tps, data, len);

	//tps->client->addr = addr;
	//tps->client->adapter->timeout = timeout;

	return (ret);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static int tps25750_start_patch_burst_mode(struct tps25750 *tps)
{
	int ret = 0;
#if 0 /* Todo: fill in equivalent */
	int ret;
	const struct firmware *fw;
	const char *firmware_name;
	struct {
		uint32_t fw_size;
		uint8_t i2c_slave_addr;
		uint8_t timeout;
	} __packed pbms_in_data;

	ret = device_property_read_string(tps->dev, "firmware-name", &firmware_name);
	if (ret)
		return ret;

	ret = request_firmware(&fw, firmware_name, tps->dev);
	if (ret) {
		debugErr("USB Port Controller: failed to retrieve \"%s\"\n", firmware_name);
		return ret;
	}

	if (fw->size == 0) {
		ret = E_INVALID;
		goto release_fw;
	}

	pbms_in_data.fw_size = fw->size;
	pbms_in_data.i2c_slave_addr = TPS_BUNDLE_SLAVE_ADDR;
	pbms_in_data.timeout = TPS_BUNDLE_TIMEOUT;

	ret = tps25750_exec_patch_cmd_pbms(tps, (uint8_t *)&pbms_in_data, sizeof(pbms_in_data));
	if (ret)
		goto release_fw;

	ret = tps25750_write_firmware(tps, fw->data, fw->size);
	if (ret) {
		debugErr("USB Port Controller: Failed to write patch %s of %lu bytes\n", firmware_name, fw->size);
	} else {
		/*
		 * A delay of 500us is required after the firmware is written
		 * based on pg.62 in TPS25750 Host Interface Technical
		 * Reference Manual
		 */
		MXC_Delay(500);
		ret = 0;
	}

release_fw:
	release_firmware(fw);
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
	uint64_t boot_status;

	ret = tps2750_is_mode(tps, TPS_MODE_PTCH);
	if (ret != 1)
		return ret;

	ret = tps25750_get_reg_boot_status(tps, &boot_status);
	if (ret)
		return ret;

	/*
	 * Nothing to be done if the configuration
	 * is being loaded from EERPOM
	 */
	if (TPS_REG_BOOT_STATUS_I2C_EEPROM_PRESENT(boot_status))
		goto wait_for_app;

	ret = tps25750_start_patch_burst_mode(tps);
	if (ret) {
		tps25750_abort_patch_process(tps);
		return ret;
	}

	ret = tps25750_complete_patch_process(tps);
	if (ret)
		return ret;

wait_for_app:
	//timeout = 1000; jiffies + msecs_to_jiffies(1000);

	do {
		ret = tps2750_is_mode(tps, TPS_MODE_APP);
		if (ret < 0)
			return ret;

#if 0 /* Todo: fill in equivalent */
		if (time_is_before_jiffies(timeout))
			return E_TIME_OUT;
#endif

		MXC_Delay(MXC_DELAY_MSEC(10));

	} while (ret != 1);

	if (ret == 0)
		return E_FAIL;

	debug("USB Port Controller: controller switched to \"APP\" mode\r\n");

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

	// Determine role for sourcing VBUS from the 5V Buck and set before swapping mode
	if (role == TYPEC_SOURCE) { PowerControl(USB_SOURCE_ENABLE, ON); } // Delay needed to let power settle?
	else { PowerControl(USB_SOURCE_ENABLE, OFF); }

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
	 *    ...
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
		dev_err(tps->dev, "data-role not found: %d\n", ret);
		goto err_role_put;
	}

	ret = typec_find_port_data_role(data_role);
	if (ret < 0) {
		dev_err(tps->dev, "unknown data-role: %s\n", data_role);
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
void USBCPortControllerInit(void)
{
	// Todo: Initial setup?
	struct tps25750 tps;
	uint64_t bootStatus;
	uint16_t powerStatus;
	uint32_t status;

	// In relation to VBUS charging (supplied externally through VBUS), what purpose does Aux Power Enable have?
	// In order to set the Aux Power Enable, external VBUS must be present

	// Check mode
	if (tps2750_is_mode(&tps, TPS_MODE_BOOT) == 1) { debugWarn("USB Port Controller: In Boot mode, likely Device booting in dead battery\r\n"); }

	// Check and clear Dead Battery flag if set
	tps25750_get_reg_boot_status(&tps, &bootStatus);

	if (TPS_REG_BOOT_STATUS_DEAD_BATTERY_FLAG(bootStatus))
	{
		debugWarn("USB Port Controller: Dead Battery flag set, must clear to continue\r\n");
		tps25750_clear_dead_battery(&tps);
	}

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

	uint8_t testBootStatus[5];
	tps25750_block_read(&tps, TPS_REG_BOOT_STATUS, testBootStatus, sizeof(testBootStatus));
	debug("USB Port Controller: Boot Status Register is 0x%x 0x%x 0x%x 0x%x 0x%x\r\n", testBootStatus[0], testBootStatus[1], testBootStatus[2], testBootStatus[3], testBootStatus[4]);
}
