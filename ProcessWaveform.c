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
#include "M23018.h"
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
void MoveWaveformEventToFile(void)
{
	static WAVE_PROCESSING_STATE waveformProcessingState = WAVE_INIT;
	static int32 sampGrpsLeft;
	static uint32 vectorSumMax;
	static uint16* aWaveformPeakPtr;
	static uint16* rWaveformPeakPtr;
	static uint16* vWaveformPeakPtr;
	static uint16* tWaveformPeakPtr;

	uint16 normalizedData;
	uint32 i;
	uint16 sample;
	uint32 vectorSum;
	uint16 tempPeak;
	INPUT_MSG_STRUCT msg;
	uint16* startOfEventPtr;
	uint16* endOfEventDataPtr;

#if 0 /* old hw */
	uint8 keypadLedConfig;
#endif
	uint32 remainingDataLength;
	uint32 compressSize;
	uint16* tempDataPtr;
	char spaceLeftBuffer[25] = {'\0'};

	FIL file;
	uint32_t bytesWritten;

	UNUSED(compressSize); // Depends on debug on/off

	if ((g_freeEventBuffers < g_maxEventBuffers) && ((g_sdCardUsageStats.waveEventsLeft != 0)))
	{
		switch (waveformProcessingState)
		{
			case WAVE_INIT:
				// Save event start time with buffered timestamp
				g_pendingEventRecord.summary.captured.eventTime = g_eventDateTimeStampBuffer[g_eventBufferReadIndex].triggerTime;

#if 0 /* old hw */
				if ((gpio_get_pin_value(AVR32_PIN_PB14) == 0) && (g_epochTimeGPS))
#else
				if (0)
#endif
				{
					g_pendingEventRecord.summary.captured.eventTime.valid = YES;
					g_pendingEventRecord.summary.captured.gpsEpochTriggerTime = g_eventDateTimeStampBuffer[g_eventBufferReadIndex].gpsEpochTriggerTime;
					g_pendingEventRecord.summary.captured.gpsFractionalSecond = g_eventDateTimeStampBuffer[g_eventBufferReadIndex].gpsFractionalSecond;
				}

				if (getSystemEventState(EXT_TRIGGER_EVENT))
				{
					// Mark in the pending event record that this due to an External trigger signal
					g_pendingEventRecord.summary.captured.externalTrigger = YES;

					clearSystemEventFlag(EXT_TRIGGER_EVENT);
				}

				// Clear out A, R, V, T channel calculated data (in case the pending event record is reused)
				memset(&g_pendingEventRecord.summary.calculated.a, 0, (NUMBER_OF_CHANNELS_DEFAULT * sizeof(CHANNEL_CALCULATED_DATA_STRUCT)));

				waveformProcessingState = WAVE_PRETRIG;
				break;

			case WAVE_PRETRIG:
				for (i = g_samplesInPretrig; i != 0; i--)
				{
					if (g_bitShiftForAccuracy) AdjustSampleForBitAccuracy();

					g_currentEventSamplePtr += NUMBER_OF_CHANNELS_DEFAULT;
				}

				waveformProcessingState = WAVE_BODY_INIT;
				break;

			case WAVE_BODY_INIT:
				sampGrpsLeft = (g_samplesInBody - 1);

				if (g_bitShiftForAccuracy) AdjustSampleForBitAccuracy();

				// A channel
				sample = *(g_currentEventSamplePtr + A_CHAN_OFFSET);
				g_pendingEventRecord.summary.calculated.a.peak = FixDataToZero(sample);
				aWaveformPeakPtr = (g_currentEventSamplePtr + A_CHAN_OFFSET);

				// R channel
				sample = *(g_currentEventSamplePtr + R_CHAN_OFFSET);
				tempPeak = g_pendingEventRecord.summary.calculated.r.peak = FixDataToZero(sample);
				vectorSum = (uint32)(tempPeak * tempPeak);
				rWaveformPeakPtr = (g_currentEventSamplePtr + R_CHAN_OFFSET);

				// V channel
				sample = *(g_currentEventSamplePtr + V_CHAN_OFFSET);
				tempPeak = g_pendingEventRecord.summary.calculated.v.peak = FixDataToZero(sample);
				vectorSum += (uint32)(tempPeak * tempPeak);
				vWaveformPeakPtr = (g_currentEventSamplePtr + V_CHAN_OFFSET);

				// T channel
				sample = *(g_currentEventSamplePtr + T_CHAN_OFFSET);
				tempPeak = g_pendingEventRecord.summary.calculated.t.peak = FixDataToZero(sample);
				vectorSum += (uint32)(tempPeak * tempPeak);
				tWaveformPeakPtr = (g_currentEventSamplePtr + T_CHAN_OFFSET);

				vectorSumMax = (uint32)vectorSum;

				g_currentEventSamplePtr += NUMBER_OF_CHANNELS_DEFAULT;

				waveformProcessingState = WAVE_BODY;
				break;

			case WAVE_BODY:
				for (i = 0; ((i < g_triggerRecord.trec.sample_rate) && (sampGrpsLeft != 0)); i++)
				{
					sampGrpsLeft--;

					if (g_bitShiftForAccuracy) AdjustSampleForBitAccuracy();

					// A channel
					sample = *(g_currentEventSamplePtr + A_CHAN_OFFSET);
					normalizedData = FixDataToZero(sample);
					if (normalizedData > g_pendingEventRecord.summary.calculated.a.peak)
					{
						g_pendingEventRecord.summary.calculated.a.peak = normalizedData;
						aWaveformPeakPtr = (g_currentEventSamplePtr + A_CHAN_OFFSET);
					}

					// R channel
					sample = *(g_currentEventSamplePtr + R_CHAN_OFFSET);
					normalizedData = FixDataToZero(sample);
					if (normalizedData > g_pendingEventRecord.summary.calculated.r.peak)
					{
						g_pendingEventRecord.summary.calculated.r.peak = normalizedData;
						rWaveformPeakPtr = (g_currentEventSamplePtr + R_CHAN_OFFSET);
					}
					vectorSum = (uint32)(normalizedData * normalizedData);

					// V channel
					sample = *(g_currentEventSamplePtr + V_CHAN_OFFSET);
					normalizedData = FixDataToZero(sample);
					if (normalizedData > g_pendingEventRecord.summary.calculated.v.peak)
					{
						g_pendingEventRecord.summary.calculated.v.peak = normalizedData;
						vWaveformPeakPtr = (g_currentEventSamplePtr + V_CHAN_OFFSET);
					}
					vectorSum += (normalizedData * normalizedData);

					// T channel
					sample = *(g_currentEventSamplePtr + T_CHAN_OFFSET);
					normalizedData = FixDataToZero(sample);
					if (normalizedData > g_pendingEventRecord.summary.calculated.t.peak)
					{
						g_pendingEventRecord.summary.calculated.t.peak = normalizedData;
						tWaveformPeakPtr = (g_currentEventSamplePtr + T_CHAN_OFFSET);
					}
					vectorSum += (normalizedData * normalizedData);

					// Vector Sum
					if (vectorSum > vectorSumMax)
					{
						vectorSumMax = (uint32)vectorSum;
					}

					g_currentEventSamplePtr += NUMBER_OF_CHANNELS_DEFAULT;
				}

				if (sampGrpsLeft == 0)
				{
					g_pendingEventRecord.summary.calculated.vectorSumPeak = vectorSumMax;

					waveformProcessingState = WAVE_CAL_PULSE;
				}
				break;

			case WAVE_CAL_PULSE:
				for (i = g_samplesInCal; i != 0; i--)
				{
					if (g_bitShiftForAccuracy) AdjustSampleForBitAccuracy();
					
					g_currentEventSamplePtr += NUMBER_OF_CHANNELS_DEFAULT;
				}

				waveformProcessingState = WAVE_CALCULATE;
				break;

			case WAVE_CALCULATE:
				startOfEventPtr = g_startOfEventBufferPtr + (g_eventBufferReadIndex * g_wordSizeInEvent);
				endOfEventDataPtr = startOfEventPtr + (g_wordSizeInPretrig + g_wordSizeInEvent);

				g_pendingEventRecord.summary.calculated.a.frequency = CalcSumFreq(aWaveformPeakPtr, g_triggerRecord.trec.sample_rate, startOfEventPtr, endOfEventDataPtr);
				g_pendingEventRecord.summary.calculated.r.frequency = CalcSumFreq(rWaveformPeakPtr, g_triggerRecord.trec.sample_rate, startOfEventPtr, endOfEventDataPtr);
				g_pendingEventRecord.summary.calculated.v.frequency = CalcSumFreq(vWaveformPeakPtr, g_triggerRecord.trec.sample_rate, startOfEventPtr, endOfEventDataPtr);
				g_pendingEventRecord.summary.calculated.t.frequency = CalcSumFreq(tWaveformPeakPtr, g_triggerRecord.trec.sample_rate, startOfEventPtr, endOfEventDataPtr);

				CompleteRamEventSummary();

				CacheResultsEventInfo((EVT_RECORD*)&g_pendingEventRecord);

				waveformProcessingState = WAVE_STORE;
				break;

			case WAVE_STORE:
				// Setup new event file name
				CheckStoredEventsCapEventsLimit();
				GetEventFilename(g_pendingEventRecord.summary.eventNumber);

				if (f_open(&file, (const TCHAR*)g_spareFileName, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
				{
					printf("<Error> Unable to create event file: %s, mode: %s\n", g_spareFileName, (g_triggerRecord.opMode == WAVEFORM_MODE) ? "Waveform" : "Combo - Waveform");
				}
				else // File created, write out the event
				{
					ActivateDisplayShortDuration(1);
					sprintf((char*)g_spareBuffer, "%s %s #%d %s... (%s)", (g_triggerRecord.opMode == WAVEFORM_MODE) ? getLangText(WAVEFORM_TEXT) : getLangText(COMBO_WAVEFORM_TEXT), getLangText(EVENT_TEXT),
							g_pendingEventRecord.summary.eventNumber, getLangText(BEING_SAVED_TEXT), getLangText(MAY_TAKE_TIME_TEXT));

					if (g_sdCardUsageStats.waveEventsLeft < 100)
					{
						sprintf((char*)spaceLeftBuffer, "%d EVTS TILL FULL", (g_sdCardUsageStats.waveEventsLeft - 1));
						OverlayMessage((char*)spaceLeftBuffer, (char*)g_spareBuffer, 0);
					}
					else
					{
						OverlayMessage(getLangText(EVENT_COMPLETE_TEXT), (char*)g_spareBuffer, 0);
					}

					// Write the event record header and summary
					f_write(&file, &g_pendingEventRecord, sizeof(EVT_RECORD), (UINT*)&bytesWritten);

					if (bytesWritten != sizeof(EVT_RECORD))
					{
						debugErr("Waveform Event Record written size incorrect (%d)\r\n", bytesWritten);
					}

					remainingDataLength = (g_wordSizeInEvent * 2);

					// Check if there are multiple chunks to write
					if (remainingDataLength > WAVEFORM_FILE_WRITE_CHUNK_SIZE)
					{
#if 0 /* old hw */
						// Get the current state of the keypad LED
						keypadLedConfig = ReadMcp23018(IO_ADDRESS_KPD, GPIOA);
#endif
					}

					tempDataPtr = g_currentEventStartPtr;

					while (remainingDataLength)
					{
						if (remainingDataLength > WAVEFORM_FILE_WRITE_CHUNK_SIZE)
						{
							// Write the event data, containing the Pretrigger, event and cal
							f_write(&file, tempDataPtr, WAVEFORM_FILE_WRITE_CHUNK_SIZE, (UINT*)&bytesWritten);

							if (bytesWritten != WAVEFORM_FILE_WRITE_CHUNK_SIZE)
							{
								debugErr("Waveform Event Data written size incorrect (%d)\r\n", bytesWritten);
							}

							remainingDataLength -= WAVEFORM_FILE_WRITE_CHUNK_SIZE;
							tempDataPtr += (WAVEFORM_FILE_WRITE_CHUNK_SIZE / 2);

							// Quickly toggle the green LED to show status of saving a waveform event (while too busy to update LCD)
#if 0 /* old hw */
							if (keypadLedConfig & GREEN_LED_PIN) { keypadLedConfig &= ~GREEN_LED_PIN; }
							else { keypadLedConfig |= GREEN_LED_PIN; }
							WriteMcp23018(IO_ADDRESS_KPD, GPIOA, keypadLedConfig);
#endif
						}
						else // Remaining data size is less than the file write chunk size
						{
							// Write the event data, containing the Pretrigger, event and cal
							f_write(&file, tempDataPtr, remainingDataLength, (UINT*)&bytesWritten);

							if (bytesWritten != remainingDataLength)
							{
								debugErr("Waveform Event Data written size incorrect (%d)\r\n", bytesWritten);
							}

							remainingDataLength = 0;
						}
					}

					// Update the remaining space left
					UpdateSDCardUsageStats(f_size(&file));

					// Done writing the event file, close the file handle
					g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
					f_close(&file);
					SetFileTimestamp(g_spareFileName);

					//==========================================================================================================
					// Save compressed data file
					//----------------------------------------------------------------------------------------------------------
					if (g_unitConfig.saveCompressedData != DO_NOT_SAVE_EXTRA_FILE_COMPRESSED_DATA)
					{
						// Get new ERData compressed event file name
						GetERDataFilename(g_pendingEventRecord.summary.eventNumber);
						if ((f_open(&file, (const TCHAR*)g_spareFileName, FA_CREATE_ALWAYS | FA_WRITE)) != FR_OK)
						{
							printf("<Error> Unable to create ERdata event file: %s\n", g_spareFileName);
						}
						else // File created, write out the event
						{
							g_spareBufferIndex = 0;
							compressSize = lzo1x_1_compress((void*)g_currentEventStartPtr, (g_wordSizeInEvent * 2), OUT_FILE);

							// Check if any remaining compressed data is queued
							if (g_spareBufferIndex)
							{
								// Finish writing the remaining compressed data
								f_write(&file, g_spareBuffer, g_spareBufferIndex, (UINT*)&bytesWritten);
								g_spareBufferIndex = 0;
							}

							debug("Wave Compressed Data length: %d (Matches file: %s)\r\n", compressSize, (compressSize == f_size(&file)) ? "Yes" : "No");

							// Update the remaining space left
							UpdateSDCardUsageStats(f_size(&file));

							// Done writing the event file, close the file handle
							g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
							f_close(&file);
							SetFileTimestamp(g_spareFileName);
						}
					}
					//==========================================================================================================

					debug("Waveform Event file closed\r\n");

					AddEventToSummaryList(&g_pendingEventRecord);

					waveformProcessingState = WAVE_COMPLETE;
				}
				break;

			case WAVE_COMPLETE:
				UpdateMonitorLogEntry();

				AddEventNumberToCache(g_pendingEventRecord.summary.eventNumber);
				StoreCurrentEventNumber();

				// Reset External Trigger event record flag
				g_pendingEventRecord.summary.captured.externalTrigger = NO;

				// Now store the updated event number in the universal ram storage.
				g_pendingEventRecord.summary.eventNumber = g_nextEventNumberToUse;

				// Update event buffer count and pointers
				if (++g_eventBufferReadIndex == g_maxEventBuffers)
				{
					g_eventBufferReadIndex = 0;
					g_currentEventStartPtr = g_currentEventSamplePtr = g_startOfEventBufferPtr;
				}
				else
				{
					g_currentEventStartPtr = g_currentEventSamplePtr = g_startOfEventBufferPtr + (g_eventBufferReadIndex * g_wordSizeInEvent);
				}

				// Remove at some point - Currently does nothing since freeing of buffer happens below and a check is at the start
				if (g_freeEventBuffers == g_maxEventBuffers)
				{
					clearSystemEventFlag(TRIGGER_EVENT);
				}

				if (g_triggerRecord.opMode == WAVEFORM_MODE)
				{
					raiseMenuEventFlag(RESULTS_MENU_EVENT);
				}
				// else (g_triggerRecord.opMode == COMBO_MODE)
				// Leave in monitor mode menu display processing for bargraph

				//debug("DataBuffs: Changing flash move state: %s\r\n", "WAVE_INIT");
				waveformProcessingState = WAVE_INIT;
				g_freeEventBuffers++;

				if (GetPowerControlState(LCD_POWER_ENABLE) == OFF)
				{
					AssignSoftTimer(LCD_BACKLIGHT_ON_OFF_TIMER_NUM, LCD_BACKLIGHT_TIMEOUT, DisplayTimerCallBack);
					AssignSoftTimer(LCD_POWER_ON_OFF_TIMER_NUM, (uint32)(g_unitConfig.lcdTimeout * TICKS_PER_MIN), LcdPwTimerCallBack);
				}

				// Check to see if there is room for another event, if not send a signal to stop monitoring
				if (g_unitConfig.flashWrapping == NO)
				{
					if (g_sdCardUsageStats.waveEventsLeft == 0)
					{
						msg.cmd = STOP_MONITORING_CMD;
						msg.length = 1;
						(*g_menufunc_ptrs[MONITOR_MENU])(msg);
					}
				}

				// Check if AutoDialout is enabled and signal the system if necessary
				CheckAutoDialoutStatusAndFlagIfAvailable();
			break;
		}
	}
	else
	{
		clearSystemEventFlag(TRIGGER_EVENT);
		clearSystemEventFlag(EXT_TRIGGER_EVENT);

		// Check if not monitoring
		if (g_sampleProcessing != ACTIVE_STATE)
		{
 			// There were queued event buffers after monitoring was stopped
 			// Close the monitor log entry now since all events have been stored
			CloseMonitorLogEntry();
		}

		// Check if no more room for another event, then send a signal to stop monitoring
		if (g_unitConfig.flashWrapping == NO)
		{
			if (g_sdCardUsageStats.waveEventsLeft == 0)
			{
				msg.cmd = STOP_MONITORING_CMD;
				msg.length = 1;
				(*g_menufunc_ptrs[MONITOR_MENU])(msg);
			}
		}
	}
}
