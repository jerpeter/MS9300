///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "RemoteCommon.h"
#include "RemoteOperation.h"
#include "OldUart.h"
#include "Menu.h"
#include "InitDataBuffers.h"
#include "SysEvents.h"
#include "PowerManagement.h"
#include "Crc.h"

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
///	Function Break
///----------------------------------------------------------------------------
void HandleAAA(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

//==================================================
// Operating parameter commands
//==================================================

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleVFV(CMD_BUFFER_STRUCT* inCmd)
{
	uint8 vfvHdr[MESSAGE_HEADER_SIMPLE_LENGTH];
	uint8 msgTypeStr[HDR_TYPE_LEN+1];
	uint8 resultCode = MSGTYPE_RESPONSE;
	uint8 firmwareVersion[40];

	UNUSED(inCmd);

	sprintf((char*)msgTypeStr, "%02d", resultCode);
	BuildOutgoingSimpleHeaderBuffer((uint8*)vfvHdr, (uint8*)"VFVx", (uint8*)msgTypeStr, (MESSAGE_SIMPLE_TOTAL_LENGTH + sizeof(firmwareVersion)), COMPRESS_NONE, CRC_NONE);

	memset(&firmwareVersion[0], 0, sizeof(firmwareVersion));
	strcpy((char*)&firmwareVersion[0], (char*)&g_buildVersion[0]);

	// Calculate the CRC on the header
	g_transmitCRC = CalcCCITT32((uint8*)&vfvHdr, MESSAGE_HEADER_SIMPLE_LENGTH, SEED_32);
	// Calculate the CRC on the data
	g_transmitCRC = CalcCCITT32((uint8*)&firmwareVersion[0], sizeof(firmwareVersion), g_transmitCRC);

	// Send Starting CRLF
	ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);
	// Send Simple header
	ModemPuts((uint8*)vfvHdr, MESSAGE_HEADER_SIMPLE_LENGTH, g_binaryXferFlag);
	// Send Firmware version
	ModemPuts((uint8*)&firmwareVersion[0], sizeof(firmwareVersion), g_binaryXferFlag);
	// Send Ending Footer
#if ENDIAN_CONVERSION
	g_transmitCRC = __builtin_bswap32(g_transmitCRC);
#endif
	ModemPuts((uint8*)&g_transmitCRC, 4, NO_CONVERSION);
	ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);

	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleDCM(CMD_BUFFER_STRUCT* inCmd)
{
	SYSTEM_CFG cfg;					// 424 bytes, or 848 chars from the pc.
	uint8 dcmHdr[MESSAGE_HEADER_SIMPLE_LENGTH];
	uint8 msgTypeStr[HDR_TYPE_LEN+2];
	int majorVer, minorVer;
	char buildVer;
	uint8* channelOverridePtr;

	UNUSED(inCmd);

	memset(&cfg, 0, sizeof(cfg));

	cfg.mode = g_triggerRecord.opMode;
	cfg.monitorStatus = g_sampleProcessing; 
	cfg.currentTime = GetCurrentTime();
	
	cfg.eventCfg.distToSource = (uint32)(g_triggerRecord.trec.dist_to_source * 100.0);
	cfg.eventCfg.weightPerDelay = (uint32)(g_triggerRecord.trec.weight_per_delay * 100.0);
	cfg.eventCfg.sampleRate = (uint16)g_triggerRecord.trec.sample_rate;
#if ENDIAN_CONVERSION
	cfg.eventCfg.distToSource = __builtin_bswap32(cfg.eventCfg.distToSource);
	cfg.eventCfg.weightPerDelay = __builtin_bswap32(cfg.eventCfg.weightPerDelay);
	cfg.eventCfg.sampleRate = __builtin_bswap16(cfg.eventCfg.sampleRate);
#endif

	// Scan for major and minor version from the app version string and store in the config
	sscanf(&g_buildVersion[0], "%d.%d.%c", &majorVer, &minorVer, &buildVer);

#if 1 /* Test spoofing an 8100 firmware version so SGx2 can handle the seismic trigger correctly */
	// Per Dave, any 3.xx.x should satisfy SGx2 using the latest seismic trigger setting logic
	majorVer = 3;
#endif

	cfg.eventCfg.appMajorVersion = (uint8)majorVer;
	cfg.eventCfg.appMinorVersion = (uint8)minorVer;
	cfg.appBuildVersion = buildVer;

	// Waveform specific - Initial conditions.
#if 1 /* Original - Fixed method (Dave prefers trigger level as 16-bit regardless of bit accuracy setting) */
	if (g_triggerRecord.trec.variableTriggerEnable == YES)
	{
		cfg.eventCfg.seismicTriggerLevel = (VARIABLE_TRIGGER_CHAR_BASE + g_triggerRecord.trec.variableTriggerVibrationStandard);
	}
	else // NO_TRIGGER_CHAR, EXTERNAL_TRIGGER_CHAR or valid trigger
	{
		cfg.eventCfg.seismicTriggerLevel = g_triggerRecord.trec.seismicTriggerLevel;
	}
#else /* New - Bit accuracy adjusted */
	if ((g_triggerRecord.trec.seismicTriggerLevel == NO_TRIGGER_CHAR) || (g_triggerRecord.trec.seismicTriggerLevel == EXTERNAL_TRIGGER_CHAR))
	{
		cfg.eventCfg.seismicTriggerLevel = g_triggerRecord.trec.seismicTriggerLevel;
	}
	else if (g_triggerRecord.trec.variableTriggerEnable == YES)
	{
		cfg.eventCfg.seismicTriggerLevel = (VARIABLE_TRIGGER_CHAR_BASE + g_triggerRecord.trec.variableTriggerVibrationStandard);
	}
	else
	{
		cfg.eventCfg.seismicTriggerLevel = (g_triggerRecord.trec.seismicTriggerLevel / (ACCURACY_16_BIT_MIDPOINT / g_bitAccuracyMidpoint));
	}
#endif
#if ENDIAN_CONVERSION
	cfg.eventCfg.seismicTriggerLevel = __builtin_bswap32(cfg.eventCfg.seismicTriggerLevel);
#endif

#if 1 /* Original - Fixed method (Dave prefers trigger level as 16-bit regardless of bit accuracy setting) */
	cfg.eventCfg.airTriggerLevel = g_triggerRecord.trec.airTriggerLevel;
#else /* New - Bit accuracy adjusted */
	if ((g_triggerRecord.trec.airTriggerLevel == NO_TRIGGER_CHAR) || (g_triggerRecord.trec.airTriggerLevel == EXTERNAL_TRIGGER_CHAR))
	{
		cfg.eventCfg.airTriggerLevel = g_triggerRecord.trec.airTriggerLevel;
	}
	else
	{
		cfg.eventCfg.airTriggerLevel = (g_triggerRecord.trec.airTriggerLevel / (ACCURACY_16_BIT_MIDPOINT / g_bitAccuracyMidpoint));
	}
#endif
#if ENDIAN_CONVERSION
	cfg.eventCfg.airTriggerLevel = __builtin_bswap32(cfg.eventCfg.airTriggerLevel);
#endif

	cfg.variableTriggerPercentageLevel = g_triggerRecord.trec.variableTriggerPercentageLevel;
	cfg.eventCfg.recordTime = g_triggerRecord.trec.record_time;
#if ENDIAN_CONVERSION
	cfg.eventCfg.recordTime = __builtin_bswap32(cfg.eventCfg.recordTime);
#endif

	// static non changing.
	cfg.eventCfg.seismicSensorType = (uint16)(g_factorySetupRecord.seismicSensorType);
	cfg.eventCfg.airSensorType = g_factorySetupRecord.acousticSensorType;
	cfg.eventCfg.adChannelVerification = g_unitConfig.adChannelVerification;
	cfg.eventCfg.bitAccuracy = g_triggerRecord.trec.bitAccuracy;
	cfg.eventCfg.numOfChannels = NUMBER_OF_CHANNELS_DEFAULT;
	cfg.eventCfg.adjustForTempDrift = g_triggerRecord.trec.adjustForTempDrift;
	cfg.eventCfg.pretrigBufferDivider = g_unitConfig.pretrigBufferDivider;
	cfg.eventCfg.numOfSamples = 0;				// Not used for configuration settings
#if ENDIAN_CONVERSION
	cfg.eventCfg.seismicSensorType = __builtin_bswap16(cfg.eventCfg.seismicSensorType);
	cfg.eventCfg.airSensorType = __builtin_bswap16(cfg.eventCfg.airSensorType);
	cfg.eventCfg.numOfSamples = __builtin_bswap16(cfg.eventCfg.numOfSamples);
#endif

	if ((g_factorySetupRecord.aWeightOption == ENABLED) && (g_unitConfig.airScale == AIR_SCALE_LINEAR))
	{
		// Need to signal remote side that current setting is linear but that A-weighting is available
		cfg.eventCfg.aWeighting = (AIR_SCALE_LINEAR | (cfg.eventCfg.aWeighting << cfg.eventCfg.aWeighting));
	}
	else { cfg.eventCfg.aWeighting = g_factorySetupRecord.aWeightOption; }

	cfg.eventCfg.preBuffNumOfSamples = (g_triggerRecord.trec.sample_rate / g_unitConfig.pretrigBufferDivider);
	cfg.eventCfg.calDataNumOfSamples = CALIBRATION_NUMBER_OF_SAMPLES;
	cfg.eventCfg.activeChannels = NUMBER_OF_CHANNELS_DEFAULT;
#if ENDIAN_CONVERSION
	cfg.eventCfg.preBuffNumOfSamples = __builtin_bswap16(cfg.eventCfg.preBuffNumOfSamples);
	cfg.eventCfg.calDataNumOfSamples = __builtin_bswap16(cfg.eventCfg.calDataNumOfSamples);
#endif

	// Overloaded elements rightfully in their own place
	cfg.extraUnitCfg.sensitivity = (uint8)g_triggerRecord.srec.sensitivity;
	cfg.extraUnitCfg.barScale = g_triggerRecord.berec.barScale;
	cfg.extraUnitCfg.barChannel = g_triggerRecord.berec.barChannel;

	// Needed for events but not remote config
	cfg.eventCfg.seismicUnitsOfMeasure = g_unitConfig.unitsOfMeasure;
	cfg.eventCfg.airUnitsOfMeasure = g_unitConfig.unitsOfAir;
	cfg.eventCfg.airTriggerInUnits = 0x0; // Not needed
	cfg.eventCfg.seismicTriggerInUnits = 0x0; // Not needed
#if ENDIAN_CONVERSION
	cfg.eventCfg.airTriggerInUnits = __builtin_bswap32(cfg.eventCfg.airTriggerInUnits);
	cfg.eventCfg.seismicTriggerInUnits = __builtin_bswap32(cfg.eventCfg.seismicTriggerInUnits);
#endif

	// Bargraph specific - Initial conditions.
	cfg.eventCfg.barInterval = (uint16)g_triggerRecord.bgrec.barInterval;
	cfg.eventCfg.summaryInterval = (uint16)g_triggerRecord.bgrec.summaryInterval;
	cfg.eventCfg.barIntervalDataType = g_triggerRecord.berec.barIntervalDataType;
#if ENDIAN_CONVERSION
	cfg.eventCfg.barInterval = __builtin_bswap16(cfg.eventCfg.barInterval);
	cfg.eventCfg.summaryInterval = __builtin_bswap16(cfg.eventCfg.summaryInterval);
#endif

	memcpy((uint8*)cfg.eventCfg.companyName, g_triggerRecord.trec.client, COMPANY_NAME_STRING_SIZE - 2);
	memcpy((uint8*)cfg.eventCfg.seismicOperator, g_triggerRecord.trec.oper, SEISMIC_OPERATOR_STRING_SIZE - 2);
	memcpy((uint8*)cfg.eventCfg.sessionLocation, g_triggerRecord.trec.loc, SESSION_LOCATION_STRING_SIZE - 2);
	memcpy((uint8*)cfg.eventCfg.sessionComments, g_triggerRecord.trec.comments, SESSION_COMMENTS_STRING_SIZE - 2);
	
	memset(&(cfg.eventCfg.seismicSensorSerialNumber[0]), 0, SENSOR_SERIAL_NUMBER_SIZE);
	memcpy(&(cfg.eventCfg.seismicSensorSerialNumber[0]), &(g_seismicSmartSensorMemory.serialNumber[0]), SENSOR_SERIAL_NUMBER_SIZE);
	cfg.eventCfg.seismicSensorCurrentCalDate = g_seismicSmartSensorMemory.currentCal.calDate;
	cfg.eventCfg.seismicSensorFacility = g_seismicSmartSensorMemory.currentCal.calFacility;
	cfg.eventCfg.seismicSensorInstrument = g_seismicSmartSensorMemory.currentCal.calInstrument;

	memset(&(cfg.eventCfg.acousticSensorSerialNumber[0]), 0, SENSOR_SERIAL_NUMBER_SIZE);
	memcpy(&(cfg.eventCfg.acousticSensorSerialNumber[0]), &(g_acousticSmartSensorMemory.serialNumber[0]), SENSOR_SERIAL_NUMBER_SIZE);
	cfg.eventCfg.acousticSensorCurrentCalDate = g_acousticSmartSensorMemory.currentCal.calDate;
	cfg.eventCfg.acousticSensorFacility = g_acousticSmartSensorMemory.currentCal.calFacility;
	cfg.eventCfg.acousticSensorInstrument = g_acousticSmartSensorMemory.currentCal.calInstrument;
#if ENDIAN_CONVERSION
	cfg.eventCfg.seismicSensorCurrentCalDate.year = __builtin_bswap16(cfg.eventCfg.seismicSensorCurrentCalDate.year);
	cfg.eventCfg.acousticSensorCurrentCalDate.year = __builtin_bswap16(cfg.eventCfg.acousticSensorCurrentCalDate.year);
#endif

	cfg.autoCfg.autoMonitorMode = g_unitConfig.autoMonitorMode;
	cfg.autoCfg.autoCalMode = g_unitConfig.autoCalMode;
	cfg.autoCfg.externalTrigger = g_unitConfig.externalTrigger;
	cfg.autoCfg.rs232PowerSavings = g_unitConfig.rs232PowerSavings;

	cfg.extraUnitCfg.autoPrint = g_unitConfig.autoPrint;
	cfg.extraUnitCfg.languageMode = g_unitConfig.languageMode;
	cfg.extraUnitCfg.vectorSum = g_unitConfig.vectorSum;
	cfg.extraUnitCfg.unitsOfMeasure = g_unitConfig.unitsOfMeasure;
	cfg.extraUnitCfg.unitsOfAir = g_unitConfig.unitsOfAir;
	cfg.extraUnitCfg.freqPlotMode = g_unitConfig.freqPlotMode;
	cfg.extraUnitCfg.freqPlotType = g_unitConfig.freqPlotType;

	cfg.alarmCfg.alarmOneMode = g_unitConfig.alarmOneMode;
	cfg.alarmCfg.alarmOneSeismicLevel = g_unitConfig.alarmOneSeismicLevel;
	cfg.alarmCfg.alarmOneSeismicMinLevel = g_unitConfig.alarmOneSeismicLevel;
	cfg.alarmCfg.alarmOneAirLevel = g_unitConfig.alarmOneAirLevel;
	cfg.alarmCfg.alarmOneAirMinLevel = g_unitConfig.alarmOneAirLevel;
	cfg.alarmCfg.alarmOneTime = (uint32)(g_unitConfig.alarmOneTime * (float)100.0);
#if ENDIAN_CONVERSION
	cfg.alarmCfg.alarmOneSeismicLevel = __builtin_bswap32(cfg.alarmCfg.alarmOneSeismicLevel);
	cfg.alarmCfg.alarmOneSeismicMinLevel = __builtin_bswap32(cfg.alarmCfg.alarmOneSeismicMinLevel);
	cfg.alarmCfg.alarmOneAirLevel = __builtin_bswap32(cfg.alarmCfg.alarmOneAirLevel);
	cfg.alarmCfg.alarmOneAirMinLevel = __builtin_bswap32(cfg.alarmCfg.alarmOneAirMinLevel);
	cfg.alarmCfg.alarmOneTime = __builtin_bswap32(cfg.alarmCfg.alarmOneTime);
#endif

	cfg.alarmCfg.alarmTwoMode = g_unitConfig.alarmTwoMode;
	cfg.alarmCfg.alarmTwoSeismicLevel = g_unitConfig.alarmTwoSeismicLevel;
	cfg.alarmCfg.alarmTwoSeismicMinLevel = g_unitConfig.alarmTwoSeismicLevel;
	cfg.alarmCfg.alarmTwoAirLevel = g_unitConfig.alarmTwoAirLevel;
	cfg.alarmCfg.alarmTwoAirMinLevel = g_unitConfig.alarmTwoAirLevel;
	cfg.alarmCfg.alarmTwoTime = (uint32)(g_unitConfig.alarmTwoTime * (float)100.0);
#if ENDIAN_CONVERSION
	cfg.alarmCfg.alarmTwoSeismicLevel = __builtin_bswap32(cfg.alarmCfg.alarmTwoSeismicLevel);
	cfg.alarmCfg.alarmTwoSeismicMinLevel = __builtin_bswap32(cfg.alarmCfg.alarmTwoSeismicMinLevel);
	cfg.alarmCfg.alarmTwoAirLevel = __builtin_bswap32(cfg.alarmCfg.alarmTwoAirLevel);
	cfg.alarmCfg.alarmTwoAirMinLevel = __builtin_bswap32(cfg.alarmCfg.alarmTwoAirMinLevel);
	cfg.alarmCfg.alarmTwoTime = __builtin_bswap32(cfg.alarmCfg.alarmTwoTime);
#endif

	cfg.alarmCfg.legacyDqmLimit = g_unitConfig.legacyDqmLimit;
	cfg.alarmCfg.storedEventsCapMode = g_unitConfig.storedEventsCapMode;
	cfg.alarmCfg.storedEventLimit = g_unitConfig.storedEventLimit;
#if ENDIAN_CONVERSION
	cfg.alarmCfg.storedEventLimit = __builtin_bswap16(cfg.alarmCfg.storedEventLimit);
#endif

	cfg.timerCfg.timerMode = g_unitConfig.timerMode;
	cfg.timerCfg.timerModeFrequency = g_unitConfig.timerModeFrequency;
	cfg.timerCfg.cycleEndTimeHour = ((g_unitConfig.cycleEndTimeHour == 0) ? 24 : g_unitConfig.cycleEndTimeHour); // Using 24 as midnight value for remote config since a zero default with older SG would incorrectly reset it

	if (DISABLED == g_unitConfig.timerMode)
	{
		cfg.timerCfg.timer_stop = cfg.timerCfg.timer_start = GetCurrentTime();
	}
	else
	{
		cfg.timerCfg.timer_start.hour = g_unitConfig.timerStartTime.hour;
		cfg.timerCfg.timer_start.min = g_unitConfig.timerStartTime.min;
		cfg.timerCfg.timer_start.sec = g_unitConfig.timerStartTime.sec;
		cfg.timerCfg.timer_start.day = g_unitConfig.timerStartDate.day;
		cfg.timerCfg.timer_start.month = g_unitConfig.timerStartDate.month;
		cfg.timerCfg.timer_start.year = g_unitConfig.timerStartDate.year;

		cfg.timerCfg.timer_stop.hour = g_unitConfig.timerStopTime.hour;
		cfg.timerCfg.timer_stop.min = g_unitConfig.timerStopTime.min;
		cfg.timerCfg.timer_stop.sec = g_unitConfig.timerStopTime.sec;
		cfg.timerCfg.timer_stop.day = g_unitConfig.timerStopDate.day;
		cfg.timerCfg.timer_stop.month = g_unitConfig.timerStopDate.month;
		cfg.timerCfg.timer_stop.year = g_unitConfig.timerStopDate.year;
	}
	
	// Add in the flash wrapping option
#if 0 /* Normal */
	cfg.flashWrapping = g_unitConfig.flashWrapping;
#else /* Forcing flash wrapping to be disabled */
	cfg.flashWrapping = NO;
#endif

	cfg.batteryLevel = (uint16)(100.0 * GetExternalVoltageLevelAveraged(BATTERY_VOLTAGE));
#if ENDIAN_CONVERSION
	cfg.batteryLevel = __builtin_bswap16(cfg.batteryLevel);
#endif

	// Add in Factory Setup Calibration Date, GPS location and UTC time overriding the A/D channel info section which is unused for DCM
	channelOverridePtr = (uint8*)&cfg.eventCfg.channel[0];
	memcpy(channelOverridePtr, &g_factorySetupRecord.calDate, sizeof(g_factorySetupRecord.calDate));
#if ENDIAN_CONVERSION
	CALIBRATION_DATE_STRUCT* tempDatePtr = (CALIBRATION_DATE_STRUCT*) channelOverridePtr;
	tempDatePtr->year = __builtin_bswap16(tempDatePtr->year);
#endif
	channelOverridePtr += sizeof(g_factorySetupRecord.calDate);

	if (GET_HARDWARE_ID == HARDWARE_ID_REV_8_WITH_GPS_MOD)
	{
		memcpy(channelOverridePtr, &g_gpsPosition, sizeof(g_gpsPosition));
	}
#if ENDIAN_CONVERSION
	GPS_POSITION* tempGpsPtr = (GPS_POSITION*)channelOverridePtr;
	tempGpsPtr->latSeconds = __builtin_bswap16(tempGpsPtr->latSeconds);
	tempGpsPtr->longSeconds = __builtin_bswap16(tempGpsPtr->longSeconds);
	tempGpsPtr->altitude = __builtin_bswap16(tempGpsPtr->altitude);
	tempGpsPtr->utcYear = __builtin_bswap16(tempGpsPtr->utcYear);
#endif

	// Spare fields, just use as a data marker
	cfg.unused[0] = 0x0A;
	cfg.unused[1] = 0x0B;
	cfg.unused[2] = 0x0C;
	//cfg.unused[3] = 0x0D;
	//cfg.unused[4] = 0x0E;
	//cfg.unused[5] = 0x0F;

	sprintf((char*)msgTypeStr, "%02d", MSGTYPE_RESPONSE);
	BuildOutgoingSimpleHeaderBuffer((uint8*)dcmHdr, (uint8*)"DCMx", (uint8*)msgTypeStr,
		(uint32)(MESSAGE_SIMPLE_TOTAL_LENGTH + sizeof(SYSTEM_CFG)), COMPRESS_NONE, CRC_NONE);	

	// Send Starting CRLF
	ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);

	// Calculate the CRC on the header
	g_transmitCRC = CalcCCITT32((uint8*)&dcmHdr, MESSAGE_HEADER_SIMPLE_LENGTH, SEED_32);		

	// Send Simple header
	ModemPuts((uint8*)dcmHdr, MESSAGE_HEADER_SIMPLE_LENGTH, g_binaryXferFlag);

	// Calculate the CRC on the data
	g_transmitCRC = CalcCCITT32((uint8*)&cfg, sizeof(SYSTEM_CFG), g_transmitCRC);		

	// Send the configuration data
	ModemPuts((uint8*)&cfg, sizeof(SYSTEM_CFG), g_binaryXferFlag);

	// Ending Footer
#if ENDIAN_CONVERSION
	g_transmitCRC = __builtin_bswap32(g_transmitCRC);
#endif
	ModemPuts((uint8*)&g_transmitCRC, 4, NO_CONVERSION);
	ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleUCM(CMD_BUFFER_STRUCT* inCmd)
{
	SYSTEM_CFG cfg;
	uint8* cfgPtr = (uint8*)(&cfg);
	uint8 timerModeModified = FALSE;
	uint16 buffDex;
	uint32 timeCheck;
	uint32 maxRecordTime;
	uint32 returnCode = CFG_ERR_NONE;
	uint32 msgCRC = 0;
	uint32 inCRC;
	uint16 j = 0;
	uint8 ucmHdr[MESSAGE_HEADER_SIMPLE_LENGTH];
	uint8 msgTypeStr[HDR_TYPE_LEN+2];
	float timeCheckFloat;
	float timeAlarmFloat;
	float div;

	if (ACTIVE_STATE == g_sampleProcessing)
	{
		returnCode = CFG_ERR_MONITORING_STATE;
	}
	else
	{	
		memset(&cfg, 0, sizeof(cfg));

		// Check to see if the incoming message is the correct size
		if ((uint32)((inCmd->size - 16)/2) < sizeof(cfg))
		{
			debug("WARNING:Msg Size incorrect msgSize=%d cfgSize=%d \r\n", ((inCmd->size - 16)/2), sizeof(cfg));
		}
		
		// Move the string data into the configuration structure. String is (2 * cfgSize)
		buffDex = MESSAGE_HEADER_SIMPLE_LENGTH;
		while ((buffDex < inCmd->size) && (buffDex < (MESSAGE_HEADER_SIMPLE_LENGTH + (sizeof(cfg) * 2))) &&
				(buffDex < CMD_BUFFER_SIZE))
		{	
			*cfgPtr++ = ConvertAscii2Binary(inCmd->msg[buffDex], inCmd->msg[buffDex + 1]);
			buffDex += 2;
		}		

		// Check if the SuperGraphics version is non-zero suggesting that it's a version that supports sending back CRC
#if ENDIAN_CONVERSION
		if (__builtin_bswap16(cfg.sgVersion))
#else /* Normal, no conversion */
		if (cfg.sgVersion)
#endif
		{
			// Get the CRC value from the data stream
			while ((buffDex < inCmd->size) && (buffDex < CMD_BUFFER_SIZE) && (j < 4))
			{
				// Add each CRC value by byte
				((uint8*)&inCRC)[j++] = ConvertAscii2Binary(inCmd->msg[buffDex], inCmd->msg[buffDex + 1]);
				buffDex += 2;
			}

			// Calcualte the CRC on the transmitted header and the converted binary data
			msgCRC = CalcCCITT32((uint8*)&(inCmd->msg[0]), MESSAGE_HEADER_SIMPLE_LENGTH, SEED_32);
			msgCRC = CalcCCITT32((uint8*)&cfg, sizeof(cfg), msgCRC);
#if ENDIAN_CONVERSION
			msgCRC = __builtin_bswap32(msgCRC);
			debug("UCM CRC compare: In %08x, Msg %08x\r\n", inCRC, msgCRC);
#endif
			// Check if the incoming CRC value matches the calucalted message CRC
			if (inCRC != msgCRC)
			{
				// Signal a bad CRC value
				returnCode = CFG_ERR_BAD_CRC;

				sprintf((char*)msgTypeStr, "%02lu", returnCode);
				BuildOutgoingSimpleHeaderBuffer((uint8*)ucmHdr, (uint8*)"UCMx",
					(uint8*)msgTypeStr, MESSAGE_SIMPLE_TOTAL_LENGTH, COMPRESS_NONE, CRC_NONE);	

				// Send Starting CRLF
				ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);

				// Calculate the CRC on the header
				g_transmitCRC = CalcCCITT32((uint8*)&ucmHdr, MESSAGE_HEADER_SIMPLE_LENGTH, SEED_32);		

				// Send Simple header
				ModemPuts((uint8*)ucmHdr, MESSAGE_HEADER_SIMPLE_LENGTH, CONVERT_DATA_TO_ASCII);

				// Send Ending Footer
#if ENDIAN_CONVERSION
				g_transmitCRC = __builtin_bswap32(g_transmitCRC);
#endif
				ModemPuts((uint8*)&g_transmitCRC, 4, NO_CONVERSION);
				ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);
				
				return;
			}
		}

		//---------------------------------------------------------------------------
		// Check the new operation mode
		//---------------------------------------------------------------------------
		switch (cfg.mode)
		{
			case WAVEFORM_MODE: 
			case BARGRAPH_MODE:
			case COMBO_MODE:
				g_triggerRecord.opMode = cfg.mode;
				break;

			case MANUAL_CAL_MODE:
			case MANUAL_TRIGGER_MODE:
			default:
				returnCode = CFG_ERR_TRIGGER_MODE;
				goto SEND_UCM_ERROR_CODE;
				break;
		}
		
		//---------------------------------------------------------------------------
		// Check date and time to see if an update needs to be applied
		//---------------------------------------------------------------------------
		if ((cfg.currentTime.day == 0) && (cfg.currentTime.month == 0) && (cfg.currentTime.year == 0) &&
			(cfg.currentTime.sec == 0) && (cfg.currentTime.min == 0) && (cfg.currentTime.hour == 0))
		{
			debug("Do not update time.\r\n");
		}
		else // Update date and time
		{
			// Check for correct values and update the month.
			if ((cfg.currentTime.day == 0) || (cfg.currentTime.day > 31) || (cfg.currentTime.month == 0) ||
				(cfg.currentTime.month > 12) || (cfg.currentTime.year > 99))
			{
				returnCode = CFG_ERR_SYSTEM_DATE;
				goto SEND_UCM_ERROR_CODE;
			}
			else // Date validated
			{
				SetExternalRtcDate(&(cfg.currentTime));
				g_unitConfig.timerMode = DISABLED;

				// Disable the Power Off timer if it's set
				ClearSoftTimer(POWER_OFF_TIMER_NUM);
			}
			
			// Check for correct values and update the date.
			if ((cfg.currentTime.sec > 59) || (cfg.currentTime.min > 59) || (cfg.currentTime.hour > 23))
			{
				returnCode = CFG_ERR_SYSTEM_TIME;
				goto SEND_UCM_ERROR_CODE;
			}
			else // Time validated
			{
				SetExternalRtcTime(&(cfg.currentTime));
				g_unitConfig.timerMode = DISABLED;
				
				// Disable the Power Off timer if it's set
				ClearSoftTimer(POWER_OFF_TIMER_NUM);
			}

			// Update the current time.
			UpdateCurrentTime();
		}

		//---------------------------------------------------------------------------
		// Distance to source check, cfg is in uint32 format not float.
		//---------------------------------------------------------------------------
#if ENDIAN_CONVERSION
		cfg.eventCfg.distToSource = __builtin_bswap32(cfg.eventCfg.distToSource);
#endif
		if (cfg.eventCfg.distToSource > (uint32)(DISTANCE_TO_SOURCE_MAX_VALUE * 100))
		{
			returnCode = CFG_ERR_DIST_TO_SRC;
			goto SEND_UCM_ERROR_CODE;
		}
		else
		{
			g_triggerRecord.trec.dist_to_source = (float)((float)cfg.eventCfg.distToSource / (float)100.0);
		}

		//---------------------------------------------------------------------------
		// Weight per delay check, cfg is in uint32 format not float
		//---------------------------------------------------------------------------
#if ENDIAN_CONVERSION
		cfg.eventCfg.weightPerDelay = __builtin_bswap32(cfg.eventCfg.weightPerDelay);
#endif
		if (cfg.eventCfg.weightPerDelay > (uint32)(WEIGHT_PER_DELAY_MAX_VALUE * 100))
		{
			returnCode = CFG_ERR_WEIGHT_DELAY;
			goto SEND_UCM_ERROR_CODE;
		}
		else
		{
			g_triggerRecord.trec.weight_per_delay = (float)((float)cfg.eventCfg.weightPerDelay / (float)100.0);
		}

		//---------------------------------------------------------------------------
		// Sensitivity check (Must be set before Seismic Trigger level)
		//---------------------------------------------------------------------------
		if ((cfg.extraUnitCfg.sensitivity == LOW) || (cfg.extraUnitCfg.sensitivity == HIGH))
		{
			g_triggerRecord.srec.sensitivity = cfg.extraUnitCfg.sensitivity;
		}
		else
		{
			returnCode = CFG_ERR_SENSITIVITY;
			goto SEND_UCM_ERROR_CODE;
		}

		//---------------------------------------------------------------------------
		// Bit Accuracy check (Must be set before Seismic Trigger level)
		//---------------------------------------------------------------------------
		if ((cfg.eventCfg.bitAccuracy == ACCURACY_10_BIT) || (cfg.eventCfg.bitAccuracy == ACCURACY_12_BIT) ||
		(cfg.eventCfg.bitAccuracy == ACCURACY_14_BIT) || (cfg.eventCfg.bitAccuracy == ACCURACY_16_BIT))
		{
			g_triggerRecord.trec.bitAccuracy = cfg.eventCfg.bitAccuracy;

			switch (g_triggerRecord.trec.bitAccuracy)
			{
				case ACCURACY_10_BIT: { g_bitAccuracyMidpoint = ACCURACY_10_BIT_MIDPOINT; } break;
				case ACCURACY_12_BIT: { g_bitAccuracyMidpoint = ACCURACY_12_BIT_MIDPOINT; } break;
				case ACCURACY_14_BIT: { g_bitAccuracyMidpoint = ACCURACY_14_BIT_MIDPOINT; } break;
				default: { g_bitAccuracyMidpoint = ACCURACY_16_BIT_MIDPOINT; } break;
			}
		}
		else
		{
			returnCode = CFG_ERR_BIT_ACCURACY;
			goto SEND_UCM_ERROR_CODE;
		}

		//---------------------------------------------------------------------------
		// Seismic Trigger Level check
		//---------------------------------------------------------------------------
#if ENDIAN_CONVERSION
		cfg.eventCfg.seismicTriggerLevel = __builtin_bswap32(cfg.eventCfg.seismicTriggerLevel);
#endif
		if ((MANUAL_TRIGGER_CHAR == cfg.eventCfg.seismicTriggerLevel) || (NO_TRIGGER_CHAR == cfg.eventCfg.seismicTriggerLevel) ||
			((cfg.eventCfg.seismicTriggerLevel >= SEISMIC_TRIGGER_MIN_VALUE) && (cfg.eventCfg.seismicTriggerLevel <= SEISMIC_TRIGGER_MAX_VALUE)))
		{
			g_triggerRecord.trec.variableTriggerEnable = NO;
			g_triggerRecord.trec.seismicTriggerLevel = cfg.eventCfg.seismicTriggerLevel;
		}
		else if (((cfg.eventCfg.seismicTriggerLevel > VARIABLE_TRIGGER_CHAR_BASE) && (cfg.eventCfg.seismicTriggerLevel < (VARIABLE_TRIGGER_CHAR_BASE + END_OF_VIBRATION_STANDARDS_LIST))) ||
				((cfg.eventCfg.seismicTriggerLevel > (VARIABLE_TRIGGER_CHAR_BASE + START_OF_CUSTOM_CURVES_LIST)) && (cfg.eventCfg.seismicTriggerLevel < (VARIABLE_TRIGGER_CHAR_BASE + END_OF_VIBRATION_CURVES_LIST))))
		{
			g_triggerRecord.trec.variableTriggerEnable = YES;
			g_triggerRecord.trec.variableTriggerVibrationStandard = (cfg.eventCfg.seismicTriggerLevel - VARIABLE_TRIGGER_CHAR_BASE);

			// Check if the value is out of bounds
			if ((cfg.variableTriggerPercentageLevel < VT_PERCENT_OF_LIMIT_MIN_VALUE) || (cfg.variableTriggerPercentageLevel > VT_PERCENT_OF_LIMIT_MAX_VALUE))
			{
				// Load the default
				g_triggerRecord.trec.variableTriggerPercentageLevel = VT_PERCENT_OF_LIMIT_DEFAULT_VALUE;
			}
			else // Load the remote value
			{
				g_triggerRecord.trec.variableTriggerPercentageLevel = cfg.variableTriggerPercentageLevel;
			}

			div = (float)(g_bitAccuracyMidpoint * g_sensorInfo.sensorAccuracy * ((g_triggerRecord.srec.sensitivity == LOW) ? 2 : 4)) / (float)(g_factorySetupRecord.seismicSensorType);

			if (g_triggerRecord.trec.variableTriggerVibrationStandard == CUSTOM_STEP_THRESHOLD)
			{
				// Set the fixed trigger level to 1 IPS since anything above this level is an automatic trigger
				g_triggerRecord.trec.seismicTriggerLevel = (uint32)(1.00 * div);
			}
			else // All other standards and custom curves
			{
				// Set the fixed trigger level to 2 IPS since anything above this level is an automatic trigger for all vibration standards
				g_triggerRecord.trec.seismicTriggerLevel = (uint32)(2.00 * div);
			}

			// Up convert to 16-bit since user selected level is based on selected bit accuracy
			g_triggerRecord.trec.seismicTriggerLevel *= (SEISMIC_TRIGGER_MAX_VALUE / g_bitAccuracyMidpoint);

			// Factor in % of Limit choice at 16-bit for better accuracy
			g_triggerRecord.trec.seismicTriggerLevel = (uint32)(g_triggerRecord.trec.seismicTriggerLevel * (float)((float)g_triggerRecord.trec.variableTriggerPercentageLevel / (float)100));
		}
		else
		{
			returnCode = CFG_ERR_SEISMIC_TRIG_LVL;
			goto SEND_UCM_ERROR_CODE;
		}
		
		//---------------------------------------------------------------------------
		// Sample Rate check
		//---------------------------------------------------------------------------
#if ENDIAN_CONVERSION
		cfg.eventCfg.sampleRate = __builtin_bswap16(cfg.eventCfg.sampleRate);
#endif
		if ((SAMPLE_RATE_1K == cfg.eventCfg.sampleRate) || (SAMPLE_RATE_2K == cfg.eventCfg.sampleRate) || (SAMPLE_RATE_4K == cfg.eventCfg.sampleRate) ||
			(SAMPLE_RATE_8K == cfg.eventCfg.sampleRate) || (SAMPLE_RATE_16K == cfg.eventCfg.sampleRate))
		{
			// Check if the selected sample rate is higher than the allowed for either Bargraph or Combo
			if (((g_triggerRecord.opMode == BARGRAPH_MODE) && (cfg.eventCfg.sampleRate > SAMPLE_RATE_8K)) || ((g_triggerRecord.opMode == COMBO_MODE) && (cfg.eventCfg.sampleRate > SAMPLE_RATE_8K)) ||
				((g_triggerRecord.trec.variableTriggerEnable == YES) && (cfg.eventCfg.sampleRate == SAMPLE_RATE_16K)))
			{
				returnCode = CFG_ERR_SAMPLE_RATE;
				goto SEND_UCM_ERROR_CODE;
			}
			else
			{
				g_triggerRecord.trec.sample_rate = (uint32)cfg.eventCfg.sampleRate;
			}
		}
		else
		{
			returnCode = CFG_ERR_SAMPLE_RATE;
			goto SEND_UCM_ERROR_CODE;
		}

		//---------------------------------------------------------------------------
		// Update air sensor type DB or MB
		//---------------------------------------------------------------------------
		if (cfg.extraUnitCfg.unitsOfAir == MILLIBAR_TYPE) { g_unitConfig.unitsOfAir = MILLIBAR_TYPE; }
		else if (cfg.extraUnitCfg.unitsOfAir == PSI_TYPE) {	g_unitConfig.unitsOfAir = PSI_TYPE; }
		else /* (cfg.extraUnitCfg.unitsOfAir == DECIBEL_TYPE) */ { g_unitConfig.unitsOfAir = DECIBEL_TYPE; }

		//---------------------------------------------------------------------------
		// A-weighting check
		//---------------------------------------------------------------------------
		if ((cfg.eventCfg.aWeighting == YES) || (cfg.eventCfg.aWeighting == NO))
		{
			if (g_factorySetupRecord.aWeightOption == ENABLED)
			{
				if (cfg.eventCfg.aWeighting == YES) { g_unitConfig.airScale = AIR_SCALE_A_WEIGHTING; }
				else { g_unitConfig.airScale = AIR_SCALE_LINEAR; }
			}
			else { g_unitConfig.airScale = AIR_SCALE_LINEAR; }
		}
		else
		{
			returnCode = CFG_ERR_A_WEIGHTING;
			goto SEND_UCM_ERROR_CODE;
		}

		//---------------------------------------------------------------------------
		// Air Trigger Level check (Changed Air trigger min check to allow 92 and 93 dB settings which are below normal minimum)
		//---------------------------------------------------------------------------
		// Check if the Air trigger level is within bounds, which is slighly different if the standard 148 dB mic is selected which uses a lower threshold
#if ENDIAN_CONVERSION
		cfg.eventCfg.airTriggerLevel = __builtin_bswap32(cfg.eventCfg.airTriggerLevel);
#endif
		if ((MANUAL_TRIGGER_CHAR == cfg.eventCfg.airTriggerLevel) || (NO_TRIGGER_CHAR == cfg.eventCfg.airTriggerLevel) ||
			((cfg.eventCfg.airTriggerLevel >= ((g_factorySetupRecord.acousticSensorType != SENSOR_MIC_148_DB) ? AIR_TRIGGER_MIN_COUNT_REMOTE_CONFIG : AIR_TRIGGER_MIN_COUNT_REMOTE_CONFIG_SPECIAL_92_DB)) &&
			(cfg.eventCfg.airTriggerLevel <= (uint32)AIR_TRIGGER_MAX_COUNT)))
		{
			g_triggerRecord.trec.airTriggerLevel = cfg.eventCfg.airTriggerLevel;
		}
		else
		{
			returnCode = CFG_ERR_SOUND_TRIG_LVL;
			goto SEND_UCM_ERROR_CODE;
		}

		//---------------------------------------------------------------------------
		// Record time check
		//---------------------------------------------------------------------------
		// Find Max Record time for a given sample rate. Max time is equal to
		// Size of the (event buff - size of Pretrigger buffer - size of calibration buff) 
		// divided by (sample rate * number of channels).
		// Number of channels is hard coded to 4 = NUMBER_OF_CHANNELS_DEFAULT 
		//---------------------------------------------------------------------------
		maxRecordTime = (uint16)(((uint32)((EVENT_BUFF_SIZE_IN_WORDS - 
				((g_triggerRecord.trec.sample_rate / g_unitConfig.pretrigBufferDivider) * NUMBER_OF_CHANNELS_DEFAULT) -
				((g_triggerRecord.trec.sample_rate / MIN_SAMPLE_RATE) * MAX_CAL_SAMPLES * NUMBER_OF_CHANNELS_DEFAULT)) / 
				(g_triggerRecord.trec.sample_rate * NUMBER_OF_CHANNELS_DEFAULT))));

#if ENDIAN_CONVERSION
		cfg.eventCfg.recordTime = __builtin_bswap32(cfg.eventCfg.recordTime);
#endif
		if ((cfg.eventCfg.recordTime >= 1) && (cfg.eventCfg.recordTime <= maxRecordTime))
		{
			g_triggerRecord.trec.record_time = cfg.eventCfg.recordTime;
		}
		else
		{
			returnCode = CFG_ERR_RECORD_TIME;
			goto SEND_UCM_ERROR_CODE;
		}
		
		//---------------------------------------------------------------------------
		// Bar Interval Level check
		//---------------------------------------------------------------------------
#if ENDIAN_CONVERSION
		cfg.eventCfg.barInterval = __builtin_bswap16(cfg.eventCfg.barInterval);
#endif
		switch (cfg.eventCfg.barInterval)
		{
			case ONE_SEC_PRD: 
			case TEN_SEC_PRD:
			case TWENTY_SEC_PRD:
			case THIRTY_SEC_PRD:
			case FOURTY_SEC_PRD:
			case FIFTY_SEC_PRD:
			case SIXTY_SEC_PRD:
				g_triggerRecord.bgrec.barInterval = (uint32)cfg.eventCfg.barInterval;
				break;

			default:
				returnCode = CFG_ERR_BAR_INTERVAL;
				goto SEND_UCM_ERROR_CODE;
				break;
		}

		//---------------------------------------------------------------------------
		// Bar Interval Level check
		//---------------------------------------------------------------------------
#if ENDIAN_CONVERSION
		cfg.eventCfg.summaryInterval = __builtin_bswap16(cfg.eventCfg.summaryInterval);
#endif
		switch (cfg.eventCfg.summaryInterval)
		{
			case FIVE_MINUTE_INTVL:
			case FIFTEEN_MINUTE_INTVL:
			case THIRTY_MINUTE_INTVL:
			case ONE_HOUR_INTVL:
			case TWO_HOUR_INTVL:
			case FOUR_HOUR_INTVL:
			case EIGHT_HOUR_INTVL:
			case TWELVE_HOUR_INTVL:
				g_triggerRecord.bgrec.summaryInterval = (uint32)cfg.eventCfg.summaryInterval;
				break;

			default:
				returnCode = CFG_ERR_SUM_INTERVAL;
				goto SEND_UCM_ERROR_CODE;
				break;
		}

		//---------------------------------------------------------------------------
		// Bar Interval Data Type check
		//---------------------------------------------------------------------------
		if ((cfg.eventCfg.barIntervalDataType == BAR_INTERVAL_A_R_V_T_DATA_TYPE_SIZE) || (cfg.eventCfg.barIntervalDataType == BAR_INTERVAL_A_R_V_T_WITH_FREQ_DATA_TYPE_SIZE))
		{
			g_triggerRecord.berec.barIntervalDataType = cfg.eventCfg.barIntervalDataType;
		}
		else
		{
			g_triggerRecord.berec.barIntervalDataType = BAR_INTERVAL_ORIGINAL_DATA_TYPE_SIZE;
		}

		//---------------------------------------------------------------------------
		// Copy Client, Operator, Location and Comments strings to default trigger record
		//---------------------------------------------------------------------------
		memcpy((uint8*)g_triggerRecord.trec.client, cfg.eventCfg.companyName, COMPANY_NAME_STRING_SIZE - 2);
		memcpy((uint8*)g_triggerRecord.trec.oper, cfg.eventCfg.seismicOperator, SEISMIC_OPERATOR_STRING_SIZE - 2);
		memcpy((uint8*)g_triggerRecord.trec.loc, cfg.eventCfg.sessionLocation, SESSION_LOCATION_STRING_SIZE - 2);
		memcpy((uint8*)g_triggerRecord.trec.comments, cfg.eventCfg.sessionComments, SESSION_COMMENTS_STRING_SIZE - 2);

		//---------------------------------------------------------------------------
		// Bar Scale check
		//---------------------------------------------------------------------------
		if ((cfg.extraUnitCfg.barScale == 1) || (cfg.extraUnitCfg.barScale == 2) || (cfg.extraUnitCfg.barScale == 4) || (cfg.extraUnitCfg.barScale == 8))
		{
			g_triggerRecord.berec.barScale = (uint8)cfg.extraUnitCfg.barScale;
		}
		else
		{
			returnCode = CFG_ERR_SCALING;
			goto SEND_UCM_ERROR_CODE;
		}

		//---------------------------------------------------------------------------
		// Bar Channel check
		//---------------------------------------------------------------------------
		if (cfg.extraUnitCfg.barChannel <= BAR_AIR_CHANNEL)
		{
#if 0 /* No longer using this option */
			g_triggerRecord.berec.barChannel = cfg.extraUnitCfg.barChannel;
#endif
		}
		else
		{
			returnCode = CFG_ERR_BAR_PRINT_CHANNEL;
			goto SEND_UCM_ERROR_CODE;
		}

		//---------------------------------------------------------------------------
		// Pretrigger Buffer check
		//---------------------------------------------------------------------------
		if ((cfg.eventCfg.pretrigBufferDivider == PRETRIGGER_BUFFER_QUARTER_SEC_DIV) || (cfg.eventCfg.pretrigBufferDivider == PRETRIGGER_BUFFER_HALF_SEC_DIV) ||
			(cfg.eventCfg.pretrigBufferDivider == PRETRIGGER_BUFFER_FULL_SEC_DIV))
		{
			g_unitConfig.pretrigBufferDivider = cfg.eventCfg.pretrigBufferDivider;
		}
		else
		{
			returnCode = CFG_ERR_PRETRIG_BUFFER_DIV;
			goto SEND_UCM_ERROR_CODE;
		}

		//---------------------------------------------------------------------------
		// Adjust for Temperature drift check
		//---------------------------------------------------------------------------
		if ((cfg.eventCfg.adjustForTempDrift == YES) || (cfg.eventCfg.adjustForTempDrift == NO))
		{
			g_triggerRecord.trec.adjustForTempDrift = cfg.eventCfg.adjustForTempDrift;
		}
		else
		{
			returnCode = CFG_ERR_TEMP_ADJUST;
			goto SEND_UCM_ERROR_CODE;
		}

#if 0 /* Move to this code block once SG can handle remotely */
		//---------------------------------------------------------------------------
		// AD Channel Verification
		//---------------------------------------------------------------------------
		if ((cfg.eventCfg.adChannelVerification == ENABLED) || (cfg.eventCfg.adChannelVerification == DISABLED))
		{
			g_unitConfig.adChannelVerification = cfg.eventCfg.adChannelVerification;
		}
		else
		{
			returnCode = CFG_ERR_CHANNEL_VERIFICATION;
			goto SEND_UCM_ERROR_CODE;
		}
#else /* Only check for enabled and default to disabled in all other cases */
		//---------------------------------------------------------------------------
		// AD Channel Verification
		//---------------------------------------------------------------------------
		if (cfg.eventCfg.adChannelVerification == ENABLED)
		{
			g_unitConfig.adChannelVerification = ENABLED;
		}
		else // Default to disabled
		{
			g_unitConfig.adChannelVerification = DISABLED;
		}
#endif

		//---------------------------------------------------------------------------
		// Auto Monitor Mode check
		//---------------------------------------------------------------------------
		switch (cfg.autoCfg.autoMonitorMode)
		{
			case AUTO_TWO_MIN_TIMEOUT:
			case AUTO_THREE_MIN_TIMEOUT:
			case AUTO_FOUR_MIN_TIMEOUT:
			case AUTO_NO_TIMEOUT:
				g_unitConfig.autoMonitorMode = cfg.autoCfg.autoMonitorMode;
				break;
				
			default:
				returnCode = CFG_ERR_AUTO_MON_MODE;
				goto SEND_UCM_ERROR_CODE;
				break;
		}

		//---------------------------------------------------------------------------
		// Auto Calibration Mode check
		//---------------------------------------------------------------------------
		switch (cfg.autoCfg.autoCalMode)
		{	
			case AUTO_24_HOUR_TIMEOUT:
			case AUTO_48_HOUR_TIMEOUT:
			case AUTO_72_HOUR_TIMEOUT:
			case AUTO_NO_CAL_TIMEOUT:
				g_unitConfig.autoCalMode = cfg.autoCfg.autoCalMode;
				break;
				
			default:
				returnCode = CFG_ERR_AUTO_CAL_MODE;
				goto SEND_UCM_ERROR_CODE;
				break;
		}

		//---------------------------------------------------------------------------
		// External Trigger check
		//---------------------------------------------------------------------------
		switch (cfg.autoCfg.externalTrigger)
		{
			case ENABLED:
			case DISABLED:
				g_unitConfig.externalTrigger = cfg.autoCfg.externalTrigger;
				break;

			default:
				returnCode = CFG_ERR_EXTERNAL_TRIGGER;
				goto SEND_UCM_ERROR_CODE;
				break;
		}

		//---------------------------------------------------------------------------
		// RS232 Power Savings check
		//---------------------------------------------------------------------------
		switch (cfg.autoCfg.rs232PowerSavings)
		{
			case ENABLED:
			case DISABLED:
				g_unitConfig.rs232PowerSavings = cfg.autoCfg.rs232PowerSavings;
			break;

			default:
				returnCode = CFG_ERR_RS232_POWER_SAVINGS;
				goto SEND_UCM_ERROR_CODE;
			break;
		}

		//---------------------------------------------------------------------------
		// Auto print
		//---------------------------------------------------------------------------
		if ((cfg.extraUnitCfg.autoPrint == NO) || (cfg.extraUnitCfg.autoPrint == YES))
		{
			g_unitConfig.autoPrint = cfg.extraUnitCfg.autoPrint;
		}
		else
		{
			returnCode = CFG_ERR_AUTO_PRINT;
			goto SEND_UCM_ERROR_CODE;
		}
		
		//---------------------------------------------------------------------------
		// Language Mode
		//---------------------------------------------------------------------------
		switch (cfg.extraUnitCfg.languageMode)
		{
			case ENGLISH_LANG:
			case FRENCH_LANG:
			case ITALIAN_LANG:
			case GERMAN_LANG:
			case SPANISH_LANG:
				if (g_unitConfig.languageMode != cfg.extraUnitCfg.languageMode)
				{
					g_unitConfig.languageMode = cfg.extraUnitCfg.languageMode;
					BuildLanguageLinkTable(g_unitConfig.languageMode);
				}
				break;
					
			default:
				returnCode = CFG_ERR_LANGUAGE_MODE;
				goto SEND_UCM_ERROR_CODE;
				break;	
		}

		//---------------------------------------------------------------------------
		// Vector Sum check
		//---------------------------------------------------------------------------
		if ((cfg.extraUnitCfg.vectorSum == NO) || (cfg.extraUnitCfg.vectorSum == YES))
		{
#if 0 /* No longer using this option */
			g_unitConfig.vectorSum = cfg.extraUnitCfg.vectorSum;
#endif
		}
		else
		{
			returnCode = CFG_ERR_VECTOR_SUM;
			goto SEND_UCM_ERROR_CODE;
		}
		
		//---------------------------------------------------------------------------
		// Units of Measure check
		//---------------------------------------------------------------------------
		if ((cfg.extraUnitCfg.unitsOfMeasure == IMPERIAL_TYPE) || (cfg.extraUnitCfg.unitsOfMeasure == METRIC_TYPE))
		{
			g_unitConfig.unitsOfMeasure = cfg.extraUnitCfg.unitsOfMeasure;
		}
		else
		{
			returnCode = CFG_ERR_UNITS_OF_MEASURE;
			goto SEND_UCM_ERROR_CODE;
		}
		
		//---------------------------------------------------------------------------
		// Frequency plot mode , Yes or No or On or Off
		//---------------------------------------------------------------------------
		if ((cfg.extraUnitCfg.freqPlotMode == NO) || (cfg.extraUnitCfg.freqPlotMode == YES))
		{
			g_unitConfig.freqPlotMode = cfg.extraUnitCfg.freqPlotMode;
		}
		else
		{
			returnCode = CFG_ERR_FREQ_PLOT_MODE;
			goto SEND_UCM_ERROR_CODE;
		}

		// Frequency plot mode , Yes or No or On or Off
		switch (cfg.extraUnitCfg.freqPlotType)
		{
			case FREQ_PLOT_US_BOM_STANDARD:
			case FREQ_PLOT_FRENCH_STANDARD:
			case FREQ_PLOT_DIN_4150_STANDARD:
			case FREQ_PLOT_BRITISH_7385_STANDARD:
			case FREQ_PLOT_SPANISH_STANDARD:
				g_unitConfig.freqPlotType = cfg.extraUnitCfg.freqPlotType;
				break;
					
			default:
				returnCode = CFG_ERR_FREQ_PLOT_TYPE;
				goto SEND_UCM_ERROR_CODE;
				break;	
		}

		//---------------------------------------------------------------------------
		// Alarm 1 check
		//---------------------------------------------------------------------------
		if (ALARM_MODE_OFF == cfg.alarmCfg.alarmOneMode)
		{
			g_unitConfig.alarmOneMode = ALARM_MODE_OFF;
		}
		else
		{
#if ENDIAN_CONVERSION
			cfg.alarmCfg.alarmOneSeismicLevel = __builtin_bswap32(cfg.alarmCfg.alarmOneSeismicLevel);
			cfg.alarmCfg.alarmOneSeismicMinLevel = __builtin_bswap32(cfg.alarmCfg.alarmOneSeismicMinLevel);
			cfg.alarmCfg.alarmOneAirLevel = __builtin_bswap32(cfg.alarmCfg.alarmOneAirLevel);
			cfg.alarmCfg.alarmOneAirMinLevel = __builtin_bswap32(cfg.alarmCfg.alarmOneAirMinLevel);
			cfg.alarmCfg.alarmOneTime = __builtin_bswap32(cfg.alarmCfg.alarmOneTime);
#endif
			// Alarm One mode
			switch (cfg.alarmCfg.alarmOneMode)
			{
				case ALARM_MODE_OFF:
				case ALARM_MODE_SEISMIC:
				case ALARM_MODE_AIR:
				case ALARM_MODE_BOTH:
					g_unitConfig.alarmOneMode = cfg.alarmCfg.alarmOneMode;
				break;
						
				default:
					g_unitConfig.alarmOneMode = ALARM_MODE_OFF;
					returnCode = CFG_ERR_ALARM_ONE_MODE;
					goto SEND_UCM_ERROR_CODE;
				break;
			}

			if ((ALARM_MODE_SEISMIC == g_unitConfig.alarmOneMode) || (ALARM_MODE_BOTH == g_unitConfig.alarmOneMode))
			{
				// Alarm One Seismic trigger level check for Waveform mode only
				if ((cfg.mode == WAVEFORM_MODE) && ((NO_TRIGGER_CHAR == cfg.alarmCfg.alarmOneSeismicLevel) ||
					((cfg.alarmCfg.alarmOneSeismicLevel >= g_triggerRecord.trec.seismicTriggerLevel) &&
					(cfg.alarmCfg.alarmOneSeismicLevel <= SEISMIC_TRIGGER_MAX_VALUE))))
				{
					g_unitConfig.alarmOneSeismicLevel = cfg.alarmCfg.alarmOneSeismicLevel;
				}
				// Alarm One Seismic trigger level check for other modes
				else if ((NO_TRIGGER_CHAR == cfg.alarmCfg.alarmOneSeismicLevel) 	||
						((cfg.alarmCfg.alarmOneSeismicLevel >= SEISMIC_TRIGGER_MIN_VALUE) &&
						(cfg.alarmCfg.alarmOneSeismicLevel <= SEISMIC_TRIGGER_MAX_VALUE)))
				{
					g_unitConfig.alarmOneSeismicLevel = cfg.alarmCfg.alarmOneSeismicLevel;
				}
				else
				{					
					returnCode = CFG_ERR_ALARM_ONE_SEISMIC_LVL;
					goto SEND_UCM_ERROR_CODE;
				}
			}

			if ((ALARM_MODE_AIR == g_unitConfig.alarmOneMode) || (ALARM_MODE_BOTH == g_unitConfig.alarmOneMode))
			{
				// Alarm One Air trigger level check DB/MB for Waveform mode only
				if ((cfg.mode == WAVEFORM_MODE) && ((NO_TRIGGER_CHAR == cfg.alarmCfg.alarmOneAirLevel) ||
					((cfg.alarmCfg.alarmOneAirLevel >= g_triggerRecord.trec.airTriggerLevel) &&
					(cfg.alarmCfg.alarmOneAirLevel <= AIR_TRIGGER_MAX_COUNT))))
				{
					g_unitConfig.alarmOneAirLevel = cfg.alarmCfg.alarmOneAirLevel;
				}
				// Alarm One Air trigger level check DB/MB for other modes
				else if ((NO_TRIGGER_CHAR == cfg.alarmCfg.alarmOneAirLevel) || ((cfg.alarmCfg.alarmOneAirLevel >= AIR_TRIGGER_MIN_COUNT_REMOTE_CONFIG) &&
						(cfg.alarmCfg.alarmOneAirLevel <= AIR_TRIGGER_MAX_COUNT)))
				{
					g_unitConfig.alarmOneAirLevel = cfg.alarmCfg.alarmOneAirLevel;
				}
				else
				{
					returnCode = CFG_ERR_ALARM_ONE_SOUND_LVL;
					goto SEND_UCM_ERROR_CODE;
				}
			}

			// Alarm One Time check
			timeAlarmFloat = (float)((float)cfg.alarmCfg.alarmOneTime/(float)100.0);
			if (	(timeAlarmFloat >= (float)ALARM_OUTPUT_TIME_MIN) && 
				(timeAlarmFloat <= (float)ALARM_OUTPUT_TIME_MAX))
			{
				timeCheck = (uint32)timeAlarmFloat;
				timeCheckFloat = (float)timeAlarmFloat - (float)timeCheck;
				if ((timeCheckFloat == (float)0.0) || (timeCheckFloat == (float)0.5))
				{
					g_unitConfig.alarmOneTime = (float)timeAlarmFloat;
				}
				else
				{
					returnCode = CFG_ERR_ALARM_ONE_TIME;
					goto SEND_UCM_ERROR_CODE;
				}
			}
			else
			{
				returnCode = CFG_ERR_ALARM_ONE_TIME;
				goto SEND_UCM_ERROR_CODE;
			}
		}			

		//---------------------------------------------------------------------------
		// Alarm 2 check
		//---------------------------------------------------------------------------
		if (ALARM_MODE_OFF == cfg.alarmCfg.alarmTwoMode)
		{
			g_unitConfig.alarmTwoMode = ALARM_MODE_OFF;
		}
		else
		{
#if ENDIAN_CONVERSION
			cfg.alarmCfg.alarmTwoSeismicLevel = __builtin_bswap32(cfg.alarmCfg.alarmTwoSeismicLevel);
			cfg.alarmCfg.alarmTwoSeismicMinLevel = __builtin_bswap32(cfg.alarmCfg.alarmTwoSeismicMinLevel);
			cfg.alarmCfg.alarmTwoAirLevel = __builtin_bswap32(cfg.alarmCfg.alarmTwoAirLevel);
			cfg.alarmCfg.alarmTwoAirMinLevel = __builtin_bswap32(cfg.alarmCfg.alarmTwoAirMinLevel);
			cfg.alarmCfg.alarmTwoTime = __builtin_bswap32(cfg.alarmCfg.alarmTwoTime);
#endif
			// Alarm Two mode
			switch (cfg.alarmCfg.alarmTwoMode)
			{
				case ALARM_MODE_OFF:
				case ALARM_MODE_SEISMIC:
				case ALARM_MODE_AIR:
				case ALARM_MODE_BOTH:
					g_unitConfig.alarmTwoMode = cfg.alarmCfg.alarmTwoMode;
					break;
						
				default:
					g_unitConfig.alarmTwoMode = ALARM_MODE_OFF;
					returnCode = CFG_ERR_ALARM_TWO_MODE;
					goto SEND_UCM_ERROR_CODE;
					break;	
			}

			if ((ALARM_MODE_SEISMIC == g_unitConfig.alarmTwoMode) || (ALARM_MODE_BOTH == g_unitConfig.alarmTwoMode))
			{
				// Alarm Two Seismic trigger level check for Waveform mode only
				if ((cfg.mode == WAVEFORM_MODE) && ((NO_TRIGGER_CHAR == cfg.alarmCfg.alarmTwoSeismicLevel) ||
					((cfg.alarmCfg.alarmTwoSeismicLevel >= g_triggerRecord.trec.seismicTriggerLevel) &&
					(cfg.alarmCfg.alarmTwoSeismicLevel <= SEISMIC_TRIGGER_MAX_VALUE))))
				{
					g_unitConfig.alarmTwoSeismicLevel = cfg.alarmCfg.alarmTwoSeismicLevel;
				}
				// Alarm Two Seismic trigger level check for other modes
				else if ((NO_TRIGGER_CHAR == cfg.alarmCfg.alarmTwoSeismicLevel) ||
						((cfg.alarmCfg.alarmTwoSeismicLevel >= g_triggerRecord.trec.seismicTriggerLevel) &&
						(cfg.alarmCfg.alarmTwoSeismicLevel <= SEISMIC_TRIGGER_MAX_VALUE)))
				{
					g_unitConfig.alarmTwoSeismicLevel = cfg.alarmCfg.alarmTwoSeismicLevel;
				}
				else
				{
					returnCode = CFG_ERR_ALARM_TWO_SEISMIC_LVL;
					goto SEND_UCM_ERROR_CODE;
				}
			}
			
			if ((ALARM_MODE_AIR == g_unitConfig.alarmTwoMode) || (ALARM_MODE_BOTH == g_unitConfig.alarmTwoMode))
			{
				// Alarm Two Air trigger level check DB/MB for Waveform mode only
				if ((cfg.mode == WAVEFORM_MODE) && ((NO_TRIGGER_CHAR == cfg.alarmCfg.alarmTwoAirLevel) ||
					((cfg.alarmCfg.alarmTwoAirLevel >= g_triggerRecord.trec.airTriggerLevel) &&
					(cfg.alarmCfg.alarmTwoAirLevel <= AIR_TRIGGER_MAX_COUNT))))
				{
					g_unitConfig.alarmTwoAirLevel = cfg.alarmCfg.alarmTwoAirLevel;
				}
				// Alarm Two Air trigger level check DB/MB for other modes
				else if ((NO_TRIGGER_CHAR == cfg.alarmCfg.alarmTwoAirLevel) || ((cfg.alarmCfg.alarmTwoAirLevel >= AIR_TRIGGER_MIN_COUNT_REMOTE_CONFIG) &&
						(cfg.alarmCfg.alarmTwoAirLevel <= AIR_TRIGGER_MAX_COUNT)))
				{
					g_unitConfig.alarmTwoAirLevel = cfg.alarmCfg.alarmTwoAirLevel;
				}
				else
				{
					returnCode = CFG_ERR_ALARM_TWO_SOUND_LVL;
					goto SEND_UCM_ERROR_CODE;
				}
			}

			// Alarm Two Time check
			timeAlarmFloat = (float)((float)cfg.alarmCfg.alarmTwoTime/(float)100.0);
			if (	(timeAlarmFloat >= (float)ALARM_OUTPUT_TIME_MIN) && 
				(timeAlarmFloat <= (float)ALARM_OUTPUT_TIME_MAX))
			{
				timeCheck = (uint32)timeAlarmFloat;
				timeCheckFloat = (float)timeAlarmFloat - (float)timeCheck;
				if ((timeCheckFloat == (float)0.0) || (timeCheckFloat == (float)0.5))
				{
					g_unitConfig.alarmTwoTime = timeAlarmFloat;
				}
				else
				{
					returnCode = CFG_ERR_ALARM_TWO_TIME;
					goto SEND_UCM_ERROR_CODE;
				}
			}
			else
			{
				returnCode = CFG_ERR_ALARM_TWO_TIME;
				goto SEND_UCM_ERROR_CODE;
			}
		}

		//---------------------------------------------------------------------------
		// Legacy DQM Limit
		//---------------------------------------------------------------------------
		if (cfg.alarmCfg.legacyDqmLimit == ENABLED)
		{
			g_unitConfig.legacyDqmLimit = ENABLED;
		}

		//---------------------------------------------------------------------------
		// Stored Event Cap & Limit
		//---------------------------------------------------------------------------
		if (cfg.alarmCfg.storedEventsCapMode == ENABLED)
		{
#if ENDIAN_CONVERSION
			cfg.alarmCfg.storedEventLimit = __builtin_bswap16(cfg.alarmCfg.storedEventLimit);
#endif
			if ((cfg.alarmCfg.storedEventLimit >= STORED_EVENT_LIMIT_MIN_VALUE) && (cfg.alarmCfg.storedEventLimit <= STORED_EVENT_LIMIT_MAX_VALUE))
			{
				g_unitConfig.storedEventsCapMode = ENABLED;
				g_unitConfig.storedEventLimit = cfg.alarmCfg.storedEventLimit;
			}
			else
			{
				returnCode = CFG_ERR_STORED_EVT_LIMIT_RANGE;
				goto SEND_UCM_ERROR_CODE;
			}
		}

		//---------------------------------------------------------------------------
		// Cycle End Time Hour (24hr)
		//---------------------------------------------------------------------------
		// Check if cycle end time hour is non-zero and valid
		if ((cfg.timerCfg.cycleEndTimeHour) && (cfg.timerCfg.cycleEndTimeHour < 25)) // Valid hours 1 to 24
		{
			// Update the unit config
			if (cfg.timerCfg.cycleEndTimeHour == 24) { g_unitConfig.cycleEndTimeHour = 0; }
			else { g_unitConfig.cycleEndTimeHour = cfg.timerCfg.cycleEndTimeHour; }
		}
#if 0 /* For now ignore changing the unit's value to a default if out of range */
		else // Set a default
		{
			g_unitConfig.cycleEndTimeHour = 0;
		}
#endif

		//---------------------------------------------------------------------------
		// Timer mode check
		//---------------------------------------------------------------------------
		if (!((DISABLED == cfg.timerCfg.timerMode) && (DISABLED == g_unitConfig.timerMode)))
		{
			timerModeModified = TRUE;
			
			// Timer Mode check
			if (ENABLED == cfg.timerCfg.timerMode)
			{
				// Timer Mode Frequency check
				switch (cfg.timerCfg.timerModeFrequency)
				{
					case TIMER_MODE_ONE_TIME:
					case TIMER_MODE_DAILY:
					case TIMER_MODE_WEEKDAYS:
					case TIMER_MODE_WEEKLY:
					case TIMER_MODE_MONTHLY:
					case TIMER_MODE_HOURLY:
						break;
					
					default:
						returnCode = CFG_ERR_TIMER_MODE_FREQ;
						goto SEND_UCM_ERROR_CODE;
						break;
				}

				// time valid value check.
				if (!((cfg.timerCfg.timer_start.hour < 24) && 
					 (cfg.timerCfg.timer_start.min < 60) &&
					 (cfg.timerCfg.timer_start.year < 100) &&
					 (cfg.timerCfg.timer_start.month >= 1) && (cfg.timerCfg.timer_start.month <= 12) &&
					 (cfg.timerCfg.timer_start.day >= 1) && (cfg.timerCfg.timer_start.day < 32)))
				{
					returnCode = CFG_ERR_START_TIME;
					goto SEND_UCM_ERROR_CODE;
				}

				if (!((cfg.timerCfg.timer_stop.hour < 24) && 
					 (cfg.timerCfg.timer_stop.min < 60) &&
					 (cfg.timerCfg.timer_stop.year < 100) &&
					 (cfg.timerCfg.timer_stop.month >= 1) && (cfg.timerCfg.timer_stop.month <= 12) &&
					 (cfg.timerCfg.timer_stop.day >= 1) && (cfg.timerCfg.timer_stop.day < 32)))
				{
					returnCode = CFG_ERR_STOP_TIME;
					goto SEND_UCM_ERROR_CODE;
				}
				
				if ((CFG_ERR_START_TIME != returnCode) && (CFG_ERR_STOP_TIME != returnCode) &&
					(CFG_ERR_TIMER_MODE_FREQ != returnCode))
				{
					g_unitConfig.timerModeFrequency = cfg.timerCfg.timerModeFrequency;
					g_unitConfig.timerStopTime.hour = cfg.timerCfg.timer_stop.hour;
					g_unitConfig.timerStopTime.min = cfg.timerCfg.timer_stop.min;
					g_unitConfig.timerStopDate.day = cfg.timerCfg.timer_stop.day;
					g_unitConfig.timerStopDate.month = cfg.timerCfg.timer_stop.month;
					g_unitConfig.timerStopDate.year = cfg.timerCfg.timer_stop.year;

					g_unitConfig.timerStartTime.hour = cfg.timerCfg.timer_start.hour;
					g_unitConfig.timerStartTime.min = cfg.timerCfg.timer_start.min;
					g_unitConfig.timerStartDate.day = cfg.timerCfg.timer_start.day;
					g_unitConfig.timerStartDate.month = cfg.timerCfg.timer_start.month;
					g_unitConfig.timerStartDate.year = cfg.timerCfg.timer_start.year;

					g_unitConfig.timerMode = cfg.timerCfg.timerMode;
				}					
			}
			else if (DISABLED == cfg.timerCfg.timerMode)
			{
				// Setting it to disabled, no error check needed.
				g_unitConfig.timerMode = cfg.timerCfg.timerMode;
			}
			else
			{	
				// In valid value for the timer mode, return an error
				returnCode = CFG_ERR_TIMER_MODE;
				goto SEND_UCM_ERROR_CODE;
			}
		}

		//---------------------------------------------------------------------------
		// Check to make sure a valid trigger source is selected
		//---------------------------------------------------------------------------
		if ((g_unitConfig.externalTrigger == DISABLED) && (g_triggerRecord.trec.seismicTriggerLevel == NO_TRIGGER_CHAR) &&
			(g_triggerRecord.trec.airTriggerLevel == NO_TRIGGER_CHAR))
		{
			// No trigger source was selected which is a config error
			returnCode = CFG_ERR_NO_TRIGGER_SOURCE;
			goto SEND_UCM_ERROR_CODE;
		}

		//---------------------------------------------------------------------------
		// Check if the Supergraphics
		//---------------------------------------------------------------------------
		// Endian conversion happens in first reference above (if enabled)
#if ENDIAN_CONVERSION
		cfg.sgVersion = __builtin_bswap16(cfg.sgVersion);
#endif
		if (cfg.sgVersion)
		{
			if ((cfg.flashWrapping == NO) || (cfg.flashWrapping == YES))
			{			
#if 0 /* Normal */
				g_unitConfig.flashWrapping = cfg.flashWrapping;
#else /* Forcing flash wrapping to be disabled */
				g_unitConfig.flashWrapping = NO;
#endif
			}
			else
			{
				// Invalid value for the flash wrapping option
				returnCode = CFG_ERR_FLASH_WRAPPING;
				goto SEND_UCM_ERROR_CODE;
			}
		}

		//---------------------------------------------------------------------------
		// Check if no errors to this point
		//---------------------------------------------------------------------------
		if (returnCode == CFG_ERR_NONE)
		{
			// Saved updated settings
			SaveRecordData(&g_triggerRecord, DEFAULT_RECORD, REC_TRIGGER_USER_MENU_TYPE);
			SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);
		}
	}

	if (TRUE == timerModeModified)
	{
		ProcessTimerModeSettings(NO_PROMPT);
		// If the user wants to enable, but it is now disable send an error.
		if (DISABLED == g_unitConfig.timerMode)
		{
			returnCode = CFG_ERR_TIMER_MODE;
		}
	}
	
SEND_UCM_ERROR_CODE:
	if (returnCode != CFG_ERR_NONE)
	{
		debug("UCM error encountered: %d\r\n", returnCode);
	}

	// -------------------------------------
	// Return codes
#if ENDIAN_CONVERSION
	//returnCode = __builtin_bswap32(returnCode);
#endif
	sprintf((char*)msgTypeStr, "%02lu", returnCode);
	BuildOutgoingSimpleHeaderBuffer((uint8*)ucmHdr, (uint8*)"UCMx",
		(uint8*)msgTypeStr, MESSAGE_SIMPLE_TOTAL_LENGTH, COMPRESS_NONE, CRC_NONE);	

	// Send Starting CRLF
	ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);

	// Calculate the CRC on the header
	g_transmitCRC = CalcCCITT32((uint8*)&ucmHdr, MESSAGE_HEADER_SIMPLE_LENGTH, SEED_32);		

	// Send Simple header
	ModemPuts((uint8*)&ucmHdr, MESSAGE_HEADER_SIMPLE_LENGTH, CONVERT_DATA_TO_ASCII);

	// Send Ending Footer
#if ENDIAN_CONVERSION
	g_transmitCRC = __builtin_bswap32(g_transmitCRC);
#endif
	ModemPuts((uint8*)&g_transmitCRC, 4, NO_CONVERSION);
	ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);

	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleDMM(CMD_BUFFER_STRUCT* inCmd)
{
	MODEM_SETUP_STRUCT modemCfg;
	uint8 dmmHdr[MESSAGE_HEADER_SIMPLE_LENGTH];
	uint8 msgTypeStr[HDR_TYPE_LEN+2];

	UNUSED(inCmd);

	memset(&modemCfg, 0, sizeof(modemCfg));

	memcpy((uint8*)&modemCfg, &g_modemSetupRecord, sizeof(MODEM_SETUP_STRUCT));
#if ENDIAN_CONVERSION
	EndianSwapModemSetupStruct(&modemCfg);
#endif

	sprintf((char*)msgTypeStr, "%02d", MSGTYPE_RESPONSE);
	BuildOutgoingSimpleHeaderBuffer((uint8*)dmmHdr, (uint8*)"DMMx", (uint8*)msgTypeStr,
		(uint32)(MESSAGE_SIMPLE_TOTAL_LENGTH + sizeof(MODEM_SETUP_STRUCT)), COMPRESS_NONE, CRC_NONE);	

	// Send Starting CRLF
	ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);

	// Calculate the CRC on the header
	g_transmitCRC = CalcCCITT32((uint8*)&dmmHdr, MESSAGE_HEADER_SIMPLE_LENGTH, SEED_32);		

	// Send Simple header
	ModemPuts((uint8*)dmmHdr, MESSAGE_HEADER_SIMPLE_LENGTH, g_binaryXferFlag);

	// Calculate the CRC on the data
	g_transmitCRC = CalcCCITT32((uint8*)&modemCfg, sizeof(MODEM_SETUP_STRUCT), g_transmitCRC);		

	// Send the configuration data
	ModemPuts((uint8*)&modemCfg, sizeof(MODEM_SETUP_STRUCT), g_binaryXferFlag);

	// Ending Footer
#if ENDIAN_CONVERSION
	g_transmitCRC = __builtin_bswap32(g_transmitCRC);
#endif
	ModemPuts((uint8*)&g_transmitCRC, 4, NO_CONVERSION);
	ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleUMM(CMD_BUFFER_STRUCT* inCmd)
{
	MODEM_SETUP_STRUCT modemCfg;
	uint8* modemCfgPtr = (uint8*)(&modemCfg);
	uint16 i = 0, j = 0;
	uint32 returnCode = CFG_ERR_NONE;
	uint32 msgCRC = 0;
	uint32 inCRC = 0;
	uint8 ummHdr[MESSAGE_HEADER_SIMPLE_LENGTH];
	uint8 msgTypeStr[HDR_TYPE_LEN+2];

	memset(&modemCfg, 0, sizeof(modemCfg));

	// Move the string data into the configuration structure. String is (2 * cfgSize)
	i = MESSAGE_HEADER_SIMPLE_LENGTH;
	while ((i < inCmd->size) && (i < (MESSAGE_HEADER_SIMPLE_LENGTH + (sizeof(MODEM_SETUP_STRUCT) * 2))) && 
			(i < CMD_BUFFER_SIZE))
	{	
		*modemCfgPtr++ = ConvertAscii2Binary(inCmd->msg[i], inCmd->msg[i + 1]);
		i += 2;
	}		

	// Get the CRC value from the data stream
	while ((i < inCmd->size) && (i < CMD_BUFFER_SIZE) && (j < 4))
	{
		((uint8*)&inCRC)[j++] = ConvertAscii2Binary(inCmd->msg[i], inCmd->msg[i + 1]);
		i += 2;
	}

	// Compare CRC transmitted CRC value with the CRC calculation on the data and check if not equal
	msgCRC = CalcCCITT32((uint8*)&(inCmd->msg[0]), MESSAGE_HEADER_SIMPLE_LENGTH, SEED_32);
	msgCRC = CalcCCITT32((uint8*)&modemCfg, sizeof(MODEM_SETUP_STRUCT), msgCRC);
#if ENDIAN_CONVERSION
	msgCRC = __builtin_bswap32(msgCRC);
	debug("UMM CRC compare: In %08x, Msg %08x\r\n", inCRC, msgCRC);
#endif

	if (inCRC != msgCRC)
	{
		// Signal a bad CRC value
		returnCode = CFG_ERR_BAD_CRC;
	}
	else
	{
#if ENDIAN_CONVERSION
		EndianSwapModemSetupStruct(&modemCfg);
#endif
		if ((modemCfg.modemStatus != NO) && (modemCfg.modemStatus != YES))
		{
			returnCode = CFG_ERR_MODEM_CONFIG;
		}
		else if (modemCfg.unlockCode > 9999)
		{
			returnCode = CFG_ERR_MODEM_CONFIG;
		}
		else if ((modemCfg.retries < 1) || (modemCfg.retries > 9))
		{
			returnCode = CFG_ERR_MODEM_CONFIG;
		}
		else if ((modemCfg.retryTime < 1) || (modemCfg.retryTime > 60))
		{
			returnCode = CFG_ERR_MODEM_CONFIG;
		}
#if 0 /* Original */
		else if (strlen(modemCfg.init) > 64)
		{
			returnCode = CFG_ERR_MODEM_CONFIG;
		}
#else /* New Auto Dial Out Events/Config/Status */
		else if (strlen(modemCfg.init) > 60)
		{
			returnCode = CFG_ERR_MODEM_CONFIG;
		}
		else if (modemCfg.dialOutType > AUTODIALOUT_EVENTS_CONFIG_STATUS)
		{
			returnCode = CFG_ERR_MODEM_CONFIG;
		}
		else if (modemCfg.dialOutCycleTime > MODEM_DIAL_OUT_TIMER_MAX_VALUE)
		{
			returnCode = CFG_ERR_MODEM_CONFIG;
		}
#endif
		else if (strlen(modemCfg.dial) > 32)
		{
			returnCode = CFG_ERR_MODEM_CONFIG;
		}
		else if (strlen(modemCfg.reset) > 16)
		{
			returnCode = CFG_ERR_MODEM_CONFIG;
		}
		
		if (returnCode == CFG_ERR_NONE)
		{
			if (modemCfg.dialOutCycleTime == 0) { modemCfg.dialOutCycleTime = MODEM_DIAL_OUT_TIMER_DEFAULT_VALUE; }

			g_modemSetupRecord = modemCfg;

			SaveRecordData(&g_modemSetupRecord, DEFAULT_RECORD, REC_MODEM_SETUP_TYPE);
		}
	}

	// -------------------------------------
	// Return codes
	sprintf((char*)msgTypeStr, "%02lu", returnCode);
	BuildOutgoingSimpleHeaderBuffer((uint8*)ummHdr, (uint8*)"UMMx",
		(uint8*)msgTypeStr, MESSAGE_SIMPLE_TOTAL_LENGTH, COMPRESS_NONE, CRC_NONE);	

	// Send Starting CRLF
	ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);

	// Calculate the CRC on the header
	g_transmitCRC = CalcCCITT32((uint8*)&ummHdr, MESSAGE_HEADER_SIMPLE_LENGTH, SEED_32);		

	// Send Simple header
	ModemPuts((uint8*)ummHdr, MESSAGE_HEADER_SIMPLE_LENGTH, CONVERT_DATA_TO_ASCII);

	// Send Ending Footer
#if ENDIAN_CONVERSION
	g_transmitCRC = __builtin_bswap32(g_transmitCRC);
#endif
	ModemPuts((uint8*)&g_transmitCRC, 4, NO_CONVERSION);
	ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 ConvertAscii2Binary(uint8 firstByte, uint8 secondByte)
{
	uint8 binaryByte = 0;
	uint8 nibble1 = 0;
	uint8 nibble2 = 0;

	// Get the first nibble
	if ((firstByte >= 0x30) && (firstByte <= 0x39))
		nibble1 = (uint8)(firstByte - 0x30);
	else if ((firstByte >= 0x41) && (firstByte <= 0x46))
		nibble1 = (uint8)(firstByte - 0x37);
	
	// Get the second nibble
	if ((secondByte >= 0x30) && (secondByte <= 0x39))
		nibble2 = (uint8)(secondByte - 0x30);
	else if ((secondByte >= 0x41) && (secondByte <= 0x46))
		nibble2 = (uint8)(secondByte - 0x37);

	// Put it into the byte.
	binaryByte = (uint8)((uint8)(nibble1 << 4) + (uint8)nibble2);
	
	return (binaryByte);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleVTI(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleSTI(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleVDA(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleSDA(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleZRO(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void handleTTO(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleCAL(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	uint32 returnCode = CFG_ERR_NONE;
	uint8 calHdr[MESSAGE_HEADER_SIMPLE_LENGTH];
	uint8 msgTypeStr[HDR_TYPE_LEN+2];
	uint8 msgResults[8];
	float div;
	float normalizedMaxPeak;
	uint8 calResults = PASSED;

	// Check if idle or actively monitoring in Waveform mode and not busy processing an event
	if ((g_sampleProcessing == IDLE_STATE) || ((g_sampleProcessing == ACTIVE_STATE) && (g_triggerRecord.opMode == WAVEFORM_MODE) && (g_busyProcessingEvent == NO) && (!getSystemEventState(TRIGGER_EVENT))))
	{
		// Check if there is room to store a calibration event
		if ((g_unitConfig.flashWrapping == YES) || (g_sdCardUsageStats.manualCalsLeft != 0))
		{
			HandleManualCalibration();

			// Calculate the divider used for converting stored A/D peak counts to units of measure
			if ((g_factorySetupRecord.seismicSensorType == SENSOR_ACC_832M1_0200) || (g_factorySetupRecord.seismicSensorType == SENSOR_ACC_832M1_0500))
			{
				div = (float)(g_bitAccuracyMidpoint * SENSOR_ACCURACY_100X_SHIFT * 2 /* normal gain */) / (float)(g_summaryList.cachedEntry.seismicSensorType * ACC_832M1_SCALER);
			}
			else div = (float)(g_bitAccuracyMidpoint * SENSOR_ACCURACY_100X_SHIFT * 2 /* normal gain */) / (float)(g_summaryList.cachedEntry.seismicSensorType);

			normalizedMaxPeak = (float)g_pendingEventRecord.summary.calculated.r.peak / (float)div; if ((normalizedMaxPeak < 0.375) || (normalizedMaxPeak > 0.625)) { calResults = FAILED; }
			normalizedMaxPeak = (float)g_pendingEventRecord.summary.calculated.v.peak / (float)div; if ((normalizedMaxPeak < 0.375) || (normalizedMaxPeak > 0.625)) { calResults = FAILED; }
			normalizedMaxPeak = (float)g_pendingEventRecord.summary.calculated.t.peak / (float)div; if ((normalizedMaxPeak < 0.375) || (normalizedMaxPeak > 0.625)) { calResults = FAILED; }
		}
	}
	else // Unable to perform a manual Cal
	{
		returnCode = CFG_ERR_MONITORING_STATE;
	}

	sprintf((char*)msgTypeStr, "%02lu", returnCode);
	BuildOutgoingSimpleHeaderBuffer((uint8*)calHdr, (uint8*)"CALx", (uint8*)msgTypeStr, (returnCode == CFG_ERR_NONE) ? (MESSAGE_SIMPLE_TOTAL_LENGTH + 4) : (MESSAGE_SIMPLE_TOTAL_LENGTH), COMPRESS_NONE, CRC_NONE);

	// Send Starting CRLF
	ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);

	// Calculate the CRC on the header
	g_transmitCRC = CalcCCITT32((uint8*)&calHdr, MESSAGE_HEADER_SIMPLE_LENGTH, SEED_32);

	// Send Simple header
	ModemPuts((uint8*)calHdr, MESSAGE_HEADER_SIMPLE_LENGTH, NO_CONVERSION);

	// Check if manual cal completed
	if (returnCode == CFG_ERR_NONE)
	{
		if (calResults == PASSED) { strcpy((char*)msgResults, "PASS"); }
		else { strcpy((char*)msgResults, "FAIL"); }

		// Calculate the CRC on the payload
		g_transmitCRC = CalcCCITT32((uint8*)&msgResults, 4, SEED_32);

		// Send payload
		ModemPuts((uint8*)msgResults, 4, NO_CONVERSION);
	}

	// Send Ending Footer
#if ENDIAN_CONVERSION
	g_transmitCRC = __builtin_bswap32(g_transmitCRC);
#endif
	ModemPuts((uint8*)&g_transmitCRC, 4, NO_CONVERSION);
	ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);

	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleVOL(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleVCG(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleVSL(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleVEL(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleVBD(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleSBD(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleVDT(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleSDT(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleVCL(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleSCL(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ModemInitProcess(void)
{
	if (g_modemStatus.testingFlag == YES) g_disableDebugPrinting = NO;

	debug("ModemInitProcess\r\n");

	if (READ_DCD == NO_CONNECTION)
	{
		if (strlen(g_modemSetupRecord.reset) != 0)
		{
			UartPuts((char*)(g_modemSetupRecord.reset), CRAFT_COM_PORT);
			UartPuts((char*)&g_CRLF, CRAFT_COM_PORT);

			SoftUsecWait(3 * SOFT_SECS);
		}

		if (strlen(g_modemSetupRecord.init) != 0)
		{
			UartPuts((char*)(g_modemSetupRecord.init), CRAFT_COM_PORT);
			UartPuts((char*)&g_CRLF, CRAFT_COM_PORT);
		}
	}

	// Assume connected.
	g_modemStatus.numberOfRings = 0;
	g_modemStatus.ringIndicator = 0;

	g_modemStatus.connectionState = CONNECTED;
	g_modemStatus.firstConnection = NOP_CMD;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ModemResetProcess(void)
{
	if (g_modemStatus.testingFlag == YES) g_disableDebugPrinting = NO;
	debug("HandleMRC\r\n");

	WaitForBargraphLiveMonitoringDataToFinishSendingWithTimeout();

	g_modemStatus.systemIsLockedFlag = YES;

	if (g_autoRetries == 0)
	{
		CLEAR_DTR;
	}	

	g_modemResetStage = 1;
	AssignSoftTimer(MODEM_RESET_TIMER_NUM, (uint32)(15 * TICKS_PER_SEC), ModemResetTimerCallback);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleMRS(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);

	g_modemStatus.systemIsLockedFlag = YES;

	if (YES == g_modemSetupRecord.modemStatus)
	{
		g_autoRetries = 0;
		ModemResetProcess();
	}

	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleMVS(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleMPO(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleMMO(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleMNO(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleMTO(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleMSD(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleMSR(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleMST(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleMSA(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleMSB(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleMSC(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleMVI(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleMVO(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

