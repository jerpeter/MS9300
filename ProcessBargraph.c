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
#include "Typedefs.h"
#include "InitDataBuffers.h"
#include "SysEvents.h"
#include "Summary.h"
#include "OldUart.h"
#include "RealTimeClock.h"
#include "Record.h"
#include "Menu.h"
#include "ProcessBargraph.h"
#include "EventProcessing.h"
#include "Minilzo.h"
#include "Math.h"

#include "ff.h"

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
void StartNewBargraph(void)
{
	// Clear the initial Bar Interval plus the Summary Interval and Freq Calc buffers
	memset(g_bargraphBarIntervalWritePtr, 0, sizeof(BARGRAPH_BAR_INTERVAL_DATA));
	memset(&g_bargraphSummaryInterval, 0, sizeof(g_bargraphSummaryInterval));
	memset(&g_bargraphFreqCalcBuffer, 0, sizeof(g_bargraphFreqCalcBuffer));

	// Init counts
	g_summaryCount = 0;
	g_barSampleCount = 0;
	g_bargraphBarIntervalsCached = 0;
	g_bargraphBarIntervalClock = 0;
	g_blmBarIntervalQueueCount = 0;

	// Init flags
	g_bargraphLiveMonitoringBISendActive = NO;
	g_modemStatus.barLiveMonitorOverride = NO;

	HandleBargraphLiveMonitoringStartMsg();

	// Update the current monitor log entry
	UpdateMonitorLogEntry();

	MoveStartOfBargraphEventRecordToFile();
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void EndBargraph(void)
{
	while (CalculateBargraphData() == BG_BUFFER_NOT_EMPTY) {}

	// Check if any bar intervals are cached or if no summaries have yet been recorded
	if ((g_bargraphBarIntervalsCached) || (g_summaryCount == 0))
	{
		// Save the Bar and Summary intervals
		MoveSummaryIntervalDataToFile();
	}

	MoveUpdatedBargraphEventRecordToFile(BARPGRAPH_SESSION_COMPLETE);

	HandleBargraphLiveMonitoringEndMsg();

	clearSystemEventFlag(BARGRAPH_EVENT);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MoveBarIntervalDataToFile(void)
{
	FIL file;
	uint32_t bytesWritten;

	// If Bar Intervals have been cached
	if (g_bargraphBarIntervalsCached > 0)
	{
		GetEventFilename(g_pendingBargraphRecord.summary.eventNumber);

		if (f_open(&file, (const TCHAR*)g_spareFileName, FA_OPEN_APPEND | FA_WRITE) != FR_OK)
		{
			debugErr("Unable to re-open event file for appending BI's: %s\r\n", g_spareFileName);
		}
		else // File opened, append event info
		{
			while (g_bargraphBarIntervalsCached)
			{
				// New BI with options
				if (g_pendingBargraphRecord.summary.parameters.barIntervalDataType == BAR_INTERVAL_ORIGINAL_DATA_TYPE_SIZE)
				{
					f_write(&file, g_bargraphBarIntervalReadPtr, BAR_INTERVAL_ORIGINAL_DATA_TYPE_SIZE, (UINT*)&bytesWritten);
				}
				else // New Bar Interval data type option, store A max, then R, V, T max, then A, R, V, T freq (if selected), and finally VS max
				{
					f_write(&file, &g_bargraphBarIntervalReadPtr->aMax, sizeof(g_bargraphBarIntervalReadPtr->aMax), (UINT*)&bytesWritten);
					f_write(&file, &g_bargraphBarIntervalReadPtr->rMax, (sizeof(g_bargraphBarIntervalReadPtr->rMax) * ((g_pendingBargraphRecord.summary.parameters.barIntervalDataType == BAR_INTERVAL_A_R_V_T_DATA_TYPE_SIZE) ? 3 : 7)), (UINT*)&bytesWritten);
					f_write(&file, &g_bargraphBarIntervalReadPtr->vsMax, sizeof(g_bargraphBarIntervalReadPtr->vsMax), (UINT*)&bytesWritten);
				}

				// Increment based on the size of the Bar Interval data type
				g_pendingBargraphRecord.header.dataLength += g_pendingBargraphRecord.summary.parameters.barIntervalDataType;

				// Advance the Bar Interval global buffer pointer
				AdvanceBarIntervalBufPtr(READ_PTR);

				g_bargraphBarIntervalsCached--;
			}

			g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
			f_close(&file);
			SetFileTimestamp(g_spareFileName);

			debug("%s event file closed\r\n", (g_triggerRecord.opMode == BARGRAPH_MODE) ? "Bargraph" : "Combo - Bargraph");
		}
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void CompleteSummaryInterval(void)
{
	float rFreq = (float)0, vFreq = (float)0, tFreq = (float)0;

	// Special check to make sure the frequency isn't zero (channel never crosses zer0/mid level)
	if (g_bargraphSummaryInterval.a.frequency == 0) { g_bargraphSummaryInterval.a.frequency = g_bargraphFreqCalcBuffer.a.freq_count; }
	if (g_bargraphSummaryInterval.r.frequency == 0) { g_bargraphSummaryInterval.r.frequency = g_bargraphFreqCalcBuffer.r.freq_count; }
	if (g_bargraphSummaryInterval.v.frequency == 0) { g_bargraphSummaryInterval.v.frequency = g_bargraphFreqCalcBuffer.v.freq_count; }
	if (g_bargraphSummaryInterval.t.frequency == 0) { g_bargraphSummaryInterval.t.frequency = g_bargraphFreqCalcBuffer.t.freq_count; }

	// Note: This should be raw unadjusted freq
	rFreq = (float)((float)g_triggerRecord.trec.sample_rate / (float)((g_bargraphSummaryInterval.r.frequency * 2) - 1));
	vFreq = (float)((float)g_triggerRecord.trec.sample_rate / (float)((g_bargraphSummaryInterval.v.frequency * 2) - 1));
	tFreq = (float)((float)g_triggerRecord.trec.sample_rate / (float)((g_bargraphSummaryInterval.t.frequency * 2) - 1));

	// Calculate the Peak Displacement
	g_bargraphSummaryInterval.a.displacement = 0;
	g_bargraphSummaryInterval.r.displacement = (uint32)(g_bargraphSummaryInterval.r.peak * 1000000 / 2 / PI / rFreq);
	g_bargraphSummaryInterval.v.displacement = (uint32)(g_bargraphSummaryInterval.v.peak * 1000000 / 2 / PI / vFreq);
	g_bargraphSummaryInterval.t.displacement = (uint32)(g_bargraphSummaryInterval.t.peak * 1000000 / 2 / PI / tFreq);

	// Calculate the Peak Acceleration
	g_bargraphSummaryInterval.a.acceleration = 0;
	g_bargraphSummaryInterval.r.acceleration = (uint32)(g_bargraphSummaryInterval.r.peak * 1000 * 2 * PI * rFreq);
	g_bargraphSummaryInterval.v.acceleration = (uint32)(g_bargraphSummaryInterval.v.peak * 1000 * 2 * PI * vFreq);
	g_bargraphSummaryInterval.t.acceleration = (uint32)(g_bargraphSummaryInterval.t.peak * 1000 * 2 * PI * tFreq);

	// Store timestamp for the end of the summary interval
	g_summaryCount++;
	g_bargraphSummaryInterval.summariesCaptured = g_summaryCount;
	g_bargraphSummaryInterval.intervalEnd_Time = GetCurrentTime();
	g_bargraphSummaryInterval.batteryLevel = (uint32)(100.0 * GetExternalVoltageLevelAveraged(BATTERY_VOLTAGE));
	g_bargraphSummaryInterval.bargraphEffectiveSampleRate = (uint16)(g_barSampleCount / (g_bargraphSummaryInterval.barIntervalsCaptured * g_triggerRecord.bgrec.barInterval));
	g_barSampleCount = 0;

	g_bargraphSummaryInterval.calcStructEndFlag = 0xEECCCCEE;	// End structure flag
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MoveSummaryIntervalDataToFile(void)
{
	FIL file;
	uint32_t bytesWritten;

	CompleteSummaryInterval();

	GetEventFilename(g_pendingBargraphRecord.summary.eventNumber);

	if (f_open(&file, (const TCHAR*)g_spareFileName, FA_OPEN_APPEND | FA_WRITE) != FR_OK)
	{
		debugErr("Unable to re-open event file for appending SI: %s\r\n", g_spareFileName);
	}
	else // File opened, append event info
	{
		// Write any cached bar intervals before storing the summary interval (may not match with bar interval write threshold)
		while (g_bargraphBarIntervalsCached)
		{
			// New BI with options
			if (g_pendingBargraphRecord.summary.parameters.barIntervalDataType == BAR_INTERVAL_ORIGINAL_DATA_TYPE_SIZE)
			{
				f_write(&file, g_bargraphBarIntervalReadPtr, BAR_INTERVAL_ORIGINAL_DATA_TYPE_SIZE, (UINT*)&bytesWritten);
			}
			else // New Bar Interval data type option, store A max, then R, V, T max, then A, R, V, T freq (if selected), and finally VS max
			{
				f_write(&file, &g_bargraphBarIntervalReadPtr->aMax, sizeof(g_bargraphBarIntervalReadPtr->aMax), (UINT*)&bytesWritten);
				f_write(&file, &g_bargraphBarIntervalReadPtr->rMax, (sizeof(g_bargraphBarIntervalReadPtr->rMax) * ((g_pendingBargraphRecord.summary.parameters.barIntervalDataType == BAR_INTERVAL_A_R_V_T_DATA_TYPE_SIZE) ? 3 : 7)), (UINT*)&bytesWritten);
				f_write(&file, &g_bargraphBarIntervalReadPtr->vsMax, sizeof(g_bargraphBarIntervalReadPtr->vsMax), (UINT*)&bytesWritten);
			}

			// Increment based on the size of the Bar Interval data type
			g_pendingBargraphRecord.header.dataLength += g_pendingBargraphRecord.summary.parameters.barIntervalDataType;

			// Advance the Bar Interval global buffer pointer
			AdvanceBarIntervalBufPtr(READ_PTR);

			g_bargraphBarIntervalsCached--;
		}

		f_write(&file, &g_bargraphSummaryInterval, sizeof(CALCULATED_DATA_STRUCT), (UINT*)&bytesWritten);

		g_pendingBargraphRecord.header.dataLength += sizeof(CALCULATED_DATA_STRUCT);

		g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
		f_close(&file);
		SetFileTimestamp(g_spareFileName);

		debug("%s event file closed\r\n", (g_triggerRecord.opMode == BARGRAPH_MODE) ? "Bargraph" : "Combo - Bargraph");
	}

	// Update the job totals.
	UpdateBargraphJobTotals();

	// Clear the Summary Interval structure for use again
	memset(&g_bargraphSummaryInterval, 0, sizeof(g_bargraphSummaryInterval));

	// Clear out the Freq Calc structure for use again
	memset(&g_bargraphFreqCalcBuffer, 0, sizeof(g_bargraphFreqCalcBuffer));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 CheckBargraphLiveMonitoringAuthorizedToSend(void)
{
	// Check if Bar Live Monitoring is active, but give remote side the power to override unit setting
	if ((g_unitConfig.barLiveMonitor || g_modemStatus.barLiveMonitorOverride == YES) && (g_modemStatus.barLiveMonitorOverride != BAR_LIVE_MONITORING_OVERRIDE_STOP))
	{
		// Make sure not actively handling a remote command response transfer of data
		if ((g_modemStatus.xferMutex == NO) && (READ_DCD == CONNECTION_ESTABLISHED) && (g_modemStatus.systemIsLockedFlag == NO) && (g_bargraphLiveMonitoringBISendActive != YES))
		{
			return (YES);
		}
	}

	return (NO);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void WaitForBargraphLiveMonitoringDataToFinishSendingWithTimeout(void)
{
	uint32 barLiveDataTransferTimeout;

	// Set the timeout to ~1 second
	barLiveDataTransferTimeout = g_lifetimeHalfSecondTickCount + 2;

	// Try to allow the full Bar Interval line data to send (ISR)
	while (g_bargraphLiveMonitoringBISendActive == YES)
	{
		if (barLiveDataTransferTimeout == g_lifetimeHalfSecondTickCount)
		{
			// Kill the Bar live data transfer if unable to complete in 1 second
			g_bargraphLiveMonitoringBISendActive = NO;

			// Todo: Determine if disabling the UART TX Ready interrupt is needed
			break;
		}
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ChecksumAndSetupBargraphLiveMonitorDataForISRTransfer(void)
{
	uint8 checksum = 0;
	uint8* checksumDataPtr;
	uint16 charCount = (MAX_TEXT_LINE_CHARS - 3); // Total chars minus 3 bytes for checksum, CR&LF

	// Calculate checksum
	checksumDataPtr = g_blmBuffer;
	while ((*checksumDataPtr) && (--charCount)) { checksum ^= *checksumDataPtr++; }

	// Apend the checksum to the end of the output string
	sprintf((char*)checksumDataPtr, "%d\r\n", checksum);

	g_bargraphBarIntervalLiveMonitorBIDataPtr = g_blmBuffer;
	g_bargraphLiveMonitoringBISendActive = YES;

#if 0 /* old hw */
	// Enable the transmit ready interrupt
	AVR32_USART1.ier = (AVR32_USART_IER_TXRDY_MASK | AVR32_USART_IER_RXRDY_MASK | AVR32_USART_IER_OVRE_MASK | AVR32_USART_IER_PARE_MASK | AVR32_USART_IER_FRAME_MASK);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleBargraphLiveMonitoringDataTransfer(void)
{
	// Check if there is a Bar Interval ready to send (prefer this as the top level conditional which is expected to fail/dropout the most)
	if (g_bargraphBarIntervalLiveMonitoringReadPtr != g_bargraphBarIntervalWritePtr)
	{
		// Verify data is authorized to be sent
		if (CheckBargraphLiveMonitoringAuthorizedToSend())
		{
			// Handle the available Bar Interval
			uint8 gainFactor = (uint8)((g_triggerRecord.srec.sensitivity == LOW) ? 2 : 4);
			float div = (float)(g_bitAccuracyMidpoint * g_sensorInfo.sensorAccuracy * gainFactor) / (float)(g_factorySetupRecord.seismicSensorType);

			// Check if the queue count is above the cached BI's meaning a full SI has passed without sending BLM data
			if (g_blmBarIntervalQueueCount > (g_triggerRecord.bgrec.summaryInterval / g_triggerRecord.bgrec.barInterval))
			{
				// Reset the BLM read pointer to the current write pointer
				g_bargraphBarIntervalLiveMonitoringReadPtr = g_bargraphBarIntervalWritePtr;

				// Reset the BLM queue count
				g_blmBarIntervalQueueCount = 0;

				// Drop out to wait for the current BI to complete
				return;
			}

			// Setup BLM data buffer for interrupt driven send over the wire
			sprintf((char*)g_blmBuffer, "BLM,%d,%d,%d,%d,A,%f,%.1f,R,%f,%.1f,V,%f,%.1f,T,%f,%.1f,VS,%f,%lu,",
				(g_bargraphBarIntervalLiveMonitoringReadPtr->summaryIntervalCount + 1), g_bargraphBarIntervalLiveMonitoringReadPtr->barIntervalCount,
				g_bargraphBarIntervalLiveMonitoringReadPtr->currentBargraphEventNumber, g_bargraphBarIntervalLiveMonitoringReadPtr->currentComboWaveformEventNumber,
				(double)(HexToMB(g_bargraphBarIntervalLiveMonitoringReadPtr->aMax, DATA_NORMALIZED, g_bitAccuracyMidpoint, g_factorySetupRecord.acousticSensorType)), (double)((float)g_pendingBargraphRecord.summary.parameters.sampleRate/(float)g_bargraphBarIntervalLiveMonitoringReadPtr->aFreq),
				(double)(float)(g_bargraphBarIntervalLiveMonitoringReadPtr->rMax/div), (double)((float)g_pendingBargraphRecord.summary.parameters.sampleRate/(float)g_bargraphBarIntervalLiveMonitoringReadPtr->rFreq),
				(double)(float)(g_bargraphBarIntervalLiveMonitoringReadPtr->vMax/div), (double)((float)g_pendingBargraphRecord.summary.parameters.sampleRate/(float)g_bargraphBarIntervalLiveMonitoringReadPtr->vFreq),
				(double)(float)(g_bargraphBarIntervalLiveMonitoringReadPtr->tMax/div), (double)((float)g_pendingBargraphRecord.summary.parameters.sampleRate/(float)g_bargraphBarIntervalLiveMonitoringReadPtr->tFreq),
				(double)(float)(sqrtf((float)g_bargraphBarIntervalLiveMonitoringReadPtr->vsMax)/div), g_bargraphBarIntervalLiveMonitoringReadPtr->epochTime);

			// Advance the read pointer
			AdvanceBarIntervalBufPtr(BLM_READ_PTR);
			if (g_blmBarIntervalQueueCount) { g_blmBarIntervalQueueCount--; }

			ChecksumAndSetupBargraphLiveMonitorDataForISRTransfer();
		}
	}
	else if (g_blmAlertAlarmStatus)
	{
		// Verify data is authorized to be sent
		if (CheckBargraphLiveMonitoringAuthorizedToSend())
		{
			// Setup BLM data buffer for interrupt driven send over the wire
			sprintf((char*)g_blmBuffer, "ALM,%d,", g_blmAlertAlarmStatus);

			// CLear the status
			g_blmAlertAlarmStatus = 0;

			ChecksumAndSetupBargraphLiveMonitorDataForISRTransfer();
		}
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleBargraphLiveMonitoringStartMsg(void)
{
	if (CheckBargraphLiveMonitoringAuthorizedToSend())
	{
		// Create data string
		sprintf((char*)g_blmBuffer, "SLM,%d,%d,%d,START,", g_pendingBargraphRecord.summary.parameters.summaryInterval, g_pendingBargraphRecord.summary.parameters.barInterval, g_pendingBargraphRecord.summary.eventNumber);

		ChecksumAndSetupBargraphLiveMonitorDataForISRTransfer();
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleBargraphLiveMonitoringEndMsg(void)
{
	uint32 barLiveDataTransferTimeout;

	// Check if actively sending Bar live monitor data
	if (g_bargraphLiveMonitoringBISendActive == YES)
	{
		// Set the timeout to ~1 second
		barLiveDataTransferTimeout = g_lifetimeHalfSecondTickCount + 2;

		// Try to allow the last full Bar Interval line data to send (ISR)
		while (g_bargraphLiveMonitoringBISendActive == YES)
		{
			if (barLiveDataTransferTimeout == g_lifetimeHalfSecondTickCount)
			{
				// Timeout hit
				break;
			}
		}
	}

	// Make sure the BLM is authorized to send
	if (CheckBargraphLiveMonitoringAuthorizedToSend())
	{
		// Create data string
		sprintf((char*)g_blmBuffer, "ELM,%d,%lu,%d,%s,", g_pendingBargraphRecord.summary.calculated.summariesCaptured, g_pendingBargraphRecord.summary.calculated.barIntervalsCaptured, g_pendingBargraphRecord.summary.eventNumber,
				(getSystemEventState(CYCLE_CHANGE_EVENT) ? "CYCLE" : "END"));

		ChecksumAndSetupBargraphLiveMonitorDataForISRTransfer();
	}
	else // Something wrong with the transfer
	{
		// Kill the Bar live data transfer by disabling the transmit ready interrupt
		g_bargraphLiveMonitoringBISendActive = NO;

		// Todo: Determine if disabling the UART TX Ready interrupt is needed
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 CalculateBargraphData(void)
{
	// Temp variables, assigned as static to prevent storing on stack
	static uint32 vsTemp;
	static uint16 aTempNorm, rTempNorm, vTempNorm, tTempNorm;
	static DATE_TIME_STRUCT aTempTime, rTempTime, vTempTime, tTempTime;
	static uint8 g_aJobFreqFlag = NO, g_rJobFreqFlag = NO, g_vJobFreqFlag = NO, g_tJobFreqFlag = NO;
	static uint8 g_aJobPeakMatch = NO, g_rJobPeakMatch = NO, g_vJobPeakMatch = NO, g_tJobPeakMatch = NO;

	int32 falloutCounter = SAMPLE_RATE_1K;
	SAMPLE_DATA_STRUCT currentDataSample;

	// While the bargraph data read pointer has not caught up with the write pointer and the fallout counter hasn't reached zero
	while ((g_bargraphDataReadPtr != g_bargraphDataWritePtr) && (--falloutCounter > 0))
	{
		// Read the next data sample set to be processed
		currentDataSample = *(SAMPLE_DATA_STRUCT*)g_bargraphDataReadPtr;

		// Increment to the next data sample set
		g_bargraphDataReadPtr += NUMBER_OF_CHANNELS_DEFAULT;
		
		// Wrap the data read pointer if at or beyond the end of the buffer
		if (g_bargraphDataReadPtr >= g_bargraphDataEndPtr) g_bargraphDataReadPtr = g_bargraphDataStartPtr;

		//________________________________________________________________________________________________
		//
		// Bargraph Bar Interval clocking changed to be time synced (instead of sample count synced)
		//________________________________________________________________________________________________

		// Check for end of Bar Interval key
		if ((currentDataSample.a == BAR_INTERVAL_END_KEY) && (currentDataSample.r == BAR_INTERVAL_END_KEY) &&
			(currentDataSample.v == BAR_INTERVAL_END_KEY) && (currentDataSample.t == BAR_INTERVAL_END_KEY))
		{
			//=================================================
			// End of Bar Interval
			//=================================================
			g_bargraphBarIntervalsCached++;
			g_bargraphSummaryInterval.barIntervalsCaptured++;

			// Advance the Bar Interval global buffer pointer
			AdvanceBarIntervalBufPtr(WRITE_PTR);

			//=================================================
			// End of Summary Interval
			//=================================================
			if (g_bargraphSummaryInterval.barIntervalsCaptured == (uint32)(g_triggerRecord.bgrec.summaryInterval / g_triggerRecord.bgrec.barInterval))
			{
				// Move Summary Interval data to the event file (and cached Bar Intervals will be saved prior to Summary)
				MoveSummaryIntervalDataToFile();

				// Changed design to update the Bargraph event record to the event file in case of unit error (which allows event to be processed)
				MoveUpdatedBargraphEventRecordToFile(BARGRAPH_SESSION_IN_PROGRESS);
			}
		}
		else // Process real data
		{
			// Count each sample processed
			g_barSampleCount++;

			// Adjust for the correct bit accuracy
			currentDataSample.a >>= g_bitShiftForAccuracy;
			currentDataSample.r >>= g_bitShiftForAccuracy;
			currentDataSample.v >>= g_bitShiftForAccuracy;
			currentDataSample.t >>= g_bitShiftForAccuracy;

			// Normalize the raw data without the message bits
			aTempNorm = FixDataToZero(currentDataSample.a);
			rTempNorm = FixDataToZero(currentDataSample.r);
			vTempNorm = FixDataToZero(currentDataSample.v);
			tTempNorm = FixDataToZero(currentDataSample.t);

			// Find the vector sum of the current sample
			vsTemp =((uint32)rTempNorm * (uint32)rTempNorm) +
					((uint32)vTempNorm * (uint32)vTempNorm) +
					((uint32)tTempNorm * (uint32)tTempNorm);

			//=================================================
			// Impulse Interval
			//=================================================
			if (g_impulseMenuCount >= g_triggerRecord.berec.impulseMenuUpdateSecs)
			{
				g_impulseMenuCount = 0;
				g_aImpulsePeak = g_rImpulsePeak = g_vImpulsePeak = g_tImpulsePeak = 0;
				g_vsImpulsePeak = 0;
			}

			// ---------------------------------
			// A, R, V, T channel and Vector Sum
			// ---------------------------------
			// Store the max A, R, V or T normalized value if a new max was found
			if (aTempNorm > g_aImpulsePeak) g_aImpulsePeak = aTempNorm;
			if (rTempNorm > g_rImpulsePeak) g_rImpulsePeak = rTempNorm;
			if (vTempNorm > g_vImpulsePeak) g_vImpulsePeak = vTempNorm;
			if (tTempNorm > g_tImpulsePeak) g_tImpulsePeak = tTempNorm;
			if (vsTemp > g_vsImpulsePeak) g_vsImpulsePeak = vsTemp;

			//=================================================
			// Bar Interval
			//=================================================
			// ---------
			// A channel
			// ---------
			// Store the max Air normalized value if a new max was found
			if (aTempNorm > g_bargraphBarIntervalWritePtr->aMax) { g_bargraphBarIntervalWritePtr->aMax = aTempNorm; g_bargraphBarIntervalWritePtr->aFreq = ((g_bargraphFreqCalcBuffer.a.freq_count * 2) + 1); }

			// ---------------
			// R, V, T channel
			// ---------------
			// Store the max R, V or T normalized value if a new max was found
			if (rTempNorm > g_bargraphBarIntervalWritePtr->rvtMax) { g_bargraphBarIntervalWritePtr->rvtMax = rTempNorm; }
			if (vTempNorm > g_bargraphBarIntervalWritePtr->rvtMax) { g_bargraphBarIntervalWritePtr->rvtMax = vTempNorm; }
			if (tTempNorm > g_bargraphBarIntervalWritePtr->rvtMax) { g_bargraphBarIntervalWritePtr->rvtMax = tTempNorm; }

			// Check if using either new Bar Interval data type format
#if 0 /* Don't filter the new BI option data to allow access with Bargraph Live Monitoring */
			if (g_pendingBargraphRecord.summary.parameters.barIntervalDataType != BAR_INTERVAL_ORIGINAL_DATA_TYPE_SIZE)
#endif
			{
				// Store the max R, V and T normalized value if a new max was found
				if (rTempNorm > g_bargraphBarIntervalWritePtr->rMax) { g_bargraphBarIntervalWritePtr->rMax = rTempNorm;	g_bargraphBarIntervalWritePtr->rFreq = ((g_bargraphFreqCalcBuffer.r.freq_count * 2) + 1); }
				if (vTempNorm > g_bargraphBarIntervalWritePtr->vMax) { g_bargraphBarIntervalWritePtr->vMax = vTempNorm;	g_bargraphBarIntervalWritePtr->vFreq = ((g_bargraphFreqCalcBuffer.v.freq_count * 2) + 1); }
				if (tTempNorm > g_bargraphBarIntervalWritePtr->tMax) { g_bargraphBarIntervalWritePtr->tMax = tTempNorm;	g_bargraphBarIntervalWritePtr->tFreq = ((g_bargraphFreqCalcBuffer.t.freq_count * 2) + 1); }
			}

			// ----------
			// Vector Sum
			// ----------
			// Store the max Vector Sum if a new max was found
			if (vsTemp > g_bargraphBarIntervalWritePtr->vsMax) { g_bargraphBarIntervalWritePtr->vsMax = vsTemp; }

			//=================================================
			// Summary Interval
			//=================================================
			// ---------
			// A channel
			// ---------
			// Check if the current sample is equal or greater than the stored max
			if (aTempNorm >= g_bargraphSummaryInterval.a.peak)
			{
				// Check if the current max matches exactly to the stored max
				if (aTempNorm == g_bargraphSummaryInterval.a.peak)
				{
					// Sample matches current max, set match flag
					g_bargraphFreqCalcBuffer.a.matchFlag = TRUE;
					// Store timestamp in case this is the max (and count) we want to keep
					aTempTime = GetCurrentTime();
				}
				else
				{
					// Copy over new max
					g_bargraphSummaryInterval.a.peak = aTempNorm;
					// Update timestamp
					g_bargraphSummaryInterval.a_Time = GetCurrentTime();
					// Reset match flag since a new peak was found
					g_bargraphFreqCalcBuffer.a.matchFlag = FALSE;
				}

				if (aTempNorm >= g_aJobPeak)
				{
					if (aTempNorm == g_aJobPeak)
						g_aJobPeakMatch = YES;
					else
					{
						g_aJobPeak = aTempNorm;
						g_aJobPeakMatch = NO;
					}

					g_aJobFreqFlag = YES;
				}

				// Set update flag since we have either a matched or a new max
				g_bargraphFreqCalcBuffer.a.updateFlag = TRUE;
			}

			// ---------
			// R channel
			// ---------
			// Check if the current sample is equal or greater than the stored max
			if (rTempNorm >= g_bargraphSummaryInterval.r.peak)
			{
				// Check if the current max matches exactly to the stored max
				if (rTempNorm == g_bargraphSummaryInterval.r.peak)
				{
					// Sample matches current max, set match flag
					g_bargraphFreqCalcBuffer.r.matchFlag = TRUE;
					// Store timestamp in case this is the max (and count) we want to keep
					rTempTime = GetCurrentTime();
				}
				else
				{
					// Copy over new max
					g_bargraphSummaryInterval.r.peak = rTempNorm;
					// Update timestamp
					g_bargraphSummaryInterval.r_Time = GetCurrentTime();
					// Reset match flag since a new peak was found
					g_bargraphFreqCalcBuffer.r.matchFlag = FALSE;
				}

				if (rTempNorm >= g_rJobPeak)
				{
					if (rTempNorm == g_rJobPeak)
						g_rJobPeakMatch = YES;
					else
					{
						g_rJobPeak = rTempNorm;
						g_rJobPeakMatch = NO;
					}

					g_rJobFreqFlag = YES;
				}

				// Set update flag since we have either a matched or a new max
				g_bargraphFreqCalcBuffer.r.updateFlag = TRUE;
			}

			// ---------
			// V channel
			// ---------
			// Check if the current sample is equal or greater than the stored max
			if (vTempNorm >= g_bargraphSummaryInterval.v.peak)
			{
				// Check if the current max matches exactly to the stored max
				if (vTempNorm == g_bargraphSummaryInterval.v.peak)
				{
					// Sample matches current max, set match flag
					g_bargraphFreqCalcBuffer.v.matchFlag = TRUE;
					// Store timestamp in case this is the max (and count) we want to keep
					vTempTime = GetCurrentTime();
				}
				else
				{
					// Copy over new max
					g_bargraphSummaryInterval.v.peak = vTempNorm;
					// Update timestamp
					g_bargraphSummaryInterval.v_Time = GetCurrentTime();
					// Reset match flag since a new peak was found
					g_bargraphFreqCalcBuffer.v.matchFlag = FALSE;
				}

				if (vTempNorm >= g_vJobPeak)
				{
					if (vTempNorm == g_vJobPeak)
						g_vJobPeakMatch = YES;
					else
					{
						g_vJobPeak = vTempNorm;
						g_vJobPeakMatch = NO;
					}

					g_vJobFreqFlag = YES;
				}

				// Set update flag since we have either a matched or a new max
				g_bargraphFreqCalcBuffer.v.updateFlag = TRUE;
			}

			// ---------
			// T channel
			// ---------
			// Check if the current sample is equal or greater than the stored max
			if (tTempNorm >= g_bargraphSummaryInterval.t.peak)
			{
				// Check if the current max matches exactly to the stored max
				if (tTempNorm == g_bargraphSummaryInterval.t.peak)
				{
					// Sample matches current max, set match flag
					g_bargraphFreqCalcBuffer.t.matchFlag = TRUE;
					// Store timestamp in case this is the max (and count) we want to keep
					tTempTime = GetCurrentTime();
				}
				else
				{
					// Copy over new max
					g_bargraphSummaryInterval.t.peak = tTempNorm;
					// Update timestamp
					g_bargraphSummaryInterval.t_Time = GetCurrentTime();
					// Reset match flag since a new peak was found
					g_bargraphFreqCalcBuffer.t.matchFlag = FALSE;
				}

				if (tTempNorm >= g_tJobPeak)
				{
					if (tTempNorm == g_tJobPeak)
						g_tJobPeakMatch = YES;
					else
					{
						g_tJobPeak = tTempNorm;
						g_tJobPeakMatch = NO;
					}

					g_tJobFreqFlag = YES;
				}

				// Set update flag since we have either a matched or a new max
				g_bargraphFreqCalcBuffer.t.updateFlag = TRUE;
			}

			// ----------
			// Vector Sum
			// ----------
			// Store the max Vector Sum if a new max was found
			if (vsTemp > g_bargraphSummaryInterval.vectorSumPeak)
			{
				// Store max vector sum
				g_bargraphSummaryInterval.vectorSumPeak = vsTemp;
				// Store timestamp
				g_bargraphSummaryInterval.vs_Time = GetCurrentTime();

				if (vsTemp > g_vsJobPeak)
				{
					g_vsJobPeak = vsTemp;
				}
			}

			//=================================================
			// Freq Calc Information
			//=================================================
			// ------------
			// All Channels
			// ------------
			// Check if the freq count is zero, meaning initial sample or the start of a new summary interval (doesn't matter which channel is checked)
			if (g_bargraphFreqCalcBuffer.a.freq_count == 0)
			{
				g_bargraphFreqCalcBuffer.a.sign = (uint16)(currentDataSample.a & g_bitAccuracyMidpoint);
				g_bargraphFreqCalcBuffer.r.sign = (uint16)(currentDataSample.r & g_bitAccuracyMidpoint);
				g_bargraphFreqCalcBuffer.v.sign = (uint16)(currentDataSample.v & g_bitAccuracyMidpoint);
				g_bargraphFreqCalcBuffer.t.sign = (uint16)(currentDataSample.t & g_bitAccuracyMidpoint);
			}

			// ---------
			// A channel
			// ---------
			// Check if the stored sign comparison signals a zero crossing
			if (g_bargraphFreqCalcBuffer.a.sign ^ (currentDataSample.a & g_bitAccuracyMidpoint))
			{
				// Store new sign for future zero crossing comparisons
				g_bargraphFreqCalcBuffer.a.sign = (uint16)(currentDataSample.a & g_bitAccuracyMidpoint);

				// If the update flag was set, update freq count information
				if (g_bargraphFreqCalcBuffer.a.updateFlag == TRUE)
				{
					// Check if the max values matched
					if (g_bargraphFreqCalcBuffer.a.matchFlag == TRUE)
					{
						// Check if current count is greater (lower freq) than stored count
						if (g_bargraphFreqCalcBuffer.a.freq_count > g_bargraphSummaryInterval.a.frequency)
						{
							// Save the new count (lower freq)
							g_bargraphSummaryInterval.a.frequency = g_bargraphFreqCalcBuffer.a.freq_count;
							// Save the timestamp corresponding to the lower freq
							g_bargraphSummaryInterval.a_Time = aTempTime;
						}
					}
					else // We had a new max, store count for freq calculation
					{
						// Move data to summary interval struct
						g_bargraphSummaryInterval.a.frequency = g_bargraphFreqCalcBuffer.a.freq_count;
					}

					// Reset flags
					g_bargraphFreqCalcBuffer.a.updateFlag = FALSE;
					g_bargraphFreqCalcBuffer.a.matchFlag = FALSE;
				}

				if (g_aJobFreqFlag == YES)
				{
					if (g_aJobPeakMatch == YES)
					{
						if (g_bargraphFreqCalcBuffer.a.freq_count > g_aJobFreq)
							g_aJobFreq = g_bargraphFreqCalcBuffer.a.freq_count;

						g_aJobPeakMatch = NO;
					}
					else
					{
						g_aJobFreq = g_bargraphFreqCalcBuffer.a.freq_count;
					}

					g_aJobFreqFlag = NO;
				}

				// Reset count to 1 since we have a sample that's crossed zero boundary
				g_bargraphFreqCalcBuffer.a.freq_count = 1;
			}
			else
			{
				// Increment count (if not maxed already) since we haven't crossed a zero boundary
				if (g_bargraphFreqCalcBuffer.a.freq_count < 0xFFFF)
				{
					g_bargraphFreqCalcBuffer.a.freq_count++;
				}
			}

			// ---------
			// R channel
			// ---------
			// Check if the stored sign comparison signals a zero crossing
			if (g_bargraphFreqCalcBuffer.r.sign ^ (currentDataSample.r & g_bitAccuracyMidpoint))
			{
				// Store new sign for future zero crossing comparisons
				g_bargraphFreqCalcBuffer.r.sign = (uint16)(currentDataSample.r & g_bitAccuracyMidpoint);

				// If the update flag was set, update freq count information
				if (g_bargraphFreqCalcBuffer.r.updateFlag == TRUE)
				{
					// Check if the max values matched
					if (g_bargraphFreqCalcBuffer.r.matchFlag == TRUE)
					{
						// Check if current count is greater (lower freq) than stored count
						if (g_bargraphFreqCalcBuffer.r.freq_count > g_bargraphSummaryInterval.r.frequency)
						{
							// Save the new count (lower freq)
							g_bargraphSummaryInterval.r.frequency = g_bargraphFreqCalcBuffer.r.freq_count;
							// Save the timestamp corresponding to the lower freq
							g_bargraphSummaryInterval.r_Time = rTempTime;
						}
					}
					else // We had a new max, store count for freq calculation
					{
						// Move data to summary interval struct
						g_bargraphSummaryInterval.r.frequency = g_bargraphFreqCalcBuffer.r.freq_count;
					}

					// Reset flags
					g_bargraphFreqCalcBuffer.r.updateFlag = FALSE;
					g_bargraphFreqCalcBuffer.r.matchFlag = FALSE;
				}

				if (g_rJobFreqFlag == YES)
				{
					if (g_rJobPeakMatch == YES)
					{
						if (g_bargraphFreqCalcBuffer.r.freq_count > g_rJobFreq)
							g_rJobFreq = g_bargraphFreqCalcBuffer.r.freq_count;

						g_rJobPeakMatch = NO;
					}
					else
					{
						g_rJobFreq = g_bargraphFreqCalcBuffer.r.freq_count;
					}

					g_rJobFreqFlag = NO;
				}

				// Reset count to 1 since we have a sample that's crossed zero boundary
				g_bargraphFreqCalcBuffer.r.freq_count = 1;
			}
			else
			{
				// Increment count (if not maxed already) since we haven't crossed a zero boundary
				if (g_bargraphFreqCalcBuffer.r.freq_count < 0xFFFF)
				{
					g_bargraphFreqCalcBuffer.r.freq_count++;
				}
			}

			// ---------
			// V channel
			// ---------
			// Check if the stored sign comparison signals a zero crossing
			if (g_bargraphFreqCalcBuffer.v.sign ^ (currentDataSample.v & g_bitAccuracyMidpoint))
			{
				// Store new sign for future zero crossing comparisons
				g_bargraphFreqCalcBuffer.v.sign = (uint16)(currentDataSample.v & g_bitAccuracyMidpoint);

				// If the update flag was set, update freq count information
				if (g_bargraphFreqCalcBuffer.v.updateFlag == TRUE)
				{
					// Check if the max values matched
					if (g_bargraphFreqCalcBuffer.v.matchFlag == TRUE)
					{
						// Check if current count is greater (lower freq) than stored count
						if (g_bargraphFreqCalcBuffer.v.freq_count > g_bargraphSummaryInterval.v.frequency)
						{
							// Save the new count (lower freq)
							g_bargraphSummaryInterval.v.frequency = g_bargraphFreqCalcBuffer.v.freq_count;
							// Save the timestamp corresponding to the lower freq
							g_bargraphSummaryInterval.v_Time = vTempTime;
						}
					}
					else // We had a new max, store count for freq calculation
					{
						// Move data to summary interval struct
						g_bargraphSummaryInterval.v.frequency = g_bargraphFreqCalcBuffer.v.freq_count;
					}

					// Reset flags
					g_bargraphFreqCalcBuffer.v.updateFlag = FALSE;
					g_bargraphFreqCalcBuffer.v.matchFlag = FALSE;
				}

				if (g_vJobFreqFlag == YES)
				{
					if (g_vJobPeakMatch == YES)
					{
						if (g_bargraphFreqCalcBuffer.v.freq_count > g_vJobFreq)
							g_vJobFreq = g_bargraphFreqCalcBuffer.v.freq_count;

						g_vJobPeakMatch = NO;
					}
					else
					{
						g_vJobFreq = g_bargraphFreqCalcBuffer.v.freq_count;
					}

					g_vJobFreqFlag = NO;
				}

				// Reset count to 1 since we have a sample that's crossed zero boundary
				g_bargraphFreqCalcBuffer.v.freq_count = 1;
			}
			else
			{
				// Increment count (if not maxed already) since we haven't crossed a zero boundary
				if (g_bargraphFreqCalcBuffer.v.freq_count < 0xFFFF)
				{
					g_bargraphFreqCalcBuffer.v.freq_count++;
				}
			}

			// ---------
			// T channel
			// ---------
			// Check if the stored sign comparison signals a zero crossing
			if (g_bargraphFreqCalcBuffer.t.sign ^ (currentDataSample.t & g_bitAccuracyMidpoint))
			{
				// Store new sign for future zero crossing comparisons
				g_bargraphFreqCalcBuffer.t.sign = (uint16)(currentDataSample.t & g_bitAccuracyMidpoint);

				// If the update flag was set, update freq count information
				if (g_bargraphFreqCalcBuffer.t.updateFlag == TRUE)
				{
					// Check if the max values matched
					if (g_bargraphFreqCalcBuffer.t.matchFlag == TRUE)
					{
						// Check if current count is greater (lower freq) than stored count
						if (g_bargraphFreqCalcBuffer.t.freq_count > g_bargraphSummaryInterval.t.frequency)
						{
							// Save the new count (lower freq)
							g_bargraphSummaryInterval.t.frequency = g_bargraphFreqCalcBuffer.t.freq_count;
							// Save the timestamp corresponding to the lower freq
							g_bargraphSummaryInterval.t_Time = tTempTime;
						}
					}
					else // We had a new max, store count for freq calculation
					{
						// Move data to summary interval struct
						g_bargraphSummaryInterval.t.frequency = g_bargraphFreqCalcBuffer.t.freq_count;
					}

					// Reset flags
					g_bargraphFreqCalcBuffer.t.updateFlag = FALSE;
					g_bargraphFreqCalcBuffer.t.matchFlag = FALSE;
				}

				if (g_tJobFreqFlag == YES)
				{
					if (g_tJobPeakMatch == YES)
					{
						if (g_bargraphFreqCalcBuffer.t.freq_count > g_tJobFreq)
							g_tJobFreq = g_bargraphFreqCalcBuffer.t.freq_count;

						g_tJobPeakMatch = NO;
					}
					else
					{
						g_tJobFreq = g_bargraphFreqCalcBuffer.t.freq_count;
					}

					g_tJobFreqFlag = NO;
				}

				// Reset count to 1 since we have a sample that's crossed zero boundary
				g_bargraphFreqCalcBuffer.t.freq_count = 1;
			}
			else
			{
				// Increment count (if not maxed already) since we haven't crossed a zero boundary
				if (g_bargraphFreqCalcBuffer.t.freq_count < 0xFFFF)
				{
					g_bargraphFreqCalcBuffer.t.freq_count++;
				}
			}
		}
	} // While != loop

	if (g_bargraphDataWritePtr != g_bargraphDataReadPtr)
	{
		return (BG_BUFFER_NOT_EMPTY);
	}
	else
	{
		return (BG_BUFFER_EMPTY);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MoveStartOfBargraphEventRecordToFile(void)
{
	FIL file;
	uint32_t bytesWritten;

	// Create new Bargraph event name
	CheckStoredEventsCapEventsLimit();
	GetEventFilename(g_pendingBargraphRecord.summary.eventNumber);

	if (f_open(&file, (const TCHAR*)g_spareFileName, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
	{
		debugErr("Unable to create event file: %s, mode: %s\r\n", g_spareFileName, (g_triggerRecord.opMode == BARGRAPH_MODE) ? "Bargraph" : "Combo - Bargraph");
	}
	else // File created, write out the event
	{
		// Write in the current but unfinished event record to provide an offset to start writing in the data
		f_write(&file, &g_pendingBargraphRecord, sizeof(EVT_RECORD), (UINT*)&bytesWritten);
		g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
		f_close(&file);
		SetFileTimestamp(g_spareFileName);

		debug("%s event file closed\r\n", (g_triggerRecord.opMode == BARGRAPH_MODE) ? "Bargraph" : "Combo - Bargraph");
	}

	// Event validation in the Event number cache will happen at the end of Bargraph (to prevent DQM from picking up the Active Bargraph event early)

	// Consume event number (also allow other events to be recorded during a Combo - Bargraph session)
	StoreCurrentEventNumber();

	if (g_triggerRecord.opMode == COMBO_MODE)
	{
		// Update the Waveform pending record with the new event number to use
		g_pendingEventRecord.summary.eventNumber = g_nextEventNumberToUse;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MoveUpdatedBargraphEventRecordToFile(uint8 status)
{
	uint32 compressSize;
	uint32 dataLength = 0;

	FIL file;
	uint32_t bytesMoved;

	UNUSED(compressSize); // Depends on debug on/off

	// The following data will be filled in when the data has been moved over to flash.
	g_pendingBargraphRecord.header.summaryChecksum = 0xAABB;
	g_pendingBargraphRecord.header.dataChecksum = 0xCCDD;
	g_pendingBargraphRecord.header.dataCompression = 0;

	g_pendingBargraphRecord.summary.captured.endTime = GetCurrentTime();

	if (status == BARPGRAPH_SESSION_COMPLETE)
	{
		g_pendingBargraphRecord.summary.captured.bargraphSessionComplete = YES;
	}

	if (g_triggerRecord.opMode == COMBO_MODE)
	{
		// Mark Combo - Waveform event numbers captured
		if ((g_pendingBargraphRecord.summary.eventNumber + 1) == g_nextEventNumberToUse)
		{
			debug("Combo - Bargraph: No other events recorded during session\r\n");
		}
		else // ((g_pendingBargraphRecord.summary.eventNumber + 1) != g_nextEventNumberToUse)
		{
			g_pendingBargraphRecord.summary.captured.comboEventsRecordedDuringSession = (g_nextEventNumberToUse - (g_pendingBargraphRecord.summary.eventNumber + 1));
			g_pendingBargraphRecord.summary.captured.comboEventsRecordedStartNumber = (g_pendingBargraphRecord.summary.eventNumber + 1);
			g_pendingBargraphRecord.summary.captured.comboEventsRecordedEndNumber = (g_nextEventNumberToUse - 1);

			debug("Combo - Bargraph: Events recorded during session, Total: %d (%d to %d)\r\n", g_pendingBargraphRecord.summary.captured.comboEventsRecordedDuringSession,
					g_pendingBargraphRecord.summary.captured.comboEventsRecordedStartNumber, g_pendingBargraphRecord.summary.captured.comboEventsRecordedEndNumber);
		}
	}

	GetEventFilename(g_pendingBargraphRecord.summary.eventNumber);

	if (f_open(&file, (const TCHAR*)g_spareFileName, FA_OPEN_APPEND | FA_READ | FA_WRITE) != FR_OK)
	{
		debugErr("Unable to re-open event file for overwritting summary: %s\r\n", g_spareFileName);
	}
	else // File opened, overwrite event summary
	{
		// Make sure at the beginning of the file
		f_lseek(&file, 0);

		// Rewrite the event record
		f_write(&file, &g_pendingBargraphRecord, sizeof(EVT_RECORD), (UINT*)&bytesMoved);

		// Setup for saving data compressed (if Bargraph session is complete)
		if (status == BARPGRAPH_SESSION_COMPLETE)
		{
			if (g_unitConfig.saveCompressedData != DO_NOT_SAVE_EXTRA_FILE_COMPRESSED_DATA)
			{
				// Make sure at the beginning of the data
				f_lseek(&file, sizeof(EVT_RECORD));

				dataLength = (f_size(&file) - sizeof(EVT_RECORD));

				// Cache the Bargraph data for compression event file creation below
				f_read(&file, (uint8*)&g_eventDataBuffer[0], dataLength, (UINT*)&bytesMoved);
			}
		}

		g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
		f_close(&file);
		SetFileTimestamp(g_spareFileName);

		debug("%s event file closed\r\n", (g_triggerRecord.opMode == BARGRAPH_MODE) ? "Bargraph" : "Combo - Bargraph");

		// Save compressed data file (if Bargraph session is complete)
		if (status == BARPGRAPH_SESSION_COMPLETE)
		{
			if (g_unitConfig.saveCompressedData != DO_NOT_SAVE_EXTRA_FILE_COMPRESSED_DATA)
			{
				// Get new compressed event filename and path
				GetERDataFilename(g_pendingBargraphRecord.summary.eventNumber);

				if ((f_open(&file, (const TCHAR*)g_spareFileName, FA_CREATE_ALWAYS | FA_WRITE)) != FR_OK)
				{
					debugErr("Unable to create ERdata event file: %s\r\n", g_spareFileName);
				}
				else // File created, write out the event
				{
					g_spareBufferIndex = 0;
					compressSize = lzo1x_1_compress((void*)&g_eventDataBuffer[0], dataLength, OUT_FILE);

					// Check if any remaining compressed data is queued
					if (g_spareBufferIndex)
					{
						// Finish writing the remaining compressed data
						f_write(&file, g_spareBuffer, g_spareBufferIndex, (UINT*)&bytesMoved);
						g_spareBufferIndex = 0;
					}
					debug("Bargraph Compressed Data length: %d (Matches file: %s)\r\n", compressSize, (compressSize == f_size(&file)) ? "Yes" : "No");

					// Done writing the event file, close the file handle
					g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
					f_close(&file);
					SetFileTimestamp(g_spareFileName);
				}
			}

			AddEventToSummaryList(&g_pendingBargraphRecord);

			// Delay the addition of the valid event until the end of Bargraph (to prevent DQM from picking up the Active Bargraph event early)
			AddEventNumberToCache(g_pendingBargraphRecord.summary.eventNumber);

			UpdateSDCardUsageStats(sizeof(EVT_RECORD) + g_pendingBargraphRecord.header.dataLength);

#if 1 /* New ability to check if we need to send out a signal that this Bargraph event is available */
			// Check if AutoDialout is enabled and signal the system if necessary
			CheckAutoDialoutStatusAndFlagIfAvailable();
#endif
		}
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AdvanceBarIntervalBufPtr(uint8 bufferType)
{
	if (bufferType == READ_PTR)
	{
		g_bargraphBarIntervalReadPtr++;
		if (g_bargraphBarIntervalReadPtr >= g_bargraphBarIntervalEndPtr)
		{
			g_bargraphBarIntervalReadPtr = (BARGRAPH_BAR_INTERVAL_DATA*)&g_eventDataBuffer[0];
		}
	}
	else if (bufferType == BLM_READ_PTR)
	{
		g_bargraphBarIntervalLiveMonitoringReadPtr++;
		if (g_bargraphBarIntervalLiveMonitoringReadPtr >= g_bargraphBarIntervalEndPtr)
		{
			g_bargraphBarIntervalLiveMonitoringReadPtr = (BARGRAPH_BAR_INTERVAL_DATA*)&g_eventDataBuffer[0];
		}
	}
	else // (bufferType == WRITE_PTR)
	{
#if 1 /* Bargraph live monitoring */
		// Set the sequence count to the current BI capture plus the total BI's captured to this point (summary intervals * BI's/SI)
		g_bargraphBarIntervalWritePtr->barIntervalCount = (uint16)g_bargraphSummaryInterval.barIntervalsCaptured;
		g_bargraphBarIntervalWritePtr->summaryIntervalCount = g_summaryCount;
		g_bargraphBarIntervalWritePtr->currentBargraphEventNumber = g_pendingBargraphRecord.summary.eventNumber;
		if (g_triggerRecord.opMode == BARGRAPH_MODE) {	g_bargraphBarIntervalWritePtr->currentComboWaveformEventNumber = 0; }
		else { g_bargraphBarIntervalWritePtr->currentComboWaveformEventNumber = GetLastStoredEventNumber(); }
		g_bargraphBarIntervalWritePtr->epochTime = ConvertDateTimeToEpochTime(GetCurrentTime());

		g_blmBarIntervalQueueCount++;
#endif
		g_bargraphBarIntervalWritePtr++;
		if (g_bargraphBarIntervalWritePtr >= g_bargraphBarIntervalEndPtr)
		{
			g_bargraphBarIntervalWritePtr = (BARGRAPH_BAR_INTERVAL_DATA*)&g_eventDataBuffer[0];
		}

		memset(g_bargraphBarIntervalWritePtr, 0, sizeof(BARGRAPH_BAR_INTERVAL_DATA));
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UpdateBargraphJobTotals(void)
{
	// Update Bargraph Job Totals structure with most recent Summary Interval max's
	// ---------
	// A channel
	// ---------
	if (g_bargraphSummaryInterval.a.peak > g_pendingBargraphRecord.summary.calculated.a.peak)
	{
		g_pendingBargraphRecord.summary.calculated.a = g_bargraphSummaryInterval.a;
		g_pendingBargraphRecord.summary.calculated.a_Time = g_bargraphSummaryInterval.a_Time;
	}
	else if (g_bargraphSummaryInterval.a.peak == g_pendingBargraphRecord.summary.calculated.a.peak)
	{
		// Summary Interval frequency is stored as a count, the higher the count being a lower frequency which is more desired to save
		if (g_bargraphSummaryInterval.a.frequency > g_pendingBargraphRecord.summary.calculated.a.frequency)
		{
			g_pendingBargraphRecord.summary.calculated.a = g_bargraphSummaryInterval.a;
			g_pendingBargraphRecord.summary.calculated.a_Time = g_bargraphSummaryInterval.a_Time;
		}
	}

	// ---------
	// R channel
	// ---------
	if (g_bargraphSummaryInterval.r.peak > g_pendingBargraphRecord.summary.calculated.r.peak)
	{
		g_pendingBargraphRecord.summary.calculated.r = g_bargraphSummaryInterval.r;
		g_pendingBargraphRecord.summary.calculated.r_Time = g_bargraphSummaryInterval.r_Time;
	}
	else if (g_bargraphSummaryInterval.r.peak == g_pendingBargraphRecord.summary.calculated.r.peak)
	{
		// Summary Interval frequency is stored as a count, the higher the count being a lower frequency which is more desired to save
		if (g_bargraphSummaryInterval.r.frequency > g_pendingBargraphRecord.summary.calculated.r.frequency)
		{
			g_pendingBargraphRecord.summary.calculated.r = g_bargraphSummaryInterval.r;
			g_pendingBargraphRecord.summary.calculated.r_Time = g_bargraphSummaryInterval.r_Time;
		}
	}

	// ---------
	// V channel
	// ---------
	if (g_bargraphSummaryInterval.v.peak > g_pendingBargraphRecord.summary.calculated.v.peak)
	{
		g_pendingBargraphRecord.summary.calculated.v = g_bargraphSummaryInterval.v;
		g_pendingBargraphRecord.summary.calculated.v_Time = g_bargraphSummaryInterval.v_Time;
	}
	else if (g_bargraphSummaryInterval.v.peak == g_pendingBargraphRecord.summary.calculated.v.peak)
	{
		// Summary Interval frequency is stored as a count, the higher the count being a lower frequency which is more desired to save
		if (g_bargraphSummaryInterval.v.frequency > g_pendingBargraphRecord.summary.calculated.v.frequency)
		{
			g_pendingBargraphRecord.summary.calculated.v = g_bargraphSummaryInterval.v;
			g_pendingBargraphRecord.summary.calculated.v_Time = g_bargraphSummaryInterval.v_Time;
		}
	}

	// ---------
	// T channel
	// ---------
	if (g_bargraphSummaryInterval.t.peak > g_pendingBargraphRecord.summary.calculated.t.peak)
	{
		g_pendingBargraphRecord.summary.calculated.t = g_bargraphSummaryInterval.t;
		g_pendingBargraphRecord.summary.calculated.t_Time = g_bargraphSummaryInterval.t_Time;
	}
	else if (g_bargraphSummaryInterval.t.peak == g_pendingBargraphRecord.summary.calculated.t.peak)
	{
		// Summary Interval frequency is stored as a count, the higher the count being a lower frequency which is more desired to save
		if (g_bargraphSummaryInterval.t.frequency > g_pendingBargraphRecord.summary.calculated.t.frequency)
		{
			g_pendingBargraphRecord.summary.calculated.t = g_bargraphSummaryInterval.t;
			g_pendingBargraphRecord.summary.calculated.t_Time = g_bargraphSummaryInterval.t_Time;
		}
	}

	// ----------
	// Vector Sum
	// ----------
	if (g_bargraphSummaryInterval.vectorSumPeak > g_pendingBargraphRecord.summary.calculated.vectorSumPeak)
	{
		g_pendingBargraphRecord.summary.calculated.vectorSumPeak = g_bargraphSummaryInterval.vectorSumPeak;
		g_pendingBargraphRecord.summary.calculated.vs_Time = g_bargraphSummaryInterval.vs_Time;
	}

	g_pendingBargraphRecord.summary.calculated.summariesCaptured = g_bargraphSummaryInterval.summariesCaptured;
	g_pendingBargraphRecord.summary.calculated.barIntervalsCaptured += g_bargraphSummaryInterval.barIntervalsCaptured;
	g_pendingBargraphRecord.summary.calculated.intervalEnd_Time = g_bargraphSummaryInterval.intervalEnd_Time;
	g_pendingBargraphRecord.summary.calculated.batteryLevel = g_bargraphSummaryInterval.batteryLevel;
	g_pendingBargraphRecord.summary.calculated.calcStructEndFlag = 0xEECCCCEE;
}
