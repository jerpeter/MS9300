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
#include "Menu.h"
#include "Record.h"
#include "Summary.h"
#include "OldUart.h"
#include "RealTimeClock.h"
#include "Display.h"
#include "spi.h"

#include "i2c.h"
#include "mxc_delay.h"
//#include "flashc.h"

/* Specific M24128 EEPROM details
Page read/write allows up to 64 bytes (if all on the same page), Pages are determined by A15 to A6
Write byte/page: dev select, byte address MSB, byte address LSB, data_in[0] .. up to data_in[63] if same page
Read byte/page: <write> dev select, byte address MSB, byte address LSB, <read> data_out[0] .. up to data_out[63] or more (unclear if restricted to 1 page rollover or not)
Device codes
Memory write: 0xA0
Memory read: 0xA1
ID write: 0xB0
ID read: 0xB1
*/

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define EEPROM_PAGE_SIZE	64

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
#include "Globals.h"

///----------------------------------------------------------------------------
///	Local Scope Globals
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SaveRecordData(void* src_ptr, uint32 num, uint8 type)
{
	uint16 loc;
	uint16 rec_size;

	/*===========================================
	Record structure layout
	---------------------------------------------
	Default trigger settings (trigger record 0)
	Saved trigger settings #1
	...
	Saved trigger settings #14
	Unit config settings
	Modem setup parameters
	Factory setup parameters
	Unique event ID storage
	Unique monitor log ID storage
	-------------------------------------------*/

	switch (type)
	{
		case REC_TRIGGER_USER_MENU_TYPE:
			debug("Programming Trigger Record...\r\n");
			((REC_EVENT_MN_STRUCT *)src_ptr)->timeStamp = GetCurrentTime();

			rec_size = sizeof(REC_EVENT_MN_STRUCT);
			loc = (uint16)(sizeof(REC_EVENT_MN_STRUCT) * num);
			SaveParameterMemory((uint8*)src_ptr, loc, rec_size);
			break;

		case REC_UNIT_CONFIG_TYPE:
			debug("Programming Unit Config...\r\n");

			if (g_lcdContrastChanged == YES) { g_lcdContrastChanged = NO; } // Reset flag since LCD Contrast change will be saved

			((UNIT_CONFIG_STRUCT *)src_ptr)->validationKey = 0xA5A5;

			rec_size = sizeof(UNIT_CONFIG_STRUCT);
			loc = (sizeof(REC_EVENT_MN_STRUCT)) * (MAX_NUM_OF_SAVED_SETUPS + 1);
			SaveParameterMemory((uint8*)src_ptr, loc, rec_size);
			break;

		case REC_MODEM_SETUP_TYPE:
			debug("Programming Modem Setup Configuration...\r\n");

			((MODEM_SETUP_STRUCT *)src_ptr)->invalid = 0x0000;

			rec_size = sizeof(MODEM_SETUP_STRUCT);
			loc = (sizeof(REC_EVENT_MN_STRUCT) * (MAX_NUM_OF_SAVED_SETUPS + 1) + sizeof(UNIT_CONFIG_STRUCT));
			SaveParameterMemory((uint8*)src_ptr, loc, rec_size);
			break;

		case REC_FACTORY_SETUP_TYPE:
			debug("Programming Factory Setup Record...\r\n");

			((FACTORY_SETUP_STRUCT *)src_ptr)->invalid = 0x0000;

			rec_size = sizeof(FACTORY_SETUP_STRUCT);
			loc = (sizeof(REC_EVENT_MN_STRUCT) * (MAX_NUM_OF_SAVED_SETUPS + 1) + sizeof(UNIT_CONFIG_STRUCT) + sizeof(MODEM_SETUP_STRUCT));
			SaveParameterMemory((uint8*)src_ptr, loc, rec_size);
			break;

		case REC_FACTORY_SETUP_CLEAR_TYPE:
			debug("Programming Factory Setup Record (Clear)...\r\n");

			((FACTORY_SETUP_STRUCT *)src_ptr)->invalid = 0xFFFF;

			rec_size = sizeof(FACTORY_SETUP_STRUCT);
			loc = (sizeof(REC_EVENT_MN_STRUCT) * (MAX_NUM_OF_SAVED_SETUPS + 1) + sizeof(UNIT_CONFIG_STRUCT) + sizeof(MODEM_SETUP_STRUCT));
			SaveParameterMemory((uint8*)src_ptr, loc, rec_size);
			break;

		case REC_UNIQUE_EVENT_ID_TYPE:
			debug("Programming Current Event Number Record...\r\n");

			((CURRENT_EVENT_NUMBER_STRUCT*)src_ptr)->invalid = 0x0000;

			rec_size = sizeof(CURRENT_EVENT_NUMBER_STRUCT);
			loc = (sizeof(REC_EVENT_MN_STRUCT) * (MAX_NUM_OF_SAVED_SETUPS + 1) + sizeof(UNIT_CONFIG_STRUCT) + sizeof(MODEM_SETUP_STRUCT) +
					sizeof(FACTORY_SETUP_STRUCT));
			SaveParameterMemory((uint8*)src_ptr, loc, rec_size);
			break;

		case REC_UNIQUE_MONITOR_LOG_ID_TYPE:
			debug("Programming Monitor Log ID Record...\r\n");

			((MONITOR_LOG_ID_STRUCT*)src_ptr)->invalid = 0x0000;

			rec_size = sizeof(MONITOR_LOG_ID_STRUCT);
			loc = (sizeof(REC_EVENT_MN_STRUCT) * (MAX_NUM_OF_SAVED_SETUPS + 1) + sizeof(UNIT_CONFIG_STRUCT) + sizeof(MODEM_SETUP_STRUCT) +
					sizeof(FACTORY_SETUP_STRUCT) + sizeof(CURRENT_EVENT_NUMBER_STRUCT));
			SaveParameterMemory((uint8*)src_ptr, loc, rec_size);
			break;

		case REC_UNIQUE_MONITOR_LOG_ID_CLEAR_TYPE:
			debug("Programming Monitor Log ID Record (Clear)...\r\n");

			((MONITOR_LOG_ID_STRUCT*)src_ptr)->invalid = 0xFFFF;

			rec_size = sizeof(MONITOR_LOG_ID_STRUCT);
			loc = (sizeof(REC_EVENT_MN_STRUCT) * (MAX_NUM_OF_SAVED_SETUPS + 1) + sizeof(UNIT_CONFIG_STRUCT) + sizeof(MODEM_SETUP_STRUCT) +
					sizeof(FACTORY_SETUP_STRUCT) + sizeof(CURRENT_EVENT_NUMBER_STRUCT));
			SaveParameterMemory((uint8*)src_ptr, loc, rec_size);
			break;

		default: // If type doesn't match, just return
			return;
			break;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GetRecordData(void* dst_ptr, uint32 num, uint8 type)
{ 
	uint16 loc;

	switch (type)
	{
		case REC_TRIGGER_USER_MENU_TYPE:
			loc = (uint16)(sizeof(REC_EVENT_MN_STRUCT) * num);
			GetParameterMemory((uint8*)dst_ptr, loc, sizeof(REC_EVENT_MN_STRUCT));
			break;

		case REC_PRINTER_USER_MENU_TYPE:
			break;

		case REC_UNIT_CONFIG_TYPE:
			loc = sizeof(REC_EVENT_MN_STRUCT) * (MAX_NUM_OF_SAVED_SETUPS + 1);
			GetParameterMemory((uint8*)dst_ptr, loc, sizeof(UNIT_CONFIG_STRUCT));
			break;

		case REC_MODEM_SETUP_TYPE:
			loc = (sizeof(REC_EVENT_MN_STRUCT) * (MAX_NUM_OF_SAVED_SETUPS + 1)) + 
					sizeof(UNIT_CONFIG_STRUCT);
			GetParameterMemory((uint8*)dst_ptr, loc, sizeof(MODEM_SETUP_STRUCT));
			break;

		case REC_FACTORY_SETUP_TYPE:
			loc = (sizeof(REC_EVENT_MN_STRUCT) * (MAX_NUM_OF_SAVED_SETUPS + 1)) + 
					sizeof(UNIT_CONFIG_STRUCT) + sizeof(MODEM_SETUP_STRUCT);
			GetParameterMemory((uint8*)dst_ptr, loc, sizeof(FACTORY_SETUP_STRUCT));
			break;

		case REC_UNIQUE_EVENT_ID_TYPE:
			loc = (sizeof(REC_EVENT_MN_STRUCT) * (MAX_NUM_OF_SAVED_SETUPS + 1)) + 
					sizeof(UNIT_CONFIG_STRUCT) + sizeof(MODEM_SETUP_STRUCT) +
					sizeof(FACTORY_SETUP_STRUCT);
			GetParameterMemory((uint8*)dst_ptr, loc, sizeof(CURRENT_EVENT_NUMBER_STRUCT));
			break;

		case REC_UNIQUE_MONITOR_LOG_ID_TYPE:
			loc = (sizeof(REC_EVENT_MN_STRUCT) * (MAX_NUM_OF_SAVED_SETUPS + 1)) + 
					sizeof(UNIT_CONFIG_STRUCT) + sizeof(MODEM_SETUP_STRUCT) +
					sizeof(FACTORY_SETUP_STRUCT) + sizeof(CURRENT_EVENT_NUMBER_STRUCT);
			GetParameterMemory((uint8*)dst_ptr, loc, sizeof(MONITOR_LOG_ID_STRUCT));
			break;

		default:
			break;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ConvertTimeStampToString(char* buff, DATE_TIME_STRUCT* timeReference, uint8 displayType)
{
	DATE_TIME_STRUCT* tempTime;
	uint8 tbuff[5];

	// Clear the buffer.
	memset(&tbuff[0], 0, sizeof(tbuff));

	switch (displayType)
	{
		case REC_DATE_TYPE:
			tempTime = timeReference;

			if ((tempTime->month >= 1) && (tempTime->month <= 12))
			{	
				strcpy((char*)tbuff, (char*)(g_monthTable[tempTime->month].name));
			}
			else
			{
				strcpy((char*)tbuff, (char*)(g_monthTable[1].name));
			}			
			
			sprintf((char*)buff,"%02d-%s-%02d", tempTime->day, tbuff, tempTime->year);
			break;

		case REC_DATE_TIME_TYPE:
			tempTime = timeReference;

			if ((tempTime->month >= 1) && (tempTime->month <= 12))
			{	
				strcpy((char*)tbuff, (char*)(g_monthTable[tempTime->month].name));
			}
			else
			{
				strcpy((char*)tbuff, (char*)(g_monthTable[1].name));
			}			
			
			sprintf((char*)buff,"%02d-%s-%02d %02d:%02d:%02d",
				tempTime->day, tbuff, tempTime->year, tempTime->hour, 
				tempTime->min, tempTime->sec);
			break;

		case REC_DATE_TIME_AM_PM_TYPE:
			tempTime = timeReference;

			if ((tempTime->month >= 1) && (tempTime->month <= 12))
			{	
				strcpy((char*)tbuff, (char*)(g_monthTable[tempTime->month].name));
			}
			else
			{
				strcpy((char*)tbuff, (char*)(g_monthTable[1].name));
			}			

			if (tempTime->hour > 12)
			{
				sprintf((char*)buff,"%02d-%s-%02d %02d:%02d:%02d pm",
					tempTime->day, tbuff, tempTime->year, (tempTime->hour - 12), 
					tempTime->min, tempTime->sec);
			}
			else
			{
				sprintf((char*)buff,"%02d-%s-%02d %02d:%02d:%02d am",
					tempTime->day, tbuff, tempTime->year, tempTime->hour, 
					tempTime->min, tempTime->sec);
			}
			break;

		case REC_DATE_TIME_DISPLAY:
			tempTime = timeReference;
			
			if ((tempTime->month >= 1) && (tempTime->month <= 12))
			{	
				strcpy((char*)tbuff, (char*)(g_monthTable[tempTime->month].name));
			}
			else
			{
				strcpy((char*)tbuff, (char*)(g_monthTable[1].name));
			}			
			
			sprintf((char*)buff,"%02d%s%02d %02d:%02d",
				tempTime->day, tbuff, tempTime->year,
				tempTime->hour, tempTime->min);
			break;

		case REC_DATE_TIME_MONITOR:
			tempTime = timeReference;
			
			if ((tempTime->month >= 1) && (tempTime->month <= 12))
			{	
				strcpy((char*)tbuff, (char*)(g_monthTable[tempTime->month].name));
			}
			else
			{
				strcpy((char*)tbuff, (char*)(g_monthTable[1].name));
			}			

			sprintf((char*)buff,"%02d-%s-%02d", 
				tempTime->day, tbuff, tempTime->year);
			break;

		default:
			break;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void CopyFlashBlock(uint16* dst, uint16* src, uint32 len)
{
	while (len > 0)
	{
		// Copy src data into dest
		*dst = *src;

		// Increment dest and src pointers
		dst++;
		src++;

		// Decrement length
		len--;
	}

	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void CopyRecordIntoFlashBk(uint16* dst,uint16* src, uint32 loc, uint32 len)
{
	//debugRaw("\n CRIFBK -> ");

	while (len > 0)
	{
		// Copy src data into dest offset by loc
		*(dst + loc) = *src;

		//debugRaw("%x ", *(dst + loc));

		// Increment dest and src pointers
		dst++;
		src++;

		// Decrement length
		len--;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 CheckForAvailableTriggerRecordEntry(char* name, uint8* match)
{
	REC_EVENT_MN_STRUCT temp_rec;
	uint8 i;
	uint8 availableRecord = 0;

#if 1 /* initialize before reference for compiler */
	temp_rec.validRecord = NO;
#endif

	// Loop through all the valid saved setup record slots (0 isn't valid, used for the default saved setup)
	for (i = 1; i <= MAX_NUM_OF_SAVED_SETUPS; i++)
	{
		// Load the temp record with the saved setup
		GetRecordData(&temp_rec, i, REC_TRIGGER_USER_MENU_TYPE);

		// Check if any of the saved setups have the same name (8 is the max number of chars that can be input)
		if (strncmp((char*)temp_rec.name, name, 8) == 0)
		{
			// Set the data pointed to by match to YES to signal a name conflict
			*match = YES;

			// Return the record location index of the matched name
			return (i);
		}
		
		// Check if an empty location hasn't been found yet (0 isn't valid)
		if (availableRecord == 0)
		{
			// Check if the current record location isn't valid
			if (temp_rec.validRecord != YES)
			{
				// Set the emptyLocation to the current index
				availableRecord = i;
			}
		}
	}
	
	return (availableRecord);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void LoadTrigRecordDefaults(REC_EVENT_MN_STRUCT* triggerRecordPtr, uint8 opMode)
{
	// General components
	triggerRecordPtr->validRecord = YES;
	triggerRecordPtr->opMode = opMode;
	triggerRecordPtr->trec.sample_rate = SAMPLE_RATE_1K;
	triggerRecordPtr->trec.samplingMethod = FIXED_SAMPLING;
	triggerRecordPtr->srec.sensitivity = LOW;
	triggerRecordPtr->trec.dist_to_source = 0;
	triggerRecordPtr->trec.weight_per_delay = 0;
	triggerRecordPtr->trec.record_time = 3;
	triggerRecordPtr->trec.airTriggerLevel = NO_TRIGGER_CHAR;
	triggerRecordPtr->trec.adjustForTempDrift = YES;
	triggerRecordPtr->trec.bitAccuracy = ACCURACY_16_BIT;
	triggerRecordPtr->trec.variableTriggerEnable = NO;
	triggerRecordPtr->trec.variableTriggerVibrationStandard = OSM_REGULATIONS_STANDARD;
	triggerRecordPtr->trec.variableTriggerPercentageLevel = 100;
	triggerRecordPtr->bgrec.barInterval = SIXTY_SEC_PRD;
	triggerRecordPtr->bgrec.summaryInterval = ONE_HOUR_INTVL;
	triggerRecordPtr->berec.barScale = BAR_SCALE_FULL;
	triggerRecordPtr->berec.barChannel = BAR_BOTH_CHANNELS;
	triggerRecordPtr->berec.barIntervalDataType = BAR_INTERVAL_ORIGINAL_DATA_TYPE_SIZE;
	triggerRecordPtr->berec.impulseMenuUpdateSecs = 1;

	// Check if sensor type is valid
	if (!g_factorySetupRecord.invalid)
	{
		triggerRecordPtr->trec.seismicTriggerLevel = ((DEFAULT_SEISMIC_TRIGGER_LEVEL_IN_INCHES_WITH_ADJUSTMENT * ACCURACY_16_BIT_MIDPOINT) / g_factorySetupRecord.seismicSensorType);
	}
	else // Don't know sensor type, use a 10 inch sensor as default
	{
		triggerRecordPtr->trec.seismicTriggerLevel = ((DEFAULT_SEISMIC_TRIGGER_LEVEL_IN_INCHES_WITH_ADJUSTMENT * ACCURACY_16_BIT_MIDPOINT) / SENSOR_10_IN);
	}

	// Clear strings
	memset((char*)triggerRecordPtr->trec.client, 0, sizeof(triggerRecordPtr->trec.client));
	memset((char*)triggerRecordPtr->trec.loc, 0, sizeof(triggerRecordPtr->trec.loc));
	memset((char*)triggerRecordPtr->trec.comments, 0, sizeof(triggerRecordPtr->trec.comments));
	memset((char*)triggerRecordPtr->trec.oper, 0, sizeof(triggerRecordPtr->trec.oper));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void LoadUnitConfigDefaults(UNIT_CONFIG_STRUCT* unitConfigPtr)
{
	// Initialize the Unit Config
	memset(unitConfigPtr, 0, sizeof(UNIT_CONFIG_STRUCT));

	// Set default conditions
	unitConfigPtr->adaptiveSampling = DISABLED;
	unitConfigPtr->adChannelVerification = ENABLED;
	unitConfigPtr->airScale = AIR_SCALE_LINEAR;
	unitConfigPtr->alarmOneAirLevel = ALARM_ONE_AIR_DEFAULT_TRIG_LVL;
	unitConfigPtr->alarmOneMode = ALARM_MODE_OFF;
	unitConfigPtr->alarmOneSeismicLevel = ALARM_ONE_SEIS_DEFAULT_TRIG_LVL;
	unitConfigPtr->alarmOneTime = ALARM_OUTPUT_TIME_DEFAULT;
	unitConfigPtr->alarmTwoAirLevel = ALARM_TWO_AIR_DEFAULT_TRIG_LVL;
	unitConfigPtr->alarmTwoMode = ALARM_MODE_OFF;
	unitConfigPtr->alarmTwoSeismicLevel = ALARM_TWO_SEIS_DEFAULT_TRIG_LVL;
	unitConfigPtr->alarmTwoTime = ALARM_OUTPUT_TIME_DEFAULT;
	unitConfigPtr->autoCalForWaveform = NO;
	unitConfigPtr->autoCalMode = AUTO_NO_CAL_TIMEOUT;
	unitConfigPtr->autoMonitorMode = AUTO_NO_TIMEOUT;
	unitConfigPtr->autoPrint = OFF;
	unitConfigPtr->barLiveMonitor = NO;
	unitConfigPtr->baudRate = BAUD_RATE_115200;
	unitConfigPtr->copies = 1;
	unitConfigPtr->externalTrigger = ENABLED;
	unitConfigPtr->flashWrapping = NO;
	unitConfigPtr->freqPlotMode = OFF;
	unitConfigPtr->freqPlotType = 1;
	unitConfigPtr->languageMode = ENGLISH_LANG;
	unitConfigPtr->lcdContrast = DEFUALT_CONTRAST;
	unitConfigPtr->lcdTimeout = 2;
	unitConfigPtr->legacyDqmLimit = DISABLED;
	unitConfigPtr->pretrigBufferDivider = PRETRIGGER_BUFFER_QUARTER_SEC_DIV;
	unitConfigPtr->cycleEndTimeHour = 0;
	unitConfigPtr->rs232PowerSavings = ENABLED;
	unitConfigPtr->saveCompressedData = SAVE_EXTRA_FILE_COMPRESSED_DATA;
	unitConfigPtr->storedEventsCapMode = DISABLED;
	unitConfigPtr->storedEventLimit = STORED_EVENT_LIMIT_DEFAULT_VALUE;
	unitConfigPtr->timerMode = DISABLED;
	unitConfigPtr->unitsOfAir = DECIBEL_TYPE;
	unitConfigPtr->unitsOfMeasure = IMPERIAL_TYPE;
	unitConfigPtr->usbSyncMode = PROMPT_OPTION;
	unitConfigPtr->vectorSum = DISABLED;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ActivateUnitConfigOptions(void)
{
	g_contrast_value = g_unitConfig.lcdContrast;

	if ((g_contrast_value < MIN_CONTRAST) || (g_contrast_value > MAX_CONTRAST))
	{
		g_unitConfig.lcdContrast = g_contrast_value = DEFUALT_CONTRAST;
	}

	SetLcdContrast(g_contrast_value);

	// The choices are between metric and sae measurement systems.
	g_sensorInfo.unitsFlag = g_unitConfig.unitsOfMeasure;
	g_sensorInfo.airUnitsFlag = g_unitConfig.unitsOfAir;

	AssignSoftTimer(LCD_BACKLIGHT_ON_OFF_TIMER_NUM, LCD_BACKLIGHT_TIMEOUT, DisplayTimerCallBack);

	if ((g_unitConfig.lcdTimeout < LCD_TIMEOUT_MIN_VALUE) || (g_unitConfig.lcdTimeout > LCD_TIMEOUT_MAX_VALUE))
	{
		g_unitConfig.lcdTimeout = LCD_TIMEOUT_DEFAULT_VALUE;
	}
	AssignSoftTimer(LCD_POWER_ON_OFF_TIMER_NUM, (uint32)(g_unitConfig.lcdTimeout * TICKS_PER_MIN), LcdPwTimerCallBack);

	debug("Auto Monitor Mode: %s\r\n", (g_unitConfig.autoMonitorMode == AUTO_NO_TIMEOUT) ? "Disabled" : "Enabled");
	AssignSoftTimer(AUTO_MONITOR_TIMER_NUM, (uint32)(g_unitConfig.autoMonitorMode * TICKS_PER_MIN), AutoMonitorTimerCallBack);

	// Check if Auto Calibration at cycle change is active (any value but zero)
	if (g_unitConfig.autoCalMode) // != AUTO_NO_CAL_TIMEOUT
	{
		g_autoCalDaysToWait = 1;
	}

	// Disable auto print since there is no printer option
	g_unitConfig.autoPrint = NO;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void LoadModemSetupRecordDefaults()
{
	// Initialize the Unit Config
	memset(&g_modemSetupRecord, 0, sizeof(g_modemSetupRecord));

	g_modemSetupRecord.modemStatus = NO;
	g_modemSetupRecord.retries = MODEM_RETRY_DEFAULT_VALUE;
	g_modemSetupRecord.retryTime = MODEM_RETRY_TIME_DEFAULT_VALUE;
	g_modemSetupRecord.dialOutCycleTime = MODEM_DIAL_OUT_TIMER_DEFAULT_VALUE;

	// No need to set these since the memset to zero satisifies initialization
	//g_modemSetupRecord.unlockCode
	//g_modemSetupRecord.init
	//g_modemSetupRecord.dial
	//g_modemSetupRecord.reset

	// New request to set the init string to point to Nomis servers
	strcpy(g_modemSetupRecord.dial, "ATDTONLINE.NOMIS.COM/8005");
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ValidateModemSetupParameters(void)
{
	uint8 updated = NO;

	if ((g_modemSetupRecord.retries < MODEM_RETRY_MIN_VALUE) || (g_modemSetupRecord.retries > MODEM_RETRY_MAX_VALUE))
	{
		g_modemSetupRecord.retries = MODEM_RETRY_DEFAULT_VALUE;
		updated = YES;	
	}

	if ((g_modemSetupRecord.retryTime < MODEM_RETRY_TIME_MIN_VALUE) || (g_modemSetupRecord.retryTime > MODEM_RETRY_TIME_MAX_VALUE))
	{
		g_modemSetupRecord.retryTime = MODEM_RETRY_TIME_DEFAULT_VALUE;
		updated = YES;
	}

	// Check if the current status is disabled and the dial string is null or empty
	if ((g_modemSetupRecord.modemStatus == NO) && ((g_modemSetupRecord.dial[0] == '\0') || (g_modemSetupRecord.dial[0] == ' ')))
	{
		// New request to set the init string to point to Nomis servers
		strcpy(g_modemSetupRecord.dial, "ATDTONLINE.NOMIS.COM/8005");
		updated = YES;
	}

	if ((g_modemSetupRecord.dialOutCycleTime == 0) || (g_modemSetupRecord.dialOutCycleTime > MODEM_DIAL_OUT_TIMER_MAX_VALUE))
	{
		g_modemSetupRecord.dialOutCycleTime = MODEM_DIAL_OUT_TIMER_DEFAULT_VALUE;
		updated = YES;
	}

	if (updated)
	{
		// Save the Modem Setup Record
		debug("Modem Setup record needed updating\r\n");
		SaveRecordData(&g_modemSetupRecord, DEFAULT_RECORD, REC_MODEM_SETUP_TYPE);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GetParameterMemory(uint8* dataDest, uint16 startAddr, uint16 dataLength)
{
#if 1 /* Normal */
	uint16 readLength;
	uint8 checkForPartialFirstPage = YES;
	uint8 lengthToPageBoundary = 0;

	while(dataLength)
	{
		//debug("SPM: Addr: %x Len: %04d -> ", startAddr, dataLength);

		// Address transfered as Big endian
		g_spareBuffer[0] = ((startAddr >> 8) & 0xFF);
		g_spareBuffer[1] = (startAddr & 0xFF);

		// Adjust for page boundaries
		if (checkForPartialFirstPage == YES)
		{
			checkForPartialFirstPage = NO;

			lengthToPageBoundary = (EEPROM_PAGE_SIZE - (startAddr % EEPROM_PAGE_SIZE));

			// Check data length against remaining length of 64 page boundary
			if (dataLength <= lengthToPageBoundary)
			{
				// Complete data storage within first/partial page
				readLength = dataLength;
				dataLength = 0;
			}
			else // Data length goes beyond first page boundary
			{
				readLength = lengthToPageBoundary;
				dataLength -= lengthToPageBoundary;
				startAddr += lengthToPageBoundary;
			}
		}
		else if (dataLength <= EEPROM_PAGE_SIZE) // Check if the rest of the data fits into the next page
		{
			readLength = dataLength;
			dataLength = 0;
		}
		else // Already aligned to a page boundary, and more than 1 page of data to write
		{
			readLength = EEPROM_PAGE_SIZE;
			dataLength -= EEPROM_PAGE_SIZE;
			startAddr += EEPROM_PAGE_SIZE;
		}

		WriteI2CDevice(MXC_I2C0, I2C_ADDR_EEPROM, &g_spareBuffer[0], 2, dataDest, readLength);
		dataDest += readLength;
	}
#else /* Test bypass since reads seem to cause the I2C to hang */
	// Todo: swap back to normal
	memset(dataDest, 0xFF, dataLength);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SaveParameterMemory(uint8* dataSrc, uint16 startAddr, uint16 dataLength)
{
#if 1 /* Normal */
	uint16 writeLength;
	uint8 checkForPartialFirstPage = YES;
	uint8 lengthToPageBoundary = 0;
	
	while(dataLength)
	{
		//debug("SPM: Addr: %x Len: %04d -> ", startAddr, dataLength);

		// Address transfered as Big endian
		g_spareBuffer[0] = ((startAddr >> 8) & 0xFF);
		g_spareBuffer[1] = (startAddr & 0xFF);

		// Adjust for page boundaries
		if (checkForPartialFirstPage == YES)
		{
			checkForPartialFirstPage = NO;
			
			lengthToPageBoundary = (EEPROM_PAGE_SIZE - (startAddr % EEPROM_PAGE_SIZE));
			
			// Check data length against remaining length of 64 page boundary
			if (dataLength <= lengthToPageBoundary)
			{
				// Complete data storage within first/partial page
				writeLength = dataLength;
				dataLength = 0;
			}
			else // Data length goes beyond first page boundary
			{
				writeLength = lengthToPageBoundary;
				dataLength -= lengthToPageBoundary;
				startAddr += lengthToPageBoundary;
			}
		}
		else if (dataLength <= EEPROM_PAGE_SIZE) // Check if the rest of the data fits into the next page
		{
			writeLength = dataLength;
			dataLength = 0;
		}
		else // Already aligned to a page boundary, and more than 1 page of data to write
		{
			writeLength = EEPROM_PAGE_SIZE;
			dataLength -= EEPROM_PAGE_SIZE;	
			startAddr += EEPROM_PAGE_SIZE;
		}			
			
		memcpy(&g_spareBuffer[2], dataSrc, writeLength);
		//debug("EEPROM SPM: Addr 0x%x 0x%x with Data 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\r\n", g_spareBuffer[0], g_spareBuffer[1], g_spareBuffer[2], g_spareBuffer[3], g_spareBuffer[4], g_spareBuffer[5], g_spareBuffer[6], g_spareBuffer[7], g_spareBuffer[8], g_spareBuffer[9]);
		//debug("EEPROM SPM: Addr 0x%x 0x%x with Data 0x%x\r\n", g_spareBuffer[0], g_spareBuffer[1], g_spareBuffer[2]);
		WriteI2CDevice(MXC_I2C0, I2C_ADDR_EEPROM, &g_spareBuffer[0], (writeLength + 2), NULL, 0);
		dataSrc += writeLength;

		// Old system used delay, new part datasheet is similar, lists 5ms write time if not able to special no ack write loop
		MXC_Delay(MXC_DELAY_MSEC(5));
	}
#else /* Test bypass since reads seem to cause the I2C to hang */
	// Todo: swap back to normal
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void EraseParameterMemory(uint16 startAddr, uint16 dataLength)
{
#if 1 /* Normal */
	uint16 pageSize;

	while(dataLength)
	{
		// Address transfered as Big endian
		g_spareBuffer[0] = ((startAddr >> 8) & 0xFF);
		g_spareBuffer[1] = (startAddr & 0xFF);

		// Check if current data length is less than 64 and can be finished in a page
		if (dataLength <= EEPROM_PAGE_SIZE)
		{
			pageSize = dataLength;
			dataLength = 0;
		}
		else // While loop will run again
		{
			pageSize = EEPROM_PAGE_SIZE;
			dataLength -= EEPROM_PAGE_SIZE;	
			startAddr += EEPROM_PAGE_SIZE;
		}			
		
		memset(&g_spareBuffer[2], 0xFF, pageSize);
		WriteI2CDevice(MXC_I2C0, I2C_ADDR_EEPROM, g_spareBuffer, (pageSize + 2), NULL, 0);

		// Old system used delay, new part datasheet is similar, lists 5ms write time if not able to special no ack write loop
		MXC_Delay(MXC_DELAY_MSEC(5));
	}
#else /* Test bypass since reads seem to cause the I2C to hang */
	// Todo: swap back to normal
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GetFlashUserPageFactorySetup(FACTORY_SETUP_STRUCT* factorySetup)
{
	// Address transfered as Big endian
	g_spareBuffer[0] = ((FLASH_USER_PAGE_BASE_ADDRESS >> 8) & 0xFF);
	g_spareBuffer[1] = (FLASH_USER_PAGE_BASE_ADDRESS & 0xFF);
	WriteI2CDevice(MXC_I2C0, I2C_ADDR_EEPROM_ID, g_spareBuffer, 2, (uint8_t*)factorySetup, sizeof(FACTORY_SETUP_STRUCT));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SaveFlashUserPageFactorySetup(FACTORY_SETUP_STRUCT* factorySetup)
{
	// Address transfered as Big endian
	g_spareBuffer[0] = ((FLASH_USER_PAGE_BASE_ADDRESS >> 8) & 0xFF);
	g_spareBuffer[1] = (FLASH_USER_PAGE_BASE_ADDRESS & 0xFF);
	memcpy(&g_spareBuffer[2], factorySetup, sizeof(FACTORY_SETUP_STRUCT));
	WriteI2CDevice(MXC_I2C0, I2C_ADDR_EEPROM_ID, g_spareBuffer, (sizeof(FACTORY_SETUP_STRUCT) + 2), NULL, 0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void EraseFlashUserPageFactorySetup(void)
{
	// Address transfered as Big endian
	g_spareBuffer[0] = ((FLASH_USER_PAGE_BASE_ADDRESS >> 8) & 0xFF);
	g_spareBuffer[1] = (FLASH_USER_PAGE_BASE_ADDRESS & 0xFF);
	memset(&g_spareBuffer[2], 0xFF, sizeof(FACTORY_SETUP_STRUCT));
	WriteI2CDevice(MXC_I2C0, I2C_ADDR_EEPROM_ID, g_spareBuffer, (sizeof(FACTORY_SETUP_STRUCT) + 2), NULL, 0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestEEPROM(void)
{
	uint8_t testMem[8];

    debug("EEPROM: Test device access...\r\n");

	memset(testMem, 0, sizeof(testMem));

#if 1 /* Test 8-byte operation */
	GetParameterMemory(&testMem[0], 0, sizeof(testMem));
	debug("EEPROM: Mem @ Addr 0 is 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\r\n", testMem[0], testMem[1], testMem[2], testMem[3], testMem[4], testMem[5], testMem[6], testMem[7]);

	testMem[0] = 0xAA;
	testMem[1] = 0x55;
	testMem[2] = 0x00;
	testMem[3] = 0x11;
	testMem[4] = 0x01;
	testMem[5] = 0x02;
	testMem[6] = 0x03;
	testMem[7] = 0x04;

	debug("EEPROM: Write Mem @ Addr 0 with 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\r\n", testMem[0], testMem[1], testMem[2], testMem[3], testMem[4], testMem[5], testMem[6], testMem[7]);
	SaveParameterMemory(&testMem[0], 0, sizeof(testMem));

	GetParameterMemory(&testMem[0], 0, sizeof(testMem));
	debug("EEPROM: Verify Mem @ Addr 0 is 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\r\n", testMem[0], testMem[1], testMem[2], testMem[3], testMem[4], testMem[5], testMem[6], testMem[7]);

	debug("EEPROM: Erasing %d bytes @ Addr 0...\r\n", sizeof(testMem));
	EraseParameterMemory(0, sizeof(testMem));

	GetParameterMemory(&testMem[0], 0, sizeof(testMem));
	debug("EEPROM: Verify erase @ Addr 0 is 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\r\n", testMem[0], testMem[1], testMem[2], testMem[3], testMem[4], testMem[5], testMem[6], testMem[7]);
#else /* Test 1-byte operation */
	GetParameterMemory(&testMem[0], 0, 1);
	debug("EEPROM: Mem @ Addr 0 is 0x%x\r\n", testMem[0]);

	testMem[0] = 0xAA;
	debug("EEPROM: Write Mem @ Addr 0 with 0x%x\r\n", testMem[0]);
	SaveParameterMemory(&testMem[0], 0, 1);

	GetParameterMemory(&testMem[0], 0, 1);
	debug("EEPROM: Verify Mem @ Addr 0 is 0x%x\r\n", testMem[0]);

	debug("EEPROM: Erasing %d bytes @ Addr 0...\r\n", 1);
	EraseParameterMemory(0, 1);

	GetParameterMemory(&testMem[0], 0, 1);
	debug("EEPROM: Verify erase @ Addr 0 is 0x%x\r\n", testMem[0]);
#endif
}
