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
#include "Menu.h"
#include "Display.h"
#include "OldUart.h"
#include "Keypad.h"
#include "TextTypes.h"
#include "Sensor.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define MAIN_MN_TABLE_SIZE		10 
#define MAIN_WND_STARTING_COL	6
#define MAIN_WND_END_COL		127 
#define MAIN_WND_STARTING_ROW	8
#define MAIN_WND_END_ROW		55
#define MAIN_MN_TBL_START_LINE	0

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
#include "Globals.h"
extern USER_MENU_STRUCT modeMenu[];
extern USER_MENU_STRUCT helpMenu[];

///----------------------------------------------------------------------------
///	Local Scope Globals
///----------------------------------------------------------------------------
static TEMP_MENU_DATA_STRUCT s_mainMenuTable[MAIN_MN_TABLE_SIZE] = {
{MAIN_PRE_TAG, NOMIS_MAIN_MENU_TEXT, MAIN_POST_TAG},
{NO_TAG, SELECT_TEXT, NO_TAG},
{NO_TAG, NULL_TEXT, NO_TAG},
{ITEM_1, SELF_TRIGGER_TEXT, NO_TAG},
{ITEM_2, BAR_GRAPH_TEXT, NO_TAG},
{ITEM_3, COMBO_TEXT, NO_TAG},
{ITEM_4, SAVED_SETTINGS_TEXT, NO_TAG},
#if 1 /* Test shortcut to Cal Setup menu */
{ITEM_5, SENSOR_CALIBRATION_TEXT, NO_TAG},
#endif
{NO_TAG, TOTAL_TEXT_STRINGS, NO_TAG}
};

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------
void MainMenu(INPUT_MSG_STRUCT);
void MainMenuProc(INPUT_MSG_STRUCT, WND_LAYOUT_STRUCT*, MN_LAYOUT_STRUCT*);
void MainMenuScroll(char, char, MN_LAYOUT_STRUCT*);

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MainMenu(INPUT_MSG_STRUCT msg)
{ 
	static WND_LAYOUT_STRUCT wnd_layout;
	static MN_LAYOUT_STRUCT mn_layout;

	MainMenuProc(msg, &wnd_layout, &mn_layout);

	if (g_activeMenu == MAIN_MENU)
	{
		DisplaySelectMenu(&wnd_layout, &mn_layout, TITLE_CENTERED);
		WriteMapToLcd(g_mmap);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 1 /* Test */
#include "usb.h"
extern void SetupUSBComposite(void);
extern void USBCPortControllerInit(void);
extern void USBCPortControllerSwapToHost(void);
#endif
void MainMenuProc(INPUT_MSG_STRUCT msg, WND_LAYOUT_STRUCT *wnd_layout_ptr, MN_LAYOUT_STRUCT *mn_layout_ptr)
{
	INPUT_MSG_STRUCT mn_msg;
	DATE_TIME_STRUCT currentTime = GetCurrentTime();
	uint32 input;
	uint8 length;

	switch (msg.cmd)
	{
		case (ACTIVATE_MENU_CMD):
			wnd_layout_ptr->start_col = MAIN_WND_STARTING_COL;
			wnd_layout_ptr->end_col = MAIN_WND_END_COL;
			wnd_layout_ptr->start_row = MAIN_WND_STARTING_ROW;
			wnd_layout_ptr->end_row = MAIN_WND_END_ROW;

			mn_layout_ptr->top_ln = 1;
			mn_layout_ptr->sub_ln = 0;

			if (msg.data[0] != ESC_KEY)
			{
				mn_layout_ptr->curr_ln = 3;
			}
			else
			{
				if (mn_layout_ptr->curr_ln > 6)
				mn_layout_ptr->top_ln = (uint16)(mn_layout_ptr->curr_ln - 5);
			}

			LoadTempMenuTable(s_mainMenuTable);
			
			// Add in time (hour:min) to the 2nd LCD line right justified
			length = strlen(getLangText(SELECT_TEXT));
			memset(&(g_menuPtr[1].data[length]), ' ', (12 - length));
			sprintf((char*)&(g_menuPtr[1].data[9]), "%02d:%02d:%02d %s", ((currentTime.hour % 12) == 0) ? 12 : (currentTime.hour % 12),
					currentTime.min, currentTime.sec, ((currentTime.hour / 12) == 1) ? "PM" : "AM");

			sprintf((char*)&(g_menuPtr[2].data[0]), "_____________________");

			// Since time was added, start the menu update timer
			AssignSoftTimer(MENU_UPDATE_TIMER_NUM, ONE_SECOND_TIMEOUT, MenuUpdateTimerCallBack);
			break;

		case (KEYPRESS_MENU_CMD):
			input = msg.data[0];
			// Handle converting a number key input into a selection
			if ((input >= ONE_KEY) && (input <= FOUR_KEY))
			{
				// Convert ASCII data to a hex number to set the current line
				mn_layout_ptr->curr_ln = (uint16)((input - 0x30) + 2);
				input = ENTER_KEY;
			}

			switch (input)
			{
				case (ENTER_KEY):
					switch (mn_layout_ptr->curr_ln)
					{
						case (DEFAULT_ROW_3): // Waveform
							g_triggerRecord.opMode = WAVEFORM_MODE;
							ClearSoftTimer(MENU_UPDATE_TIMER_NUM);
							UpdateModeMenuTitle(g_triggerRecord.opMode);
							SETUP_USER_MENU_MSG(&modeMenu, MONITOR);
							JUMP_TO_ACTIVE_MENU();
							break; 

						case (DEFAULT_ROW_4): // Bargraph
							g_triggerRecord.opMode = BARGRAPH_MODE;
							ClearSoftTimer(MENU_UPDATE_TIMER_NUM);
							UpdateModeMenuTitle(g_triggerRecord.opMode);
							SETUP_USER_MENU_MSG(&modeMenu, MONITOR);
							JUMP_TO_ACTIVE_MENU();
							break;

						case (DEFAULT_ROW_5): // Combo
							g_triggerRecord.opMode = COMBO_MODE;
							ClearSoftTimer(MENU_UPDATE_TIMER_NUM);
							UpdateModeMenuTitle(g_triggerRecord.opMode);
							SETUP_USER_MENU_MSG(&modeMenu, MONITOR);
							JUMP_TO_ACTIVE_MENU();
							break;

						case (DEFAULT_ROW_6): // Load Saved Record
							ClearSoftTimer(MENU_UPDATE_TIMER_NUM);
							SETUP_MENU_MSG(LOAD_REC_MENU);
							JUMP_TO_ACTIVE_MENU();
							break;
#if 1 /* Test sohrtcut to Cal Setup menu */
						case (DEFAULT_ROW_7): // Load Saved Record
							ClearSoftTimer(MENU_UPDATE_TIMER_NUM);
							g_keypadTable[SOFT_KEY_1] = KB_SK_1;
							g_keypadTable[SOFT_KEY_2] = KB_SK_2;
							g_keypadTable[SOFT_KEY_4] = KB_SK_4;
							SETUP_MENU_MSG(CAL_SETUP_MENU);
							JUMP_TO_ACTIVE_MENU();
							break;
#endif
						default:
							break;
					}
					break;

				case (DOWN_ARROW_KEY):
					MainMenuScroll(DOWN, SELECT_MN_WND_LNS, mn_layout_ptr);
					break;
				case (UP_ARROW_KEY):
#if 1 /* Original */
					MainMenuScroll(UP, SELECT_MN_WND_LNS, mn_layout_ptr);
#else /* Test */
					USBCPortControllerSwapToHost();
#endif
					break;
				case (LEFT_ARROW_KEY):
#if 0 /* Original */
					AdjustLcdContrast(DARKER);
#else /* Test */
					MXC_USB_Disconnect();
#endif
					break;
				case (RIGHT_ARROW_KEY):
#if 0 /* Original */
					AdjustLcdContrast(LIGHTER);
#else /* Test */
					MXC_USB_Connect();
#endif
					break;
				case (ESC_KEY):
					// Reset the current line to tbe the top line	
					mn_layout_ptr->curr_ln = 3;
					break;
				case (HELP_KEY):
					ClearSoftTimer(MENU_UPDATE_TIMER_NUM);
					SETUP_USER_MENU_MSG(&helpMenu, CONFIG_CHOICE);
					JUMP_TO_ACTIVE_MENU();
					break;
				default:
					break;
			}
			break;

		default:
			// Menu called without action, most likely the menu update timer
			sprintf((char*)&(g_menuPtr[1].data[9]), "%02d:%02d:%02d %s", ((currentTime.hour % 12) == 0) ? 12 : (currentTime.hour % 12),
					currentTime.min, currentTime.sec, ((currentTime.hour / 12) == 1) ? "PM" : "AM");

			// Since time was added, start the menu update timer
			AssignSoftTimer(MENU_UPDATE_TIMER_NUM, ONE_SECOND_TIMEOUT, MenuUpdateTimerCallBack);
			break;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MainMenuScroll(char direction, char wnd_size, MN_LAYOUT_STRUCT * mn_layout_ptr)
{
	uint8 buff[50];

	strcpy((char*)buff, (char*)(g_menuPtr + mn_layout_ptr->curr_ln + 1)->data);

	switch (direction)
	{
		case (DOWN):
			if (strcmp((char*)buff, ".end."))
			{
				mn_layout_ptr->curr_ln++;

				if ((mn_layout_ptr->curr_ln - mn_layout_ptr->top_ln) >= wnd_size)
				{
					mn_layout_ptr->top_ln++;
				}
			}
			break;

		case (UP):
			if (mn_layout_ptr->curr_ln > 3)
			{
				if (mn_layout_ptr->curr_ln == (mn_layout_ptr->top_ln + 2))
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
uint8 CheckAndDisplayErrorThatPreventsMonitoring(uint8 messageType)
{
	uint8 errorCondition = NO;

	if (g_factorySetupRecord.invalid)
	{
		errorCondition = YES;
		debugWarn("Factory setup record not found.\r\n");
		
		if (g_lcdPowerFlag == ENABLED)
		{
			if (messageType == PROMPT) { MessageBox(getLangText(ERROR_TEXT), getLangText(FACTORY_SETUP_DATA_COULD_NOT_BE_FOUND_TEXT), MB_OK); }
			else { OverlayMessage(getLangText(ERROR_TEXT), getLangText(FACTORY_SETUP_DATA_COULD_NOT_BE_FOUND_TEXT), (3 * SOFT_SECS)); }
		}
	}
	else if (g_lowBatteryState == YES)
	{
		errorCondition = YES;
		debugWarn("Monitoring unavailable due to low battery voltage\r\n");

		sprintf((char*)g_spareBuffer, "%s %s (%3.2f)", getLangText(BATTERY_VOLTAGE_TEXT), getLangText(LOW_TEXT), (double)(GetExternalVoltageLevelAveraged(BATTERY_VOLTAGE)));

		if (g_lcdPowerFlag == ENABLED)
		{
			if (messageType == PROMPT) { MessageBox(getLangText(WARNING_TEXT), (char*)g_spareBuffer, MB_OK); }
			else { OverlayMessage(getLangText(WARNING_TEXT), (char*)g_spareBuffer, (3 * SOFT_SECS)); }
		}
	}
	else if ((g_triggerRecord.opMode != BARGRAPH_MODE) && (CheckTriggerSourceExists() == NO))
	{
		errorCondition = YES;
		debugWarn("Monitoring unavailable due to no valid trigger source\r\n");

		sprintf((char*)g_spareBuffer, "%s. %s", getLangText(NO_TRIGGER_SOURCE_SELECTED_TEXT), getLangText(PLEASE_CHANGE_TEXT));

		if (g_lcdPowerFlag == ENABLED)
		{
			if (messageType == PROMPT)
			{
				MessageBox(getLangText(WARNING_TEXT), getLangText(BOTH_TRIGGERS_SET_TO_NO_AND_EXTERNAL_TRIGGER_DISABLED_TEXT), MB_OK);
				MessageBox(getLangText(WARNING_TEXT), (char*)g_spareBuffer, MB_OK);
			}
			else
			{
				OverlayMessage(getLangText(WARNING_TEXT), getLangText(BOTH_TRIGGERS_SET_TO_NO_AND_EXTERNAL_TRIGGER_DISABLED_TEXT), (3 * SOFT_SECS));
				OverlayMessage(getLangText(WARNING_TEXT), (char*)g_spareBuffer, (3 * SOFT_SECS));
			}
		}
	}
	
	return (errorCondition);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* Static delay removed */
void PromptUserWaitingForSensorWarmup(void)
{
	debugWarn("Monitoring temporarily unavailable due to sensors warming up\r\n");

	sprintf((char*)g_spareBuffer, "%s ", getLangText(ZEROING_SENSORS_TEXT));
	OverlayMessage(getLangText(STATUS_TEXT), (char*)g_spareBuffer, (3 * SOFT_SECS));

	while ((volatile uint32)g_lifetimeHalfSecondTickCount < 120)
	{
		strcat((char*)g_spareBuffer, ".");
		OverlayMessage(getLangText(STATUS_TEXT), (char*)g_spareBuffer, (3 * SOFT_SECS));
	}
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void CheckAndPromptUserWaitingForSensorWarmup(void)
{
	//=========================================================================
	// Sensor Warm Up Delay
	//-------------------------------------------------------------------------
	if (g_lifetimeHalfSecondTickCount < (SENSOR_WARMUP_DELAY_IN_SECONDS * 2))
	{
		debug("Monitor pending sensor warmup\r\n");

		while ((volatile uint32)g_lifetimeHalfSecondTickCount < (SENSOR_WARMUP_DELAY_IN_SECONDS * 2))
		{
#if 1 /* Test breaking out of loop until hardware is stable enough to find a zero level */
			if (GetKeypadKey(CHECK_ONCE_FOR_KEY) == ON_ESC_KEY) { break; }
#endif
			sprintf((char*)g_spareBuffer, "%s (%lu sec)", getLangText(SENSOR_WARMING_UP_FROM_COLD_START_TEXT), (((SENSOR_WARMUP_DELAY_IN_SECONDS * 2) - g_lifetimeHalfSecondTickCount) / 2));
			OverlayMessage(getLangText(STATUS_TEXT), (char*)g_spareBuffer, (1 * SOFT_SECS));
		}

		ResetSoftTimer(LCD_BACKLIGHT_ON_OFF_TIMER_NUM);
		ResetSoftTimer(LCD_POWER_ON_OFF_TIMER_NUM);
	}
}
