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
#include <math.h>
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
#include "M23018.h"
//#include "sd_mmc_spi.h"
#include "adc.h"
#include "Sensor.h"
//#include "usb_task.h"
//#include "device_mass_storage_task.h"
//#include "usb_drv.h"
#include "srec.h"
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
void BootloaderEntryCheck(void)
{
	// Check if a char waiting
	if (UartCharWaiting(GLOBAL_DEBUG_PRINT_PORT))
	{
		// Check if a Ctrl-B was found in the UART receive holding register requesting to jump to boot
		if (UartGetc(GLOBAL_DEBUG_PRINT_PORT, UART_BLOCK) == CTRL_B)
		{
			g_quickBootEntryJump = QUICK_BOOT_ENTRY_FROM_SERIAL;
			BootLoadManager();
		}
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DisplayVersionToDebug(void)
{
	int majorVer, minorVer;
	char buildVer;
	sscanf(&g_buildVersion[0], "%d.%d.%c", &majorVer, &minorVer, &buildVer);

	debug("--- System Init complete (Version %d.%d.%c) ---\r\n", majorVer, minorVer, buildVer);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void LoadDefaultTriggerRecord(void)
{
	GetRecordData(&g_triggerRecord, DEFAULT_RECORD, REC_TRIGGER_USER_MENU_TYPE);

	// Check if the Default Trig Record is uninitialized
	if (g_triggerRecord.validRecord != YES)
	{
		debugWarn("Default Trigger Record: Invalid, loading defaults\r\n");
		LoadTrigRecordDefaults(&g_triggerRecord, WAVEFORM_MODE);
		SaveRecordData(&g_triggerRecord, DEFAULT_RECORD, REC_TRIGGER_USER_MENU_TYPE);
	}
	else
	{
		debug("Default Trigger record is valid\r\n");
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void LoadUnitConfig(void)
{
	GetRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

	// Check if the Unit Config is uninitialized
	if (g_unitConfig.validationKey != 0xA5A5)
	{
		// Set defaults in Unit Config
		debugWarn("Unit Config: Not found.\r\n");
#if EXTENDED_DEBUG
		debug("Loading Unit Config Defaults\r\n");
#endif

		LoadUnitConfigDefaults((UNIT_CONFIG_STRUCT*)&g_unitConfig);
		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);
	}
	else
	{
		// Unit Config is valid
#if EXTENDED_DEBUG
		debug("Unit Config record is valid\r\n");
#endif

		if ((g_unitConfig.pretrigBufferDivider != PRETRIGGER_BUFFER_QUARTER_SEC_DIV) && (g_unitConfig.pretrigBufferDivider != PRETRIGGER_BUFFER_HALF_SEC_DIV) &&
			(g_unitConfig.pretrigBufferDivider != PRETRIGGER_BUFFER_FULL_SEC_DIV))
		{
			g_unitConfig.pretrigBufferDivider = PRETRIGGER_BUFFER_QUARTER_SEC_DIV;
			SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);
		}

#if 1 /* Forcing flash wrapping to be disabled */
		g_unitConfig.flashWrapping = NO;
#endif

#if 0 /* Moved this init to the hardware section to allow for the saved Baud rate to be established from the start */
		// Set the baud rate to the user stored baud rate setting (initialized to 115200)
		switch (g_unitConfig.baudRate)
		{
			case BAUD_RATE_57600: rs232Options.baudrate = 57600; break;
			case BAUD_RATE_38400: rs232Options.baudrate = 38400; break;
			case BAUD_RATE_19200: rs232Options.baudrate = 19200; break;
			case BAUD_RATE_9600: rs232Options.baudrate = 9600; break;
		}

		if (g_unitConfig.baudRate != BAUD_RATE_115200)
		{
			// Re-Initialize the RS232 with the stored baud rate
			usart_init_rs232(&AVR32_USART1, &rs232Options, FOSC0);
		}
#endif
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void LoadModemSetupRecord(void)
{
	GetRecordData(&g_modemSetupRecord, 0, REC_MODEM_SETUP_TYPE);

	// Check if the Modem Setup Record is invalid
	if (g_modemSetupRecord.invalid)
	{
		// Warn the user
		debugWarn("Modem setup record not found.\r\n");

		// Initialize the Modem Setup Record
		LoadModemSetupRecordDefaults();

		// Save the Modem Setup Record
		SaveRecordData(&g_modemSetupRecord, DEFAULT_RECORD, REC_MODEM_SETUP_TYPE);
	}
	else
	{
#if EXTENDED_DEBUG
		debug("Modem Setup record is valid\r\n");
#endif
		ValidateModemSetupParameters();
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void LoadFactorySetupRecord(void)
{
	DATE_TIME_STRUCT tempTime;
	char buff[50];

	GetRecordData(&g_factorySetupRecord, DEFAULT_RECORD, REC_FACTORY_SETUP_TYPE);

	// Check if the Factory Setup Record is invalid
	if (g_factorySetupRecord.invalid)
	{
		// Warn the user
		debugWarn("Factory setup record not found.\r\n");
		OverlayMessage(getLangText(ERROR_TEXT), getLangText(FACTORY_SETUP_DATA_COULD_NOT_BE_FOUND_TEXT), (2 * SOFT_SECS));

		// Check if the Shadow Factory setup is valid
		GetFlashUserPageFactorySetup(&g_shadowFactorySetupRecord);
		if (!g_shadowFactorySetupRecord.invalid)
		{
			// Warn the user
			debugWarn("Factory setup shadow copy exists.\r\n");
			if (MessageBox(getLangText(CONFIRM_TEXT), getLangText(RESTORE_FACTORY_SETUP_FROM_BACKUP_Q_TEXT), MB_YESNO) == MB_FIRST_CHOICE)
			{
				GetFlashUserPageFactorySetup(&g_factorySetupRecord);
				SaveRecordData(&g_factorySetupRecord, DEFAULT_RECORD, REC_FACTORY_SETUP_TYPE);
			}
		}
	}

	// Check if the Factory Setup Record is valid (in case shadow factory setup was copied over)
	if (!g_factorySetupRecord.invalid)
	{
		UpdateUnitSensorsWithSmartSensorTypes();

		UpdateWorkingCalibrationDate();

		// Print the Factory Setup Record to the console
		memset(&buff[0], 0, sizeof(buff));
		ConvertCalDatetoDateTime(&tempTime, &g_currentCalibration.date);
		ConvertTimeStampToString(buff, &tempTime, REC_DATE_TYPE);

		if (g_factorySetupRecord.seismicSensorType > SENSOR_ACC_RANGE_DIVIDER) { strcpy((char*)&g_spareBuffer, "Acc"); }
		else { sprintf((char*)&g_spareBuffer, "%3.1f in", (double)((float)g_factorySetupRecord.seismicSensorType / (float)204.8)); }

		// Check if an older unit doesn't have the Analog Channel Config set
		if ((g_factorySetupRecord.analogChannelConfig != CHANNELS_R_AND_V_SCHEMATIC) && (g_factorySetupRecord.analogChannelConfig != CHANNELS_R_AND_V_SWAPPED))
		{
			// Set the default
			g_factorySetupRecord.analogChannelConfig = CHANNELS_R_AND_V_SWAPPED;
		}

		debug("Factory Setup: Serial #: %s\r\n", g_factorySetupRecord.unitSerialNumber);
		debug("Factory Setup: Cal Date: %s\r\n", buff);
		debug("Factory Setup: Sensor Type: %s\r\n", (char*)g_spareBuffer);
		debug("Factory Setup: A-Weighting: %s\r\n", (g_factorySetupRecord.aWeightOption == YES) ? "Enabled" : "Disabled");
		debug("Factory Setup: Analog Channel Config: %s\r\n", (g_factorySetupRecord.analogChannelConfig == CHANNELS_R_AND_V_SCHEMATIC) ? "Schematic" : "Swapped");
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitSensorParameters(uint16 seismicSensorType, uint8 sensitivity)
{
	uint8 gainFactor = (uint8)((sensitivity == LOW) ? 2 : 4);

	// Sensor type information
	g_sensorInfo.numOfChannels = NUMBER_OF_CHANNELS_DEFAULT;		// The number of channels from a sensor.
	g_sensorInfo.unitsFlag = g_unitConfig.unitsOfMeasure;			// 0 = SAE; 1 = Metric

	g_sensorInfo.sensorAccuracy = SENSOR_ACCURACY_100X_SHIFT;		// 100, sensor values are X 100 for accuaracy.
	g_sensorInfo.ADCResolution = ADC_RESOLUTION;				// Raw data Input Range, unless ADC is changed

	// Get the shift value
	g_sensorInfo.shiftVal = 1;

	if ((g_factorySetupRecord.seismicSensorType == SENSOR_ACC_832M1_0200) || (g_factorySetupRecord.seismicSensorType == SENSOR_ACC_832M1_0500))
	{
		g_sensorInfo.sensorTypeNormalized = (float)(seismicSensorType) * (float)ACC_832M1_SCALER /(float)(gainFactor * SENSOR_ACCURACY_100X_SHIFT);
	}
	else g_sensorInfo.sensorTypeNormalized = (float)(seismicSensorType)/(float)(gainFactor * SENSOR_ACCURACY_100X_SHIFT);

	if ((IMPERIAL_TYPE == g_unitConfig.unitsOfMeasure) || (seismicSensorType > SENSOR_ACC_RANGE_DIVIDER))
	{
		g_sensorInfo.measurementRatio = (float)IMPERIAL; 				// 1 = SAE; 25.4 = Metric
	}
	else
	{
		g_sensorInfo.measurementRatio = (float)METRIC; 				// 1 = SAE; 25.4 = Metric
	}

	// Get the sensor type in terms of the measurement units.
	g_sensorInfo.sensorTypeNormalized = (float)(g_sensorInfo.sensorTypeNormalized) * (float)(g_sensorInfo.measurementRatio);

	// the conversion is length(in or mm) = hexValue * (sensor scale/ADC Max Value)
	g_sensorInfo.hexToLengthConversion = (float)((float)ADC_RESOLUTION / (float)g_sensorInfo.sensorTypeNormalized);

	if ((g_factorySetupRecord.seismicSensorType == SENSOR_ACC_832M1_0200) || (g_factorySetupRecord.seismicSensorType == SENSOR_ACC_832M1_0500))
	{
		g_sensorInfo.sensorValue = (uint16)(g_factorySetupRecord.seismicSensorType * ACC_832M1_SCALER / gainFactor); // sensor value X 100.
	}
	else g_sensorInfo.sensorValue = (uint16)(g_factorySetupRecord.seismicSensorType / gainFactor); // sensor value X 100.
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
//#include "navigation.h"
void InitSoftwareSettings_MS9300(void)
{
	INPUT_MSG_STRUCT mn_msg;

	debug("Init Software Settings...\r\n");

	//-------------------------------------------------------------------------
	// Init version msg
	//-------------------------------------------------------------------------
	InitVersionMsg();

	//-------------------------------------------------------------------------
	// Init time msg and update current time
	//-------------------------------------------------------------------------
	InitTimeMsg();
	UpdateCurrentTime();

	//-------------------------------------------------------------------------
	// Get the function address passed by the bootloader
	//-------------------------------------------------------------------------
	CheckBootloaderAppPresent(); debug("Bootloader check complete\r\n");

	//-------------------------------------------------------------------------
	// Load the Unit Config
	//-------------------------------------------------------------------------
	LoadUnitConfig(); debug("Loading Unit Config record\r\n");

	//-------------------------------------------------------------------------
	// Build the language table based on the user's last language choice
	//-------------------------------------------------------------------------
	BuildLanguageLinkTable(g_unitConfig.languageMode); debug("Language Tables built\r\n");

	//-------------------------------------------------------------------------
	// Initialize Unit Config items such as contrast, timers
	//-------------------------------------------------------------------------
	ActivateUnitConfigOptions(); debug("Activated Unit Config Options\r\n");

	//-------------------------------------------------------------------------
	// Display the Splash screen
	//-------------------------------------------------------------------------
	DisplaySplashScreen(); debug("Display Splash screen complete\r\n");

	//-------------------------------------------------------------------------
	// Wait at least 3 seconds for the main screen to be displayed
	//-------------------------------------------------------------------------
	debug("Three second delay for Splash screen\r\n");
	SoftUsecWait(3 * SOFT_SECS);

	//-------------------------------------------------------------------------
	// Display Smart Sensor information if found
	//-------------------------------------------------------------------------
	DisplaySmartSensorInfo(INFO_ON_INIT);

	//-------------------------------------------------------------------------
	// Bootloader entry check
	//-------------------------------------------------------------------------
	BootloaderEntryCheck();

	//-------------------------------------------------------------------------
	// Load the Factory Setup Record
	//-------------------------------------------------------------------------
	LoadFactorySetupRecord(); debug("Factory setup record loaded\r\n");

	//-------------------------------------------------------------------------
	// Load Default Trigger Record
	//-------------------------------------------------------------------------
	LoadDefaultTriggerRecord(); debug("Default Trigger record loaded\r\n");

	//-------------------------------------------------------------------------
	// Load the Modem Setup Record
	//-------------------------------------------------------------------------
	LoadModemSetupRecord();	debug("Modem Setup record loaded\r\n");

	//-------------------------------------------------------------------------
	// Add OnOff Log Timestamp
	//-------------------------------------------------------------------------
	AddOnOffLogTimestamp(ON); debug("On/Off Log timestamp appended\r\n");

#if 0 /* Removed debug log file due to inducing system problems */
	//-------------------------------------------------------------------------
	// Switch Debug Log file
	SwitchDebugLogFile();
#endif

	//-------------------------------------------------------------------------
	// Init Global Unique Event Number
	//-------------------------------------------------------------------------
	InitCurrentEventNumber(); debug("Current Event Number initialized\r\n");

	//-------------------------------------------------------------------------
	// Init AutoDialout
	//-------------------------------------------------------------------------
	InitAutoDialout(); debug("Auto Dialout initialized\r\n");

	//-------------------------------------------------------------------------
	// Init Monitor Log
	//-------------------------------------------------------------------------
	InitMonitorLog(); debug("Monitor Log initialized\r\n");

	//-------------------------------------------------------------------------
	// Init the Sensor Parameters
	//-------------------------------------------------------------------------
	InitSensorParameters(g_factorySetupRecord.seismicSensorType, (uint8)g_triggerRecord.srec.sensitivity); debug("Sensor Parameters initialized\r\n");

	//-------------------------------------------------------------------------
	// Init the Summary List file
	//-------------------------------------------------------------------------
	ManageEventsDirectory();
	InitSummaryListFile(); debug("Summary List initialized\r\n");

	//-------------------------------------------------------------------------
	// Update Flash Usage Stats
	//-------------------------------------------------------------------------
	OverlayMessage(getLangText(STATUS_TEXT), getLangText(CALCULATING_EVENT_STORAGE_SPACE_FREE_TEXT), 0);
	GetSDCardUsageStats(); debug("Flash Usage Stats updated (Cluster size: %d bytes)\r\n", g_sdCardUsageStats.clusterSizeInBytes);

	//-------------------------------------------------------------------------
	// Check for Timer mode activation
	//-------------------------------------------------------------------------
	if (TimerModeActiveCheck() == TRUE)
	{
		debug("--- Timer Mode Startup ---\r\n");
		ProcessTimerMode();
	}
	else // Normal startup
	{
		debug("--- Normal Startup ---\r\n");
	}

	//-------------------------------------------------------------------------
	// Init the cmd message handling buffers before initialization of the ports.
	//-------------------------------------------------------------------------
	RemoteCmdMessageHandlerInit(); debug("Craft Message Handler initialized\r\n");

	//-------------------------------------------------------------------------
	// Init the input buffers and status flags for input craft data.
	//-------------------------------------------------------------------------
	CraftInitStatusFlags(); debug("Craft Status flags initilaized\r\n");

	//-------------------------------------------------------------------------
	// Signal remote end that RS232 Comm is available
	//-------------------------------------------------------------------------
	SET_RTS; SET_DTR; debug("Craft RTS and DTR enabled\r\n");

	//-------------------------------------------------------------------------
	// Reset LCD timers
	//-------------------------------------------------------------------------
	ResetSoftTimer(LCD_BACKLIGHT_ON_OFF_TIMER_NUM);
	ResetSoftTimer(LCD_POWER_ON_OFF_TIMER_NUM);

	//-------------------------------------------------------------------------
	// Turn on the Green keypad LED when system init complete
	//-------------------------------------------------------------------------
	debug("Init complete, turning Kepypad LED Green...\r\n");
	// Todo: Correct for the right LED once LED map is available (1&2=Red, 3&4=Green)
	PowerControl(LED_3, ON);

	//-------------------------------------------------------------------------
	// Assign a one second keypad led update timer
	//-------------------------------------------------------------------------
	AssignSoftTimer(KEYPAD_LED_TIMER_NUM, ONE_SECOND_TIMEOUT, KeypadLedUpdateTimerCallBack); debug("Keypad LED Timer initialized\r\n");

	//-------------------------------------------------------------------------
	// Jump to the true main menu
	//-------------------------------------------------------------------------
	debug("Jumping to Main Menu\r\n");
	SETUP_MENU_MSG(MAIN_MENU);
	JUMP_TO_ACTIVE_MENU();

	//-------------------------------------------------------------------------
	// Enable Craft input (delayed to prevent serial input from locking unit)
	//-------------------------------------------------------------------------
	InitCraftInterruptBuffers();

	if (GET_HARDWARE_ID == HARDWARE_ID_REV_8_WITH_GPS_MOD)
	{
		EnableGps();
	}

	//-------------------------------------------------------------------------
	// Display last line of system init
	//-------------------------------------------------------------------------
	DisplayVersionToDebug();
}

