///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

#ifndef _KEYPAD_CMMN_H_
#define _KEYPAD_CMMN_H_

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include "Typedefs.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define NUM_KEYPAD_ROWS			8
#define NUM_KEYPAD_COLUMNS		8
#define TOTAL_KEYPAD_KEYS		8

// Special Char
#define KEY_ENTER			0X8c
#define KEY_MINUS			0x95
#define KEY_PLUS			0x96
#define KEY_DOWNARROW		0x8a
#define KEY_UPARROW			0x8b
#define KEY_ESCAPE			0X1b
#define KEY_BACKLIGHT		0X86
#define KEY_HELP			0x94
#define KEY_SHIFT			0x90
#define KEY_RETURN			0X8c
#define KEY_DELETE			0x93
#define KEY_PAPERFEED		0x97
#define KEY_CTRL			0x91
#define KEY_NONE			0X00

// Number keys (Ascii hex values)
#define ZERO_KEY		0x30 // '0'
#define ONE_KEY 		0x31 // '1'
#define TWO_KEY 		0x32 // '2'
#define THREE_KEY 		0x33 // '3'
#define FOUR_KEY 		0x34 // '4'
#define FIVE_KEY 		0x35 // '5'
#define SIX_KEY 		0x36 // '6'
#define SEVEN_KEY 		0x37 // '7'
#define EIGHT_KEY 		0x38 // '8'
#define NINE_KEY 		0x39 // '9'

// Arrow keys
#define DOWN_ARROW_KEY	KEY_DOWNARROW
#define UP_ARROW_KEY 	KEY_UPARROW
#define MINUS_KEY 		KEY_MINUS
#define PLUS_KEY 		KEY_PLUS

// Other keys
#define ENTER_KEY 		KEY_RETURN
#define ESC_KEY 		KEY_ESCAPE
#define HELP_KEY 		KEY_HELP
#define DELETE_KEY 		KEY_DELETE
#define SHIFT_KEY		KEY_SHIFT
#define CTRL_KEY 		KEY_CTRL
#define ON_ESC_KEY		0xAA	// Key defines a mess, just making a unique value

// Power Keys
#define ON_KEY			0x04
#define OFF_KEY			0x02

#define WAIT_FOR_KEY			0
#define CHECK_ONCE_FOR_KEY		1

enum {
	SEQ_NOT_STARTED = 0,
	STAGE_1,
	STAGE_2,
	ENTER_FACTORY_SETUP,
	PROCESS_FACTORY_SETUP,
	TOTAL_FACTORY_SETUP_STATES
};

enum {
	KEY_SOURCE_IRQ,
	KEY_SOURCE_TIMER
};

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------
BOOLEAN KeypadProcessing(uint8 keySource);
void KeypressEventMgr(void);
uint8 GetShiftChar(uint8 inputChar);
uint8 HandleCtrlKeyCombination(uint8 inputChar);
uint8 GetKeypadKey(uint8 mode);
uint8 ScanKeypad(void);

#endif /* _KEYPAD_CMMN_H_ */

