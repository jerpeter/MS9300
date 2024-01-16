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
#include "Board.h"
#include "Record.h"
#include "OldUart.h"
#include "Menu.h"
#include "SysEvents.h"
#include "TextTypes.h"
#include "string.h"
#include "Common.h"

#include "ff.h"
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

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitMonitorLog(void)
{
	// Check if the table key is not valid or if the table index is not within range
	if ((__monitorLogTblKey != VALID_MONITOR_LOG_TABLE_KEY) || (__monitorLogTblIndex >= TOTAL_MONITOR_LOG_ENTRIES))
	{
		//debugRaw("Clearing Monitor Log table\r\n");

		// Clear Monitor Log table
		memset(&__monitorLogTbl[0], 0, sizeof(__monitorLogTbl));

		// Set the index to the first element
		__monitorLogTblIndex = 0;

		// Set the table key to be valid
		__monitorLogTblKey = VALID_MONITOR_LOG_TABLE_KEY;
		
		InitMonitorLogUniqueEntryId();

		InitMonitorLogTableFromLogFile();
	}
	// Check if the current monitor log entry is a partial entry (suggesting the entry was not closed)
	else if (__monitorLogTbl[__monitorLogTblIndex].status == PARTIAL_LOG_ENTRY)
	{
		//debugRaw("Found partial entry at Monitor Log table index: 0x%x, Now making Incomplete\r\n", __monitorLogTblIndex);

		// Complete entry by setting abnormal termination
		__monitorLogTbl[__monitorLogTblIndex].status = INCOMPLETE_LOG_ENTRY;
	}

	//debugRaw("Monitor Log key: 0x%x, Monitor Log index: %d, Monitor Log Unique Entry Id: %d\r\n",
	//			__monitorLogTblKey, __monitorLogTblIndex, __monitorLogUniqueEntryId);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AdvanceMonitorLogIndex(void)
{
	__monitorLogTblIndex++;
	if (__monitorLogTblIndex >= TOTAL_MONITOR_LOG_ENTRIES)
		__monitorLogTblIndex = 0;
		
	//debugRaw("Next Monitor Log table index: %d\r\n", __monitorLogTblIndex);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16 GetStartingMonitorLogTableIndex(void)
{
	if ((__monitorLogTblIndex + (unsigned)1) >= TOTAL_MONITOR_LOG_ENTRIES)
		return (0);
	else
		return (uint16)(__monitorLogTblIndex + 1);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16 GetStartingEventNumberForCurrentMonitorLog(void)
{
	return(__monitorLogTbl[__monitorLogTblIndex].startEventNumber);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ClearMonitorLogEntry(void)
{
	// Set all log entries to all zero's, status to EMPTY_LOG_ENTRY, start and stop times INVALID
	memset(&__monitorLogTbl[__monitorLogTblIndex], 0, sizeof(MONITOR_LOG_ENTRY_STRUCT));

	//debugRaw("Clearing entry at Monitor Log table index: %d\r\n", __monitorLogTblIndex);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void NewMonitorLogEntry(uint8 mode)
{
	// Advance to the next log entry
	AdvanceMonitorLogIndex();

	// Clear out the log entry (if wrapped)
	ClearMonitorLogEntry();

	//debugRaw("New Monitor Log entry with Unique Id: %d\r\n", __monitorLogUniqueEntryId);

	// Set the unique Monitor Log Entry number
	__monitorLogTbl[__monitorLogTblIndex].uniqueEntryId = __monitorLogUniqueEntryId;

	// Store the current entry number
	StoreMonitorLogUniqueEntryId();

	//debugRaw("Writing partial info to entry at Monitor Log table index: %d\r\n", __monitorLogTblIndex);

	// Set the elements to start a new log entry
	//debugRaw("Writing start time to Monitor Log entry at: 0x%x\r\n", &(__monitorLogTbl[__monitorLogTblIndex].startTime));
	__monitorLogTbl[__monitorLogTblIndex].startTime = GetCurrentTime();
	__monitorLogTbl[__monitorLogTblIndex].startTime.valid = TRUE;
	__monitorLogTbl[__monitorLogTblIndex].mode = mode;
	__monitorLogTbl[__monitorLogTblIndex].startEventNumber = g_nextEventNumberToUse;
	__monitorLogTbl[__monitorLogTblIndex].status = PARTIAL_LOG_ENTRY;
	
	// Bit accuracy adjusted trigger level
	if (g_triggerRecord.trec.variableTriggerEnable == YES)
	{
		__monitorLogTbl[__monitorLogTblIndex].seismicTriggerLevel = (VARIABLE_TRIGGER_CHAR_BASE + g_triggerRecord.trec.variableTriggerVibrationStandard);
	}
#if 1 /* Fixed method (keep as native 16-bit to standardize/match DCM/UCM) */
	else
	{
		__monitorLogTbl[__monitorLogTblIndex].seismicTriggerLevel = g_triggerRecord.trec.seismicTriggerLevel;
	}
#else /* Bit accuracy adjusted (original) */
	else if ((g_triggerRecord.trec.seismicTriggerLevel == NO_TRIGGER_CHAR) || (g_triggerRecord.trec.seismicTriggerLevel == EXTERNAL_TRIGGER_CHAR))
	{
		__monitorLogTbl[__monitorLogTblIndex].seismicTriggerLevel = g_triggerRecord.trec.seismicTriggerLevel;
	}
	else
	{
		__monitorLogTbl[__monitorLogTblIndex].seismicTriggerLevel = g_triggerRecord.trec.seismicTriggerLevel / (ACCURACY_16_BIT_MIDPOINT / g_bitAccuracyMidpoint);
	}
#endif

#if 1 /* Fixed method (keep as native 16-bit to standardize/match DCM/UCM) */
	__monitorLogTbl[__monitorLogTblIndex].airTriggerLevel = g_triggerRecord.trec.airTriggerLevel;
#else /*Bit accuracy adjusted (original) */
	if ((g_triggerRecord.trec.airTriggerLevel == NO_TRIGGER_CHAR) || (g_triggerRecord.trec.airTriggerLevel == EXTERNAL_TRIGGER_CHAR))
	{
		__monitorLogTbl[__monitorLogTblIndex].airTriggerLevel = g_triggerRecord.trec.airTriggerLevel;
	}
	else
	{
		__monitorLogTbl[__monitorLogTblIndex].airTriggerLevel = g_triggerRecord.trec.airTriggerLevel / (ACCURACY_16_BIT_MIDPOINT / g_bitAccuracyMidpoint);
	}
#endif

	__monitorLogTbl[__monitorLogTblIndex].bitAccuracy = ((g_triggerRecord.trec.bitAccuracy < ACCURACY_10_BIT) || (g_triggerRecord.trec.bitAccuracy > ACCURACY_16_BIT)) ?
														ACCURACY_16_BIT : g_triggerRecord.trec.bitAccuracy;
	__monitorLogTbl[__monitorLogTblIndex].adjustForTempDrift = g_triggerRecord.trec.adjustForTempDrift;

	__monitorLogTbl[__monitorLogTblIndex].seismicSensorType = g_factorySetupRecord.seismicSensorType;
#if 1 /* New field added */
	__monitorLogTbl[__monitorLogTblIndex].acousticSensorType = g_factorySetupRecord.acousticSensorType;
#endif
	__monitorLogTbl[__monitorLogTblIndex].sensitivity = (uint8)g_triggerRecord.srec.sensitivity;
	__monitorLogTbl[__monitorLogTblIndex].spare1 = 0;

	//ConvertTimeStampToString((char*)g_spareBuffer, &__monitorLogTbl[__monitorLogTblIndex].startTime, REC_DATE_TIME_TYPE);
	//debug("\tStart Time: %s\r\n", (char*)g_spareBuffer);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UpdateMonitorLogEntry()
{
	//debugRaw("Updating entry at Monitor Log table index: %d, event#: %d, total: %d\r\n", __monitorLogTblIndex, g_nextEventNumberToUse,
	//			(uint16)(g_nextEventNumberToUse - __monitorLogTbl[__monitorLogTblIndex].startEventNumber + 1));

	if (__monitorLogTbl[__monitorLogTblIndex].status == PARTIAL_LOG_ENTRY)
	{
		// Set the number of events recorded
		__monitorLogTbl[__monitorLogTblIndex].eventsRecorded = (uint16)(g_nextEventNumberToUse - __monitorLogTbl[__monitorLogTblIndex].startEventNumber + 1);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void CloseMonitorLogEntry()
{
	//debug("Closing entry at Monitor Log table index: %d, total recorded: %d\r\n", __monitorLogTblIndex, __monitorLogTbl[__monitorLogTblIndex].eventsRecorded);

	if (__monitorLogTbl[__monitorLogTblIndex].status == PARTIAL_LOG_ENTRY)
	{
		// Set the elements to close the current log entry
		//debug("Writing stop time to Monitor Log entry at: 0x%x\r\n", &(__monitorLogTbl[__monitorLogTblIndex].stopTime));
		__monitorLogTbl[__monitorLogTblIndex].stopTime = GetCurrentTime();
		__monitorLogTbl[__monitorLogTblIndex].stopTime.valid = TRUE;
		__monitorLogTbl[__monitorLogTblIndex].status = COMPLETED_LOG_ENTRY;

		AppendMonitorLogEntryFile();

		//ConvertTimeStampToString((char*)g_spareBuffer, &__monitorLogTbl[__monitorLogTblIndex].stopTime, REC_DATE_TIME_TYPE);
		//debug("\tStop Time: %s\r\n", (char*)g_spareBuffer);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitMonitorLogUniqueEntryId(void)
{
	MONITOR_LOG_ID_STRUCT monitorLogRec;

	GetRecordData(&monitorLogRec, DEFAULT_RECORD, REC_UNIQUE_MONITOR_LOG_ID_TYPE);

	// Check if the Monitor Log Record is valid
	if (!monitorLogRec.invalid)
	{
		if (monitorLogRec.currentMonitorLogID == 0xFFFF)
		{
			__monitorLogUniqueEntryId = 1;
		}
		else
		{
			// Set the Current Event number to the last event number stored plus 1
			__monitorLogUniqueEntryId = (monitorLogRec.currentMonitorLogID + 1);
		}
	}
	else // record is invalid
	{
		__monitorLogUniqueEntryId = 1;
		monitorLogRec.currentMonitorLogID = __monitorLogUniqueEntryId;
		
#if 0 /* Don't save initial ID beacsue a power cycle will start the Id at 2 instead of 1 */
		SaveRecordData(&monitorLogRec, DEFAULT_RECORD, REC_UNIQUE_MONITOR_LOG_ID_TYPE);
#endif
	}

	debug("Total Monitor Log entries to date: %d, Current Monitor Log entry number: %d\r\n",
		(__monitorLogUniqueEntryId - 1), __monitorLogUniqueEntryId);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void StoreMonitorLogUniqueEntryId(void)
{
	MONITOR_LOG_ID_STRUCT monitorLogRec;

	monitorLogRec.currentMonitorLogID = __monitorLogUniqueEntryId;

	SaveRecordData(&monitorLogRec, DEFAULT_RECORD, REC_UNIQUE_MONITOR_LOG_ID_TYPE);

	// Increment to a new Monitor Log Entry number
	__monitorLogUniqueEntryId++;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 GetNextMonitorLogEntry(uint16 uid, uint16 startIndex, uint16* tempIndex, MONITOR_LOG_ENTRY_STRUCT* logEntry)
{
	uint8 found = NO;

	// Loop while a new entry has not been found and the temp index does not equal the start (wrapped & finished)
	while ((found == NO) && ((*tempIndex) != startIndex))
	{
		// Check if initial condition
		if ((*tempIndex) == TOTAL_MONITOR_LOG_ENTRIES)
		{
			// Set the temp index to the start index
			*tempIndex = startIndex;
		}

		// Check if the entry at the temp index is complete or incomplete and newer than the comparison unique entry Id 
		if (((__monitorLogTbl[(*tempIndex)].status == COMPLETED_LOG_ENTRY) || 
			(__monitorLogTbl[(*tempIndex)].status == INCOMPLETE_LOG_ENTRY)) && 
			(__monitorLogTbl[(*tempIndex)].uniqueEntryId > uid))
		{
			// Copy the monitor log entry over to the log entry buffer
			*logEntry = __monitorLogTbl[(*tempIndex)];

#if EXTENDED_DEBUG
			debug("(ID: %03d) M: %d, Evt#: %d, S: %d, ST: 0x%x, AT: 0x%x, BA: %d, TA: %d, ST: %d, AT: %d, G: %d\r\n",
			logEntry->uniqueEntryId, logEntry->mode, logEntry->startEventNumber, logEntry->status,
			logEntry->seismicTriggerLevel, logEntry->airTriggerLevel, logEntry->bitAccuracy, logEntry->adjustForTempDrift,
			logEntry->seismicSensorType, logEntry->acousticSensorType, logEntry->sensitivity);
#endif

			// Set the found flag to mark that an entry was discovered
			found = TRUE;
		}

		// Increment the temp index
		(*tempIndex)++;
		if (*tempIndex >= TOTAL_MONITOR_LOG_ENTRIES)
			*tempIndex = 0;
	}
	
	return (found);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16 NumOfNewMonitorLogEntries(uint16 uid)
{
	uint16 total = 0;
	uint16 i = 0;

	//uint16 startIndex = __monitorLogTblIndex;
	//uint16 tempIndex = __monitorLogTblIndex;
	
	for (i = 0; i < TOTAL_MONITOR_LOG_ENTRIES; i++)
	{
		if (((__monitorLogTbl[i].status == COMPLETED_LOG_ENTRY) ||
			(__monitorLogTbl[i].status == INCOMPLETE_LOG_ENTRY)) && 
			(__monitorLogTbl[i].uniqueEntryId > uid))
		{
			total++;
		}
	}

	return (total);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static char s_monitorLogFilename[] = LOGS_PATH MONITOR_LOG_BIN_FILE;
static char s_monitorLogHumanReadableFilename[] = LOGS_PATH MONITOR_LOG_READABLE_FILE;

void AppendMonitorLogEntryFile(void)
{
	char modeString[10];
	char statusString[16];
	char startTimeString[20];
	char stopTimeString[20];
	char seisString[30];
	char airString[15];
	char seisSensorString[10];
	char airSensorString[15];
	MONITOR_LOG_ENTRY_STRUCT *mle;
	float tempSesmicTriggerInUnits;
	float unitsDiv;
	uint32 airInUnits;

	FIL file;
	uint32_t writeSize;

	// -------------------------------------
	// Monitor Log Binary file write
	// -------------------------------------
    if ((f_open(&file, (const TCHAR*)s_monitorLogFilename, FA_OPEN_APPEND | FA_WRITE)) != FR_OK)
	{
		debugErr("Unable to open file: %s\r\n", s_monitorLogFilename);
	}
	else // File created or exists
	{
		if (f_size(&file) % sizeof(MONITOR_LOG_ENTRY_STRUCT) != 0)
		{
			debugWarn("Monitor Log File size does not comprise all whole entries\r\n");
		}

		debug("Writing Monitor log entry to log file...\r\n");

#if ENDIAN_CONVERSION
		// Swap Monitor Log entry to Big Endian for monitor log file
		EndianSwapMonitorLogStruct(&(__monitorLogTbl[__monitorLogTblIndex]));
#endif
		f_write(&file, (uint8*)&(__monitorLogTbl[__monitorLogTblIndex]), sizeof(MONITOR_LOG_ENTRY_STRUCT), (UINT*)&writeSize);
#if ENDIAN_CONVERSION
		// Swap Monitor Log entry back to Litte Endian, since it's referenced again below
		EndianSwapMonitorLogStruct(&(__monitorLogTbl[__monitorLogTblIndex]));
#endif
		// Done writing, close the monitor log file
		g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
		f_close(&file);
		SetFileTimestamp(s_monitorLogFilename);

		debug("Monitor log entry appended to log file\r\n");
	}

	// -------------------------------------
	// Monitor Log Human Readable file write
	// -------------------------------------
    if ((f_open(&file, (const TCHAR*)s_monitorLogHumanReadableFilename, FA_OPEN_APPEND | FA_WRITE)) != FR_OK)
	{
		debugErr("Unable to open file: %s\r\n", s_monitorLogHumanReadableFilename);
	}
	else // File created or exists
	{
		mle = &__monitorLogTbl[__monitorLogTblIndex];

		debug("Writing Monitor log entry to readable log file...\r\n");

		if (mle->mode == WAVEFORM_MODE) { strcpy((char*)&modeString, "Waveform"); }
		else if (mle->mode == BARGRAPH_MODE) { strcpy((char*)&modeString, "Bargraph"); }
		else if (mle->mode == COMBO_MODE) { strcpy((char*)&modeString, "Combo"); }

		if (mle->status == COMPLETED_LOG_ENTRY) { strcpy((char*)&statusString, "Completed"); }
		else if (mle->status == PARTIAL_LOG_ENTRY) { strcpy((char*)&statusString, "Partial"); }
		else if (mle->status == INCOMPLETE_LOG_ENTRY) { strcpy((char*)&statusString, "Incomplete"); }

		sprintf((char*)&startTimeString, "%02d-%02d-%02d %02d:%02d:%02d", mle->startTime.day, mle->startTime.month, mle->startTime.year, mle->startTime.hour, mle->startTime.min, mle->startTime.sec);
		sprintf((char*)&stopTimeString, "%02d-%02d-%02d %02d:%02d:%02d", mle->stopTime.day, mle->stopTime.month, mle->stopTime.year, mle->stopTime.hour, mle->stopTime.min, mle->stopTime.sec);

		if (g_triggerRecord.trec.seismicTriggerLevel == NO_TRIGGER_CHAR) {strcpy((char*)seisString, "None"); }
		else if (g_triggerRecord.trec.variableTriggerEnable == YES)
		{
			switch (g_triggerRecord.trec.variableTriggerVibrationStandard)
			{
				case USBM_RI_8507_DRYWALL_STANDARD: strcpy((char*)seisString, "VT (USBM Drywall)"); break;
				case USBM_RI_8507_PLASTER_STANDARD: strcpy((char*)seisString, "VT (USBM Plaster)"); break;
				case OSM_REGULATIONS_STANDARD: strcpy((char*)seisString, "VT (OSM Regulations)"); break;
				case CUSTOM_STEP_THRESHOLD: strcpy((char*)seisString, "VT (Custom Step Threshold)"); break;
				case CUSTOM_STEP_LIMITING: strcpy((char*)seisString, "VT (Custom Step Limiting)"); break;
				default: strcpy((char*)seisString, "VT (No VS/error)"); break;
			}
		}
		else
		{
			// Calculate the divider used for converting stored A/D peak counts to units of measure
			if ((g_factorySetupRecord.seismicSensorType == SENSOR_ACC_832M1_0200) || (g_factorySetupRecord.seismicSensorType == SENSOR_ACC_832M1_0500))
			{
				unitsDiv = (float)(g_bitAccuracyMidpoint * SENSOR_ACCURACY_100X_SHIFT * ((g_triggerRecord.srec.sensitivity == LOW) ? 2 : 4)) / (float)(g_factorySetupRecord.seismicSensorType * ACC_832M1_SCALER);
			}
			else unitsDiv = (float)(g_bitAccuracyMidpoint * SENSOR_ACCURACY_100X_SHIFT * ((g_triggerRecord.srec.sensitivity == LOW) ? 2 : 4)) / (float)(g_factorySetupRecord.seismicSensorType);

			tempSesmicTriggerInUnits = (float)(g_triggerRecord.trec.seismicTriggerLevel >> g_bitShiftForAccuracy) / (float)unitsDiv;
			if ((g_factorySetupRecord.seismicSensorType < SENSOR_ACC_RANGE_DIVIDER) && (g_unitConfig.unitsOfMeasure == METRIC_TYPE)) { tempSesmicTriggerInUnits *= (float)METRIC; }

			sprintf((char*)seisString, "%05.2f %s", (double)tempSesmicTriggerInUnits, (g_unitConfig.unitsOfMeasure == METRIC_TYPE ? "mm" : "in"));
		}

		if (g_triggerRecord.trec.airTriggerLevel == NO_TRIGGER_CHAR) {strcpy((char*)airString, "None"); }
		else
		{
			airInUnits = AirTriggerConvertToUnits(g_triggerRecord.trec.airTriggerLevel);
			if (g_unitConfig.unitsOfAir == MILLIBAR_TYPE) { sprintf((char*)airString, "%05.3f mb", ((double)airInUnits / 10000)); }
			else if (g_unitConfig.unitsOfAir == PSI_TYPE) { sprintf((char*)airString, "%05.3f psi", ((double)airInUnits / 10000)); }
			else /* (g_unitConfig.unitsOfAir == DECIBEL_TYPE) */ { sprintf((char*)airString, "%d dB", (uint16)airInUnits); }
		}

		if (g_factorySetupRecord.seismicSensorType > SENSOR_ACC_RANGE_DIVIDER) { strcpy((char*)&seisSensorString, "Acc"); }
		else { sprintf((char*)&seisSensorString, "%3.1f in", (double)g_factorySetupRecord.seismicSensorType / (double)204.8); }

		if (g_factorySetupRecord.acousticSensorType == SENSOR_MIC_160_DB) { strcpy((char*)&airSensorString, "Mic 160 dB"); }
		else if (g_factorySetupRecord.acousticSensorType == SENSOR_MIC_5_PSI) { strcpy((char*)&airSensorString, "Mic 5 PSI"); }
		else if (g_factorySetupRecord.acousticSensorType == SENSOR_MIC_10_PSI) { strcpy((char*)&airSensorString, "Mic 10 PSI"); }
		else { strcpy((char*)&airSensorString, "Mic 148 dB"); }

		sprintf((char*)g_spareBuffer, "Log ID: %03d --> Status: %10s, Mode: %8s, Start Time: %s, Stop Time: %s\r\n\tEvents: %3d, Start Evt #: %4d, "\
				"Seismic Trig: %10s, Air Trig: %11s\r\n\tBit Acc: %d, Temp Adjust: %3s, Seismic Sensor: %8s, Acoustic Sensor: %8s, Sensitivity: %6s\r\n\n",
				mle->uniqueEntryId, (char*)statusString, (char*)modeString, (char*)startTimeString, (char*)stopTimeString, mle->eventsRecorded, mle->startEventNumber,
				(char*)seisString, (char*)airString, mle->bitAccuracy, ((mle->adjustForTempDrift == YES) ? "YES" : "NO"),
				(char*)seisSensorString, (char*)airSensorString, ((mle->sensitivity == LOW) ? "NORMAL" : "HIGH"));

		f_write(&file, g_spareBuffer, strlen((char*)g_spareBuffer), (UINT*)&writeSize);

		// Done writing, close the monitor log file
		g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
		f_close(&file);
		SetFileTimestamp(s_monitorLogHumanReadableFilename);

		debug("Monitor log readable entry appended to log file\r\n");
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitMonitorLogTableFromLogFile(void)
{
#if 1 /* temp remove while unused */	
	MONITOR_LOG_ENTRY_STRUCT monitorLogEntry;
	uint16 lowestId = 0;
	uint16 highestId = 0;
	uint16 foundIds = 0;
#endif
	FIL file;
	uint32_t readSize;

	// -------------------------------------
	// Monitor Log Binary file read
	// -------------------------------------
    if ((f_stat((const TCHAR*)s_monitorLogFilename, NULL)) == FR_NO_FILE)
	{ 
		debugWarn("Warning: Monitor Log File not found or has not yet been created\r\n");
	}
	else // File exists
	{
		if ((f_open(&file, (const TCHAR*)s_monitorLogFilename, FA_READ)) != FR_OK) { debugErr("Unable to open file: %s\r\n", s_monitorLogFilename); }

		if (f_size(&file) < sizeof(MONITOR_LOG_ENTRY_STRUCT))
		{
			DisplayFileCorrupt(s_monitorLogFilename);
		}
		else // Monitor log file contains entries
		{
			OverlayMessage(getLangText(MONITOR_LOG_TEXT), getLangText(INITIALIZING_MONITOR_LOG_WITH_SAVED_ENTRIES_TEXT), 1 * SOFT_SECS);

			f_read(&file, (uint8*)&monitorLogEntry, sizeof(MONITOR_LOG_ENTRY_STRUCT), (UINT*)&readSize);
#if ENDIAN_CONVERSION
			EndianSwapMonitorLogStruct(&monitorLogEntry);
#endif
			// Loop while data continues to be read from MONITOR log file
			while (readSize > 0)
			{
				if ((monitorLogEntry.status == COMPLETED_LOG_ENTRY) || (monitorLogEntry.status == INCOMPLETE_LOG_ENTRY))
				{
					//debug("Found Valid Monitor Log Entry with ID: %d\r\n", monitorLogEntry.uniqueEntryId);

					if (foundIds)
					{
						if (monitorLogEntry.uniqueEntryId < lowestId) { lowestId = monitorLogEntry.uniqueEntryId; }
						if (monitorLogEntry.uniqueEntryId > highestId) { highestId = monitorLogEntry.uniqueEntryId; }
					}
					else
					{
						lowestId = monitorLogEntry.uniqueEntryId;
						highestId = monitorLogEntry.uniqueEntryId;
					}

					foundIds++;
#if EXTENDED_DEBUG
					debug("(ID: %03d) M: %d, Evt#: %d, S: %d, ST: 0x%x, AT: 0x%x, BA: %d, TA: %d, ST: %d, G: %d\r\n",
							monitorLogEntry.uniqueEntryId, monitorLogEntry.mode, monitorLogEntry. startEventNumber, monitorLogEntry.status,
							monitorLogEntry.seismicTriggerLevel, monitorLogEntry.airTriggerLevel, monitorLogEntry.bitAccuracy, monitorLogEntry.adjustForTempDrift,
							monitorLogEntry.seismicSensorType, monitorLogEntry.sensitivity);
#endif
					__monitorLogTbl[__monitorLogTblIndex] = monitorLogEntry;
			
					AdvanceMonitorLogIndex();
				}
				else
				{
					// Consider what to do if there is a corrupt entry
				}

				// Always read the next entry
				f_read(&file, (uint8*)&monitorLogEntry, sizeof(MONITOR_LOG_ENTRY_STRUCT), (UINT*)&readSize);
#if ENDIAN_CONVERSION
				EndianSwapMonitorLogStruct(&monitorLogEntry);
#endif
			}

			// Done reading, close the monitor log file
			g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;

			debug("Found Valid Monitor Log Entries, ID's: %d --> %d\r\n", lowestId, highestId);
		}

		f_close(&file);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static char s_onOffLogHumanReadableFilename[] = LOGS_PATH ON_OFF_READABLE_FILE;

void AddOnOffLogTimestamp(uint8 onOffState)
{
	DATE_TIME_STRUCT time = GetCurrentTime();
	float extCharge = GetExternalVoltageLevelAveraged(EXT_CHARGE_VOLTAGE);
	char onOffStateString[6];
	char timeString[20];
	char extChargeString[8];

	FIL file;
	uint32_t writeSize;

    if ((f_open(&file, (const TCHAR*)s_onOffLogHumanReadableFilename, FA_OPEN_APPEND | FA_WRITE)) != FR_OK)
	{
		debugErr("Unable to open file: %s\r\n", s_onOffLogHumanReadableFilename);
	}
	else // File successfully created or opened
	{
		if (onOffState == ON) { strcpy((char*)onOffStateString, "On"); }
		else if (onOffState == OFF) { strcpy((char*)onOffStateString, "Off"); }
		else if (onOffState == JUMP_TO_BOOT) { strcpy((char*)onOffStateString, "J2B"); }
		else if (onOffState == OFF_EXCEPTION) { strcpy((char*)onOffStateString, "OfE"); }
		else if (onOffState == FORCED_OFF) { strcpy((char*)onOffStateString, "FOf"); }

		if (extCharge > EXTERNAL_VOLTAGE_PRESENT) { sprintf((char*)&extChargeString, "%4.2fv", (double)extCharge); }
		else { sprintf((char*)&extChargeString, "<none>"); }

		sprintf((char*)&timeString, "%02d-%02d-%02d %02d:%02d:%02d", time.day, time.month, time.year, time.hour, time.min, time.sec);

		sprintf((char*)g_spareBuffer, "Unit <%s>: %3s, Version: %s, Mode: %6s, Time: %s, Battery: %3.2fv, Ext charge: %6s\r\n",
				(char*)&g_factorySetupRecord.unitSerialNumber, (char*)onOffStateString, (char*)g_buildVersion,
				((g_unitConfig.timerMode == ENABLED) ? "Timer" : "Normal"), (char*)timeString,
				(double)GetExternalVoltageLevelAveraged(BATTERY_VOLTAGE), extChargeString);

		f_write(&file, g_spareBuffer, strlen((char*)g_spareBuffer), (UINT*)&writeSize);

		// Done writing, close the on/off log file
		g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
		f_close(&file);
		SetFileTimestamp(s_onOffLogHumanReadableFilename);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 1
void WriteDebugBufferToFile(void)
{
	FIL file;
	uint32_t writeSize;

	if (g_debugBufferCount)
	{
		if (f_open(&file, (const TCHAR*)LOGS_PATH"DebugLogReadable.txt", FA_OPEN_APPEND | FA_WRITE) == FR_OK)
		{
			f_write(&file, (uint8*)&g_debugBuffer, g_debugBufferCount, (UINT*)&writeSize);
			SetFileTimestamp(LOGS_PATH"DebugLogReadable.txt");

			// Done writing, close the debug log file
			g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
			f_close(&file);

			memset(&g_debugBuffer, 0, sizeof(g_debugBuffer));
			g_debugBufferCount = 0;
		}
		else
		{
			DisplayFileNotFound(LOGS_PATH"DebugLogReadable.txt");
		}
	}
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 1
void SwitchDebugLogFile(void)
{
	f_unlink((const TCHAR*)LOGS_PATH"DebugLogLastRun.txt");
	f_rename((const TCHAR*)LOGS_PATH"DebugLogReadable.txt", (const TCHAR*)LOGS_PATH"DebugLogLastRun.txt");
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void FillInAdditionalExceptionReportInfo(FIL file)
{
	DATE_TIME_STRUCT exceptionTime = GetCurrentTime();
	char modeString[10];
	uint32_t writeSize;

	if (g_triggerRecord.opMode == WAVEFORM_MODE) { strcpy((char*)&modeString, "Waveform"); }
	else if (g_triggerRecord.opMode == BARGRAPH_MODE) { strcpy((char*)&modeString, "Bargraph"); }
	else if (g_triggerRecord.opMode == COMBO_MODE) { strcpy((char*)&modeString, "Combo"); }

	sprintf((char*)g_spareBuffer, "Mode: %s, Monitoring: %s, Sample Rate: %lu, Record Time: %lu\r\n", (char*)&modeString, (g_sampleProcessing == ACTIVE_STATE) ? "Yes" : "No",
			g_triggerRecord.trec.sample_rate, g_triggerRecord.trec.record_time);
	f_write(&file, g_spareBuffer, strlen((char*)g_spareBuffer), (UINT*)&writeSize);

	sprintf((char*)g_spareBuffer, "Error time: %02d-%02d-%02d %02d:%02d:%02d\r\n", exceptionTime.day, exceptionTime.month, exceptionTime.year,
			exceptionTime.hour, exceptionTime.min, exceptionTime.sec);
	f_write(&file, g_spareBuffer, strlen((char*)g_spareBuffer), (UINT*)&writeSize);
}
