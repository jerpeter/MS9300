///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include "Keypad.h"
#include "Menu.h"
#include "Record.h"
#include "Common.h"
#include "Typedefs.h"
#include "SoftTimer.h"
#include "Display.h"
#include "PowerManagement.h"
#include "OldUart.h"
#include "SysEvents.h"
#include "TextTypes.h"
#include "stdio.h"
//#include "usb_task.h"
//#include "device_mass_storage_task.h"
//#include "host_mass_storage_task.h"
#include "string.h"

#include "lcd.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define CHECK_FOR_REPEAT_KEY_DELAY 	750		// 750 ms
#define CHECK_FOR_COMBO_KEY_DELAY 	20		// 20 ms
#define WAIT_AFTER_COMBO_KEY_DELAY 	250		// 250 ms
#define REPEAT_DELAY 				100		// 100 ms
#define KEY_DONE_DEBOUNCE_DELAY		25		// 25 ms

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
#include "Globals.h"

///----------------------------------------------------------------------------
///	Local Scope Globals
///----------------------------------------------------------------------------
static uint32 s_fixedSpecialSpeed;

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
BOOLEAN KeypadProcessing(uint8 keySource)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {0}};
	uint16 rowMask = 0;
	uint16 keyPressed = KEY_NONE;
	uint16 keyMapRead;
	uint8 numKeysPressed = 0;
	uint8 i = 0;

	// Prevents the interrupt routine from setting the system keypress flag.
	g_kpadProcessingFlag = ACTIVATED;
	// Set flag to run keypad again to check for repeating keys or ctrl/shift key combos
	g_kpadCheckForKeyFlag = ACTIVATED;

	// Check if key source was an interrupt and the timer hasn't been started (new key interrupt)
	if ((keySource == KEY_SOURCE_IRQ) && (g_tcTypematicTimerActive == NO))
	{
		// Read the buttons/keys
#if 0 /* Original */
		keyMapRead = READ_KEY_BUTTON_MAP;
#else /* Pull key map from ISR reading */
		keyMapRead = g_kpadIsrKeymap;
#endif
#if 0 /* Multiple reads to try to pick up the key */
		SoftUsecWait(1 * SOFT_MSECS);
		keyMapRead |= READ_KEY_BUTTON_MAP;
		SoftUsecWait(1 * SOFT_MSECS);
		keyMapRead |= READ_KEY_BUTTON_MAP;
#endif
	}
	else if (keySource == KEY_SOURCE_TIMER) // Cyclic check
	{
		// Read once (full key pressed shouldn't lose contact)
		keyMapRead = READ_KEY_BUTTON_MAP;
	}
	else //((keySource == KEY_SOURCE_IRQ) && (g_tcTypematicTimerActive == YES)) // Either multi-key or key release processing
	{
		// Read the key that is being released (couple reads to filter any bouncing contact)
#if 0 /* Original */
		keyMapRead = READ_KEY_BUTTON_MAP;
#else /* Pull key map from ISR reading */
		keyMapRead = g_kpadIsrKeymap;
#endif
#if 0 /* Multiple reads to filter any bouncing contact */
		SoftUsecWait(3 * SOFT_MSECS);
		keyMapRead |= READ_KEY_BUTTON_MAP;
		SoftUsecWait(3 * SOFT_MSECS);
		keyMapRead |= READ_KEY_BUTTON_MAP;
#endif
		// Check if the key to be released is still present and that this key is the only one present
		if ((keyMapRead & g_kpadLastKeymap) && ((keyMapRead & (keyMapRead - 1)) == 0))
		{
			keyMapRead = READ_KEY_BUTTON_MAP;
#if 0 /* Multiple reads to filter any bouncing contact */
			SoftUsecWait(5 * SOFT_MSECS);
			keyMapRead &= READ_KEY_BUTTON_MAP;
			SoftUsecWait(5 * SOFT_MSECS);
			keyMapRead &= READ_KEY_BUTTON_MAP;
			SoftUsecWait(5 * SOFT_MSECS);
			keyMapRead &= READ_KEY_BUTTON_MAP;
#endif
		}
	}

	if (keyMapRead != g_kpadIsrKeymap) { debugWarn("Keypad processing missed key (0x%x, 0x%x)\r\n", keyMapRead, g_kpadIsrKeymap); }

#if 0 /* Original */
	if (keyMapRead) { debugRaw(" (Key Pressed: %x)", keyMapRead); }
	else { debugRaw(" (Key Release)"); }
#else
	char keyName[50]; memset(keyName, 0, sizeof(keyName));
	if (keyMapRead & 0x0001) { strcat(keyName, "(SK 4)"); }
	if (keyMapRead & 0x0002) { strcat(keyName, "(SK 3)"); }
	if (keyMapRead & 0x0004) { strcat(keyName, "(SK 2)"); }
	if (keyMapRead & 0x0008) { strcat(keyName, "(SK 1)"); }
	if (keyMapRead & 0x0010) { strcat(keyName, "(Enter)"); }
	if (keyMapRead & 0x0020) { strcat(keyName, "(Right)"); }
	if (keyMapRead & 0x0040) { strcat(keyName, "(Left)"); }
	if (keyMapRead & 0x0080) { strcat(keyName, "(Down)"); }
	if (keyMapRead & 0x0100) { strcat(keyName, "(Up)"); }
	if (keyMapRead) { debugRaw(" Key Pressed: %s", keyName); }
	else { debugRaw(" (Key Release)"); }
#endif

#if 1 /* Test special method for redirect */
	if (g_kpadIsrKeymap & 0x8000) { g_kpadIsrKeymap = 0; }
#endif

	//---------------------------------------------------------------------------------
	// Find key
	//---------------------------------------------------------------------------------
	// Find keys by locating the 1's in the map
	for (rowMask = 0x01, i = 0; i < TOTAL_KEYPAD_KEYS; i++)
	{
		if (keyMapRead & rowMask)
		{
			//debug("Key Found: Row:%d, value is 0x%x\r\n", i, g_keypadTable[i]);

			keyPressed = g_keypadTable[i];
			numKeysPressed++;
		}

		rowMask <<= 1;
	}

	//---------------------------------------------------------------------------------
	// Check if no key was discovered
	//---------------------------------------------------------------------------------
	if (numKeysPressed == 0)
	{
		SoftUsecWait(KEY_DONE_DEBOUNCE_DELAY * SOFT_MSECS);

		while (g_kpadInterruptWhileProcessing == YES)
		{
			g_kpadInterruptWhileProcessing = NO;
			// Clear interrupt flags
			MXC_GPIO_ClearFlags(REGULAR_BUTTONS_GPIO_PORT, REGULAR_BUTTONS_GPIO_MASK);
		}

		// Done looking for keys
		g_kpadProcessingFlag = DEACTIVATED;
		g_kpadCheckForKeyFlag = DEACTIVATED;

		// Check if key interrupt but timer not started and last key cleared meaning a fresh interrupt with no key found (likely heavy processing and missed the key)
		if ((keySource == KEY_SOURCE_IRQ) && (g_tcTypematicTimerActive == NO) && (g_kpadLastKeyPressed == KEY_NONE))
		{
			// Send a dummy message which will wake the LCD if it's off
			SendInputMsg(&mn_msg);
		}

		// Disable the key timer
		StopInteralPITTimer(TYPEMATIC_TIMER);

		// Clear last key pressed
		g_kpadLastKeyPressed = KEY_NONE;

		// No keys detected, done processing
		return(PASSED);
	}

	// Key detected, begin higher level processing
	// Check if the key timer hasn't been activated already
	if (g_tcTypematicTimerActive == NO)
	{
		// Start the key timer
		StartInteralPITTimer(TYPEMATIC_TIMER);
	}

	//---------------------------------------------------------------------------------
	// Process repeating key
	//---------------------------------------------------------------------------------
	// Check if the same key is still being pressed
	if ((keyPressed != KEY_NONE) && (g_kpadLastKeyPressed == keyPressed))
	{
		// Process repeating key

		// Set delay before looking for key again
		g_kpadDelayTickCount = REPEAT_DELAY + g_keypadTimerTicks;

		// The following determines the scrolling adjustment magnitude
		// which is determined by multiplying adjustment by g_keypadNumberSpeed.
		g_kpadKeyRepeatCount++;

		if (g_kpadKeyRepeatCount < 10)		{ g_keypadNumberSpeed = 1; }
		else if (g_kpadKeyRepeatCount < 20) { g_keypadNumberSpeed = 10; }
		else if (g_kpadKeyRepeatCount < 30) { g_keypadNumberSpeed = 50; }
		else if (g_kpadKeyRepeatCount < 40) { g_keypadNumberSpeed = 100; }
		else if (g_kpadKeyRepeatCount < 50) { g_keypadNumberSpeed = 1000; }
		else
		{
			s_fixedSpecialSpeed = 10000;
		}

		mn_msg.length = 1;
		mn_msg.cmd = KEYPRESS_MENU_CMD;
		mn_msg.data[0] = keyPressed;

		if (g_factorySetupSequence != PROCESS_FACTORY_SETUP)
		{
			// Reset the sequence
			g_factorySetupSequence = SEQ_NOT_STARTED;
		}

		// Enqueue the message
		SendInputMsg(&mn_msg);

		// Done processing repeating key
	}
	//---------------------------------------------------------------------------------
	// Process new key
	//---------------------------------------------------------------------------------
	else // New key pressed
	{
		// Store current key and keymap for later comparison
		g_kpadLastKeyPressed = keyPressed;
		g_kpadLastKeymap = keyMapRead;

		// Reset variables
		g_keypadNumberSpeed = 1;
		g_kpadKeyRepeatCount = 0;
		s_fixedSpecialSpeed = 0;

		// Set delay for some time before considering key repeating
		g_kpadDelayTickCount = CHECK_FOR_REPEAT_KEY_DELAY + g_keypadTimerTicks;

		//---------------------------------------------------------------------------------
		// Send new key message for action
		//---------------------------------------------------------------------------------
		// Process new key
		if (keyPressed != KEY_NONE)
		{
#if 0 /* Fill in */
			// Todo: Process soft key translation?
			// Turn soft keys 1..4 into their mapped action?
			if (keyPressed == KB_SK_1) { keyPressed = g_softKeyTranslation[0]; }
			if (keyPressed == KB_SK_2) { keyPressed = g_softKeyTranslation[1]; }
			if (keyPressed == KB_SK_3) { keyPressed = g_softKeyTranslation[2]; }
			if (keyPressed == KB_SK_4) { keyPressed = g_softKeyTranslation[3]; }
#else /* Temporary hard translation */
			// Setup in the header defines
#endif
			if (keyPressed == BACKLIGHT_KEY)
			{
				mn_msg.cmd = BACK_LIGHT_CMD;
				mn_msg.length = 0;
			}
			else if (keyPressed == LCD_OFF_KEY)
			{
				// Check if the LCD is currently powered
				if (g_lcdPowerFlag == ENABLED)
				{
					ClearSoftTimer(LCD_BACKLIGHT_ON_OFF_TIMER_NUM);
					ClearSoftTimer(LCD_POWER_ON_OFF_TIMER_NUM);
					LcdPwTimerCallBack();
				}
				else // Allow LCD Off key to wake the unit when the LCD is off
				{
					keyPressed = ESC_KEY;
				}
			}
			else // All other keys
			{
				mn_msg.length = 1;
				mn_msg.data[0] = keyPressed;
				mn_msg.cmd = KEYPRESS_MENU_CMD;
			}

			//---------------------------------------------------------------------------------
			// Factory setup staging
			//---------------------------------------------------------------------------------
			// Handle factory setup special sequence
			if ((g_factorySetupSequence == STAGE_1) && (keyPressed == UP_ARROW_KEY))
			{
				g_factorySetupSequence = STAGE_2;
			}
			else if ((g_factorySetupSequence == STAGE_2) && (keyPressed == DOWN_ARROW_KEY))
			{
#if 0 /* Original */
				// Check if actively in Monitor mode
				if (g_sampleProcessing == ACTIVE_STATE)
				{
					// Don't allow access to the factory setup
					g_factorySetupSequence = SEQ_NOT_STARTED;
				}
				else // Not in Monitor mode
				{
					// Allow access to factory setup
					g_factorySetupSequence = ENTER_FACTORY_SETUP;
				}
#else /* New */
				// Check if actively in Monitor mode
				if (g_sampleProcessing == ACTIVE_STATE)
				{
					// Don't allow access to the factory setup
					g_factorySetupSequence = SEQ_NOT_STARTED;
				}
				else // Not in Monitor mode
				{
					if (MessageBox(getLangText(STATUS_TEXT), "FACTORY SETUP LOCKED", MB_OK) == MB_SPECIAL_ACTION)
					{
						// Allow access to factory setup
						g_factorySetupSequence = ENTER_FACTORY_SETUP;
					}
					else
					{
						// Don't allow access to the factory setup
						g_factorySetupSequence = SEQ_NOT_STARTED;

						// Clear out the message parameters
						mn_msg.cmd = 0; mn_msg.length = 0; mn_msg.data[0] = 0;

						// Recall the current active menu
						JUMP_TO_ACTIVE_MENU();
					}
				}
#endif
			}
			else
			{
				// Check if the On key is being pressed
				if (GetPowerOnButtonState() == ON)
				{
					// Reset the factory setup process
					g_factorySetupSequence = SEQ_NOT_STARTED;

					//===================================================
					// On-Enter Combo key
					//---------------------------------------------------
					if (keyPressed == ENTER_KEY)
					{
						// Issue a Ctrl-C for Manual Calibration
						mn_msg.cmd = CTRL_CMD;
						mn_msg.data[0] = 'C';
					}
					//===================================================
					// On-Escape Combo key
					//---------------------------------------------------
					else if (keyPressed == ESC_KEY)
					{
#if 0 /* Test */
						if (g_sampleProcessing == IDLE_STATE)
						{
							g_quickBootEntryJump = QUICK_BOOT_ENTRY_FROM_MENU;
							BootLoadManager();
						}
#endif
#if 0 /* Test */
						debug("Flagging Auto Dialout start\r\n");
						raiseSystemEventFlag(AUTO_DIALOUT_EVENT);

						if (g_lcdPowerFlag == ENABLED)
						{
							ClearSoftTimer(LCD_BACKLIGHT_ON_OFF_TIMER_NUM);
							ClearSoftTimer(LCD_POWER_ON_OFF_TIMER_NUM);
							LcdPwTimerCallBack();
						}
#endif
#if 0 /* Test */
						g_tcpServerStartStage = 1;
						AssignSoftTimer(TCP_SERVER_START_NUM, (1 * TICKS_PER_SEC), TcpServerStartCallback);
#endif
					}
					//===================================================
					// On-Help Combo key
					//---------------------------------------------------
					else if (keyPressed == HELP_KEY)
					{
#if 0 /* Test */
						g_breakpointCause = BP_END;

						__asm__ __volatile__ ("breakpoint");
#endif
#if 0 /* Test */
static uint8_t s_bcChargeState = ON;
						s_bcChargeState ^= ON;
						SetBattChargerChargeState(s_bcChargeState);
						debug("Battery charging toggle: %s\r\n", ((s_bcChargeState == ON) ? "Enabled" : "Disabled"));
						// Clear out the message parameters
						mn_msg.cmd = 0; mn_msg.length = 0; mn_msg.data[0] = 0;
#endif
#if 1 /* Test */
						if (g_sampleProcessing == ACTIVE_STATE)
						{
							// Establish trigger signal
							g_externalTrigger = EXTERNAL_TRIGGER_EVENT;
						}
#endif
					}
					//===================================================
					// On-Help Combo key
					//---------------------------------------------------
					else if (keyPressed == BACKLIGHT_KEY)
					{
#if 0 /* Test */
						if (IsSoftTimerActive(AUTO_EVENT_GENERATION_NUM)) { ClearSoftTimer(AUTO_EVENT_GENERATION_NUM); debug("Battery charging toggle timer: Disabled\r\n"); }
						else { AssignSoftTimer(AUTO_EVENT_GENERATION_NUM, (20 * TICKS_PER_SEC), AutoEventGenerationCallback); debug("Battery charging toggle timer: Enabled (20 second cycle)\r\n"); }
						// Clear out the message parameters
						mn_msg.cmd = 0; mn_msg.length = 0; mn_msg.data[0] = 0;
#endif
					}
				}
				else if (g_factorySetupSequence != PROCESS_FACTORY_SETUP)
				{
					// Reset the sequence
					g_factorySetupSequence = SEQ_NOT_STARTED;
				}

#if 0 /* Orignal */
				// Enqueue the message
				SendInputMsg(&mn_msg);
#else
				if (keyPressed != LCD_OFF_KEY)
				{
					// Enqueue the message
					SendInputMsg(&mn_msg);
				}
#endif
			}
		}
	}

	while (g_kpadInterruptWhileProcessing == YES)
	{
		g_kpadInterruptWhileProcessing = NO;
		// Clear interrupt flags
		MXC_GPIO_ClearFlags(REGULAR_BUTTONS_GPIO_PORT, REGULAR_BUTTONS_GPIO_MASK);
	}

	g_kpadProcessingFlag = DEACTIVATED;

	return(PASSED);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void KeypressEventMgr(void)
{
#if LCD_RESOURCE_UNAVAILABLE
	return;
#endif

	// Check if the LCD Power was turned off
	if (g_lcdPowerFlag == DISABLED)
	{
		g_lcdPowerFlag = ENABLED;
		raiseSystemEventFlag(UPDATE_MENU_EVENT);
		ft81x_init(); // Power up and init display
		AssignSoftTimer(LCD_POWER_ON_OFF_TIMER_NUM, (uint32)(g_unitConfig.lcdTimeout * TICKS_PER_MIN), LcdPwTimerCallBack);

		// Check if the unit is monitoring, if so, reassign the monitor update timer
		if (g_sampleProcessing == ACTIVE_STATE)
		{
			debug("Keypress Timer Mgr: enabling Monitor Update Timer.\r\n");
			AssignSoftTimer(MENU_UPDATE_TIMER_NUM, ONE_SECOND_TIMEOUT, MenuUpdateTimerCallBack);
		}
	}
	else // Reassign the LCD Power countdown timer
	{
		AssignSoftTimer(LCD_POWER_ON_OFF_TIMER_NUM, (uint32)(g_unitConfig.lcdTimeout * TICKS_PER_MIN), LcdPwTimerCallBack);
	}

	// Check if the LCD Backlight was turned off (now treating BACKLIGHT_SUPER_LOW as disabled since the LCD really can't be seen with the backlight on at some level)
	if (g_lcdBacklightFlag == DISABLED)
	{
		g_lcdBacklightFlag = ENABLED;
		SetLcdBacklightState(BACKLIGHT_MID);
		AssignSoftTimer(LCD_BACKLIGHT_ON_OFF_TIMER_NUM, LCD_BACKLIGHT_TIMEOUT, DisplayTimerCallBack);
	}
	else // Reassign the LCD Backlight countdown timer
	{
		AssignSoftTimer(LCD_BACKLIGHT_ON_OFF_TIMER_NUM, LCD_BACKLIGHT_TIMEOUT, DisplayTimerCallBack);
	}

	// Check if Auto Monitor is active and not in monitor mode
	if ((g_unitConfig.autoMonitorMode != AUTO_NO_TIMEOUT) && (g_sampleProcessing != ACTIVE_STATE))
	{
		AssignSoftTimer(AUTO_MONITOR_TIMER_NUM, (uint32)(g_unitConfig.autoMonitorMode * TICKS_PER_MIN), AutoMonitorTimerCallBack);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 GetShiftChar(uint8 inputChar)
{
	switch (inputChar)
	{
		case '1': return ('?'); break;
		case '2': return ('@'); break;
		case '3': return ('#'); break;
		case '4': return ('/'); break;
		case '5': return ('%'); break;
		case '6': return ('^'); break;
		case '7': return ('&'); break;
		case '8': return ('*'); break;
		case '9': return ('('); break;
		case '0': return (')'); break;
		case 'Q': return (':'); break;
		case 'W': return (','); break;
		case 'E': return ('+'); break;
		case 'I': return ('<'); break;
		case 'O': return ('>'); break;
		case 'P': return ('-'); break;
		default:
			break;
	}

	return (inputChar);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16 HandleCtrlKeyCombination(uint16 inputChar)
{
	switch (inputChar)
	{
		case ESC_KEY:
			break;

		case 'C':
			HandleManualCalibration();
			break;

		default:
			break;
	}

	return (KEY_NONE);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 1 /* Test */
#include "RemoteHandler.h"
#endif
uint16 GetKeypadKey(uint8 mode)
{
	//uint8 columnSelection = 0;
	//uint8 columnIndex = 0;
	uint16 keyPressed = KEY_NONE;
	//uint8 lookForKey = 1;
	//uint8 foundKeyDepressed = 0;
	//uint8 data = 0;

	g_kpadProcessingFlag = ACTIVATED;

	if (mode == WAIT_FOR_KEY)
	{
		keyPressed = ScanKeypad();
		// If there is a key, wait until it's depressed
		while (keyPressed != KEY_NONE)
		{
#if 0 /* Original (just a delay) */
			SoftUsecWait(1000);
#else /* Process USB as a delay */
			// Process USB core routines (do not call UsbDeviceManager since it's not designed to be re-entrant)
			ProcessUsbCoreHandling();
#if 1 /* Test special method to redirect from serial */
			ProcessCraftData(); if (getSystemEventState(CRAFT_PORT_EVENT)) { clearSystemEventFlag(CRAFT_PORT_EVENT); RemoteCmdMessageProcessing(); }
#endif
#endif
			keyPressed = ScanKeypad();
		}

		// Wait for a key to be pressed
		keyPressed = ScanKeypad();
		while (keyPressed == KEY_NONE)
		{
#if 0 /* Original (just a delay) */
			SoftUsecWait(1000);
#else /* Process USB as a delay */
			// Process USB core routines (do not call UsbDeviceManager since it's not designed to be re-entrant)
			ProcessUsbCoreHandling();
#if 1 /* Test special method to redirect from serial */
			ProcessCraftData(); if (getSystemEventState(CRAFT_PORT_EVENT)) { clearSystemEventFlag(CRAFT_PORT_EVENT); RemoteCmdMessageProcessing(); }
#endif
#endif
			keyPressed = ScanKeypad();
		}

#if 1 /* New ability to process On-Esc as a special ability */
		if (keyPressed == ESC_KEY)
		{
			// Check if the On key is also being pressed
			if (GetPowerOnButtonState() == ON)
			{
				keyPressed = ON_ESC_KEY;
			}
		}
#endif

		// Wait for a key to be released
		while (ScanKeypad() != KEY_NONE)
		{
#if 0 /* Original (just a delay) */
			SoftUsecWait(1000);
#else /* Process USB as a delay */
			// Process USB core routines (do not call UsbDeviceManager since it's not designed to be re-entrant)
			ProcessUsbCoreHandling();
#if 1 /* Test special method to redirect from serial */
			ProcessCraftData(); if (getSystemEventState(CRAFT_PORT_EVENT)) { clearSystemEventFlag(CRAFT_PORT_EVENT); RemoteCmdMessageProcessing(); }
#endif
#endif
		}
	}
	else // mode = CHECK_ONCE_FOR_KEY
	{
		// Check once if there is a key depressed
#if 1 /* Test special method to redirect from serial */
		ProcessCraftData(); if (getSystemEventState(CRAFT_PORT_EVENT)) { clearSystemEventFlag(CRAFT_PORT_EVENT); RemoteCmdMessageProcessing(); }
#endif
		keyPressed = ScanKeypad();

		if (keyPressed == KEY_NONE)
		{
			SoftUsecWait(1000);

			while (g_kpadInterruptWhileProcessing == YES)
			{
				g_kpadInterruptWhileProcessing = NO;
				// Clear interrupt flags
				MXC_GPIO_ClearFlags(REGULAR_BUTTONS_GPIO_PORT, REGULAR_BUTTONS_GPIO_MASK);
			}

			g_kpadProcessingFlag = DEACTIVATED;
			clearSystemEventFlag(KEYPAD_EVENT);

			return (KEY_NONE);
		}

#if 1 /* New ability to process On-Esc as a special ability */
		if (keyPressed == ESC_KEY)
		{
			// Check if the On key is also being pressed
			if (GetPowerOnButtonState() == ON)
			{
				keyPressed = ON_ESC_KEY;
			}
		}
#endif
	}

	SoftUsecWait(1000);

	// Reassign the LCD Power countdown timer
	AssignSoftTimer(LCD_POWER_ON_OFF_TIMER_NUM, (uint32)(g_unitConfig.lcdTimeout * TICKS_PER_MIN), LcdPwTimerCallBack);

	// Reassign the LCD Backlight countdown timer
	AssignSoftTimer(LCD_BACKLIGHT_ON_OFF_TIMER_NUM, LCD_BACKLIGHT_TIMEOUT, DisplayTimerCallBack);

	while (g_kpadInterruptWhileProcessing == YES)
	{
		g_kpadInterruptWhileProcessing = NO;
		// Clear interrupt flags
		MXC_GPIO_ClearFlags(REGULAR_BUTTONS_GPIO_PORT, REGULAR_BUTTONS_GPIO_MASK);
	}

	g_kpadProcessingFlag = DEACTIVATED;

	// Prevent a bouncing key from causing any action after this
	clearSystemEventFlag(KEYPAD_EVENT);

	return (keyPressed);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16 ScanKeypad(void)
{
	uint16 rowMask = 0;
	uint16 keyPressed = KEY_NONE;
	uint16 keyMapRead;
	uint8 i = 0;

#if 1 /* Test special method for redirect */
	if (g_kpadIsrKeymap & 0x8000)
	{
		keyMapRead = g_kpadIsrKeymap;
		g_kpadIsrKeymap = 0;

		if (keyMapRead & 0x4000)
		{
			return (ON_ESC_KEY);
		}
	}
	else
#endif
	keyMapRead = READ_KEY_BUTTON_MAP;
#if 0 /* extra reads for debounce? */
	SoftUsecWait(1 * SOFT_MSECS);
	keyMapRead &= READ_KEY_BUTTON_MAP;
	SoftUsecWait(1 * SOFT_MSECS);
	keyMapRead &= READ_KEY_BUTTON_MAP;
#endif

	//debug("Scan Keypad: Key: %x\r\n", keyMapRead);

	// Find keys by locating the 1's in the map
	for (rowMask = 0x01, i = 0; i < TOTAL_KEYPAD_KEYS; i++)
	{
		if (keyMapRead & rowMask)
		{
			keyPressed = g_keypadTable[i];
		}

		rowMask <<= 1;
	}

	return (keyPressed);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetDynamicSoftKeyLayout(uint16 softKey1, uint16 softKey2, uint16 softKey3, uint16 softKey4)
{
	g_dynamicSoftKeyLayout[0] = softKey1;
	g_dynamicSoftKeyLayout[1] = softKey2;
	g_dynamicSoftKeyLayout[2] = softKey3;
	g_dynamicSoftKeyLayout[3] = softKey4;
}
