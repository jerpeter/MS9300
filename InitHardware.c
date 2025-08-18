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
#include "tmr.h"
#include "mxc_delay.h"
//#include "usb_protocol.h"
// SDHC includes
#include "mxc_device.h"
#include "sdhc_regs.h"
#include "tmr.h"
#include "mxc_delay.h"
#include "sdhc_lib.h"
#include "ff.h"
#include "sdhc_reva.h"
#include "sdhc_reva_regs.h"
#include "spi_reva_regs.h"
#include "spi_reva1.h"

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

#if 1 /* Test */
#include "math.h"
#endif

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
	uint16 keyScan = READ_KEY_BUTTON_MAP;
	if (keyScan)
	{
		debugWarn("Keypad button being pressed (likely a bug), Key: %x\r\n", keyScan);
	}

#if 0 /* Test */
	debug("Keypad: Looking for keys... (Press Power key to stop)\r\n");
	while (1)
	{
		keyScan = READ_KEY_BUTTON_MAP;

		if (keyScan) { debug("Keys:"); }
		if (keyScan & 0x001) { debugRaw(" <Soft Key 4>"); }
		if (keyScan & 0x002) { debugRaw(" <Soft Key 3>"); }
		if (keyScan & 0x004) { debugRaw(" <Soft Key 2>"); }
		if (keyScan & 0x008) { debugRaw(" <Soft Key 1>"); }
		if (keyScan & 0x010) { debugRaw(" <Enter>"); }
		if (keyScan & 0x020) { debugRaw(" <Right>"); }
		if (keyScan & 0x040) { debugRaw(" <Left>"); }
		if (keyScan & 0x080) { debugRaw(" <Down>"); }
		if (keyScan & 0x100) { debugRaw(" <Up>"); }
		if (keyScan) { debug("\r\n"); }

		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(5));

		if (GetPowerOnButtonState() == ON) { debug("Keypad: Loop break\r\n");break; }
	}
#endif

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
void InitExternalADC(void)
{
	// Check if External ADC is in reset
	if (GetPowerControlState(ADC_RESET) == ON) { WaitAnalogPower5vGood(); }

	// Configure External ADC
	AD4695_Init();

	// Setup the A/D Channel configuration
#if 1 /* Normal */
	SetupADChannelConfig(SAMPLE_RATE_DEFAULT, OVERRIDE_ENABLE_CHANNEL_VERIFICATION);
#else /* Skip until hardware fix for current problems */
	g_adChannelConfig = FOUR_AD_CHANNELS_WITH_READBACK_WITH_TEMP;
	AD4695_SetTemperatureSensorEnable(((g_adChannelConfig != FOUR_AD_CHANNELS_NO_READBACK_NO_TEMP) ? YES : NO));
	AD4695_EnterConversionMode(((g_adChannelConfig == FOUR_AD_CHANNELS_WITH_READBACK_WITH_TEMP) ? YES : NO));
#endif

#if 0 /* Test loop */
extern uint16_t dataTemperature;
	SAMPLE_DATA_STRUCT tempData;
	uint32_t i = 0;
	while (1)
	{
#if 1
		ReadAnalogData(&tempData);
		//debug("Ext ADC (Batt: %1.3f): R:%04x T:%04x V:%04x A:%04x TempF:%d\r\n", (double)GetExternalVoltageLevelAveraged(BATTERY_VOLTAGE), tempData.r, tempData.t, tempData.v, tempData.a, AD4695_TemperatureConversionCtoF(dataTemperature));
		debug("Ext ADC (%s): R:%04x T:%04x V:%04x A:%04x TempF:%d\r\n", FuelGaugeDebugString(), tempData.r, tempData.t, tempData.v, tempData.a, AD4695_TemperatureConversionCtoF(dataTemperature));
		if (FuelGaugeGetCurrentAbs() > 500000) { debug("Fuel Gauge: Current over 500 (%d)\r\n", FuelGaugeGetCurrentAbs()); break; }
		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(100));
		//if (i++ % 100 == 0) { debugRaw("."); }
		//if (i++ % 10000 == 0) { debugRaw("."); }
#else
		debug("Monitor Sensor Enables: %s\r\n", FuelGaugeDebugString());
		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(1)); tempData.a++; dataTemperature++;
		if (FuelGaugeGetCurrentAbs() > 500000) { debug("Fuel Gauge: Current over 500 (%d)\r\n", FuelGaugeGetCurrentAbs()); break; }
#endif
	}
	AD4695_ExitConversionMode();
	i = 0; while (1)
	{
		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(1)); debug("Fuel Gauge: %s\r\n", FuelGaugeDebugString());
		if (i++ > 20) { break; }
	}

	debug("Ext ADC: Disable Sensor Blocks\r\n");
	DisableSensorBlocks();
	i = 0; while (1)
	{
		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(1)); debug("Fuel Gauge: %s\r\n", FuelGaugeDebugString());
		if (i++ > 20) { break; }
	}

	while (FuelGaugeGetCurrentAbs() > 500000)
	{
		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(1));
		debug("Fuel Gauge: Current high, %s\r\n", FuelGaugeDebugString());
	}

	debug("Ext ADC: Re-Entering conversions and Get Channel Offsets\r\n");
	// Setup the A/D Channel configuration
	SetupADChannelConfig(SAMPLE_RATE_DEFAULT, UNIT_CONFIG_CHANNEL_VERIFICATION);
	debug("Fuel Gauge: %s\r\n", FuelGaugeDebugString());
	debug("Fuel Gauge: %s\r\n", FuelGaugeDebugString());
#endif

#if 0 /* Test loop */
	while (1)
	{
		GetChannelOffsets(SAMPLE_RATE_DEFAULT);
	}
#endif

#if 0 /* Test */
	debug("Fuel Gauge: %s\r\n", FuelGaugeDebugString());
	GetChannelOffsets(SAMPLE_RATE_DEFAULT);
	debug("Fuel Gauge: %s\r\n", FuelGaugeDebugString());
	GetChannelOffsets(SAMPLE_RATE_DEFAULT);
	debug("Fuel Gauge: %s\r\n", FuelGaugeDebugString());
#endif
	// Read a few test samples
	GetChannelOffsets(SAMPLE_RATE_DEFAULT);
	debug("Fuel Gauge: %s\r\n", FuelGaugeDebugString());
	AD4695_ExitConversionMode();
	debug("Fuel Gauge: %s\r\n", FuelGaugeDebugString());

	debug("Ext ADC: Disable Sensor Blocks\r\n");
	debug("Fuel Gauge: %s\r\n", FuelGaugeDebugString());
	DisableSensorBlocks();
	debug("Fuel Gauge: %s\r\n", FuelGaugeDebugString());

#if 0 /* Test */
	while (FuelGaugeGetCurrentAbs() > 500000)
	{
		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(1));
		debug("Fuel Gauge: Current high, %s\r\n", FuelGaugeDebugString());
	}
	debug("Fuel Gauge: ADC put in reset and Analog 5V power off\r\n");
#endif

	PowerControl(ADC_RESET, ON);

#if 0 /* Test */
	while (FuelGaugeGetCurrentAbs() > 500000)
	{
		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(1));
		debug("Fuel Gauge: %s\r\n", FuelGaugeDebugString());
	}
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitLCD(void)
{
	// Power up and init the display controller
	ft81x_init();

#if 0 /* Bitmap display to LCD not currently working */
	// Load the logo bitmap into the display controller and send to LCD
	DisplayLogoToLcd();
#else
#if 0 /* Test */
extern void test_logo(void);
	//test_logo();
#endif

	ft81x_NomisLoadScreen();
#endif

#if 0 /* Test displaying the Main menu */
	BuildLanguageLinkTable(ENGLISH_LANG);
	INPUT_MSG_STRUCT mn_msg;
	debug("Jumping to Main Menu\r\n");
	SETUP_MENU_MSG(MAIN_MENU);
	JUMP_TO_ACTIVE_MENU();
#endif

#if 0 /* Test disable of LCD for now */
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(3));
	PowerControl(LCD_POWER_ENABLE, OFF);
	PowerControl(LCD_POWER_DOWN, ON);
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(1));
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitCellLTE(void)
{
	if(GetPowerControlState(CELL_ENABLE) == OFF)
	{
		debug("Cell/LTE: Powering section...\r\n");
		PowerControl(CELL_ENABLE, ON);
		SoftUsecWait(1 * SOFT_SECS); // Small charge up delay
		debug("Cell/LTE: Disabling LTE reset...\r\n");
		PowerControl(LTE_RESET, OFF);

		if (CheckforModemReady(6) == YES)
		{
			// Flag for yes if prior state was not available
			g_modemStatus.modemAvailable = YES;
			ClearSoftTimer(MODEM_DELAY_TIMER_NUM);
		}
	}

#if 1 /* Test TCP Server */
#if 0 /* Test removal */
		// Temp disable Auto Dialout
		g_modemSetupRecord.modemStatus = NO;
		ClearSoftTimer(AUTO_DIAL_OUT_CYCLE_TIMER_NUM);
#endif

		int strLen, status;
#if 0 /* Test */
		// System mode setting
		//debug("AT%%XSYSTEMMODE...\r\n"); sprintf((char*)g_spareBuffer, "AT%%XSYSTEMMODE=0,1,0,0\r\n"); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }

#if 0 /* Test verify */
		SoftUsecWait(3 * SOFT_SECS);
		{ ProcessCraftData(); if (getSystemEventState(CRAFT_PORT_EVENT)) { clearSystemEventFlag(CRAFT_PORT_EVENT); RemoteCmdMessageProcessing(); } }

		debug("AT%%XSYSTEMMODE?...\r\n"); sprintf((char*)g_spareBuffer, "AT%%XSYSTEMMODE?\r\n"); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
		SoftUsecWait(3 * SOFT_SECS);
		{ ProcessCraftData(); if (getSystemEventState(CRAFT_PORT_EVENT)) { clearSystemEventFlag(CRAFT_PORT_EVENT); RemoteCmdMessageProcessing(); } }
#endif
#endif
		if (strlen(g_cellModemSetupRecord.pdnApn))
		{
			// PDN/APN setting, AT+CGDCONT=0,"IP","psmtneofin"
			debug("AT+CGDCONT=0,\"IP\",\"%s\"...\r\n", g_cellModemSetupRecord.pdnApn); sprintf((char*)g_spareBuffer, "AT+CGDCONT=0,\"IP\",\"%s\"\r\n", g_cellModemSetupRecord.pdnApn); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
			{ SoftUsecWait(500 * SOFT_MSECS); ProcessCraftData(); if (getSystemEventState(CRAFT_PORT_EVENT)) { clearSystemEventFlag(CRAFT_PORT_EVENT); RemoteCmdMessageProcessing(); } }
		}

		if (g_cellModemSetupRecord.pdnAuthProtocol != AUTH_NONE)
		{
			debug("AT+CGAUTH=0,%d,\"%s\",\"%s\"...\r\n", g_cellModemSetupRecord.pdnAuthProtocol, g_cellModemSetupRecord.pdnUsername, g_cellModemSetupRecord.pdnPassword);
			sprintf((char*)g_spareBuffer, "AT+CGAUTH=0,%d,\"%s\",\"%s\"\r\n", g_cellModemSetupRecord.pdnAuthProtocol, g_cellModemSetupRecord.pdnUsername, g_cellModemSetupRecord.pdnPassword);
			strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
			{ SoftUsecWait(500 * SOFT_MSECS); ProcessCraftData(); if (getSystemEventState(CRAFT_PORT_EVENT)) { clearSystemEventFlag(CRAFT_PORT_EVENT); RemoteCmdMessageProcessing(); } }
		}

#if 0 /* Test verify */
		debug("AT+CGDCONT?...\r\n"); sprintf((char*)g_spareBuffer, "AT+CGDCONT?\r\n"); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
		{ SoftUsecWait(500 * SOFT_MSECS); ProcessCraftData(); if (getSystemEventState(CRAFT_PORT_EVENT)) { clearSystemEventFlag(CRAFT_PORT_EVENT); RemoteCmdMessageProcessing(); } }
#endif
		debug("AT+CFUN=1...\r\n"); sprintf((char*)g_spareBuffer, "AT+CFUN=1\r\n"); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
		{ SoftUsecWait(500 * SOFT_MSECS); ProcessCraftData(); if (getSystemEventState(CRAFT_PORT_EVENT)) { clearSystemEventFlag(CRAFT_PORT_EVENT); RemoteCmdMessageProcessing(); } }

		while (g_modemStatus.remoteResponse != MODEM_CELL_NETWORK_REGISTERED)
		{
			debug("AT+CEREG?...\r\n"); sprintf((char*)g_spareBuffer, "AT+CEREG?\r\n"); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
			{ SoftUsecWait(1000 * SOFT_MSECS); ProcessCraftData(); if (getSystemEventState(CRAFT_PORT_EVENT)) { clearSystemEventFlag(CRAFT_PORT_EVENT); RemoteCmdMessageProcessing(); } }
		}

		debug("AT#XTCPSVR=1,%d...\r\n", g_cellModemSetupRecord.tcpServerListenPort); sprintf((char*)g_spareBuffer, "AT#XTCPSVR=1,%d\r\n", g_cellModemSetupRecord.tcpServerListenPort); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
		{ SoftUsecWait(500 * SOFT_MSECS); ProcessCraftData(); if (getSystemEventState(CRAFT_PORT_EVENT)) { clearSystemEventFlag(CRAFT_PORT_EVENT); RemoteCmdMessageProcessing(); } }

		while ((g_modemStatus.remoteResponse == TCP_SERVER_NOT_STARTED) || (g_modemStatus.remoteResponse == ERROR_RESPONSE))
		{
			SoftUsecWait(5 * SOFT_SECS);
			debug("AT#XTCPSVR=1,%d...\r\n", g_cellModemSetupRecord.tcpServerListenPort); sprintf((char*)g_spareBuffer, "AT#XTCPSVR=1,%d\r\n", g_cellModemSetupRecord.tcpServerListenPort); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
			{ SoftUsecWait(500 * SOFT_MSECS); ProcessCraftData(); if (getSystemEventState(CRAFT_PORT_EVENT)) { clearSystemEventFlag(CRAFT_PORT_EVENT); RemoteCmdMessageProcessing(); } }
		}

		debug("AT#XTCPSEND"); sprintf((char*)g_spareBuffer, "AT#XTCPSEND\r\n"); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
		{ SoftUsecWait(500 * SOFT_MSECS); ProcessCraftData(); if (getSystemEventState(CRAFT_PORT_EVENT)) { clearSystemEventFlag(CRAFT_PORT_EVENT); RemoteCmdMessageProcessing(); } }
 #endif

	//debug("Cell/LTE: Powered...\r\n");

#if 0 /* Shut down power to cell module */
	PowerControl(CELL_ENABLE, OFF);
	debug("Cell/LTE: Power off\r\n");
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestInternalRAM(void)
{
	uint32 i, j;
	uint32 index;
	uint32 printErrors = 0;
	uint32 testSize = EVENT_BUFF_SIZE_IN_WORDS_PLUS_EVT_RECORD; // Was (EVENT_BUFF_SIZE_IN_WORDS) - 614400

#if EXTENDED_DEBUG
	debug("RAM Test: Incrementing index with rolling increment...\r\n");
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
			debugErr("Test of RAM: failed (Index: %d, Address: 0x%x, Expected: 0x%x, Got: 0x%x)\r\n",
			index, &g_eventDataBuffer[index], (uint16)(i + j), g_eventDataBuffer[index]);
			printErrors++; if (printErrors > 5000) { debugErr("Too many errors, bailing on memory test\r\n"); return; }
		}
		i++;
		if ((i & 0xFFFF) == 0) { j++; }
	}

	if (printErrors) { debug("RAM: Total errors: %d\r\n", printErrors); }
#if EXTENDED_DEBUG
	else { debug("Test of RAM: passed\r\n"); }
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
	// LED 2: Port 1, Pin 26, Output, No external pull, Active low, 3.3V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_LED_2_PORT;
	setupGPIO.mask = GPIO_LED_2_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_WEAK_PULL_UP;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start as off
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

#if /* New board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN)
	//----------------------------------------------------------------------------------------------------------------------
	// External Battery Presence Slot 1: Port 0, Pin 2, Input, No external pullup, Active high, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_EXT_BATTERY_PRESENCE_1_PORT;
	setupGPIO.mask = GPIO_EXT_BATTERY_PRESENCE_1_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)External_battery_presence_irq, NULL);
	MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_BOTH); //MXC_GPIO_INT_RISING);
	MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);
#else /* HARDWARE_ID_REV_PROTOTYPE_1 */
	//----------------------------------------------------------------------------------------------------------------------
	// Expanded Battery Detect: Port 0, Pin 2, Input, External pullup, Active high, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_EXPANDED_BATTERY_PORT;
	setupGPIO.mask = GPIO_EXPANDED_BATTERY_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)External_battery_presence_irq, NULL);
	MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_BOTH); //MXC_GPIO_INT_RISING);
	MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);
#endif

#if /* New board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN)
	//----------------------------------------------------------------------------------------------------------------------
	// External Battery Presence Slot 2: Port 0, Pin 3, Input, No external pullup, Active high, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_EXT_BATTERY_PRESENCE_2_PORT;
	setupGPIO.mask = GPIO_EXT_BATTERY_PRESENCE_2_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)External_battery_presence_irq, NULL);
	MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_BOTH); //MXC_GPIO_INT_RISING);
	MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);
#else /* HARDWARE_ID_REV_PROTOTYPE_1 */
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
#endif

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
	// Battery Charger IRQ: Port 0, Pin 5, Input, External pullup, Active low, 3.3V, Interrupt
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_BATTERY_CHARGER_IRQ_PORT;
	setupGPIO.mask = GPIO_BATTERY_CHARGER_IRQ_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH;
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

#if /* New board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN)
	//----------------------------------------------------------------------------------------------------------------------
	// Sensor Detect 1: Port 0, Pin 7, Input, No external pullup, Active high, 1.8, Interrupt
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_SENSOR_DETECT_1_PORT;
	setupGPIO.mask = GPIO_SENSOR_DETECT_1_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)Sensor_detect_1_irq, NULL);
	MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_BOTH);
	MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);
#else /* HARDWARE_ID_REV_PROTOTYPE_1 */
	//----------------------------------------------------------------------------------------------------------------------
	// Enable 5V: Port 0, Pin 7, Output, External pulldown, Active high, 1.8V (minimum 0.9V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_ENABLE_5V_PORT;
	setupGPIO.mask = GPIO_ENABLE_5V_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start enabled since controls removed
#endif

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
#if 1 /* Original */
	MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);
#else /* Wait until Expansion I2C Bridge is powered to enable */
#endif

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
#if 1 /* Test line with interrupt */
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)Power_good_battery_charger_irq, NULL);
	MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_BOTH);
	MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);
#endif

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

	//----------------------------------------------------------------------------------------------------------------------
	// External ADC SPI3 Serial Clock: Port 0, Pin 16, Input when not powered, SPI Control when powered, External pullup, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_ADC_SPI3_SCK_PORT;
	setupGPIO.mask = GPIO_ADC_SPI3_SCK_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);

	//----------------------------------------------------------------------------------------------------------------------
	// External ADC Busy Alt Gpio0: Port 0, Pin 17, Input, External pullup, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_ADC_BUSY_ALT_GP0_PORT;
	setupGPIO.mask = GPIO_ADC_BUSY_ALT_GP0_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);

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
	// External ADC SPI3 Slave Select 0: Port 0, Pin 19, Input when not powered, SPI Control when powered, External pullup, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_ADC_SPI3_SS0_PORT;
	setupGPIO.mask = GPIO_ADC_SPI3_SS0_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);

	//----------------------------------------------------------------------------------------------------------------------
	// External ADC SPI3 Serial Data Out: Port 0, Pin 20, Input when not powered, SPI Control when powered, External pullup, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_ADC_SPI3_SDO1_PORT;
	setupGPIO.mask = GPIO_ADC_SPI3_SDO1_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);

	//----------------------------------------------------------------------------------------------------------------------
	// External ADC SPI3 Serial Data In: Port 0, Pin 21, Input when not powered, SPI Control when powered, External pullup, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_ADC_SPI3_SDI_PORT;
	setupGPIO.mask = GPIO_ADC_SPI3_SDI_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);

	//----------------------------------------------------------------------------------------------------------------------
	// Cal Mux Pre-A/D Enable: Port 0, Pin 22, Output, External pulldown, Active low, 3.3V (minimum 2V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_CAL_MUX_PRE_AD_ENABLE_PORT;
	setupGPIO.mask = GPIO_CAL_MUX_PRE_AD_ENABLE_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH;
	MXC_GPIO_Config(&setupGPIO);
#if 1 /* Original */
	MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start as disabled
#else /* Test starting as enabled so not back powering the analog section */
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as enabled to prevent back powering
#endif

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

#if /* New board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN)
	//----------------------------------------------------------------------------------------------------------------------
	// Sensor Detect 2: Port 1, Pin 2, Input, No external pullup, Active high, 1.8, Interrupt
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_SENSOR_DETECT_2_PORT;
	setupGPIO.mask = GPIO_SENSOR_DETECT_2_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)Sensor_detect_2_irq, NULL);
	MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_BOTH);
	MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);
#else /* Old board - HARDWARE_ID_REV_PROTOTYPE_1 */
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
#endif

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
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)Accelerometer_irq, NULL);
	MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_RISING);
	MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);

#if /* New board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN)
	//----------------------------------------------------------------------------------------------------------------------
	// Sensor Detect 3: Port 1, Pin 13, Input, No external pullup, Active high, 1.8, Interrupt
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_SENSOR_DETECT_3_PORT;
	setupGPIO.mask = GPIO_SENSOR_DETECT_3_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)Sensor_detect_3_irq, NULL);
	MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_BOTH);
	MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);
#else /* Old board - HARDWARE_ID_REV_PROTOTYPE_1 */
	//----------------------------------------------------------------------------------------------------------------------
	// Accel Int 2: Port 1, Pin 13, Input, No external pull, Active high, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_ACCEL_INT_2_PORT;
	setupGPIO.mask = GPIO_ACCEL_INT_2_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)Accelerometer_irq, NULL);
	MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_RISING);
	MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);
#endif

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
	MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_BOTH); //MXC_GPIO_INT_FALLING);
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
	MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_BOTH); //MXC_GPIO_INT_FALLING);
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
	MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_BOTH); //MXC_GPIO_INT_FALLING);
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
	MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_BOTH); //MXC_GPIO_INT_FALLING);
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
	MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_BOTH); //MXC_GPIO_INT_FALLING);
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
	MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_BOTH); //MXC_GPIO_INT_FALLING);
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
	MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_BOTH); //MXC_GPIO_INT_FALLING);
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
	MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_BOTH); //MXC_GPIO_INT_FALLING);
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
	MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_BOTH); //MXC_GPIO_INT_FALLING);
	MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);

#if /* New board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN)
	//----------------------------------------------------------------------------------------------------------------------
	// LED 1: Port 1, Pin 25, Output, No external pull, Active high, 3.3V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_LED_1_PORT;
	setupGPIO.mask = GPIO_LED_1_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_WEAK_PULL_UP;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH;
	MXC_GPIO_Config(&setupGPIO);
	//MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as on (Beta/re-spin reversed)
	MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start as off (Beta/re-spin reversed)
#else /* Old board - HARDWARE_ID_REV_PROTOTYPE_1 */
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
#endif

#if /* New board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN)
	//----------------------------------------------------------------------------------------------------------------------
	// LED 2: Port 1, Pin 26, Output, No external pull, Active high, 3.3V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_LED_2_PORT;
	setupGPIO.mask = GPIO_LED_2_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_WEAK_PULL_UP;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as on (Beta/re-spin reversed)
	//MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start as off (Beta/re-spin reversed)
#else /* Old board - HARDWARE_ID_REV_PROTOTYPE_1 */
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
#endif

#if /* New board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN)
	//----------------------------------------------------------------------------------------------------------------------
	// Sensor Detect 4: Port 1, Pin 27, Input, No external pullup, Active high, 1.8, Interrupt
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_SENSOR_DETECT_4_PORT;
	setupGPIO.mask = GPIO_SENSOR_DETECT_4_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)Sensor_detect_4_irq, NULL);
	MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_BOTH);
	MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);
#else /* Old board - HARDWARE_ID_REV_PROTOTYPE_1 */
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
#endif

	//----------------------------------------------------------------------------------------------------------------------
	// RTC Int A: Port 1, Pin 28, Input, External pullup, Active low, 1.8V (minimum 0.66V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_EXT_RTC_INTA_PORT;
	setupGPIO.mask = GPIO_EXT_RTC_INTA_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
#if 0 /* ISR removal option for 1 board where RTC Int is triggering to start */
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)External_rtc_irq, NULL);
	MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_FALLING);
	MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);
#endif
#if 1 /* Test periodic 1 second interrupt */
extern void External_rtc_periodic_timer(void);
	MXC_GPIO_RegisterCallback(&setupGPIO, (mxc_gpio_callback_fn)External_rtc_periodic_timer, NULL);
	MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_FALLING);
	MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);
#endif

#if /* New board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN)
	//----------------------------------------------------------------------------------------------------------------------
	// RTC Timestamp: Port 1, Pin 29, Output, No external pull, Active high, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_EXT_RTC_TIMESTAMP_PORT;
	setupGPIO.mask = GPIO_EXT_RTC_TIMESTAMP_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as off
#else /* Old board - HARDWARE_ID_REV_PROTOTYPE_1 */
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
#endif

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

#if /* New board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN)
	//----------------------------------------------------------------------------------------------------------------------
	// SPI2 Slave Select 1 Accelerometer: Port 2, Pin 1, Output, External pullup, Active low, 1.8V (minimum 1.7V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_SPI2_SS1_ACC_PORT;
	setupGPIO.mask = GPIO_SPI2_SS1_ACC_PIN;
	//setupGPIO.func = MXC_GPIO_FUNC_ALT1; // SPI2 Master would control, but setup for LCD uses manual control
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start as inactive
#else /* Old board - HARDWARE_ID_REV_PROTOTYPE_1 */
	//----------------------------------------------------------------------------------------------------------------------
	// LCD Power Down: Port 2, Pin 1, Output, External pulldown, Active low, 1.8V (minimum 1.7V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_LCD_POWER_DOWN_PORT;
	setupGPIO.mask = GPIO_LCD_POWER_DOWN_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as disabled
#endif

	//----------------------------------------------------------------------------------------------------------------------
	// SPI2 Serial Clock: Port 2, Pin 2, Input when not powered, SPI Control when powered, No external pull, 1.8V (minimum 1.7V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_SPI2_SCK_PORT;
	setupGPIO.mask = GPIO_SPI2_SCK_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);

	//----------------------------------------------------------------------------------------------------------------------
	// SPI2 Master In Slave Out: Port 2, Pin 3, Input when not powered, SPI Control when powered, No external pull, 1.8V (minimum 1.7V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_SPI2_MISO_PORT;
	setupGPIO.mask = GPIO_SPI2_MISO_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);

	//----------------------------------------------------------------------------------------------------------------------
	// SPI2 Master Out Slave In: Port 2, Pin 4, Input when not powered, SPI Control when powered, No external pull, 1.8V (minimum 1.7V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_SPI2_MOSI_PORT;
	setupGPIO.mask = GPIO_SPI2_MOSI_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_IN;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);

#if 0 /* Moved to LCD Power up section */
	//----------------------------------------------------------------------------------------------------------------------
	// SPI2 Slave Select 0 LCD: Port 2, Pin 5, Output, External pullup, Active low, 1.8V (minimum 1.7V)
	//----------------------------------------------------------------------------------------------------------------------
	if (FT81X_SPI_2_SS_CONTROL_MANUAL)
	{
		setupGPIO.port = GPIO_SPI2_SS0_LCD_PORT;
		setupGPIO.mask = GPIO_SPI2_SS0_LCD_PIN;
		setupGPIO.func = MXC_GPIO_FUNC_OUT;
		setupGPIO.pad = MXC_GPIO_PAD_NONE;
		setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
		MXC_GPIO_Config(&setupGPIO);
#if 0 /* Orignal */
		MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start as disabled
#else /* Start as enabled to prevent back powering the LCD until it's on */
		MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as enabled to prevent back powering
#endif
	}
#endif

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
#if 0 /* Original */
	MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);
#else /* Wait until LCD Controller is powered to enable */
#endif

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

#if /* New board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN)
	//----------------------------------------------------------------------------------------------------------------------
	// LCD Power Down: Port 2, Pin 12, Output, External pulldown, Active low, 1.8V (minimum 1.7V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_LCD_POWER_DOWN_PORT;
	setupGPIO.mask = GPIO_LCD_POWER_DOWN_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as disabled
#endif

	//----------------------------------------------------------------------------------------------------------------------
	// LTE Reset: Port 2, Pin 13, Output, External pull up, Active low, 3.3V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_LTE_RESET_PORT;
	setupGPIO.mask = GPIO_LTE_RESET_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH;
	MXC_GPIO_Config(&setupGPIO);
#if 0 /* Orignal */
	MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start as disabled
#else /* Test starting as enabled so not back powering the Cell section */
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as disabled
#endif

#if /* New board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN)
	//----------------------------------------------------------------------------------------------------------------------
	// Unused GPIO 1: Port 2, 14, Output, No external pull up, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_UNUSED_1_PORT;
	setupGPIO.mask = GPIO_UNUSED_1_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as disabled
#endif

#if /* New board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN)
	//----------------------------------------------------------------------------------------------------------------------
	// Unused GPIO 2: Port 2, 15, Output, No external pull up, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_UNUSED_2_PORT;
	setupGPIO.mask = GPIO_UNUSED_2_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&setupGPIO);
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as disabled
#else /* Old board - HARDWARE_ID_REV_PROTOTYPE_1 */
	//----------------------------------------------------------------------------------------------------------------------
	// BLE Reset: Port 2, 15, Output, External pull up, Active low, 3.3V
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_BLE_RESET_PORT;
	setupGPIO.mask = GPIO_BLE_RESET_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH;
	MXC_GPIO_Config(&setupGPIO);
#if 0 /* Orignal */
	MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start as disabled
#else /* Test starting as enabled so not back powering the Cell section */
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as disabled
#endif
#endif

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
#if 1 /* Original */
	MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start as disabled (960Hz select)
#else /* Start as enabled to prevent back powering the 5V analog section */
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as enabled to prevent back powering
#endif

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
	//MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as disabled
	MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start as enabled

	//----------------------------------------------------------------------------------------------------------------------
	// Sensor Enable Aop1: Port 3, Pin 2, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_SENSOR_ENABLE_AOP1_PORT;
	setupGPIO.mask = GPIO_SENSOR_ENABLE_AOP1_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
	MXC_GPIO_Config(&setupGPIO);
	//MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as disabled
	MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start as enabled

	//----------------------------------------------------------------------------------------------------------------------
	// Sensor Enable Geo2: Port 3, Pin 3, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_SENSOR_ENABLE_GEO2_PORT;
	setupGPIO.mask = GPIO_SENSOR_ENABLE_GEO2_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
	MXC_GPIO_Config(&setupGPIO);
	//MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as disabled
	MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start as enabled

	//----------------------------------------------------------------------------------------------------------------------
	// Sensor Enable Aop2: Port 3, Pin 4, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_SENSOR_ENABLE_AOP2_PORT;
	setupGPIO.mask = GPIO_SENSOR_ENABLE_AOP2_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
	MXC_GPIO_Config(&setupGPIO);
	//MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as disabled
	MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start as enabled

	//----------------------------------------------------------------------------------------------------------------------
	// Gain Select Geo1: Port 3, Pin 5, Output, External pulldown, Select, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_GAIN_SELECT_GEO1_PORT;
	setupGPIO.mask = GPIO_GAIN_SELECT_GEO1_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
	MXC_GPIO_Config(&setupGPIO);
#if 1 /* Original */
	MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start as high (Normal gain)
#else /* Start as low to prevent possibly back powering the 5V analog section */
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as low (High gain) to prevent back powering
#endif

	//----------------------------------------------------------------------------------------------------------------------
	// Path Select Aop1: Port 3, Pin 6, Output, External pulldown, Select, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_PATH_SELECT_AOP1_PORT;
	setupGPIO.mask = GPIO_PATH_SELECT_AOP1_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
	MXC_GPIO_Config(&setupGPIO);
#if 1 /* Original */
	MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start as high (AOP path)
#else /* Start as low to prevent possibly back powering the 5V analog section */
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as low (A-weighting path) to prevent back powering
#endif

	//----------------------------------------------------------------------------------------------------------------------
	// Gain Select Geo2: Port 3, Pin 7,Output, External pulldown, Select, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_GAIN_SELECT_GEO2_PORT;
	setupGPIO.mask = GPIO_GAIN_SELECT_GEO2_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
	MXC_GPIO_Config(&setupGPIO);
#if 1 /* Original */
	MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start as high (Normal gain)
#else /* Start as low to prevent possibly back powering the 5V analog section */
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as low (High gain) to prevent back powering
#endif

	//----------------------------------------------------------------------------------------------------------------------
	// Path Select Aop2: Port 3, Pin 8, Output, External pulldown, Select, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	setupGPIO.port = GPIO_PATH_SELECT_AOP2_PORT;
	setupGPIO.mask = GPIO_PATH_SELECT_AOP2_PIN;
	setupGPIO.func = MXC_GPIO_FUNC_OUT;
	setupGPIO.pad = MXC_GPIO_PAD_NONE;
	setupGPIO.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
	MXC_GPIO_Config(&setupGPIO);
#if 1 /* Original */
	MXC_GPIO_OutSet(setupGPIO.port, setupGPIO.mask); // Start as high (AOP path)
#else /* Start as low to prevent possibly back powering the 5V analog section */
	MXC_GPIO_OutClr(setupGPIO.port, setupGPIO.mask); // Start as low (A-weighting path) to prevent back powering
#endif

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
	MXC_GPIO_IntConfig(&setupGPIO, MXC_GPIO_INT_RISING);
#if 0 /* Original */
	MXC_GPIO_EnableInt(setupGPIO.port, setupGPIO.mask);
#else /* Only enabling when the Analog 5V section is powered and the RTC clock is enabled to prevent an early trigger */
#endif

#if 0 /* Moved to Init Interrupts section */
	//----------------------------------------------------------------------------------------------------------------------
	// Enable IRQ's for any of the appropritate GPIO input interrupts
	//----------------------------------------------------------------------------------------------------------------------
	NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(MXC_GPIO0)));
	NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(MXC_GPIO1)));
	NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(MXC_GPIO2)));
	NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(MXC_GPIO3)));
#else /* Test GPIO0 */
	NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(MXC_GPIO0)));
	NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(MXC_GPIO1)));
	NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(MXC_GPIO2)));
	NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(MXC_GPIO3)));
#endif
}

#define UART_BUFFER_SIZE 512
uint8_t g_Uart0_RxBuffer[UART_BUFFER_SIZE];
uint8_t g_Uart0_TxBuffer[UART_BUFFER_SIZE];
uint8_t g_Uart1_RxBuffer[UART_BUFFER_SIZE];
uint8_t g_Uart1_TxBuffer[UART_BUFFER_SIZE];
//uint8_t g_Uart2_RxBuffer[UART_BUFFER_SIZE];
//uint8_t g_Uart2_TxBuffer[UART_BUFFER_SIZE];

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
mxc_uart_req_t uart0ReadRequest; // Needs persistant storage because Maxim UART driver grabs only a reference to the request object
void SetupCellModuleRxUART(void)
{
	int status;

	status = MXC_UART_Init(MXC_UART0, UART_BAUD);
	if (status != E_SUCCESS) { debugErr("UART0 failed init with code: %d\r\n", status); }

	// Move to Interrupt init
	NVIC_ClearPendingIRQ(UART0_IRQn);
	NVIC_DisableIRQ(UART0_IRQn);
	MXC_NVIC_SetVector(UART0_IRQn, UART0_Handler);
	NVIC_EnableIRQ(UART0_IRQn);

	// Setup the asynchronous request
	uart0ReadRequest.uart = MXC_UART0;
	uart0ReadRequest.rxData = g_Uart0_RxBuffer;
	uart0ReadRequest.rxLen = 1; // Trigger size
	uart0ReadRequest.txLen = 0;
	uart0ReadRequest.callback = UART0_Read_Callback;

	status = MXC_UART_TransactionAsync(&uart0ReadRequest);
	if (status != E_SUCCESS) { debugErr("Uart0 Read setup (async) failed with code: %d\r\n", status); }

#if 1 /* Test to check interrupt flags set */
	debug("Uart0: Interrupt enables are 0x%0x, Int flags are 0x%0x, Status is 0x%0x\r\n", MXC_UART0->int_en, MXC_UART0->int_fl, MXC_UART0->stat);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
mxc_uart_req_t uart1ReadRequest; // Needs persistant storage because Maxim UART driver grabs only a reference to the request object
void SetupCellModuleTxUART(void)
{
	int status;

	status = MXC_UART_Init(MXC_UART1, UART_BAUD);
	if (status != E_SUCCESS) { debugErr("UART1 failed init with code: %d\r\n", status); }

	// Move to Interrupt init
	NVIC_ClearPendingIRQ(UART1_IRQn);
	NVIC_DisableIRQ(UART1_IRQn);
	MXC_NVIC_SetVector(UART1_IRQn, UART1_Handler);
	NVIC_EnableIRQ(UART1_IRQn);

	// Setup the asynchronous request
	uart1ReadRequest.uart = MXC_UART1;
	uart1ReadRequest.rxData = g_Uart1_RxBuffer;
	uart1ReadRequest.rxLen = 1; // Trigger size
	uart1ReadRequest.txLen = 0;
	uart1ReadRequest.callback = UART1_Read_Callback;

	status = MXC_UART_TransactionAsync(&uart1ReadRequest);
	if (status != E_SUCCESS) { debugErr("Uart1 Read setup (async) failed with code: %d\r\n", status); }

#if 1 /* Test to check interrupt flags set */
	debug("Uart1: Interrupt enables are 0x%0x, Int flags are 0x%0x, Status is 0x%0x\r\n", MXC_UART1->int_en, MXC_UART1->int_fl, MXC_UART1->stat);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
mxc_uart_req_t uart2ReadRequest; // Needs persistant storage because Maxim UART driver grabs only a reference to the request object
void SetupDebugUART(void)
{
	int status;

	status = MXC_UART_Init(MXC_UART2, UART_BAUD);
	if (status != E_SUCCESS) { } // Where to report?

#if 0 /* Rx Interrupt setup, however due to MCU errata it's possible to lock up the UART with simultaneous Tx/Rx */
	// Move to Interrupt init
	NVIC_ClearPendingIRQ(UART2_IRQn);
	NVIC_DisableIRQ(UART2_IRQn);
	MXC_NVIC_SetVector(UART2_IRQn, UART2_Handler);
	NVIC_EnableIRQ(UART2_IRQn);

	// Setup the asynchronous request
	uart2ReadRequest.uart = MXC_UART2;
	uart2ReadRequest.rxData = g_Uart2_RxBuffer;
	uart2ReadRequest.rxLen = 1; // Turns out this is not buffer space but trigger level //UART_BUFFER_SIZE;
	uart2ReadRequest.txLen = 0;
	uart2ReadRequest.callback = UART2_Read_Callback;

	status = MXC_UART_TransactionAsync(&uart2ReadRequest);
	if (status != E_NO_ERROR) { debugErr("Debug Uart2 Read setup (async) failed with code: %d\r\n", status); }
#endif
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

#if 0 /* Test special case for EEPROM ID */
	if (slaveAddr == I2C_ADDR_EEPROM_ID) { masterRequest.restart = 1; }
#endif

	// Need access lock to not interrupt in progress I2C1 comms when trying to perform Sensor Check (specifically sample rate change) while in ISR
	if ((slaveAddr == I2C_ADDR_EXPANSION) || (slaveAddr == I2C_ADDR_FUEL_GAUGE) || ((slaveAddr == I2C_ADDR_EXTERNAL_RTC) && (g_i2c1AccessLock != SENSOR_CHECK_LOCK)))
	{
		GetI2C1MutexLock(slaveAddr);
	}

#if /* Old board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_PROTOTYPE_1)
	// Test interrupt isolation (for Acc data collection)
	__disable_irq();
#endif
	status = MXC_I2C_MasterTransaction(&masterRequest);
#if /* Old board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_PROTOTYPE_1)
	// Test interrupt isolation (for Acc data collection)
	__enable_irq();
#endif

	// Clear I2C1 access lock if not
	if ((slaveAddr == I2C_ADDR_EXPANSION) || (slaveAddr == I2C_ADDR_FUEL_GAUGE) || ((slaveAddr == I2C_ADDR_EXTERNAL_RTC) && (g_i2c1AccessLock != SENSOR_CHECK_LOCK)))
	{
		ReleaseI2C1MutexLock();
	}

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
#if 1 /* Fast Speed */
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
//#define SPI_SPEED_LCD 12000000 // Bit Rate, LCD can go up but Accelerometer won't work at 16 MHz and above
// Works at 10 MHz
#define SPI_SPEED_LCD 10000000 // Bit Rate, Trying LCD at less than 11 MHz per odd note in programmers guide, LCD can go up but Accelerometer won't work at 16 MHz and above
// Mostly works at 14 MHz but Acc sometimes doesn't init right away and a couple LCD glitches (needs SPI2 mods to really verify)
//#define SPI_SPEED_LCD 14000000 // Trying alternatives, at 15MHz the ACC occsaionally hangs

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
void SpiTransaction(uint8_t spiDevice, uint8_t dataBits, uint8_t ssDeassert, uint8_t* writeData, uint32_t writeSize, uint8_t* readData, uint32_t readSize, uint8_t method)
{
	mxc_spi_req_t spiRequest;
	IRQn_Type spiIrq;
	void (*irqHandler)(void);

	if (spiDevice == SPI_ADC) { spiRequest.spi = MXC_SPI3; }
	else /* LCD or Acc */ { spiRequest.spi = MXC_SPI2; }
	spiRequest.txData = writeData;
	spiRequest.txLen = writeSize;
	spiRequest.rxData = readData;
	spiRequest.rxLen = readSize;
	if (spiDevice == SPI_ACC) { spiRequest.ssIdx = 1; } else { spiRequest.ssIdx = 0; } // Both ADC and LCD Slave Selects are 0
	spiRequest.ssDeassert = ssDeassert;
	spiRequest.txCnt = 0;
	spiRequest.rxCnt = 0;
	spiRequest.completeCB = (spi_complete_cb_t)SPI_Callback;

#if 0 /* Prevent re-setting the data since since this has to disable the SPI to set each time, moved to SPI init */
	// Set the number of data bits for the transfer
	MXC_SPI_SetDataSize(spiPort, dataBits);
#endif

	if (method == BLOCKING)
	{
		if (spiDevice == SPI_LCD)
		{
			// Check if the SPI2 is not actively handling an LCD stream and actively sampling and the sensor is the internal Acc
			if ((!g_spi2InUseByLCD) && (g_sampleProcessing == ACTIVE_STATE) && (IsSeismicSensorInternalAccelerometer(g_factorySetupRecord.seismicSensorType)))
			{
				// Pre cache an internal Acc sample to be picked up by the sampling ISR if the SPI2 resource is busy
				GetAccelerometerChannelData(&g_accDataCache);
			}

			g_spi2InUseByLCD |= SPI2_ACTIVE;
		}

#if 1 /* Test interrupt isolation */
		__disable_irq();
#endif
		MXC_SPI_MasterTransaction(&spiRequest);
#if 1 /* Test interrupt isolation */
		__enable_irq();
#endif
		if (spiDevice == SPI_LCD) { g_spi2InUseByLCD &= ~SPI2_ACTIVE; }
	}
	else if (method == ASYNC_ISR)
	{
		// Check if selecting the ADC
		if (spiDevice == SPI_ADC)
		{
			spiIrq = SPI3_IRQn;
			irqHandler = SPI3_IRQHandler;
		}
		else // Selecting the LCD or Accelerometer (beta/re-spin)
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
//void SetupSPI3_ExternalADC(void)
void SetupSPI3_ExternalADC(uint32_t clockSpeed)
{
	int status;

	// Setup the SPI3 GPIO control lines (including Slave Select) to the alternate function
	mxc_gpio_cfg_t gpio_cfg_spi3 = { MXC_GPIO0, (GPIO_ADC_SPI3_SCK_PIN | GPIO_ADC_SPI3_SS0_PIN | GPIO_ADC_SPI3_SDO1_PIN | GPIO_ADC_SPI3_SDI_PIN), MXC_GPIO_FUNC_ALT1, MXC_GPIO_PAD_NONE, MXC_GPIO_VSSEL_VDDIO };
	MXC_GPIO_Config(&gpio_cfg_spi3);

#if 0 /* SPI GPIO init inside SPI init call reassigns ADC_RESET and does not set the Slave Select 0 alternate function */
	status = MXC_SPI_Init(MXC_SPI3, YES, NO, 1, LOW, SPI_SPEED_ADC);
#else /* Manual setup */
	MXC_SYS_Reset_Periph(MXC_SYS_RESET_SPI3);
	MXC_SYS_ClockEnable(MXC_SYS_PERIPH_CLOCK_SPI3);
	//status = MXC_SPI_RevA1_Init((mxc_spi_reva_regs_t *)MXC_SPI3, YES, NO, 1, LOW, SPI_SPEED_ADC);
	status = MXC_SPI_RevA1_Init((mxc_spi_reva_regs_t *)MXC_SPI3, YES, NO, 1, LOW, clockSpeed);
#endif
	if (status != E_SUCCESS) { debugErr("SPI3 (ADC) Init failed with code: %d\r\n", status); }

	// Set standard SPI 4-wire (MISO/MOSI, full duplex), (Turns out ADC dual-SDO and MAX32651 dual mode are incompatible, can only use single mode)
	MXC_SPI_SetWidth(MXC_SPI3, SPI_WIDTH_STANDARD);

	// External SPI3 uses SPI Mode 3 only
	MXC_SPI_SetMode(MXC_SPI3, SPI_MODE_3);

#if 1 /* Test moving setting the data size once per init since this doesn't change and setting requires disabling the SPI */
	MXC_SPI_SetDataSize(MXC_SPI3, SPI_8_BIT_DATA_SIZE);
#endif

	debug("SPI3 Clock control config: Scale: %d, High CC: %d, Low CC: %d\r\n", (MXC_SPI3->clk_cfg >> 16), ((MXC_SPI3->clk_cfg >> 8) & 0xFF), (MXC_SPI3->clk_cfg >> 8) & 0xFF);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupSPI2_LCDAndAcc(void)
{
	int status;

	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { MXC_SPI2->ctrl0 &= ~MXC_F_SPI_CTRL0_SS_SEL; } // Clear Master SS select control

	status = MXC_SPI_Init(MXC_SPI2, YES, NO, 1, LOW, SPI_SPEED_LCD);
	if (status != E_SUCCESS) { debugErr("SPI2 (LCD/ACC) Init failed with code: %d\r\n", status); }
	else { debug("SPI2 (LCD/ACC) Init with clock freq: %d\r\n", SPI_SPEED_LCD); }

	// Set standard SPI 4-wire (MISO/MOSI, full duplex)
	MXC_SPI_SetWidth(MXC_SPI2, SPI_WIDTH_STANDARD);

	// LCD controller uses SPI Mode 0 only
	MXC_SPI_SetMode(MXC_SPI2, SPI_MODE_0);

#if 1 /* Test moving setting the data size once per init since this doesn't change and setting requires disabling the SPI */
	MXC_SPI_SetDataSize(MXC_SPI2, SPI_8_BIT_DATA_SIZE);
#endif

#if 0 /* Test */
	// Try to change the drive strength to 2x
	//gpio_cfg_spi2.port->ds_sel0 |= gpio_cfg_spi2.mask; // Set drive strength to 8x

	// Try to change the drive strength to 4x
	//gpio_cfg_spi2.port->ds_sel1 |= gpio_cfg_spi2.mask; // Set drive strength to 8x

	// Try to change the drive strength to 8x
	//gpio_cfg_spi2.port->ds_sel0 |= gpio_cfg_spi2.mask; // Set drive strength to 8x
	//gpio_cfg_spi2.port->ds_sel1 |= gpio_cfg_spi2.mask; // Set drive strength to 8x
#endif
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
#if USB_COMPOSITE_OPTION /* Composite MSC + CDC-ACM */
int setconfigCallback_Composite(MXC_USB_SetupPkt *sud, void *cbdata);
int usbEventCallback_Composite(maxusb_event_t evt, void *data);
#elif USB_CDC_ACM_ONLY_OPTION /* CDC-ACM only */
int setconfigCallback_CDCACM(MXC_USB_SetupPkt *sud, void *cbdata);
int usbEventCallback_CDCACM(maxusb_event_t evt, void *data);
#elif USB_MSC_ONLY_OPTION /* MSC only */
int setconfigCallback_MSC(MXC_USB_SetupPkt *sud, void *cbdata);
int usbEventCallback_MSC(maxusb_event_t evt, void *data);
#endif
static int setfeatureCallback(MXC_USB_SetupPkt *sud, void *cbdata);
static int clrfeatureCallback(MXC_USB_SetupPkt *sud, void *cbdata);
static void usbAppSleep(void);
static void usbAppWakeup(void);
/* static removed while testing MSC only */ int usbReadCallback(void);
int usbStartupCallback();
int usbShutdownCallback();
void echoUSB(void);

// This EP assignment must match the Configuration Descriptor
msc_cfg_t msc_cfg = {
	1, /* EP OUT */
	MXC_USBHS_MAX_PACKET, /* OUT max packet size */
	2, /* EP IN */
	MXC_USBHS_MAX_PACKET, /* IN max packet size */
};

msc_idstrings_t ids = {
	"NOMIS", /* Vendor string.  Maximum of 8 bytes */
	"MSC FLASH DRIVE", /* Product string.  Maximum of 16 bytes */
	"1.0" /* Version string.  Maximum of 4 bytes */
};

// This EP assignment must match the Configuration Descriptor
acm_cfg_t acm_cfg = {
	2, /* EP OUT */
	MXC_USBHS_MAX_PACKET, /* OUT max packet size */
	3, /* EP IN */
	MXC_USBHS_MAX_PACKET, /* IN max packet size */
	4, /* EP Notify */
	MXC_USBHS_MAX_PACKET, /* Notify max packet size */
};

// Functions to control "disk" memory. See msc.h for definitions
msc_mem_t mem = { mscmem_Init, mscmem_Start, mscmem_Stop, mscmem_Ready, mscmem_Size, mscmem_Read, mscmem_Write };

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void delay_us(unsigned int usec)
{
	MXC_TMR_Delay(MXC_TMR5, usec);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupUSBComposite(void)
{
	maxusb_cfg_options_t usb_opts;

	debug("Waiting for VBUS...\r\n");

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

#if USB_COMPOSITE_OPTION /* Original - Composite MSC + CDC-ACM */
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
	enum_register_callback(ENUM_SETCONFIG, setconfigCallback_Composite, NULL);

	// Handle feature set/clear
	enum_register_callback(ENUM_SETFEATURE, setfeatureCallback, NULL);
	enum_register_callback(ENUM_CLRFEATURE, clrfeatureCallback, NULL);

	// Initialize the class driver
	if (msc_init(&composite_config_descriptor.msc_interface_descriptor, &ids, &mem) != 0) { debugErr("MSC Init failed\r\n"); }
	if (acm_init(&composite_config_descriptor.comm_interface_descriptor) != 0) { debugErr("CDC/ACM Init failed\r\n"); }

#elif USB_CDC_ACM_ONLY_OPTION /* CDC-ACM only */
	/* Register enumeration data */
	enum_register_descriptor(ENUM_DESC_DEVICE, (uint8_t *)&device_descriptor, 0);
	enum_register_descriptor(ENUM_DESC_CONFIG, (uint8_t *)&config_descriptor, 0);

	if (usb_opts.enable_hs) {
		/* Two additional descriptors needed for high-speed operation */
		enum_register_descriptor(ENUM_DESC_OTHER, (uint8_t *)&config_descriptor_hs, 0);
		enum_register_descriptor(ENUM_DESC_QUAL, (uint8_t *)&device_qualifier_descriptor, 0);
	}

	enum_register_descriptor(ENUM_DESC_STRING, lang_id_desc_cdcacm, 0);
	enum_register_descriptor(ENUM_DESC_STRING, mfg_id_desc_cdcacm, 1);
	enum_register_descriptor(ENUM_DESC_STRING, prod_id_desc_cdcacm, 2);
	enum_register_descriptor(ENUM_DESC_STRING, serial_id_desc_cdcacm, 3);
	enum_register_descriptor(ENUM_DESC_STRING, cdcacm_func_desc_cdcacm, 4);

	/* Handle configuration */
	enum_register_callback(ENUM_SETCONFIG, setconfigCallback_CDCACM, NULL);

	/* Handle feature set/clear */
	enum_register_callback(ENUM_SETFEATURE, setfeatureCallback, NULL);
	enum_register_callback(ENUM_CLRFEATURE, clrfeatureCallback, NULL);

	/* Initialize the class driver */
	if (acm_init(&config_descriptor.comm_interface_descriptor) != 0) {
		debugErr("USB: acm_init() failed\r\n");
		while (1) {}
	}

#elif USB_MSC_ONLY_OPTION /* MSC only */
	/* Register enumeration data */
	enum_register_descriptor(ENUM_DESC_DEVICE, (uint8_t *)&device_descriptor, 0);
	enum_register_descriptor(ENUM_DESC_CONFIG, (uint8_t *)&config_descriptor, 0);

	if (usb_opts.enable_hs) {
		/* Two additional descriptors needed for high-speed operation */
		enum_register_descriptor(ENUM_DESC_OTHER, (uint8_t *)&config_descriptor_hs, 0);
		enum_register_descriptor(ENUM_DESC_QUAL, (uint8_t *)&device_qualifier_descriptor, 0);
	}

	enum_register_descriptor(ENUM_DESC_STRING, lang_id_desc_msc, 0);
	enum_register_descriptor(ENUM_DESC_STRING, mfg_id_desc_msc, 1);
	enum_register_descriptor(ENUM_DESC_STRING, prod_id_desc_msc, 2);
	enum_register_descriptor(ENUM_DESC_STRING, serial_id_desc_msc, 3);

	/* Handle configuration */
	enum_register_callback(ENUM_SETCONFIG, setconfigCallback_MSC, NULL);

	/* Handle feature set/clear */
	enum_register_callback(ENUM_SETFEATURE, setfeatureCallback, NULL);
	enum_register_callback(ENUM_CLRFEATURE, clrfeatureCallback, NULL);

	/* Initialize the class driver */
	if (msc_init(&config_descriptor.msc_interface_descriptor, &ids, &mem) != 0) {
		debugErr("USB: msc_init() failed\r\n");
		while (1) {}
	}
#endif

	// Register callbacks
#if USB_COMPOSITE_OPTION /* Original - Composite MSC + CDC-ACM */
	MXC_USB_EventEnable(MAXUSB_EVENT_NOVBUS, usbEventCallback_Composite, NULL);
	MXC_USB_EventEnable(MAXUSB_EVENT_VBUS, usbEventCallback_Composite, NULL);
	acm_register_callback(ACM_CB_READ_READY, usbReadCallback);
#elif USB_CDC_ACM_ONLY_OPTION /* CDC-ACM */
	MXC_USB_EventEnable(MAXUSB_EVENT_NOVBUS, usbEventCallback_CDCACM, NULL);
	MXC_USB_EventEnable(MAXUSB_EVENT_VBUS, usbEventCallback_CDCACM, NULL);
	acm_register_callback(ACM_CB_READ_READY, usbReadCallback);
#elif USB_MSC_ONLY_OPTION /* MSC only */
	MXC_USB_EventEnable(MAXUSB_EVENT_NOVBUS, usbEventCallback_MSC, NULL);
	MXC_USB_EventEnable(MAXUSB_EVENT_VBUS, usbEventCallback_MSC, NULL);
#endif

	// Start with USB in low power mode
	usbAppSleep();
#if 1 /* Original */
	NVIC_EnableIRQ(USB_IRQn);
#else /* Test delayed start so that the USB driver isn't initializing while the unit is going through init */
	MXC_USB_Disconnect();
#endif
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
	debugRaw("<U-sc>");

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
	debugRaw("<U-xc>");

	MXC_SYS_ClockDisable(MXC_SYS_PERIPH_CLOCK_USB);

	return E_NO_ERROR;
}

uint8_t g_mscDelayState = OFF;
#if USB_COMPOSITE_OPTION /* Composite MSC + CDC-ACM */
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int setconfigCallback_Composite(MXC_USB_SetupPkt *sud, void *cbdata)
{
	debugRaw("<U-cc>");

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

#if 0 /* Test */
		debugRaw("<msc/%d/%d,acm/%d/%d/%d>", msc_cfg.out_maxpacket, msc_cfg.in_maxpacket, acm_cfg.out_maxpacket, acm_cfg.in_maxpacket, acm_cfg.notify_maxpacket);
#endif
		if (g_mscDelayState == ON) { SoftUsecWait(1 * SOFT_SECS); }
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

#elif USB_CDC_ACM_ONLY_OPTION /* CDC-ACM only */
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int setconfigCallback_CDCACM(MXC_USB_SetupPkt *sud, void *cbdata)
{
	debugRaw("<U-cc>");

	/* Confirm the configuration value */
	if (sud->wValue == config_descriptor.config_descriptor.bConfigurationValue) {
		configured = 1;
		MXC_SETBIT(&event_flags, EVENT_ENUM_COMP);

		acm_cfg.out_ep = config_descriptor.endpoint_descriptor_4.bEndpointAddress & 0x7;
		acm_cfg.out_maxpacket = config_descriptor.endpoint_descriptor_4.wMaxPacketSize;
		acm_cfg.in_ep = config_descriptor.endpoint_descriptor_5.bEndpointAddress & 0x7;
		acm_cfg.in_maxpacket = config_descriptor.endpoint_descriptor_5.wMaxPacketSize;
		acm_cfg.notify_ep = config_descriptor.endpoint_descriptor_3.bEndpointAddress & 0x7;
		acm_cfg.notify_maxpacket = config_descriptor.endpoint_descriptor_3.wMaxPacketSize;

#if 0 /* Test */
		debugRaw("<acm/%d/%d/%d>", acm_cfg.out_maxpacket, acm_cfg.in_maxpacket, acm_cfg.notify_maxpacket);
#endif
		return acm_configure(&acm_cfg); /* Configure the device class */
	} else if (sud->wValue == 0) {
		configured = 0;
		return acm_deconfigure();
	}

	return -1;
}

#elif USB_MSC_ONLY_OPTION /* MSC only */
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int setconfigCallback_MSC(MXC_USB_SetupPkt *sud, void *cbdata)
{
	debugRaw("<U-cc>");

	/* Confirm the configuration value */
	if (sud->wValue == config_descriptor.config_descriptor.bConfigurationValue) {
		configured = 1;
		MXC_SETBIT(&event_flags, EVENT_ENUM_COMP);

		if (MXC_USB_GetStatus() & MAXUSB_STATUS_HIGH_SPEED) {
			msc_cfg.out_ep = config_descriptor_hs.endpoint_descriptor_1.bEndpointAddress & 0x7;
			msc_cfg.out_maxpacket = config_descriptor_hs.endpoint_descriptor_1.wMaxPacketSize;
			msc_cfg.in_ep = config_descriptor_hs.endpoint_descriptor_2.bEndpointAddress & 0x7;
			msc_cfg.in_maxpacket = config_descriptor_hs.endpoint_descriptor_2.wMaxPacketSize;
		} else {
			msc_cfg.out_ep = config_descriptor.endpoint_descriptor_1.bEndpointAddress & 0x7;
			msc_cfg.out_maxpacket = config_descriptor.endpoint_descriptor_1.wMaxPacketSize;
			msc_cfg.in_ep = config_descriptor.endpoint_descriptor_2.bEndpointAddress & 0x7;
			msc_cfg.in_maxpacket = config_descriptor.endpoint_descriptor_2.wMaxPacketSize;
		}

#if 0 /* Test */
		debugRaw("<msc/%d/%d>", msc_cfg.out_maxpacket, msc_cfg.in_maxpacket);
#endif
		if (g_mscDelayState == ON) { SoftUsecWait(1 * SOFT_SECS); }
		return msc_configure(&msc_cfg); /* Configure the device class */

	} else if (sud->wValue == 0) {
		configured = 0;
		return msc_deconfigure();
	}

	return -1;
}
#endif

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

#if USB_COMPOSITE_OPTION /* Composite MSC + CDC-ACM */
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int usbEventCallback_Composite(maxusb_event_t evt, void *data)
{
	debugRaw("<U-ec:%d>", evt);

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
		MXC_USB_EventEnable(MAXUSB_EVENT_BRST, usbEventCallback_Composite, NULL);
		MXC_USB_EventClear(MAXUSB_EVENT_BRSTDN);
		MXC_USB_EventEnable(MAXUSB_EVENT_BRSTDN, usbEventCallback_Composite, NULL);
		MXC_USB_EventClear(MAXUSB_EVENT_SUSP);
		MXC_USB_EventEnable(MAXUSB_EVENT_SUSP, usbEventCallback_Composite, NULL);
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

#elif USB_CDC_ACM_ONLY_OPTION /* CDC-ACM only */
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int usbEventCallback_CDCACM(maxusb_event_t evt, void *data)
{
	debugRaw("<U-ec:%d>", evt);

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
		acm_deconfigure();
		usbAppSleep();
		break;
	case MAXUSB_EVENT_VBUS:
		MXC_USB_EventClear(MAXUSB_EVENT_BRST);
		MXC_USB_EventEnable(MAXUSB_EVENT_BRST, usbEventCallback_CDCACM, NULL);
		MXC_USB_EventClear(MAXUSB_EVENT_BRSTDN);
		MXC_USB_EventEnable(MAXUSB_EVENT_BRSTDN, usbEventCallback_CDCACM, NULL);
		MXC_USB_EventClear(MAXUSB_EVENT_SUSP);
		MXC_USB_EventEnable(MAXUSB_EVENT_SUSP, usbEventCallback_CDCACM, NULL);
		MXC_USB_Connect();
		usbAppSleep();
		break;
	case MAXUSB_EVENT_BRST:
		usbAppWakeup();
		enum_clearconfig();
		acm_deconfigure();
		configured = 0;
		suspended = 0;
		break;
	case MAXUSB_EVENT_BRSTDN:
		if (MXC_USB_GetStatus() & MAXUSB_STATUS_HIGH_SPEED) {
			enum_register_descriptor(ENUM_DESC_CONFIG, (uint8_t *)&config_descriptor_hs, 0);
			enum_register_descriptor(ENUM_DESC_OTHER, (uint8_t *)&config_descriptor, 0);
		} else {
			enum_register_descriptor(ENUM_DESC_CONFIG, (uint8_t *)&config_descriptor, 0);
			enum_register_descriptor(ENUM_DESC_OTHER, (uint8_t *)&config_descriptor_hs, 0);
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

#elif USB_MSC_ONLY_OPTION /* MSC only */
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int usbEventCallback_MSC(maxusb_event_t evt, void *data)
{
	debugRaw("<U-ec:%d>", evt);

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
		usbAppSleep();
		break;

	case MAXUSB_EVENT_VBUS:
		MXC_USB_EventClear(MAXUSB_EVENT_BRST);
		MXC_USB_EventEnable(MAXUSB_EVENT_BRST, usbEventCallback_MSC, NULL);
		MXC_USB_EventClear(MAXUSB_EVENT_BRSTDN);
		MXC_USB_EventEnable(MAXUSB_EVENT_BRSTDN, usbEventCallback_MSC, NULL);
		MXC_USB_EventClear(MAXUSB_EVENT_SUSP);
		MXC_USB_EventEnable(MAXUSB_EVENT_SUSP, usbEventCallback_MSC, NULL);
		MXC_USB_Connect();
		usbAppSleep();
		break;

	case MAXUSB_EVENT_BRST:
		usbAppWakeup();
		enum_clearconfig();
		msc_deconfigure();
		configured = 0;
		suspended = 0;
		break;

	case MAXUSB_EVENT_BRSTDN:
		if (MXC_USB_GetStatus() & MAXUSB_STATUS_HIGH_SPEED) {
			enum_register_descriptor(ENUM_DESC_CONFIG, (uint8_t *)&config_descriptor_hs, 0);
			enum_register_descriptor(ENUM_DESC_OTHER, (uint8_t *)&config_descriptor, 0);
		} else {
			enum_register_descriptor(ENUM_DESC_CONFIG, (uint8_t *)&config_descriptor, 0);
			enum_register_descriptor(ENUM_DESC_OTHER, (uint8_t *)&config_descriptor_hs, 0);
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
#endif

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
/* static removed while testing MSC only */ int usbReadCallback(void)
{
	debugRaw("<U-rc>");

	uint16_t numChars = acm_canread();
	uint8 recieveData;

#if 1 /* Normal */
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
#else /* Redirect to the Cell/LTE module */
	// Grab data from the USB CDC/ACM port and check if status is any error
	UNUSED(recieveData);
	memset(g_debugBuffer, 0, sizeof(g_debugBuffer));
	if (acm_read(g_debugBuffer, numChars) != numChars) { debugErr("USB CDC/ACM: Read failure\r\n"); }
	else // Successful read
	{
		debug("Passing along serial data: <%s>\r\n", (char*)g_debugBuffer);
		int len = numChars;
		uint8_t status = MXC_UART_Write(MXC_UART1, g_debugBuffer, &len); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
	}
#endif

	return 0;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UsbReportEvents(void)
{
#if 0 /* Prevent reporting every call, or find another status to toggle like LED or display */
	if ((suspended) || (!configured)) { debug("USB: Suspended or not configured\r\n"); }
	else { debug("USB: Configured\r\n"); }
#endif

	if (event_flags)
	{
		if (MXC_GETBIT(&event_flags, MAXUSB_EVENT_NOVBUS)) { MXC_CLRBIT(&event_flags, MAXUSB_EVENT_NOVBUS); debug("USB: VBUS Disconnect\r\n"); }
		else if (MXC_GETBIT(&event_flags, MAXUSB_EVENT_VBUS)) { MXC_CLRBIT(&event_flags, MAXUSB_EVENT_VBUS); debug("USB: VBUS Connect\r\n"); }
		else if (MXC_GETBIT(&event_flags, MAXUSB_EVENT_BRST)) { MXC_CLRBIT(&event_flags, MAXUSB_EVENT_BRST); debug("USB: Bus Reset\r\n"); }
		else if (MXC_GETBIT(&event_flags, MAXUSB_EVENT_BRSTDN)) { MXC_CLRBIT(&event_flags, MAXUSB_EVENT_BRSTDN); debug("USB: Bus Reset Done: %s speed\r\n", (MXC_USB_GetStatus() & MAXUSB_STATUS_HIGH_SPEED) ? "High" : "Full"); }
		else if (MXC_GETBIT(&event_flags, MAXUSB_EVENT_SUSP)) { MXC_CLRBIT(&event_flags, MAXUSB_EVENT_SUSP); debug("USB: Suspended\r\n"); }
		else if (MXC_GETBIT(&event_flags, MAXUSB_EVENT_DPACT)) { MXC_CLRBIT(&event_flags, MAXUSB_EVENT_DPACT); debug("USB: Resume\r\n"); }
		else if (MXC_GETBIT(&event_flags, EVENT_ENUM_COMP)) { MXC_CLRBIT(&event_flags, EVENT_ENUM_COMP); debug("USB: Enumeration complete...\r\n"); }
		else if (MXC_GETBIT(&event_flags, EVENT_REMOTE_WAKE)) { MXC_CLRBIT(&event_flags, EVENT_REMOTE_WAKE); debug("USB: Remote Wakeup\r\n"); }
	}

	// 0 = int enabled, 1 = disabled
	if (__get_PRIMASK() != 0)
	{
		debugWarn("MCU: Interrupts are disabled, attempting re-enable...\r\n");
		__enable_irq();
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
TCHAR *FF_ERRORS[20] = { "FR_OK", "FR_DISK_ERR", "FR_INT_ERR", "FR_NOT_READY", "FR_NO_FILE", "FR_NO_PATH", "FR_INVLAID_NAME", "FR_DENIED", "FR_EXIST", "FR_INVALID_OBJECT", "FR_WRITE_PROTECTED", "FR_INVALID_DRIVE", "FR_NOT_ENABLED", "FR_NO_FILESYSTEM", "FR_MKFS_ABORTED", "FR_TIMEOUT", 
"FR_LOCKED", "FR_NOT_ENOUGH_CORE", "FR_TOO_MANY_OPEN_FILES", "FR_INVALID_PARAMETER" };
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

	debug("Formatting flash drive...\r\n");

#if 0 /* For version FF13 and FF14 */
	if ((err = f_mkfs("", FM_ANY, 0, work, sizeof(work))) != FR_OK)
#else /* Version FF15 */
	MKFS_PARM setupFS = { FM_ANY, 0, 0, 0, 0 };
	if ((err = f_mkfs("", &setupFS, work, sizeof(work))) != FR_OK)
#endif
	{ //Format the default drive to FAT32
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
int example(uint8_t formatDrive)
{
	unsigned int length = 256;

	if (formatDrive)
	{
		if ((err = formatSDHC()) != FR_OK) {
			debugErr("Unable to format flash drive: %s\r\n", FF_ERRORS[err]);
			return err;
		}
	}

	//open SD Card
	if ((err = mount()) != FR_OK) { debugErr("Unable to open flash drive: %s\r\n", FF_ERRORS[err]); return err; }
	debug("Flash drive opened\n");

	if ((err = f_setlabel("NOMIS")) != FR_OK) { debugErr("Problem setting drive label: %s\r\n", FF_ERRORS[err]); f_mount(NULL, "", 0); return err; }

	if ((err = f_getfree(&volume, &clusters_free, &fs)) != FR_OK) { debugErr("Problem finding free size of card: %s\r\n", FF_ERRORS[err]); f_mount(NULL, "", 0); return err; }

	if ((err = f_getlabel(&volume, volume_label, &volume_sn)) != FR_OK) { debugErr("Problem reading drive label: %s\r\n", FF_ERRORS[err]); f_mount(NULL, "", 0); return err; }

	if ((err = f_open(&file, "0:HelloWorld.txt", FA_CREATE_ALWAYS | FA_WRITE)) != FR_OK) { debugErr("Unable to open file: %s\r\n", FF_ERRORS[err]);f_mount(NULL, "", 0); return err; }
	debug("File opened\n");

	generateMessage(length);

	if ((err = f_write(&file, &message, length, &bytes_written)) != FR_OK) { debugErr("Unable to write file: %s\r\n", FF_ERRORS[err]); f_mount(NULL, "", 0); return err; }
	debug("%d bytes written to file\n", bytes_written);

	if ((err = f_close(&file)) != FR_OK) { debugErr("Unable to close file: %s\n", FF_ERRORS[err]); f_mount(NULL, "", 0); return err; }
	debug("File closed\n");

	if ((err = f_chmod("HelloWorld.txt", 0, AM_RDO | AM_ARC | AM_SYS | AM_HID)) != FR_OK) { debugErr("Problem with chmod: %s\r\n", FF_ERRORS[err]); f_mount(NULL, "", 0); return err; }

	err = f_stat("MaximSDHC", &fno);
	if (err == FR_NO_FILE) {
		debug("Creating directory...\r\n");
		if ((err = f_mkdir("MaximSDHC")) != FR_OK) { debugErr("Unable to create directory: %s\r\n", FF_ERRORS[err]); f_mount(NULL, "", 0); return err; }
	}
	else
	{
		f_unlink("0:MaximSDHC/HelloMaxim.txt");
	}

	debug("Renaming File...\r\n");
	if ((err = f_rename("0:HelloWorld.txt", "0:MaximSDHC/HelloMaxim.txt")) != FR_OK) { /* /cr: clearify 0:file notation */ debugErr("Unable to move file: %s\r\n", FF_ERRORS[err]); f_mount(NULL, "", 0); return err; }

	if ((err = f_chdir("/MaximSDHC")) != FR_OK) { debugErr("Problem with chdir: %s\r\n", FF_ERRORS[err]); f_mount(NULL, "", 0); return err; }

	debug("Attempting to read back file...\r\n");
	if ((err = f_open(&file, "HelloMaxim.txt", FA_READ)) != FR_OK) { debugErr("Unable to open file: %s\r\n", FF_ERRORS[err]); f_mount(NULL, "", 0); return err; }

	if ((err = f_read(&file, &message, bytes_written, &bytes_read)) != FR_OK) { debugErr("Unable to read file: %s\r\n", FF_ERRORS[err]); f_mount(NULL, "", 0); return err; }

	debug("Read Back %d bytes\r\n", bytes_read);
	debug("Message: %s\r\n", message);

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
			case 9: example(YES); break;
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

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int CreateFilesystem_eMMCFlash(void)
{
	MKFS_PARM setupFS = { FM_ANY, 0, 0, 0, 0 };
	int status = E_FAIL;

	if ((err = f_mkfs("", &setupFS, work, sizeof(work))) != FR_OK)
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
			f_mount(NULL, "", 0);
		}
		else if ((err = f_setlabel("NOMIS")) != FR_OK)
		{
			debugErr("Drive(eMMC): Setting label failed with error %s\r\n", FF_ERRORS[err]);
			f_mount(NULL, "", 0);
		}
		else { status = E_SUCCESS; }
	}

	return (status);
}

#define MXC_SDHC_LIB_CMD6	0x060A
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
	if (highSpeedTiming == MXC_SDHC_LIB_LEGACY_TIMING) { MXC_SDHC->host_cn_2 |= MXC_S_SDHC_HOST_CN_2_UHS_SDR12; }
	else if (highSpeedTiming == MXC_SDHC_LIB_HIGH_SPEED_TIMING_SDR) { MXC_SDHC->host_cn_2 |= MXC_S_SDHC_HOST_CN_2_UHS_SDR25; }
	else if (highSpeedTiming == MXC_SDHC_LIB_HS200_TIMING) { MXC_SDHC->host_cn_2 |= MXC_S_SDHC_HOST_CN_2_UHS_SDR50; }

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
uint8_t SetupSDHCeMMC(void)
{
	mxc_sdhc_cfg_t cfg;
	mxc_sdhc_hs_timing timingMode;
	mxc_sdhc_lib_card_type cardType;

	// Initialize SDHC peripheral
	cfg.bus_voltage = MXC_SDHC_Bus_Voltage_1_8;
	cfg.block_gap = 0;
#if 0 /* Normal */
	cfg.clk_div = 0x96; // Large divide ratio, setting frequency to 400 kHz during Card Identification phase
#elif 0 /* Test full speed init */
	//cfg.clk_div = 0; // Full speed
#else /* Test slowest speed */
	cfg.clk_div = 0x12C; // Large divide ratio for testing formatting
#endif

#if 0 /* Interface call assigns incorrect GPIO (P0.31/SDHC_CDN and P1.2/SDHC_WP) */
	if (MXC_SDHC_Init(&cfg) != E_NO_ERROR) { debugErr("SDHC/eMMC initialization failed\r\n"); }
#else /* Manual setup */
	mxc_gpio_cfg_t gpio_cfg_sdhc_1 = { GPIO_SDHC_PORT, (GPIO_SDHC_CLK_PIN | GPIO_SDHC_CMD_PIN | GPIO_SDHC_DAT0_PIN | GPIO_SDHC_DAT1_PIN | GPIO_SDHC_DAT2_PIN | GPIO_SDHC_DAT3_PIN),
										MXC_GPIO_FUNC_ALT1, MXC_GPIO_PAD_NONE, MXC_GPIO_VSSEL_VDDIO };

	MXC_SYS_ClockEnable(MXC_SYS_PERIPH_CLOCK_SDHC);

#if 1 /* Test */
	mxc_gpio_cfg_t gpio_sdhc_cmd = { GPIO_SDHC_PORT, GPIO_SDHC_CMD_PIN, MXC_GPIO_FUNC_OUT, MXC_GPIO_PAD_NONE, MXC_GPIO_VSSEL_VDDIO };
	MXC_GPIO_Config(&gpio_sdhc_cmd);
	MXC_GPIO_OutClr(GPIO_SDHC_PORT, GPIO_SDHC_CMD_PIN);
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(74)); // Delay 74 clock cycles, but unsure of clock, going with 74ms to be sure
#endif

	MXC_GPIO_Config(&gpio_cfg_sdhc_1);
	gpio_cfg_sdhc_1.port->vssel &= ~(gpio_cfg_sdhc_1.mask); // Set voltage select to MXC_GPIO_VSSEL_VDDIO, since it seems digital interface is at 1.8V
#if 1 /* Normal set to 2x borrowing from example */
	debug("SDHC: Setting GPIO drive strength to 2x\r\n");
	gpio_cfg_sdhc_1.port->ds_sel0 |= gpio_cfg_sdhc_1.mask; // Set drive strength to 2x (borrowing from internal driver)
#elif 0 /* Test drive strength 4x */
	debug("SDHC: Setting GPIO drive strength to 4x\r\n");
	gpio_cfg_sdhc_1.port->ds_sel1 |= gpio_cfg_sdhc_1.mask; // Set drive strength to 4x
#elif 0 /* Test drive strength 8x */
	debug("SDHC: Setting GPIO drive strength to 8x\r\n");
	gpio_cfg_sdhc_1.port->ds_sel0 |= gpio_cfg_sdhc_1.mask; // Set drive strength to 8x
	gpio_cfg_sdhc_1.port->ds_sel1 |= gpio_cfg_sdhc_1.mask; // Set drive strength to 8x
#else /* Test drive strength 1x (MCU default) */
	debug("SDHC: Setting GPIO drive strength to 1x\r\n");
#endif

	// Setup the 1.8V Signaling Enable
	MXC_SDHC->host_cn_2 |= MXC_F_SDHC_HOST_CN_2_1_8V_SIGNAL;

	if (MXC_SDHC_RevA_Init((mxc_sdhc_reva_regs_t *)MXC_SDHC, &cfg) != E_NO_ERROR) { debugErr("SDHC/eMMC initialization failed\r\n"); }
#endif

	// Set up card to get it ready for a transaction
	if (MXC_SDHC_Lib_InitCard(10) == E_NO_ERROR) { debug("SDHC: Card/device Initialized\r\n"); }
	else { debugWarn("SDHC: No card/device response\n"); }

	cardType = MXC_SDHC_Lib_Get_Card_Type();
	if (cardType == CARD_MMC) { debug("SDHC: Card type discovered is MMC/eMMC\r\n"); }
	else if (cardType == CARD_SDHC) { debug("SDHC: Card type discovered is SD/SDHC\r\n"); }
	else { cardType = CARD_NONE; debugWarn("SDHC: No card type found\r\n"); }

	/*
		Note: The 0-52 MHz eMMC devices supported the legacy SDR mode as well as a newer transfer mode introduced by JEDEC version 4.4 called Dual Data Rate (DDR)
		DDR mode allows the transfer of two bits on each data line per clock cycle, one per clock edge; This helps achieve a transfer rate of up to 104 MB/s

		32651 Datasheet: Supports SDR50 with SDHC clock of up to 60MHz (30MB/sec) -or- Supports DDR50 with SDHC clock of up to 30MHz (30MB/sec)
	*/

	// Change the timing mode
	// Note: HS DDR mode does not look supported by MCU
#if 0 /* Start with HS SDR */
	// Set timing mode to High Speed SDR (60MHz @ 30MB/sec)
	timingMode = MXC_SDHC_LIB_HIGH_SPEED_TIMING_SDR;
#else /* Try HS200 timing at some point */
	// Set timing mode to UHS SDR50 (30MHz @ 30MB/sec)
	//timingMode = MXC_SDHC_LIB_HS200_TIMING;
	timingMode = MXC_SDHC_LIB_LEGACY_TIMING;
#endif

#if 0 /* Doesn't work */
	// Enable high speed timing when sorted out
	if (MXC_SDHC_Lib_SetHighSpeedTiming(timingMode) != E_NO_ERROR) { debugErr("SDHC: Failed to set flash device timing via CMD\r\n"); }
#else
	UNUSED(timingMode);
#endif

#if 0 /* Test early setting of width, doesn't work */
	// Swap over to quad data mode, although it looks like every transaction proceeds with setting the bus width again
	if (MXC_SDHC_Lib_SetBusWidth(MXC_SDHC_LIB_QUAD_DATA) != E_NO_ERROR) { debugErr("SDHC: Failed to set Bus width to Quad\r\n"); }
#endif

#if 1 /* Normal */
	// Configure for best clock divider, must not exceed 52 MHz for eMMC in Legacy mode, or use lower clock rate for High Speed DDR mode (max 30MHz)
#if 1 /* Limit setting HS SDR to 60MHz until data exchange verified */
	if (SystemCoreClock > 96000000)
#else /* More selective control on clock selection allowing HS SDR to run 60MHz */
	if (((SystemCoreClock > 96000000) && (timingMode == MXC_SDHC_LIB_LEGACY_TIMING)) || (timingMode == MXC_SDHC_LIB_HIGH_SPEED_TIMING_DDR))
#endif
	{
		//debug("SD clock ratio (at card/device) is 4:1, %dMHz, (eMMC not to exceed 52 MHz for legacy or high speed modes)\r\n", (SystemCoreClock / 4));
		//MXC_SDHC_Set_Clock_Config(1);
		//debug("SD clock ratio: Super slow (%dHz)\r\n", (SystemCoreClock / (2 * 0x96)));
		//MXC_SDHC_Set_Clock_Config(0x96);
		debug("SD clock ratio: Extermely slow (%dHz)\r\n", (SystemCoreClock / (2 * 0x12C)));
		MXC_SDHC_Set_Clock_Config(0x12C);
	}
	else // Use smallest clock divider for fastest clock rate (max 60MHz)
	{
		debug("SD clock ratio (at card/device) is 2:1, %dMHz\r\n", (SystemCoreClock / 2));
		MXC_SDHC_Set_Clock_Config(0);
	}
#else /* Test */
	debug("SD clock ratio (at card/device) is 2:1, %dMHz\r\n", (SystemCoreClock / 2));
	MXC_SDHC_Set_Clock_Config(0);
#endif

#if 0 /* Moved earlier in setup, doesn't work */
	// Swap over to quad data mode, although it looks like every transaction proceeds with setting the bus width again
	if (MXC_SDHC_Lib_SetBusWidth(MXC_SDHC_LIB_QUAD_DATA) != E_NO_ERROR) { debugErr("SDHC: Failed to set Bus width to Quad\r\n"); }
#endif

	// Return 0/E_NO_ERROR if the card is MMC
	return (!(cardType == CARD_MMC));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupDriveAndFilesystem(void)
{
	FILINFO fileInfo;

#if 0 /* Version FF13 */
	debug("Drive(eMMC): Using FF13 version\r\n");
#else /* Version FF15 */
	debug("Drive(eMMC): Using FF15 version\r\n");
#endif

#if 0 /* Test Re-formatting to start */
	// FF13
	//if ((err = f_mkfs("", FM_ANY, 0, work, sizeof(work))) != FR_OK)	{ debugErr("Drive(eMMC): Formatting failed with error %s\r\n", FF_ERRORS[err]); }
	//else { debug("Drive(eMMC): Formatted successfully\r\n"); }

	// FF15
	MKFS_PARM setupFS = { FM_FAT32, 0, 0, 0, 0 };
	if ((err = f_mkfs("", &setupFS, work, sizeof(work))) != FR_OK) { debugErr("Drive(eMMC): Formatting failed with error %s\r\n", FF_ERRORS[err]); }
	else { debug("Drive(eMMC): Formatted successfully\r\n"); }

	// Remount
	if ((err = f_mount(&fs_obj, "", 1)) != FR_OK) { debugErr("Drive(eMMC): filed to mount after formatting, with error %s\r\n", FF_ERRORS[err]); f_mount(NULL, "", 0); }
	else if ((err = f_setlabel("NOMIS")) != FR_OK) { debugErr("Drive(eMMC): Setting label failed with error %s\r\n", FF_ERRORS[err]); f_mount(NULL, "", 0); }
#endif

#if 0 /* Test making FS immedaitely to exercise formatting flash */
	MKFS_PARM setupFS = { FM_ANY, 0, 0, 0, 0 };
	debug("Drive(eMMC): Formatting...\r\n");
	if ((err = f_mkfs("", &setupFS, work, sizeof(work))) != FR_OK)
	{
		debugErr("Drive(eMMC): Formatting failed with error %s\r\n", FF_ERRORS[err]);
	}
	else { debug("Drive(eMMC): Formatted successfully\r\n"); }
#endif

	// Mount the default drive to determine if the filesystem is created
	if ((err = f_mount(&fs_obj, "", 1)) != FR_OK)
	{
		// Check if failure was due to no filesystem
		if (err == FR_NO_FILESYSTEM)
		{
			debug("Drive(eMMC): Formatting...\r\n");

			// Format the default drive to a FAT filesystem
#if 0 /* For versions FF13 and FF14 */
			if ((err = f_mkfs("", FM_ANY, 0, work, sizeof(work))) != FR_OK)
#else /* Version FF15 */
			MKFS_PARM setupFS = { FM_ANY, 0, 0, 0, 0 };
			//MKFS_PARM setupFS = { FM_FAT32, 0, 0, 0, 0 };
			if ((err = f_mkfs("", &setupFS, work, sizeof(work))) != FR_OK)
#endif
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
					f_mount(NULL, "", 0);
				}
				else if ((err = f_setlabel("NOMIS")) != FR_OK)
				{
					debugErr("Drive(eMMC): Setting label failed with error %s\r\n", FF_ERRORS[err]);
					f_mount(NULL, "", 0);
				}
				else { mounted = 1; }
			}
		}
		else // Mount error was other than no filesystem
		{
			debugErr("Drive(eMMC): filed to mount with error %s\r\n", FF_ERRORS[err]);
			f_mount(NULL, "", 0);

#if 1 /* Test */
#if 0 /* For version FF13 and FF14 */
			if ((err = f_mkfs("", FM_ANY, 0, work, sizeof(work))) != FR_OK)
#else /* Version FF15 */
			MKFS_PARM setupFS = { FM_ANY, 0, 0, 0, 0 };
			//MKFS_PARM setupFS = { FM_FAT32, 0, 0, 0, 0 };
			if ((err = f_mkfs("", &setupFS, work, sizeof(work))) != FR_OK)
#endif
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
					f_mount(NULL, "", 0);
				}
				else if ((err = f_setlabel("NOMIS")) != FR_OK)
				{
					debugErr("Drive(eMMC): Setting label failed with error %s\r\n", FF_ERRORS[err]);
					f_mount(NULL, "", 0);
				}
				else { mounted = 1; }
			}
#endif
		}
	}
	else // Mount successful
	{
		debug("Drive(eMMC): mounted successfully\r\n");
#if 1 /* Test */
		mounted = 1;
#endif
	}

	// Check if filesystem available
	if (mounted)
	{
		// Need to make sure directory structure is created and available
		if (f_stat(SYSTEM_PATH, &fileInfo) != FR_OK) { if (f_mkdir(SYSTEM_PATH) != FR_OK) { debugErr("Filesystem: Unable to create %s directory\r\n", SYSTEM_PATH); } }
		if (f_stat(EVENTS_PATH, &fileInfo) != FR_OK) { if (f_mkdir(EVENTS_PATH) != FR_OK) { debugErr("Filesystem: Unable to create %s directory\r\n", EVENTS_PATH); } }
		if (f_stat(ER_DATA_PATH, &fileInfo) != FR_OK) { if (f_mkdir(ER_DATA_PATH) != FR_OK) { debugErr("Filesystem: Unable to create %s directory\r\n", ER_DATA_PATH); } }
		if (f_stat(LANGUAGE_PATH, &fileInfo) != FR_OK) { if (f_mkdir(LANGUAGE_PATH) != FR_OK) { debugErr("Filesystem: Unable to create %s directory\r\n", LANGUAGE_PATH); } }
		if (f_stat(LOGS_PATH, &fileInfo) != FR_OK) { if (f_mkdir(LOGS_PATH) != FR_OK) { debugErr("Filesystem: Unable to create %s directory\r\n", LOGS_PATH); } }
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
#if 1 /* Internal PIT Timer based, will not generate interrupts in Deepsleep or Backup */
	MXC_SYS_Reset_Periph(MXC_SYS_RESET_TIMER2);
	MXC_SYS_ClockEnable(MXC_SYS_PERIPH_CLOCK_TIMER2);

	// Clear interrupt flag
	CYCLIC_HALF_SEC_TIMER_NUM->intr = MXC_F_TMR_INTR_IRQ;

	// Disable the PWM Output (datasheet says it's disabled on reset, but not showing that to be the case)
	CYCLIC_HALF_SEC_TIMER_NUM->cn |= (MXC_S_TMR_CN_PWMCKBD_DIS);

	// Set the prescaler (TMR_PRES_4096)
	CYCLIC_HALF_SEC_TIMER_NUM->cn |= (MXC_F_TMR_CN_PRES3);
	CYCLIC_HALF_SEC_TIMER_NUM->cn |= (MXC_S_TMR_CN_PRES_DIV4096);

	// Set the mode
	CYCLIC_HALF_SEC_TIMER_NUM->cn |= (MXC_S_TMR_CN_TMODE_CONTINUOUS);

	// Set the polarity
	CYCLIC_HALF_SEC_TIMER_NUM->cn |= (MXC_S_TMR_CN_TPOL_ACTIVELO); // Polarity (0 or 1) doesn't matter

	// Init the compare value
	//CYCLIC_HALF_SEC_TIMER_NUM->cmp = 7324; // 60MHz clock / 4096 = 14648 counts/sec, 1/2 second count = 7324
	CYCLIC_HALF_SEC_TIMER_NUM->cmp = 30000000; // Note: For some reason the prescaler peripheral clock divider isn't working as described in the datasheet

	// Init the counter
	CYCLIC_HALF_SEC_TIMER_NUM->cnt = 0x1;

	// Setup the Timer 0 interrupt
	NVIC_ClearPendingIRQ(TMR2_IRQn);
	NVIC_DisableIRQ(TMR2_IRQn);
	MXC_NVIC_SetVector(TMR2_IRQn, Soft_timer_tick_irq);
	NVIC_EnableIRQ(TMR2_IRQn);

	// Enable the timer
	CYCLIC_HALF_SEC_TIMER_NUM->cn |= MXC_F_TMR_CN_TEN;

	//debug("Timer2 Control register: 0x%04x\r\n", CYCLIC_HALF_SEC_TIMER_NUM->cn);
#else /* Internal RTC based off of Sub-Second Alarm register, will generate interrupts in sleep modes */
	while (MXC_RTC_Init(0, 0) == E_BUSY) {}
	while (MXC_RTC_DisableInt(MXC_F_RTC_CTRL_SSEC_ALARM_EN) == E_BUSY) {}
	while (MXC_RTC_SetSubsecondAlarm(2048) == E_BUSY) {} // 4K clock, 2048 = 1/2 second
	MXC_NVIC_SetVector(RTC_IRQn, Internal_rtc_alarms);
	while (MXC_RTC_EnableInt(MXC_F_RTC_CTRL_SSEC_ALARM_EN) == E_BUSY) {}
	while (MXC_RTC_Start() == E_BUSY) {}

#if 1 /* Test */
	// Needed?
	NVIC_EnableIRQ(RTC_IRQn);
#endif
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t IdentiifyI2C(uint8_t i2cNum, uint8_t readBytes, uint8_t regAddr)
{
	char deviceName[50];
	uint8_t identified = NO;
extern uint8_t accelerometerI2CAddr;

	switch (regAddr)
	{
		case I2C_ADDR_ACCELEROMETER: sprintf(deviceName, "Accelerometer"); identified = YES; break;
		case I2C_ADDR_ACCELEROMETER_ALT_1: sprintf(deviceName, "Accelerometer Alt 1"); identified = YES; accelerometerI2CAddr = I2C_ADDR_ACCELEROMETER_ALT_1; break;
		case I2C_ADDR_ACCELEROMETER_ALT_2: sprintf(deviceName, "Accelerometer Alt 2"); identified = YES; accelerometerI2CAddr = I2C_ADDR_ACCELEROMETER_ALT_2; break;
		case I2C_ADDR_ACCELEROMETER_ALT_3: sprintf(deviceName, "Accelerometer Alt 3"); identified = YES; accelerometerI2CAddr = I2C_ADDR_ACCELEROMETER_ALT_3; break;
		case I2C_ADDR_ACCELEROMETER_ALT_4: sprintf(deviceName, "Accelerometer Alt 4"); identified = YES; accelerometerI2CAddr = I2C_ADDR_ACCELEROMETER_ALT_4; break;
		case I2C_ADDR_ACCELEROMETER_ALT_5: sprintf(deviceName, "Accelerometer Alt 5"); identified = YES; accelerometerI2CAddr = I2C_ADDR_ACCELEROMETER_ALT_5; break;
		case I2C_ADDR_ACCELEROMETER_ALT_6: sprintf(deviceName, "Accelerometer Alt 6"); identified = YES; accelerometerI2CAddr = I2C_ADDR_ACCELEROMETER_ALT_6; break;
		case I2C_ADDR_ACCELEROMETER_ALT_7: sprintf(deviceName, "Accelerometer Alt 7"); identified = YES; accelerometerI2CAddr = I2C_ADDR_ACCELEROMETER_ALT_7; break;
		case I2C_ADDR_1_WIRE: sprintf(deviceName, "1-Wire"); identified = YES; break;
		case I2C_ADDR_EEPROM: sprintf(deviceName, "EEPROM"); identified = YES; break;
		case I2C_ADDR_EEPROM_ID: sprintf(deviceName, "EEPROM ID"); identified = YES; break;
		case I2C_ADDR_BATT_CHARGER: sprintf(deviceName, "Battery Charger"); identified = YES; break;
		case I2C_ADDR_USBC_PORT_CONTROLLER: sprintf(deviceName, "USB Port Controller"); identified = YES; break;
		case I2C_ADDR_EXTERNAL_RTC: sprintf(deviceName, "External RTC"); identified = YES; break;
		case I2C_ADDR_FUEL_GAUGE: sprintf(deviceName, "Fuel Gauge"); identified = YES; break;
		case I2C_ADDR_EXPANSION: sprintf(deviceName, "Expansion"); identified = YES; break;
	}

	if (identified) { debug("(R%d) I2C%d device identified: %s (0x%x)\r\n", readBytes, i2cNum, deviceName, regAddr); }

	return (identified);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestI2CDeviceAddresses(void)
{
#if 1 /* Test device addresses */
	uint8_t regAddr;
	uint8_t regData[2];
	mxc_i2c_req_t masterRequest;
	int status;
	uint8_t numDevices;

	for (uint8_t i = 0; i < 1; i++)
	{
		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(1));
		debug("-- I2C Test, Cycle %d --\r\n", i);

		if (i == -1) //0)
		{
			debug("-- Power up SS, Exp --\r\n");
			//SetSmartSensorSleepState(OFF);
			// Bring up Expansion
			PowerControl(EXPANSION_ENABLE, ON);
			MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(500));
			PowerControl(EXPANSION_RESET, OFF);
		}

		if (i == -1)//0)
		{
			debug("-- Power up 5V --\r\n");
			// Bring up Ext ADC
			WaitAnalogPower5vGood();
		}

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
			else if (status != E_COMM_ERR) { debug("(R2) Possible I2C0 device @ 0x%x, status: %d, data: 0x%x 0x%x\r\n", regAddr, status, regData[0], regData[1]); }
			memset(&regData[0], 0, sizeof(regData));
#if 1
			masterRequest.i2c = MXC_I2C1;
			status = MXC_I2C_MasterTransaction(&masterRequest); if (status == E_SUCCESS) { numDevices++; if (IdentiifyI2C(1, 2, regAddr) == NO) debug("(R2) I2C1 device @ 0x%x, status: %d, data: 0x%x 0x%x\r\n", regAddr, status, regData[0], regData[1]); }
			//else if (status == E_COMM_ERR) { debug("(R2) No I2C1 device @ 0x%x (Comm error)\r\n", regAddr); }
			else if (status != E_COMM_ERR) { debug("(R2) Possible I2C1 device @ 0x%x, status: %d, data: 0x%x 0x%x\r\n", regAddr, status, regData[0], regData[1]); }
#endif
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
	debug("-- Power down ADC and Expansion --\r\n");
	PowerControl(ADC_RESET, ON);
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(1));

	SetSmartSensorSleepState(ON);
	PowerControl(EXPANSION_RESET, ON);
	PowerControl(EXPANSION_ENABLE, OFF);

#if 1 /* Test delay for Expanion interupt that shows up shortly after power down */
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(500));
#endif
#endif

#if 0 /* Test */
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
		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(1));
		//if (j++ % 1000 == 0) { debugRaw("."); }
		if (j++ % 1000 == 0) { FuelGaugeDebugInfo(); }
	}
#endif

#if 0 /* Test */
	//-------------------------------------------------------------------------
	// Test I2C
	//-------------------------------------------------------------------------
	debug("I2C Test Loop (forever)\r\n");

	while (1)
	{
#if 0 /* Test 1 */
extern uint8_t VerifyAccManuIDAndPartID(void);
		VerifyAccManuIDAndPartID();
		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(1));
#elif 1 /* Test 2 */
	// Write 0x00 to register 0x7F, primary or flipped address (I2C_ADDR_ACCELEROMETER or I2C_ADDR_ACCELEROMETER_ALT_1)
	uint8_t registerAddrAndData[2];
	uint8_t readVal;

	registerAddrAndData[0] = 0x7F;
	registerAddrAndData[1] = 0x00;

	// Write 0x00 to addr 0x7F for ack/nack
	if (WriteI2CDevice(MXC_I2C0, I2C_ADDR_ACCELEROMETER, &registerAddrAndData[0], sizeof(registerAddrAndData), NULL, 0) == E_SUCCESS)
	{
		// Write 0x00 to addr 0x1C looking for nack
		registerAddrAndData[0] = 0x1C;
		registerAddrAndData[1] = 0x00;
		if (WriteI2CDevice(MXC_I2C0, I2C_ADDR_ACCELEROMETER, &registerAddrAndData[0], sizeof(registerAddrAndData), NULL, 0) != E_SUCCESS) { debugErr("Acc (P:%02x) Addr: Device needs to be power cycled\r\n", I2C_ADDR_ACCELEROMETER); }

		// Write 0x80 to addr 0x1C looking for ack
		registerAddrAndData[0] = 0x1C;
		registerAddrAndData[1] = 0x80;
		if (WriteI2CDevice(MXC_I2C0, I2C_ADDR_ACCELEROMETER, &registerAddrAndData[0], sizeof(registerAddrAndData), NULL, 0) != E_SUCCESS) { debugErr("Acc (P:%02x) Addr: Device needs to be power cycled\r\n", I2C_ADDR_ACCELEROMETER); }

		// Wait for device reset
		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(50));

		// Read addr 0x13
		registerAddrAndData[0] = 0x13;
		WriteI2CDevice(MXC_I2C0, I2C_ADDR_ACCELEROMETER, &registerAddrAndData[0], sizeof(uint8_t), &readVal, sizeof(uint8_t));
		if (readVal == 0x46) { debug("Acc (P:%02x) Addr: 'Who am I' verified\r\n", I2C_ADDR_ACCELEROMETER); }
		else { debugErr("Acc (P:%02x) Addr: 'Who am I' not valid\r\n", I2C_ADDR_ACCELEROMETER); }
	}
	else if (WriteI2CDevice(MXC_I2C0, I2C_ADDR_ACCELEROMETER_ALT_1, &registerAddrAndData[0], sizeof(registerAddrAndData), NULL, 0) == E_SUCCESS) // Try flipped address I2C_ADDR_ACCELEROMETER_ALT_1
	{
		registerAddrAndData[0] = 0x1C;
		registerAddrAndData[1] = 0x00;
		if (WriteI2CDevice(MXC_I2C0, I2C_ADDR_ACCELEROMETER_ALT_1, &registerAddrAndData[0], sizeof(registerAddrAndData), NULL, 0) != E_SUCCESS) { debugErr("Acc (F:%02x) Addr: Device needs to be power cycled\r\n", I2C_ADDR_ACCELEROMETER_ALT_1); }

		registerAddrAndData[0] = 0x1C;
		registerAddrAndData[1] = 0x80;
		if (WriteI2CDevice(MXC_I2C0, I2C_ADDR_ACCELEROMETER_ALT_1, &registerAddrAndData[0], sizeof(registerAddrAndData), NULL, 0) != E_SUCCESS) { debugErr("Acc (F:%02x) Addr: Device needs to be power cycled\r\n", I2C_ADDR_ACCELEROMETER_ALT_1); }

		// Wait for device reset
		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(50));

		registerAddrAndData[0] = 0x13;
		WriteI2CDevice(MXC_I2C0, I2C_ADDR_ACCELEROMETER_ALT_1, &registerAddrAndData[0], sizeof(uint8_t), &readVal, sizeof(uint8_t));
		if (readVal == 0x46) { debug("Acc (F:%02x) Addr: 'Who am I' verified\r\n", I2C_ADDR_ACCELEROMETER_ALT_1); }
		else { debugErr("Acc (F:%02x) Addr: 'Who am I' not valid\r\n", I2C_ADDR_ACCELEROMETER_ALT_1); }
	}
	else
	{
		debugWarn("Acc: Did not respond to primary or flipped address\r\n");

extern uint8_t accelerometerI2CAddr;
		debug("Acc: Trying dynamic addr %02x\r\n", accelerometerI2CAddr);
		if (WriteI2CDevice(MXC_I2C0, accelerometerI2CAddr, &registerAddrAndData[0], sizeof(registerAddrAndData), NULL, 0) == E_SUCCESS)
		{
			// Write 0x00 to addr 0x1C looking for nack
			registerAddrAndData[0] = 0x1C;
			registerAddrAndData[1] = 0x00;
			if (WriteI2CDevice(MXC_I2C0, accelerometerI2CAddr, &registerAddrAndData[0], sizeof(registerAddrAndData), NULL, 0) != E_SUCCESS) { debugErr("Acc (D:%02x) Addr: Device needs to be power cycled\r\n", accelerometerI2CAddr); }

			// Write 0x80 to addr 0x1C looking for ack
			registerAddrAndData[0] = 0x1C;
			registerAddrAndData[1] = 0x80;
			if (WriteI2CDevice(MXC_I2C0, accelerometerI2CAddr, &registerAddrAndData[0], sizeof(registerAddrAndData), NULL, 0) != E_SUCCESS) { debugErr("Acc (D:%02x) Addr: Device needs to be power cycled\r\n", accelerometerI2CAddr); }

			// Wait for device reset
			MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(50));

			// Read addr 0x13
			registerAddrAndData[0] = 0x13;
			WriteI2CDevice(MXC_I2C0, accelerometerI2CAddr, &registerAddrAndData[0], sizeof(uint8_t), &readVal, sizeof(uint8_t));
			if (readVal == 0x46) { debug("Acc (D:%02x) Addr: 'Who am I' verified\r\n", accelerometerI2CAddr); }
			else { debugErr("Acc (D:%02x) Addr: 'Who am I' not valid\r\n", accelerometerI2CAddr); }
		}
	}
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(1));
#endif
	}
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestGPIO(void)
{
#if 0 /* Test 12V external power for alarms */
	PowerControl(ENABLE_12V, ON);
	PowerControl(ENABLE_12V, OFF);

	PowerControl(TRIGGER_OUT, ON);
	PowerControl(TRIGGER_OUT, OFF);

	MXC_GPIO_OutSet(GPIO_ALERT_1_PORT, GPIO_ALERT_1_PIN);
	MXC_GPIO_OutClr(GPIO_ALERT_1_PORT, GPIO_ALERT_1_PIN);

	MXC_GPIO_OutSet(GPIO_ALERT_2_PORT, GPIO_ALERT_2_PIN);
	MXC_GPIO_OutClr(GPIO_ALERT_2_PORT, GPIO_ALERT_2_PIN);
#endif

#if 0 /* Test Alarm states */
	debug("Forever: Toggling Alarm Pins, Alarm1 twice Alarm 2...\r\n");
	while (1)
	{
		MXC_GPIO_OutSet(GPIO_ALERT_1_PORT, GPIO_ALERT_1_PIN);
		MXC_GPIO_OutSet(GPIO_ALERT_2_PORT, GPIO_ALERT_2_PIN);
		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(1));

		MXC_GPIO_OutClr(GPIO_ALERT_1_PORT, GPIO_ALERT_1_PIN);
		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(1));

		MXC_GPIO_OutSet(GPIO_ALERT_1_PORT, GPIO_ALERT_1_PIN);
		MXC_GPIO_OutClr(GPIO_ALERT_2_PORT, GPIO_ALERT_2_PIN);
		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(1));

		MXC_GPIO_OutClr(GPIO_ALERT_1_PORT, GPIO_ALERT_1_PIN);
		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(1));
	}
#endif

#if 0 /* Test power states */
	PowerControl(ALARM_1_ENABLE, ON);
	PowerControl(EXPANSION_ENABLE, ON);
	debug("Power state ALARM_1_ENABLE: 0x%x\r\n", GetPowerControlState(ALARM_1_ENABLE));
	debug("Power state ALARM_2_ENABLE: 0x%x\r\n", GetPowerControlState(ALARM_2_ENABLE));
	debug("Power state LCD_POWER_ENABLE: 0x%x\r\n", GetPowerControlState(LCD_POWER_ENABLE));
	debug("Power state TRIGGER_OUT: 0x%x\r\n", GetPowerControlState(TRIGGER_OUT));
	debug("Power state MCU_POWER_LATCH: 0x%x\r\n", GetPowerControlState(MCU_POWER_LATCH));
	debug("Power state ENABLE_12V: 0x%x\r\n", GetPowerControlState(ENABLE_12V));
	debug("Power state USB_SOURCE_ENABLE: 0x%x\r\n", GetPowerControlState(USB_SOURCE_ENABLE));
	debug("Power state USB_AUX_POWER_ENABLE: 0x%x\r\n", GetPowerControlState(USB_AUX_POWER_ENABLE));
	debug("Power state ADC_RESET: 0x%x\r\n", GetPowerControlState(ADC_RESET));
	debug("Power state EXPANSION_ENABLE: 0x%x\r\n", GetPowerControlState(EXPANSION_ENABLE));
	debug("Power state SENSOR_CHECK_ENABLE: 0x%x\r\n", GetPowerControlState(SENSOR_CHECK_ENABLE));
	debug("Power state LTE_RESET: 0x%x\r\n", GetPowerControlState(LTE_RESET));
	debug("Power state BLE_RESET: 0x%x\r\n", GetPowerControlState(BLE_RESET));
	debug("Power state CELL_ENABLE: 0x%x\r\n", GetPowerControlState(CELL_ENABLE));
	debug("Power state EXPANSION_RESET: 0x%x\r\n", GetPowerControlState(EXPANSION_RESET));
	debug("Power state LCD_POWER_DOWN: 0x%x\r\n", GetPowerControlState(LCD_POWER_DOWN));
	debug("Power state LED_1: 0x%x\r\n", GetPowerControlState(LED_1));
	debug("Power state LED_2: 0x%x\r\n", GetPowerControlState(LED_2));
	debug("Power state LEDS: 0x%x\r\n", GetCurrentLedStates());
	PowerControl(ALARM_1_ENABLE, OFF);
	debug("Power state ALARM_1_ENABLE: 0x%x\r\n", GetPowerControlState(ALARM_1_ENABLE));
#endif

#if 0 /* Test back powering */
	PowerControl(CELL_ENABLE, ON);
	PowerControl(CELL_ENABLE, OFF);
#endif

#if 0 /* Test GPIO pins */
	PowerControl(USB_SOURCE_ENABLE, ON);
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(1));
	PowerControl(USB_SOURCE_ENABLE, OFF);
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(1));

	PowerControl(USB_AUX_POWER_ENABLE, ON);
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(1));
	PowerControl(USB_AUX_POWER_ENABLE, OFF);
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(1));

	PowerControl(ENABLE_12V, ON);
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(1));
	PowerControl(ENABLE_12V, OFF);
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(1));

	SetSmartSensorMuxA0State(ON);
	SetSmartSensorMuxA0State(OFF);
	SetSmartSensorMuxA1State(ON);
	SetSmartSensorMuxA1State(OFF);
	SetSmartSensorMuxEnableState(ON);
	SetSmartSensorMuxEnableState(OFF);

	MXC_GPIO_OutSet(GPIO_CAL_MUX_PRE_AD_SELECT_PORT, GPIO_CAL_MUX_PRE_AD_SELECT_PIN);
	MXC_GPIO_OutClr(GPIO_CAL_MUX_PRE_AD_SELECT_PORT, GPIO_CAL_MUX_PRE_AD_SELECT_PIN);

	MXC_GPIO_OutSet(GPIO_CAL_MUX_PRE_AD_ENABLE_PORT, GPIO_CAL_MUX_PRE_AD_ENABLE_PIN);
	MXC_GPIO_OutClr(GPIO_CAL_MUX_PRE_AD_ENABLE_PORT, GPIO_CAL_MUX_PRE_AD_ENABLE_PIN);

	PowerControl(ALARM_1_ENABLE, ON);
	PowerControl(ALARM_1_ENABLE, OFF);
	PowerControl(ALARM_2_ENABLE, ON);
	PowerControl(ALARM_2_ENABLE, OFF);

	PowerControl(TRIGGER_OUT, ON);
	PowerControl(TRIGGER_OUT, OFF);

	PowerControl(SENSOR_CHECK_ENABLE, ON);
	SetSensorCheckState(ON);
	SetSensorCheckState(OFF);
	PowerControl(SENSOR_CHECK_ENABLE, OFF);

	SetNyquist0State(ON);
	SetNyquist0State(OFF);
	SetNyquist1State(ON);
	SetNyquist1State(OFF);
	SetNyquist2EnableState(OFF);
	SetNyquist2EnableState(ON);

	SetGainGeo1State(ON);
	SetGainGeo1State(OFF);
	SetGainGeo2State(ON);
	SetGainGeo2State(OFF);
	SetPathSelectAop1State(ON);
	SetPathSelectAop1State(OFF);
	SetPathSelectAop2State(ON);
	SetPathSelectAop2State(OFF);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ValidatePowerOn(void)
{
#if 1 /* Normal */
	uint8_t powerOnButtonDetect;
	uint8_t vbusChargingDetect;
	uint16_t i;
	uint32_t timer;

	SetupPowerOnDetectGPIO();

	powerOnButtonDetect = GetPowerOnButtonState();
	vbusChargingDetect = GetPowerGoodBatteryChargerState();

	if (powerOnButtonDetect) { debugRaw("\r\n-----------------------\r\nPower On button pressed\r\n"); }
	if (vbusChargingDetect) { debugRaw("\r\n-----------------------\r\nUSB Charging detected\r\n"); }

	// Check if Power on button is the startup source
	if (powerOnButtonDetect)
	{
		debugRaw("(2 second press validation) Waiting");

		// Monitor Power on button for 2 secs making sure it remains depressed signaling desire to turn unit on
		for (i = 0; i < 40; i++)
		{
			MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(50));
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
		// Make sure latch is disabled in case it was still enabled from prior run and MCU reset
		PowerControl(MCU_POWER_LATCH, OFF);

		SetupHalfSecondTickTimer();

		// Enable Aux Charging Bypass so only the Battery Charger needs to run (otherwise USBC Port Controller needs to be initialized too)
		MXC_GPIO_OutSet(GPIO_USB_AUX_POWER_ENABLE_PORT, GPIO_USB_AUX_POWER_ENABLE_PIN); // Enable

		ft81x_init();
		ft81x_NomisChargingScreen();

		// Test delay before I2C startup since it starts with some failures
		SoftUsecWait(50 * SOFT_MSECS);

		// Note: USB charging is reason for power up, could be USB or another source like 12V external battery (where setting Aux power enable is needed)
		// Setup Battery Charger and Fuel Gauge, then monitor for user power on
		SetupI2C();
		TestI2CDeviceAddresses();

		// Test delay before I2C startup since it starts with some failures
		SoftUsecWait(50 * SOFT_MSECS);

		FuelGaugeInit();
		BatteryChargerInit();

		SoftUsecWait(1 * SOFT_SECS);
		PowerControl(LCD_POWER_DOWN, ON);
		PowerControl(LCD_POWER_ENABLE, OFF);

		timer = g_lifetimeHalfSecondTickCount + 8;

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
					MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(50));
					debugRaw(".");

					// Determine if the Power on button was released early
					if (GetPowerOnButtonState() == OFF)
					{
						debugRaw("\r\nPower On qualificaiton not met\r\n");
						// Break the For loop
						break;
					}
				}

				// Check one last time that the Power On button is still depressed
				if (GetPowerOnButtonState() == ON)
				{
					debugRaw(" Power On activated\r\n");

					// Unit startup condition verified, latch power and continue
					PowerControl(MCU_POWER_LATCH, ON);
					// Break the While loop and proceed with unit startup
					break;
				}
			}

			if (g_lifetimeHalfSecondTickCount == timer)
			{
				timer = g_lifetimeHalfSecondTickCount + 8;
				PowerControl(LED_2, ON);
				debug("(Cyclic Event) %s\r\n", FuelGaugeDebugString());
				PowerControl(LED_2, OFF);
			}

			// Check if charging was removed
			if (GetPowerGoodBatteryChargerState() == NO)
			{
				// Power down
				PowerControl(MCU_POWER_LATCH, OFF);
				while (1) { /* Wait for darkness (caps bleed and unit powers off) */}
			}
		}
	}
	else
	{
		debugWarn("MCU Power latch is power on source\r\n");
	}
#else /* Test without keyboard */
	SetupPowerOnDetectGPIO();
	debugRaw("\r\n-----------------------\r\nPower On Button check bypassed (for testing without keypad)\r\nPower On activated\r\n");
	PowerControl(MCU_POWER_LATCH, ON);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestSDHCeMMC(void)
{
#if 0 /* Test a second init call */
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(500));
	SetupSDHCeMMC();
#endif

#if 0 /* Test Flash reset pin */
	SetupSDHCeMMC();
	SetupDriveAndFilesystem();
	MXC_GPIO_OutClr(GPIO_EMMC_RESET_PORT, GPIO_EMMC_RESET_PIN);
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(1));
	SetupSDHCeMMC();
	SetupDriveAndFilesystem();
	SetupSDHCeMMC();
	SetupDriveAndFilesystem();
	MXC_GPIO_OutSet(GPIO_EMMC_RESET_PORT, GPIO_EMMC_RESET_PIN);
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(2));
	SetupSDHCeMMC();
	SetupDriveAndFilesystem();
	SetupSDHCeMMC();
	SetupDriveAndFilesystem();
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestFlashAndFatFilesystem(void)
{
#if 1 /* Test reporting the CSD c_size and c_size_mult */
	mxc_sdhc_csd_regs_t* csd = NULL;

	debug("SDHC Lib OCR: 0x%04x\r\n", g_processingCal);
	MXC_SDHC_Lib_GetCSD(csd);
	//debug("SDHC Lib CSD: c_size is 0x%03x (should be 0xFFF), c_size_mult is 0x%01x (should be 0x7)\r\n", csd->csd.c_size, csd->csd.c_size_mult);
	uint8_t* trashPtr = (uint8_t*)&csd->array[0];
	debug("Flash CSD: Raw %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n",
			trashPtr[0], trashPtr[1], trashPtr[2], trashPtr[3], trashPtr[4], trashPtr[5], trashPtr[6], trashPtr[7], trashPtr[8], trashPtr[9], trashPtr[10], trashPtr[11], trashPtr[12], trashPtr[13], trashPtr[14], trashPtr[15]);
#endif
#if 1 /* Test reporting the CID */
	uint32_t* cid = NULL;

	MXC_SDHC_Lib_GetCID(cid);
	trashPtr = (uint8_t*)cid;
	debug("Flash CID: Raw %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n",
			trashPtr[0], trashPtr[1], trashPtr[2], trashPtr[3], trashPtr[4], trashPtr[5], trashPtr[6], trashPtr[7], trashPtr[8], trashPtr[9], trashPtr[10], trashPtr[11], trashPtr[12], trashPtr[13], trashPtr[14], trashPtr[15]);
#endif

#if 0 /* Test size and CSD */
	getSize();

	mxc_sdhc_csd_regs_t* csd = NULL;
	if ((err = MXC_SDHC_Lib_GetCSD(csd)) == E_NO_ERROR) { debug("SDHC Lib Get Capacity: Flash is %lld\r\n", MXC_SDHC_Lib_GetCapacity(csd)); }
	else { debug("SDHC Lib Get CSD: error (%d)\r\n"); }

#if 0 /* Test CSD Structure read, seems to differ from Flash CSD */
	debug("Flash CSD: <Start>\r\n");
	debug("Flash CSD: rsv0 (2b) is 0x%x\r\n", csd->csd.rsv0);
	debug("Flash CSD: file_format (2b) is 0x%x\r\n", csd->csd.file_format);
	debug("Flash CSD: temp_write_protect (1b) is 0x%x\r\n", csd->csd.temp_write_protect);
	debug("Flash CSD: perm_write_protect (1b) is 0x%x\r\n", csd->csd.perm_write_protect);
	debug("Flash CSD: copy (1b) is 0x%x\r\n", csd->csd.copy);
	debug("Flash CSD: rsv1 (5b) is 0x%x\r\n", csd->csd.rsv1);
	debug("Flash CSD: write bl partial (1b) is 0x%x\r\n", csd->csd.write_bl_partial);
	debug("Flash CSD: write_bl_len (4b) is 0x%x\r\n", csd->csd.write_bl_len);
	debug("Flash CSD: r2w_factor (3b) is 0x%x\r\n", csd->csd.r2w_factor);
	debug("Flash CSD: rsv2 (2b) is 0x%x\r\n", csd->csd.rsv2);
	debug("Flash CSD: wp_grp_enable (1b) is 0x%x\r\n", csd->csd.wp_grp_enable);
	debug("Flash CSD: wp_grp_size (7b) is 0x%x\r\n", csd->csd.wp_grp_size);
	debug("Flash CSD: sector_size_0 (1b) is 0x%x\r\n", csd->csd.sector_size_0);
	debug("Flash CSD: sector_size_1 (6b) is 0x%x\r\n", csd->csd.sector_size_1);
	debug("Flash CSD: erase_blk_en (1b) is 0x%x\r\n", csd->csd.erase_blk_en);
	debug("Flash CSD: rsv3 (1b) is 0x%x\r\n", csd->csd.rsv3);
	debug("Flash CSD: c_size (22b) is 0x%x\r\n", csd->csd.c_size);
	debug("Flash CSD: rsv4 (2b) is 0x%x\r\n", csd->csd.rsv4);
	debug("Flash CSD: rsv5 (4b) is 0x%x\r\n", csd->csd.rsv5);
	debug("Flash CSD: dsr_imp (1b) is 0x%x\r\n", csd->csd.dsr_imp);
	debug("Flash CSD: read_blk_misalign (1b) is 0x%x\r\n", csd->csd.read_blk_misalign);
	debug("Flash CSD: write_blk_misalign (1b) is 0x%x\r\n", csd->csd.write_blk_misalign);
	debug("Flash CSD: read_bl_partial (1b) is 0x%x\r\n", csd->csd.read_bl_partial);
	debug("Flash CSD: read_bl_len (4b) is 0x%x\r\n", csd->csd.read_bl_len);
	debug("Flash CSD: ccc (12b) is 0x%x\r\n", csd->csd.ccc);
	debug("Flash CSD: tran_speed (8b) is 0x%x\r\n", csd->csd.tran_speed);
	debug("Flash CSD: nsac (8b) is 0x%x\r\n", csd->csd.nsac);
	debug("Flash CSD: taac (8b) is 0x%x\r\n", csd->csd.taac);
	debug("Flash CSD: rsv6 (6b) is 0x%x\r\n", csd->csd.rsv6);
	debug("Flash CSD: csd_structure (2b) is 0x%x\r\n", csd->csd.csd_structure);
	debug("Flash CSD: rsv7 (8b) is 0x%x\r\n", csd->csd.rsv7);
	debug("Flash CSD: <End>\r\n");
	debug("Flash CSD: Raw 0x%08x 0x%08x 0x%08x 0x%08x\r\n", csd->array[0], csd->array[1], csd->array[2], csd->array[3]);
	uint8_t* trashPtr = (uint8_t*)&csd->array[0];
	debug("Flash CSD: Raw %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n",
			trashPtr[0], trashPtr[1], trashPtr[2], trashPtr[3], trashPtr[4], trashPtr[5], trashPtr[6], trashPtr[7], trashPtr[8], trashPtr[9], trashPtr[10], trashPtr[11], trashPtr[12], trashPtr[13], trashPtr[14], trashPtr[15]);
	trashPtr = (uint8_t*)&csd->csd;
	debug("Flash CSD: Raw %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n",
			trashPtr[0], trashPtr[1], trashPtr[2], trashPtr[3], trashPtr[4], trashPtr[5], trashPtr[6], trashPtr[7], trashPtr[8], trashPtr[9], trashPtr[10], trashPtr[11], trashPtr[12], trashPtr[13], trashPtr[14], trashPtr[15]);

#if 0 /* Test second reading of the Flash CSD, which doesn't seem to work and likely why the driver caches the structure */
extern bool g_csd_is_cached;
	g_csd_is_cached = false;
	uint32_t csdArray[4];
	memcpy(csdArray, (void*)csd->array, sizeof(csdArray)); memset((void*)csd->array, 0, sizeof(csd->array));
	if ((err = MXC_SDHC_Lib_GetCSD(csd)) == E_NO_ERROR)
	{
		debug("SDHC Lib Get CSD: success\r\n");
		if (memcmp(csdArray, (void*)csd->array, sizeof(csdArray)) == 0) { debug("SDHC Lib Get CSD: Verify success\r\n"); }
		else { debug("SDHC Lib Get CSD: Verify error\r\n"); }
	}
	else { debug("SDHC Lib Get CSD: error\r\n"); }
#endif
#endif
#endif

#if 0 /* Test CID, doesn't seem to work */
	//uint32_t cid[4];
	//if ((err = MXC_SDHC_Lib_GetCID(&cid[0])) == E_NO_ERROR) { debug("SDHC Lib Get CID: success\r\n"); debug("Flash CID: Raw 0x%x 0x%x 0x%x 0x%x\r\n", cid[0], cid[1], cid[2], cid[3]); }
	//else { debug("SDHC Lib Get CID: error\r\n"); }
#endif

#if 0 /* Test SDHC Clock */
	int32_t j = 0;
	uint32_t cycles;
	volatile uint32_t hsTime;
extern volatile uint8_t hsChange;
	//for (j = 0x94; j >= 0; j--)
	for (j = 0x12C; j >= 0; j--)
	{
#if 0 /* Test FS abilities */
		debug("SDHC Clock Config: Changed to %d (0x%x)\r\n", j, j);
		MXC_SDHC_Set_Clock_Config(j);

		cycles = 0;
		hsTime = g_lifetimeHalfSecondTickCount;
		while (hsChange == 0) {;}
		hsChange = 0;

		generateMessage(256);
		while (1)
		{
			//if ((err = f_open(&file, "0:MaximSDHC/HelloMaxim.txt", FA_READ)) != FR_OK) { debugErr("Unable to open file: %s\r\n"); }
			//if ((err = f_read(&file, &message, bytes_written, &bytes_read)) != FR_OK) { debugErr("Unable to read file: %s\r\n"); }
			//if ((err = f_close(&file)) != FR_OK) { debugErr("Unable to close file: %s\r\n"); f_mount(NULL, "", 0); }

			if ((err = f_open(&file, "0:HelloWorld.txt", FA_CREATE_ALWAYS | FA_WRITE)) != FR_OK) { debugErr("Unable to open file\r\n"); f_mount(NULL, "", 0); }
			if ((err = f_write(&file, &message, 256, &bytes_written)) != FR_OK) { debugErr("Unable to write file\r\n"); f_mount(NULL, "", 0); }
			if ((err = f_close(&file)) != FR_OK) { debugErr("Unable to close file: %s\n"); f_mount(NULL, "", 0); }
			if ((err = f_chmod("HelloWorld.txt", 0, AM_RDO | AM_ARC | AM_SYS | AM_HID)) != FR_OK) { debugErr("Problem with chmod: %s\r\n"); f_mount(NULL, "", 0); }
			err = f_stat("MaximSDHC", &fno);
			if (err == FR_NO_FILE) { if ((err = f_mkdir("MaximSDHC")) != FR_OK) { debugErr("Unable to create directory: %s\r\n"); f_mount(NULL, "", 0); } }
			else { if ((err = f_unlink("0:MaximSDHC/HelloMaxim.txt")) != FR_OK) { debugErr("Unable to create directory: %s\r\n"); f_mount(NULL, "", 0); } }
			if ((err = f_rename("0:HelloWorld.txt", "0:MaximSDHC/HelloMaxim.txt")) != FR_OK) { /* /cr: clearify 0:file notation */ debugErr("Unable to move file: %s\r\n"); f_mount(NULL, "", 0); }
			if ((err = f_chdir("/MaximSDHC")) != FR_OK) { debugErr("Problem with chdir: %s\r\n"); f_mount(NULL, "", 0); }
			if ((err = f_open(&file, "HelloMaxim.txt", FA_READ)) != FR_OK) { debugErr("Unable to open file: %s\r\n"); f_mount(NULL, "", 0); }
			if ((err = f_read(&file, &message, bytes_written, &bytes_read)) != FR_OK) { debugErr("Unable to read file: %s\r\n"); f_mount(NULL, "", 0); }
			if ((err = f_close(&file)) != FR_OK) { debugErr("Unable to close file\r\n"); f_mount(NULL, "", 0); }

			if (g_lifetimeHalfSecondTickCount > (hsTime + 3)) { break; }
			cycles++;
		}
		debug("Filesystem read cycles: %ld (clock divider: %d)\r\n", cycles, j);
#else /* Test FS mount/format */
		UNUSED(cycles); UNUSED(hsTime); UNUSED(hsChange);
		MXC_SYS_ClockDisable(MXC_SYS_PERIPH_CLOCK_SDHC);
		if (j % 4 == 0) { 	MXC_GPIO_OutClr(GPIO_EMMC_RESET_PORT, GPIO_EMMC_RESET_PIN); }
		else { MXC_GPIO_OutSet(GPIO_EMMC_RESET_PORT, GPIO_EMMC_RESET_PIN); }

		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(250));
		SetupSDHCeMMC();
		debug("SDHC Clock Config: Changed to %d (0x%x)\r\n", j, j);
		MXC_SDHC_Set_Clock_Config(j);
		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(5));
		SetupDriveAndFilesystem();
#endif
		j--;
	}
#endif

#if 0 /* Test SDHC eMMC Flash access, read/write, speed/throughput */
	debug("SDHC Clock Config: Changed to %d (0x%x)\r\n", 0, 0);
	MXC_SDHC_Set_Clock_Config(0);

	uint32_t i = 0;
	while (1)
	{
#if 0 /* Test 1 */
		if ((err = f_open(&file, "0:MaximSDHC/HelloMaxim.txt", FA_READ)) != FR_OK) { debugErr("Unable to open file: %s\r\n", FF_ERRORS[err]); }
		if ((err = f_read(&file, &message, bytes_written, &bytes_read)) != FR_OK) { debugErr("Unable to read file: %s\r\n", FF_ERRORS[err]); }
		if ((err = f_close(&file)) != FR_OK) { debugErr("Unable to close file: %s\r\n", FF_ERRORS[err]); f_mount(NULL, "", 0); }
#elif 0 /* Test 2 */
		if ((err = f_open(&file, "0:HelloWorld.txt", FA_CREATE_ALWAYS | FA_WRITE)) != FR_OK) { debugErr("Unable to open file\r\n"); f_mount(NULL, "", 0); }
		if ((err = f_write(&file, &message, 256, &bytes_written)) != FR_OK) { debugErr("Unable to write file\r\n"); f_mount(NULL, "", 0); }
		if ((err = f_close(&file)) != FR_OK) { debugErr("Unable to close file: %s\n"); f_mount(NULL, "", 0); }
		if ((err = f_chmod("HelloWorld.txt", 0, AM_RDO | AM_ARC | AM_SYS | AM_HID)) != FR_OK) { debugErr("Problem with chmod: %s\r\n"); f_mount(NULL, "", 0); }
		err = f_stat("MaximSDHC", &fno);
		if (err == FR_NO_FILE) { if ((err = f_mkdir("MaximSDHC")) != FR_OK) { debugErr("Unable to create directory: %s\r\n"); f_mount(NULL, "", 0); } }
		else { if ((err = f_unlink("0:MaximSDHC/HelloMaxim.txt")) != FR_OK) { debugErr("Unable to create directory: %s\r\n"); f_mount(NULL, "", 0); } }
		if ((err = f_rename("0:HelloWorld.txt", "0:MaximSDHC/HelloMaxim.txt")) != FR_OK) { /* /cr: clearify 0:file notation */ debugErr("Unable to move file: %s\r\n"); f_mount(NULL, "", 0); }
		if ((err = f_chdir("/MaximSDHC")) != FR_OK) { debugErr("Problem with chdir: %s\r\n"); f_mount(NULL, "", 0); }
		if ((err = f_open(&file, "HelloMaxim.txt", FA_READ)) != FR_OK) { debugErr("Unable to open file: %s\r\n"); f_mount(NULL, "", 0); }
		if ((err = f_read(&file, &message, bytes_written, &bytes_read)) != FR_OK) { debugErr("Unable to read file: %s\r\n"); f_mount(NULL, "", 0); }
		if ((err = f_close(&file)) != FR_OK) { debugErr("Unable to close file\r\n"); f_mount(NULL, "", 0); }
#elif 0 /* Test 3 verify (verified) */
		generateMessage(256);
		uint8_t readBuf[256];
		if ((err = f_open(&file, "0:HelloWorld.txt", FA_CREATE_ALWAYS | FA_WRITE)) != FR_OK) { debugErr("Unable to open file HelloWorld.txt\r\n"); f_mount(NULL, "", 0); }
		if ((err = f_write(&file, &message, 256, &bytes_written)) != FR_OK) { debugErr("Unable to write file HelloWorld.txt\r\n"); f_mount(NULL, "", 0); }
		if ((err = f_close(&file)) != FR_OK) { debugErr("Unable to close file HelloWorld.txt\n"); f_mount(NULL, "", 0); }
		if ((err = f_open(&file, "0:Test.txt", FA_CREATE_ALWAYS | FA_WRITE)) != FR_OK) { debugErr("Unable to open file Test.txt\r\n"); f_mount(NULL, "", 0); }
		if ((err = f_write(&file, &message, 256, &bytes_written)) != FR_OK) { debugErr("Unable to write file Test.txt\r\n"); f_mount(NULL, "", 0); }
		if ((err = f_close(&file)) != FR_OK) { debugErr("Unable to close file Test.txt\n"); f_mount(NULL, "", 0); }
		if ((err = f_open(&file, "0:HelloWorld.txt", FA_READ)) != FR_OK) { debugErr("Unable to open file HelloWorld.txt (verify)\r\n"); f_mount(NULL, "", 0); }
		if ((err = f_read(&file, readBuf, 256, &bytes_read)) != FR_OK) { debugErr("Unable to read file HelloWorld.txt (verify)\r\n"); f_mount(NULL, "", 0); }
		if ((err = f_close(&file)) != FR_OK) { debugErr("Unable to close file HelloWorld.txt (verify)\r\n"); f_mount(NULL, "", 0); }
		if (bytes_read != 256) { debugErr("Verify did not read correct number of bytes (%d)\r\n", bytes_read); }
		for (uint16_t j = 0; j < 256; j++)
		{
			if (readBuf[j] != message[j]) { debugErr("Verify error at byte %d\r\n", j); break; }
		}
#elif 1 /* Test 4 speed test */
extern volatile uint8_t hsChange;
		volatile uint32_t hsTime;
		uint16_t c;
		uint32_t fileSize;

		if (i == 0)
		{
			debug("-- SDHC/eMMC Speet Test --\r\n");
			for (fileSize = 0; fileSize < 65536; fileSize++) { g_eventDataBuffer[fileSize] = fileSize; }
		}

		c = 0;
		if ((err = f_open(&file, "0:HelloWorld.txt", FA_CREATE_ALWAYS | FA_WRITE)) != FR_OK) { debugErr("Unable to open file HelloWorld.txt\r\n"); f_mount(NULL, "", 0); }

		hsTime = g_lifetimeHalfSecondTickCount;
		while (hsChange == 0) {;}
		hsChange = 0;

		while (1)
		{
			if ((err = f_write(&file, &g_eventDataBuffer[0], 65536, &bytes_written)) != FR_OK) { debugErr("Unable to write file HelloWorld.txt\r\n"); f_mount(NULL, "", 0); }
			c++;
			if (g_lifetimeHalfSecondTickCount > (hsTime + 6)) { break; }
		}

		fileSize = f_size(&file);
		if ((err = f_close(&file)) != FR_OK) { debugErr("Unable to close file HelloWorld.txt\n"); f_mount(NULL, "", 0); }

		if (fileSize != (c * 65536)) { debugWarn("SDHC/eMMC Write file size difference (%ld <> %ld)\r\n", fileSize, (c * 65536)); }
		if ((c * 65536) > 1000000) { debug("SDHC/eMMC Test write speed: %.3f MB/sec\r\n", (double)((c * 65536) / 3) / (double)1000000); }
		else { debug("SDHC/eMMC Test write speed: %.3f KB/sec\r\n", (double)((c * 65536) / 3) / (double)1000); }

		if ((err = f_open(&file, "0:HelloWorld.txt", FA_READ)) != FR_OK) { debugErr("Unable to open file HelloWorld.txt\r\n"); f_mount(NULL, "", 0); }

		hsTime = g_lifetimeHalfSecondTickCount;
		while (hsChange == 0) {;}
		hsChange = 0;

		while (c--)
		{
			if ((err = f_read(&file, &g_eventDataBuffer[0], 65536, &bytes_written)) != FR_OK) { debugErr("Unable to read file HelloWorld.txt\r\n"); f_mount(NULL, "", 0); }
			if (g_lifetimeHalfSecondTickCount > (hsTime + 6)) { debugWarn("SDHC/eMMC Read took longer than write\r\n"); }
		}
		debug("SDHC/eMMC Read finished in %d HS ticks\r\n", (g_lifetimeHalfSecondTickCount - hsTime));

		if ((err = f_close(&file)) != FR_OK) { debugErr("Unable to close file HelloWorld.txt\n"); f_mount(NULL, "", 0); }

		if (i >= 11) break;
		debug("SDHC Clock Config: Changed to %d (0x%x)\r\n", ((i + 1) * 2), ((i + 1) * 2));
		MXC_SDHC_Set_Clock_Config(((i + 1) * 2));
#endif
		i++; if (i % 100 == 0) { debugRaw("."); }
	}
#endif

#if 0 /* Test SDHC clock changes and ability to mount flash */
	for (uint8_t i = 0x94; i > 0; i--)
	{
		debug("SDHC Clock Config: Changed to %d (0x%x)\r\n", i, i);
		MXC_SDHC_Set_Clock_Config(i);

		//example(NO);

		SetupDriveAndFilesystem();
		f_mount(NULL, "", 0);
		i--;
	}
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestAnalog5V(void)
{
#if 0 /* Test */
	debug("Forever loop, testing interrupts...\r\n");
	debug("External Trigger In state: %s\r\n", (MXC_GPIO_OutGet(GPIO_EXTERNAL_TRIGGER_IN_PORT, GPIO_EXTERNAL_TRIGGER_IN_PIN) > 0 ? "HIGH" : "LOW"));
	while (1) /* Spin, checking interrupts */
	{
		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(15));
		PowerControl(TRIGGER_OUT, ON);
		debug("External Trigger In state: %s\r\n", (MXC_GPIO_OutGet(GPIO_EXTERNAL_TRIGGER_IN_PORT, GPIO_EXTERNAL_TRIGGER_IN_PIN) > 0 ? "HIGH" : "LOW"));
		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(3));
		PowerControl(TRIGGER_OUT, OFF);
		debug("External Trigger In state: %s\r\n", (MXC_GPIO_OutGet(GPIO_EXTERNAL_TRIGGER_IN_PORT, GPIO_EXTERNAL_TRIGGER_IN_PIN) > 0 ? "HIGH" : "LOW"));
	}
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestExtADC(void)
{
#if 0 /* Test power draw for Analog channels */
	uint16 i = 0;
	for (i = 0; i < 10; i++) { debug("Analog Power Test: Stage %d, %s\r\n", 1, FuelGaugeDebugString()); SoftUsecWait(1 * SOFT_SECS); }

	PowerUpAnalog5VandExternalADC();
	WaitAnalogPower5vGood();
	AD4695_Init();

	for (i = 0; i < 10; i++) { debug("Analog Power Test: Stage %d, %s\r\n", 2, FuelGaugeDebugString()); SoftUsecWait(1 * SOFT_SECS); }

	SetAnalogCutoffFrequency(ANALOG_CUTOFF_FREQ_1K);
	SetSeismicGainSelect(SEISMIC_GAIN_NORMAL);
	SetAcousticPathSelect(ACOUSTIC_PATH_AOP);

	for (i = 0; i < 10; i++) { debug("Analog Power Test: Stage %d, %s\r\n", 3, FuelGaugeDebugString()); SoftUsecWait(1 * SOFT_SECS); }

	MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_AOP1_PORT, GPIO_SENSOR_ENABLE_AOP1_PIN);
	for (i = 0; i < 10; i++) { debug("Analog Power Test: Stage %d, %s\r\n", 4, FuelGaugeDebugString()); SoftUsecWait(1 * SOFT_SECS); }
	MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_GEO1_PORT, GPIO_SENSOR_ENABLE_GEO1_PIN);
	for (i = 0; i < 10; i++) { debug("Analog Power Test: Stage %d, %s\r\n", 5, FuelGaugeDebugString()); SoftUsecWait(1 * SOFT_SECS); }
	MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_GEO2_PORT, GPIO_SENSOR_ENABLE_GEO2_PIN);
	for (i = 0; i < 10; i++) { debug("Analog Power Test: Stage %d, %s\r\n", 6, FuelGaugeDebugString()); SoftUsecWait(1 * SOFT_SECS); }
	MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_AOP2_PORT, GPIO_SENSOR_ENABLE_AOP2_PIN);
	for (i = 0; i < 10; i++) { debug("Analog Power Test: Stage %d, %s\r\n", 7, FuelGaugeDebugString()); SoftUsecWait(1 * SOFT_SECS); }

	MXC_GPIO_OutClr(GPIO_SENSOR_ENABLE_GEO1_PORT, GPIO_SENSOR_ENABLE_GEO1_PIN);
	MXC_GPIO_OutClr(GPIO_SENSOR_ENABLE_AOP1_PORT, GPIO_SENSOR_ENABLE_AOP1_PIN);
	MXC_GPIO_OutClr(GPIO_SENSOR_ENABLE_GEO2_PORT, GPIO_SENSOR_ENABLE_GEO2_PIN);
	MXC_GPIO_OutClr(GPIO_SENSOR_ENABLE_AOP2_PORT, GPIO_SENSOR_ENABLE_AOP2_PIN);
	for (i = 0; i < 10; i++) { debug("Analog Power Test: Stage %d, %s\r\n", 8, FuelGaugeDebugString()); SoftUsecWait(1 * SOFT_SECS); }

	PowerControl(ADC_RESET, ON);
	for (i = 0; i < 10; i++) { debug("Analog Power Test: Stage %d, %s\r\n", 9, FuelGaugeDebugString()); SoftUsecWait(1 * SOFT_SECS); }
#endif

#if 0 /* Test Re-init after shutting down the SPI and domain */
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(1));
	InitExternalADC(); debug("External ADC: Init complete\r\n");
#endif

#if 0 /* Test sampling clock (Ext RTC) accuracy against accurate timing source */
#if 0 /* Test loop of clock output */
	StartExternalRtcClock(SAMPLE_RATE_1K);
	while (1)
	{
		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(5));
		StopExternalRtcClock();
		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(3));
		StartExternalRtcClock(SAMPLE_RATE_1K);
	}
#endif

extern volatile uint8_t psChange;
extern volatile uint32_t g_lifetimePeriodicSecondCount;
	volatile uint32 trackSeconds = 0;

	debug("Sample clock: Testing External RTC clock...\r\n");

	uint8_t rtcReg = 0;
	rtcReg = 0x02; SetRtcRegisters(PCF85263_CTL_PIN_IO, (uint8_t*)&rtcReg, sizeof(rtcReg));
	MXC_GPIO_EnableInt(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);

	g_sampleCount = 0;
	psChange = 0; while (psChange == 0) {;}
	trackSeconds = g_lifetimePeriodicSecondCount;
	//StartExternalRtcClock(SAMPLE_RATE_1K);
	rtcReg = 0x25; SetRtcRegisters(PCF85263_CTL_FUNCTION, (uint8_t*)&rtcReg, sizeof(rtcReg));
	//MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(16));
	while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackSeconds + 64)) {;}
	//StopExternalRtcClock();
	rtcReg = 0x27; SetRtcRegisters(PCF85263_CTL_FUNCTION, (uint8_t*)&rtcReg, sizeof(rtcReg));
	debug("Sample clock: Sample rate @ %d results in %lu actual (%lu)\r\n", SAMPLE_RATE_1K, (g_sampleCount >> 6), g_sampleCount);

	g_sampleCount = 0;
	psChange = 0; while (psChange == 0) {;}
	trackSeconds = g_lifetimePeriodicSecondCount;
	//StartExternalRtcClock(SAMPLE_RATE_2K);
	rtcReg = 0x24; SetRtcRegisters(PCF85263_CTL_FUNCTION, (uint8_t*)&rtcReg, sizeof(rtcReg));
	//MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(16));
	while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackSeconds + 64)) {;}
	//StopExternalRtcClock();
	rtcReg = 0x27; SetRtcRegisters(PCF85263_CTL_FUNCTION, (uint8_t*)&rtcReg, sizeof(rtcReg));
	debug("Sample clock: Sample rate @ %d results in %lu actual (%lu)\r\n", SAMPLE_RATE_2K, (g_sampleCount >> 6), g_sampleCount);

	g_sampleCount = 0;
	psChange = 0; while (psChange == 0) {;}
	trackSeconds = g_lifetimePeriodicSecondCount;
	//StartExternalRtcClock(SAMPLE_RATE_4K);
	rtcReg = 0x23; SetRtcRegisters(PCF85263_CTL_FUNCTION, (uint8_t*)&rtcReg, sizeof(rtcReg));
	//MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(16));
	while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackSeconds + 64)) {;}
	//StopExternalRtcClock();
	rtcReg = 0x27; SetRtcRegisters(PCF85263_CTL_FUNCTION, (uint8_t*)&rtcReg, sizeof(rtcReg));
	debug("Sample clock: Sample rate @ %d results in %lu actual (%lu)\r\n", SAMPLE_RATE_4K, (g_sampleCount >> 6), g_sampleCount);

	g_sampleCount = 0;
	psChange = 0; while (psChange == 0) {;}
	trackSeconds = g_lifetimePeriodicSecondCount;
	//StartExternalRtcClock(SAMPLE_RATE_8K);
	rtcReg = 0x22; SetRtcRegisters(PCF85263_CTL_FUNCTION, (uint8_t*)&rtcReg, sizeof(rtcReg));
	//MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(16));
	while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackSeconds + 64)) {;}
	//StopExternalRtcClock();
	rtcReg = 0x27; SetRtcRegisters(PCF85263_CTL_FUNCTION, (uint8_t*)&rtcReg, sizeof(rtcReg));
	debug("Sample clock: Sample rate @ %d results in %lu actual (%lu)\r\n", SAMPLE_RATE_8K, (g_sampleCount >> 6), g_sampleCount);

	g_sampleCount = 0;
	psChange = 0; while (psChange == 0) {;}
	trackSeconds = g_lifetimePeriodicSecondCount;
	//StartExternalRtcClock(SAMPLE_RATE_16K);
	rtcReg = 0x21; SetRtcRegisters(PCF85263_CTL_FUNCTION, (uint8_t*)&rtcReg, sizeof(rtcReg));
	//MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(16));
	while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackSeconds + 64)) {;}
	//StopExternalRtcClock();
	rtcReg = 0x27; SetRtcRegisters(PCF85263_CTL_FUNCTION, (uint8_t*)&rtcReg, sizeof(rtcReg));
	debug("Sample clock: Sample rate @ %d results in %lu actual (%lu)\r\n", SAMPLE_RATE_16K, (g_sampleCount >> 6), g_sampleCount);

	g_sampleCount = 0;
	psChange = 0; while (psChange == 0) {;}
	trackSeconds = g_lifetimePeriodicSecondCount;
	//StartExternalRtcClock(SAMPLE_RATE_32K);
	rtcReg = 0x20; SetRtcRegisters(PCF85263_CTL_FUNCTION, (uint8_t*)&rtcReg, sizeof(rtcReg));
	//MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(16));
	while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackSeconds + 64)) {;}
	//StopExternalRtcClock();
	rtcReg = 0x27; SetRtcRegisters(PCF85263_CTL_FUNCTION, (uint8_t*)&rtcReg, sizeof(rtcReg));
	debug("Sample clock: Sample rate @ %d results in %lu actual (%lu)\r\n", SAMPLE_RATE_32K, (g_sampleCount >> 6), g_sampleCount);

	MXC_GPIO_DisableInt(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
#endif

#if 0 /* Test max rate of collecting data only */
extern volatile uint8_t psChange;
extern volatile uint32_t g_lifetimePeriodicSecondCount;
	volatile uint32 trackedSeconds = 0;
	SAMPLE_DATA_STRUCT tempData;
	g_sampleCount = 0;
	uint32_t sampleProcessTiming = 0;

	// Test 4 Chan + Temp + Readback
	PowerUpAnalog5VandExternalADC();
	SetupADChannelConfig(SAMPLE_RATE_DEFAULT, OVERRIDE_ENABLE_CHANNEL_VERIFICATION);
	MXC_GPIO_DisableInt(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
	debug("Sample clock: Testing External ADC successive reads...\r\n");

	psChange = 0; while (psChange == 0) {;}
	trackedSeconds = g_lifetimePeriodicSecondCount;

	SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */
	SysTick->CTRL = (SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk); /* Enable SysTick Timer */

	while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackedSeconds + 64))
	{
		ReadAnalogData(&tempData);
		g_sampleCount++;
		sampleProcessTiming += ((0xffffff - SysTick->VAL) / 120);
		SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */
	}
	debug("Sample clock: Max successive ADC read sample rate is %lu actual (%lu), SPT: %0.2f\r\n", (g_sampleCount >> 6), g_sampleCount, (double)((float)sampleProcessTiming / (float)g_sampleCount));

	// Test 4 Chan + Temp (No readback)
	g_sampleCount = 0; sampleProcessTiming = 0;
	AD4695_ExitConversionMode();
	SetupADChannelConfig(SAMPLE_RATE_DEFAULT, 0);
	MXC_GPIO_DisableInt(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
	debug("Sample clock: Testing External ADC successive reads...\r\n");

	psChange = 0; while (psChange == 0) {;}
	trackedSeconds = g_lifetimePeriodicSecondCount;

	SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */
	while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackedSeconds + 64))
	{
		ReadAnalogData(&tempData);
		g_sampleCount++;
		sampleProcessTiming += ((0xffffff - SysTick->VAL) / 120);
		SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */
	}
	debug("Sample clock: Max successive ADC read sample rate is %lu actual (%lu), SPT: %0.2f\r\n", (g_sampleCount >> 6), g_sampleCount, (double)((float)sampleProcessTiming / (float)g_sampleCount));

	// Test 4 Chan (No Temp, no readback)
	g_sampleCount = 0; sampleProcessTiming = 0;
	AD4695_ExitConversionMode();
	SetupADChannelConfig(SAMPLE_RATE_16K, 0);
	MXC_GPIO_DisableInt(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
	debug("Sample clock: Testing External ADC successive reads...\r\n");

	psChange = 0; while (psChange == 0) {;}
	trackedSeconds = g_lifetimePeriodicSecondCount;

	SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */
	while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackedSeconds + 64))
	{
		ReadAnalogData(&tempData);
		g_sampleCount++;
		sampleProcessTiming += ((0xffffff - SysTick->VAL) / 120);
		SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */
	}
	debug("Sample clock: Max successive ADC read sample rate is %lu actual (%lu), SPT: %0.2f\r\n", (g_sampleCount >> 6), g_sampleCount, (double)((float)sampleProcessTiming / (float)g_sampleCount));

	//------------------------------------------------------------------------------------------------------------------------------------------
	SetupSPI3_ExternalADC(1 * 1000000); debug("ADC Sample clock: %d MHz\r\n", 1);

	// Test 4 Chan + Temp + Readback
	g_sampleCount = 0; sampleProcessTiming = 0; AD4695_ExitConversionMode(); SetupADChannelConfig(SAMPLE_RATE_DEFAULT, OVERRIDE_ENABLE_CHANNEL_VERIFICATION); MXC_GPIO_DisableInt(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
	debug("Sample clock: Testing External ADC successive reads...\r\n");
	psChange = 0; while (psChange == 0) {;} trackedSeconds = g_lifetimePeriodicSecondCount;
	SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackedSeconds + 64))
	{ ReadAnalogData(&tempData); g_sampleCount++; sampleProcessTiming += ((0xffffff - SysTick->VAL) / 120); SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ }
	debug("Sample clock: Max successive ADC read sample rate is %lu actual (%lu), SPT: %0.2f\r\n", (g_sampleCount >> 6), g_sampleCount, (double)((float)sampleProcessTiming / (float)g_sampleCount));

	// Test 4 Chan + Temp (No readback)
	g_sampleCount = 0; sampleProcessTiming = 0; AD4695_ExitConversionMode(); SetupADChannelConfig(SAMPLE_RATE_DEFAULT, 0); MXC_GPIO_DisableInt(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
	debug("Sample clock: Testing External ADC successive reads...\r\n");
	psChange = 0; while (psChange == 0) {;} trackedSeconds = g_lifetimePeriodicSecondCount;
	SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackedSeconds + 64))
	{ ReadAnalogData(&tempData); g_sampleCount++; sampleProcessTiming += ((0xffffff - SysTick->VAL) / 120); SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */	}
	debug("Sample clock: Max successive ADC read sample rate is %lu actual (%lu), SPT: %0.2f\r\n", (g_sampleCount >> 6), g_sampleCount, (double)((float)sampleProcessTiming / (float)g_sampleCount));

	// Test 4 Chan (No Temp, no readback)
	g_sampleCount = 0; sampleProcessTiming = 0; AD4695_ExitConversionMode(); SetupADChannelConfig(SAMPLE_RATE_16K, 0); MXC_GPIO_DisableInt(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
	debug("Sample clock: Testing External ADC successive reads...\r\n");
	psChange = 0; while (psChange == 0) {;} trackedSeconds = g_lifetimePeriodicSecondCount;
	SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackedSeconds + 64))
	{ ReadAnalogData(&tempData); g_sampleCount++; sampleProcessTiming += ((0xffffff - SysTick->VAL) / 120); SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ }
	debug("Sample clock: Max successive ADC read sample rate is %lu actual (%lu), SPT: %0.2f\r\n", (g_sampleCount >> 6), g_sampleCount, (double)((float)sampleProcessTiming / (float)g_sampleCount));

	//------------------------------------------------------------------------------------------------------------------------------------------
	SetupSPI3_ExternalADC(2 * 1000000); debug("ADC Sample clock: %d MHz\r\n", 2);

	// Test 4 Chan + Temp + Readback
	g_sampleCount = 0; sampleProcessTiming = 0; AD4695_ExitConversionMode(); SetupADChannelConfig(SAMPLE_RATE_DEFAULT, OVERRIDE_ENABLE_CHANNEL_VERIFICATION); MXC_GPIO_DisableInt(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
	debug("Sample clock: Testing External ADC successive reads...\r\n");
	psChange = 0; while (psChange == 0) {;} trackedSeconds = g_lifetimePeriodicSecondCount;
	SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackedSeconds + 64))
	{ ReadAnalogData(&tempData); g_sampleCount++; sampleProcessTiming += ((0xffffff - SysTick->VAL) / 120); SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ }
	debug("Sample clock: Max successive ADC read sample rate is %lu actual (%lu), SPT: %0.2f\r\n", (g_sampleCount >> 6), g_sampleCount, (double)((float)sampleProcessTiming / (float)g_sampleCount));

	// Test 4 Chan + Temp (No readback)
	g_sampleCount = 0; sampleProcessTiming = 0; AD4695_ExitConversionMode(); SetupADChannelConfig(SAMPLE_RATE_DEFAULT, 0); MXC_GPIO_DisableInt(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
	debug("Sample clock: Testing External ADC successive reads...\r\n");
	psChange = 0; while (psChange == 0) {;} trackedSeconds = g_lifetimePeriodicSecondCount;
	SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackedSeconds + 64))
	{ ReadAnalogData(&tempData); g_sampleCount++; sampleProcessTiming += ((0xffffff - SysTick->VAL) / 120); SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */	}
	debug("Sample clock: Max successive ADC read sample rate is %lu actual (%lu), SPT: %0.2f\r\n", (g_sampleCount >> 6), g_sampleCount, (double)((float)sampleProcessTiming / (float)g_sampleCount));

	// Test 4 Chan (No Temp, no readback)
	g_sampleCount = 0; sampleProcessTiming = 0; AD4695_ExitConversionMode(); SetupADChannelConfig(SAMPLE_RATE_16K, 0); MXC_GPIO_DisableInt(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
	debug("Sample clock: Testing External ADC successive reads...\r\n");
	psChange = 0; while (psChange == 0) {;} trackedSeconds = g_lifetimePeriodicSecondCount;
	SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackedSeconds + 64))
	{ ReadAnalogData(&tempData); g_sampleCount++; sampleProcessTiming += ((0xffffff - SysTick->VAL) / 120); SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ }
	debug("Sample clock: Max successive ADC read sample rate is %lu actual (%lu), SPT: %0.2f\r\n", (g_sampleCount >> 6), g_sampleCount, (double)((float)sampleProcessTiming / (float)g_sampleCount));

	//------------------------------------------------------------------------------------------------------------------------------------------
	SetupSPI3_ExternalADC(4 * 1000000); debug("ADC Sample clock: %d MHz\r\n", 4);

	// Test 4 Chan + Temp + Readback
	g_sampleCount = 0; sampleProcessTiming = 0; AD4695_ExitConversionMode(); SetupADChannelConfig(SAMPLE_RATE_DEFAULT, OVERRIDE_ENABLE_CHANNEL_VERIFICATION); MXC_GPIO_DisableInt(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
	debug("Sample clock: Testing External ADC successive reads...\r\n");
	psChange = 0; while (psChange == 0) {;} trackedSeconds = g_lifetimePeriodicSecondCount;
	SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackedSeconds + 64))
	{ ReadAnalogData(&tempData); g_sampleCount++; sampleProcessTiming += ((0xffffff - SysTick->VAL) / 120); SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ }
	debug("Sample clock: Max successive ADC read sample rate is %lu actual (%lu), SPT: %0.2f\r\n", (g_sampleCount >> 6), g_sampleCount, (double)((float)sampleProcessTiming / (float)g_sampleCount));

	// Test 4 Chan + Temp (No readback)
	g_sampleCount = 0; sampleProcessTiming = 0; AD4695_ExitConversionMode(); SetupADChannelConfig(SAMPLE_RATE_DEFAULT, 0); MXC_GPIO_DisableInt(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
	debug("Sample clock: Testing External ADC successive reads...\r\n");
	psChange = 0; while (psChange == 0) {;} trackedSeconds = g_lifetimePeriodicSecondCount;
	SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackedSeconds + 64))
	{ ReadAnalogData(&tempData); g_sampleCount++; sampleProcessTiming += ((0xffffff - SysTick->VAL) / 120); SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */	}
	debug("Sample clock: Max successive ADC read sample rate is %lu actual (%lu), SPT: %0.2f\r\n", (g_sampleCount >> 6), g_sampleCount, (double)((float)sampleProcessTiming / (float)g_sampleCount));

	// Test 4 Chan (No Temp, no readback)
	g_sampleCount = 0; sampleProcessTiming = 0; AD4695_ExitConversionMode(); SetupADChannelConfig(SAMPLE_RATE_16K, 0); MXC_GPIO_DisableInt(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
	debug("Sample clock: Testing External ADC successive reads...\r\n");
	psChange = 0; while (psChange == 0) {;} trackedSeconds = g_lifetimePeriodicSecondCount;
	SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackedSeconds + 64))
	{ ReadAnalogData(&tempData); g_sampleCount++; sampleProcessTiming += ((0xffffff - SysTick->VAL) / 120); SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ }
	debug("Sample clock: Max successive ADC read sample rate is %lu actual (%lu), SPT: %0.2f\r\n", (g_sampleCount >> 6), g_sampleCount, (double)((float)sampleProcessTiming / (float)g_sampleCount));

	//------------------------------------------------------------------------------------------------------------------------------------------
	SetupSPI3_ExternalADC(8 * 1000000); debug("ADC Sample clock: %d MHz\r\n", 8);

	// Test 4 Chan + Temp + Readback
	g_sampleCount = 0; sampleProcessTiming = 0; AD4695_ExitConversionMode(); SetupADChannelConfig(SAMPLE_RATE_DEFAULT, OVERRIDE_ENABLE_CHANNEL_VERIFICATION); MXC_GPIO_DisableInt(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
	debug("Sample clock: Testing External ADC successive reads...\r\n");
	psChange = 0; while (psChange == 0) {;} trackedSeconds = g_lifetimePeriodicSecondCount;
	SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackedSeconds + 64))
	{ ReadAnalogData(&tempData); g_sampleCount++; sampleProcessTiming += ((0xffffff - SysTick->VAL) / 120); SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ }
	debug("Sample clock: Max successive ADC read sample rate is %lu actual (%lu), SPT: %0.2f\r\n", (g_sampleCount >> 6), g_sampleCount, (double)((float)sampleProcessTiming / (float)g_sampleCount));

	// Test 4 Chan + Temp (No readback)
	g_sampleCount = 0; sampleProcessTiming = 0; AD4695_ExitConversionMode(); SetupADChannelConfig(SAMPLE_RATE_DEFAULT, 0); MXC_GPIO_DisableInt(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
	debug("Sample clock: Testing External ADC successive reads...\r\n");
	psChange = 0; while (psChange == 0) {;} trackedSeconds = g_lifetimePeriodicSecondCount;
	SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackedSeconds + 64))
	{ ReadAnalogData(&tempData); g_sampleCount++; sampleProcessTiming += ((0xffffff - SysTick->VAL) / 120); SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */	}
	debug("Sample clock: Max successive ADC read sample rate is %lu actual (%lu), SPT: %0.2f\r\n", (g_sampleCount >> 6), g_sampleCount, (double)((float)sampleProcessTiming / (float)g_sampleCount));

	// Test 4 Chan (No Temp, no readback)
	g_sampleCount = 0; sampleProcessTiming = 0; AD4695_ExitConversionMode(); SetupADChannelConfig(SAMPLE_RATE_16K, 0); MXC_GPIO_DisableInt(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
	debug("Sample clock: Testing External ADC successive reads...\r\n");
	psChange = 0; while (psChange == 0) {;} trackedSeconds = g_lifetimePeriodicSecondCount;
	SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackedSeconds + 64))
	{ ReadAnalogData(&tempData); g_sampleCount++; sampleProcessTiming += ((0xffffff - SysTick->VAL) / 120); SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ }
	debug("Sample clock: Max successive ADC read sample rate is %lu actual (%lu), SPT: %0.2f\r\n", (g_sampleCount >> 6), g_sampleCount, (double)((float)sampleProcessTiming / (float)g_sampleCount));

	//------------------------------------------------------------------------------------------------------------------------------------------
	SetupSPI3_ExternalADC(16 * 1000000); debug("ADC Sample clock: %d MHz\r\n", 16);

	// Test 4 Chan + Temp + Readback
	g_sampleCount = 0; sampleProcessTiming = 0; AD4695_ExitConversionMode(); SetupADChannelConfig(SAMPLE_RATE_DEFAULT, OVERRIDE_ENABLE_CHANNEL_VERIFICATION); MXC_GPIO_DisableInt(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
	debug("Sample clock: Testing External ADC successive reads...\r\n");
	psChange = 0; while (psChange == 0) {;} trackedSeconds = g_lifetimePeriodicSecondCount;
	SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackedSeconds + 64))
	{ ReadAnalogData(&tempData); g_sampleCount++; sampleProcessTiming += ((0xffffff - SysTick->VAL) / 120); SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ }
	debug("Sample clock: Max successive ADC read sample rate is %lu actual (%lu), SPT: %0.2f\r\n", (g_sampleCount >> 6), g_sampleCount, (double)((float)sampleProcessTiming / (float)g_sampleCount));

	// Test 4 Chan + Temp (No readback)
	g_sampleCount = 0; sampleProcessTiming = 0; AD4695_ExitConversionMode(); SetupADChannelConfig(SAMPLE_RATE_DEFAULT, 0); MXC_GPIO_DisableInt(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
	debug("Sample clock: Testing External ADC successive reads...\r\n");
	psChange = 0; while (psChange == 0) {;} trackedSeconds = g_lifetimePeriodicSecondCount;
	SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackedSeconds + 64))
	{ ReadAnalogData(&tempData); g_sampleCount++; sampleProcessTiming += ((0xffffff - SysTick->VAL) / 120); SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */	}
	debug("Sample clock: Max successive ADC read sample rate is %lu actual (%lu), SPT: %0.2f\r\n", (g_sampleCount >> 6), g_sampleCount, (double)((float)sampleProcessTiming / (float)g_sampleCount));

	// Test 4 Chan (No Temp, no readback)
	g_sampleCount = 0; sampleProcessTiming = 0; AD4695_ExitConversionMode(); SetupADChannelConfig(SAMPLE_RATE_16K, 0); MXC_GPIO_DisableInt(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
	debug("Sample clock: Testing External ADC successive reads...\r\n");
	psChange = 0; while (psChange == 0) {;} trackedSeconds = g_lifetimePeriodicSecondCount;
	SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackedSeconds + 64))
	{ ReadAnalogData(&tempData); g_sampleCount++; sampleProcessTiming += ((0xffffff - SysTick->VAL) / 120); SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ }
	debug("Sample clock: Max successive ADC read sample rate is %lu actual (%lu), SPT: %0.2f\r\n", (g_sampleCount >> 6), g_sampleCount, (double)((float)sampleProcessTiming / (float)g_sampleCount));

	//------------------------------------------------------------------------------------------------------------------------------------------
	SetupSPI3_ExternalADC(20 * 1000000); debug("ADC Sample clock: %d MHz\r\n", 20);

	// Test 4 Chan + Temp + Readback
	g_sampleCount = 0; sampleProcessTiming = 0; AD4695_ExitConversionMode(); SetupADChannelConfig(SAMPLE_RATE_DEFAULT, OVERRIDE_ENABLE_CHANNEL_VERIFICATION); MXC_GPIO_DisableInt(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
	debug("Sample clock: Testing External ADC successive reads...\r\n");
	psChange = 0; while (psChange == 0) {;} trackedSeconds = g_lifetimePeriodicSecondCount;
	SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackedSeconds + 64))
	{ ReadAnalogData(&tempData); g_sampleCount++; sampleProcessTiming += ((0xffffff - SysTick->VAL) / 120); SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ }
	debug("Sample clock: Max successive ADC read sample rate is %lu actual (%lu), SPT: %0.2f\r\n", (g_sampleCount >> 6), g_sampleCount, (double)((float)sampleProcessTiming / (float)g_sampleCount));

	// Test 4 Chan + Temp (No readback)
	g_sampleCount = 0; sampleProcessTiming = 0; AD4695_ExitConversionMode(); SetupADChannelConfig(SAMPLE_RATE_DEFAULT, 0); MXC_GPIO_DisableInt(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
	debug("Sample clock: Testing External ADC successive reads...\r\n");
	psChange = 0; while (psChange == 0) {;} trackedSeconds = g_lifetimePeriodicSecondCount;
	SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackedSeconds + 64))
	{ ReadAnalogData(&tempData); g_sampleCount++; sampleProcessTiming += ((0xffffff - SysTick->VAL) / 120); SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */	}
	debug("Sample clock: Max successive ADC read sample rate is %lu actual (%lu), SPT: %0.2f\r\n", (g_sampleCount >> 6), g_sampleCount, (double)((float)sampleProcessTiming / (float)g_sampleCount));

	// Test 4 Chan (No Temp, no readback)
	g_sampleCount = 0; sampleProcessTiming = 0; AD4695_ExitConversionMode(); SetupADChannelConfig(SAMPLE_RATE_16K, 0); MXC_GPIO_DisableInt(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
	debug("Sample clock: Testing External ADC successive reads...\r\n");
	psChange = 0; while (psChange == 0) {;} trackedSeconds = g_lifetimePeriodicSecondCount;
	SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackedSeconds + 64))
	{ ReadAnalogData(&tempData); g_sampleCount++; sampleProcessTiming += ((0xffffff - SysTick->VAL) / 120); SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ }
	debug("Sample clock: Max successive ADC read sample rate is %lu actual (%lu), SPT: %0.2f\r\n", (g_sampleCount >> 6), g_sampleCount, (double)((float)sampleProcessTiming / (float)g_sampleCount));

	//------------------------------------------------------------------------------------------------------------------------------------------
	SetupSPI3_ExternalADC(25 * 1000000); debug("ADC Sample clock: %d MHz\r\n", 25);

	// Test 4 Chan + Temp + Readback
	g_sampleCount = 0; sampleProcessTiming = 0; AD4695_ExitConversionMode(); SetupADChannelConfig(SAMPLE_RATE_DEFAULT, OVERRIDE_ENABLE_CHANNEL_VERIFICATION); MXC_GPIO_DisableInt(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
	debug("Sample clock: Testing External ADC successive reads...\r\n");
	psChange = 0; while (psChange == 0) {;} trackedSeconds = g_lifetimePeriodicSecondCount;
	SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackedSeconds + 64))
	{ ReadAnalogData(&tempData); g_sampleCount++; sampleProcessTiming += ((0xffffff - SysTick->VAL) / 120); SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ }
	debug("Sample clock: Max successive ADC read sample rate is %lu actual (%lu), SPT: %0.2f\r\n", (g_sampleCount >> 6), g_sampleCount, (double)((float)sampleProcessTiming / (float)g_sampleCount));

	// Test 4 Chan + Temp (No readback)
	g_sampleCount = 0; sampleProcessTiming = 0; AD4695_ExitConversionMode(); SetupADChannelConfig(SAMPLE_RATE_DEFAULT, 0); MXC_GPIO_DisableInt(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
	debug("Sample clock: Testing External ADC successive reads...\r\n");
	psChange = 0; while (psChange == 0) {;} trackedSeconds = g_lifetimePeriodicSecondCount;
	SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackedSeconds + 64))
	{ ReadAnalogData(&tempData); g_sampleCount++; sampleProcessTiming += ((0xffffff - SysTick->VAL) / 120); SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */	}
	debug("Sample clock: Max successive ADC read sample rate is %lu actual (%lu), SPT: %0.2f\r\n", (g_sampleCount >> 6), g_sampleCount, (double)((float)sampleProcessTiming / (float)g_sampleCount));

	// Test 4 Chan (No Temp, no readback)
	g_sampleCount = 0; sampleProcessTiming = 0; AD4695_ExitConversionMode(); SetupADChannelConfig(SAMPLE_RATE_16K, 0); MXC_GPIO_DisableInt(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
	debug("Sample clock: Testing External ADC successive reads...\r\n");
	psChange = 0; while (psChange == 0) {;} trackedSeconds = g_lifetimePeriodicSecondCount;
	SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackedSeconds + 64))
	{ ReadAnalogData(&tempData); g_sampleCount++; sampleProcessTiming += ((0xffffff - SysTick->VAL) / 120); SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ }
	debug("Sample clock: Max successive ADC read sample rate is %lu actual (%lu), SPT: %0.2f\r\n", (g_sampleCount >> 6), g_sampleCount, (double)((float)sampleProcessTiming / (float)g_sampleCount));

	//------------------------------------------------------------------------------------------------------------------------------------------
	SetupSPI3_ExternalADC(30 * 1000000); debug("ADC Sample clock: %d MHz\r\n", 30);

	// Test 4 Chan + Temp + Readback
	g_sampleCount = 0; sampleProcessTiming = 0; AD4695_ExitConversionMode(); SetupADChannelConfig(SAMPLE_RATE_DEFAULT, OVERRIDE_ENABLE_CHANNEL_VERIFICATION); MXC_GPIO_DisableInt(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
	debug("Sample clock: Testing External ADC successive reads...\r\n");
	psChange = 0; while (psChange == 0) {;} trackedSeconds = g_lifetimePeriodicSecondCount;
	SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackedSeconds + 64))
	{ ReadAnalogData(&tempData); g_sampleCount++; sampleProcessTiming += ((0xffffff - SysTick->VAL) / 120); SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ }
	debug("Sample clock: Max successive ADC read sample rate is %lu actual (%lu), SPT: %0.2f\r\n", (g_sampleCount >> 6), g_sampleCount, (double)((float)sampleProcessTiming / (float)g_sampleCount));

	// Test 4 Chan + Temp (No readback)
	g_sampleCount = 0; sampleProcessTiming = 0; AD4695_ExitConversionMode(); SetupADChannelConfig(SAMPLE_RATE_DEFAULT, 0); MXC_GPIO_DisableInt(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
	debug("Sample clock: Testing External ADC successive reads...\r\n");
	psChange = 0; while (psChange == 0) {;} trackedSeconds = g_lifetimePeriodicSecondCount;
	SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackedSeconds + 64))
	{ ReadAnalogData(&tempData); g_sampleCount++; sampleProcessTiming += ((0xffffff - SysTick->VAL) / 120); SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */	}
	debug("Sample clock: Max successive ADC read sample rate is %lu actual (%lu), SPT: %0.2f\r\n", (g_sampleCount >> 6), g_sampleCount, (double)((float)sampleProcessTiming / (float)g_sampleCount));

	// Test 4 Chan (No Temp, no readback)
	g_sampleCount = 0; sampleProcessTiming = 0; AD4695_ExitConversionMode(); SetupADChannelConfig(SAMPLE_RATE_16K, 0); MXC_GPIO_DisableInt(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
	debug("Sample clock: Testing External ADC successive reads...\r\n");
	psChange = 0; while (psChange == 0) {;} trackedSeconds = g_lifetimePeriodicSecondCount;
	SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ while ((volatile uint32_t)g_lifetimePeriodicSecondCount < (trackedSeconds + 64))
	{ ReadAnalogData(&tempData); g_sampleCount++; sampleProcessTiming += ((0xffffff - SysTick->VAL) / 120); SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */ }
	debug("Sample clock: Max successive ADC read sample rate is %lu actual (%lu), SPT: %0.2f\r\n", (g_sampleCount >> 6), g_sampleCount, (double)((float)sampleProcessTiming / (float)g_sampleCount));
	//------------------------------------------------------------------------------------------------------------------------------------------

	SysTick->CTRL = 0; /* Disable */
	AD4695_ExitConversionMode();
	PowerControl(ADC_RESET, ON);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestLCDController(void)
{
#if 0 /* Test LCD screens */
	TestLCD();
#endif

#if 0 /* Test LCD Logo */
extern void test_logo(void);
	test_logo();
	// Forever loop displaying current draw
	//while (1) { MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(1)); debug("Fuel Gauge: %s\r\n", FuelGaugeDebugString()); }
	PowerControl(LCD_POWER_DOWN, ON);
	PowerControl(LCD_POWER_ENABLE, OFF);
#endif

#if 0 /* Test backlight and power draw */
extern void test_black_screen(void);
extern void ft81x_backlight_off(void);
extern void ft81x_set_backlight_level(uint8_t backlightLevel);
extern void test_logo(void);
	test_logo();
	test_black_screen();
	ft81x_backlight_off();
	uint32_t i = 1;
	while (1)
	{
		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(1)); debug("Fuel Gauge: %s, Backlight level: %d\r\n", FuelGaugeDebugString(), (i % 128));
		ft81x_set_backlight_level(i++ % 128);
	}
#endif

#if 0 /* Test Re-init after shutting down SPI and domain */
extern void test_logo(void);
	test_logo();
	PowerControl(LCD_POWER_DOWN, ON);
	PowerControl(LCD_POWER_ENABLE, OFF);
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(1));
	InitLCD(); debug("LCD Display: Init complete\r\n");
	test_logo();
	PowerControl(LCD_POWER_DOWN, ON);
	PowerControl(LCD_POWER_ENABLE, OFF);
#endif
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
	// Setup all GPIO
	//-------------------------------------------------------------------------
	SetupAllGPIO();
#if 1 /* Test */
	TestGPIO();
#endif

	//-------------------------------------------------------------------------
	// Check power on source and validate for system startup
	//-------------------------------------------------------------------------
	ValidatePowerOn();

	//-------------------------------------------------------------------------
	// Setup Half Second tick timer
	//-------------------------------------------------------------------------
	SetupHalfSecondTickTimer();

	//-------------------------------------------------------------------------
	// Display Debug Banner (UART2)
	//-------------------------------------------------------------------------
	DebugUartInitBanner();

	//-------------------------------------------------------------------------
	// Enable Cycle Counter
	//-------------------------------------------------------------------------
#if 0 /* Original */
	EnableMpuCycleCounter();
#else /* DWT (Data Watchpoint and Trace) doesn't look to be implemented in this version of the Cotrex-M4 core */
	// Using the SysTick function instead
#endif

	//-------------------------------------------------------------------------
	// Setup Watchdog
	//-------------------------------------------------------------------------
	SetupWatchdog();

	//-------------------------------------------------------------------------
	// Enable the instruction cache
	//-------------------------------------------------------------------------
	SetupICC();

	//-------------------------------------------------------------------------
	// Init the LCD display
	//-------------------------------------------------------------------------
#if 1 /* Only enable if LCD connector fixed or hardware modded */
	InitLCD(); debug("LCD Display: Init complete\r\n");
#endif

	//-------------------------------------------------------------------------
	// Setup Cell Module UARTs (Cell LTE Rx on U0) and (Cell LTE Tx on U1)
	//-------------------------------------------------------------------------
	SetupCellModuleRxUART(); // Uart0
	SetupCellModuleTxUART(); // Uart1

	//-------------------------------------------------------------------------
	// Setup I2C0 (1.8V devices) and I2C1 (3.3V devices)
	//-------------------------------------------------------------------------
	SetupI2C();
#if 1 /* Test */
	TestI2CDeviceAddresses();
#endif

	//-------------------------------------------------------------------------
	// Setup SPI3 (ADC) and SPI2 (LCD)
	//-------------------------------------------------------------------------
	SetupSPI3_ExternalADC(30 * 1000000);
#if 0 /* Only initializing when the power domain is activated */
	SetupSPI2_LCDAndAcc();
#endif

	//-------------------------------------------------------------------------
	// Setup SDHC/eMMC
	//-------------------------------------------------------------------------
	if (SetupSDHCeMMC() != E_NO_ERROR) { /* Run the setup again */ SetupSDHCeMMC(); } // Init often fails the first time on cold boot
#if 1 /* Test */
	TestSDHCeMMC();
#endif

	//-------------------------------------------------------------------------
	// Setup Drive(eMMC) and Filesystem
	//-------------------------------------------------------------------------
	SetupDriveAndFilesystem();
#if 1 /* Test */
	TestFlashAndFatFilesystem();
#endif

	//-------------------------------------------------------------------------
	// Setup USB Composite (MSC + CDC/ACM)
	//-------------------------------------------------------------------------
#if 0 /* Normal */
	SetupUSBComposite();
#else /* Test wihtout setting up MCU USBC for Port Controller and power delivery */
#endif

	//-------------------------------------------------------------------------
	// Disable all interrupts
	//-------------------------------------------------------------------------
#if 0
	// Todo: Determine if this is necessary, thus far no problems
	__disable_irq();
#endif

#if 1 /* Early call to adjust power savings for testing current */
	AdjustPowerSavings(POWER_SAVINGS_HIGH);
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
	// External RTC
	// Fuel Gauge
	// 1-Wire
	// USB-C Port Controller
	// Battery Charger
	// Accelerometer
	// Expansion I2C bridge
	// EEPROM (should be none)
	// External ADC
	// LCD
	// Cell/LTE
	// Keypad

	//-------------------------------------------------------------------------
	// Initialize the external RTC
	//-------------------------------------------------------------------------
	ExternalRtcInit(); debug("External RTC: Init complete\r\n");

	//-------------------------------------------------------------------------
	// Initalize the Fuel Gauge
	//-------------------------------------------------------------------------
	FuelGaugeInit(); debug("Fuel Gauge: Init complete\r\n");

	//-------------------------------------------------------------------------
	// Smart Sensor data/control init (Hardware pull up on signal)
	//-------------------------------------------------------------------------
	OneWireInit(); debug("One Wire Driver: Init complete\r\n");

	//-------------------------------------------------------------------------
	// Initalize the USB-C Port Controller
	//-------------------------------------------------------------------------
#if 1 /* Normal */
	USBCPortControllerInit(); debug("USB Port Controller: Init complete\r\n");
#endif

	//-------------------------------------------------------------------------
	// Initalize the Battery Charger
	//-------------------------------------------------------------------------
#if 1 /* Normal */
	BatteryChargerInit(); debug("Battery Charger: Init complete\r\n");
#endif

	//-------------------------------------------------------------------------
	// Initalize the Accelerometer
	//-------------------------------------------------------------------------
#if 1 /* Normal */
	AccelerometerInit(); debug("Accelerometer: Init complete\r\n");
#else
	debugWarn("Accelerometer: Init skipped for now\r\n");
#endif

	//-------------------------------------------------------------------------
	// Initalize the Expansion I2C UART Bridge
	//-------------------------------------------------------------------------
#if 0 /* Normal */
	ExpansionBridgeInit(); debug("Expansion I2C Uart Bridge: Init complete\r\n");
#else /* New modded Alpha board I2C doesn't work if epxansion powered */
#endif

#if 0 /* Test */
	//-------------------------------------------------------------------------
	// Test EERPOM (No specific init needed, only I2C comms)
	//-------------------------------------------------------------------------
	TestEEPROM();
#endif

	//-------------------------------------------------------------------------
	// Initialize the AD Control
	//-------------------------------------------------------------------------
#if 0 /* Original */
	AnalogControlInit(); debug("Analog Control: Init complete\r\n");
#else /* No longer want to set GPIO when Analog 5V is disabled, only back powers, moved to Power management */
#endif
#if 1 /* Test */
	TestAnalog5V();
#endif

	//-------------------------------------------------------------------------
	// Init and configure the A/D to prevent the unit from burning current charging internal reference (default config)
	//-------------------------------------------------------------------------
	InitExternalADC(); debug("External ADC: Init complete\r\n");
#if 1 /* Test */
	TestExtADC();
#endif

	//-------------------------------------------------------------------------
	// Init the LCD display
	//-------------------------------------------------------------------------
#if 0 /* Moved to the start of hardware init, only enable if LCD connector fixed or hardware modded */
	InitLCD(); debug("LCD Display: Init complete\r\n");
#endif
#if 0 /* Test */
	TestLCDController();
#endif

	//-------------------------------------------------------------------------
	// Init Cell/LTE
	//-------------------------------------------------------------------------
#if 0 /* Normal */
	InitCellLTE();

#if 0 /* Test re-init of SDHC since Cell/LTE power on seems to kill the eMMC Flash */
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(1));
	if (SetupSDHCeMMC() != E_NO_ERROR) { SetupSDHCeMMC(); } // Run the setup again if it fails the first time
	SetupDriveAndFilesystem();
	TestFlashAndFatFilesystem();
	//MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(1));
	//PowerControl(CELL_ENABLE, ON);
	//MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(1));
	//PowerControl(CELL_ENABLE, OFF);
#endif
#else /* Skip for now */
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
#elif 1 /* Initial test */
	// Attempt to run with most non essential ancillary subsystems disabled
	AdjustPowerSavings(POWER_SAVINGS_HIGH);
#endif

	//-------------------------------------------------------------------------
	// Read and cache Smart Sensor data
	//-------------------------------------------------------------------------
	SmartSensorReadRomAndMemory(SEISMIC_SENSOR); debug("Smart Sensor check for Seismic sensor complete\r\n");
	SmartSensorReadRomAndMemory(ACOUSTIC_SENSOR); debug("Smart Sensor check for Acoustic sensor complete\r\n");
	SmartSensorReadRomAndMemory(SEISMIC_SENSOR_2); debug("Smart Sensor check for Seismic 2 sensor complete\r\n");
	SmartSensorReadRomAndMemory(ACOUSTIC_SENSOR_2); debug("Smart Sensor check for Acoustic 2 sensor complete\r\n");

	//-------------------------------------------------------------------------
	// Hardware initialization complete
	//-------------------------------------------------------------------------
	debug("Hardware Init complete\r\n");

#if 0 /* Test */
	TestInternalRAM();
#endif
#if 0 /* Test LED 1 & 2 states */
	debug("LED 1 & 2 State test...\r\n");
	while (1)
	{
		PowerControl(LED_1, ON);
		SoftUsecWait(3 * SOFT_SECS);
		PowerControl(LED_2, ON);
		SoftUsecWait(3 * SOFT_SECS);
		PowerControl(LED_1, OFF);
		SoftUsecWait(3 * SOFT_SECS);
		PowerControl(LED_2, OFF);
		SoftUsecWait(3 * SOFT_SECS);
	}
#endif
}
