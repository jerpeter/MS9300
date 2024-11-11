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
#include <string.h>
#include <math.h>
#include "Menu.h"
#include "OldUart.h"
#include "Display.h"
#include "Common.h"
#include "InitDataBuffers.h"
#include "Summary.h"
#include "SysEvents.h"
#include "Board.h"
#include "PowerManagement.h"
#include "Record.h"
#include "SoftTimer.h"
#include "Keypad.h"
#include "ProcessBargraph.h"
#include "TextTypes.h"
#include "EventProcessing.h"
#include "Analog.h"
#include "Globals.h"
#include "RealTimeClock.h"
#include "RemoteOperation.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Local Scope Globals
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------
void DataIsrInit(uint16 sampleRate);

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void StartMonitoring(uint8 operationMode, TRIGGER_EVENT_DATA_STRUCT* opModeParamsPtr)
{
	// Check if any events are still stored in buffers and need to be stored into flash
	if (getSystemEventState(TRIGGER_EVENT))
	{
		while (getSystemEventState(TRIGGER_EVENT))
		{
			MoveWaveformEventToFile();
		}
	}

	// Display a message to be patient while the software disables the power off key and setups up parameters
	OverlayMessage(getLangText(STATUS_TEXT), getLangText(PLEASE_BE_PATIENT_TEXT), 0);

	// Assign a one second menu update timer
	AssignSoftTimer(MENU_UPDATE_TIMER_NUM, ONE_SECOND_TIMEOUT, MenuUpdateTimerCallBack);

	// Requirements check only for Waveform mode to enforce that alarm levels can't be lower than the trigger level
	if (operationMode == WAVEFORM_MODE)
	{
		if ((g_unitConfig.alarmOneMode == ALARM_MODE_SEISMIC) || (g_unitConfig.alarmOneMode == ALARM_MODE_BOTH))
		{
			// No need to check for No Trigger setting since value is above trigger level threshold
			if (g_unitConfig.alarmOneSeismicLevel < opModeParamsPtr->seismicTriggerLevel)
			{
				g_unitConfig.alarmOneSeismicLevel = opModeParamsPtr->seismicTriggerLevel;
			}
		}

		if ((g_unitConfig.alarmOneMode == ALARM_MODE_AIR) || (g_unitConfig.alarmOneMode == ALARM_MODE_BOTH))
		{
			// No need to check for No Trigger setting since value is above trigger level threshold
			if (g_unitConfig.alarmOneAirLevel < opModeParamsPtr->airTriggerLevel)
			{
				g_unitConfig.alarmOneAirLevel = opModeParamsPtr->airTriggerLevel;
			}
		}

		if ((g_unitConfig.alarmTwoMode == ALARM_MODE_SEISMIC) || (g_unitConfig.alarmTwoMode == ALARM_MODE_BOTH))
		{
			// No need to check for No Trigger setting since value is above trigger level threshold
			if (g_unitConfig.alarmTwoSeismicLevel < opModeParamsPtr->seismicTriggerLevel)
			{
				g_unitConfig.alarmTwoSeismicLevel = opModeParamsPtr->seismicTriggerLevel;
			}
		}

		if ((g_unitConfig.alarmTwoMode == ALARM_MODE_AIR) || (g_unitConfig.alarmTwoMode == ALARM_MODE_BOTH))
		{
			// No need to check for No Trigger setting since value is above trigger level threshold
			if (g_unitConfig.alarmTwoAirLevel < opModeParamsPtr->airTriggerLevel)
			{
				g_unitConfig.alarmTwoAirLevel = opModeParamsPtr->airTriggerLevel;
			}
		}
	}

	//-------------------------
	// Debug Information
	//-------------------------

	if (operationMode == WAVEFORM_MODE)
	{
		debug("--- Waveform Mode Settings ---\r\n");
		debug("\tRecord Time: %d, Sample Rate: %d, Channels: %d\r\n", g_triggerRecord.trec.record_time, opModeParamsPtr->sample_rate, g_sensorInfo.numOfChannels);

		if (g_unitConfig.unitsOfAir == DECIBEL_TYPE)
		{
			debug("\tSeismic Trigger Count: 0x%x, Air Level: %d dB, Air Trigger Count: 0x%x\r\n", opModeParamsPtr->seismicTriggerLevel, AirTriggerConvertToUnits(opModeParamsPtr->airTriggerLevel), opModeParamsPtr->airTriggerLevel);
		}
		else // (g_unitConfig.unitsOfAir == MILLIBAR_TYPE) || (g_unitConfig.unitsOfAir == PSI_TYPE)
		{
			debug("\tSeismic Trigger Count: 0x%x, Air Level: %0.3f %s, Air Trigger Count: 0x%x\r\n", opModeParamsPtr->seismicTriggerLevel,
					(double)(((float)AirTriggerConvertToUnits(opModeParamsPtr->airTriggerLevel) / (float)10000)), (g_unitConfig.unitsOfAir == MILLIBAR_TYPE) ? "mb" : "psi", opModeParamsPtr->airTriggerLevel);
		}
	}
	else if (operationMode == BARGRAPH_MODE)
	{
		debug("--- Bargraph Mode Settings ---\r\n");
		debug("\tSample Rate: %d, Bar Interval: %d secs, Summary Interval: %d mins\r\n", opModeParamsPtr->sample_rate, g_triggerRecord.bgrec.barInterval,
				(g_triggerRecord.bgrec.summaryInterval / 60));
	}
	else if (operationMode == COMBO_MODE)
	{
		debug("--- Combo Mode Settings ---\r\n");
		debug("\tRecord Time: %d, Sample Rate: %d, Channels: %d\r\n", g_triggerRecord.trec.record_time, opModeParamsPtr->sample_rate, g_sensorInfo.numOfChannels);

		if (g_unitConfig.unitsOfAir == DECIBEL_TYPE)
		{
			debug("\tSeismic Trigger Count: 0x%x, Air Level: %d dB, Air Trigger Count: 0x%x\r\n", opModeParamsPtr->seismicTriggerLevel, AirTriggerConvertToUnits(opModeParamsPtr->airTriggerLevel), opModeParamsPtr->airTriggerLevel);
		}
		else // (g_unitConfig.unitsOfAir == MILLIBAR_TYPE) || (g_unitConfig.unitsOfAir == PSI_TYPE)
		{
			debug("\tSeismic Trigger Count: 0x%x, Air Level: %0.3f %s, Air Trigger Count: 0x%x\r\n", opModeParamsPtr->seismicTriggerLevel,
					(double)(((float)AirTriggerConvertToUnits(opModeParamsPtr->airTriggerLevel) / (float)10000)), (g_unitConfig.unitsOfAir == MILLIBAR_TYPE) ? "mb" : "psi", opModeParamsPtr->airTriggerLevel);
		}

		debug("\tBar Interval: %d secs, Summary Interval: %d mins\r\n", g_triggerRecord.bgrec.barInterval, (g_triggerRecord.bgrec.summaryInterval / 60));
	}
	else if (operationMode == MANUAL_TRIGGER_MODE)
	{
		debug("--- Manual Trigger Mode Settings ---\r\n");
	}
	else if (operationMode == MANUAL_CAL_MODE)
	{
		debug("--- Manual Cal Mode Settings ---\r\n");
		debug("\tSample Rate: %d, Channels: %d\r\n", opModeParamsPtr->sample_rate, g_sensorInfo.numOfChannels);
	}

	debug("\tAD Channel Verification: %s\r\n", ((opModeParamsPtr->sample_rate == SAMPLE_RATE_16K) || (g_unitConfig.adChannelVerification == DISABLED)) ? "Disabled" : "Enabled");
	debug("\tGeo Gain: %s, Air Path: %s\r\n", ((g_triggerRecord.srec.sensitivity == LOW) ? "Normal" : "High"), (((g_factorySetupRecord.aWeightOption == ENABLED) && (g_unitConfig.airScale == AIR_SCALE_A_WEIGHTING)) ? "A-weighted" : "AOP/Linear"));
	debug("---------------------------\r\n");

	if (GET_HARDWARE_ID == HARDWARE_ID_REV_8_WITH_GPS_MOD)
	{
		EnableGps();
	}

	// Check if mode is Manual Cal
	if (operationMode == MANUAL_CAL_MODE)
	{
		// Raise flag
		g_manualCalFlag = TRUE;
		g_manualCalSampleCount = MAX_CAL_SAMPLES;
	}
	else // Waveform, Bargraph, Combo
	{
		// Clear flag
		g_manualCalFlag = FALSE;
		g_manualCalSampleCount = 0;

		// Create a new monitor log entry
		NewMonitorLogEntry(operationMode);
	}

	if ((operationMode == BARGRAPH_MODE) || (operationMode == COMBO_MODE) || ((operationMode == WAVEFORM_MODE) && (g_unitConfig.autoCalForWaveform == ENABLED)))
	{
		ForcedCalibration();
	}

	// Initialize buffers and settings and gp_ramEventRecord
	debug("Init data buffers (mode: %d)\r\n", operationMode);
	InitDataBuffs(operationMode);

	// New Adaptive Sampling setup
	if (opModeParamsPtr->samplingMethod == ADAPTIVE_SAMPLING)
	{
		// Check if the unit config Adaptive Sampling is not enabled or if the sample rate is 1K which doesn't use Adaptive Sampling
		if ((g_unitConfig.adaptiveSampling != ENABLED) || (g_triggerRecord.trec.sample_rate == SAMPLE_RATE_1K))
		{
			opModeParamsPtr->samplingMethod = FIXED_SAMPLING;
		}
		else // Setup for Adaptive Sampling
		{
			// Trigger threshold to maintain lower sampling or boost sampling to selected rate
			// Seismic default 0.05 IPS = 160 counts A/D, minimum is 4 counts @ 12-bit low sensitivity (64 counts)
			if (opModeParamsPtr->seismicTriggerLevel < (DEFAULT_SEISMIC_TRIGGER_LEVEL_IN_INCHES_WITH_ADJUSTMENT * 16))
			{
				// Set adaptive threshold to 30% of trigger level if less than 0.05 IPS
				g_adaptiveSeismicThreshold = (uint16)((opModeParamsPtr->seismicTriggerLevel * 3) / 10);
			}
			else // Set adaptive threshold to 48 counts for any trigger levels above 0.05 IPS
			{
				g_adaptiveSeismicThreshold = 48;
			}

			// Acoustic lowest possible triggers are 92 dB (~51 counts) or 100 mb (40 counts)
			// 160 A/D counts is ~102 dB
			if (opModeParamsPtr->airTriggerLevel < 160)
			{
				// Set adaptive threshold to 30% of trigger level if less than ~102 dB
				g_adaptiveAcousticThreshold = (uint16)((opModeParamsPtr->airTriggerLevel * 3) / 10);
			}
			else // Set adaptive threshold to 48 counts (close to 92 dB) for any trigger levels above ~102 dB
			{
				g_adaptiveAcousticThreshold = 48;
			}

			g_adaptiveState = ADAPTIVE_MAX_RATE;
			g_adaptiveSampleDelay = opModeParamsPtr->sample_rate * 30; // Wait 30 seconds after start monitoring to check for adaptive sampling drop
			g_adaptiveBoundaryCount = opModeParamsPtr->sample_rate / SAMPLE_RATE_1K;
			g_adaptiveBoundaryMarker = 0;
			g_adaptiveLastRealSamplePtr = g_tailOfPretriggerBuff;
		}
	}
	else // Sampling method is FIXED_SAMPLING
	{
		g_adaptiveState = ADAPTIVE_DISABLED;
		g_adaptiveSampleDelay = 0;
		g_adaptiveBoundaryCount = 0;
		g_adaptiveBoundaryMarker = 0;
	}

#if 0 /* Moved to StartDataCollection since the Analog 5V isn't enabled at this point and the Analog controls can't be set yet */
	// Setup Analog controls
	debug("Setup Analog controls\r\n");

	// Set the cutoff frequency based on sample rate
	switch (opModeParamsPtr->sample_rate)
	{
		case SAMPLE_RATE_1K: SetAnalogCutoffFrequency(ANALOG_CUTOFF_FREQ_1K); break;
		case SAMPLE_RATE_2K: SetAnalogCutoffFrequency(ANALOG_CUTOFF_FREQ_1K); break;
		case SAMPLE_RATE_4K: SetAnalogCutoffFrequency(ANALOG_CUTOFF_FREQ_2K); break;
		case SAMPLE_RATE_8K: SetAnalogCutoffFrequency(ANALOG_CUTOFF_FREQ_4K); break;
		case SAMPLE_RATE_16K: SetAnalogCutoffFrequency(ANALOG_CUTOFF_FREQ_8K); break;
		case SAMPLE_RATE_32K: SetAnalogCutoffFrequency(ANALOG_CUTOFF_FREQ_16K); break;

		// Default just in case it's a custom frequency
		default: SetAnalogCutoffFrequency(ANALOG_CUTOFF_FREQ_1K); break;
	}

	// Set the sensitivity (aka gain) based on the current settings
	if (g_triggerRecord.srec.sensitivity == LOW) { SetSeismicGainSelect(SEISMIC_GAIN_NORMAL); }
	else { SetSeismicGainSelect(SEISMIC_GAIN_HIGH); }

	// Check if A-weighting is enabled
	if ((g_factorySetupRecord.aWeightOption == ENABLED) && (g_unitConfig.airScale == AIR_SCALE_A_WEIGHTING))
	{
		// Set acoustic for A-weighted gain
		SetAcousticPathSelect(ACOUSTIC_PATH_A_WEIGHTED);
	}
	// Set acoustic for normal gain
	else { SetAcousticPathSelect(ACOUSTIC_PATH_AOP); }
#endif

#if 0 /* Necessary? Probably need 1 sec for changes, however 1 sec worth of samples thrown away with getting channel offsets  */
	// Delay for Analog cutoff and gain select changes to propagate
	SoftUsecWait(500 * SOFT_MSECS);
#endif

	// Monitor some data.. oh yeah
	StartDataCollection(opModeParamsPtr->sample_rate);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void StartDataCollection(uint32 sampleRate)
{
	// Check if External ADC is in reset
	if (GetPowerControlState(ADC_RESET) == ON) { WaitAnalogPower5vGood(); }

	// Configure External ADC
	AD4695_Init();

#if 1 /* Moved from StartMonitoring since the Analog 5V wasn't enabled at that point to setup the Analog controls */
	// Setup Analog controls
	debug("Setup Analog controls\r\n");

	// Set the cutoff frequency based on sample rate
	switch (sampleRate)
	{
		case SAMPLE_RATE_1K: SetAnalogCutoffFrequency(ANALOG_CUTOFF_FREQ_1K); break;
		case SAMPLE_RATE_2K: SetAnalogCutoffFrequency(ANALOG_CUTOFF_FREQ_1K); break;
		case SAMPLE_RATE_4K: SetAnalogCutoffFrequency(ANALOG_CUTOFF_FREQ_2K); break;
		case SAMPLE_RATE_8K: SetAnalogCutoffFrequency(ANALOG_CUTOFF_FREQ_4K); break;
		case SAMPLE_RATE_16K: SetAnalogCutoffFrequency(ANALOG_CUTOFF_FREQ_8K); break;
		case SAMPLE_RATE_32K: SetAnalogCutoffFrequency(ANALOG_CUTOFF_FREQ_16K); break;

		// Default just in case it's a custom frequency
		default: SetAnalogCutoffFrequency(ANALOG_CUTOFF_FREQ_1K); break;
	}

	// Set the sensitivity (aka gain) based on the current settings
	if (g_triggerRecord.srec.sensitivity == LOW) { SetSeismicGainSelect(SEISMIC_GAIN_NORMAL); }
	else { SetSeismicGainSelect(SEISMIC_GAIN_HIGH); }

	// Check if A-weighting is enabled
	if ((g_factorySetupRecord.aWeightOption == ENABLED) && (g_unitConfig.airScale == AIR_SCALE_A_WEIGHTING))
	{
		// Set acoustic for A-weighted gain
		SetAcousticPathSelect(ACOUSTIC_PATH_A_WEIGHTED);
	}
	// Set acoustic for normal gain
	else { SetAcousticPathSelect(ACOUSTIC_PATH_AOP); }
#endif

	// Setup the A/D Channel configuration
	SetupADChannelConfig(sampleRate, UNIT_CONFIG_CHANNEL_VERIFICATION);

#if 1 /* Test adding delay for Analog stabilization before zeroing */
	SoftUsecWait(3 * SOFT_SECS);
#endif

	// Get current A/D offsets for normalization
	debug("Getting channel offsets...\r\n");
	GetChannelOffsets(sampleRate);

	// Setup ISR to clock the data sampling
#if INTERNAL_SAMPLING_SOURCE
	debug("Setup Internal Timer...\r\n");
	SetupInteralSampleTimer(sampleRate);
#elif EXTERNAL_SAMPLING_SOURCE
	debug("Setup External RTC Sample clock...\r\n");
	// Setup of external sampling source is done with the start clock driver call
#endif

	// Init a few key values for data collection
	DataIsrInit(sampleRate);

	// Start the timer for collecting data
	debug("Start sampling...\r\n");
#if INTERNAL_SAMPLING_SOURCE
	StartInteralSampleTimer();
#elif EXTERNAL_SAMPLING_SOURCE
	StartExternalRtcClock(sampleRate);
#endif

	// Change state to start processing the samples
	debug("Raise signal to start sampling\r\n");
	g_sampleProcessing = ACTIVE_STATE;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void StopMonitoring(uint8 mode, uint8 operation)
{
	sprintf((char*)g_spareBuffer, "%s...", getLangText(CLOSING_MONITOR_SESSION_TEXT));
	OverlayMessage(getLangText(STATUS_TEXT), (char*)g_spareBuffer, 0);

	// Check if the system was trying to recalibrate the offset due to temperature change
	if (getSystemEventState(UPDATE_OFFSET_EVENT))
	{
		debug("Clearing Update offset event\r\n");

		// Reset the update offset count
		g_updateOffsetCount = 0;

		clearSystemEventFlag(UPDATE_OFFSET_EVENT);
	}

	// Check if the unit is currently monitoring
	if (g_sampleProcessing == ACTIVE_STATE)
	{
		if ((mode == WAVEFORM_MODE) || (mode == COMBO_MODE))
		{
			// Check if not handling a force power off
			if (g_powerOffActivated != YES)
			{
				// Set flag to prevent any more incoming events from being processed
				g_doneTakingEvents = PENDING;

				// Wait for any triggered events to finish sending
				WaitForEventProcessingToFinish();
			}
		}

		// Check for any active alarms and setup timers to end them
		HandleActiveAlarmExtension();

		// Stop the data transfers
		StopDataCollection();

		// Reset the Waveform flag
		g_doneTakingEvents = NO;

		if ((mode == WAVEFORM_MODE) && (operation == FINISH_PROCESSING))
		{
			while (getSystemEventState(TRIGGER_EVENT))
			{
				debug("Handle Waveform Trigger Event Completion\r\n");
				MoveWaveformEventToFile();
			}
		}

		if ((mode == COMBO_MODE) && (operation == FINISH_PROCESSING))
		{
			while (getSystemEventState(TRIGGER_EVENT))
			{
				debug("Handle Combo - Waveform Trigger Event Completion\r\n");
				MoveWaveformEventToFile();
			}
		}

		if (mode == BARGRAPH_MODE)
		{
			// Handle the end of a Bargraph event
			debug("Handle End of Bargraph event\r\n");
			EndBargraph();
		}
		
		if (mode == COMBO_MODE)
		{
			// Handle the end of a Combo Bargraph event
			debug("Handle End of Combo - Bargraph event\r\n");
			EndBargraph();
		}
		
		// Check if any events are waiting to still be processed
		if (!getSystemEventState(TRIGGER_EVENT))
		{
			CloseMonitorLogEntry();
		}
	}
	
	// Turn on the Green keypad LED
	// Todo: Set the correct LED

	// Check if Auto Monitor is active and not in monitor mode
	if ((g_unitConfig.autoMonitorMode != AUTO_NO_TIMEOUT) && (operation == EVENT_PROCESSING))
	{
		AssignSoftTimer(AUTO_MONITOR_TIMER_NUM, (uint32)(g_unitConfig.autoMonitorMode * TICKS_PER_MIN), AutoMonitorTimerCallBack);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void StopDataCollection(void)
{
	g_sampleProcessing = IDLE_STATE;

#if INTERNAL_SAMPLING_SOURCE
	StopInteralSampleTimer();
#elif EXTERNAL_SAMPLING_SOURCE
	StopExternalRtcClock();
#endif

	AD4695_ExitConversionMode();
	DisableSensorBlocks();
	PowerControl(ADC_RESET, ON);

	ClearSoftTimer(MENU_UPDATE_TIMER_NUM);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void StopDataClock(void)
{
	g_sampleProcessing = IDLE_STATE;

#if INTERNAL_SAMPLING_SOURCE
	StopInteralSampleTimer();
#elif EXTERNAL_SAMPLING_SOURCE
	StopExternalRtcClock();
#endif

	AD4695_ExitConversionMode();
	DisableSensorBlocks();
	PowerControl(ADC_RESET, ON);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void WaitForEventProcessingToFinish(void)
{
	if (g_doneTakingEvents == PENDING)
	{
		debug("ISR Monitor process is still pending\r\n");
		
		OverlayMessage(getLangText(STATUS_TEXT), getLangText(PLEASE_BE_PATIENT_TEXT), 0);

		while (g_doneTakingEvents == PENDING)
		{
			// Just wait for the cal and end immediately afterwards
			SoftUsecWait(250);
		}
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16 AirTriggerConvert(uint32 airTriggerToConvert)
{
	// Check if the air trigger level is not no trigger and not manual trigger
	if ((airTriggerToConvert != NO_TRIGGER_CHAR) && (airTriggerToConvert != MANUAL_TRIGGER_CHAR) && (airTriggerToConvert != EXTERNAL_TRIGGER_CHAR))
	{
		if (g_unitConfig.unitsOfAir == MILLIBAR_TYPE)
		{
			// Convert mb to an A/D count
			airTriggerToConvert = (uint32)(MbToHex(airTriggerToConvert, g_factorySetupRecord.acousticSensorType));
		}
		else if (g_unitConfig.unitsOfAir == PSI_TYPE)
		{
			// Convert PSI to an A/D count
			airTriggerToConvert = (uint32)(PsiToHex(airTriggerToConvert, g_factorySetupRecord.acousticSensorType));
		}
		else // (g_unitConfig.unitsOfAir == DECIBEL_TYPE)
		{
			// Convert dB to an A/D count
			airTriggerToConvert = (uint32)(DbToHex(airTriggerToConvert, g_factorySetupRecord.acousticSensorType));
		}
	}

	return (airTriggerToConvert);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint32 SeismicTriggerConvertBitAccuracy(uint32 seismicTriggerToConvert)
{
	if ((seismicTriggerToConvert != NO_TRIGGER_CHAR) && (seismicTriggerToConvert != MANUAL_TRIGGER_CHAR) && (seismicTriggerToConvert != EXTERNAL_TRIGGER_CHAR))
	{
		seismicTriggerToConvert = seismicTriggerToConvert / (SEISMIC_TRIGGER_MAX_VALUE / g_bitAccuracyMidpoint);
	}

	return (seismicTriggerToConvert);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint32 AirTriggerConvertToUnits(uint32 airTriggerToConvert)
{
	// Check if the air trigger level is not no trigger and not manual trigger
	if ((airTriggerToConvert != NO_TRIGGER_CHAR) && (airTriggerToConvert != MANUAL_TRIGGER_CHAR) && (airTriggerToConvert != EXTERNAL_TRIGGER_CHAR))
	{
		if (g_unitConfig.unitsOfAir == MILLIBAR_TYPE)
		{
			airTriggerToConvert = (HexToMB(airTriggerToConvert, DATA_NORMALIZED, ACCURACY_16_BIT_MIDPOINT, g_factorySetupRecord.acousticSensorType) * 10000);
		}
		else if (g_unitConfig.unitsOfAir == PSI_TYPE)
		{
			airTriggerToConvert = (HexToPSI(airTriggerToConvert, DATA_NORMALIZED, ACCURACY_16_BIT_MIDPOINT, g_factorySetupRecord.acousticSensorType) * 10000);
		}
		else // (g_unitConfig.unitsOfAir == DECIBEL_TYPE)
		{
			airTriggerToConvert = HexToDB(airTriggerToConvert, DATA_NORMALIZED, ACCURACY_16_BIT_MIDPOINT, g_factorySetupRecord.acousticSensorType);
		}
	}

	return (airTriggerToConvert);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GetManualCalibration(void)
{
	InitDataBuffs(MANUAL_CAL_MODE);
	g_manualCalFlag = TRUE;
	g_manualCalSampleCount = MAX_CAL_SAMPLES;

	// Set the analog cutoff for low (for fixed Cal 1K sample rate)
	SetAnalogCutoffFrequency(ANALOG_CUTOFF_FREQ_1K);

	// Set the seismic gain to low (part of the fixed Calibration settings)
	SetSeismicGainSelect(SEISMIC_GAIN_NORMAL);

	// Check if A-weighting is enabled
	if ((g_factorySetupRecord.aWeightOption == ENABLED) && (g_unitConfig.airScale == AIR_SCALE_A_WEIGHTING))
	{
		// Set acoustic for A-weighted gain
		SetAcousticPathSelect(ACOUSTIC_PATH_A_WEIGHTED);
	}
	// Set acoustic for normal gain
	else { SetAcousticPathSelect(ACOUSTIC_PATH_AOP); }

	StartDataCollection(MANUAL_CAL_DEFAULT_SAMPLE_RATE);

	// Make absolutely sure we are done with the Cal pulse
	while ((volatile uint32)g_manualCalSampleCount != 0) { /* spin */ }

	// Stop data transfer
#if 0 /* Normal */
	StopDataClock();
#else /* Test skipping channel disable and analog 5V power down */
	g_sampleProcessing = IDLE_STATE;

#if INTERNAL_SAMPLING_SOURCE
	StopInteralSampleTimer();
#elif EXTERNAL_SAMPLING_SOURCE
	StopExternalRtcClock();
#endif

	AD4695_ExitConversionMode();
#endif

	if (getSystemEventState(MANUAL_CAL_EVENT))
	{
		debug("Manual Cal Pulse Event (Monitoring)\r\n");
		MoveManualCalToFile();
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleManualCalibration(void)
{
	INPUT_MSG_STRUCT mn_msg;

	// Check if actively monitoring in Waveform mode (don't process for Bargraph and Combo)
	if ((g_sampleProcessing == ACTIVE_STATE) && (g_triggerRecord.opMode == WAVEFORM_MODE))
	{
		// Check if not busy processing an event (otherwise skip cycle change calibration since handling an event will be accompanied by a cal pulse)
		if ((g_busyProcessingEvent == NO) && (!getSystemEventState(TRIGGER_EVENT)))
		{
			// Check if there is no room to store a calibration event
			if ((g_unitConfig.flashWrapping == NO) && (g_sdCardUsageStats.manualCalsLeft == 0))
			{
				sprintf((char*)g_spareBuffer, "%s (%s %s) %s %s", getLangText(FLASH_MEMORY_IS_FULL_TEXT), getLangText(WRAPPING_TEXT), getLangText(DISABLED_TEXT),
				getLangText(CALIBRATION_TEXT), getLangText(UNAVAILABLE_TEXT));
				OverlayMessage(getLangText(WARNING_TEXT), (char*)g_spareBuffer, (5 * SOFT_SECS));
			}
			else // Handle cycle change calibration while in Waveform mode (without leaving monitor session)
			{
				// Stop data transfer
#if 0 /* Normal */
				StopDataClock();
#else /* Test skipping channel disable and analog 5V power down */
				g_sampleProcessing = IDLE_STATE;

#if INTERNAL_SAMPLING_SOURCE
				StopInteralSampleTimer();
#elif EXTERNAL_SAMPLING_SOURCE
				StopExternalRtcClock();
#endif

				AD4695_ExitConversionMode();
#endif

				// Perform Cal while in monitor mode
				OverlayMessage(getLangText(STATUS_TEXT), getLangText(PERFORMING_CALIBRATION_TEXT), 0);

				GetManualCalibration();

				InitDataBuffs(g_triggerRecord.opMode);

				// Make sure to reset the analog parameters back to the current Waveform settings
				switch (g_triggerRecord.trec.sample_rate)
				{
					// Set the cutoff frequency based on sample rate
					case SAMPLE_RATE_1K: SetAnalogCutoffFrequency(ANALOG_CUTOFF_FREQ_1K); break;
					case SAMPLE_RATE_2K: SetAnalogCutoffFrequency(ANALOG_CUTOFF_FREQ_1K); break;
					case SAMPLE_RATE_4K: SetAnalogCutoffFrequency(ANALOG_CUTOFF_FREQ_2K); break;
					case SAMPLE_RATE_8K: SetAnalogCutoffFrequency(ANALOG_CUTOFF_FREQ_4K); break;
					case SAMPLE_RATE_16K: SetAnalogCutoffFrequency(ANALOG_CUTOFF_FREQ_8K); break;
					case SAMPLE_RATE_32K: SetAnalogCutoffFrequency(ANALOG_CUTOFF_FREQ_16K); break;

					// Default just in case it's a custom frequency
					default: SetAnalogCutoffFrequency(ANALOG_CUTOFF_FREQ_1K); break;
				}

				// Set the sensitivity (aka gain) based on the current settings
				if (g_triggerRecord.srec.sensitivity == LOW) { SetSeismicGainSelect(SEISMIC_GAIN_NORMAL); }
				else { SetSeismicGainSelect(SEISMIC_GAIN_HIGH); }

				// Check if A-weighting is enabled
				if ((g_factorySetupRecord.aWeightOption == ENABLED) && (g_unitConfig.airScale == AIR_SCALE_A_WEIGHTING))
				{
					// Set acoustic for A-weighted gain
					SetAcousticPathSelect(ACOUSTIC_PATH_A_WEIGHTED);
				}
				// Set acoustic for normal gain
				else { SetAcousticPathSelect(ACOUSTIC_PATH_AOP); }

				StartDataCollection(g_triggerRecord.trec.sample_rate);
			}
		}
	}
	else if (g_sampleProcessing == IDLE_STATE) // Perform calibration (outside of active monitoring)
	{
		// Check if there is no room to store a calibration event
		if ((g_unitConfig.flashWrapping == NO) && (g_sdCardUsageStats.manualCalsLeft == 0))
		{
			SETUP_MENU_MSG(MAIN_MENU);
			JUMP_TO_ACTIVE_MENU();

			sprintf((char*)g_spareBuffer, "%s (%s %s) %s %s", getLangText(FLASH_MEMORY_IS_FULL_TEXT), getLangText(WRAPPING_TEXT), getLangText(DISABLED_TEXT),
					getLangText(CALIBRATION_TEXT), getLangText(UNAVAILABLE_TEXT));
			OverlayMessage(getLangText(WARNING_TEXT), (char*)g_spareBuffer, (5 * SOFT_SECS));
		}
		else
		{
			// If the user was in the Summary list menu, reset the global
			g_summaryListMenuActive = NO;

			// Perform calibration (outside of active monitoring)
			OverlayMessage(getLangText(STATUS_TEXT), getLangText(PERFORMING_CALIBRATION_TEXT), 0);

			GetManualCalibration();
		}
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ForcedCalibration(void)
{
	uint8 pendingMode = g_triggerRecord.opMode;
	
	g_forcedCalibration = YES;

	OverlayMessage(getLangText(STATUS_TEXT), getLangText(PERFORMING_CALIBRATION_TEXT), 0);

	GetManualCalibration();

	if (getMenuEventState(RESULTS_MENU_EVENT)) 
	{
		clearMenuEventFlag(RESULTS_MENU_EVENT);
	}

	// Wait until after the Cal Pulse has completed
	SoftUsecWait(1 * SOFT_SECS);

	g_activeMenu = MONITOR_MENU;

	g_forcedCalibration = NO;

	// Reset the mode back to the previous mode
	g_triggerRecord.opMode = pendingMode;

	UpdateMonitorLogEntry();
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleCycleChangeEvent(void)
{
	INPUT_MSG_STRUCT mn_msg;
	uint8 performCycleChangeCalibration = NO;

	// Check if Auto Calibration at cycle change is active (any value but zero)
	if (g_unitConfig.autoCalMode) // != AUTO_NO_CAL_TIMEOUT
	{
		// Decrement days to wait
		if (g_autoCalDaysToWait) { g_autoCalDaysToWait--; }

		// Check if time to do Auto Calibration
		if (g_autoCalDaysToWait == 0)
		{
			performCycleChangeCalibration = YES;

			// Reset the days to wait count
			switch (g_unitConfig.autoCalMode)
			{
				case AUTO_24_HOUR_TIMEOUT: g_autoCalDaysToWait = 1; break;
				case AUTO_48_HOUR_TIMEOUT: g_autoCalDaysToWait = 2; break;
				case AUTO_72_HOUR_TIMEOUT: g_autoCalDaysToWait = 3; break;
			}
		}
	}

	// Check if actively monitoring in either Bargraph or Combo mode (ignore cycle change calibration since the start of a new session creates a calibration)
	if ((g_sampleProcessing == ACTIVE_STATE) && ((g_triggerRecord.opMode == BARGRAPH_MODE) || (g_triggerRecord.opMode == COMBO_MODE)))
	{
		// Overlay a message that the current Bargraph or Combo is ending
		sprintf((char*)g_spareBuffer, "%s %s", (g_triggerRecord.opMode == BARGRAPH_MODE) ? (getLangText(BARGRAPH_MODE_TEXT)) : (getLangText(COMBO_MODE_TEXT)), getLangText(END_TEXT));
		OverlayMessage(getLangText(STATUS_TEXT), (char*)g_spareBuffer, 0);

		// Handle stopping the current Bargraph or Combo
		StopMonitoring(g_triggerRecord.opMode, FINISH_PROCESSING);

		// Start up a new Bargraph or Combo
		SETUP_MENU_WITH_DATA_MSG(MONITOR_MENU, g_triggerRecord.opMode);
		JUMP_TO_ACTIVE_MENU();
	}
	else if (performCycleChangeCalibration == YES)
	{
		HandleManualCalibration();
	}

	// Check if Auto Dialout processing is not active
	if (g_autoDialoutState == AUTO_DIAL_IDLE)
	{
#if 0 /* Original */
		// At cycle change reset the modem (to better handle problems with USR modems)
		g_autoRetries = 0;
		ModemResetProcess();
#else /* New AutoDialOut Config/Status if no events during the day */
		// Check if Auto Dial Out in Events only mode and not active connections during the current cycle
		if ((g_modemSetupRecord.dialOutType != AUTODIALOUT_EVENTS_CONFIG_STATUS) && (__autoDialoutTbl.currentCycleConnects == 0))
		{
			if (CheckAutoDialoutStatusAndFlagIfAvailable() == YES)
			{
				// Set current cycle connections to a special 0xFFFF so that incrementing by 1 will leave this zero, excluding this call out from counting in the current cycle stats
				__autoDialoutTbl.currentCycleConnects = 0xFFFF;
			}
		}
		else
		{
			// Reset current day number of connections
			__autoDialoutTbl.currentCycleConnects = 0;

			// At cycle change reset the modem (to better handle problems with USR modems)
			g_autoRetries = 0;
			ModemResetProcess();
		}
#endif
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void StopMonitoringForLowPowerState(void)
{
	INPUT_MSG_STRUCT mn_msg;

	// Disable the monitor menu update timer
	ClearSoftTimer(MENU_UPDATE_TIMER_NUM);

	// Handle and finish monitoring
	StopMonitoring(g_triggerRecord.opMode, FINISH_PROCESSING);

	// Jump to the main menu
	SETUP_MENU_MSG(MAIN_MENU);
	JUMP_TO_ACTIVE_MENU();
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void StartADDataCollectionForCalibration(uint16 sampleRate)
{
	// Check if External ADC is in reset
	if (GetPowerControlState(ADC_RESET) == ON) { WaitAnalogPower5vGood(); }

	// Configure External ADC
	AD4695_Init();

	// Setup Analog controls
	SetAnalogCutoffFrequency(ANALOG_CUTOFF_FREQ_1K);
	SetSeismicGainSelect(SEISMIC_GAIN_NORMAL);
	SetAcousticPathSelect(ACOUSTIC_PATH_AOP);

	// Delay to allow AD to power up/stabilize
	SoftUsecWait(50 * SOFT_MSECS);

	// Setup AD Channel config
	SetupADChannelConfig(sampleRate, OVERRIDE_ENABLE_CHANNEL_VERIFICATION);

	DataIsrInit(sampleRate);

	// Get channel offsets
	GetChannelOffsets(sampleRate);

#if INTERNAL_SAMPLING_SOURCE
	// Setup Interal timer and ISR to clock the data sampling
	SetupInteralSampleTimer(sampleRate);

	// Start the timer for collecting data
	StartInteralSampleTimer();
#elif EXTERNAL_SAMPLING_SOURCE
	StartExternalRtcClock(sampleRate);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void StopADDataCollectionForCalibration(void)
{
#if INTERNAL_SAMPLING_SOURCE
	StopInteralSampleTimer();
#elif EXTERNAL_SAMPLING_SOURCE
	StopExternalRtcClock();
#endif

	AD4695_ExitConversionMode();
	DisableSensorBlocks();
	PowerControl(ADC_RESET, ON);
}
