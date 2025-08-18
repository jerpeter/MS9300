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
#include "Typedefs.h"
#include "RealTimeClock.h"
#include "Menu.h"
#include "SoftTimer.h"
#include "Display.h"
#include "PowerManagement.h"
#include "InitDataBuffers.h"
#include "SysEvents.h"
#include "OldUart.h"
#include "Keypad.h"
#include "RemoteOperation.h"
#include "ProcessBargraph.h"
#include "TextTypes.h"
#include "EventProcessing.h"
#include "RemoteHandler.h"

#include "uart.h"
#include "mxc_errors.h"

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
uint8 IsSoftTimerActive(uint16 timerNum)
{
	if ((timerNum < NUM_OF_SOFT_TIMERS) && (g_rtcTimerBank[timerNum].state == TIMER_ENABLED))
	{
		return YES;
	}

	return NO;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AssignSoftTimer(uint16 timerNum, uint32 timeout, void* callback)
{
	if (timerNum >= NUM_OF_SOFT_TIMERS)
	{
		debugErr("AssignSoftTimer Error: Timer Number not valid: %d\r\n", timerNum);
		return;
	}

	// Check that the timeout condition is set for some time in the future
	if (timeout > 0)
	{
		g_rtcTimerBank[timerNum].state = TIMER_ENABLED;
		g_rtcTimerBank[timerNum].tickStart = g_lifetimeHalfSecondTickCount;
		g_rtcTimerBank[timerNum].timePeriod = g_lifetimeHalfSecondTickCount + timeout;
		g_rtcTimerBank[timerNum].timeoutValue = timeout;
		g_rtcTimerBank[timerNum].callback = callback;
	}
	else // Timeout is zero, go ahead and clear the timer just in case it's already set/active
	{
		ClearSoftTimer(timerNum);
	}

	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ResetSoftTimer(uint16 timerNum)
{
	if (timerNum >= NUM_OF_SOFT_TIMERS)
	{
		debugErr("AssignSoftTimer Error: Timer Number not valid: %d\r\n", timerNum);
		return;
	}

	if (g_rtcTimerBank[timerNum].state == TIMER_ENABLED)
	{
		g_rtcTimerBank[timerNum].tickStart = g_lifetimeHalfSecondTickCount;
		g_rtcTimerBank[timerNum].timePeriod = g_lifetimeHalfSecondTickCount + g_rtcTimerBank[timerNum].timeoutValue;
	}

	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ClearSoftTimer(uint16 timerNum)
{
	if (timerNum >= NUM_OF_SOFT_TIMERS)
	{
		debugErr("ClearSoftTimer Error: Timer Number not valid: %d\r\n", timerNum);
		return;
	}

	g_rtcTimerBank[timerNum].state = TIMER_DISABLED;
	g_rtcTimerBank[timerNum].tickStart = 0;
	g_rtcTimerBank[timerNum].timePeriod = 0;
	g_rtcTimerBank[timerNum].timeoutValue = 0;
	g_rtcTimerBank[timerNum].callback = NULL;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void CheckSoftTimers(void)
{
	uint16 softTimerIndex;

	// Loop for our of our timers.
	for (softTimerIndex = 0; softTimerIndex < NUM_OF_SOFT_TIMERS; softTimerIndex++)
	{
		// Check if it is in use.
		if (g_rtcTimerBank[softTimerIndex].state == TIMER_ENABLED)
		{
			// Did our timer loop or wrap around the max?
			if (g_rtcTimerBank[softTimerIndex].tickStart > g_rtcTimerBank[softTimerIndex].timePeriod)
			{
				// We looped but did the tick count loop around the MAX?
				if (g_lifetimeHalfSecondTickCount < g_rtcTimerBank[softTimerIndex].tickStart)
				{
					// The counter looped so check the condition of the time period.
					if (g_lifetimeHalfSecondTickCount >= g_rtcTimerBank[softTimerIndex].timePeriod)
					{
						// Once the timer has activated, disable the timer and clear values
						g_rtcTimerBank[softTimerIndex].state = TIMER_DISABLED;
						g_rtcTimerBank[softTimerIndex].tickStart = 0;
						g_rtcTimerBank[softTimerIndex].timePeriod = 0;

						// Process the callback, or function is null and an error.
						if (g_rtcTimerBank[softTimerIndex].callback != NULL)
						{
							(*(g_rtcTimerBank[softTimerIndex].callback)) ();
						}
						else
						{
							debug("CheckSoftTimers:Error function call is NULL\r\n");
						}
					}
				}
			}
			// The timer did not loop. But did our tick count loop?
			else
			{
				// Did the tick count loop around the MAX? Or did we go past the time?
				if ((g_lifetimeHalfSecondTickCount < g_rtcTimerBank[softTimerIndex].tickStart) ||
					(g_lifetimeHalfSecondTickCount >= g_rtcTimerBank[softTimerIndex].timePeriod))
				{
					// Once the timer has activated, disable the timer state and clear values
					g_rtcTimerBank[softTimerIndex].state = TIMER_DISABLED;
					g_rtcTimerBank[softTimerIndex].tickStart = 0;
					g_rtcTimerBank[softTimerIndex].timePeriod = 0;

					// Process the callback, or function is null and an error.
					if (g_rtcTimerBank[softTimerIndex].callback != NULL)
					{
						(*(g_rtcTimerBank[softTimerIndex].callback)) ();
					}
					else
					{
						debug("CheckSoftTimers:Error function call is NULL\r\n");
					}
				}
			}
		}
	}

	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DisplayTimerCallBack(void)
{
	debug("LCD Backlight Timer callback: activated.\r\n");

#if 0 /* Orignial */
	g_lcdBacklightFlag = DISABLED;
#if 0 /* Original */
	SetLcdBacklightState(BACKLIGHT_OFF);
#else /* LCD is basically not readable when the Backlight is off */
	SetLcdBacklightState(BACKLIGHT_SUPER_LOW);
#endif
#else /* Updated method to make sure the LCD resource is available before adjusting */
	//if (g_lcdBacklightFlag == ENABLED)
	if ((g_lcdBacklightFlag == ENABLED) && (GetPowerControlState(LCD_POWER_ENABLE) == ON))
	{
		g_lcdBacklightFlag = DISABLED; // Now treating BACKLIGHT_SUPER_LOW as disabled since the LCD really can't be seen with the backlight on at some level
		SetLcdBacklightState(BACKLIGHT_SUPER_LOW);
	}
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AlarmOneOutputTimerCallback(void)
{
	// Deactivate alarm 1 signal
	PowerControl(ALARM_1_ENABLE, OFF);

	debug("Alert/Alarm 1 Timer callback: warning finished\r\n");

	// Check if the state of the other Alert/Alarm 2 is inactive
	if (GetPowerControlState(ALARM_2_ENABLE) == OFF)
	{
		// Can disable the remote power for the Alert/Alarms for now
		PowerControl(ENABLE_12V, OFF);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AlarmTwoOutputTimerCallback(void)
{
	// Deactivate alarm 2 signal
	PowerControl(ALARM_2_ENABLE, OFF);

	debug("Alert/Alarm 2 Timer callback: warning finished\r\n");

	// Check if the state of the other Alert/Alarm 1 is inactive
	if (GetPowerControlState(ALARM_1_ENABLE) == OFF)
	{
		// Can disable the remote power for the Alert/Alarms for now
		PowerControl(ENABLE_12V, OFF);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void LcdPwTimerCallBack(void)
{
	debug("LCD Power Timer callback: activated.\r\n");

	g_lcdPowerFlag = DISABLED;
	g_lcdBacklightFlag = DISABLED; // Added disabling the Backlight flag since it goes down when power removed

	// Todo: Update to only power off display first, set timer to power off entire LCD section
	PowerControl(LCD_POWER_DOWN, ON);
	PowerControl(LCD_POWER_ENABLE, OFF);

	if (g_sampleProcessing == ACTIVE_STATE)
	{
#if 0 /* Normal */
		debug("LCD Power Timer callback: disabling Monitor Update Timer.\r\n");
		ClearSoftTimer(MENU_UPDATE_TIMER_NUM);
#else /* Test */
		// Display normal info to debug which won't happen if the monitoring menu display logic isn't run with the menu update
#endif
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MenuUpdateTimerCallBack(void)
{
	INPUT_MSG_STRUCT mn_msg;

	// Clear out the message parameters
	mn_msg.cmd = 0; mn_msg.length = 0; mn_msg.data[0] = 0;

	// Recall the current active menu to repaint the display
	JUMP_TO_ACTIVE_MENU();

	AssignSoftTimer(MENU_UPDATE_TIMER_NUM, ONE_SECOND_TIMEOUT, MenuUpdateTimerCallBack);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void KeypadLedUpdateTimerCallBack(void)
{
	static uint8 s_ledState = KEYPAD_LED_STATE_UNKNOWN;
	static uint8 s_repeatCount = 0;
	uint8 lastLedState;
	BOOLEAN externalChargePresent = CheckExternalChargeVoltagePresent();

	// States (on prior 8100 unit)
	// 1) Init complete, not monitoring, not charging --> Static Green
	// 2) Init complete, not monitoring, charging --> Static Red
	// 3) Init complete, monitoring, not charging --> Flashing Green (state transition)
	// 4) Init complete, monitoring, charging --> Alternate Flashing Green/Red (state transition)

	// States (on prior 8100 unit)
	// 1) Init complete, not monitoring, not charging, LCD on --> Static Green
	// 2) Init complete, not monitoring, not charging, LCD off --> Pulse Green 0.5s every 4s
	// 3) Init complete, not monitoring, charging, LCD on --> Static Blue
	// 4) Init complete, not monitoring, charging, LCD off --> Pulse Blue 0.5s every 4s
	// 5) Init complete, monitoring, not charging, LCD on --> Static Green
	// 6) Init complete, monitoring, not charging, LCD off --> Pulse Green 0.5s every 2s
	// 7) Init complete, monitoring, charging, LCD on --> Static Blue
	// 8) Init complete, monitoring, charging, LCD off --> Pulse Blue 0.5s every 4s

	// Temp for field test
	// 1) Not monitoring, not charging, Pulse Green 0.5 ever 4s
	// 2) Not monitoring, charging, Pulse Blue 0.5 ever 4s
	// 3) Monitoring, not charging, Pulse Green 0.5 ever 2s
	// 4) Monitoring, charging, Pulse Blue 0.5 ever 2s

	// Hold the last state
	lastLedState = s_ledState;

	if ((g_sampleProcessing == IDLE_STATE) && (externalChargePresent == FALSE))
	{
		//debug("Keypad LED: State 1\r\n");
		if (lastLedState == KEYPAD_LED_STATE_PULSE_GREEN_SLOW_ON) { s_ledState = KEYPAD_LED_STATE_PULSE_GREEN_SLOW_OFF; s_repeatCount = 0; }
		else if (lastLedState == KEYPAD_LED_STATE_PULSE_GREEN_SLOW_OFF) { if (++s_repeatCount == 7) { s_ledState = KEYPAD_LED_STATE_PULSE_GREEN_SLOW_ON; } }
		else { s_ledState = KEYPAD_LED_STATE_PULSE_GREEN_SLOW_ON; }
	}
	else if ((g_sampleProcessing == IDLE_STATE) && (externalChargePresent == TRUE))
	{
		//debug("Keypad LED: State 2\r\n");
		if (lastLedState == KEYPAD_LED_STATE_PULSE_BLUE_SLOW_ON) { s_ledState = KEYPAD_LED_STATE_PULSE_BLUE_SLOW_OFF; s_repeatCount = 0; }
		else if (lastLedState == KEYPAD_LED_STATE_PULSE_BLUE_SLOW_OFF) { if (++s_repeatCount == 7) { s_ledState = KEYPAD_LED_STATE_PULSE_BLUE_SLOW_ON; } }
		else { s_ledState = KEYPAD_LED_STATE_PULSE_BLUE_SLOW_ON; }
	}
	else if ((g_sampleProcessing == ACTIVE_STATE) && (externalChargePresent == FALSE))
	{
		//debug("Keypad LED: State 3\r\n");
		if (lastLedState == KEYPAD_LED_STATE_PULSE_GREEN_FAST_ON) { s_ledState = KEYPAD_LED_STATE_PULSE_GREEN_FAST_OFF; s_repeatCount = 0; }
		else if (lastLedState == KEYPAD_LED_STATE_PULSE_GREEN_FAST_OFF) { if (++s_repeatCount == 3) { s_ledState = KEYPAD_LED_STATE_PULSE_GREEN_FAST_ON; } }
		else { s_ledState = KEYPAD_LED_STATE_PULSE_GREEN_FAST_ON; }
	}		
	else // ((g_sampleProcessing == ACTIVE_STATE) && (externalChargePresent == TRUE))
	{
		//debug("Keypad LED: State 4\r\n");
		if (lastLedState == KEYPAD_LED_STATE_PULSE_BLUE_FAST_ON) { s_ledState = KEYPAD_LED_STATE_PULSE_BLUE_FAST_OFF; s_repeatCount = 0; }
		else if (lastLedState == KEYPAD_LED_STATE_PULSE_BLUE_FAST_OFF) { if (++s_repeatCount == 3) { s_ledState = KEYPAD_LED_STATE_PULSE_BLUE_FAST_ON; } }
		else { s_ledState = KEYPAD_LED_STATE_PULSE_BLUE_FAST_ON; }
	}
	
	switch (s_ledState)
	{
		case KEYPAD_LED_STATE_BOTH_OFF:
		case KEYPAD_LED_STATE_ACTIVE_GREEN_OFF:
			// Todo: Set the correct state or LED's
			break;

		case KEYPAD_LED_STATE_IDLE_GREEN_ON:
		case KEYPAD_LED_STATE_ACTIVE_GREEN_ON:
		case KEYPAD_LED_STATE_ACTIVE_CHARGE_GREEN_ON:
			// Todo: Set the correct state or LED's
			break;

		case KEYPAD_LED_STATE_CHARGE_RED_ON:
		case KEYPAD_LED_STATE_ACTIVE_CHARGE_RED_ON:
			// Todo: Set the correct state or LED's
			break;

		case KEYPAD_LED_STATE_PULSE_GREEN_SLOW_ON:
		case KEYPAD_LED_STATE_PULSE_GREEN_FAST_ON:
			PowerControl(LED_2, ON); PowerControl(LED_1, OFF);
			break;

		case KEYPAD_LED_STATE_PULSE_GREEN_SLOW_OFF:
		case KEYPAD_LED_STATE_PULSE_GREEN_FAST_OFF:
			PowerControl(LED_2, OFF); PowerControl(LED_1, OFF);
			break;

		case KEYPAD_LED_STATE_PULSE_BLUE_SLOW_ON:
		case KEYPAD_LED_STATE_PULSE_BLUE_FAST_ON:
			PowerControl(LED_1, ON); PowerControl(LED_2, OFF);
			break;

		case KEYPAD_LED_STATE_PULSE_BLUE_SLOW_OFF:
		case KEYPAD_LED_STATE_PULSE_BLUE_FAST_OFF:
			PowerControl(LED_1, OFF); PowerControl(LED_2, OFF);
			break;
	}

	AssignSoftTimer(KEYPAD_LED_TIMER_NUM, HALF_SECOND_TIMEOUT, KeypadLedUpdateTimerCallBack);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void PowerOffTimerCallback(void)
{
	debug("Power Off Timer callback: activated.\r\n");

	// New method to validate power off by holding the Power On button for some length of time

	// Check if attempting to power off and Power On button is still pressed
	if ((g_powerOffAttempted) && (GetPowerOnButtonState() == OFF))
	{
		// Reset state and return without any further action
		g_powerOffAttempted = NO;
		return;
	}

	// Prevent power off if monitoring
	if (g_sampleProcessing != IDLE_STATE)
	{
		ActivateDisplayShortDuration(1);
		OverlayMessage(getLangText(WARNING_TEXT), "MONITORING.. UNABLE TO POWER OFF", 2 * SOFT_SECS);
		return;
	}

	if ((g_unitConfig.timerMode == ENABLED) && (g_allowQuickPowerOffForTimerModeSetup == NO))
	{
		// Check if the user wants to leave timer mode
		HandleUserPowerOffDuringTimerMode();
		return;
	}

	// Begin shutdown due to power button action or Timer mode
	g_powerOffActivated = YES;

	// Handle and finish any processing
	StopMonitoring(g_triggerRecord.opMode, FINISH_PROCESSING);

	if (g_lcdContrastChanged == YES)
	{
		// Save Unit Config here to prevent constant saving on LCD contrast adjustment
		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);
	}

	ActivateDisplayShortDuration(1);
	OverlayMessage(getLangText(STATUS_TEXT), getLangText(POWERING_UNIT_OFF_NOW_TEXT), 2 * SOFT_SECS);

	// Power the unit off
	debug("Normal mode: User desires to power off\r\n");

	PowerUnitOff(SHUTDOWN_UNIT);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void PowerOffTimerModeCallback(void)
{
	debug("Power Off Timer Mode callback: activated.\r\n");

	// Begin shutdown
	g_powerOffActivated = YES;

	// Handle and finish any processing
	StopMonitoring(g_triggerRecord.opMode, FINISH_PROCESSING);

	if (g_timerModeLastRun == YES)
	{
		debug("Timer Mode: Ending last session, now disabling...\r\n");
		g_unitConfig.timerMode = DISABLED;

		// Save Unit Config (also covers LCD contrast change case)
		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);
	}
	else if (g_lcdContrastChanged == YES)
	{
		// Save Unit Config here to prevent constant saving on LCD contrast adjustment
		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);
	}

	OverlayMessage(getLangText(TIMER_MODE_TEXT), getLangText(POWERING_UNIT_OFF_NOW_TEXT), 3 * SOFT_SECS);

	// Power the unit off
	debug("Timer mode: Finished for the day, sleep time.\r\n");

	PowerUnitOff(SHUTDOWN_UNIT);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ModemDelayTimerCallback(void)
{
#if 0 /* Orignal */
	if (YES == g_modemStatus.modemAvailable)
	{
		if ((READ_DSR == MODEM_CONNECTED) && (READ_DCD == NO_CONNECTION))
		{
			ModemInitProcess();
		}
		else
		{
			AssignSoftTimer(MODEM_DELAY_TIMER_NUM, MODEM_ATZ_DELAY, ModemDelayTimerCallback);
		}
	}
#else
	// Check if actively sending a command or if the system is unlocked (presumably with an active conneciton)
	if ((g_modemStatus.xferState != NOP_CMD) || (g_modemStatus.systemIsLockedFlag == NO))
	{
		// Check again when not busy
		AssignSoftTimer(MODEM_DELAY_TIMER_NUM, MODEM_ATZ_DELAY, ModemDelayTimerCallback);
	}
	else if ((g_modemStatus.modemAvailable == YES) || ((g_modemStatus.remoteResponse == OK_RESPONSE)) || ((g_modemStatus.remoteResponse == READY_RESPONSE)))
	{
		// Flag for yes if prior state was not available
		g_modemStatus.modemAvailable = YES;

#if 0 /* Original */
		ModemInitProcess();
#else /* Cell modem */
		debug("Cell modem available\r\n");
		PowerControl(LTE_RESET, ON);
		PowerControl(CELL_ENABLE, OFF);
#endif
	}
	else // ((g_modemStatus.modemAvailable == NO) && (g_modemStatus.remoteResponse != OK_RESPONSE))
	{
#if 0 /* Original */
		g_modemStatus.remoteResponse = NO_RESPONSE;
		UartPuts((char*)(INIT_CMD_STRING), CRAFT_COM_PORT);
		UartPuts((char*)&g_CRLF, CRAFT_COM_PORT);
#else /* Cell modem */
		if(GetPowerControlState(CELL_ENABLE) == OFF)
		{
			debug("Cell/LTE: Powering section...\r\n");
			PowerControl(CELL_ENABLE, ON);
			SoftUsecWait(1 * SOFT_SECS); // Small charge up delay
			debug("Cell/LTE: Disabling LTE reset...\r\n");
			PowerControl(LTE_RESET, OFF);
		}
#if 0 /* Original */
		else // Check External modem
		{
			g_modemStatus.remoteResponse = NO_RESPONSE;
			UartPuts((char*)(INIT_CMD_STRING), CRAFT_COM_PORT);
			UartPuts((char*)&g_CRLF, CRAFT_COM_PORT);
		}
#endif
#endif

		// Check again
		AssignSoftTimer(MODEM_DELAY_TIMER_NUM, MODEM_ATZ_DELAY, ModemDelayTimerCallback);
	}
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ModemResetTimerCallback(void)
{
	if (g_modemResetStage == 0)
	{
		// If for some reason this executes, make sure the timer is disabled
		ClearSoftTimer(MODEM_RESET_TIMER_NUM);
	}
	else if (g_modemResetStage == 1)
	{
		SET_DTR;

		AssignSoftTimer(MODEM_RESET_TIMER_NUM, (uint32)(3 * TICKS_PER_SEC), ModemResetTimerCallback);
		g_modemResetStage = 2;
	}
	else if (g_modemResetStage == 2)
	{
		if (g_modemStatus.xferState == NOP_CMD) { UartPuts((char*)(CMDMODE_CMD_STRING), CRAFT_COM_PORT); }
		AssignSoftTimer(MODEM_RESET_TIMER_NUM, (uint32)(3 * TICKS_PER_SEC), ModemResetTimerCallback);
		g_modemResetStage = 3;
	}
	else if (g_modemResetStage == 3)
	{
		if (g_modemStatus.xferState == NOP_CMD) { UartPuts((char*)(CMDMODE_CMD_STRING), CRAFT_COM_PORT); }
		AssignSoftTimer(MODEM_RESET_TIMER_NUM, (uint32)(3 * TICKS_PER_SEC), ModemResetTimerCallback);
		g_modemResetStage = 4;
	}
	else if (g_modemResetStage == 4)
	{
		g_modemStatus.remoteResponse = NO_RESPONSE;
		if (g_modemStatus.xferState == NOP_CMD) { UartPuts((char*)(ATH_CMD_STRING), CRAFT_COM_PORT); }
		UartPuts((char*)&g_CRLF, CRAFT_COM_PORT);
		AssignSoftTimer(MODEM_RESET_TIMER_NUM, (uint32)(3 * TICKS_PER_SEC), ModemResetTimerCallback);
		g_modemResetStage = 5;
	}
	else if (g_modemResetStage == 5)
	{
		// Check if the modem did not respond to the ATH command
		if (g_modemStatus.remoteResponse != OK_RESPONSE)
		{
			g_modemStatus.modemAvailable = NO;

			g_modemStatus.remoteResponse = NO_RESPONSE;
			UartPuts((char*)(INIT_CMD_STRING), CRAFT_COM_PORT);
			UartPuts((char*)&g_CRLF, CRAFT_COM_PORT);

			AssignSoftTimer(MODEM_DELAY_TIMER_NUM, MODEM_ATZ_DELAY, ModemDelayTimerCallback);
		}
		else
		{
			ModemInitProcess();
		}

		g_modemResetStage = 0;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AutoDialOutCycleTimerCallBack(void)
{
	debug("Auto Dial Out Timer callback: activated.\r\n");

	// Make sure the Auto Dial Out Cycle Timer is disabled
	ClearSoftTimer(AUTO_DIAL_OUT_CYCLE_TIMER_NUM);

	// Check if AutoDialout is enabled and signal the system if necessary
	if (CheckAutoDialoutStatusAndFlagIfAvailable() == NO)
	{
		debug("Auto Dial Out: Unable to start, resetting ADO timer (State %d, Lock %d, M-Avail %d, Reset %d, Status %d)\r\n",
				g_autoDialoutState, g_modemStatus.systemIsLockedFlag, g_modemStatus.modemAvailable, g_modemResetStage, g_modemSetupRecord.modemStatus);
		AssignSoftTimer(AUTO_DIAL_OUT_CYCLE_TIMER_NUM, (uint32)(g_modemSetupRecord.dialOutCycleTime * TICKS_PER_MIN), AutoDialOutCycleTimerCallBack);

#if 0 /* Original - External modem */
		// Check if the modem wasn't found as the reason
		if (g_modemStatus.modemAvailable == NO)
		{
			AssignSoftTimer(MODEM_DELAY_TIMER_NUM, MODEM_ATZ_DELAY, ModemDelayTimerCallback);
		}
#endif
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AutoMonitorTimerCallBack(void)
{
	INPUT_MSG_STRUCT mn_msg;

	debug("Auto Monitor Timer callback: activated.\r\n");

	// Check if the USB is currently handling an active connection
	if (g_usbMassStorageState == USB_CONNECTED_AND_PROCESSING)
	{
		AssignSoftTimer(AUTO_MONITOR_TIMER_NUM, (uint32)(g_unitConfig.autoMonitorMode * TICKS_PER_MIN), AutoMonitorTimerCallBack);
	}
	else
	{
		// Make sure the Auto Monitor timer is disabled
		ClearSoftTimer(AUTO_MONITOR_TIMER_NUM);

		// Check if the unit is not already monitoring
		if (g_sampleProcessing != ACTIVE_STATE)
		{
			if (CheckAndDisplayErrorThatPreventsMonitoring(OVERLAY))
			{
				AssignSoftTimer(AUTO_MONITOR_TIMER_NUM, (uint32)(g_unitConfig.autoMonitorMode * TICKS_PER_MIN), AutoMonitorTimerCallBack);
			}
			else // Safe to enter monitor mode
			{
				// Enter monitor mode with the current mode
				SETUP_MENU_WITH_DATA_MSG(MONITOR_MENU, g_triggerRecord.opMode);
				JUMP_TO_ACTIVE_MENU();
			}
		}
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GpsPowerOffTimerCallBack(void)
{
	DisableGps();

	ActivateDisplayShortDuration(3);
	sprintf((char*)g_spareBuffer, "%s. %s %s", getLangText(TIMED_OUT_TEXT), getLangText(LOCATION_TEXT), getLangText(NOT_FOUND_TEXT));
	OverlayMessage(getLangText(GPS_LOCATION_TEXT), (char*)g_spareBuffer, (3 * SOFT_SECS));

	AssignSoftTimer(GPS_POWER_ON_TIMER_NUM, (GPS_REACTIVATION_TIME_SEARCH_FAIL * TICKS_PER_MIN), GpsPowerOnTimerCallBack);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GpsPowerOnTimerCallBack(void)
{
	EnableGps();
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
extern void AppendBatteryLogEntryFile(uint8 eventType);
void BatteryLogTimerCallback(void)
{
	AppendBatteryLogEntryFile(BATTERY_VOLTAGE);

	AssignSoftTimer(BATTERY_LOG_TIMER_NUM, (g_unitConfig.copies * TICKS_PER_MIN), BatteryLogTimerCallback);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SystemLockTimerCallback(void)
{
	RemoteSystemLock(SET);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AutoEventGenerationCallback(void)
{
#if 0 /* Normal */
	debug("Auto Event Gen: Signaling an external event\r\n");

	if (g_sampleProcessing == ACTIVE_STATE)
	{
		// Establish trigger signal
		g_externalTrigger = EXTERNAL_TRIGGER_EVENT;
	}

	// Reset timer
	AssignSoftTimer(AUTO_EVENT_GENERATION_NUM, (20 * TICKS_PER_MIN), AutoEventGenerationCallback);
#else /* Test */
static uint8_t s_bcChargeState = ON;
	s_bcChargeState ^= ON;
	debug("BC Toggle: %s\r\n", ((s_bcChargeState == ON) ? "On" : "Off"));
	SetBattChargerChargeState(s_bcChargeState);
	AssignSoftTimer(AUTO_EVENT_GENERATION_NUM, (20 * TICKS_PER_SEC), AutoEventGenerationCallback);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TcpServerStartCallback(void)
{
	int strLen, status;

	//----------------------------------------------------------------
	// Stage 1
	//----------------------------------------------------------------
	if (g_tcpServerStartStage == 1)
	{
		//OverlayMessage(getLangText(STATUS_TEXT), "CELL MODEM STARTNG LISTEN SERVER. PLEASE WAIT A MOMENT", (0 * SOFT_SECS));

		if(GetPowerControlState(CELL_ENABLE) == OFF)
		{
			debug("Cell/LTE: Powering section...\r\n");
			PowerControl(CELL_ENABLE, ON);
			SoftUsecWait(1 * SOFT_SECS); // Small charge up delay
			debug("Cell/LTE: Disabling LTE reset...\r\n");
			PowerControl(LTE_RESET, OFF);

			if (CheckforModemReady(6) == YES)
			{
				// Flag for yes if prior state was not available
				g_modemStatus.modemAvailable = YES;
				ClearSoftTimer(MODEM_DELAY_TIMER_NUM);

				g_tcpServerStartStage = 2;
				AssignSoftTimer(TCP_SERVER_START_NUM, (1 * TICKS_PER_SEC), TcpServerStartCallback);
			}
			else // No modem found
			{
				// Reset state
				g_tcpServerStartStage = 0;

				// Power down the cell module
				ShutdownPdnAndCellModem();
				PowerControl(LTE_RESET, ON);
				PowerControl(CELL_ENABLE, OFF);

				// Try TCP Server again after a delay
				debug("TCP Server: No modem found, retrying after delay\r\n");
				AssignSoftTimer(TCP_SERVER_START_NUM, (5 * TICKS_PER_MIN), TcpServerStartCallback);
			}
		}
		else
		{
			g_tcpServerStartStage = 2;
			AssignSoftTimer(TCP_SERVER_START_NUM, (1 * TICKS_PER_SEC), TcpServerStartCallback);
		}
	}
	//----------------------------------------------------------------
	// Stage 2
	//----------------------------------------------------------------
	else if (g_tcpServerStartStage == 2)
	{
		if (strlen(g_cellModemSetupRecord.pdnApn))
		{
			// PDN/APN setting, AT+CGDCONT=0,"IP","psmtneofin"
			debug("AT+CGDCONT=0,\"IP\",\"%s\"...\r\n", g_cellModemSetupRecord.pdnApn); sprintf((char*)g_spareBuffer, "AT+CGDCONT=0,\"IP\",\"%s\"\r\n", g_cellModemSetupRecord.pdnApn); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
		}

		g_tcpServerStartStage = 3;
		AssignSoftTimer(TCP_SERVER_START_NUM, (1 * TICKS_PER_SEC), TcpServerStartCallback);
	}
	//----------------------------------------------------------------
	// Stage 3
	//----------------------------------------------------------------
	else if (g_tcpServerStartStage == 3)
	{
		if (g_cellModemSetupRecord.pdnAuthProtocol != AUTH_NONE)
		{
			debug("AT+CGAUTH=0,%d,\"%s\",\"%s\"...\r\n", g_cellModemSetupRecord.pdnAuthProtocol, g_cellModemSetupRecord.pdnUsername, g_cellModemSetupRecord.pdnPassword);
			sprintf((char*)g_spareBuffer, "AT+CGAUTH=0,%d,\"%s\",\"%s\"\r\n", g_cellModemSetupRecord.pdnAuthProtocol, g_cellModemSetupRecord.pdnUsername, g_cellModemSetupRecord.pdnPassword);
			strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
		}

		g_tcpServerStartStage = 4;
		AssignSoftTimer(TCP_SERVER_START_NUM, (1 * TICKS_PER_SEC), TcpServerStartCallback);
	}
	//----------------------------------------------------------------
	// Stage 4
	//----------------------------------------------------------------
	else if (g_tcpServerStartStage == 4)
	{
		debug("AT+CFUN=1...\r\n"); sprintf((char*)g_spareBuffer, "AT+CFUN=1\r\n"); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }

#if 1 /* Test */
		SetStartCellConnectTime();
#endif
		g_tcpServerStartStage = 5;
		AssignSoftTimer(TCP_SERVER_START_NUM, (1 * TICKS_PER_SEC), TcpServerStartCallback);
	}
	//----------------------------------------------------------------
	// Stage 5
	//----------------------------------------------------------------
	else if (g_tcpServerStartStage == 5)
	{
		if (g_modemStatus.remoteResponse == MODEM_CELL_NETWORK_REGISTERED)
		{
#if 1 /* Test */
			CellConnectStatsUpdate();
			debug("TCP Server: Cell network connect time was %d seconds (Avg: %d)\r\n", GetCellConnectStatsLastConenct(), GetCellConnectStatsAverage());
#endif
			g_tcpServerStartStage = 6;
			AssignSoftTimer(TCP_SERVER_START_NUM, (1 * TICKS_PER_SEC), TcpServerStartCallback);
		}
		// Check if the current cell connect time is greater than 10 minutes (600 seconds)
		else if (GetCurrentCellConnectTime() > (600))
		{
			// Reset state
			g_tcpServerStartStage = 0;

			// Power down the cell module
			ShutdownPdnAndCellModem();
			PowerControl(LTE_RESET, ON);
			PowerControl(CELL_ENABLE, OFF);

			// Try TCP Server again after a delay
			debug("TCP Server: No cell network connection found after 10 minutes, retrying after delay\r\n");
			AssignSoftTimer(TCP_SERVER_START_NUM, (5 * TICKS_PER_MIN), TcpServerStartCallback);
		}
		else
		{
			debug("AT+CEREG?...\r\n"); sprintf((char*)g_spareBuffer, "AT+CEREG?\r\n"); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }

			g_tcpServerStartStage = 5;
			AssignSoftTimer(TCP_SERVER_START_NUM, (1 * TICKS_PER_SEC), TcpServerStartCallback);
		}
	}
	//----------------------------------------------------------------
	// Stage 6
	//----------------------------------------------------------------
	else if (g_tcpServerStartStage == 6)
	{
#if 0 /* Original */
		debug("AT#XTCPSVR=1,8005...\r\n"); sprintf((char*)g_spareBuffer, "AT#XTCPSVR=1,8005\r\n"); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
#else
		debug("AT#XTCPSVR=1,%d...\r\n", g_cellModemSetupRecord.tcpServerListenPort); sprintf((char*)g_spareBuffer, "AT#XTCPSVR=1,%d\r\n", g_cellModemSetupRecord.tcpServerListenPort); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
#endif
		g_tcpServerStartStage = 7;
		AssignSoftTimer(TCP_SERVER_START_NUM, (1 * TICKS_PER_SEC), TcpServerStartCallback);
	}
	//----------------------------------------------------------------
	// Stage 7
	//----------------------------------------------------------------
	else if (g_tcpServerStartStage == 7)
	{
		if ((g_modemStatus.remoteResponse == TCP_SERVER_NOT_STARTED) || (g_modemStatus.remoteResponse == ERROR_RESPONSE))
		{
#if 0 /* Original */
			debug("AT#XTCPSVR=1,8005...\r\n"); sprintf((char*)g_spareBuffer, "AT#XTCPSVR=1,8005\r\n"); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
#else
			debug("AT#XTCPSVR=1,%d...\r\n", g_cellModemSetupRecord.tcpServerListenPort); sprintf((char*)g_spareBuffer, "AT#XTCPSVR=1,%d\r\n", g_cellModemSetupRecord.tcpServerListenPort); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
#endif
			g_tcpServerStartStage = 7;
			AssignSoftTimer(TCP_SERVER_START_NUM, (5 * TICKS_PER_SEC), TcpServerStartCallback);
		}
		else
		{
			g_tcpServerStartStage = 8;
			AssignSoftTimer(TCP_SERVER_START_NUM, (1 * TICKS_PER_SEC), TcpServerStartCallback);
		}
	}
	//----------------------------------------------------------------
	// Stage 8
	//----------------------------------------------------------------
	else if (g_tcpServerStartStage == 8)
	{
		debug("AT#XTCPSEND"); sprintf((char*)g_spareBuffer, "AT#XTCPSEND\r\n"); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }

		g_tcpServerStartStage = 0;
		ClearSoftTimer(TCP_SERVER_START_NUM);

		//OverlayMessage(getLangText(STATUS_TEXT), "CELL MODEM TCP LISTEN SERVER STARTED", (0 * SOFT_SECS));
	}
}