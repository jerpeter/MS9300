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
#include "Sensor.h"
//#include "usb_drv.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define MONITOR_MN_TABLE_SIZE 8
#define MONITOR_WND_STARTING_COL DEFAULT_COL_THREE 
#define MONITOR_WND_END_COL DEFAULT_END_COL
#define MONITOR_WND_STARTING_ROW DEFAULT_MENU_ROW_ZERO
#define MONITOR_WND_END_ROW (DEFAULT_MENU_ROW_SEVEN)
#define MONITOR_MN_TBL_START_LINE 0
#define TOTAL_DOTS 4

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
void MonitorMenu(INPUT_MSG_STRUCT);
void MonitorMenuDsply(WND_LAYOUT_STRUCT *);
void MonitorMenuProc(INPUT_MSG_STRUCT, WND_LAYOUT_STRUCT*, MN_LAYOUT_STRUCT*);

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MonitorMenu(INPUT_MSG_STRUCT msg)
{
	static WND_LAYOUT_STRUCT wnd_layout;
	static MN_LAYOUT_STRUCT mn_layout;

	MonitorMenuProc(msg, &wnd_layout, &mn_layout);

	if (g_activeMenu == MONITOR_MENU)
	{
		MonitorMenuDsply(&wnd_layout);
		WriteMapToLcd(g_mmap);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 g_showRVTA = NO;
void MonitorMenuProc(INPUT_MSG_STRUCT msg, WND_LAYOUT_STRUCT *wnd_layout_ptr, MN_LAYOUT_STRUCT *mn_layout_ptr)
{
	INPUT_MSG_STRUCT mn_msg;
	REC_EVENT_MN_STRUCT temp_g_triggerRecord;
	uint8 recheckSpaceRemaining = NO;

	switch (msg.cmd)
	{
		case (ACTIVATE_MENU_WITH_DATA_CMD):
			wnd_layout_ptr->start_col = MONITOR_WND_STARTING_COL;
			wnd_layout_ptr->end_col = MONITOR_WND_END_COL;
			wnd_layout_ptr->start_row = MONITOR_WND_STARTING_ROW;
			wnd_layout_ptr->end_row = MONITOR_WND_END_ROW;
			mn_layout_ptr->curr_ln = MONITOR_MN_TBL_START_LINE;
			mn_layout_ptr->top_ln = MONITOR_MN_TBL_START_LINE;

			memset(&(g_mmap[0][0]), 0, sizeof(g_mmap));
			
			// Check if USB device mode is active and connected to a PC (where it's possible to change the contents of the SD card under the covers)
#if 0 /* old hw */
			if ((g_usbMassStorageState == USB_CONNECTED_AND_PROCESSING) && (Is_usb_id_device()))
			{
				recheckSpaceRemaining = YES;
			}
#endif
			// Special call before Monitoring to disable USB processing
			UsbDeviceManager();

			if (recheckSpaceRemaining)
			{
				// Recalculate remaining space in case
				OverlayMessage(getLangText(STATUS_TEXT), getLangText(CALCULATING_EVENT_STORAGE_SPACE_FREE_TEXT), 0);
				GetSDCardUsageStats();
			}

			g_monitorOperationMode = (uint8)msg.data[0];

			// Check if flash wrapping is disabled and if there is space left in flash
			if (g_unitConfig.flashWrapping == NO)
			{
				if (((g_monitorOperationMode == WAVEFORM_MODE) && (g_sdCardUsageStats.waveEventsLeft == 0)) ||
					((g_monitorOperationMode == BARGRAPH_MODE) && (g_sdCardUsageStats.barHoursLeft == 0)) ||
					((g_monitorOperationMode == COMBO_MODE) && (g_sdCardUsageStats.waveEventsLeft == 0) && (g_sdCardUsageStats.barHoursLeft == 0)) ||
					((g_monitorOperationMode == MANUAL_CAL_MODE) && (g_sdCardUsageStats.manualCalsLeft == 0)))
				{
					// Unable to store any more data in the selected mode

					// Jump back to main menu
					SETUP_MENU_MSG(MAIN_MENU);
					JUMP_TO_ACTIVE_MENU();
					
					sprintf((char*)g_spareBuffer, "%s (%s %s) %s %s", getLangText(FLASH_MEMORY_IS_FULL_TEXT), getLangText(WRAPPING_TEXT), getLangText(DISABLED_TEXT),
							getLangText(MONITORING_TEXT), getLangText(UNAVAILABLE_TEXT));
					OverlayMessage(getLangText(WARNING_TEXT), (char*)g_spareBuffer, (5 * SOFT_SECS));
					return;
				}
			}

			// Read and cache Smart Sensor data
			SmartSensorReadRomAndMemory(SEISMIC_SENSOR);
			SmartSensorReadRomAndMemory(ACOUSTIC_SENSOR);
			UpdateUnitSensorsWithSmartSensorTypes();

			UpdateWorkingCalibrationDate();

			// Make sure the parameters are up to date based on the trigger setup information
			InitSensorParameters(g_factorySetupRecord.seismicSensorType, (uint8)g_triggerRecord.srec.sensitivity);

			CheckAndPromptUserWaitingForSensorWarmup();
			ZeroingSensorCalibration();

			switch(g_monitorOperationMode)
			{
				case WAVEFORM_MODE:
					// Check if the Variable Trigger sample rate is greater than max working sample rate
					if ((g_triggerRecord.trec.variableTriggerEnable == YES) && (g_triggerRecord.trec.sample_rate == SAMPLE_RATE_16K))
					{
						// Drop to highest allowed Variable Trigger sample rate
						g_triggerRecord.trec.sample_rate = SAMPLE_RATE_8K;
					}

					StartMonitoring(g_triggerRecord.opMode, &g_triggerRecord.trec);
				break;

				case BARGRAPH_MODE:
				case COMBO_MODE:
					// Set the default display mode to be the summary interval results
					g_displayBargraphResultsMode = SUMMARY_INTERVAL_RESULTS;
					g_displayAlternateResultState = DEFAULT_RESULTS;
					
					// Check if the sample rate is greater than max working sample rate
					if (((g_monitorOperationMode == BARGRAPH_MODE) && (g_triggerRecord.trec.sample_rate > SAMPLE_RATE_8K)) ||
						((g_monitorOperationMode == COMBO_MODE) && (g_triggerRecord.trec.sample_rate > SAMPLE_RATE_8K)))
					{
						// Set to a default
						g_triggerRecord.trec.sample_rate = SAMPLE_RATE_1K;
					}

					g_aImpulsePeak = g_rImpulsePeak = g_vImpulsePeak = g_tImpulsePeak = 0;
					g_aJobPeak = g_rJobPeak = g_vJobPeak = g_tJobPeak = 0;
					g_aJobFreq = g_rJobFreq = g_vJobFreq = g_tJobFreq = 0;
					g_vsImpulsePeak = g_vsJobPeak = 0;
					
					if ((g_triggerRecord.berec.impulseMenuUpdateSecs < LCD_IMPULSE_TIME_MIN_VALUE) || 
						(g_triggerRecord.berec.impulseMenuUpdateSecs > LCD_IMPULSE_TIME_MAX_VALUE))
					{
						g_triggerRecord.berec.impulseMenuUpdateSecs = LCD_IMPULSE_TIME_DEFAULT_VALUE;
					}

					StartMonitoring(g_triggerRecord.opMode, &g_triggerRecord.trec);
				break;

				case MANUAL_TRIGGER_MODE:
					memcpy((uint8*)&temp_g_triggerRecord, &g_triggerRecord, sizeof(REC_EVENT_MN_STRUCT));
					temp_g_triggerRecord.trec.seismicTriggerLevel = MANUAL_TRIGGER_CHAR;
					temp_g_triggerRecord.trec.airTriggerLevel = MANUAL_TRIGGER_CHAR;

					StartMonitoring(g_triggerRecord.opMode, &temp_g_triggerRecord.trec);
				break;

				default:
					break;
			}
		break;

		case (KEYPRESS_MENU_CMD):
			if (g_waitForUser == TRUE)
			{
				switch (msg.data[0])
				{
					case (ENTER_KEY):
					case (ESC_KEY):
						g_waitForUser = FALSE;
					break;
				}
				
				// Done processing
				return;
			}

			if ((g_promtForCancelingPrintJobs == TRUE) || (g_promtForLeavingMonitorMode == TRUE))
			{
				switch (msg.data[0])
				{
					case (ENTER_KEY):
						if (g_promtForCancelingPrintJobs == TRUE)
						{
							// Done handling cancel print jobs, now handle leaving monitor mode
							g_promtForCancelingPrintJobs = FALSE;
							g_promtForLeavingMonitorMode = TRUE;
						}
						else if (g_promtForLeavingMonitorMode == TRUE)
						{
							if (g_monitorModeActiveChoice == MB_FIRST_CHOICE)
							{
								StopMonitoring(g_monitorOperationMode, EVENT_PROCESSING);

								SETUP_MENU_MSG(MAIN_MENU);
								JUMP_TO_ACTIVE_MENU();
							}

							g_promtForLeavingMonitorMode = FALSE;
						}

						g_monitorModeActiveChoice = MB_FIRST_CHOICE;
					break;
					
					case (ESC_KEY):
						// Do not process the escape key
					break;

					case UP_ARROW_KEY:
						if (g_monitorModeActiveChoice == MB_SECOND_CHOICE) 
							g_monitorModeActiveChoice = MB_FIRST_CHOICE;
					break;

					case DOWN_ARROW_KEY:
						if (g_monitorModeActiveChoice == MB_FIRST_CHOICE) 
							g_monitorModeActiveChoice = MB_SECOND_CHOICE;
					break;
				}
				
				// Done processing
				return;
			}

			switch (msg.data[0])
			{
				case (ENTER_KEY):
					break;
					
				case (ESC_KEY):
					g_monitorEscapeCheck = YES;
					g_promtForLeavingMonitorMode = TRUE;
				break;
				
				case (DOWN_ARROW_KEY):
#if 1
						g_showRVTA = YES;
#endif					
					if ((g_monitorOperationMode == BARGRAPH_MODE) || (g_monitorOperationMode == COMBO_MODE))
					{
						// Check if at the Impulse Results screen
						if (g_displayBargraphResultsMode == IMPULSE_RESULTS)
						{
							// Change to the Summary Interval Results Screen
							g_displayBargraphResultsMode = SUMMARY_INTERVAL_RESULTS;
						}
						// Check if at the Summary Interval Results screen
						else if (g_displayBargraphResultsMode == SUMMARY_INTERVAL_RESULTS)
						{
							// Change to the Job Peak Results Screen
							g_displayBargraphResultsMode = JOB_PEAK_RESULTS;
						}
					}
				break;
					
				case (UP_ARROW_KEY):
#if 1
						g_showRVTA = NO;
#endif					
					if ((g_monitorOperationMode == BARGRAPH_MODE) || (g_monitorOperationMode == COMBO_MODE))
					{
						// Check if at the Job Peak Results screen
						if (g_displayBargraphResultsMode == JOB_PEAK_RESULTS)
						{
							// Change to the Summary Interval Results Screen
							g_displayBargraphResultsMode = SUMMARY_INTERVAL_RESULTS;
						}
						// Check if at the Summary Interval Results screen
						else if (g_displayBargraphResultsMode == SUMMARY_INTERVAL_RESULTS)
						{
							// Change to the Impulse Results Screen
							g_displayBargraphResultsMode = IMPULSE_RESULTS;
							
							// Check if results mode is Peak Displacement
							if ((g_displayAlternateResultState == PEAK_DISPLACEMENT_RESULTS) || (g_displayAlternateResultState == PEAK_ACCELERATION_RESULTS))
							{
								// Change it since Peak Displacement and Peak Acceleration are not valid for Impulse results
								g_displayAlternateResultState = DEFAULT_RESULTS;
							}
						}
					}
				break;
					
				case (RIGHT_ARROW_KEY):
					switch (g_displayAlternateResultState)
					{
						case DEFAULT_RESULTS:
							g_displayAlternateResultState = VECTOR_SUM_RESULTS;
						break;

						case VECTOR_SUM_RESULTS:
							if (g_displayBargraphResultsMode != IMPULSE_RESULTS) { g_displayAlternateResultState = PEAK_DISPLACEMENT_RESULTS; }
							else { g_displayAlternateResultState = DEFAULT_RESULTS; }
						break;

						case PEAK_DISPLACEMENT_RESULTS:
							if (g_displayBargraphResultsMode != IMPULSE_RESULTS) { g_displayAlternateResultState = PEAK_ACCELERATION_RESULTS; }
							else { g_displayAlternateResultState = DEFAULT_RESULTS; }
						break;

						case PEAK_ACCELERATION_RESULTS:
							g_displayAlternateResultState = DEFAULT_RESULTS;
						break;
					}
				break;

				case (LEFT_ARROW_KEY):
					switch (g_displayAlternateResultState)
					{
						case DEFAULT_RESULTS:
							if (g_displayBargraphResultsMode != IMPULSE_RESULTS) { g_displayAlternateResultState = PEAK_ACCELERATION_RESULTS; }
							else { g_displayAlternateResultState = VECTOR_SUM_RESULTS; }
						break;

						case VECTOR_SUM_RESULTS:
							g_displayAlternateResultState = DEFAULT_RESULTS;
						break;

						case PEAK_DISPLACEMENT_RESULTS:
							g_displayAlternateResultState = VECTOR_SUM_RESULTS;
						break;

						case PEAK_ACCELERATION_RESULTS:
							if (g_displayBargraphResultsMode != IMPULSE_RESULTS) { g_displayAlternateResultState = PEAK_DISPLACEMENT_RESULTS; }
							else { g_displayAlternateResultState = VECTOR_SUM_RESULTS; }
						break;
					}
				break;

				default:
					break;
			}
		break;

		case STOP_MONITORING_CMD:
			StopMonitoring(g_monitorOperationMode, EVENT_PROCESSING);

			SETUP_MENU_MSG(MAIN_MENU);
			JUMP_TO_ACTIVE_MENU();
			
			ActivateDisplayShortDuration(5);
			sprintf((char*)g_spareBuffer, "%s (%s %s) %s %s", getLangText(FLASH_MEMORY_IS_FULL_TEXT), getLangText(WRAPPING_TEXT), getLangText(DISABLED_TEXT),
					getLangText(MONITORING_TEXT), getLangText(STOPPED_TEXT));
			OverlayMessage(getLangText(WARNING_TEXT), (char*)g_spareBuffer, (5 * SOFT_SECS));
		break;

		default:
			break;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MonitorMenuDsply(WND_LAYOUT_STRUCT *wnd_layout_ptr)
{
	char buff[50];
	char srBuff[6];
	uint8 dotBuff[TOTAL_DOTS];
	uint8 length = 0, i = 0;
	static uint8 dotState = 0;
	char displayFormat[22];
	float div = 1, tempR = 0, tempV = 0, tempT = 0, tempA = 0, tempVS = 0, tempPeakDisp, normalize_max_peak;
	float tempPeakAcc;
	float rFreq = 0, vFreq = 0, tFreq = 0, tempFreq;
	uint8 arrowChar;
	uint8 gainFactor = (uint8)((g_triggerRecord.srec.sensitivity == LOW) ? 2 : 4);
	char modeChar = 'W';
	char chanVerifyChar = '+';
	DATE_TIME_STRUCT time;

	wnd_layout_ptr->curr_row = wnd_layout_ptr->start_row;
	wnd_layout_ptr->curr_col = wnd_layout_ptr->start_col;
	wnd_layout_ptr->next_row = wnd_layout_ptr->start_row;
	wnd_layout_ptr->next_col = wnd_layout_ptr->start_col;

	memset(&(g_mmap[0][0]), 0, sizeof(g_mmap));

	//-----------------------------------------------------------------------
	// PRINT MONITORING
	//-----------------------------------------------------------------------
	memset(&buff[0], 0, sizeof(buff));
	memset(&dotBuff[0], 0, sizeof(dotBuff));
	
	for (i = 0; i < dotState; i++)
	{
		dotBuff[i] = '.';
	}
	for (; i < (TOTAL_DOTS - 1); i++)
	{
		dotBuff[i] = ' ';
	}

	if (++dotState >= TOTAL_DOTS)
		dotState = 0;

	if (g_adaptiveState == ADAPTIVE_MIN_RATE) { sprintf((char*)srBuff, "A1K"); }
	else { sprintf((char*)srBuff, "%dK", (int)(g_triggerRecord.trec.sample_rate / SAMPLE_RATE_1K)); }

	// Set the mode character (default is 'W' for Waveform on init)
	if (g_monitorOperationMode == BARGRAPH_MODE) { modeChar = 'B'; }
	else if (g_monitorOperationMode == COMBO_MODE) { modeChar = 'C'; }

	// Set the channel verification character (default is '+' for enabled on init)
	if ((g_triggerRecord.trec.sample_rate == SAMPLE_RATE_16K) || (g_unitConfig.adChannelVerification == DISABLED)) { chanVerifyChar = '-'; }

	if (g_busyProcessingEvent)
	{
		length = (uint8)sprintf((char*)buff, "%s%s(%c%c%s)", getLangText(PROCESSING_TEXT), dotBuff, modeChar, chanVerifyChar, srBuff);
	}
	else
	{
		length = (uint8)sprintf((char*)buff, "%s%s(%c%c%s)", getLangText(MONITORING_TEXT), dotBuff, modeChar, chanVerifyChar, srBuff);
	}

#if 0 /* Replacing this old code */
	if (g_monitorOperationMode == WAVEFORM_MODE)
	{
		if (g_busyProcessingEvent)
		{
			length = (uint8)sprintf((char*)buff, "%s%s(W-%s)", getLangText(PROCESSING_TEXT), dotBuff, srBuff);
		}
		else
		{
			length = (uint8)sprintf((char*)buff, "%s%s(W-%s)", getLangText(MONITORING_TEXT), dotBuff, srBuff);
		}
	}
	else if (g_monitorOperationMode == BARGRAPH_MODE)
	{
		length = (uint8)sprintf((char*)buff, "%s%s(B-%s)", getLangText(MONITORING_TEXT), dotBuff, srBuff);
	}
	else if (g_monitorOperationMode == COMBO_MODE)
	{
		if (g_busyProcessingEvent)
		{
			length = (uint8)sprintf((char*)buff, "%s%s(C-%s)", getLangText(PROCESSING_TEXT), dotBuff, srBuff);
		}
		else
		{
			length = (uint8)sprintf((char*)buff, "%s%s(C-%s)", getLangText(MONITORING_TEXT), dotBuff, srBuff);
		}

	}
#endif

	// Setup current column to center text
	wnd_layout_ptr->curr_col = (uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));
	// Write string to screen
	WndMpWrtString((uint8*)&buff[0], wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

	if ((g_monitorOperationMode == BARGRAPH_MODE) || (g_monitorOperationMode == COMBO_MODE))
	{
		if (g_displayBargraphResultsMode == SUMMARY_INTERVAL_RESULTS)
			arrowChar = BOTH_ARROWS_CHAR;
		else if (g_displayBargraphResultsMode == JOB_PEAK_RESULTS)
			arrowChar = UP_ARROW_CHAR;
		else // g_displayBargraphResultsMode == IMPULSE_RESULTS
			arrowChar = DOWN_ARROW_CHAR;
				
		sprintf(buff, "%c", arrowChar);
		wnd_layout_ptr->curr_col = 120;
		WndMpWrtString((uint8*)&buff[0], wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
	}

	// Advance to next row
	wnd_layout_ptr->curr_row = wnd_layout_ptr->next_row;

	// Check mode
	if (g_monitorOperationMode == WAVEFORM_MODE)
	{
		//-----------------------------------------------------------------------
		// Skip Line
		//-----------------------------------------------------------------------
		WndMpWrtString((uint8*)" ", wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
		wnd_layout_ptr->curr_row = wnd_layout_ptr->next_row;

		//-----------------------------------------------------------------------
		// Date
		//-----------------------------------------------------------------------
		memset(&buff[0], 0, sizeof(buff));
		time = GetCurrentTime();
		ConvertTimeStampToString(buff, &time, REC_DATE_TIME_MONITOR);
		length = (uint8)strlen(buff);

		wnd_layout_ptr->curr_col =(uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));

		WndMpWrtString((uint8*)(&buff[0]), wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
		wnd_layout_ptr->curr_row = wnd_layout_ptr->next_row;

		//-----------------------------------------------------------------------
		// Time
		//-----------------------------------------------------------------------
		memset(&buff[0], 0, sizeof(buff));
		length = (uint8)sprintf(buff,"%02d:%02d:%02d",time.hour, time.min, time.sec);

		wnd_layout_ptr->curr_col =(uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));

		WndMpWrtString((uint8*)(&buff[0]),wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
		wnd_layout_ptr->curr_row = wnd_layout_ptr->next_row;

		//-----------------------------------------------------------------------
		// Skip Line
		//-----------------------------------------------------------------------
		WndMpWrtString((uint8*)" ", wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
		wnd_layout_ptr->curr_row = wnd_layout_ptr->next_row;

		//-----------------------------------------------------------------------
		// Battery Voltage
		//-----------------------------------------------------------------------
		memset(&buff[0], 0, sizeof(buff));
		length = (uint8)sprintf(buff,"%s %.2f", getLangText(BATTERY_TEXT), (double)GetExternalVoltageLevelAveraged(BATTERY_VOLTAGE));

		wnd_layout_ptr->curr_col =(uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));

		WndMpWrtString((uint8*)(&buff[0]),wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
		wnd_layout_ptr->curr_row = wnd_layout_ptr->next_row;

#if 1 /* Show hidden RTVA Values */
		if (g_showRVTA == YES)
		{
			debug("R: %x, V: %x, T: %x, A: %x, Temp: %x\r\n", (((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->r), (((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->v),
					(((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->t), (((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->a), g_previousTempReading);

			//-----------------------------------------------------------------------
			// Show RTVA
			//-----------------------------------------------------------------------
			memset(&buff[0], 0, sizeof(buff));
			length = (uint8)sprintf(buff,"R    V    T    A"); 

			wnd_layout_ptr->curr_col = (uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));

			WndMpWrtString((uint8*)(&buff[0]), wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
			wnd_layout_ptr->curr_row = wnd_layout_ptr->next_row;

			memset(&buff[0], 0, sizeof(buff));
			length = (uint8)sprintf(buff," %04x %04x %04x %04x", 
				(((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->r),
				(((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->v),
				(((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->t),
				(((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->a));

			wnd_layout_ptr->curr_col = (uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));

			WndMpWrtString((uint8*)(&buff[0]), wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
		}
#endif
	}
	else if ((g_monitorOperationMode == BARGRAPH_MODE) || (g_monitorOperationMode == COMBO_MODE))
	{
		//-----------------------------------------------------------------------
		// Date and Time
		//-----------------------------------------------------------------------
		memset(&buff[0], 0, sizeof(buff));
		time = GetCurrentTime();

		ConvertTimeStampToString(buff, &time, REC_DATE_TIME_TYPE);
		length = (uint8)strlen(buff);
		wnd_layout_ptr->curr_col =(uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));

		WndMpWrtString((uint8*)(&buff[0]),wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
		wnd_layout_ptr->curr_row = wnd_layout_ptr->next_row;

		//-----------------------------------------------------------------------
		// Skip Line
		//-----------------------------------------------------------------------
		WndMpWrtString((uint8*)" ", wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
		wnd_layout_ptr->curr_row = wnd_layout_ptr->next_row;

		//-----------------------------------------------------------------------
		// Print Result Type Header
		//-----------------------------------------------------------------------
		memset(&buff[0], 0, sizeof(buff));

		if (g_displayBargraphResultsMode == SUMMARY_INTERVAL_RESULTS)
		{
#if 0 /* Original */
			length = (uint8)sprintf(buff, "%s", getLangText(SUMMARY_INTERVAL_TEXT));
#else /* Add SI number to display */
			length = (uint8)sprintf(buff, "%s %d", getLangText(SUMMARY_INTERVAL_TEXT), (g_pendingBargraphRecord.summary.calculated.summariesCaptured + 1));
#endif
		}
		else if (g_displayBargraphResultsMode == JOB_PEAK_RESULTS)
		{
			length = (uint8)sprintf(buff, "%s", getLangText(JOB_PEAK_RESULTS_TEXT));
		}
		else // g_displayBargraphResultsMode == IMPULSE_RESULTS
		{
			length = (uint8)sprintf(buff, "%s", getLangText(IMPULSE_RESULTS_TEXT));
		}
				
		wnd_layout_ptr->curr_col =(uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));

		WndMpWrtString((uint8*)(&buff[0]), wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
		wnd_layout_ptr->curr_row = wnd_layout_ptr->next_row;

		if ((g_factorySetupRecord.seismicSensorType == SENSOR_ACC_832M1_0200) || (g_factorySetupRecord.seismicSensorType == SENSOR_ACC_832M1_0500))
		{
			div = (float)(g_bitAccuracyMidpoint * g_sensorInfo.sensorAccuracy * gainFactor) / (float)(g_factorySetupRecord.seismicSensorType * ACC_832M1_SCALER);
		}
		else div = (float)(g_bitAccuracyMidpoint * g_sensorInfo.sensorAccuracy * gainFactor) / (float)(g_factorySetupRecord.seismicSensorType);

		if (1) // Removing option to check the Bar Channel (g_triggerRecord.berec.barChannel != BAR_AIR_CHANNEL)
		{
			//-----------------------------------------------------------------------
			// Max Results Header
			//-----------------------------------------------------------------------
			memset(&buff[0], 0, sizeof(buff));
			length = (uint8)sprintf(buff, "PEAK | R  | T  | V  |");

			wnd_layout_ptr->curr_col =(uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));

			WndMpWrtString((uint8*)(&buff[0]), wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
			wnd_layout_ptr->curr_row = wnd_layout_ptr->next_row;

			//-----------------------------------------------------------------------
			// Peak Results
			//-----------------------------------------------------------------------
			memset(&buff[0], 0, sizeof(buff));
			memset(&displayFormat[0], 0, sizeof(displayFormat));

			if (g_displayBargraphResultsMode == SUMMARY_INTERVAL_RESULTS)
			{
				tempR = ((float)g_bargraphSummaryInterval.r.peak / (float)div);
				tempT = ((float)g_bargraphSummaryInterval.t.peak / (float)div);
				tempV = ((float)g_bargraphSummaryInterval.v.peak / (float)div);
			}
			else if (g_displayBargraphResultsMode == JOB_PEAK_RESULTS)
			{
				tempR = ((float)g_rJobPeak / (float)div);
				tempT = ((float)g_tJobPeak / (float)div);
				tempV = ((float)g_vJobPeak / (float)div);
			}
			else // g_displayBargraphResultsMode == IMPULSE_RESULTS
			{
				tempR = ((float)g_rImpulsePeak / (float)div);
				tempT = ((float)g_tImpulsePeak / (float)div);
				tempV = ((float)g_vImpulsePeak / (float)div);
			}

			if ((g_sensorInfo.unitsFlag == IMPERIAL_TYPE) || (g_factorySetupRecord.seismicSensorType > SENSOR_ACC_RANGE_DIVIDER))
			{
				if (g_factorySetupRecord.seismicSensorType > SENSOR_ACC_RANGE_DIVIDER)
					strcpy(buff, "mg/s |");
				else
					strcpy(buff, "in/s |");
			}
			else // g_sensorInfo.unitsFlag == METRIC_TYPE
			{
				tempR *= (float)METRIC;
				tempT *= (float)METRIC;
				tempV *= (float)METRIC;

				strcpy(buff, "mm/s |");
			}		

			// Make sure formatting is correct
			if (tempR > 100)
				sprintf(displayFormat, "%4d|", (int)tempR);
			else
				sprintf(displayFormat, "%4.1f|", (double)tempR);
			strcat(buff, displayFormat);
		
			if (tempT > 100)
				sprintf(displayFormat, "%4d|", (int)tempT);
			else
				sprintf(displayFormat, "%4.1f|", (double)tempT);
			strcat(buff, displayFormat);

			if (tempV > 100)
				sprintf(displayFormat, "%4d|", (int)tempV);
			else
				sprintf(displayFormat, "%4.1f|", (double)tempV);
			strcat(buff, displayFormat);

			length = 21;
			wnd_layout_ptr->curr_col =(uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));

			WndMpWrtString((uint8*)(&buff[0]),wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
			wnd_layout_ptr->curr_row = wnd_layout_ptr->next_row;

			//-----------------------------------------------------------------------
			// Peak Freq Results
			//-----------------------------------------------------------------------
			memset(&buff[0], 0, sizeof(buff));
			memset(&displayFormat[0], 0, sizeof(displayFormat));
			tempR = tempV = tempT = (float)0.0;

			if (g_displayBargraphResultsMode != IMPULSE_RESULTS)
			{
				if (g_displayBargraphResultsMode == SUMMARY_INTERVAL_RESULTS)
				{
					if (g_bargraphSummaryInterval.r.frequency > 0)
					{
						tempR = (float)((float)g_pendingBargraphRecord.summary.parameters.sampleRate /
								((float)((g_bargraphSummaryInterval.r.frequency * 2) - 1)));
					}
					if (g_bargraphSummaryInterval.v.frequency > 0)
					{
						tempV = (float)((float)g_pendingBargraphRecord.summary.parameters.sampleRate /
								((float)((g_bargraphSummaryInterval.v.frequency * 2) - 1)));
					}
					if (g_bargraphSummaryInterval.t.frequency > 0)
					{
						tempT = (float)((float)g_pendingBargraphRecord.summary.parameters.sampleRate /
								((float)((g_bargraphSummaryInterval.t.frequency * 2) - 1)));
					}
				}
				else // g_displayBargraphResultsMode == JOB_PEAK_RESULTS
				{
					if (g_rJobFreq > 0)
					{
						tempR = (float)((float)g_pendingBargraphRecord.summary.parameters.sampleRate /
								((float)((g_rJobFreq * 2) - 1)));
					}
					if (g_vJobFreq > 0)
					{
						tempV = (float)((float)g_pendingBargraphRecord.summary.parameters.sampleRate /
								((float)((g_vJobFreq * 2) - 1)));
					}
					if (g_tJobFreq > 0)
					{
						tempT = (float)((float)g_pendingBargraphRecord.summary.parameters.sampleRate /
								((float)((g_tJobFreq * 2) - 1)));
					}
				}

				strcpy(buff, "F(Hz)|");

				// Make sure formatting is correct
				if (tempR > 100)
					sprintf(displayFormat, "%4d|", (int)tempR);
				else
					sprintf(displayFormat, "%4.1f|", (double)tempR);
				strcat(buff, displayFormat);
			
				if (tempT > 100)
					sprintf(displayFormat, "%4d|", (int)tempT);
				else
					sprintf(displayFormat, "%4.1f|", (double)tempT);
				strcat(buff, displayFormat);

				if (tempV > 100)
					sprintf(displayFormat, "%4d|", (int)tempV);
				else
					sprintf(displayFormat, "%4.1f|", (double)tempV);
				strcat(buff, displayFormat);

				length = 21;
				wnd_layout_ptr->curr_col =(uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));

				WndMpWrtString((uint8*)(&buff[0]),wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
				wnd_layout_ptr->curr_row = wnd_layout_ptr->next_row;
			}
		}

		//-----------------------------------------------------------------------
		// Max Air/Max Vector Sum Results
		//-----------------------------------------------------------------------
		memset(&buff[0], 0, sizeof(buff));
		memset(&displayFormat[0], 0, sizeof(displayFormat));
		tempA = (float)0.0;

		// Check if displaying the vector sum and if the bar channel isn't just air
		if (g_displayAlternateResultState == VECTOR_SUM_RESULTS) // Removing option to check Bar Channel (g_triggerRecord.berec.barChannel != BAR_AIR_CHANNEL)
		{
			if (g_displayBargraphResultsMode == IMPULSE_RESULTS) { tempVS = sqrtf((float)g_vsImpulsePeak) / (float)div; }
			else if (g_displayBargraphResultsMode == SUMMARY_INTERVAL_RESULTS)
			{
					tempVS = sqrtf((float)g_bargraphSummaryInterval.vectorSumPeak) / (float)div;
			}			
			else if (g_displayBargraphResultsMode == JOB_PEAK_RESULTS) { tempVS = sqrtf((float)g_vsJobPeak) / (float)div; }

			if ((g_sensorInfo.unitsFlag == IMPERIAL_TYPE) || (g_factorySetupRecord.seismicSensorType > SENSOR_ACC_RANGE_DIVIDER))
			{
				if (g_factorySetupRecord.seismicSensorType > SENSOR_ACC_RANGE_DIVIDER)
					strcpy(displayFormat, "mg/s");
				else
					strcpy(displayFormat, "in/s");
			}
			else // Metric
			{
				tempVS *= (float)METRIC;

				strcpy(displayFormat, "mm/s");
			}

			sprintf(buff,"VS %.2f %s", (double)tempVS, displayFormat);
		}
		else if (g_displayAlternateResultState == PEAK_DISPLACEMENT_RESULTS) // Removing option to check Bar Channel (g_triggerRecord.berec.barChannel != BAR_AIR_CHANNEL)
		{
			if (g_displayBargraphResultsMode == SUMMARY_INTERVAL_RESULTS)
			{
				tempR = g_bargraphSummaryInterval.r.peak;
				tempV = g_bargraphSummaryInterval.v.peak;
				tempT = g_bargraphSummaryInterval.t.peak;

				rFreq = (float)((float)g_pendingBargraphRecord.summary.parameters.sampleRate /
						((float)((g_bargraphSummaryInterval.r.frequency * 2) - 1)));
				vFreq = (float)((float)g_pendingBargraphRecord.summary.parameters.sampleRate /
						((float)((g_bargraphSummaryInterval.v.frequency * 2) - 1)));
				tFreq = (float)((float)g_pendingBargraphRecord.summary.parameters.sampleRate /
						((float)((g_bargraphSummaryInterval.t.frequency * 2) - 1)));
			}
			else if (g_displayBargraphResultsMode == JOB_PEAK_RESULTS)
			{
				tempR = g_rJobPeak;
				tempV = g_vJobPeak;
				tempT = g_tJobPeak;

				rFreq = (float)((float)g_pendingBargraphRecord.summary.parameters.sampleRate /
						((float)((g_rJobFreq * 2) - 1)));
				vFreq = (float)((float)g_pendingBargraphRecord.summary.parameters.sampleRate /
						((float)((g_vJobFreq * 2) - 1)));
				tFreq = (float)((float)g_pendingBargraphRecord.summary.parameters.sampleRate /
						((float)((g_tJobFreq * 2) - 1)));
			}

			if (tempR > tempV)
			{
				if (tempR > tempT)
				{
					// R Disp is max
					normalize_max_peak = (float)tempR / (float)div;
					tempFreq = rFreq;
				}
				else
				{
					// T Disp is max
					normalize_max_peak = (float)tempT / (float)div;
					tempFreq = tFreq;
				}
			}
			else
			{
				if (tempV > tempT)
				{
					// V Disp is max
					normalize_max_peak = (float)tempV / (float)div;
					tempFreq = vFreq;
				}
				else
				{
					// T Disp is max
					normalize_max_peak = (float)tempT / (float)div;
					tempFreq = tFreq;
				}
			}

			tempPeakDisp = (float)normalize_max_peak / ((float)2 * (float)PI * (float)tempFreq);

			if ((g_sensorInfo.unitsFlag == IMPERIAL_TYPE) || (g_factorySetupRecord.seismicSensorType > SENSOR_ACC_RANGE_DIVIDER))
			{
				if (g_factorySetupRecord.seismicSensorType > SENSOR_ACC_RANGE_DIVIDER)
					strcpy(displayFormat, "mg");
				else
					strcpy(displayFormat, "in");
			}
			else // Metric
			{
				tempPeakDisp *= (float)METRIC;

				strcpy(displayFormat, "mm");
			}

			sprintf(buff, "PEAK DISP %5.4f %s", (double)tempPeakDisp, displayFormat);
		}
		else if (g_displayAlternateResultState == PEAK_ACCELERATION_RESULTS) // Removing option to check Bar Channel (g_triggerRecord.berec.barChannel != BAR_AIR_CHANNEL)
		{
			if (g_displayBargraphResultsMode == SUMMARY_INTERVAL_RESULTS)
			{
				tempR = g_bargraphSummaryInterval.r.peak;
				tempV = g_bargraphSummaryInterval.v.peak;
				tempT = g_bargraphSummaryInterval.t.peak;

				rFreq = (float)((float)g_pendingBargraphRecord.summary.parameters.sampleRate /
				((float)((g_bargraphSummaryInterval.r.frequency * 2) - 1)));
				vFreq = (float)((float)g_pendingBargraphRecord.summary.parameters.sampleRate /
				((float)((g_bargraphSummaryInterval.v.frequency * 2) - 1)));
				tFreq = (float)((float)g_pendingBargraphRecord.summary.parameters.sampleRate /
				((float)((g_bargraphSummaryInterval.t.frequency * 2) - 1)));
			}
			else if (g_displayBargraphResultsMode == JOB_PEAK_RESULTS)
			{
				tempR = g_rJobPeak;
				tempV = g_vJobPeak;
				tempT = g_tJobPeak;

				rFreq = (float)((float)g_pendingBargraphRecord.summary.parameters.sampleRate / ((float)((g_rJobFreq * 2) - 1)));
				vFreq = (float)((float)g_pendingBargraphRecord.summary.parameters.sampleRate / ((float)((g_vJobFreq * 2) - 1)));
				tFreq = (float)((float)g_pendingBargraphRecord.summary.parameters.sampleRate / ((float)((g_tJobFreq * 2) - 1)));
			}

			if ((tempR * rFreq) > (tempV * vFreq))
			{
				if ((tempR * rFreq) > (tempT * tFreq))
				{
					// R Acc is max
					normalize_max_peak = (float)tempR / (float)div;
					tempFreq = rFreq;
				}
				else
				{
					// T Acc is max
					normalize_max_peak = (float)tempT / (float)div;
					tempFreq = tFreq;
				}
			}
			else
			{
				if ((tempV * vFreq) > (tempT * tFreq))
				{
					// V Acc is max
					normalize_max_peak = (float)tempV / (float)div;
					tempFreq = vFreq;
				}
				else
				{
					// T Acc is max
					normalize_max_peak = (float)tempT / (float)div;
					tempFreq = tFreq;
				}
			}

			tempPeakAcc = (float)normalize_max_peak * (float)2 * (float)PI * (float)tempFreq;

			if ((g_sensorInfo.unitsFlag == IMPERIAL_TYPE) || (g_factorySetupRecord.seismicSensorType > SENSOR_ACC_RANGE_DIVIDER))
			{
				tempPeakAcc /= (float)ONE_GRAVITY_IN_INCHES;
			}
			else // Metric
			{
				tempPeakAcc *= (float)METRIC;
				tempPeakAcc /= (float)ONE_GRAVITY_IN_MM;
			}

			if (g_factorySetupRecord.seismicSensorType > SENSOR_ACC_RANGE_DIVIDER) { strcpy(displayFormat, "mg/s2"); }
			else { strcpy(displayFormat, "g"); }

			sprintf(buff,"PEAK ACC %5.4f %s", (double)tempPeakAcc, displayFormat);
		}
		else // g_displayAlternateResultState == DEFAULT_RESULTS || g_triggerRecord.berec.barChannel == BAR_AIR_CHANNEL
		{
			// Check if the bar channel to display isn't just seismic
			if (1) // Removing option to check Bar Channel (g_triggerRecord.berec.barChannel != BAR_SEISMIC_CHANNEL)
			{
				if (g_displayBargraphResultsMode == IMPULSE_RESULTS)
				{
					if (g_unitConfig.unitsOfAir == MILLIBAR_TYPE) {	sprintf(buff, "%s %0.3f mb", getLangText(PEAK_AIR_TEXT), (double)HexToMB(g_aImpulsePeak, DATA_NORMALIZED, g_bitAccuracyMidpoint, g_factorySetupRecord.acousticSensorType)); }
					else if (g_unitConfig.unitsOfAir == PSI_TYPE) { sprintf(buff, "%s %0.3f psi", getLangText(PEAK_AIR_TEXT), (double)HexToPSI(g_aImpulsePeak, DATA_NORMALIZED, g_bitAccuracyMidpoint, g_factorySetupRecord.acousticSensorType)); }
					else /* (g_unitConfig.unitsOfAir == DECIBEL_TYPE) */ { sprintf(buff, "%s %4.1f dB", getLangText(PEAK_AIR_TEXT), (double)HexToDB(g_aImpulsePeak, DATA_NORMALIZED, g_bitAccuracyMidpoint, g_factorySetupRecord.acousticSensorType)); }
				}
				else // (g_displayBargraphResultsMode == SUMMARY_INTERVAL_RESULTS) || (g_displayBargraphResultsMode == JOB_PEAK_RESULTS)
				{
					if (g_displayBargraphResultsMode == SUMMARY_INTERVAL_RESULTS)
					{
						if (g_bargraphSummaryInterval.a.frequency > 0)
						{
							tempA = (float)((float)g_pendingBargraphRecord.summary.parameters.sampleRate /
									((float)((g_bargraphSummaryInterval.a.frequency * 2) - 1)));
						}

						if (g_unitConfig.unitsOfAir == MILLIBAR_TYPE) { sprintf(buff, "AIR %0.3f mb ", (double)HexToMB(g_bargraphSummaryInterval.a.peak, DATA_NORMALIZED, g_bitAccuracyMidpoint, g_factorySetupRecord.acousticSensorType));	}
						else if (g_unitConfig.unitsOfAir == PSI_TYPE) { sprintf(buff, "AIR %0.3f psi ", (double)HexToPSI(g_bargraphSummaryInterval.a.peak, DATA_NORMALIZED, g_bitAccuracyMidpoint, g_factorySetupRecord.acousticSensorType)); }
						else /* (g_unitConfig.unitsOfAir == DECIBEL_TYPE) */ { sprintf(buff, "AIR %4.1f dB ", (double)HexToDB(g_bargraphSummaryInterval.a.peak, DATA_NORMALIZED, g_bitAccuracyMidpoint, g_factorySetupRecord.acousticSensorType)); }
					}
					else // g_displayBargraphResultsMode == JOB_PEAK_RESULTS
					{
						if (g_aJobFreq > 0)
						{
							tempA = (float)((float)g_pendingBargraphRecord.summary.parameters.sampleRate /
									((float)((g_aJobFreq * 2) - 1)));
						}

						if (g_unitConfig.unitsOfAir == MILLIBAR_TYPE) { sprintf(buff, "AIR %0.3f mb ", (double)HexToMB(g_aJobPeak, DATA_NORMALIZED, g_bitAccuracyMidpoint, g_factorySetupRecord.acousticSensorType)); }
						else if (g_unitConfig.unitsOfAir == PSI_TYPE) { sprintf(buff, "AIR %0.3f psi ", (double)HexToPSI(g_aJobPeak, DATA_NORMALIZED, g_bitAccuracyMidpoint, g_factorySetupRecord.acousticSensorType)); }
						else /* (g_unitConfig.unitsOfAir == DECIBEL_TYPE) */ { sprintf(buff, "AIR %4.1f dB ", (double)HexToDB(g_aJobPeak, DATA_NORMALIZED, g_bitAccuracyMidpoint, g_factorySetupRecord.acousticSensorType)); }
					}

					if (tempA > 100)
						sprintf(displayFormat, "%3d(Hz)", (int)tempA);
					else
						sprintf(displayFormat, "%3.1f(Hz)", (double)tempA);
					strcat(buff, displayFormat);
				}
			}
		}

		length = 21;
		wnd_layout_ptr->curr_col =(uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));

		WndMpWrtString((uint8*)(&buff[0]), wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
		wnd_layout_ptr->curr_row = wnd_layout_ptr->next_row;

		if (g_displayBargraphResultsMode == IMPULSE_RESULTS)
		{
			memset(&buff[0], 0, sizeof(buff));
		
			length = (uint8)sprintf(buff, "(%d %s)", g_triggerRecord.berec.impulseMenuUpdateSecs, (g_triggerRecord.berec.impulseMenuUpdateSecs == 1) ?
									getLangText(SECOND_TEXT) : getLangText(SECONDS_TEXT));

			wnd_layout_ptr->curr_col = (uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));

			// Always display the refresh time on the last line
			wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_SEVEN;
			WndMpWrtString((uint8*)(&buff[0]),wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
		}

		// Keep impulse values refreshed based on LCD updates (every second)
		g_impulseMenuCount++;
	}
	
	if (g_promtForCancelingPrintJobs == TRUE)
	{
		MessageBorder();
		MessageTitle(getLangText(VERIFY_TEXT));
		MessageText(getLangText(CANCEL_ALL_PRINT_JOBS_Q_TEXT));
		MessageChoice(MB_YESNO);

		if (g_monitorModeActiveChoice == MB_SECOND_CHOICE)
			MessageChoiceActiveSwap(MB_YESNO);
	}
	
	if (g_promtForLeavingMonitorMode == TRUE)
	{
		MessageBorder();
		MessageTitle(getLangText(WARNING_TEXT));
		MessageText(getLangText(DO_YOU_WANT_TO_LEAVE_MONITOR_MODE_Q_TEXT));
		MessageChoice(MB_YESNO);

		if (g_monitorModeActiveChoice == MB_SECOND_CHOICE)
			MessageChoiceActiveSwap(MB_YESNO);
	}
}
