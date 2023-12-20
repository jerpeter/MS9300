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
#include "SoftTimer.h"
#include "Record.h"
#include "Menu.h"
#include "TextTypes.h"
#include "PowerManagement.h"
#include "RealTimeClock.h"
#include "SysEvents.h"
#include "Keypad.h"
#include "OldUart.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define IN_PROGRESS	2

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
BOOLEAN TimerModeActiveCheck(void)
{
	BOOLEAN status = FALSE;
	DATE_TIME_STRUCT time = GetExternalRtcTime();
	uint8 choice;

	if (g_unitConfig.validationKey == 0xA5A5)
	{
		// Check if timer mode is enabled
		if (g_unitConfig.timerMode == ENABLED)
		{
			debug("Timer Mode active\r\n");

			// Check if the timer mode settings match the current hour and minute meaning the unit powered itself on
			if ((g_unitConfig.timerStartTime.hour == time.hour) && (g_unitConfig.timerStartTime.min == time.min))
			{
				debug("Timer Mode Check: Matched Timer settings...\r\n");
				status = TRUE;
			}
			// Check again if settings match current hour and near minute meaning unit powered itself on but suffered from long startup
			else if ((g_unitConfig.timerStartTime.hour == time.hour) &&
					((((g_unitConfig.timerStartTime.min + 1) == 60) && (time.min == 0)) || ((g_unitConfig.timerStartTime.min + 1) == time.min)))
			{
				debug("Timer Mode Check: Matched Timer settings (long startup)...\r\n");
				status = TRUE;
			}
			// Check specialty hourly mode and if current minute matches and the current hour is within range
			else if ((g_unitConfig.timerModeFrequency == TIMER_MODE_HOURLY) && (g_unitConfig.timerStartTime.min == time.min) &&
					// Check if the start and stop hours match meaning hourly mode runs every hour
					((g_unitConfig.timerStartTime.hour == g_unitConfig.timerStopTime.hour) ||
					// OR Check if hourly mode does not cross a 24 hour boundary and the current hour is within range
					((g_unitConfig.timerStartTime.hour < g_unitConfig.timerStopTime.hour) && (time.hour >= g_unitConfig.timerStartTime.hour) &&
					(time.hour <= g_unitConfig.timerStopTime.hour)) ||
					// OR Check if the current hour is within range with an hourly mode that does cross a 24 hour boundary
					((time.hour >= g_unitConfig.timerStartTime.hour) || (time.hour <= g_unitConfig.timerStopTime.hour))))
			{
				debug("Timer Mode Check: Matched Timer settings (hourly)...\r\n");
				status = TRUE;
			}
			else
			{
				MessageBox(getLangText(STATUS_TEXT), getLangText(UNIT_IS_IN_TIMER_MODE_TEXT), MB_OK);
				choice = MessageBox(getLangText(WARNING_TEXT), getLangText(CANCEL_TIMER_MODE_Q_TEXT), MB_YESNO);

				if (choice == MB_FIRST_CHOICE)
				{
					g_unitConfig.timerMode = DISABLED;

					// Save Unit Config
					SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

					OverlayMessage(getLangText(STATUS_TEXT), getLangText(TIMER_MODE_DISABLED_TEXT), 2 * SOFT_SECS);
				}
				else // User decided to stay in Timer mode
				{
					// TOD Alarm registers still set from previous run, TOD Alarm Mask re-enabled in rtc init
					// Clear TOD Alarm Mask to allow TOD Alarm interrupt to be generated
					// ADD CODE TO CLEAR ALARM

					OverlayMessage(getLangText(WARNING_TEXT), getLangText(POWERING_UNIT_OFF_NOW_TEXT), 2 * SOFT_SECS);

					// Turn unit off/sleep
					debug("Timer mode: Staying in Timer mode. Powering off now...\r\n");
					PowerUnitOff(SHUTDOWN_UNIT); // Return unnecessary
				}
			}
		}
	}

	return (status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ProcessTimerMode(void)
{
	DATE_TIME_STRUCT currTime = GetExternalRtcTime();

	// Check if the Timer mode activated after stop date
	if (// First Check for past year
		(currTime.year > g_unitConfig.timerStopDate.year) ||

		// Second check for equal year but past month
		((currTime.year == g_unitConfig.timerStopDate.year) && (currTime.month > g_unitConfig.timerStopDate.month)) ||

		// Third check for equal year, equal month, but past day
		((currTime.year == g_unitConfig.timerStopDate.year) && (currTime.month == g_unitConfig.timerStopDate.month) &&
		(currTime.day > g_unitConfig.timerStopDate.day)))
	{
		// Disable alarm output generation
		debug("Timer Mode: Activated after date...\r\n");
		debug("Timer Mode: Disabling...\r\n");
		g_unitConfig.timerMode = DISABLED;

		// Save Unit Config
		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		// Deactivate alarm interrupts
		DisableExternalRtcAlarm();

		// Turn unit off/sleep
		debug("Timer mode: Powering unit off...\r\n");
		PowerUnitOff(SHUTDOWN_UNIT); // Return unnecessary
	}
	// Check if the Timer mode activated before start date
	else if (// First Check for before year
		(currTime.year < g_unitConfig.timerStartDate.year) ||

		// Second check for equal year but before month
		((currTime.year == g_unitConfig.timerStartDate.year) && (currTime.month < g_unitConfig.timerStartDate.month)) ||

		// Third check for equal year, equal month, but before day
		((currTime.year == g_unitConfig.timerStartDate.year) && (currTime.month == g_unitConfig.timerStartDate.month) &&
		(currTime.day < g_unitConfig.timerStartDate.day)))
	{
		debug("Timer Mode: Activated before date...\r\n");
		ResetTimeOfDayAlarm();

		// Turn unit off/sleep
		debug("Timer mode: Powering unit off...\r\n");
		PowerUnitOff(SHUTDOWN_UNIT); // Return unnecessary
	}
	// Check if the Timer mode activated during active dates but on an off day
	else if ((g_unitConfig.timerModeFrequency == TIMER_MODE_WEEKDAYS) && ((currTime.weekday == SAT) || (currTime.weekday == SUN)))
	{
		debug("Timer Mode: Activated on an off day (weekday freq)...\r\n");
		ResetTimeOfDayAlarm();

		// Turn unit off/sleep
		debug("Timer mode: Powering unit off...\r\n");
		PowerUnitOff(SHUTDOWN_UNIT); // Return unnecessary
	}
	// Check if the Timer mode activated during active dates but on an off day
	else if ((g_unitConfig.timerModeFrequency == TIMER_MODE_MONTHLY) && (currTime.day != g_unitConfig.timerStartDate.day))
	{
		debug("Timer Mode: Activated on off day (monthly freq)...\r\n");
		ResetTimeOfDayAlarm();

		// Turn unit off/sleep
		debug("Timer mode: Powering unit off...\r\n");
		PowerUnitOff(SHUTDOWN_UNIT); // Return unnecessary
	}
	// Check if the Timer mode activated on end date (and not hourly mode)
	else if ((g_unitConfig.timerModeFrequency != TIMER_MODE_HOURLY) && (currTime.year == g_unitConfig.timerStopDate.year) &&
			(currTime.month == g_unitConfig.timerStopDate.month) && (currTime.day == g_unitConfig.timerStopDate.day))
	{
		// Disable alarm output generation
		debug("Timer Mode: Activated on end date...\r\n");

		// Signal the timer mode end of session timer to stop timer mode due to this being the last run
		g_timerModeLastRun = YES;

		// Deactivate alarm interrupts
		DisableExternalRtcAlarm();
	}
	// Check if the Timer mode activated on end date and end hour (hourly mode)
	else if ((g_unitConfig.timerModeFrequency == TIMER_MODE_HOURLY) && (currTime.year == g_unitConfig.timerStopDate.year) &&
			(currTime.month == g_unitConfig.timerStopDate.month) && (currTime.day == g_unitConfig.timerStopDate.day) &&
			(currTime.hour == g_unitConfig.timerStopTime.hour))
	{
		// Disable alarm output generation
		debug("Timer Mode: Activated on end date...\r\n");

		// Signal the timer mode end of session timer to stop timer mode due to this being the last run
		g_timerModeLastRun = YES;

		// Deactivate alarm interrupts
		DisableExternalRtcAlarm();
	}
	else // Timer mode started during the active dates on a day it's supposed to run
	{
		if (g_unitConfig.timerModeFrequency == TIMER_MODE_ONE_TIME)
		{
			// Signal the timer mode end of session timer to stop timer mode due to this being the last run
			g_timerModeLastRun = YES;

			// Deactivate alarm interrupts
			DisableExternalRtcAlarm();
		}
		else // All other timer modes
		{
			// Reset the Time of Day Alarm to wake the unit up again
			ResetTimeOfDayAlarm();
		}
	}

	OverlayMessage(getLangText(STATUS_TEXT), getLangText(TIMER_MODE_NOW_ACTIVE_TEXT), (2 * SOFT_SECS));

	raiseTimerEventFlag(TIMER_MODE_TIMER_EVENT);

	// Setup soft timer to turn system off when timer mode is finished for the day (minus the expired secs in the current minute
	AssignSoftTimer(POWER_OFF_TIMER_MODE_NUM, ((g_unitConfig.TimerModeActiveMinutes * 60 * 2) - (currTime.sec * 2)), PowerOffTimerModeCallback);

	debug("Timer mode: running...\r\n");
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleUserPowerOffDuringTimerMode(void)
{
	INPUT_MSG_STRUCT mn_msg;
	uint8 choice;

	// Simulate a keypress if the user pressed the off key, which doesn't register as a keypess
	KeypressEventMgr();

	MessageBox(getLangText(STATUS_TEXT), getLangText(UNIT_IS_IN_TIMER_MODE_TEXT), MB_OK);
	choice = MessageBox(getLangText(WARNING_TEXT), getLangText(CANCEL_TIMER_MODE_Q_TEXT), MB_YESNO);

	// User decided to cancel Timer mode
	if (choice == MB_FIRST_CHOICE)
	{
		g_unitConfig.timerMode = DISABLED;

		// Disable the Power Off timer
		ClearSoftTimer(POWER_OFF_TIMER_NUM);

		// Save Unit Config
		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		OverlayMessage(getLangText(STATUS_TEXT), getLangText(TIMER_MODE_DISABLED_TEXT), 2 * SOFT_SECS);
	}
	else // User decided to stay in Timer mode
	{
		choice = MessageBox(getLangText(STATUS_TEXT), getLangText(POWER_UNIT_OFF_EARLY_Q_TEXT), MB_YESNO);

		if (choice == MB_FIRST_CHOICE)
		{
			MessageBox(getLangText(STATUS_TEXT), getLangText(POWERING_UNIT_OFF_NOW_TEXT), MB_OK);
			MessageBox(getLangText(STATUS_TEXT), getLangText(PLEASE_PRESS_ENTER_TEXT), MB_OK);

			// Turn unit off/sleep
			debug("Timer mode: Shutting down unit early due to user request. Powering off now...\r\n");
			PowerUnitOff(SHUTDOWN_UNIT); // Return unnecessary
		}
	}

	SETUP_MENU_MSG(MAIN_MENU);
	JUMP_TO_ACTIVE_MENU();
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ResetTimeOfDayAlarm(void)
{
	DATE_TIME_STRUCT currTime = GetExternalRtcTime();
	uint8 startDay = 0;
	uint8 startHour = 0;
	uint8 month;

	// RTC Weekday's: Sunday = 1, Monday = 2, Tuesday = 3, Wednesday = 4, Thursday = 5, Friday = 6, Saturday = 7

	//___________________________________________________________________________________________
	//___TIMER_MODE_DAILY
	if (g_unitConfig.timerModeFrequency == TIMER_MODE_DAILY)
	{
#if 1 /* Normal */
		// Get the current day and add one
		startDay = (currTime.day + 1);

		// Check if the start day is beyond the total days in the current month
		if (startDay > g_monthTable[currTime.month].days)
		{
			// Set the start day to the first of next month
			startDay = 1;
		}

		debug("Timer mode: Resetting TOD Alarm with (hour) %d, (min) %d, (start day) %d\r\n",
				g_unitConfig.timerStartTime.hour, g_unitConfig.timerStartTime.min, startDay);
#else /* Test */
		// Loop test on and off in a cycle
		startDay = currTime.day;
		
		g_unitConfig.timerStartTime.min += 3;
		
		if (g_unitConfig.timerStartTime.min >= 60)
		{
			g_unitConfig.timerStartTime.min -= 60;
			g_unitConfig.timerStartTime.hour	+= 1;
			
			if (g_unitConfig.timerStartTime.hour >= 24)
			{
				g_unitConfig.timerStartTime.hour = 0;
				startDay = currTime.day + 1; // Ignore month boundary for testing
			}
		}
		
		// Save new testing timer mode adjustments
		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		debug("Timer mode: Resetting TOD Alarm with (hour) %d, (min) %d, (start day) %d\r\n",
				g_unitConfig.timerStartTime.hour, g_unitConfig.timerStartTime.min, startDay);
#endif

		EnableExternalRtcAlarm(startDay, g_unitConfig.timerStartTime.hour, g_unitConfig.timerStartTime.min, 0);
	}
	//___________________________________________________________________________________________
	//___TIMER_MODE_HOURLY
	else if (g_unitConfig.timerModeFrequency == TIMER_MODE_HOURLY)
	{
		// Establish most common value for the start day
		startDay = currTime.day;
		
		// Check if another hour time slot to run again today
		if (currTime.hour != g_unitConfig.timerStopTime.hour)
		{
			// Set alarm for the next hour
			startHour = currTime.hour + 1;
					
			// Account for end of day boundary
			if (startHour > 23)
			{
				startHour = 0;

				// Get the current day and add one
				startDay = (currTime.day + 1);

				// Check if the start day is beyond the total days in the current month
				if (startDay > g_monthTable[currTime.month].days)
				{
					// Set the start day to the first of next month
					startDay = 1;
				}
			}
		}
		else // Last hour time slot to run today, set alarm for next hour grouping
		{
			// Establish the most common value for the start hour
			startHour = g_unitConfig.timerStartTime.hour;

			// Check if the setup time did not cross the midnight boundary
			if (g_unitConfig.timerStartTime.hour < g_unitConfig.timerStopTime.hour)
			{
				// Set to the following day
				startDay++;

				// Check if the start day is beyond the total days in the current month
				if (startDay > g_monthTable[currTime.month].days)
				{
					// Set the start day to the first of next month
					startDay = 1;
				}
			}
			// Check if start and stop hour are the same meaning hourly mode runs every hour of every day
			else if (g_unitConfig.timerStartTime.hour == g_unitConfig.timerStopTime.hour)
			{
				// Current hour matches start and stop, need to set start hour to the following hour slot
				startHour = (currTime.hour + 1);
				
				// Check if start hour is into tomorrow
				if (startHour > 23)
				{
					// Set start hour to the first hour
					startHour = 0;
					
					// Set to the following day
					startDay++;

					// Check if the start day is beyond the total days in the current month
					if (startDay > g_monthTable[currTime.month].days)
					{
						// Set the start day to the first of next month
						startDay = 1;
					}
				}
			}
			// else the startDay remains today (start day is today, start hour is timer mode start hour)
		}

		debug("Timer mode: Resetting TOD Alarm with (hour) %d, (min) %d, (start day) %d\r\n",
				startHour, g_unitConfig.timerStartTime.min, startDay);

		EnableExternalRtcAlarm(startDay, startHour, g_unitConfig.timerStartTime.min, 0);
	}
	//___________________________________________________________________________________________
	//___TIMER_MODE_WEEKDAYS
	else if (g_unitConfig.timerModeFrequency == TIMER_MODE_WEEKDAYS)
	{
		// Get the current day
		startDay = currTime.day;

		if (currTime.weekday == FRI)
		{
			// Advance 3 days
			startDay += 3;
		}
		else if (currTime.weekday == SAT)
		{
			// Advance 2 days
			startDay += 2;
		}
		else // The rest of the days of the week
		{
			// Advance 1 day
			startDay++;
		}

		// Check if the start day is beyond the number of days in the current month
		if (startDay > g_monthTable[currTime.month].days)
		{
			// Subtract out the number of days in the current month to get the start day for next month
			startDay -= g_monthTable[currTime.month].days;
		}

		debug("Timer mode: Resetting TOD Alarm with (hour) %d, (min) %d, (start ay) %d\r\n",
				g_unitConfig.timerStartTime.hour, g_unitConfig.timerStartTime.min, startDay);

		EnableExternalRtcAlarm(startDay, g_unitConfig.timerStartTime.hour, g_unitConfig.timerStartTime.min, 0);
	}
	//___________________________________________________________________________________________
	//___TIMER_MODE_WEEKLY
	else if (g_unitConfig.timerModeFrequency == TIMER_MODE_WEEKLY)
	{
		// Set the start day to the current day + 7 days
		startDay = (uint8)(currTime.day + 7);

		// Check if the start day is beyond the number of days in the current month
		if (startDay > g_monthTable[currTime.month].days)
		{
			// Subtract out the days of the current month to get the start day for next month
			startDay -= g_monthTable[currTime.month].days;
		}

		debug("Timer mode: Resetting TOD Alarm with (hour) %d, (min) %d, (start day) %d\r\n",
				g_unitConfig.timerStartTime.hour, g_unitConfig.timerStartTime.min, startDay);

		EnableExternalRtcAlarm(startDay, g_unitConfig.timerStartTime.hour, g_unitConfig.timerStartTime.min, 0);
	}
	//___________________________________________________________________________________________
	//___TIMER_MODE_MONTHLY
	else if (g_unitConfig.timerModeFrequency == TIMER_MODE_MONTHLY)
	{
		// Check if advancing a month is still within the same year
		if (currTime.month + 1 <= 12)
		{
			month = (uint8)(currTime.month + 1);
		}
		else // It's December and next month is Jan
		{
			month = 1;
		}

		// Get the current day
		startDay = currTime.day;

		// Check if the stat day is greater than the total number of days in the next month
		if (startDay > g_monthTable[month].days)
		{
			// Limit the start day to the last day of the next month
			startDay = (uint8)(g_monthTable[month].days);
		}

		debug("Timer mode: Resetting TOD Alarm with (hour) %d, (min) %d, (start day) %d\r\n",
				g_unitConfig.timerStartTime.hour, g_unitConfig.timerStartTime.min, startDay);

		EnableExternalRtcAlarm(startDay, g_unitConfig.timerStartTime.hour, g_unitConfig.timerStartTime.min, 0);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetTimeOfDayAlarmNearFuture(uint8 secondsInFuture)
{
	DATE_TIME_STRUCT currTime = GetExternalRtcTime();

	uint8 startDay = currTime.day;
	uint8 startHour = currTime.hour;
	uint8 startMin = currTime.min;
	uint8 startSec = currTime.sec;

	// Can't set alarm any further than 195 seconds into future
	if (secondsInFuture > 195) { secondsInFuture = 195; }

	// Add in seconds in future for alarm to kick
	startSec += secondsInFuture;

	// Check if seconds rolled
	if (startSec > 59)
	{
		// Set to start seconds and increment minute
		startSec = startSec - 60;
		startMin += 1;

		// Check if minutes rolled
		if (startMin > 59)
		{
			// Set to start minute and increment hour
			startMin = 0;
			startHour += 1;

			// Check if hours rolled
			if (startHour > 23)
			{
				// Set to start hour and increment day
				startHour = 0;
				startDay += 1;

				// Check if start day rolled into next month
				if (startDay > g_monthTable[currTime.month].days)
				{
					// Set to the first of the month
					startDay = 1;
				}
			}
		}
	}

	debug("Timer mode: Set TOD Alarm Near Future with (hour) %d, (min) %d, (sec) %d, (start day) %d\r\n", startHour, startMin, startSec, startDay);

	EnableExternalRtcAlarm(startDay, startHour, startMin, startSec);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 ValidateTimerModeSettings(void)
{
	DATE_TIME_STRUCT time = GetCurrentTime();

	char start_min = g_unitConfig.timerStartTime.min;
	char start_hour = g_unitConfig.timerStartTime.hour;
	char stop_min = g_unitConfig.timerStopTime.min;
	char stop_hour = g_unitConfig.timerStopTime.hour;
	char start_day = g_unitConfig.timerStartDate.day;
	char start_month = g_unitConfig.timerStartDate.month;
	char start_year = g_unitConfig.timerStartDate.year;
	char stop_day = g_unitConfig.timerStopDate.day;
	char stop_month = g_unitConfig.timerStopDate.month;
	char stop_year = g_unitConfig.timerStopDate.year;

	debug("Timer Date: (Start) %d/%d/%d -> (End) %d/%d/%d\r\n", start_month, start_day, start_year, stop_month, stop_day, stop_year);

	// Check if user picked a start date that is before the current day
	if ((start_year < time.year) ||
	((start_year == time.year) && (start_month < time.month)) ||
	((start_year == time.year) && (start_month == time.month) && (start_day < time.day)))
	{
		return (FAILED);
	}

	// Check if user picked a stop date that is before a start date
	if ((stop_year < start_year) ||
	((stop_year == start_year) && (stop_month < start_month)) ||
	((stop_year == start_year) && (stop_month == start_month) && (stop_day < start_day)))
	{
		return (FAILED);
	}

	// Check if start date and end date are the same (one-time freq)
	if ((stop_year == start_year) && (stop_month == start_month) && (stop_day == start_day))
	{
		// Check if the user specified the unit to run past midnight resulting in a second day
		if ((stop_hour < start_hour) || ((stop_hour == start_hour) && (stop_min < start_min)))
		{
			return (FAILED);
		}
	}

	// Check if start time and end time are the same
	if ((start_hour == stop_hour) && (start_min == stop_min))
	{
		return (FAILED);
	}

	// Check if start date is the current day
	if ((start_year == time.year) && (start_month == time.month) && (start_day == time.day))
	{
		// Check if the user specified the unit to run before the current time
		if ((start_hour < time.hour) || ((start_hour == time.hour) && (start_min <= time.min)))
		{
			// Check if the current time is less than the end time, signaling that timer mode is in progress
			if ((time.hour < stop_hour) || ((time.hour == stop_hour) && (time.min < stop_min)))
			{
				return (IN_PROGRESS);
			}
			else // Timer mode setting would have already completed, thus the current day is an invalid start day
			{
				return (FAILED);
			}
		}
	}

	return (PASSED);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ProcessTimerModeSettings(uint8 mode)
{
	//uint8 dayOfWeek = 0;
	uint8 startDay = 0;
	uint8 startHour = 0;
	uint16 minutesLeft = 0;
	DATE_TIME_STRUCT currentTime = GetCurrentTime();
	uint8 status = ValidateTimerModeSettings();

	// Check if the timer mode settings check failed or if the timer mode setting has been disabled
	if ((status == FAILED) || (g_unitConfig.timerMode == DISABLED))
	{
		g_unitConfig.timerMode = DISABLED;
		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		// Disable the Power Off timer in case it's set
		ClearSoftTimer(POWER_OFF_TIMER_NUM);

		if (mode == PROMPT)
		{
			sprintf((char*)g_spareBuffer, "%s %s", getLangText(TIMER_SETTINGS_INVALID_TEXT), getLangText(TIMER_MODE_DISABLED_TEXT));
			MessageBox(getLangText(ERROR_TEXT), (char*)g_spareBuffer, MB_OK);
		}
	}
	else // status == PASSED || status == IN_PROGRESS
	{
		// Calculate timer mode active run time in minutes
		TimerModeActiveMinutes();

		// Init start day based on the start date provided by the user
		startDay = g_unitConfig.timerStartDate.day;

		// Check if in progress, requiring extra logic to determine alarm settings
		if (status == IN_PROGRESS)
		{
			// Check if the stop time is greater than the start time
			if ((g_unitConfig.timerStopTime.hour > g_unitConfig.timerStartTime.hour) ||
			((g_unitConfig.timerStopTime.hour == g_unitConfig.timerStartTime.hour) &&
			(g_unitConfig.timerStopTime.min > g_unitConfig.timerStartTime.min)))
			{
				// Advance the start day
				startDay++;

				// Check if the start day is beyond the total days in the current month
				if (startDay > g_monthTable[(uint8)(g_unitConfig.timerStartDate.month)].days)
				{
					// Set the start day to the first day of next month
					startDay = 1;
				}
			}
		}

		// Check for specialty case hourly mode and in progress, requiring extra logic to determine alarm settings
		if ((g_unitConfig.timerModeFrequency == TIMER_MODE_HOURLY) && (status == IN_PROGRESS))
		{
			// Check if another hour time slot to run again today
			if (currentTime.hour != g_unitConfig.timerStopTime.hour)
			{
				// Start day remains the same
				startDay = g_unitConfig.timerStartDate.day;

				// Check if current hour time slot has not started
				if (currentTime.min < g_unitConfig.timerStartTime.min)
				{
					// Set alarm for the same hour
					startHour = currentTime.hour;
				}
				else
				{
					// Set alarm for the next hour
					startHour = currentTime.hour + 1;
					
					// Account for end of day boundary
					if (startHour > 23)
					{
						startHour = 0;

						// Advance the start day
						startDay++;

						// Check if the start day is beyond the total days in the current month
						if (startDay > g_monthTable[(uint8)(g_unitConfig.timerStartDate.month)].days)
						{
							// Set the start day to the first day of next month
							startDay = 1;
						}
					}
				}
				
				EnableExternalRtcAlarm(startDay, startHour, g_unitConfig.timerStartTime.min, 0);
			}
			else // This is the last hour time slot to run today, set alarm for next day
			{
				// startDay calculated correctly in above previous status == IN_PROGRESS logic
				EnableExternalRtcAlarm(startDay, g_unitConfig.timerStartTime.hour, g_unitConfig.timerStartTime.min, 0);
			}
		}
		else // All other timer modes
		{
			EnableExternalRtcAlarm(startDay, g_unitConfig.timerStartTime.hour, g_unitConfig.timerStartTime.min, 0);
		}

		if (status == PASSED)
		{
			if (mode == PROMPT)
			{
				sprintf((char*)g_spareBuffer, "%s %s", getLangText(TIMER_MODE_NOW_ACTIVE_TEXT), getLangText(PLEASE_POWER_OFF_UNIT_TEXT));
				MessageBox(getLangText(STATUS_TEXT), (char*)g_spareBuffer, MB_OK);
			}

			// Check if start time is greater than the current time
			if (((g_unitConfig.timerStartTime.hour * 60) + g_unitConfig.timerStartTime.min) >
			((currentTime.hour * 60) + currentTime.min))
			{
				// Take the difference between start time and current time
				minutesLeft = (uint16)(((g_unitConfig.timerStartTime.hour * 60) + g_unitConfig.timerStartTime.min) -
				((currentTime.hour * 60) + currentTime.min));
			}
			else // Current time is after the start time, meaning the start time is the next day
			{
				// Take the difference between 24 hours and the current time plus the start time
				minutesLeft = (uint16)((24 * 60) - ((currentTime.hour * 60) + currentTime.min) +
				((g_unitConfig.timerStartTime.hour * 60) + g_unitConfig.timerStartTime.min));
			}

			// Check if the start time is within the next minute
			if (minutesLeft <= 1)
			{
				OverlayMessage(getLangText(WARNING_TEXT), getLangText(POWERING_UNIT_OFF_NOW_TEXT), 2 * SOFT_SECS);

				// Need to shutdown the unit now, otherwise the start time window will be missed
				PowerUnitOff(SHUTDOWN_UNIT); // Return unnecessary
			}
			else // More than 1 minute left before the start time
			{
				// Make sure the unit turns off one minute before the start time if the user forgets to turn the unit off
				minutesLeft -= 1;

				// Need to handle state where timer mode is going active but unit hasn't power cycled into timer mode yet
				g_allowQuickPowerOffForTimerModeSetup = YES;

				// Set the Power off soft timer to prevent the unit from staying on past the Timer mode start time
				AssignSoftTimer(POWER_OFF_TIMER_MODE_NUM, (uint32)(minutesLeft * 60 * 2), PowerOffTimerModeCallback);
			}
		}
		else // status == IN_PROGRESS
		{
			if (mode == PROMPT)
			{
				sprintf((char*)g_spareBuffer, "%s", getLangText(TIMER_MODE_NOW_ACTIVE_TEXT));
				MessageBox(getLangText(STATUS_TEXT), (char*)g_spareBuffer, MB_OK);
			}

			// Check if specialty mode hourly
			if (g_unitConfig.timerModeFrequency == TIMER_MODE_HOURLY)
			{
				if (currentTime.min	< g_unitConfig.timerStopTime.min)
				{
					minutesLeft = (g_unitConfig.timerStopTime.min - currentTime.min);
				}
				else
				{
					minutesLeft = (60 + g_unitConfig.timerStopTime.min - currentTime.min);
				}
				
				if (minutesLeft > 58)
				minutesLeft = 58;
			}
			// Check if the current time is greater than the stop time, indicating that midnight boundary was crossed
			else if (((currentTime.hour * 60) + currentTime.min) > ((g_unitConfig.timerStopTime.hour * 60) + g_unitConfig.timerStopTime.min))
			{
				// Calculate the time left before powering off to be 24 + the stop time minus the current time
				minutesLeft = (uint16)(((24 * 60) + (g_unitConfig.timerStopTime.hour * 60) + g_unitConfig.timerStopTime.min) -
				((currentTime.hour * 60) + currentTime.min));
			}
			else // Current time is less than start time, operating within the same day
			{
				// Calculate the time left before powering off to be the stop time minus the current time
				minutesLeft = (uint16)(((g_unitConfig.timerStopTime.hour * 60) + g_unitConfig.timerStopTime.min) -
				((currentTime.hour * 60) + currentTime.min));
			}

			// Make sure timeout value is not zero
			if (minutesLeft == 0) minutesLeft = 1;

			debug("Timer Mode: In progress, minutes left before power off: %d (Expired secs this min: %d)\r\n", minutesLeft, currentTime.sec);

			// Setup soft timer to turn system off when timer mode is finished for the day
			AssignSoftTimer(POWER_OFF_TIMER_MODE_NUM, (uint32)((minutesLeft * 60 * 2) - (currentTime.sec	* 2)), PowerOffTimerModeCallback);
		}
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TimerModeActiveMinutes(void)
{
	// Find the Time mode active time period in minutes

	// Check for specialty case hourly
	if (g_unitConfig.timerModeFrequency == TIMER_MODE_HOURLY)
	{
		// Check if the stop min is greater than the start min
		if (g_unitConfig.timerStopTime.min > g_unitConfig.timerStartTime.min)
		{
			g_unitConfig.TimerModeActiveMinutes = (g_unitConfig.timerStopTime.min - g_unitConfig.timerStartTime.min);
		}
		else // The timer mode hourly active period will cross the hour boundary
		{
			g_unitConfig.TimerModeActiveMinutes = (60 + g_unitConfig.timerStopTime.min - g_unitConfig.timerStartTime.min);
		}
		
		// In order to restart up every hour, the active minutes needs to be less than 60, 1 min for shutdown + 1 min for being off
		if (g_unitConfig.TimerModeActiveMinutes > 58)
		g_unitConfig.TimerModeActiveMinutes = 58;
	}
	// Check if the stop time (in minutes resolution) is greater than the start time, thus running the same day
	else if (((g_unitConfig.timerStopTime.hour * 60) + g_unitConfig.timerStopTime.min) >
	((g_unitConfig.timerStartTime.hour * 60) + g_unitConfig.timerStartTime.min))
	{
		// Set active minutes as the difference in the stop and start times
		g_unitConfig.TimerModeActiveMinutes = (uint16)(((g_unitConfig.timerStopTime.hour * 60) + g_unitConfig.timerStopTime.min) -
		((g_unitConfig.timerStartTime.hour * 60) + g_unitConfig.timerStartTime.min));

		// Check for specialty case one time
		if (g_unitConfig.timerModeFrequency == TIMER_MODE_ONE_TIME)
		{
			// Calculate the number of days to run consecutively (Days * Hours in a day * Minutes in an hour) and add to the active minutes
			g_unitConfig.TimerModeActiveMinutes += (24 * 60 * (GetTotalDaysFromReference(g_unitConfig.timerStopDate) -
			GetTotalDaysFromReference(g_unitConfig.timerStartDate)));
		}
	}
	else // The timer mode active period will see midnight, thus running 2 consecutive days
	{
		// Set active minutes as the difference from midnight and the start time, plus the stop time the next day
		g_unitConfig.TimerModeActiveMinutes = (uint16)((24 * 60) - ((g_unitConfig.timerStartTime.hour * 60) + g_unitConfig.timerStartTime.min) +
		((g_unitConfig.timerStopTime.hour * 60) + g_unitConfig.timerStopTime.min));

		// Check for specialty case one time
		if (g_unitConfig.timerModeFrequency == TIMER_MODE_ONE_TIME)
		{
			// Calculate the number of days to run consecutively (Days * Hours in a day * Minutes in an hour) minus crossover day and add to the active minutes
			g_unitConfig.TimerModeActiveMinutes += (24 * 60 * (GetTotalDaysFromReference(g_unitConfig.timerStopDate) -
			GetTotalDaysFromReference(g_unitConfig.timerStartDate) - 1));
		}
	}

	debug("Timer Active Minutes: %d\r\n", g_unitConfig.TimerModeActiveMinutes);

	SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);
}
