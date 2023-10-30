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
#include "Common.h"
#include "Typedefs.h"
#include "Menu.h"
#include "Uart.h"
#include "Display.h"
#include "RealTimeClock.h"
#include "PowerManagement.h"
#include "Keypad.h"
#include "TextTypes.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define TIMER_MODE_DATE_MN_SIZE_EXTENSION
#define TIMER_MODE_DATE_MN_TABLE_SIZE 8
#define TIMER_MODE_DATE_WND_STARTING_COL 3
#define TIMER_MODE_DATE_WND_END_COL 127
#define TIMER_MODE_DATE_WND_STARTING_ROW DEFAULT_MENU_ROW_THREE
#define TIMER_MODE_DATE_WND_END_ROW DEFAULT_MENU_ROW_SIX
#define TIMER_MODE_DATE_MN_TBL_START_LINE 0
#define TMD_START_DAY	0
#define TMD_START_MONTH	1
#define TMD_START_YEAR	2
#define TMD_STOP_DAY	3
#define TMD_STOP_MONTH	4
#define TMD_STOP_YEAR	5
#define END_DATA_STRING_SIZE 6
#define MIN_CHAR_STRING 0x41
#define MIN_NUM_STRING 0x2E
#define MAX_CHAR_STRING 0x5a
#define MAX_NUM_STRING 0x39
#define INPUT_MENU_SECOND_LINE_INDENT 6
#define MAX_CHARS_PER_LINE 20
#define DATE_TIME_MN_WND_LNS 6

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
void TimerModeDateMenu(INPUT_MSG_STRUCT);
void TimerModeDateMenuProc(INPUT_MSG_STRUCT, REC_MN_STRUCT*, WND_LAYOUT_STRUCT*, MN_LAYOUT_STRUCT*);
void TimerModeDateMenuDisplay(REC_MN_STRUCT*, WND_LAYOUT_STRUCT*, MN_LAYOUT_STRUCT*);
void LoadTimerModeDateMnDefRec(REC_MN_STRUCT*, DATE_TIME_STRUCT*);
void TimerModeDateMenuDvScroll(char dir_key, REC_MN_STRUCT*);
void TimerModeDateMenuScroll(char, MN_LAYOUT_STRUCT*);

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TimerModeDateMenu (INPUT_MSG_STRUCT msg)
{
	static WND_LAYOUT_STRUCT wnd_layout;
	static MN_LAYOUT_STRUCT mn_layout;
	static REC_MN_STRUCT mn_rec[6];

	TimerModeDateMenuProc(msg, mn_rec, &wnd_layout, &mn_layout);

	if (g_activeMenu == TIMER_MODE_DATE_MENU)
	{
		TimerModeDateMenuDisplay(mn_rec, &wnd_layout, &mn_layout);
		WriteMapToLcd(g_mmap);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TimerModeDateMenuProc(INPUT_MSG_STRUCT msg, REC_MN_STRUCT *rec_ptr, WND_LAYOUT_STRUCT *wnd_layout_ptr, MN_LAYOUT_STRUCT *mn_layout_ptr)
{
	INPUT_MSG_STRUCT mn_msg;
	DATE_TIME_STRUCT time;
	uint32 input;
	//uint8 dayOfWeek = 0;

	switch (msg.cmd)
	{
		case (ACTIVATE_MENU_CMD):
			wnd_layout_ptr->start_col = TIMER_MODE_DATE_WND_STARTING_COL;
			wnd_layout_ptr->end_col = TIMER_MODE_DATE_WND_END_COL;
			wnd_layout_ptr->start_row = TIMER_MODE_DATE_WND_STARTING_ROW;
			wnd_layout_ptr->end_row = TIMER_MODE_DATE_WND_END_ROW;

			mn_layout_ptr->curr_ln = 0;
			mn_layout_ptr->top_ln = 0;
			mn_layout_ptr->sub_ln = 0;

			time = GetCurrentTime();
			LoadTimerModeDateMnDefRec(rec_ptr, &time);
			rec_ptr[mn_layout_ptr->curr_ln].enterflag = TRUE;
			break;

		case (KEYPRESS_MENU_CMD):

			input = msg.data[0];
			switch (input)
			{
				case (ENTER_KEY):
					g_unitConfig.timerStartDate.day = (char)rec_ptr[TMD_START_DAY].numrec.tindex;
					g_unitConfig.timerStartDate.month = (char)rec_ptr[TMD_START_MONTH].numrec.tindex;
					g_unitConfig.timerStartDate.year = (char)rec_ptr[TMD_START_YEAR].numrec.tindex;
					g_unitConfig.timerStopDate.day = (char)rec_ptr[TMD_STOP_DAY].numrec.tindex;
					g_unitConfig.timerStopDate.month = (char)rec_ptr[TMD_STOP_MONTH].numrec.tindex;
					g_unitConfig.timerStopDate.year = (char)rec_ptr[TMD_STOP_YEAR].numrec.tindex;

					ProcessTimerModeSettings(PROMPT);

					SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
					JUMP_TO_ACTIVE_MENU();
					break;
				case (DOWN_ARROW_KEY):
						if (rec_ptr[mn_layout_ptr->curr_ln].enterflag == TRUE)
						{
						TimerModeDateMenuDvScroll(DOWN, &rec_ptr[mn_layout_ptr->curr_ln]);
						}
						break;
				case (UP_ARROW_KEY):
						if (rec_ptr[mn_layout_ptr->curr_ln].enterflag == TRUE)
						{
						TimerModeDateMenuDvScroll(UP, &rec_ptr[mn_layout_ptr->curr_ln]);
						}
						break;
				case (PLUS_KEY):
						rec_ptr[mn_layout_ptr->curr_ln].enterflag = FALSE;
						TimerModeDateMenuScroll(DOWN, mn_layout_ptr);
						rec_ptr[mn_layout_ptr->curr_ln].enterflag = TRUE;
						break;
				case (MINUS_KEY):
						rec_ptr[mn_layout_ptr->curr_ln].enterflag = FALSE;
						TimerModeDateMenuScroll(UP, mn_layout_ptr);
						rec_ptr[mn_layout_ptr->curr_ln].enterflag = TRUE;
						break;
				case (ESC_KEY):
						SETUP_MENU_MSG(TIMER_MODE_TIME_MENU);
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
void TimerModeDateMenuScroll(char direction, MN_LAYOUT_STRUCT* mn_layout_ptr)
{
	switch (direction)
	{
		case (DOWN):
			if (mn_layout_ptr->curr_ln < 5)
			{
				mn_layout_ptr->curr_ln++;
			}
		break;

		case (UP):
			if (mn_layout_ptr->curr_ln > 0)
			{
				mn_layout_ptr->curr_ln--;
			}
		break;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TimerModeDateMenuDvScroll(char dir_key, REC_MN_STRUCT *rec_ptr)
{
	switch (dir_key)
	{
		case (UP):
			if (rec_ptr->numrec.tindex < rec_ptr->numrec.nmax)
			{
				rec_ptr->numrec.tindex += 1;
			}
			else
			{
				rec_ptr->numrec.tindex = rec_ptr->numrec.nmin;
			}
			break;

		case (DOWN):
			if (rec_ptr->numrec.tindex > rec_ptr->numrec.nmin)
			{
				rec_ptr->numrec.tindex -= 1;
			}
			else
			{
				rec_ptr->numrec.tindex = rec_ptr->numrec.nmax;
			}
			break;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TimerModeDateMenuDisplay(REC_MN_STRUCT *rec_ptr, WND_LAYOUT_STRUCT *wnd_layout_ptr, MN_LAYOUT_STRUCT *mn_layout_ptr)
{
	uint8 sbuff[50];
	uint8 top;
	uint8 menu_ln;
	uint8 length = 0;

	memset(&(g_mmap[0][0]), 0, sizeof(g_mmap));

	menu_ln = 0;
	top = (uint8)mn_layout_ptr->top_ln;

	// Add in a title for the menu
	memset(&sbuff[0], 0, sizeof(sbuff));
	length = (uint8)sprintf((char*)sbuff, "-%s-", getLangText(ACTIVE_DATE_PERIOD_TEXT));

	wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_ZERO;
	wnd_layout_ptr->curr_col = (uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));
	WndMpWrtString(sbuff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

	// Add in a title for the menu
	memset(&sbuff[0], 0, sizeof(sbuff));
	length = (uint8)sprintf((char*)sbuff, "(%s)", getLangText(DAY_MONTH_YEAR_TEXT));

	wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_ONE;
	wnd_layout_ptr->curr_col = (uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));
	WndMpWrtString(sbuff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

	memset(&sbuff[0], 0, sizeof(sbuff));

	wnd_layout_ptr->curr_row = wnd_layout_ptr->start_row;
	wnd_layout_ptr->curr_col = wnd_layout_ptr->start_col;
	wnd_layout_ptr->next_row = wnd_layout_ptr->start_row;
	wnd_layout_ptr->next_col = wnd_layout_ptr->start_col;

	rec_ptr[TMD_START_DAY].numrec.nmax = GetDaysPerMonth((uint8)(rec_ptr[TMD_START_MONTH].numrec.tindex),
																(uint8)(rec_ptr[TMD_START_YEAR].numrec.tindex));
	if (rec_ptr[TMD_START_DAY].numrec.tindex > rec_ptr[TMD_START_DAY].numrec.nmax)
		rec_ptr[TMD_START_DAY].numrec.tindex = rec_ptr[TMD_START_DAY].numrec.nmax;

	rec_ptr[TMD_STOP_DAY].numrec.nmax = GetDaysPerMonth((uint8)(rec_ptr[TMD_STOP_MONTH].numrec.tindex),
																(uint8)(rec_ptr[TMD_STOP_YEAR].numrec.tindex));
	if (rec_ptr[TMD_STOP_DAY].numrec.tindex > rec_ptr[TMD_STOP_DAY].numrec.nmax)
		rec_ptr[TMD_STOP_DAY].numrec.tindex = rec_ptr[TMD_STOP_DAY].numrec.nmax;

	// ---------------------------------------------------------------------------------
	// Write out the start date line which includes the start date text, day, month, and year values
	// ---------------------------------------------------------------------------------
	wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_THREE;
	wnd_layout_ptr->curr_col = wnd_layout_ptr->start_col;

	// Display the start date text
	sprintf((char*)sbuff, "%s: ", getLangText(START_DATE_TEXT));
	WndMpWrtString(sbuff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

	wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_FOUR;
	wnd_layout_ptr->curr_col = (uint16)(((wnd_layout_ptr->end_col)/2) - ((9 * SIX_COL_SIZE)/2));

	// Display the day
	sprintf((char*)sbuff, "%02d", (uint16)(uint16)rec_ptr[TMD_START_DAY].numrec.tindex);
	WndMpWrtString(sbuff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, (mn_layout_ptr->curr_ln == TMD_START_DAY) ? CURSOR_LN : REG_LN);
	wnd_layout_ptr->curr_col = wnd_layout_ptr->next_col;

	WndMpWrtString((uint8*)("-"), wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
	wnd_layout_ptr->curr_col = wnd_layout_ptr->next_col;

	// Display the month text
	sprintf((char*)sbuff, "%s", (char*)&(g_monthTable[(uint8)(rec_ptr[TMD_START_MONTH].numrec.tindex)].name[0]));
	WndMpWrtString(sbuff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, (mn_layout_ptr->curr_ln == TMD_START_MONTH) ? CURSOR_LN : REG_LN);
	wnd_layout_ptr->curr_col = wnd_layout_ptr->next_col;

	WndMpWrtString((uint8*)("-"), wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
	wnd_layout_ptr->curr_col = wnd_layout_ptr->next_col;

	// Display the year
	sprintf((char*)sbuff, "%02d", (uint16)(uint16)rec_ptr[TMD_START_YEAR].numrec.tindex);
	WndMpWrtString(sbuff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, (mn_layout_ptr->curr_ln == TMD_START_YEAR) ? CURSOR_LN : REG_LN);

	// ---------------------------------------------------------------------------------
	// Write out the stop date line which includes the stop date text, day, month, and year values
	// ---------------------------------------------------------------------------------
	wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_FIVE;
	wnd_layout_ptr->curr_col = wnd_layout_ptr->start_col;

	// Display the date text
	sprintf((char*)sbuff, "%s: ", getLangText(STOP_DATE_TEXT));
	WndMpWrtString(sbuff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

	wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_SIX;
	wnd_layout_ptr->curr_col = (uint16)(((wnd_layout_ptr->end_col)/2) - ((9 * SIX_COL_SIZE)/2));

	// Display the day
	sprintf((char*)sbuff, "%02d", (uint16)(uint16)rec_ptr[TMD_STOP_DAY].numrec.tindex);
	WndMpWrtString(sbuff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, (mn_layout_ptr->curr_ln == TMD_STOP_DAY) ? CURSOR_LN : REG_LN);
	wnd_layout_ptr->curr_col = wnd_layout_ptr->next_col;

	WndMpWrtString((uint8*)("-"), wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
	wnd_layout_ptr->curr_col = wnd_layout_ptr->next_col;

	// Display the month text
	sprintf((char*)sbuff, "%s", (char*)&(g_monthTable[(uint8)(rec_ptr[TMD_STOP_MONTH].numrec.tindex)].name[0]));
	WndMpWrtString(sbuff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, (mn_layout_ptr->curr_ln == TMD_STOP_MONTH) ? CURSOR_LN : REG_LN);
	wnd_layout_ptr->curr_col = wnd_layout_ptr->next_col;

	WndMpWrtString((uint8*)("-"), wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
	wnd_layout_ptr->curr_col = wnd_layout_ptr->next_col;

	// Display the year
	sprintf((char*)sbuff, "%02d", (uint16)(uint16)rec_ptr[TMD_STOP_YEAR].numrec.tindex);
	WndMpWrtString(sbuff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, (mn_layout_ptr->curr_ln == TMD_STOP_YEAR) ? CURSOR_LN : REG_LN);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void LoadTimerModeDateMnDefRec(REC_MN_STRUCT *rec_ptr, DATE_TIME_STRUCT *time_ptr)
{
	// START DAY
	rec_ptr[TMD_START_DAY].enterflag = FALSE;
	rec_ptr[TMD_START_DAY].type = INPUT_NUM_STRING;
	rec_ptr[TMD_START_DAY].wrapflag = FALSE;
	rec_ptr[TMD_START_DAY].numrec.nindex = 0;
	rec_ptr[TMD_START_DAY].numrec.nmax = 31;
	rec_ptr[TMD_START_DAY].numrec.nmin = 1;
	rec_ptr[TMD_START_DAY].numrec.incr_value = 1;
	rec_ptr[TMD_START_DAY].numrec.tindex = time_ptr->day;
	rec_ptr[TMD_START_DAY].numrec.num_type = FIXED_TIME_TYPE_DAY;

	// START MONTH
	rec_ptr[TMD_START_MONTH].enterflag = FALSE;
	rec_ptr[TMD_START_MONTH].type = INPUT_NUM_STRING;
	rec_ptr[TMD_START_MONTH].wrapflag = FALSE;
	rec_ptr[TMD_START_MONTH].numrec.nindex = 0;
	rec_ptr[TMD_START_MONTH].numrec.nmax = 12;
	rec_ptr[TMD_START_MONTH].numrec.nmin = 1;
	rec_ptr[TMD_START_MONTH].numrec.incr_value = 1;
	rec_ptr[TMD_START_MONTH].numrec.tindex = time_ptr->month;
	rec_ptr[TMD_START_MONTH].numrec.num_type = FIXED_TIME_TYPE_MONTH;

	// START YEAR
	rec_ptr[TMD_START_YEAR].enterflag = FALSE;
	rec_ptr[TMD_START_YEAR].type = INPUT_NUM_STRING;
	rec_ptr[TMD_START_YEAR].wrapflag = FALSE;
	rec_ptr[TMD_START_YEAR].numrec.nindex = 0;
	rec_ptr[TMD_START_YEAR].numrec.nmax = 99;
	rec_ptr[TMD_START_YEAR].numrec.nmin = 0;
	rec_ptr[TMD_START_YEAR].numrec.incr_value = 1;
	rec_ptr[TMD_START_YEAR].numrec.tindex = time_ptr->year;
	rec_ptr[TMD_START_YEAR].numrec.num_type = FIXED_TIME_TYPE_YEAR;

	// STOP DAY
	rec_ptr[TMD_STOP_DAY].enterflag = FALSE;
	rec_ptr[TMD_STOP_DAY].type = INPUT_NUM_STRING;
	rec_ptr[TMD_STOP_DAY].wrapflag = FALSE;
	rec_ptr[TMD_STOP_DAY].numrec.nindex = 0;
	rec_ptr[TMD_STOP_DAY].numrec.nmax = 31;
	rec_ptr[TMD_STOP_DAY].numrec.nmin = 1;
	rec_ptr[TMD_STOP_DAY].numrec.incr_value = 1;
	rec_ptr[TMD_STOP_DAY].numrec.tindex = time_ptr->day;
	rec_ptr[TMD_STOP_DAY].numrec.num_type = FIXED_TIME_TYPE_DAY;

	// STOP MONTH
	rec_ptr[TMD_STOP_MONTH].enterflag = FALSE;
	rec_ptr[TMD_STOP_MONTH].type = INPUT_NUM_STRING;
	rec_ptr[TMD_STOP_MONTH].wrapflag = FALSE;
	rec_ptr[TMD_STOP_MONTH].numrec.nindex = 0;
	rec_ptr[TMD_STOP_MONTH].numrec.nmax = 12;
	rec_ptr[TMD_STOP_MONTH].numrec.nmin = 1;
	rec_ptr[TMD_STOP_MONTH].numrec.incr_value = 1;
	rec_ptr[TMD_STOP_MONTH].numrec.tindex = time_ptr->month;
	rec_ptr[TMD_STOP_MONTH].numrec.num_type = FIXED_TIME_TYPE_MONTH;

	// STOP YEAR
	rec_ptr[TMD_STOP_YEAR].enterflag = FALSE;
	rec_ptr[TMD_STOP_YEAR].type = INPUT_NUM_STRING;
	rec_ptr[TMD_STOP_YEAR].wrapflag = FALSE;
	rec_ptr[TMD_STOP_YEAR].numrec.nindex = 0;
	rec_ptr[TMD_STOP_YEAR].numrec.nmax = 99;
	rec_ptr[TMD_STOP_YEAR].numrec.nmin = 0;
	rec_ptr[TMD_STOP_YEAR].numrec.incr_value = 1;
	rec_ptr[TMD_STOP_YEAR].numrec.tindex = (time_ptr->year + 1);
	rec_ptr[TMD_STOP_YEAR].numrec.num_type = FIXED_TIME_TYPE_YEAR;
}
