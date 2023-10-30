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
#include "Menu.h"
#include "Display.h"
#include "Uart.h"
#include "Display.h"
#include "Keypad.h"
#include "TextTypes.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define BATTERY_MN_TABLE_SIZE 8
#define BATTERY_WND_STARTING_COL 6
#define BATTERY_WND_END_COL 127
#define BATTERY_WND_STARTING_ROW 8
#define BATTERY_WND_END_ROW 55
#define BATTERY_MN_TBL_START_LINE 0

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
#include "Globals.h"
extern USER_MENU_STRUCT configMenu[];

///----------------------------------------------------------------------------
///	Local Scope Globals
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------
void BatteryMn(INPUT_MSG_STRUCT);
void BatteryMnProc(INPUT_MSG_STRUCT msg, WND_LAYOUT_STRUCT*, MN_LAYOUT_STRUCT *);
void BatteryMnDsply(WND_LAYOUT_STRUCT*);

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void BatteryMn(INPUT_MSG_STRUCT msg)
{
	static WND_LAYOUT_STRUCT wnd_layout;
	static MN_LAYOUT_STRUCT mn_layout;

	BatteryMnProc(msg, &wnd_layout, &mn_layout);

	if (g_activeMenu == BATTERY_MENU)
	{
		BatteryMnDsply(&wnd_layout);
		WriteMapToLcd(g_mmap);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void BatteryMnProc(INPUT_MSG_STRUCT msg, WND_LAYOUT_STRUCT *wnd_layout_ptr, MN_LAYOUT_STRUCT *mn_layout_ptr)
{
	INPUT_MSG_STRUCT mn_msg;
	uint32 input;

	switch (msg.cmd)
	{
		case (ACTIVATE_MENU_CMD):
			wnd_layout_ptr->start_col = BATTERY_WND_STARTING_COL;
			wnd_layout_ptr->end_col = BATTERY_WND_END_COL;
			wnd_layout_ptr->start_row = BATTERY_WND_STARTING_ROW;
			wnd_layout_ptr->end_row = BATTERY_WND_END_ROW;

			mn_layout_ptr->curr_ln = 1;
			mn_layout_ptr->top_ln = 1;
			mn_layout_ptr->sub_ln = 0;
			break;

		case (KEYPRESS_MENU_CMD):

			input = msg.data[0];
			switch (input)
			{
				case (ENTER_KEY):
					SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
					JUMP_TO_ACTIVE_MENU();
					break;
				case (DOWN_ARROW_KEY):
						break;
				case (UP_ARROW_KEY):
						break;
				case (MINUS_KEY): AdjustLcdContrast(DARKER); break;
				case (PLUS_KEY): AdjustLcdContrast(LIGHTER); break;
				case (ESC_KEY):
					SETUP_USER_MENU_MSG(&configMenu, BATTERY);
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
void BatteryMnDsply(WND_LAYOUT_STRUCT *wnd_layout_ptr)
{
	char spaceBuff[25];
	uint8 batt_buff[20];
	uint32 x = 0;
	float curr_batt_volts;
	float batt_rng;
	uint8 length;

	memset(&(g_mmap[0][0]), 0, sizeof(g_mmap));

	// Add in a title for the menu
	length = sprintf((char*)g_spareBuffer, "-%s-", getLangText(BATTERY_VOLTAGE_TEXT));

	wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_ZERO;
	wnd_layout_ptr->curr_col = (uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));
	WndMpWrtString(g_spareBuffer, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

	wnd_layout_ptr->curr_col = wnd_layout_ptr->start_col;
	wnd_layout_ptr->next_row = wnd_layout_ptr->start_row;
	wnd_layout_ptr->next_col = wnd_layout_ptr->start_col;

	curr_batt_volts = GetExternalVoltageLevelAveraged(BATTERY_VOLTAGE);

	// ********** Print Battery text **********
	sprintf((char*)g_spareBuffer, "%.2f %s", curr_batt_volts, getLangText(VOLTS_TEXT));
	debug("Battery: %s\r\n", (char*)g_spareBuffer);

	wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_TWO;
	WndMpWrtString(g_spareBuffer, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

	// ********** Print the Low and Full text **********
	memset(&spaceBuff[0], 0, sizeof(spaceBuff));
	memset(&spaceBuff[0], ' ', sizeof(spaceBuff) - 1);

	length = (uint8)(strlen(getLangText(LOW_TEXT)) + strlen(getLangText(FULL_TEXT)));
	spaceBuff[(20 - length)] = '\0';

	sprintf((char*)g_spareBuffer, "%s%s%s", getLangText(LOW_TEXT), (char*)&spaceBuff[0], getLangText(FULL_TEXT));

	wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_FOUR;
	WndMpWrtString(g_spareBuffer, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

	// ********** E========F **********
	memset(&batt_buff[0], 0, sizeof(batt_buff));
	memset(&batt_buff[0], ' ', (sizeof(batt_buff) - 1));

	batt_rng = (float).25;
	x = 0;
	if (curr_batt_volts > BATT_MIN_VOLTS)
	{
		curr_batt_volts = (float)(curr_batt_volts - BATT_MIN_VOLTS);
	}
	else
	{
		curr_batt_volts = 0;
	}

	// Creating a string to give the appearance of a battery metter. Using '=' as the bar
	for (x = 0; x < 10; x++)
	{
		if ((curr_batt_volts > batt_rng) && (x < 9))
		{
			batt_buff[x] = '=';
			curr_batt_volts -= batt_rng;
		}
		else
		{
			batt_buff[x] = '|';
			break;
		}
	}
	batt_buff[10] = 0; // Assign to null

	length = (uint8)sprintf((char*)g_spareBuffer, "[%s]", batt_buff);
	wnd_layout_ptr->curr_col =(uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));

	wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_FIVE;
	WndMpWrtString(g_spareBuffer, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

	// ********** Print other battery voltages **********
	curr_batt_volts = GetExternalVoltageLevelAveraged(EXT_CHARGE_VOLTAGE);

	// Check if the external charge voltage is above 0.5 volts indicating that it's active
	if (curr_batt_volts < 3.5)
	{
		curr_batt_volts = 0;
	}

	length = (uint8)sprintf((char*)g_spareBuffer, "(%.2f %s)", curr_batt_volts, getLangText(VOLTS_TEXT));

	wnd_layout_ptr->curr_col =(uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));
	wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_SEVEN;
	WndMpWrtString(g_spareBuffer, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
}
