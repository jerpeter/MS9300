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
#define TOTAL_KEYPAD_KEYS		9

/*
-= Button map translation =-
Button 9 = Up
Button 8 = Down
Button 7 = Left
Button 6 = Right
Button 5 = Enter
Button 4 = Soft key 1
Button 3 = Soft key 2
Button 2 = Soft key 3
Button 1 = Soft key 4
*/

// Keyboard button raw mask in GPIO1 register
#define KB_9	0x01000000
#define KB_8	0x00800000
#define KB_7	0x00400000
#define KB_6	0x00200000
#define KB_5	0x00100000
#define KB_4	0x00080000
#define KB_3	0x00040000
#define KB_2	0x00020000
#define KB_1	0x00010000

// Keyboard button mask full shifted right
#define KB_UP		0x100
#define KB_DOWN		0x080
#define KB_LEFT		0x040
#define KB_RIGHT	0x020
#define KB_ENTER	0x010
#define KB_SK_1		0x008
#define KB_SK_2		0x004
#define KB_SK_3		0x002
#define KB_SK_4		0x001
#define KEY_NONE	0X000

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
#define DOWN_ARROW_KEY	KB_DOWN
#define UP_ARROW_KEY 	KB_UP
#define LEFT_ARROW_KEY 	KB_LEFT
#define RIGHT_ARROW_KEY KB_RIGHT

// Other keys
#define ENTER_KEY 		KB_ENTER
#define ESC_KEY 		KB_SK_1
#define HELP_KEY 		KB_SK_4
#define DELETE_KEY 		KB_SK_3
#define ON_ESC_KEY		0x200	// Make a unique value from the other keys

#define WAIT_FOR_KEY			0
#define CHECK_ONCE_FOR_KEY		1

// New buttons
#define BUTTON_GPIO_MASK	0x1FF0000

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
uint16 HandleCtrlKeyCombination(uint16 inputChar);
uint16 GetKeypadKey(uint8 mode);
uint16 ScanKeypad(void);

#endif /* _KEYPAD_CMMN_H_ */

