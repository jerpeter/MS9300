///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include "Typedefs.h"
#include "Common.h"
#include "OldUart.h"
#include "powerManagement.h"
#include "RealTimeClock.h"
#include "gpio.h"
#include "lcd.h"

#include "mxc_errors.h"
#include "i2c.h"
#include "ff.h"
//#include "navigation.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
#include "Globals.h"

///----------------------------------------------------------------------------
///	Local Scope Globals
///----------------------------------------------------------------------------
static uint32 s_powerManagement;

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void PowerControl(POWER_MGMT_OPTIONS option, BOOLEAN mode)
{
#if 0 /* temp remove while unused */
	uint8 state;
#endif

	switch (option)
	{
		//----------------------------------------------------------------------------
		case ALARM_1_ENABLE: // Active high
		//----------------------------------------------------------------------------
			debug("Alarm 1 Enable: %s\r\n", mode == ON ? "On" : "Off");
			if (mode == ON) { MXC_GPIO_OutSet(GPIO_ALERT_1_PORT, GPIO_ALERT_1_PIN); }
			else /* (mode == OFF) */ { MXC_GPIO_OutClr(GPIO_ALERT_1_PORT, GPIO_ALERT_1_PIN); }
			break;

		//----------------------------------------------------------------------------
		case ALARM_2_ENABLE: // Active high
		//----------------------------------------------------------------------------
			debug("Alarm 2 Enable: %s\r\n", mode == ON ? "On" : "Off");
			if (mode == ON) { MXC_GPIO_OutSet(GPIO_ALERT_2_PORT, GPIO_ALERT_2_PIN); }
			else /* (mode == OFF) */ { MXC_GPIO_OutClr(GPIO_ALERT_2_PORT, GPIO_ALERT_2_PIN); }
			break;

		//----------------------------------------------------------------------------
		case LCD_POWER_ENABLE: // Active high
		//----------------------------------------------------------------------------
			debug("LCD Power Enable: %s\r\n", mode == ON ? "On" : "Off");
			if (mode == ON) { MXC_GPIO_OutSet(GPIO_LCD_POWER_ENABLE_PORT, GPIO_LCD_POWER_ENABLE_PIN); /* 20ms delay needed before FT810Q ready */ }
			else /* (mode == OFF) */ { MXC_GPIO_OutClr(GPIO_LCD_POWER_ENABLE_PORT, GPIO_LCD_POWER_ENABLE_PIN); }
			break;

		//----------------------------------------------------------------------------
		case ANALOG_5V_ENABLE: // Active high
		//----------------------------------------------------------------------------
			debug("Analog (5V) Enable: %s\r\n", mode == ON ? "On" : "Off");
			if (mode == ON) { MXC_GPIO_OutSet(GPIO_ENABLE_5V_PORT, GPIO_ENABLE_5V_PIN); }
			else /* (mode == OFF) */ { MXC_GPIO_OutClr(GPIO_ENABLE_5V_PORT, GPIO_ENABLE_5V_PIN); }
			break;

		//----------------------------------------------------------------------------
		case TRIGGER_OUT: // Active high
		//----------------------------------------------------------------------------
			debug("External Trigger Out: %s\r\n", mode == ON ? "On" : "Off");
			if (mode == ON) { MXC_GPIO_OutSet(GPIO_EXTERNAL_TRIGGER_OUT_PORT, GPIO_EXTERNAL_TRIGGER_OUT_PIN); }
			else /* (mode == OFF) */ { MXC_GPIO_OutClr(GPIO_EXTERNAL_TRIGGER_OUT_PORT, GPIO_EXTERNAL_TRIGGER_OUT_PIN); }
			break;

		//----------------------------------------------------------------------------
		case MCU_POWER_LATCH: // Active high
		//----------------------------------------------------------------------------
			debug("MCU Power Latch: %s\r\n", mode == ON ? "On" : "Off");
			if (mode == ON) { MXC_GPIO_OutSet(GPIO_MCU_POWER_LATCH_PORT, GPIO_MCU_POWER_LATCH_PIN); }
			else /* (mode == OFF) */ { MXC_GPIO_OutClr(GPIO_MCU_POWER_LATCH_PORT, GPIO_MCU_POWER_LATCH_PIN); }
			break;

		//----------------------------------------------------------------------------
		case ENABLE_12V: // Active high
		//----------------------------------------------------------------------------
			debug("12V Enable: %s\r\n", mode == ON ? "On" : "Off");
			if (mode == ON) { MXC_GPIO_OutSet(GPIO_ENABLE_12V_PORT, GPIO_ENABLE_12V_PIN); }
			else /* (mode == OFF) */ { MXC_GPIO_OutClr(GPIO_ENABLE_12V_PORT, GPIO_ENABLE_12V_PIN); }
			break;

		//----------------------------------------------------------------------------
		case USB_SOURCE_ENABLE: // Active high
		//----------------------------------------------------------------------------
			debug("USB Source Enable: %s\r\n", mode == ON ? "On" : "Off");
			if (mode == ON) { MXC_GPIO_OutSet(GPIO_USB_SOURCE_ENABLE_PORT, GPIO_USB_SOURCE_ENABLE_PIN); }
			else /* (mode == OFF) */ { MXC_GPIO_OutClr(GPIO_USB_SOURCE_ENABLE_PORT, GPIO_USB_SOURCE_ENABLE_PIN); }
			break;

		//----------------------------------------------------------------------------
		case USB_AUX_POWER_ENABLE: // Active high
		//----------------------------------------------------------------------------
			debug("USB Aux Power Enable: %s\r\n", mode == ON ? "On" : "Off");
			if (mode == ON) { MXC_GPIO_OutSet(GPIO_USB_AUX_POWER_ENABLE_PORT, GPIO_USB_AUX_POWER_ENABLE_PIN); }
			else /* (mode == OFF) */ { MXC_GPIO_OutClr(GPIO_USB_AUX_POWER_ENABLE_PORT, GPIO_USB_AUX_POWER_ENABLE_PIN); }
			break;

		//----------------------------------------------------------------------------
		case ADC_RESET: // Active low
		//----------------------------------------------------------------------------
			debug("ADC Reset: %s\r\n", mode == ON ? "On" : "Off");
			if (mode == ON) { MXC_GPIO_OutClr(GPIO_ADC_RESET_PORT, GPIO_ADC_RESET_PIN); }
			else /* (mode == OFF) */ { MXC_GPIO_OutSet(GPIO_ADC_RESET_PORT, GPIO_ADC_RESET_PIN); }
			break;

		//----------------------------------------------------------------------------
		case EXPANSION_ENABLE: // Active high
		//----------------------------------------------------------------------------
			debug("Expansion Enable: %s\r\n", mode == ON ? "On" : "Off");
			if (mode == ON) { MXC_GPIO_OutSet(GPIO_EXPANSION_ENABLE_PORT, GPIO_EXPANSION_ENABLE_PIN); }
			else /* (mode == OFF) */ { MXC_GPIO_OutClr(GPIO_EXPANSION_ENABLE_PORT, GPIO_EXPANSION_ENABLE_PIN); }
			break;

		//----------------------------------------------------------------------------
		case SENSOR_CHECK_ENABLE: // Active high
		//----------------------------------------------------------------------------
			debug("Sensor Check Enable: %s\r\n", mode == ON ? "On" : "Off");
			if (mode == ON) { MXC_GPIO_OutSet(GPIO_SENSOR_CHECK_ENABLE_PORT, GPIO_SENSOR_CHECK_ENABLE_PIN); }
			else /* (mode == OFF) */ { MXC_GPIO_OutClr(GPIO_SENSOR_CHECK_ENABLE_PORT, GPIO_SENSOR_CHECK_ENABLE_PIN); }
			break;

		//----------------------------------------------------------------------------
		case LTE_RESET: // Active low
		//----------------------------------------------------------------------------
			debug("LTE Reset: %s\r\n", mode == ON ? "On" : "Off");
			if (mode == ON) { MXC_GPIO_OutClr(GPIO_LTE_RESET_PORT, GPIO_LTE_RESET_PIN); }
			else /* (mode == OFF) */ { MXC_GPIO_OutSet(GPIO_LTE_RESET_PORT, GPIO_LTE_RESET_PIN); }
			break;

		//----------------------------------------------------------------------------
		case BLE_RESET: // Active low
		//----------------------------------------------------------------------------
			debug("BLE Reset: %s\r\n", mode == ON ? "On" : "Off");
			if (mode == ON) { MXC_GPIO_OutClr(GPIO_BLE_RESET_PORT, GPIO_BLE_RESET_PIN); }
			else /* (mode == OFF) */ { MXC_GPIO_OutSet(GPIO_BLE_RESET_PORT, GPIO_BLE_RESET_PIN); }
			break;

		//----------------------------------------------------------------------------
		case CELL_ENABLE: // Active high
		//----------------------------------------------------------------------------
			debug("Cellular: %s\r\n", mode == ON ? "On" : "Off");
			if (mode == ON) { MXC_GPIO_OutSet(GPIO_CELL_ENABLE_PORT, GPIO_CELL_ENABLE_PIN); }
			else /* (mode == OFF) */ { MXC_GPIO_OutClr(GPIO_CELL_ENABLE_PORT, GPIO_CELL_ENABLE_PIN); }
			break;

		//----------------------------------------------------------------------------
		case EXPANSION_RESET: // Active low
		//----------------------------------------------------------------------------
			debug("Expansion Reset: %s\r\n", mode == ON ? "On" : "Off");
			if (mode == ON) { MXC_GPIO_OutClr(GPIO_EXPANSION_RESET_PORT, GPIO_EXPANSION_RESET_PIN); }
			else /* (mode == OFF) */ { MXC_GPIO_OutSet(GPIO_EXPANSION_RESET_PORT, GPIO_EXPANSION_RESET_PIN); }
			break;

		//----------------------------------------------------------------------------
		case LCD_POWER_DISPLAY: // Active low
		//----------------------------------------------------------------------------
			debug("LCD Power Display: %s\r\n", mode == ON ? "On" : "Off");
			if (mode == ON) { MXC_GPIO_OutClr(GPIO_LCD_POWER_DISPLAY_PORT, GPIO_LCD_POWER_DISPLAY_PIN); /* 20ms delay needed before FT810Q ready */ }
			else /* (mode == OFF) */ { MXC_GPIO_OutSet(GPIO_LCD_POWER_DISPLAY_PORT, GPIO_LCD_POWER_DISPLAY_PIN); }
			break;

		//----------------------------------------------------------------------------
		case LED_1: // Active high (Red)
		//----------------------------------------------------------------------------
			debug("LED 1: %s\r\n", mode == ON ? "On" : "Off");
			if (mode == ON) { MXC_GPIO_OutSet(GPIO_LED_1_PORT, GPIO_LED_1_PIN); }
			else /* (mode == OFF) */ { MXC_GPIO_OutClr(GPIO_LED_1_PORT, GPIO_LED_1_PIN); }
			break;

		//----------------------------------------------------------------------------
		case LED_2: // Active high (Red)
		//----------------------------------------------------------------------------
			debug("LED 2: %s\r\n", mode == ON ? "On" : "Off");
			if (mode == ON) { MXC_GPIO_OutSet(GPIO_LED_2_PORT, GPIO_LED_2_PIN); }
			else /* (mode == OFF) */ { MXC_GPIO_OutClr(GPIO_LED_2_PORT, GPIO_LED_2_PIN); }
			break;

		//----------------------------------------------------------------------------
		case LED_3: // Active high (Green)
		//----------------------------------------------------------------------------
			debug("LED 3: %s\r\n", mode == ON ? "On" : "Off");
			if (mode == ON) { MXC_GPIO_OutSet(GPIO_LED_3_PORT, GPIO_LED_3_PIN); }
			else /* (mode == OFF) */ { MXC_GPIO_OutClr(GPIO_LED_3_PORT, GPIO_LED_3_PIN); }
			break;

		//----------------------------------------------------------------------------
		case LED_4: // Active high (Green)
		//----------------------------------------------------------------------------
			debug("LED 4: %s\r\n", mode == ON ? "On" : "Off");
			if (mode == ON) { MXC_GPIO_OutSet(GPIO_LED_4_PORT, GPIO_LED_4_PIN); }
			else /* (mode == OFF) */ { MXC_GPIO_OutClr(GPIO_LED_4_PORT, GPIO_LED_4_PIN); }
			break;
	}

	// Set Power Control state for the option selected
	if (mode == ON)
	{
		s_powerManagement |= (1 << option);
	}
	else // (mode == OFF)
	{
		s_powerManagement &= ~(1 << option);
	}

	// Can no longer delay locally since this can be called from within an interrupt
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
BOOLEAN GetPowerControlState(POWER_MGMT_OPTIONS option)
{
	BOOLEAN state = OFF;

	switch (option)
	{
		case ALARM_1_ENABLE: state = MXC_GPIO_OutGet(GPIO_ALERT_1_PORT, GPIO_ALERT_1_PIN); break;
		case ALARM_2_ENABLE: state = MXC_GPIO_OutGet(GPIO_ALERT_2_PORT, GPIO_ALERT_2_PIN); break;
		case LCD_POWER_ENABLE: state = MXC_GPIO_OutGet(GPIO_LCD_POWER_ENABLE_PORT, GPIO_LCD_POWER_ENABLE_PIN); break;
		case ANALOG_5V_ENABLE: state = MXC_GPIO_OutGet(GPIO_ENABLE_5V_PORT, GPIO_ENABLE_5V_PIN); break;
		case TRIGGER_OUT: state = MXC_GPIO_OutGet(GPIO_EXTERNAL_TRIGGER_OUT_PORT, GPIO_EXTERNAL_TRIGGER_OUT_PIN); break;
		case MCU_POWER_LATCH: state = MXC_GPIO_OutGet(GPIO_MCU_POWER_LATCH_PORT, GPIO_MCU_POWER_LATCH_PIN); break;
		case ENABLE_12V: state = MXC_GPIO_OutGet(GPIO_ENABLE_12V_PORT, GPIO_ENABLE_12V_PIN); break;
		case USB_SOURCE_ENABLE: state = MXC_GPIO_OutGet(GPIO_USB_SOURCE_ENABLE_PORT, GPIO_USB_SOURCE_ENABLE_PIN); break;
		case USB_AUX_POWER_ENABLE: state = MXC_GPIO_OutGet(GPIO_USB_AUX_POWER_ENABLE_PORT, GPIO_USB_AUX_POWER_ENABLE_PIN); break;
		case ADC_RESET: state = !MXC_GPIO_OutGet(GPIO_ADC_RESET_PORT, GPIO_ADC_RESET_PIN); break; // Active low, invert state
		case EXPANSION_ENABLE: state = MXC_GPIO_OutGet(GPIO_EXPANSION_ENABLE_PORT, GPIO_EXPANSION_ENABLE_PIN); break;
		case SENSOR_CHECK_ENABLE: state = MXC_GPIO_OutGet(GPIO_SENSOR_CHECK_ENABLE_PORT, GPIO_SENSOR_CHECK_ENABLE_PIN); break;
		case LTE_RESET: state = !MXC_GPIO_OutGet(GPIO_LTE_RESET_PORT, GPIO_LTE_RESET_PIN); break; // Active low, invert state
		case BLE_RESET: state = !MXC_GPIO_OutGet(GPIO_BLE_RESET_PORT, GPIO_BLE_RESET_PIN); break; // Active low, invert state
		case CELL_ENABLE: state = MXC_GPIO_OutGet(GPIO_CELL_ENABLE_PORT, GPIO_CELL_ENABLE_PIN); break;
		case EXPANSION_RESET: state = !MXC_GPIO_OutGet(GPIO_EXPANSION_RESET_PORT, GPIO_EXPANSION_RESET_PIN); break; // Active low, invert state
		case LCD_POWER_DISPLAY: state = !MXC_GPIO_OutGet(GPIO_LCD_POWER_DISPLAY_PORT, GPIO_LCD_POWER_DISPLAY_PIN); break; // Active low, invert state
		case LED_1: state = MXC_GPIO_OutGet(GPIO_LED_1_PORT, GPIO_LED_1_PIN); break;
		case LED_2: state = MXC_GPIO_OutGet(GPIO_LED_2_PORT, GPIO_LED_2_PIN); break;
		case LED_3: state = MXC_GPIO_OutGet(GPIO_LED_3_PORT, GPIO_LED_3_PIN); break;
		case LED_4: state = MXC_GPIO_OutGet(GPIO_LED_4_PORT, GPIO_LED_4_PIN); break;
	}

	return (state);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
BOOLEAN GetShadowPowerControlState(POWER_MGMT_OPTIONS option)
{
	BOOLEAN state = OFF;

	if (s_powerManagement & (1 << option))
	{
		state = ON;
	}

	return (state);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t GetCurrentLedStates(void)
{
	uint8_t state;

	state = MXC_GPIO_OutGet(GPIO_LED_1_PORT, GPIO_LED_1_PIN);
	state |= (MXC_GPIO_OutGet(GPIO_LED_2_PORT, GPIO_LED_2_PIN) << 1);
	state |= (MXC_GPIO_OutGet(GPIO_LED_3_PORT, GPIO_LED_3_PIN) << 2);
	state |= (MXC_GPIO_OutGet(GPIO_LED_4_PORT, GPIO_LED_4_PIN) << 3);

	return (state);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void PowerUnitOff(uint8 powerOffMode)
{
	OverlayMessage(getLangText(STATUS_TEXT), getLangText(POWERING_UNIT_OFF_NOW_TEXT), 0);

	// Check if the user adjusted the contrast
	if (g_lcdContrastChanged == YES)
	{
		// Save Unit Config here to prevent constant saving on LCD contrast adjustment
		debug("Saving LCD contrast adjustment\r\n");
		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);
	}

	debug("Adding On/Off Log timestamp\r\n");
	AddOnOffLogTimestamp(OFF);

	// Make sure all open files are closed and data is flushed (unmount)
	f_mount(NULL, "", 0);

	if (powerOffMode == SHUTDOWN_UNIT)
	{
		debug("Powering unit off (shutdown)...\r\n");

		// Shutdown application
		PowerControl(MCU_POWER_LATCH, OFF);
	}
	else
	{
		debug("Powering unit off (reboot)...\r\n");

#if 0 /* Original method doesn't work (especially with debug enabled) */
		Disable_global_interrupt();
		AVR32_WDT.ctrl |= 0x00000001;
#else /* Updated method */
		// Use the External RTC
		SetTimeOfDayAlarmNearFuture(2);

		// Shutdown application
		PowerControl(MCU_POWER_LATCH, OFF);
#endif
	}

	while (1) { /* do nothing */ };
}

///============================================================================
///----------------------------------------------------------------------------
///	Battery charger - MP2651
///----------------------------------------------------------------------------
///============================================================================
/*
MP2651 REGISTER MAP
-------------------------------------------------------------------------------------------------------------
Reg		Address	OTP R/W	Description
-------------------------------------------------------------------------------------------------------------
REG05h	0x05	Yes	R/W	Device Address Setting
REG06h	0x06	Yes	R/W	Input Minimum Voltage Limit Setting
REG08h	0x08	Yes	R/W	Input Current Limit Setting
REG09h	0x09	No	R/W	Output Voltage Setting in Source Mode
REG0Ah	0x0A	No	R/W	Battery Impedance Compensation and Output Current Limit Setting in Source Mode
REG0Bh	0x0B	Yes	R/W	Battery Low Voltage Threshold and Battery Discharge Current Regulation in Source Mode
REG0Ch	0x0C	No	R/W	JEITA Action Setting
REG0Dh	0x0D	Yes	R/W	Temperature Protection Setting
REG0Eh	0x0E	Yes	R/W	Configuration Register 0
REG0Fh	0x0F	Yes	R/W	Configuration Register 1
REG10h	0x10	Yes	R/W	Configuration Register 2
REG11h	0x11	Yes	R/W	Configuration Register 3
REG12h	0x12	Yes	R/W	Configuration Register 4
REG14h	0x14	Yes	R/W	Charge Current Setting
REG15h	0x15	Yes	R/W	Battery Regulation Voltage Setting
REG16h	0x16	No	R	Status and Fault Register 0
REG17h	0x17	No	R	Status and Fault Register 1
REG18h	0x18	No	R/W	INT Mask Setting Register 0
REG19h	0x19	No	R/W	INT Mask Setting Register 1
REG22h	0x22	No	R	Internal DAC Output of the Input Current Limit Setting
REG23h	0x23	No	R	ADC Result of the Input Voltage
REG24h	0x24	No	R	ADC Result of the Input Current
REG25h	0x25	No	R	ADC Result of the Battery Voltage
REG27h	0x27	No	R	ADC Result of the Battery Current
REG28h	0x28	No	R	ADC Result of the NTC Voltage Ratio
REG29h	0x29	No	R	ADC Result of the TS Voltage Ratio
REG2Ah	0x2A	No	R	ADC Result of the Junction Temperature
REG2Bh	0x2B	No	R	ADC Result of the Battery Discharge Current
REG2Ch	0x2C	No	R	ADC Result of the Input Voltage in Discharge Mode
REG2Dh	0x2D	No	R	ADC Result of the Output Current in Discharge Mode
*/

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GetBattChargerRegister(uint8_t registerAddress, uint16_t* registerContents)
{
	// Word data read back in Little endian format
	WriteI2CDevice(MXC_I2C0, I2C_ADDR_BATT_CHARGER, &registerAddress, sizeof(uint8_t), (uint8_t*)registerContents, sizeof(uint16_t));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetBattChargerRegister(uint8_t registerAddress, uint16_t registerContents)
{
	uint8_t writeData[3];

	// Word data written in Little endian format
	writeData[0] = registerAddress;
	writeData[1] = (registerContents & 0xFF);
	writeData[2] = ((registerAddress >> 8) & 0xFF);

	WriteI2CDevice(MXC_I2C0, I2C_ADDR_BATT_CHARGER, writeData, sizeof(writeData), NULL, 0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitBattChargerRegisters(void)
{
	// Note: Some registers have OTP fields but assuming since no reference on how to accomplish this, assuming factory option only

	// Device Address setting
	//	Default watchdog set is enabled, highest 4-bit addr is 0xD
	//	Change watchdog to disable (to prevent defaults from loading back in when triggered)
	SetBattChargerRegister(BATT_CHARGER_DEVICE_ADDRESS_SETTING, 0x00E9);

	// Input Min V limit
	// 	Default Vin minimum limit is 4.56V
	// 	Usb spec is 4.4V-5.25V, for 3.0, 4.55V-5.25V
	//	Change to 4.4V to accomodate older specs
	SetBattChargerRegister(BATT_CHARGER_INPUT_MINIMUM_VOLTAGE_LIMIT_SETTING, 0x0037);

	// Input Current limit
	// 	Default Iin current limit is 500mA
	//	Change Iin current limit to 3000mA for single pack and 5000mA (spec limit) for double pack
	if (GetExpandedBatteryPresenceState() == NO)
	{
		SetBattChargerRegister(BATT_CHARGER_INPUT_CURRENT_LIMIT_SETTING, 0x003C); // Single pack
	}
	else { SetBattChargerRegister(BATT_CHARGER_INPUT_CURRENT_LIMIT_SETTING, 0x0064); } // Double pack

	// Output V setting in Source mode
	// 	Default Vin_src additional V is 0V, config Vin_src by register select, Vin_src is 4.98V
	//	No change from default
	// Todo: Determine if we want to use source mode and if so, set to Vout to 5V, 9V, 12V, 15V, or 20
	SetBattChargerRegister(BATT_CHARGER_OUTPUT_VOLTAGE_SETTING_IN_SOURCE_MODE, 0x00F9);

	// Batt Impedance Comp and Output Current Limit in Source mode
	// 	Default battery impedance is 0 mOhm, max compensaton voltage is 0mV/cell, Iout limit in source mode is 2A
	//	Change Iout limit to 3000mA for single pack and 5500mA (spec limit) for double pack
	if (GetExpandedBatteryPresenceState() == NO)
	{
		SetBattChargerRegister(BATT_CHARGER_BATTERY_IMPEDANCE_COMPENSATION_AND_OUTPUT_CURRENT_LIMIT_SETTING_IN_SOURCE_MODE, 0x003C); // Single pack
	}
	else { SetBattChargerRegister(BATT_CHARGER_BATTERY_IMPEDANCE_COMPENSATION_AND_OUTPUT_CURRENT_LIMIT_SETTING_IN_SOURCE_MODE, 0x006E); } // Double pack

	// Batt low V setting and Batt Discharge Current Reg in Source mode
	// 	Default battery LV procetion is on, pre-charge to CC is 3V/cell, battery low action is INT only, Vbatt low is 3V/cell
	// 	Default batt discharge current regulation in source disabled, batt discharge current in source is 6.4A
	//	Change batt discharge current regulation in source mode to on and 3000mA for single pack and 6000mA for double pack
	if (GetExpandedBatteryPresenceState() == NO)
	{
		SetBattChargerRegister(BATT_CHARGER_BATTERY_LOW_VOLTAGE_THRESHOLD_AND_BATTERY_DISCHARGE_CURRENT_REGULATION_IN_SOURCE_MODE, 0x313C); // Single pack
	}
	else { SetBattChargerRegister(BATT_CHARGER_BATTERY_LOW_VOLTAGE_THRESHOLD_AND_BATTERY_DISCHARGE_CURRENT_REGULATION_IN_SOURCE_MODE, 0x3178); } // Double pack

	// JEITA Action
	//	Default warm protect is only reduce Vbatt_reg, cool protect is only reduce Icc, decrement value for batt full voltage if NTC cool/warm protect occurs is 320mV/cell
	// 	Default scaling value of CC charge current is 1/4 times
	//	No change from default
	// Todo: Determine JETIA actions/settings
	SetBattChargerRegister(BATT_CHARGER_JEITA_ACTION_SETTING, 0x3410);

	// Temp Protection
	//	Default Ext Temp is enabled, OPT action is deliver INT and take TS action, TS OT threshold is 80C, NTC protect is on
	//	Default NTC protect action is deliver INT and take JEITA action, NTC hot thr is 60C, NTC warm thr is 45C, NTC cool thr is 10C, NTC cold thr is 0C
	//	No change from default
	// Todo: Determine thermistor specs and set percentages for temperature thresholds
	SetBattChargerRegister(BATT_CHARGER_TEMPERATURE_PROTECTION_SETTING, 0xB399);

	// Config Reg 0
	//	Default ADC start is disabled, ADC conv is one shot, switcing freq is 600kHz
	//	No change from default
	// Todo: Determine best switching frequency (lower should be slightly more efficient)
	SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_0, 0x0010);

	// Config Reg 1
	//	Default junction temp OT regulation is enabled, juntion temp regulation point is 120C, tricle charge current is 100mA, pre-charge current is 400mA
	//	Default terminaiton current is 200mA
	// Desire pre-charge @ C/10 (6600/10=660 or 13200/10=1320), termination around C/10 to C/20 (going with C/20 yields 6600/20=330 or 13200/20=660)
	if (GetExpandedBatteryPresenceState() == NO)
	{
		SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_1, 0xF277); // Single pack, pre-charge @ 700mA, termination @ 350mA
	}
	else { SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_1, 0xF2DD); } // Double pack, pre-charge @ 1300mA, termination @ 650mA

	// Config Reg 2
	//	Default ACgate not forced, TS/IMON (Pin 7) config is TS, auto recharge thr is -200mV/cell, batt cells in series is 2, Iin sense gain is 10mOhm
	//	Default batt current sense gain is 10mOhm, ACgate driver is enabled
	//	Change batt cells in series to 4
	SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_2, 0x0E40);

	// Config Reg 3
	//	Default OV thr for source Vout is 110%, UV thf for source Vout is 75%, deglitch time for OVP in charge mode is 1us, input UVP thr is 3.2V
	//	Default input OVP thr is 22.4V, batt OVP is enabled
	//	No change from default
	// Todo: Determine input UVP and OVP
	SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_3, 0x60E8);

	// Config Reg 4
	//	Default charge saftey timer is enabled, CC/CV timer is 20hr, saftey timer is doubled, reset WDT is normal, WDT timer is disabled, DC/DC converter is enabled
	//	Default charge termination is enabled, source mode is disabled, register reset is keep current settings, Iin limit loop is enabled, charge mode enabled
	//	No change from default
	// Todo: Determine the CC/CV timer, source mode enable, charge mode
	SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_4, 0x3C53);

	// Charge Current
	//	Default charge current is 2A
	//	Change to 1300mA for single pack (standard charge current per datasheet) and 2650mA for double pack
	// Todo: Determine if standard charge current or max
	if (GetExpandedBatteryPresenceState() == NO)
	{
		SetBattChargerRegister(BATT_CHARGER_CHARGE_CURRENT_SETTING, 0x0680); // Single pack, 1300mA
	}
	else { SetBattChargerRegister(BATT_CHARGER_CHARGE_CURRENT_SETTING, 0x0CC0); } // Double pack, 2650mA

	// Batt Reg V
	//	Default charge full voltage is 8.4V
	// 	Change to 7.3V (max charge voltage)
	SetBattChargerRegister(BATT_CHARGER_BATTERY_REGULATION_VOLTAGE_SETTING, 0x2DA0); // 7.3V

	// Int Mask setting
	//	Default all INT masked
	//	Change all to unmasked
	SetBattChargerRegister(BATT_CHARGER_INT_MASK_SETTING_REGISTER_0, 0x3CFF);

	// Int Mask setting
	//	Default all INT masked
	//	Change all to unmasked
	SetBattChargerRegister(BATT_CHARGER_INT_MASK_SETTING_REGISTER_1, 0x0003);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16_t GetBattChargerStatusReg0(void)
{
	uint16_t registerValue;

	GetBattChargerRegister(BATT_CHARGER_STATUS_AND_FAULT_REGISTER_0, &registerValue);

	return (registerValue);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16_t GetBattChargerStatusReg1(void)
{
	uint16_t registerValue;

	GetBattChargerRegister(BATT_CHARGER_STATUS_AND_FAULT_REGISTER_1, &registerValue);

	return (registerValue);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16_t GetBattChargerInputVoltage(void)
{
	uint16_t registerValue;
	uint16_t result = 0;
	uint8_t i;

	GetBattChargerRegister(BATT_CHARGER_ADC_RESULT_OF_THE_INPUT_VOLTAGE, &registerValue);

	for (i = 0; i < 10; i++)
	{
		if (registerValue & 0x0001)
		{
			result += (20 * (2^i));
			registerValue >>= 1;
		}
	}

	// Result units: mV
	return (result);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16_t GetBattChargerInputCurrent(void)
{
	uint16_t registerValue;
	float result = 0;
	uint8_t i;

	GetBattChargerRegister(BATT_CHARGER_ADC_RESULT_OF_THE_INPUT_CURRENT, &registerValue);

	for (i = 0; i < 10; i++)
	{
		if (registerValue & 0x0001)
		{
			result += (6.25 * (2^i));
			registerValue >>= 1;
		}
	}

	// Result units: mA
	return ((uint16_t)result);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16_t GetBattChargerBatteryVoltagePerCell(void)
{
	uint16_t registerValue;
	uint16_t result = 0;
	uint8_t i;

	GetBattChargerRegister(BATT_CHARGER_ADC_RESULT_OF_THE_BATTERY_VOLTAGE, &registerValue);

	for (i = 0; i < 10; i++)
	{
		if (registerValue & 0x0001)
		{
			result += (5 * (2^i));
			registerValue >>= 1;
		}
	}

	// Result units: mV/cell
	return (result);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16_t GetBattChargerBatteryChargeCurrent(void)
{
	uint16_t registerValue;
	float result = 0;
	uint8_t i;

	GetBattChargerRegister(BATT_CHARGER_ADC_RESULT_OF_THE_BATTERY_CURRENT, &registerValue);

	for (i = 0; i < 10; i++)
	{
		if (registerValue & 0x0001)
		{
			result += (12.5 * (2^i));
			registerValue >>= 1;
		}
	}

	// Result units: mA
	return ((uint16_t)result);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16_t GetBattChargerNTCSenseRatio(void)
{
	uint16_t registerValue;
	float result = 0;
	uint8_t i;

	GetBattChargerRegister(BATT_CHARGER_ADC_RESULT_OF_THE_NTC_VOLTAGE_RATIO, &registerValue);

	for (i = 0; i < 10; i++)
	{
		if (registerValue & 0x0001)
		{
			result += (1 * (2^i));
			registerValue >>= 1;
		}
	}

	// Create ratio and percentage	
	result /= 1024;
	result *= 100;
	// Result units: % of Vntc
	return ((uint16_t)result);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16_t GetBattChargerTSSenseRatio(void)
{
	uint16_t registerValue;
	float result = 0;
	uint8_t i;

	GetBattChargerRegister(BATT_CHARGER_ADC_RESULT_OF_THE_TS_VOLTAGE_RATIO, &registerValue);

	for (i = 0; i < 10; i++)
	{
		if (registerValue & 0x0001)
		{
			result += (1 * (2^i));
			registerValue >>= 1;
		}
	}

	// Create ratio and percentage	
	result /= 1024;
	result *= 100;
	// Result units: % of Vntc
	return ((uint16_t)result);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16_t GetBattChargerJunctionTemperature(void)
{
	uint16_t registerValue;
	float result = 0;
	uint8_t i;

	GetBattChargerRegister(BATT_CHARGER_ADC_RESULT_OF_THE_JUNCTION_TEMPERATURE, &registerValue);

	for (i = 0; i < 10; i++)
	{
		if (registerValue & 0x0001)
		{
			result += (1 * (2^i));
			registerValue >>= 1;
		}
	}

	// Temperature conversion equation
	result = (314 - (0.5703 * result));
	// Result units: Assume temp in C and not F, unconfirmed
	return ((uint16_t)result);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16_t GetBattChargerBatteryDischargeCurrent(void)
{
	uint16_t registerValue;
	float result = 0;
	uint8_t i;

	GetBattChargerRegister(BATT_CHARGER_ADC_RESULT_OF_THE_BATTERY_DISCHARGE_CURRENT, &registerValue);

	for (i = 0; i < 10; i++)
	{
		if (registerValue & 0x0001)
		{
			result += (12.5 * (2^i));
			registerValue >>= 1;
		}
	}

	// Result units: mA
	return ((uint16_t)result);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16_t GetBattChargerInputVoltageInDischargeMode(void)
{
	uint16_t registerValue;
	uint16_t result = 0;
	uint8_t i;

	GetBattChargerRegister(BATT_CHARGER_ADC_RESULT_OF_THE_INPUT_VOLTAGE_IN_DISCHARGE_MODE, &registerValue);

	for (i = 0; i < 10; i++)
	{
		if (registerValue & 0x0001)
		{
			result += (20 * (2^i));
			registerValue >>= 1;
		}
	}

	// Result units: mV
	return (result);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16_t GetBattChargerOutputCurrentInDischargeMode(void)
{
	uint16_t registerValue;
	float result = 0;
	uint8_t i;

	GetBattChargerRegister(BATT_CHARGER_ADC_RESULT_OF_THE_OUTPUT_CURRENT_IN_DISCHARGE_MODE, &registerValue);

	for (i = 0; i < 10; i++)
	{
		if (registerValue & 0x0001)
		{
			result += (6.25 * (2^i));
			registerValue >>= 1;
		}
	}

	// Result units: mA
	return ((uint16_t)result);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestBatteryCharger(void)
{
    debug("Battery Charger: Test device access...\r\n");

	debug("Battery Charger: Init to disable internal watchdog to keep in Host mode (prevents defaults on timeout)\r\n");
	InitBattChargerRegisters();

	debug("Battery Charger: Status reg 0 is 0x%x\r\n", GetBattChargerStatusReg0()); // Result units: mV
	debug("Battery Charger: Status reg 1 is 0x%x\r\n", GetBattChargerStatusReg1()); // Result units: mV

	debug("Battery Charger: Input voltage is %u (mV)\r\n", GetBattChargerInputVoltage()); // Result units: mV
	debug("Battery Charger: Input current is %u (mA)\r\n", GetBattChargerInputCurrent()); // Result units: mA
	debug("Battery Charger: Batt voltage per cell is %u (mV/cell)\r\n", GetBattChargerBatteryVoltagePerCell()); // Result units: mV/cell
	debug("Battery Charger: Batt current is %u (mA)\r\n", GetBattChargerBatteryChargeCurrent()); // Result units: mA
	debug("Battery Charger: NTSC sense ratio is %u (%% of Vntc)\r\n", GetBattChargerNTCSenseRatio()); // Result units: % of Vntc
	debug("Battery Charger: TS sense ration is %u (%% of Vntc)\r\n", GetBattChargerTSSenseRatio()); // Result units: % of Vntc
	debug("Battery Charger: Junciton temp is %u (believe in degrees C)\r\n", GetBattChargerJunctionTemperature()); // Result units: Assume temp in C and not F, unconfirmed
	debug("Battery Charger: Batt discharge current is %u (mA)\r\n", GetBattChargerBatteryDischargeCurrent()); // Result units: mA
	debug("Battery Charger: Input voltage in discharge mode is %u (mV)\r\n", GetBattChargerInputVoltageInDischargeMode()); // Result units: mV
	debug("Battery Charger: Output current in discharge mode is %u (mA)\r\n", GetBattChargerOutputCurrentInDischargeMode()); // Result units: mA
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void BatteryChargerInit(void)
{
	// ACOK (pin 11 of part) indicates when the input power supply (VBUS charging) is in charge mode
	// Note: ACOK prevents VCC from V_batt from powering the VBUS present line (Power logic latch)

	// Note: V_adp under voltage lockout threshold is 2.4-2.8V (typically 2.6V), over volatge ~24V
	// Note: V_batt under voltage lockout threshold is 2.5-2.7V (typically 2.6V)

	// Setup the following Battery charger registers
	/*
		Device Address Setting
		Input Minimum Voltage Limit Setting
		Input Current Limit Setting
		Output Voltage Setting in Source Mode
		Battery Impedance Compensation and Output Current Limit Setting in Source Mode
		Battery Low Voltage Threshold and Battery Discharge Current Regulation in Source Mode
		JEITA Action Setting
		Temperature Protection Setting
		Configuration Register 0, 1, 2, 3, 4
		Charge Current Setting
		Battery Regulation Voltage Setting
		INT Mask Setting Register 0, 1
	*/
	InitBattChargerRegisters();

	// Set Continuous mode for ADC_CONV
	SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_0, 0x0090);
}

///============================================================================
///----------------------------------------------------------------------------
///	Fuel Guage - LTC2944
///----------------------------------------------------------------------------
///============================================================================
/*
ADDR NAME REGISTER DESCRIPTION R/W DEFAULT
00h	A	Status	R	See Table 2
01h	B	Control	R/W	3Ch
02h	C	Accumulated Charge MSB	R/W	7Fh
03h	D	Accumulated Charge LSB	R/W	FFh
04h	E	Charge Threshold High MSB	R/W	FFh
05h	F	Charge Threshold High LSB	R/W	FFh
06h	G	Charge Threshold Low MSB	R/W	00h
07h	H	Charge Threshold Low LSB	R/W	00h
08h	I	Voltage MSB	R	00h
09h	J	Voltage LSB	R	00h
0Ah	K	Voltage Threshold High MSB	R/W	FFh
0Bh	L	Voltage Threshold High LSB	R/W	FFh
0Ch	M	Voltage Threshold Low MSB	R/W	00h
0Dh	N	Voltage Threshold Low LSB	R/W	00h
0Eh	O	Current MSB	R	00h
0Fh	P	Current LSB	R	00h
10h	Q	Current Threshold High MSB	R/W	FFh
11h	R	Current Threshold High LSB	R/W	FFh
12h	S	Current Threshold Low MSB	R/W	00h
13h	T	Current Threshold Low LSB	R/W	00h
14h	U	Temperature MSB	R	00h
15h	V	Temperature LSB	R	00h
16h	W	Temperature Threshold High	R/W	FFh
17h	X	Temperature Threshold Low	R/W	00h
*/
/*
 * I2C client/driver for the Linear Technology LTC2941, LTC2942, LTC2943 and LTC2944 Battery Gas Gauge IC
 * Copyright (C) 2014 Topic Embedded Systems
 * Author: Auryn Verwegen
 * Author: Mike Looijmans
 */

#define I16_MSB(x)			((x >> 8) & 0xFF)
#define I16_LSB(x)			(x & 0xFF)

//#define LTC294X_WORK_DELAY		10	/* Update delay in seconds */

#define LTC294X_MAX_VALUE		0xFFFF
#define LTC294X_MID_SUPPLY		0x7FFF

enum ltc294x_reg {
	LTC294X_REG_STATUS					= 0x00, // Reg A, as named in datasheet
	LTC294X_REG_CONTROL					= 0x01, // Reg B
	LTC294X_REG_ACC_CHARGE_MSB			= 0x02, // Reg C
	LTC294X_REG_ACC_CHARGE_LSB			= 0x03, // Reg D
	LTC294X_REG_CHARGE_THR_HIGH_MSB		= 0x04, // Reg E
	LTC294X_REG_CHARGE_THR_HIGH_LSB		= 0x05, // Reg F
	LTC294X_REG_CHARGE_THR_LOW_MSB		= 0x06, // Reg G
	LTC294X_REG_CHARGE_THR_LOW_LSB		= 0x07, // Reg H
	LTC294X_REG_VOLTAGE_MSB				= 0x08, // Reg I
	LTC294X_REG_VOLTAGE_LSB				= 0x09, // Reg J
	LTC294X_REG_VOLTAGE_THR_HIGH_MSB 	= 0x0A, // Reg K
	LTC294X_REG_VOLTAGE_THR_HIGH_LSB 	= 0x0B, // Reg L
	LTC294X_REG_VOLTAGE_THR_LOW_MSB 	= 0x0C, // Reg M
	LTC294X_REG_VOLTAGE_THR_LOW_LSB 	= 0x0D, // Reg N
	LTC2943_REG_CURRENT_MSB				= 0x0E, // Reg O
	LTC2943_REG_CURRENT_LSB				= 0x0F, // Reg P
	LTC2943_REG_CURRENT_THR_HIGH_MSB	= 0x10, // Reg Q
	LTC2943_REG_CURRENT_THR_HIGH_LSB	= 0x11, // Reg R
	LTC2943_REG_CURRENT_THR_LOW_MSB		= 0x12, // Reg S
	LTC2943_REG_CURRENT_THR_LOW_LSB		= 0x13, // Reg T
	LTC2943_REG_TEMPERATURE_MSB			= 0x14, // Reg U
	LTC2943_REG_TEMPERATURE_LSB			= 0x15, // Reg V
	LTC2943_REG_TEMPERATURE_THR_HIGH 	= 0x16, // Reg W
	LTC2943_REG_TEMPERATURE_THR_LOW 	= 0x17 	// Reg X
};

#define LTC2941_REG_STATUS_CHIP_ID	(1 << 7)

#define LTC2942_REG_CONTROL_MODE_AUTO	((1 << 7) | (1 << 6))
#define LTC2943_REG_CONTROL_MODE_SCAN	(1 << 7)
#define LTC294X_REG_CONTROL_PRESCALER_MASK	((1 << 5) | (1 << 4) | (1 << 3))
#define LTC294X_REG_CONTROL_SHUTDOWN_MASK	((1 << 0))
#define LTC294X_REG_CONTROL_PRESCALER_SET(x)	((x << 3) & LTC294X_REG_CONTROL_PRESCALER_MASK)
#define LTC294X_REG_CONTROL_ALCC_CONFIG_DISABLED	0
#define LTC294X_REG_CONTROL_ALCC_CONFIG_ALERT_MODE	((0x10) << 1)
#define LTC294X_REG_CONTROL_ADC_DISABLE(x)	((x) & ~((1 << 7) | (1 << 6)))

#define LTC294X_PRESCALER_1		0x0
#define LTC294X_PRESCALER_4		0x1
#define LTC294X_PRESCALER_16	0x2
#define LTC294X_PRESCALER_64	0x3
#define LTC294X_PRESCALER_256	0x4
#define LTC294X_PRESCALER_1024	0x5
#define LTC294X_PRESCALER_4096	0x6

#define LTC294X_M_1		1
#define LTC294X_M_4		4
#define LTC294X_M_16	16
#define LTC294X_M_64	64
#define LTC294X_M_256	256
#define LTC294X_M_1024	1024
#define LTC294X_M_4096	4096
#define LTC294X_MAX_PRESCALER	4096

typedef struct {
	int charge; // Last charge register content
	int r_sense; // mOhm
	int prescaler; // M value between 1 and 4096
	int Qlsb; // nAh
} ltc294x_info;
ltc294x_info ltc2944_device;

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
inline int convert_bin_to_uAh(int Qlsb, int Q)
{
	return (((Q * (Qlsb / 10))) / 100);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
inline int convert_uAh_to_bin(int Qlsb, int uAh)
{
	int Q;

	Q = (uAh * 100) / (Qlsb/10);
	return ((Q < LTC294X_MAX_VALUE) ? Q : LTC294X_MAX_VALUE);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int ltc294x_read_regs(enum ltc294x_reg reg, uint8_t *buf, int bufSize)
{
	return (WriteI2CDevice(MXC_I2C1, I2C_ADDR_FUEL_GUAGE, &reg, sizeof(uint8_t), buf, bufSize));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int ltc294x_write_regs(enum ltc294x_reg reg, const uint8_t *buf, int bufSize)
{
	g_spareBuffer[0] = reg;
	memcpy(&g_spareBuffer[1], buf, bufSize);

	return (WriteI2CDevice(MXC_I2C1, I2C_ADDR_FUEL_GUAGE, &reg, (bufSize + 1), NULL, 0));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t ltc294x_get_status(void)
{
	uint8_t statusReg;
	
	WriteI2CDevice(MXC_I2C1, I2C_ADDR_FUEL_GUAGE, LTC294X_REG_STATUS, sizeof(uint8_t), &statusReg, sizeof(uint8_t));

	return (statusReg);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int ltc294x_reset(int prescaler_exp)
{
	int ret;
	uint8_t value;
	uint8_t control;

	// Read status and control registers
	ret = ltc294x_read_regs(LTC294X_REG_CONTROL, &value, 1);
	if (ret < 0) { return ret; }

	control = LTC294X_REG_CONTROL_PRESCALER_SET(prescaler_exp) | LTC294X_REG_CONTROL_ALCC_CONFIG_ALERT_MODE;

	//control |= LTC2942_REG_CONTROL_MODE_AUTO; // Continuous conversions (more power)
	control |= LTC2943_REG_CONTROL_MODE_SCAN; // Conversions every 10 sec (less power)

	if (value != control)
	{
		ret = ltc294x_write_regs(LTC294X_REG_CONTROL, &control, 1);
		if (ret < 0) { return ret; }
	}

	return (0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int ltc294x_read_charge_register(enum ltc294x_reg reg)
 {
	int ret;
	uint8_t datar[2];

	ret = ltc294x_read_regs(reg, &datar[0], 2);

	if (ret < 0) { return ret; }
	return ((datar[0] << 8) + datar[1]);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int ltc294x_get_charge(int Qlsb, enum ltc294x_reg reg, int* val)
{
	int value = ltc294x_read_charge_register(reg);

	if (value < 0) { return value; }

	/* When r_sense < 0, this counts up when the battery discharges */
	if (Qlsb < 0) { value -= 0xFFFF; }

	*val = convert_bin_to_uAh(Qlsb, value);
	return (0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* reference function */
int ltc294x_set_charge_now_uAh(ltc294x_info* info, int val)
{
	int ret;
	uint8_t dataw[2];
	uint8_t ctrl_reg;
	int32_t value;

	value = convert_uAh_to_bin(info, val);
	// Direction depends on how sense +/- were connected
	if (info->Qlsb < 0) { value += 0xFFFF; }

	if ((value < 0) || (value > 0xFFFF)) { return E_INVALID; } // Input validation

	// Read control register
	ret = ltc294x_read_regs(LTC294X_REG_CONTROL, &ctrl_reg, 1);
	if (ret < 0) { return ret; }

	// Disable analog section
	ctrl_reg |= LTC294X_REG_CONTROL_SHUTDOWN_MASK;
	ret = ltc294x_write_regs(LTC294X_REG_CONTROL, &ctrl_reg, 1);
	if (ret < 0) { return ret; }

	// Set new charge value
	dataw[0] = I16_MSB(value);
	dataw[1] = I16_LSB(value);
	ret = ltc294x_write_regs(LTC294X_REG_ACC_CHARGE_MSB, &dataw[0], 2);
	if (ret < 0) { goto error_exit; }
	// Enable analog section

error_exit:
	ctrl_reg &= ~LTC294X_REG_CONTROL_SHUTDOWN_MASK;
	ret = ltc294x_write_regs(LTC294X_REG_CONTROL, &ctrl_reg, 1);

	return ((ret < 0) ? ret : 0);
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int ltc294x_set_charge_now(int Qlsb, int val)
{
	int ret;
	uint8_t dataw[2];
	uint8_t ctrl_reg;
	int32_t value;

	value = (val / Qlsb);
	if ((value < 0) || (value > 0xFFFF)) { return E_INVALID; } // Input validation

	// Read control register
	ret = ltc294x_read_regs(LTC294X_REG_CONTROL, &ctrl_reg, 1);
	if (ret < 0) { return ret; }

	// Disable analog section
	ctrl_reg |= LTC294X_REG_CONTROL_SHUTDOWN_MASK;
	ret = ltc294x_write_regs(LTC294X_REG_CONTROL, &ctrl_reg, 1);
	if (ret < 0) { return ret; }

	// Set new charge value
	dataw[0] = I16_MSB(value);
	dataw[1] = I16_LSB(value);
	ret = ltc294x_write_regs(LTC294X_REG_ACC_CHARGE_MSB, &dataw[0], 2);

	// Enable analog section
	ctrl_reg &= ~LTC294X_REG_CONTROL_SHUTDOWN_MASK;
	ret = ltc294x_write_regs(LTC294X_REG_CONTROL, &ctrl_reg, 1);

	return ((ret < 0) ? ret : 0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* reference function, bug discovered; write should be 4 */
int ltc294x_set_charge_thr_uAh(ltc294x_info* info, int thrHigh, int thrLow)
{
	uint8_t dataWrite[4];
	int32_t chargeThrHigh;
	int32_t chargeThrLow;

	// Todo: Verify conversion and values
	chargeThrHigh = convert_uAh_to_bin(info, thrHigh);
	// Direction depends on how sense +/- were connected
	if (info->Qlsb < 0) { chargeThrHigh += 0xFFFF; }
	if ((chargeThrHigh < 0) || (chargeThrHigh > 0xFFFF)) { return E_INVALID; } // input validation

	chargeThrLow = convert_uAh_to_bin(info, thrLow);
	// Direction depends on how sense +/- were connected
	if (info->Qlsb < 0) { chargeThrLow += 0xFFFF; }
	if ((chargeThrLow < 0) || (chargeThrLow > 0xFFFF)) { return E_INVALID; } // input validation

	// Set new charge value
	dataWrite[0] = I16_MSB(chargeThrHigh);
	dataWrite[1] = I16_LSB(chargeThrHigh);
	dataWrite[2] = I16_MSB(chargeThrLow);
	dataWrite[3] = I16_LSB(chargeThrLow);

	return (ltc294x_write_regs(LTC294X_REG_CHARGE_THR_HIGH_MSB, &dataWrite[0], 2));
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int ltc294x_set_charge_thr(int Qlsb, int thrHigh, int thrLow)
{
	uint8_t dataWrite[4];
	int32_t chargeThrHigh;
	int32_t chargeThrLow;

	// Set the charge ADC value by dividing by the value represented by 1 bit
	chargeThrHigh = (thrHigh / Qlsb);
	if ((chargeThrHigh < 0) || (chargeThrHigh > 0xFFFF)) { return E_INVALID; } // input validation

	// Set the charge ADC value by dividing by the value represented by 1 bit
	chargeThrLow = (thrLow / Qlsb);
	if ((chargeThrLow < 0) || (chargeThrLow > 0xFFFF)) { return E_INVALID; } // input validation

	// Set new charge value
	dataWrite[0] = I16_MSB(chargeThrHigh);
	dataWrite[1] = I16_LSB(chargeThrHigh);
	dataWrite[2] = I16_MSB(chargeThrLow);
	dataWrite[3] = I16_LSB(chargeThrLow);

	return (ltc294x_write_regs(LTC294X_REG_CHARGE_THR_HIGH_MSB, &dataWrite[0], 4));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int ltc294x_get_charge_counter(int Qlsb, int* val)
{
	int value = ltc294x_read_charge_register(LTC294X_REG_ACC_CHARGE_MSB);

	if (value < 0) { return value; }

	value -= LTC294X_MID_SUPPLY;
	*val = convert_bin_to_uAh(Qlsb, value);

	return (0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int ltc294x_get_voltage(int* val)
{
	int ret;
	uint8_t datar[2];
	uint64_t voltage; // Need storage above uint32 size if calc done in mV

	ret = ltc294x_read_regs(LTC294X_REG_VOLTAGE_MSB, &datar[0], 2);
	voltage = ((datar[0] << 8) | datar[1]);

	voltage *= (70800 / 0xFFFF); // units in mV

	*val = (int)voltage;
	return (ret);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int ltc294x_set_voltage_thr(int vHigh, int vLow)
{
	uint8_t dataWrite[4];
	uint32_t vThrHigh;
	uint32_t vThrLow;

	vThrHigh = ((vHigh * 0xFFFF) / 70800); // vHigh in mV
	vThrLow = ((vLow * 0xFFFF) / 70800); // vLow in mV

	// Set new charge value
	dataWrite[0] = I16_MSB(vThrHigh);
	dataWrite[1] = I16_LSB(vThrHigh);
	dataWrite[2] = I16_MSB(vThrLow);
	dataWrite[3] = I16_LSB(vThrLow);

	return (ltc294x_write_regs(LTC294X_REG_VOLTAGE_THR_HIGH_MSB, &dataWrite[0], sizeof(dataWrite)));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int ltc294x_get_current(int r_sense, int* val)
{
	int ret;
	uint8_t datar[2];
	int32_t value;

	ret = ltc294x_read_regs(LTC2943_REG_CURRENT_MSB, &datar[0], 2);
	value = ((datar[0] << 8) | datar[1]);
	value -= 0x7FFF;
	value *= 64000;

	// Value is in range -32k..+32k, r_sense is usually 10..50 mOhm, the formula below keeps everything in s32 range while preserving enough digits
	*val = (1000 * (value / (r_sense * 0x7FFF))); // Units in uA

	return (ret);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int ltc294x_set_current_thr_same(int r_sense, int currThr)
{
	uint8_t dataWrite[4];
	int32_t currThrHigh;
	int32_t currThrLow;

	// Current in mA converted to A for equation
	currThrHigh = ((((currThr / 1000) * r_sense / 64) * 0x7FFF) + 0x7FFF);
	currThrLow = ((((-currThr / 1000) * r_sense / 64) * 0x7FFF) + 0x7FFF);

	// Set new charge value
	dataWrite[0] = I16_MSB(currThrHigh);
	dataWrite[1] = I16_LSB(currThrHigh);
	dataWrite[2] = I16_MSB(currThrLow);
	dataWrite[3] = I16_LSB(currThrLow);

	return (ltc294x_write_regs(LTC2943_REG_CURRENT_THR_HIGH_MSB, &dataWrite[0], sizeof(dataWrite)));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int ltc294x_set_current_thr(int r_sense, int currThrCharging, int currThrDischarging)
{
	uint8_t dataWrite[4];
	int32_t currThrHigh;
	int32_t currThrLow;

	// Current in mA converted to A for equation
	currThrHigh = ((((currThrCharging / 1000) * r_sense / 64) * 0x7FFF) + 0x7FFF);
	currThrLow = ((((-currThrDischarging / 1000) * r_sense / 64) * 0x7FFF) + 0x7FFF);

	// Set new charge value
	dataWrite[0] = I16_MSB(currThrHigh);
	dataWrite[1] = I16_LSB(currThrHigh);
	dataWrite[2] = I16_MSB(currThrLow);
	dataWrite[3] = I16_LSB(currThrLow);

	return (ltc294x_write_regs(LTC2943_REG_CURRENT_THR_HIGH_MSB, &dataWrite[0], sizeof(dataWrite)));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int ltc294x_get_temperature(int* val)
{
	int ret;
	uint8_t datar[2];
	uint32_t value;

	ret = ltc294x_read_regs(LTC2943_REG_TEMPERATURE_MSB, &datar[0], 2);
	value = ((datar[0] << 8) | datar[1]);

	// Convert to degree Celsius
	*val = (((value * 510) / 0xFFFF) - 273.15); // C = K - 273.15

	return (ret);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int ltc294x_get_temperature_farenheit(int* val)
{
	int ret;
	uint8_t datar[2];
	uint32_t value;

	ret = ltc294x_read_regs(LTC2943_REG_TEMPERATURE_MSB, &datar[0], 2);
	value = ((datar[0] << 8) | datar[1]);

	// Convert to F, F = (C * (9/5)) + 32
	*val = (((((value * 510) / 0xFFFF) - 273.15) * (9 / 5)) + 32);

	return (ret);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int ltc294x_set_temp_thr(int tempHigh, int tempLow)
{
	uint8_t dataWrite[2];
	uint32_t tempHighThr;
	uint32_t tempLowThr;

	tempHighThr = ((tempHigh + 273.15) * 0xFFFF / 510);
	tempLowThr = ((tempHigh + 273.15) * 0xFFFF / 510);

	// Set new charge value (only 11 bits in temp mode, can only set the top 8 MSB bits)
	dataWrite[0] = I16_MSB(tempHighThr);
	dataWrite[1] = I16_MSB(tempLowThr);

	return (ltc294x_write_regs(LTC2943_REG_CURRENT_THR_HIGH_MSB, &dataWrite[0], sizeof(dataWrite)));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int ltc294x_set_temp_thr_farenheit(int tempHigh, int tempLow)
{
	uint8_t dataWrite[2];
	uint32_t tempHighThr;
	uint32_t tempLowThr;

	tempHighThr = (((((tempHigh - 32) * 5) / 9) + 273.15) * 0xFFFF / 510);
	tempLowThr = (((((tempLow - 32) * 5) / 9) + 273.15) * 0xFFFF / 510);

	// Set new charge value (only 11 bits in temp mode, can only set the top 8 MSB bits)
	dataWrite[0] = I16_MSB(tempHighThr);
	dataWrite[1] = I16_MSB(tempLowThr);

	return (ltc294x_write_regs(LTC2943_REG_CURRENT_THR_HIGH_MSB, &dataWrite[0], sizeof(dataWrite)));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ltc294x_update(ltc294x_info* info)
{
	int charge = ltc294x_read_charge_register(LTC294X_REG_ACC_CHARGE_MSB);

	if (charge != info->charge)
	{
		info->charge = charge;
		
		// Power supply changed
		// Action?
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int ltc294x_i2c_probe(void)
{
	// Note: Unfinished and non-functional at the moment

	//struct power_supply_config psy_cfg = {};
	//ltc294x_info* info;
	//struct device_node *np;
	int ret;
	uint32_t prescaler_exp;
	int32_t r_sense;
	//uint8_t status;

	//info = devm_kzalloc(sizeof(*info), GFP_KERNEL);
	//if (info == NULL) { return -ENOMEM; }

	//np = of_node_get();

	//info->supply_desc.name = np->name;

	// r_sense can be negative, when sense+ is connected to the battery instead of the sense-, this results in reversed measurements
	// Set r_sense value, which should be less than or equal to 50mV / I(max)
	// r_sense is usually 10..50 mOhm, but probably between 150 mOhm and 500 mOhm
	r_sense = 10; // 10 mOhm showing on schematic as Rsense

	ltc2944_device.r_sense = r_sense;

	prescaler_exp = LTC294X_PRESCALER_256;

	ltc2944_device.Qlsb = ((340 * 50) / r_sense) * (LTC294X_M_256 / LTC294X_MAX_PRESCALER) * 1000; // nAh units, .340 scaled up to uA and * 1000 to scale up to nA

	//info->supply_desc.external_power_changed = NULL;

	//psy_cfg.drv_data = info;

	//ret = devm_delayed_work_autocancel(&info->work, ltc294x_work);
	//if (ret) { return ret; }

	ret = ltc294x_reset(prescaler_exp);
	if (ret < 0) { return (ret); }

	//info->supply = devm_power_supply_register(&info->supply_desc, &psy_cfg);

	//schedule_delayed_work(&info->work, LTC294X_WORK_DELAY * HZ);

	return 0;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ltc294x_i2c_shutdown(void)
{
	int ret;
	uint8_t value;
	uint8_t control;

	// Read control register
	ret = ltc294x_read_regs(LTC294X_REG_CONTROL, &value, 1);
	if (ret < 0) { return; }

	// Disable continuous ADC conversion as this drains the battery
	control = LTC294X_REG_CONTROL_ADC_DISABLE(value);
	if (control != value) { ltc294x_write_regs(LTC294X_REG_CONTROL, &control, 1); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestFuelGauge(void)
{
	ltc294x_info info;
	int val;

	info.r_sense = 10;
	if (GetExpandedBatteryPresenceState() == NO) { info.prescaler = LTC294X_PRESCALER_256; } // Single pack
	else { info.prescaler = LTC294X_PRESCALER_1024; } // Double pack
	info.Qlsb = (((340 * 1000) * 50) / info.r_sense) * (LTC294X_M_256 / LTC294X_MAX_PRESCALER); // nAh units, .340 scaled up to uA and * 1000 to scale up to nA

    debug("Fuel Gauge: Test device access...\r\n");

	debug("Fuel Gauge: Enabling conversions\r\n");
	ltc294x_reset(info.prescaler);

	debug("Fuel Gauge: Status reg is 0x%x\r\n", ltc294x_get_status());

	ltc294x_get_charge(info.Qlsb, LTC294X_REG_ACC_CHARGE_MSB, &val);
	debug("Fuel Gauge: Accumulated charge is %d (uA)\r\n", val);

	ltc294x_get_voltage(&val);
	debug("Fuel Gauge: Voltage is %d (mV)\r\n", val);

	ltc294x_get_current(info.r_sense, &val);
	debug("Fuel Gauge: Current is %d (uA)\r\n", val);

	ltc294x_get_temperature_farenheit(&val);
	debug("Fuel Gauge: Temperature is %d (degrees F)\r\n", val);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void FuelGaugeInit(void)
{
	/*
		----------------------------------------------------------------------
		Battery: Single Pack
		----------------------------------------------------------------------
		Capacity: 6600mAh
		Nominal voltage: 6.4V
		Max input voltage: 9V
		Max charge voltage: 7.3V
		Standard charge current: 1320mA
		Max charge current: 3000mA
		Min discharge voltage: 4.0+/-0.1V
		Standard discharge current: 1320mA
		Max continuous discharge: 3000mA
		Operating temp, charging: 0C - 45C
		Operating temp, discharging: -20C - 60C

		----------------------------------------------------------------------
		Battery: Double Pack
		----------------------------------------------------------------------
		Capacity: 13200mAh
		Nominal voltage: 6.4V
		Max input voltage: 9V
		Max charge voltage: 7.3V
		Standard charge current: 2640mA
		Max charge current: 6000mA
		Min discharge voltage: 4.0+/-0.1V
		Standard discharge current: 2640mA
		Max continuous discharge: 6000mA

		----------------------------------------------------------------------
		Fuel Gauge parameters
		----------------------------------------------------------------------
		Rsense = 10mOhm

		Rsense <= 50mV / Imax
		Rsense <= 0.01666
		0.010 <= 0.01666

		RSENSE <= 0.340mAh * 2^16 / QBAT * 50mOhm
		RSENSE <= 168.80
		0.010 <= 168.80 (single pack), 84.40 (double pack)

		Minimum Qlsb > Qbat / 2^16
		Minimum Qlsb > 0.10070 (single pack), 0.20141 (double pack)

		qLSB = 0.340mAh * 50mOhm / RSENSE
		qLSB = 0.340mAh * 50mOhm / 10mOhm
		qLSB = 1.7 with max prescaler (4096)

		Qlsb with prescaler 4096 = 1.7
		Qlsb with prescaler 1024 = 0.425
		Qlsb with prescaler 256 = 0.10625
		Qlsb with prescaler 64 = 0.0265
		Qlsb with prescaler 4 = 0.00166
		Qlsb with prescaler 1 = 0.000415

		M >= 4096 * QBAT / 2^16 / 0.340mAh * RSENSE / 50mOhm
		M >= 242.647 (single pack), 485.294 (double pack)

		With prescaler 256, max ADC capacity reading = 6963.2 (below our 6600 capacity, single pack)
		With prescaler 1024, max ADC capacity reading = 27852.8 (below our 13200 capacity, double pack)

		----------------------------------------------------------------------
		Current Threshold
		----------------------------------------------------------------------
		Current reading max = 6600mA (single pack)
		3000mA / 6600mA * 32768 counts = 14,894.54
		Current Threshold High = 32767 + 14,895 = 47662 (0xBA2E)
		Current Threshold Low = 32767 - 14,895 = 17872 (0x45D0)

		----------------------------------------------------------------------
		Accumulated Charge Threshold
		----------------------------------------------------------------------
		6600 / 0.10625 (Qlsb with M=256) = 62117.64 counts (single pack)
		13200 / 0.425 (Qlsb with M=1024) = 31058.82 counts (double pack)

		Note: Before writing to the accumulated charge registers, the analog section should be temporarily shut down by setting B[0] to 1
	*/

	ltc2944_device.r_sense = 10; // 10 mOhm showing on schematic as Rsense

	if (GetExpandedBatteryPresenceState() == NO)
	{
		ltc2944_device.prescaler = LTC294X_PRESCALER_256; // Single pack
	}
	else { ltc2944_device.prescaler = LTC294X_PRESCALER_1024; } // Double pack

	ltc2944_device.Qlsb = (((340 * 1000) * 50) / ltc2944_device.r_sense) * (LTC294X_M_256 / LTC294X_MAX_PRESCALER); // nAh units, .340 scaled up to uA and * 1000 to scale up to nA

	// Reset the device with desired prescaler, ADC mode will be set to Scan and ALCC config set to Alert mode
	ltc294x_reset(ltc2944_device.prescaler);

#if 0 /* Todo: Verify how Charge operates */
	// Set the charge based on expected near full charge capacity (95%), function handles disabling the analog section for setting the charge and re-enabling when done
	if (GetExpandedBatteryPresenceState() == NO)
	{
		ltc294x_set_charge_now(&ltc2944_device, (6600 * 0.95)); // Single pack
	}
	else { ltc294x_set_charge_now(&ltc2944_device, (13200 * 0.95)); } // Double pack

	// Set the Charge Thresold High and Low
	if (GetExpandedBatteryPresenceState() == NO)
	{
		ltc294x_set_charge_thr(&ltc2944_device, 6600, 0); // Single pack
	}
	else { ltc294x_set_charge_thr(&ltc2944_device, 13200, 0); } // Double pack
#endif

	// Set the Voltage Thresold High and Low based on 7300mV max, 4000mV min
	ltc294x_set_voltage_thr(7300, 4000);

	// Set the Current Thresold High and Low based on 3000mA
	if (GetExpandedBatteryPresenceState() == NO)
	{
		// Todo: Determine if charge/discharge current should be Standard 1320mA or Max continuous 3000mA
		ltc294x_set_current_thr(ltc2944_device.r_sense, 3000, 3000); // Single pack
	}
	// Todo: Determine if charge/discharge current should be Standard 2640mA or Max continuous 6000mA
	else { ltc294x_set_current_thr(ltc2944_device.r_sense, 6000, 6000); } // Double pack

	// Set thresholds for temperature (discharge range)
	ltc294x_set_temp_thr(60, -20);
}
