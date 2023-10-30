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
#include "EventProcessing.h"
#include "Summary.h"
#include "Uart.h"
#include "Display.h"
#include "Keypad.h"
#include "TextTypes.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define SUMMARY_MN_TABLE_SIZE 		9 
#define SUMMARY_WND_STARTING_COL 	DEFAULT_COL_THREE
#define SUMMARY_WND_END_COL 		DEFAULT_END_COL
#define SUMMARY_WND_STARTING_ROW 	DEFAULT_MENU_ROW_ONE
#define SUMMARY_WND_END_ROW 		DEFAULT_MENU_ROW_SEVEN
#define SUMMARY_MN_TBL_START_LINE 	0
#define SUMMARY_MENU_ACTIVE_ITEMS	7
#define NO_EVENT_LINK	 			0xFFFFFFFF

typedef struct {
	uint32 eventNumber;
	DATE_TIME_STRUCT eventTime;
	uint8 mode;
	uint8 subMode;
	uint8 unused;
	uint8 validFlag;
} SUMMARY_MENU_EVENT_CACHE_STRUCT;

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
#include "Globals.h"
extern USER_MENU_STRUCT configMenu[];

///----------------------------------------------------------------------------
///	Local Scope Globals
///----------------------------------------------------------------------------
static uint16 s_topMenuSummaryIndex = 0;
static uint16 s_currentSummaryIndex = 0;
static uint16 s_totalRamSummaries = 0;

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------
SUMMARY_MENU_EVENT_CACHE_STRUCT* GetSummaryEventInfo(uint16 tempSummaryIndex);
void SummaryMenu(INPUT_MSG_STRUCT);
void SummaryMenuProc(INPUT_MSG_STRUCT, WND_LAYOUT_STRUCT *);
void SummaryMenuDisplay(WND_LAYOUT_STRUCT *);
void SummaryMenuScroll(char direction);
uint16 GetFirstValidRamSummaryIndex(void);
uint16 GetNextValidRamSummaryIndex(uint16 currentValidSummaryIndex);
uint16 GetPreviousValidRamSummaryIndex(uint16 currentValidSummaryIndex);
BOOLEAN CheckRamSummaryIndexForValidEventLink(uint16 ramSummaryIndex);
void cacheSummaryListEntry(uint16 tempSummaryIndex);

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SummaryMenu(INPUT_MSG_STRUCT msg)
{
	static WND_LAYOUT_STRUCT wnd_layout;
	//static MN_LAYOUT_STRUCT mn_layout;
	//BOOL mode = 0;

	SummaryMenuProc(msg, &wnd_layout);
	
	if (g_activeMenu == SUMMARY_MENU)
	{
		SummaryMenuDisplay(&wnd_layout);
		WriteMapToLcd(g_mmap);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SummaryMenuProc(INPUT_MSG_STRUCT msg, WND_LAYOUT_STRUCT *wnd_layout_ptr)
{
	INPUT_MSG_STRUCT mn_msg;
	uint16 tempSummaryIndex = 0;

	if (msg.cmd == ACTIVATE_MENU_CMD)
	{
		wnd_layout_ptr->start_col = SUMMARY_WND_STARTING_COL; /* 6 */
		wnd_layout_ptr->end_col = SUMMARY_WND_END_COL; /* 127 leaving one pixel space at the end */
		wnd_layout_ptr->start_row = SUMMARY_WND_STARTING_ROW; /* 8 */
		wnd_layout_ptr->end_row = SUMMARY_WND_END_ROW; /* 6 */

		DumpSummaryListFileToEventBuffer();

		g_summaryListMenuActive = YES;
		
		s_totalRamSummaries = g_summaryList.totalEntries;

		if (msg.data[0] == START_FROM_TOP)
		{
			// Find the first valid summary index and set the top and current index to match
			s_topMenuSummaryIndex = s_currentSummaryIndex = GetFirstValidRamSummaryIndex();
			debug("First valid ram summary index: %d\r\n", s_topMenuSummaryIndex);
			
			g_summaryListArrowChar = DOWN_ARROW_CHAR;
		}
	}
	else if (msg.cmd == KEYPRESS_MENU_CMD)
	{
		switch (msg.data[0])
		{
			case (ENTER_KEY):
				// Check if the top menu summary index represents a valid index
				if (s_topMenuSummaryIndex < s_totalRamSummaries)
				{
					// Grab the event info, assuming it's cached
					cacheSummaryListEntry(s_currentSummaryIndex);

					g_summaryEventNumber = g_summaryList.cachedEntry.eventNumber;
					g_updateResultsEventRecord = YES;

					debug("Summary menu: Calling Results Menu for Event #%d\r\n", g_summaryEventNumber);

					switch (g_summaryList.cachedEntry.mode)
					{
						case WAVEFORM_MODE:
						case MANUAL_CAL_MODE:
						case BARGRAPH_MODE: 
						case COMBO_MODE:
							SETUP_MENU_MSG(RESULTS_MENU);
							JUMP_TO_ACTIVE_MENU();
							break;

						case MANUAL_TRIGGER_MODE:
							break;

						default:
							break;
					} 
				}
				break;
				
			case (DOWN_ARROW_KEY):
				// Check if the top menu summary index represents a valid index
				if (s_topMenuSummaryIndex < s_totalRamSummaries)
				{
					SummaryMenuScroll(DOWN);
				}
				break;
			case (UP_ARROW_KEY):
				// Check if the top menu summary index represents a valid index
				if (s_topMenuSummaryIndex < s_totalRamSummaries)
				{
					SummaryMenuScroll(UP);
				}
				break;
				
			case (ESC_KEY):
				tempSummaryIndex = GetFirstValidRamSummaryIndex();
				
				// Check if the top menu summary index represents a valid index and if the current index isn't the first
				if ((s_topMenuSummaryIndex < s_totalRamSummaries) && (tempSummaryIndex != s_currentSummaryIndex))
				{
					s_topMenuSummaryIndex = s_currentSummaryIndex = tempSummaryIndex;

					g_summaryListArrowChar = DOWN_ARROW_CHAR;

					debug("Summary List: Setting Current Index to first valid Summary index\r\n");
				}
				else // We're done here
				{
					g_summaryListMenuActive = NO;
					
					SETUP_USER_MENU_MSG(&configMenu, EVENT_SUMMARIES);
					JUMP_TO_ACTIVE_MENU();
				}
				break;
			default:
				break;
		}
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SummaryMenuDisplay(WND_LAYOUT_STRUCT *wnd_layout_ptr)
{
	SUMMARY_MENU_EVENT_CACHE_STRUCT* eventInfo;
	char dateBuff[25];
	char lineBuff[30];
	char modeBuff[2];
	uint16 itemsDisplayed = 1;
	uint16 length;
	uint16 tempSummaryIndex = 0;

	// Clear the LCD map
	memset(&(g_mmap[0][0]), 0, sizeof(g_mmap));

	// Display the Title centered on the Top line
	length = (uint8)sprintf(lineBuff, "-%s-", getLangText(LIST_OF_SUMMARIES_TEXT));
	wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_ZERO;
	wnd_layout_ptr->curr_col = (uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));
	WndMpWrtString((uint8*)(&lineBuff[0]), wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

	// Setup layout
	wnd_layout_ptr->curr_row = wnd_layout_ptr->start_row;
	wnd_layout_ptr->curr_col = wnd_layout_ptr->start_col;
	wnd_layout_ptr->next_row = wnd_layout_ptr->start_row;
	wnd_layout_ptr->next_col = wnd_layout_ptr->start_col;

	// Check if s_topMenuSummaryIndex is valid
	if (s_topMenuSummaryIndex == s_totalRamSummaries)
	{
		debug("Summary List: No valid summary found for display\r\n");
		sprintf(lineBuff, "<%s>", getLangText(EMPTY_TEXT));
		WndMpWrtString((uint8*)(&lineBuff[0]), wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
		// We're done
	}
	else // s_topMenuSummaryIndex is a valid index indicating valid events in storage
	{
		tempSummaryIndex = s_topMenuSummaryIndex;

		while ((itemsDisplayed <= SUMMARY_MENU_ACTIVE_ITEMS) && (tempSummaryIndex < s_totalRamSummaries))
		{
			// Check if entry is cached to prevent long delay reading files
			eventInfo = GetSummaryEventInfo(tempSummaryIndex);

			// Clear and setup the time stamp string for the current event
			memset(&dateBuff[0], 0, sizeof(dateBuff));

			ConvertTimeStampToString(dateBuff, &(eventInfo->eventTime), REC_DATE_TIME_DISPLAY);

			// Clear and setup the mode string for the curent event
			memset(&modeBuff[0], 0, sizeof(modeBuff));

			switch (eventInfo->mode)
			{
				case WAVEFORM_MODE: 		strcpy(modeBuff, "W"); break;
				case BARGRAPH_MODE: 		strcpy(modeBuff, "B"); break;
				case COMBO_MODE:	 		strcpy(modeBuff, "C"); break;
				case MANUAL_CAL_MODE:		strcpy(modeBuff, "P"); break;
				case MANUAL_TRIGGER_MODE:	strcpy(modeBuff, "M"); break;
			}
			
			// Clear and setup the event line string for the curent event
			memset(&lineBuff[0], 0, sizeof(lineBuff));
			sprintf(lineBuff, "E%03d %s %s", (int)eventInfo->eventNumber, dateBuff, modeBuff);

			// Check if the current line is to be highlighted
			if (tempSummaryIndex == s_currentSummaryIndex)
			{
				WndMpWrtString((uint8*)(&lineBuff[0]), wnd_layout_ptr, SIX_BY_EIGHT_FONT, CURSOR_LN);
			}
			else // Print as a regular line
			{
				WndMpWrtString((uint8*)(&lineBuff[0]), wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
			}

			// Set the current row as the next row
			wnd_layout_ptr->curr_row = wnd_layout_ptr->next_row;

			// Increment the items displayed
			itemsDisplayed++;
			
			tempSummaryIndex = GetNextValidRamSummaryIndex(tempSummaryIndex);
		}

		// Check if the summary index is at the end of the list and still room on the LCD
		if ((itemsDisplayed <= SUMMARY_MENU_ACTIVE_ITEMS) && (tempSummaryIndex == s_totalRamSummaries))
		{
			debug("Summary List: End of the list\r\n");
			sprintf(lineBuff, "<%s>", getLangText(END_TEXT));
			WndMpWrtString((uint8*)(&lineBuff[0]), wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
		}
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SummaryMenuScroll(char direction)
{
	uint16 tempSummaryIndex = 0;
	uint16 compareCurrentSummaryIndex = s_currentSummaryIndex;
	uint16 i = 0;
	
	g_summaryListArrowChar = BOTH_ARROWS_CHAR;
	
	for (i = 0; i < g_keypadNumberSpeed; i++)
	{	
		if (direction == DOWN)
		{
			tempSummaryIndex = GetNextValidRamSummaryIndex(s_currentSummaryIndex);
			
			if (tempSummaryIndex < s_totalRamSummaries)
			{
				s_currentSummaryIndex = tempSummaryIndex;
				
				if ((s_currentSummaryIndex - s_topMenuSummaryIndex) >= SUMMARY_MENU_ACTIVE_ITEMS)
				{
					s_topMenuSummaryIndex = GetNextValidRamSummaryIndex(s_topMenuSummaryIndex);
				}
			}
			else // For the last item, bump the top menu summary index one to show the end of the list
			{
				if ((s_currentSummaryIndex - s_topMenuSummaryIndex) >= (SUMMARY_MENU_ACTIVE_ITEMS - 1))
				{
					s_topMenuSummaryIndex = GetNextValidRamSummaryIndex(s_topMenuSummaryIndex);
				}
			}
			
			compareCurrentSummaryIndex = GetNextValidRamSummaryIndex(s_currentSummaryIndex);

			if (compareCurrentSummaryIndex == s_totalRamSummaries)
			{
				g_summaryListArrowChar = UP_ARROW_CHAR;
			}

			//debug("Summary List: Scroll Down, Top Menu Index: %d, Current Index: %d\r\n",
			//		s_topMenuSummaryIndex, s_currentSummaryIndex);
		}
		else if (direction == UP)
		{
			tempSummaryIndex = GetPreviousValidRamSummaryIndex(s_currentSummaryIndex);
			
			if (tempSummaryIndex < s_totalRamSummaries)
			{
				s_currentSummaryIndex = tempSummaryIndex;
				
				if (s_currentSummaryIndex < s_topMenuSummaryIndex)
				{
					s_topMenuSummaryIndex = s_currentSummaryIndex;
				}
			}

			compareCurrentSummaryIndex = GetPreviousValidRamSummaryIndex(s_currentSummaryIndex);

			if (compareCurrentSummaryIndex == s_totalRamSummaries)
			{
				g_summaryListArrowChar = DOWN_ARROW_CHAR;
			}

			//debug("Summary List: Scroll Up, Top Menu Index: %d, Current Index: %d\r\n",
			//		s_topMenuSummaryIndex, s_currentSummaryIndex);
		}
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16 GetFirstValidRamSummaryIndex(void)
{
	uint16 ramSummaryIndex = 0;
	
	while ((ramSummaryIndex < s_totalRamSummaries) && (CheckRamSummaryIndexForValidEventLink(ramSummaryIndex) == NO))
		ramSummaryIndex++;

	return (ramSummaryIndex);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16 GetNextValidRamSummaryIndex(uint16 currentValidSummaryIndex)
{
	uint16 ramSummaryIndex = currentValidSummaryIndex;

	if (ramSummaryIndex < s_totalRamSummaries)
	{
		while (CheckRamSummaryIndexForValidEventLink(++ramSummaryIndex) == NO)
		{
			if (ramSummaryIndex == s_totalRamSummaries)
			{
				break;
			}
		}
	}

	return (ramSummaryIndex);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16 GetPreviousValidRamSummaryIndex(uint16 currentValidSummaryIndex)
{
	uint16 ramSummaryIndex = currentValidSummaryIndex;

	if (ramSummaryIndex == 0)
	{
		ramSummaryIndex = s_totalRamSummaries;
	}
	else
	{
		while (CheckRamSummaryIndexForValidEventLink(--ramSummaryIndex) == NO)
		{
			if (ramSummaryIndex == 0)
			{
				ramSummaryIndex = s_totalRamSummaries;
				break;
			}
		}
	}

	return (ramSummaryIndex);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
BOOLEAN CheckRamSummaryIndexForValidEventLink(uint16 ramSummaryIndex)
{
	BOOLEAN validEventLink = NO;
	SUMMARY_LIST_ENTRY_STRUCT* summaryListCache = (SUMMARY_LIST_ENTRY_STRUCT*)&g_eventDataBuffer[0];

	if (ramSummaryIndex < s_totalRamSummaries)
	{
		if ((summaryListCache[ramSummaryIndex].eventNumber != 0) && (g_eventNumberCache[summaryListCache[ramSummaryIndex].eventNumber] == EVENT_REFERENCE_VALID))
		{
			validEventLink = YES;
		}
	}

	return (validEventLink);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
SUMMARY_MENU_EVENT_CACHE_STRUCT* GetSummaryEventInfo(uint16 tempSummaryIndex)
{
	static SUMMARY_MENU_EVENT_CACHE_STRUCT summaryMenuCacheEntry;
	SUMMARY_LIST_ENTRY_STRUCT* summaryListCache = (SUMMARY_LIST_ENTRY_STRUCT*)&g_eventDataBuffer[0];

	//debug("Summary Menu: Get Summary Info for index: %d\r\n", tempSummaryIndex);

	summaryMenuCacheEntry.eventNumber = summaryListCache[tempSummaryIndex].eventNumber;
	summaryMenuCacheEntry.mode = summaryListCache[tempSummaryIndex].mode;
	summaryMenuCacheEntry.subMode = summaryListCache[tempSummaryIndex].subMode;
	summaryMenuCacheEntry.eventTime = summaryListCache[tempSummaryIndex].eventTime;
	summaryMenuCacheEntry.validFlag = YES;

	return (&summaryMenuCacheEntry);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void cacheSummaryListEntry(uint16 tempSummaryIndex)
{
	SUMMARY_LIST_ENTRY_STRUCT* summaryListCache = (SUMMARY_LIST_ENTRY_STRUCT*)&g_eventDataBuffer[0];

	memcpy(&g_summaryList.cachedEntry, &summaryListCache[tempSummaryIndex], sizeof(SUMMARY_LIST_ENTRY_STRUCT));
}
