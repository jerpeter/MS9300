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
#include "M23018.h"
//#include "sd_mmc_spi.h"
#include "adc.h"
//#include "usb_task.h"
//#include "device_mass_storage_task.h"
//#include "usb_drv.h"
#include "srec.h"
//#include "flashc.h"
#include "rtc.h"
#include "Sensor.h"
#include "NomisLogo.h"
//#include "navigation.h"
#include "Analog.h"
#include "cs8900.h"

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
void InitSerial232(void)
{
#if 0 /* old hw */
	// Setup debug serial port
	usart_options_t usart_1_rs232_options =
	{
		.baudrate = 115200,
		.charlength = 8,
		.paritytype = USART_NO_PARITY,
		.stopbits = USART_1_STOPBIT,
		.channelmode = USART_NORMAL_CHMODE
	};

	// Load the Unit Config to get the stored Baud rate. Only dependency should be SPI
	GetRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

	// Check if the Unit Config is valid
	if (g_unitConfig.validationKey == 0xA5A5)
	{
		// Set the baud rate to the user stored baud rate setting (initialized to 115200)
		switch (g_unitConfig.baudRate)
		{
			case BAUD_RATE_57600: usart_1_rs232_options.baudrate = 57600; break;
			case BAUD_RATE_38400: usart_1_rs232_options.baudrate = 38400; break;
			case BAUD_RATE_19200: usart_1_rs232_options.baudrate = 19200; break;
			case BAUD_RATE_9600: usart_1_rs232_options.baudrate = 9600; break;
			default: usart_1_rs232_options.baudrate = 115200; break;
		}
	}

	// Initialize it in RS232 mode.
	usart_init_modem(&AVR32_USART1, &usart_1_rs232_options, FOSC0);

	// Enable internal pullups on input lines since external pullups aren't present
	gpio_enable_pin_pull_up(AVR32_USART1_RXD_0_0_PIN);
	gpio_enable_pin_pull_up(AVR32_USART1_DCD_0_PIN);
	gpio_enable_pin_pull_up(AVR32_USART1_DSR_0_PIN);
	gpio_enable_pin_pull_up(AVR32_USART1_CTS_0_0_PIN);
	gpio_enable_pin_pull_up(AVR32_USART1_RI_0_PIN);

	/* To prevent the TXD line from falling when the USART is disabled, the use of an internal pull up
	is mandatory. If the hardware handshaking feature or Modem mode is used, the internal pull up
	on TXD must also be enabled. */
	gpio_enable_pin_pull_up(AVR32_USART1_TXD_0_0_PIN);

	sprintf((char*)g_spareBuffer, "-----     NS8100 Fresh boot, App version: %s (Date: %s)     -----\r\n", (char*)g_buildVersion, (char*)g_buildDate);
	usart_write_line((&AVR32_USART1), "\r\n\n");
	usart_write_line((&AVR32_USART1), "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\r\n");
	usart_write_line((&AVR32_USART1), "---------------------------------------------------------------------------------------\r\n");
	usart_write_line((&AVR32_USART1), (char*)g_spareBuffer);
	usart_write_line((&AVR32_USART1), "---------------------------------------------------------------------------------------\r\n");
	usart_write_line((&AVR32_USART1), "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\r\n");
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DebugUartInitBanner(void)
{
	debug("\r\n\n");
	debug("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\r\n");
	debug("---------------------------------------------------------------------------------------\r\n");
	debug("-----     MS9300 Debug port, App version: %s (Date: %s)     -----\r\n", (char*)g_buildVersion, (char*)g_buildDate);
	debug("---------------------------------------------------------------------------------------\r\n");
	debug("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\r\n\r\n");
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitGps232(void)
{
#if 0 /* old hw */
	// Setup Gps serial port
	usart_options_t usart_0_rs232_options =
	{
		.baudrate = 9600,
		.charlength = 8,
		.paritytype = USART_NO_PARITY,
		.stopbits = USART_1_STOPBIT,
		.channelmode = USART_NORMAL_CHMODE
	};

	// Initialize RS232 mode.
	usart_init_rs232(&AVR32_USART0, &usart_0_rs232_options, FOSC0);

	gpio_enable_pin_pull_up(AVR32_USART0_RXD_0_0_PIN);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitExternalKeypad(void)
{
	uint8 keyScan = MXC_GPIO_InGet(MXC_GPIO1, BUTTON_GPIO_MASK);
	if (keyScan)
	{
		debugWarn("Keypad button being pressed (likely a bug), Key: %x\r\n", keyScan);
	}

	// Todo: Find the right LED to light (1&2=Red?, 3&4=Green?)
	PowerControl(LED_1, ON);
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

	// Read a few test samples
	GetChannelOffsets(SAMPLE_RATE_DEFAULT);

	DisableSensorBlocks();
	PowerControl(ADC_RESET, ON);
	PowerControl(ANALOG_5V_ENABLE, OFF);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestPowerDownAndStop(void)
{
#if 0 /* old hw */
	// Turn off the keypad LED
	WriteMcp23018(IO_ADDRESS_KPD, GPIOA, ((ReadMcp23018(IO_ADDRESS_KPD, GPIOA) & 0xCF) | NO_LED_PINS));

	spi_reset(&AVR32_SPI1);
	gpio_clr_gpio_pin(AVR32_SPI1_MISO_0_0_PIN);
	gpio_clr_gpio_pin(AVR32_SPI1_MOSI_0_0_PIN);
	gpio_clr_gpio_pin(AVR32_SPI1_NPCS_3_PIN);

	debug("\nClosing up shop.\n\r\n");

	//DisplayTimerCallBack();
	SetLcdBacklightState(BACKLIGHT_OFF);

	//LcdPwTimerCallBack();
	PowerControl(LCD_CONTRAST_ENABLE, OFF);
	ClearLcdDisplay();
	ClearControlLinesLcdDisplay();
	LcdClearPortReg();
	PowerControl(LCD_POWER_ENABLE, OFF);

	// Drive the unused pin
	gpio_clr_gpio_pin(AVR32_PIN_PB20);

	//SLEEP(AVR32_PM_SMODE_IDLE);
	SLEEP(AVR32_PM_SMODE_STOP);
	//SLEEP(AVR32_PM_SMODE_STANDBY);
	while (1) {}
#endif
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

mxc_gpio_cfg_t g_MCUPowerLatch;
mxc_gpio_cfg_t g_ExpandedBattery;
mxc_gpio_cfg_t g_LED1;
mxc_gpio_cfg_t g_GaugeAlert;
mxc_gpio_cfg_t g_BatteryChargerIRQ;
mxc_gpio_cfg_t g_Enable12V;
mxc_gpio_cfg_t g_Enable5V;
mxc_gpio_cfg_t g_ExpansionIRQ;
mxc_gpio_cfg_t g_USBSourceEnable;
mxc_gpio_cfg_t g_USBAuxPowerEnable;
mxc_gpio_cfg_t g_PowerGood5v;
mxc_gpio_cfg_t g_PowerGoodBatteryCharge;
mxc_gpio_cfg_t g_SmartSensorSleep;
mxc_gpio_cfg_t g_SmartSensorMuxEnable;
mxc_gpio_cfg_t g_ADCReset;
mxc_gpio_cfg_t g_ADCConversion;
mxc_gpio_cfg_t g_CalMuxPreADEnable;
mxc_gpio_cfg_t g_CalMuxPreADSelect;
mxc_gpio_cfg_t g_Alert1;
mxc_gpio_cfg_t g_Alert2;
mxc_gpio_cfg_t g_LTEOTA;
mxc_gpio_cfg_t g_ExpansionEnable;
mxc_gpio_cfg_t g_ExpansionReset;
mxc_gpio_cfg_t g_USBCPortControllerI2CIRQ;
mxc_gpio_cfg_t g_AccelInt1;
mxc_gpio_cfg_t g_AccelInt2;
mxc_gpio_cfg_t g_AccelTrig;
mxc_gpio_cfg_t g_PowerButtonIRQ;
mxc_gpio_cfg_t g_Button1;
mxc_gpio_cfg_t g_Button2;
mxc_gpio_cfg_t g_Button3;
mxc_gpio_cfg_t g_Button4;
mxc_gpio_cfg_t g_Button5;
mxc_gpio_cfg_t g_Button6;
mxc_gpio_cfg_t g_Button7;
mxc_gpio_cfg_t g_Button8;
mxc_gpio_cfg_t g_Button9;
mxc_gpio_cfg_t g_LED2;
mxc_gpio_cfg_t g_LED3;
mxc_gpio_cfg_t g_LED4;
mxc_gpio_cfg_t g_ExtRTCIntA;
mxc_gpio_cfg_t g_BLEOTA;
mxc_gpio_cfg_t g_ExternalTriggerOut;
mxc_gpio_cfg_t g_ExternalTriggerIn;
mxc_gpio_cfg_t g_LCDPowerEnable;
mxc_gpio_cfg_t g_LCDPowerDisplay;
mxc_gpio_cfg_t g_Spi2SlaveSelect0LCD;
mxc_gpio_cfg_t g_LCDInt;
mxc_gpio_cfg_t g_SensorCheckEnable;
mxc_gpio_cfg_t g_SensorCheck;
mxc_gpio_cfg_t g_LTEReset;
mxc_gpio_cfg_t g_BLEReset;
mxc_gpio_cfg_t g_SmartSensorMux_A0;
mxc_gpio_cfg_t g_SmartSensorMux_A1;
mxc_gpio_cfg_t g_Nyquist0_A0;
mxc_gpio_cfg_t g_Nyquist1_A1;
mxc_gpio_cfg_t g_Nyquist2_Enable;
mxc_gpio_cfg_t g_CellEnable;
mxc_gpio_cfg_t g_SensorEnable1_Geo1;
mxc_gpio_cfg_t g_SensorEnable2_Aop1;
mxc_gpio_cfg_t g_SensorEnable3_Geo2;
mxc_gpio_cfg_t g_SensorEnable4_Aop2;
mxc_gpio_cfg_t g_GainPathSelect1_Geo1;
mxc_gpio_cfg_t g_GainPathSelect2_Aop1;
mxc_gpio_cfg_t g_GainPathSelect3_Geo2;
mxc_gpio_cfg_t g_GainPathSelect4_Aop2;
mxc_gpio_cfg_t g_RTCClock;
mxc_gpio_cfg_t g_AdcBusyAltGP0;

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
void RTCClock_ISR(void *cbdata);

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupPowerOnDetectGPIO(void)
{
	//----------------------------------------------------------------------------------------------------------------------
	// MCU Power Latch: Port 0, Pin 1, Output, External pulldown, Active high, 3.3V
	//----------------------------------------------------------------------------------------------------------------------
	g_MCUPowerLatch.port = MXC_GPIO0;
	g_MCUPowerLatch.mask = MXC_GPIO_PIN_1;
	g_MCUPowerLatch.pad = MXC_GPIO_PAD_NONE;
	g_MCUPowerLatch.func = MXC_GPIO_FUNC_OUT;
	g_MCUPowerLatch.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_MCUPowerLatch);

	//----------------------------------------------------------------------------------------------------------------------
	// Power Good Battery Charge: Port 0, Pin 12, Input, External pullup, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_PowerGoodBatteryCharge.port = MXC_GPIO0;
	g_PowerGoodBatteryCharge.mask = MXC_GPIO_PIN_12;
	g_PowerGoodBatteryCharge.pad = MXC_GPIO_PAD_NONE;
	g_PowerGoodBatteryCharge.func = MXC_GPIO_FUNC_IN;
	g_PowerGoodBatteryCharge.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&g_PowerGoodBatteryCharge);

	//----------------------------------------------------------------------------------------------------------------------
	// Power Button Int: Port 1, Pin 15, Input, External pullup, Active high, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_PowerButtonIRQ.port = MXC_GPIO1;
	g_PowerButtonIRQ.mask = MXC_GPIO_PIN_15;
	g_PowerButtonIRQ.pad = MXC_GPIO_PAD_NONE;
	g_PowerButtonIRQ.func = MXC_GPIO_FUNC_IN;
	g_PowerButtonIRQ.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&g_PowerButtonIRQ);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupGPIO(void)
{
	//----------------------------------------------------------------------------------------------------------------------
	// MCU Power Latch: Port 0, Pin 1, Output, External pulldown, Active high, 3.3V
	//----------------------------------------------------------------------------------------------------------------------
	g_MCUPowerLatch.port = MXC_GPIO0;
	g_MCUPowerLatch.mask = MXC_GPIO_PIN_1;
	g_MCUPowerLatch.pad = MXC_GPIO_PAD_NONE;
	g_MCUPowerLatch.func = MXC_GPIO_FUNC_OUT;
	g_MCUPowerLatch.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_MCUPowerLatch);
	MXC_GPIO_OutSet(g_MCUPowerLatch.port, g_MCUPowerLatch.mask); // Start enabled

	//----------------------------------------------------------------------------------------------------------------------
	// Expanded Battery Detect: Port 0, Pin 2, Input, External pullup, Active high, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_ExpandedBattery.port = MXC_GPIO0;
	g_ExpandedBattery.mask = MXC_GPIO_PIN_2;
	g_ExpandedBattery.pad = MXC_GPIO_PAD_NONE;
	g_ExpandedBattery.func = MXC_GPIO_FUNC_IN;
	g_ExpandedBattery.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&g_ExpandedBattery);

	//----------------------------------------------------------------------------------------------------------------------
	// LED 1: Port 0, Pin 3, Output, No external pull, Active high, 3.3V
	//----------------------------------------------------------------------------------------------------------------------
	g_LED1.port = MXC_GPIO0;
	g_LED1.mask = MXC_GPIO_PIN_3;
	g_LED1.pad = MXC_GPIO_PAD_NONE;
	g_LED1.func = MXC_GPIO_FUNC_OUT;
	g_LED1.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_LED1);
	MXC_GPIO_OutClr(g_LED1.port, g_LED1.mask); // Start as off

	//----------------------------------------------------------------------------------------------------------------------
	// Gauge Alert: Port 0, Pin 4, Input, External pullup, Active low, 1.8V, Interrupt
	//----------------------------------------------------------------------------------------------------------------------
	g_GaugeAlert.port = MXC_GPIO0;
	g_GaugeAlert.mask = MXC_GPIO_PIN_4;
	g_GaugeAlert.pad = MXC_GPIO_PAD_NONE;
	g_GaugeAlert.func = MXC_GPIO_FUNC_IN;
	g_GaugeAlert.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&g_GaugeAlert);
	MXC_GPIO_RegisterCallback(&g_GaugeAlert, (mxc_gpio_callback_fn)Fuel_gauge_alert_irq, NULL);
    MXC_GPIO_IntConfig(&g_GaugeAlert, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_GaugeAlert.port, g_GaugeAlert.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Battery Charger IRQ: Port 0, Pin 5, Input, Needs strong internal pullup, Active low, 1.8V, Interrupt
	//----------------------------------------------------------------------------------------------------------------------
	g_BatteryChargerIRQ.port = MXC_GPIO0;
	g_BatteryChargerIRQ.mask = MXC_GPIO_PIN_5;
	g_BatteryChargerIRQ.pad = MXC_GPIO_PAD_STRONG_PULL_UP;
	g_BatteryChargerIRQ.func = MXC_GPIO_FUNC_IN;
	g_BatteryChargerIRQ.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&g_BatteryChargerIRQ);
	MXC_GPIO_RegisterCallback(&g_BatteryChargerIRQ, (mxc_gpio_callback_fn)Battery_charger_irq, NULL);
    MXC_GPIO_IntConfig(&g_BatteryChargerIRQ, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_BatteryChargerIRQ.port, g_BatteryChargerIRQ.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Enable 12V = Port 0, Pin 6, Output, External pulldown, Active high, 1.8V (minimum 1.5V)
	//----------------------------------------------------------------------------------------------------------------------
	g_Enable12V.port = MXC_GPIO0;
	g_Enable12V.mask = MXC_GPIO_PIN_6;
	g_Enable12V.pad = MXC_GPIO_PAD_NONE;
	g_Enable12V.func = MXC_GPIO_FUNC_OUT;
	g_Enable12V.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&g_Enable12V);
	MXC_GPIO_OutClr(g_Enable12V.port, g_Enable12V.mask); // Start disabled (only needed for alarms)

	//----------------------------------------------------------------------------------------------------------------------
	// Enable 5V: Port 0, Pin 7, Output, External pulldown, Active high, 1.8V (minimum 0.9V)
	//----------------------------------------------------------------------------------------------------------------------
	g_Enable5V.port = MXC_GPIO0;
	g_Enable5V.mask = MXC_GPIO_PIN_7;
	g_Enable5V.pad = MXC_GPIO_PAD_NONE;
	g_Enable5V.func = MXC_GPIO_FUNC_OUT;
	g_Enable5V.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&g_Enable5V);
	MXC_GPIO_OutClr(g_Enable5V.port, g_Enable5V.mask); // Start disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Expansion IRQ: Port 0, Pin 8, Input, External pullup, Active low, 3.3V (minimum 2V)
	//----------------------------------------------------------------------------------------------------------------------
	g_ExpansionIRQ.port = MXC_GPIO1;
	g_ExpansionIRQ.mask = MXC_GPIO_PIN_9;
	g_ExpansionIRQ.pad = MXC_GPIO_PAD_NONE;
	g_ExpansionIRQ.func = MXC_GPIO_FUNC_IN;
	g_ExpansionIRQ.vssel = MXC_GPIO_VSSEL_VDDIOH;
	MXC_GPIO_Config(&g_ExpansionIRQ);
	MXC_GPIO_RegisterCallback(&g_ExpansionIRQ, (mxc_gpio_callback_fn)Expansion_irq, NULL);
    MXC_GPIO_IntConfig(&g_ExpansionIRQ, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_ExpansionIRQ.port, g_ExpansionIRQ.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// USB Source Enable: Port 0, Pin 9, Output, External pulldown, Active high, 1.8V (minimum 1.2V)
	//----------------------------------------------------------------------------------------------------------------------
	g_USBSourceEnable.port = MXC_GPIO0;
	g_USBSourceEnable.mask = MXC_GPIO_PIN_9;
	g_USBSourceEnable.pad = MXC_GPIO_PAD_NONE;
	g_USBSourceEnable.func = MXC_GPIO_FUNC_OUT;
	g_USBSourceEnable.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&g_USBSourceEnable);
	MXC_GPIO_OutClr(g_USBSourceEnable.port, g_USBSourceEnable.mask); // Start disabled

	//----------------------------------------------------------------------------------------------------------------------
	// USB Aux Power Enable: Port 0, Pin 10, Output, External pulldown, Active high, 1.8V (minimum 0.6V)
	//----------------------------------------------------------------------------------------------------------------------
	g_USBAuxPowerEnable.port = MXC_GPIO0;
	g_USBAuxPowerEnable.mask = MXC_GPIO_PIN_10;
	g_USBAuxPowerEnable.pad = MXC_GPIO_PAD_NONE;
	g_USBAuxPowerEnable.func = MXC_GPIO_FUNC_OUT;
	g_USBAuxPowerEnable.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&g_USBAuxPowerEnable);
	MXC_GPIO_OutClr(g_USBAuxPowerEnable.port, g_USBAuxPowerEnable.mask); // Start disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Power Good 5v: Port 0, Pin 11, Input, External pullup, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_PowerGood5v.port = MXC_GPIO0;
	g_PowerGood5v.mask = MXC_GPIO_PIN_11;
	g_PowerGood5v.pad = MXC_GPIO_PAD_NONE;
	g_PowerGood5v.func = MXC_GPIO_FUNC_IN;
	g_PowerGood5v.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&g_PowerGood5v);

	//----------------------------------------------------------------------------------------------------------------------
	// Power Good Battery Charge: Port 0, Pin 12, Input, External pullup, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_PowerGoodBatteryCharge.port = MXC_GPIO0;
	g_PowerGoodBatteryCharge.mask = MXC_GPIO_PIN_12;
	g_PowerGoodBatteryCharge.pad = MXC_GPIO_PAD_NONE;
	g_PowerGoodBatteryCharge.func = MXC_GPIO_FUNC_IN;
	g_PowerGoodBatteryCharge.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&g_PowerGoodBatteryCharge);

	//----------------------------------------------------------------------------------------------------------------------
	// Smart Sensor Sleep: Port 0, Pin 13, Output, No external pull, Active low, 1.8V (minimum 1.3V)
	//----------------------------------------------------------------------------------------------------------------------
	g_SmartSensorSleep.port = MXC_GPIO0;
	g_SmartSensorSleep.mask = MXC_GPIO_PIN_13;
	g_SmartSensorSleep.pad = MXC_GPIO_PAD_NONE;
	g_SmartSensorSleep.func = MXC_GPIO_FUNC_OUT;
	g_SmartSensorSleep.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&g_SmartSensorSleep);
	MXC_GPIO_OutClr(g_SmartSensorSleep.port, g_SmartSensorSleep.mask); // Start in sleep

	//----------------------------------------------------------------------------------------------------------------------
	// Smart Sensor Mux Enable: Port 0, Pin 14, Output, External pulldown, Active high, 3.3V (minimum 2.0V)
	//----------------------------------------------------------------------------------------------------------------------
	g_SmartSensorMuxEnable.port = MXC_GPIO0;
	g_SmartSensorMuxEnable.mask = MXC_GPIO_PIN_14;
	g_SmartSensorMuxEnable.pad = MXC_GPIO_PAD_NONE;
	g_SmartSensorMuxEnable.func = MXC_GPIO_FUNC_OUT;
	g_SmartSensorMuxEnable.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_SmartSensorMuxEnable);
	MXC_GPIO_OutClr(g_SmartSensorMuxEnable.port, g_SmartSensorMuxEnable.mask); // Start disabled

	//----------------------------------------------------------------------------------------------------------------------
	// ADC Reset: Port 0, Pin 15, Output, External pulldown, Active low, 1.8V (minimuim 1.26V)
	//----------------------------------------------------------------------------------------------------------------------
	g_ADCReset.port = MXC_GPIO0;
	g_ADCReset.mask = MXC_GPIO_PIN_15;
	g_ADCReset.pad = MXC_GPIO_PAD_NONE;
	g_ADCReset.func = MXC_GPIO_FUNC_OUT;
	g_ADCReset.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&g_ADCReset);
	MXC_GPIO_OutClr(g_ADCReset.port, g_ADCReset.mask); // Start in reset

	//----------------------------------------------------------------------------------------------------------------------
	// ADC Conversion: Port 0, Pin 18, Output, External pulldown, Active high, 1.8V (minimuim 1.26V)
	//----------------------------------------------------------------------------------------------------------------------
	g_ADCConversion.port = MXC_GPIO0;
	g_ADCConversion.mask = MXC_GPIO_PIN_18;
	g_ADCConversion.pad = MXC_GPIO_PAD_NONE;
	g_ADCConversion.func = MXC_GPIO_FUNC_OUT;
	g_ADCConversion.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&g_ADCConversion);
	MXC_GPIO_OutClr(g_ADCConversion.port, g_ADCConversion.mask); // Start as no conversion

	//----------------------------------------------------------------------------------------------------------------------
	// Cal Mux Pre-A/D Enable: Port 0, Pin 22, Output, External pulldown, Active low, 3.3V (minimum 2V)
	//----------------------------------------------------------------------------------------------------------------------
	g_CalMuxPreADEnable.port = MXC_GPIO0;
	g_CalMuxPreADEnable.mask = MXC_GPIO_PIN_22;
	g_CalMuxPreADEnable.pad = MXC_GPIO_PAD_NONE;
	g_CalMuxPreADEnable.func = MXC_GPIO_FUNC_OUT;
	g_CalMuxPreADEnable.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_CalMuxPreADEnable);
	MXC_GPIO_OutSet(g_CalMuxPreADEnable.port, g_CalMuxPreADEnable.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Cal Mux Pre-A/D Select: Port 0, Pin 23, Output, External pulldown, Select (0 is default), 3.3V (minimum 2V)
	//----------------------------------------------------------------------------------------------------------------------
	g_CalMuxPreADSelect.port = MXC_GPIO0;
	g_CalMuxPreADSelect.mask = MXC_GPIO_PIN_23;
	g_CalMuxPreADSelect.pad = MXC_GPIO_PAD_NONE;
	g_CalMuxPreADSelect.func = MXC_GPIO_FUNC_OUT;
	g_CalMuxPreADSelect.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_CalMuxPreADSelect);
	MXC_GPIO_OutClr(g_CalMuxPreADSelect.port, g_CalMuxPreADSelect.mask); // Start as 0 (Full sensor group Geo1 + AOP1)

	//----------------------------------------------------------------------------------------------------------------------
	// Alert 1: Port 0, Pin 24, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	g_Alert1.port = MXC_GPIO0;
	g_Alert1.mask = MXC_GPIO_PIN_24;
	g_Alert1.pad = MXC_GPIO_PAD_NONE;
	g_Alert1.func = MXC_GPIO_FUNC_OUT;
	g_Alert1.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&g_Alert1);
	MXC_GPIO_OutClr(g_Alert1.port, g_Alert1.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Alert 2: Port 0, Pin 25, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	g_Alert2.port = MXC_GPIO0;
	g_Alert2.mask = MXC_GPIO_PIN_25;
	g_Alert2.pad = MXC_GPIO_PAD_NONE;
	g_Alert2.func = MXC_GPIO_FUNC_OUT;
	g_Alert2.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&g_Alert2);
	MXC_GPIO_OutClr(g_Alert2.port, g_Alert2.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// LTE OTA: Port 0, Pin 30, Input, No external pull, Active unknown, 3.3V (device runs 3.3V)
	//----------------------------------------------------------------------------------------------------------------------
	g_LTEOTA.port = MXC_GPIO0;
	g_LTEOTA.mask = MXC_GPIO_PIN_30;
	g_LTEOTA.pad = MXC_GPIO_PAD_NONE;
	g_LTEOTA.func = MXC_GPIO_FUNC_IN;
	g_LTEOTA.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_LTEOTA);
	// Todo: Fill in handling when more information known

	//----------------------------------------------------------------------------------------------------------------------
	// Expansion Enable: Port 1, Pin 7, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	g_ExpansionEnable.port = MXC_GPIO1;
	g_ExpansionEnable.mask = MXC_GPIO_PIN_7;
	g_ExpansionEnable.pad = MXC_GPIO_PAD_NONE;
	g_ExpansionEnable.func = MXC_GPIO_FUNC_OUT;
	g_ExpansionEnable.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&g_ExpansionEnable);
	MXC_GPIO_OutClr(g_ExpansionEnable.port, g_ExpansionEnable.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Expansion Reset: Port 1, Pin 8, Output, External pulldown, Active low, 3.3V (minimum 2V)
	//----------------------------------------------------------------------------------------------------------------------
	g_ExpansionReset.port = MXC_GPIO1;
	g_ExpansionReset.mask = MXC_GPIO_PIN_8;
	g_ExpansionReset.pad = MXC_GPIO_PAD_NONE;
	g_ExpansionReset.func = MXC_GPIO_FUNC_OUT;
	g_ExpansionReset.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_ExpansionReset);
	MXC_GPIO_OutClr(g_ExpansionReset.port, g_ExpansionReset.mask); // Start in reset

	//----------------------------------------------------------------------------------------------------------------------
	// USBC Port Controller I2C IRQ: Port 1, Pin 11, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_USBCPortControllerI2CIRQ.port = MXC_GPIO1;
	g_USBCPortControllerI2CIRQ.mask = MXC_GPIO_PIN_11;
	g_USBCPortControllerI2CIRQ.pad = MXC_GPIO_PAD_NONE;
	g_USBCPortControllerI2CIRQ.func = MXC_GPIO_FUNC_IN;
	g_USBCPortControllerI2CIRQ.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&g_USBCPortControllerI2CIRQ);
	MXC_GPIO_RegisterCallback(&g_USBCPortControllerI2CIRQ, (mxc_gpio_callback_fn)Usbc_port_controller_i2c_irq, NULL);
    MXC_GPIO_IntConfig(&g_USBCPortControllerI2CIRQ, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_USBCPortControllerI2CIRQ.port, g_USBCPortControllerI2CIRQ.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Accel Int 1: Port 1, Pin 12, Input, No external pull, Active high, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_AccelInt1.port = MXC_GPIO1;
	g_AccelInt1.mask = MXC_GPIO_PIN_12;
	g_AccelInt1.pad = MXC_GPIO_PAD_NONE;
	g_AccelInt1.func = MXC_GPIO_FUNC_IN;
	g_AccelInt1.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&g_AccelInt1);
	MXC_GPIO_RegisterCallback(&g_AccelInt1, (mxc_gpio_callback_fn)Accelerometer_irq_1, NULL);
    MXC_GPIO_IntConfig(&g_AccelInt1, MXC_GPIO_INT_RISING);
    MXC_GPIO_EnableInt(g_AccelInt1.port, g_AccelInt1.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Accel Int 2: Port 1, Pin 13, Input, No external pull, Active high, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_AccelInt2.port = MXC_GPIO1;
	g_AccelInt2.mask = MXC_GPIO_PIN_13;
	g_AccelInt2.pad = MXC_GPIO_PAD_NONE;
	g_AccelInt2.func = MXC_GPIO_FUNC_IN;
	g_AccelInt2.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&g_AccelInt2);
	MXC_GPIO_RegisterCallback(&g_AccelInt2, (mxc_gpio_callback_fn)Accelerometer_irq_2, NULL);
    MXC_GPIO_IntConfig(&g_AccelInt2, MXC_GPIO_INT_RISING);
    MXC_GPIO_EnableInt(g_AccelInt2.port, g_AccelInt2.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Accel Trig: Port 1, Pin 14, Output, No external pull, Active high, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_AccelTrig.port = MXC_GPIO1;
	g_AccelTrig.mask = MXC_GPIO_PIN_14;
	g_AccelTrig.pad = MXC_GPIO_PAD_NONE;
	g_AccelTrig.func = MXC_GPIO_FUNC_OUT;
	g_AccelTrig.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&g_AccelTrig);
	MXC_GPIO_OutClr(g_AccelTrig.port, g_AccelTrig.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Power Button Int: Port 1, Pin 15, Input, External pullup, Active high, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_PowerButtonIRQ.port = MXC_GPIO1;
	g_PowerButtonIRQ.mask = MXC_GPIO_PIN_15;
	g_PowerButtonIRQ.pad = MXC_GPIO_PAD_NONE;
	g_PowerButtonIRQ.func = MXC_GPIO_FUNC_IN;
	g_PowerButtonIRQ.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&g_PowerButtonIRQ);
	MXC_GPIO_RegisterCallback(&g_PowerButtonIRQ, (mxc_gpio_callback_fn)System_power_button_irq, NULL);
    MXC_GPIO_IntConfig(&g_PowerButtonIRQ, MXC_GPIO_INT_RISING);
    MXC_GPIO_EnableInt(g_PowerButtonIRQ.port, g_PowerButtonIRQ.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Button 1: Port 1, Pin 16, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_Button1.port = MXC_GPIO1;
	g_Button1.mask = MXC_GPIO_PIN_16;
	g_Button1.pad = MXC_GPIO_PAD_NONE;
	g_Button1.func = MXC_GPIO_FUNC_IN;
	g_Button1.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&g_Button1);
	MXC_GPIO_RegisterCallback(&g_Button1, (mxc_gpio_callback_fn)Keypad_irq, NULL);
    MXC_GPIO_IntConfig(&g_Button1, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_Button1.port, g_Button1.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Button 2: Port 1, Pin 17, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_Button2.port = MXC_GPIO1;
	g_Button2.mask = MXC_GPIO_PIN_17;
	g_Button2.pad = MXC_GPIO_PAD_NONE;
	g_Button2.func = MXC_GPIO_FUNC_IN;
	g_Button2.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&g_Button2);
	MXC_GPIO_RegisterCallback(&g_Button2, (mxc_gpio_callback_fn)Keypad_irq, NULL);
    MXC_GPIO_IntConfig(&g_Button2, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_Button2.port, g_Button2.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Button 3: Port 1, Pin 18, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_Button3.port = MXC_GPIO1;
	g_Button3.mask = MXC_GPIO_PIN_18;
	g_Button3.pad = MXC_GPIO_PAD_NONE;
	g_Button3.func = MXC_GPIO_FUNC_IN;
	g_Button3.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&g_Button3);
	MXC_GPIO_RegisterCallback(&g_Button3, (mxc_gpio_callback_fn)Keypad_irq, NULL);
    MXC_GPIO_IntConfig(&g_Button3, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_Button3.port, g_Button3.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Button 4: Port 1, Pin 19, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_Button4.port = MXC_GPIO1;
	g_Button4.mask = MXC_GPIO_PIN_19;
	g_Button4.pad = MXC_GPIO_PAD_NONE;
	g_Button4.func = MXC_GPIO_FUNC_IN;
	g_Button4.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&g_Button4);
	MXC_GPIO_RegisterCallback(&g_Button4, (mxc_gpio_callback_fn)Keypad_irq, NULL);
    MXC_GPIO_IntConfig(&g_Button4, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_Button4.port, g_Button4.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Button 5: Port 1, Pin 20, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_Button5.port = MXC_GPIO1;
	g_Button5.mask = MXC_GPIO_PIN_20;
	g_Button5.pad = MXC_GPIO_PAD_NONE;
	g_Button5.func = MXC_GPIO_FUNC_IN;
	g_Button5.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&g_Button5);
	MXC_GPIO_RegisterCallback(&g_Button5, (mxc_gpio_callback_fn)Keypad_irq, NULL);
    MXC_GPIO_IntConfig(&g_Button5, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_Button5.port, g_Button5.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Button 6: Port 1, Pin 21, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_Button6.port = MXC_GPIO1;
	g_Button6.mask = MXC_GPIO_PIN_21;
	g_Button6.pad = MXC_GPIO_PAD_NONE;
	g_Button6.func = MXC_GPIO_FUNC_IN;
	g_Button6.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&g_Button6);
	MXC_GPIO_RegisterCallback(&g_Button6, (mxc_gpio_callback_fn)Keypad_irq, NULL);
    MXC_GPIO_IntConfig(&g_Button6, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_Button6.port, g_Button6.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Button 7: Port 1, Pin 22, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_Button7.port = MXC_GPIO1;
	g_Button7.mask = MXC_GPIO_PIN_22;
	g_Button7.pad = MXC_GPIO_PAD_NONE;
	g_Button7.func = MXC_GPIO_FUNC_IN;
	g_Button7.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&g_Button7);
	MXC_GPIO_RegisterCallback(&g_Button7, (mxc_gpio_callback_fn)Keypad_irq, NULL);
    MXC_GPIO_IntConfig(&g_Button7, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_Button7.port, g_Button7.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Button 8: Port 1, Pin 23, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_Button8.port = MXC_GPIO1;
	g_Button8.mask = MXC_GPIO_PIN_23;
	g_Button8.pad = MXC_GPIO_PAD_NONE;
	g_Button8.func = MXC_GPIO_FUNC_IN;
	g_Button8.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&g_Button8);
	MXC_GPIO_RegisterCallback(&g_Button8, (mxc_gpio_callback_fn)Keypad_irq, NULL);
    MXC_GPIO_IntConfig(&g_Button8, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_Button8.port, g_Button8.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Button 9: Port 1, Pin 24, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_Button9.port = MXC_GPIO1;
	g_Button9.mask = MXC_GPIO_PIN_24;
	g_Button9.pad = MXC_GPIO_PAD_NONE;
	g_Button9.func = MXC_GPIO_FUNC_IN;
	g_Button9.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&g_Button9);
	MXC_GPIO_RegisterCallback(&g_Button9, (mxc_gpio_callback_fn)Keypad_irq, NULL);
    MXC_GPIO_IntConfig(&g_Button9, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_Button9.port, g_Button9.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// LED 2: Port 1, Pin 25, Output, No external pull, Active high, 3.3V
	//----------------------------------------------------------------------------------------------------------------------
	g_LED2.port = MXC_GPIO1;
	g_LED2.mask = MXC_GPIO_PIN_25;
	g_LED2.pad = MXC_GPIO_PAD_NONE;
	g_LED2.func = MXC_GPIO_FUNC_OUT;
	g_LED2.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_LED2);
	MXC_GPIO_OutClr(g_LED2.port, g_LED2.mask); // Start as off

	//----------------------------------------------------------------------------------------------------------------------
	// LED 3: Port 1, Pin 26, Output, No external pull, Active high, 3.3V
	//----------------------------------------------------------------------------------------------------------------------
	g_LED3.port = MXC_GPIO1;
	g_LED3.mask = MXC_GPIO_PIN_26;
	g_LED3.pad = MXC_GPIO_PAD_NONE;
	g_LED3.func = MXC_GPIO_FUNC_OUT;
	g_LED3.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_LED3);
	MXC_GPIO_OutClr(g_LED3.port, g_LED3.mask); // Start as off

	//----------------------------------------------------------------------------------------------------------------------
	// LED 4: Port 1, Pin 27, Output, No external pull, Active high, 3.3V
	//----------------------------------------------------------------------------------------------------------------------
	g_LED4.port = MXC_GPIO1;
	g_LED4.mask = MXC_GPIO_PIN_27;
	g_LED4.pad = MXC_GPIO_PAD_NONE;
	g_LED4.func = MXC_GPIO_FUNC_OUT;
	g_LED4.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_LED4);
	MXC_GPIO_OutClr(g_LED4.port, g_LED4.mask); // Start as off

	//----------------------------------------------------------------------------------------------------------------------
	// RTC Int A: Port 1, Pin 28, Input, External pullup, Active low, 1.8V (minimum 0.66V)
	//----------------------------------------------------------------------------------------------------------------------
	g_ExtRTCIntA.port = MXC_GPIO1;
	g_ExtRTCIntA.mask = MXC_GPIO_PIN_28;
	g_ExtRTCIntA.pad = MXC_GPIO_PAD_NONE;
	g_ExtRTCIntA.func = MXC_GPIO_FUNC_IN;
	g_ExtRTCIntA.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&g_ExtRTCIntA);
	MXC_GPIO_RegisterCallback(&g_ExtRTCIntA, (mxc_gpio_callback_fn)External_rtc_irq, NULL);
    MXC_GPIO_IntConfig(&g_ExtRTCIntA, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_ExtRTCIntA.port, g_ExtRTCIntA.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// BLE OTA: Port 1, Pin 29, Input, No external pull, Active unknown, 3.3V (device runs 3.3V)
	//----------------------------------------------------------------------------------------------------------------------
	g_BLEOTA.port = MXC_GPIO1;
	g_BLEOTA.mask = MXC_GPIO_PIN_29;
	g_BLEOTA.pad = MXC_GPIO_PAD_NONE;
	g_BLEOTA.func = MXC_GPIO_FUNC_IN;
	g_BLEOTA.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_BLEOTA);
	// Todo: Fill in handling when more information known

	//----------------------------------------------------------------------------------------------------------------------
	// Trig Out: Port 1, Pin 30, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	g_ExternalTriggerOut.port = MXC_GPIO1;
	g_ExternalTriggerOut.mask = MXC_GPIO_PIN_30;
	g_ExternalTriggerOut.pad = MXC_GPIO_PAD_NONE;
	g_ExternalTriggerOut.func = MXC_GPIO_FUNC_OUT;
	g_ExternalTriggerOut.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&g_ExternalTriggerOut);
	MXC_GPIO_OutClr(g_ExternalTriggerOut.port, g_ExternalTriggerOut.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Trig In: Port 1, Pin 31, Input, External pullup, Active high, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_ExternalTriggerIn.port = MXC_GPIO1;
	g_ExternalTriggerIn.mask = MXC_GPIO_PIN_31;
	g_ExternalTriggerIn.pad = MXC_GPIO_PAD_NONE;
	g_ExternalTriggerIn.func = MXC_GPIO_FUNC_IN;
	g_ExternalTriggerIn.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
	MXC_GPIO_Config(&g_ExternalTriggerIn);
	MXC_GPIO_RegisterCallback(&g_ExternalTriggerIn, (mxc_gpio_callback_fn)External_trigger_irq, NULL);
    MXC_GPIO_IntConfig(&g_ExternalTriggerIn, MXC_GPIO_INT_RISING);
    MXC_GPIO_EnableInt(g_ExternalTriggerIn.port, g_ExternalTriggerIn.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// LCD Power Enable: Port 2, Pin 0, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	g_LCDPowerEnable.port = MXC_GPIO2;
	g_LCDPowerEnable.mask = MXC_GPIO_PIN_0;
	g_LCDPowerEnable.pad = MXC_GPIO_PAD_NONE;
	g_LCDPowerEnable.func = MXC_GPIO_FUNC_OUT;
	g_LCDPowerEnable.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&g_LCDPowerEnable);
	MXC_GPIO_OutClr(g_LCDPowerEnable.port, g_LCDPowerEnable.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// LCD Power Display: Port 2, Pin 1, Output, External pulldown, Active low, 1.8V (minimum 1.7V)
	//----------------------------------------------------------------------------------------------------------------------
	g_LCDPowerDisplay.port = MXC_GPIO2;
	g_LCDPowerDisplay.mask = MXC_GPIO_PIN_1;
	g_LCDPowerDisplay.pad = MXC_GPIO_PAD_NONE;
	g_LCDPowerDisplay.func = MXC_GPIO_FUNC_OUT;
	g_LCDPowerDisplay.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&g_LCDPowerDisplay);
	MXC_GPIO_OutSet(g_LCDPowerDisplay.port, g_LCDPowerDisplay.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// SPI2 Slave Select 0 LCD: Port 2, Pin 5, Output, External pullup, Active low, 1.8V (minimum 1.7V)
	//----------------------------------------------------------------------------------------------------------------------
	if (FT81X_SPI_2_SS_CONTROL_MANUAL)
	{
		g_Spi2SlaveSelect0LCD.port = MXC_GPIO2;
		g_Spi2SlaveSelect0LCD.mask = MXC_GPIO_PIN_5;
		g_Spi2SlaveSelect0LCD.pad = MXC_GPIO_PAD_NONE;
		g_Spi2SlaveSelect0LCD.func = MXC_GPIO_FUNC_OUT;
		g_Spi2SlaveSelect0LCD.vssel = MXC_GPIO_VSSEL_VDDIO;
		MXC_GPIO_Config(&g_Spi2SlaveSelect0LCD);
		MXC_GPIO_OutSet(g_Spi2SlaveSelect0LCD.port, g_Spi2SlaveSelect0LCD.mask); // Start as disabled
	}

	//----------------------------------------------------------------------------------------------------------------------
	// LCD Int: Port 2, Pin 6, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_LCDInt.port = MXC_GPIO2;
	g_LCDInt.mask = MXC_GPIO_PIN_6;
	g_LCDInt.pad = MXC_GPIO_PAD_NONE;
	g_LCDInt.func = MXC_GPIO_FUNC_IN;
	g_LCDInt.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&g_LCDInt);
	MXC_GPIO_RegisterCallback(&g_LCDInt, (mxc_gpio_callback_fn)Lcd_irq, NULL);
    MXC_GPIO_IntConfig(&g_LCDInt, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_LCDInt.port, g_LCDInt.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Sensor Check Enable: Port 2, Pin 9, Output, External pulldown, Active high, 1.8V (minimum 0.65 * Vin)
	//----------------------------------------------------------------------------------------------------------------------
	g_SensorCheckEnable.port = MXC_GPIO2;
	g_SensorCheckEnable.mask = MXC_GPIO_PIN_9;
	g_SensorCheckEnable.pad = MXC_GPIO_PAD_NONE;
	g_SensorCheckEnable.func = MXC_GPIO_FUNC_OUT;
	g_SensorCheckEnable.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&g_SensorCheckEnable);
	MXC_GPIO_OutClr(g_SensorCheckEnable.port, g_SensorCheckEnable.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Sensor Check: Port 2, Pin 10, Output, No external pull, Active high, 1.8V (must match Sensor Check enable)
	//----------------------------------------------------------------------------------------------------------------------
	g_SensorCheck.port = MXC_GPIO2;
	g_SensorCheck.mask = MXC_GPIO_PIN_10;
	g_SensorCheck.pad = MXC_GPIO_PAD_NONE; // Consider weak pulldown?
	g_SensorCheck.func = MXC_GPIO_FUNC_OUT;
	g_SensorCheck.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&g_SensorCheck);
	MXC_GPIO_OutClr(g_SensorCheck.port, g_SensorCheck.mask); // Start as disabled


	//----------------------------------------------------------------------------------------------------------------------
	// LTE Reset: Port 2, Pin 13, Input/Output???, External pull up, Active low, 3.3V
	//----------------------------------------------------------------------------------------------------------------------
	g_LTEReset.port = MXC_GPIO2;
	g_LTEReset.mask = MXC_GPIO_PIN_13;
	g_LTEReset.pad = MXC_GPIO_PAD_NONE;
	g_LTEReset.func = MXC_GPIO_FUNC_OUT; //MXC_GPIO_FUNC_IN;
	g_LTEReset.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_LTEReset);
	MXC_GPIO_OutSet(g_LTEReset.port, g_LTEReset.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// BLE Reset: Port 2, 15, Input/Output???, External pull up, Active low, 3.3V
	//----------------------------------------------------------------------------------------------------------------------
	g_BLEReset.port = MXC_GPIO2;
	g_BLEReset.mask = MXC_GPIO_PIN_15;
	g_BLEReset.pad = MXC_GPIO_PAD_NONE;
	g_BLEReset.func = MXC_GPIO_FUNC_OUT; //MXC_GPIO_FUNC_IN;
	g_BLEReset.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_BLEReset);
	MXC_GPIO_OutSet(g_BLEReset.port, g_BLEReset.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Smart Sensor Mux A0: Port 2, Pin 23, Output, External pulldown, Select, 3.3V (minimum 2.0V)
	//----------------------------------------------------------------------------------------------------------------------
	g_SmartSensorMux_A0.port = MXC_GPIO2;
	g_SmartSensorMux_A0.mask = MXC_GPIO_PIN_23;
	g_SmartSensorMux_A0.pad = MXC_GPIO_PAD_NONE;
	g_SmartSensorMux_A0.func = MXC_GPIO_FUNC_OUT;
	g_SmartSensorMux_A0.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_SmartSensorMux_A0);
	MXC_GPIO_OutClr(g_SmartSensorMux_A0.port, g_SmartSensorMux_A0.mask); // Start as low (Smart Sensor Geo1 select)

	//----------------------------------------------------------------------------------------------------------------------
	// Smart Sensor Mux A1: Port 2, Pin 25, Output, External pulldown, Select, 3.3V (minimum 2.0V)
	//----------------------------------------------------------------------------------------------------------------------
	g_SmartSensorMux_A1.port = MXC_GPIO2;
	g_SmartSensorMux_A1.mask = MXC_GPIO_PIN_25;
	g_SmartSensorMux_A1.pad = MXC_GPIO_PAD_NONE;
	g_SmartSensorMux_A1.func = MXC_GPIO_FUNC_OUT;
	g_SmartSensorMux_A1.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_SmartSensorMux_A1);
	MXC_GPIO_OutClr(g_SmartSensorMux_A1.port, g_SmartSensorMux_A1.mask); // Start as low (Smart Sensor Geo1 select)

	//----------------------------------------------------------------------------------------------------------------------
	// Nyquist 0/A0:Port 2, Pin 26, Output, External pulldown, Select, 3.3V (minimum 2.4V)
	//----------------------------------------------------------------------------------------------------------------------
	g_Nyquist0_A0.port = MXC_GPIO2;
	g_Nyquist0_A0.mask = MXC_GPIO_PIN_26;
	g_Nyquist0_A0.pad = MXC_GPIO_PAD_NONE;
	g_Nyquist0_A0.func = MXC_GPIO_FUNC_OUT;
	g_Nyquist0_A0.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_Nyquist0_A0);
	MXC_GPIO_OutClr(g_Nyquist0_A0.port, g_Nyquist0_A0.mask); // Start as low

	//----------------------------------------------------------------------------------------------------------------------
	// Nyquist 1/A1: Port 2, Pin 28, Output, External pulldown, Select, 3.3V (minimum 2.4V))
	//----------------------------------------------------------------------------------------------------------------------
	g_Nyquist1_A1.port = MXC_GPIO2;
	g_Nyquist1_A1.mask = MXC_GPIO_PIN_28;
	g_Nyquist1_A1.pad = MXC_GPIO_PAD_NONE;
	g_Nyquist1_A1.func = MXC_GPIO_FUNC_OUT;
	g_Nyquist1_A1.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_Nyquist1_A1);
	MXC_GPIO_OutClr(g_Nyquist1_A1.port, g_Nyquist1_A1.mask); // Start as low

	//----------------------------------------------------------------------------------------------------------------------
	// Nyquist 2/Enable: Port 2, Pin 30, Output, External pulldown, Active low, 3.3V (minimum 2.4V))
	//----------------------------------------------------------------------------------------------------------------------
	g_Nyquist2_Enable.port = MXC_GPIO2;
	g_Nyquist2_Enable.mask = MXC_GPIO_PIN_30;
	g_Nyquist2_Enable.pad = MXC_GPIO_PAD_NONE;
	g_Nyquist2_Enable.func = MXC_GPIO_FUNC_OUT;
	g_Nyquist2_Enable.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_Nyquist2_Enable);
	MXC_GPIO_OutSet(g_Nyquist2_Enable.port, g_Nyquist2_Enable.mask); // Start as disabled (960Hz select)

	//----------------------------------------------------------------------------------------------------------------------
	// Cellular Enable: Port 3, Pin 0, Output, No external pull, Active high, 3.3V (minimum 0.5)
	//----------------------------------------------------------------------------------------------------------------------
	g_CellEnable.port = MXC_GPIO0;
	g_CellEnable.mask = MXC_GPIO_PIN_8;
	g_CellEnable.pad = MXC_GPIO_PAD_NONE;
	g_CellEnable.func = MXC_GPIO_FUNC_OUT;
	g_CellEnable.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_CellEnable);
	MXC_GPIO_OutClr(g_CellEnable.port, g_CellEnable.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Sensor Enable 1(Geo1): Port 3, Pin 1, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	g_SensorEnable1_Geo1.port = MXC_GPIO3;
	g_SensorEnable1_Geo1.mask = MXC_GPIO_PIN_1;
	g_SensorEnable1_Geo1.pad = MXC_GPIO_PAD_NONE;
	g_SensorEnable1_Geo1.func = MXC_GPIO_FUNC_OUT;
	g_SensorEnable1_Geo1.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&g_SensorEnable1_Geo1);
	MXC_GPIO_OutClr(g_SensorEnable1_Geo1.port, g_SensorEnable1_Geo1.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Sensor Enable 2(Aop1): Port 3, Pin 2, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	g_SensorEnable2_Aop1.port = MXC_GPIO3;
	g_SensorEnable2_Aop1.mask = MXC_GPIO_PIN_2;
	g_SensorEnable2_Aop1.pad = MXC_GPIO_PAD_NONE;
	g_SensorEnable2_Aop1.func = MXC_GPIO_FUNC_OUT;
	g_SensorEnable2_Aop1.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&g_SensorEnable2_Aop1);
	MXC_GPIO_OutClr(g_SensorEnable2_Aop1.port, g_SensorEnable2_Aop1.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Sensor Enable 3(Geo2): Port 3, Pin 3, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	g_SensorEnable3_Geo2.port = MXC_GPIO3;
	g_SensorEnable3_Geo2.mask = MXC_GPIO_PIN_3;
	g_SensorEnable3_Geo2.pad = MXC_GPIO_PAD_NONE;
	g_SensorEnable3_Geo2.func = MXC_GPIO_FUNC_OUT;
	g_SensorEnable3_Geo2.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&g_SensorEnable3_Geo2);
	MXC_GPIO_OutClr(g_SensorEnable3_Geo2.port, g_SensorEnable3_Geo2.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Sensor Enable 4(Aop2): Port 3, Pin 4, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	g_SensorEnable4_Aop2.port = MXC_GPIO3;
	g_SensorEnable4_Aop2.mask = MXC_GPIO_PIN_4;
	g_SensorEnable4_Aop2.pad = MXC_GPIO_PAD_NONE;
	g_SensorEnable4_Aop2.func = MXC_GPIO_FUNC_OUT;
	g_SensorEnable4_Aop2.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&g_SensorEnable4_Aop2);
	MXC_GPIO_OutClr(g_SensorEnable4_Aop2.port, g_SensorEnable4_Aop2.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Gain/Path Select 1(Geo1): Port 3, Pin 5, Output, External pulldown, Select, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	g_GainPathSelect1_Geo1.port = MXC_GPIO3;
	g_GainPathSelect1_Geo1.mask = MXC_GPIO_PIN_5;
	g_GainPathSelect1_Geo1.pad = MXC_GPIO_PAD_NONE;
	g_GainPathSelect1_Geo1.func = MXC_GPIO_FUNC_OUT;
	g_GainPathSelect1_Geo1.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&g_GainPathSelect1_Geo1);
	MXC_GPIO_OutSet(g_GainPathSelect1_Geo1.port, g_GainPathSelect1_Geo1.mask); // Start as high (Normal gain)

	//----------------------------------------------------------------------------------------------------------------------
	// Gain/Path Select 2(Aop1): Port 3, Pin 6, Output, External pulldown, Select, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	g_GainPathSelect2_Aop1.port = MXC_GPIO3;
	g_GainPathSelect2_Aop1.mask = MXC_GPIO_PIN_6;
	g_GainPathSelect2_Aop1.pad = MXC_GPIO_PAD_NONE;
	g_GainPathSelect2_Aop1.func = MXC_GPIO_FUNC_OUT;
	g_GainPathSelect2_Aop1.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&g_GainPathSelect2_Aop1);
	MXC_GPIO_OutSet(g_GainPathSelect2_Aop1.port, g_GainPathSelect2_Aop1.mask); // Start as high (AOP path)

	//----------------------------------------------------------------------------------------------------------------------
	// Gain/Path Select 3(Geo2): Port 3, Pin 7,Output, External pulldown, Select, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	g_GainPathSelect3_Geo2.port = MXC_GPIO3;
	g_GainPathSelect3_Geo2.mask = MXC_GPIO_PIN_7;
	g_GainPathSelect3_Geo2.pad = MXC_GPIO_PAD_NONE;
	g_GainPathSelect3_Geo2.func = MXC_GPIO_FUNC_OUT;
	g_GainPathSelect3_Geo2.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&g_GainPathSelect3_Geo2);
	MXC_GPIO_OutSet(g_GainPathSelect3_Geo2.port, g_GainPathSelect3_Geo2.mask); // Start as high (Normal gain)

	//----------------------------------------------------------------------------------------------------------------------
	// Gain/Path Select 4(Aop2): Port 3, Pin 8, Output, External pulldown, Select, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	g_GainPathSelect4_Aop2.port = MXC_GPIO3;
	g_GainPathSelect4_Aop2.mask = MXC_GPIO_PIN_8;
	g_GainPathSelect4_Aop2.pad = MXC_GPIO_PAD_NONE;
	g_GainPathSelect4_Aop2.func = MXC_GPIO_FUNC_OUT;
	g_GainPathSelect4_Aop2.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&g_GainPathSelect4_Aop2);
	MXC_GPIO_OutSet(g_GainPathSelect4_Aop2.port, g_GainPathSelect4_Aop2.mask); // Start as high (AOP path)

	//----------------------------------------------------------------------------------------------------------------------
	// RTC Clock: Port 3, Pin 9, Input, No external pull, Active high, 3.3V (minimum 2.64V)
	//----------------------------------------------------------------------------------------------------------------------
	g_RTCClock.port = MXC_GPIO3;
	g_RTCClock.mask = MXC_GPIO_PIN_9;
	g_RTCClock.pad = MXC_GPIO_PAD_NONE; // Consider a weak internal pull?
	g_RTCClock.func = MXC_GPIO_FUNC_IN;
	g_RTCClock.vssel = MXC_GPIO_VSSEL_VDDIOH;
	MXC_GPIO_Config(&g_RTCClock);
#if 0 /* Generic */
	MXC_GPIO_RegisterCallback(&g_RTCClock, RTCClock_ISR, NULL);
#else /* Hooked into sample ISR */
	MXC_GPIO_RegisterCallback(&g_RTCClock, (mxc_gpio_callback_fn)Sample_irq, NULL);
#endif
    MXC_GPIO_IntConfig(&g_RTCClock, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_RTCClock.port, g_RTCClock.mask);

#if 1 /* Special addition (ADC Dual-SDO and Max32651 Dual mode not compatible), re-configure SPI3_SDIO2 (P0.17) from SPI line to ADC GPIO */
	//----------------------------------------------------------------------------------------------------------------------
	// External ADC Busy, Alt, GP0: Port 0, Pin 17, Input, No external pullup, Active high, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_AdcBusyAltGP0.port = MXC_GPIO0;
	g_AdcBusyAltGP0.mask = MXC_GPIO_PIN_17;
	g_AdcBusyAltGP0.pad = MXC_GPIO_PAD_WEAK_PULL_DOWN; // ADC GP0 line inits as input, set pull down until ADC GP0 configured as output
	g_AdcBusyAltGP0.func = MXC_GPIO_FUNC_IN;
	g_AdcBusyAltGP0.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_Config(&g_AdcBusyAltGP0);
#endif

	//----------------------------------------------------------------------------------------------------------------------
	// Enable IRQ's for any of the appropritate GPIO input interrupts
	//----------------------------------------------------------------------------------------------------------------------
	NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(MXC_GPIO0)));
	NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(MXC_GPIO1)));
	NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(MXC_GPIO2)));
	NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(MXC_GPIO3)));
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
    MXC_I2C_SetFrequency(MXC_I2C0, MXC_I2C_FAST_SPEED);
    MXC_I2C_SetFrequency(MXC_I2C1, MXC_I2C_FAST_SPEED);
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

#define SPI_SPEED_ADC 10000000 // Bit Rate
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
	status = MXC_SPI_Init(MXC_SPI3, YES, NO, 1, LOW, SPI_SPEED_ADC);
	if (status != E_SUCCESS) { debugErr("SPI3 (ADC) Init failed with code: %d\r\n", status); }

	mxc_gpio_cfg_t spi3SlaveSelect0GpioConfig = { MXC_GPIO0, (MXC_GPIO_PIN_19), MXC_GPIO_FUNC_ALT1, MXC_GPIO_PAD_NONE };
	MXC_GPIO_Config(&spi3SlaveSelect0GpioConfig); // Seems the SPI framework driver does not set the Slave Select 0 alternate function on the GPIO pin

	// Set standard SPI 4-wire (MISO/MOSI, full duplex), (Turns out ADC dual-SDO and MAX32651 dual mode are incompatible, can only use single mode)
	MXC_SPI_SetWidth(MXC_SPI3, SPI_WIDTH_STANDARD);

	// External SPI3 uses SPI Mode 3 only
	MXC_SPI_SetMode(MXC_SPI3, SPI_MODE_3);

	//-----------
	// SPI2 - LCD
	//-----------
	status = MXC_SPI_Init(MXC_SPI2, YES, NO, 1, LOW, SPI_SPEED_LCD);
	if (status != E_SUCCESS) { debugErr("SPI2 (LCD) Init failed with code: %d\r\n", status); }

	if (FT81X_SPI_2_SS_CONTROL_MANUAL)
	{
		mxc_gpio_cfg_t spi2SlaveSelect0GpioConfig = { MXC_GPIO2, (MXC_GPIO_PIN_5), MXC_GPIO_FUNC_OUT, MXC_GPIO_PAD_NONE };
		MXC_GPIO_Config(&spi2SlaveSelect0GpioConfig); // Seems the SPI framework driver does not set the Slave Select 0 alternate function on the GPIO pin
	}
	else // SPI2 Slave Select controlled by the SPI driver
	{
		mxc_gpio_cfg_t spi2SlaveSelect0GpioConfig = { MXC_GPIO2, (MXC_GPIO_PIN_5), MXC_GPIO_FUNC_ALT1, MXC_GPIO_PAD_NONE };
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
static int eventCallback(maxusb_event_t evt, void *data);
static void usbAppSleep(void);
static void usbAppWakeup(void);
static int usbReadCallback(void);
int usbStartupCallback();
int usbShutdownCallback();
static void echoUSB(void);

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

static volatile int usb_read_complete;

// Functions to control "disk" memory. See msc.h for definitions
static const msc_mem_t mem = { mscmem_Init, mscmem_Start, mscmem_Stop, mscmem_Ready,
                               mscmem_Size, mscmem_Read,  mscmem_Write };

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
    MXC_USB_EventEnable(MAXUSB_EVENT_NOVBUS, eventCallback, NULL);
    MXC_USB_EventEnable(MAXUSB_EVENT_VBUS, eventCallback, NULL);
    acm_register_callback(ACM_CB_READ_READY, usbReadCallback);
    usb_read_complete = 0;

    // Start with USB in low power mode
    usbAppSleep();
    NVIC_EnableIRQ(USB_IRQn);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static void echoUSB(void)
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
    if (sud->wValue == composite_config_descriptor.config_descriptor.bConfigurationValue) {
        //      on++;
        configured = 1;
        MXC_SETBIT(&event_flags, EVENT_ENUM_COMP);
        if (MXC_USB_GetStatus() & MAXUSB_STATUS_HIGH_SPEED) { ///
            msc_cfg.out_ep = composite_config_descriptor_hs.endpoint_descriptor_1.bEndpointAddress &
                             0x7;
            msc_cfg.out_maxpacket =
                composite_config_descriptor_hs.endpoint_descriptor_1.wMaxPacketSize;
            msc_cfg.in_ep = composite_config_descriptor_hs.endpoint_descriptor_2.bEndpointAddress &
                            0x7;
            msc_cfg.in_maxpacket =
                composite_config_descriptor_hs.endpoint_descriptor_2.wMaxPacketSize;
        } else {
            msc_cfg.out_ep = composite_config_descriptor.endpoint_descriptor_1.bEndpointAddress &
                             0x7;
            msc_cfg.out_maxpacket =
                composite_config_descriptor.endpoint_descriptor_1.wMaxPacketSize;
            msc_cfg.in_ep = composite_config_descriptor.endpoint_descriptor_2.bEndpointAddress &
                            0x7;
            msc_cfg.in_maxpacket = composite_config_descriptor.endpoint_descriptor_2.wMaxPacketSize;
        }

        acm_cfg.out_ep = composite_config_descriptor.endpoint_descriptor_4.bEndpointAddress & 0x7;
        acm_cfg.out_maxpacket = composite_config_descriptor.endpoint_descriptor_4.wMaxPacketSize;
        acm_cfg.in_ep = composite_config_descriptor.endpoint_descriptor_5.bEndpointAddress & 0x7;
        acm_cfg.in_maxpacket = composite_config_descriptor.endpoint_descriptor_5.wMaxPacketSize;
        acm_cfg.notify_ep = composite_config_descriptor.endpoint_descriptor_3.bEndpointAddress &
                            0x7;
        acm_cfg.notify_maxpacket = composite_config_descriptor.endpoint_descriptor_3.wMaxPacketSize;

        msc_configure(&msc_cfg);
        return acm_configure(&acm_cfg);
        /* Configure the device class */
    } else if (sud->wValue == 0) {
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
    /* TODO: Place low-power code here */
    suspended = 1;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static void usbAppWakeup(void)
{
    /* TODO: Place low-power code here */
    suspended = 0;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static int eventCallback(maxusb_event_t evt, void *data)
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
        MXC_USB_EventEnable(MAXUSB_EVENT_BRST, eventCallback, NULL);
        MXC_USB_EventClear(MAXUSB_EVENT_BRSTDN); ///
        MXC_USB_EventEnable(MAXUSB_EVENT_BRSTDN, eventCallback, NULL); ///
        MXC_USB_EventClear(MAXUSB_EVENT_SUSP);
        MXC_USB_EventEnable(MAXUSB_EVENT_SUSP, eventCallback, NULL);
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
    case MAXUSB_EVENT_BRSTDN: ///
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
    usb_read_complete = 1;
    return 0;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UsbWaitForEvents(void)
{
	/* Wait for events */
    while (1) {
        echoUSB();

        if (suspended || !configured) {
            // Suspended or not configured, alert debug
        } else {
            // Configured, alert debug
        }

        if (event_flags) {
            /* Display events */
            if (MXC_GETBIT(&event_flags, MAXUSB_EVENT_NOVBUS)) {
                MXC_CLRBIT(&event_flags, MAXUSB_EVENT_NOVBUS);
                debug("VBUS Disconnect\n");
            } else if (MXC_GETBIT(&event_flags, MAXUSB_EVENT_VBUS)) {
                MXC_CLRBIT(&event_flags, MAXUSB_EVENT_VBUS);
                debug("VBUS Connect\n");
            } else if (MXC_GETBIT(&event_flags, MAXUSB_EVENT_BRST)) {
                MXC_CLRBIT(&event_flags, MAXUSB_EVENT_BRST);
                debug("Bus Reset\n");
            } else if (MXC_GETBIT(&event_flags, MAXUSB_EVENT_BRSTDN)) { ///
                MXC_CLRBIT(&event_flags, MAXUSB_EVENT_BRSTDN);
                debug("Bus Reset Done: %s speed\n",
                       (MXC_USB_GetStatus() & MAXUSB_STATUS_HIGH_SPEED) ? "High" : "Full");
            } else if (MXC_GETBIT(&event_flags, MAXUSB_EVENT_SUSP)) {
                MXC_CLRBIT(&event_flags, MAXUSB_EVENT_SUSP);
                debug("Suspended\n");
            } else if (MXC_GETBIT(&event_flags, MAXUSB_EVENT_DPACT)) {
                MXC_CLRBIT(&event_flags, MAXUSB_EVENT_DPACT);
                debug("Resume\n");
            } else if (MXC_GETBIT(&event_flags, EVENT_ENUM_COMP)) {
                MXC_CLRBIT(&event_flags, EVENT_ENUM_COMP);
                debug("Enumeration complete...\n");
            } else if (MXC_GETBIT(&event_flags, EVENT_REMOTE_WAKE)) {
                MXC_CLRBIT(&event_flags, EVENT_REMOTE_WAKE);
                debug("Remote Wakeup\n");
            }
        }
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

    if ((err = f_stat((const TCHAR *)filename, &fno)) == FR_NO_FILE) {
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

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupSDHCeMMC(void)
{
    mxc_sdhc_cfg_t cfg;

    // Initialize SDHC peripheral
    cfg.bus_voltage = MXC_SDHC_Bus_Voltage_1_8;
    cfg.block_gap = 0;
    cfg.clk_div = 0x0b0; // Maximum divide ratio, frequency must be >= 400 kHz during Card Identification phase

    if (MXC_SDHC_Init(&cfg) != E_NO_ERROR) { debugErr("SDHC/eMMC initialization failed\r\n"); }

    // Set up card to get it ready for a transaction
    if (MXC_SDHC_Lib_InitCard(10) == E_NO_ERROR) { debug("SDHC: Card/device Initialized\r\n"); }
	else { debugErr("SDHC: No card/device response\n"); }

    if (MXC_SDHC_Lib_Get_Card_Type() == CARD_MMC) { debug("SDHC: Card type discovered is MMC/eMMC\r\n"); }
	else /* CARD_SDHC */ { debug("SDHC: Card type discovered is SD/SDHC\r\n"); }

    // Configure for fastest possible clock, must not exceed 52 MHz for eMMC
    if (SystemCoreClock > 96000000)
	{
        debug("SD clock ratio (at card/device) is 4:1 (eMMC not to exceed 52 MHz)\r\n");
        MXC_SDHC_Set_Clock_Config(1);
    }
	else
	{
        debug("SD clock ratio (at card/device) is 2:1\r\n");
        MXC_SDHC_Set_Clock_Config(0);
    }
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
void SetupHalfSecondTickTimer(void)
{
#if 0 /* Internal PIT Timer based, will not generate interrupts in Deepsleep or Backup */
	MXC_SYS_Reset_Periph(MXC_SYS_RESET_TIMER0);
	MXC_SYS_ClockEnable(MXC_SYS_PERIPH_CLOCK_TIMER0);

    // Clear interrupt flag
    MXC_TMR0->intr = MXC_F_TMR_INTR_IRQ;

    // Set the prescaler (TMR_PRES_4096)
	MXC_TMR0->cn |= (MXC_F_TMR_CN_PRES3);
	MXC_TMR0->cn |= (MXC_V_TMR_CN_PRES_DIV4096);

    // Set the mode
	MXC_TMR0->cn |= TMR_MODE_CONTINUOUS << MXC_F_TMR_CN_TMODE_POS;

	// Set the polarity
    MXC_TMR0->cn |= (0) << MXC_F_TMR_CN_TPOL_POS; // Polarity (0 or 1) doesn't matter

	// Init the compare value
    MXC_TMR0->cmp = 7324; // 60MHz clock / 4096 = 14648 counts/sec, 1/2 second count = 7324

	// Init the counter
    MXC_TMR0->cnt = 0x1;

	// Setup the Timer 0 interrupt
	NVIC_ClearPendingIRQ(TMR0_IRQn);
    NVIC_DisableIRQ(TMR0_IRQn);
    MXC_NVIC_SetVector(TMR0_IRQn, Soft_timer_tick_irq);
    NVIC_EnableIRQ(TMR0_IRQn);

	// Enable the timer
	MXC_TMR0->cn |= MXC_F_TMR_CN_TEN;
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

	if (powerOnButtonDetect) { debug("Power On button pressed\r\n"); }
	if (vbusChargingDetect) { debug("USB Charging detected\r\n"); }

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
				while (1) {}
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
		// Todo: Determine necessary action if USB charging is reason for power up, Aux power enable?

		// Todo: Turn on appropriate LED
		//PowerControl(LED???, ON);
	}
	else
	{
		debugWarn("MCU Power latch is power on source\r\n");
	}
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
	SetupGPIO();

	//-------------------------------------------------------------------------
	// Setup USB Composite (MSC + CDC/ACM)
	//-------------------------------------------------------------------------
	SetupUSBComposite();

	//-------------------------------------------------------------------------
	// Setup HDSC/eMMC
	//-------------------------------------------------------------------------
	SetupSDHCeMMC();

	//-------------------------------------------------------------------------
	// Setup Drive(eMMC) and Filesystem
	//-------------------------------------------------------------------------
	SetupDriveAndFilesystem();

	//-------------------------------------------------------------------------
	// Setup Half Second tick timer
	//-------------------------------------------------------------------------
	SetupHalfSecondTickTimer();

	//-------------------------------------------------------------------------
	// Disable all interrupts and clear all interrupt vectors 
	//-------------------------------------------------------------------------
	// Todo: Determine if this is necessary
#if 0 /* old hw */
	Disable_global_interrupt();
	INTC_init_interrupts();
#endif

	//-------------------------------------------------------------------------
	// Set Alarm 1 and Alarm 2 low (Active high control)
	//-------------------------------------------------------------------------
	PowerControl(ALARM_1_ENABLE, OFF); // Default, technically done with SetupGPIO call
	PowerControl(ALARM_2_ENABLE, OFF); // Default, technically done with SetupGPIO call

	//-------------------------------------------------------------------------
	// Set Trigger Out low (Active high control)
	//-------------------------------------------------------------------------
	PowerControl(TRIGGER_OUT, OFF); // Technically done with SetupGPIO call
	MXC_GPIO_ClearFlags(MXC_GPIO1, MXC_GPIO_PIN_31); // Clear External Trigger In interrupt flag (Port 1, Pin 31)

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

	//-------------------------------------------------------------------------
	// Initialize the AD Control
	//-------------------------------------------------------------------------
	AnalogControlInit(); debug("Analog Control: Init complete\r\n");

	//-------------------------------------------------------------------------
	// Init and configure the A/D to prevent the unit from burning current charging internal reference (default config)
	//-------------------------------------------------------------------------
	InitExternalAD(); debug("External ADC: Init complete\r\n");

	//-------------------------------------------------------------------------
	// Init the LCD display
	//-------------------------------------------------------------------------
	InitLCD(); debug("LCD Display: Init complete\r\n");

	//-------------------------------------------------------------------------
	// Init Keypad
	//-------------------------------------------------------------------------
	InitExternalKeypad(); debug("Keyboard: Init complete\r\n");

	//-------------------------------------------------------------------------
	// Disable unused subsystems to save power
	//-------------------------------------------------------------------------
	AdjustPowerSavings(POWER_SAVINGS_NORMAL); debug("Power Savings: Init complete\r\n");

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
