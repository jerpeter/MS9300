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
#include "math.h"
#include "Typedefs.h"
#include "InitDataBuffers.h"
#include "SysEvents.h"
#include "Record.h"
#include "Menu.h"
#include "Summary.h"
#include "EventProcessing.h"
#include "Board.h"
#include "Record.h"
#include "OldUart.h"
#include "RealTimeClock.h"
#include "ProcessBargraph.h"
#include "PowerManagement.h"
#include "RemoteCommon.h"
#include "Minilzo.h"

#include "ff.h"
//#include "fsaccess.h"

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
void MoveManualCalToFile(void)
{
	uint16 i;
	uint16 sample;
	uint16 normalizedData;
	uint16 hiA = 0, hiR = 0, hiV = 0, hiT = 0;
	uint16 lowA = 0xFFFF, lowR = 0xFFFF, lowV = 0xFFFF, lowT = 0xFFFF;
	uint16* startOfEventPtr;
	uint16* endOfEventDataPtr;
	uint32 compressSize;
	uint16* aManualCalPeakPtr = NULL;
	uint16* rManualCalPeakPtr = NULL;
	uint16* vManualCalPeakPtr = NULL;
	uint16* tManualCalPeakPtr = NULL;

	FIL file;
	uint32_t bytesWritten;

	uint16 scPeakA = 0, scPeakR = 0, scPeakV = 0, scPeakT = 0;
	uint16 scFreqA = 0, scFreqR = 0, scFreqV = 0, scFreqT = 0;
	uint8 calResults = 0;
	uint8 ss1status = 0, ss2status = 0, as1status = 0, as2status = 0;

	UNUSED(compressSize); // Depends on debug on/off
#if 1 /* New method to process Sensor Check for results */
	UNUSED(file);
	UNUSED(bytesWritten);
#endif

	//debug("Processing Manual Cal to be saved\r\n");
	debug("Processing Sensor Check operation for sensor status\r\n");

	if (g_freeEventBuffers < g_maxEventBuffers)
	{
		g_pendingEventRecord.summary.captured.eventTime = GetCurrentTime();

		// Clear out A, R, V, T channel calculated data (in case the pending event record is reused)
		memset(&g_pendingEventRecord.summary.calculated.a, 0, (NUMBER_OF_CHANNELS_DEFAULT * sizeof(CHANNEL_CALCULATED_DATA_STRUCT)));

		startOfEventPtr = g_currentEventStartPtr;
		endOfEventDataPtr = g_currentEventStartPtr + g_wordSizeInCal;
		
		for (i = (uint16)g_samplesInCal; i != 0; i--)
		{
#if 0 /* Original */
			if (g_bitShiftForAccuracy) AdjustSampleForBitAccuracy();
#else /* New method to evaluate Sensor check at 16-bit*/
			// Handle at 16-bit
#endif
			//=========================================================
			// First channel - A
			sample = *(g_currentEventSamplePtr + A_CHAN_OFFSET);

			if (sample > hiA) hiA = sample;
			if (sample < lowA) lowA = sample;

#if 0 /* Original */
			normalizedData = FixDataToZero(sample);
#else /* New method to evaluate Sensor check at 16-bit */
			normalizedData = FixDataToZero16Bit(sample);
#endif
			if (normalizedData > g_pendingEventRecord.summary.calculated.a.peak)
			{
				g_pendingEventRecord.summary.calculated.a.peak = normalizedData;
				aManualCalPeakPtr = (g_currentEventSamplePtr + A_CHAN_OFFSET);
			}

			//=========================================================
			// Second channel - R
			sample = *(g_currentEventSamplePtr + R_CHAN_OFFSET);

			if (sample > hiR) hiR = sample;
			if (sample < lowR) lowR = sample;

#if 0 /* Original */
			normalizedData = FixDataToZero(sample);
#else /* New method to evaluate Sensor check at 16-bit */
			normalizedData = FixDataToZero16Bit(sample);
#endif
			if (normalizedData > g_pendingEventRecord.summary.calculated.r.peak)
			{
				g_pendingEventRecord.summary.calculated.r.peak = normalizedData;
				rManualCalPeakPtr = (g_currentEventSamplePtr + R_CHAN_OFFSET);
			}

			//=========================================================
			// Third channel - V
			sample = *(g_currentEventSamplePtr + V_CHAN_OFFSET);

			if (sample > hiV) hiV = sample;
			if (sample < lowV) lowV = sample;

#if 0 /* Original */
			normalizedData = FixDataToZero(sample);
#else /* New method to evaluate Sensor check at 16-bit */
			normalizedData = FixDataToZero16Bit(sample);
#endif
			if (normalizedData > g_pendingEventRecord.summary.calculated.v.peak)
			{
				g_pendingEventRecord.summary.calculated.v.peak = normalizedData;
				vManualCalPeakPtr = (g_currentEventSamplePtr + V_CHAN_OFFSET);
			}

			//=========================================================
			// Fourth channel - T
			sample = *(g_currentEventSamplePtr + T_CHAN_OFFSET);

			if (sample > hiT) hiT = sample;
			if (sample < lowT) lowT = sample;

#if 0 /* Original */
			normalizedData = FixDataToZero(sample);
#else /* New method to evaluate Sensor check at 16-bit */
			normalizedData = FixDataToZero16Bit(sample);
#endif
			if (normalizedData > g_pendingEventRecord.summary.calculated.t.peak)
			{
				g_pendingEventRecord.summary.calculated.t.peak = normalizedData;
				tManualCalPeakPtr = (g_currentEventSamplePtr + T_CHAN_OFFSET);
			}

			g_currentEventSamplePtr += NUMBER_OF_CHANNELS_DEFAULT;
		}

		scPeakA = g_pendingEventRecord.summary.calculated.a.peak = (uint16)(hiA - lowA + 1);
		scPeakR = g_pendingEventRecord.summary.calculated.r.peak = (uint16)(hiR - lowR + 1);
		scPeakV = g_pendingEventRecord.summary.calculated.v.peak = (uint16)(hiV - lowV + 1);
		scPeakT = g_pendingEventRecord.summary.calculated.t.peak = (uint16)(hiT - lowT + 1);

		scFreqA = g_pendingEventRecord.summary.calculated.a.frequency = CalcSumFreq(aManualCalPeakPtr, SAMPLE_RATE_1K, startOfEventPtr, endOfEventDataPtr);
		scFreqR = g_pendingEventRecord.summary.calculated.r.frequency = CalcSumFreq(rManualCalPeakPtr, SAMPLE_RATE_1K, startOfEventPtr, endOfEventDataPtr);
		scFreqV = g_pendingEventRecord.summary.calculated.v.frequency = CalcSumFreq(vManualCalPeakPtr, SAMPLE_RATE_1K, startOfEventPtr, endOfEventDataPtr);
		scFreqT = g_pendingEventRecord.summary.calculated.t.frequency = CalcSumFreq(tManualCalPeakPtr, SAMPLE_RATE_1K, startOfEventPtr, endOfEventDataPtr);

#if 1 /* New method to process Sensor Check for results */
		debug("Sensor Check peaks (P2P): r:%04x v:%04x t:%04x a:%04x\r\n", scPeakR, scPeakV, scPeakT, scPeakA);
		debug("Sensor Check freq: r:%d v:%d t:%d a:%d \r\n", scFreqR, scFreqV, scFreqT, scFreqA);

		SensorCheckChannelPeaks(scPeakR, scPeakV, scPeakT, scPeakA, &calResults, &ss1status, &ss2status, &as1status, &as2status);
		CacheSensorCheckIndividualSensorResults(ss1status, ss2status, as1status, as2status);
		CacheSensorCheckResults(calResults);

		// Log Sensor check results and data
		SENSOR_CHECK_LOG_ENTRY_STRUCT scLog;
		memset(&scLog, 0, sizeof(scLog));
		scLog.sensorStatus = g_lastSensorCheck;
		// Store group A/1 results, if selected or both groups
		if ((g_currentSensorGroup == SENSOR_GROUP_A_1) || (g_currentSensorGroup == SENSOR_GROUP_BOTH))
		{
			scLog.geo1R.peak = scPeakR; scLog.geo1R.freq = scFreqA;
			scLog.geo1T.peak = scPeakT; scLog.geo1T.freq = scFreqT;
			scLog.geo1V.peak = scPeakV; scLog.geo1V.freq = scFreqV;
			scLog.air2A.peak = scPeakA; scLog.air2A.freq = scFreqA;
		}
		// Store group B/2 results, if selected or both groups
		if ((g_currentSensorGroup == SENSOR_GROUP_B_2) || (g_currentSensorGroup == SENSOR_GROUP_BOTH))
		{
			scLog.geo2R.peak = scPeakR; scLog.geo2R.freq = scFreqA;
			scLog.geo2T.peak = scPeakT; scLog.geo2T.freq = scFreqT;
			scLog.geo2V.peak = scPeakV; scLog.geo2V.freq = scFreqV;
			scLog.air1A.peak = scPeakA; scLog.air1A.freq = scFreqA;
		}
		AppendSensorCheckResultsLogFile(&scLog);

#else /* Original */
		CompleteRamEventSummary();

		CacheResultsEventInfo((EVT_RECORD*)&g_pendingEventRecord);

		// Get new event file name
		CheckStoredEventsCapEventsLimit();
		GetEventFilename(g_pendingEventRecord.summary.eventNumber);
		MakeDirectoryIfNotPresent(EVENTS_PATH, g_pendingEventRecord.summary.eventNumber);

		if (f_open(&file, (const TCHAR*)g_spareFileName, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
		{
			debugErr("Unable to create cal event file: %s\r\n", g_spareFileName);
		}
		else // File created, write out the event
		{
			sprintf((char*)g_spareBuffer, "%s %s #%d %s...", getLangText(CALIBRATION_TEXT), getLangText(EVENT_TEXT),
					g_pendingEventRecord.summary.eventNumber, getLangText(BEING_SAVED_TEXT));
			OverlayMessage(getLangText(EVENT_COMPLETE_TEXT), (char*)g_spareBuffer, 0);

#if ENDIAN_CONVERSION
			// Swap event record to Big Endian for event file
			EndianSwapEventRecord(&g_pendingEventRecord);
#endif
			// Write the event record header and summary
			f_write(&file, &g_pendingEventRecord, sizeof(EVT_RECORD), (UINT*)&bytesWritten);
#if ENDIAN_CONVERSION
			// Swap event record to Litte Endian, since cached event record is referenced again below
			EndianSwapEventRecord(&g_pendingEventRecord);
#endif
#if ENDIAN_CONVERSION
			// Swap data to Big Endian for event file (and compression below if used)
			EndianSwapDataX16(g_currentEventStartPtr, g_wordSizeInCal);
#endif
			// Write the event data, containing the Pretrigger, event and cal
			f_write(&file, g_currentEventStartPtr, (g_wordSizeInCal * 2), (UINT*)&bytesWritten);

			// Update the remaining space left
			UpdateSDCardUsageStats(f_size(&file));

			// Done writing the event file, close the file handle
			g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
			f_close(&file);
			SetFileTimestamp(g_spareFileName);

			//==========================================================================================================
			// Save compressed event record file
			//----------------------------------------------------------------------------------------------------------
			if (g_unitConfig.saveCompressedData != DO_NOT_SAVE_EXTRA_FILE_COMPRESSED_DATA)
			{
				// Get new ERData compressed event file name
				GetEREventRecordFilename(g_pendingEventRecord.summary.eventNumber);
				MakeDirectoryIfNotPresent(ER_DATA_PATH, g_pendingEventRecord.summary.eventNumber);

				if ((f_open(&file, (const TCHAR*)g_spareFileName, FA_CREATE_ALWAYS | FA_WRITE)) != FR_OK)
				{
					debugErr("Unable to create EReventrecord event file: %s\r\n", g_spareFileName);
				}
				else // File created, write out the event
				{
					g_globalFileHandle = &file;
					g_spareBufferIndex = 0;
#if ENDIAN_CONVERSION
					// Swap event record to Big Endian for compression
					EndianSwapEventRecord(&g_pendingEventRecord);
#endif
					compressSize = lzo1x_1_compress((void*)&g_pendingEventRecord, sizeof(g_pendingEventRecord), OUT_FILE);

					// Check if any remaining compressed data is queued
					if (g_spareBufferIndex)
					{
						// Finish writing the remaining compressed data
						f_write(&file, g_spareBuffer, g_spareBufferIndex, (UINT*)&bytesWritten);
						g_spareBufferIndex = 0;
					}

					debug("Wave Compressed Event Record length: %d (Matches file: %s)\r\n", compressSize, (compressSize == f_size(&file)) ? "Yes" : "No");
#if ENDIAN_CONVERSION
					// Swap event record back to Little Endian for processing
					EndianSwapEventRecord(&g_pendingEventRecord);
#endif
					// Update the remaining space left
					UpdateSDCardUsageStats(f_size(&file));

					// Done writing the event file, close the file handle
					g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
					f_close(&file);
					SetFileTimestamp(g_spareFileName);
				}
			}
			//==========================================================================================================

			//==========================================================================================================
			// Save compressed data file
			//----------------------------------------------------------------------------------------------------------
			if (g_unitConfig.saveCompressedData != DO_NOT_SAVE_EXTRA_FILE_COMPRESSED_DATA)
			{
				// Get new ERData compressed event file name
				GetERDataFilename(g_pendingEventRecord.summary.eventNumber);
				MakeDirectoryIfNotPresent(ER_DATA_PATH, g_pendingEventRecord.summary.eventNumber);

				if ((f_open(&file, (const TCHAR*)g_spareFileName, FA_CREATE_ALWAYS | FA_WRITE)) != FR_OK)
				{
					debugErr("Unable to create ERdata event file: %s\r\n", g_spareFileName);
				}
				else // File created, write out the event
				{
					g_globalFileHandle = &file;
					g_spareBufferIndex = 0;
					compressSize = lzo1x_1_compress((void*)g_currentEventStartPtr, (g_wordSizeInCal * 2), OUT_FILE);

					// Check if any remaining compressed data is queued
					if (g_spareBufferIndex)
					{
						// Finish writing the remaining compressed data
						f_write(&file, g_spareBuffer, g_spareBufferIndex, (UINT*)&bytesWritten);
						g_spareBufferIndex = 0;
					}

					debug("Manual Cal Compressed Data length: %d (Matches file: %s)\r\n", compressSize, (compressSize == f_size(&file)) ? "Yes" : "No");

					// Update the remaining space left
					UpdateSDCardUsageStats(f_size(&file));

					// Done writing the event file, close the file handle
					g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
					f_close(&file);
					SetFileTimestamp(g_spareFileName);
				}
			}
			//==========================================================================================================
			debug("Manual Cal Event file closed\r\n");

			AddEventToSummaryList(&g_pendingEventRecord);

			// Don't log a monitor entry for Manual Cal
			//UpdateMonitorLogEntry();

			// After event numbers have been saved, store current event number in persistent storage.
			AddEventNumberToCache(g_pendingEventRecord.summary.eventNumber);
			StoreCurrentEventNumber();

			// Now store the updated event number in the universal ram storage.
			g_pendingEventRecord.summary.eventNumber = g_nextEventNumberToUse;
		}
#endif
		if (++g_eventBufferReadIndex == g_maxEventBuffers)
		{
			g_eventBufferReadIndex = 0;
			g_currentEventSamplePtr = g_startOfEventBufferPtr;
		}
		else
		{
			g_currentEventSamplePtr = g_startOfEventBufferPtr + (g_eventBufferReadIndex * g_wordSizeInEvent);
		}

		clearSystemEventFlag(MANUAL_CAL_EVENT);

#if 0 /* Original */
		// Set flag to display calibration results if not monitoring or monitoring in waveform
		if ((g_sampleProcessing == IDLE_STATE) || ((g_sampleProcessing == ACTIVE_STATE) && (g_triggerRecord.opMode == WAVEFORM_MODE)))
		{
			// Set printout mode to allow the results menu processing to know this is a manual cal pulse
			raiseMenuEventFlag(RESULTS_MENU_EVENT);
		}
#else /* Display status */
		// Todo: Show Sensor Check status on LCD screen
#endif
		g_freeEventBuffers++;
	}
	else
	{
		//debugWarn("Manual Cal: No free buffers\r\n");
		debug("Sensor Check: No free buffers for operation\r\n");

		clearSystemEventFlag(MANUAL_CAL_EVENT);
	}
}
