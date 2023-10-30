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
#include "Typedefs.h"
#include "Common.h"
#include "Uart.h"
#include "powerManagement.h"
#include "RealTimeClock.h"
#include "gpio.h"
#include "M23018.h"
#include "lcd.h"
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
	uint8 state;

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
