///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

#ifndef _DISPLAY_H_
#define _DISPLAY_H_

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include "Typedefs.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------

// LCD to Processor pins
// ---------------------
// CS1 (active low)..........PD0 (Port D)
// CS2 (active low)..........PD1 (Port D)
// Reset (active low)........PD2 (Port D)
// Data/Inst (active low)....PD3 (Port D)
// Read/Write (active low)...PD4 (Port D)
// Enable....................PD5 (Port D)
// No Connect................PD6 (Port D)
// No Connect................PD7 (Port D)
//
// LD0.......................PD8 (Port C)
// LD1.......................PD9 (Port C)
// LD2.......................PD10 (Port C)
// LD3.......................PD11 (Port C)
// LD4.......................PD12 (Port C)
// LD5.......................PD13 (Port C)
// LD6.......................PD14 (Port C)
// LD7.......................PD15 (Port C)

// LCD Control bit defines
#define LCD_CS1_BIT					0x0800 //0x01
#define LCD_CS2_BIT					0x1000 //0x02
#define LCD_RESET_BIT				0x2000 //0x04
#define LCD_RS_BIT					0x0100 //0x08
#define LCD_READ_WRITE_BIT			0x0200 //0x10
#define LCD_ENABLE_BIT				0x0400 //0x20

// Helper control bit defines
#define LCD_INSTRUCTION_BIT			0x00 // Same as LCD_RS_BIT
#define LCD_WRITE_BIT				0x00 // Same as LCD_READ_WRITE_BIT
#define LCD_CS1_ENABLE_BITS			0x02 // CS1 active low, set CS2 high
#define LCD_CS2_ENABLE_BITS			0x01 // CS2 active low, set CS1 high

// LCD Status flags
#define LCD_BUSY_FLAG			0x80
#define LCD_OFF_FLAG			0x20
#define LCD_RESET_FLAG			0x10

// LCD Instruction defines
#define LCD_DISPLAY_ON_INSTRUCTION		0x3F
#define LCD_DISPLAY_OFF_INSTRUCTION		0x3E
#define LCD_SET_Y_ADDRESS				0x40 // Lower 6 bits are Y address, 0 - 63
#define LCD_SET_ADDRESS					0x40 // Lower 6 bits are address, 0 - 63
#define LCD_SET_X_ADDRESS				0xB8 // Lower 3 bits are X address, 0 - 7
#define LCD_SET_PAGE					0xB8 // Lower 6 bits are Page address, 0 - 7
#define LCD_START_LINE_INSTRUCTION		0xC0 // Lower 6 bits are start line, 0 - 63

// Contrast defines
#define DEFUALT_CONTRAST		34
#define MAX_CONTRAST			50
#define MIN_CONTRAST			20
#define DEFAULT_MAX_CONTRAST	63
#define DEFAULT_MIN_CONTRAST	0
#define CONTRAST_STEPPING 		1
#define CONTRAST_FINE_STEPPING 	1

#define LCD_NUM_OF_ROWS			8
#define LCD_NUM_OF_BIT_COLUMNS	128
#define MMAP_SIZE_IN_BYTES		(LCD_NUM_OF_ROWS * LCD_NUM_OF_BIT_COLUMNS)

#define SEGMENT_ONE_BLOCK_BORDER 		63
#define SEGMENT_TWO_BLOCK_BORDER 		127

#define LCD_ACCESS_DELAY 				500 // usecs

enum {
	LCD_DISPLAY_ON,
	LCD_DISPLAY_OFF
};
 
enum {
	LCD_INSTRUCTION,
	LCD_DATA
};

enum { 
	LCD_SEGMENT1,
	LCD_SEGMENT2
};

typedef enum
{
	BACKLIGHT_OFF,
	BACKLIGHT_DIM,
	BACKLIGHT_BRIGHT
} LCD_BACKLIGHT_STATES;

typedef enum
{
	LIGHTER,
	DARKER
} CONTRAST_ADJUSTMENT;

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------
void WaitForLcdReady (uint8);
void LcdResetPulse(void);
uint8 ReadLcdData (uint8);
void WriteLcdData (uint8,uint8);
void LcdClearPortReg(void);
void SetLcdStartLine (uint8, uint8);
void SetLcdXPosition (uint8, uint8);
void SetLcdYPosition (uint8, uint8);
void SetLcdMode (uint8, uint8);
void SetLcdOrigin (uint8,uint8,uint8);
uint8 ClockDataFromLcd(uint8);
void ClockDataToLcd(uint8, uint8);
void WriteStringToLcd (uint8*, uint8, uint8, uint8 (*table_ptr)[2][10]);
void WriteMapToLcd (uint8 (*g_mmap_ptr)[128]);
void InitLcdDisplay(void);
void ClearLcdDisplay(void);
void FillLcdDisplay(void);
void ClearControlLinesLcdDisplay(void);
void SetNextLcdBacklightState(void);
void SetLcdBacklightState(LCD_BACKLIGHT_STATES);
LCD_BACKLIGHT_STATES GetLcdBacklightState(void);
void AdjustLcdContrast(CONTRAST_ADJUSTMENT);
void SetLcdContrast(uint8);
void LcdInit(void);
void LcdWrite(uint8 mode, uint8 data, uint8 segment);
uint8 LcdRead(uint8 mode, uint8 segment);
void TurnDisplayOff(void);
void ActivateDisplayShortDuration(uint16 secondsToDisplay);

#endif // _DISPLAY_H_
