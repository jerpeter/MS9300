///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include "board.h"
#include "gpio.h"
#include "wdt.h"
//#include "pm.h"
//#include "sdramc.h"
//#include "intc.h"
//#include "usart.h"
//#include "print_funcs.h"
#include "lcd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Typedefs.h"
#include "Common.h"
#include "Display.h"
#include "Menu.h"
#include "OldUart.h"
#include "spi.h"
#include "ProcessBargraph.h"
#include "SysEvents.h"
#include "Record.h"
#include "InitDataBuffers.h"
#include "PowerManagement.h"
#include "SoftTimer.h"
#include "Keypad.h"
#include "RealTimeClock.h"
#include "RemoteHandler.h"
#include "TextTypes.h"
#include "RemoteCommon.h"
//#include "twi.h"
//#include "sd_mmc_spi.h"
#include "adc.h"
//#include "usb_task.h"
//#include "device_mass_storage_task.h"
//#include "host_mass_storage_task.h"
//#include "usb_drv.h"
//#include "flashc.h"
#include "ushell_task.h"
#include "rtc.h"
//#include "tc.h"
//#include "fsaccess.h"
//#include "cycle_counter.h"

#include "max32650.h"
#include "gcr_regs.h"
#include "mxc_sys.h"
#include "pwrseq_regs.h"
#include "lp.h"
#include "uart.h"
#include "mxc_sys.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
#include "Globals.h"

///----------------------------------------------------------------------------
///	Local Scope Globals
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SystemEventManager(void)
{
	//___________________________________________________________________________________________
	if (getSystemEventState(TRIGGER_EVENT))
	{
		if ((g_triggerRecord.opMode == WAVEFORM_MODE) || (g_triggerRecord.opMode == COMBO_MODE))
		{
			debug("Trigger Event (%s)\r\n", (g_triggerRecord.opMode == WAVEFORM_MODE) ? "Wave" : "Combo");
#if 1 /* Normal */
			MoveWaveformEventToFile();
#else /* Test trigger without creating event */
			// Make sure to clear trigger event
			clearSystemEventState(TRIGGER_EVENT);
#endif
		}
	}

	//___________________________________________________________________________________________
	if (getSystemEventState(BARGRAPH_EVENT))
	{
		clearSystemEventFlag(BARGRAPH_EVENT);

		if ((g_triggerRecord.opMode == BARGRAPH_MODE) || (g_triggerRecord.opMode == COMBO_MODE))
		{
			CalculateBargraphData();
			HandleBargraphLiveMonitoringDataTransfer();
		}
	}

	//___________________________________________________________________________________________
	if (getSystemEventState(MANUAL_CAL_EVENT))
	{
		debug("Manual Cal Pulse Event\r\n");
		MoveManualCalToFile();
	}

	//___________________________________________________________________________________________
	if ((getSystemEventState(KEYPAD_EVENT)) || (g_kpadCheckForKeyFlag && (g_kpadDelayTickCount < g_keypadTimerTicks)))
	{
		if (getSystemEventState(KEYPAD_EVENT))
		{
			debug("Keypad Event\r\n");
			clearSystemEventFlag(KEYPAD_EVENT);

			KeypadProcessing(KEY_SOURCE_IRQ);
		}
		else
		{
			KeypadProcessing(KEY_SOURCE_TIMER);
		}
	}

	//___________________________________________________________________________________________
#if 0 /* Unused */
	if (getSystemEventState())
	{
		debug("Power Off Event\r\n");
		clearSystemEventFlag();

		HandleUserPowerOffDuringTimerMode();
	}
#endif

	//___________________________________________________________________________________________
	if (getSystemEventState(LOW_BATTERY_WARNING_EVENT))
	{
		clearSystemEventFlag(LOW_BATTERY_WARNING_EVENT);
		debugWarn("Low Battery Event\r\n");

		// Check if actively monitoring
		if (g_sampleProcessing == ACTIVE_STATE)
		{
			// Stop monitoring
			StopMonitoringForLowPowerState();
		}

		sprintf((char*)g_spareBuffer, "%s %s (%3.2f) %s", getLangText(BATTERY_VOLTAGE_TEXT), getLangText(LOW_TEXT),
				(double)(GetExternalVoltageLevelAveraged(BATTERY_VOLTAGE)), getLangText(PLEASE_CHARGE_BATTERY_TEXT));
		OverlayMessage(getLangText(WARNING_TEXT), (char*)g_spareBuffer, (1 * SOFT_SECS));

		g_lowBatteryState = YES;
	}

uint32_t USBCPortControllerReadAndClearInt(void);
	USBCPortControllerReadAndClearInt();

	//___________________________________________________________________________________________
	if (getSystemEventState(CYCLIC_EVENT))
	{
		clearSystemEventFlag(CYCLIC_EVENT);

#if 0 /* Test Expansion RS232 loopback */
		static uint32_t testCyclicCycles = 0;
		testCyclicCycles++;
		uint8_t messageLength = 0;
		if (testCyclicCycles == 5)
		{
#if 1 /* Test */
			messageLength = sprintf((char*)g_spareBuffer, "unl0000\r\n"); ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
#else
			uint8_t messageLength = sprintf((char*)g_spareBuffer, "u"); ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION); SoftUsecWait(18);
			messageLength = sprintf((char*)g_spareBuffer, "n"); ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION); SoftUsecWait(18);
			messageLength = sprintf((char*)g_spareBuffer, "l"); ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION); SoftUsecWait(18);
			messageLength = sprintf((char*)g_spareBuffer, "0"); ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION); SoftUsecWait(18);
			messageLength = sprintf((char*)g_spareBuffer, "0"); ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION); SoftUsecWait(18);
			messageLength = sprintf((char*)g_spareBuffer, "0"); ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION); SoftUsecWait(18);
			messageLength = sprintf((char*)g_spareBuffer, "0"); ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION); SoftUsecWait(18);
			messageLength = sprintf((char*)g_spareBuffer, "\r"); ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION); SoftUsecWait(18);
			messageLength = sprintf((char*)g_spareBuffer, "\n"); ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION); SoftUsecWait(18);
#endif
		}
		//else if (testCyclicCycles == 10) { messageLength = sprintf((char*)g_spareBuffer, "dqm\r\n"); ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION); }
		else if (testCyclicCycles == 8) { ModemPutc('\r', NO_CONVERSION); }
		else if (testCyclicCycles == 9) { ModemPutc('\n', NO_CONVERSION); }
		else if (testCyclicCycles == 10) { ModemPutc('d', NO_CONVERSION); }
		else if (testCyclicCycles == 11) { ModemPutc('q', NO_CONVERSION); }
		else if (testCyclicCycles == 12) { ModemPutc('m', NO_CONVERSION); }
		else if (testCyclicCycles == 13) { ModemPutc('\r', NO_CONVERSION); }
		else if (testCyclicCycles == 14) { ModemPutc('\n', NO_CONVERSION); }
#endif

#if 1 /* Test (ISR/Exec Cycles) */
		extern uint32 sampleProcessTiming;

		if ((g_execCycles / 4) > 10000) { strcpy((char*)g_spareBuffer, ">10K"); }
		else { sprintf((char*)g_spareBuffer, "%d", (uint16)(g_execCycles / 4)); }

#if 0 /* Original */
		debug("(Cyclic Event) ISR Ticks/sec: %d (E:%s), Exec/sec: %s, SPT: %dus\r\n", (g_sampleCountHold / 4), ((g_channelSyncError == YES) ? "YES" : "NO"), (char*)g_spareBuffer, CycleCountToMicroseconds(sampleProcessTiming, SYS_CLK));
#else /* Added Fuel Gauge info */
extern uint32_t USBCPortControllerStatus(void);
extern uint16_t USBCPortControllerPStatus(void);
extern uint32_t USBCPortControllerPDStatus(void);
extern int32_t testLifetimeCurrentAvg;
extern uint32_t testLifetimeCurrentAvgCount;
//extern volatile uint32_t g_lifetimePeriodicSecondCount;
		UNUSED(sampleProcessTiming);
		//debug("(Cyclic Event) (%s) (USB: %08x %04x %08x) Exe/s: %s\r\n", FuelGaugeDebugString(), USBCPortControllerStatus(), USBCPortControllerPStatus(), USBCPortControllerPDStatus(), (char*)g_spareBuffer);
		testLifetimeCurrentAvg += (FuelGaugeGetCurrent() / 1000);
		testLifetimeCurrentAvgCount++;
		if (g_sampleProcessing == ACTIVE_STATE)
		{
			debug("(Cyclic Event) (%s) (%.0fmA avg) SPT: %lu, SPS: %d, Exe/s: %s\r\n", FuelGaugeDebugString(), (double)(((float)testLifetimeCurrentAvg) / (float)testLifetimeCurrentAvgCount), CycleCountToMicroseconds(sampleProcessTiming, SYS_CLK), (g_sampleCountHold / 4), (char*)g_spareBuffer);
		}
		else { debug("(Cyclic Event) (%s) (%.0fmA avg) Exe/s: %s\r\n", FuelGaugeDebugString(), (double)(((float)testLifetimeCurrentAvg) / (float)testLifetimeCurrentAvgCount), (char*)g_spareBuffer); }
		//else { debug("(Cyclic Event) (%d) (%s) (%.0fmA avg) Exe/s: %s\r\n", g_lifetimePeriodicSecondCount, FuelGaugeDebugString(), (double)(((float)testLifetimeCurrentAvg) / (float)testLifetimeCurrentAvgCount), (char*)g_spareBuffer); }

#if 0 /* Test Exp serial */
		// Test write out U8 every cycle (4 secs)
		Expansion_UART_WriteCharacter(0x55);
		Expansion_UART_WriteCharacter(0x38);
#endif
#if 0 /* Test 1 */
extern void tps25750_int_status_and_clear(void);
		tps25750_int_status_and_clear();
#elif 0 /* Test 2 */
extern void VerifyAccManuIDAndPartID(void);
		VerifyAccManuIDAndPartID();
#endif

#if 0 /* Not correct */
int tps25750_issue_get_sink_cap(void);
uint8_t* tps25750_get_rx_sink_cap(void);
		if (USBCPortControllerPDStatus() == 0x00090004)
		{
			if (tps25750_issue_get_sink_cap() == 0) { debug("Port Controller: Get Sink Capabilities command successful\r\n"); }
			else /* Command failed */ { debugErr("Port Controller: Get Sink Capabilities command failed\r\n"); }
			tps25750_get_rx_sink_cap();
			debugRaw("\r\nPort Controller: Rx Sink Caps is ");
			for (uint8_t i = 0; i < 31; i++)
			{
				debugRaw("%02x", g_spareBuffer[i]);
			}
			debugRaw("\r\n");
		}
#endif

#endif
		//g_sampleCountHold = 0;
		g_execCycles = 0;
		g_channelSyncError = NO;
#else
		debug("Cyclic Event\r\n");
#endif

		if (g_lowBatteryState == YES)
		{
			//if (gpio_get_pin_value(AVR32_PIN_PA21) == HIGH)
			if (GetExternalVoltageLevelAveraged(BATTERY_VOLTAGE) > LOW_VOLTAGE_THRESHOLD)
			{
				debug("Recovered from Low Battery Warning Event\r\n");

				g_lowBatteryState = NO;
			}
#if 0 /* Don't annoy the user for now */
			else
			{
				sprintf((char*)g_spareBuffer, "%s %s (%3.2f) %s", getLangText(BATTERY_VOLTAGE_TEXT), getLangText(LOW_TEXT),
						(GetExternalVoltageLevelAveraged(BATTERY_VOLTAGE)), getLangText(PLEASE_CHARGE_BATTERY_TEXT));
				OverlayMessage(getLangText(WARNING_TEXT), (char*)g_spareBuffer, (1 * SOFT_SECS));
			}
#endif
		}
		// Check if the battery voltage is below the safe threshold for operation
		else if (GetExternalVoltageLevelAveraged(BATTERY_VOLTAGE) < LOW_VOLTAGE_THRESHOLD)
		{
			// Change state to signal a low battery
			raiseSystemEventFlag(LOW_BATTERY_WARNING_EVENT);
		}

		CheckForCycleChange();
	}

#if 1 /* Test */
	//___________________________________________________________________________________________
extern uint8_t batteryChargerInterruptActive;
	if (batteryChargerInterruptActive)
	{
		debug("BC Int: Reg0: %04x, Reg1: %04x\r\n", GetBattChargerStatusReg0(), GetBattChargerStatusReg1());
		batteryChargerInterruptActive = NO;
	}
#endif

	//___________________________________________________________________________________________
	if (getSystemEventState(CYCLE_CHANGE_EVENT))
	{
		debug("Cycle Change Event (24 Hour cycle end)\r\n");
		g_testTimeSinceLastCycleChange = g_lifetimeHalfSecondTickCount;

		HandleCycleChangeEvent();

		// Clear flag after HandleCycleChangeEvent call to allowing checking for cycle change status
		clearSystemEventFlag(CYCLE_CHANGE_EVENT);
	}

	//___________________________________________________________________________________________
	if (getSystemEventState(UPDATE_TIME_EVENT))
	{
		if (UpdateCurrentTime() == PASSED)
		{
			clearSystemEventFlag(UPDATE_TIME_EVENT);
		}
	}

	//___________________________________________________________________________________________
	if (getSystemEventState(WARNING_EVENT))
	{
		clearSystemEventFlag(WARNING_EVENT);

		if (IsSoftTimerActive(ALARM_ONE_OUTPUT_TIMER_NUM) == NO)
		{
			// Enable power to the port before enabling the alert/alarm
			PowerControl(ENABLE_12V, ON);
			PowerControl(ALARM_1_ENABLE, ON);
			debug("Warning Event 1 Alarm started\r\n");

			// Assign soft timer to turn the Alarm 1 signal off
			AssignSoftTimer(ALARM_ONE_OUTPUT_TIMER_NUM, (uint32)(g_unitConfig.alarmOneTime * TICKS_PER_SEC), AlarmOneOutputTimerCallback);
		}

		if (IsSoftTimerActive(ALARM_TWO_OUTPUT_TIMER_NUM) == NO)
		{
			// Enable power to the port before enabling the alert/alarm
			PowerControl(ENABLE_12V, ON);
			PowerControl(ALARM_2_ENABLE, ON);
			debug("Warning Event 2 Alarm started\r\n");

			// Assign soft timer to turn the Alarm 2 signal off
			AssignSoftTimer(ALARM_TWO_OUTPUT_TIMER_NUM, (uint32)(g_unitConfig.alarmTwoTime * TICKS_PER_SEC), AlarmTwoOutputTimerCallback);
		}
	}

	//___________________________________________________________________________________________
	if (getSystemEventState(AUTO_DIALOUT_EVENT))
	{
		clearSystemEventFlag(AUTO_DIALOUT_EVENT);

		StartAutoDialoutProcess();
	}

	//___________________________________________________________________________________________
	if (getSystemEventState(UPDATE_OFFSET_EVENT))
	{
		UpdateChannelOffsetsForTempChange();
	}

	//___________________________________________________________________________________________
#if 0 /* old logic */
	if (g_powerOffActivated == YES)
	{
		// Check if idle
		if (g_sampleProcessing == IDLE_STATE)
		{
			// Check if not in Timer mode
			if ((g_unitConfig.timerMode == DISABLED) || ((g_unitConfig.timerMode == ENABLED) && (g_allowQuickPowerOffForTimerModeSetup == YES)))
			{
				if (!(getSystemEventState(TRIGGER_EVENT)) && !(getSystemEventState(BARGRAPH_EVENT)))
				{
					PowerUnitOff(SHUTDOWN_UNIT);
				}
			}
			else // In Timer mode
			{
				// Reset the flag
				g_powerOffActivated = NO;

				// Check if the user wants to leave timer mode
				HandleUserPowerOffDuringTimerMode();
			}
		}
		// Else Monitoring
		else
		{
			// Disable power off attempt
			g_powerOffActivated = NO;
		}
	}
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MenuEventManager(void)
{
	INPUT_MSG_STRUCT mn_msg;

	//debug("Menu Event Manager called\r\n");

	//___________________________________________________________________________________________
	if (getMenuEventState(RESULTS_MENU_EVENT))
	{
		clearMenuEventFlag(RESULTS_MENU_EVENT);

		if (g_triggerRecord.opMode == MANUAL_CAL_MODE)
		{
			SETUP_RESULTS_MENU_MANUAL_CAL_MSG(RESULTS_MENU);
		}
		else
		{
			SETUP_RESULTS_MENU_MONITORING_MSG(RESULTS_MENU);
		}
		JUMP_TO_ACTIVE_MENU();
	}

	//___________________________________________________________________________________________
	if (getTimerEventState(SOFT_TIMER_CHECK_EVENT))
	{
		clearTimerEventFlag(SOFT_TIMER_CHECK_EVENT);

		// Handle and process software timers
		CheckSoftTimers();
	}

	//___________________________________________________________________________________________
	if (getTimerEventState(TIMER_MODE_TIMER_EVENT))
	{
		clearTimerEventFlag(TIMER_MODE_TIMER_EVENT);

		if (CheckAndDisplayErrorThatPreventsMonitoring(OVERLAY) == NO)
		{
			// Safe to start monitoring
			debug("Timer mode: Start monitoring...\r\n");
			SETUP_MENU_WITH_DATA_MSG(MONITOR_MENU, g_triggerRecord.opMode);
			JUMP_TO_ACTIVE_MENU();
		}
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void CraftManager(void)
{
	//debug("Craft Manager called\r\n");

#if 0 /* No hardware modem controls, needs changing */
	// Check if there is an modem present and if the connection state is connected
	if ((MODEM_CONNECTED == READ_DSR) && (CONNECTED == g_modemStatus.connectionState))
	{
		// Check if the connection has been lost
		if (NO_CONNECTION == READ_DCD)
		{
			// Check if a ring indicator has been received
			if ((g_modemStatus.ringIndicator != READ_RI) && (READ_RI == RING))
			{
				// Check if nine rings have been received
				if (g_modemStatus.numberOfRings++ > 9)
				{
					// Reset flag
					g_modemStatus.numberOfRings = 0;

					// Assign timer to process modem init
					AssignSoftTimer(MODEM_DELAY_TIMER_NUM, MODEM_ATZ_QUICK_DELAY, ModemDelayTimerCallback);
				}
			}

			// Assign ring indicator to the status of the ring indicator line
			g_modemStatus.ringIndicator = (uint8)READ_RI;

			// Relock the system
			RemoteSystemLock(SET);

#if 1 /* New BLM feature */
			if (g_bargraphLiveMonitoringBISendActive == YES)
			{
				// Kill the Bar live data transfer
				g_bargraphLiveMonitoringBISendActive = NO;

				// Todo: Determine if disabling the UART TX Ready interrupt is needed
			}

			// Stop sending BLM data unless remote side requests
			g_modemStatus.barLiveMonitorOverride = BAR_LIVE_MONITORING_OVERRIDE_STOP;
#endif

			if (CONNECTED == g_modemStatus.firstConnection)
			{
				g_modemStatus.firstConnection = NOP_CMD;
				AssignSoftTimer(MODEM_DELAY_TIMER_NUM, MODEM_ATZ_QUICK_DELAY, ModemDelayTimerCallback);
			}
		}
		else
		{
			g_modemStatus.firstConnection = CONNECTED;
		}
	}
	else
	{
		if ((YES == g_modemSetupRecord.modemStatus) && (CONNECTED == g_modemStatus.connectionState))
		{
			g_modemStatus.connectionState = NOP_CMD;
			RemoteSystemLock(SET);
			AssignSoftTimer(MODEM_DELAY_TIMER_NUM, MODEM_ATZ_DELAY, ModemDelayTimerCallback);

#if 1 /* New addition to make sure the Auto Monitor Timer is refreshed in case it was changed via UCM */
			if ((g_unitConfig.autoMonitorMode != AUTO_NO_TIMEOUT) && (g_sampleProcessing != ACTIVE_STATE) && (IsSoftTimerActive(AUTO_MONITOR_TIMER_NUM) == NO))
			{
				AssignSoftTimer(AUTO_MONITOR_TIMER_NUM, (uint32)(g_unitConfig.autoMonitorMode * TICKS_PER_MIN), AutoMonitorTimerCallBack);
			}
#endif
		}
	}
#endif

	// Check if the Auto Dialout state is not idle
	if (g_autoDialoutState != AUTO_DIAL_IDLE)
	{
		// Run the Auto Dialout State machine
		AutoDialoutStateMachine();
	}

	// Are we transfering data, if so process the correct command.
	if (NOP_CMD != g_modemStatus.xferState)
	{
		if (DEMx_CMD == g_modemStatus.xferState)
		{
			g_modemStatus.xferState = sendDEMData();
		}
		else if (DSMx_CMD == g_modemStatus.xferState)
		{
			g_modemStatus.xferState = sendDSMData();
		}
		else if (DQMx_CMD == g_modemStatus.xferState)
		{
			g_modemStatus.xferState = sendDQMData();
		}
		else if (VMLx_CMD == g_modemStatus.xferState)
		{
			sendVMLData();
		}
	}

#if 1 /* Add ability to read and process the Expansion RS232 */
	ExpansionBridgeCheckAndReadData();
#endif

	// Data from the craft port is independent of the modem.
	if (YES == g_modemStatus.craftPortRcvFlag)
	{
		g_modemStatus.craftPortRcvFlag = NO;
		ProcessCraftData();
	}

	// Did we raise the craft data port flag, if so process the incomming data.
	if (getSystemEventState(CRAFT_PORT_EVENT))
	{
		clearSystemEventFlag(CRAFT_PORT_EVENT);
		RemoteCmdMessageProcessing();
	}

#if 0 /* Test multifunction actions on power button when keypad is not functional */
extern uint8_t g_powerButtonAction;
static uint8_t s_eventCount = 0;
	if (g_powerButtonAction)
	{
		INPUT_MSG_STRUCT mn_msg;
		if (g_powerButtonAction == 1)
		{
			g_triggerRecord.opMode = WAVEFORM_MODE;
			SETUP_MENU_WITH_DATA_MSG(MONITOR_MENU, g_triggerRecord.opMode);
			JUMP_TO_ACTIVE_MENU();
		}
		else if (g_powerButtonAction == 2)
		{
			strcpy((char*)&g_isrMessageBufferStruct.msg[0], "1\r\n");
			RemoteCmdMessageHandler(&g_isrMessageBufferStruct);
		}
		else if (g_powerButtonAction == 3)
		{
			if (s_eventCount < 3)
			{
				strcpy((char*)&g_isrMessageBufferStruct.msg[0], "T\r\n");
				RemoteCmdMessageHandler(&g_isrMessageBufferStruct);
				s_eventCount++;
			}
			else
			{
				mn_msg.cmd = STOP_MONITORING_CMD;
				mn_msg.length = 1;
				(*g_menufunc_ptrs[MONITOR_MENU])(mn_msg);
			}
		}
		else if (g_powerButtonAction == 4)
		{
		}

		g_powerButtonAction = 0;
	}
#endif
#if 1 /* Test */
extern uint8_t uart0BufferFull;
extern uint32_t uart0BufferCount;
extern uint8_t uart1BufferFull;
extern uint32_t uart1BufferCount;
		if (uart0BufferFull)
		{
			UNUSED(uart0BufferCount); debugRaw("<Cell: %s> ", (char*)&g_spareBuffer[2048]);
			memset(&g_spareBuffer[2048], 0, 2048);
			uart0BufferFull = NO;
		}

		if (uart1BufferFull)
		{
			debugRaw("<%s> (U1, %d chars)\r\n", (char*)&g_spareBuffer[4096], uart1BufferCount);
			memset(&g_spareBuffer[4096], 0, 2048);
			uart0BufferFull = NO;
		}
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GpsManager(void)
{
	if (g_gpsSerialData.ready == YES)
	{
		g_gpsSerialData.ready = NO;
		ProcessGpsSerialData();
	}

	if (g_gpsQueue.messageReady == YES)
	{
		g_gpsQueue.messageReady = NO;
		ProcessGpsMessage();
	}

	if (g_gpsBinaryQueue.binaryMessageReady == YES)
	{
		g_gpsBinaryQueue.binaryMessageReady = NO;
		ProcessGpsBinaryMessage();
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MessageManager(void)
{
	INPUT_MSG_STRUCT input_msg;

	// Check if there is a message in the queue
	if (CheckInputMsg(&input_msg) != INPUT_BUFFER_EMPTY)
	{
		debug("Processing message...\r\n");

		// Process message
		ProcessInputMsg(input_msg);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void FactorySetupManager(void)
{
	//debug("Factory Setup Manager called\r\n");

	INPUT_MSG_STRUCT mn_msg;

	if (g_factorySetupSequence == ENTER_FACTORY_SETUP)
	{
		g_factorySetupSequence = PROCESS_FACTORY_SETUP;

		KeypressEventMgr();
		MessageBox(getLangText(STATUS_TEXT), getLangText(YOU_HAVE_ENTERED_THE_FACTORY_SETUP_TEXT), MB_OK);

		SETUP_MENU_MSG(DATE_TIME_MENU);
		JUMP_TO_ACTIVE_MENU();
	}
#if 0 /* Added to work around On Key IRQ problem */
	else if (g_factorySetupRecord.invalid && g_factorySetupSequence != PROCESS_FACTORY_SETUP)
	{
		g_factorySetupSequence = PROCESS_FACTORY_SETUP;

		KeypressEventMgr();
		MessageBox(getLangText(STATUS_TEXT), getLangText(YOU_HAVE_ENTERED_THE_FACTORY_SETUP_TEXT), MB_OK);

		SETUP_MENU_MSG(DATE_TIME_MENU);
		JUMP_TO_ACTIVE_MENU();
	}
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleSystemEvents(void)
{
	if (g_systemEventFlags.wrd)
	SystemEventManager();

	if (g_menuEventFlags.wrd)
	MenuEventManager();

	MessageManager();
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UsbResetStateMachine(void)
{
	g_usbMassStorageState = USB_INIT_DRIVER;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
bool CheckUSBInOTGHostWaitingForDevice(void)
{
	// Check if USB state allows for sleep
	// Check if USB is in OTG Host mode but no active device is connected (otherwise ms_usb_prevent_sleep would be yes)

	// Todo: Fill in if necessary

	return (NO);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
extern uint8 g_sampleProcessing;
#if 0 /* old hw */
extern volatile U8 device_state;
extern Bool wrong_class_connected;
extern Bool ms_process_first_connect_disconnect;
extern Bool ms_usb_prevent_sleep;
#endif

#if 0 /* old hw */
extern Bool ushell_cmd_syncevents(uint8, uint16_t*, uint16_t*, uint16_t*, uint16_t*);
#include "ushell_task.h"
#endif

void UsbDeviceManager(void)
{
#if 1 /* Skip with immedaite return for now */
	return;
#endif

	INPUT_MSG_STRUCT mn_msg;
#if 0 /* temp remove while unused */
	uint16 totalFilesCopied;
	uint16 totalFilesSkipped;
	uint16 totalFilesReplaced;
	uint16 totalFilesDuplicated;
#endif
	//___________________________________________________________________________________________
	//
	// USB Driver Init
	//___________________________________________________________________________________________
	if (g_usbMassStorageState == USB_INIT_DRIVER)
	{
		// Check if the USB and Mass Storage Driver have never been initialized
		debug("Init USB Mass Storage Driver...\r\n");

#if 1 /* (NS8100_ALPHA_PROTOTYPE || NS8100_BETA_PROTOTYPE) */
#if 0 /* old hw */
		if (Is_usb_id_device())
		{
			// Disable VBUS power
			Usb_set_vbof_active_low();
		}
		else
		{
			// Enable VBUS power
			Usb_set_vbof_active_high();
		}
#endif
		// Wait for line to settle
		SoftUsecWait(25 * SOFT_MSECS);
#endif
		UNUSED(mn_msg);

#if 0 /* old hw */
		// Init the USB and Mass Storage driver			
		usb_task_init();
		device_mass_storage_task_init();
		host_mass_storage_task_init();
		ushell_task_init(SYS_CLK);

		// Set state to ready to process
		//g_usbMassStorageState = USB_NOT_CONNECTED;
		if (Is_usb_id_device())
		{
			//g_usbMode = USB_DEVICE_MODE_SELECTED;
			//g_usbMassStorageState = USB_DEVICE_MODE_SELECTED;
			debug("USB Device Mode enabled\r\n");
		}
		else
		{
			//g_usbMode = USB_HOST_MODE_SELECTED;
			//g_usbMassStorageState = USB_HOST_MODE_SELECTED;
			debug("USB OTG Host Mode enabled\r\n");
		}
		g_usbMassStorageState = USB_READY;
		ms_usb_prevent_sleep = NO;
		debug("USB State changed to: Ready (Post init)\r\n");
#endif

		//___________________________________________________________________________________________
		//
		// (On Re-Init) Check if USB was in OTG Host mode and cable + USB Thumb drive were removed at the same time (now in Device mode)
		//___________________________________________________________________________________________
		if (g_usbThumbDriveWasConnected == YES)
		{
			g_usbThumbDriveWasConnected = FALSE;
#if 0 /* old hw */
			ms_process_first_connect_disconnect = FALSE;
#endif
			sprintf((char*)g_spareBuffer, "%s %s", getLangText(USB_THUMB_DRIVE_TEXT), getLangText(DISCONNECTED_TEXT));
			OverlayMessage(getLangText(STATUS_TEXT), (char*)g_spareBuffer, (2 * SOFT_SECS));

			// Recall the current active menu to repaint the display
			mn_msg.cmd = 0; mn_msg.length = 0; mn_msg.data[0] = 0;
			JUMP_TO_ACTIVE_MENU();
		}
	}
	//___________________________________________________________________________________________
	//
	// USB Ready or USB Connected and actively Processing
	//___________________________________________________________________________________________
	else if ((g_usbMassStorageState == USB_READY) || (g_usbMassStorageState == USB_CONNECTED_AND_PROCESSING))
	{
		//___________________________________________________________________________________________
		//
		// Check if processing is needed elsewhere (requiring USB to be disabled)
		//___________________________________________________________________________________________
		if ((g_sampleProcessing == ACTIVE_STATE) || (getSystemEventState(TRIGGER_EVENT)) || (g_activeMenu == CAL_SETUP_MENU) || (g_activeMenu == MONITOR_MENU))
		{
			// Need to disable USB for other processing
			debug("USB disabled for other processing\r\n");

#if 0 /* old hw */
			if (Is_usb_enabled()) { Usb_disable(); }

			g_usbMassStorageState = USB_DISABLED_FOR_OTHER_PROCESSING;
			ms_usb_prevent_sleep = NO;
			debug("USB State changed to: Disabled for other processing\r\n");
#if 1 /* Added to prevent Host mode problems when a USB thumb drive is still connected going into monitoring and causing file system issues */
			// Re-Init the USB and Mass Storage driver
			usb_task_init();
			device_mass_storage_task_init();
			host_mass_storage_task_init();
			ushell_task_init(SYS_CLK);
#endif
#endif
		}
		//___________________________________________________________________________________________
		//
		else // Ready to process USB in either Device or OTG Host mode
		//___________________________________________________________________________________________
		{
#if 0 /* old hw */
			usb_task();
			device_mass_storage_task();
			host_mass_storage_task();
			ushell_task();

			//___________________________________________________________________________________________
			//
			// Check if USB is in Device mode (Ready to connect to a PC)
			//___________________________________________________________________________________________
			if (Is_usb_id_device())
			{
				//___________________________________________________________________________________________
				//
				// Check if VBUS is high meaning a remote PC is powering
				//___________________________________________________________________________________________
				if (Is_usb_vbus_high())
				{
					if (g_usbMassStorageState == USB_READY)
					{
						g_usbMassStorageState = USB_CONNECTED_AND_PROCESSING;
						ms_usb_prevent_sleep = YES;
						debug("USB State changed to: Connected and processing (Device)\r\n");
					}
					// else run as device in USB_CONNECTED_AND_PROCESSING state
				}
				//___________________________________________________________________________________________
				//
				// Check if state was connected and VBUS power is not present or been removed
				//___________________________________________________________________________________________
				else if (g_usbMassStorageState == USB_CONNECTED_AND_PROCESSING)
				{
					//___________________________________________________________________________________________
					//
					// Check if USB was in OTG Host mode and cable + USB Thumb drive were removed at the same time (now in Device mode)
					//___________________________________________________________________________________________
					if (g_usbThumbDriveWasConnected == YES)
					{
						g_usbThumbDriveWasConnected = FALSE;
						ms_process_first_connect_disconnect = FALSE;

						sprintf((char*)g_spareBuffer, "%s %s", getLangText(USB_THUMB_DRIVE_TEXT), getLangText(DISCONNECTED_TEXT));
						OverlayMessage(getLangText(STATUS_TEXT), (char*)g_spareBuffer, (2 * SOFT_SECS));

						// Recall the current active menu to repaint the display
						mn_msg.cmd = 0; mn_msg.length = 0; mn_msg.data[0] = 0;
						JUMP_TO_ACTIVE_MENU();
					}
#if 0 /* Not working properly */
					else // Device mode was connected and processing
					{
						debug("USB Device cable was removed\r\n");
						OverlayMessage(getLangText(STATUS_TEXT), getLangText(USB_DEVICE_CABLE_WAS_REMOVED_TEXT), (2 * SOFT_SECS));
					}
#endif
					g_usbMassStorageState = USB_INIT_DRIVER;
					ms_usb_prevent_sleep = NO;
					debug("USB State changed to: Ready (After USB device cable or thumb drive removed)\r\n");
				}
#if 0 /* Not working properly */
				else if (promptForUsbOtgHostCableRemoval == YES)
				{
					promptForUsbOtgHostCableRemoval = NO;

					debug("USB OTG Host cable was removed\r\n");
					OverlayMessage(getLangText(STATUS_TEXT), getLangText(USB_HOST_OTG_CABLE_WAS_REMOVED_TEXT), (2 * SOFT_SECS));
				}
#endif
			}
			//___________________________________________________________________________________________
			//
			else // USB is in OTG Host mode (Ready for USB Thumb drive)
			//___________________________________________________________________________________________
			{
				//___________________________________________________________________________________________
				//
				// Check if the wrong class was connected
				//___________________________________________________________________________________________
				if (wrong_class_connected == TRUE) //|| (ms_device_not_supported == TRUE))
				{
					Usb_disable();
					debug("USB disabled due to unsupported device\r\n");

					sprintf((char*)g_spareBuffer, "%s. %s", getLangText(UNSUPPORTED_DEVICE_TEXT), getLangText(PLEASE_REMOVE_IMMEDIATELY_BEFORE_SELECTING_OK_TEXT));
					MessageBox(getLangText(ERROR_TEXT), (char*)g_spareBuffer, MB_OK);

					wrong_class_connected = FALSE;
					//ms_device_not_supported = FALSE;

					Usb_enable();
					debug("USB re-enabled for processing again\r\n");

					usb_task_init();
					device_mass_storage_task_init();
					host_mass_storage_task_init();
					ushell_task_init(SYS_CLK);

					g_usbMassStorageState = USB_INIT_DRIVER;
					ms_usb_prevent_sleep = NO;
					debug("USB State changed to: Ready (USB re-enable after wrong class connected)\r\n");
				}
				//___________________________________________________________________________________________
				//
				// Check if USB Thumb drive has just been connected
				//___________________________________________________________________________________________
				else if ((ms_connected == TRUE) && (ms_process_first_connect_disconnect == TRUE))
				{
					// USB is active
					g_usbMassStorageState = USB_CONNECTED_AND_PROCESSING;
					ms_usb_prevent_sleep = YES;
					debug("USB State changed to: Connected and processing (Host)\r\n");

					// Don't process this connection again until it's disconnected
					ms_process_first_connect_disconnect = FALSE;

					// State processing to help with differentiating between USB Thumb drive disconnect and cable disconnect
					g_usbThumbDriveWasConnected = YES;

					// Check if the LCD Power was turned off
					if (g_lcdPowerFlag == DISABLED)
					{
						g_lcdPowerFlag = ENABLED;
#if 0 /* old hw */
						PowerControl(LCD_POWER_ENABLE, ON);
						SoftUsecWait(LCD_ACCESS_DELAY);
						SetLcdContrast(g_contrast_value);
						InitLcdDisplay();					// Setup LCD segments and clear display buffer
#else
						ft81x_init();
#endif
						AssignSoftTimer(LCD_POWER_ON_OFF_TIMER_NUM, (uint32)(g_unitConfig.lcdTimeout * TICKS_PER_MIN), LcdPwTimerCallBack);

						g_lcdBacklightFlag = ENABLED;
						SetLcdBacklightState(BACKLIGHT_MID);
						AssignSoftTimer(LCD_BACKLIGHT_ON_OFF_TIMER_NUM, LCD_BACKLIGHT_TIMEOUT, DisplayTimerCallBack);
					}
					else
					{
						SetLcdBacklightState(BACKLIGHT_MID);
					}

					sprintf((char*)g_spareBuffer, "%s %s", getLangText(USB_THUMB_DRIVE_TEXT), getLangText(DISCOVERED_TEXT));
					OverlayMessage(getLangText(STATUS_TEXT), (char*)g_spareBuffer, (2 * SOFT_SECS));

					//___________________________________________________________________________________________
					//
					// Check if the user wants to sync the unit events to the USB Thumb drive
					//___________________________________________________________________________________________
					if (MessageBox(getLangText(USB_DOWNLOAD_TEXT), getLangText(SYNC_EVENTS_TO_USB_THUMB_DRIVE_Q_TEXT), MB_YESNO) == MB_FIRST_CHOICE)
					{
						OverlayMessage(getLangText(STATUS_TEXT), getLangText(SYNC_IN_PROGRESS_TEXT), 0);

						totalFilesCopied = totalFilesSkipped = totalFilesReplaced = totalFilesDuplicated = 0;

#if 0 /* old hw */
						if (ushell_cmd_syncevents(USB_SYNC_NORMAL, &totalFilesCopied, &totalFilesSkipped, &totalFilesReplaced, &totalFilesDuplicated) == TRUE)
						{
							sprintf((char*)g_spareBuffer, "%s. %s (%d) %s (%d) %s (%d)", getLangText(SYNC_SUCCESSFUL_TEXT), getLangText(TOTAL_FILES_TEXT),
									(totalFilesCopied + totalFilesSkipped), getLangText(NEW_TEXT), totalFilesCopied, getLangText(SKIPPED_TEXT), totalFilesSkipped);
							MessageBox(getLangText(USB_DOWNLOAD_TEXT), (char*)g_spareBuffer, MB_OK);

							if (totalFilesReplaced || totalFilesDuplicated)
							{
								sprintf((char*)g_spareBuffer, "%s. %s (%d) %s (%d)", getLangText(SYNC_SUCCESSFUL_TEXT), getLangText(REPLACED_TEXT), totalFilesReplaced, getLangText(DUPLICATED_TEXT), totalFilesDuplicated);
								MessageBox(getLangText(USB_DOWNLOAD_TEXT), (char*)g_spareBuffer, MB_OK);
							}
						}
						else // Error
						{
							MessageBox(getLangText(USB_DOWNLOAD_TEXT), getLangText(SYNC_ENCOUNTERED_AN_ERROR_TEXT), MB_OK);
						}
#endif
						// Return to the main menu (in case the sync file exists menu is displayed)
						SETUP_MENU_MSG(MAIN_MENU);
					}
					//___________________________________________________________________________________________
					//
					else // User does not want to sync events
					//___________________________________________________________________________________________
					{
						// Recall the current active menu to repaint the display
						mn_msg.cmd = 0; mn_msg.length = 0; mn_msg.data[0] = 0;
					}

					//___________________________________________________________________________________________
					//
					// Check if there is no active external charging
					//___________________________________________________________________________________________
					if (GetExternalVoltageLevelAveraged(EXT_CHARGE_VOLTAGE) < EXTERNAL_VOLTAGE_PRESENT)
					{
						// Check if a device is still connected
						if (ms_connected == TRUE)
						{
							OverlayMessage(getLangText(STATUS_TEXT), getLangText(PLEASE_REMOVE_THUMB_DRIVE_TO_CONSERVE_POWER_TEXT), (2 * SOFT_SECS));
						}
					}

					JUMP_TO_ACTIVE_MENU();
				}
				//___________________________________________________________________________________________
				//
				// Check if USB Thumb drive has just been removed
				//___________________________________________________________________________________________
				else if ((ms_connected == FALSE) && (ms_process_first_connect_disconnect == TRUE))
				{
					if (g_usbThumbDriveWasConnected == YES)
					{
						g_usbThumbDriveWasConnected = NO;

						sprintf((char*)g_spareBuffer, "%s %s", getLangText(USB_THUMB_DRIVE_TEXT), getLangText(DISCONNECTED_TEXT));
						OverlayMessage(getLangText(STATUS_TEXT), (char*)g_spareBuffer, (2 * SOFT_SECS));

						// Recall the current active menu to repaint the display
						mn_msg.cmd = 0; mn_msg.length = 0; mn_msg.data[0] = 0;
						JUMP_TO_ACTIVE_MENU();
					}

					// Don't process this disconnection again until it's connected
					ms_process_first_connect_disconnect = FALSE;

					// USB needs to be reset
					g_usbMassStorageState = USB_INIT_DRIVER;
					ms_usb_prevent_sleep = NO;
					//promptForUsbOtgHostCableRemoval = YES;
					debug("USB State changed to: Ready (After USB thumb drive removed)\r\n");
				}
			}
#endif
		}
	}
	//___________________________________________________________________________________________
	//
	// USB Disabled for other processing
	//___________________________________________________________________________________________
	else if (g_usbMassStorageState == USB_DISABLED_FOR_OTHER_PROCESSING)
	{
		if ((g_sampleProcessing != ACTIVE_STATE) && (!getSystemEventState(TRIGGER_EVENT)) && (g_activeMenu != CAL_SETUP_MENU))
		{
#if 0 /* old hw */
			debug("USB enabled for processing again\r\n");
			Usb_enable();

			usb_task_init();
			device_mass_storage_task_init();
			host_mass_storage_task_init();
			ushell_task_init(SYS_CLK);

			g_usbMassStorageState = USB_READY;
			ms_usb_prevent_sleep = NO;
			debug("USB State changed to: Ready (After Usb was disabled for other processing)\r\n");
#endif
		}
	}

#if 0
	if (Is_usb_id_device() && (g_usbMode == USB_HOST_MODE_SELECTED))
	{
		g_usbMode = USB_DEVICE_MODE_SELECTED;

		// Disable VBUS power
		Usb_set_vbof_active_low();

		// Wait for line to settle
		SoftUsecWait(25 * SOFT_MSECS);
	}
	else if ((!Is_usb_id_device()) && (g_usbMode == USB_DEVICE_MODE_SELECTED))
	{
		g_usbMode = USB_HOST_MODE_SELECTED;

		// Enable VBUS power
		Usb_set_vbof_active_high();

		// Wait for line to settle
		SoftUsecWait(25 * SOFT_MSECS);
	}
#endif

#if 0
	if ((g_usbMassStorageState == USB_DEVICE_MODE_SELECTED) && (Is_usb_id_device()))
	{
		usb_task();
		device_mass_storage_task();
	}
	else if ((g_usbMassStorageState == USB_HOST_MODE_SELECTED) && (!Is_usb_id_device()))
	{
		usb_task();
		host_mass_storage_task();
		ushell_task();
	}
	else
	{
		if (Is_usb_id_device())
		{
			debug("Changing USB to Device Mode\r\n");
			g_usbMassStorageState = USB_DEVICE_MODE_SELECTED;
			usb_task_init();
			device_mass_storage_task_init();
		}
		else
		{
			debug("Changing USB to OTG Host Mode\r\n");
			g_usbMassStorageState = USB_HOST_MODE_SELECTED;
			usb_task_init();
			host_mass_storage_task_init();
			ushell_task_init(SYS_CLK);
		}
	}
#endif

#if 0 /* Normal */
	//___________________________________________________________________________________________
	else if ((g_usbMassStorageState == USB_NOT_CONNECTED) || (g_usbMassStorageState == USB_HOST_MODE_WAITING_FOR_DEVICE))
	{
		// Check if ready for USB (not monitoring and not handling a trigger and not processing an SD card file)
		if ((g_sampleProcessing != ACTIVE_STATE) && (!getSystemEventState(TRIGGER_EVENT)))
		{
			// Check if USB ID is set for Device and VBUS is High (plugged into PC)
			if ((Is_usb_id_device()) && (Is_usb_vbus_high()))
			{
				OverlayMessage(getLangText(USB_DEVICE_STATUS_TEXT), getLangText(USB_TO_PC_CABLE_WAS_CONNECTED_TEXT), 1 * SOFT_SECS);
				debug("USB Device mode: USB to PC connection established\r\n");

				// Recall the current active menu to repaint the display
				mn_msg.cmd = 0; mn_msg.length = 0; mn_msg.data[0] = 0;
				JUMP_TO_ACTIVE_MENU();

				// Re-Init the USB and Mass Storage driver
				debug("USB Device Mass Storage Driver Re-Init\r\n");
				usb_task_init();
				device_mass_storage_task_init();

				// Set state to ready to process
				g_usbMassStorageState = USB_CONNECTED_AND_PROCESSING;
			}
			// Check if USB ID is set for Host
			else if (!Is_usb_id_device())
			{
				if ((Is_host_device_connection()) || (device_state == DEVICE_POWERED))
				{
					OverlayMessage(getLangText(USB_HOST_STATUS_TEXT), getLangText(USB_DEVICE_WAS_CONNECTED_TEXT), 1 * SOFT_SECS);
					debug("USB OTG Host mode: OTG Host cable was connected\r\n");

					// Recall the current active menu to repaint the display
					mn_msg.cmd = 0; mn_msg.length = 0; mn_msg.data[0] = 0;
					JUMP_TO_ACTIVE_MENU();

					// Re-Init the USB and Mass Storage driver
					debug("USB OTG Host Mass Storage Driver Re-Init\r\n");
					usb_task_init();
					host_mass_storage_task_init();
					ushell_task_init(SYS_CLK);

					// Set state to host mode looking for a device
					g_usbMassStorageState = USB_CONNECTED_AND_PROCESSING;
				}
				else if (g_usbMassStorageState == USB_NOT_CONNECTED)
				{
					OverlayMessage(getLangText(USB_HOST_STATUS_TEXT), getLangText(USB_OTG_HOST_CABLE_WAS_CONNECTED_TEXT), 1 * SOFT_SECS);
					debug("USB OTG Host mode: OTG Host cable was connected\r\n");

					// Recall the current active menu to repaint the display
					mn_msg.cmd = 0; mn_msg.length = 0; mn_msg.data[0] = 0;
					JUMP_TO_ACTIVE_MENU();

					// Re-Init the USB and Mass Storage driver
					debug("USB OTG Host Mass Storage Driver Re-Init\r\n");
					usb_task_init();
					host_mass_storage_task_init();
					ushell_task_init(SYS_CLK);

					// Set state to host mode looking for a device
					g_usbMassStorageState = USB_HOST_MODE_WAITING_FOR_DEVICE;
				}
			}
		}
	}
	//___________________________________________________________________________________________
	else if (g_usbMassStorageState == USB_HOST_MODE_WAITING_FOR_DEVICE)
	{
		if ((Is_host_device_connection()) || (device_state == DEVICE_POWERED))
		{
			OverlayMessage(getLangText(USB_HOST_STATUS_TEXT), getLangText(USB_DEVICE_WAS_CONNECTED_TEXT), 1 * SOFT_SECS);
			debug("USB OTG Host mode: OTG Host cable was connected\r\n");

			// Set state to host mode looking for a device
			g_usbMassStorageState = USB_CONNECTED_AND_PROCESSING;
		}
	}
	//___________________________________________________________________________________________
	else if (g_usbMassStorageState == USB_CONNECTED_AND_PROCESSING)
	{
		if (((Is_usb_id_device() && Is_usb_vbus_high()) || ((!Is_usb_id_device()) && (!Is_host_device_disconnection())))
			&& (g_sampleProcessing != ACTIVE_STATE) && (!getSystemEventState(TRIGGER_EVENT)))
		{
			// Call Usb and Device Storage drivers if connected to check and handle incoming actions
			usb_task();
			device_mass_storage_task();
			host_mass_storage_task();
			ushell_task();
		}
		else // USB connection lost/gone or needs to be disabled
		{
			if (g_sampleProcessing == ACTIVE_STATE) // A Trigger event is an extension of Active state so don't need to check for that specifically
			{
				OverlayMessage(getLangText(USB_STATUS_TEXT), getLangText(USB_CONNECTION_DISABLED_FOR_MONITORING_TEXT), (1 * SOFT_SECS));
				debug("USB disabled for monitoring\r\n");
				Usb_disable();
				g_usbMassStorageState = USB_DISABLED_FOR_OTHER_PROCESSING;
			}
			else
			{
				// Check if USB ID is set for Device mode (PC connection)
				if (Is_usb_id_device())
				{
					OverlayMessage(getLangText(USB_DEVICE_STATUS_TEXT), getLangText(USB_TO_PC_CABLE_WAS_DISCONNECTED_TEXT), (1 * SOFT_SECS));
					debug("USB Device mode: USB to PC connection removed\r\n");
				}
				else
				{
					OverlayMessage(getLangText(USB_HOST_STATUS_TEXT), getLangText(USB_DEVICE_WAS_REMOVED_TEXT), (1 * SOFT_SECS));
					debug("USB OTG Host mode: Device disconnected\r\n");
				}

				// Recall the current active menu to repaint the display
				mn_msg.cmd = 0; mn_msg.length = 0; mn_msg.data[0] = 0;
				JUMP_TO_ACTIVE_MENU();

				g_usbMassStorageState = USB_NOT_CONNECTED;
			}
		}
	}
	//___________________________________________________________________________________________
	else if (g_usbMassStorageState == USB_DISABLED_FOR_OTHER_PROCESSING)
	{
		// Check if system is ready for USB processing again
		if ((g_sampleProcessing != ACTIVE_STATE) && (!getSystemEventState(TRIGGER_EVENT)))
		{
			// Reenable the USB
			debug("USB re-enabled\r\n");
			Usb_enable();
			g_usbMassStorageState = USB_NOT_CONNECTED;
		}
	}
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UsbDisableIfActive(void)
{
	if ((g_usbMassStorageState == USB_READY) || (g_usbMassStorageState == USB_CONNECTED_AND_PROCESSING))
	{
		// Need to disable USB for other processing
		debug("USB disabled for other processing\r\n");

#if 0 /* old hw */
		if (Is_usb_enabled()) { Usb_disable(); }

		g_usbMassStorageState = USB_DISABLED_FOR_OTHER_PROCESSING;
		ms_usb_prevent_sleep = NO;
		debug("USB State changed to: Disabled for other processing\r\n");
#endif
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#define APPLICATION		((void *)0x10000000) // Internal Flash memory start address
const char default_boot_name[] = { "Boot.s" };

void BootLoadManager(void)
{
	static void (*func)(void);
	func = (void(*)())APPLICATION;
#if 0 /* temp remove while unused */
	int file = -1;
	uint32 baudRate;
#endif

	// Check if requested to jump to boot
	if (g_quickBootEntryJump)
	{
		if (g_sampleProcessing == ACTIVE_STATE)
		{
			// Don't allow hidden jumping to boot during Monitoring
			g_quickBootEntryJump = NO;
			return;
		}
		else if (g_unitConfig.timerMode == ENABLED)
		{
			// Disable Timer mode since restarting would force a prompt for user action
			g_unitConfig.timerMode = DISABLED;
			SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

			OverlayMessage(getLangText(WARNING_TEXT), getLangText(TIMER_MODE_DISABLED_TEXT), (2 * SOFT_SECS));
		}
		// else (g_sampleProcessing == IDLE_STATE)

		if (g_lcdPowerFlag == DISABLED)
		{
			ft81x_init(); // Power up display and init driver
			SetLcdBacklightState(BACKLIGHT_MID);
		}
		else
		{
			SetLcdBacklightState(BACKLIGHT_MID);
		}

		if (g_quickBootEntryJump == QUICK_BOOT_ENTRY_FROM_MENU)
		{
			sprintf((char*)g_spareBuffer, "%s. %s...", getLangText(MENU_ACTIVATION_TEXT), getLangText(INITIALIZING_TEXT));
			OverlayMessage(getLangText(APP_LOADER_TEXT), (char*)g_spareBuffer, (2 * SOFT_SECS));
		}
		else if (g_quickBootEntryJump == QUICK_BOOT_ENTRY_FROM_SERIAL)
		{
			sprintf((char*)g_spareBuffer, "%s. %s...", getLangText(SERIAL_ACTIVATION_TEXT), getLangText(INITIALIZING_TEXT));
			OverlayMessage(getLangText(APP_LOADER_TEXT), (char*)g_spareBuffer, (2 * SOFT_SECS));
		}

		if (g_fileAccessLock != AVAILABLE)
		{
			ReportFileSystemAccessProblem("Bootloader access");
		}

#if 0 /* old hw */
		sprintf(g_spareFileName, "%s%s", SYSTEM_PATH, default_boot_name);
		file = open(g_spareFileName, O_RDONLY);

		while (file == -1)
		{
			sprintf((char*)g_spareBuffer, "%s %s. %s", getLangText(APP_LOADER_TEXT), getLangText(FILE_NOT_FOUND_TEXT), getLangText(CONNECT_USB_CABLE_TEXT));
			OverlayMessage(getLangText(APP_LOADER_TEXT), (char*)g_spareBuffer, 0);

			usb_task_init();
			device_mass_storage_task_init();

			while(!Is_usb_vbus_high()) {}

			sprintf((char*)"%s:%s (%s)", getLangText(COPY_TO_SYSTEM_DIR_TEXT), default_boot_name, getLangText(REMOVE_CABLE_WHEN_DONE_TEXT));
			OverlayMessage(getLangText(APP_LOADER_TEXT), (char*)g_spareBuffer, 0);

			usb_task_init();
			device_mass_storage_task_init();

			while (Is_usb_vbus_high())
			{
				usb_task();
				device_mass_storage_task();
			}
			
			SoftUsecWait(250 * SOFT_MSECS);
			
			file = open(g_spareFileName, O_RDONLY);
		}

		// Display initializing message
		debug("Starting Boot..\r\n");

		sprintf((char*)g_spareBuffer, "%s...", getLangText(STARTING_APP_LOADER_TEXT));
		OverlayMessage(getLangText(APP_LOADER_TEXT), (char*)g_spareBuffer, 2 * SOFT_SECS);

		ClearLcdDisplay();

		if (Unpack_srec(file) == -1)
		{
			debugErr("SREC unpack unsuccessful\r\n");
		}

		g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
		close(file);
#endif
#if 0 /* Removed debug log file due to inducing system problems */
		debug("Dumping debug output to debug log file\r\n");
		debug("Adding On/Off Log timestamp before jumping to boot\r\n");
		WriteDebugBufferToFile();
#endif
		AddOnOffLogTimestamp(JUMP_TO_BOOT);

		// -------------------
		// Shut down resources
		// -------------------

		// Disable drivers, timers

		// Disable USB

		// Close filesystem
		f_mount(NULL, "", 0);

		// Disable interrupts
		__disable_irq();

		//Jump to boot application code
		func();
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
/* static removed from rs232PutToSleepState since inline functons which reference aren't static */
uint8 rs232PutToSleepState = NO;
inline void SetupPowerSavingsBeforeSleeping(void)
{
	// Check if Rs232 power savings is enabled
	if (g_unitConfig.rs232PowerSavings)
	{
		if (READ_DCD == NO_CONNECTION)
		{
			rs232PutToSleepState = YES;
		}
	}

	g_powerSavingsForSleepEnabled = YES;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
/* inline */ void RevertPowerSavingsAfterSleeping(void)
{
	// Check if Rs232 power savings is enabled
	if (g_unitConfig.rs232PowerSavings)
	{
		if (rs232PutToSleepState == YES)
		{
			rs232PutToSleepState = NO;
		}
	}

	g_powerSavingsForSleepEnabled = NO;
}

void GoSleepState(uint32_t mode)
{
	switch (mode)
	{
		//---------------------------------------------------------------------------------------------
		case ACTIVE_MODE:
		//---------------------------------------------------------------------------------------------
			/*
				This is the highest performance mode. All internal clocks, registers, memory, and peripherals are enabled. The CPU is
				running and executing application code. The Smart DMA can perform background processing and data transfers. All
				oscillators are available.
				Dynamic clocking allows firmware to selectively enable or disable clocks and power to individual peripherals, providing the
				optimal mix of high-performance and power conservation. Internal RAM that can be enabled, disabled, or placed in low-
				power RAM Retention Mode include data SRAM memory blocks, on-chip caches, and on-chip FIFOs.
			*/
			MXC_GCR->pmr = ((MXC_GCR->pmr & ~(MXC_F_GCR_PMR_MODE)) | MXC_S_GCR_PMR_MODE_ACTIVE); // Not necessary since this is done automatically by the MPU coming out of sleep
			break;

		//---------------------------------------------------------------------------------------------
		case SLEEP_MODE:
		//---------------------------------------------------------------------------------------------
			/*
				This is a low-power mode that suspends the CPU with a fast wakeup time to ACTIVE mode. It is like ACTIVE mode except the
				CPU clock is disabled, which temporarily prevents the CPU from executing code. The Smart DMA can operate in the
				background to perform background processing and data transfers. All oscillators remain active if enabled and the Always On
				Domain (AOD) and RAM retention is retain state.
				The device returns to ACTIVE mode from any internal or external interrupt.
			*/
#if 0 /* Raw from datasheet */
			SCB->SCR &= ~(SCB_SCR_SLEEPDEEP_Msk);
			__ASM volatile("wfi");
#else /* Turns out there is a framework driver for Sleep mode */
			MXC_LP_EnterSleepMode();
#endif
			break;

		//---------------------------------------------------------------------------------------------
		case BACKGROUND_MODE:
		//---------------------------------------------------------------------------------------------
			/*
				This mode is suitable for the Smart DMA to operate in the background and perform background processing data transfers
				on peripheral and SRAM data.
				This is the same as SLEEP mode except both the CPU clock and CPU power (VCORE) are temporarily gated off. State retention
				of the CPU is enabled, allowing all CPU registers to maintain their contents and the oscillators remain active if enabled.
				Because both the clock and power to the CPU is disabled, this has the advantage of drawing less power than SLEEP.
				However, the CPU takes longer to wakeup compared to SLEEP.
			*/
#if 0 /* Raw from datasheet */
			MXC_PWRSEQ->ctrl |= (MXC_F_PWRSEQ_CTRL_BKGRND);
			SCB->SCR |= (SCB_SCR_SLEEPDEEP_Msk);
			__ASM volatile("wfi");
#else /* Turns out there is a framework driver for Background mode */
			MXC_LP_EnterBackgroundMode();
#endif
			break;

		//---------------------------------------------------------------------------------------------
		case DEEPSLEEP_MODE:
		//---------------------------------------------------------------------------------------------
			/*
				This is like BACKGROUND mode except that all internal clocks are gated off. SYSOSC is gated off, so the two main bus clocks
				PCLK and HCLK are inactive. The CPU state is retained.
				Because the main bus clocks are disabled, all peripherals are inactive except for the RTC which has its own independent
				oscillator. Only the RTC, USB wakeup or external interrupt can return the device to ACTIVE. The Smart DMA and Watchdog
				Timers are inactive in this mode.
				All internal register contents and all RAM contents are preserved. The GPIO pins retain their state in this mode. The Always-
				on Domain (AoD) and RAM Retention are available.
				Three oscillators can be set to optionally automatically disable themselves when the device enters DEEPSLEEP mode: the
				7.3728MHz oscillator, the 50MHz oscillator, and the 120MHz oscillator. The 8Khz and 32.768kHz oscillators are available.

				Note: DEEPSLEEP Mode Fast Wakeup Enable, LP_CTRL, Bit 10, fwkm, R/W, 0 (Set to 1 to enable fast wakeup from DEEPSLEEP mode)
			*/
#if 0 /* Raw from datasheet */
			MXC_PWRSEQ->ctrl &= ~(MXC_F_PWRSEQ_CTRL_BKGRND);
			SCB->SCR |= (SCB_SCR_SLEEPDEEP_Msk);
			__ASM volatile("wfi");
#else /* Turns out there is a framework driver for Deep Sleep mode */
			MXC_LP_EnterDeepSleepMode();
#endif
			break;

		//---------------------------------------------------------------------------------------------
		case BACKUP_MODE:
		//---------------------------------------------------------------------------------------------
			/*
				This is the lowest power operating mode. All oscillators are disabled except for the 8kHz and the 32kHz oscillator. SYSOSC is
				gated off, so PCLK and HCLK are inactive. The CPU state is not maintained.
				Only the RTC can operate in BACKUP mode. The AoD and RAM Retention can optionally be set to automatically disable (and
				clear) themselves when entering this mode. Data retention in this mode is maintained using VCORE and/or VRTC. The type of
				data retained is dependent upon whether only one, or both, of these voltages are enabled.
				Optionally, VCORE can be gated off and the internal retention regulator enabled, allowing the device to be powered only by
				VRTC. Enabling VCORE will wake the device to ACTIVE mode.
				The amount of RAM memory retained is dependent upon which voltages are enabled.
				If only VRTC is enabled, up to 96KBytes of SRAM can be retained.
				If both VRTC and VCORE are enabled, up to 1024KBytes SRAM can be retained.
				BACKUP mode supports the same wakeup sources as DEEPSLEEP mode.
			*/
#if 0 /* Raw from datasheet */
			MXC_GCR->pmr = ((MXC_GCR->pmr & ~(MXC_F_GCR_PMR_MODE)) | MXC_S_GCR_PMR_MODE_BACKUP);
#else /* Turns out there is a framework driver for Backup mode */
			MXC_LP_EnterBackupMode();
#endif
			break;
	} // End of switch
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void PowerManager(void)
{
	uint8 sleepStateNeeded;

	// Check if no System Events (or just update offset) and LCD is off and Modem is not transferring and USB is not connected
	if (((g_systemEventFlags.wrd == NO_SYSTEM_EVENT_ACTIVE) || (g_systemEventFlags.wrd == UPDATE_OFFSET_EVENT)) && (GetPowerControlState(LCD_POWER_ENABLE) == OFF) &&
		(g_modemStatus.xferState == NOP_CMD) && (g_usbMassStorageState != USB_CONNECTED_AND_PROCESSING))
	{
		SetupPowerSavingsBeforeSleeping();

		// Check if actively monitoring
		if (g_sampleProcessing == ACTIVE_STATE)
		{
			// Check if a higher sample rate that can't use Stop sleep mode (Wave 16K and Bar 8K)
			if ((g_triggerRecord.trec.sample_rate == SAMPLE_RATE_16K) || (((g_triggerRecord.opMode == BARGRAPH_MODE) || (g_triggerRecord.opMode == COMBO_MODE)) && (g_triggerRecord.trec.sample_rate == SAMPLE_RATE_8K)))
			{
				sleepStateNeeded = SLEEP_MODE;
			}
			else // Lower sample rates can use Stop mode (Wave, Bar and Combo for 0.5K, 1K and 2K)
			{
				sleepStateNeeded = BACKGROUND_MODE;
			}
		}
		else // Not monitoring
		{
			sleepStateNeeded = DEEPSLEEP_MODE;
		}

		// Check if USB is in OTG Host mode but no active device is connected (requiring sleep level be no deeper than Frozen)
		if (CheckUSBInOTGHostWaitingForDevice() == YES)
		{
			// Set to frozen to allow USB OTG Host processing to recognize a device
			sleepStateNeeded = SLEEP_MODE;
		}

#if 1 /* Test not going so deep to sleep due to lack of a wake up source at the moment */
		if (sleepStateNeeded > SLEEP_MODE) { sleepStateNeeded = SLEEP_MODE; }
		//if (sleepStateNeeded > SLEEP_MODE) { sleepStateNeeded = DEEPSLEEP_MODE; }
#endif
		// Check if sleep mode changed
		if (g_sleepModeState != sleepStateNeeded)
		{
			// Track the new state
			g_sleepModeState = sleepStateNeeded;

			if (g_sleepModeState == SLEEP_MODE) { debug("Changing Sleep mode to Sleep\r\n"); }
			else if (g_sleepModeState == BACKGROUND_MODE) { debug("Changing Sleep mode to Background\r\n"); }
			else if (g_sleepModeState == DEEPSLEEP_MODE) { debug("Changing Sleep mode to Deepsleep\r\n"); }
			else if (g_sleepModeState == BACKUP_MODE) { debug("Changing Sleep mode to Backup\r\n"); }
		}

		switch (g_sleepModeState)
		{
			//---------------------------------------------------------------------------------------------
			case ACTIVE_MODE:
			//---------------------------------------------------------------------------------------------
				/*
					This is the highest performance mode. All internal clocks, registers, memory, and peripherals are enabled. The CPU is
					running and executing application code. The Smart DMA can perform background processing and data transfers. All
					oscillators are available.
					Dynamic clocking allows firmware to selectively enable or disable clocks and power to individual peripherals, providing the
					optimal mix of high-performance and power conservation. Internal RAM that can be enabled, disabled, or placed in low-
					power RAM Retention Mode include data SRAM memory blocks, on-chip caches, and on-chip FIFOs.
				*/
				//MXC_GCR->pmr = ((MXC_GCR->pmr & ~(MXC_F_GCR_PMR_MODE)) | MXC_S_GCR_PMR_MODE_ACTIVE); // Not necessary since this is done automatically by the MPU coming out of sleep
				break;

			//---------------------------------------------------------------------------------------------
			case SLEEP_MODE:
			//---------------------------------------------------------------------------------------------
				/*
					This is a low-power mode that suspends the CPU with a fast wakeup time to ACTIVE mode. It is like ACTIVE mode except the
					CPU clock is disabled, which temporarily prevents the CPU from executing code. The Smart DMA can operate in the
					background to perform background processing and data transfers. All oscillators remain active if enabled and the Always On
					Domain (AOD) and RAM retention is retain state.
					The device returns to ACTIVE mode from any internal or external interrupt.
				*/
#if 0 /* Raw from datasheet */
				SCB->SCR &= ~(SCB_SCR_SLEEPDEEP_Msk);
				__ASM volatile("wfi");
#else /* Turns out there is a framework driver for Sleep mode */
				MXC_LP_EnterSleepMode();
#endif
				break;

			//---------------------------------------------------------------------------------------------
			case BACKGROUND_MODE:
			//---------------------------------------------------------------------------------------------
				/*
					This mode is suitable for the Smart DMA to operate in the background and perform background processing data transfers
					on peripheral and SRAM data.
					This is the same as SLEEP mode except both the CPU clock and CPU power (VCORE) are temporarily gated off. State retention
					of the CPU is enabled, allowing all CPU registers to maintain their contents and the oscillators remain active if enabled.
					Because both the clock and power to the CPU is disabled, this has the advantage of drawing less power than SLEEP.
					However, the CPU takes longer to wakeup compared to SLEEP.
				*/
#if 0 /* Raw from datasheet */
				MXC_PWRSEQ->ctrl |= (MXC_F_PWRSEQ_CTRL_BKGRND);
				SCB->SCR |= (SCB_SCR_SLEEPDEEP_Msk);
				__ASM volatile("wfi");
#else /* Turns out there is a framework driver for Background mode */
				MXC_LP_EnterBackgroundMode();
#endif
				break;

			//---------------------------------------------------------------------------------------------
			case DEEPSLEEP_MODE:
			//---------------------------------------------------------------------------------------------
				/*
					This is like BACKGROUND mode except that all internal clocks are gated off. SYSOSC is gated off, so the two main bus clocks
					PCLK and HCLK are inactive. The CPU state is retained.
					Because the main bus clocks are disabled, all peripherals are inactive except for the RTC which has its own independent
					oscillator. Only the RTC, USB wakeup or external interrupt can return the device to ACTIVE. The Smart DMA and Watchdog
					Timers are inactive in this mode.
					All internal register contents and all RAM contents are preserved. The GPIO pins retain their state in this mode. The Always-
					on Domain (AoD) and RAM Retention are available.
					Three oscillators can be set to optionally automatically disable themselves when the device enters DEEPSLEEP mode: the
					7.3728MHz oscillator, the 50MHz oscillator, and the 120MHz oscillator. The 8Khz and 32.768kHz oscillators are available.

					Note: DEEPSLEEP Mode Fast Wakeup Enable, LP_CTRL, Bit 10, fwkm, R/W, 0 (Set to 1 to enable fast wakeup from DEEPSLEEP mode)
				*/
#if 0 /* Raw from datasheet */
				MXC_PWRSEQ->ctrl &= ~(MXC_F_PWRSEQ_CTRL_BKGRND);
				SCB->SCR |= (SCB_SCR_SLEEPDEEP_Msk);
				__ASM volatile("wfi");
#else /* Turns out there is a framework driver for Deep Sleep mode */
				MXC_LP_EnterDeepSleepMode();
#endif
				break;

			//---------------------------------------------------------------------------------------------
			case BACKUP_MODE:
			//---------------------------------------------------------------------------------------------
				/*
					This is the lowest power operating mode. All oscillators are disabled except for the 8kHz and the 32kHz oscillator. SYSOSC is
					gated off, so PCLK and HCLK are inactive. The CPU state is not maintained.
					Only the RTC can operate in BACKUP mode. The AoD and RAM Retention can optionally be set to automatically disable (and
					clear) themselves when entering this mode. Data retention in this mode is maintained using VCORE and/or VRTC. The type of
					data retained is dependent upon whether only one, or both, of these voltages are enabled.
					Optionally, VCORE can be gated off and the internal retention regulator enabled, allowing the device to be powered only by
					VRTC. Enabling VCORE will wake the device to ACTIVE mode.
					The amount of RAM memory retained is dependent upon which voltages are enabled.
					If only VRTC is enabled, up to 96KBytes of SRAM can be retained.
					If both VRTC and VCORE are enabled, up to 1024KBytes SRAM can be retained.
					BACKUP mode supports the same wakeup sources as DEEPSLEEP mode.
				*/
#if 0 /* Raw from datasheet */
				MXC_GCR->pmr = ((MXC_GCR->pmr & ~(MXC_F_GCR_PMR_MODE)) | MXC_S_GCR_PMR_MODE_BACKUP);
#else /* Turns out there is a framework driver for Backup mode */
				MXC_LP_EnterBackupMode();
#endif
				break;
		} // End of switch

		// Check if needing to revert the power savings (if monitoring then the ISR will handle this operation)
		if (g_powerSavingsForSleepEnabled == YES)
		{
			RevertPowerSavingsAfterSleeping();
		}
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestExternalSamplingSource(void)
{
	g_updateCounter = 1;
	uint16 sampleRate = 1024;

	StartExternalRtcClock(sampleRate);

	while (1==1)
	{
		g_updateCounter++;
	
		if (g_updateCounter % sampleRate == 0)
		{
			debug("Tick tock (%d, %d)\r\n", sampleRate, g_updateCounter / sampleRate);
		}
	
		// Delay while UART transmitting or FIFO not full
		while ((MXC_UART2->stat & MXC_F_UART_STAT_TX_BUSY) && !(MXC_UART2->stat & MXC_F_UART_STAT_TX_FULL)) {;}

		//Sleep
		PowerManager();
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static char s_errorReportFilename[] = LOGS_PATH EXCEPTION_REPORT_FILE;
void CheckExceptionReportLogExists(void)
{
	FILINFO fno;

	if (f_stat((const TCHAR*)s_errorReportFilename, &fno) == FR_OK)
	{
		sprintf((char*)g_spareBuffer, "%s. %s", getLangText(EXCEPTION_REPORT_EXISTS_DUE_TO_ERROR_TEXT), getLangText(PLEASE_CONTACT_SUPPORT_TEXT));
		MessageBox(getLangText(WARNING_TEXT), (char*)g_spareBuffer, MB_OK);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
__attribute__((__interrupt__))
void exception(uint32_t r12, uint32_t r11, uint32_t r10, uint32_t r9, uint32_t exception_number, uint32_t lr, uint32_t r7, uint32_t r6, uint32_t r5, uint32_t r4,
				uint32_t r3, uint32_t r2, uint32_t r1, uint32_t r0, uint32_t sp, uint32_t sr, uint32_t pc, uint32_t stack0, uint32_t stack1, uint32_t stack2)
{
	char exceptionText[30];
	char exceptionMessage[75];
	int LFST = -1;
	int LTT = -1;
	int LMT = -1;
	int LCPT = -1;
	int exceptionReportFile;
	uint16 loops = 5;
#if 1 /* temp */
	UNUSED(exceptionReportFile);
#endif

	// Adjust Stack pointer to pre-exception value (PC and SR are 4 bytes each)
	sp += 8;

	UNUSED(r12); UNUSED(r11); UNUSED(r10); UNUSED(r9); UNUSED(lr); UNUSED(r7); UNUSED(r6); UNUSED(r5); UNUSED(r4); UNUSED(r3); UNUSED(r2); UNUSED(r1); UNUSED(r0);
	UNUSED(sp); UNUSED(sr); UNUSED(stack0); UNUSED(stack1); UNUSED(stack2);

	switch (((exception_number * 4) - 4))
	{
#if 0 /* old hw */
		case EVBA_UNRECOVERABLE: strcpy((char*)&exceptionText, "Unrecoverable"); debugRaw("Unrecoverable"); break;
		case EVBA_TLB_MULTIPLE:	strcpy((char*)&exceptionText, "TLB Multiple Hit"); debugRaw("TLB Multiple Hit"); break;
		case EVBA_BUS_ERROR_DATA: strcpy((char*)&exceptionText, "Bus Error Data Fetch"); debugRaw("Bus Error Data Fetch"); break;
		case EVBA_BUS_ERROR_INSTR: strcpy((char*)&exceptionText, "Bus Error instruction Fetch"); debugRaw("Bus Error instruction Fetch"); break;
		case EVBA_NMI: strcpy((char*)&exceptionText, "NMI"); debugRaw("NMI"); break;
		case EVBA_INSTR_ADDR: strcpy((char*)&exceptionText, "Instruction Address"); debugRaw("Instruction Address"); break;
		case EVBA_ITLB_PROT: strcpy((char*)&exceptionText, "ITLB Protection"); debugRaw("ITLB Protection"); break;
		case EVBA_BREAKPOINT: strcpy((char*)&exceptionText, "Breakpoint"); debugRaw("Breakpoint"); break;
		case EVBA_ILLEGAL_OPCODE: strcpy((char*)&exceptionText, "Illegal Opcode"); debugRaw("Illegal Opcode"); break;
		case EVBA_UNIMPLEMENTED: strcpy((char*)&exceptionText, "Unimplemented Instruction"); debugRaw("Unimplemented Instruction"); break;
		case EVBA_PRIVILEGE_VIOL: strcpy((char*)&exceptionText, "Privilege Violation"); debugRaw("Privilege Violation"); break;
		case EVBA_FLOATING_POINT: strcpy((char*)&exceptionText, "Floating Point"); debugRaw("Floating Point"); break;
		case EVBA_COP_ABSENT: strcpy((char*)&exceptionText, "Coprocessor Absent"); debugRaw("Coprocessor Absent"); break;
		case EVBA_DATA_ADDR_R: strcpy((char*)&exceptionText, "Data Address (Read)"); debugRaw("Data Address (Read)"); break;
		case EVBA_DATA_ADDR_W: strcpy((char*)&exceptionText, "Data Address (Write)"); debugRaw("Data Address (Write)"); break;
		case EVBA_DTLB_PROT_R: strcpy((char*)&exceptionText, "DLTB Protection (Read)"); debugRaw("DLTB Protection (Read)"); break;
		case EVBA_DTLB_PROT_W: strcpy((char*)&exceptionText, "DTLB Protection (Write)"); debugRaw("DTLB Protection (Write)"); break;
		case EVBA_DTLB_MODIFIED: strcpy((char*)&exceptionText, "DTLB Modified"); debugRaw("DTLB Modified"); break;
		case EVBA_ITLB_MISS: strcpy((char*)&exceptionText, "ITLB Miss"); debugRaw("ITLB Miss"); break;
		case EVBA_DTLB_MISS_R: strcpy((char*)&exceptionText, "DTLB Miss (Read)"); debugRaw("DTLB Miss (Read)"); break;
		case EVBA_DTLB_MISS_W: strcpy((char*)&exceptionText, "DTLB Miss (Write)"); debugRaw("DTLB Miss (Write)"); break;
		case EVBA_SCALL: strcpy((char*)&exceptionText, "Scall"); debugRaw("Scall"); break;
#endif
		default: strcpy((char*)&exceptionText, "Unknown EVBA offset"); debugErr("Unknown EVBA offset"); break;
	}

	if (g_lifetimeHalfSecondTickCount >= g_testTimeSinceLastFSWrite) { LFST = (g_lifetimeHalfSecondTickCount - g_testTimeSinceLastFSWrite); }
	if (g_lifetimeHalfSecondTickCount >= g_testTimeSinceLastTrigger) { LTT = (g_lifetimeHalfSecondTickCount - g_testTimeSinceLastTrigger); }
	if (g_lifetimeHalfSecondTickCount >= g_testTimeSinceLastCycleChange) { LMT = (g_lifetimeHalfSecondTickCount - g_testTimeSinceLastCycleChange); }
	if (g_lifetimeHalfSecondTickCount >= g_testTimeSinceLastCalPulse) { LCPT = (g_lifetimeHalfSecondTickCount - g_testTimeSinceLastCalPulse); }

	// Check if the LCD Power was turned off
	if (g_lcdPowerFlag == DISABLED)
	{
		g_lcdPowerFlag = ENABLED;
		ft81x_init(); // Power up display and init driver
	}

	// Check if the LCD Backlight was turned off (now treating BACKLIGHT_SUPER_LOW as disabled since the LCD really can't be seen with the backlight on at some level)
	if (g_lcdBacklightFlag == DISABLED)
	{
		g_lcdBacklightFlag = ENABLED;
		SetLcdBacklightState(BACKLIGHT_MID);
	}

#if 0 /* old hw */
	exceptionReportFile = open(s_errorReportFilename, O_APPEND);

	// Check if Exception report file doesn't exist
	if (exceptionReportFile == -1)
	{
		// Create Exception report file for the first time
		nav_setcwd(s_errorReportFilename, TRUE, TRUE);
		exceptionReportFile = open(s_errorReportFilename, O_APPEND);
	}

	// Verify file ID is valid
	if (exceptionReportFile != -1)
	{
		//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
		sprintf((char*)g_spareBuffer, "===== Exception Report =====\r\n");
		write(exceptionReportFile, g_spareBuffer, strlen((char*)g_spareBuffer));
		//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
		if (((exception_number * 4) - 4) == EVBA_BREAKPOINT)
		{
			switch (g_breakpointCause)
			{
				case BP_INT_MEM_CORRUPTED: sprintf((char*)&exceptionMessage, "BREAKPOINT: INTERNAL MEM TAMPERED"); break;
				case BP_SOFT_LOOP: sprintf((char*)&exceptionMessage, "BREAKPOINT: NON ISR FOREVER LOOP DETECTED"); break;
				case BP_MB_LOOP: sprintf((char*)&exceptionMessage, "BREAKPOINT: MESSAGE BOX FOREVER LOOP DETECTED"); break;
				case BP_UNHANDLED_INT: sprintf((char*)&exceptionMessage, "BREAKPOINT: UNHANDLED INTERRUPT DETECTED"); break;
				case BP_AD_CHAN_SYNC_ERR: sprintf((char*)&exceptionMessage, "BREAKPOINT: A/D CHAN SYNC ERROR"); break;
				default: sprintf((char*)&exceptionMessage, "BREAKPOINT: CAUSE UNKNOWN");
			}

			sprintf((char*)g_spareBuffer, "%s: %s\r\n", (char*)&g_buildVersion, (char*)&exceptionMessage);
			write(exceptionReportFile, g_spareBuffer, strlen((char*)g_spareBuffer));

			if (g_breakpointCause == BP_MB_LOOP)
			{
				sprintf((char*)g_spareBuffer, "%s: MSG TEXT: %s\r\n", "EXC SCREEN 1", (char*)g_debugBuffer);
				write(exceptionReportFile, g_spareBuffer, strlen((char*)g_spareBuffer));
			}
		}
		else // Non-Breakpoint exception
		{
			sprintf((char*)g_spareBuffer, "%s: EXC: %s at PC:0x%lx\r\n", (char*)&g_buildVersion, (char*)exceptionText, pc);
			write(exceptionReportFile, g_spareBuffer, strlen((char*)g_spareBuffer));
		}
		//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
		sprintf((char*)g_spareBuffer, "%s: SP:0x%lx S0:0x%lx S1:0x%lx S2:0x%lx\r\n", "EXC SCREEN 2", sp, stack0, stack1, stack2);
		write(exceptionReportFile, g_spareBuffer, strlen((char*)g_spareBuffer));
		//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
		sprintf((char*)g_spareBuffer, "%s: SR:0x%lx LR:0x%lx EXC:0x%lx\r\n", "EXC SCREEN 3", sr, lr, exception_number);
		write(exceptionReportFile, g_spareBuffer, strlen((char*)g_spareBuffer));
		//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
		sprintf((char*)g_spareBuffer, "%s: R0:0x%lx R1:0x%lx R2:0x%lx R3:0x%lx\r\n", "EXC SCREEN 4", r0, r1, r2, r3);
		write(exceptionReportFile, g_spareBuffer, strlen((char*)g_spareBuffer));
		//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
		sprintf((char*)g_spareBuffer, "%s: R4:0x%lx R5:0x%lx R6:0x%lx R7:0x%lx\r\n", "EXC SCREEN 5", r4, r5, r6, r7);
		write(exceptionReportFile, g_spareBuffer, strlen((char*)g_spareBuffer));
		//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
		sprintf((char*)g_spareBuffer, "%s: R9:0x%lx R10:0x%lx R11:0x%lx R12:0x%lx\r\n", "EXC SCREEN 6", r9, r10, r11, r12);
		write(exceptionReportFile, g_spareBuffer, strlen((char*)g_spareBuffer));
		//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
		sprintf((char*)g_spareBuffer, "%s: SR:%lu FS:%d T:%d CP:%d M:%d\r\n", "EXC SCREEN 7", g_lifetimeHalfSecondTickCount, LFST, LTT, LCPT, LMT);
		write(exceptionReportFile, g_spareBuffer, strlen((char*)g_spareBuffer));
		//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
		FillInAdditionalExceptionReportInfo(exceptionReportFile);
		SetFileDateTimestamp(FS_DATE_LAST_WRITE);
		close(exceptionReportFile);

		AddOnOffLogTimestamp(OFF_EXCEPTION);
	}

	// Make sure all open files are closed and data is flushed
	nav_exit();
#endif

	while (loops--)
	{
#if 0 /* old hw */
		if (((exception_number * 4) - 4) == EVBA_BREAKPOINT)
		{
			switch (g_breakpointCause)
			{
				case BP_INT_MEM_CORRUPTED: sprintf((char*)&exceptionMessage, "BREAKPOINT: INTERNAL MEM TAMPERED"); break;
				case BP_SOFT_LOOP: sprintf((char*)&exceptionMessage, "BREAKPOINT: NON ISR FOREVER LOOP DETECTED"); break;
				case BP_MB_LOOP: sprintf((char*)&exceptionMessage, "BREAKPOINT: MESSAGE BOX FOREVER LOOP DETECTED"); break;
				case BP_UNHANDLED_INT: sprintf((char*)&exceptionMessage, "BREAKPOINT: UNHANDLED INTERRUPT DETECTED"); break;
				case BP_AD_CHAN_SYNC_ERR: sprintf((char*)&exceptionMessage, "BREAKPOINT: A/D CHAN SYNC ERROR"); break;
				default: sprintf((char*)&exceptionMessage, "BREAKPOINT: CAUSE UNKNOWN");
			}
			OverlayMessage((char*)&g_buildVersion, (char*)&exceptionMessage, (EXCEPTION_HANDLING_USE_SOFT_DELAY_KEY | EXCEPTION_MSG_DISPLAY_TIME));

			if (g_breakpointCause == BP_MB_LOOP)
			{
				sprintf((char*)&exceptionMessage, "MSG TEXT: %s", (char*)g_debugBuffer);
				OverlayMessage("EXC SCREEN 1", (char*)&exceptionMessage, (EXCEPTION_HANDLING_USE_SOFT_DELAY_KEY | EXCEPTION_MSG_DISPLAY_TIME));
			}
		}
		else
		{
			sprintf((char*)&exceptionMessage, "EXC: %s at PC:0x%lx", (char*)exceptionText, pc);
			OverlayMessage((char*)&g_buildVersion, (char*)&exceptionMessage, (EXCEPTION_HANDLING_USE_SOFT_DELAY_KEY | EXCEPTION_MSG_DISPLAY_TIME));
		}
#endif
		//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
		sprintf((char*)&exceptionMessage, "SP:0x%lx S0:0x%lx S1:0x%lx S2:0x%lx", sp, stack0, stack1, stack2);
		OverlayMessage("EXC SCREEN 2", (char*)&exceptionMessage, (EXCEPTION_HANDLING_USE_SOFT_DELAY_KEY | EXCEPTION_MSG_DISPLAY_TIME));
		//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
		sprintf((char*)&exceptionMessage, "SR:0x%lx LR:0x%lx EXC:0x%lx", sr, lr, exception_number);
		OverlayMessage("EXC SCREEN 3", (char*)&exceptionMessage, (EXCEPTION_HANDLING_USE_SOFT_DELAY_KEY | EXCEPTION_MSG_DISPLAY_TIME));
		//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
		sprintf((char*)&exceptionMessage, "R0:0x%lx R1:0x%lx R2:0x%lx R3:0x%lx", r0, r1, r2, r3);
		OverlayMessage("EXC SCREEN 4", (char*)&exceptionMessage, (EXCEPTION_HANDLING_USE_SOFT_DELAY_KEY | EXCEPTION_MSG_DISPLAY_TIME));
		//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
		sprintf((char*)&exceptionMessage, "R4:0x%lx R5:0x%lx R6:0x%lx R7:0x%lx", r4, r5, r6, r7);
		OverlayMessage("EXC SCREEN 5", (char*)&exceptionMessage, (EXCEPTION_HANDLING_USE_SOFT_DELAY_KEY | EXCEPTION_MSG_DISPLAY_TIME));
		//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
		sprintf((char*)&exceptionMessage, "R9:0x%lx R10:0x%lx R11:0x%lx R12:0x%lx", r9, r10, r11, r12);
		OverlayMessage("EXC SCREEN 6", (char*)&exceptionMessage, (EXCEPTION_HANDLING_USE_SOFT_DELAY_KEY | EXCEPTION_MSG_DISPLAY_TIME));
		//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
		sprintf((char*)&exceptionMessage, "SR:%lu FS:%d T:%d CP:%d M:%d", g_lifetimeHalfSecondTickCount, LFST, LTT, LCPT, LMT);
		OverlayMessage("EXC SCREEN 7", (char*)&exceptionMessage, (EXCEPTION_HANDLING_USE_SOFT_DELAY_KEY | EXCEPTION_MSG_DISPLAY_TIME));
		//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
	}

	OverlayMessage(getLangText(STATUS_TEXT), getLangText(POWERING_UNIT_OFF_NOW_TEXT), 0);

	if (g_unitConfig.timerMode != ENABLED)
	{
		SetTimeOfDayAlarmNearFuture(2);
	}

	// Shutdown application
	PowerControl(MCU_POWER_LATCH, OFF);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void EnableGlobalException(void)
{
#if 0 /* old hw */
	// Import the Exception Vector Base Address.
	extern void _evba;

	Set_system_register(AVR32_EVBA, (int)&_evba);

	// Enable exceptions.
	Enable_global_exception();
#endif
}

#include "mxc_errors.h"
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestInternalADCPowerMonitor(void)
{
	uint16_t adcVal;
	uint8_t overflow;

    /* Initialize ADC */
    if (MXC_ADC_Init() != E_NO_ERROR) { debugErr("Internal ADC: Init failed\r\n"); }
    while (MXC_ADC->status & (MXC_F_ADC_STATUS_ACTIVE | MXC_F_ADC_STATUS_PWR_UP_ACTIVE)) {}

    MXC_ADC_SetMonitorChannel(MXC_ADC_MONITOR_0, MXC_ADC_CH_VCORE);
    MXC_ADC_EnableMonitor(MXC_ADC_MONITOR_0);

	MXC_ADC_StartConversion(MXC_ADC_CH_VCORE);

	overflow = (MXC_ADC_GetData(&adcVal) == E_OVERFLOW ? 1 : 0);

	debug("Internal ADC: Vcore voltage is 0x%04x%s\n\n", adcVal, overflow ? " (overflow)" : "");

	MXC_ADC_Shutdown();
}

#include "tpu.h"
#include "mxc_errors.h"
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestHardwareCRC(void)
{
	uint16_t i;
	uint32_t softwareCrc;
	uint32_t hardwareCrc;

	for (i=0; i<256; i++)
	{
		g_eventDataBuffer[i] = rand();
	}

	softwareCrc = hardwareCrc = SEED_32;
    if (MXC_TPU_CRC((uint8_t*)g_eventDataBuffer, 256, MXC_TPU_CRC32_ETHERNET, &hardwareCrc) != E_SUCCESS) { debugErr("Test CRC: CRC-32 failed \r\n"); }
	softwareCrc = CalcCCITT32((uint8_t*)g_eventDataBuffer, 256, softwareCrc);
	if (softwareCrc == hardwareCrc) { debug("Test CRC: CRC-32/CCITT validation passed, TPU hardware CRC good\r\n"); }
	else { debugErr("Test CRC: CRC-32/CCITT validation failed, 0x%x != 0x%x\r\n", softwareCrc, hardwareCrc); }

	softwareCrc = hardwareCrc = SEED_32;
    if (MXC_TPU_CRC((uint8_t*)g_eventDataBuffer, 256, MXC_TPU_CRC_CCITT, &hardwareCrc) != E_SUCCESS) { debugErr("Test CRC: CRC-16-CCITT failed \r\n"); }
	softwareCrc = CalcCrc16((uint8_t*)g_eventDataBuffer, 256, softwareCrc);
	if ((uint16_t)softwareCrc == (uint16_t)hardwareCrc) { debug("Test CRC: CRC-16-CCITT validation passed, TPU hardware CRC good\r\n"); }
	else { debugErr("Test CRC: CRC-16-CCITT validation failed, 0x%x != 0x%x\r\n", (uint16_t)softwareCrc, (uint16_t)hardwareCrc); }

	softwareCrc = hardwareCrc = SEED_32;
    if (MXC_TPU_CRC((uint8_t*)g_eventDataBuffer, 256, MXC_TPU_CRC16, &hardwareCrc) != E_SUCCESS) { debugErr("Test CRC: CRC-16 failed \r\n"); }
	softwareCrc = CalcCrc16((uint8_t*)g_eventDataBuffer, 256, softwareCrc);
	if ((uint16_t)softwareCrc == (uint16_t)hardwareCrc) { debug("Test CRC: CRC-16 validation passed, TPU hardware CRC good\r\n"); }
	else { debugErr("Test CRC: CRC-16 validation failed, 0x%x != 0x%x\r\n", (uint16_t)softwareCrc, (uint16_t)hardwareCrc); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestExternalDeviceAccessAndComms(void)
{
	//Device list: Acc, 1-Wire, Batt Charger, EEPROM, USB-C Port Controller, External RTC, Fuel Gauge, Expansion I2C bridge, eMMC + FF driver, External ADC, LCD
	debug("External Device Access and Comms testing...\r\n");

	TestAccelerometer();
	Test1Wire();
	TestBatteryCharger();
	TestEEPROM();
	TestUSBCPortController();
	TestExternalRTC();
	TestFuelGauge();
	TestExpansionI2CBridge();
	TestEMMCFatFilesystem();
	TestExternalADC();
	TestLCD();
	TestHardwareCRC();
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int main(void)
{
	// Initialize the system
	InitSystemHardware_MS9300();
	InitInterrupts_MS9300();
	InitSoftwareSettings_MS9300();
	EnableGlobalException();

#if 0 /* Test */
extern void SetupUSBComposite(void);
	SetupUSBComposite();
#endif

#if 0 /* Test */
	TestExternalDeviceAccessAndComms();
#endif

#if 0 /* Hardware test phase */
	// End execution here for now until hardware passes testing
	while (1) {}
#else /* Normal operation */
 	// ==============
	// Executive loop
	// ==============
	while (1)
	{
		// Handle system events
		SystemEventManager();

		// Handle menu events
		MenuEventManager();

		// Handle craft processing
		CraftManager();

		// Handle GPS processing
		GpsManager();

		// Handle messages to be processed
		MessageManager();

		// Handle USB device
		//UsbDeviceManager();
extern void UsbReportEvents(void);
		UsbReportEvents();
		
		// Handle processing the factory setup
		FactorySetupManager();

		// Check if able to go to sleep
#if 1 /* Can't run PowerManager while using JTAG on Alpha boards with SBU */
		PowerManager();
#endif
		// Count Exec cycles
		g_execCycles++;

#if 0 /* Test */
		if (strlen((char*)g_blmBuffer)) { debug("%s", (char*)g_blmBuffer); memset(g_blmBuffer, 0, sizeof(g_blmBuffer)); }
#endif

#if 0 /* Todo: Re-enable in init, disabled for testing */
		//Reset watchdog
		MXC_WDT_ResetTimer(MXC_WDT0);
#endif
	}
	// End of NS9300 Main
#endif

	// End of the world
	return (0);
}
