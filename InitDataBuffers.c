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
void InitDataBuffs(uint8 opMode)
{ 
	uint32 pretriggerBufferSize;
	uint32 barIntervalOffsetInWords;
	uint32 sampleRate;
	
	if (opMode == MANUAL_CAL_MODE)
	{
		// Per requirement fix sample rate for Calibration
		sampleRate = MANUAL_CAL_DEFAULT_SAMPLE_RATE;

		// Variable Pretrigger size in words; sample rate / Pretrigger buffer divider times channels plus 1 extra sample (to ensure a full Pretrigger plus a trigger sample)
		pretriggerBufferSize = ((uint32)(sampleRate / MANUAL_CAL_PRETRIGGER_BUFFER_DIVIDER) * g_sensorInfo.numOfChannels) + g_sensorInfo.numOfChannels;
	}
	else // Waveform, Bargraph, Combo
	{
		sampleRate = g_triggerRecord.trec.sample_rate;

		// Variable Pretrigger size in words; sample rate / Pretrigger buffer divider times channels plus 1 extra sample (to ensure a full Pretrigger plus a trigger sample)
		pretriggerBufferSize = ((uint32)(sampleRate / g_unitConfig.pretrigBufferDivider) * g_sensorInfo.numOfChannels) + g_sensorInfo.numOfChannels;
	}

	// Setup the Pretrigger buffer pointers
	g_startOfPretriggerBuff = &(g_pretriggerBuff[0]);
	g_tailOfPretriggerBuff = &(g_pretriggerBuff[0]);

#if VT_FEATURE_DISABLED
	g_endOfPretriggerBuff = &(g_pretriggerBuff[pretriggerBufferSize]);
#else /* New VT feature */
	// Check if using standard trigger (not variable)
	if (g_triggerRecord.trec.variableTriggerEnable != YES)
	{
		// Cap the Pretrigger buffer right at the desired size
		g_endOfPretriggerBuff = &(g_pretriggerBuff[pretriggerBufferSize]);
	}
	else // Using variable trigger standard (USBM, OSM)
	{
		// Use full size of the Pretrigger buffer (which is 16K 1sec buffer, max VT is 8K but needs double space, so using the full Pretrigger size)
		g_endOfPretriggerBuff = &(g_pretriggerBuff[PRE_TRIG_BUFF_SIZE_IN_WORDS]);
	}
#endif

	// Setup Bit Accuracy globals
	if (opMode == MANUAL_CAL_MODE)
		{ g_bitAccuracyMidpoint = ACCURACY_16_BIT_MIDPOINT; g_bitShiftForAccuracy = AD_BIT_ACCURACY - ACCURACY_16_BIT; }
	else if (g_triggerRecord.trec.bitAccuracy == ACCURACY_10_BIT)
		{ g_bitAccuracyMidpoint = ACCURACY_10_BIT_MIDPOINT; g_bitShiftForAccuracy = AD_BIT_ACCURACY - ACCURACY_10_BIT; }
	else if (g_triggerRecord.trec.bitAccuracy == ACCURACY_12_BIT) 
		{ g_bitAccuracyMidpoint = ACCURACY_12_BIT_MIDPOINT; g_bitShiftForAccuracy = AD_BIT_ACCURACY - ACCURACY_12_BIT; }
	else if (g_triggerRecord.trec.bitAccuracy == ACCURACY_14_BIT) 
		{ g_bitAccuracyMidpoint = ACCURACY_14_BIT_MIDPOINT; g_bitShiftForAccuracy = AD_BIT_ACCURACY - ACCURACY_14_BIT; }
	else // Default to 16-bit accuracy
		{ g_bitAccuracyMidpoint = ACCURACY_16_BIT_MIDPOINT; g_bitShiftForAccuracy = AD_BIT_ACCURACY - ACCURACY_16_BIT; } 

	// Setup the pending event record information that is available at this time
	InitEventRecord(opMode);

	// Setup buffers based on mode
	//=============================================================================================
	if ((opMode == WAVEFORM_MODE) || (opMode == MANUAL_CAL_MODE) || (opMode == COMBO_MODE))
	//---------------------------------------------------------------------------------------------
	{
		// Calculate samples for each section and total event
		g_samplesInPretrig = (uint32)(sampleRate / g_unitConfig.pretrigBufferDivider);
		g_samplesInBody = (uint32)(sampleRate * g_triggerRecord.trec.record_time);
		g_samplesInCal = (uint32)MAX_CAL_SAMPLES;
		g_samplesInEvent = g_samplesInPretrig + g_samplesInBody + g_samplesInCal;

		// Calculate word size for each section and total event, since buffer is an array of words
		g_wordSizeInPretrig = g_samplesInPretrig * g_sensorInfo.numOfChannels;
		g_wordSizeInBody = g_samplesInBody * g_sensorInfo.numOfChannels;
		g_wordSizeInCal = g_samplesInCal * g_sensorInfo.numOfChannels;
		g_wordSizeInEvent = g_wordSizeInPretrig + g_wordSizeInBody + g_wordSizeInCal;

		if (opMode == COMBO_MODE)
		{
			// Calculate total event buffers available (partial event buffer size)
			g_maxEventBuffers = (uint16)((EVENT_BUFF_SIZE_IN_WORDS - COMBO_MODE_BARGRAPH_BUFFER_SIZE_WORDS) / (g_wordSizeInEvent));

			// Init starting event buffer pointer beyond the reserved area for Combo - Bargraph
			g_startOfEventBufferPtr = &(g_eventDataBuffer[COMBO_MODE_BARGRAPH_BUFFER_SIZE_WORDS]);
		}
		else // ((opMode == WAVEFORM_MODE) || (opMode == MANUAL_CAL_MODE))
		{
			// Calculate total event buffers available (full event buffer size)
			g_maxEventBuffers = (uint16)(EVENT_BUFF_SIZE_IN_WORDS / (g_wordSizeInEvent));

			// Init starting event buffer pointer
			g_startOfEventBufferPtr = &(g_eventDataBuffer[0]);
		}

		g_freeEventBuffers = g_maxEventBuffers;
		g_eventBufferReadIndex = 0;
		g_eventBufferWriteIndex = 0;

		g_currentEventSamplePtr = g_currentEventStartPtr = g_startOfEventBufferPtr;
		g_eventBufferPretrigPtr = g_startOfEventBufferPtr;
		g_eventBufferBodyPtr = g_eventBufferPretrigPtr + g_wordSizeInPretrig;
		g_eventBufferCalPtr = g_eventBufferPretrigPtr + g_wordSizeInPretrig + g_wordSizeInBody;

		// Init flags
		g_isTriggered = 0;
		g_processingCal = 0;
		g_calTestExpected = 0;
		g_doneTakingEvents = NO;
		
		// Update the data length to be used based on the size calculations
		if (opMode == MANUAL_CAL_MODE)
		{
			// Update the manual cal data length to be used based on the size calculations
			g_pendingEventRecord.header.dataLength = (g_wordSizeInCal * 2);
		}
		else // ((opMode == WAVEFORM_MODE) || (opMode == COMBO_MODE))
		{
			// Update the waveform data length to be used based on the size calculations
			g_pendingEventRecord.header.dataLength = (g_wordSizeInEvent * 2);
		}
	}

	//=============================================================================================
	if ((opMode == BARGRAPH_MODE) || (opMode == COMBO_MODE))
	//---------------------------------------------------------------------------------------------
	{
		// Get the total size (in words) needed for caching the total number of Bar Intervals for the Summary Interval period selected (+1 spare)
		barIntervalOffsetInWords = ((((g_triggerRecord.bgrec.summaryInterval / g_triggerRecord.bgrec.barInterval) + 1) * sizeof(BARGRAPH_BAR_INTERVAL_DATA)) / 2);

		// Init Bar Interval pointers
		g_bargraphBarIntervalWritePtr = (BARGRAPH_BAR_INTERVAL_DATA*)&(g_eventDataBuffer[0]);
		g_bargraphBarIntervalReadPtr = (BARGRAPH_BAR_INTERVAL_DATA*)&(g_eventDataBuffer[0]);
		g_bargraphBarIntervalLiveMonitoringReadPtr = (BARGRAPH_BAR_INTERVAL_DATA*)&(g_eventDataBuffer[0]);
		g_bargraphBarIntervalEndPtr = (BARGRAPH_BAR_INTERVAL_DATA*)&(g_eventDataBuffer[barIntervalOffsetInWords]);

		g_bargraphDataStartPtr = &(g_eventDataBuffer[barIntervalOffsetInWords]);
		g_bargraphDataWritePtr = &(g_eventDataBuffer[barIntervalOffsetInWords]);
		g_bargraphDataReadPtr = &(g_eventDataBuffer[barIntervalOffsetInWords]);

		if (opMode == COMBO_MODE)
		{
			g_bargraphDataEndPtr = &(g_eventDataBuffer[COMBO_MODE_BARGRAPH_BUFFER_SIZE_WORDS]);
		}
		else // (opMode == BARGRAPH_MODE)
		{
			g_bargraphDataEndPtr = &(g_eventDataBuffer[EVENT_BUFF_SIZE_IN_WORDS]);
		}

		// Start the total off with zero (incremented when bar and summary intervals are stored)
		g_pendingBargraphRecord.header.dataLength = 0;
		
		StartNewBargraph();
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16 CalcSumFreq(uint16* peakPtr, uint32 sampleRate, uint16* startAddrPtr, uint16* endAddrPtr)
{
	uint16* samplePtr;
	uint32 sampleCount = 0;
	uint16 freq = 0;
	uint16 freqValidPeakAdjusted = (FREQ_VALID_PEAK_16_BIT >> (g_bitShiftForAccuracy >> 1));
	uint16 freqCrossoverBackwardAdjusted = (FREQ_CROSSOVER_BACKWARD_16_BIT >> (g_bitShiftForAccuracy >> 1));
	uint16 freqCrossoverForwardAdjusted = (FREQ_CROSSOVER_FORWARD_16_BIT >> (g_bitShiftForAccuracy >> 1));

	//------------------------------------------------------------------------------------
	// Note: A/D data is RAW and has only been adjusted for Bit Accuracy
	//------------------------------------------------------------------------------------
	// g_bitAccuracyMidpoint reference tracks the current midpoint level for the current bit accuracy setting
	// 16-bit results in g_bitAccuracyMidpoint being equal to 0x8000
	// 14-bit results in g_bitAccuracyMidpoint being equal to 0x2000
	// 12-bit results in g_bitAccuracyMidpoint being equal to 0x800
	// 10-bit results in g_bitAccuracyMidpoint being equal to 0x200

	//------------------------------------------------------------------------------------
	// Set working pointer to the peak
	//------------------------------------------------------------------------------------
	samplePtr = peakPtr;

	//------------------------------------------------------------------------------------
	// Check if peak is above the midpoint adjusted by valid peak count
	//------------------------------------------------------------------------------------
	if (*samplePtr >= (g_bitAccuracyMidpoint + freqValidPeakAdjusted))
	{
		//------------------------------------------------------------------------------------
		// Continue while the A/D data is above the midpoint plus backwards crossover count
		//------------------------------------------------------------------------------------
		while ((*samplePtr >= (g_bitAccuracyMidpoint + freqCrossoverBackwardAdjusted)) && (samplePtr > (startAddrPtr + 4)))
		{
			// Decrement pointer by to get previous channel A/D reading
			samplePtr -= 4;

			// Increment the sample count
			sampleCount++;
		}

		//------------------------------------------------------------------------------------
		// Reset working pointer to peak (also means the peak will be counted twice)
		//------------------------------------------------------------------------------------
		samplePtr = peakPtr;

		//------------------------------------------------------------------------------------
		// Continue while the A/D data is above the midpoint adjusted by forwards crossover count
		//------------------------------------------------------------------------------------
		while ((*samplePtr >= (g_bitAccuracyMidpoint + freqCrossoverForwardAdjusted)) && (samplePtr < (endAddrPtr - 4)))
		{
			// Increment pointer by to get next channel A/D reading
			samplePtr += 4;

			// Increment the sample count
			sampleCount++;
		}
	}
	//------------------------------------------------------------------------------------
	// Check if peak is below the midpoint adjusted by valid peak count
	//------------------------------------------------------------------------------------
	else if (*samplePtr <= (g_bitAccuracyMidpoint - freqValidPeakAdjusted))
	{
		//------------------------------------------------------------------------------------
		// Continue while the A/D data is below the midpoint adjusted by backwards crossover count
		//------------------------------------------------------------------------------------
		while ((*samplePtr <= (g_bitAccuracyMidpoint - freqCrossoverBackwardAdjusted)) && (samplePtr > (startAddrPtr + 4)))
		{
			// Decrement pointer by to get previous channel A/D reading
			samplePtr -= 4;

			// Increment the sample count
			sampleCount++;
		}

		//------------------------------------------------------------------------------------
		// Reset working pointer to peak (also means the peak will be counted twice)
		//------------------------------------------------------------------------------------
		samplePtr = peakPtr;

		//------------------------------------------------------------------------------------
		// Continue while the A/D data is below the midpoint adjusted by forwards crossover count
		//------------------------------------------------------------------------------------
		while ((*samplePtr <= (g_bitAccuracyMidpoint - freqCrossoverForwardAdjusted)) && (samplePtr < (endAddrPtr - 4)))
		{
			// Increment pointer by to get next channel A/D reading
			samplePtr += 4;

			// Increment the sample count
			sampleCount++;
		}
	}

	//------------------------------------------------------------------------------------
	// Peak is too low for finding frequency
	//------------------------------------------------------------------------------------
	else
	{
		//------------------------------------------------------------------------------------
		// Peak was less than 4 counts, assume to be noise
		//------------------------------------------------------------------------------------
		return (0);
	}

	//------------------------------------------------------------------------------------
	// Total counts between 0 crossings (subtract 1 for duplicate peak being counted)
	//------------------------------------------------------------------------------------
	sampleCount = (uint32)((sampleCount - 1) * 2);

	//------------------------------------------------------------------------------------
	// Verify a divide by zero can't occur
	//------------------------------------------------------------------------------------
	if (sampleCount > 0)
	{
		//------------------------------------------------------------------------------------
		// Calculate frequency with given sample rate and multiply by 10 to provide one decimal place but save as an integer
		//------------------------------------------------------------------------------------
		freq = (uint16)(((float)(sampleRate * 10) / (float)sampleCount));
	}

	return (freq);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16 FixDataToZero(uint16 data_)
{
	if (data_ > g_bitAccuracyMidpoint)
		data_ = (uint16)(data_ - g_bitAccuracyMidpoint);
	else
		data_ = (uint16)(g_bitAccuracyMidpoint - data_);

	return (data_);
}
