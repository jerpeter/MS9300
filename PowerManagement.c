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
#include "tmr.h"
#include "mxc_delay.h"
#include "stdlib.h"
#include "spi.h"
//#include "navigation.h"

#include "usb.h"
#include "uart.h"

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
			//debug("Alarm 1 Enable: %s\r\n", mode == ON ? "On" : "Off"); // Can run in ISR context
			if (mode == ON) { MXC_GPIO_OutSet(GPIO_ALERT_1_PORT, GPIO_ALERT_1_PIN); }
			else /* (mode == OFF) */ { MXC_GPIO_OutClr(GPIO_ALERT_1_PORT, GPIO_ALERT_1_PIN); }
			break;

		//----------------------------------------------------------------------------
		case ALARM_2_ENABLE: // Active high
		//----------------------------------------------------------------------------
			//debug("Alarm 2 Enable: %s\r\n", mode == ON ? "On" : "Off"); // Can run in ISR context
			if (mode == ON) { MXC_GPIO_OutSet(GPIO_ALERT_2_PORT, GPIO_ALERT_2_PIN); }
			else /* (mode == OFF) */ { MXC_GPIO_OutClr(GPIO_ALERT_2_PORT, GPIO_ALERT_2_PIN); }
			break;

		//----------------------------------------------------------------------------
		case LCD_POWER_ENABLE: // Active high
		//----------------------------------------------------------------------------
			debug("LCD Power Enable: %s\r\n", mode == ON ? "On" : "Off");
			if (mode == ON) { MXC_GPIO_OutSet(GPIO_LCD_POWER_ENABLE_PORT, GPIO_LCD_POWER_ENABLE_PIN); /* 20ms delay needed before FT810Q ready */ }
			else /* (mode == OFF) */ { MXC_GPIO_OutClr(GPIO_LCD_POWER_ENABLE_PORT, GPIO_LCD_POWER_ENABLE_PIN); }
			LcdPowerGpioSetup(mode);
			break;

		//----------------------------------------------------------------------------
		case TRIGGER_OUT: // Active high
		//----------------------------------------------------------------------------
			//debug("External Trigger Out: %s\r\n", mode == ON ? "On" : "Off"); // Can run in ISR context
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
			ExpansionPowerGpioSetup(mode);
			break;

		//----------------------------------------------------------------------------
		case SENSOR_CHECK_ENABLE: // Active high
		//----------------------------------------------------------------------------
			//debug("Sensor Check Enable: %s\r\n", mode == ON ? "On" : "Off"); // Can run in ISR context
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
		case CELL_ENABLE: // Active high
		//----------------------------------------------------------------------------
			debug("Cellular: %s\r\n", mode == ON ? "On" : "Off");
			if (mode == ON) { MXC_GPIO_OutSet(GPIO_CELL_ENABLE_PORT, GPIO_CELL_ENABLE_PIN); }
			else /* (mode == OFF) */ { MXC_GPIO_OutClr(GPIO_CELL_ENABLE_PORT, GPIO_CELL_ENABLE_PIN); }
			CellPowerGpioSetup(mode);
			break;

		//----------------------------------------------------------------------------
		case EXPANSION_RESET: // Active low
		//----------------------------------------------------------------------------
			debug("Expansion Reset: %s\r\n", mode == ON ? "On" : "Off");
			if (mode == ON) { MXC_GPIO_OutClr(GPIO_EXPANSION_RESET_PORT, GPIO_EXPANSION_RESET_PIN); }
			else /* (mode == OFF) */ { MXC_GPIO_OutSet(GPIO_EXPANSION_RESET_PORT, GPIO_EXPANSION_RESET_PIN); }
			ExpansionI2CBridgeGpioSetup(!mode); // Swap mode to Active high
			break;

		//----------------------------------------------------------------------------
		case LCD_POWER_DOWN: // Active low
		//----------------------------------------------------------------------------
			debug("LCD Power Down: %s\r\n", mode == ON ? "On" : "Off");
			if (mode == ON) { MXC_GPIO_OutClr(GPIO_LCD_POWER_DOWN_PORT, GPIO_LCD_POWER_DOWN_PIN); /* 20ms delay needed before FT810Q ready */ }
			else /* (mode == OFF) */ { MXC_GPIO_OutSet(GPIO_LCD_POWER_DOWN_PORT, GPIO_LCD_POWER_DOWN_PIN); }
			LcdControllerGpioSetup(!mode); // Swap mode to Active high
			break;

		//----------------------------------------------------------------------------
		case LED_1: // Active low (Blue)
		//----------------------------------------------------------------------------
			//debug("LED 1: %s\r\n", mode == ON ? "On" : "Off");
#if 0 /* Original Active high */
			if (mode == ON) { MXC_GPIO_OutSet(GPIO_LED_1_PORT, GPIO_LED_1_PIN); }
			else /* (mode == OFF) */ { MXC_GPIO_OutClr(GPIO_LED_1_PORT, GPIO_LED_1_PIN); }
#else /* Active low */
			if (mode == ON) { MXC_GPIO_OutClr(GPIO_LED_1_PORT, GPIO_LED_1_PIN); }
			else /* (mode == OFF) */ { MXC_GPIO_OutSet(GPIO_LED_1_PORT, GPIO_LED_1_PIN); }
#endif
			break;

		//----------------------------------------------------------------------------
		case LED_2: // Active low (Green)
		//----------------------------------------------------------------------------
			//debug("LED 2: %s\r\n", mode == ON ? "On" : "Off");
#if 0 /* Original Active high */
			if (mode == ON) { MXC_GPIO_OutSet(GPIO_LED_2_PORT, GPIO_LED_2_PIN); }
			else /* (mode == OFF) */ { MXC_GPIO_OutClr(GPIO_LED_2_PORT, GPIO_LED_2_PIN); }
#else /* Active low */
			if (mode == ON) { MXC_GPIO_OutClr(GPIO_LED_2_PORT, GPIO_LED_2_PIN); }
			else /* (mode == OFF) */ { MXC_GPIO_OutSet(GPIO_LED_2_PORT, GPIO_LED_2_PIN); }
#endif
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
	uint32_t gpioReg = 0;

	switch (option)
	{
		case ALARM_1_ENABLE: gpioReg = MXC_GPIO_OutGet(GPIO_ALERT_1_PORT, GPIO_ALERT_1_PIN); break;
		case ALARM_2_ENABLE: gpioReg = MXC_GPIO_OutGet(GPIO_ALERT_2_PORT, GPIO_ALERT_2_PIN); break;
		case LCD_POWER_ENABLE: gpioReg = MXC_GPIO_OutGet(GPIO_LCD_POWER_ENABLE_PORT, GPIO_LCD_POWER_ENABLE_PIN); break;
		case TRIGGER_OUT: gpioReg = MXC_GPIO_OutGet(GPIO_EXTERNAL_TRIGGER_OUT_PORT, GPIO_EXTERNAL_TRIGGER_OUT_PIN); break;
		case MCU_POWER_LATCH: gpioReg = MXC_GPIO_OutGet(GPIO_MCU_POWER_LATCH_PORT, GPIO_MCU_POWER_LATCH_PIN); break;
		case ENABLE_12V: gpioReg = MXC_GPIO_OutGet(GPIO_ENABLE_12V_PORT, GPIO_ENABLE_12V_PIN); break;
		case USB_SOURCE_ENABLE: gpioReg = MXC_GPIO_OutGet(GPIO_USB_SOURCE_ENABLE_PORT, GPIO_USB_SOURCE_ENABLE_PIN); break;
		case USB_AUX_POWER_ENABLE: gpioReg = MXC_GPIO_OutGet(GPIO_USB_AUX_POWER_ENABLE_PORT, GPIO_USB_AUX_POWER_ENABLE_PIN); break;
		case ADC_RESET: gpioReg = !MXC_GPIO_OutGet(GPIO_ADC_RESET_PORT, GPIO_ADC_RESET_PIN); break; // Active low, invert state
		case EXPANSION_ENABLE: gpioReg = MXC_GPIO_OutGet(GPIO_EXPANSION_ENABLE_PORT, GPIO_EXPANSION_ENABLE_PIN); break;
		case SENSOR_CHECK_ENABLE: gpioReg = MXC_GPIO_OutGet(GPIO_SENSOR_CHECK_ENABLE_PORT, GPIO_SENSOR_CHECK_ENABLE_PIN); break;
		case LTE_RESET: gpioReg = !MXC_GPIO_OutGet(GPIO_LTE_RESET_PORT, GPIO_LTE_RESET_PIN); break; // Active low, invert state
		case CELL_ENABLE: gpioReg = MXC_GPIO_OutGet(GPIO_CELL_ENABLE_PORT, GPIO_CELL_ENABLE_PIN); break;
		case EXPANSION_RESET: gpioReg = !MXC_GPIO_OutGet(GPIO_EXPANSION_RESET_PORT, GPIO_EXPANSION_RESET_PIN); break; // Active low, invert state
		case LCD_POWER_DOWN: gpioReg = !MXC_GPIO_OutGet(GPIO_LCD_POWER_DOWN_PORT, GPIO_LCD_POWER_DOWN_PIN); break; // Active low, invert state
		case LED_1: gpioReg = MXC_GPIO_OutGet(GPIO_LED_1_PORT, GPIO_LED_1_PIN); break;
		case LED_2: gpioReg = MXC_GPIO_OutGet(GPIO_LED_2_PORT, GPIO_LED_2_PIN); break;
	}

	// GPIO pin value tied to bit position so normalize to a logic 1 if any bit is set
	if (gpioReg) { state = ON; }

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
void Analog5vPowerGpioSetup(uint8_t mode)
{
	if (mode == ON)
	{
		// Analog 5V Gpio pins initial state (since all set low on disable to prevent back powering)
		MXC_GPIO_OutSet(GPIO_CAL_MUX_PRE_AD_ENABLE_PORT, GPIO_CAL_MUX_PRE_AD_ENABLE_PIN); // Disable (Active low)
		MXC_GPIO_OutSet(GPIO_NYQUIST_2_ENABLE_PORT, GPIO_NYQUIST_2_ENABLE_PIN); // Disable (Active low)
		MXC_GPIO_OutSet(GPIO_GAIN_SELECT_GEO1_PORT, GPIO_GAIN_SELECT_GEO1_PIN); // Enable Normal gain (Active high)
		MXC_GPIO_OutSet(GPIO_GAIN_SELECT_GEO2_PORT, GPIO_GAIN_SELECT_GEO2_PIN); // Enable Normal gain (Active high)
		MXC_GPIO_OutSet(GPIO_PATH_SELECT_AOP1_PORT, GPIO_PATH_SELECT_AOP1_PIN); // Enable AOP path (Active high)
		MXC_GPIO_OutSet(GPIO_PATH_SELECT_AOP2_PORT, GPIO_PATH_SELECT_AOP2_PIN); // Enable AOP path (Active high)

		//SetupSPI3_ExternalADC()
		SetupSPI3_ExternalADC(30 * 1000000);
		//SetupSPI3_ExternalADC(0.5 * 1000000);
	}
	else // (mode == OFF)
	{
		// Analog 5V Gpio pins that need to be set low or turned into inputs to prevent back powering
		// GPIO 0 pins: SS sleep, SS mux enable, ADC Conversion, Cal Mux enable, Cal Mux select
		// GPIO 2 pins: Sensor Check enable, Sensor Check state, SS Mux A0, SS Mux A1, Nyquist 0, Nyquist 1, Nyquist 2
		// GPIO 3 pins: Sensor Enable 1-4, Gain/Path select 1-4

		MXC_GPIO_OutClr(MXC_GPIO0, (GPIO_SMART_SENSOR_SLEEP_PIN | GPIO_SMART_SENSOR_MUX_ENABLE_PIN | GPIO_ADC_CONVERSION_PIN | GPIO_CAL_MUX_PRE_AD_ENABLE_PIN | GPIO_CAL_MUX_PRE_AD_SELECT_PIN)); // Set low to prevent back powering
		MXC_GPIO_OutClr(MXC_GPIO2, (GPIO_SENSOR_CHECK_ENABLE_PIN | GPIO_SENSOR_CHECK_PIN | GPIO_SMART_SENSOR_MUX_A0_PIN | GPIO_SMART_SENSOR_MUX_A1_PIN |
						GPIO_NYQUIST_0_A0_PIN | GPIO_NYQUIST_1_A1_PIN | GPIO_NYQUIST_2_ENABLE_PIN)); // Set low to prevent back powering
		MXC_GPIO_OutClr(MXC_GPIO3, (GPIO_SENSOR_ENABLE_GEO1_PIN | GPIO_SENSOR_ENABLE_AOP1_PIN | GPIO_SENSOR_ENABLE_GEO2_PIN | GPIO_SENSOR_ENABLE_AOP2_PIN |
						GPIO_GAIN_SELECT_GEO1_PIN | GPIO_PATH_SELECT_AOP1_PIN | GPIO_GAIN_SELECT_GEO2_PIN | GPIO_PATH_SELECT_AOP2_PIN)); // Set low to prevent back powering

		MXC_SPI_Shutdown(MXC_SPI3);
		mxc_gpio_cfg_t gpio_cfg_spi3 = { MXC_GPIO0, (GPIO_ADC_SPI3_SCK_PIN | GPIO_ADC_SPI3_SS0_PIN | GPIO_ADC_SPI3_SDO1_PIN | GPIO_ADC_SPI3_SDI_PIN), MXC_GPIO_FUNC_IN, MXC_GPIO_PAD_NONE, MXC_GPIO_VSSEL_VDDIO };
		MXC_GPIO_Config(&gpio_cfg_spi3);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void LcdPowerGpioSetup(uint8_t mode)
{
	if (mode == ON)
	{
		// Mark SPI2 state that LCD is active
		g_spi2State |= SPI2_LCD_ON;

		// Check if SPI2 is not operational and setup if necessary
		if ((g_spi2State & SPI2_OPERAITONAL) == NO)
		{
			SetupSPI2_LCDAndAcc();
			g_spi2State |= SPI2_OPERAITONAL; // Mark SPI2 state is operational

			// Setup the SPI2 Slave Select since the driver init call does not initialize the GPIO
			mxc_gpio_cfg_t spi2SlaveSelect0GpioConfig = { GPIO_SPI2_SS0_LCD_PORT, GPIO_SPI2_SS0_LCD_PIN, MXC_GPIO_FUNC_OUT, MXC_GPIO_PAD_NONE, MXC_GPIO_VSSEL_VDDIO };
			if (FT81X_SPI_2_SS_CONTROL_MANUAL == NO) { spi2SlaveSelect0GpioConfig.func = MXC_GPIO_FUNC_ALT1; } // SPI2 Slave Select controlled by the SPI driver
			MXC_GPIO_Config(&spi2SlaveSelect0GpioConfig);
		}
	}
	else // (mode == OFF)
	{
		// Clear SPI2 state for LCD active
		g_spi2State &= ~SPI2_LCD_ON;

		// Check if SPI2 is not active for the Accelerometer
		if ((g_spi2State & SPI2_ACC_ON) == NO)
		{
			MXC_SPI_Shutdown(MXC_SPI2);
			g_spi2State &= ~SPI2_OPERAITONAL; // Mark SPI2 state is shutdown

			// Change the SPI2 GPIO config to inputs to disable (for back powering concerns)
			mxc_gpio_cfg_t gpio_cfg_spi2 = { MXC_GPIO2, (GPIO_SPI2_SCK_PIN | GPIO_SPI2_MISO_PIN | GPIO_SPI2_MOSI_PIN | GPIO_SPI2_SS0_LCD_PIN), MXC_GPIO_FUNC_IN, MXC_GPIO_PAD_NONE, MXC_GPIO_VSSEL_VDDIO };
			MXC_GPIO_Config(&gpio_cfg_spi2);
		}
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void LcdControllerGpioSetup(uint8_t mode)
{
	if (mode == ON)
	{
		MXC_GPIO_EnableInt(GPIO_LCD_INT_PORT, GPIO_LCD_INT_PIN);
	}
	else // (mode == OFF)
	{
		MXC_GPIO_DisableInt(GPIO_LCD_INT_PORT, GPIO_LCD_INT_PIN);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ExpansionPowerGpioSetup(uint8_t mode)
{
	if (mode == ON)
	{

	}
	else // (mode == OFF)
	{

	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ExpansionI2CBridgeGpioSetup(uint8_t mode)
{
	if (mode == ON)
	{
		MXC_GPIO_EnableInt(GPIO_EXPANSION_IRQ_PORT, GPIO_EXPANSION_IRQ_PIN);
	}
	else // (mode == OFF)
	{
		MXC_GPIO_DisableInt(GPIO_EXPANSION_IRQ_PORT, GPIO_EXPANSION_IRQ_PIN);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void CellPowerGpioSetup(uint8_t mode)
{
	if (mode == ON)
	{
#if 1 /* Normal */
		MXC_GPIO_OutSet(GPIO_LTE_RESET_PORT, GPIO_LTE_RESET_PIN); // Disable (Active low)
#else /* Test without setting these immediately */
#endif
	}
	else // (mode == OFF)
	{
		MXC_GPIO_OutClr(GPIO_LTE_RESET_PORT, GPIO_LTE_RESET_PIN); // Set low to prevent back powering
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupSPI2_Accelerometer(uint8_t mode)
{
	if (mode == ON)
	{
		g_spi2State |= SPI2_ACC_ON;

		// Check if SPI2 is not already active
		if ((g_spi2State & SPI2_OPERAITONAL) == NO)
		{
			SetupSPI2_LCDAndAcc();
			g_spi2State |= SPI2_OPERAITONAL; // Mark SPI2 state is active
		}
	}
	else // (mode == OFF)
	{
		g_spi2State &= ~SPI2_ACC_ON;

		// Check if SPI2 is not still active for the LCD
		if ((g_spi2State & SPI2_LCD_ON) == NO)
		{
			MXC_SPI_Shutdown(MXC_SPI2);
			g_spi2State &= ~SPI2_OPERAITONAL; // Mark SPI2 state is shutdown

			// Change the SPI2 GPIO config to inputs to disable (for back powering concerns)
			mxc_gpio_cfg_t gpio_cfg_spi2 = { MXC_GPIO2, (GPIO_SPI2_SCK_PIN | GPIO_SPI2_MISO_PIN | GPIO_SPI2_MOSI_PIN), MXC_GPIO_FUNC_IN, MXC_GPIO_PAD_NONE, MXC_GPIO_VSSEL_VDDIO };
			MXC_GPIO_Config(&gpio_cfg_spi2);
		}
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t GetCurrentLedStates(void)
{
	uint8_t state;

	// Get the LED states and shift bit representation to the lowest nibble, with LED 1 at bit 0
	state = (MXC_GPIO_OutGet(GPIO_LED_1_PORT, GPIO_LED_1_PIN) ? ON : OFF);
	state |= ((MXC_GPIO_OutGet(GPIO_LED_2_PORT, GPIO_LED_2_PIN) ? ON : OFF) << 1);

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

		// Put Fuel Gauge ADC to sleep while off (device is battery powered and not placed into reset)
		Ltc2944_i2c_shutdown();

		// Disable USB
		MXC_USB_Shutdown();

		// Disable Cell module
		if (GetPowerControlState(CELL_ENABLE) == ON)
		{
			ShutdownPdnAndCellModem();
			PowerControl(LTE_RESET, ON);
			PowerControl(CELL_ENABLE, OFF);
		}

		// Disable power blocks
		PowerControl(ADC_RESET, ON);
		PowerControl(LCD_POWER_ENABLE, OFF);
		PowerControl(ENABLE_12V, OFF);
		PowerControl(EXPANSION_ENABLE, OFF);

		// Check if charging is present which prevents powering down
		if (GetPowerGoodBatteryChargerState() == YES)
		{
			debug("Charging present, issuing MCU reset...\r\n");

			// Toggle MCU system reset
			MXC_GCR->rst0 |= MXC_F_GCR_RST0_SYS;
		}
		else
		{
			debug("Bye...\r\n");

			// Shutdown application
			PowerControl(MCU_POWER_LATCH, OFF);
		}
	}
	else
	{
		debug("Powering unit off (reboot)...\r\n");

#if 0 /* Original method doesn't work (especially with debug enabled) */
		Disable_global_interrupt();
		AVR32_WDT.ctrl |= 0x00000001;
#else /* Updated method */
		// Can't use the External RTC and MCU latch since there is no source to wake the unit back up

		// Toggle MCU system reset
		MXC_GCR->rst0 |= MXC_F_GCR_RST0_SYS;
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
uint16_t ReturnBattChargerRegister(uint8_t registerAddress)
{
	uint16_t registerContents;

	// Word data read back in Little endian format
	WriteI2CDevice(MXC_I2C0, I2C_ADDR_BATT_CHARGER, &registerAddress, sizeof(uint8_t), (uint8_t*)&registerContents, sizeof(uint16_t));

	return (registerContents);
}

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
	writeData[2] = ((registerContents >> 8) & 0xFF);

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
	// 	Default Iin current limit is 500mA, 5000mA (spec limit)
	//	Change Iin current closer to spec max, 4A (input current, not charge current so single/dual pack logic not needed)
	SetBattChargerRegister(BATT_CHARGER_INPUT_CURRENT_LIMIT_SETTING, 0x0050);

	// Output V setting in Source mode
	// 	Default Vin_src additional V is 0V, config Vin_src by register select, Vin_src is 4.98V
	//	No change from default
	SetBattChargerRegister(BATT_CHARGER_OUTPUT_VOLTAGE_SETTING_IN_SOURCE_MODE, 0x00F9);

	// Batt Impedance Comp and Output Current Limit in Source mode
	// 	Default battery impedance is 0 mOhm, max compensaton voltage is 0mV/cell, Iout limit in source mode is 2A
	//	No change from default
	SetBattChargerRegister(BATT_CHARGER_BATTERY_IMPEDANCE_COMPENSATION_AND_OUTPUT_CURRENT_LIMIT_SETTING_IN_SOURCE_MODE, 0x003C);

	// Batt low V setting and Batt Discharge Current Reg in Source mode
	// 	Default battery LV procetion is on, pre-charge to CC is 3V/cell, battery low action is INT only, Vbatt low is 3V/cell
	// 	Default batt discharge current regulation in source disabled, batt discharge current in source is 6.4A
	//	Change batt discharge current in source mode to 3000mA for single pack and 6000mA for double pack
	if (GetExpandedBatteryPresenceState() == NO)
	{
		debug("Battery Charger: Batt discharge in source set for Single pack\r\n");
		SetBattChargerRegister(BATT_CHARGER_BATTERY_LOW_VOLTAGE_THRESHOLD_AND_BATTERY_DISCHARGE_CURRENT_REGULATION_IN_SOURCE_MODE, 0x303C); // Single pack
	}
	else { debug("Battery Charger: Batt discharge in source set for Dual pack\r\n"); SetBattChargerRegister(BATT_CHARGER_BATTERY_LOW_VOLTAGE_THRESHOLD_AND_BATTERY_DISCHARGE_CURRENT_REGULATION_IN_SOURCE_MODE, 0x3078); } // Double pack

	// JEITA Action
	//	Default warm protect is only reduce Vbatt_reg, cool protect is only reduce Icc, decrement value for batt full voltage if NTC cool/warm protect occurs is 320mV/cell
	// 	Default scaling value of CC charge current is 1/4 times
	//	Change Warm Act to reduce both Vbatt_reg and Icc, Cool Act to reduce both Vbatt_reg and Icc
	SetBattChargerRegister(BATT_CHARGER_JEITA_ACTION_SETTING, 0x7C10);

	// Temperature Protection
	//	Default Ext Temp is enabled, OPT action is deliver INT and take TS action, TS OT threshold is 80C, NTC protect is on
	//	Default NTC protect action is deliver INT and take JEITA action, NTC hot thr is 60C, NTC warm thr is 45C, NTC cool thr is 10C, NTC cold thr is 0C
	//  Change Vts_hot to 65C
	//  Todo: Change V_hot to 50C ??
	SetBattChargerRegister(BATT_CHARGER_TEMPERATURE_PROTECTION_SETTING, 0xBF99);

	// Config Reg 0
	//	Default ADC start is disabled, ADC conv is one shot, switcing freq is 600kHz
	//	No change from default
	SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_0, 0x0010);

	// Config Reg 1
	//	Default junction temp OT regulation is enabled, juntion temp regulation point is 120C, tricle charge current is 100mA, pre-charge current is 400mA
	//	Default terminaiton current is 200mA
	//	References to setting pre-charge @ C/10 (6600/10=660 or 13200/10=1320), termination around C/10 to C/20 (going with C/20 yields 6600/20=330 or 13200/20=660)
	//	Change Ipre to 600mA single/1200mA double, Iterm to 200mA single/400mA double
	if (GetExpandedBatteryPresenceState() == NO)
	{
		if (BATTERY_PACK_SINGLE_CAPACITY == 8000) { SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_1, 0xF285); } // Single pack (larger), pre-charge @ 800mA, termination @ 250mA
		else { SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_1, 0xF264); } // Single pack, pre-charge @ 600mA, termination @ 200mA
	}
	else
	{
		if (BATTERY_PACK_DOUBLE_CAPACITY == 16000) { SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_1, 0xF2F9); } // Double pack (larger), pre-charge @ 1500mA, termination @ 450mA
		else { SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_1, 0xF2C8); } // Double pack, pre-charge @ 1200mA, termination @ 400mA
	}

	// Config Reg 2
	//	Default ACgate not forced, TS/IMON (Pin 7) config is TS, auto recharge thr is -200mV/cell, batt cells in series is 2, Iin sense gain is 10mOhm
	//	Default batt current sense gain is 10mOhm, ACgate driver is enabled
	//	Note: Batt cells in series is 2 cells (2Sx2P config)
	//	Change ACgate to disabled
	SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_2, 0x0A00);

	// Config Reg 3
	//	Default OV thr for source Vout is 110%, UV thf for source Vout is 75%, deglitch time for OVP in charge mode is 1us, input UVP thr is 3.2V
	//	Default input OVP thr is 22.4V, batt OVP is enabled
	//	No change from default
	SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_3, 0x60E8);

	// Config Reg 4
	//	Default charge saftey timer is enabled, CC/CV timer is 20hr, saftey timer is doubled, reset WDT is normal, WDT timer is disabled, DC/DC converter is enabled
	//	Default charge termination is enabled, source mode is disabled, register reset is keep current settings, Iin limit loop is enabled, charge mode enabled
	//	No change from default
	SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_4, 0x3C53);

	// Charge Current
	//	Default charge current is 2A
	//	Change to 1300mA for single pack (standard charge current per datasheet) and 2650mA for double pack
	//	Note: Can chage to max charge current for fast charge
	if (GetExpandedBatteryPresenceState() == NO)
	{
		if (BATTERY_PACK_SINGLE_CAPACITY == 8000) { SetBattChargerRegister(BATT_CHARGER_CHARGE_CURRENT_SETTING, 0x0800); } // Single pack (larger), 1600mA
		else { SetBattChargerRegister(BATT_CHARGER_CHARGE_CURRENT_SETTING, 0x0680); } // Single pack, 1300mA
	}
	else
	{
		if (BATTERY_PACK_DOUBLE_CAPACITY == 16000) { SetBattChargerRegister(BATT_CHARGER_CHARGE_CURRENT_SETTING, 0x1000); } // Double pack (larger), 3200mA
		else { SetBattChargerRegister(BATT_CHARGER_CHARGE_CURRENT_SETTING, 0x0D40); } // Double pack, 2650mA
	}

	// Batt Reg V
	//	Default charge full voltage is 8.4V
	// 	Change to 7.2V (just under max charge voltage to help prolong battery life)
	SetBattChargerRegister(BATT_CHARGER_BATTERY_REGULATION_VOLTAGE_SETTING, 0x2D00); // 7.2V

	// Int Mask setting
	//	Default all INT masked
	//	Change all to unmasked
	SetBattChargerRegister(BATT_CHARGER_INT_MASK_SETTING_REGISTER_0, 0x3CFF);

	// Int Mask setting
	//	Default all INT masked
	//	Change all to unmasked
	SetBattChargerRegister(BATT_CHARGER_INT_MASK_SETTING_REGISTER_1, 0x0003);

#if 1 /* Test */
	uint16_t regResults, errStatus = NO;
	// Device address doesn't read as expected
	//GetBattChargerRegister(BATT_CHARGER_DEVICE_ADDRESS_SETTING, &regResults); if (regResults != 0x00E9) { debugErr("Battery Charger Read failed: Device address, 0x%x/0x%x\r\n", 0x00E9, regResults); }
	GetBattChargerRegister(BATT_CHARGER_INPUT_MINIMUM_VOLTAGE_LIMIT_SETTING, &regResults); if (regResults != 0x0037) { errStatus = YES; debugErr("Battery Charger Read failed: Input min V, 0x%x/0x%x\r\n", 0x0037, regResults); }
	GetBattChargerRegister(BATT_CHARGER_INPUT_CURRENT_LIMIT_SETTING, &regResults); if (regResults != 0x0050) {errStatus = YES; debugErr("Battery Charger Read failed: Input current limit, 0x%x/0x%x\r\n", 0x0050, regResults); }
	GetBattChargerRegister(BATT_CHARGER_OUTPUT_VOLTAGE_SETTING_IN_SOURCE_MODE, &regResults); if (regResults != 0x00F9) { errStatus = YES; debugErr("Battery Charger Read failed: Output voltage, 0x%x/0x%x\r\n", 0x00F9, regResults); }
	GetBattChargerRegister(BATT_CHARGER_BATTERY_IMPEDANCE_COMPENSATION_AND_OUTPUT_CURRENT_LIMIT_SETTING_IN_SOURCE_MODE, &regResults); if (regResults != 0x003C) { errStatus = YES; debugErr("Battery Charger Read failed: Output current, 0x%x/0x%x\r\n", 0x003C, regResults); }
#if 1 /* Test delay to see if that changes expanded Batt presenece to read different */
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(500));
#endif
	if (GetExpandedBatteryPresenceState() == NO)
	{
		debug("Battery Charger: Batt discharge in source validate for Single pack\r\n");
		GetBattChargerRegister(BATT_CHARGER_BATTERY_LOW_VOLTAGE_THRESHOLD_AND_BATTERY_DISCHARGE_CURRENT_REGULATION_IN_SOURCE_MODE, &regResults); if (regResults != 0x303C) { errStatus = YES; debugErr("Battery Charger Read failed: Batt discharge source, 0x%x/0x%x\r\n", 0x303C, regResults); }
	}
	else { debug("Battery Charger: Batt discharge in source validate for Dual pack\r\n"); GetBattChargerRegister(BATT_CHARGER_BATTERY_LOW_VOLTAGE_THRESHOLD_AND_BATTERY_DISCHARGE_CURRENT_REGULATION_IN_SOURCE_MODE, &regResults); if (regResults != 0x3078) { errStatus = YES; debugErr("Battery Charger Read failed: Batt discharge source, 0x%x/0x%x\r\n", 0x3078, regResults); } }
	GetBattChargerRegister(BATT_CHARGER_JEITA_ACTION_SETTING, &regResults); if (regResults != 0x7C10) { errStatus = YES; debugErr("Battery Charger Read failed: JEITA, 0x%x/0x%x\r\n", 0x7C10, regResults); }
	GetBattChargerRegister(BATT_CHARGER_TEMPERATURE_PROTECTION_SETTING, &regResults); if (regResults != 0xBF99) { errStatus = YES; debugErr("Battery Charger Read failed: Temp protect, 0x%x/0x%x\r\n", 0xBF99, regResults); }
	GetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_0, &regResults); if ((regResults & 0x00FF) != 0x0010) { errStatus = YES; debugErr("Battery Charger Read failed: config reg 0, 0x%x/0x%x\r\n", 0x0010, (regResults & 0x00FF)); } // Filter for ADC conversion flag in case Continuous conversion mode enabled from prior run
	if (GetExpandedBatteryPresenceState() == NO)
	{
		if (BATTERY_PACK_SINGLE_CAPACITY == 8000) { GetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_1, &regResults); if (regResults != 0xF285) { errStatus = YES; debugErr("Battery Charger Read failed: Config reg 1, 0x%x/0x%x\r\n", 0xF285, regResults); } }
		else { GetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_1, &regResults); if (regResults != 0xF264) { errStatus = YES; debugErr("Battery Charger Read failed: Config reg 1, 0x%x/0x%x\r\n", 0xF264, regResults); } }
	}
	else
	{
		if (BATTERY_PACK_DOUBLE_CAPACITY == 16000) { GetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_1, &regResults); if (regResults != 0xF2F9) { errStatus = YES; debugErr("Battery Charger Read failed: Config reg 1, 0x%x/0x%x\r\n", 0xF2F9, regResults); } }
		else { GetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_1, &regResults); if (regResults != 0xF2C8) { errStatus = YES; debugErr("Battery Charger Read failed: Config reg 1, 0x%x/0x%x\r\n", 0xF2C8, regResults); } }
	}
	GetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_2, &regResults); if (regResults != 0x0A00) { errStatus = YES; debugErr("Battery Charger Read failed: Config reg 2, 0x%x/0x%x\r\n", 0x0A00, regResults); }
	GetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_3, &regResults); if (regResults != 0x60E8) { errStatus = YES; debugErr("Battery Charger Read failed: Config reg 3, 0x%x/0x%x\r\n", 0x60E8, regResults); }
	GetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_4, &regResults); if (regResults != 0x3C53) { errStatus = YES; debugErr("Battery Charger Read failed: Config reg 4, 0x%x/0x%x\r\n", 0x3C53, regResults); }
	if (GetExpandedBatteryPresenceState() == NO)
	{
		if (BATTERY_PACK_SINGLE_CAPACITY == 8000) { GetBattChargerRegister(BATT_CHARGER_CHARGE_CURRENT_SETTING, &regResults); if (regResults != 0x0800) { errStatus = YES; debugErr("Battery Charger Read failed: Charge current, 0x%x/0x%x\r\n", 0x0800, regResults); } }
		else { GetBattChargerRegister(BATT_CHARGER_CHARGE_CURRENT_SETTING, &regResults); if (regResults != 0x0680) { errStatus = YES; debugErr("Battery Charger Read failed: Charge current, 0x%x/0x%x\r\n", 0x0680, regResults); } }
	}
	else
	{
		if (BATTERY_PACK_DOUBLE_CAPACITY == 16000) { GetBattChargerRegister(BATT_CHARGER_CHARGE_CURRENT_SETTING, &regResults); if (regResults != 0x1000) { errStatus = YES; debugErr("Battery Charger Read failed: Charge current, 0x%x/0x%x\r\n", 0x1000, regResults); } }
		else { GetBattChargerRegister(BATT_CHARGER_CHARGE_CURRENT_SETTING, &regResults); if (regResults != 0x0D40) { errStatus = YES; debugErr("Battery Charger Read failed: Charge current, 0x%x/0x%x\r\n", 0x0D40, regResults); } }
	}
	GetBattChargerRegister(BATT_CHARGER_BATTERY_REGULATION_VOLTAGE_SETTING, &regResults); if (regResults != 0x2D00) { errStatus = YES; debugErr("Battery Charger Read failed: Batt regulation, 0x%x/0x%x\r\n", 0x2D00, regResults); }
	GetBattChargerRegister(BATT_CHARGER_INT_MASK_SETTING_REGISTER_0, &regResults); if (regResults != 0x3CFF) { errStatus = YES; debugErr("Battery Charger Read failed: Int mask reg 0, 0x%x/0x%x\r\n", 0x3CFF, regResults); }
	GetBattChargerRegister(BATT_CHARGER_INT_MASK_SETTING_REGISTER_1, &regResults); if (regResults != 0x0003) { errStatus = YES; debugErr("Battery Charger Read failed: Int mask reg 1, 0x%x/0x%x\r\n", 0x0003, regResults); }

	if (errStatus == NO) { debug("Battery Charger: Configure registers written and verified\r\n"); }
#endif
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

	GetBattChargerRegister(BATT_CHARGER_ADC_RESULT_OF_THE_JUNCTION_TEMPERATURE, &registerValue);

	// Temperature conversion equation (in C)
	result = (314 - (0.5703 * registerValue));

	// Convert to Fahrenheit
	result = ((((result * 9) / 5)) + 32);

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
void SetBattChargerChargeState(uint8_t state)
{
	debug("Battery Charger: Charging control %s\r\n", ((state & 0x01) ? "enabled" : "disabled"));
	SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_4, (0x3C52 | (state & 0x01)));
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

#if 0 /* Test proving that setting the ADC Conversion mode changes the state of the Expanded Battery Presence GPIO line */
	debug("Battery Charger: Battery presence state is %s\r\n", ((GetExpandedBatteryPresenceState() == YES) ? "Dual" : "Single"));
	debug("Battery Charger: Write Config Reg 0 Conversion state test for Expanded battery line change...\r\n");

	debug("Battery Charger: ADC Conv Off\r\n"); SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_0, 0x0010);
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(2));
	debug("Battery Charger: ADC Conv On\r\n"); SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_0, 0x0090);
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(2));

	debug("Battery Charger: ADC Conv Off\r\n"); SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_0, 0x0010);
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(2));
	debug("Battery Charger: ADC Conv On\r\n"); SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_0, 0x0090);
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(2));

	debug("Battery Charger: ADC Conv Off\r\n"); SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_0, 0x0010);
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(2));
	debug("Battery Charger: ADC Conv On\r\n"); SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_0, 0x0090);
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(2));

	debug("Battery Charger: Test One-Shot conversions for Expanded battery line change...\r\n");
	debug("Battery Charger: ADC Conv One-Shot\r\n"); SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_0, 0x0110);
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(2));
	debug("Battery Charger: ADC Conv One-Shot\r\n"); SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_0, 0x0110);
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(2));
	debug("Battery Charger: ADC Conv One-Shot\r\n"); SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_0, 0x0110);
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(2));
#endif
	debug("Battery Charger: Config Reg 0 reads: 0x%x\r\n", ReturnBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_0));

	InitBattChargerRegisters();

	// Set Continuous mode for ADC_CONV
	SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_0, 0x0090);

	debug("Battery Charger: Status 0 is %04x, Status 1 is %04x\r\n", GetBattChargerStatusReg0(), GetBattChargerStatusReg1());

#if 0 /* Test PG BC line with interrupt */
	debug("Testing BC PG line...\r\n");
	while(1) {}
#endif
#if 0 /* Test interrupt pin, verified line going low triggers the MCU int, but can't get the part to issue an int */
	uint16_t readReg;
	uint8_t nullReg;
	while (1)
	{
		// Enable Watchdog for 40s and reset WD
		SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_4, 0x3E93);
		debug("Battery Charger: Status/Fault Reg 0 after clear reads: 0x%x\r\n", ReturnBattChargerRegister(BATT_CHARGER_STATUS_AND_FAULT_REGISTER_0));
		//debug("Battery Charger: Status/Fault Reg 1 after clear reads: 0x%x\r\n", ReturnBattChargerRegister(BATT_CHARGER_STATUS_AND_FAULT_REGISTER_1));
		for (uint8_t i = 0; i < 9; i++) { MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(5)); debug("Battery Charger: Delay...\r\n"); }
		// Read int status reg 16 and 17
		//debug("Battery Charger: Status/Fault Reg 0 reads: 0x%x\r\n", ReturnBattChargerRegister(BATT_CHARGER_STATUS_AND_FAULT_REGISTER_0));
		readReg = ReturnBattChargerRegister(BATT_CHARGER_STATUS_AND_FAULT_REGISTER_0);
		debug("Battery Charger: Status/Fault Reg 0 reads: 0x%x\r\n", readReg);
		//readReg = ReturnBattChargerRegister(BATT_CHARGER_STATUS_AND_FAULT_REGISTER_1);
		//debug("Battery Charger: Status/Fault Reg 1 reads: 0x%x\r\n", readReg);
		if (readReg)
		{
			SetBattChargerRegister(BATT_CHARGER_CONFIGURATION_REGISTER_4, 0x3ED3);
			debug("Battery Charger: Interrupt found, clearing...\r\n");
			//WriteI2CDevice(MXC_I2C0, (0x18 >> 1), &writeReg, 1, NULL, 0); }
			//WriteI2CDevice(MXC_I2C0, (0x18 >> 1), NULL, 0, NULL, 0);
			WriteI2CDevice(MXC_I2C0, (0x18 >> 0), NULL, 0, NULL, 0);
			//WriteI2CDevice(MXC_I2C0, (0x18 >> 1), NULL, 0, &nullReg, 1);
			WriteI2CDevice(MXC_I2C0, (0x18 >> 0), NULL, 0, &nullReg, 1);
			//WriteI2CDevice(MXC_I2C0, (0x18 >> 1), NULL, 0, (uint8_t*)&readReg, 2);
			WriteI2CDevice(MXC_I2C0, (0x18 >> 0), NULL, 0, (uint8_t*)&readReg, 2);
		}
	}
#endif
}

///============================================================================
///----------------------------------------------------------------------------
///	Fuel Guage - Ltc2944
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
 * I2C client/driver for the Linear Technology Ltc2944 Battery Gas Gauge IC
 * Copyright (C) 2014 Topic Embedded Systems
 * Author: Auryn Verwegen
 * Author: Mike Looijmans
 */

#define I16_MSB(x)			((x >> 8) & 0xFF)
#define I16_LSB(x)			(x & 0xFF)

//#define Ltc2944_WORK_DELAY		10	/* Update delay in seconds */

#define LTC2944_MAX_VALUE		0xFFFF
#define LTC2944_MID_SUPPLY		0x7FFF

enum LTC2944_REG {
	LTC2944_REG_STATUS					= 0x00, // Reg A, as named in datasheet
	LTC2944_REG_CONTROL					= 0x01, // Reg B
	LTC2944_REG_ACC_CHARGE_MSB			= 0x02, // Reg C
	LTC2944_REG_ACC_CHARGE_LSB			= 0x03, // Reg D
	LTC2944_REG_CHARGE_THR_HIGH_MSB		= 0x04, // Reg E
	LTC2944_REG_CHARGE_THR_HIGH_LSB		= 0x05, // Reg F
	LTC2944_REG_CHARGE_THR_LOW_MSB		= 0x06, // Reg G
	LTC2944_REG_CHARGE_THR_LOW_LSB		= 0x07, // Reg H
	LTC2944_REG_VOLTAGE_MSB				= 0x08, // Reg I
	LTC2944_REG_VOLTAGE_LSB				= 0x09, // Reg J
	LTC2944_REG_VOLTAGE_THR_HIGH_MSB 	= 0x0A, // Reg K
	LTC2944_REG_VOLTAGE_THR_HIGH_LSB 	= 0x0B, // Reg L
	LTC2944_REG_VOLTAGE_THR_LOW_MSB 	= 0x0C, // Reg M
	LTC2944_REG_VOLTAGE_THR_LOW_LSB 	= 0x0D, // Reg N
	LTC2944_REG_CURRENT_MSB				= 0x0E, // Reg O
	LTC2944_REG_CURRENT_LSB				= 0x0F, // Reg P
	LTC2944_REG_CURRENT_THR_HIGH_MSB	= 0x10, // Reg Q
	LTC2944_REG_CURRENT_THR_HIGH_LSB	= 0x11, // Reg R
	LTC2944_REG_CURRENT_THR_LOW_MSB		= 0x12, // Reg S
	LTC2944_REG_CURRENT_THR_LOW_LSB		= 0x13, // Reg T
	LTC2944_REG_TEMPERATURE_MSB			= 0x14, // Reg U
	LTC2944_REG_TEMPERATURE_LSB			= 0x15, // Reg V
	LTC2944_REG_TEMPERATURE_THR_HIGH 	= 0x16, // Reg W
	LTC2944_REG_TEMPERATURE_THR_LOW 	= 0x17 	// Reg X
};

#define LTC2944_REG_CONTROL_ADC_MODE_MASK	((1 << 7) | (1 << 6))
#define LTC2944_REG_CONTROL_MODE_AUTO	((1 << 7) | (1 << 6))
#define LTC2944_REG_CONTROL_MODE_SCAN	(1 << 7)
#define LTC2944_REG_CONTROL_MODE_MANUAL	(1 << 6)
#define LTC2944_REG_CONTROL_MODE_SLEEP	(0)
#define LTC2944_REG_CONTROL_PRESCALER_MASK	((1 << 5) | (1 << 4) | (1 << 3))
#define LTC2944_REG_CONTROL_SHUTDOWN_MASK	((1 << 0))
#define LTC2944_REG_CONTROL_PRESCALER_SET(x)	((x << 3) & LTC2944_REG_CONTROL_PRESCALER_MASK)
#define LTC2944_REG_CONTROL_ALCC_CONFIG_DISABLED	0
#define LTC2944_REG_CONTROL_ALCC_CONFIG_ALERT_MODE	((0x10) << 1)
#define LTC2944_REG_CONTROL_ADC_DISABLE(x)	((x) & ~((1 << 7) | (1 << 6)))

#define LTC2944_PRESCALER_1		0x0
#define LTC2944_PRESCALER_4		0x1
#define LTC2944_PRESCALER_16	0x2
#define LTC2944_PRESCALER_64	0x3
#define LTC2944_PRESCALER_256	0x4
#define LTC2944_PRESCALER_1024	0x5
#define LTC2944_PRESCALER_4096	0x6

#define LTC2944_M_1		1
#define LTC2944_M_4		4
#define LTC2944_M_16	16
#define LTC2944_M_64	64
#define LTC2944_M_256	256
#define LTC2944_M_1024	1024
#define LTC2944_M_4096	4096
#define LTC2944_MAX_PRESCALER	4096

typedef struct {
	int charge; // Last charge register content
	int r_sense; // mOhm
	int prescaler; // M value between 1 and 4096
	int Qlsb; // nAh
} Ltc2944_info;
Ltc2944_info Ltc2944_device;

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
	return ((Q < LTC2944_MAX_VALUE) ? Q : LTC2944_MAX_VALUE);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int Ltc2944_read_regs(enum LTC2944_REG reg, uint8_t *buf, int bufSize)
{
	return (WriteI2CDevice(MXC_I2C1, I2C_ADDR_FUEL_GAUGE, &reg, sizeof(uint8_t), buf, bufSize));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int Ltc2944_write_regs(enum LTC2944_REG reg, const uint8_t *buf, int bufSize)
{
	g_spareBuffer[0] = reg;
	memcpy(&g_spareBuffer[1], buf, bufSize);

	return (WriteI2CDevice(MXC_I2C1, I2C_ADDR_FUEL_GAUGE, g_spareBuffer, (bufSize + 1), NULL, 0));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t Ltc2944_get_status(void)
{
	uint8_t statusReg;
	
	WriteI2CDevice(MXC_I2C1, I2C_ADDR_FUEL_GAUGE, LTC2944_REG_STATUS, sizeof(uint8_t), &statusReg, sizeof(uint8_t));

	return (statusReg);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int Ltc2944_config(int prescaler_exp, uint8_t adcMode)
{
	int ret;
	uint8_t value;
	uint8_t control;

	// Read control register
	ret = Ltc2944_read_regs(LTC2944_REG_CONTROL, &value, 1);
	if (ret < 0) { return ret; }

	// Set Prescaler and ALCC alert
	control = LTC2944_REG_CONTROL_PRESCALER_SET(prescaler_exp) | LTC2944_REG_CONTROL_ALCC_CONFIG_ALERT_MODE;

	// Set ADC mode, Automatic = Continuous conversions (more power), Scan = Conversions every 10 sec (less power)
	control &= ~LTC2944_REG_CONTROL_ADC_MODE_MASK;
	control |= adcMode;

	control &= ~LTC2944_REG_CONTROL_SHUTDOWN_MASK; // Start up fuel gauge monitoring

	if (value != control)
	{
		ret = Ltc2944_write_regs(LTC2944_REG_CONTROL, &control, 1);
		if (ret < 0) { return ret; }
	}

#if 1 /* Test */
	Ltc2944_read_regs(LTC2944_REG_CONTROL, &value, 1);
	if (value != control) { debugErr("Fuel Gauge: Control register write failed verification (0x%x, 0x%x)\r\n", control, value);}
	else { debug("Fuel Gauge: Control register write verified\r\n"); }
#endif

	return (0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16_t Ltc2944_read_register_value_x16(enum LTC2944_REG reg)
 {
	int ret;
	uint8_t datar[2];

	ret = Ltc2944_read_regs(reg, &datar[0], 2);

	if (ret < 0) { return ret; }
	return ((datar[0] << 8) | datar[1]);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int Ltc2944_read_charge_register(enum LTC2944_REG reg)
 {
	int ret;
	uint8_t datar[2];

	ret = Ltc2944_read_regs(reg, &datar[0], 2);

	if (ret < 0) { return ret; }
	return ((datar[0] << 8) + datar[1]);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int Ltc2944_get_charge(int Qlsb, enum LTC2944_REG reg, int* val)
{
	int value = Ltc2944_read_charge_register(reg);

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
int Ltc2944_set_charge_now_uAh(Ltc2944_info* info, int val)
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
	ret = Ltc2944_read_regs(LTC2944_REG_CONTROL, &ctrl_reg, 1);
	if (ret < 0) { return ret; }

	// Disable analog section
	ctrl_reg |= LTC2944_REG_CONTROL_SHUTDOWN_MASK;
	ret = Ltc2944_write_regs(LTC2944_REG_CONTROL, &ctrl_reg, 1);
	if (ret < 0) { return ret; }

	// Set new charge value
	dataw[0] = I16_MSB(value);
	dataw[1] = I16_LSB(value);
	ret = Ltc2944_write_regs(LTC2944_REG_ACC_CHARGE_MSB, &dataw[0], 2);
	if (ret < 0) { goto error_exit; }
	// Enable analog section

error_exit:
	ctrl_reg &= ~LTC2944_REG_CONTROL_SHUTDOWN_MASK;
	ret = Ltc2944_write_regs(LTC2944_REG_CONTROL, &ctrl_reg, 1);

	return ((ret < 0) ? ret : 0);
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int Ltc2944_set_charge_now(int Qlsb, int val)
{
	int ret;
	uint8_t dataw[2];
	uint8_t ctrl_reg;
	uint32_t value;

	value = (uint32_t)(((float)val * 1000 * 1000) / Qlsb);
	//debug("Fuel Gauge: Charge now value is %d\r\n", value);
	if ((value < 0) || (value > 0xFFFF)) { debugErr("Fuel Gauge: Charge now invalid\r\n"); return E_INVALID; } // Input validation

	// Read control register
	ret = Ltc2944_read_regs(LTC2944_REG_CONTROL, &ctrl_reg, 1);
	if (ret < 0) { return ret; }

	// Disable analog section
	ctrl_reg |= LTC2944_REG_CONTROL_SHUTDOWN_MASK;
	ret = Ltc2944_write_regs(LTC2944_REG_CONTROL, &ctrl_reg, 1);
	if (ret < 0) { return ret; }

	// Set new charge value
	dataw[0] = I16_MSB(value);
	dataw[1] = I16_LSB(value);
	ret = Ltc2944_write_regs(LTC2944_REG_ACC_CHARGE_MSB, &dataw[0], 2);
	//debug("Fuel Gauge: Charge now reg is 0x%x / 0x%x\r\n", dataw[0], dataw[1]);

	// Enable analog section
	ctrl_reg &= ~LTC2944_REG_CONTROL_SHUTDOWN_MASK;
	ret = Ltc2944_write_regs(LTC2944_REG_CONTROL, &ctrl_reg, 1);

	return ((ret < 0) ? ret : 0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* reference function, bug discovered; write should be 4 */
int Ltc2944_set_charge_thr_uAh(Ltc2944_info* info, int thrHigh, int thrLow)
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

	return (Ltc2944_write_regs(LTC2944_REG_CHARGE_THR_HIGH_MSB, &dataWrite[0], 2));
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int Ltc2944_set_charge_thr(int Qlsb, int thrHigh, int thrLow)
{
	uint8_t dataWrite[4];
	uint32_t chargeThrHigh;
	uint32_t chargeThrLow;

	// Set the charge ADC value by dividing by the value represented by 1 bit
	chargeThrHigh = (uint32_t)(((float)thrHigh * 1000 * 1000) / Qlsb);
	if ((chargeThrHigh < 0) || (chargeThrHigh > 0xFFFF)) { debugErr("Fuel Gauge: Charge Thr High invalid\r\n"); return E_INVALID; } // input validation

	// Set the charge ADC value by dividing by the value represented by 1 bit
	chargeThrLow = (uint32_t)(((float)thrLow * 1000 * 1000) / Qlsb);
	if ((chargeThrLow < 0) || (chargeThrLow > 0xFFFF)) { debugErr("Fuel Gauge: Charge Thr Low invalid\r\n"); return E_INVALID; } // input validation

	// Set new charge value
	dataWrite[0] = I16_MSB(chargeThrHigh);
	dataWrite[1] = I16_LSB(chargeThrHigh);
	dataWrite[2] = I16_MSB(chargeThrLow);
	dataWrite[3] = I16_LSB(chargeThrLow);
	//debug("Fuel Gauge: Charge Thr reg 0x%x 0x%x / 0x%x 0x%x\r\n", dataWrite[0], dataWrite[1], dataWrite[2], dataWrite[3]);

	return (Ltc2944_write_regs(LTC2944_REG_CHARGE_THR_HIGH_MSB, &dataWrite[0], 4));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int Ltc2944_get_charge_counter(int Qlsb, int* val)
{
	int value = Ltc2944_read_charge_register(LTC2944_REG_ACC_CHARGE_MSB);

	if (value < 0) { return value; }

	value -= LTC2944_MID_SUPPLY;
	*val = convert_bin_to_uAh(Qlsb, value);

	return (0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int Ltc2944_get_voltage(int* val)
{
	int ret;
	uint8_t datar[2];
	uint64_t voltage; // Need storage above uint32 size if calc done in mV

	ret = Ltc2944_read_regs(LTC2944_REG_VOLTAGE_MSB, &datar[0], 2);
	voltage = ((datar[0] << 8) | datar[1]);

	//debug("Fuel Gauge: Get voltage register is 0x%x\r\n", voltage);

	voltage = ((voltage * 70800) / 0xFFFF); // units in mV

	//debug("Fuel Gauge: Voltage calc is %lu\r\n", voltage);

	*val = (int)voltage;
	return (ret);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int Ltc2944_set_voltage_thr(int vHigh, int vLow)
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

	return (Ltc2944_write_regs(LTC2944_REG_VOLTAGE_THR_HIGH_MSB, &dataWrite[0], sizeof(dataWrite)));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int Ltc2944_get_current(int r_sense, int* val)
{
	int ret;
	uint8_t datar[2];
	int64_t value;

	ret = Ltc2944_read_regs(LTC2944_REG_CURRENT_MSB, &datar[0], 2);
	value = ((datar[0] << 8) | datar[1]);
	value -= 0x7FFF;
	value *= 64000;

	// Value is in range -32k..+32k, r_sense is usually 10..50 mOhm, the formula below keeps everything in s32 range while preserving enough digits
	*val = (int)((1000 * value) / (r_sense * 0x7FFF)); // Units in uA

	return (ret);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int Ltc2944_set_current_thr_same(int r_sense, int currThr)
{
	uint8_t dataWrite[4];
	int16_t currThrHigh;
	int16_t currThrLow;

	// Current in mA converted to A for equation
	currThrHigh = ((((currThr / 1000) * r_sense / 64) * 0x7FFF) + 0x7FFF);
	currThrLow = ((((-currThr / 1000) * r_sense / 64) * 0x7FFF) + 0x7FFF);

	// Set new charge value
	dataWrite[0] = I16_MSB(currThrHigh);
	dataWrite[1] = I16_LSB(currThrHigh);
	dataWrite[2] = I16_MSB(currThrLow);
	dataWrite[3] = I16_LSB(currThrLow);

	return (Ltc2944_write_regs(LTC2944_REG_CURRENT_THR_HIGH_MSB, &dataWrite[0], sizeof(dataWrite)));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int Ltc2944_set_current_thr(int r_sense, int currThrCharging, int currThrDischarging)
{
	uint8_t dataWrite[4];
	uint16_t currThrHigh;
	uint16_t currThrLow;

	// Current in mA converted to A for equation
#if 0 /* Original */
	currThrHigh = ((((currThrCharging / 1000) * r_sense * 0x7FFF) / 64) + 0x7FFF);
	currThrLow = ((((-currThrDischarging / 1000) * r_sense * 0x7FFF) / 64) + 0x7FFF);
#else /* Prevent small number integer divison leading to zero */
	currThrHigh = ((((currThrCharging * r_sense * 0x7FFF) / 1000) / 64) + 0x7FFF);
	currThrLow = ((((-currThrDischarging * r_sense * 0x7FFF) / 1000) / 64) + 0x7FFF);
#endif

	// Set new charge value
	dataWrite[0] = I16_MSB(currThrHigh);
	dataWrite[1] = I16_LSB(currThrHigh);
	dataWrite[2] = I16_MSB(currThrLow);
	dataWrite[3] = I16_LSB(currThrLow);

#if 0 /* Test */
	debug("Fuel Gauge: Set Curent Thr, High Thr calc is 0x%x, Low Thr calc is 0x%x\r\n", currThrHigh, currThrLow);
	debug("Fuel Gauge: Set Curent Thr, High Thr reg: 0x%x / 0x%x, Low Thr reg: 0x%x / 0x%x\r\n", dataWrite[0], dataWrite[1], dataWrite[2], dataWrite[3]);
#endif

	return (Ltc2944_write_regs(LTC2944_REG_CURRENT_THR_HIGH_MSB, &dataWrite[0], sizeof(dataWrite)));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int Ltc2944_get_temperature(int* val)
{
	int ret;
	uint8_t datar[2];
	uint32_t value;

	ret = Ltc2944_read_regs(LTC2944_REG_TEMPERATURE_MSB, &datar[0], 2);
	value = ((datar[0] << 8) | datar[1]);

	// Convert to degree Celsius
	*val = (((value * 510) / 0xFFFF) - 273.15); // C = K - 273.15

	return (ret);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int Ltc2944_get_temperature_fahrenheit(int* val)
{
	int ret;
	uint8_t datar[2];
	uint32_t value;

	ret = Ltc2944_read_regs(LTC2944_REG_TEMPERATURE_MSB, &datar[0], 2);
	value = ((datar[0] << 8) | datar[1]);

	// Convert to F, F = (C * (9/5)) + 32
	*val = (int)((((((((float)value * 510) / 0xFFFF) - 273.15) * 9) / 5)) + 32);

	return (ret);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int Ltc2944_set_temp_thr(int tempHigh, int tempLow)
{
	uint8_t dataWrite[2];
	uint16_t tempHighThr;
	uint16_t tempLowThr;

	tempHighThr = (((tempHigh + 273.15) * 0xFFFF) / 510);
	tempLowThr = (((tempLow + 273.15) * 0xFFFF) / 510);

	// Set new charge value (only 11 bits in temp mode, can only set the top 8 MSB bits)
	dataWrite[0] = I16_MSB(tempHighThr);
	dataWrite[1] = I16_MSB(tempLowThr);

#if 0 /* Test */
	debug("Fuel Gauge: Set Temp Thr, High Thr calc is 0x%x, Low Thr calc is 0x%x\r\n", tempHighThr, tempLowThr);
	debug("Fuel Gauge: Set Temp Thr, High Thr reg: 0x%x, Low Thr reg: 0x%x\r\n", dataWrite[0], dataWrite[1]);
#endif

	return (Ltc2944_write_regs(LTC2944_REG_TEMPERATURE_THR_HIGH, &dataWrite[0], sizeof(dataWrite)));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int Ltc2944_set_temp_thr_fahrenheit(int tempHigh, int tempLow)
{
	uint8_t dataWrite[2];
	uint16_t tempHighThr;
	uint16_t tempLowThr;

	tempHighThr = ((((((tempHigh - 32) * 5) / 9) + 273.15) * 0xFFFF) / 510);
	tempLowThr = ((((((tempLow - 32) * 5) / 9) + 273.15) * 0xFFFF) / 510);

	// Set new charge value (only 11 bits in temp mode, can only set the top 8 MSB bits)
	dataWrite[0] = I16_MSB(tempHighThr);
	dataWrite[1] = I16_MSB(tempLowThr);

#if 0 /* Test */
	debug("Fuel Gauge: Set TempF Thr, High Thr calc is 0x%x, Low Thr calc is 0x%x\r\n", tempHighThr, tempLowThr);
	debug("Fuel Gauge: Set TempF Thr, High Thr reg: 0x%x, Low Thr reg: 0x%x\r\n", dataWrite[0], dataWrite[1]);
#endif

	return (Ltc2944_write_regs(LTC2944_REG_TEMPERATURE_THR_HIGH, &dataWrite[0], sizeof(dataWrite)));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Ltc2944_update(Ltc2944_info* info)
{
	int charge = Ltc2944_read_charge_register(LTC2944_REG_ACC_CHARGE_MSB);

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
int Ltc2944_i2c_probe(void)
{
	// Note: Unfinished and non-functional at the moment

	//struct power_supply_config psy_cfg = {};
	//Ltc2944_info* info;
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

	Ltc2944_device.r_sense = r_sense;

	prescaler_exp = LTC2944_PRESCALER_256;

	Ltc2944_device.Qlsb = (((((340 * 1000) * 50) / r_sense) * LTC2944_M_256) / LTC2944_MAX_PRESCALER); // nAh units, .340 scaled up to uA and * 1000 to scale up to nA

	//info->supply_desc.external_power_changed = NULL;

	//psy_cfg.drv_data = info;

	//ret = devm_delayed_work_autocancel(&info->work, Ltc2944_work);
	//if (ret) { return ret; }

	ret = Ltc2944_config(prescaler_exp, LTC2944_REG_CONTROL_MODE_SCAN);
	if (ret < 0) { return (ret); }

	//info->supply = devm_power_supply_register(&info->supply_desc, &psy_cfg);

	//schedule_delayed_work(&info->work, Ltc2944_WORK_DELAY * HZ);

	return 0;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Ltc2944_i2c_shutdown(void)
{
	int ret;
	uint8_t value;
	uint8_t control;

	// Read control register
	ret = Ltc2944_read_regs(LTC2944_REG_CONTROL, &value, 1);
	if (ret < 0) { return; }

	// Disable ADC conversion as this drains the battery, and place part into Shutdown
	control = (LTC2944_REG_CONTROL_ADC_DISABLE(value) | LTC2944_REG_CONTROL_SHUTDOWN_MASK);
	if (control != value) { Ltc2944_write_regs(LTC2944_REG_CONTROL, &control, 1); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestFuelGauge(void)
{
	Ltc2944_info info;
	int val;

	info.r_sense = 10;
	if (GetExpandedBatteryPresenceState() == NO)
	{
		if (BATTERY_PACK_SINGLE_CAPACITY < 6963) // Max capacity for prescaler 256
		{
			info.prescaler = LTC2944_PRESCALER_256; // Single pack
		}
		else { info.prescaler = LTC2944_PRESCALER_1024; } // Single pack but large enough to need the prescaler 1024
	}
	else { info.prescaler = LTC2944_PRESCALER_1024; } // Double pack
	info.Qlsb = (((((340 * 1000) * 50) / info.r_sense) * LTC2944_M_256) / LTC2944_MAX_PRESCALER); // nAh units, .340 scaled up to uA and * 1000 to scale up to nA

	debug("Fuel Gauge: Test device access...\r\n");

	debug("Fuel Gauge: Enabling conversions\r\n");
	Ltc2944_config(info.prescaler, LTC2944_REG_CONTROL_MODE_AUTO);

	debug("Fuel Gauge: Status reg is 0x%x\r\n", Ltc2944_get_status());

	Ltc2944_get_charge(info.Qlsb, LTC2944_REG_ACC_CHARGE_MSB, &val);
	debug("Fuel Gauge: Accumulated charge is %d (uA)\r\n", val);

	Ltc2944_get_voltage(&val);
	debug("Fuel Gauge: Voltage is %d (mV)\r\n", val);

	Ltc2944_get_current(info.r_sense, &val);
	debug("Fuel Gauge: Current is %d (uA)\r\n", val);

	Ltc2944_get_temperature_fahrenheit(&val);
	debug("Fuel Gauge: Temperature is %d (degrees F)\r\n", val);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void FuelGaugeDebugInfo(void)
{
	int vVal, cVal, tVal;
	Ltc2944_get_voltage(&vVal);
	Ltc2944_get_current(Ltc2944_device.r_sense, &cVal);
	Ltc2944_get_temperature_fahrenheit(&tVal);
	debug("Fuel Gauge: %0.3fV, %0.3fmA, %dF\r\n", (double)((float)vVal / 1000), (double)((float)cVal / 1000), tVal);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
char* FuelGaugeDebugString(void)
{
	int vVal, cVal, tVal, bc_tVal;
	Ltc2944_get_voltage(&vVal);
	Ltc2944_get_current(Ltc2944_device.r_sense, &cVal);
	Ltc2944_get_temperature_fahrenheit(&tVal);
	bc_tVal = GetBattChargerJunctionTemperature();
	sprintf((char*)g_debugBuffer, "Batt: %0.3fV, %0.3fmA, %dF, %dF", (double)((float)vVal / 1000), (double)((float)cVal / 1000), tVal, bc_tVal);
	return ((char*)g_debugBuffer);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int FuelGaugeGetVoltage(void)
{
	int vVal;
	Ltc2944_get_voltage(&vVal);
	return (vVal);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int FuelGaugeGetCurrent(void)
{
	int cVal;
	Ltc2944_get_current(Ltc2944_device.r_sense, &cVal);
	return (cVal);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int FuelGaugeGetCurrentAbs(void)
{
	int cVal;
	Ltc2944_get_current(Ltc2944_device.r_sense, &cVal);
	return (abs(cVal));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int FuelGaugeGetTemperature(void)
{
	int tVal;
	Ltc2944_get_temperature_fahrenheit(&tVal);
	return (tVal);
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

	// Note: Since IC power is supplied by the battery directly, this device isn't placed in reset on unit power down

	Ltc2944_device.r_sense = 10; // 10 mOhm showing on schematic as Rsense

	if (GetExpandedBatteryPresenceState() == NO)
	{
		debug("Fuel Gauge: Prescaler set for Single Pack\r\n");
		if (BATTERY_PACK_SINGLE_CAPACITY < 6963) // Max capacity for prescaler 256
		{
			Ltc2944_device.prescaler = LTC2944_PRESCALER_256; // Single pack
		}
		else { Ltc2944_device.prescaler = LTC2944_PRESCALER_1024; } // Single pack but large enough to need the prescaler 1024
	}
	else { debug("Fuel Gauge: Prescaler set for Double Pack\r\n"); Ltc2944_device.prescaler = LTC2944_PRESCALER_1024; } // Double pack, covers smaller and larger capacity packs

	Ltc2944_device.Qlsb = (((((340 * 1000) * 50) / Ltc2944_device.r_sense) * ((Ltc2944_device.prescaler == LTC2944_PRESCALER_256) ? 256 : 1024)) / LTC2944_MAX_PRESCALER); // nAh units, .340 scaled up to uA and * 1000 to scale up to nA
	debug("Fuel Gauge: Qlsb = %d nAh\r\n", Ltc2944_device.Qlsb);

	// Check if the Fuel Gauge has not been configured already (First time power applied by battery or battery replaced), otherwise bypass config if reset values have been changed
	if ((Ltc2944_read_register_value_x16(LTC2944_REG_CHARGE_THR_HIGH_MSB) == 0xFFFF) && (Ltc2944_read_register_value_x16(LTC2944_REG_CHARGE_THR_LOW_MSB) == 0))
	{
		debug("Fuel Gauge: First time configure\r\n");

		// Set the inital charge level based on half charge capacity (50%), function handles disabling the analog section for setting the charge and re-enabling when done
		if (GetExpandedBatteryPresenceState() == NO)
		{
			Ltc2944_set_charge_now(Ltc2944_device.Qlsb, (BATTERY_PACK_SINGLE_CAPACITY / 2)); // Single pack
		}
		else { Ltc2944_set_charge_now(Ltc2944_device.Qlsb, (BATTERY_PACK_DOUBLE_CAPACITY / 2)); } // Double pack

		// Set the Charge Thresold High and Low
		if (GetExpandedBatteryPresenceState() == NO)
		{
			Ltc2944_set_charge_thr(Ltc2944_device.Qlsb, BATTERY_PACK_SINGLE_CAPACITY, 0); // Single pack
		}
		else { Ltc2944_set_charge_thr(Ltc2944_device.Qlsb, BATTERY_PACK_DOUBLE_CAPACITY, 0); } // Double pack

		// Set the Voltage Thresold High and Low based on 7300mV max, 5400mV min
		Ltc2944_set_voltage_thr(BATTERY_VOLTAGE_THRESHOLD_HIGH, BATTERY_VOLTAGE_THRESHOLD_LOW);

		// Set the Current Thresold High and Low based on 3000mA
		if (GetExpandedBatteryPresenceState() == NO)
		{
			// Todo: Determine if charge/discharge current should be Standard 1320mA or Max continuous 3000mA
			Ltc2944_set_current_thr(Ltc2944_device.r_sense, BATTERY_PACK_SINGLE_MAX_CONTINUOUS_CURRENT, BATTERY_PACK_SINGLE_MAX_CONTINUOUS_CURRENT); // Single pack
		}
		// Todo: Determine if charge/discharge current should be Standard 2640mA or Max continuous 6000mA
		else { Ltc2944_set_current_thr(Ltc2944_device.r_sense, BATTERY_PACK_DOUBLE_MAX_CONTINUOUS_CURRENT, BATTERY_PACK_DOUBLE_MAX_CONTINUOUS_CURRENT); } // Double pack

		// Set thresholds for temperature (discharge range)
		Ltc2944_set_temp_thr(BATTERY_OPERATING_DISCHARGE_TEMP_HIGH, BATTERY_OPERATING_DISCHARGE_TEMP_LOW);
	}
	else { debug("Fuel Gauge: Appears to have been already configured a first time\r\n"); }

	debug("Fuel Gauge: Status reg is 0x%x\r\n", Ltc2944_get_status());
	debug("Fuel Gauge: Status reg is 0x%x (should be 0/cleared)\r\n", Ltc2944_get_status());

#if 0 /* Test with Scan mode */
	// Config the device with desired prescaler, ADC mode will be set to Scan and ALCC config set to Alert mode
	debug("Fuel Gauge: Enabling conversions (Scan mode)\r\n");
	Ltc2944_config(Ltc2944_device.prescaler, LTC2944_REG_CONTROL_MODE_SCAN);

	debug("Fuel Gauge: 10 second delay to allow scan mode to process...\r\n");
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(10));

	debug("Fuel Gauge: Battery voltage is %f\r\n", (double)GetExternalVoltageLevelAveraged(BATTERY_VOLTAGE));

#if 0 /* Verified reads are fine */
	uint8_t tempReg[2];
	Ltc2944_read_regs(LTC2944_REG_VOLTAGE_MSB, &tempReg[0], 2); debug("Fuel Gauge: Vh is 0x%x, Vl is 0x%x\r\n", tempReg[0], tempReg[1]);
	Ltc2944_read_regs(LTC2944_REG_CURRENT_MSB, &tempReg[0], 2); debug("Fuel Gauge: Ch is 0x%x, Cl is 0x%x\r\n", tempReg[0], tempReg[1]);
	Ltc2944_read_regs(LTC2944_REG_TEMPERATURE_MSB, &tempReg[0], 2); debug("Fuel Gauge: Th is 0x%x, Tl is 0x%x\r\n", tempReg[0], tempReg[1]);

	Ltc2944_read_regs(LTC2944_REG_VOLTAGE_MSB, &tempReg[0], 1); Ltc2944_read_regs(LTC2944_REG_VOLTAGE_LSB, &tempReg[1], 1); debug("Fuel Gauge: Vh is 0x%x, Vl is 0x%x\r\n", tempReg[0], tempReg[1]);
	Ltc2944_read_regs(LTC2944_REG_CURRENT_MSB, &tempReg[0], 1); Ltc2944_read_regs(LTC2944_REG_CURRENT_LSB, &tempReg[1], 1); debug("Fuel Gauge: Ch is 0x%x, Cl is 0x%x\r\n", tempReg[0], tempReg[1]);
	Ltc2944_read_regs(LTC2944_REG_TEMPERATURE_MSB, &tempReg[0], 1); Ltc2944_read_regs(LTC2944_REG_TEMPERATURE_LSB, &tempReg[1], 1); debug("Fuel Gauge: Th is 0x%x, Tl is 0x%x\r\n", tempReg[0], tempReg[1]);
#endif
#endif

#if 1 /* Test with Auto mode */
	int val;
	debug("Fuel Gauge: Enabling conversions (Auto mode)\r\n");
	Ltc2944_config(Ltc2944_device.prescaler, LTC2944_REG_CONTROL_MODE_AUTO);

	// Small delay to allow conversions
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(100));

	debug("Fuel Gauge: Battery voltage is %0.2fV\r\n", (double)GetExternalVoltageLevelAveraged(BATTERY_VOLTAGE));

#if 0 /* Verified reads are fine */
	uint8_t tempReg[2];
	Ltc2944_read_regs(LTC2944_REG_VOLTAGE_MSB, &tempReg[0], 2); debug("Fuel Gauge: Vh is 0x%x, Vl is 0x%x\r\n", tempReg[0], tempReg[1]);
	Ltc2944_read_regs(LTC2944_REG_CURRENT_MSB, &tempReg[0], 2); debug("Fuel Gauge: Ch is 0x%x, Cl is 0x%x\r\n", tempReg[0], tempReg[1]);
	Ltc2944_read_regs(LTC2944_REG_TEMPERATURE_MSB, &tempReg[0], 2); debug("Fuel Gauge: Th is 0x%x, Tl is 0x%x\r\n", tempReg[0], tempReg[1]);

	Ltc2944_read_regs(LTC2944_REG_VOLTAGE_MSB, &tempReg[0], 1); Ltc2944_read_regs(LTC2944_REG_VOLTAGE_LSB, &tempReg[1], 1); debug("Fuel Gauge: Vh is 0x%x, Vl is 0x%x\r\n", tempReg[0], tempReg[1]);
	Ltc2944_read_regs(LTC2944_REG_CURRENT_MSB, &tempReg[0], 1); Ltc2944_read_regs(LTC2944_REG_CURRENT_LSB, &tempReg[1], 1); debug("Fuel Gauge: Ch is 0x%x, Cl is 0x%x\r\n", tempReg[0], tempReg[1]);
	Ltc2944_read_regs(LTC2944_REG_TEMPERATURE_MSB, &tempReg[0], 1); Ltc2944_read_regs(LTC2944_REG_TEMPERATURE_LSB, &tempReg[1], 1); debug("Fuel Gauge: Th is 0x%x, Tl is 0x%x\r\n", tempReg[0], tempReg[1]);
#endif

#if 0 /* Read register map */
	uint8_t tempReg[2];
	Ltc2944_read_regs(LTC2944_REG_ACC_CHARGE_MSB, &tempReg[0], 2); debug("Fuel Gauge: Acc Charge is 0x%02x%02x\r\n", tempReg[0], tempReg[1]);
	Ltc2944_read_regs(LTC2944_REG_CHARGE_THR_HIGH_MSB, &tempReg[0], 2); debug("Fuel Gauge: Charge Thr High is 0x%02x%02x\r\n", tempReg[0], tempReg[1]);
	Ltc2944_read_regs(LTC2944_REG_CHARGE_THR_LOW_MSB, &tempReg[0], 2); debug("Fuel Gauge: Charge Thr Low is 0x%02x%02x\r\n", tempReg[0], tempReg[1]);
	Ltc2944_read_regs(LTC2944_REG_VOLTAGE_MSB, &tempReg[0], 2); debug("Fuel Gauge: Voltage is 0x%02x%02x\r\n", tempReg[0], tempReg[1]);
	Ltc2944_read_regs(LTC2944_REG_VOLTAGE_THR_HIGH_MSB, &tempReg[0], 2); debug("Fuel Gauge: Voltage Thr High is 0x%02x%02x\r\n", tempReg[0], tempReg[1]);
	Ltc2944_read_regs(LTC2944_REG_VOLTAGE_THR_LOW_MSB, &tempReg[0], 2); debug("Fuel Gauge: Voltage Thr Low is 0x%02x%02x\r\n", tempReg[0], tempReg[1]);
	Ltc2944_read_regs(LTC2944_REG_CURRENT_MSB, &tempReg[0], 2); debug("Fuel Gauge: Current is 0x%02x%02x\r\n", tempReg[0], tempReg[1]);
	Ltc2944_read_regs(LTC2944_REG_CURRENT_THR_HIGH_MSB, &tempReg[0], 2); debug("Fuel Gauge: Current Thx High is 0x%02x%02x\r\n", tempReg[0], tempReg[1]);
	Ltc2944_read_regs(LTC2944_REG_CURRENT_THR_LOW_MSB, &tempReg[0], 2); debug("Fuel Gauge: Current Thr Low is 0x%02x%02x\r\n", tempReg[0], tempReg[1]);
	Ltc2944_read_regs(LTC2944_REG_TEMPERATURE_MSB, &tempReg[0], 2); debug("Fuel Gauge: Temperature is 0x%02x%02x\r\n", tempReg[0], tempReg[1]);
	Ltc2944_read_regs(LTC2944_REG_TEMPERATURE_THR_HIGH, &tempReg[0], 2); debug("Fuel Gauge: Temperature Thr is 0x%02x%02x\r\n", tempReg[0], tempReg[1]);
#endif

	debug("Fuel Gauge: Status reg is 0x%x\r\n", Ltc2944_get_status());

	Ltc2944_get_charge(Ltc2944_device.Qlsb, LTC2944_REG_ACC_CHARGE_MSB, &val);
	debug("Fuel Gauge: Accumulated charge is %d (uA)\r\n", val);

	Ltc2944_get_voltage(&val);
	debug("Fuel Gauge: Voltage is %d (mV)\r\n", val);

	Ltc2944_get_current(Ltc2944_device.r_sense, &val);
	debug("Fuel Gauge: Current is %d (uA)\r\n", val);

	Ltc2944_get_temperature_fahrenheit(&val);
	debug("Fuel Gauge: Temperature is %d (degrees F)\r\n", val);

	debug("Fuel Gauge: Re-enabling conversions (Scan mode)\r\n");
	Ltc2944_config(Ltc2944_device.prescaler, LTC2944_REG_CONTROL_MODE_SCAN);
#endif
	FuelGaugeDebugInfo();

#if 0 /* Test Fuel Gauge ALCC interrupt pin */
	debug("Fuel Gauge: Re-enabling Auto mode\r\n");
	Ltc2944_config(Ltc2944_device.prescaler, LTC2944_REG_CONTROL_MODE_AUTO);

	uint8_t readReg, readReg2;
	debug("Fuel Gauge: Enabling Alert mode (ALCC pin)\r\n");
	uint8_t writeReg = 0xE4; Ltc2944_write_regs(LTC2944_REG_CONTROL, &writeReg, 1);

	Ltc2944_read_regs(LTC2944_REG_STATUS, &readReg, 1); debug("Fuel Gauge: Status register is 0x%x\r\n", readReg);

	while (1)
	{
		//debug("Fuel Gauge: Setting threshold values out of range to test ALCC interrupt\r\n");
		//Ltc2944_set_voltage_thr(5400, 5000);
		//Ltc2944_set_current_thr(Ltc2944_device.r_sense, 1, 1); // Single pack
		Ltc2944_set_temp_thr(0, -20);

		//Ltc2944_read_regs(LTC2944_REG_STATUS, &readReg, 1); debug("Fuel Gauge: Status register is 0x%x\r\n", readReg);
		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(50));
		Ltc2944_read_regs(LTC2944_REG_STATUS, &readReg, 1); //debug("Fuel Gauge: Status register is 0x%x\r\n", readReg);

		if (readReg)
		{
			// Special method to clear the alert response (interrupt) to allow the ALCC pin to continue to function
			WriteI2CDevice(MXC_I2C1, (0x18 >> 1), NULL, 0, &readReg, 1);

			// Follow up read no necessary, but is the following transaction ok?
			//WriteI2CDevice(MXC_I2C1, I2C_ADDR_FUEL_GAUGE, NULL, 0, &readReg, 1);
		}
		Ltc2944_read_regs(LTC2944_REG_CONTROL, &readReg, 1);
		Ltc2944_read_regs(LTC2944_REG_STATUS, &readReg2, 1);
		debug("Fuel Gauge: Control/Status register is 0x%x/0x%x\r\n", readReg, readReg2);

		//debug("Fuel Gauge: Setting threshold values in range\r\n");
		//Ltc2944_set_voltage_thr(7300, 5400);
		//Ltc2944_set_current_thr(Ltc2944_device.r_sense, 3000, 3000); // Single pack
		Ltc2944_set_temp_thr(60, -20);

		//Ltc2944_read_regs(LTC2944_REG_STATUS, &readReg, 1); debug("Fuel Gauge: Status register is 0x%x\r\n", readReg);
		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(50));
		Ltc2944_read_regs(LTC2944_REG_STATUS, &readReg, 1); //debug("Fuel Gauge: Status register is 0x%x\r\n", readReg);
	}

	//Ltc2944_read_regs(LTC2944_REG_CONTROL, &readReg, 1); debug("Fuel Gauge: Control register is 0x%x\r\n", readReg);
#endif
}
