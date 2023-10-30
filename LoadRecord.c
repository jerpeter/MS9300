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
#include "Keypad.h"
#include "TextTypes.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define LOAD_REC_MN_TABLE_SIZE		8 
#define LOAD_REC_WND_STARTING_COL	DEFAULT_COL_THREE
#define LOAD_REC_WND_END_COL		DEFAULT_END_COL 
#define LOAD_REC_WND_STARTING_ROW	DEFAULT_MENU_ROW_ONE
#define LOAD_REC_WND_END_ROW		DEFAULT_MENU_ROW_SEVEN

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
#include "Globals.h"
extern USER_MENU_STRUCT modeMenu[];
extern USER_MENU_STRUCT helpMenu[];

///----------------------------------------------------------------------------
///	Local Scope Globals
///----------------------------------------------------------------------------
static TEMP_MENU_DATA_STRUCT s_loadRecordMenuTable [LOAD_REC_MN_TABLE_SIZE] = {
{TITLE_PRE_TAG, SAVED_SETTINGS_TEXT, TITLE_POST_TAG},
{NO_TAG, DEFAULT_SELF_TRG_TEXT, NO_TAG},
{NO_TAG, DEFAULT_BAR_TEXT, NO_TAG},
{NO_TAG, DEFAULT_COMBO_TEXT, NO_TAG},
{NO_TAG, TOTAL_TEXT_STRINGS, NO_TAG}
};

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------
void LoadRecordMenu(INPUT_MSG_STRUCT);
void LoadRecordMenuProc(INPUT_MSG_STRUCT, WND_LAYOUT_STRUCT*, MN_LAYOUT_STRUCT*);

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void LoadRecordMenu(INPUT_MSG_STRUCT msg)
{ 
	static WND_LAYOUT_STRUCT wnd_layout;
	static MN_LAYOUT_STRUCT mn_layout;

	LoadRecordMenuProc(msg, &wnd_layout, &mn_layout);

	if (g_activeMenu == LOAD_REC_MENU)
	{
		DisplaySelectMenu(&wnd_layout, &mn_layout, TITLE_CENTERED);
		WriteMapToLcd(g_mmap);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void LoadRecordMenuProc(INPUT_MSG_STRUCT msg, WND_LAYOUT_STRUCT *wnd_layout_ptr, MN_LAYOUT_STRUCT *mn_layout_ptr)
{
	uint8 buff[20];
	REC_EVENT_MN_STRUCT temp_rec;
	INPUT_MSG_STRUCT mn_msg;
	uint32 input;
	uint8 i;

	switch (msg.cmd)
	{
		case (ACTIVATE_MENU_CMD):
			wnd_layout_ptr->start_col = LOAD_REC_WND_STARTING_COL;
			wnd_layout_ptr->end_col = LOAD_REC_WND_END_COL;
			wnd_layout_ptr->start_row = LOAD_REC_WND_STARTING_ROW;
			wnd_layout_ptr->end_row = LOAD_REC_WND_END_ROW;

			mn_layout_ptr->curr_ln = 1;
			mn_layout_ptr->top_ln = 1;

			LoadTempMenuTable(s_loadRecordMenuTable);

			for (i = 1; i <= MAX_NUM_OF_SAVED_SETUPS; i++)
			{
				GetRecordData(&temp_rec, i, REC_TRIGGER_USER_MENU_TYPE);

				if (temp_rec.validRecord == YES)
				{
					switch (temp_rec.opMode)
					{
						case (WAVEFORM_MODE):
							sprintf((char*)buff, "%s (%s)", (char*)temp_rec.name, getLangText(SELF_TRG_TEXT));
							break;
						case (BARGRAPH_MODE):
							sprintf((char*)buff, "%s (%s)", (char*)temp_rec.name, getLangText(BAR_TEXT));
							break;
						case (COMBO_MODE):
							sprintf((char*)buff, "%s (%s)", (char*)temp_rec.name, getLangText(COMBO_TEXT));
							break;
						default:
							sprintf((char*)buff, "%s (UNK)", (char*)temp_rec.name);
							break;
					}

				}
				else
				{
					sprintf((char*)buff, "<%s>", getLangText(EMPTY_TEXT));
				}

				// Add 3 to i to offset past the default settings
				strcpy((char*)&(g_menuPtr[i + 3].data[0]), (char*)buff);
			}

			// Add in the "<END>" text to signal the end of the list
			sprintf((char*)buff, "<%s>", getLangText(END_TEXT));
			strcpy((char*)&(g_menuPtr[i + 3].data[0]), (char*)buff);
			i++;

			// Add the end string to allow other routines to match on the end of the menu
			strcpy((char*)&(g_menuPtr[i + 3].data[0]), ".end.");
			break;

		case (KEYPRESS_MENU_CMD):
			input = msg.data[0];
			switch (input)
			{
				case (ENTER_KEY):
					switch (mn_layout_ptr->curr_ln)
					{
						case (1): 
							LoadTrigRecordDefaults(&g_triggerRecord, WAVEFORM_MODE);
							break; 
						case (2): 
							LoadTrigRecordDefaults(&g_triggerRecord, BARGRAPH_MODE);
							break;
						case (3):
							LoadTrigRecordDefaults(&g_triggerRecord, COMBO_MODE);
							break;
						default:
							// Check if the first char matches a "<" for either "<EMPTY>" or "<END>"
							if (strncmp((char*)&g_menuPtr[mn_layout_ptr->curr_ln].data[0], "<", 1) == 0)
							{
								// Record slot is empty, do nothing
								return;
							}
							else
							{
								GetRecordData(&g_triggerRecord, (uint32)(mn_layout_ptr->curr_ln - 3), REC_TRIGGER_USER_MENU_TYPE);
							}
							break;
					}

					UpdateModeMenuTitle(g_triggerRecord.opMode);
					SETUP_USER_MENU_MSG(&modeMenu, MONITOR);
					JUMP_TO_ACTIVE_MENU();
					break;

				case (DELETE_KEY):
				case (MINUS_KEY):
					// Check if the current line is beyond the default entries
					if (mn_layout_ptr->curr_ln > 3)
					{
						GetRecordData(&temp_rec, (uint32)(mn_layout_ptr->curr_ln - 3), REC_TRIGGER_USER_MENU_TYPE);

						if (temp_rec.validRecord == YES)
						{
							sprintf((char*)g_spareBuffer, "%s (%s)", getLangText(DELETE_SAVED_SETUP_Q_TEXT), temp_rec.name);

							if (MessageBox(getLangText(WARNING_TEXT), (char*)g_spareBuffer, MB_YESNO) == MB_FIRST_CHOICE)
							{
								memset(&temp_rec.name, 0, sizeof(temp_rec.name));
								temp_rec.validRecord = NO;

								SaveRecordData(&temp_rec, (uint32)(mn_layout_ptr->curr_ln - 3), REC_TRIGGER_USER_MENU_TYPE);
								
								sprintf((char*)&(g_menuPtr[mn_layout_ptr->curr_ln].data[0]), "<%s>", getLangText(EMPTY_TEXT));
							}
						}
					}
					break;

				case (DOWN_ARROW_KEY): 
					MenuScroll(DOWN, SELECT_MN_WND_LNS, mn_layout_ptr);

					// Prevent the cursor from selecting the "<END>" text
					if (mn_layout_ptr->curr_ln == 18)
					{
						mn_layout_ptr->curr_ln = 17;
					}
					break;
				case (UP_ARROW_KEY): 
					MenuScroll(UP, SELECT_MN_WND_LNS, mn_layout_ptr);
					break;
				case (ESC_KEY):
					SETUP_MENU_MSG(MAIN_MENU); mn_msg.data[0] = ESC_KEY;
					JUMP_TO_ACTIVE_MENU();
					break;
				case (HELP_KEY):
					SETUP_USER_MENU_MSG(&helpMenu, CONFIG_CHOICE);
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

