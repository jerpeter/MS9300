///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

#ifndef _SOFT_TIMER_H_
#define _SOFT_TIMER_H_ 

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include "Typedefs.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
enum {
	TIMER_DISABLED,
	TIMER_ENABLED
};

enum {
	LCD_BACKLIGHT_ON_OFF_TIMER_NUM = 0,
	LCD_POWER_ON_OFF_TIMER_NUM,
	AUTO_MONITOR_TIMER_NUM,
	MENU_UPDATE_TIMER_NUM,
	ALARM_ONE_OUTPUT_TIMER_NUM,
	ALARM_TWO_OUTPUT_TIMER_NUM,
	POWER_OFF_TIMER_NUM,
	MODEM_DELAY_TIMER_NUM,
	MODEM_RESET_TIMER_NUM,
	KEYPAD_LED_TIMER_NUM,
	GPS_POWER_OFF_TIMER_NUM,
	GPS_POWER_ON_TIMER_NUM,
	LOOSE_EVENT_MIGRATION_TIMER_NUM,
	AUTO_DIAL_OUT_CYCLE_TIMER_NUM,
	// Add new timers here
	NUM_OF_SOFT_TIMERS
};

#define DEACTIVATED		0
#define ACTIVATED		1

#define TICKS_PER_SEC 	2 // Number of ticks per second
#define TICKS_PER_MIN	(TICKS_PER_SEC * 60) // Number of ticks per minute
#define TICKS_PER_HOUR	(TICKS_PER_MIN * 60) // Number of ticks per hour

#define ONE_SECOND_TIMEOUT		1 * TICKS_PER_SEC // secs * ticks

#define TIMEOUT_DISABLED		0 // secs
#define LCD_BACKLIGHT_TIMEOUT	60 * TICKS_PER_SEC // secs * ticks
#define LCD_POWER_TIMEOUT		120 * TICKS_PER_SEC // secs * ticks
#define MODEM_ATZ_DELAY			15 * TICKS_PER_SEC
#define MODEM_ATZ_QUICK_DELAY	2 * TICKS_PER_SEC

typedef struct
{
	uint32 	state;			// The timer is in use or not
	uint32	tickStart;		// Tick value when the timer was started
	uint32 	timePeriod;		// The time period to wait, number of ticks
	uint32	timeoutValue;	// Added to hold the value for a reset of the timeout period
	void (*callback)(void);

} SOFT_TIMER_STRUCT;

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------
void AssignSoftTimer(uint16 timerNum, uint32 timeout, void* callback);
void ResetSoftTimer(uint16 timerNum);
void ClearSoftTimer(uint16 timerNum);
void CheckSoftTimers(void);
void ProcessTimerMode(void);
BOOLEAN TimerModeActiveCheck(void);
uint8 IsSoftTimerActive(uint16);
void ResetTimeOfDayAlarm(void);
void SetTimeOfDayAlarmNearFuture(uint8 secondsInFuture);
void HandleUserPowerOffDuringTimerMode(void);
void DisplayTimerCallBack(void);
void LcdPwTimerCallBack(void);
void AutoDialOutCycleTimerCallBack(void);
void AutoMonitorTimerCallBack(void);
void MenuUpdateTimerCallBack(void);
void KeypadLedUpdateTimerCallBack(void);
void AlarmOneOutputTimerCallback(void);
void AlarmTwoOutputTimerCallback(void);
void PowerOffTimerCallback(void);
void ModemDelayTimerCallback(void);
void ModemResetTimerCallback(void);
void GpsPowerOffTimerCallBack(void);
void GpsPowerOnTimerCallBack(void);
void LooseEventMigrationTimerCallBack(void);

#endif // _SOFT_TIMER_H_
