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
#include "Board.h"
#include "Record.h"
#include "Uart.h"
#include "Display.h"
#include "Keypad.h"
#include "TextTypes.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define MONITOR_LOG_MN_TABLE_SIZE 8
#define MONITOR_LOG_WND_STARTING_COL 6
#define MONITOR_LOG_WND_END_COL 127
#define MONITOR_LOG_WND_STARTING_ROW 8
#define MONITOR_LOG_WND_END_ROW 55
#define MONITOR_LOG_MN_TBL_START_LINE 0

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
#include "Globals.h"
extern USER_MENU_STRUCT configMenu[];

///----------------------------------------------------------------------------
///	Local Scope Globals
///----------------------------------------------------------------------------
static int16 s_MonitorMenuCurrentLogIndex = 0;
static uint16 s_MonitorMenuLastLogIndex = 0;
static uint16 s_MonitorMenuStartLogIndex = 0;

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------
void MonitorLogMn(INPUT_MSG_STRUCT);
void MonitorLogMnProc(INPUT_MSG_STRUCT msg, WND_LAYOUT_STRUCT *, MN_LAYOUT_STRUCT *);
void MonitorLogMnDsply(WND_LAYOUT_STRUCT *);

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MonitorLogMn(INPUT_MSG_STRUCT msg)
{
	static WND_LAYOUT_STRUCT wnd_layout;
	static MN_LAYOUT_STRUCT mn_layout;

	MonitorLogMnProc(msg, &wnd_layout, &mn_layout);

	if (g_activeMenu == VIEW_MONITOR_LOG_MENU)
	{
		MonitorLogMnDsply(&wnd_layout);
		WriteMapToLcd(g_mmap);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MonitorLogMnProc(INPUT_MSG_STRUCT msg, WND_LAYOUT_STRUCT *wnd_layout_ptr, MN_LAYOUT_STRUCT *mn_layout_ptr)
{
	INPUT_MSG_STRUCT mn_msg;
	uint32 input;

	switch(msg.cmd)
	{
		case (ACTIVATE_MENU_CMD):
			wnd_layout_ptr->start_col = MONITOR_LOG_WND_STARTING_COL;
			wnd_layout_ptr->end_col = MONITOR_LOG_WND_END_COL;
			wnd_layout_ptr->start_row = MONITOR_LOG_WND_STARTING_ROW;
			wnd_layout_ptr->end_row = MONITOR_LOG_WND_END_ROW;

			mn_layout_ptr->curr_ln = 1;
			mn_layout_ptr->top_ln = 1;
			mn_layout_ptr->sub_ln = 0;

			s_MonitorMenuCurrentLogIndex = (int16)GetStartingMonitorLogTableIndex();
			s_MonitorMenuLastLogIndex = __monitorLogTblIndex;

			// Loop through circular buffer to find starting index
			while((__monitorLogTbl[s_MonitorMenuCurrentLogIndex].status != COMPLETED_LOG_ENTRY) &&
					(s_MonitorMenuCurrentLogIndex != s_MonitorMenuLastLogIndex))
			{
				s_MonitorMenuCurrentLogIndex++;

				if (s_MonitorMenuCurrentLogIndex >= (int16)TOTAL_MONITOR_LOG_ENTRIES)
					s_MonitorMenuCurrentLogIndex = 0;
			}

			s_MonitorMenuStartLogIndex = (uint16)s_MonitorMenuCurrentLogIndex;

			if ((s_MonitorMenuCurrentLogIndex == s_MonitorMenuLastLogIndex) &&
				(__monitorLogTbl[s_MonitorMenuCurrentLogIndex].status != COMPLETED_LOG_ENTRY))
			{
				s_MonitorMenuCurrentLogIndex = -1;
			}
		break;

		case (KEYPRESS_MENU_CMD):
			input = msg.data[0];
			switch(input)
			{
				case (ENTER_KEY):
					MessageBox(getLangText(STATUS_TEXT), getLangText(NOT_INCLUDED_TEXT), MB_OK);
				break;

				case (DOWN_ARROW_KEY):
					if ((s_MonitorMenuCurrentLogIndex != s_MonitorMenuLastLogIndex) && (s_MonitorMenuCurrentLogIndex >= 0))
					{
						s_MonitorMenuCurrentLogIndex++;

						if (s_MonitorMenuCurrentLogIndex >= (int16)TOTAL_MONITOR_LOG_ENTRIES)
							s_MonitorMenuCurrentLogIndex = 0;

						while((__monitorLogTbl[s_MonitorMenuCurrentLogIndex].status != COMPLETED_LOG_ENTRY) &&
								(s_MonitorMenuCurrentLogIndex != s_MonitorMenuLastLogIndex))
						{
							s_MonitorMenuCurrentLogIndex++;

							if (s_MonitorMenuCurrentLogIndex >= (int16)TOTAL_MONITOR_LOG_ENTRIES)
								s_MonitorMenuCurrentLogIndex = 0;
						}
					}
				break;

				case (UP_ARROW_KEY):
					if ((s_MonitorMenuCurrentLogIndex != s_MonitorMenuStartLogIndex) && (s_MonitorMenuCurrentLogIndex >= 0))
					{
						s_MonitorMenuCurrentLogIndex--;

						if (s_MonitorMenuCurrentLogIndex < 0)
							s_MonitorMenuCurrentLogIndex = TOTAL_MONITOR_LOG_ENTRIES - 1;

						while((__monitorLogTbl[s_MonitorMenuCurrentLogIndex].status != COMPLETED_LOG_ENTRY) &&
								(s_MonitorMenuCurrentLogIndex != s_MonitorMenuStartLogIndex))
						{
							s_MonitorMenuCurrentLogIndex--;

							if (s_MonitorMenuCurrentLogIndex < 0)
								s_MonitorMenuCurrentLogIndex = TOTAL_MONITOR_LOG_ENTRIES - 1;
						}
					}
				break;

				case (MINUS_KEY): AdjustLcdContrast(DARKER); break;
				case (PLUS_KEY): AdjustLcdContrast(LIGHTER); break;

				case (ESC_KEY):
					SETUP_USER_MENU_MSG(&configMenu, MONITOR_LOG);
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
void MonitorLogMnDsply(WND_LAYOUT_STRUCT *wnd_layout_ptr)
{
	uint8 length;

	memset(&(g_mmap[0][0]), 0, sizeof(g_mmap));

	// Add in a title for the menu
	length = (uint8)sprintf((char*)g_spareBuffer, "-%s-", getLangText(VIEW_MONITOR_LOG_TEXT));

	wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_ZERO;
	wnd_layout_ptr->curr_col = (uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));
	WndMpWrtString(g_spareBuffer, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

	if (s_MonitorMenuCurrentLogIndex != -1)
	{
		if (__monitorLogTbl[s_MonitorMenuCurrentLogIndex].status != EMPTY_LOG_ENTRY)
		{
			// Display Mode text
			switch(__monitorLogTbl[s_MonitorMenuCurrentLogIndex].mode)
			{
				case WAVEFORM_MODE: length = (uint8)sprintf((char*)g_spareBuffer, "%s", getLangText(WAVEFORM_MODE_TEXT)); break;
				case BARGRAPH_MODE: length = (uint8)sprintf((char*)g_spareBuffer, "%s", getLangText(BARGRAPH_MODE_TEXT)); break;
				case MANUAL_CAL_MODE: length = (uint8)sprintf((char*)g_spareBuffer, "%s", getLangText(CALIBRATION_TEXT)); break;
				case COMBO_MODE: length = (uint8)sprintf((char*)g_spareBuffer, "%s", getLangText(COMBO_MODE_TEXT)); break;
			}

			wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_TWO;
			wnd_layout_ptr->curr_col = (uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));
			WndMpWrtString(g_spareBuffer, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

			// Display Start Time text
			ConvertTimeStampToString((char*)g_spareBuffer, &__monitorLogTbl[s_MonitorMenuCurrentLogIndex].startTime, REC_DATE_TIME_TYPE);
			length = (uint8)strlen((char*)g_spareBuffer);

			wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_THREE;
			wnd_layout_ptr->curr_col = (uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));
			WndMpWrtString(g_spareBuffer, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

			// Display Stop Time text
			ConvertTimeStampToString((char*)g_spareBuffer, &__monitorLogTbl[s_MonitorMenuCurrentLogIndex].stopTime, REC_DATE_TIME_TYPE);
			length = (uint8)strlen((char*)g_spareBuffer);

			wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_FOUR;
			wnd_layout_ptr->curr_col = (uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));
			WndMpWrtString(g_spareBuffer, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

			// Display Number of Events recorded text
			if (__monitorLogTbl[s_MonitorMenuCurrentLogIndex].eventsRecorded == 0)
			{
				length = (uint8)sprintf((char*)g_spareBuffer, "%s %s", getLangText(NO_TEXT), getLangText(EVENTS_RECORDED_TEXT));

				wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_FIVE;

				if (length > 21)
				{
					wnd_layout_ptr->curr_col = 1;
					WndMpWrtString(&g_spareBuffer[(length - 21)], wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
				}
				else
				{
					wnd_layout_ptr->curr_col = (uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));
					WndMpWrtString(g_spareBuffer, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
				}
			}
			else
			{
				length = (uint8)sprintf((char*)g_spareBuffer, "%s: %d", getLangText(EVENTS_RECORDED_TEXT), __monitorLogTbl[s_MonitorMenuCurrentLogIndex].eventsRecorded);

				wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_FIVE;

				if (length > 21)
				{
					wnd_layout_ptr->curr_col = 1;
					WndMpWrtString(&g_spareBuffer[(length - 21)], wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
				}
				else
				{
					wnd_layout_ptr->curr_col = (uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));
					WndMpWrtString(g_spareBuffer, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
				}

				if (__monitorLogTbl[s_MonitorMenuCurrentLogIndex].eventsRecorded == 1)
				{
					length = (uint8)sprintf((char*)g_spareBuffer, "%s: %d", getLangText(EVENT_NUMBER_TEXT), __monitorLogTbl[s_MonitorMenuCurrentLogIndex].startEventNumber);
				}
				else
				{
					length = (uint8)sprintf((char*)g_spareBuffer, "%s: %d-%d", getLangText(EVENT_NUMBER_TEXT), __monitorLogTbl[s_MonitorMenuCurrentLogIndex].startEventNumber,
											(__monitorLogTbl[s_MonitorMenuCurrentLogIndex].startEventNumber + __monitorLogTbl[s_MonitorMenuCurrentLogIndex].eventsRecorded - 1));
				}

				wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_SIX;

				if (length > 21)
				{
					wnd_layout_ptr->curr_col = 1;
					WndMpWrtString(&g_spareBuffer[(length - 21)], wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
				}
				else
				{
					wnd_layout_ptr->curr_col = (uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));
					WndMpWrtString(g_spareBuffer, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
				}
			}
		}

		if (s_MonitorMenuCurrentLogIndex == s_MonitorMenuStartLogIndex)
		{
			// Display Start of Log
			length = (uint8)sprintf((char*)g_spareBuffer, "<%s>", getLangText(START_OF_LOG_TEXT));

			wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_SEVEN;
			wnd_layout_ptr->curr_col = (uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));
			WndMpWrtString(g_spareBuffer, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
		}
		else if (s_MonitorMenuCurrentLogIndex == s_MonitorMenuLastLogIndex)
		{
			// Display End of Log
			length = (uint8)sprintf((char*)g_spareBuffer, "<%s>", getLangText(END_OF_LOG_TEXT));

			wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_SEVEN;
			wnd_layout_ptr->curr_col = (uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));
			WndMpWrtString(g_spareBuffer, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
		}
	}
	else
	{
		// Display Mode text
		length = (uint8)sprintf((char*)g_spareBuffer, "<%s>", getLangText(EMPTY_TEXT));

		wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_TWO;
		wnd_layout_ptr->curr_col = (uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));
		WndMpWrtString(g_spareBuffer, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
	}
}
