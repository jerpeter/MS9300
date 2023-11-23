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
#include <math.h>
#include "Typedefs.h"
#include "Common.h"
#include "Menu.h"
#include "PowerManagement.h"
#include "OldUart.h"
#include "RealTimeClock.h"
#include "SysEvents.h"
#include "Summary.h"
#include "EventProcessing.h"
#include "Display.h"
#include "Keypad.h"
#include "Font_Six_by_Eight_table.h"
#include "TextTypes.h"
#include "RemoteCommon.h"
//#include "usart.h"
//#include "usb_task.h"
//#include "device_mass_storage_task.h"
//#include "host_mass_storage_task.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define END_DATA_STRING_SIZE 6
#define MIN_CHAR_STRING 0x41
#define MIN_NUM_STRING 0x2E
#define MAX_CHAR_STRING 0x5a
#define MAX_NUM_STRING 0x39
#define INPUT_MENU_SECOND_LINE_INDENT 6

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
#include "Globals.h"
extern USER_MENU_STRUCT modeMenu[];

///----------------------------------------------------------------------------
///	Local Scope Globals
///----------------------------------------------------------------------------
static MB_CHOICE s_MessageChoices[MB_TOTAL_CHOICES] =
{
	//{Num Choices,		1st/Single,	2nd Choice,	}
	//{MB_ONE_CHOICE,	"OK\0",		"\0"		},
	//{MB_TWO_CHOICES,	"YES\0",	"NO\0"		},
	//{MB_TWO_CHOICES,	"OK\0",		"CANCEL\0"	}
	{MB_ONE_CHOICE,		OK_TEXT,	NULL_TEXT	},
	{MB_TWO_CHOICES,	YES_TEXT,	NO_TEXT		},
	{MB_TWO_CHOICES,	OK_TEXT,	CANCEL_TEXT	}
	// Add new s_MessageChoices entry for new choices aboove this line
};

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void LoadTempMenuTable(TEMP_MENU_DATA_STRUCT* currentMenu)
{
	uint16 i = 0;

	while (currentMenu[i].textEntry != TOTAL_TEXT_STRINGS)
	{
		sprintf((char*)g_menuPtr[i].data, "%s%s%s", g_menuTags[currentMenu[i].preTag].text,
				getLangText(currentMenu[i].textEntry), g_menuTags[currentMenu[i].postTag].text);
		i++;
	}

	strcpy((char*)g_menuPtr[i].data, ".end.");
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MenuScroll(char direction, char wnd_size, MN_LAYOUT_STRUCT* mn_layout_ptr)
{
	strcpy((char*)g_spareBuffer, (char*)(g_menuPtr + mn_layout_ptr->curr_ln + 1)->data);

	switch (direction)
	{
		case (DOWN):
			if (strcmp((char*)g_spareBuffer, ".end."))
			{
				mn_layout_ptr->curr_ln++;

				if ((mn_layout_ptr->curr_ln - mn_layout_ptr->top_ln) >= wnd_size)
				{
					mn_layout_ptr->top_ln++;
				}
			}
			break;

		case (UP):
			if (mn_layout_ptr->curr_ln > 1)
			{
				if (mn_layout_ptr->curr_ln == mn_layout_ptr->top_ln)
				{
					if (mn_layout_ptr->top_ln > 1)
					{
						mn_layout_ptr->top_ln--;
					}
				}

				mn_layout_ptr->curr_ln--;
			}
			break;

		default:
			break;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UserMenuScroll(uint32 direction, char wnd_size, MN_LAYOUT_STRUCT* mn_layout_ptr)
{
	strcpy((char*)g_spareBuffer, (g_userMenuCachePtr + mn_layout_ptr->curr_ln + 1)->text);

	switch (direction)
	{
		case (DOWN_ARROW_KEY):
			if (strcmp((char*)g_spareBuffer, ".end."))
			{
				mn_layout_ptr->curr_ln++;

				if ((mn_layout_ptr->curr_ln - mn_layout_ptr->top_ln) >= wnd_size)
				{
					mn_layout_ptr->top_ln++;
				}
			}
		break;

		case (UP_ARROW_KEY):
			if (mn_layout_ptr->curr_ln > 1)
			{
				if (mn_layout_ptr->curr_ln == mn_layout_ptr->top_ln)
				{
					if (mn_layout_ptr->top_ln > 1)
					{
						mn_layout_ptr->top_ln--;
					}
				}

				mn_layout_ptr->curr_ln--;
			}
		break;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DisplaySelectMenu(WND_LAYOUT_STRUCT *wnd_layout_ptr, MN_LAYOUT_STRUCT *mn_layout_ptr, uint8 titlePosition)
{
	uint8 buff[50];
	uint8 top;
	uint8 menu_ln;
	uint32 length;

	memset(&(g_mmap[0][0]), 0, sizeof(g_mmap));

	menu_ln = 0;
	top = 0;
	strcpy((char*)buff, (char*)(g_menuPtr + top + menu_ln)->data);
	length = strlen((char*)buff);
	wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_ZERO;

	if (titlePosition == TITLE_LEFT_JUSTIFIED)
	{
		wnd_layout_ptr->curr_col = wnd_layout_ptr->start_col;
	}
	else // titlePosition is TITLE_CENTERED
	{
		wnd_layout_ptr->curr_col =(uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));
	}

	WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

	menu_ln = 0;
	top = (uint8)mn_layout_ptr->top_ln;
	wnd_layout_ptr->curr_row = wnd_layout_ptr->start_row;
	wnd_layout_ptr->curr_col = wnd_layout_ptr->start_col;
	wnd_layout_ptr->next_row = wnd_layout_ptr->start_row;
	wnd_layout_ptr->next_col = wnd_layout_ptr->start_col;

	while (wnd_layout_ptr->curr_row <= wnd_layout_ptr->end_row)
	{
		strcpy((char*)buff, (char*)(g_menuPtr + top + menu_ln)->data);
		if (strcmp((char*)buff, ".end."))
		{
			if (menu_ln == (mn_layout_ptr->curr_ln - mn_layout_ptr->top_ln))
			{
				if (mn_layout_ptr->sub_ln == 0)
				{
					WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, CURSOR_LN);
				}
				else // sub_ln > 0
				{
					wnd_layout_ptr->index = mn_layout_ptr->sub_ln;
					WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, CURSOR_CHAR);
				}
			}
			else
			{
				WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
			}
		}
		else
			break;

		wnd_layout_ptr->curr_row = wnd_layout_ptr->next_row;
		menu_ln++;
	}/* END OF WHILE LOOP */
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DisplayUserMenu(WND_LAYOUT_STRUCT *wnd_layout_ptr, MN_LAYOUT_STRUCT *mn_layout_ptr, uint8 titlePosition)
{
	uint8 buff[50]; /* made it bigger then NUM_CHAR_PER_LN just in case someone trys to make a big string.*/
	uint16 top;
	uint8 menu_ln;
	uint32 length;

	// Clear out LCD map buffer
	memset(&(g_mmap[0][0]), 0, sizeof(g_mmap));

	// Init var's
	menu_ln = 0;
	top = 0;

	// Copy Menu Title into buffer
	strcpy((char*)(&buff[0]), (g_userMenuCachePtr + top + menu_ln)->text);

	// Get length of title
	length = strlen((char*)(&buff[0]));

	// Set current row to the top row
	wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_ZERO;

	// Check if title position is specified as left justified
	if (titlePosition == TITLE_LEFT_JUSTIFIED)
	{
		wnd_layout_ptr->curr_col = wnd_layout_ptr->start_col;
	}
	else // titlePosition is TITLE_CENTERED
	{
		wnd_layout_ptr->curr_col =(uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));
	}

	// Write string to LCD map
	WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

	// Reset var's... purpose?
	menu_ln = 0;
	top = mn_layout_ptr->top_ln;

	// Reset menu display parameters
	wnd_layout_ptr->curr_row = wnd_layout_ptr->start_row;
	wnd_layout_ptr->curr_col = wnd_layout_ptr->start_col;
	wnd_layout_ptr->next_row = wnd_layout_ptr->start_row;
	wnd_layout_ptr->next_col = wnd_layout_ptr->start_col;

	// Handle the rest of the menu table
	while (wnd_layout_ptr->curr_row <= wnd_layout_ptr->end_row)
	{
		// Copy the next menu text into the buffer
		strcpy((char*)(&buff[0]), (g_userMenuCachePtr + top + menu_ln)->text);

		// Check if we have reached the end of the menu text
		if (strcmp((char*)(&buff[0]), ".end."))
		{
			// If the incrementing menu line matches the current line minus the top line
			if (menu_ln == (mn_layout_ptr->curr_ln - mn_layout_ptr->top_ln))
			{
				if (mn_layout_ptr->sub_ln == 0)
				{
					// Write the text to the LCD map highlighted
					WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, CURSOR_LN);
				}
				else // sub_ln > 0
				{
					// Write just one char highlighted
					wnd_layout_ptr->index = mn_layout_ptr->sub_ln;
					WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, CURSOR_CHAR);
				}
			}
			else // Write text as a regular line
			{
				WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
			}
		}
		else // Reached end of menu text
			break;

		// Set current row to already advanced next row
		wnd_layout_ptr->curr_row = wnd_layout_ptr->next_row;

		// Advance menu line
		menu_ln++;
	}
}

/*
New LCD display steps, LCD map clear replacement
	Start stream -- ft81x_stream_start();
	Set color -- ft81x_color_rgb32();
	Set foreground color -- ft81x_fgcolor_rgb32();
	Set background color -- ft81x_bgcolor_rgb32();
*/

/*
WndMpWrtString replacement
	if line type is cursor line (CURSOR_LN)
		Swap foreground color
		Swap background color
	Adjust the layout coordinates to the new LCD size (positioned top left)
	Draw text at layout coordinates -- ft81x_cmd_text();
	When finished, if line type is cursor line (CURSOR_LN)
		Reset foreground color
		Reset background color
*/

/*
Write map to LCD replacement
	End the display list -- ft81x_display();
	Trigger FT81x to read the command buffer -- ft81x_getfree(0);
	Finish streaming to command buffer -- ft81x_stream_stop();

	Wait till the GPU is finished?? -- ft81x_wait_finish();
*/

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void WndMpWrtString(uint8* buff, WND_LAYOUT_STRUCT* wnd_layout, int font_type, int ln_type)
{
	const uint8 (*fmap_ptr)[FONT_MAX_COL_SIZE];
	uint8 mmcurr_row;
	uint8 mmend_row;
	uint8 mmcurr_col;
	uint8 mmend_col;
	uint8 cbit_size;
	uint8 crow_size;
	uint8 ccol_size;
	uint8 temp1 = 0;
	int32 index;
	int32 row;
	int32 col;
	int32 bits_wrtn;
	int32 first_column;

	first_column = 0;

	switch (font_type)
	{

		case SIX_BY_EIGHT_FONT:
			cbit_size = EIGHT_ROW_SIZE;
			crow_size = (uint8)(cbit_size / 8);
			if (EIGHT_ROW_SIZE % 8)
				crow_size++;
			ccol_size = SIX_COL_SIZE;
			fmap_ptr = font_table_68;

			wnd_layout->next_row = (uint16)(wnd_layout->curr_row + cbit_size);
			wnd_layout->next_col = (uint16)(wnd_layout->curr_col + ccol_size);
			break;

		default:
			break;
	}

	mmcurr_row = (uint8)(wnd_layout->curr_row /8);
	if (wnd_layout->curr_row %8)
		mmcurr_row++;

	mmend_row = (uint8)(wnd_layout->end_row /8);
	if (wnd_layout->end_row %8)
		mmend_row++;

	mmcurr_col = (uint8)(wnd_layout->curr_col);
	mmend_col = (uint8)wnd_layout->end_col;

	index = 0;

	// While not at the end of the string
	while (buff[index] != '\0')
	{
		// Loop through the pixel width (columns) of the font
		for (row = 0, col = 0; col < ccol_size; col++)
		{
			// Check that the column and row are within range
			if (((col + mmcurr_col) <= mmend_col) && ((row + mmcurr_row) <= mmend_row))
			{
				if ((ln_type == REG_LN) || (ln_type == CURSOR_CHAR))
				{
					// Get the font table bitmap for each column
					g_mmap[mmcurr_row + row][mmcurr_col + col] |= fmap_ptr[buff[index]][col] << (wnd_layout->curr_row %8);
				}
				else if (ln_type == CURSOR_LN)
				{
					// Get the inverse of the font table bitmap for the column
					temp1 = (uint8)~(fmap_ptr[buff[index]][col]);

					// Write the inverse bitmap into the buffer
					g_mmap[mmcurr_row + row][mmcurr_col + col] |= temp1 << (wnd_layout->curr_row %8);
					// Write the last pixel highlighted (reversed) in each column for the previous row
					g_mmap[mmcurr_row + row -1][mmcurr_col + col] |= 0x80;

					// Check if this is the first char
					if (first_column == 0)
					{
						// pre-highlight (reverse) the column just before the first char
						g_mmap[mmcurr_row + row][mmcurr_col + col -1] |= 0xFF;
						// pre-highlight (reverse) the last pixel in the column for the previous row
						g_mmap[mmcurr_row + row -1][mmcurr_col + col -1] |= 0x80;

						// Increment to prevent accessing this code again
						first_column++;
					}
				}
			}
		}

		// See if character externds past current row
		bits_wrtn = (8 - (wnd_layout->curr_row %8));
		if (bits_wrtn >= cbit_size)
		{
			row++;
		}

		// Finish writing the rest of the character
		for (; row < crow_size; row++)
		{
			for (col = 0; col < ccol_size; col++)
			{
				if ((ln_type == REG_LN) || (ln_type == CURSOR_CHAR))
				{
					if (((col + mmcurr_col) <= mmend_col) && ((row + mmcurr_row) <= mmend_row))
					{
						temp1 = (uint8)((fmap_ptr[buff[index]][col] >> (8 - (wnd_layout->curr_row %8))));
						g_mmap[mmcurr_row + row + 1][mmcurr_col + col] = temp1;
					}
				}
				else if (ln_type == CURSOR_LN)
				{
					if (((col + mmcurr_col) <= mmend_col) && ((row + mmcurr_row) <= mmend_row))
					{
						g_mmap[mmcurr_row + row + 1][mmcurr_col + col] = (uint8)(temp1 & (0xff >> (cbit_size - (cbit_size - bits_wrtn))));

						temp1 = (uint8)((fmap_ptr[buff[index]][col] >> (8 - (wnd_layout->curr_row %8))));
						g_mmap[mmcurr_row + row + 1][mmcurr_col + col] = temp1;
					}
				}
			}
		}

		// Increment to the next column
		mmcurr_col += ccol_size;
		// Increment the character index
		index++;
	}

	if (ln_type == CURSOR_CHAR)
	{
		mmcurr_col = (uint8)((wnd_layout->curr_col) + ((wnd_layout->index - 1) * ccol_size));

		if ((mmcurr_col > 0) && (mmcurr_row > 0))
		{
			// Set the column before the char reversed
			g_mmap[mmcurr_row][mmcurr_col - 1] |= 0xff;
			// Set the previous row last pixel before the char reversed
			g_mmap[mmcurr_row - 1][mmcurr_col - 1] |= 0x80;
		}

		// Loop through the pixel width (columns) of the cursor char
		for (row = 0, col = 0; col < ccol_size; col++)
		{
			// Check that the column and row are within range
			if (((col + mmcurr_col) <= mmend_col) && ((row + mmcurr_row) <= mmend_row))
			{
				temp1 = (uint8)~g_mmap[mmcurr_row + row][mmcurr_col + col];

				// Get the font table bitmap for each column
				g_mmap[mmcurr_row + row][mmcurr_col + col] = temp1;

				if ((mmcurr_row + row - 1) >= 0)
				{
					// Write the last pixel highlighted (reversed) in each column for the previous row
					g_mmap[mmcurr_row + row - 1][mmcurr_col + col] |= 0x80;
				}
			}
		}
	}

	// Store next column location
	wnd_layout->next_col = mmcurr_col;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MessageBorder(void)
{
	uint8 i = 0;

	// Top and bottom horizontal bars
	for (i = 12; i < 116; i++)
	{
		g_mmap[0][i] |= 0xe0;
		g_mmap[7][i] |= 0x07;
	}

	// Left and right vertical bars
	for (i = 1; i < 7; i++)
	{
		g_mmap[i][9] = 0xff;
		g_mmap[i][10] = 0xff;
		g_mmap[i][11] = 0xff;
		g_mmap[i][116] = 0xff;
		g_mmap[i][117] = 0xff;
		g_mmap[i][118] = 0xff;
	}

	// Rounded ends
	g_mmap[0][9] |= 0x80;		g_mmap[0][10] |= 0xc0;	g_mmap[0][11] |= 0xe0;
	g_mmap[0][116] |= 0xe0;	g_mmap[0][117] |= 0xc0;	g_mmap[0][118] |= 0x80;
	g_mmap[7][9] |= 0x01;		g_mmap[7][10] |= 0x03;	g_mmap[7][11] |= 0x07;
	g_mmap[7][116] |= 0x07;	g_mmap[7][117] |= 0x03;	g_mmap[7][118] |= 0x01;

	// Clear inside message box area minus title area (highlighted)
	for (i = 12; i < 116; i++)
	{
		g_mmap[2][i] = 0x00;
		g_mmap[3][i] = 0x00;
		g_mmap[4][i] = 0x00;
		g_mmap[5][i] = 0x00;
		g_mmap[6][i] = 0x00;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MessageTitle(char* titleString)
{
	uint8 i = 0, j = 0;
	uint8 length = 0;
	uint8 textPosition = 0;

	// Find starting pixel length of title
	length = (uint8)(strlen((char*)titleString) * 3); // result in pixel width, 3 = 6 (font width) / 2
	if (length > 52)
		textPosition = 12;
	else
		textPosition = (uint8)(64 - length);

	// Reverse fill up to the beginning of the title
	for (i = 12; i < textPosition; i++)
		g_mmap[1][i] = 0xff;

	// Write the title (highlighted)
	i = 0;
	while ((titleString[i] != '\0') && (textPosition < 116))
	{
		g_mmap[1][textPosition++] = (uint8)~font_table_68[(uint8)(titleString[i])][j++];
		if (j == 6) {i++; j = 0;}
	}

	// Reverse fill from title to end of row
	for (i = textPosition; i < 116; i++)
		g_mmap[1][i] = 0xff;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MessageText(char* textString)
{
	uint8 i = 0, j = 0;
	uint8 textPosition = 0;
	uint8 length = 0, subLength = 0;

	// ======================
	// Setup 1st line Indexes
	// ======================
	// Find out if message text goes beyond 1 line (17 chars)
	length = subLength = (uint8)strlen((char*)textString);
	if (length > 17) // max 17 chars per line
	{
		subLength = 17;

		// Look for a space
		while (textString[subLength-1] != ' ')
		{
			if (--subLength == 0)
				break;
		}

		// If no space was found, use the max length
		if (subLength == 0)
			subLength = 17;
	}

	// =========================
	// Write 1st line of Message
	// =========================
	textPosition = 13; // Shifted by one pixel since 2 extra pixel rows

	// Write in the number of characters that will fit on the 1st line
	for (i = 0, j = 0; i < subLength;)
	{
		g_mmap[2][textPosition++] = font_table_68[(uint8)(textString[i])][j++];
		// When 6 pixel columns have been written, advance to the next character
		if (j == 6) {i++; j = 0;}
	}

	// ======================
	// Setup 2nd line Indexes
	// ======================
	// Find out if total length minus the current index goes beyond the 2nd line
	if ((length-i) > 17)
	{
		// New sub length is a max of the current index plus 17
		subLength = (uint8)(i + 17);

		// Look for a space
		while ((textString[subLength-1] != ' ') && (subLength > i))
		{
			// If the subLength equals the current index, leave
			if (--subLength == i)
				break;
		}

		// If no space was found, use the index plus 17
		if (subLength == i)
			subLength = (uint8)(i + 17);
	}
	else subLength = length;

	// =========================
	// Write 2nd line of Message
	// =========================
	textPosition = 13; // Shifted by one pixel since 2 extra pixel rows

	// Write in the number of characters that will fit on the 2nd line
	for (j = 0; i < subLength;)
	{
		g_mmap[3][textPosition++] = font_table_68[(uint8)(textString[i])][j++];
		// When 6 pixel columns have been written, advance to the next character
		if (j == 6) {i++; j = 0;}
	}

	// ======================
	// Setup 3rd line Indexes
	// ======================
	// Find out if total length minus the current index goes beyond the 2nd line
	if ((length-i) > 17)
	{
		// New sub length is a max of the current index plus 17
		subLength = (uint8)(i + 17);

		// Look for a space
		while ((textString[subLength-1] != ' ') && (subLength > i))
		{
			// If the subLength equals the current index, leave
			if (--subLength == i)
				break;
		}

		// If no space was found, use the index plus 17
		if (subLength == i)
			subLength = (uint8)(i + 17);
	}
	else subLength = length;

	// =========================
	// Write 3nd line of Message
	// =========================
	textPosition = 13; // Shifted by one pixel since 2 extra pixel rows

	// Write in the number of characters that will fit on the 3rd line
	for (j = 0; i < subLength;)
	{
		g_mmap[4][textPosition++] = font_table_68[(uint8)(textString[i])][j++];
		// When 6 pixel columns have been written, advance to the next character
		if (j == 6) {i++; j = 0;}
	}

	// =============================================================
	// Setup 4th line Indexes (only valid for message with 1 choice)
	// =============================================================
	// Find out if total length minus the current index goes beyond the 3rd line
	if ((length-i) > 17)
		subLength = (uint8)(i + 17);
	else subLength = length;

	// ================================================================
	// Write 4th line of Message (only valid for message with 1 choice)
	// ================================================================
	textPosition = 13; // Shifted by one pixel since 2 extra pixel rows
	for (j = 0; i < subLength;)
	{
		g_mmap[5][textPosition++] = font_table_68[(uint8)(textString[i])][j++];
		// When 6 pixel columns have been written, advance to the next character
		if (j == 6) {i++; j = 0;}
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MessageChoice(MB_CHOICE_TYPE choiceType)
{
	uint8 i = 0, j = 0;
	uint8 text1Position = 0, text2Position = 0, startPosition = 0;
	uint8 startRow = 0;
	char firstChoiceText[30];
	char secondChoiceText[30];

	strcpy((char*)firstChoiceText, getLangText(s_MessageChoices[choiceType].firstTextEntry));
	strcpy((char*)secondChoiceText, getLangText(s_MessageChoices[choiceType].secondTextEntry));

	// 64 = half screen, char len * 3 = char width*6(pixel width)/2(half)
	text1Position = (uint8)(64 - strlen((char*)firstChoiceText) * 3);
	text2Position = (uint8)(64 - strlen((char*)secondChoiceText) * 3);

	// Find starting pixel position with extra char space, 6 = extra char space in pixel width
	startPosition = (uint8)((text1Position < text2Position ? text1Position : text2Position) - 6);

	if (s_MessageChoices[choiceType].numChoices == MB_ONE_CHOICE)
		startRow = 6;
	else // s_MessageChoices[choiceType].numChoices == MB_TWO_CHOICES
		startRow = 5;

	// Clear out unused portion of first choice row in case message text ran long
	for (i = 12; i < startPosition; i++)
	{
		g_mmap[startRow][i] = 0x00;
		g_mmap[startRow][127-i] = 0x00;
	}

	// Highlight extra char space before and after text
	for (i = startPosition; i < text1Position; i++)
	{
		g_mmap[startRow-1][i] |= 0x80;
		g_mmap[startRow][i] = 0xff;
		g_mmap[startRow-1][127-i] |= 0x80;
		g_mmap[startRow][127-i] = 0xff;
	}

	// Display first choice
	i = 0;
	while ((firstChoiceText[i] != '\0') && (text1Position < 116))
	{
		// Steal pixel line for active (reversed) choice
		g_mmap[startRow-1][text1Position] |= 0x80;
		// Write in text (highlighted)
		g_mmap[startRow][text1Position++] = (uint8)~font_table_68[(uint8)(firstChoiceText[i])][j++];
		// When 6 pixel columns have been written, advance to the next character
		if (j == 6) {i++; j = 0;}
	}

	// Add active choice round ends
	g_mmap[startRow][startPosition - 2] = 0x3e;
	g_mmap[startRow][startPosition - 1] = 0x7f;
	g_mmap[startRow][128 - startPosition] = 0x7f;
	g_mmap[startRow][128 - startPosition + 1] = 0x3e;

	if (s_MessageChoices[choiceType].numChoices == MB_TWO_CHOICES)
	{
		// Display second choice
		i = 0;
		while ((secondChoiceText[i] != '\0') && (text2Position < 116))
		{
			// Write in the text (plain)
			g_mmap[6][text2Position++] = font_table_68[(uint8)(secondChoiceText[i])][j++];
			// When 6 pixel columns have been written, advance to the next character
			if (j == 6) {i++; j = 0;}
		}
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MessageChoiceActiveSwap(MB_CHOICE_TYPE choiceType)
{
	uint8 i = 0;
	uint8 text1Position = 0, text2Position = 0, startPosition = 0;
	char firstChoiceText[30];
	char secondChoiceText[30];

	strcpy((char*)firstChoiceText, getLangText(s_MessageChoices[choiceType].firstTextEntry));
	strcpy((char*)secondChoiceText, getLangText(s_MessageChoices[choiceType].secondTextEntry));

	// 64 = half screen, char len * 3 = char width*6(pixel width)/2(half)
	text1Position = (uint8)(64 - strlen((char*)firstChoiceText) * 3);
	text2Position = (uint8)(64 - strlen((char*)secondChoiceText) * 3);

	// Find starting pixel position with extra char space, 6 = extra char space in pixel width
	startPosition = (uint8)((text1Position < text2Position ? text1Position : text2Position) - 6);

	for (i = startPosition; i < (128-startPosition); i++)
	{
		// Toggle bottom pixel line of 4th row
		g_mmap[4][i] ^= 0x80;
		// Inverse the row/text leaving the bottom pixel row active (highlighted)
		g_mmap[5][i] = (uint8)(~g_mmap[5][i] | 0x80);
		// Inverse the row/text
		g_mmap[6][i] = (uint8)~g_mmap[6][i];
	}

	// Xor the round ends of the choices to invert them
	g_mmap[5][startPosition-2] ^= 0x3e;
	g_mmap[5][startPosition-1] ^= 0x7f;
	g_mmap[5][128-startPosition] ^= 0x7f;
	g_mmap[5][128-startPosition+1] ^= 0x3e;
	g_mmap[6][startPosition-2] ^= 0x3e;
	g_mmap[6][startPosition-1] ^= 0x7f;
	g_mmap[6][128-startPosition] ^= 0x7f;
	g_mmap[6][128-startPosition+1] ^= 0x3e;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 MessageBox(char* titleString, char* textString, MB_CHOICE_TYPE choiceType)
{
	uint8 activeChoice = MB_FIRST_CHOICE;
	volatile uint16 key = 0;

#if 0 /* ET test */
	g_debugBufferCount = 1;
	strcpy((char*)g_debugBuffer, textString);
#endif

	// Build MessageBox into g_mmap with the following calls
	MessageBorder();
	MessageTitle(titleString);
	MessageText(textString);
	MessageChoice(choiceType);

	WriteMapToLcd(g_mmap);

	debug("MB: Look for a key\r\n");

	// Loop forever unitl an enter or escape key is found
	while ((key != ENTER_KEY) && (key != ESC_KEY) && (key != ON_ESC_KEY))
	{
		// Blocking call to wait for a key to be pressed on the keypad
		key = GetKeypadKey(WAIT_FOR_KEY);

		// Check if there are two choices
		if (s_MessageChoices[choiceType].numChoices == MB_TWO_CHOICES)
		{
			switch (key)
			{
				case UP_ARROW_KEY:
					// Check if the active choice is the second/bottom choice
					if (activeChoice == MB_SECOND_CHOICE)
					{
						// Swap the active choice
						MessageChoiceActiveSwap(choiceType);
						WriteMapToLcd(g_mmap);

						activeChoice = MB_FIRST_CHOICE;
					}
					break;
				case DOWN_ARROW_KEY:
					// Check if the active choice is the first/top choice
					if (activeChoice == MB_FIRST_CHOICE)
					{
						// Swap the active choice
						MessageChoiceActiveSwap(choiceType);
						WriteMapToLcd(g_mmap);

						activeChoice = MB_SECOND_CHOICE;
					}
					break;
			}
		}
	}

	// Clear LCD map buffer to remove message from showing up
	memset(&(g_mmap[0][0]), 0, sizeof(g_mmap));
	WriteMapToLcd(g_mmap);

#if 0 /* ET Test */
	g_debugBufferCount = 0;
#endif

	if (key == ENTER_KEY) { return (activeChoice); }
	if (key == ON_ESC_KEY) { return (MB_SPECIAL_ACTION); }
	else // key == ESC_KEY (escape)
	{
		return (MB_NO_ACTION);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void OverlayMessage(char* titleString, char* textString, uint32 displayTime)
{
	MessageBorder();
	MessageTitle(titleString);
	MessageText(textString);

	WriteMapToLcd(g_mmap);

#if EXTERNAL_SAMPLING_SOURCE
	volatile uint32 msDisplayTime = (displayTime / SOFT_MSECS);

	// Check if the display time is less than 5 ms
	if (displayTime < (5 * SOFT_MSECS))
	{
		SoftUsecWait(displayTime);
	}
	// Check for exception handling key
	else if ((displayTime & EXCEPTION_HANDLING_USE_SOFT_DELAY_KEY) == EXCEPTION_HANDLING_USE_SOFT_DELAY_KEY)
	{
		// Remove the key
		displayTime &= ~EXCEPTION_HANDLING_USE_SOFT_DELAY_KEY;

		// Convert exception time to seconds
		displayTime *= SOFT_SECS;

		SoftUsecWait(displayTime);
	}
	else // Use the millisecond timer to handle the display time and call USB Manager to handle USB requests in the meantime
	{
		// Start the key timer
		Start_Data_Clock(TC_MILLISECOND_TIMER_CHANNEL);

		while (g_msTimerTicks < msDisplayTime)
		{
			if ((g_usbMassStorageState != USB_INIT_DRIVER) && (g_usbMassStorageState != USB_DISABLED_FOR_OTHER_PROCESSING))
			{
				// Process USB core routines (do not call UsbDeviceManager since it's not designed to be re-entrant)
				ProcessUsbCoreHandling();
			}
		}

		// Disable the key timer
		Stop_Data_Clock(TC_MILLISECOND_TIMER_CHANNEL);
	}
#else /* INTERNAL_SAMPLING_SOURCE */
	SoftUsecWait(displayTime);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UpdateModeMenuTitle(uint8 mode)
{
	switch (mode)
	{
		case WAVEFORM_MODE:
			modeMenu[0].textEntry = WAVEFORM_MODE_TEXT;
		break;

		case BARGRAPH_MODE:
			modeMenu[0].textEntry = BARGRAPH_MODE_TEXT;
		break;

		case COMBO_MODE:
			modeMenu[0].textEntry = COMBO_MODE_TEXT;
		break;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 testg_mmap[LCD_NUM_OF_ROWS][LCD_NUM_OF_BIT_COLUMNS];

void DisplaySplashScreen(void)
{
	WND_LAYOUT_STRUCT wnd_layout;
	uint8 buff[50];
	uint8 length;

	wnd_layout.end_row = DEFAULT_END_ROW;
	wnd_layout.end_col = DEFAULT_END_COL;

	// Clear cached LCD memory map
	memset(&(g_mmap[0][0]), 0, sizeof(g_mmap));

	//----------------------------------------------------------------------------------------
	// Add in a title for the menu
	//----------------------------------------------------------------------------------------
	memset(&buff[0], 0, sizeof(buff));

	length = sprintf((char*)(&buff[0]), "%s", "NOMIS 8100 GRAPH");
	wnd_layout.curr_row = DEFAULT_MENU_ROW_ONE;
	wnd_layout.curr_col = (uint16)(((wnd_layout.end_col)/2) - ((length * SIX_COL_SIZE)/2));
	WndMpWrtString(&buff[0], &wnd_layout, SIX_BY_EIGHT_FONT, REG_LN);

	//----------------------------------------------------------------------------------------
	// Add in Software Version
	//----------------------------------------------------------------------------------------
	memset(&buff[0], 0, sizeof(buff));
	length = (uint8)sprintf((char*)(&buff[0]), "%s %s", getLangText(SOFTWARE_VER_TEXT), (char*)g_buildVersion);

	wnd_layout.curr_row = DEFAULT_MENU_ROW_THREE;
	wnd_layout.curr_col = (uint16)(((wnd_layout.end_col)/2) - ((length * SIX_COL_SIZE)/2));
	WndMpWrtString(&buff[0], &wnd_layout, SIX_BY_EIGHT_FONT, REG_LN);

	//----------------------------------------------------------------------------------------
	// Add in Software Date and Time
	//----------------------------------------------------------------------------------------
	memset(&buff[0], 0, sizeof(buff));
	length = (uint8)sprintf((char*)(&buff[0]), "%s", (char*)g_buildDate);

	wnd_layout.curr_row = DEFAULT_MENU_ROW_FOUR;
	wnd_layout.curr_col = (uint16)(((wnd_layout.end_col)/2) - ((length * SIX_COL_SIZE)/2));
	WndMpWrtString(&buff[0], &wnd_layout, SIX_BY_EIGHT_FONT, REG_LN);

	//----------------------------------------------------------------------------------------
	// Add in Battery Voltage
	//----------------------------------------------------------------------------------------
	memset(&buff[0], 0, sizeof(buff));
	length = (uint8)sprintf((char*)(&buff[0]), "%s: %.2f", getLangText(BATT_VOLTAGE_TEXT), (double)GetExternalVoltageLevelAveraged(BATTERY_VOLTAGE));

	wnd_layout.curr_row = DEFAULT_MENU_ROW_SIX;
	wnd_layout.curr_col = (uint16)(((wnd_layout.end_col)/2) - ((length * SIX_COL_SIZE)/2));
	WndMpWrtString(&buff[0], &wnd_layout, SIX_BY_EIGHT_FONT, REG_LN);

	debug("Init Write Splash Screen to LCD...\r\n");

	// Write the map to the LCD
	WriteMapToLcd(g_mmap);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DisplayCalDate(void)
{
	char dateString[35];
	DATE_TIME_STRUCT tempTime;

	if (!g_factorySetupRecord.invalid)
	{
		memset(&dateString[0], 0, sizeof(dateString));

		ConvertCalDatetoDateTime(&tempTime, &g_currentCalibration.date);
		ConvertTimeStampToString(dateString, &tempTime, REC_DATE_TYPE);

		sprintf((char*)g_spareBuffer, "%s: %s", getLangText(CALIBRATION_DATE_TEXT), (char*)dateString);
		MessageBox(getLangText(STATUS_TEXT), (char*)g_spareBuffer, MB_OK);
	}
	else
	{
		MessageBox(getLangText(STATUS_TEXT), getLangText(CALIBRATION_DATE_NOT_SET_TEXT), MB_OK);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint32 GetAirDefaultValue(void)
{
	return (AIR_TRIGGER_DEFAULT_VALUE);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint32 GetAirMinValue(void)
{
	uint32 airMinValue;

	if (g_unitConfig.unitsOfAir == MILLIBAR_TYPE)
	{
		switch (g_factorySetupRecord.acousticSensorType)
		{
			case SENSOR_MIC_160_DB: airMinValue = AIR_TRIGGER_MIC_160_DB_IN_MB_MIN_VALUE; break;
			case SENSOR_MIC_5_PSI: airMinValue = AIR_TRIGGER_MIC_5_PSI_IN_MB_MIN_VALUE; break;
			case SENSOR_MIC_10_PSI: airMinValue = AIR_TRIGGER_MIC_10_PSI_IN_MB_MIN_VALUE; break;
			default: /* SENSOR_MIC_148_DB */ airMinValue = AIR_TRIGGER_MIC_148_DB_IN_MB_MIN_VALUE; break;
		}
	}
	else if (g_unitConfig.unitsOfAir == PSI_TYPE)
	{
		switch (g_factorySetupRecord.acousticSensorType)
		{
			case SENSOR_MIC_160_DB: airMinValue = AIR_TRIGGER_MIC_160_DB_IN_PSI_MIN_VALUE; break;
			case SENSOR_MIC_5_PSI: airMinValue = AIR_TRIGGER_MIC_5_PSI_MENU_MIN_VALUE; break;
			case SENSOR_MIC_10_PSI: airMinValue = AIR_TRIGGER_MIC_10_PSI_MENU_MIN_VALUE; break;
			default: /* SENSOR_MIC_148_DB */ airMinValue = AIR_TRIGGER_MIC_148_DB_IN_PSI_MIN_VALUE; break;
		}
	}
	else // (g_unitConfig.unitsOfAir == DECIBEL_TYPE)
	{
		switch (g_factorySetupRecord.acousticSensorType)
		{
			case SENSOR_MIC_160_DB: airMinValue = AIR_TRIGGER_MIC_160_DB_MIN_VALUE; break;
			case SENSOR_MIC_5_PSI: airMinValue = AIR_TRIGGER_MIC_5_PSI_IN_DB_MIN_VALUE; break;
			case SENSOR_MIC_10_PSI: airMinValue = AIR_TRIGGER_MIC_10_PSI_IN_DB_MIN_VALUE; break;
			default: /* SENSOR_MIC_148_DB */ airMinValue = AIR_TRIGGER_MIC_148_DB_MIN_VALUE; break;
		}
	}

	return (airMinValue);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint32 GetAirMaxValue(void)
{
	uint32 airMaxValue;

	if (g_unitConfig.unitsOfAir == MILLIBAR_TYPE)
	{
		switch (g_factorySetupRecord.acousticSensorType)
		{
			case SENSOR_MIC_160_DB: airMaxValue = AIR_TRIGGER_MIC_160_DB_IN_MB_MAX_VALUE; break;
			case SENSOR_MIC_5_PSI: airMaxValue = AIR_TRIGGER_MIC_5_PSI_IN_MB_MAX_VALUE; break;
			case SENSOR_MIC_10_PSI: airMaxValue = AIR_TRIGGER_MIC_10_PSI_IN_MB_MAX_VALUE; break;
			default: /* SENSOR_MIC_148_DB */ airMaxValue = AIR_TRIGGER_MIC_148_DB_IN_MB_MAX_VALUE; break;
		}
	}
	else if (g_unitConfig.unitsOfAir == PSI_TYPE)
	{
		switch (g_factorySetupRecord.acousticSensorType)
		{
			case SENSOR_MIC_160_DB: airMaxValue = AIR_TRIGGER_MIC_160_DB_IN_PSI_MAX_VALUE; break;
			case SENSOR_MIC_5_PSI: airMaxValue = AIR_TRIGGER_MIC_5_PSI_MENU_MAX_VALUE; break;
			case SENSOR_MIC_10_PSI: airMaxValue = AIR_TRIGGER_MIC_10_PSI_MENU_MAX_VALUE; break;
			default: /* SENSOR_MIC_148_DB */ airMaxValue = AIR_TRIGGER_MIC_148_DB_IN_PSI_MAX_VALUE; break;
		}
	}
	else // (g_unitConfig.unitsOfAir == DECIBEL_TYPE)
	{
		switch (g_factorySetupRecord.acousticSensorType)
		{
			case SENSOR_MIC_160_DB: airMaxValue = AIR_TRIGGER_MIC_160_DB_MAX_VALUE; break;
			case SENSOR_MIC_5_PSI: airMaxValue = AIR_TRIGGER_MIC_5_PSI_IN_DB_MAX_VALUE; break;
			case SENSOR_MIC_10_PSI: airMaxValue = AIR_TRIGGER_MIC_10_PSI_IN_DB_MAX_VALUE; break;
			default: /* SENSOR_MIC_148_DB */ airMaxValue = AIR_TRIGGER_MIC_148_DB_MAX_VALUE; break;
		}
	}

	return (airMaxValue);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GetAirSensorTypeName(char* airSensorTypeName)
{
	if (g_acousticSmartSensorMemory.sensorType == SENSOR_MIC_160_DB) { strcpy(airSensorTypeName, "MIC 160 dB"); }
	else if (g_acousticSmartSensorMemory.sensorType == SENSOR_MIC_5_PSI) { strcpy(airSensorTypeName, "MIC 5 PSI"); }
	else if (g_acousticSmartSensorMemory.sensorType == SENSOR_MIC_10_PSI) { strcpy(airSensorTypeName, "MIC 10 PSI"); }
	else { strcpy(airSensorTypeName, "MIC 148 dB"); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DisplaySensorType(void)
{
	uint16 sensorType;
	uint16 sensorTypeTextElement = NULL_TEXT;
	uint8 acousticSensorType = g_factorySetupRecord.acousticSensorType;
	char airSensorTypeName[16];
#if 1 /* temp */
	UNUSED(acousticSensorType);
#endif

	if (!g_factorySetupRecord.invalid)
	{
		// Check if optioned to use Seismic Smart Sensor and Seismic smart sensor was successfully read
		if ((g_factorySetupRecord.calibrationDateSource == SEISMIC_SMART_SENSOR_CAL_DATE) && (g_seismicSmartSensorMemory.version & SMART_SENSOR_OVERLAY_KEY))
		{
			sensorType = (pow(2, g_seismicSmartSensorMemory.sensorType) * SENSOR_2_5_IN);
		}
		else // Default to factory setup record sensor type
		{
			sensorType = g_factorySetupRecord.seismicSensorType;
		}

		switch (sensorType)
		{
			case SENSOR_80_IN			: sensorTypeTextElement = X025_80_IPS_TEXT; break;
			case SENSOR_40_IN			: sensorTypeTextElement = X05_40_IPS_TEXT; break;
			case SENSOR_20_IN			: sensorTypeTextElement = X1_20_IPS_TEXT; break;
			case SENSOR_10_IN			: sensorTypeTextElement = X2_10_IPS_TEXT; break;
			case SENSOR_5_IN			: sensorTypeTextElement = X4_5_IPS_TEXT; break;
			case SENSOR_2_5_IN			: sensorTypeTextElement = X8_2_5_IPS_TEXT; break;
			case SENSOR_ACCELEROMETER	: sensorTypeTextElement = ACC_793L_TEXT; break;
		}

		if (sensorType == SENSOR_ACC_832M1_0200)
		{
			sprintf((char*)g_spareBuffer, "SEISMIC GAIN/TYPE: ACC (832M1-0200)");
		}
		else if (sensorType == SENSOR_ACC_832M1_0500)
		{
			sprintf((char*)g_spareBuffer, "SEISMIC GAIN/TYPE: ACC (832M1-0500)");
		}
		else sprintf((char*)g_spareBuffer, "%s: %s", "SEISMIC GAIN/TYPE", getLangText(sensorTypeTextElement));
		MessageBox(getLangText(STATUS_TEXT), (char*)g_spareBuffer, MB_OK);

		if (g_acousticSmartSensorMemory.version & SMART_SENSOR_OVERLAY_KEY)
		{
			if ((g_acousticSmartSensorMemory.sensorType == SENSOR_MIC_148_DB) || (g_acousticSmartSensorMemory.sensorType == SENSOR_MIC_160_DB) || (g_acousticSmartSensorMemory.sensorType == SENSOR_MIC_5_PSI) ||
				(g_acousticSmartSensorMemory.sensorType == SENSOR_MIC_10_PSI))
			{
				acousticSensorType = g_acousticSmartSensorMemory.sensorType;
			}
		}

		GetAirSensorTypeName(&airSensorTypeName[0]);
		sprintf((char*)g_spareBuffer, "%s: %s", "ACOUSTIC GAIN/TYPE", airSensorTypeName);
		MessageBox(getLangText(STATUS_TEXT), (char*)g_spareBuffer, MB_OK);
	}
	else
	{
		MessageBox(getLangText(STATUS_TEXT), getLangText(SENSOR_GAIN_TYPE_NOT_SET_TEXT), MB_OK);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint32 SerialNumberConverter(uint8* snPtr)
{
	// Due to Smart sensor serial number nibble storage the first two bytes can be ignored
	return ((uint32)((snPtr[2] << 24) | (snPtr[3] << 16) | (snPtr[4] << 8) | (snPtr[5])));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DisplaySerialNumber(void)
{
	char serialNumberString[20];
	uint32 serialNumberConversion;

	if (!g_factorySetupRecord.invalid)
	{
		memset(&serialNumberString[0], 0, sizeof(serialNumberString));
		memcpy(&serialNumberString[0], &g_factorySetupRecord.unitSerialNumber[0], sizeof(g_factorySetupRecord.unitSerialNumber));
		sprintf((char*)g_spareBuffer, "%s: %s", getLangText(SERIAL_NUMBER_TEXT), (char*)serialNumberString);
		MessageBox(getLangText(STATUS_TEXT), (char*)g_spareBuffer, MB_OK);
	}
	else
	{
		MessageBox(getLangText(STATUS_TEXT), getLangText(SERIAL_NUMBER_NOT_SET_TEXT), MB_OK);
	}

	if (g_seismicSmartSensorMemory.version & SMART_SENSOR_OVERLAY_KEY)
	{
		serialNumberConversion = SerialNumberConverter(&g_seismicSmartSensorMemory.serialNumber[0]);
		sprintf((char*)g_spareBuffer, "%s %s: %06lu", getLangText(SEISMIC_TEXT), getLangText(SERIAL_NUMBER_TEXT), serialNumberConversion);
		MessageBox(getLangText(STATUS_TEXT), (char*)g_spareBuffer, MB_OK);
	}
	else
	{
		sprintf((char*)g_spareBuffer, "%s %s %s %s", getLangText(SEISMIC_TEXT), getLangText(SMART_SENSOR_TEXT), getLangText(SERIAL_NUMBER_TEXT), getLangText(NOT_FOUND_TEXT));
		MessageBox(getLangText(STATUS_TEXT), (char*)g_spareBuffer, MB_OK);
	}

	if (g_acousticSmartSensorMemory.version & SMART_SENSOR_OVERLAY_KEY)
	{
		serialNumberConversion = SerialNumberConverter(&g_acousticSmartSensorMemory.serialNumber[0]);
		sprintf((char*)g_spareBuffer, "%s %s: %06lu", getLangText(ACOUSTIC_TEXT), getLangText(SERIAL_NUMBER_TEXT), serialNumberConversion);
		MessageBox(getLangText(STATUS_TEXT), (char*)g_spareBuffer, MB_OK);
	}
	else
	{
		sprintf((char*)g_spareBuffer, "%s %s %s %s", getLangText(ACOUSTIC_TEXT), getLangText(SMART_SENSOR_TEXT), getLangText(SERIAL_NUMBER_TEXT), getLangText(NOT_FOUND_TEXT));
		MessageBox(getLangText(STATUS_TEXT), (char*)g_spareBuffer, MB_OK);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DisplayTimerModeSettings(void)
{
	char activeTime[25];
	char activeDates[30];
	uint16 activeModeTextType;

	memset(&activeTime[0], 0, sizeof(activeTime));
	memset(&activeDates[0], 0, sizeof(activeDates));

	sprintf((char*)activeTime, "%02d:%02d -> %02d:%02d", g_unitConfig.timerStartTime.hour, g_unitConfig.timerStartTime.min,
			g_unitConfig.timerStopTime.hour, g_unitConfig.timerStopTime.min);

	switch (g_unitConfig.timerModeFrequency)
	{
		case TIMER_MODE_ONE_TIME: 	activeModeTextType = ONE_TIME_TEXT; 		break;
		case TIMER_MODE_HOURLY: 	activeModeTextType = HOURLY_TEXT; 			break;
		case TIMER_MODE_DAILY: 		activeModeTextType = DAILY_EVERY_DAY_TEXT; 	break;
		case TIMER_MODE_WEEKDAYS: 	activeModeTextType = DAILY_WEEKDAYS_TEXT; 	break;
		case TIMER_MODE_WEEKLY: 	activeModeTextType = WEEKLY_TEXT; 			break;
		case TIMER_MODE_MONTHLY: 	activeModeTextType = MONTHLY_TEXT; 			break;
		default:					activeModeTextType = ERROR_TEXT;			break;
	}

	sprintf((char*)activeDates, "%02d-%s-%02d -> %02d-%s-%02d", g_unitConfig.timerStartDate.day,
			(char*)&(g_monthTable[(uint8)(g_unitConfig.timerStartDate.month)].name[0]), g_unitConfig.timerStartDate.year,
			g_unitConfig.timerStopDate.day, (char*)&(g_monthTable[(uint8)(g_unitConfig.timerStopDate.month)].name[0]),
			g_unitConfig.timerStopDate.year);

	// Display SAVED SETTINGS, ACTIVE TIME PERIOD HH:MM -> HH:MM
	sprintf((char*)g_spareBuffer, "%s, %s: %s", getLangText(SAVED_SETTINGS_TEXT),
			getLangText(ACTIVE_TIME_PERIOD_TEXT), activeTime);
	MessageBox(getLangText(TIMER_MODE_TEXT), (char*)g_spareBuffer, MB_OK);

	// Display MODE, HH:MM -> HH:MM
	sprintf((char*)g_spareBuffer, "%s, %s", getLangText(activeModeTextType), activeDates);
	MessageBox(getLangText(TIMER_MODE_TEXT), (char*)g_spareBuffer, MB_OK);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DisplayFlashUsageStats(void)
{
	char sizeUsedStr[40];
	char sizeFreeStr[40];
	char subString[50];
	uint8 i = 0;

	if (g_sdCardUsageStats.sizeUsed < 1000)
		sprintf(&sizeUsedStr[0], "%s: %.1fKB (%d%%)", getLangText(USED_TEXT), (double)((float)g_sdCardUsageStats.sizeUsed / (float)1000), g_sdCardUsageStats.percentUsed);
	else if (g_sdCardUsageStats.sizeUsed < 1000000)
		sprintf(&sizeUsedStr[0], "%s: %luKB (%d%%)", getLangText(USED_TEXT), (g_sdCardUsageStats.sizeUsed / 1000), g_sdCardUsageStats.percentUsed);
	else
		sprintf(&sizeUsedStr[0], "%s: %.0fMB (%d%%)", getLangText(USED_TEXT), (double)((float)g_sdCardUsageStats.sizeUsed / (float)1000000), g_sdCardUsageStats.percentUsed);

#if 0 /* Test (always display in bytes) */
	sprintf(&sizeUsedStr[0], "%s: %lu B (%d%%)", getLangText(USED_TEXT), g_sdCardUsageStats.sizeUsed, g_sdCardUsageStats.percentUsed);
#endif

	if (g_sdCardUsageStats.sizeFree < 1000)
		sprintf(&sizeFreeStr[0], "%s: %.1fKB (%d%%)", getLangText(FREE_TEXT), (double)((float)g_sdCardUsageStats.sizeFree / (float)1000), g_sdCardUsageStats.percentFree);
	else if (g_sdCardUsageStats.sizeFree < 1000000)
		sprintf(&sizeFreeStr[0], "%s: %luKB (%d%%)", getLangText(FREE_TEXT), (g_sdCardUsageStats.sizeFree / 1000), g_sdCardUsageStats.percentFree);
	else
		sprintf(&sizeFreeStr[0], "%s: %.0fMB (%d%%)", getLangText(FREE_TEXT), (double)((float)g_sdCardUsageStats.sizeFree / (float)1000000), g_sdCardUsageStats.percentFree);

#if 0 /* Test (always display in bytes) */
	sprintf(&sizeFreeStr[0], "%s: %lu B (%d%%)", getLangText(FREE_TEXT), g_sdCardUsageStats.sizeFree, g_sdCardUsageStats.percentFree);
#endif

	if (g_unitConfig.flashWrapping == NO)
	{
		sprintf((char*)g_spareBuffer, "%s %s", getLangText(EVENT_DATA_TEXT), sizeUsedStr);
		MessageBox(getLangText(FLASH_USAGE_STATS_TEXT), (char*)g_spareBuffer, MB_OK);

		sprintf((char*)g_spareBuffer, "%s %s", getLangText(EVENT_DATA_TEXT), sizeFreeStr);
		MessageBox(getLangText(FLASH_USAGE_STATS_TEXT), (char*)g_spareBuffer, MB_OK);
	}
	else
	{
		sprintf((char*)g_spareBuffer, "%s (%s: %s) %s", getLangText(EVENT_DATA_TEXT), getLangText(WRAPPED_TEXT), ((g_unitConfig.flashWrapping == YES) ? "YES" : "NO"), sizeUsedStr);
		MessageBox(getLangText(FLASH_USAGE_STATS_TEXT), (char*)g_spareBuffer, MB_OK);

		sprintf((char*)g_spareBuffer, "%s (%s: %s) %s", getLangText(EVENT_DATA_TEXT), getLangText(WRAPPED_TEXT), ((g_unitConfig.flashWrapping == YES) ? "YES" : "NO"), sizeFreeStr);
		MessageBox(getLangText(FLASH_USAGE_STATS_TEXT), (char*)g_spareBuffer, MB_OK);
	}

	sprintf(subString, "%s", getLangText(SPACE_REMAINING_TEXT));
	while ((subString[i] != '\0') && (i < 50))
	{
		if (subString[i] == '(') { subString[i] = '\0'; }

		i++;
	}

	if (g_unitConfig.flashWrapping == NO)
	{
		if (g_sdCardUsageStats.waveEventsLeft < 1000)
		{
			sprintf((char*)g_spareBuffer, "%s %s: %d", subString, getLangText(WAVEFORMS_TEXT), g_sdCardUsageStats.waveEventsLeft);
			MessageBox(getLangText(FLASH_USAGE_STATS_TEXT), (char*)g_spareBuffer, MB_OK);
		}
		else // Scale output in thousands
		{
			sprintf((char*)g_spareBuffer, "%s %s: %dK", subString, getLangText(WAVEFORMS_TEXT), (g_sdCardUsageStats.waveEventsLeft / 1000));
			MessageBox(getLangText(FLASH_USAGE_STATS_TEXT), (char*)g_spareBuffer, MB_OK);
		}

		if (g_sdCardUsageStats.barHoursLeft < 1000)
		{
			sprintf((char*)g_spareBuffer, "%s %s: ~%d", subString, getLangText(BAR_HOURS_TEXT), g_sdCardUsageStats.barHoursLeft);
			MessageBox(getLangText(FLASH_USAGE_STATS_TEXT), (char*)g_spareBuffer, MB_OK);
		}
		else // Scale output in thousands
		{
			sprintf((char*)g_spareBuffer, "%s %s: ~%dK", subString, getLangText(BAR_HOURS_TEXT), (g_sdCardUsageStats.barHoursLeft / 1000));
			MessageBox(getLangText(FLASH_USAGE_STATS_TEXT), (char*)g_spareBuffer, MB_OK);
		}
	}
	else // Wrapping is on
	{
		if (g_sdCardUsageStats.waveEventsLeft < 1000)
		{
			sprintf((char*)g_spareBuffer, "%s %s: %d", getLangText(BEFORE_OVERWRITE_TEXT), getLangText(WAVEFORMS_TEXT), g_sdCardUsageStats.waveEventsLeft);
			MessageBox(getLangText(FLASH_USAGE_STATS_TEXT), (char*)g_spareBuffer, MB_OK);
		}
		else
		{
			sprintf((char*)g_spareBuffer, "%s %s: %dK", getLangText(BEFORE_OVERWRITE_TEXT), getLangText(WAVEFORMS_TEXT), (g_sdCardUsageStats.waveEventsLeft / 1000));
			MessageBox(getLangText(FLASH_USAGE_STATS_TEXT), (char*)g_spareBuffer, MB_OK);
		}

		if (g_sdCardUsageStats.barHoursLeft < 1000)
		{
			sprintf((char*)g_spareBuffer, "%s %s: ~%d", getLangText(BEFORE_OVERWRITE_TEXT), getLangText(BAR_HOURS_TEXT), g_sdCardUsageStats.barHoursLeft);
			MessageBox(getLangText(FLASH_USAGE_STATS_TEXT), (char*)g_spareBuffer, MB_OK);
		}
		else
		{
			sprintf((char*)g_spareBuffer, "%s %s: ~%dK", getLangText(BEFORE_OVERWRITE_TEXT), getLangText(BAR_HOURS_TEXT), (g_sdCardUsageStats.barHoursLeft / 1000));
			MessageBox(getLangText(FLASH_USAGE_STATS_TEXT), (char*)g_spareBuffer, MB_OK);
		}
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DisplayAutoDialInfo(void)
{
	char dateStr[40];

	if (__autoDialoutTbl.lastConnectTime.valid == NO)
	{
		sprintf(&dateStr[0], "N/A");
	}
	else
	{
		sprintf(&dateStr[0], "%d/%d/%02d %02d:%02d:%02d",
				__autoDialoutTbl.lastConnectTime.month,
				__autoDialoutTbl.lastConnectTime.day,
				__autoDialoutTbl.lastConnectTime.year,
				__autoDialoutTbl.lastConnectTime.hour,
				__autoDialoutTbl.lastConnectTime.min,
				__autoDialoutTbl.lastConnectTime.sec);
	}

	sprintf((char*)g_spareBuffer, "%s: %d, %s: %d, %s: %s", getLangText(LAST_DIAL_EVENT_TEXT),__autoDialoutTbl.lastDownloadedEvent,
			getLangText(LAST_RECEIVED_TEXT), GetLastStoredEventNumber(), getLangText(LAST_CONNECTED_TEXT), dateStr);

	MessageBox(getLangText(AUTO_DIALOUT_INFO_TEXT), (char*)g_spareBuffer, MB_OK);
}
