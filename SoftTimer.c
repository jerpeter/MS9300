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
#include "M23018.h"

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

	g_lcdBacklightFlag = DISABLED;
	SetLcdBacklightState(BACKLIGHT_OFF);
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

	// Todo: Only power off display first, set timer to power off LCD section
	PowerControl(LCD_POWER_DISPLAY, OFF);
	PowerControl(LCD_POWER_ENABLE, OFF);

	if (g_sampleProcessing == ACTIVE_STATE)
	{
		debug("LCD Power Timer callback: disabling Monitor Update Timer.\r\n");
		ClearSoftTimer(MENU_UPDATE_TIMER_NUM);
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
	static uint8 ledState = KEYPAD_LED_STATE_UNKNOWN;
	uint8 lastLedState;
	uint8 config;
	BOOLEAN externalChargePresent = CheckExternalChargeVoltagePresent();

	// States
	// 1) Init complete, not monitoring, not charging --> Static Green
	// 2) Init complete, not monitoring, charging --> Static Red
	// 3) Init complete, monitoring, not charging --> Flashing Green (state transition)
	// 4) Init complete, monitoring, charging --> Alternate Flashing Green/Red (state transition)

	// Hold the last state
	lastLedState = ledState;

	if ((g_sampleProcessing == IDLE_STATE) && (externalChargePresent == FALSE))
	{
		//debug("Keypad LED: State 1\r\n");
		
		ledState = KEYPAD_LED_STATE_IDLE_GREEN_ON;
	}
	else if ((g_sampleProcessing == IDLE_STATE) && (externalChargePresent == TRUE))
	{
		//debug("Keypad LED: State 2\r\n");

		ledState = KEYPAD_LED_STATE_CHARGE_RED_ON;
	}
	else if ((g_sampleProcessing == ACTIVE_STATE) && (externalChargePresent == FALSE))
	{
		//debug("Keypad LED: State 3\r\n");

		if (ledState == KEYPAD_LED_STATE_ACTIVE_GREEN_ON)
		{
			ledState = KEYPAD_LED_STATE_ACTIVE_GREEN_OFF;
		}
		else
		{
			ledState = KEYPAD_LED_STATE_ACTIVE_GREEN_ON;
		}
	}		
	else // ((g_sampleProcessing == ACTIVE_STATE) && (externalChargePresent == TRUE))
	{
		//debug("Keypad LED: State 4\r\n");

		if (ledState == KEYPAD_LED_STATE_ACTIVE_CHARGE_GREEN_ON)
		{
			ledState = KEYPAD_LED_STATE_ACTIVE_CHARGE_RED_ON;
		}
		else
		{
			ledState = KEYPAD_LED_STATE_ACTIVE_CHARGE_GREEN_ON;
		}
	}
	
	// Check if the state changed
	if (ledState != lastLedState)
	{
		// Todo: Possibly read current LED's into config state?
		
		switch (ledState)
		{
			case KEYPAD_LED_STATE_BOTH_OFF:
			case KEYPAD_LED_STATE_ACTIVE_GREEN_OFF:
				config &= ~RED_LED_PIN;
				config &= ~GREEN_LED_PIN;
				break;
				
			case KEYPAD_LED_STATE_IDLE_GREEN_ON:
			case KEYPAD_LED_STATE_ACTIVE_GREEN_ON:
			case KEYPAD_LED_STATE_ACTIVE_CHARGE_GREEN_ON:
				config &= ~RED_LED_PIN;
				config |= GREEN_LED_PIN;
				break;
				
			case KEYPAD_LED_STATE_CHARGE_RED_ON:
			case KEYPAD_LED_STATE_ACTIVE_CHARGE_RED_ON:
				config &= ~GREEN_LED_PIN;
				config |= RED_LED_PIN;
				break;
		}

		// Todo: Set the correct LED's
	}

	AssignSoftTimer(KEYPAD_LED_TIMER_NUM, ONE_SECOND_TIMEOUT, KeypadLedUpdateTimerCallBack);
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
		OverlayMessage(getLangText(WARNING_TEXT), "MONITORING.. UNaABLE TO POWER OFF", 1 * SOFT_SECS);
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

	OverlayMessage(getLangText(STATUS_TEXT), getLangText(POWERING_UNIT_OFF_NOW_TEXT), 1 * SOFT_SECS);

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
		if (g_modemStatus.xferState == NOP_CMD) { UartPuts((char*)(ATH_CMD_STRING), CRAFT_COM_PORT); }
		UartPuts((char*)&g_CRLF, CRAFT_COM_PORT);
		AssignSoftTimer(MODEM_RESET_TIMER_NUM, (uint32)(3 * TICKS_PER_SEC), ModemResetTimerCallback);
		g_modemResetStage = 5;
	}
	else if (g_modemResetStage == 5)
	{
		ModemInitProcess();
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
		AssignSoftTimer(AUTO_DIAL_OUT_CYCLE_TIMER_NUM, (uint32)(g_modemSetupRecord.dialOutCycleTime * TICKS_PER_MIN), AutoDialOutCycleTimerCallBack);
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
