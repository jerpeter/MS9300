///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include "board.h"
//#include "pm.h"
#include "gpio.h"
//#include "sdramc.h"
//#include "intc.h"
//#include "usart.h"
//#include "print_funcs.h"
#include "lcd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Typedefs.h"
#include "Common.h"
#include "Display.h"
#include "Menu.h"
#include "OldUart.h"
#include "spi.h"
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
void TestSnippetsBeforeInit(void)
{
	#if 0 /* Test (Enable serial and put processor in deep stop) */
	// Setup debug serial port
	usart_options_t usart_1_rs232_options =
	{
		.baudrate = 115200,
		.charlength = 8,
		.paritytype = USART_NO_PARITY,
		.stopbits = USART_1_STOPBIT,
		.channelmode = USART_NORMAL_CHMODE
	};

	// Initialize it in RS232 mode.
	usart_init_rs232(&AVR32_USART1, &usart_1_rs232_options, FOSC0);

	// Init signals for ready to send and terminal ready
	SET_RTS; SET_DTR;

	debug("--- System Deep Stop ---\n");

	SLEEP(AVR32_PM_SMODE_DEEP_STOP);

	while (1) {;}
	#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestSnippetsAfterInit(void)
{

	#if 0
	CMD_BUFFER_STRUCT inCmd;
	inCmd.msg[MESSAGE_HEADER_SIMPLE_LENGTH] = 0;
	HandleVML(&inCmd);
	#endif

	#if 0 /* Craft Test */
	Menu_Items = MAIN_MENU_FUNCTIONS_ITEMS;
	Menu_Functions = (unsigned long *)Main_Menu_Functions;
	Menu_String = (unsigned char *)&Main_Menu_Text;
	#endif

	#if 0 /* Clear Internal RAM static variables */
	uint32 i = 0x28;
	while (i)
	{
		*((uint8*)i--) = 0;
	}
	#endif

	#if 0 /* Test (Effective CS Low for SDMMC) */
	while (1)
	{
		debug("SPI1 SDMMC CS Active (0)\n");
		spi_selectChip(&AVR32_SPI1, 2);
		spi_write(&AVR32_SPI1, 0x0000);
		SoftUsecWait(5 * SOFT_SECS);


		debug("SPI1 SDMMC CS Inactive (1)\n");
		spi_unselectChip(&AVR32_SPI1, 2);
		SoftUsecWait(5 * SOFT_SECS);
	}
	#endif

	#if 0 /* Test (SDMMC Reset to enable standby power) */
	uint8 r1;
	uint32 retry = 0;
	uint32 timedAccess = 0;
	FL_FILE* monitorLogFile;
	MONITOR_LOG_ENTRY_STRUCT monitorLogEntry;
	int32 bytesRead = 0;

	while (1)
	{
		//---Idle low power-------------------------------------------------------------------
		spi_selectChip(&AVR32_SPI1, SD_MMC_SPI_NPCS);
		retry = 0;
		do
		{
			r1 = sd_mmc_spi_send_command(MMC_GO_IDLE_STATE, 0);
			spi_write(SD_MMC_SPI,0xFF);
			retry++;
		}
		while(r1 != 0x01);
		spi_unselectChip(&AVR32_SPI1, SD_MMC_SPI_NPCS);
		debug("SDMMC Cycles to enter idle state: %d\n", retry);
		
		SoftUsecWait(5 * SOFT_SECS);
		//----------------------------------------------------------------------
		
		//---Init and idle-------------------------------------------------------------------
		debug("SDMMC Init and idle...\n", retry);
		spi_selectChip(&AVR32_SPI1, SD_MMC_SPI_NPCS);
		sd_mmc_spi_internal_init();
		spi_unselectChip(&AVR32_SPI1, SD_MMC_SPI_NPCS);

		SoftUsecWait(5 * SOFT_SECS);
		//----------------------------------------------------------------------
		
		//---Idle low power-------------------------------------------------------------------
		spi_selectChip(&AVR32_SPI1, SD_MMC_SPI_NPCS);
		retry = 0;
		do
		{
			r1 = sd_mmc_spi_send_command(MMC_GO_IDLE_STATE, 0);
			spi_write(SD_MMC_SPI,0xFF);
			retry++;
		}
		while(r1 != 0x01);
		spi_unselectChip(&AVR32_SPI1, SD_MMC_SPI_NPCS);
		debug("SDMMC Cycles to enter idle state: %d\n", retry);

		SoftUsecWait(5 * SOFT_SECS);
		//----------------------------------------------------------------------

		//---Init and FAT32 Cycle-------------------------------------------------------------------
		debug("SDMMC Init and FAT32 Init cycle...\n", retry);
		spi_selectChip(&AVR32_SPI1, SD_MMC_SPI_NPCS);
		sd_mmc_spi_internal_init();
		spi_unselectChip(&AVR32_SPI1, SD_MMC_SPI_NPCS);

		timedAccess = g_lifetimeHalfSecondTickCount;
		
		while (g_lifetimeHalfSecondTickCount < (timedAccess + 10))
		{
			FAT32_InitDrive();
			if (FAT32_InitFAT() == FALSE)
			{
				debugErr("FAT32 Initialization failed!\n\r");
			}
		}
		//----------------------------------------------------------------------

		//---Idle low power-------------------------------------------------------------------
		spi_selectChip(&AVR32_SPI1, SD_MMC_SPI_NPCS);
		retry = 0;
		do
		{
			r1 = sd_mmc_spi_send_command(MMC_GO_IDLE_STATE, 0);
			spi_write(SD_MMC_SPI,0xFF);
			retry++;
		}
		while(r1 != 0x01);
		spi_unselectChip(&AVR32_SPI1, SD_MMC_SPI_NPCS);
		debug("SDMMC Cycles to enter idle state: %d\n", retry);

		SoftUsecWait(5 * SOFT_SECS);
		//----------------------------------------------------------------------

		//---Init and Read cycle-------------------------------------------------------------------
		debug("SDMMC Init and Read cycle...\n", retry);
		spi_selectChip(&AVR32_SPI1, SD_MMC_SPI_NPCS);
		sd_mmc_spi_internal_init();
		spi_unselectChip(&AVR32_SPI1, SD_MMC_SPI_NPCS);

		FAT32_InitDrive();
		if (FAT32_InitFAT() == FALSE)
		{
			debugErr("FAT32 Initialization failed!\n\r");
		}

		timedAccess = g_lifetimeHalfSecondTickCount;
		
		while (g_lifetimeHalfSecondTickCount < (timedAccess + 10))
		{
			monitorLogFile = fl_fopen("C:\\Logs\\TestLogRead.ns8", "r");
			if (monitorLogFile == NULL) { debugErr("Test Read file not found\n"); }
			bytesRead = fl_fread(monitorLogFile, (uint8*)&monitorLogEntry, sizeof(MONITOR_LOG_ENTRY_STRUCT));
			while (bytesRead > 0) { bytesRead = fl_fread(monitorLogFile, (uint8*)&monitorLogEntry, sizeof(MONITOR_LOG_ENTRY_STRUCT)); }
			fl_fclose(monitorLogFile);
		}
		//----------------------------------------------------------------------

		//---Idle low power-------------------------------------------------------------------
		spi_selectChip(&AVR32_SPI1, SD_MMC_SPI_NPCS);
		retry = 0;
		do
		{
			r1 = sd_mmc_spi_send_command(MMC_GO_IDLE_STATE, 0);
			spi_write(SD_MMC_SPI,0xFF);
			retry++;
		}
		while(r1 != 0x01);
		spi_unselectChip(&AVR32_SPI1, SD_MMC_SPI_NPCS);
		debug("SDMMC Cycles to enter idle state: %d\n", retry);

		SoftUsecWait(5 * SOFT_SECS);
		//----------------------------------------------------------------------

		//---Init and Write cycle-------------------------------------------------------------------
		debug("SDMMC Init and Write cycle...\n", retry);
		spi_selectChip(&AVR32_SPI1, SD_MMC_SPI_NPCS);
		sd_mmc_spi_internal_init();
		spi_unselectChip(&AVR32_SPI1, SD_MMC_SPI_NPCS);

		FAT32_InitDrive();
		if (FAT32_InitFAT() == FALSE)
		{
			debugErr("FAT32 Initialization failed!\n\r");
		}

		timedAccess = g_lifetimeHalfSecondTickCount;
		
		while (g_lifetimeHalfSecondTickCount < (timedAccess + 10))
		{
			monitorLogFile = fl_fopen("C:\\Logs\\TestLogWrite.ns8", "a+");
			if (monitorLogFile == NULL) { debugErr("Test Write file not opened\n"); }
			for (retry = 0; retry < 250; retry++)
			{
				fl_fwrite((uint8*)&(__monitorLogTbl[__monitorLogTblIndex]), sizeof(MONITOR_LOG_ENTRY_STRUCT), 1, monitorLogFile);
			}
			fl_fclose(monitorLogFile);
		}
		//----------------------------------------------------------------------
	}
	#endif

	#if 0 /* Test (Ineffective CS Low for SDMMC) */
	gpio_enable_gpio_pin(AVR32_PIN_PA14);
	gpio_enable_gpio_pin(AVR32_PIN_PA18);
	gpio_enable_gpio_pin(AVR32_PIN_PA19);
	gpio_enable_gpio_pin(AVR32_PIN_PA20);

	while (1)
	{
		debug("SPI1 CS's 0\n");
		//gpio_clr_gpio_pin(AVR32_PIN_PA14);
		//gpio_clr_gpio_pin(AVR32_PIN_PA18);
		gpio_clr_gpio_pin(AVR32_PIN_PA19);
		//gpio_clr_gpio_pin(AVR32_PIN_PA20);

		spi_selectChip(&AVR32_SPI1, 2);
		// Small delay before the RTC device is accessible
		SoftUsecWait(500);
		unsigned int i;
		for (i = 0; i < 10000; i++)
		spi_write(&AVR32_SPI1, 0x0000);
		spi_unselectChip(&AVR32_SPI1, 2);

		__monitorLogTblKey = 0;
		InitMonitorLog();

		SoftUsecWait(3 * SOFT_SECS);

		debug("SPI1 CS's 1\n");
		//gpio_set_gpio_pin(AVR32_PIN_PB14);
		//gpio_set_gpio_pin(AVR32_PIN_PB19);
		//gpio_set_gpio_pin(AVR32_PIN_PB20);

		SoftUsecWait(3 * SOFT_SECS);
	}

	#endif

	#if 0 /* Test (Timer mode) */
	EnableExternalRtcAlarm(0, 0, 0, 0);

	static RTC_MEM_MAP_STRUCT rtcMap;
	static uint8 clearAlarmFlag = 0x03; //0xEF; // Logic 0 on the bit will clear Alarm flag, bit 4
	static uint32 counter = 0;

	ExternalRtcRead(RTC_CONTROL_1_ADDR, 3, (uint8*)&rtcMap);
	
	if (rtcMap.control_2 & 0x10)
	{
		debug("RTC Alarm Flag indicates alarm condition raised. (0x%x, 0x%x, 0x%x)\n", rtcMap.control_1, rtcMap.control_2, rtcMap.control_3);
		
		ExternalRtcWrite(RTC_CONTROL_2_ADDR, 1, &clearAlarmFlag);
		debug("RTC Alarm Flag being cleared\n");

		ExternalRtcRead(RTC_CONTROL_1_ADDR, 3, (uint8*)&rtcMap);

		if (rtcMap.control_2 & 0x10)
		{
			debugWarn("RTC Alarm flag was not cleared successfully! (0x%x, 0x%x, 0x%x)\n", rtcMap.control_1, rtcMap.control_2, rtcMap.control_3);
		}
		else
		{
			debug("RTC Alarm Flag cleared. (0x%x, 0x%x, 0x%x)\n", rtcMap.control_1, rtcMap.control_2, rtcMap.control_3);
		}
	}
	else
	{
		ExternalRtcWrite(RTC_CONTROL_2_ADDR, 1, &clearAlarmFlag);
		debug("RTC Alarm Flag does not show an alarm condition. (0x%x, 0x%x, 0x%x)\n", rtcMap.control_1, rtcMap.control_2, rtcMap.control_3);
	}
	#endif

	#if 0 /* Test (LCD off and Proc stop) */
	debug("\n--- System Init Complete ---\n");
	SoftUsecWait(10 * SOFT_SECS);
	DisplayTimerCallBack();
	LcdPwTimerCallBack();
	SoftUsecWait(10 * SOFT_SECS);
	debug("--- System Deep Stop ---\n");

	SLEEP(AVR32_PM_SMODE_DEEP_STOP);

	while (1) {;}
	#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestSnippetsExecLoop(void)
{
	#if 0 /* Test (Timer mode) */
	if (counter)
	{
		counter--;
		
		if (counter == 0)
		{
			ExternalRtcWrite(RTC_CONTROL_2_ADDR, 1, &clearAlarmFlag);
			debug("RTC Alarm Flag being cleared\n");

			ExternalRtcRead(RTC_CONTROL_1_ADDR, 3, (uint8*)&rtcMap);

			if (rtcMap.control_2 & 0x10)
			{
				debugWarn("RTC Alarm flag was not cleared successfully! (0x%x, 0x%x, 0x%x)\n", rtcMap.control_1, rtcMap.control_2, rtcMap.control_3);
			}
			else
			{
				debug("RTC Alarm Flag cleared. (0x%x, 0x%x, 0x%x)\n", rtcMap.control_1, rtcMap.control_2, rtcMap.control_3);
			}
		}
	}
	else
	{
		ExternalRtcRead(RTC_CONTROL_1_ADDR, 3, (uint8*)&rtcMap);
		
		if (rtcMap.control_2 & 0x10)
		{
			debug("RTC Alarm Flag indicates alarm condition raised. (0x%x, 0x%x, 0x%x)\n", rtcMap.control_1, rtcMap.control_2, rtcMap.control_3);
			counter = 1000000;
		}
	}
	
	#endif

	#if 0 /* Test (Display temperature change readings) */
	static uint16 s_tempReading = 0;
	
	// 0x49 to 0x4c
	if (abs((int)(g_currentTempReading - s_tempReading)) > 4)
	{
		debug("Temp update on change, Old: 0x%x, New: 0x%x\n", s_tempReading, g_currentTempReading);
		
		s_tempReading = g_currentTempReading;
	}
	#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* Exception testing */
void garbage(void)
{
#if 1 /* ET test */
	uint16* intMem = (uint16*)0x0004;
	while (intMem < (uint16*)0x10000)
	{
		// Ignore the top of the stack
		if ((uint32)intMem < 0x3FF0 || (uint32)intMem >= 0x4000)
		{
			*intMem = 0xD673;
		}

		intMem++;
	}
#endif
#if 0 /* ET test */
	//for (g_execCycles = 0; g_execCycles < 100000; g_execCycles++) { *(uint16*)0x0004 = 0xD673; }
	intMem = (uint16*)0x0004;
	while (intMem < (uint16*)0x10000)
	{
		// Ignore the top of the stack
		if ((uint32)intMem != 0x3FFC && (uint32)intMem != 0x3FFE)
		{
			*intMem = 0xD673;
		}

		intMem++;
	}
#endif
#if 0 /* ET test */
	g_currentEventSamplePtr = (uint16*)0x0004;
	while (g_currentEventSamplePtr < (uint16*)0x10000)
	{
		// Ignore the top of the stack
		if ((uint32)g_currentEventSamplePtr != 0x3FFC && (uint32)g_currentEventSamplePtr != 0x3FFE)
		{
			*g_currentEventSamplePtr = 0xD673;
		}

		g_currentEventSamplePtr++;
	}
#endif
	TestIntMem("After IMEM Init");
	intMemProblem = NO;

	AVR32_PM.gplp[0] = 0x12345678;
	AVR32_PM.gplp[1] = 0x90ABCDEF;

	// Initialize the system
	InitSystemHardware_MS9300(); TestIntMem("After HW Init");
	InitInterrupts_MS9300(); TestIntMem("After Int Init");
	InitSoftwareSettings_MS9300(); TestIntMem("After SW Init");

	BootLoadManager(); TestIntMem("After BLM Init");
	DisplayVersionToCraft(); TestIntMem("After DVC Init");

	if (intMemProblem == YES)
	{
		sprintf((char*)g_spareBuffer, "%s (%lu)", (char*)spareBufferText, intMemCount);
		OverlayMessage((char*)spareBufferTitle, (char*)g_spareBuffer, (12 * SOFT_SECS));
	}
#if 0
	else
	{
		sprintf((char*)g_spareBuffer, "Addr intMem: %p", &intMem);
		OverlayMessage("STATUS", (char*)g_spareBuffer, (12 * SOFT_SECS));
	}
#endif

#if 0
	int i = 0;
	for (; i < 22; i++)
	{
		exceptionHandlerTable[i] = (uint32)&generic_exception;
	}

	// Load the Exception Vector Base Address in the corresponding system register.
	Set_system_register(AVR32_EVBA, (int)&exceptionHandlerTable);
#endif

#if 0
	// Import the Exception Vector Base Address.
	extern void _evba;

	Set_system_register(AVR32_EVBA, (int)&_evba);

	int i = 0;
	for (; i < 22; i++)
	{
		_register_exception_handler(&generic_exception, i);
	}

	// Enable exceptions.
	Enable_global_exception();
#endif

#if 1
	// Import the Exception Vector Base Address.
	extern void _evba;

	Set_system_register(AVR32_EVBA, (int)&_evba);

	// Enable exceptions.
	Enable_global_exception();
#endif
}
#endif

#if 0 /* Test (LAN register map read) */
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void testLan(void)
{
	debug("\n\r\n");
	*((uint16*)0xC800030A) = 0x0000; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0000, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0002; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0002, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0020; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0020, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0022; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0022, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0024; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0024, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0026; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0026, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0028; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0028, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x002A; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x002A, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x002C; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x002C, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0030; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0030, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0034; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0034, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0040; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0040, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0042; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0042, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0102; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0102, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0104; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0104, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0106; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0106, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0108; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0108, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x010A; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x010A, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0112; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0112, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0114; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0114, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0116; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0116, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0118; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0118, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0120; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0120, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0124; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0124, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0128; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0128, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x012C; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x012C, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0130; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0130, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0132; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0132, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0134; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0134, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0136; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0136, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0138; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0138, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x013C; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x013C, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0144; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0144, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0146; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0146, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0150; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0150, *((uint16*)0xC800030C));
	*((uint16*)0xC800030A) = 0x0158; debug("Lan Address (0x%04x) returns Data: 0x%x\r\n", 0x0158, *((uint16*)0xC800030C));
	debug("\r\n");
}
#endif
