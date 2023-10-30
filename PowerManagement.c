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
#include "M23018.h"
#include "lcd.h"

#include "mxc_errors.h"
#include "i2c.h"
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
		case POWER_OFF:
		//----------------------------------------------------------------------------
			//debug("Power Off: %s.\r\n", mode == ON ? "On" : "Off");
			if (mode == ON)
			{
#if 0 /* old hw */
				state = ReadMcp23018(IO_ADDRESS_KPD, GPIOA);
				state |= 0x40;
				WriteMcp23018(IO_ADDRESS_KPD, OLATA, state);
				s_powerManagement |= POWER_OFF_BIT;
#endif
			}			
			else // (mode == OFF)
			{
				// Unit is actually off
			}
			break;

		//----------------------------------------------------------------------------
		case POWER_OFF_PROTECTION_ENABLE:
		//----------------------------------------------------------------------------
			//debug("Power Shutdown Enable: %s.\r\n", mode == ON ? "On" : "Off");
			if (mode == ON)
			{
#if 0 /* old hw */
				state = ReadMcp23018(IO_ADDRESS_KPD, GPIOA);
				state |= 0x80;
				WriteMcp23018(IO_ADDRESS_KPD, OLATA, state);
				s_powerManagement |= POWER_OFF_PROTECTION_ENABLE_BIT;
#endif
			} 
			else // (mode == OFF)
			{
#if 0 /* old hw */
				state = ReadMcp23018(IO_ADDRESS_KPD, GPIOA);
				state &= ~0x80;
				WriteMcp23018(IO_ADDRESS_KPD, OLATA, state);
				s_powerManagement &= ~(POWER_OFF_PROTECTION_ENABLE_BIT);
#endif
			}
			break;

		//----------------------------------------------------------------------------
		case ALARM_1_ENABLE: // Active high control
		//----------------------------------------------------------------------------
			//debug("Alarm 1 Enable: %s.\r\n", mode == ON ? "On" : "Off");
			if (mode == ON)
			{
#if 0 /* old hw */
				gpio_set_gpio_pin(AVR32_PIN_PB06);
#endif
			}
			else // (mode == OFF)
			{
#if 0 /* old hw */
				gpio_clr_gpio_pin(AVR32_PIN_PB06);
#endif
			}
			break;

		//----------------------------------------------------------------------------
		case ALARM_2_ENABLE: // Active high control
		//----------------------------------------------------------------------------
			//debug("Alarm 2 Enable: %s.\r\n", mode == ON ? "On" : "Off");
			if (mode == ON)
			{
#if 0 /* old hw */
				gpio_set_gpio_pin(AVR32_PIN_PB07);
#endif
			}
			else // (mode == OFF)
			{
#if 0 /* old hw */
				gpio_clr_gpio_pin(AVR32_PIN_PB07);
#endif
			}
			break;

		//----------------------------------------------------------------------------
		case LCD_POWER_ENABLE: // Active high control
		//----------------------------------------------------------------------------
			//debug("Lcd Sleep Enable: %s.\r\n", mode == ON ? "On" : "Off");
			if (mode == ON)
			{
#if 0 /* old hw */
				gpio_set_gpio_pin(AVR32_PIN_PB21);
#endif
			}
			else // (mode == OFF)
			{
#if 0 /* old hw */
				gpio_clr_gpio_pin(AVR32_PIN_PB21);
#endif
			}
			break;

		//----------------------------------------------------------------------------
		case LCD_CONTRAST_ENABLE: // Active high control
		//----------------------------------------------------------------------------
			//debug("Lcd Contrast Enable: %s.\r\n", mode == ON ? "On" : "Off");
			if (mode == ON)
			{
#if 0 /* old hw */
				gpio_set_gpio_pin(AVR32_PIN_PB22); 
#endif
			}				
			else // (mode == OFF)
			{
#if 0 /* old hw */
				gpio_clr_gpio_pin(AVR32_PIN_PB22);
#endif
			}				
			break;

		//----------------------------------------------------------------------------
		case LCD_BACKLIGHT_ENABLE:
		//----------------------------------------------------------------------------
			//debug("Lcd Backlight Enable: %s.\r\n", mode == ON ? "On" : "Off");
			if (mode == ON)
			{
				Backlight_On();
			}				
			else // (mode == OFF)
			{
				Backlight_Off();
			}				
			break;

		//----------------------------------------------------------------------------
		case LCD_BACKLIGHT_HI_ENABLE:
		//----------------------------------------------------------------------------
			//debug("Lcd Backlight Hi Enable: %s.\r\n", mode == ON ? "On" : "Off");
			if (mode == ON)
			{
				Backlight_High();
			}
			else // (mode == OFF)
			{
				Backlight_Low();
			}
			break;

		//----------------------------------------------------------------------------
		case SERIAL_232_DRIVER_ENABLE: // Active low control
		//----------------------------------------------------------------------------
			//debug("Serial 232 Driver Enable: %s.\r\n", mode == ON ? "On" : "Off");
			if (mode == ON)
			{
#if 0 /* old hw */
				gpio_clr_gpio_pin(AVR32_PIN_PB08);
#endif
			}
			else // (mode == OFF)
			{
#if 0 /* old hw */
				gpio_set_gpio_pin(AVR32_PIN_PB08);
#endif
			}
			break;

		//----------------------------------------------------------------------------
		case SERIAL_232_RECEIVER_ENABLE: // Active low control
		//----------------------------------------------------------------------------
			//debug("Serial 232 Receiver Enable: %s.\r\n", mode == ON ? "On" : "Off");
			if (mode == ON)
			{
#if 0 /* old hw */
				gpio_clr_gpio_pin(AVR32_PIN_PB09);
#endif
			}
			else // (mode == OFF)
			{
#if 0 /* old hw */
				gpio_set_gpio_pin(AVR32_PIN_PB09);
#endif
			}
			break;

		//----------------------------------------------------------------------------
		case SERIAL_485_DRIVER_ENABLE:
		//----------------------------------------------------------------------------
			//debug("Serial 485 Driver Enable: %s.\r\n", mode == ON ? "On" : "Off");
			if (mode == ON)
			{
#if 0 /* old hw */
				gpio_set_gpio_pin(AVR32_PIN_PB00);
#endif
			}
			else // (mode == OFF)
			{
#if 0 /* old hw */
				gpio_clr_gpio_pin(AVR32_PIN_PB00);
#endif
			}
			break;

		//----------------------------------------------------------------------------
		case SERIAL_485_RECEIVER_ENABLE:
		//----------------------------------------------------------------------------
#if (NS8100_ALPHA_PROTOTYPE || NS8100_BETA_PROTOTYPE)
			//debug("Serial 485 Receiver Enable: %s.\r\n", mode == ON ? "On" : "Off");
			if (mode == ON)
			{
#if 0 /* old hw */
				gpio_clr_gpio_pin(AVR32_PIN_PB31);
#endif
			}
			else // (mode == OFF)
			{
#if 0 /* old hw */
				gpio_set_gpio_pin(AVR32_PIN_PB31);
#endif
			}
#endif
			break;

		//----------------------------------------------------------------------------
		case USB_HOST_SLEEP_ENABLE:
		//----------------------------------------------------------------------------
			//debug("Usb Host Sleep Enable: %s.\r\n", mode == ON ? "On" : "Off");
			if (mode == ON)
			{
				// Fill in at some point
			}
			else // (mode == OFF)
			{
				// Fill in at some point
			}
			break;

		//----------------------------------------------------------------------------
		case USB_DEVICE_SLEEP_ENABLE:
		//----------------------------------------------------------------------------
			//debug("Usb Device Sleep Enable: %s.\r\n", mode == ON ? "On" : "Off");
			if (mode == ON)
			{
				// Fill in at some point
			}
			else // (mode == OFF)
			{
				// Fill in at some point
			}
			break;

		//----------------------------------------------------------------------------
		case LAN_SLEEP_ENABLE: // Active low control
		//----------------------------------------------------------------------------
			//debug("Lan Sleep Enable: %s.\r\n", mode == ON ? "On" : "Off");
			if (mode == ON)
			{
#if 0 /* old hw */
				gpio_clr_gpio_pin(AVR32_PIN_PB27);
#endif
			}
			else // (mode == OFF)
			{
#if 0 /* old hw */
				gpio_set_gpio_pin(AVR32_PIN_PB27);
#endif
			}
			break;

		//----------------------------------------------------------------------------
		case ANALOG_SLEEP_ENABLE:
		//----------------------------------------------------------------------------
			//debug("Analog Sleep Enable: %s.\r\n", mode == ON ? "On" : "Off");
			if (mode == ON)
			{
				// Fill in at some point if power control for the A/D is available
			}
			else // (mode == OFF)
			{
				// Fill in at some point if power control for the A/D is available
			}
			break;

		//----------------------------------------------------------------------------
		case RTC_TIMESTAMP: // Active low control
		//----------------------------------------------------------------------------
			//debug("RTC Timestamp Enable: %s.\r\n", mode == ON ? "On" : "Off");
			if (mode == ON)
			{
#if 0 /* old hw */
				gpio_clr_gpio_pin(AVR32_PIN_PB18);
#endif
			}
			else // (mode == OFF)
			{
#if 0 /* old hw */
				gpio_set_gpio_pin(AVR32_PIN_PB18);
#endif
			}
			break;

		//----------------------------------------------------------------------------
		case TRIGGER_OUT: // Active high control
		//----------------------------------------------------------------------------
			//debug("Trigger Out Enable: %s.\r\n", mode == ON ? "On" : "Off");
			if (mode == ON)
			{
#if 0 /* old hw */
				gpio_set_gpio_pin(AVR32_PIN_PB05);
#endif
			}
			else // (mode == OFF)
			{
#if 0 /* old hw */
				gpio_clr_gpio_pin(AVR32_PIN_PB05);
#endif
			}
			break;

		//----------------------------------------------------------------------------
		case SEISMIC_SENSOR_DATA_CONTROL: // Active low control
		//----------------------------------------------------------------------------
			//debug("Seismic Sensor Data Control Enable: %s.\r\n", mode == ON ? "On" : "Off");
			if (mode == ON)
			{
#if 0 /* old hw */
				gpio_clr_gpio_pin(AVR32_PIN_PB02);
#endif
			}
			else // (mode == OFF)
			{
#if 0 /* old hw */
				gpio_set_gpio_pin(AVR32_PIN_PB02);
#endif
			}
			break;

		//----------------------------------------------------------------------------
		case ACOUSTIC_SENSOR_DATA_CONTROL: // Active low control
		//----------------------------------------------------------------------------
			//debug("Acoustic Sensor Data Control Enable: %s.\r\n", mode == ON ? "On" : "Off");
			if (mode == ON)
			{
#if 0 /* old hw */
				gpio_clr_gpio_pin(AVR32_PIN_PB03);
#endif
			}
			else // (mode == OFF)
			{
#if 0 /* old hw */
				gpio_set_gpio_pin(AVR32_PIN_PB03);
#endif
			}
			break;

		//----------------------------------------------------------------------------
		case USB_LED: // Active high control
		//----------------------------------------------------------------------------
			//debug("USB LED Enable: %s.\r\n", mode == ON ? "On" : "Off");
			if (mode == ON)
			{
#if 0 /* old hw */
				gpio_set_gpio_pin(AVR32_PIN_PB28);
#endif
			}
			else // (mode == OFF)
			{
#if 0 /* old hw */
				gpio_clr_gpio_pin(AVR32_PIN_PB28);
#endif
			}
			break;

		//----------------------------------------------------------------------------
		case SD_POWER: // Active high control
		//----------------------------------------------------------------------------
			//debug("SD Power Enable: %s.\r\n", mode == ON ? "On" : "Off");
			if (mode == ON)
			{
#if 0 /* old hw */
				gpio_set_gpio_pin(AVR32_PIN_PB15);
#endif
			}
			else // (mode == OFF)
			{
#if 0 /* old hw */
				gpio_clr_gpio_pin(AVR32_PIN_PB15);
#endif
			}
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

	if (s_powerManagement & (1 << option))
	{
		state = ON;
	}

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

#if 0 /* old hw */
	// Make sure all open files are closed and data is flushed
	nav_exit();
#endif

	// Disable Power Off Protection
	PowerControl(POWER_OFF_PROTECTION_ENABLE, OFF);

	if (powerOffMode == SHUTDOWN_UNIT)
	{
		debug("Powering unit off (shutdown)...\r\n");

		// Shutdown application
		PowerControl(POWER_OFF, ON);
	}
	else
	{
		debug("Powering unit off (reboot)...\r\n");

#if 0 /* Original method doesn't work (especially with debug enabled) */
		Disable_global_interrupt();
		AVR32_WDT.ctrl |= 0x00000001;
#else /* Updated method */
		SetTimeOfDayAlarmNearFuture(2);

		// Shutdown application
		PowerControl(POWER_OFF, ON);
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
void InitBattChargerRegister(uint8_t registerAddress, uint16_t registerContents)
{
	// Device setting, disable watchdog
	SetBattChargerRegister(BATT_CHARGER_DEVICE_ADDRESS_SETTING, 0x00E9);

	// Input min V limit (default)
	//SetBattChargerRegister(BATT_CHARGER_INPUT_MINIMUM_VOLTAGE_LIMIT_SETTING, 0x0039);

	// Input Current limit (default)
	//SetBattChargerRegister(BATT_CHARGER_INPUT_CURRENT_LIMIT_SETTING, 0x000A);

	// Output V setting in Source mode (default)
	//SetBattChargerRegister(BATT_CHARGER_OUTPUT_VOLTAGE_SETTING_IN_SOURCE_MODE, 0x00F9);

	// Batt Impedance Comp and Output Current Limit in Source mode (default)
	//SetBattChargerRegister(BATT_CHARGER_BATTERY_IMPEDANCE_COMPENSATION_AND_OUTPUT_CURRENT_LIMIT_SETTING_IN_SOURCE_MODE, 0x0028);

	// Batt low V setting and Batt Discharge Current Reg in Source mode (default)
	//SetBattChargerRegister(BATT_CHARGER_BATTERY_LOW_VOLTAGE_THRESHOLD_AND_BATTERY_DISCHARGE_CURRENT_REGULATION_IN_SOURCE_MODE, 0x3080);

	// JEITA Action (default)
	//SetBattChargerRegister(BATT_CHARGER_JEITA_ACTION_SETTING, 0x3410);

	// Temp Protection (default)
	//SetBattChargerRegister(BATT_CHARGER_TEMPERATURE_PROTECTION_SETTING, 0xB399);

	// Config Reg 0 (default)
	//SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_0, 0x0010);

	// Config Reg 1 (default)
	//SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_1, 0xF244);

	// Config Reg 2 (default)
	//SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_2, 0x0A40);

	// Config Reg 3 (default)
	//SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_3, 0x50E8);

	// Config Reg 4 (default)
	//SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_4, 0x3C53);

	// Charge Current (default)
	//SetBattChargerRegister(BATT_CHARGER_CHARGE_CURRENT_SETTING, 0x0A00);

	// Batt Reg V (default)
	//SetBattChargerRegister(BATT_CHARGER_BATTERY_REGULATION_VOLTAGE_SETTING, 0x3480);

	// Int Mask setting (default)
	//SetBattChargerRegister(BATT_CHARGER_INT_MASK_SETTING_REGISTER_0, 0x0000);

	// Int Mask setting (default)
	//SetBattChargerRegister(BATT_CHARGER_INT_MASK_SETTING_REGISTER_1, 0x0000);
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
	LTC294X_REG_STATUS					= 0x00,
	LTC294X_REG_CONTROL					= 0x01,
	LTC294X_REG_ACC_CHARGE_MSB			= 0x02,
	LTC294X_REG_ACC_CHARGE_LSB			= 0x03,
	LTC294X_REG_CHARGE_THR_HIGH_MSB		= 0x04,
	LTC294X_REG_CHARGE_THR_HIGH_LSB		= 0x05,
	LTC294X_REG_CHARGE_THR_LOW_MSB		= 0x06,
	LTC294X_REG_CHARGE_THR_LOW_LSB		= 0x07,
	LTC294X_REG_VOLTAGE_MSB				= 0x08,
	LTC294X_REG_VOLTAGE_LSB				= 0x09,
	LTC294X_REG_VOLTAGE_THR_HIGH_MSB 	= 0x0A,
	LTC294X_REG_VOLTAGE_THR_HIGH_LSB 	= 0x0B,
	LTC294X_REG_VOLTAGE_THR_LOW_MSB 	= 0x0C,
	LTC294X_REG_VOLTAGE_THR_LOW_LSB 	= 0x0D,
	LTC2943_REG_CURRENT_MSB				= 0x0E,
	LTC2943_REG_CURRENT_LSB				= 0x0F,
	LTC2943_REG_CURRENT_THR_HIGH_MSB	= 0x10,
	LTC2943_REG_CURRENT_THR_HIGH_LSB	= 0x11,
	LTC2943_REG_CURRENT_THR_LOW_MSB		= 0x12,
	LTC2943_REG_CURRENT_THR_LOW_LSB		= 0x13,
	LTC2943_REG_TEMPERATURE_MSB			= 0x14,
	LTC2943_REG_TEMPERATURE_LSB			= 0x15,
	LTC2943_REG_TEMPERATURE_THR_HIGH 	= 0x16,
	LTC2943_REG_TEMPERATURE_THR_LOW 	= 0x17
};

#define LTC2941_REG_STATUS_CHIP_ID	(1 << 7)

#define LTC2942_REG_CONTROL_MODE_SCAN	((1 << 7) | (1 << 6))
#define LTC2943_REG_CONTROL_MODE_SCAN	(1 << 7)
#define LTC294X_REG_CONTROL_PRESCALER_MASK	((1 << 5) | (1 << 4) | (1 << 3))
#define LTC294X_REG_CONTROL_SHUTDOWN_MASK	((1 << 0))
#define LTC294X_REG_CONTROL_PRESCALER_SET(x)	((x << 3) & LTC294X_REG_CONTROL_PRESCALER_MASK)
#define LTC294X_REG_CONTROL_ALCC_CONFIG_DISABLED	0
#define LTC294X_REG_CONTROL_ADC_DISABLE(x)	((x) & ~((1 << 7) | (1 << 6)))

#define LTC294X_PRESCALER_1		0x0
#define LTC294X_PRESCALER_4		0x1
#define LTC294X_PRESCALER_16	0x2
#define LTC294X_PRESCALER_64	0x3
#define LTC294X_PRESCALER_256	0x4
#define LTC294X_PRESCALER_1024	0x5
#define LTC294X_PRESCALER_4096	0x6
#define LTC294X_MAX_PRESCALER	4096


struct ltc294x_info {
	//struct power_supply *supply; // Supply pointer
	//struct power_supply_desc supply_desc; // Supply description
	//struct delayed_work work; // Work scheduler
	//enum ltc294x_id id; // Chip type
	int charge; // Last charge register content
	int r_sense; // mOhm
	int Qlsb; // nAh
};
struct ltc294x_info ltc2944_device;

inline int convert_bin_to_uAh(const struct ltc294x_info *info, int Q)
{
	return (((Q * (info->Qlsb / 10))) / 100);
}

inline int convert_uAh_to_bin(const struct ltc294x_info *info, int uAh)
{
	int Q;

	Q = (uAh * 100) / (info->Qlsb/10);
	return ((Q < LTC294X_MAX_VALUE) ? Q : LTC294X_MAX_VALUE);
}

int ltc294x_read_regs(enum ltc294x_reg reg, uint8_t *buf, int bufSize)
{
	return (WriteI2CDevice(MXC_I2C1, I2C_ADDR_FUEL_GUAGE, &reg, sizeof(uint8_t), buf, bufSize));
}

int ltc294x_write_regs(enum ltc294x_reg reg, const uint8_t *buf, int bufSize)
{
	g_spareBuffer[0] = reg;
	memcpy(&g_spareBuffer[1], buf, bufSize);

	return (WriteI2CDevice(MXC_I2C1, I2C_ADDR_FUEL_GUAGE, &reg, (bufSize + 1), NULL, 0));
}

int ltc294x_reset(int prescaler_exp)
{
	int ret;
	uint8_t value;
	uint8_t control;

	// Read status and control registers
	ret = ltc294x_read_regs(LTC294X_REG_CONTROL, &value, 1);
	if (ret < 0) { return ret; }

	control = LTC294X_REG_CONTROL_PRESCALER_SET(prescaler_exp) | LTC294X_REG_CONTROL_ALCC_CONFIG_DISABLED;
	control |= LTC2943_REG_CONTROL_MODE_SCAN; // 2944 measures every 10 sec

	if (value != control)
	{
		ret = ltc294x_write_regs(LTC294X_REG_CONTROL, &control, 1);
		if (ret < 0) { return ret; }
	}

	return (0);
}

int ltc294x_read_charge_register(const struct ltc294x_info *info, enum ltc294x_reg reg)
 {
	int ret;
	uint8_t datar[2];

	ret = ltc294x_read_regs(reg, &datar[0], 2);

	if (ret < 0) { return ret; }
	return ((datar[0] << 8) + datar[1]);
}

int ltc294x_get_charge(const struct ltc294x_info *info, enum ltc294x_reg reg, int *val)
{
	int value = ltc294x_read_charge_register(info, reg);

	if (value < 0) { return value; }

	/* When r_sense < 0, this counts up when the battery discharges */
	if (info->Qlsb < 0) { value -= 0xFFFF; }

	*val = convert_bin_to_uAh(info, value);
	return (0);
}

int ltc294x_set_charge_now(const struct ltc294x_info *info, int val)
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

int ltc294x_set_charge_thr(const struct ltc294x_info *info, enum ltc294x_reg reg, int val)
{
	uint8_t dataw[2];
	int32_t value;

	value = convert_uAh_to_bin(info, val);
	// Direction depends on how sense +/- were connected
	if (info->Qlsb < 0) { value += 0xFFFF; }
	if ((value < 0) || (value > 0xFFFF)) { return E_INVALID; } // input validation

	// Set new charge value
	dataw[0] = I16_MSB(value);
	dataw[1] = I16_LSB(value);

	return (ltc294x_write_regs(reg, &dataw[0], 2));
}

int ltc294x_get_charge_counter( const struct ltc294x_info *info, int *val)
{
	int value = ltc294x_read_charge_register(info, LTC294X_REG_ACC_CHARGE_MSB);

	if (value < 0) { return value; }

	value -= LTC294X_MID_SUPPLY;
	*val = convert_bin_to_uAh(info, value);

	return (0);
}

int ltc294x_get_voltage(const struct ltc294x_info *info, int *val)
{
	int ret;
	uint8_t datar[2];
	uint32_t value;

	ret = ltc294x_read_regs(LTC294X_REG_VOLTAGE_MSB, &datar[0], 2);
	value = ((datar[0] << 8) | datar[1]);

	value *= 70800 / 5*4; // Max value can be clipped in uint32, scale down
	value /= 0xFFFF;
#if 1 /* Results in uV */
	value *= 1000 * 5/4; // Reverse the scale down
#else /* Results in mV */
	value *= 5/4; // Reverse the scale down
#endif

	*val = value;
	return (ret);
}

int ltc294x_get_current(const struct ltc294x_info *info, int *val)
{
	int ret;
	uint8_t datar[2];
	int32_t value;

	ret = ltc294x_read_regs(LTC2943_REG_CURRENT_MSB, &datar[0], 2);
	value = ((datar[0] << 8) | datar[1]);
	value -= 0x7FFF;
	value *= 64000;

	// Value is in range -32k..+32k, r_sense is usually 10..50 mOhm, the formula below keeps everything in s32 range while preserving enough digits
	*val = (1000 * (value / (info->r_sense * 0x7FFF))); // Units in uA

	return (ret);
}

int ltc294x_get_temperature(const struct ltc294x_info *info, int *val)
{
	int ret;
	uint8_t datar[2];
	uint32_t value;

	value = 510 * 10; // Full-scale is 510 Kelvin, shift up for tenths

	ret = ltc294x_read_regs(LTC2943_REG_TEMPERATURE_MSB, &datar[0], 2);
	value *= ((datar[0] << 8) | datar[1]);

#if 0 /* Results in C */
	// Convert to tenths of degree Celsius
	*val = value / 0xFFFF - 2732 // C = K - 273.15, in tenths is 2732 // old reference value was 2722;
#else /* Results in F */
	// Convert to F, F = (C * (9/5)) + 32
	*val = (((value / 0xFFFF - 2732) * (9/5)) + 32);
#endif

	return (ret);
}

void ltc294x_update(struct ltc294x_info *info)
{
	int charge = ltc294x_read_charge_register(info, LTC294X_REG_ACC_CHARGE_MSB);

	if (charge != info->charge)
	{
		info->charge = charge;
		
		// Power supply changed
		// Action?
	}
}

int ltc294x_i2c_probe(void)
{
	//struct power_supply_config psy_cfg = {};
	//struct ltc294x_info *info;
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
	// r_sense is usually 10..50 mOhm
	r_sense = 10; // Attempting wrong value for now (to compile), must fill in with right value

	ltc2944_device.r_sense = r_sense;

	prescaler_exp = LTC294X_PRESCALER_4096;

	ltc2944_device.Qlsb = ((340 * 50000) / r_sense) * (prescaler_exp / LTC294X_MAX_PRESCALER);

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
