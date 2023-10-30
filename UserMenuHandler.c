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
#include "Record.h"
#include "Display.h"
#include "Typedefs.h"
#include "Uart.h"
#include "RealTimeClock.h"
#include "Keypad.h"
#include "TextTypes.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define PERCENT_INCREMENT_DEFAULT	(uint32)(5)

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
#include "Globals.h"
extern USER_MENU_STRUCT helpMenu[];

///----------------------------------------------------------------------------
///	Local Scope Globals
///----------------------------------------------------------------------------
static USER_TYPE_STRUCT unitTypes[TOTAL_TYPES] = {
{"", NO_TYPE, 1},
{"", NO_ALT_TYPE, 1},
{"in", IN_TYPE, 1},
{"mm", MM_TYPE, (float)25.4},
{"ft", FT_TYPE, 1},
{"m", M_TYPE, (float)0.3048},
{"lbs", LBS_TYPE, 1},
{"kg", KG_TYPE, (float)0.45454},
{"dB", DB_TYPE, 1},
{"dBa", DBA_TYPE, 1},
{"mb", MB_TYPE, 1},
{"secs", SECS_TYPE, 1},
{"mins", MINS_TYPE, 1},
{"mg", MG_TYPE, 1},
{"hour", HOUR_TYPE, 1},
{"%", PERCENT_TYPE, 1},
{"psi", PSI_UNIT_TYPE, 1}
};

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------
void UserMenu(INPUT_MSG_STRUCT);
void UserMenuProc(INPUT_MSG_STRUCT, WND_LAYOUT_STRUCT*, MN_LAYOUT_STRUCT*);
void AdvanceInputChar(uint32 dir);
void CopyMenuToCache(USER_MENU_STRUCT* currentMenu);
void CopyDataToCache(void* data);
void CopyDataToMenu(MN_LAYOUT_STRUCT*);
uint16 FindCurrentItemEntry(uint16 startLine, uint32 item);
void AdvanceInputNumber(uint32 dir);
void RemoveExtraSpaces(void);

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UserMenu(INPUT_MSG_STRUCT msg)
{ 
	static WND_LAYOUT_STRUCT wnd_layout;
	static MN_LAYOUT_STRUCT mn_layout;

	// Handle all the preprocessing to setup the menu before it is displayed
	UserMenuProc(msg, &wnd_layout,&mn_layout);

	// Verify that the active menu is still the User Menu
	if (g_activeMenu == USER_MENU)
	{
		// Setup the LCD map with the title position set inside the menu structure
		DisplayUserMenu(&wnd_layout, &mn_layout, USER_MENU_TITLE_POSITION(g_userMenuCachePtr));

		// Write the LCD map to the screen
		WriteMapToLcd(g_mmap);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UserMenuProc(INPUT_MSG_STRUCT msg, WND_LAYOUT_STRUCT *wnd_layout_ptr, MN_LAYOUT_STRUCT *mn_layout_ptr)
{
	INPUT_MSG_STRUCT mn_msg;
	uint32 input;
	//uint16 tempCurrLine = 0, tempSubLine = 0;
	//uint16 tempLength = 0;

	// Switch on the incoming command type	
	switch (msg.cmd)
	{
		// New menu display command
		case (ACTIVATE_MENU_WITH_DATA_CMD):
			wnd_layout_ptr->start_col = DEFAULT_COL_SIX;
			wnd_layout_ptr->end_col = DEFAULT_END_COL;
			wnd_layout_ptr->start_row = DEFAULT_MENU_ROW_ONE;
			wnd_layout_ptr->end_row = DEFAULT_MENU_ROW_SEVEN;
			mn_layout_ptr->top_ln = 1;
			mn_layout_ptr->sub_ln = 0;

			// Copy the static menu data into the user menu display and set the user menu handler
			CopyMenuToCache((USER_MENU_STRUCT*)msg.data[CURRENT_USER_MENU]);

			// Check if the current menu is a select type
			if ((USER_MENU_TYPE(g_userMenuCachePtr) == SELECT_TYPE) || ((USER_MENU_TYPE(g_userMenuCachePtr) == SELECT_SPECIAL_TYPE)))
			{
				// Get the current item and set the current line to be highlighted (Adjust down one line for Select Special, otherwise start at the top
				mn_layout_ptr->curr_ln = FindCurrentItemEntry(((USER_MENU_TYPE(g_userMenuCachePtr) == SELECT_SPECIAL_TYPE) ? (mn_layout_ptr->top_ln + 1) : mn_layout_ptr->top_ln), msg.data[CURRENT_ITEM_VALUE]);

				// Adjust top line if current line is below the first screen's worth of text
				if (mn_layout_ptr->curr_ln > 6)
					mn_layout_ptr->top_ln = (uint16)(mn_layout_ptr->curr_ln - 5);
			}
			else // Handle other types, INTEGER_BYTE_TYPE, 	INTEGER_BYTE_OFFSET_TYPE, INTEGER_WORD_TYPE, INTEGER_WORD_FIXED_TYPE, INTEGER_WORD_OFFSET_TYPE, INTEGER_LONG_TYPE,
					// INTEGER_SPECIAL_TYPE, INTEGER_COUNT_TYPE, STRING_TYPE, FLOAT_TYPE, FLOAT_SPECIAL_TYPE, FLOAT_WITH_N_TYPE
			{
				// Get the default item and set the current line to be highlighted
				mn_layout_ptr->curr_ln = USER_MENU_DEFAULT_ROW(g_userMenuCachePtr);

				// Copy the (passed by pointer) variable menu data to the user menu data cache
				CopyDataToCache((void*)msg.data[CURRENT_DATA_POINTER]);

				// Copy the variable data from the user menu data cache to the user menu display
				CopyDataToMenu(mn_layout_ptr);
			}

			debug("User Menu <%s>, Displayable Items: %d\r\n", g_userMenuCachePtr[MENU_INFO].text, USER_MENU_DISPLAY_ITEMS(g_userMenuCachePtr));
		break;

		// Hanle a keypress message
		case (KEYPRESS_MENU_CMD):
			input = msg.data[0];

			// Check if the current menu is a select type
			if ((USER_MENU_TYPE(g_userMenuCachePtr) == SELECT_TYPE) || ((USER_MENU_TYPE(g_userMenuCachePtr) == SELECT_SPECIAL_TYPE)))
			{
				// Check if the total number of active items (minus the title and end line) is less than 10
				if (USER_MENU_ACTIVE_ITEMS(g_userMenuCachePtr) < 10)
				{
					// Handle converting a number key input into a manu selection
					if ((input >= (uint32)ONE_KEY) && (input <= (uint32)(USER_MENU_ACTIVE_ITEMS(g_userMenuCachePtr) | 0x30)))
					{
						// Convert the ASCII key hex value to a true number to set the current line
						mn_layout_ptr->curr_ln = (uint16)(input - 0x30);

						// Set the input to be an Enter key to allow processing the current line
						input = ENTER_KEY;
					}
				}
			}

			// Handle the specific key that was pressed
			switch (input)
			{
				case (ENTER_KEY):
					// Make sure the user menu handler is not null before jumping to the routine
					if (g_userMenuHandler != NULL)
					{
						if ((USER_MENU_TYPE(g_userMenuCachePtr) == SELECT_TYPE) || ((USER_MENU_TYPE(g_userMenuCachePtr) == SELECT_SPECIAL_TYPE)))
						{
							// Set the current index to the user menu current line
							g_userMenuCacheData.currentIndex = mn_layout_ptr->curr_ln;
							
							// Call the user menu handler, passing the key and the address of the index
							(*g_userMenuHandler)(ENTER_KEY, &g_userMenuCacheData.currentIndex);
						}
						else if ((USER_MENU_TYPE(g_userMenuCachePtr) == STRING_TYPE) || (USER_MENU_TYPE(g_userMenuCachePtr) == STRING_SPECIAL_TYPE))
						{
							// Remove any extra spaces as the end of the string
							RemoveExtraSpaces();

							// Call the user menu handler, passing the key and the address of the string
							(*g_userMenuHandler)(ENTER_KEY, &g_userMenuCacheData.text);
						}
						else if ((USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_BYTE_TYPE) ||
								(USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_BYTE_OFFSET_TYPE))
						{
							// Call the user menu handler, passing the key and the address of the byte data
							(*g_userMenuHandler)(ENTER_KEY, &g_userMenuCacheData.numByteData);
						}
						else if ((USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_WORD_TYPE) ||
								(USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_WORD_FIXED_TYPE) ||
								(USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_WORD_OFFSET_TYPE))
						{
							// Call the user menu handler, passing the key and the address of the word data
							(*g_userMenuHandler)(ENTER_KEY, &g_userMenuCacheData.numWordData);
						}
						else if ((USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_LONG_TYPE) || 
								(USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_SPECIAL_TYPE) ||
								(USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_COUNT_TYPE))
						{
							// Call the user menu handler, passing the key and the address of the long data
							(*g_userMenuHandler)(ENTER_KEY, &g_userMenuCacheData.numLongData);
						}
						else if ((USER_MENU_TYPE(g_userMenuCachePtr) == FLOAT_TYPE) ||
								(USER_MENU_TYPE(g_userMenuCachePtr) == FLOAT_SPECIAL_TYPE) ||
								(USER_MENU_TYPE(g_userMenuCachePtr) == FLOAT_WITH_N_TYPE))
						{
							// Call the user menu handler, passing the key and the address of the float data
							(*g_userMenuHandler)(ENTER_KEY, &g_userMenuCacheData.floatData);
						}
					}
				break;

				case (ESC_KEY):
					if (g_userMenuHandler != NULL)
					{
						// Call the user menu handler, passing the key and null for the data pointer
						(*g_userMenuHandler)(ESC_KEY, NULL);
					}
				break;

				case (HELP_KEY):
					// Jump to the Help menu
					SETUP_USER_MENU_MSG(&helpMenu, CONFIG_CHOICE);
					JUMP_TO_ACTIVE_MENU();
				break;

				case (DOWN_ARROW_KEY):
				case (UP_ARROW_KEY):
					if ((USER_MENU_TYPE(g_userMenuCachePtr) == SELECT_TYPE) || ((USER_MENU_TYPE(g_userMenuCachePtr) == SELECT_SPECIAL_TYPE)))
					{
						// Scroll the highlighted menu item up or down based on the key used
						UserMenuScroll((uint8)input, SELECT_MN_WND_LNS, mn_layout_ptr);

						// Make sure Select Special doesn't show the first line as an active item
						if ((USER_MENU_TYPE(g_userMenuCachePtr) == SELECT_SPECIAL_TYPE) && (mn_layout_ptr->curr_ln == 1))
						{
							mn_layout_ptr->curr_ln++;
						}
					}
					else if ((USER_MENU_TYPE(g_userMenuCachePtr) == STRING_TYPE) || (USER_MENU_TYPE(g_userMenuCachePtr) == STRING_SPECIAL_TYPE))
					{
						// If the current char is a null and not the max index of the string
						if ((g_userMenuCacheData.text[g_userMenuCachePtr[CURRENT_TEXT_INDEX].data] == '\0') &&
							(g_userMenuCachePtr[CURRENT_TEXT_INDEX].data < g_userMenuCachePtr[MAX_TEXT_CHARS].data))
						{
							// Set the char at the current index to be an "A"
							g_userMenuCacheData.text[g_userMenuCachePtr[CURRENT_TEXT_INDEX].data] = 'A';
						}
						else
						{
							// Handle advancing the character up or down based on the key used
							AdvanceInputChar(input);
						}

						// Copy the string data to the user menu display
						CopyDataToMenu(mn_layout_ptr);
					}
					else // INTEGER_BYTE_TYPE, 	INTEGER_BYTE_OFFSET_TYPE, INTEGER_WORD_TYPE, INTEGER_WORD_FIXED_TYPE, INTEGER_WORD_OFFSET_TYPE, INTEGER_LONG_TYPE,
							// INTEGER_SPECIAL_TYPE, INTEGER_COUNT_TYPE, FLOAT_TYPE, FLOAT_SPECIAL_TYPE, FLOAT_WITH_N_TYPE
					{
						// Handle advancing the numerical data up or down based on the key used
						AdvanceInputNumber(input);

						// Copy the numerical data to the user menu display
						CopyDataToMenu(mn_layout_ptr);
					}
				break;

				case (PLUS_KEY):
					if ((USER_MENU_TYPE(g_userMenuCachePtr) == SELECT_TYPE) || ((USER_MENU_TYPE(g_userMenuCachePtr) == SELECT_SPECIAL_TYPE)))
					{
						// Change the contrast
						AdjustLcdContrast(LIGHTER);
					}
					else if ((USER_MENU_TYPE(g_userMenuCachePtr) == STRING_TYPE) || (USER_MENU_TYPE(g_userMenuCachePtr) == STRING_SPECIAL_TYPE))
					{
						// Check if the current index is less than the max number of characters for the string
						if (g_userMenuCachePtr[CURRENT_TEXT_INDEX].data < g_userMenuCachePtr[MAX_TEXT_CHARS].data)
						{
							// Check if a null is in the current location
							if (g_userMenuCacheData.text[g_userMenuCachePtr[CURRENT_TEXT_INDEX].data] == '\0')
							{
								// Inject a space at the current position to keep the string continuous
								g_userMenuCacheData.text[g_userMenuCachePtr[CURRENT_TEXT_INDEX].data++] = ' ';
								
								// Set the next char position to a null to allow for termination
								g_userMenuCacheData.text[g_userMenuCachePtr[CURRENT_TEXT_INDEX].data] = '\0';
							}
							else
							{
								// Increment the index
								g_userMenuCachePtr[CURRENT_TEXT_INDEX].data++;
							}

							// Copy the string data to the user menu display							
							CopyDataToMenu(mn_layout_ptr);
						}
					}
				break;

				case (MINUS_KEY):
					if ((USER_MENU_TYPE(g_userMenuCachePtr) == SELECT_TYPE) || ((USER_MENU_TYPE(g_userMenuCachePtr) == SELECT_SPECIAL_TYPE)))
					{
						// Change the contrast
						AdjustLcdContrast(DARKER);
					}
					else if ((USER_MENU_TYPE(g_userMenuCachePtr) == STRING_TYPE) || (USER_MENU_TYPE(g_userMenuCachePtr) == STRING_SPECIAL_TYPE))
					{
						// Check if a space is in the current location, and a null follows it
						if ((g_userMenuCacheData.text[g_userMenuCachePtr[CURRENT_TEXT_INDEX].data] == ' ') &&
							(g_userMenuCacheData.text[(g_userMenuCachePtr[CURRENT_TEXT_INDEX].data) + 1] == '\0'))
						{
							// Set the current space char to be a null
							g_userMenuCacheData.text[g_userMenuCachePtr[CURRENT_TEXT_INDEX].data] = '\0';
						}

						// Check if the index is past the first position
						if (g_userMenuCachePtr[CURRENT_TEXT_INDEX].data > 0)
						{
							// Decrement the index
							g_userMenuCachePtr[CURRENT_TEXT_INDEX].data--;
						}

						// Copy the string data to the user menu display
						CopyDataToMenu(mn_layout_ptr);
					}
				break;

				case (DELETE_KEY):
					if ((USER_MENU_TYPE(g_userMenuCachePtr) == STRING_TYPE) || (USER_MENU_TYPE(g_userMenuCachePtr) == STRING_SPECIAL_TYPE))
					{
						// Check if the current index is the first position and the string length is greater than zero
						if ((g_userMenuCachePtr[CURRENT_TEXT_INDEX].data == 0) && (strlen(g_userMenuCacheData.text) > 0))
						{
							// Set the current index to 1 to allow the first char to be deleted (next if statement)
							g_userMenuCachePtr[CURRENT_TEXT_INDEX].data = 1;
						}

						// Check if the current index is beyond the first position
						if (g_userMenuCachePtr[CURRENT_TEXT_INDEX].data > 0)
						{
							//debug("User Input Data String: <%s>\r\n", g_userMenuCacheData.text);

							// Copy the string from the current index to one position left of the index
							strcpy((char*)&(g_userMenuCacheData.text[g_userMenuCachePtr[CURRENT_TEXT_INDEX].data - 1]), 
									(char*)&(g_userMenuCacheData.text[g_userMenuCachePtr[CURRENT_TEXT_INDEX].data]));

							//debug("User Input Data String: <%s>\r\n", g_userMenuCacheData.text);

							// Set the index to be one less (back to the same char)
							g_userMenuCachePtr[CURRENT_TEXT_INDEX].data--;
						}

						// Copy the string data to the user menu display
						CopyDataToMenu(mn_layout_ptr);
					}
				break;
				
				default:
					if ((USER_MENU_TYPE(g_userMenuCachePtr) == STRING_TYPE) || (USER_MENU_TYPE(g_userMenuCachePtr) == STRING_SPECIAL_TYPE))
					{
						// Check if the current index is less then the max length of the string
						if (g_userMenuCachePtr[CURRENT_TEXT_INDEX].data < g_userMenuCachePtr[MAX_TEXT_CHARS].data)
						{
							// Set the char at the current index to be the key input
							g_userMenuCacheData.text[g_userMenuCachePtr[CURRENT_TEXT_INDEX].data++] = (char)input;
						}	

						// Copy the string data to the user menu display
						CopyDataToMenu(mn_layout_ptr);
					}
				break;
			}
		break;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AdvanceInputChar(uint32 direction)
{
	// Store the char at the current index
	char currVal = g_userMenuCacheData.text[g_userMenuCachePtr[CURRENT_TEXT_INDEX].data];
	char newVal = 'A';

	// Check to make sure the index isn't the max (one past the end of the allocated amount)
	if (g_userMenuCachePtr[CURRENT_TEXT_INDEX].data < g_userMenuCachePtr[MAX_TEXT_CHARS].data)
	{
		if (direction == UP_ARROW_KEY)
		{
			if (USER_MENU_TYPE(g_userMenuCachePtr) == STRING_SPECIAL_TYPE)
			{
				switch (currVal)
				{
					// Advance the char based on the following "rules"
					case ' ': newVal = 'A'; break;
					case 'Z': newVal = '0'; break;
					case '9': newVal = '#'; break;
					case '#': newVal = '%'; break;
					case '&': newVal = '('; break;
					case ')': newVal = '+'; break;
					case '.': newVal = '@'; break;
					case '@': newVal = '='; break;
					case '=': newVal = '^'; break;
					case '^': newVal = '^'; break;
					default : newVal = ++currVal; break;
				}
			}
			else // (USER_MENU_TYPE(g_userMenuCachePtr) == STRING_TYPE)
			{
				switch (currVal)
				{
					// Advance the char based on the following "rules"
					case ' ': newVal = 'A'; break;
					case 'Z': newVal = '0'; break;
					case '9': newVal = '#'; break;
					case '#': newVal = '%'; break;
					case '&': newVal = '('; break;
					case '/': newVal = '\\'; break;
					case '\\': newVal = ':'; break;
					case ':': newVal = '<'; break;
					case '<': newVal = '>'; break;
					case '@': newVal = '='; break;
					case '=': newVal = '^'; break;
					case '^': newVal = '^'; break;
					default : newVal = ++currVal; break;
				}
			}
		}
		else // direction == DOWN_ARROW_KEY
		{
			if (USER_MENU_TYPE(g_userMenuCachePtr) == STRING_SPECIAL_TYPE)
			{
				switch (currVal)
				{
					// Advance the char based on the following "rules"
					case '^': newVal = '='; break;
					case '=': newVal = '@'; break;
					case '@': newVal = '.'; break;
					case '+': newVal = ')'; break;
					case '(': newVal = '&'; break;
					case '%': newVal = '#'; break;
					case '#': newVal = '9'; break;
					case '0': newVal = 'Z'; break;
					case 'A': newVal = ' '; break;
					case ' ': newVal = ' '; break;
					default : newVal = --currVal; break;
				}
			}
			else // (USER_MENU_TYPE(g_userMenuCachePtr) == STRING_TYPE)
			{
				switch (currVal)
				{
					// Advance the char based on the following "rules"
					case '^': newVal = '='; break;
					case '=': newVal = '@'; break;
					case '>': newVal = '<'; break;
					case '<': newVal = ':'; break;
					case ':': newVal = '\\'; break;
					case '\\': newVal = '/'; break;
					case '(': newVal = '&'; break;
					case '%': newVal = '#'; break;
					case '#': newVal = '9'; break;
					case '0': newVal = 'Z'; break;
					case 'A': newVal = ' '; break;
					case ' ': newVal = ' '; break;
					default : newVal = --currVal; break;
				}
			}
		}

		debug("User Input Advance Char: %c\r\n", newVal);

		// Change the char at the current position to be the new value
		g_userMenuCacheData.text[g_userMenuCachePtr[CURRENT_TEXT_INDEX].data] = newVal;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint32 GetAirTriggerIncrementForMB(void)
{
	uint32 airIncrement;

	switch (g_factorySetupRecord.acousticSensorType)
	{
		case SENSOR_MIC_160_DB: airIncrement = AIR_INCREMENT_MIC_160_DB_IN_MB; break;
		case SENSOR_MIC_5_PSI: airIncrement = AIR_INCREMENT_MIC_5_PSI_IN_MB; break;
		case SENSOR_MIC_10_PSI: airIncrement = AIR_INCREMENT_MIC_10_PSI_IN_MB; break;
		default: /* SENSOR_MIC_148_DB */ airIncrement = AIR_INCREMENT_MIC_148_DB_IN_MB; break;
	}

	return (airIncrement);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint32 GetAirTriggerIncrementForPSI(void)
{
	uint32 airIncrement;

	switch (g_factorySetupRecord.acousticSensorType)
	{
		case SENSOR_MIC_160_DB: airIncrement = AIR_INCREMENT_MIC_160_DB_IN_PSI; break;
		case SENSOR_MIC_5_PSI: airIncrement = AIR_INCREMENT_MIC_5_PSI_IN_PSI; break;
		case SENSOR_MIC_10_PSI: airIncrement = AIR_INCREMENT_MIC_10_PSI_IN_PSI; break;
		default: /* SENSOR_MIC_148_DB */ airIncrement = AIR_INCREMENT_MIC_148_DB_IN_PSI; break;
	}

	return (airIncrement);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AdvanceInputNumber(uint32 direction)
{
	// Check if direction is an up arrow to increment the number
	if (direction == UP_ARROW_KEY)
	{
		// Switch on the user menu type
		switch (USER_MENU_TYPE(g_userMenuCachePtr))
		{
			case INTEGER_BYTE_TYPE:
			case INTEGER_BYTE_OFFSET_TYPE:
#if 1 /* New rolling increment by 5 for Percent */
				// Check if the unit type is Percent and if the current integer byte data is less than the max
				if ((unitTypes[USER_MENU_DEFAULT_TYPE(g_userMenuCachePtr)].type == PERCENT_TYPE) && (g_userMenuCacheData.numByteData < g_userMenuCacheData.intMaxValue))
				{
					// Check if the data incremented by the key scrolling speed is greater than the max
					if ((g_userMenuCacheData.numByteData + PERCENT_INCREMENT_DEFAULT) > g_userMenuCacheData.intMaxValue)
					{
						// Set the max value
						g_userMenuCacheData.numByteData = (uint8)g_userMenuCacheData.intMaxValue;
					}
					else
					{
						// Increment the data by the key scrolling speed
						g_userMenuCacheData.numByteData += PERCENT_INCREMENT_DEFAULT;
					}
				}
				else
#endif
				// Check if the current integer byte data is less than the max
				if (g_userMenuCacheData.numByteData < g_userMenuCacheData.intMaxValue)
				{
					// Check if the data incremented by the key scrolling speed is greater than the max
					if ((g_userMenuCacheData.numByteData + g_keypadNumberSpeed) > g_userMenuCacheData.intMaxValue)
					{
						// Set the max value
						g_userMenuCacheData.numByteData = (uint8)g_userMenuCacheData.intMaxValue;
					}
					else
					{
						// Increment the data by the key scrolling speed
						g_userMenuCacheData.numByteData += g_keypadNumberSpeed;
					}
				}
			break;

			case INTEGER_WORD_TYPE:
			case INTEGER_WORD_FIXED_TYPE:
			case INTEGER_WORD_OFFSET_TYPE:
				// Check if the current integer word data is less than the max
				if (g_userMenuCacheData.numWordData < g_userMenuCacheData.intMaxValue)
				{
					// Check if the data incremented by the key scrolling speed is greater than the max
					if ((g_userMenuCacheData.numWordData + g_keypadNumberSpeed) > g_userMenuCacheData.intMaxValue)
					{
						// Set the max value
						g_userMenuCacheData.numWordData = (uint16)g_userMenuCacheData.intMaxValue;
					}
					else
					{
						// Increment the data by the key scrolling speed
						g_userMenuCacheData.numWordData += g_keypadNumberSpeed;
					}
				}
			break;

			case INTEGER_LONG_TYPE:
			case INTEGER_SPECIAL_TYPE:
			case INTEGER_COUNT_TYPE:
				// Check if the menu type is integer special or integer count and currently set as NO_TRIGGER
				if (((USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_SPECIAL_TYPE) || 
					(USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_COUNT_TYPE)) && 
					(g_userMenuCacheData.numLongData == NO_TRIGGER_CHAR))
				{
					// Check if the boundary condition is not the top (bottom then)
					if (g_userMenuCacheData.boundary != TOP_BOUNDARY)
					{
						// Set the min value						
						g_userMenuCacheData.numLongData = g_userMenuCacheData.intMinValue;

						// Clear the boundary condition
						g_userMenuCacheData.boundary = NO_BOUNDARY;
					}
				}
				// Check if the current integer long data is less than the max
				else if (g_userMenuCacheData.numLongData < g_userMenuCacheData.intMaxValue)
				{
					// Check if the data incremented by the key scrolling speed is greater than the max
					if ((g_userMenuCacheData.numLongData + g_keypadNumberSpeed) > g_userMenuCacheData.intMaxValue)
					{
						// Set the max value
						g_userMenuCacheData.numLongData = g_userMenuCacheData.intMaxValue;
					}
					else
					{
						if ((USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_SPECIAL_TYPE) && (g_unitConfig.unitsOfAir == MILLIBAR_TYPE))
						{
							// Increment the data by the key scrolling speed
							g_userMenuCacheData.numLongData += g_keypadNumberSpeed * GetAirTriggerIncrementForMB();
						}
						else if ((USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_SPECIAL_TYPE) && (g_unitConfig.unitsOfAir == PSI_TYPE))
						{
							// Increment the data by the key scrolling speed
							g_userMenuCacheData.numLongData += g_keypadNumberSpeed * GetAirTriggerIncrementForPSI();
						}
						else
						{
							// Increment the data by the key scrolling speed
							g_userMenuCacheData.numLongData += g_keypadNumberSpeed;
						}
					}
				}
				// Check if the menu type is integer special or integer count
				else if ((USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_SPECIAL_TYPE) ||
						(USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_COUNT_TYPE))
				{
					// If here, then the current data is equal to the max

					// Set the current data to the NO_TRIGGER value
					g_userMenuCacheData.numLongData = NO_TRIGGER_CHAR;

					// Set the boundary condition to the top
					g_userMenuCacheData.boundary = TOP_BOUNDARY;
				}
			break;

			case FLOAT_TYPE:
			case FLOAT_SPECIAL_TYPE:
			case FLOAT_WITH_N_TYPE:
				// Check if the menu type is float special and currently set as NO_TRIGGER
				if ((USER_MENU_TYPE(g_userMenuCachePtr) == FLOAT_SPECIAL_TYPE) && 
					(g_userMenuCacheData.floatData == NO_TRIGGER_CHAR))
				{
					// Check if the boundary condition is not the top (bottom then)
					if (g_userMenuCacheData.boundary != TOP_BOUNDARY)
					{
						// Set the min value						
						g_userMenuCacheData.floatData = g_userMenuCacheData.floatMinValue;

						// Clear the boundary condition
						g_userMenuCacheData.boundary = NO_BOUNDARY;
					}
				}
				// Check if the current float long data is less than the max
				else if (g_userMenuCacheData.floatData < g_userMenuCacheData.floatMaxValue)
				{
					// Check if the data incremented by the key scrolling speed times the increment value is greater than the max
					if ((g_userMenuCacheData.floatData + (g_userMenuCacheData.floatIncrement * g_keypadNumberSpeed)) >
						g_userMenuCacheData.floatMaxValue)
					{
						// Set the max value
						g_userMenuCacheData.floatData = g_userMenuCacheData.floatMaxValue;
					}
					else
					{
						// Increment the data by the key scrolling speed times the increment value
						g_userMenuCacheData.floatData += (g_userMenuCacheData.floatIncrement * g_keypadNumberSpeed);
					}
				}
				// Check if the menu type is float special
				else if (USER_MENU_TYPE(g_userMenuCachePtr) == FLOAT_SPECIAL_TYPE)
				{
					// If here, then the current data is equal to the max

					// Set the current data to the NO_TRIGGER value
					g_userMenuCacheData.floatData = NO_TRIGGER_CHAR;

					// Set the boundary condition to the top
					g_userMenuCacheData.boundary = TOP_BOUNDARY;
				}
			break;
		}
	}
	else // direction == DOWN_ARROW_KEY, need to decrement the number
	{
		switch (USER_MENU_TYPE(g_userMenuCachePtr))
		{
			case INTEGER_BYTE_TYPE:
			case INTEGER_BYTE_OFFSET_TYPE:
#if 1 /* New rolling increment by 5 for Percent */
				// Check if the unit type is Percent and if the current integer byte data is greater than the min
				if ((unitTypes[USER_MENU_DEFAULT_TYPE(g_userMenuCachePtr)].type == PERCENT_TYPE) && (g_userMenuCacheData.numByteData > g_userMenuCacheData.intMinValue))
				{
					// Check if the data decremented by the key scrolling speed is less than the min
					if ((g_userMenuCacheData.numByteData - PERCENT_INCREMENT_DEFAULT) < g_userMenuCacheData.intMinValue)
					{
						// Set the min value
						g_userMenuCacheData.numByteData = (uint8)g_userMenuCacheData.intMinValue;
					}
					else
					{
						// Decrement the data by the key scrolling speed
						g_userMenuCacheData.numByteData -= PERCENT_INCREMENT_DEFAULT;
					}
				}
				else
#endif
				// Check if the current integer byte data is greater than the min
				if (g_userMenuCacheData.numByteData > g_userMenuCacheData.intMinValue)
				{
					// Check if the current data is greater than the key scrolling speed
					if (g_userMenuCacheData.numByteData > g_keypadNumberSpeed)
					{
						// Check if the data decremented by the key scrolling speed is less than the min
						if ((g_userMenuCacheData.numByteData - g_keypadNumberSpeed) < g_userMenuCacheData.intMinValue)
						{
							// Set the min value
							g_userMenuCacheData.numByteData = (uint8)g_userMenuCacheData.intMinValue;
						}
						else
						{
							// Decrement the data by the key scrolling speed
							g_userMenuCacheData.numByteData -= g_keypadNumberSpeed;
						}
					}
					else // current data is less than the key scrolling speed
					{
						// Set the min value
						g_userMenuCacheData.numByteData = (uint8)g_userMenuCacheData.intMinValue;
					}
				}
			break;

			case INTEGER_WORD_TYPE:
			case INTEGER_WORD_FIXED_TYPE:
			case INTEGER_WORD_OFFSET_TYPE:
				// Check if the current integer word data is greater than the min
				if (g_userMenuCacheData.numWordData > g_userMenuCacheData.intMinValue)
				{
					// Check if the current data is greater than the key scrolling speed
					if (g_userMenuCacheData.numWordData > g_keypadNumberSpeed)
					{
						// Check if the data decremented by the key scrolling speed is less than the min
						if ((g_userMenuCacheData.numWordData - g_keypadNumberSpeed) < g_userMenuCacheData.intMinValue)
						{
							// Set the min value
							g_userMenuCacheData.numWordData = (uint16)g_userMenuCacheData.intMinValue;
						}
						else
						{
							// Decrement the data by the key scrolling speed
							g_userMenuCacheData.numWordData -= g_keypadNumberSpeed;
						}
					}
					else // current data is less than the key scrolling speed
					{
						// Set the min value
						g_userMenuCacheData.numWordData = (uint16)g_userMenuCacheData.intMinValue;
					}
				}
			break;

			case INTEGER_LONG_TYPE:
			case INTEGER_SPECIAL_TYPE:
			case INTEGER_COUNT_TYPE:
				// Check if the menu type is integer special or integer count and currently set as NO_TRIGGER
				if (((USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_SPECIAL_TYPE) || 
					(USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_COUNT_TYPE)) && 
					(g_userMenuCacheData.numLongData == NO_TRIGGER_CHAR))
				{
					// Check if the boundary condition is not the bottom (top then)
					if (g_userMenuCacheData.boundary != BOTTOM_BOUNDARY)
					{
						// Set the max value						
						g_userMenuCacheData.numLongData = g_userMenuCacheData.intMaxValue;

						// Clear the boundary condition
						g_userMenuCacheData.boundary = NO_BOUNDARY;
					}
				}
				// Check if the current integer long data is greater than the min
				else if (g_userMenuCacheData.numLongData > g_userMenuCacheData.intMinValue)
				{
					// Check if the current data is greater than the key scrolling speed
					if (g_userMenuCacheData.numLongData > g_keypadNumberSpeed)
					{
						// Check if the data decremented by the key scrolling speed is less than the min
						if ((g_userMenuCacheData.numLongData - g_keypadNumberSpeed) < g_userMenuCacheData.intMinValue)
						{
							// Set the min value
							g_userMenuCacheData.numLongData = g_userMenuCacheData.intMinValue;
						}
						else
						{
							if ((USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_SPECIAL_TYPE) && (g_unitConfig.unitsOfAir == MILLIBAR_TYPE))
							{
								if ((g_userMenuCacheData.numLongData - (g_keypadNumberSpeed * GetAirTriggerIncrementForMB())) > g_userMenuCacheData.intMaxValue)
								{
									// Set the min value
									g_userMenuCacheData.numLongData = g_userMenuCacheData.intMinValue;
								}
								else
								{
									// Decrement the data by the key scrolling speed
									g_userMenuCacheData.numLongData -= g_keypadNumberSpeed * GetAirTriggerIncrementForMB();
								}
							}
							else if ((USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_SPECIAL_TYPE) && (g_unitConfig.unitsOfAir == PSI_TYPE))
							{
								if ((g_userMenuCacheData.numLongData - (g_keypadNumberSpeed * GetAirTriggerIncrementForPSI())) > g_userMenuCacheData.intMaxValue)
								{
									// Set the min value
									g_userMenuCacheData.numLongData = g_userMenuCacheData.intMinValue;
								}
								else
								{
									// Decrement the data by the key scrolling speed
									g_userMenuCacheData.numLongData -= g_keypadNumberSpeed * GetAirTriggerIncrementForPSI();
								}
							}
							else // ((USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_SPECIAL_TYPE) && (g_unitConfig.unitsOfAir == DECIBEL_TYPE))
							{
								// Decrement the data by the key scrolling speed
								g_userMenuCacheData.numLongData -= g_keypadNumberSpeed;
							}
						}
					}
					else // current data is less than the key scrolling speed
					{
						g_userMenuCacheData.numLongData = g_userMenuCacheData.intMinValue;
					}
				}
				// Check if the menu type is integer special or integer count
				else if ((USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_SPECIAL_TYPE) ||
						(USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_COUNT_TYPE))
				{
					// If here, then the current data is equal to the min

					// Set the current data to the NO_TRIGGER value
					g_userMenuCacheData.numLongData = NO_TRIGGER_CHAR;

					// Set the boundary condition to the bottom
					g_userMenuCacheData.boundary = BOTTOM_BOUNDARY;
				}
			break;

			case FLOAT_TYPE:
			case FLOAT_SPECIAL_TYPE:
			case FLOAT_WITH_N_TYPE:
				// Check if the menu type is float special and currently set as NO_TRIGGER
				if ((USER_MENU_TYPE(g_userMenuCachePtr) == FLOAT_SPECIAL_TYPE) && 
					(g_userMenuCacheData.floatData == NO_TRIGGER_CHAR))
				{
					// Check if the boundary condition is not the bottom (top then)
					if (g_userMenuCacheData.boundary != BOTTOM_BOUNDARY)
					{
						// Set the max value						
						g_userMenuCacheData.floatData = g_userMenuCacheData.floatMaxValue;

						// Clear the boundary condition
						g_userMenuCacheData.boundary = NO_BOUNDARY;
					}
				}
				// Check if the current float long data is greater than the min
				else if (g_userMenuCacheData.floatData > g_userMenuCacheData.floatMinValue)
				{
					// Check if the current data is greater than the key scrolling speed times the increment value
					if (g_userMenuCacheData.floatData > (g_userMenuCacheData.floatIncrement * g_keypadNumberSpeed))
					{
						// Check if the data decremented by the key scrolling speed times the increment value is less than the min
						if ((g_userMenuCacheData.floatData - (g_userMenuCacheData.floatIncrement * g_keypadNumberSpeed)) <
							g_userMenuCacheData.floatMinValue)
						{
							// Set the min value
							g_userMenuCacheData.floatData = g_userMenuCacheData.floatMinValue;
						}
						else
						{
							// Decrement the data by the key scrolling speed times the increment value
							g_userMenuCacheData.floatData -= (g_userMenuCacheData.floatIncrement * g_keypadNumberSpeed);
						}
					}
					else // current data is less than the key scrolling speed
					{
						g_userMenuCacheData.floatData = g_userMenuCacheData.floatMinValue;
					}
				}
				// Check if the menu type is float special
				else if (USER_MENU_TYPE(g_userMenuCachePtr) == FLOAT_SPECIAL_TYPE)
				{
					// If here, then the current data is equal to the min

					// Set the current data to the NO_TRIGGER value
					g_userMenuCacheData.floatData = NO_TRIGGER_CHAR;

					// Set the boundary condition to the bottom
					g_userMenuCacheData.boundary = BOTTOM_BOUNDARY;
				}
			break;
		}
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void CopyMenuToCache(USER_MENU_STRUCT* currentMenu)
{
	int i;
	
	// Set the user menu handler (data value in the last menu index)
	g_userMenuHandler = (void*)currentMenu[(USER_MENU_TOTAL_ITEMS(currentMenu) - 1)].data;

	// Clear the user menu display cache
	memset(&g_userMenuCache, 0, sizeof(g_userMenuCache));

	// Copy the menu contents over to the user menu display cache (minus the last line with the menu handler)
	for (i = 0; i < USER_MENU_DISPLAY_ITEMS(currentMenu); i++)
	{
		// Check if the menu line pre-number value is zero
		if (currentMenu[i].preNum == 0)
		{
			// Copy the pre tag, text entry and post tag to the text of the current user menu display line
			sprintf(g_userMenuCachePtr[i].text, "%s%s%s", g_menuTags[currentMenu[i].preTag].text,
					getLangText(currentMenu[i].textEntry), g_menuTags[currentMenu[i].postTag].text);
		}
		else // A pre-number is present
		{
			// Copy the pre tag, pre num, text entry and post tag to the text of the current user menu display line
			sprintf(g_userMenuCachePtr[i].text, "%s%d %s%s", g_menuTags[currentMenu[i].preTag].text,
					(uint16)currentMenu[i].preNum, getLangText(currentMenu[i].textEntry), 
					g_menuTags[currentMenu[i].postTag].text);
		}

		// Copy the menu data over
		g_userMenuCachePtr[i].data = currentMenu[i].data;
	}
	
	// Copy ".end." to the text to signal the end of the user menu display text data
	strcpy(g_userMenuCachePtr[i].text, ".end.");
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void CopyDataToCache(void* data)
{
	// Switch on the menu type
	switch (USER_MENU_TYPE(g_userMenuCachePtr))
	{
		case STRING_TYPE:
		case STRING_SPECIAL_TYPE:
			// Clear the data cache text string
			memset(&(g_userMenuCacheData.text[0]), 0, sizeof(g_userMenuCacheData.text));
		
			// Check to make sure the data pointer isn't null
			if (data != NULL)
			{
				// Copy the string into the data cache
				strcpy(g_userMenuCacheData.text, (char*)data);
				debug("User Input Text <%s>, Length: %d\r\n", g_userMenuCacheData.text, strlen(g_userMenuCacheData.text));
			}			

			// Set the current index to the string length
			g_userMenuCachePtr[CURRENT_TEXT_INDEX].data = strlen(g_userMenuCacheData.text);
		break;
		
		case INTEGER_BYTE_TYPE:
		case INTEGER_BYTE_OFFSET_TYPE:
			// Clear the data cache byte data
			g_userMenuCacheData.numByteData = 0;
			
			// Check to make sure the data pointer isn't null
			if (data != NULL)
			{
				// Copy the byte into the data cache
				g_userMenuCacheData.numByteData = *((uint8*)data);
			}

			// Check if the byte data is greater than the max or less than the min
			if ((g_userMenuCacheData.numByteData > g_userMenuCacheData.intMaxValue) ||
				(g_userMenuCacheData.numByteData < g_userMenuCacheData.intMinValue))
			{
				if (USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_BYTE_OFFSET_TYPE)
				{
					// Set the default value in the word data to the average of the min and max
					g_userMenuCacheData.numByteData = (uint8)((g_userMenuCacheData.intMinValue + g_userMenuCacheData.intMaxValue) / 2);
				}
				else // INTEGER_BYTE_TYPE
				{
					// Set the default value in the byte data
					g_userMenuCacheData.numByteData = (uint8)g_userMenuCacheData.intDefault;
				}

				debug("User Input Integer not within Range, Setting to Default: %d\r\n", g_userMenuCacheData.numByteData);
			}
			
			// Set the unit text pointer to the default unit type for the data
			g_userMenuCacheData.unitText = unitTypes[USER_MENU_DEFAULT_TYPE(g_userMenuCachePtr)].text;

			// Check if units is metric and the alternative unit type is set
			if ((g_unitConfig.unitsOfMeasure == METRIC_TYPE) && (USER_MENU_ALT_TYPE(g_userMenuCachePtr) != NO_ALT_TYPE))
			{
				// Adjust the byte data, min and max values by the units conversion for display purposes
				g_userMenuCacheData.numByteData = (uint8)(g_userMenuCacheData.numByteData * unitTypes[USER_MENU_ALT_TYPE(g_userMenuCachePtr)].conversion);
				g_userMenuCacheData.intMinValue = (uint32)(g_userMenuCacheData.intMinValue * unitTypes[USER_MENU_ALT_TYPE(g_userMenuCachePtr)].conversion);
				g_userMenuCacheData.intMaxValue = (uint32)(g_userMenuCacheData.intMaxValue * unitTypes[USER_MENU_ALT_TYPE(g_userMenuCachePtr)].conversion);

				// Set the unit text pointer to the alternative unit type
				g_userMenuCacheData.unitText = unitTypes[USER_MENU_ALT_TYPE(g_userMenuCachePtr)].text;
			}
		break;
		
		case INTEGER_WORD_TYPE:
		case INTEGER_WORD_FIXED_TYPE:
		case INTEGER_WORD_OFFSET_TYPE:
			// Clear the data cache word data
			g_userMenuCacheData.numWordData = 0;
			
			// Check to make sure the data pointer isn't null
			if (data != NULL)
			{
				// Copy the word into the data cache
				g_userMenuCacheData.numWordData = *((uint16*)data);
			}

			// Check if the word data is greater than the max or less than the min
			if ((g_userMenuCacheData.numWordData > g_userMenuCacheData.intMaxValue) ||
				(g_userMenuCacheData.numWordData < g_userMenuCacheData.intMinValue))
			{
				if (USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_WORD_OFFSET_TYPE)
				{
					// Set the default value in the word data to the average of the min and max
					g_userMenuCacheData.numWordData = (uint16)((g_userMenuCacheData.intMinValue + g_userMenuCacheData.intMaxValue) / 2);
				}
				else // INTEGER_WORD_TYPE, INTEGER_WORD_FIXED_TYPE
				{
					// Set the default value in the word data
					g_userMenuCacheData.numWordData = (uint16)g_userMenuCacheData.intDefault;
				}

				debug("User Input Integer not within Range, Setting to Default: %d\r\n", g_userMenuCacheData.numWordData);
			}

			// Set the unit text pointer to the default unit type for the data
			g_userMenuCacheData.unitText = unitTypes[USER_MENU_DEFAULT_TYPE(g_userMenuCachePtr)].text;

			// Check if units is metric and the alternative unit type is set
			if ((g_unitConfig.unitsOfMeasure == METRIC_TYPE) && (USER_MENU_ALT_TYPE(g_userMenuCachePtr) != NO_ALT_TYPE))
			{
				// Adjust the word data, min and max values by the units conversion for display purposes
				g_userMenuCacheData.numWordData = (uint16)(g_userMenuCacheData.numWordData * unitTypes[USER_MENU_ALT_TYPE(g_userMenuCachePtr)].conversion);
				g_userMenuCacheData.intMinValue = (uint32)(g_userMenuCacheData.intMinValue * unitTypes[USER_MENU_ALT_TYPE(g_userMenuCachePtr)].conversion);
				g_userMenuCacheData.intMaxValue = (uint32)(g_userMenuCacheData.intMaxValue * unitTypes[USER_MENU_ALT_TYPE(g_userMenuCachePtr)].conversion);

				// Set the unit text pointer to the alternative unit type
				g_userMenuCacheData.unitText = unitTypes[USER_MENU_ALT_TYPE(g_userMenuCachePtr)].text;
			}
		break;
		
		case INTEGER_LONG_TYPE:
		case INTEGER_SPECIAL_TYPE:
		case INTEGER_COUNT_TYPE:
			// Clear the data cache long data
			g_userMenuCacheData.numLongData = 0;

			// Clear the boundary condition
			g_userMenuCacheData.boundary = NO_BOUNDARY;
			
			// Check to make sure the data pointer isn't null
			if (data != NULL)
			{
				// Copy the long into the data cache
				g_userMenuCacheData.numLongData = *((uint32*)data);
			}

			// Check if the menu type is integer long or the data isn't NO_TRIGGER
			if ((USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_LONG_TYPE) || (g_userMenuCacheData.numLongData != NO_TRIGGER_CHAR))
			{
				// Check if the long data is greater than the max or less than the min
				if ((g_userMenuCacheData.numLongData > g_userMenuCacheData.intMaxValue) ||
					(g_userMenuCacheData.numLongData < g_userMenuCacheData.intMinValue))
				{
					// Set the default value in the long data
					g_userMenuCacheData.numLongData = g_userMenuCacheData.intDefault;
					debug("User Input Integer not within Range, Setting to Default: %d\r\n", g_userMenuCacheData.numLongData);
				}
			}

			// Set the unit text pointer to the default unit type for the data
			g_userMenuCacheData.unitText = unitTypes[USER_MENU_DEFAULT_TYPE(g_userMenuCachePtr)].text;

			// Check if the decibels type is set as the default type
			if (USER_MENU_DEFAULT_TYPE(g_userMenuCachePtr) == DB_TYPE)
			{
				// Check if the factory setup is valid and A weighting is enabled
				if ((!g_factorySetupRecord.invalid) && (g_factorySetupRecord.aWeightOption == ENABLED))
				{
					// Set the unit text pointer to the alternative type
					g_userMenuCacheData.unitText = unitTypes[USER_MENU_ALT_TYPE(g_userMenuCachePtr)].text;
				}
			}
			// Check if units is metric and the alternative unit type is set
			else if ((g_unitConfig.unitsOfMeasure == METRIC_TYPE) && (USER_MENU_ALT_TYPE(g_userMenuCachePtr) != NO_ALT_TYPE))
			{
				// Check if the menu type isn't integer count
				if (USER_MENU_TYPE(g_userMenuCachePtr) != INTEGER_COUNT_TYPE)
				{
					// Check if the data isn't equal to NO_TRIGGER
					if (g_userMenuCacheData.numLongData != NO_TRIGGER_CHAR)
					{
						// Adjust the long data by the units conversion for display purposes
						g_userMenuCacheData.numLongData = (uint32)(g_userMenuCacheData.numLongData * unitTypes[USER_MENU_ALT_TYPE(g_userMenuCachePtr)].conversion);
					}

					// Adjust the min and max values by the units conversion for display purposes
					g_userMenuCacheData.intMinValue = (uint32)(g_userMenuCacheData.intMinValue * unitTypes[USER_MENU_ALT_TYPE(g_userMenuCachePtr)].conversion);
					g_userMenuCacheData.intMaxValue = (uint32)(g_userMenuCacheData.intMaxValue * unitTypes[USER_MENU_ALT_TYPE(g_userMenuCachePtr)].conversion);
				}

				// Set the unit text pointer to the alternative unit type
				g_userMenuCacheData.unitText = unitTypes[USER_MENU_ALT_TYPE(g_userMenuCachePtr)].text;
			}
		break;
		
		case FLOAT_TYPE:
		case FLOAT_SPECIAL_TYPE:
		case FLOAT_WITH_N_TYPE:
			// Clear the data cache float data
			g_userMenuCacheData.floatData = 0;

			// Clear the boundary condition
			g_userMenuCacheData.boundary = NO_BOUNDARY;

			// Check to make sure the data pointer isn't null
			if (data != NULL)
			{
				// Copy the float into the data cache
				g_userMenuCacheData.floatData = *((float*)data);
			}

			// Check if the menu type isn't float special or the data isn't NO_TRIGGER
			if ((USER_MENU_TYPE(g_userMenuCachePtr) != FLOAT_SPECIAL_TYPE) || (g_userMenuCacheData.floatData != NO_TRIGGER_CHAR))
			{
				// Check if the float data is greater than the max or less than the min
				if ((g_userMenuCacheData.floatData > g_userMenuCacheData.floatMaxValue) ||
					(g_userMenuCacheData.floatData < g_userMenuCacheData.floatMinValue))
				{
					// Set the default value in the long data
					g_userMenuCacheData.floatData = g_userMenuCacheData.floatDefault;
					debug("User Input Float not within Range, Setting to Default: %f\r\n", g_userMenuCacheData.floatData);
				}
			}

			// Set the unit text pointer to the default unit type for the data
			g_userMenuCacheData.unitText = unitTypes[USER_MENU_DEFAULT_TYPE(g_userMenuCachePtr)].text;

			// Check if units is metric and the alternative unit type is set
			if ((g_unitConfig.unitsOfMeasure == METRIC_TYPE) && (USER_MENU_ALT_TYPE(g_userMenuCachePtr) != NO_ALT_TYPE))
			{
				// Check if the data isn't equal to NO_TRIGGER
				if (g_userMenuCacheData.floatData != NO_TRIGGER_CHAR)
				{
					// Adjust the float data by the units conversion for display purposes
					g_userMenuCacheData.floatData = (float)(g_userMenuCacheData.floatData * unitTypes[USER_MENU_ALT_TYPE(g_userMenuCachePtr)].conversion);
				}

				// Adjust the min and max values by the units conversion for display purposes
				g_userMenuCacheData.floatMinValue = (float)(g_userMenuCacheData.floatMinValue * unitTypes[USER_MENU_ALT_TYPE(g_userMenuCachePtr)].conversion);
				g_userMenuCacheData.floatMaxValue = (float)(g_userMenuCacheData.floatMaxValue * unitTypes[USER_MENU_ALT_TYPE(g_userMenuCachePtr)].conversion);

				// Set the unit text pointer to the alternative unit type
				g_userMenuCacheData.unitText = unitTypes[USER_MENU_ALT_TYPE(g_userMenuCachePtr)].text;
			}
		break;
		
		case DATE_TIME_TYPE:
			// To be added in the future
		break;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void CopyDataToMenu(MN_LAYOUT_STRUCT* menu_layout)
{
	uint32 charLen = 0;
	int tempRow = USER_MENU_DEFAULT_ROW(g_userMenuCachePtr);
	int i = 0;
	
	// Switch on the menu type
	switch (USER_MENU_TYPE(g_userMenuCachePtr))
	{
		case STRING_TYPE:
		case STRING_SPECIAL_TYPE:
			// Set the specifications line for max chars
			sprintf(g_userMenuCachePtr[MAX_TEXT_CHARS].text, "(%s %lu %s)", getLangText(MAX_TEXT),
					g_userMenuCachePtr[MAX_TEXT_CHARS].data, getLangText(CHARS_TEXT));

			// For the current row to the end of the menu (minus the last line with the menu handler)
			for (i = tempRow; i < USER_MENU_DISPLAY_ITEMS(g_userMenuCachePtr); i++)
			{
				// Clear the user menu display cache
				memset(&(g_userMenuCachePtr[i].text[0]), 0, MAX_CHAR_PER_LN);
			}

			// get the string length
			charLen = strlen(g_userMenuCacheData.text);
			//debug("User Input Data String(%d): <%s>\r\n", charLen, g_userMenuCacheData.text);

			// Check if the string length is zero
			if (charLen == 0)
			{
				// Print the "<EMPTY>" text
				sprintf(g_userMenuCachePtr[tempRow].text, "<%s>", getLangText(EMPTY_TEXT));

				// Set the current line and sub line (column)
				menu_layout->curr_ln = (uint16)tempRow;
				menu_layout->sub_ln = 0;
			}
			else // string length is greater than zero
			{
				// While the number of characters left is greater than 20 (chars per line)
				while ((charLen > 20) && (tempRow < USER_MENU_ACTIVE_ITEMS(g_userMenuCachePtr)))
				{
					// Copy the 20 chars to the user menu display cache
					strncpy(g_userMenuCachePtr[tempRow].text, (char*)&(g_userMenuCacheData.text[(tempRow - 2) * 20]), 20);

					// Increment the row
					tempRow++;

					// Decrement the number of chars left
					charLen -= 20;
				}

				// Check if the number of chars left is less than 20
				if (charLen <= 20)
				{
					// Copy the rest of the chars to the current row
					strcpy(g_userMenuCachePtr[tempRow].text, (char*)&(g_userMenuCacheData.text[(tempRow - 2) * 20]));
				}

				// Set the current line and sub line (column)
				menu_layout->curr_ln = (uint8)((g_userMenuCachePtr[CURRENT_TEXT_INDEX].data / 20) + 
										USER_MENU_DEFAULT_ROW(g_userMenuCachePtr));
				menu_layout->sub_ln = (uint8)((g_userMenuCachePtr[CURRENT_TEXT_INDEX].data % 20) + 1);
			}
			
			//debug("User Input Current Index: %d, Current Line: %d, Sub Line: %d\r\n",
			//		g_userMenuCachePtr[CURRENT_TEXT_INDEX].data, menu_layout->curr_ln, menu_layout->sub_ln);
		break;
		
		case INTEGER_BYTE_TYPE:
		case INTEGER_BYTE_OFFSET_TYPE:
		case INTEGER_WORD_TYPE:
		case INTEGER_WORD_FIXED_TYPE:
		case INTEGER_WORD_OFFSET_TYPE:
		case INTEGER_LONG_TYPE:
		case INTEGER_SPECIAL_TYPE:
		case INTEGER_COUNT_TYPE:
			if (USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_SPECIAL_TYPE)
			{
				g_userMenuCacheData.floatMinValue = (float)g_userMenuCacheData.intMinValue / 10000;
				g_userMenuCacheData.floatMaxValue = (float)g_userMenuCacheData.intMaxValue / 10000;
				g_userMenuCacheData.floatData = (float)g_userMenuCacheData.numLongData / 10000;

				if (g_unitConfig.unitsOfAir == DECIBEL_TYPE)
				{
					// Set the specifications line for the integer type
					sprintf(g_userMenuCachePtr[INTEGER_RANGE].text, "(%lu-%lu%s, N)", g_userMenuCacheData.intMinValue, g_userMenuCacheData.intMaxValue, unitTypes[DB_TYPE].text);
				}
				else if (g_unitConfig.unitsOfAir == MILLIBAR_TYPE)
				{
					// Set the specifications line for the float type
					sprintf(g_userMenuCachePtr[INTEGER_RANGE].text, "(%.3f-%.3f%s,N)", g_userMenuCacheData.floatMinValue, g_userMenuCacheData.floatMaxValue, unitTypes[MB_TYPE].text);
				}
				else // (g_unitConfig.unitsOfAir == PSI_TYPE)
				{
					// Set the specifications line for the float type
					sprintf(g_userMenuCachePtr[INTEGER_RANGE].text, "(%.4f-%.3f%s,N)", g_userMenuCacheData.floatMinValue, g_userMenuCacheData.floatMaxValue, unitTypes[PSI_UNIT_TYPE].text);
				}
				// Check if the data is NO_TRIGGER
				if (g_userMenuCacheData.numLongData == NO_TRIGGER_CHAR)
				{
					// Print a "N"
					sprintf(g_userMenuCachePtr[tempRow].text, "N");
				}
				else
				{
					// Print the data value
					if (g_unitConfig.unitsOfAir == DECIBEL_TYPE)
					{
						sprintf(g_userMenuCachePtr[tempRow].text, "%lu", g_userMenuCacheData.numLongData);
					}
					else // (g_unitConfig.unitsOfAir == MILLIBAR_TYPE) || (g_unitConfig.unitsOfAir == PSI_TYPE)
					{
						sprintf(g_userMenuCachePtr[tempRow].text, "%.4f", g_userMenuCacheData.floatData);
					}
				}
			}
			else if (USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_COUNT_TYPE)
			{
				// Check if the units are metric and no alternative type is set and not the accelerometer
				if ((g_unitConfig.unitsOfMeasure == METRIC_TYPE) && (USER_MENU_ALT_TYPE(g_userMenuCachePtr) != NO_ALT_TYPE) &&
					(g_factorySetupRecord.seismicSensorType < SENSOR_ACC_RANGE_DIVIDER))
				{
					// Init the float increment value adjusted by the units conversion
					g_userMenuCacheData.floatIncrement = ((float)(g_factorySetupRecord.seismicSensorType * unitTypes[USER_MENU_ALT_TYPE(g_userMenuCachePtr)].conversion) /
														(float)(((g_triggerRecord.srec.sensitivity == LOW) ? 200 : 400) * g_bitAccuracyMidpoint));
				}
				else // Imperial or Accelerometer
				{
					// Init the float increment value
					if ((g_factorySetupRecord.seismicSensorType == SENSOR_ACC_832M1_0200) || (g_factorySetupRecord.seismicSensorType == SENSOR_ACC_832M1_0500))
					{
						g_userMenuCacheData.floatIncrement = ((float)(g_factorySetupRecord.seismicSensorType) * (float)ACC_832M1_SCALER / (float)(((g_triggerRecord.srec.sensitivity == LOW) ? 200 : 400) * g_bitAccuracyMidpoint));
					}
					else g_userMenuCacheData.floatIncrement = ((float)(g_factorySetupRecord.seismicSensorType) / (float)(((g_triggerRecord.srec.sensitivity == LOW) ? 200 : 400) * g_bitAccuracyMidpoint));
				}

				// Set the min, max and data count values adjusted by the float increment
				g_userMenuCacheData.floatMinValue = (float)g_userMenuCacheData.intMinValue * g_userMenuCacheData.floatIncrement;
				g_userMenuCacheData.floatMaxValue = (float)g_userMenuCacheData.intMaxValue * g_userMenuCacheData.floatIncrement;
				g_userMenuCacheData.floatData = (float)g_userMenuCacheData.numLongData * g_userMenuCacheData.floatIncrement;
				
				// The following code will check sensor type and sensitivity to auto adjust the accuracy being printed to the screen
				if (((g_factorySetupRecord.seismicSensorType >= SENSOR_20_IN) && (g_factorySetupRecord.seismicSensorType < SENSOR_ACC_RANGE_DIVIDER)) ||
					((g_factorySetupRecord.seismicSensorType > SENSOR_ACC_RANGE_DIVIDER) && (g_triggerRecord.srec.sensitivity == LOW)) ||
					((g_factorySetupRecord.seismicSensorType == SENSOR_10_IN) && (g_triggerRecord.srec.sensitivity == LOW)))
				{
					sprintf(g_userMenuCachePtr[INTEGER_RANGE].text, "(%.3f-%.3f%s,N)",
						g_userMenuCacheData.floatMinValue, g_userMenuCacheData.floatMaxValue, g_userMenuCacheData.unitText);
					sprintf(g_userMenuCachePtr[INTEGER_RANGE+1].text, "(+/- %.4f%s)",
						g_userMenuCacheData.floatIncrement, g_userMenuCacheData.unitText);

					if (g_userMenuCacheData.numLongData == NO_TRIGGER_CHAR)
						sprintf(g_userMenuCachePtr[tempRow].text, "N");
					else
						sprintf(g_userMenuCachePtr[tempRow].text, "%.4f", g_userMenuCacheData.floatData);
				}
				else if (((g_factorySetupRecord.seismicSensorType == SENSOR_10_IN) && (g_triggerRecord.srec.sensitivity == HIGH)) ||
						((g_factorySetupRecord.seismicSensorType > SENSOR_ACC_RANGE_DIVIDER) && (g_triggerRecord.srec.sensitivity == HIGH)) ||
						((g_factorySetupRecord.seismicSensorType == SENSOR_5_IN) && (g_triggerRecord.srec.sensitivity == LOW)))
				{
					sprintf(g_userMenuCachePtr[INTEGER_RANGE].text, "(%.4f-%.3f%s,N)",
						g_userMenuCacheData.floatMinValue, g_userMenuCacheData.floatMaxValue, g_userMenuCacheData.unitText);
					sprintf(g_userMenuCachePtr[INTEGER_RANGE+1].text, "(+/- %.5f%s)",
						g_userMenuCacheData.floatIncrement, g_userMenuCacheData.unitText);

					if (g_userMenuCacheData.numLongData == NO_TRIGGER_CHAR)
						sprintf(g_userMenuCachePtr[tempRow].text, "N");
					else
						sprintf(g_userMenuCachePtr[tempRow].text, "%.5f", g_userMenuCacheData.floatData);
				}
				else if (((g_factorySetupRecord.seismicSensorType == SENSOR_5_IN) && (g_triggerRecord.srec.sensitivity == HIGH)) ||
						((g_factorySetupRecord.seismicSensorType == SENSOR_2_5_IN) && (g_triggerRecord.srec.sensitivity == LOW)))
				{
					sprintf(g_userMenuCachePtr[INTEGER_RANGE].text, "(%.5f-%.3f%s,N)",
						g_userMenuCacheData.floatMinValue, g_userMenuCacheData.floatMaxValue, g_userMenuCacheData.unitText);
					sprintf(g_userMenuCachePtr[INTEGER_RANGE+1].text, "(+/- %.6f%s)",
						g_userMenuCacheData.floatIncrement, g_userMenuCacheData.unitText);

					if (g_userMenuCacheData.numLongData == NO_TRIGGER_CHAR)
						sprintf(g_userMenuCachePtr[tempRow].text, "N");
					else
						sprintf(g_userMenuCachePtr[tempRow].text, "%.6f", g_userMenuCacheData.floatData);
				}
				else // ((g_factorySetupRecord.seismicSensorType == SENSOR_2_5_IN) && (g_triggerRecord.srec.sensitivity == HIGH))
				{
					if (g_unitConfig.unitsOfMeasure == IMPERIAL_TYPE)
						sprintf(g_userMenuCachePtr[INTEGER_RANGE].text, "(%.6f-%.3f%s,N)",
							g_userMenuCacheData.floatMinValue, g_userMenuCacheData.floatMaxValue, g_userMenuCacheData.unitText);
					else
						sprintf(g_userMenuCachePtr[INTEGER_RANGE].text, "(%0.6f-%.3f%s,N)",
							g_userMenuCacheData.floatMinValue, g_userMenuCacheData.floatMaxValue, g_userMenuCacheData.unitText);
					sprintf(g_userMenuCachePtr[INTEGER_RANGE+1].text, "(+/- %.7f%s)",
						g_userMenuCacheData.floatIncrement, g_userMenuCacheData.unitText);

					if (g_userMenuCacheData.numLongData == NO_TRIGGER_CHAR)
						sprintf(g_userMenuCachePtr[tempRow].text, "N");
					else
						sprintf(g_userMenuCachePtr[tempRow].text, "%.7f", g_userMenuCacheData.floatData);
				}
			}
			else //(USER_MENU_TYPE(g_userMenuCachePtr) != INTEGER_SPECIAL_TYPE, INTEGER_COUNT_TYPE
			{
				if (USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_WORD_FIXED_TYPE)
				{
					// Set the specifications line
					sprintf(g_userMenuCachePtr[INTEGER_RANGE].text, "%s: %04lu-%04lu %s", getLangText(RANGE_TEXT),
						g_userMenuCacheData.intMinValue, g_userMenuCacheData.intMaxValue, g_userMenuCacheData.unitText);
				}
				else if ((USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_BYTE_OFFSET_TYPE) || (USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_WORD_OFFSET_TYPE))
				{
					// Set the specifications line
					sprintf(g_userMenuCachePtr[INTEGER_RANGE].text, "%s: (%d)-(%d) %s", getLangText(RANGE_TEXT),
						(int)(g_userMenuCacheData.intMinValue - g_userMenuCacheData.intDefault), (int)(g_userMenuCacheData.intMaxValue - g_userMenuCacheData.intDefault), g_userMenuCacheData.unitText);
				}
				else // INTEGER_BYTE_TYPE, INTEGER_WORD_TYPE, INTEGER_LONG_TYPE
				{
					// Set the specifications line
					sprintf(g_userMenuCachePtr[INTEGER_RANGE].text, "%s: %lu-%lu %s", getLangText(RANGE_TEXT),
						g_userMenuCacheData.intMinValue, g_userMenuCacheData.intMaxValue, g_userMenuCacheData.unitText);
				}

				// Print the data based on the formats for each type
				if (USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_BYTE_TYPE)
					sprintf(g_userMenuCachePtr[tempRow].text, "%d", g_userMenuCacheData.numByteData);
				else if (USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_BYTE_OFFSET_TYPE)
					sprintf(g_userMenuCachePtr[tempRow].text, "%d", (int)(g_userMenuCacheData.numByteData - g_userMenuCacheData.intDefault));
				else if (USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_WORD_TYPE)
					sprintf(g_userMenuCachePtr[tempRow].text, "%d", g_userMenuCacheData.numWordData);
				else if (USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_WORD_FIXED_TYPE)
					sprintf(g_userMenuCachePtr[tempRow].text, "%04d", g_userMenuCacheData.numWordData);
				else if (USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_WORD_OFFSET_TYPE)
					sprintf(g_userMenuCachePtr[tempRow].text, "%d", (int)(g_userMenuCacheData.numWordData - g_userMenuCacheData.intDefault));
				else if (USER_MENU_TYPE(g_userMenuCachePtr) == INTEGER_LONG_TYPE)
					sprintf(g_userMenuCachePtr[tempRow].text, "%lu", g_userMenuCacheData.numLongData);
			}
		break;
		
		case FLOAT_TYPE:
		case FLOAT_SPECIAL_TYPE:
		case FLOAT_WITH_N_TYPE:
			if (USER_MENU_TYPE(g_userMenuCachePtr) == FLOAT_SPECIAL_TYPE)
			{
				// Set the specifications line
				sprintf(g_userMenuCachePtr[FLOAT_RANGE].text, "(%.2f-%.2f%s,N)",
					g_userMenuCacheData.floatMinValue, g_userMenuCacheData.floatMaxValue, g_userMenuCacheData.unitText);
				sprintf(g_userMenuCachePtr[FLOAT_RANGE+1].text, "(+/- %.3f%s)",
					g_userMenuCacheData.floatIncrement, g_userMenuCacheData.unitText);

				if (g_userMenuCacheData.floatData == NO_TRIGGER_CHAR)
					sprintf(g_userMenuCachePtr[tempRow].text, "N");
				else
					sprintf(g_userMenuCachePtr[tempRow].text, "%.2f", g_userMenuCacheData.floatData);
			}
			else if (USER_MENU_TYPE(g_userMenuCachePtr) == FLOAT_WITH_N_TYPE)
			{
				sprintf(g_userMenuCachePtr[FLOAT_RANGE].text, "%s: N,%.0f-%.0f %s", getLangText(RANGE_TEXT),
					(g_userMenuCacheData.floatMinValue + 1), g_userMenuCacheData.floatMaxValue, g_userMenuCacheData.unitText);

				if (g_userMenuCacheData.floatData == 0.0)
					sprintf(g_userMenuCachePtr[tempRow].text, "N");
				else
					sprintf(g_userMenuCachePtr[tempRow].text, "%.0f", g_userMenuCacheData.floatData);
			}
			// The following auto adjusts the formats based on the increment value
			else if (g_userMenuCacheData.floatIncrement >= (float)(1.0))
			{
				sprintf(g_userMenuCachePtr[FLOAT_RANGE].text, "%s: %.0f-%.0f %s", getLangText(RANGE_TEXT),
					g_userMenuCacheData.floatMinValue, g_userMenuCacheData.floatMaxValue, g_userMenuCacheData.unitText);
				sprintf(g_userMenuCachePtr[tempRow].text, "%.0f", g_userMenuCacheData.floatData);
			}
			else if (g_userMenuCacheData.floatIncrement >= (float)(0.1))
			{
				sprintf(g_userMenuCachePtr[FLOAT_RANGE].text, "%s: %.1f-%.1f %s", getLangText(RANGE_TEXT),
					g_userMenuCacheData.floatMinValue, g_userMenuCacheData.floatMaxValue, g_userMenuCacheData.unitText);
				sprintf(g_userMenuCachePtr[tempRow].text, "%.1f", g_userMenuCacheData.floatData);
			}
			else if (g_userMenuCacheData.floatIncrement >= (float)(0.01))
			{
				sprintf(g_userMenuCachePtr[FLOAT_RANGE].text, "%s: %.2f-%.2f %s", getLangText(RANGE_TEXT),
					g_userMenuCacheData.floatMinValue, g_userMenuCacheData.floatMaxValue, g_userMenuCacheData.unitText);
				sprintf(g_userMenuCachePtr[tempRow].text, "%.2f", g_userMenuCacheData.floatData);
			}
			else if (g_userMenuCacheData.floatIncrement >= (float)(0.001))
			{
				sprintf(g_userMenuCachePtr[FLOAT_RANGE].text, "%s: %.3f-%.3f %s", getLangText(RANGE_TEXT),
					g_userMenuCacheData.floatMinValue, g_userMenuCacheData.floatMaxValue, g_userMenuCacheData.unitText);
				sprintf(g_userMenuCachePtr[tempRow].text, "%.3f", g_userMenuCacheData.floatData);
			}
			else
			{
				sprintf(g_userMenuCachePtr[FLOAT_RANGE].text, "%s: %.4f-%.4f %s", getLangText(RANGE_TEXT),
					g_userMenuCacheData.floatMinValue, g_userMenuCacheData.floatMaxValue, g_userMenuCacheData.unitText);
				sprintf(g_userMenuCachePtr[tempRow].text, "%.4f", g_userMenuCacheData.floatData);
			}
		break;
		
		case DATE_TIME_TYPE:
			// To be added in the future
		break;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16 FindCurrentItemEntry(uint16 startLine, uint32 item)
{
	uint16 i;
	uint16 totalMenuElements = (uint8)(USER_MENU_DISPLAY_ITEMS(g_userMenuCachePtr));

	for (i = startLine; i < totalMenuElements; i++)
	{
		// Check if the current item matches the current index
		if (g_userMenuCachePtr[i].data == item)
			// Return the current entry
			return (i);
	}
	
	// Didnt find item, return default entry
	return (USER_MENU_DEFAULT_ITEM(g_userMenuCachePtr));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void RemoveExtraSpaces(void)
{
	uint16 i = (uint16)strlen(g_userMenuCacheData.text);

	// While the index is greater than the first position and the current position minus one is a space
	while ((i > 0) && (g_userMenuCacheData.text[i - 1] == ' '))
	{
		// Set the space character to be a null
		g_userMenuCacheData.text[i - 1] = '\0';
		i--;
	}
}
