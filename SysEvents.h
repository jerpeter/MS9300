///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

#ifndef _SYS_EVENT_H_
#define _SYS_EVENT_H_

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include "Typedefs.h"
//#include "compiler.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
typedef struct
{
	uint16 wrd;
} SYS_EVENT_STRUCT;

typedef struct
{
	uint16 wrd;
} MN_TIMER_STRUCT;

typedef struct
{
	uint16 wrd;
} MN_EVENT_STRUCT;

// System Events
#define TRIGGER_EVENT				0x0001
#define BARGRAPH_EVENT				0x0002
#define UPDATE_OFFSET_EVENT			0x0004
#define LOW_BATTERY_WARNING_EVENT	0x0008
#define KEYPAD_EVENT				0x0010
#define CYCLIC_EVENT				0x0020
#define CYCLE_CHANGE_EVENT			0x0040
#define CAL_EVENT					0x0080
#define AUTO_DIALOUT_EVENT			0x0100
#define WARNING_EVENT 				0x0200
#define UNUSED_EVENT				0x0400
#define MANUAL_CAL_EVENT 			0x0800
#define UPDATE_TIME_EVENT			0x1000
#define UPDATE_MENU_EVENT			0x2000
#define CRAFT_PORT_EVENT			0x4000
#define EXT_TRIGGER_EVENT			0x8000
#define ANY_SYSTEM_EVENT			0xFFFF
#define NO_SYSTEM_EVENT_ACTIVE		0x0000

// Timer Events
#define TIMER_MODE_TIMER_EVENT		0x0080
#define SOFT_TIMER_CHECK_EVENT		0x0100
#define ANY_TIMER_EVENT				0xFFFF

// Menu Events
#define RESULTS_MENU_EVENT			0x0020
#define ANY_MENU_EVENT				0xFFFF

// System Event Macros
#if 0 /* old hw */
#define raiseSystemEventFlag(x)		Disable_global_interrupt(); g_systemEventFlags.wrd |= x; Enable_global_interrupt();
#define clearSystemEventFlag(x)		Disable_global_interrupt(); g_systemEventFlags.wrd &= ~x; Enable_global_interrupt();
#else
#define raiseSystemEventFlag(x)		g_systemEventFlags.wrd |= x;
#define clearSystemEventFlag(x)		g_systemEventFlags.wrd &= ~x;
#endif
#define raiseSystemEventFlag_ISR(x)	(g_systemEventFlags.wrd |= x)
#define clearSystemEventFlag_ISR(x)	(g_systemEventFlags.wrd &= ~x)
#define getSystemEventState(x)		(g_systemEventFlags.wrd & x)
#define anySystemEventExcept(x)		(g_systemEventFlags.wrd & ~x)

// Timer Event Macros
#if 0 /* old hw */
#define raiseTimerEventFlag(x)		Disable_global_interrupt(); g_timerEventFlags.wrd |= x; Enable_global_interrupt();
#define clearTimerEventFlag(x)		Disable_global_interrupt(); g_timerEventFlags.wrd &= ~x; Enable_global_interrupt();
#else
#define raiseTimerEventFlag(x)		g_timerEventFlags.wrd |= x;
#define clearTimerEventFlag(x)		g_timerEventFlags.wrd &= ~x;
#endif
#define raiseTimerEventFlag_ISR(x)	(g_timerEventFlags.wrd |= x)
#define clearTimerEventFlag_ISR(x)	(g_timerEventFlags.wrd &= ~x)
#define getTimerEventState(x)		(g_timerEventFlags.wrd & x)

// Menu Event Macros
#if 0 /* old hw */
#define raiseMenuEventFlag(x)		Disable_global_interrupt(); g_menuEventFlags.wrd |= x; Enable_global_interrupt();
#define clearMenuEventFlag(x)		Disable_global_interrupt(); g_menuEventFlags.wrd &= ~x; Enable_global_interrupt();
#else
#define raiseMenuEventFlag(x)		g_menuEventFlags.wrd |= x;
#define clearMenuEventFlag(x)		g_menuEventFlags.wrd &= ~x;
#endif
#define raiseMenuEventFlag_ISR(x)	(g_menuEventFlags.wrd |= x)
#define clearMenuEventFlag_ISR(x)	(g_menuEventFlags.wrd &= ~x)
#define getMenuEventState(x)		(g_menuEventFlags.wrd & x)

// Processing states
enum {
	IDLE_STATE = 0,
	ACTIVE_STATE
};

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------

#endif // _SYS_EVENT_H_
