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
#include "Typedefs.h"
#include "Display.h"
#include "Board.h"
#include "Common.h"
#include "PowerManagement.h"
#include "OldUart.h"
#include "Record.h"
#include "lcd.h"

#include "mxc_delay.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define DEFAULT_START_LINE	0
#define DEFAULT_X_LOC		0
#define	DEFAULT_Y_LOC		0
#define EXCLUDE_LCD_DRIVER_FUNCTIONS	0

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
void LcdInit(void)
{
	// No special init
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void LcdResetPulse(void)
{
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void LcdClearPortReg(void)
{
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void LcdWrite(uint8 mode, uint8 data, uint8 segment)
{
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if EXCLUDE_LCD_DRIVER_FUNCTIONS
uint8 LcdRead(uint8 mode, uint8 segment)
{
	uint8 data = 0x00;

	return (data);
}
#endif

#if EXCLUDE_LCD_DRIVER_FUNCTIONS
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
inline uint8 ClockDataFromLcd(uint8 lcdCmd)
{
	// Take Eanble high
	*((volatile uint8*)LCD_CMD_PORT) = lcdCmd;

	// Take Eanble low which will clock in the LCD data
	*((volatile uint8*)LCD_CMD_PORT) = (uint8)(lcdCmd & ~(LCD_ENABLE));

	// Read out LCD data
	return (*((volatile uint8*)LCD_DATA_PORT));
}
#endif

#if EXCLUDE_LCD_DRIVER_FUNCTIONS
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
inline void ClockDataToLcd(uint8 lcdCmd, uint8 lcdData)
{
	// Take Eanble high
	*((volatile uint8*)LCD_CMD_PORT) = lcdCmd;

	// Write in LCD data
	*((volatile uint8*)LCD_DATA_PORT) = lcdData;

	// Take Eanble low which will clock in the LCD data
	*((volatile uint8*)LCD_CMD_PORT) = (uint8)(lcdCmd & ~(LCD_ENABLE));
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
inline void WaitForLcdReady(uint8 segment)
{
	UNUSED(segment);
	// Wait for Lcd busy flag to clear
	//while (LcdRead(LCD_INSTRUCTION, segment) & LCD_BUSY_FLAG) {};
}

#if EXCLUDE_LCD_DRIVER_FUNCTIONS
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
inline uint8 ReadLcdData(uint8 segment)
{
	return (LcdRead(LCD_DATA, segment));
}
#endif

#if EXCLUDE_LCD_DRIVER_FUNCTIONS
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
inline void WriteLcdData(uint8 lcdData, uint8 segment)
{
	LcdWrite(LCD_DATA, lcdData, segment);
	
	WaitForLcdReady(segment);
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
inline void SetLcdStartLine(uint8 lcdData, uint8 segment)
{
	// Or in Start Line instruction to LCD data
	lcdData |= (uint8)(LCD_START_LINE_INSTRUCTION);
	
	LcdWrite(LCD_INSTRUCTION, lcdData, segment);
	
	WaitForLcdReady(segment);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
inline void SetLcdXPosition(uint8 lcdData, uint8 segment)
{
	// Or in X location instruction to the row selection in LCD data
	lcdData |= LCD_SET_PAGE;

	LcdWrite(LCD_INSTRUCTION, lcdData, segment);
		
	WaitForLcdReady(segment);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
inline void SetLcdYPosition(uint8 lcdData, uint8 segment)
{
	// Or in Y location instruction to the column slection in LCD data
	lcdData |= LCD_SET_ADDRESS;
	
	LcdWrite(LCD_INSTRUCTION, lcdData, segment);

	WaitForLcdReady(segment);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
inline void SetLcdMode(uint8 lcdData, uint8 segment)
{
	// Check if LCD data is set for turining on or off display
	if (lcdData == LCD_DISPLAY_ON)
	{
		// Write in Turn on Display command
		lcdData = (uint8)(LCD_DISPLAY_ON_INSTRUCTION);
	}
	else
	{ 
		// Write in Turn off Display command
		lcdData = (uint8)(LCD_DISPLAY_OFF_INSTRUCTION);
	}

	LcdWrite(LCD_INSTRUCTION, lcdData, segment);
	
	WaitForLcdReady(segment);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
inline void SetLcdOrigin(uint8 x, uint8 y, uint8 segment)
{
	// Set both X and Y coordinate positions
	SetLcdXPosition(x, segment);
	SetLcdYPosition(y, segment);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void WriteStringToLcd(uint8* p, uint8 x, uint8 y, uint8 (*table_ptr)[2][10])
{
	// =========================================
	// Warning: This routine has not been tested
	// =========================================
	
	uint8* font_char_ptr;
	uint8 font_size;
	uint8 segment;
	uint8 pixel_byte;

	// Check if the LCD has power. If not, can't get status, so return
	if (GetPowerControlState(LCD_POWER_ENABLE) == OFF)
	{
		return;
	}

	/* THIS ROUTINE WAS FOR TESTING THE LCD ONLY */	
	segment = LCD_SEGMENT1;
	SetLcdXPosition(x, segment);
	SetLcdYPosition(y, segment);

	while (*p != '\0')
	{
		font_char_ptr = table_ptr[(*p)][0];	
		font_size = 6;
		pixel_byte = 0;

		while (pixel_byte<font_size)
		{
			if (segment == LCD_SEGMENT1)
			{
				if ((y + pixel_byte) > SEGMENT_ONE_BLOCK_BORDER) /*NOTE: BORDER is 63 its zero based 0 - 63 */
				{
					segment = LCD_SEGMENT2;
					SetLcdXPosition(x, segment);

					SetLcdYPosition((uint8)(y - 64), segment);
				}
			}
			else
			{
				if ((y + pixel_byte) > SEGMENT_TWO_BLOCK_BORDER) /* NOTE SEG2 BORDER IS 127 0 - 127 */
				{
					return; // NO WORD WRAP
				}
			}

			//WriteLcdData(*(font_char_ptr + pixel_byte),segment);
			LcdWrite(LCD_DATA, *(font_char_ptr + pixel_byte), segment);
			WaitForLcdReady(segment);

			pixel_byte++;
		}
		
		y = (uint8)(y + font_size);
		p++;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* Empty call until LCD connector fixed or hardware modded */
void WriteMapToLcd(uint8 (*g_mmap_ptr)[128]) {}
#else
void WriteMapToLcd(uint8 (*g_mmap_ptr)[128])
{
#if 0 /* original function */
	uint8 segment;
	uint8* pixel_byte_ptr;
	uint8 col_index;
	uint8 row_index;

	// Check if the LCD has power. If not, can't get status, so return
	if (GetPowerControlState(LCD_POWER_ENABLE) == OFF)
	{
		return;
	}

	row_index = 0;

	while (row_index < 8)
	{
		col_index = 0;
		pixel_byte_ptr = (uint8*)g_mmap_ptr[row_index];

		segment = LCD_SEGMENT1;
		SetLcdOrigin(row_index, DEFAULT_Y_LOC, segment);

		while (col_index <= SEGMENT_ONE_BLOCK_BORDER)
		{
			// Normal command replaced by code below
			//WriteLcdData(*(pixel_byte_ptr + col_index),segment);

			// Optimized WriteLcdData operation for speed
			//ClockDataToLcd(lcdCmdSeg1, *(pixel_byte_ptr + col_index));
			//while (ClockDataFromLcd(lcdStatusSeg1) & LCD_BUSY_FLAG) {};
			// End of optimized WriteLcdData operation

			LcdWrite(LCD_DATA, *(pixel_byte_ptr + col_index), segment);
			WaitForLcdReady(segment);

			col_index++;
		}
		
		segment = LCD_SEGMENT2;
		SetLcdOrigin(row_index, DEFAULT_Y_LOC, segment);

		while (col_index <= SEGMENT_TWO_BLOCK_BORDER)
		{
			// Normal command replaced by code below
			//WriteLcdData(*(pixel_byte_ptr + col_index),segment);

			// Optimized WriteLcdData operation for speed
			//ClockDataToLcd(lcdCmdSeg2, *(pixel_byte_ptr + col_index));
			//while (ClockDataFromLcd(lcdStatusSeg2) & LCD_BUSY_FLAG) {};
			// End of optimized WriteLcdData operation

			LcdWrite(LCD_DATA, *(pixel_byte_ptr + col_index), segment);
			WaitForLcdReady(segment);

			col_index++;
		}

		row_index++;
	}
#else /* new LCd driver */
	/*
		----------------------------
		Write map to LCD replacement
		----------------------------
		Add soft key buttons and dynamic labels

		End the display list -- ft81x_display();
		Trigger FT81x to read the command buffer -- ft81x_getfree(0);
		Finish streaming to command buffer -- ft81x_stream_stop();

		Wait till the GPU is finished?? -- ft81x_wait_finish();
	*/

	/*
		----------------------------
		Soft Buttons
		----------------------------
		Width: 70 for small, 120 for large
		Heigth: 32 for either

		Small button start Y position: 240
		Small button start X position:
		1st start: 12
		2nd start: 140
		3rd start: 264
		4th start: 394
	*/
	// Option parameter: By default, the button is drawn with a 3D effect (value is zero), OPT_FLAT removes the 3D effect (value of OPT_FLAT is 256)
	// Todo: Load the dynamic key label

	//if(LcdControllerActive())
	if (g_lcdPowerFlag == DISABLED) { return; }

	// Swap to white text
	ft81x_color_rgb32(0xffffff);

	ft81x_cmd_button(12, 420, 132, 55, 29, 0, "LCD OFF");
	ft81x_cmd_button(225, 420, 132, 55, 29, 0, "BACKLIGHT");
	ft81x_cmd_button(432, 420, 132, 55, 29, 0, "CONFIG");
	ft81x_cmd_button(650, 420, 132, 55, 29, 0, "ESCAPE");

	// Swap back to default blue text
	ft81x_color_rgb32(0x0000ff);

#if 1 /* Test display of Battery information */
	char debugInfo[32];
	sprintf(debugInfo, "Battery: %0.3fV", (double)((float)FuelGaugeGetVoltage() / 1000));
	ft81x_cmd_text(580, 40, 28, 0, debugInfo);

	sprintf(debugInfo, "Current: %0.3fmA", (double)((float)FuelGaugeGetCurrent() / 1000));
	if (FuelGaugeGetCurrent() > 0) { ft81x_color_rgb32(0x068c3b); ft81x_cmd_text(580, 60, 28, 0, debugInfo); ft81x_color_rgb32(0x0000ff); }
	else { ft81x_cmd_text(580, 60, 28, 0, debugInfo); }

	sprintf(debugInfo, "Temperature: %dF", FuelGaugeGetTemperature());
	ft81x_cmd_text(580, 80, 28, 0, debugInfo);
#endif

	ft81x_display(); // End the display list started with the ClearLcdMap function
	ft81x_getfree(0); // Trigger FT81x to read the command buffer
	ft81x_stream_stop(); // Finish streaming to command buffer

	ft81x_wait_finish(); // Wait till the GPU is finished? (or delay at start of next display interaction?)
#endif

#if 0 /* Test disable of LCD for now */
	MXC_Delay(MXC_DELAY_SEC(3));
	PowerControl(LCD_POWER_ENABLE, OFF);
	PowerControl(LCD_POWER_DOWN, ON);
#endif
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* Empty call until LCD connector fixed or hardware modded */
void ClearLcdMap(void) {}
#else
void ClearLcdMap(void)
{
#if 0 /* original function */
	memset(&(g_mmap[0][0]), 0, sizeof(g_mmap));
#else /* new LCD driver */
	/*
		----------------------------
		New LCD display steps, LCD map clear replacement
		----------------------------
		Start stream -- ft81x_stream_start();
		Start new display list and swap over -- ft81x_cmd_dlstart(); and ft81x_cmd_swap();
		Set color -- ft81x_color_rgb32();
		Set foreground color -- ft81x_fgcolor_rgb32();
		Set background color -- ft81x_bgcolor_rgb32();
	*/
#if 0 /* Test method to re-power the LCD */
		if (GetPowerControlState(LCD_POWER_ENABLE) == OFF) { ft81x_init(); }
#else
		if (g_lcdPowerFlag == DISABLED) { return; }
#endif
		ft81x_stream_start(); // Start streaming
		ft81x_cmd_dlstart(); // Set REG_CMD_DL when done?
		ft81x_cmd_swap(); // Set AUTO swap at end of display list?
		ft81x_clear_color_rgb32(0xfdfdfd); // Todo: Determine color? (datasheet example shows this order, clear color before clear)
		ft81x_clear();
		ft81x_color_rgb32(0x101010); // Todo: Determine palatte color
		ft81x_bgcolor_rgb32(0xff0000); // Todo: Determine background color of graphics objects
		ft81x_fgcolor_rgb32(0x0000ff); // Todo: Determine foreground color of graphics objects

		ft81x_tag_mask(0); // Turn off tagging

		// Todo: clear soft key map until the new screen has been written (changed)
		//memset(&g_softKeyTranslation[0], 0, sizeof(g_softKeyTranslation));
#endif
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitLcdDisplay(void)
{
	// Issue reset pulse to LCD display
	LcdResetPulse();
	
	// Make sure LCD is ready for commands
	WaitForLcdReady(LCD_SEGMENT1);

	// Init LCD segment 1
	SetLcdMode(LCD_DISPLAY_ON, LCD_SEGMENT1);
	SetLcdStartLine(DEFAULT_START_LINE, LCD_SEGMENT1);
	SetLcdOrigin(DEFAULT_X_LOC, DEFAULT_Y_LOC, LCD_SEGMENT1);

	// Init LCD segment 2
	SetLcdMode(LCD_DISPLAY_ON, LCD_SEGMENT2);
	SetLcdStartLine(DEFAULT_START_LINE, LCD_SEGMENT2);
	SetLcdOrigin(DEFAULT_X_LOC, DEFAULT_Y_LOC, LCD_SEGMENT2);

	// Clear the LCD display
	ClearLcdDisplay();
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ClearControlLinesLcdDisplay(void)
{
	LcdWrite(LCD_INSTRUCTION, 0x00, LCD_SEGMENT1);
	LcdWrite(LCD_DATA, 0x00, LCD_SEGMENT1);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ClearLcdDisplay(void)
{
	// Turn all of the LCD pixels off (0's), effectively clearing the display
	memset(&(g_mmap[0][0]), 0, sizeof(g_mmap));
	WriteMapToLcd(g_mmap);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void FillLcdDisplay(void)
{
	// Turn all of the LCD pixels on (1's), effectively filling the display
	memset(&(g_mmap[0][0]), 0xFF, sizeof(g_mmap));
	WriteMapToLcd(g_mmap);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetNextLcdBacklightState(void)
{
	LCD_BACKLIGHT_STATES backlightState;

	// Get current backlight state
	backlightState = GetLcdBacklightState();

	// Advance to next backlight state
	switch (backlightState)
	{
		case BACKLIGHT_OFF		: SetLcdBacklightState(BACKLIGHT_SUPER_LOW);break;
		case BACKLIGHT_SUPER_LOW: SetLcdBacklightState(BACKLIGHT_LOW);		break;
		case BACKLIGHT_LOW		: SetLcdBacklightState(BACKLIGHT_DIM);		break;
		case BACKLIGHT_DIM		: SetLcdBacklightState(BACKLIGHT_MID);		break;
		case BACKLIGHT_MID		: SetLcdBacklightState(BACKLIGHT_BRIGHT);	break;
		case BACKLIGHT_BRIGHT	: SetLcdBacklightState(BACKLIGHT_FULL);		break;
		case BACKLIGHT_FULL		: SetLcdBacklightState(BACKLIGHT_OFF);		break;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* Empty call until LCD connector fixed or hardware modded */
LCD_BACKLIGHT_STATES GetLcdBacklightState(void) { return (BACKLIGHT_OFF); }
#else
LCD_BACKLIGHT_STATES GetLcdBacklightState(void)
{
	uint8_t backlightLevel, backlightState;

	backlightLevel = ft81x_get_backlight_level();

	switch (backlightLevel)
	{
		case FT81X_BACKLIGHT_OFF: backlightState = BACKLIGHT_OFF; break;
		case FT81X_BACKLIGHT_SUPER_LOW: backlightState = BACKLIGHT_SUPER_LOW; break;
		case FT81X_BACKLIGHT_LOW: backlightState = BACKLIGHT_LOW; break;
		case FT81X_BACKLIGHT_DIM: backlightState = BACKLIGHT_DIM; break;
		case FT81X_BACKLIGHT_MID: backlightState = BACKLIGHT_MID; break;
		case FT81X_BACKLIGHT_BRIGHT: backlightState = BACKLIGHT_BRIGHT; break;
		case FT81X_BACKLIGHT_FULL: backlightState = BACKLIGHT_FULL; break;
		default: backlightState = BACKLIGHT_OFF; break;
	}

	return (backlightState);
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* Empty call until LCD connector fixed or hardware modded */
void SetLcdBacklightState(LCD_BACKLIGHT_STATES state) {}
#else
void SetLcdBacklightState(LCD_BACKLIGHT_STATES state)
{
	switch (state)
	{
		case BACKLIGHT_OFF: ft81x_backlight_off(); break;
		case BACKLIGHT_SUPER_LOW: ft81x_set_backlight_level(FT81X_BACKLIGHT_SUPER_LOW); break;
		case BACKLIGHT_LOW: ft81x_set_backlight_level(FT81X_BACKLIGHT_LOW); break;
		case BACKLIGHT_DIM: ft81x_set_backlight_level(FT81X_BACKLIGHT_DIM); break;
		case BACKLIGHT_MID: ft81x_set_backlight_level(FT81X_BACKLIGHT_MID); break;
		case BACKLIGHT_BRIGHT: ft81x_set_backlight_level(FT81X_BACKLIGHT_BRIGHT); break;
		case BACKLIGHT_FULL: ft81x_set_backlight_level(FT81X_BACKLIGHT_FULL); break;
	}
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AdjustLcdContrast(CONTRAST_ADJUSTMENT adjust)
{
	switch (adjust)
	{
		case LIGHTER:
			// Check if an increment in contrast will exceed the max
			if ((g_contrast_value + CONTRAST_STEPPING) <= MAX_CONTRAST)
			{
				g_contrast_value += CONTRAST_STEPPING;
			} 

			SetLcdContrast(g_contrast_value);
			break;
		
		case DARKER:
			// Check if a decrement in contrast will exceed the min
			if ((g_contrast_value - CONTRAST_STEPPING) >= MIN_CONTRAST)
			{
				g_contrast_value -= CONTRAST_STEPPING;
			} 

			SetLcdContrast(g_contrast_value);
			break;
	}
	
	g_unitConfig.lcdContrast = g_contrast_value;

	// Set flag to save LCD contrast change before powering off (or next unit config save) to prevent constant eeprom programming if a key is stuck
	g_lcdContrastChanged = YES;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetLcdContrast(uint8 cmd)
{
	// Check if lcd contrast adjustment is out of visable range
	if (cmd > DEFAULT_MAX_CONTRAST)
	{
		g_contrast_value = DEFAULT_MAX_CONTRAST;
		return;
	}

	// Fill in if contrast can be set
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TurnDisplayOff(void)
{
	DisplayTimerCallBack();
	LcdPwTimerCallBack();

	while (g_kpadInterruptWhileProcessing == YES)
	{
		g_kpadInterruptWhileProcessing = NO;

		// Clear key/button interrupt flags
		MXC_GPIO_ClearFlags(REGULAR_BUTTONS_GPIO_PORT, REGULAR_BUTTONS_GPIO_MASK);
	}

	g_kpadProcessingFlag = DEACTIVATED;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ActivateDisplayShortDuration(uint16 secondsToDisplay)
{
	// Check if the LCD Power was turned off
	if (g_lcdPowerFlag == DISABLED)
	{
		g_lcdPowerFlag = ENABLED;
		ft81x_init(); // Power up and init display
		AssignSoftTimer(LCD_POWER_ON_OFF_TIMER_NUM, (secondsToDisplay * TICKS_PER_SEC), LcdPwTimerCallBack);

		// Check if the unit is monitoring, if so, reassign the monitor update timer
		if (g_sampleProcessing == ACTIVE_STATE)
		{
			debug("Keypress Timer Mgr: enabling Monitor Update Timer.\r\n");
			AssignSoftTimer(MENU_UPDATE_TIMER_NUM, ONE_SECOND_TIMEOUT, MenuUpdateTimerCallBack);
		}
	}

	// Check if the LCD Backlight was turned off
	if (g_lcdBacklightFlag == DISABLED)
	{
		g_lcdBacklightFlag = ENABLED;
		SetLcdBacklightState(BACKLIGHT_MID);
		AssignSoftTimer(LCD_BACKLIGHT_ON_OFF_TIMER_NUM, (secondsToDisplay * TICKS_PER_SEC), DisplayTimerCallBack);
	}
}
