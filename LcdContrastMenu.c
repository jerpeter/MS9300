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
#include "Menu.h"
#include "Display.h"
#include "Display.h"
#include "Uart.h"
#include "Keypad.h"
#include "TextTypes.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define LCD_CONTRAST_MN_TABLE_SIZE DEFAULT_MN_SIZE
#define LCD_CONTRAST_WND_STARTING_COL 6
#define LCD_CONTRAST_WND_END_COL 127
#define LCD_CONTRAST_WND_STARTING_ROW 8
#define LCD_CONTRAST_WND_END_ROW 55
#define LCD_CONTRAST_MN_TBL_START_LINE 0

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
#include "Globals.h"
extern USER_MENU_STRUCT configMenu[];

///----------------------------------------------------------------------------
///	Local Scope Globals
///----------------------------------------------------------------------------
static TEMP_MENU_DATA_STRUCT s_lcdContrastTable [LCD_CONTRAST_MN_TABLE_SIZE] = {
{TITLE_PRE_TAG, LCD_CONTRAST_TEXT, TITLE_POST_TAG},
{ITEM_1, LIGHTER_TEXT, NO_TAG},
{ITEM_2, DEFAULT_TEXT, NO_TAG},
{ITEM_3, DARKER_TEXT, NO_TAG},
{ITEM_4, SAVE_CHANGES_TEXT, NO_TAG},
{NO_TAG, TOTAL_TEXT_STRINGS, NO_TAG}
};

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------
void LcdContrastMn (INPUT_MSG_STRUCT);
void LcdContrastMnProc(INPUT_MSG_STRUCT, WND_LAYOUT_STRUCT *, MN_LAYOUT_STRUCT *);
void AddLcdContrastLevelDisplay(WND_LAYOUT_STRUCT *wnd_layout_ptr);

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void LcdContrastMn(INPUT_MSG_STRUCT msg)
{
	static WND_LAYOUT_STRUCT wnd_layout;
	static MN_LAYOUT_STRUCT mn_layout;

	LcdContrastMnProc(msg, &wnd_layout, &mn_layout);

	if (g_activeMenu == LCD_CONTRAST_MENU)
	{
		DisplaySelectMenu(&wnd_layout, &mn_layout, TITLE_CENTERED);
		AddLcdContrastLevelDisplay(&wnd_layout);
		WriteMapToLcd(g_mmap);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void LcdContrastMnProc(INPUT_MSG_STRUCT msg, WND_LAYOUT_STRUCT *wnd_layout_ptr, MN_LAYOUT_STRUCT *mn_layout_ptr)
{
	INPUT_MSG_STRUCT mn_msg;
	uint32 input;

	switch (msg.cmd)
	{
		case (ACTIVATE_MENU_CMD):
			wnd_layout_ptr->start_col = LCD_CONTRAST_WND_STARTING_COL; /* 6 */
			wnd_layout_ptr->end_col = LCD_CONTRAST_WND_END_COL; /* 127 leaving one pixel space at the end*/
			wnd_layout_ptr->start_row = LCD_CONTRAST_WND_STARTING_ROW; /*/ 8*/
			wnd_layout_ptr->end_row = LCD_CONTRAST_WND_END_ROW; /* 6 */

			mn_layout_ptr->curr_ln = 2;
			mn_layout_ptr->top_ln = 1;
			mn_layout_ptr->sub_ln = 0;

			LoadTempMenuTable(s_lcdContrastTable);
			break;

		case (KEYPRESS_MENU_CMD):
			input = msg.data[0];
			// Handle converting a number key input into a selection
			if ((input >= ONE_KEY) && (input <= FOUR_KEY))
			{
				// Convert ASCII data to a hex number to set the current line
				mn_layout_ptr->curr_ln = (uint16)(input - 0x30);
				input = ENTER_KEY;
			}

			switch (input)
			{
				case (ENTER_KEY):
					switch (mn_layout_ptr->curr_ln)
					{
						case (1): // Lighter
							if ((g_contrast_value - CONTRAST_STEPPING) >= MIN_CONTRAST)
							{
								g_unitConfig.lcdContrast = g_contrast_value -= CONTRAST_STEPPING;
							}

							SetLcdContrast(g_contrast_value);
							break;

						case (2): // Default
							g_unitConfig.lcdContrast = g_contrast_value = DEFUALT_CONTRAST;

							SetLcdContrast(g_contrast_value);
							break;

						case (3): // Darker
							if ((g_contrast_value + CONTRAST_STEPPING) <= MAX_CONTRAST)
							{
								g_unitConfig.lcdContrast = g_contrast_value += CONTRAST_STEPPING;
							}

							SetLcdContrast(g_contrast_value);
							break;

						case (4): // Save changes
							SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

							SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
							JUMP_TO_ACTIVE_MENU();
							break;

						default:
							break;
					}
				break;

			case (DOWN_ARROW_KEY): { MenuScroll(DOWN,SELECT_MN_WND_LNS, mn_layout_ptr); break; }
			case (UP_ARROW_KEY): { MenuScroll(UP,SELECT_MN_WND_LNS,mn_layout_ptr); break; }
			case (MINUS_KEY): { AdjustLcdContrast(DARKER); break; }
			case (PLUS_KEY): { AdjustLcdContrast(LIGHTER); break; }

			case (ESC_KEY):
				SETUP_USER_MENU_MSG(&configMenu, LCD_CONTRAST);
				JUMP_TO_ACTIVE_MENU();
				break;

			default:
				break;
			}
			break;

		default:
			break;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AddLcdContrastLevelDisplay(WND_LAYOUT_STRUCT *wnd_layout_ptr)
{
	uint8 buff[25];
	uint8 spaceBuff[25];
	uint8 contrast_buff[16];
	uint8 x;
	uint8 clvl;
	uint8 length;

	wnd_layout_ptr->curr_col = wnd_layout_ptr->start_col;

	// *** Print the Light and Dark text ***
	memset(&buff[0], 0, sizeof(buff));
	memset(&spaceBuff[0], 0, sizeof(spaceBuff));
	memset(&spaceBuff[0], ' ', sizeof(spaceBuff) - 1);

	length = (uint8)(strlen(getLangText(LIGHT_TEXT)) + strlen(getLangText(DARK_TEXT)));
	spaceBuff[(20 - length)] = '\0';

	sprintf((char*)buff,"%s%s%s", getLangText(LIGHT_TEXT), spaceBuff, getLangText(DARK_TEXT));

	wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_SIX;
	WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

	// *** Print the Contrast Bar
	memset(&buff[0], 0, sizeof(buff));
	memset(&contrast_buff[0], 0, sizeof(contrast_buff));
	memset(&contrast_buff[0], ' ', (sizeof(contrast_buff) - 1));

	clvl = g_contrast_value;
	clvl -= MIN_CONTRAST;

	// Creating a string to give the appearance of a contrast level. Using '=' as the bar
	for (x = 0; x < (sizeof(contrast_buff) - 1); x++)
	{
		if (clvl == 0) { break; }
		else if (clvl == 1) { contrast_buff[x] = '-'; break; }
		else if (clvl == 2) { contrast_buff[x] = '='; break; }
		else { contrast_buff[x] = '='; clvl -= (CONTRAST_STEPPING * 2); }
	}

	debug("Contrast level: <%s>\r\n", buff);
	length = (uint8)sprintf((char*)buff,"[%s]", contrast_buff);
	wnd_layout_ptr->curr_col =(uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));

	wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_SEVEN;
	WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
}
