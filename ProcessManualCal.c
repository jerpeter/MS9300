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
#include "Uart.h"
#include "RealTimeClock.h"
#include "ProcessBargraph.h"
#include "PowerManagement.h"
#include "RemoteCommon.h"
#include "Minilzo.h"
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
	int manualCalFileHandle = -1;
	uint16* aManualCalPeakPtr;
	uint16* rManualCalPeakPtr;
	uint16* vManualCalPeakPtr;
	uint16* tManualCalPeakPtr;

	debug("Processing Manual Cal to be saved\r\n");

	if (g_freeEventBuffers < g_maxEventBuffers)
	{
		g_pendingEventRecord.summary.captured.eventTime = GetCurrentTime();

		// Clear out A, R, V, T channel calculated data (in case the pending event record is reused)
		memset(&g_pendingEventRecord.summary.calculated.a, 0, (NUMBER_OF_CHANNELS_DEFAULT * sizeof(CHANNEL_CALCULATED_DATA_STRUCT)));

		startOfEventPtr = g_currentEventStartPtr;
		endOfEventDataPtr = g_currentEventStartPtr + g_wordSizeInCal;
		
		for (i = (uint16)g_samplesInCal; i != 0; i--)
		{
			if (g_bitShiftForAccuracy) AdjustSampleForBitAccuracy();

			//=========================================================
			// First channel - A
			sample = *(g_currentEventSamplePtr + A_CHAN_OFFSET);

			if (sample > hiA) hiA = sample;
			if (sample < lowA) lowA = sample;

			normalizedData = FixDataToZero(sample);

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

			normalizedData = FixDataToZero(sample);

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

			normalizedData = FixDataToZero(sample);

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

			normalizedData = FixDataToZero(sample);

			if (normalizedData > g_pendingEventRecord.summary.calculated.t.peak)
			{
				g_pendingEventRecord.summary.calculated.t.peak = normalizedData;
				tManualCalPeakPtr = (g_currentEventSamplePtr + T_CHAN_OFFSET);
			}

			g_currentEventSamplePtr += NUMBER_OF_CHANNELS_DEFAULT;
		}

		g_pendingEventRecord.summary.calculated.a.peak = (uint16)(hiA - lowA + 1);
		g_pendingEventRecord.summary.calculated.r.peak = (uint16)(hiR - lowR + 1);
		g_pendingEventRecord.summary.calculated.v.peak = (uint16)(hiV - lowV + 1);
		g_pendingEventRecord.summary.calculated.t.peak = (uint16)(hiT - lowT + 1);

		g_pendingEventRecord.summary.calculated.a.frequency = CalcSumFreq(aManualCalPeakPtr, SAMPLE_RATE_1K, startOfEventPtr, endOfEventDataPtr);
		g_pendingEventRecord.summary.calculated.r.frequency = CalcSumFreq(rManualCalPeakPtr, SAMPLE_RATE_1K, startOfEventPtr, endOfEventDataPtr);
		g_pendingEventRecord.summary.calculated.v.frequency = CalcSumFreq(vManualCalPeakPtr, SAMPLE_RATE_1K, startOfEventPtr, endOfEventDataPtr);
		g_pendingEventRecord.summary.calculated.t.frequency = CalcSumFreq(tManualCalPeakPtr, SAMPLE_RATE_1K, startOfEventPtr, endOfEventDataPtr);

		CompleteRamEventSummary();

		CacheResultsEventInfo((EVT_RECORD*)&g_pendingEventRecord);

		if (g_fileAccessLock != AVAILABLE)
		{
			ReportFileSystemAccessProblem("Save Manual Cal");
		}
		else // (g_fileAccessLock == AVAILABLE)
		{
			GetSpi1MutexLock(SDMMC_LOCK);

#if 0 /* old hw */
			nav_select(FS_NAV_ID_DEFAULT);

			// Get new event file handle
			manualCalFileHandle = GetEventFileHandle(g_pendingEventRecord.summary.eventNumber, CREATE_EVENT_FILE);

			if (manualCalFileHandle == -1)
			{
				ReleaseSpi1MutexLock();

				debugErr("Failed to get a new file handle for the Manual Cal event\r\n");
			}
			else // Write the file event to the SD card
			{
				sprintf((char*)g_spareBuffer, "%s %s #%d %s...", getLangText(CALIBRATION_TEXT), getLangText(EVENT_TEXT),
						g_pendingEventRecord.summary.eventNumber, getLangText(BEING_SAVED_TEXT));
				OverlayMessage(getLangText(EVENT_COMPLETE_TEXT), (char*)g_spareBuffer, 0);

				// Write the event record header and summary
				write(manualCalFileHandle, &g_pendingEventRecord, sizeof(EVT_RECORD));

				// Write the event data, containing the Pretrigger, event and cal
				write(manualCalFileHandle, g_currentEventStartPtr, (g_wordSizeInCal * 2));

				SetFileDateTimestamp(FS_DATE_LAST_WRITE);

				// Update the remaining space left
				UpdateSDCardUsageStats(nav_file_lgt());

				// Done writing the event file, close the file handle
				g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
				close(manualCalFileHandle);

				//==========================================================================================================
				// Save compressed data file
				//----------------------------------------------------------------------------------------------------------
				if (g_unitConfig.saveCompressedData != DO_NOT_SAVE_EXTRA_FILE_COMPRESSED_DATA)
				{
					// Get new event file handle
					g_globalFileHandle = GetERDataFileHandle(g_pendingEventRecord.summary.eventNumber, CREATE_EVENT_FILE);

					g_spareBufferIndex = 0;
					compressSize = lzo1x_1_compress((void*)g_currentEventStartPtr, (g_wordSizeInCal * 2), OUT_FILE);

					// Check if any remaining compressed data is queued
					if (g_spareBufferIndex)
					{
						// Finish writing the remaining compressed data
						write(g_globalFileHandle, g_spareBuffer, g_spareBufferIndex);
						g_spareBufferIndex = 0;
					}
					debug("Manual Cal Compressed Data length: %d (Matches file: %s)\r\n", compressSize, (compressSize == nav_file_lgt()) ? "Yes" : "No");

					SetFileDateTimestamp(FS_DATE_LAST_WRITE);

					// Update the remaining space left
					UpdateSDCardUsageStats(nav_file_lgt());

					// Done writing the event file, close the file handle
					g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
					close(g_globalFileHandle);
				}
				//==========================================================================================================
				ReleaseSpi1MutexLock();

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

			// Set flag to display calibration results if not monitoring or monitoring in waveform
			if ((g_sampleProcessing == IDLE_STATE) || ((g_sampleProcessing == ACTIVE_STATE) && (g_triggerRecord.opMode == WAVEFORM_MODE)))
			{
				// Set printout mode to allow the results menu processing to know this is a manual cal pulse
				raiseMenuEventFlag(RESULTS_MENU_EVENT);
			}

			g_freeEventBuffers++;
		}
	}
	else
	{
		debugWarn("Manual Cal: No free buffers\r\n");

		clearSystemEventFlag(MANUAL_CAL_EVENT);
	}
}
