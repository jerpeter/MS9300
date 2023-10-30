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
#define OVERWRITE_MN_TABLE_SIZE		8 
#define OVERWRITE_WND_STARTING_COL	DEFAULT_COL_THREE
#define OVERWRITE_WND_END_COL		DEFAULT_END_COL 
#define OVERWRITE_WND_STARTING_ROW	DEFAULT_MENU_ROW_ONE
#define OVERWRITE_WND_END_ROW		DEFAULT_MENU_ROW_SEVEN

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
#include "Globals.h"
extern USER_MENU_STRUCT modeMenu[];
extern USER_MENU_STRUCT helpMenu[];

///----------------------------------------------------------------------------
///	Local Scope Globals
///----------------------------------------------------------------------------
static TEMP_MENU_DATA_STRUCT s_overwriteMenuTable[OVERWRITE_MN_TABLE_SIZE] = {
{TITLE_PRE_TAG, OVERWRITE_SETTINGS_TEXT, TITLE_POST_TAG},
{NO_TAG, DEFAULT_SELF_TRG_TEXT, NO_TAG},
{NO_TAG, DEFAULT_BAR_TEXT, NO_TAG},
{NO_TAG, DEFAULT_COMBO_TEXT, NO_TAG},
{NO_TAG, TOTAL_TEXT_STRINGS, NO_TAG}
};

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------
void OverwriteMenu(INPUT_MSG_STRUCT);
void OverwriteMenuProc(INPUT_MSG_STRUCT, WND_LAYOUT_STRUCT*, MN_LAYOUT_STRUCT*);

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void OverwriteMenu(INPUT_MSG_STRUCT msg)
{ 
	static WND_LAYOUT_STRUCT wnd_layout;
	static MN_LAYOUT_STRUCT mn_layout;

	OverwriteMenuProc(msg, &wnd_layout, &mn_layout);

	if (g_activeMenu == OVERWRITE_MENU)
	{
		DisplaySelectMenu(&wnd_layout, &mn_layout, TITLE_CENTERED);
		WriteMapToLcd(g_mmap);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void OverwriteMenuProc(INPUT_MSG_STRUCT msg, WND_LAYOUT_STRUCT *wnd_layout_ptr, MN_LAYOUT_STRUCT *mn_layout_ptr)
{
	char buff[30];
	REC_EVENT_MN_STRUCT temp_rec;
	INPUT_MSG_STRUCT mn_msg;
	uint32 input;
	uint8 i;

	switch (msg.cmd)
	{
		case (ACTIVATE_MENU_WITH_DATA_CMD):
			wnd_layout_ptr->start_col = OVERWRITE_WND_STARTING_COL; /* 6 */
			wnd_layout_ptr->end_col = OVERWRITE_WND_END_COL; /* 127 leaving one pixel space at the end*/
			wnd_layout_ptr->start_row = OVERWRITE_WND_STARTING_ROW; /*/ 8*/
			wnd_layout_ptr->end_row = OVERWRITE_WND_END_ROW; /* 6 */

			mn_layout_ptr->curr_ln = 1;
			mn_layout_ptr->top_ln = 1;

			LoadTempMenuTable(s_overwriteMenuTable);

			for (i = 1; i <= MAX_NUM_OF_SAVED_SETUPS; i++)
			{
				GetRecordData(&temp_rec, i, REC_TRIGGER_USER_MENU_TYPE);

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
						sprintf((char*)buff, "%s (UNK)",(char*)temp_rec.name);
						break;
				}

				strcpy((char*)&(g_menuPtr[i].data[0]), (char*)buff);
			}

			// Add in the end string
			strcpy((char*)&(g_menuPtr[i].data[0]), ".end.");
		break;

		case (KEYPRESS_MENU_CMD):
			input = msg.data[0];

			switch (input)
			{
				case (ENTER_KEY):
					SaveRecordData(&g_triggerRecord, mn_layout_ptr->curr_ln, REC_TRIGGER_USER_MENU_TYPE);

					UpdateModeMenuTitle(g_triggerRecord.opMode);
					SETUP_USER_MENU_MSG(&modeMenu, MONITOR);
					JUMP_TO_ACTIVE_MENU();
					break;

				case (DELETE_KEY):
					GetRecordData(&temp_rec, mn_layout_ptr->curr_ln, REC_TRIGGER_USER_MENU_TYPE);

					if (temp_rec.validRecord == YES)
					{
						sprintf((char*)g_spareBuffer, "%s (%s)", getLangText(DELETE_SAVED_SETUP_Q_TEXT), temp_rec.name);

						if (MessageBox(getLangText(WARNING_TEXT), (char*)g_spareBuffer, MB_YESNO) == MB_FIRST_CHOICE)
						{
							memset(&temp_rec.name, 0, sizeof(temp_rec.name));
							temp_rec.validRecord = NO;

							SaveRecordData(&temp_rec, mn_layout_ptr->curr_ln, REC_TRIGGER_USER_MENU_TYPE);

							sprintf((char*)&(g_menuPtr[mn_layout_ptr->curr_ln].data[0]), "<%s>", getLangText(EMPTY_TEXT));
						}
					}
					break;

				case (DOWN_ARROW_KEY):
					MenuScroll(DOWN,SELECT_MN_WND_LNS,mn_layout_ptr);
					break;

				case (UP_ARROW_KEY):
					MenuScroll(UP,SELECT_MN_WND_LNS,mn_layout_ptr);
					break;

				case (ESC_KEY):
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
