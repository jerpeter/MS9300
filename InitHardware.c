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
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "board.h"
#include "mxc_errors.h"
#include "uart.h"
#include "nvic_table.h"
#include "icc.h"
#include "i2c.h"
#include "gpio.h"
#include "wdt.h"
#include "spi.h"
//#include "spi_reva1.h" // try without
// USB includes
#include "mxc_sys.h"
#include "usb.h"
#include "usb_event.h"
#include "enumerate.h"
#include "cdc_acm.h"
#include "msc.h"
#include "descriptors.h"
#include "mscmem.h"
#include "mxc_delay.h"
//#include "usb_protocol.h"
// SDHC includes
#include "mxc_device.h"
#include "mxc_sys.h"
#include "sdhc_regs.h"
#include "tmr.h"
#include "sdhc_lib.h"
#include "ff.h"
#include "sdhc_reva.h"
#include "sdhc_reva_regs.h"

//#include "mxc_delay.h"
//#include "mxc_errors.h"
//#include "uart.h"
//#include "gpio.h"

//#include "pm.h"
//#include "sdramc.h"
//#include "intc.h"
//#include "usart.h"
//#include "print_funcs.h"
#include "lcd.h"
#include "Typedefs.h"
#include "Common.h"
#include "Display.h"
#include "Menu.h"
#include "OldUart.h"
#include "ProcessBargraph.h"
#include "SysEvents.h"
#include "Record.h"
#include "InitDataBuffers.h"
#include "PowerManagement.h"
#include "SoftTimer.h"
#include "Keypad.h"
#include "RealTimeClock.h"
#include "RemoteHandler.h"
#include "TextTypes.h"
#include "RemoteCommon.h"
//#include "twi.h"
//#include "sd_mmc_spi.h"
#include "adc.h"
//#include "usb_task.h"
//#include "device_mass_storage_task.h"
//#include "usb_drv.h"
//#include "flashc.h"
#include "rtc.h"
#include "Sensor.h"
#include "NomisLogo.h"
//#include "navigation.h"
#include "Analog.h"

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

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void _init_startup(void)
{
	//-----------------------------------------------------------------
	// Setup/Enable system/peripheral clocks
	//-----------------------------------------------------------------
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DebugUartInitBanner(void)
{
	//debug("\r\n\n");
	debug("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\r\n");
	debug("---------------------------------------------------------------------------------------\r\n");
	debug("-----     MS9300 Debug port, App version: %s (Date: %s)     -----\r\n", (char*)g_buildVersion, (char*)g_buildDate);
	debug("---------------------------------------------------------------------------------------\r\n");
	debug("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\r\n");
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitExternalKeypad(void)
{
	uint8 keyScan = MXC_GPIO_InGet(REGULAR_BUTTONS_GPIO_PORT, REGULAR_BUTTONS_GPIO_MASK);
	if (keyScan)
	{
		debugWarn("Keypad button being pressed (likely a bug), Key: %x\r\n", keyScan);
	}

	// Todo: Find the right LED to light (1&2=Red?, 3&4=Green?)
	//PowerControl(LED_1, ON);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitInternalRTC(void)
{
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitInternalAD(void)
{
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitExternalAD(void)
{
	if (GetPowerControlState(ANALOG_5V_ENABLE) == OFF)
	{
		PowerUpAnalog5VandExternalADC();
	}
	else // Analog 5V already enabled
	{
		// Check if External ADC is still in reset and if so take out of reset
		if (GetPowerControlState(ADC_RESET) == ON) { WaitAnalogPower5vGood(); }

		// Configure External ADC
		AD4695_Init();
	}

	// Setup the A/D Channel configuration
	SetupADChannelConfig(SAMPLE_RATE_DEFAULT, UNIT_CONFIG_CHANNEL_VERIFICATION);

extern uint16_t dataTemperature;
	SAMPLE_DATA_STRUCT tempData;
	while (1)
	{
		ReadAnalogData(&tempData);
		debug("Ext ADC (Batt: %1.3f): R:%04x T:%04x V:%04x A:%04x TempF:%04x (%d)\r\n", (double)GetExternalVoltageLevelAveraged(BATTERY_VOLTAGE), tempData.r, tempData.t, tempData.v, tempData.a, dataTemperature, AD4695_TemperatureConversionCtoF(dataTemperature));
	}

	// Read a few test samples
	GetChannelOffsets(SAMPLE_RATE_DEFAULT);

	DisableSensorBlocks();
	PowerControl(ADC_RESET, ON);
	PowerControl(ANALOG_5V_ENABLE, OFF);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitLCD(void)
{
	// Power up and init the display controller
	ft81x_init();

	// Load the logo bitmap into the display controller and send to LCD
	DisplayLogoToLcd();
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestExternalRAM(void)
{
	uint32 i, j;
	uint32 index;
	uint32 printErrors = 0;
	uint32 testSize = 5120; // Was (EVENT_BUFF_SIZE_IN_WORDS) - 614400

#if EXTENDED_DEBUG
	debug("External RAM Test: Incrementing index with rolling increment...\r\n");
#endif

	for (i = 0, j = 0, index = 0; index < testSize; index++)
	{
		g_eventDataBuffer[index] = (uint16)(i + j); i++;
		if ((i & 0xFFFF) == 0) { j++; }
	}

	for (i = 0, j = 0, index = 0; index < testSize; index++)
	{
		if (g_eventDataBuffer[index] != (uint16)(i + j))
		{
			debugErr("Test of External RAM: failed (Index: %d, Address: 0x%x, Expected: 0x%x, Got: 0x%x)\r\n",
			index, &g_eventDataBuffer[index], (uint16)(i + j), g_eventDataBuffer[index]);
			printErrors++; if (printErrors > 5000) { debugErr("Too many errors, bailing on memory test\r\n"); return; }
		}
		i++;
		if ((i & 0xFFFF) == 0) { j++; }
	}

	if (printErrors) { debug("External RAM: Total errors: %d\r\n", printErrors); }
#if EXTENDED_DEBUG
	else { debug("Test of External RAM: passed\r\n"); }
#endif
}

void GaugeAlert_ISR(void *cbdata);
void BatteryCharger_ISR(void *cbdata);
void Expansion_ISR(void *cbdata);
void USBCI2C_ISR(void *cbdata);
void AccelInt1_ISR(void *cbdata);
void AccelInt2_ISR(void *cbdata);
void PowerButton_ISR(void *cbdata);
void Button1_ISR(void *cbdata);
void Button2_ISR(void *cbdata);
void Button3_ISR(void *cbdata);
void Button4_ISR(void *cbdata);
void Button5_ISR(void *cbdata);
void Button6_ISR(void *cbdata);
void Button7_ISR(void *cbdata);
void Button8_ISR(void *cbdata);
void Button9_ISR(void *cbdata);
void ExtRTCIntA_ISR(void *cbdata);
void ExternalTriggerIn_ISR(void *cbdata);
void LCD_ISR(void *cbdata);

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupPowerOnDetectGPIO(void)
{
	mxc_gpio_cfg_t setupGPIO;

	//----------------------------------------------------------------------------------------------------------------------
	// MCU Power Latch: Port 0, Pin 1, Output, External pulldown, Active high, 3.3V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_MCU_POWER_LATCH_PORT;
	setupGPIO.mask = GPIO_MCU_POWER_LATCH_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&setupGPIO);

	//----------------------------------------------------------------------------------------------------------------------
	// Power Good Battery Charge: Port 0, Pin 12, Input, External pullup, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_POWER_GOOD_BATTERY_CHARGE_PORT;
	setupGPIO.mask = GPIO_POWER_GOOD_BATTERY_CHARGE_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&setupGPIO);

	//----------------------------------------------------------------------------------------------------------------------
	// Power Button Int: Port 1, Pin 15, Input, External pullup, Active high, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_POWER_BUTTON_IRQ_PORT;
	setupGPIO.mask = GPIO_POWER_BUTTON_IRQ_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupAllGPIO(void)
{
	mxc_gpio_cfg_t setupGPIO;

	//----------------------------------------------------------------------------------------------------------------------
	// MCU Power Latch: Port 0, Pin 1, Output, External pulldown, Active high, 3.3V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_MCU_POWER_LATCH_PORT;
	setupGPIO.mask = GPIO_MCU_POWER_LATCH_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start enabled

	//----------------------------------------------------------------------------------------------------------------------
	// Expanded Battery Detect: Port 0, Pin 2, Input, External pullup, Active high, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_EXPANDED_BATTERY_PORT;
	setupGPIO.mask = GPIO_EXPANDED_BATTERY_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&setupGPIO);

	//----------------------------------------------------------------------------------------------------------------------
	// LED 1: Port 0, Pin 3, Output, No external pull, Active high, 3.3V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_LED_1_PORT;
	setupGPIO.mask = GPIO_LED_1_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as off

	//----------------------------------------------------------------------------------------------------------------------
	// Gauge Alert: Port 0, Pin 4, Input, External pullup, Active low, 1.8V, Interrupt
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_GAUGE_ALERT_PORT;
	setupGPIO.mask = GPIO_GAUGE_ALERT_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)Fuel_gauge_alert_irq, NULL);
    MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Battery Charger IRQ: Port 0, Pin 5, Input, Needs strong internal pullup, Active low, 1.8V, Interrupt
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_BATTERY_CHARGER_IRQ_PORT;
	setupGPIO.mask = GPIO_BATTERY_CHARGER_IRQ_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_STRONG_PULL_UP;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)Battery_charger_irq, NULL);
    MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Enable 12V = Port 0, Pin 6, Output, External pulldown, Active high, 1.8V (minimum 1.5V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_ENABLE_12V_PORT;
	setupGPIO.mask = GPIO_ENABLE_12V_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start disabled (only needed for alarms)

	//----------------------------------------------------------------------------------------------------------------------
	// Enable 5V: Port 0, Pin 7, Output, External pulldown, Active high, 1.8V (minimum 0.9V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_ENABLE_5V_PORT;
	setupGPIO.mask = GPIO_ENABLE_5V_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Expansion IRQ: Port 0, Pin 8, Input, External pullup, Active low, 3.3V (minimum 2V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_EXPANSION_IRQ_PORT;
	setupGPIO.mask = GPIO_EXPANSION_IRQ_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)Expansion_irq, NULL);
    MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// USB Source Enable: Port 0, Pin 9, Output, External pulldown, Active high, 1.8V (minimum 1.2V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_USB_SOURCE_ENABLE_PORT;
	setupGPIO.mask = GPIO_USB_SOURCE_ENABLE_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start disabled

	//----------------------------------------------------------------------------------------------------------------------
	// USB Aux Power Enable: Port 0, Pin 10, Output, External pulldown, Active high, 1.8V (minimum 0.6V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_USB_AUX_POWER_ENABLE_PORT;
	setupGPIO.mask = GPIO_USB_AUX_POWER_ENABLE_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Power Good 5v: Port 0, Pin 11, Input, External pullup, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_POWER_GOOD_5V_PORT;
	setupGPIO.mask = GPIO_POWER_GOOD_5V_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&setupGPIO);

	//----------------------------------------------------------------------------------------------------------------------
	// Power Good Battery Charge: Port 0, Pin 12, Input, External pullup, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_POWER_GOOD_BATTERY_CHARGE_PORT;
	setupGPIO.mask = GPIO_POWER_GOOD_BATTERY_CHARGE_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&setupGPIO);

	//----------------------------------------------------------------------------------------------------------------------
	// Smart Sensor Sleep: Port 0, Pin 13, Output, No external pull, Active low, 1.8V (minimum 1.3V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_SMART_SENSOR_SLEEP_PORT;
	setupGPIO.mask = GPIO_SMART_SENSOR_SLEEP_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start in sleep

	//----------------------------------------------------------------------------------------------------------------------
	// Smart Sensor Mux Enable: Port 0, Pin 14, Output, External pulldown, Active high, 3.3V (minimum 2.0V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_SMART_SENSOR_MUX_ENABLE_PORT;
	setupGPIO.mask = GPIO_SMART_SENSOR_MUX_ENABLE_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start disabled

	//----------------------------------------------------------------------------------------------------------------------
	// ADC Reset: Port 0, Pin 15, Output, External pulldown, Active low, 1.8V (minimuim 1.26V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_ADC_RESET_PORT;
	setupGPIO.mask = GPIO_ADC_RESET_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start in reset

#if 1 /* Special addition (ADC Dual-SDO and Max32651 Dual mode not compatible), re-configure SPI3_SDIO2 (P0.17) from SPI line to ADC GPIO */
	//----------------------------------------------------------------------------------------------------------------------
	// External ADC Busy, Alt, GP0: Port 0, Pin 17, Input, No external pullup, Active high, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_ADC_BUSY_ALT_GP0_PORT;
	setupGPIO.mask = GPIO_ADC_BUSY_ALT_GP0_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_WEAK_PULL_DOWN; // ADC GP0 line inits as input, set pull down until ADC GP0 configured as output
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
#endif

	//----------------------------------------------------------------------------------------------------------------------
	// ADC Conversion: Port 0, Pin 18, Output, External pulldown, Active high, 1.8V (minimuim 1.26V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_ADC_CONVERSION_PORT;
	setupGPIO.mask = GPIO_ADC_CONVERSION_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as no conversion

	//----------------------------------------------------------------------------------------------------------------------
	// Cal Mux Pre-A/D Enable: Port 0, Pin 22, Output, External pulldown, Active low, 3.3V (minimum 2V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_CAL_MUX_PRE_AD_ENABLE_PORT;
	setupGPIO.mask = GPIO_CAL_MUX_PRE_AD_ENABLE_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Cal Mux Pre-A/D Select: Port 0, Pin 23, Output, External pulldown, Select (0 is default), 3.3V (minimum 2V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_CAL_MUX_PRE_AD_SELECT_PORT;
	setupGPIO.mask = GPIO_CAL_MUX_PRE_AD_SELECT_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as 0 (Full sensor group Geo1 + AOP1)

	//----------------------------------------------------------------------------------------------------------------------
	// Alert 1: Port 0, Pin 24, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_ALERT_1_PORT;
	setupGPIO.mask = GPIO_ALERT_1_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Alert 2: Port 0, Pin 25, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_ALERT_2_PORT;
	setupGPIO.mask = GPIO_ALERT_2_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// LTE OTA: Port 0, Pin 30, Input, No external pull, Active unknown, 3.3V (device runs 3.3V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_LTE_OTA_PORT;
	setupGPIO.mask = GPIO_LTE_OTA_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&setupGPIO);
	// Todo: Fill in handling when more information known

	//----------------------------------------------------------------------------------------------------------------------
	// eMMC Reset: Port 0, Pin 31, Output, External pulldown, Active low, 1.8V (device runs 1.8V interface & 3.3V part)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_EMMC_RESET_PORT;
	setupGPIO.mask = GPIO_EMMC_RESET_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start by removing from reset

	//----------------------------------------------------------------------------------------------------------------------
	// eMMC Data Strobe: Port 1, Pin 2, Input, External pulldown, Active high, 1.8V (device runs 1.8V interface & 3.3V part)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_EMMC_DATA_STROBE_PORT;
	setupGPIO.mask = GPIO_EMMC_DATA_STROBE_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&setupGPIO);
	// Note: Doesn't look like we can use this line which is only enabled for HS400 mode and HS400 mode seems to only work with 8-bit data bus width (we're 4-bit max)

	//----------------------------------------------------------------------------------------------------------------------
	// Expansion Enable: Port 1, Pin 7, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_EXPANSION_ENABLE_PORT;
	setupGPIO.mask = GPIO_EXPANSION_ENABLE_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Expansion Reset: Port 1, Pin 8, Output, External pulldown, Active low, 3.3V (minimum 2V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_EXPANSION_RESET_PORT;
	setupGPIO.mask = GPIO_EXPANSION_RESET_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start in reset

	//----------------------------------------------------------------------------------------------------------------------
	// USBC Port Controller I2C IRQ: Port 1, Pin 11, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_USBC_PORT_CONTROLLER_I2C_IRQ_PORT;
	setupGPIO.mask = GPIO_USBC_PORT_CONTROLLER_I2C_IRQ_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)Usbc_port_controller_i2c_irq, NULL);
    MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Accel Int 1: Port 1, Pin 12, Input, No external pull, Active high, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_ACCEL_INT_1_PORT;
	setupGPIO.mask = GPIO_ACCEL_INT_1_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)Accelerometer_irq_1, NULL);
    MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_RISING);
    MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Accel Int 2: Port 1, Pin 13, Input, No external pull, Active high, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_ACCEL_INT_2_PORT;
	setupGPIO.mask = GPIO_ACCEL_INT_2_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)Accelerometer_irq_2, NULL);
    MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_RISING);
    MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Accel Trig: Port 1, Pin 14, Output, No external pull, Active high, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_ACCEL_TRIG_PORT;
	setupGPIO.mask = GPIO_ACCEL_TRIG_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Power Button Int: Port 1, Pin 15, Input, External pullup, Active high, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_POWER_BUTTON_IRQ_PORT;
	setupGPIO.mask = GPIO_POWER_BUTTON_IRQ_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)System_power_button_irq, NULL);
    MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_RISING);
    MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Button 1: Port 1, Pin 16, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_BUTTON_1_PORT;
	setupGPIO.mask = GPIO_BUTTON_1_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)Keypad_irq, NULL);
    MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Button 2: Port 1, Pin 17, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_BUTTON_2_PORT;
	setupGPIO.mask = GPIO_BUTTON_2_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)Keypad_irq, NULL);
    MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Button 3: Port 1, Pin 18, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_BUTTON_3_PORT;
	setupGPIO.mask = GPIO_BUTTON_3_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)Keypad_irq, NULL);
    MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Button 4: Port 1, Pin 19, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_BUTTON_4_PORT;
	setupGPIO.mask = GPIO_BUTTON_4_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)Keypad_irq, NULL);
    MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Button 5: Port 1, Pin 20, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_BUTTON_5_PORT;
	setupGPIO.mask = GPIO_BUTTON_5_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)Keypad_irq, NULL);
    MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Button 6: Port 1, Pin 21, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_BUTTON_6_PORT;
	setupGPIO.mask = GPIO_BUTTON_6_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)Keypad_irq, NULL);
    MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Button 7: Port 1, Pin 22, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_BUTTON_7_PORT;
	setupGPIO.mask = GPIO_BUTTON_7_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)Keypad_irq, NULL);
    MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Button 8: Port 1, Pin 23, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_BUTTON_8_PORT;
	setupGPIO.mask = GPIO_BUTTON_8_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)Keypad_irq, NULL);
    MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Button 9: Port 1, Pin 24, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_BUTTON_9_PORT;
	setupGPIO.mask = GPIO_BUTTON_9_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)Keypad_irq, NULL);
    MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// LED 2: Port 1, Pin 25, Output, No external pull, Active high, 3.3V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_LED_2_PORT;
	setupGPIO.mask = GPIO_LED_2_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as off

	//----------------------------------------------------------------------------------------------------------------------
	// LED 3: Port 1, Pin 26, Output, No external pull, Active high, 3.3V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_LED_3_PORT;
	setupGPIO.mask = GPIO_LED_3_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as off

	//----------------------------------------------------------------------------------------------------------------------
	// LED 4: Port 1, Pin 27, Output, No external pull, Active high, 3.3V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_LED_4_PORT;
	setupGPIO.mask = GPIO_LED_4_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as off

	//----------------------------------------------------------------------------------------------------------------------
	// RTC Int A: Port 1, Pin 28, Input, External pullup, Active low, 1.8V (minimum 0.66V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_EXT_RTC_INTA_PORT;
	setupGPIO.mask = GPIO_EXT_RTC_INTA_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)External_rtc_irq, NULL);
    MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// BLE OTA: Port 1, Pin 29, Input, No external pull, Active unknown, 3.3V (device runs 3.3V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_BLE_OTA_PORT;
	setupGPIO.mask = GPIO_BLE_OTA_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&setupGPIO);
	// Todo: Fill in handling when more information known

	//----------------------------------------------------------------------------------------------------------------------
	// Trig Out: Port 1, Pin 30, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_EXTERNAL_TRIGGER_OUT_PORT;
	setupGPIO.mask = GPIO_EXTERNAL_TRIGGER_OUT_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Trig In: Port 1, Pin 31, Input, External pullup, Active high, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_EXTERNAL_TRIGGER_IN_PORT;
	setupGPIO.mask = GPIO_EXTERNAL_TRIGGER_IN_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)External_trigger_irq, NULL);
    MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_RISING);
    MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// LCD Power Enable: Port 2, Pin 0, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_LCD_POWER_ENABLE_PORT;
	setupGPIO.mask = GPIO_LCD_POWER_ENABLE_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// LCD Power Display: Port 2, Pin 1, Output, External pulldown, Active low, 1.8V (minimum 1.7V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_LCD_POWER_DISPLAY_PORT;
	setupGPIO.mask = GPIO_LCD_POWER_DISPLAY_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// SPI2 Slave Select 0 LCD: Port 2, Pin 5, Output, External pullup, Active low, 1.8V (minimum 1.7V)
	//----------------------------------------------------------------------------------------------------------------------
	if (FT81X_SPI_2_SS_CONTROL_MANUAL)
	{
		setupGPIO.port = GPIO_SPI_2_SS_0_LCD_PORT;
		setupGPIO.mask = GPIO_SPI_2_SS_0_LCD_PIN;
		setupGPIO.func = MXC_GPIO_FUNC_OUT;
		setupGPIO.pad = MXC_GPIO_PAD_NONE;
		setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
		MXC_GPIO_Config(&setupGPIO);
		MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start as disabled
	}

	//----------------------------------------------------------------------------------------------------------------------
	// LCD Int: Port 2, Pin 6, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_LCD_INT_PORT;
	setupGPIO.mask = GPIO_LCD_INT_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)Lcd_irq, NULL);
    MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Sensor Check Enable: Port 2, Pin 9, Output, External pulldown, Active high, 1.8V (minimum 0.65 * Vin)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_SENSOR_CHECK_ENABLE_PORT;
	setupGPIO.mask = GPIO_SENSOR_CHECK_ENABLE_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Sensor Check: Port 2, Pin 10, Output, No external pull, Active high, 1.8V (must match Sensor Check enable)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_SENSOR_CHECK_PORT;
	setupGPIO.mask = GPIO_SENSOR_CHECK_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE; // Consider weak pulldown?
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as disabled


	//----------------------------------------------------------------------------------------------------------------------
	// LTE Reset: Port 2, Pin 13, Output, External pull up, Active low, 3.3V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_LTE_RESET_PORT;
	setupGPIO.mask = GPIO_LTE_RESET_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// BLE Reset: Port 2, 15, Output, External pull up, Active low, 3.3V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_BLE_RESET_PORT;
	setupGPIO.mask = GPIO_BLE_RESET_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Smart Sensor Mux A0: Port 2, Pin 23, Output, External pulldown, Select, 3.3V (minimum 2.0V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_SMART_SENSOR_MUX_A0_PORT;
	setupGPIO.mask = GPIO_SMART_SENSOR_MUX_A0_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as low (Smart Sensor Geo1 select)

	//----------------------------------------------------------------------------------------------------------------------
	// Smart Sensor Mux A1: Port 2, Pin 25, Output, External pulldown, Select, 3.3V (minimum 2.0V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_SMART_SENSOR_MUX_A1_PORT;
	setupGPIO.mask = GPIO_SMART_SENSOR_MUX_A1_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as low (Smart Sensor Geo1 select)

	//----------------------------------------------------------------------------------------------------------------------
	// Nyquist 0/A0:Port 2, Pin 26, Output, External pulldown, Select, 3.3V (minimum 2.4V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_NYQUIST_0_A0_PORT;
	setupGPIO.mask = GPIO_NYQUIST_0_A0_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as low

	//----------------------------------------------------------------------------------------------------------------------
	// Nyquist 1/A1: Port 2, Pin 28, Output, External pulldown, Select, 3.3V (minimum 2.4V))
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_NYQUIST_1_A1_PORT;
	setupGPIO.mask = GPIO_NYQUIST_1_A1_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as low

	//----------------------------------------------------------------------------------------------------------------------
	// Nyquist 2/Enable: Port 2, Pin 30, Output, External pulldown, Active low, 3.3V (minimum 2.4V))
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_NYQUIST_2_ENABLE_PORT;
	setupGPIO.mask = GPIO_NYQUIST_2_ENABLE_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start as disabled (960Hz select)

	//----------------------------------------------------------------------------------------------------------------------
	// Cellular Enable: Port 3, Pin 0, Output, No external pull, Active high, 3.3V (minimum 0.5)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_CELL_ENABLE_PORT;
	setupGPIO.mask = GPIO_CELL_ENABLE_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Sensor Enable Geo1: Port 3, Pin 1, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_SENSOR_ENABLE_GEO1_PORT;
	setupGPIO.mask = GPIO_SENSOR_ENABLE_GEO1_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Sensor Enable Aop1: Port 3, Pin 2, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_SENSOR_ENABLE_AOP1_PORT;
	setupGPIO.mask = GPIO_SENSOR_ENABLE_AOP1_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Sensor Enable Geo2: Port 3, Pin 3, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_SENSOR_ENABLE_GEO2_PORT;
	setupGPIO.mask = GPIO_SENSOR_ENABLE_GEO2_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Sensor Enable Aop2: Port 3, Pin 4, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_SENSOR_ENABLE_AOP2_PORT;
	setupGPIO.mask = GPIO_SENSOR_ENABLE_AOP2_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Gain Select Geo1: Port 3, Pin 5, Output, External pulldown, Select, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_GAIN_SELECT_GEO1_PORT;
	setupGPIO.mask = GPIO_GAIN_SELECT_GEO1_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start as high (Normal gain)

	//----------------------------------------------------------------------------------------------------------------------
	// Path Select Aop1: Port 3, Pin 6, Output, External pulldown, Select, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_PATH_SELECT_AOP1_PORT;
	setupGPIO.mask = GPIO_PATH_SELECT_AOP1_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start as high (AOP path)

	//----------------------------------------------------------------------------------------------------------------------
	// Gain Select Geo2: Port 3, Pin 7,Output, External pulldown, Select, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_GAIN_SELECT_GEO2_PORT;
	setupGPIO.mask = GPIO_GAIN_SELECT_GEO2_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start as high (Normal gain)

	//----------------------------------------------------------------------------------------------------------------------
	// Path Select Aop2: Port 3, Pin 8, Output, External pulldown, Select, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_PATH_SELECT_AOP2_PORT;
	setupGPIO.mask = GPIO_PATH_SELECT_AOP2_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start as high (AOP path)

	//----------------------------------------------------------------------------------------------------------------------
	// RTC Clock: Port 3, Pin 9, Input, No external pull, Active high, 3.3V (minimum 2.64V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_RTC_CLOCK_PORT;
	setupGPIO.mask = GPIO_RTC_CLOCK_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE; // Consider a weak internal pull?
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)Sample_irq, NULL);
    MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);

#if 0 /* Moved to Init Interrupts section */
	//----------------------------------------------------------------------------------------------------------------------
	// Enable IRQ's for any of the appropritate GPIO input interrupts
	//----------------------------------------------------------------------------------------------------------------------
	NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(MXC_GPIO0)));
	NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(MXC_GPIO1)));
	NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(MXC_GPIO2)));
	NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(MXC_GPIO3)));
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UART0_Read_Callback(mxc_uart_req_t *req, int error)
{
    // UART0 receive processing
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UART1_Read_Callback(mxc_uart_req_t *req, int error)
{
    // UART1 receive processing
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UART2_Read_Callback(mxc_uart_req_t *req, int error)
{
    // UART2 receive processing
}

#define UART_BUFFER_SIZE 512
uint8_t g_Uart0_RxBuffer[UART_BUFFER_SIZE];
uint8_t g_Uart0_TxBuffer[UART_BUFFER_SIZE];
uint8_t g_Uart1_RxBuffer[UART_BUFFER_SIZE];
uint8_t g_Uart1_TxBuffer[UART_BUFFER_SIZE];
uint8_t g_Uart2_RxBuffer[UART_BUFFER_SIZE];
uint8_t g_Uart2_TxBuffer[UART_BUFFER_SIZE];

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UART0_Handler(void)
{
    MXC_UART_AsyncHandler(MXC_UART0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UART1_Handler(void)
{
    MXC_UART_AsyncHandler(MXC_UART1);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UART2_Handler(void)
{
    MXC_UART_AsyncHandler(MXC_UART2);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupUART(void)
{
	int status;

    status = MXC_UART_Init(MXC_UART0, UART_BAUD);
    if (status != E_SUCCESS) { debugErr("UART0 failed init with code: %d\r\n", status); }

    // Move to Interrupt init
	NVIC_ClearPendingIRQ(UART0_IRQn);
    NVIC_DisableIRQ(UART0_IRQn);
    MXC_NVIC_SetVector(UART0_IRQn, UART0_Handler);
    NVIC_EnableIRQ(UART0_IRQn);

    status = MXC_UART_Init(MXC_UART1, UART_BAUD);
    if (status != E_SUCCESS) { debugErr("UART1 failed init with code: %d\r\n", status); }

    // Move to Interrupt init
    NVIC_ClearPendingIRQ(UART1_IRQn);
    NVIC_DisableIRQ(UART1_IRQn);
    MXC_NVIC_SetVector(UART1_IRQn, UART1_Handler);
    NVIC_EnableIRQ(UART1_IRQn);

    // Setup the asynchronous request
    mxc_uart_req_t uart0ReadRequest;
    uart0ReadRequest.uart = MXC_UART0;
    uart0ReadRequest.rxData = g_Uart0_RxBuffer;
    uart0ReadRequest.rxLen = UART_BUFFER_SIZE;
    uart0ReadRequest.txLen = 0;
    uart0ReadRequest.callback = UART0_Read_Callback;

    // Setup the asynchronous request
    mxc_uart_req_t uart1ReadRequest;
    uart1ReadRequest.uart = MXC_UART1;
    uart1ReadRequest.rxData = g_Uart1_RxBuffer;
    uart1ReadRequest.rxLen = UART_BUFFER_SIZE;
    uart1ReadRequest.txLen = 0;
    uart1ReadRequest.callback = UART1_Read_Callback;

    status = MXC_UART_TransactionAsync(&uart0ReadRequest);
    if (status != E_SUCCESS) { debugErr("Uart0 Read setup (async) failed with code: %d\r\n", status); }

    status = MXC_UART_TransactionAsync(&uart1ReadRequest);
    if (status != E_SUCCESS) { debugErr("Uart1 Read setup (async) failed with code: %d\r\n", status); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupDebugUART(void)
{
	int status;

    status = MXC_UART_Init(MXC_UART2, UART_BAUD);
    if (status != E_SUCCESS) { } // Where to report?

    // Move to Interrupt init
	NVIC_ClearPendingIRQ(UART2_IRQn);
    NVIC_DisableIRQ(UART2_IRQn);
    MXC_NVIC_SetVector(UART2_IRQn, UART2_Handler);
    NVIC_EnableIRQ(UART2_IRQn);

    // Setup the asynchronous request
    mxc_uart_req_t uart2ReadRequest;
    uart2ReadRequest.uart = MXC_UART0;
    uart2ReadRequest.rxData = g_Uart2_RxBuffer;
    uart2ReadRequest.rxLen = UART_BUFFER_SIZE;
    uart2ReadRequest.txLen = 0;
    uart2ReadRequest.callback = UART2_Read_Callback;

    status = MXC_UART_TransactionAsync(&uart2ReadRequest);
    if (status != E_NO_ERROR) { debugErr("Debug Uart2 Read setup (async) failed with code: %d\r\n", status); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
// <uart driver option> int MXC_UART_RevA_Write(mxc_uart_reva_regs_t *uart, uint8_t *byte, int *len)
void UART0_Write_Blocking(uint8_t* data, uint32_t size)
{
	int error;

    mxc_uart_req_t uart0WriteRequest;
    uart0WriteRequest.uart = MXC_UART0;
    uart0WriteRequest.txData = data;
    uart0WriteRequest.txLen = size;
    uart0WriteRequest.rxLen = 0;
    uart0WriteRequest.callback = NULL;

    error = MXC_UART_Transaction(&uart0WriteRequest);
    if (error != E_NO_ERROR) { debugErr("Uart0 write failed with code: %d\r\n", error); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
// <uart driver option> int MXC_UART_RevA_Write(mxc_uart_reva_regs_t *uart, uint8_t *byte, int *len)
void UART1_Write_Blocking(uint8_t* data, uint32_t size)
{
	int error;

    mxc_uart_req_t uart1WriteRequest;
    uart1WriteRequest.uart = MXC_UART1;
    uart1WriteRequest.txData = data;
    uart1WriteRequest.txLen = size;
    uart1WriteRequest.rxLen = 0;
    uart1WriteRequest.callback = NULL;

    error = MXC_UART_Transaction(&uart1WriteRequest);
    if (error != E_NO_ERROR) { debugErr("Uart0 write failed with code: %d\r\n", error); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UART0_Write_Async_ISR(uint8_t* data, uint32_t size)
{
	int error;

	memcpy(g_Uart0_TxBuffer, data, (size < UART_BUFFER_SIZE) ? size : UART_BUFFER_SIZE);

    mxc_uart_req_t uart0WriteRequest;
    uart0WriteRequest.uart = MXC_UART0;
    uart0WriteRequest.txData = g_Uart0_TxBuffer;
    uart0WriteRequest.txLen = size;
    uart0WriteRequest.rxLen = 0;
    uart0WriteRequest.callback = NULL;

    error = MXC_UART_TransactionAsync(&uart0WriteRequest);
    if (error != E_NO_ERROR) { debugErr("Uart0 write setup (async) failed with code: %d\r\n", error); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UART1_Write_Async_ISR(uint8_t* data, uint32_t size)
{
	int error;

	memcpy(g_Uart1_TxBuffer, data, (size < UART_BUFFER_SIZE) ? size : UART_BUFFER_SIZE);

    mxc_uart_req_t uart1WriteRequest;
    uart1WriteRequest.uart = MXC_UART1;
    uart1WriteRequest.txData = g_Uart1_TxBuffer;
    uart1WriteRequest.txLen = size;
    uart1WriteRequest.rxLen = 0;
    uart1WriteRequest.callback = NULL;

    error = MXC_UART_TransactionAsync(&uart1WriteRequest);
    if (error != E_NO_ERROR) { debugErr("Uart1 write setup (async) failed with code: %d\r\n", error); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int WriteI2CDevice(mxc_i2c_regs_t* i2cChannel, uint8_t slaveAddr, uint8_t* writeData, uint32_t writeSize, uint8_t* readData, uint32_t readSize)
{
	int status;

    mxc_i2c_req_t masterRequest;
    masterRequest.i2c = i2cChannel;
    masterRequest.addr = slaveAddr;
    masterRequest.tx_buf = writeData;
    masterRequest.tx_len = writeSize;
    masterRequest.rx_buf = readData;
    masterRequest.rx_len = readSize;
    masterRequest.restart = 0;
    masterRequest.callback = NULL;

    status = MXC_I2C_MasterTransaction(&masterRequest);
	if (status != E_SUCCESS) { debugErr("I2C%d Master transaction to Slave (%02x) failed with code: %d\r\n", ((i2cChannel == MXC_I2C0) ? 0 : 1), slaveAddr, status); }

	return (status);
}

#if 0 /* Used if setting up the MXC I2C as a slave */
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void I2C0_IRQHandler(void)
{
    MXC_I2C_AsyncHandler(MXC_I2C0);
    return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void I2C1_IRQHandler(void)
{
    MXC_I2C_AsyncHandler(MXC_I2C1);
    return;
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupI2C(void)
{
	int error;

    // Setup I2C0 as Master (1.8V) 
    error = MXC_I2C_Init(MXC_I2C0, 1, 0);
    if (error != E_NO_ERROR) { debugErr("I2C0 init (master) failed to initialize with code: %d\r\n", error); }

    // Setup I2C1 as Master (3.3V) 
    error = MXC_I2C_Init(MXC_I2C1, 1, 0);
    if (error != E_NO_ERROR) { debugErr("I2C1 init (master) failed to initialize with code: %d\r\n", error); }

#if 0 /* Needed if setting up the MXC I2C as a slave */
    MXC_NVIC_SetVector(I2C0_IRQn, I2C0_IRQHandler);
    NVIC_EnableIRQ(I2C0_IRQn);
    MXC_NVIC_SetVector(I2C1_IRQn, I2C1_IRQHandler);
    NVIC_EnableIRQ(I2C1_IRQn);
#endif

	// Set I2C speed, either Standard (MXC_I2C_STD_MODE = 100000) or Fast (MXC_I2C_FAST_SPEED = 400000)
#if 0 /* Fast Speed */
    MXC_I2C_SetFrequency(MXC_I2C0, MXC_I2C_FAST_SPEED);
    MXC_I2C_SetFrequency(MXC_I2C1, MXC_I2C_FAST_SPEED);
#elif 1 /* Standard */
    MXC_I2C_SetFrequency(MXC_I2C0, MXC_I2C_STD_MODE);
    MXC_I2C_SetFrequency(MXC_I2C1, MXC_I2C_STD_MODE);
#else /* Test */
#define MXC_I2C_STD_TEST 75000 // Can't be lower than 58,593 to fit in hi/lo
    //MXC_I2C_SetFrequency(MXC_I2C0, MXC_I2C_STD_TEST);
    //MXC_I2C_SetFrequency(MXC_I2C1, MXC_I2C_STD_TEST);
	MXC_I2C0->clk_hi = 0x1FF;
	MXC_I2C0->clk_lo = 0x1FF;
	MXC_I2C1->clk_hi = 0x1FF;
	MXC_I2C1->clk_lo = 0x1FF;
#endif
	debug("I2C Frequency: Channel 0: %d, Channel 1: %d\r\n", MXC_I2C_GetFrequency(MXC_I2C0), MXC_I2C_GetFrequency(MXC_I2C0));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupWatchdog(void)
{
    // Check if watchdog caused reset
	if (MXC_WDT_GetResetFlag(MXC_WDT0))
	{
		MXC_WDT_ClearResetFlag(MXC_WDT0);
		MXC_WDT_DisableReset(MXC_WDT0);
		MXC_WDT_Disable(MXC_WDT0);
		debugErr("Watchdog reset the unit\r\n");
    }

	// Reset the Watchdog peripheral 
	MXC_WDT_Init(MXC_WDT0);

	MXC_WDT_SetResetPeriod(MXC_WDT0, MXC_WDT_PERIOD_2_31); // ~18 secs
	MXC_WDT_SetIntPeriod(MXC_WDT0, MXC_WDT_PERIOD_2_30); // ~9 secs
	MXC_WDT_EnableReset(MXC_WDT0);
	MXC_WDT_EnableInt(MXC_WDT0);
	NVIC_EnableIRQ(WDT0_IRQn);

	// Reset watchdog timer for enable sequence
	MXC_WDT_ResetTimer(MXC_WDT0);

#if 0 /* Todo: After hardware checks pass, enable once the executive loop runs */
    MXC_WDT_Enable(MXC_WDT0);
#endif
}

#if 0 /* Normal */
#define SPI_SPEED_ADC 10000000 // Bit Rate
#else
//#define SPI_SPEED_ADC 3000000 // Bit Rate
#define SPI_SPEED_ADC 60000000 // Bit Rate
#endif
#define SPI_SPEED_LCD 10000000 // Bit Rate
//#define SPI_WIDTH_DUAL	2

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupICC(void)
{
	uint32_t timeout;
#if 0 /* Skip global enable since it starts both ICC's and we don't want ICC1 */
	MXC_ICC_Enable();
#else /* Manual enable of ICC0 */
    // Invalidate cache and wait until ready
    MXC_ICC0->cache_ctrl &= ~MXC_F_ICC_CACHE_CTRL_ENABLE;
    MXC_ICC0->invalidate = 1;

    timeout = 0x8000; // Arbitrary value
	while (!(MXC_ICC0->cache_ctrl & MXC_F_ICC_CACHE_CTRL_READY)) { if (timeout-- == 0) { debugErr("ICC: timed out invalidating\r\n"); break; } }

    // Enable Cache
    MXC_ICC0->cache_ctrl |= MXC_F_ICC_CACHE_CTRL_ENABLE;
    timeout = 0x8000; // Arbitrary value
    while (!(MXC_ICC0->cache_ctrl & MXC_F_ICC_CACHE_CTRL_READY)) { if (timeout-- == 0) { debugErr("ICC: timed out enabling\r\n"); break; } }
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SPI3_IRQHandler(void)
{
    MXC_SPI_AsyncHandler(MXC_SPI3);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SPI2_IRQHandler(void)
{
    MXC_SPI_AsyncHandler(MXC_SPI2);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SPI_Callback(mxc_spi_req_t *req, int result)
{
    // SPI data processing
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SpiTransaction(mxc_spi_regs_t* spiPort, uint8_t dataBits, uint8_t ssDeassert, uint8_t* writeData, uint32_t writeSize, uint8_t* readData, uint32_t readSize, uint8_t method)
{
	mxc_spi_req_t spiRequest;
	IRQn_Type spiIrq;
	void (*irqHandler)(void);

	spiRequest.spi = spiPort;
	spiRequest.txData = writeData;
	spiRequest.txLen = writeSize;
	spiRequest.rxData = readData;
	spiRequest.rxLen = readSize;
	spiRequest.ssIdx = 0; // Both ADC and LCD Slave Selects are 0
	spiRequest.ssDeassert = ssDeassert;
	spiRequest.txCnt = 0;
	spiRequest.rxCnt = 0;
	spiRequest.completeCB = (spi_complete_cb_t)SPI_Callback;

	// Set the number of data bits for the transfer
	MXC_SPI_SetDataSize(spiPort, dataBits);

	if (method == BLOCKING)
	{
		MXC_SPI_MasterTransaction(&spiRequest);
	}
	else if (method == ASYNC_ISR)
	{
		// Check if selecting the ADC
		if (spiPort == MXC_SPI3)
		{
			spiIrq = SPI3_IRQn;
			irqHandler = SPI3_IRQHandler;
		}
		else // Selecting the LCD
		{
			spiIrq = SPI2_IRQn;
			irqHandler = SPI2_IRQHandler;
		}

		MXC_NVIC_SetVector(spiIrq, irqHandler);
		NVIC_EnableIRQ(spiIrq);
		MXC_SPI_MasterTransactionAsync(&spiRequest);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupSPI(void)
{
	int status;

	//--------------------
	// SPI3 - External ADC
	//--------------------
#if 1 /* Test SS Gpio setup before init */
	mxc_gpio_cfg_t spi3SlaveSelect0GpioConfig = { GPIO_ADC_SPI_3_SS_0_PORT, GPIO_ADC_SPI_3_SS_0_PIN, MXC_GPIO_FUNC_ALT1, MXC_GPIO_PAD_NONE, MXC_GPIO_VSSEL_VDDIO };
	MXC_GPIO_Config(&spi3SlaveSelect0GpioConfig); // Seems the SPI framework driver does not set the Slave Select 0 alternate function on the GPIO pin
#endif
	status = MXC_SPI_Init(MXC_SPI3, YES, NO, 1, LOW, SPI_SPEED_ADC);
	if (status != E_SUCCESS) { debugErr("SPI3 (ADC) Init failed with code: %d\r\n", status); }

	//mxc_gpio_cfg_t spi3SlaveSelect0GpioConfig = { GPIO_ADC_SPI_3_SS_0_PORT, GPIO_ADC_SPI_3_SS_0_PIN, MXC_GPIO_FUNC_ALT1, MXC_GPIO_PAD_NONE, MXC_GPIO_VSSEL_VDDIO };
	MXC_GPIO_Config(&spi3SlaveSelect0GpioConfig); // Seems the SPI framework driver does not set the Slave Select 0 alternate function on the GPIO pin

	// Set standard SPI 4-wire (MISO/MOSI, full duplex), (Turns out ADC dual-SDO and MAX32651 dual mode are incompatible, can only use single mode)
	MXC_SPI_SetWidth(MXC_SPI3, SPI_WIDTH_STANDARD);

	// External SPI3 uses SPI Mode 3 only
	MXC_SPI_SetMode(MXC_SPI3, SPI_MODE_3);

	debug("SPI3 Clock control config: Scale: %d, High CC: %d, Low CC: %d\r\n", (MXC_SPI3->clk_cfg >> 16), ((MXC_SPI3->clk_cfg >> 8) & 0xFF), (MXC_SPI3->clk_cfg >> 8) & 0xFF);

	//-----------
	// SPI2 - LCD
	//-----------
	status = MXC_SPI_Init(MXC_SPI2, YES, NO, 1, LOW, SPI_SPEED_LCD);
	if (status != E_SUCCESS) { debugErr("SPI2 (LCD) Init failed with code: %d\r\n", status); }

	if (FT81X_SPI_2_SS_CONTROL_MANUAL)
	{
		mxc_gpio_cfg_t spi2SlaveSelect0GpioConfig = { GPIO_SPI_2_SS_0_LCD_PORT, GPIO_SPI_2_SS_0_LCD_PIN, MXC_GPIO_FUNC_OUT, MXC_GPIO_PAD_NONE, MXC_GPIO_VSSEL_VDDIO };
		MXC_GPIO_Config(&spi2SlaveSelect0GpioConfig); // Seems the SPI framework driver does not set the Slave Select 0 alternate function on the GPIO pin

		// Clear Master SS select control
		MXC_SPI2->ctrl0 &= ~MXC_F_SPI_CTRL0_SS_SEL;
	}
	else // SPI2 Slave Select controlled by the SPI driver
	{
		mxc_gpio_cfg_t spi2SlaveSelect0GpioConfig = { GPIO_SPI_2_SS_0_LCD_PORT, GPIO_SPI_2_SS_0_LCD_PIN, MXC_GPIO_FUNC_ALT1, MXC_GPIO_PAD_NONE, MXC_GPIO_VSSEL_VDDIO };
		MXC_GPIO_Config(&spi2SlaveSelect0GpioConfig); // Seems the SPI framework driver does not set the Slave Select 0 alternate function on the GPIO pin
	}

	// Set standard SPI 4-wire (MISO/MOSI, full duplex)
	MXC_SPI_SetWidth(MXC_SPI2, SPI_WIDTH_STANDARD);

	// LCD controller uses SPI Mode 0 only
	MXC_SPI_SetMode(MXC_SPI2, SPI_MODE_0);
}

// USB Definitions
#define EVENT_ENUM_COMP MAXUSB_NUM_EVENTS
#define EVENT_REMOTE_WAKE (EVENT_ENUM_COMP + 1)

#define BUFFER_SIZE 64

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

// USB Global Data
volatile int configured;
volatile int suspended;
volatile unsigned int event_flags;
int remote_wake_en;

// USB Function Prototypes
static int setconfigCallback(MXC_USB_SetupPkt *sud, void *cbdata);
static int setfeatureCallback(MXC_USB_SetupPkt *sud, void *cbdata);
static int clrfeatureCallback(MXC_USB_SetupPkt *sud, void *cbdata);
static int usbEventCallback(maxusb_event_t evt, void *data);
static void usbAppSleep(void);
static void usbAppWakeup(void);
static int usbReadCallback(void);
int usbStartupCallback();
int usbShutdownCallback();
void echoUSB(void);

// This EP assignment must match the Configuration Descriptor
static msc_cfg_t msc_cfg = {
    1, /* EP OUT */
    MXC_USBHS_MAX_PACKET, /* OUT max packet size */
    2, /* EP IN */
    MXC_USBHS_MAX_PACKET, /* IN max packet size */
};

static const msc_idstrings_t ids = {
    "NOMIS", /* Vendor string.  Maximum of 8 bytes */
    "MSC FLASH DRIVE", /* Product string.  Maximum of 16 bytes */
    "1.0" /* Version string.  Maximum of 4 bytes */
};

// This EP assignment must match the Configuration Descriptor
static acm_cfg_t acm_cfg = {
    2, /* EP OUT */
    MXC_USBHS_MAX_PACKET, /* OUT max packet size */
    3, /* EP IN */
    MXC_USBHS_MAX_PACKET, /* IN max packet size */
    4, /* EP Notify */
    MXC_USBHS_MAX_PACKET, /* Notify max packet size */
};

// Functions to control "disk" memory. See msc.h for definitions
static const msc_mem_t mem = { mscmem_Init, mscmem_Start, mscmem_Stop, mscmem_Ready, mscmem_Size, mscmem_Read, mscmem_Write };

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void delay_us(unsigned int usec)
{
    /* mxc_delay() takes unsigned long, so can't use it directly */
    MXC_Delay(usec);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupUSBComposite(void)
{
    maxusb_cfg_options_t usb_opts;

    debug("Waiting for VBUS...\n");

    // Initialize state
    configured = 0;
    suspended = 0;
    event_flags = 0;
    remote_wake_en = 0;

    // Start out in full speed
    usb_opts.enable_hs = 1; // 0 for Full Speed, 1 for High Speed
    usb_opts.delay_us = delay_us; // Function used for delays
    usb_opts.init_callback = usbStartupCallback;
    usb_opts.shutdown_callback = usbShutdownCallback;

    // Initialize the usb module
    if (MXC_USB_Init(&usb_opts) != 0) { debugErr("USB Init failed\r\n"); }

    // Initialize the enumeration module
    if (enum_init() != 0) { debugErr("Enumeration Init failed\r\n"); }

    // Register enumeration data
    enum_register_descriptor(ENUM_DESC_DEVICE, (uint8_t *)&composite_device_descriptor, 0);
    enum_register_descriptor(ENUM_DESC_CONFIG, (uint8_t *)&composite_config_descriptor, 0);
    if (usb_opts.enable_hs) {
        // Two additional descriptors needed for high-speed operation
        enum_register_descriptor(ENUM_DESC_OTHER, (uint8_t *)&composite_config_descriptor_hs, 0);
        enum_register_descriptor(ENUM_DESC_QUAL, (uint8_t *)&composite_device_qualifier_descriptor,
                                 0);
    }
    enum_register_descriptor(ENUM_DESC_STRING, lang_id_desc, 0);
    enum_register_descriptor(ENUM_DESC_STRING, mfg_id_desc, 1);
    enum_register_descriptor(ENUM_DESC_STRING, prod_id_desc, 2);
    enum_register_descriptor(ENUM_DESC_STRING, serial_id_desc, 3);
    enum_register_descriptor(ENUM_DESC_STRING, cdcacm_func_desc, 4);
    enum_register_descriptor(ENUM_DESC_STRING, msc_func_desc, 5);

    // Handle configuration
    enum_register_callback(ENUM_SETCONFIG, setconfigCallback, NULL);

    // Handle feature set/clear
    enum_register_callback(ENUM_SETFEATURE, setfeatureCallback, NULL);
    enum_register_callback(ENUM_CLRFEATURE, clrfeatureCallback, NULL);

    // Initialize the class driver
    if (msc_init(&composite_config_descriptor.msc_interface_descriptor, &ids, &mem) != 0) { debugErr("MSC Init failed\r\n"); }
    if (acm_init(&composite_config_descriptor.comm_interface_descriptor) != 0) { debugErr("CDC/ACM Init failed\r\n"); }

    // Register callbacks
    MXC_USB_EventEnable(MAXUSB_EVENT_NOVBUS, usbEventCallback, NULL);
    MXC_USB_EventEnable(MAXUSB_EVENT_VBUS, usbEventCallback, NULL);
    acm_register_callback(ACM_CB_READ_READY, usbReadCallback);

    // Start with USB in low power mode
    usbAppSleep();
    NVIC_EnableIRQ(USB_IRQn);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void echoUSB(void)
{
    int chars;
    uint8_t buffer[BUFFER_SIZE];
    //uint8_t echoText[32];

    if ((chars = acm_canread()) > 0) {
        if (chars > BUFFER_SIZE) {
            chars = BUFFER_SIZE;
        }

        // Read the data from USB
        if (acm_read(buffer, chars) != chars) {
            debugErr("acm_read() failed\r\n");
            return;
        }

        // Echo it back
        if (acm_present()) {
            //sprintf((char*)&echoText[0], "Echo: ");
            //acm_write(&echoText[0], (unsigned int)strlen((char*)&echoText[0]));
            if (acm_write(buffer, chars) != chars) {
                debugErr("acm_write() failed\r\n");
            }
        }
    }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int usbStartupCallback()
{
    // Startup the HIRC96M clock if it's not on already
    if (!(MXC_GCR->clk_ctrl & MXC_F_GCR_CLK_CTRL_HIRC96_EN)) {
        MXC_GCR->clk_ctrl |= MXC_F_GCR_CLK_CTRL_HIRC96_EN;

        if (MXC_SYS_Clock_Timeout(MXC_F_GCR_CLK_CTRL_HIRC96_RDY) != E_NO_ERROR) {
            return E_TIME_OUT;
        }
    }

    MXC_SYS_ClockEnable(MXC_SYS_PERIPH_CLOCK_USB);

    return E_NO_ERROR;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int usbShutdownCallback()
{
    MXC_SYS_ClockDisable(MXC_SYS_PERIPH_CLOCK_USB);

    return E_NO_ERROR;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static int setconfigCallback(MXC_USB_SetupPkt *sud, void *cbdata)
{
    /* Confirm the configuration value */
    if (sud->wValue == composite_config_descriptor.config_descriptor.bConfigurationValue)
	{
        configured = 1;
        MXC_SETBIT(&event_flags, EVENT_ENUM_COMP);

        if (MXC_USB_GetStatus() & MAXUSB_STATUS_HIGH_SPEED)
		{
            msc_cfg.out_ep = composite_config_descriptor_hs.endpoint_descriptor_1.bEndpointAddress & 0x7;
            msc_cfg.out_maxpacket = composite_config_descriptor_hs.endpoint_descriptor_1.wMaxPacketSize;
            msc_cfg.in_ep = composite_config_descriptor_hs.endpoint_descriptor_2.bEndpointAddress & 0x7;
            msc_cfg.in_maxpacket = composite_config_descriptor_hs.endpoint_descriptor_2.wMaxPacketSize;
        }
		else // Not high speed
		{
            msc_cfg.out_ep = composite_config_descriptor.endpoint_descriptor_1.bEndpointAddress & 0x7;
            msc_cfg.out_maxpacket = composite_config_descriptor.endpoint_descriptor_1.wMaxPacketSize;
            msc_cfg.in_ep = composite_config_descriptor.endpoint_descriptor_2.bEndpointAddress & 0x7;
            msc_cfg.in_maxpacket = composite_config_descriptor.endpoint_descriptor_2.wMaxPacketSize;
        }

        acm_cfg.out_ep = composite_config_descriptor.endpoint_descriptor_4.bEndpointAddress & 0x7;
        acm_cfg.out_maxpacket = composite_config_descriptor.endpoint_descriptor_4.wMaxPacketSize;
        acm_cfg.in_ep = composite_config_descriptor.endpoint_descriptor_5.bEndpointAddress & 0x7;
        acm_cfg.in_maxpacket = composite_config_descriptor.endpoint_descriptor_5.wMaxPacketSize;
        acm_cfg.notify_ep = composite_config_descriptor.endpoint_descriptor_3.bEndpointAddress & 0x7;
        acm_cfg.notify_maxpacket = composite_config_descriptor.endpoint_descriptor_3.wMaxPacketSize;

        msc_configure(&msc_cfg);
        return acm_configure(&acm_cfg);
        /* Configure the device class */
    }
	else if (sud->wValue == 0)
	{
        configured = 0;
        msc_deconfigure();
        return acm_deconfigure();
    }

    return -1;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static int setfeatureCallback(MXC_USB_SetupPkt *sud, void *cbdata)
{
    if (sud->wValue == FEAT_REMOTE_WAKE) {
        remote_wake_en = 1;
    } else {
        // Unknown callback
        return -1;
    }

    return 0;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static int clrfeatureCallback(MXC_USB_SetupPkt *sud, void *cbdata)
{
    if (sud->wValue == FEAT_REMOTE_WAKE) {
        remote_wake_en = 0;
    } else {
        // Unknown callback
        return -1;
    }

    return 0;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static void usbAppSleep(void)
{
    suspended = 1;

    // Todo: Any low power code to place here?
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static void usbAppWakeup(void)
{
    suspended = 0;

    // Todo: Any power up code to place here?
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static int usbEventCallback(maxusb_event_t evt, void *data)
{
    /* Set event flag */
    MXC_SETBIT(&event_flags, evt);

    switch (evt) {
    case MAXUSB_EVENT_NOVBUS:
        MXC_USB_EventDisable(MAXUSB_EVENT_BRST);
        MXC_USB_EventDisable(MAXUSB_EVENT_SUSP);
        MXC_USB_EventDisable(MAXUSB_EVENT_DPACT);
        MXC_USB_Disconnect();
        configured = 0;
        enum_clearconfig();
        msc_deconfigure();
        acm_deconfigure();
        usbAppSleep();
        break;
    case MAXUSB_EVENT_VBUS:
        MXC_USB_EventClear(MAXUSB_EVENT_BRST);
        MXC_USB_EventEnable(MAXUSB_EVENT_BRST, usbEventCallback, NULL);
        MXC_USB_EventClear(MAXUSB_EVENT_BRSTDN);
        MXC_USB_EventEnable(MAXUSB_EVENT_BRSTDN, usbEventCallback, NULL);
        MXC_USB_EventClear(MAXUSB_EVENT_SUSP);
        MXC_USB_EventEnable(MAXUSB_EVENT_SUSP, usbEventCallback, NULL);
        MXC_USB_Connect();
        usbAppSleep();
        break;
    case MAXUSB_EVENT_BRST:
        usbAppWakeup();
        enum_clearconfig();
        msc_deconfigure();
        acm_deconfigure();
        configured = 0;
        suspended = 0;
        break;
    case MAXUSB_EVENT_BRSTDN:
        if (MXC_USB_GetStatus() & MAXUSB_STATUS_HIGH_SPEED) {
            enum_register_descriptor(ENUM_DESC_CONFIG, (uint8_t *)&composite_config_descriptor_hs,
                                     0);
            enum_register_descriptor(ENUM_DESC_OTHER, (uint8_t *)&composite_config_descriptor, 0);
        } else {
            enum_register_descriptor(ENUM_DESC_CONFIG, (uint8_t *)&composite_config_descriptor, 0);
            enum_register_descriptor(ENUM_DESC_OTHER, (uint8_t *)&composite_config_descriptor_hs,
                                     0);
        }
        break;
    case MAXUSB_EVENT_SUSP:
        usbAppSleep();
        break;
    case MAXUSB_EVENT_DPACT:
        usbAppWakeup();
        break;
    default:
        break;
    }

    return 0;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void USB_IRQHandler(void)
{
    MXC_USB_EventHandler();
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static int usbReadCallback(void)
{
	uint16_t numChars = acm_canread();
	uint8 recieveData;

	// Loop through remaining chars to be read
	while (numChars--)
	{
		// Grab data from the USB CDC/ACM port and check if status is any error
		if (acm_read(&recieveData, sizeof(recieveData)) != sizeof(recieveData)) { debugErr("USB CDC/ACM: Read failure\r\n"); break; /* error, abort reading loop */ }
		else // Successful read
		{
			// Raise the Craft Data flag
			g_modemStatus.craftPortRcvFlag = YES;
		}

		// Write the received data into the buffer
		*g_isrMessageBufferPtr->writePtr = recieveData;

		// Advance the buffer pointer
		g_isrMessageBufferPtr->writePtr++;

		// Check if buffer pointer goes beyond the end
		if (g_isrMessageBufferPtr->writePtr >= (g_isrMessageBufferPtr->msg + CMD_BUFFER_SIZE))
		{
			// Reset the buffer pointer to the beginning of the buffer
			g_isrMessageBufferPtr->writePtr = g_isrMessageBufferPtr->msg;
		}
	}

    return 0;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UsbReportEvents(void)
{
	if ((suspended) || (!configured)) { debug("USB: Suspended or not configured\r\n"); }
	else { debug("USB: Configured\r\n"); }

	if (event_flags)
	{
		if (MXC_GETBIT(&event_flags, MAXUSB_EVENT_NOVBUS)) { MXC_CLRBIT(&event_flags, MAXUSB_EVENT_NOVBUS); debug("USB: VBUS Disconnect\n"); }
		else if (MXC_GETBIT(&event_flags, MAXUSB_EVENT_VBUS)) { MXC_CLRBIT(&event_flags, MAXUSB_EVENT_VBUS); debug("USB: VBUS Connect\n"); }
		else if (MXC_GETBIT(&event_flags, MAXUSB_EVENT_BRST)) { MXC_CLRBIT(&event_flags, MAXUSB_EVENT_BRST); debug("USB: Bus Reset\n"); }
		else if (MXC_GETBIT(&event_flags, MAXUSB_EVENT_BRSTDN)) { MXC_CLRBIT(&event_flags, MAXUSB_EVENT_BRSTDN); debug("USB: Bus Reset Done: %s speed\n", (MXC_USB_GetStatus() & MAXUSB_STATUS_HIGH_SPEED) ? "High" : "Full"); }
		else if (MXC_GETBIT(&event_flags, MAXUSB_EVENT_SUSP)) { MXC_CLRBIT(&event_flags, MAXUSB_EVENT_SUSP); debug("USB: Suspended\n"); }
		else if (MXC_GETBIT(&event_flags, MAXUSB_EVENT_DPACT)) { MXC_CLRBIT(&event_flags, MAXUSB_EVENT_DPACT); debug("USB: Resume\n"); }
		else if (MXC_GETBIT(&event_flags, EVENT_ENUM_COMP)) { MXC_CLRBIT(&event_flags, EVENT_ENUM_COMP); debug("USB: Enumeration complete...\n"); }
		else if (MXC_GETBIT(&event_flags, EVENT_REMOTE_WAKE)) { MXC_CLRBIT(&event_flags, EVENT_REMOTE_WAKE); debug("USB: Remote Wakeup\n"); }
	}
}

// Defined with SPI
//#define STRINGIFY(x) #x
//#define TOSTRING(x) STRINGIFY(x)

#define MAXLEN 256

// Globals
FATFS* fs; //FFat Filesystem Object
FATFS fs_obj;
FIL file; //FFat File Object
FRESULT err; //FFat Result (Struct)
FILINFO fno; //FFat File Information Object
DIR dir; //FFat Directory Object
TCHAR message[MAXLEN], directory[MAXLEN], cwd[MAXLEN], filename[MAXLEN], volume_label[24], volume = '0';
TCHAR *FF_ERRORS[20];
DWORD clusters_free = 0, sectors_free = 0, sectors_total = 0, volume_sn = 0;
UINT bytes_written = 0, bytes_read = 0, mounted = 0;
BYTE work[4096];
static char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.-#'?!";

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void generateMessage(unsigned length)
{
    for (int i = 0; i < length; i++) {
        /*Generate some random data to put in file*/
        message[i] = charset[rand() % (sizeof(charset) - 1)];
    }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int mount(void)
{
    fs = &fs_obj;
    if ((err = f_mount(fs, "", 1)) != FR_OK) { //Mount the default drive to fs now
        debugErr("Unable to open flash drive: %s\r\n", FF_ERRORS[err]);
        f_mount(NULL, "", 0);
    } else {
        debug("Flash drive mounted\r\n");
        mounted = 1;
    }

    f_getcwd(cwd, sizeof(cwd)); //Set the Current working directory

    return err;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int umount(void)
{
    if ((err = f_mount(NULL, "", 0)) != FR_OK) { //Unmount the default drive from its mount point
        debugErr("Unable to unmount volume: %s\r\n", FF_ERRORS[err]);
    } else {
        debug("Flash drive unmounted\r\n");
        mounted = 0;
    }

    return err;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int formatSDHC(void)
{
    debugWarn("\n\n*****THE DRIVE WILL BE FORMATTED IN 5 SECONDS*****r\n");
    debugWarn("**************PRESS ANY KEY TO ABORT**************\r\n\n");
    MXC_UART_ClearRXFIFO(MXC_UART2);
    MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(5000));
    if (MXC_UART_GetRXFIFOAvailable(MXC_UART2) > 0) {
        return E_ABORT;
    }

    debug("Formatting flash drive...\n");

    if ((err = f_mkfs("", FM_ANY, 0, work, sizeof(work))) !=
        FR_OK) { //Format the default drive to FAT32
        debugErr("Formatting flash drive/device failed: %s\r\n", FF_ERRORS[err]);
    } else {
        debug("Flash drive formatted\n");
    }

    mount();

    if ((err = f_setlabel("NOMIS")) != FR_OK) {
        debugErr("Setting drive label failed: %s\r\n", FF_ERRORS[err]);
        f_mount(NULL, "", 0);
    }

    umount();

    return err;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int getSize(void)
{
    if (!mounted) {
        mount();
    }

    if ((err = f_getfree(&volume, &clusters_free, &fs)) != FR_OK) {
        debugErr("Problem finding free size of card: %s\r\n", FF_ERRORS[err]);
        f_mount(NULL, "", 0);
    }

    sectors_total = (fs->n_fatent - 2) * fs->csize;
    sectors_free = clusters_free * fs->csize;

    debug("Disk Size: %u bytes\n", sectors_total / 2);
    debug("Available: %u bytes\n", sectors_free / 2);

    return err;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int ls(void)
{
    if (!mounted) {
        mount();
    }

    debug("Listing Contents of %s - \r\n", cwd);

    if ((err = f_opendir(&dir, cwd)) == FR_OK) {
        while (1) {
            err = f_readdir(&dir, &fno);
            if (err != FR_OK || fno.fname[0] == 0)
                break;

            debug("%s/%s", cwd, fno.fname);

            if (fno.fattrib & AM_DIR) {
                debug("/");
            }

            debug("\n");
        }
        f_closedir(&dir);
    } else {
        debugErr("Unable to opening directory\r\n");
        return err;
    }

    debug("\nFinished listing contents\r\n");

    return err;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int createFile(void)
{
    unsigned int length = 128;

    if (!mounted) {
        mount();
    }

    debug("Enter the name of the text file: \r\n");
    scanf("%255s", filename);
    debug("Enter the length of the file: (256 max)\r\n");
    scanf("%d", &length);
    debug("Creating file %s with length %d\n", filename, length);

    if ((err = f_open(&file, (const TCHAR *)filename, FA_CREATE_ALWAYS | FA_WRITE)) != FR_OK) {
        debugErr("Unable to open file: %s\n", FF_ERRORS[err]);
        f_mount(NULL, "", 0);
        return err;
    }
    debug("File opened\n");

    generateMessage(length);

    if ((err = f_write(&file, &message, length, &bytes_written)) != FR_OK) {
        debugErr("Unable to write file: %s\r\n", FF_ERRORS[err]);
        f_mount(NULL, "", 0);
        return err;
    }
    debug("%d bytes written to file\r\n", bytes_written);

    if ((err = f_close(&file)) != FR_OK) {
        debugErr("Unable to close file: %s\r\n", FF_ERRORS[err]);
        f_mount(NULL, "", 0);
        return err;
    }
    debug("File closed\n");
    return err;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int appendFile(void)
{
    unsigned int length = 0;

    if (!mounted) {
        mount();
    }

    debug("Type name of file to append: \r\n");
    scanf("%255s", filename);
    debug("Type length of random data to append: \r\n");
    scanf("%d", &length);

    if ((err = f_stat((const TCHAR *)filename, &fno)) != FR_OK) {
        debugErr("File %s doesn't exist\r\n", (const TCHAR *)filename);
        return err;
    }
    if ((err = f_open(&file, (const TCHAR *)filename, FA_OPEN_APPEND | FA_WRITE)) != FR_OK) {
        debugErr("Unable to open file %s\r\n", FF_ERRORS[err]);
        return err;
    }
    debug("File opened\n");

    generateMessage(length);

    if ((err = f_write(&file, &message, length, &bytes_written)) != FR_OK) {
        debugErr("Unable to write file: %s\r\n", FF_ERRORS[err]);
        return err;
    }
    debug("Bytes written to file: %d\n", bytes_written);

    if ((err = f_close(&file)) != FR_OK) {
        debugErr("Unable to close file: %s\r\n", FF_ERRORS[err]);
        return err;
    }
    debug("File closed\r\n");
    return err;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int mkdir(void)
{
    if (!mounted) {
        mount();
    }

    debug("Enter directory name: \r\n");
    scanf("%255s", directory);

    err = f_stat((const TCHAR *)directory, &fno);
    if (err == FR_NO_FILE) {
        debug("Creating directory...\r\n");

        if ((err = f_mkdir((const TCHAR *)directory)) != FR_OK) {
            debugErr("Unable to create directory: %s\r\n", FF_ERRORS[err]);
            f_mount(NULL, "", 0);
            return err;
        } else {
            debug("Directory %s created\r\n", directory);
        }

    } else {
        debugWarn("Directory already exists\r\n");
    }
    return err;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int cd(void)
{
    if (!mounted) {
        mount();
    }

    debug("Directory to change into: \r\n");
    scanf("%255s", directory);

    if ((err = f_stat((const TCHAR *)directory, &fno)) == FR_NO_FILE) {
        debugWarn("Directory doesn't exist (Did you mean mkdir?)\r\n");
        return err;
    }

    if ((err = f_chdir((const TCHAR *)directory)) != FR_OK) {
        debugErr("Unable to chdir: %s\r\n", FF_ERRORS[err]);
        f_mount(NULL, "", 0);
        return err;
    }

    debug("Changed to %s\r\n", directory);
    f_getcwd(cwd, sizeof(cwd));

    return err;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int delete(void)
{
    if (!mounted) {
        mount();
    }

    debug("File or directory to delete (always recursive)\r\n");
    scanf("%255s", filename);

    if ((err = f_stat((const TCHAR *)filename, &fno)) == FR_NO_FILE) {
        debugErr("File or directory doesn't exist\r\n");
        return err;
    }

    if ((err = f_unlink(filename)) != FR_OK) {
        debugErr("Unable to delete file\r\n");
        return err;
    }
    debug("Deleted file %s\r\n", filename);
    return err;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int example(void)
{
    unsigned int length = 256;

    if ((err = formatSDHC()) != FR_OK) {
        debugErr("Unable to format flash drive: %s\r\n", FF_ERRORS[err]);
        return err;
    }

    //open SD Card
    if ((err = mount()) != FR_OK) { debugErr("Unable to open flash drive: %s\r\n", FF_ERRORS[err]); return err; }
    debug("Flash drive opened\n");

    if ((err = f_setlabel("NOMIS")) != FR_OK) { debugErr("Problem setting drive label: %s\r\n", FF_ERRORS[err]); f_mount(NULL, "", 0); return err; }

    if ((err = f_getfree(&volume, &clusters_free, &fs)) != FR_OK) { debugErr("Problem finding free size of card: %s\r\n", FF_ERRORS[err]); f_mount(NULL, "", 0); return err; }

    if ((err = f_getlabel(&volume, volume_label, &volume_sn)) != FR_OK) { debugErr("Problem reading drive label: %s\r\n", FF_ERRORS[err]); f_mount(NULL, "", 0); return err;  }

    if ((err = f_open(&file, "0:HelloWorld.txt", FA_CREATE_ALWAYS | FA_WRITE)) != FR_OK) { debugErr("Unable to open file: %s\r\n", FF_ERRORS[err]);f_mount(NULL, "", 0); return err; }
    debug("File opened\n");

    generateMessage(length);

    if ((err = f_write(&file, &message, length, &bytes_written)) != FR_OK) { debugErr("Unable to write file: %s\r\n", FF_ERRORS[err]); f_mount(NULL, "", 0); return err; }
    debug("%d bytes written to file!\n", bytes_written);

    if ((err = f_close(&file)) != FR_OK) { debugErr("Unable to close file: %s\n", FF_ERRORS[err]); f_mount(NULL, "", 0); return err; }
    debug("File closed\n");

    if ((err = f_chmod("HelloWorld.txt", 0, AM_RDO | AM_ARC | AM_SYS | AM_HID)) != FR_OK) { debugErr("Problem with chmod: %s\r\n", FF_ERRORS[err]); f_mount(NULL, "", 0); return err; }

    err = f_stat("MaximSDHC", &fno);
    if (err == FR_NO_FILE) {
        debug("Creating directory...\n");
        if ((err = f_mkdir("MaximSDHC")) != FR_OK) { debugErr("Unable to create directory: %s\r\n", FF_ERRORS[err]); f_mount(NULL, "", 0); return err; }
    }

    debug("Renaming File...\n");
    if ((err = f_rename("0:HelloWorld.txt", "0:MaximSDHC/HelloMaxim.txt")) != FR_OK) { /* /cr: clearify 0:file notation */ debugErr("Unable to move file: %s\r\n", FF_ERRORS[err]); f_mount(NULL, "", 0); return err; }

    if ((err = f_chdir("/MaximSDHC")) != FR_OK) { debugErr("Problem with chdir: %s\r\n", FF_ERRORS[err]); f_mount(NULL, "", 0); return err; }

    debug("Attempting to read back file...\n");
    if ((err = f_open(&file, "HelloMaxim.txt", FA_READ)) != FR_OK) { debugErr("Unable to open file: %s\r\n", FF_ERRORS[err]); f_mount(NULL, "", 0); return err; }

    if ((err = f_read(&file, &message, bytes_written, &bytes_read)) != FR_OK) { debugErr("Unable to read file: %s\r\n", FF_ERRORS[err]); f_mount(NULL, "", 0); return err; }

    debug("Read Back %d bytes\r\n", bytes_read);
    debug("Message: ");
    debug("%s", message);
    debug("\r\n");

    if ((err = f_close(&file)) != FR_OK) { debugErr("Unable to close file: %s\r\n", FF_ERRORS[err]); f_mount(NULL, "", 0); return err; }
    debug("File closed\n");

    //unmount SD Card
    //f_mount(fs, "", 0);
    if ((err = f_mount(NULL, "", 0)) != FR_OK) { debugErr("Problem unmounting volume: %s\r\n", FF_ERRORS[err]); return err; }

    return 0;
}

void TestDriveAndFilesystem(void)
{
    FF_ERRORS[0] = "FR_OK";
    FF_ERRORS[1] = "FR_DISK_ERR";
    FF_ERRORS[2] = "FR_INT_ERR";
    FF_ERRORS[3] = "FR_NOT_READY";
    FF_ERRORS[4] = "FR_NO_FILE";
    FF_ERRORS[5] = "FR_NO_PATH";
    FF_ERRORS[6] = "FR_INVLAID_NAME";
    FF_ERRORS[7] = "FR_DENIED";
    FF_ERRORS[8] = "FR_EXIST";
    FF_ERRORS[9] = "FR_INVALID_OBJECT";
    FF_ERRORS[10] = "FR_WRITE_PROTECTED";
    FF_ERRORS[11] = "FR_INVALID_DRIVE";
    FF_ERRORS[12] = "FR_NOT_ENABLED";
    FF_ERRORS[13] = "FR_NO_FILESYSTEM";
    FF_ERRORS[14] = "FR_MKFS_ABORTED";
    FF_ERRORS[15] = "FR_TIMEOUT";
    FF_ERRORS[16] = "FR_LOCKED";
    FF_ERRORS[17] = "FR_NOT_ENOUGH_CORE";
    FF_ERRORS[18] = "FR_TOO_MANY_OPEN_FILES";
    FF_ERRORS[19] = "FR_INVALID_PARAMETER";
    srand(12347439);
    int run = 1, input = -1;

    while (run)
	{
        f_getcwd(cwd, sizeof(cwd));

        debug("\nChoose one of the following options: \r\n");
        debug("0. Find the Size of the SD Card and Free Space\r\n");
        debug("1. Format the Card\r\n");
        debug("2. Manually Mount Card\r\n");
        debug("3. List Contents of Current Directory\r\n");
        debug("4. Create a Directory\r\n");
        debug("5. Move into a Directory (cd)\r\n");
        debug("6. Create a File of Random Data\r\n");
        debug("7. Add Random Data to an Existing File\r\n");
        debug("8. Delete a File\r\n");
        debug("9. Format Card and Run Exmaple of FatFS Operations\r\n");
        debug("10. Unmount Card and Quit\r\n");
        debug("%s>>", cwd);

        input = -1;
        scanf("%d", &input);
        debug("%d\n", input);

        err = 0;

        switch (input)
		{
			case 0: getSize(); break;
			case 1: formatSDHC(); break;
			case 2: mount(); break;
			case 3: ls(); break;
			case 4: mkdir(); break;
			case 5: cd(); break;
			case 6: createFile(); break;
			case 7: appendFile(); break;
			case 8: delete(); break;
			case 9: example(); break;
			case 10: umount(); run = 0; break;
			default: debugErr("Invalid Selection %d!\r\n", input); err = -1; break;
		}

        if (err >= 0 && err <= 20) { debugErr("Function Returned with code: %d\r\n", FF_ERRORS[err]); }
		else { debug("Function Returned with code: %d\r\n", err); }

        MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(500));
    }
}

typedef enum {
	MXC_SDHC_LIB_LEGACY_TIMING = 0,
	MXC_SDHC_LIB_HIGH_SPEED_TIMING_SDR, // 1
	MXC_SDHC_LIB_HS200_TIMING, // 2
	MXC_SDHC_LIB_HS400_TIMING, // 3
	MXC_SDHC_LIB_HIGH_SPEED_TIMING_DDR = 5 // 5
} mxc_sdhc_hs_timing;

#define MXC_SDHC_LIB_CMD6       0x060A
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int MXC_SDHC_Lib_SetHighSpeedTiming(mxc_sdhc_hs_timing highSpeedTiming)
{
	/*
		This field is 0 after power-on, H/W reset or software reset, thus selecting the backwards compatible interface timing for the Device
		If the host sets 1 to this field, the Device changes the timing to high speed interface timing (see Section 10.6.1 of JESD84-B50)
		If the host sets value 2, the Device changes its timing to HS200 interface timing (see Section 10.8.1 of JESD84-B50)
		If the host sets HS_TIMING [3:0] to 0x3, the device changes it’s timing to HS400 interface timing (see 10.10)
	*/

	mxc_sdhc_cmd_cfg_t cmd_cfg;
	uint32_t card_status;
	int result;

	cmd_cfg.direction = MXC_SDHC_DIRECTION_CFG;
	cmd_cfg.callback = NULL;

	cmd_cfg.host_control_1 = MXC_SDHC_Get_Host_Cn_1();
	if (highSpeedTiming == MXC_SDHC_LIB_LEGACY_TIMING)
	{
		cmd_cfg.arg_1 = 0x03B90000; // Legacy timing
		cmd_cfg.host_control_1 &= ~MXC_F_SDHC_HOST_CN_1_HS_EN; // Select Normal speed
	}
	else if (highSpeedTiming == MXC_SDHC_LIB_HIGH_SPEED_TIMING_SDR)
	{
		cmd_cfg.arg_1 = 0x03B90100; // High speed timing SDR
		cmd_cfg.host_control_1 |= MXC_F_SDHC_HOST_CN_1_HS_EN; // Select High speed
	}
	else if (highSpeedTiming == MXC_SDHC_LIB_HS200_TIMING)
	{
		cmd_cfg.arg_1 = 0x03B90200; // High speed timing
		cmd_cfg.host_control_1 |= MXC_F_SDHC_HOST_CN_1_HS_EN; // Select High speed
	}
	else if (highSpeedTiming == MXC_SDHC_LIB_HS400_TIMING)
	{
		// Can't use HS400 mode, data width can't be set to 8
		result = E_BAD_STATE;
	}
#if 0 /* Not supported by MCU */
	else if (highSpeedTiming == MXC_SDHC_LIB_HIGH_SPEED_TIMING_DDR)
	{
		cmd_cfg.arg_1 = 0x03B90500; // High speed timing DDR
		cmd_cfg.host_control_1 |= MXC_F_SDHC_HOST_CN_1_HS_EN; // Select High speed
	}
#endif
	else { result = E_BAD_STATE; }

	cmd_cfg.command = MXC_SDHC_LIB_CMD6;
	result = MXC_SDHC_SendCommand(&cmd_cfg);

	// Setup the UHS mode as SDR25
	MXC_SDHC->host_cn_2 &= MXC_F_SDHC_HOST_CN_2_UHS;
	if (highSpeedTiming == MXC_SDHC_LIB_LEGACY_TIMING) { MXC_SDHC->host_cn_2 |= MXC_S_SDHC_HOST_CN_2_UHS_SDR12;	}
	else if (highSpeedTiming == MXC_SDHC_LIB_HIGH_SPEED_TIMING_SDR) { MXC_SDHC->host_cn_2 |= MXC_S_SDHC_HOST_CN_2_UHS_SDR25;	}
	else if (highSpeedTiming == MXC_SDHC_LIB_HS200_TIMING) { MXC_SDHC->host_cn_2 |= MXC_S_SDHC_HOST_CN_2_UHS_SDR50;	}

	/* Wait until card busy (D0 low) disappears */
	while (MXC_SDHC_Card_Busy()) {}
	card_status = MXC_SDHC_Get_Response32();

	if ((result == E_NO_ERROR) && (card_status & 0x80))
	{
		/* SWITCH_ERROR */
		result = E_BAD_STATE;
	}

	return result;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupSDHCeMMC(void)
{
    mxc_sdhc_cfg_t cfg;
	mxc_sdhc_hs_timing timingMode;
	mxc_sdhc_lib_card_type cardType;

    // Initialize SDHC peripheral
    cfg.bus_voltage = MXC_SDHC_Bus_Voltage_1_8;
    cfg.block_gap = 0;
    cfg.clk_div = 0x96; // Large divide ratio, setting frequency to 400 kHz during Card Identification phase

#if 0 /* Interface call assigns incorrect GPIO (P0.31/SDHC_CDN and P1.2/SDHC_WP) */
    if (MXC_SDHC_Init(&cfg) != E_NO_ERROR) { debugErr("SDHC/eMMC initialization failed\r\n"); }
#else /* Manual setup */
	mxc_gpio_cfg_t gpio_cfg_sdhc_1 = { GPIO_SDHC_PORT, (GPIO_SDHC_CMD_PIN | GPIO_SDHC_DAT2_PIN | GPIO_SDHC_DAT3_PIN | GPIO_SDHC_DAT0_PIN | GPIO_SDHC_CLK_PIN | GPIO_SDHC_DAT1_PIN),
										MXC_GPIO_FUNC_ALT1, MXC_GPIO_PAD_NONE, MXC_GPIO_VSSEL_VDDIO };

    MXC_SYS_ClockEnable(MXC_SYS_PERIPH_CLOCK_SDHC);

    MXC_GPIO_Config(&gpio_cfg_sdhc_1);
    gpio_cfg_sdhc_1.port->vssel &= ~(gpio_cfg_sdhc_1.mask); // Set voltage select to MXC_GPIO_VSSEL_VDDIO, since it seems digital interface is at 1.8V
    gpio_cfg_sdhc_1.port->ds_sel0 |= gpio_cfg_sdhc_1.mask; // Set drive strength to 2x (borrowing from internal driver)

	// Setup the 1.8V Signaling Enable
	MXC_SDHC->host_cn_2 |= MXC_F_SDHC_HOST_CN_2_1_8V_SIGNAL;

    if (MXC_SDHC_RevA_Init((mxc_sdhc_reva_regs_t *)MXC_SDHC, &cfg) != E_NO_ERROR) { debugErr("SDHC/eMMC initialization failed\r\n"); }
#endif

    // Set up card to get it ready for a transaction
    if (MXC_SDHC_Lib_InitCard(10) == E_NO_ERROR) { debug("SDHC: Card/device Initialized\r\n"); }
	else { debugErr("SDHC: No card/device response\n"); }

    cardType = MXC_SDHC_Lib_Get_Card_Type();
	if (cardType == CARD_MMC) { debug("SDHC: Card type discovered is MMC/eMMC\r\n"); }
	else if (cardType == CARD_SDHC) { debug("SDHC: Card type discovered is SD/SDHC\r\n"); }
	else { debugErr("SDHC: No card type found\r\n"); }

	/*
		Note: The 0-52 MHz eMMC devices supported the legacy SDR mode as well as a newer transfer mode introduced by JEDEC version 4.4 called Dual Data Rate (DDR)
		DDR mode allows the transfer of two bits on each data line per clock cycle, one per clock edge; This helps achieve a transfer rate of up to 104 MB/s

		32651 Datasheet: Supports SDR50 with SDHC clock of up to 60MHz (30MB/sec) -or- Supports DDR50 with SDHC clock of up to 30MHz (30MB/sec)
	*/

	// Change the timing mode
	// Note: HS DDR mode does not look supported by MCU
#if 1 /* Start with HS SDR */
	// Set timing mode to High Speed SDR (60MHz @ 30MB/sec)
	timingMode = MXC_SDHC_LIB_HIGH_SPEED_TIMING_SDR;
#else /* Try HS200 timing at some point */
	// Set timing mode to UHS SDR50 (30MHz @ 30MB/sec)
	timingMode = MXC_SDHC_LIB_HS200_TIMING;
#endif

	// Enable high speed timing when sorted out
	MXC_SDHC_Lib_SetHighSpeedTiming(timingMode);

    // Configure for best clock divider, must not exceed 52 MHz for eMMC in Legacy mode, or use lower clock rate for High Speed DDR mode (max 30MHz)
#if 1 /* Limit setting HS SDR to 60MHz until data exchange verified */
	if (SystemCoreClock > 96000000)
#else /* More selective control on clock selection allowing HS SDR to run 60MHz */
    if (((SystemCoreClock > 96000000) && (timingMode == MXC_SDHC_LIB_LEGACY_TIMING)) || (timingMode == MXC_SDHC_LIB_HIGH_SPEED_TIMING_DDR))
#endif
	{
        debug("SD clock ratio (at card/device) is 4:1, %dMHz, (eMMC not to exceed 52 MHz for legacy or high speed modes)\r\n", (SystemCoreClock / 4));
        MXC_SDHC_Set_Clock_Config(1);
    }
	else // Use smallest clock divider for fastest clock rate (max 60MHz)
	{
        debug("SD clock ratio (at card/device) is 2:1, %dMHz\r\n", (SystemCoreClock / 2));
        MXC_SDHC_Set_Clock_Config(0);
    }

	// Swap over to quad data mode, although it looks like every transaction proceeds with setting the bus width again
	MXC_SDHC_Lib_SetBusWidth(MXC_SDHC_LIB_QUAD_DATA);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupDriveAndFilesystem(void)
{
    // Mount the default drive to determine if the filesystem is created
	if ((err = f_mount(&fs_obj, "", 1)) != FR_OK)
	{
		// Check if failure was due to no filesystem
		if (err == FR_NO_FILESYSTEM)
		{
			debug("Drive(eMMC): Formatting...\r\n");

			// Format the default drive to a FAT filesystem
			if ((err = f_mkfs("", FM_ANY, 0, work, sizeof(work))) !=  FR_OK)
			{
				debugErr("Drive(eMMC): Formatting failed with error %s\r\n", FF_ERRORS[err]);
			}
			else
			{
				debug("Drive(eMMC): Formatted successfully\r\n");

				// Remount
				if ((err = f_mount(&fs_obj, "", 1)) != FR_OK)
				{
					debugErr("Drive(eMMC): filed to mount after formatting, with error %s\r\n", FF_ERRORS[err]);
				}

				if ((err = f_setlabel("NOMIS")) != FR_OK)
				{
					debugErr("Drive(eMMC): Setting label failed with error %s\r\n", FF_ERRORS[err]);
					f_mount(NULL, "", 0);
				}
			}
		}
		else
		{
			debugErr("Drive(eMMC): filed to mount with error %s\r\n", FF_ERRORS[err]);
			f_mount(NULL, "", 0);
		}
    }
	else
	{
        debug("Drive(eMMC): mounted successfully\r\n");
    }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GetDriveSize(void)
{
    if ((err = f_getfree(&volume, &clusters_free, &fs)) != FR_OK) { debugErr("Unable to find free size of card: %s\r\n", FF_ERRORS[err]); f_mount(NULL, "", 0); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void EnableMpuCycleCounter(void)
{
	// Check if the Cycle Counter Enable is already set in the DWT Control register
	if (DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk)
	{
		debug("DWT: Cycle Counter ready (already enable)\r\n");
	}
	else
	{
		// Set the Cycle Enable bit in the DWT Control register
		DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
		debug("DWT: Cycle Counter enabled\r\n");
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DisableMpuCycleCounter(void)
{
	// Clear the Cycle Enable bit in the DWT Control register
	DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupInteralPITTimer(uint8_t channel, uint16_t freq)
{
	// Either MILLISECOND_TIMER or TYPEMATIC_TIMER

	mxc_tmr_regs_t* mxcTimerPtr;
	mxc_sys_reset_t mxcTimerReset;
	mxc_sys_periph_clock_t mxcTimerClock;
	int timerIrqNum;
	void (*irqHandlerFunc)(void);

	if (channel == MILLISECOND_TIMER)
	{
		mxcTimerPtr = MILLISECOND_TIMER_NUM;
		mxcTimerReset = MILLISECOND_TIMER_RESET;
		mxcTimerClock = MILLISECOND_TIMER_PERIPH_CLOCK;
		timerIrqNum = MILLISECOND_TIMER_IRQ;
		irqHandlerFunc = Tc_ms_timer_irq;
	}
	else // TYPEMATIC_TIMER
	{
		mxcTimerPtr = TYPEMATIC_TIMER_NUM;
		mxcTimerReset = TYPEMATIC_TIMER_RESET;
		mxcTimerClock = TYPEMATIC_TIMER_PERIPH_CLOCK;
		timerIrqNum = TYPEMATIC_TIMER_IRQ;
		irqHandlerFunc = Tc_typematic_irq;
	}

	// Reset preipheral
	MXC_SYS_Reset_Periph(mxcTimerReset);

	// Start timer clock
	MXC_SYS_ClockEnable(mxcTimerClock);

    // Clear interrupt flag
    mxcTimerPtr->intr = MXC_F_TMR_INTR_IRQ;

    // Set the prescaler (TMR_PRES_4096)
	mxcTimerPtr->cn |= (MXC_S_TMR_CN_PRES_DIV1);

    // Set the mode
	mxcTimerPtr->cn |= TMR_MODE_CONTINUOUS << MXC_F_TMR_CN_TMODE_POS;

	// Set the polarity
    mxcTimerPtr->cn |= (0) << MXC_F_TMR_CN_TPOL_POS; // Polarity (0 or 1) doesn't matter

	// Init the compare value
    mxcTimerPtr->cmp = (60000000 / freq);

	// Init the counter
    mxcTimerPtr->cnt = 0x1;

	// Setup the Timer 0 interrupt
	NVIC_ClearPendingIRQ(timerIrqNum);
    NVIC_DisableIRQ(timerIrqNum);
    MXC_NVIC_SetVector(timerIrqNum, irqHandlerFunc);
    NVIC_EnableIRQ(timerIrqNum);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void StartInteralPITTimer(PIT_TIMER_NUM channel)
{
	mxc_tmr_regs_t* mxcTimerPtr = NULL;

	switch (channel)
	{
		case MILLISECOND_TIMER:
			mxcTimerPtr = MILLISECOND_TIMER_NUM;
			g_msTimerTicks = 0;
			break;

		case TYPEMATIC_TIMER:
			mxcTimerPtr = TYPEMATIC_TIMER_NUM;
			g_tcTypematicTimerActive = YES;
			break;
	}

	// Enable the timer
	if (mxcTimerPtr) { mxcTimerPtr->cn |= MXC_F_TMR_CN_TEN; }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void StopInteralPITTimer(PIT_TIMER_NUM channel)
{
	mxc_tmr_regs_t* mxcTimerPtr = NULL;

	switch (channel)
	{
		case MILLISECOND_TIMER:
			mxcTimerPtr = MILLISECOND_TIMER_NUM;
			g_msTimerTicks = 0;
			break;

		case TYPEMATIC_TIMER:
			mxcTimerPtr = TYPEMATIC_TIMER_NUM;
			g_tcTypematicTimerActive = NO;
			break;
	}

	// Disable the timer
	if (mxcTimerPtr) { mxcTimerPtr->cn &= ~MXC_F_TMR_CN_TEN; }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupHalfSecondTickTimer(void)
{
#if 0 /* Internal PIT Timer based, will not generate interrupts in Deepsleep or Backup */
	MXC_SYS_Reset_Periph(MXC_SYS_RESET_TIMER0);
	MXC_SYS_ClockEnable(MXC_SYS_PERIPH_CLOCK_TIMER0);

    // Clear interrupt flag
    CYCLIC_HALF_SEC_TIMER_NUM->intr = MXC_F_TMR_INTR_IRQ;

    // Set the prescaler (TMR_PRES_4096)
	CYCLIC_HALF_SEC_TIMER_NUM->cn |= (MXC_F_TMR_CN_PRES3);
	CYCLIC_HALF_SEC_TIMER_NUM->cn |= (MXC_V_TMR_CN_PRES_DIV4096);

    // Set the mode
	CYCLIC_HALF_SEC_TIMER_NUM->cn |= TMR_MODE_CONTINUOUS << MXC_F_TMR_CN_TMODE_POS;

	// Set the polarity
    CYCLIC_HALF_SEC_TIMER_NUM->cn |= (0) << MXC_F_TMR_CN_TPOL_POS; // Polarity (0 or 1) doesn't matter

	// Init the compare value
    CYCLIC_HALF_SEC_TIMER_NUM->cmp = 7324; // 60MHz clock / 4096 = 14648 counts/sec, 1/2 second count = 7324

	// Init the counter
    CYCLIC_HALF_SEC_TIMER_NUM->cnt = 0x1;

	// Setup the Timer 0 interrupt
	NVIC_ClearPendingIRQ(TMR0_IRQn);
    NVIC_DisableIRQ(TMR0_IRQn);
    MXC_NVIC_SetVector(TMR0_IRQn, Soft_timer_tick_irq);
    NVIC_EnableIRQ(TMR0_IRQn);

	// Enable the timer
	CYCLIC_HALF_SEC_TIMER_NUM->cn |= MXC_F_TMR_CN_TEN;
#else /* Internal RTC based off of Sub-Second Alarm register, will generate interrupts in sleep modes */
    while (MXC_RTC_Init(0, 0) == E_BUSY) {}
    while (MXC_RTC_DisableInt(MXC_F_RTC_CTRL_SSEC_ALARM_EN) == E_BUSY) {}
    while (MXC_RTC_SetSubsecondAlarm(2048) == E_BUSY) {} // 4K clock, 2048 = 1/2 second
	MXC_NVIC_SetVector(RTC_IRQn, Internal_rtc_alarms);
    while (MXC_RTC_EnableInt(MXC_F_RTC_CTRL_SSEC_ALARM_EN) == E_BUSY) {}
    while (MXC_RTC_Start() == E_BUSY) {}
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ValidatePowerOn(void)
{
	uint8_t powerOnButtonDetect;
	uint8_t vbusChargingDetect;
	uint16_t i;

	SetupPowerOnDetectGPIO();

	powerOnButtonDetect = GetPowerOnButtonState();
	vbusChargingDetect = GetPowerGoodBatteryChargerState();

	if (powerOnButtonDetect) { debugRaw("\r\n-----------------------\r\nPower On button pressed\r\n"); }
	if (vbusChargingDetect) { debugRaw("\r\nUSB Charging detected\r\n"); }

	// Check if Power on button is the startup source
	if (powerOnButtonDetect)
	{
		debugRaw("(2 second press validation) Waiting");

		// Monitor Power on button for 2 secs making sure it remains depressed signaling desire to turn unit on
		for (i = 0; i < 40; i++)
		{
			MXC_Delay(MXC_DELAY_MSEC(50));
			debugRaw(".");

			// Determine if the Power on button was released early
			if (GetPowerOnButtonState() == OFF)
			{
				debugRaw("\r\nPower On qualificaiton not met, Turning off\r\n");

				// Power on button released therefore startup condition not met, shut down
				PowerControl(MCU_POWER_LATCH, OFF);
				while (1) { /* Wait for darkness (caps bleed and unit powers off) */}
			}
		}

		debugRaw(" Power On activated\r\n");

		// Unit startup condition verified, latch power and continue
		PowerControl(MCU_POWER_LATCH, ON);

		// Todo: Turn on appropriate LED
		//PowerControl(LED???, ON);
	}
	// Check if USB charging is startup source
	else if (vbusChargingDetect)
	{
		// Note: USB charging is reason for power up, could be USB or another source like 12V external battery (where setting Aux power enable is needed)
		// Setup Battery Charger and Fuel Gauge, then monitor for user power on
		BatteryChargerInit();
		FuelGaugeInit();

		// Todo: Turn on appropriate LED
		//PowerControl(LED???, ON);

		while (1)
		{
			// Check if Power On button depressed
			if (GetPowerOnButtonState() == ON)
			{
				debugRaw("(2 second press validation) Power On button depress detected, Waiting");

				// Monitor Power on button for 2 secs making sure it remains depressed signaling desire to turn unit on
				for (i = 0; i < 40; i++)
				{
					MXC_Delay(MXC_DELAY_MSEC(50));
					debugRaw(".");

					// Determine if the Power on button was released early
					if (GetPowerOnButtonState() == OFF)
					{
						debugRaw("\r\nPower On qualificaiton not met\r\n");
						// Break the For loop
						break;
					}
				}

				debugRaw(" Power On activated\r\n");

				// Unit startup condition verified, latch power and continue
				PowerControl(MCU_POWER_LATCH, ON);
				// Break the While loop and proceed with unit startup
				break;
			}
		}
	}
	else
	{
		debugWarn("MCU Power latch is power on source\r\n");
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t IdentiifyI2C(uint8_t i2cNum, uint8_t readBytes, uint8_t regAddr)
{
	char deviceName[50];
	uint8_t identified = NO;

	switch (regAddr)
	{
		case I2C_ADDR_ACCELEROMETER: sprintf(deviceName, "Accelerometer"); identified = YES; break;
		case I2C_ADDR_1_WIRE: sprintf(deviceName, "1-Wire"); identified = YES; break;
		case I2C_ADDR_EEPROM: sprintf(deviceName, "EEPROM"); identified = YES; break;
		case I2C_ADDR_EEPROM_ID: sprintf(deviceName, "EEPROM ID"); identified = YES; break;
		case I2C_ADDR_BATT_CHARGER: sprintf(deviceName, "Battery Charger"); identified = YES; break;
		case I2C_ADDR_USBC_PORT_CONTROLLER: sprintf(deviceName, "USB Port Controller"); identified = YES; break;
		case I2C_ADDR_EXTERNAL_RTC: sprintf(deviceName, "External RTC"); identified = YES; break;
		case I2C_ADDR_FUEL_GUAGE: sprintf(deviceName, "Fuel Gauge"); identified = YES; break;
		case I2C_ADDR_EXPANSION: sprintf(deviceName, "Expansion"); identified = YES; break;
	}

	if (identified) { debug("(R%d) I2C%d device identified: %s (0x%x)\r\n", readBytes, i2cNum, deviceName, regAddr); }

	return (identified);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitSystemHardware_MS9300(void)
{
	//-------------------------------------------------------------------------
	// Setup Debug Uart (UART2)
	//-------------------------------------------------------------------------
	SetupDebugUART();

	//-------------------------------------------------------------------------
	// Check power on source and validate for system startup
	//-------------------------------------------------------------------------
	ValidatePowerOn();

	//-------------------------------------------------------------------------
	// Display Debug Banner (UART2)
	//-------------------------------------------------------------------------
	DebugUartInitBanner();

	//-------------------------------------------------------------------------
	// Enable Cycle Counter
	//-------------------------------------------------------------------------
	EnableMpuCycleCounter();

	//-------------------------------------------------------------------------
	// Setup Watchdog
	//-------------------------------------------------------------------------
    SetupWatchdog();

	//-------------------------------------------------------------------------
	// Enable the instruction cache
	//-------------------------------------------------------------------------
    SetupICC();

	//-------------------------------------------------------------------------
	// Setup UART0 (LTE) and UART1 (BLE)
	//-------------------------------------------------------------------------
	SetupUART();

	//-------------------------------------------------------------------------
	// Setup I2C0 (1.8V devices) and I2C1 (3.3V devices)
	//-------------------------------------------------------------------------
	SetupI2C();

	//-------------------------------------------------------------------------
	// Setup SPI3 (ADC) and SPI2 (LCD)
	//-------------------------------------------------------------------------
	// SPI3 GPIO pin config (pins_me10.c, library code) is outside the scope of this project code and incorrectly assigns Port 0 Pin 15
	SetupSPI();

	//-------------------------------------------------------------------------
	// Setup all GPIO
	//-------------------------------------------------------------------------
	// Must init after SPI to correct for a wrong GPIO pin config in the library (and allow this project to work with an unmodified framework)
	SetupAllGPIO();

#if 1 /* Test device addresses */
	uint8_t regAddr;
	uint8_t regData[2];
    mxc_i2c_req_t masterRequest;
	int status;
	uint8_t numDevices;

	for (uint8_t i = 0; i < 2; i++)
	{
		MXC_Delay(MXC_DELAY_SEC(1));
		debug("-- I2C Test, Cycle %d --\r\n", i);

		if (i == 1)
		{
			debug("-- Power up SS, Exp --\r\n");
			SetSmartSensorSleepState(OFF);
			// Bring up Expansion
			PowerControl(EXPANSION_ENABLE, ON);
			MXC_Delay(MXC_DELAY_MSEC(500));
			PowerControl(EXPANSION_RESET, OFF);
		}

		if (i == 1)
		{
			debug("-- Power up 5V --\r\n");
			// Bring up Analog 5V
			PowerControl(ANALOG_5V_ENABLE, ON);
			WaitAnalogPower5vGood();
		}

		if (i == 2)
		{
			debug("-- Power down 5V --\r\n");
			PowerControl(ANALOG_5V_ENABLE, OFF);
			MXC_Delay(MXC_DELAY_SEC(1));
		}

#if 0
		else
		{
			debug("Power down 5V & Exp\r\n");
			// Turn off Analog 5V
			PowerControl(ANALOG_5V_ENABLE, OFF);
			// Turn off Expansion
			PowerControl(EXPANSION_RESET, ON);
			PowerControl(EXPANSION_ENABLE, OFF);
		}
#endif

		numDevices = 0;
		for (regAddr = 0; regAddr < 0x80; regAddr++)
		{
			memset(&regData[0], 0, sizeof(regData));
			masterRequest.i2c = MXC_I2C0;
			masterRequest.addr = regAddr;
			masterRequest.tx_buf = NULL;
			masterRequest.tx_len = 0;
			masterRequest.rx_buf = &regData[0];
			masterRequest.rx_len = sizeof(regData);
			masterRequest.restart = 0;
			masterRequest.callback = NULL;

			status = MXC_I2C_MasterTransaction(&masterRequest); if (status == E_SUCCESS) { numDevices++; if (IdentiifyI2C(0, 2, regAddr) == NO) debug("(R2) I2C0 device @ 0x%x, status: %d, data: 0x%x 0x%x\r\n", regAddr, status, regData[0], regData[1]); }
			//else if (status == E_COMM_ERR) { debug("(R2) No I2C0 device @ 0x%x (Comm error)\r\n", regAddr); }
			//else { debug("(R2) Possible I2C0 device @ 0x%x, status: %d, data: 0x%x 0x%x\r\n", regAddr, status, regData[0], regData[1]); }
			memset(&regData[0], 0, sizeof(regData));
			masterRequest.i2c = MXC_I2C1;
			status = MXC_I2C_MasterTransaction(&masterRequest); if (status == E_SUCCESS) { numDevices++; if (IdentiifyI2C(1, 2, regAddr) == NO) debug("(R2) I2C1 device @ 0x%x, status: %d, data: 0x%x 0x%x\r\n", regAddr, status, regData[0], regData[1]); }
			//else if (status == E_COMM_ERR) { debug("(R2) No I2C1 device @ 0x%x (Comm error)\r\n", regAddr); }
			//else { debug("(R2) Possible I2C1 device @ 0x%x, status: %d, data: 0x%x 0x%x\r\n", regAddr, status, regData[0], regData[1]); }
		}
		debug("(R2) I2C devices found: %d\r\n", numDevices);

#if 0 /* Read 1 */
		numDevices = 0;
		for (regAddr = 0; regAddr < 0x80; regAddr++)
		{
			memset(&regData[0], 0, sizeof(regData));
			masterRequest.i2c = MXC_I2C0;
			masterRequest.addr = regAddr;
			masterRequest.tx_buf = NULL;
			masterRequest.tx_len = 0;
			masterRequest.rx_buf = &regData[0];
			masterRequest.rx_len = sizeof(uint8_t);
			masterRequest.restart = 0;
			masterRequest.callback = NULL;

			status = MXC_I2C_MasterTransaction(&masterRequest); if (status == E_SUCCESS) { numDevices++; if (IdentiifyI2C(0, 1, regAddr) == NO) debug("(R1) I2C0 device @ 0x%x, status: %d, data: 0x%x\r\n", regAddr, status, regData[0]); }
			//else if (status == E_COMM_ERR) { debug("(R1) No I2C0 device @ 0x%x (Comm error)\r\n", regAddr); }
			//else { debug("(R1) Possible I2C0 device @ 0x%x, status: %d, data: 0x%x\r\n", regAddr, status, regData[0]); }
			memset(&regData[0], 0, sizeof(regData));
			masterRequest.i2c = MXC_I2C1;
			status = MXC_I2C_MasterTransaction(&masterRequest); if (status == E_SUCCESS) { numDevices++; if (IdentiifyI2C(1, 1, regAddr) == NO) debug("(R1) I2C1 device @ 0x%x, status: %d, data: 0x%x\r\n", regAddr, status, regData[0]); }
			//else if (status == E_COMM_ERR) { debug("(R1) No I2C1 device @ 0x%x (Comm error)\r\n", regAddr); }
			//else { debug("(R1) Possible I2C1 device @ 0x%x, status: %d, data: 0x%x\r\n", regAddr, status, regData[0]); }
		}
		debug("(R1) I2C devices found: %d\r\n", numDevices);
#endif
	} // for i
	debug("-- Power down 5V --\r\n");
	PowerControl(ANALOG_5V_ENABLE, OFF);
	MXC_Delay(MXC_DELAY_SEC(1));
#endif

	//-------------------------------------------------------------------------
	// Setup SDHC/eMMC
	//-------------------------------------------------------------------------
	SetupSDHCeMMC();
#if 1 /* Test a second init call */
	MXC_Delay(MXC_DELAY_MSEC(500));
	SetupSDHCeMMC();
#endif
	//-------------------------------------------------------------------------
	// Setup Drive(eMMC) and Filesystem
	//-------------------------------------------------------------------------
	SetupDriveAndFilesystem();

	//-------------------------------------------------------------------------
	// Setup USB Composite (MSC + CDC/ACM)
	//-------------------------------------------------------------------------
	SetupUSBComposite();

	//-------------------------------------------------------------------------
	// Setup Half Second tick timer
	//-------------------------------------------------------------------------
	SetupHalfSecondTickTimer();

	//-------------------------------------------------------------------------
	// Disable all interrupts
	//-------------------------------------------------------------------------
	// Todo: Determine if this is necessary
#if 0
	__disable_irq();
#endif

	//-------------------------------------------------------------------------
	// Setup Internal PIT Timers
	//-------------------------------------------------------------------------
	SetupInteralPITTimer(MILLISECOND_TIMER, 1000);
	SetupInteralPITTimer(TYPEMATIC_TIMER, 1000);

	//-------------------------------------------------------------------------
	// Set Alarm 1 and Alarm 2 low (Active high control)
	//-------------------------------------------------------------------------
	PowerControl(ALARM_1_ENABLE, OFF); // Default, technically done with SetupGPIO call
	PowerControl(ALARM_2_ENABLE, OFF); // Default, technically done with SetupGPIO call

	//-------------------------------------------------------------------------
	// Set Trigger Out low (Active high control)
	//-------------------------------------------------------------------------
	PowerControl(TRIGGER_OUT, OFF); // Technically done with SetupGPIO call
	MXC_GPIO_ClearFlags(GPIO_EXTERNAL_TRIGGER_IN_PORT, GPIO_EXTERNAL_TRIGGER_IN_PIN); // Clear External Trigger In interrupt flag (Port 1, Pin 31)

	//---------------------
	// Device specific init
	//---------------------
	// Accelerometer
	// 1-Wire
	// Battery Charger
	// USB-C Port Controller
	// External RTC
	// Fuel Gauge
	// Expansion I2C bridge
	// External ADC
	// LCD
	// Keypad
	// EEPROM (should be none)
	// eMMC + FF driver (should be none)

	//-------------------------------------------------------------------------
	// Initalize the Accelerometer
	//-------------------------------------------------------------------------
	AccelerometerInit(); debug("Accelerometer: Init complete\r\n");

	//-------------------------------------------------------------------------
	// Smart Sensor data/control init (Hardware pull up on signal)
	//-------------------------------------------------------------------------
	OneWireInit(); debug("One Wire Driver: Init complete\r\n");

	//-------------------------------------------------------------------------
	// Initalize the Battery Charger
	//-------------------------------------------------------------------------
	BatteryChargerInit(); debug("Battery Charger: Init complete\r\n");

	//-------------------------------------------------------------------------
	// Initalize the USB-C Port Controller
	//-------------------------------------------------------------------------
	USBCPortControllerInit(); debug("USB-C Port Controller: Init complete\r\n");

	//-------------------------------------------------------------------------
	// Initialize the external RTC
	//-------------------------------------------------------------------------
	ExternalRtcInit(); debug("External RTC: Init complete\r\n");

	//-------------------------------------------------------------------------
	// Initalize the Fuel Gauge
	//-------------------------------------------------------------------------
	FuelGaugeInit(); debug("Fuel Gauge: Init complete\r\n");

	//-------------------------------------------------------------------------
	// Initalize the Expansion I2C UART Bridge
	//-------------------------------------------------------------------------
	ExpansionBridgeInit(); debug("Expansion I2C Uart Bridge: Init complete\r\n");

#if 1 /* Test */
	//-------------------------------------------------------------------------
	// Test EERPOM
	//-------------------------------------------------------------------------
	TestEEPROM();
#endif

	//-------------------------------------------------------------------------
	// Initialize the AD Control
	//-------------------------------------------------------------------------
	AnalogControlInit(); debug("Analog Control: Init complete\r\n");

#if 1 /* Test */
	//-------------------------------------------------------------------------
	// Test Expanded battery presence
	//-------------------------------------------------------------------------
	uint32_t j = 16000000;
	debug("Expanded Battery Presence Test...\r\n");
	while (1)
	{
		if (GetExpandedBatteryPresenceState() == YES) { debugErr("Expanded Battery Presence: False detection of 2nd battery pack\r\n"); break; }
		if (--j == 0) { break; }
	}
	if (j == 0) { debug("Expanded Battery Presence Test passed\r\n"); }
#endif

#if 1 /* Test */
	//-------------------------------------------------------------------------
	// Test I2C
	//-------------------------------------------------------------------------
	debug("I2C Test Loop (forever)\r\n");
	//uint16_t testReg16;
	uint8_t testBootStatus[6];
	uint8_t regA = 0x2D;
	uint8_t ramData = 0xA5;
	j = 1;
	SetRtcRegisters(PCF85263_CTL_RAM_BYTE, &ramData, sizeof(ramData));
	while (1)
	{
		//GetBattChargerRegister(BATT_CHARGER_STATUS_AND_FAULT_REGISTER_0, &testReg16);
		WriteI2CDevice(MXC_I2C0, I2C_ADDR_USBC_PORT_CONTROLLER, &regA, sizeof(uint8_t), testBootStatus, sizeof(testBootStatus));
		GetRtcRegisters(PCF85263_CTL_RAM_BYTE, &ramData, sizeof(ramData));
		MXC_Delay(MXC_DELAY_MSEC(1));
		if (j++ % 1000 == 0) { debugRaw("."); }
	}
#endif

	//-------------------------------------------------------------------------
	// Init and configure the A/D to prevent the unit from burning current charging internal reference (default config)
	//-------------------------------------------------------------------------
#if 0 /* Normal */
	InitExternalAD(); debug("External ADC: Init complete\r\n");
#else
	while (1)
	{
		InitExternalAD(); debug("External ADC: Init complete\r\n");
		MXC_Delay(1);
	}
#endif

	//-------------------------------------------------------------------------
	// Init the LCD display
	//-------------------------------------------------------------------------
#if 0 /* Disabled until LCD connector fixed or hardware modded */
	InitLCD(); debug("LCD Display: Init complete\r\n");
#endif

	//-------------------------------------------------------------------------
	// Init Keypad
	//-------------------------------------------------------------------------
	InitExternalKeypad(); debug("Keyboard: Init complete\r\n");

	//-------------------------------------------------------------------------
	// Disable unused subsystems to save power
	//-------------------------------------------------------------------------
#if 0 /* Normal */
	AdjustPowerSavings(POWER_SAVINGS_NORMAL); debug("Power Savings: Init complete\r\n");
#else /* Initial test */
	// Make sure no subsystems are incorrectly disabled
	//AdjustPowerSavings(POWER_SAVINGS_NONE);
	AdjustPowerSavings(POWER_SAVINGS_HIGH);
#endif
	//-------------------------------------------------------------------------
	// Read and cache Smart Sensor data
	//-------------------------------------------------------------------------
	SmartSensorReadRomAndMemory(SEISMIC_SENSOR); debug("Smart Sensor check for Seismic sensor\r\n");
	SmartSensorReadRomAndMemory(ACOUSTIC_SENSOR); debug("Smart Sensor check for Acoustic sensor\r\n");

	//-------------------------------------------------------------------------
	// Hardware initialization complete
	//-------------------------------------------------------------------------
	debug("Hardware Init complete\r\n");
}
