///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include <stdlib.h>
#include "Typedefs.h"
#include "Common.h"
#include "Record.h"
#include "Menu.h"
#include "InitDataBuffers.h"
#include "ProcessBargraph.h"
#include "SysEvents.h"
#include "OldUart.h"
#include "Keypad.h"
#include "RealTimeClock.h"
#include "SoftTimer.h"
#include "RemoteHandler.h"
#include "PowerManagement.h"
#include "Analog.h"
#include "gpio.h"
//#include "intc.h"
#include "SysEvents.h"
#include "rtc.h"
//#include "tc.h"
//#include "twi.h"
#include "spi.h"
//#include "usart.h"
#include "string.h"
//#include "navigation.h"
//#include "cycle_counter.h"
//#include "compiler.h"
#include "math.h"

#include "tmr.h"
#include "mxc_delay.h"
#include "ff.h"
#include "wdt.h"
#include "usb.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define DUMMY_READ(x) if ((volatile int)x == 0) {}

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
#include "Globals.h"

extern BOOLEAN processCraftCmd;
extern uint8 craft_g_input_buffer[];

///----------------------------------------------------------------------------
///	Local Scope Globals
///----------------------------------------------------------------------------
// Flags and variables to statically defined to prevent creating and clearing stack variables every ISR data process call
static uint16 s_R_channelReading, s_V_channelReading, s_T_channelReading, s_A_channelReading;
#if 0 /* temp removed while unused */
static uint16 s_channelConfigReadBack;
#endif
static uint16* s_pretrigPtr;
static uint16* s_samplePtr;
static uint16* s_calPtr[3] = {NULL, NULL, NULL};
static uint32 s_pretrigCount = 0;
static uint32 s_sampleCount = 0;
static uint32 s_calSampleCount = 0;
static uint32 s_pendingCalCount = 0;
static uint32 s_pretriggerCount = 0;
static uint32 s_alarmOneCount = 0;
static uint32 s_alarmTwoCount = 0;

static uint16* s_variablePretriggerBuffPtr;
static uint16* s_variableEventPretriggerBuffPtr;

static uint16 s_sampleRate = 1024;
static uint16 s_consecSeismicTriggerCount = 0;
static uint16 s_consecAirTriggerCount = 0;
static uint16 s_sensorCalSampleCount = CALIBRATION_FIXED_SAMPLE_RATE;
static int16 s_temperatureDelta = 0;

static uint8 s_pretriggerFull = NO;
static uint8 s_checkForTempDrift = NO;
static uint8 s_seismicTriggerSample = NO;
static uint8 s_airTriggerSample = NO;
static uint8 s_recordingEvent = NO;
static uint8 s_calPulse = NO;
static uint8 s_consecEventsWithoutCal = 0;
static uint8 s_channelSyncError = NO;
static uint8 s_channelSyncErrorCount = 0;
static uint8 s_consecutiveEventsWithoutCalThreshold = CONSEC_EVENTS_WITHOUT_CAL_THRESHOLD;
#if 0 /* Old */
static uint8 s_channelReadsToSync = 0;
#endif

static SAMPLE_DATA_STRUCT s_tempSensorCalPeaks;
static SAMPLE_DATA_STRUCT s_tempSensorCalFreqCounts;
static SAMPLE_DATA_STRUCT s_tempSensorCalFreqSign;
static SAMPLE_DATA_STRUCT s_tempSensorCalFreqCounter;
static int32 s_tempChanMin[MAX_NUM_OF_CHANNELS];
static int32 s_tempChanMax[MAX_NUM_OF_CHANNELS];
static int32 s_tempChanAvg[MAX_NUM_OF_CHANNELS];
static uint32 s_tempChanMed[MAX_NUM_OF_CHANNELS][8];

static VARIABLE_TRIGGER_FREQ_CALC_BUFFER s_variableTriggerFreqCalcBuffer;
static float s_vtDiv;
#if 0 /* Needed for Full wave count algorithm (temp disabled) */
static VARIABLE_TRIGGER_FREQ_CHANNEL_BUFFER* s_workingVTChanData;
#endif

#if 0 /* temp removed while unused */
static uint32 s_fractionSecondMarker;
#endif

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------
__attribute__((__interrupt__))
void Sample_irq(void);
__attribute__((__interrupt__))
void Tc_typematic_irq(void);

#if EXTERNAL_SAMPLING_SOURCE
__attribute__((__interrupt__))
void Tc_ms_timer_irq(void);
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void NMI_Handler(void)
{
	debugErr("Exception: Non-Maskable Interrupt\r\n");

	while (1) {}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HardFault_Handler(void)
{
	debugErr("Exception: Hard Fault\r\n");

	while (1) {}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void MemManage_Handler(void)
{
	debugErr("Exception: Memory Management Fault\r\n");

	while (1) {}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void BusFault_Handler(void)
{
	debugErr("Exception: Bus Fault\r\n");

	while (1) {}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UsageFault_Handler(void)
{
	debugErr("Exception: Usage Fault\r\n");

	while (1) {}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SVC_Handler(void)
{
	debugErr("Exception: Supervisor Call\r\n");

	while (1) {}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DebugMon_Handler(void)
{
	debugErr("Exception: Debug Monitor\r\n");

	while (1) {}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void PF_IRQHandler(void)
{
	debugErr("Power Fail IRQ triggered\r\n");

	while (1) {}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void WDT0_IRQHandler(void)
{
    MXC_WDT_ClearIntFlag(MXC_WDT0);

    debugErr("Watchdog IRQ triggered\r\n");
	//debugErr("Watchdog IRQ triggered, attempting to gracefully close shop before reset...\r\n");

	// Shutdown/data handling before reset
	while (1) {}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
__attribute__((__interrupt__))
void External_battery_presence_irq(void)
{
	//debugRaw("=");
#if /* New board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN)
	debugWarn("-(ISR) Battery pack status alert (Slot 1:%s, Slot 2:%s)-\r\n", ((GPIO_EXT_BATTERY_PRESENCE_1_PORT->in & GPIO_EXT_BATTERY_PRESENCE_1_PIN) ? "Added" : "Removed"), ((GPIO_EXT_BATTERY_PRESENCE_2_PORT->in & GPIO_EXT_BATTERY_PRESENCE_2_PIN) ? "Added" : "Removed"));
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
__attribute__((__interrupt__))
void Fuel_gauge_alert_irq(void)
{
	//debugRaw("'");
	debugWarn("-(ISR) FG alert-\r\n");

	// Flag for special method to clear the alert response (interrupt) to allow the ALCC pin to continue to function
	// Todo: create FG flag for carrying out operation

	// Clear Fuel Gauge Alert interrupt flag (Port 0, Pin 4)
	GPIO_GAUGE_ALERT_PORT->int_clr = GPIO_GAUGE_ALERT_PIN;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t batteryChargerInterruptActive = NO;
__attribute__((__interrupt__))
void Battery_charger_irq(void)
{
	//debugRaw("+");
	debugWarn("-(ISR) Batt Charger-\r\n");

	batteryChargerInterruptActive = YES;

	// Clear Battery Charger interrupt flag (Port 0, Pin 5)
	GPIO_BATTERY_CHARGER_IRQ_PORT->int_clr = GPIO_BATTERY_CHARGER_IRQ_PIN;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if /* New board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN)
__attribute__((__interrupt__))
void Sensor_detect_1_irq(void)
{
	//debugRaw("+");
	debugWarn("-(ISR) Sensor Detect 1: %s-\r\n", ((MXC_GPIO_OutGet(GPIO_SENSOR_DETECT_1_PORT, GPIO_SENSOR_DETECT_1_PIN) == 0) ? "Removed" : "Added"));

	// Clear Sensor Detect 1 flag (Port 0, Pin 7)
#if /* New board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN)
	GPIO_SENSOR_DETECT_1_PORT->int_clr = GPIO_SENSOR_DETECT_1_PIN;
#endif
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if /* New board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN)
__attribute__((__interrupt__))
void Sensor_detect_2_irq(void)
{
	//debugRaw("+");
	debugWarn("-(ISR) Sensor Detect 2: %s -\r\n", ((MXC_GPIO_OutGet(GPIO_SENSOR_DETECT_2_PORT, GPIO_SENSOR_DETECT_2_PIN) == 0) ? "Removed" : "Added"));

	// Clear Sensor Detect 2 flag (Port 1, Pin 2)
#if /* New board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN)
	GPIO_SENSOR_DETECT_2_PORT->int_clr = GPIO_SENSOR_DETECT_2_PIN;
#endif
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if /* New board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN)
__attribute__((__interrupt__))
void Sensor_detect_3_irq(void)
{
	//debugRaw("+");
	debugWarn("-(ISR) Sensor Detect 3: %s -\r\n", ((MXC_GPIO_OutGet(GPIO_SENSOR_DETECT_3_PORT, GPIO_SENSOR_DETECT_3_PIN) == 0) ? "Removed" : "Added"));

	// Clear Sensor Detect 3 flag (Port 1, Pin 13)
#if /* New board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN)
	GPIO_SENSOR_DETECT_3_PORT->int_clr = GPIO_SENSOR_DETECT_3_PIN;
#endif
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if /* New board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN)
__attribute__((__interrupt__))
void Sensor_detect_4_irq(void)
{
	//debugRaw("+");
	debugWarn("-(ISR) Sensor Detect 4: %s -\r\n", ((MXC_GPIO_OutGet(GPIO_SENSOR_DETECT_1_PORT, GPIO_SENSOR_DETECT_1_PIN) == 0) ? "Removed" : "Added"));

	// Clear Sensor Detect 4 flag (Port 1, Pin 27)
#if /* New board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN)
	GPIO_SENSOR_DETECT_4_PORT->int_clr = GPIO_SENSOR_DETECT_4_PIN;
#endif
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t g_expansionIrqActive = 0;
__attribute__((__interrupt__))
void Expansion_irq(void)
{
	//debugRaw("_");
	//debugWarn("-(ISR) Expansion-\r\n");

#if 1 /* Flag */
	g_expansionIrqActive = 1;
#else
extern uint8_t ExpansionBridgeReadLSRStatus(void);
	uint8_t lcrStatus = ExpansionBridgeReadLSRStatus();

	if (lcrStatus & 0x01)
	{
		//debugRaw("<E-rc>");

extern uint8_t Expansion_UART_ReadCharacter(void);
		uint8_t recieveData = Expansion_UART_ReadCharacter();

		// Raise the Craft Data flag
		g_modemStatus.craftPortRcvFlag = YES;

		// Write the received data into the buffer
		*g_isrMessageBufferPtr->writePtr = recieveData;

		// Advance the buffer pointer
		g_isrMessageBufferPtr->writePtr++;

		// Check if buffer pointer goes beyond the end
		if (g_isrMessageBufferPtr->writePtr >= (g_isrMessageBufferPtr->msg + CMD_BUFFER_SIZE))
		{
			// Reset the buffer pointer to the beginning of the buffer
			g_isrMessageBufferPtr->writePtr = g_isrMessageBufferPtr->msg;
		}
	}
#endif

	// Clear Expansion interrupt flag (Port 0, Pin 8)
	GPIO_EXPANSION_IRQ_PORT->int_clr = GPIO_EXPANSION_IRQ_PIN;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t usbIsrActive = NO;
__attribute__((__interrupt__))
void Usbc_port_controller_i2c_irq(void)
{
	//debugRaw("/");
	debugWarn("-(ISR) USB I2C-\r\n");

	usbIsrActive = YES;

	// Clear USBC I2C interrupt flag (Port 1, Pin 11)
	GPIO_USBC_PORT_CONTROLLER_I2C_IRQ_PORT->int_clr = GPIO_USBC_PORT_CONTROLLER_I2C_IRQ_PIN;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
__attribute__((__interrupt__))
void Power_good_battery_charger_irq(void)
{
	//debugRaw("_");
	debugWarn("-(ISR) PG BC-\r\n");

	// Clear PG BC interrupt flag (Port 0, Pin 12)
	GPIO_POWER_GOOD_BATTERY_CHARGE_PORT->int_clr = GPIO_POWER_GOOD_BATTERY_CHARGE_PIN;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
volatile uint8_t testAccInt = 0;
__attribute__((__interrupt__))
void Accelerometer_irq(void)
{
	//debugRaw("<");
	//debugWarn("-(ISR) Acc IRQ 1-\r\n");

	testAccInt = 1;

	// Clear Accelerometer interrupt flag 1 (Port 1, Pin 12)
	GPIO_ACCEL_INT_1_PORT->int_clr = GPIO_ACCEL_INT_1_PIN;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
__attribute__((__interrupt__))
void External_rtc_irq(void)
{
	//debugRaw("`");
	debugWarn("-(ISR) Ext RTC-\r\n");

	// Clear External RTC interrupt flag (Port 1, Pin 28)
	GPIO_EXT_RTC_INTA_PORT->int_clr = GPIO_EXT_RTC_INTA_PIN;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
__attribute__((__interrupt__))
void Lcd_irq(void)
{
	//debugRaw(":");
	debugRaw("-LCD IRQ-\r\n");

	// Clear LCD interrupt flag (Port 2, Pin 6)
	GPIO_LCD_INT_PORT->int_clr = GPIO_LCD_INT_PIN;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
__attribute__((__interrupt__))
void Eic_low_battery_irq(void)
{
	debugRaw("-LowBatt-");

	raiseSystemEventFlag_ISR(LOW_BATTERY_WARNING_EVENT);

	// Clear the interrupt flag in the processor
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
__attribute__((__interrupt__))
void Gps_status_irq(void)
{
	if (g_epochTimeGPS)
	{
		g_epochTimeGPS++;
	}

	// Clear the interrupt flag in the processor
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
__attribute__((__interrupt__))
void Keypad_irq(void)
{
	// Print test for verification of operation
	//debugRaw("^");

	// Read the keymap
	g_kpadIsrKeymap = READ_KEY_BUTTON_MAP;

#if 0 /* Test */
	// KB_SK_4, KB_SK_3, KB_SK_2, KB_SK_1, KB_ENTER, KB_RIGHT, KB_LEFT, KB_DOWN, KB_UP
	char keyName[25]; sprintf(keyName, "None");
	if (g_kpadIsrKeymap & 0x0001) { sprintf(keyName, "Soft Key 4"); }
	else if (g_kpadIsrKeymap & 0x0002) { sprintf(keyName, "Soft Key 3"); }
	else if (g_kpadIsrKeymap & 0x0004) { sprintf(keyName, "Soft Key 2"); }
	else if (g_kpadIsrKeymap & 0x0008) { sprintf(keyName, "Soft Key 1"); }
	else if (g_kpadIsrKeymap & 0x0010) { sprintf(keyName, "Enter"); }
	else if (g_kpadIsrKeymap & 0x0020) { sprintf(keyName, "Right"); }
	else if (g_kpadIsrKeymap & 0x0040) { sprintf(keyName, "Left"); }
	else if (g_kpadIsrKeymap & 0x0080) { sprintf(keyName, "Down"); }
	else if (g_kpadIsrKeymap & 0x0100) { sprintf(keyName, "Up"); }
	debugWarn("-(ISR) Key found: %s- (0x%x)\r\n", keyName, g_kpadIsrKeymap);
#endif

	if (g_kpadProcessingFlag == DEACTIVATED)
	{
		raiseSystemEventFlag_ISR(KEYPAD_EVENT);
	}
	else
	{
		g_kpadInterruptWhileProcessing = YES;
	}

	// Clear the interrupt flags for all button (1-9) GPIO lines
	MXC_GPIO_ClearFlags(REGULAR_BUTTONS_GPIO_PORT, REGULAR_BUTTONS_GPIO_MASK);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* Test */
uint8_t g_powerButtonAction = 0;
#endif
__attribute__((__interrupt__))
void System_power_button_irq(void)
{
	static uint8 onKeyCount = 0;
	uint16 onKeyFlag;

	// Print test for verification of operation
	//debugRaw("&");
	debugRaw("<P>");

	onKeyFlag = ((GPIO_POWER_BUTTON_IRQ_PORT->in) & GPIO_POWER_BUTTON_IRQ_PIN); // Power button interrupt on GPIO port 1, pin 15
	//keyScan = READ_KEY_BUTTON_MAP; // If needed to check for Combo keys

	//-----------------------------------------------------------------------------------
	// Check if the On key was pressed
	if (onKeyFlag)
	{
		// Flag system to monitor power on button for turning unit off
		g_powerOffAttempted = YES;

		// Flag to potentially start factory setup unlock
		if (g_factorySetupSequence != PROCESS_FACTORY_SETUP)
		{
			//debug("Factory Setup: Stage 1\r\n");
			g_factorySetupSequence = STAGE_1;
		}

		// Check if the power off timer is disabled
#if 0 /* Normal */
		if (IsSoftTimerActive(POWER_OFF_TIMER_NUM) == NO)
#else
		if ((IsSoftTimerActive(POWER_OFF_TIMER_NUM) == NO) || (g_activeMenu == CAL_SETUP_MENU))
#endif
		{
			// Reset count
			onKeyCount = 0;
		}
		else // Power off timer was already enabled
		{
			onKeyCount++;
		}

#if 0 /* Test multifunction actions on power button when keypad is not functional */
		if (g_sampleProcessing != ACTIVE_STATE)
		{
			// Go monitoring
			g_powerButtonAction = 1;
		}
		else
		{
			if (GetPowerControlState(LCD_POWER_ENABLE) == ON)
			{
				// Disable LCD
				g_powerButtonAction = 2;
			}
			else
			{
				// External Trigger
				g_powerButtonAction = 3;
			}
		}
#endif
		// Check if repeated On key press was detected 3 additional times (original press is unlock)
#if 1 /* Normal */
		if (onKeyCount == 3)
#else /* Test multifunction actions on power button when keypad is not functional */
		if (0)
#endif
		{
			// Gracefully fall off the ledge.. No returning from this
			debugRaw("\n--> SAFE FALL <--");

			// Handle and finish any processing
			StopMonitoring(g_triggerRecord.opMode, FINISH_PROCESSING);
			OverlayMessage(getLangText(WARNING_TEXT), getLangText(FORCED_POWER_OFF_TEXT), 0);
			AddOnOffLogTimestamp(FORCED_OFF);

			// Unmount the file system, graceful shutdown
			f_mount(NULL, "", 0);

			// Put Fuel Gauge ADC to sleep while off (device is battery powered and not placed into reset)
			Ltc2944_i2c_shutdown();

			// Disable USB
			MXC_USB_Shutdown();

			// Disable power blocks
			PowerControl(ADC_RESET, ON);
			PowerControl(LCD_POWER_ENABLE, OFF);
			PowerControl(ENABLE_12V, OFF);
			PowerControl(CELL_ENABLE, OFF);
			PowerControl(EXPANSION_ENABLE, OFF);

			SoftUsecWait(1 * SOFT_SECS);
#if 1 /* Test disabling interrupts to prevent unit powering right back on */
			__disable_irq();
#endif
			PowerControl(MCU_POWER_LATCH, OFF);
		}

		// Check if repeated On key press was detected 6 additional times (original press is unlock)
#if 1 /* Normal */
		if (onKeyCount == 6)
#else /* Test multifunction actions on power button when keypad is not functional */
		if (0)
#endif
		{
			// Jumping off the ledge.. No returning from this
			debugRaw("\n--> BOOM <--");
			PowerControl(MCU_POWER_LATCH, OFF);
		}

		// Manage monitoring the On key for powering off
		AssignSoftTimer(POWER_OFF_TIMER_NUM, (2 * TICKS_PER_SEC), PowerOffTimerCallback);
	}

	// Clear Power Button interrupt flag (Port 1, Pin 15)
	GPIO_POWER_BUTTON_IRQ_PORT->int_clr = GPIO_POWER_BUTTON_IRQ_PIN;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
__attribute__((__interrupt__))
void External_trigger_irq(void)
{
	// Clear trigger out signal in case it was self generated (Active high control)
	PowerControl(TRIGGER_OUT, OFF);

	//debugRaw("-ET-");
	//debugWarn("-(ISR) External Trigger-");

	// Check if monitoring and not bargraph and not processing an event
	if (((g_sampleProcessing == ACTIVE_STATE)) && (g_triggerRecord.opMode != BARGRAPH_MODE) && (g_busyProcessingEvent == NO))
	{
		if (g_unitConfig.externalTrigger == ENABLED)
		{
			// Signal the start of an event
			g_externalTrigger = EXTERNAL_TRIGGER_EVENT;
		}
	}

	// Clear External Trigger In interrupt flag (Port 1, Pin 31)
	GPIO_EXTERNAL_TRIGGER_IN_PORT->int_clr = GPIO_EXTERNAL_TRIGGER_IN_PIN;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint32_t s_sampleCountHold = 0;
__attribute__((__interrupt__))
void Internal_rtc_alarms(void)
{
	// Test print to verify the interrupt is running
	debugRaw("~");

    int flags = MXC_RTC_GetFlags();

    if (flags & MXC_F_RTC_CTRL_SSEC_ALARM_FL)
	{
		// Based on Internal RTC sub-second alarm, but generate interrupts in Deepsleep and Backup

		// Increment the lifetime soft timer tick count
		g_lifetimeHalfSecondTickCount++;

		// Every tick raise the flag to check soft timers
		raiseTimerEventFlag(SOFT_TIMER_CHECK_EVENT);

		// Every 8 ticks (4 secs) trigger the cyclic event flag
		if (++g_cyclicEventDelay == CYCLIC_EVENT_TIME_THRESHOLD)
		{
			g_cyclicEventDelay = 0;
			raiseSystemEventFlag_ISR(CYCLIC_EVENT);
#if 1 /* Test */
			g_sampleCountHold = g_sampleCount - s_sampleCountHold;
			s_sampleCountHold = g_sampleCount;
#endif
		}

		// Every so often flag for updating to the External RTC time.
		if (++g_rtcTickCountSinceLastExternalUpdate >= UPDATE_TIME_EVENT_THRESHOLD)
		{
			raiseSystemEventFlag_ISR(UPDATE_TIME_EVENT);
		}

        MXC_RTC_ClearFlags(MXC_F_RTC_CTRL_SSEC_ALARM_FL);
    }

    if (flags & MXC_F_RTC_CTRL_TOD_ALARM_FL)
	{
        MXC_RTC_ClearFlags(MXC_F_RTC_CTRL_TOD_ALARM_FL);
    }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 1 /* Test */
volatile uint8_t hsChange = 0;
#endif
__attribute__((__interrupt__))
void Soft_timer_tick_irq(void)
{
	// Based on Internal PIT timer, but will not generate interrupts in Deepsleep or Backup

	// Test print to verify the interrupt is running
	//debugRaw("`");

	// Increment the lifetime soft timer tick count
	g_lifetimeHalfSecondTickCount++;

#if 1 /* Test */
	hsChange = 1;
#endif

	// Every tick raise the flag to check soft timers
	raiseTimerEventFlag(SOFT_TIMER_CHECK_EVENT);

	// Every 8 ticks (4 secs) trigger the cyclic event flag
	if (++g_cyclicEventDelay == CYCLIC_EVENT_TIME_THRESHOLD)
	{
		g_cyclicEventDelay = 0;
		raiseSystemEventFlag_ISR(CYCLIC_EVENT);
#if 1 /* Test */
		g_sampleCountHold = g_sampleCount - s_sampleCountHold;
		s_sampleCountHold = g_sampleCount;
#endif
	}

	// Every so often flag for updating to the External RTC time.
	if (++g_rtcTickCountSinceLastExternalUpdate >= UPDATE_TIME_EVENT_THRESHOLD)
	{
		raiseSystemEventFlag_ISR(UPDATE_TIME_EVENT);
	}

	CYCLIC_HALF_SEC_TIMER_NUM->intr = MXC_F_TMR_INTR_IRQ;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 1 /* Test */
volatile uint8_t psChange = 0;
volatile uint32_t g_lifetimePeriodicSecondCount = 0;
#endif
__attribute__((__interrupt__))
void External_rtc_periodic_timer(void)
{
	// Based on Internal PIT timer, but will not generate interrupts in Deepsleep or Backup

	// Test print to verify the interrupt is running
	//debugRaw("$");

	// Increment the lifetime soft timer tick count
	g_lifetimePeriodicSecondCount++;

#if 1 /* Test */
	psChange = 1;
#endif

	MXC_GPIO_ClearFlags(GPIO_EXT_RTC_INTA_PORT, GPIO_EXT_RTC_INTA_PIN);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
__attribute__((__interrupt__))
void Tc_typematic_irq(void)
{
	// Test print to verify the interrupt is running
	//if ((g_keypadTimerTicks % 1000) == 0) {debugRaw("&"); }

	// Increment the ms seconds counter
	g_keypadTimerTicks++;

	// Clear the interrupt flag
	TYPEMATIC_TIMER_NUM->intr = MXC_F_TMR_INTR_IRQ;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if EXTERNAL_SAMPLING_SOURCE
__attribute__((__interrupt__))
void Tc_ms_timer_irq(void)
{
	// Test print to verify the interrupt is running
	//if ((g_msTimerTicks % 1000) == 0) { debugRaw("$"); }

	// Increment the ms seconds counter
	g_msTimerTicks++;

	// Clear the interrupt flag
	MILLISECOND_TIMER_NUM->intr = MXC_F_TMR_INTR_IRQ;
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static inline void fillPretriggerBufferUntilFull_ISR_Inline(void)
{
	s_pretriggerCount++;

	// Check if the Pretrigger count has accumulated the full number of samples (variable Pretrigger size)
	if (s_pretriggerCount >= (g_triggerRecord.trec.sample_rate / g_unitConfig.pretrigBufferDivider))
	{ 
		s_pretriggerFull = YES;
		s_pretriggerCount = 0;
		
		// Save the current temperature
		g_storedTempReading = g_previousTempReading = g_currentTempReading;

		// Check if setup to monitor temp drift and not 16K sample rate
		if ((g_triggerRecord.trec.adjustForTempDrift == YES) && (g_triggerRecord.trec.sample_rate != SAMPLE_RATE_16K))
		{
			s_checkForTempDrift = YES;
		}
	}					
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static inline void normalizeSampleData_ISR_Inline(void)
{
	if (s_R_channelReading < ACCURACY_16_BIT_MIDPOINT) { s_R_channelReading = ACCURACY_16_BIT_MIDPOINT - s_R_channelReading; }
	else { s_R_channelReading -= ACCURACY_16_BIT_MIDPOINT; }

	if (s_V_channelReading < ACCURACY_16_BIT_MIDPOINT) { s_V_channelReading = ACCURACY_16_BIT_MIDPOINT - s_V_channelReading; }
	else { s_V_channelReading -= ACCURACY_16_BIT_MIDPOINT; }

	if (s_T_channelReading < ACCURACY_16_BIT_MIDPOINT) { s_T_channelReading = ACCURACY_16_BIT_MIDPOINT - s_T_channelReading; }
	else { s_T_channelReading -= ACCURACY_16_BIT_MIDPOINT; }

	if (s_A_channelReading < ACCURACY_16_BIT_MIDPOINT) { s_A_channelReading = ACCURACY_16_BIT_MIDPOINT - s_A_channelReading; }
	else { s_A_channelReading -= ACCURACY_16_BIT_MIDPOINT; }
}
	
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static inline void checkAlarms_ISR_Inline(void)
{
	// Check if Alarm 1 mode is enabled
	if (g_unitConfig.alarmOneMode)
	{
		// Check if Alarm 1 is active
		if (s_alarmOneCount)
		{
			// Decrement and check if Alarm 1 active time period is complete
			if (--s_alarmOneCount == 0)
			{
				// Clear Alarm 1
				PowerControl(ALARM_1_ENABLE, OFF);
			}
		}
		// Check for Alarm 1 condition only if not processing a calibration pulse
		else if (s_calPulse != YES)
		{
			// Check if seismic is enabled for Alarm 1 (bitwise operation)
			if (g_unitConfig.alarmOneMode & ALARM_MODE_SEISMIC)
			{
				if ((s_R_channelReading > g_unitConfig.alarmOneSeismicLevel) || (s_V_channelReading > g_unitConfig.alarmOneSeismicLevel) ||
					(s_T_channelReading > g_unitConfig.alarmOneSeismicLevel))
				{
					// Start Alarm 1
					PowerControl(ALARM_1_ENABLE, ON);

					// Set Alarm 1 count to time in seconds multiplied by sample rate
					s_alarmOneCount = (uint32)(g_unitConfig.alarmOneTime * s_sampleRate);

					g_blmAlertAlarmStatus |= ALERT_ALARM_ONE;
				}
			}

			// Check if air is enabled for Alarm 1 (bitwise operation)
			if (g_unitConfig.alarmOneMode & ALARM_MODE_AIR)
			{
				if (s_A_channelReading > g_unitConfig.alarmOneAirLevel)
				{
					// Start Alarm 1
					PowerControl(ALARM_1_ENABLE, ON);

					// Set Alarm 1 count to time in seconds multiplied by sample rate
					s_alarmOneCount = (uint32)(g_unitConfig.alarmOneTime * s_sampleRate);

					g_blmAlertAlarmStatus |= ALERT_ALARM_ONE;
				}
			}
		}
	}
						
	// Check if Alarm 2 mode is enabled
	if (g_unitConfig.alarmTwoMode)
	{
		// Check if Alarm 2 is active
		if (s_alarmTwoCount)
		{
			// Decrement and check if Alarm 1 active time period is complete
			if (--s_alarmTwoCount == 0)
			{
				// Clear Alarm 2
				PowerControl(ALARM_2_ENABLE, OFF);
			}
		}
		// Check for Alarm 2 condition only if not processing a calibration pulse
		else if (s_calPulse != YES)
		{
			// Check if seismic is enabled for Alarm 2 (bitwise operation)
			if (g_unitConfig.alarmTwoMode & ALARM_MODE_SEISMIC)
			{
				if ((s_R_channelReading > g_unitConfig.alarmTwoSeismicLevel) || (s_V_channelReading > g_unitConfig.alarmTwoSeismicLevel) ||
					(s_T_channelReading > g_unitConfig.alarmTwoSeismicLevel))
				{
					// Start Alarm 2
					PowerControl(ALARM_2_ENABLE, ON);

					// Set Alarm 2 count to time in seconds multiplied by sample rate
					s_alarmTwoCount = (uint32)(g_unitConfig.alarmTwoTime * s_sampleRate);

					g_blmAlertAlarmStatus |= ALERT_ALARM_TWO;
				}
			}

			// Check if air is enabled for Alarm 2 (bitwise operation)
			if (g_unitConfig.alarmTwoMode & ALARM_MODE_AIR)
			{
				if (s_A_channelReading > g_unitConfig.alarmTwoAirLevel)
				{
					// Start Alarm 2
					PowerControl(ALARM_2_ENABLE, ON);

					// Set Alarm 2 count to time in seconds multiplied by sample rate
					s_alarmTwoCount = (uint32)(g_unitConfig.alarmTwoTime * s_sampleRate);

					g_blmAlertAlarmStatus |= ALERT_ALARM_TWO;
				}
			}
		}
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static inline uint8 usbmAndOsmFirstSlope_ISR_Inline(float freq, uint16 peak)
{
	// Check if peak in counts is greater than the freq conversion to PPV turned into counts
	// PPV = 2 * PI * Freq * Fixed Disp equaling 0.03
	if (peak > (s_vtDiv * freq * 0.188495)) { return (YES); }

	return (NO);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static inline uint8 usbmSecondSlope_ISR_Inline(float freq, uint16 peak)
{
	// Check if peak in counts is greater than the freq conversion to PPV turned into counts
	// PPV = 2 * PI * Freq * Fixed Disp equaling 0.008
	if (peak > (s_vtDiv * freq * 0.050265)) { return (YES); }

	return (NO);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static inline uint8 osmSecondSlope_ISR_Inline(float freq, uint16 peak)
{
	// Check if peak in counts is greater than the freq conversion to PPV turned into counts
	// PPV = 2 * PI * Freq * Fixed Disp equaling 0.0107
	if (peak > (s_vtDiv * freq * 0.067230)) { return (YES); }

	return (NO);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void checkVariableTriggerAndFreq(VARIABLE_TRIGGER_FREQ_CHANNEL_BUFFER* chan)
{
/*
	==================================================
	=== Actual equations provided by Ken E. at OSM
	==================================================
	--- USBM Drywall ---
	1 Hz - 3.979 Hz (1st curve, D = 0.03)
	3.979 Hz - 14.921 Hz (flat 0.75 IPS)
	14.921 Hz - 39.789 Hz (2nd curve, D = 0.008)
	39.789+ Hz (flat 2.00 IPS)

	--- USBM Plaster ---
	1 Hz - 2.653 Hz (1st curve, D = 0.03)
	2.653 Hz - 9.947 Hz (flat 0.50 IPS)
	9.947 Hz - 39.789 Hz (2nd curve, D = 0.008)
	39.789+ Hz (flat 2.00 IPS)

	--- OSM Standard ---
	1 Hz - 3.979 Hz (1st curve, D = 0.03)
	3.979 Hz - 11.156 Hz (flat 0.75 IPS)
	11.156 Hz - 29.749 Hz (2nd curve, D = 0.0107)
	29.749+ Hz (flat 2.00 IPS)

	=========================
	=== Custom Threshold Step
	=========================
	0.10 ips below 10 Hz
	0.25 ips below 20 Hz
	0.5 ips below 30 Hz
	0.75 ips below 40 Hz
	1.0 ips over 40 Hz

	========================
	=== Custom Limiting Step
	========================
	0.15 ips below 10 Hz
	0.5 ips below 20 Hz
	1.0 ips below 30 Hz
	1.5 ips below 40 Hz
	2.0 ips over 40 Hz
*/

	// Make sure the frequency count is non-zero
	if (chan->freq_count == 0) { return; }

	float freq = (float)(g_triggerRecord.trec.sample_rate) / (float)(chan->freq_count);
	float peak = (float)(chan->peak) / s_vtDiv;
	// New ability to trigger off of a fractional level below the threshold
	peak *= (float)((float)g_triggerRecord.trec.variableTriggerPercentageLevel / (float)100);
	uint8 triggerFound = NO;

	// Filter out any frequencies lower than 1 Hz (which is below the specs of the transducer)
	if (freq < 1.0) { return; }

	// Check the frequency band for the specific vibration standard
	if (g_triggerRecord.trec.variableTriggerVibrationStandard == USBM_RI_8507_DRYWALL_STANDARD)
	{
		if (freq < 3.979) { if (peak > (freq * 0.188495)) { triggerFound = YES; } } // USBM Drywall 1st slopt, PPV = 2 * PI * Freq * 0.03 (Fixed Disp)
		else if (freq < 14.921) { if (peak > 0.75) { triggerFound = YES; } }
		else if (freq < 39.789) { if (peak > (freq * 0.050265)) { triggerFound = YES; } } // USBM Drywall 2nd slope, PPV = 2 * PI * Freq * 0.008 (Fixed Disp)
	}
	else if (g_triggerRecord.trec.variableTriggerVibrationStandard == USBM_RI_8507_PLASTER_STANDARD)
	{
		if (freq < 2.653) { if (peak > (freq * 0.188495)) { triggerFound = YES; } } // USBM Plaster 1st slope, PPV = 2 * PI * Freq * 0.03 (Fixed Disp)
		else if (freq < 9.947) { if (peak > 0.50) { triggerFound = YES; } }
		else if (freq < 39.789) { if (peak > (freq * 0.050265)) { triggerFound = YES; } } // USBM Plaster 2nd slope, PPV = 2 * PI * Freq * 0.008 (Fixed Disp)
	}
	else if (g_triggerRecord.trec.variableTriggerVibrationStandard == OSM_REGULATIONS_STANDARD)
	{
		if (freq < 3.979) { if (peak > (freq * 0.188495)) { triggerFound = YES; } } // OSM 1st slope, PPV = 2 * PI * Freq * 0.03 (Fixed Disp)
		else if (freq < 11.156) { if (peak > 0.75) { triggerFound = YES; } }
		else if (freq < 29.749) { if (peak > (freq * 0.067230)) { triggerFound = YES; } } // OSM 2nd slope, PPV = 2 * PI * Freq * 0.0107 (Fixed Disp)
	}
	else if (g_triggerRecord.trec.variableTriggerVibrationStandard == CUSTOM_STEP_THRESHOLD)
	{
		if (freq < 10.0) { if (peak > 0.10) { triggerFound = YES; } }
		else if (freq < 20.0) {	if (peak > 0.25) { triggerFound = YES; } }
		else if (freq < 30.0) {	if (peak > 0.50) { triggerFound = YES; } }
		else if (freq < 40.0) {	if (peak > 0.75) { triggerFound = YES; } }
	}
	else if (g_triggerRecord.trec.variableTriggerVibrationStandard == CUSTOM_STEP_LIMITING)
	{
		if (freq < 10.0) { if (peak > 0.15) { triggerFound = YES; } }
		else if (freq < 20.0) {	if (peak > 0.50) { triggerFound = YES; } }
		else if (freq < 30.0) {	if (peak > 1.00) { triggerFound = YES; } }
		else if (freq < 40.0) {	if (peak > 1.50) { triggerFound = YES; } }
	}

	if (triggerFound)
	{
		g_externalTrigger = VARIABLE_TRIGGER_EVENT;
		s_variableEventPretriggerBuffPtr = chan->peakSamplePtr;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static inline void processVariableTriggerDataHalfWave_ISR_Inline(void)
{
	// ------------
	// All Channels
	// ------------
	// Check if the freq count signals initial sample
	if (s_variableTriggerFreqCalcBuffer.r_sign == 0xFFFF)
	{
		s_variableTriggerFreqCalcBuffer.r_sign = (uint16)(((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->r & ACCURACY_16_BIT_MIDPOINT);
		s_variableTriggerFreqCalcBuffer.v_sign = (uint16)(((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->v & ACCURACY_16_BIT_MIDPOINT);
		s_variableTriggerFreqCalcBuffer.t_sign = (uint16)(((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->t & ACCURACY_16_BIT_MIDPOINT);
	}

	// ---------
	// R channel
	// ---------
	// Check if the stored sign comparison signals a zero crossing
	if (s_variableTriggerFreqCalcBuffer.r_sign ^ (((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->r & ACCURACY_16_BIT_MIDPOINT))
	{
		// Double the half wave count to get a full wave number of samples and check if a trigger is found
		s_variableTriggerFreqCalcBuffer.r[0].freq_count *= 2;
		checkVariableTriggerAndFreq(&s_variableTriggerFreqCalcBuffer.r[0]);
		if (g_externalTrigger == VARIABLE_TRIGGER_EVENT) { return; }

		// Store new sign for future zero crossing comparisons
		s_variableTriggerFreqCalcBuffer.r_sign = (uint16)(((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->r & ACCURACY_16_BIT_MIDPOINT);

		// Reset count since we crossed the zero boundary
		s_variableTriggerFreqCalcBuffer.r[0].freq_count = 1;

		// Reset peak and peak pointer to the current sample
		s_variableTriggerFreqCalcBuffer.r[0].peak = s_R_channelReading;
		s_variableTriggerFreqCalcBuffer.r[0].peakSamplePtr = g_tailOfPretriggerBuff;
	}
	else
	{
		// Check if the peak is high enough to change to the lower noise band filter otherwise check the noise band starting filter to prevent midpoint bias from bloating the frequency count
		if (((s_variableTriggerFreqCalcBuffer.r[0].peak > AD_PEAK_CHANGE_NOISE_THRESHOLD) && (s_R_channelReading > AD_NORMALIZED_NOISE_THRESHOLD)) || (s_R_channelReading > AD_NORMALIZED_NOISE_THRESHOLD_START))
		{
			// Increment count
			s_variableTriggerFreqCalcBuffer.r[0].freq_count++;
		}

		// Check if a new peak was found, and the store it
		if (s_R_channelReading > s_variableTriggerFreqCalcBuffer.r[0].peak)
		{
			s_variableTriggerFreqCalcBuffer.r[0].peak = s_R_channelReading;
			s_variableTriggerFreqCalcBuffer.r[0].peakSamplePtr = g_tailOfPretriggerBuff;
		}
	}

	// ---------
	// V channel
	// ---------
	// Check if the stored sign comparison signals a zero crossing
	if (s_variableTriggerFreqCalcBuffer.v_sign ^ (((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->v & ACCURACY_16_BIT_MIDPOINT))
	{
		// Double the half wave count to get a full wave number of samples and check if a trigger is found
		s_variableTriggerFreqCalcBuffer.v[0].freq_count *= 2;
		checkVariableTriggerAndFreq(&s_variableTriggerFreqCalcBuffer.v[0]);
		if (g_externalTrigger == VARIABLE_TRIGGER_EVENT) { return; }

		// Store new sign for future zero crossing comparisons
		s_variableTriggerFreqCalcBuffer.v_sign = (uint16)(((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->v & ACCURACY_16_BIT_MIDPOINT);

		// Reset count since we crossed the zero boundary
		s_variableTriggerFreqCalcBuffer.v[0].freq_count = 1;

		// Reset peak and peak pointer to the current sample
		s_variableTriggerFreqCalcBuffer.v[0].peak = s_V_channelReading;
		s_variableTriggerFreqCalcBuffer.v[0].peakSamplePtr = g_tailOfPretriggerBuff;
	}
	else
	{
		// Check if the peak is high enough to change to the lower noise band filter otherwise check the noise band starting filter to prevent midpoint bias from bloating the frequency count
		if (((s_variableTriggerFreqCalcBuffer.v[0].peak > AD_PEAK_CHANGE_NOISE_THRESHOLD) && (s_V_channelReading > AD_NORMALIZED_NOISE_THRESHOLD)) || (s_V_channelReading > AD_NORMALIZED_NOISE_THRESHOLD_START))
		{
			// Increment count
			s_variableTriggerFreqCalcBuffer.v[0].freq_count++;
		}

		// Check if a new peak was found, and the store it
		if (s_V_channelReading > s_variableTriggerFreqCalcBuffer.v[0].peak)
		{
			s_variableTriggerFreqCalcBuffer.v[0].peak = s_V_channelReading;
			s_variableTriggerFreqCalcBuffer.v[0].peakSamplePtr = g_tailOfPretriggerBuff;
		}
	}

	// ---------
	// T channel
	// ---------
	// Check if the stored sign comparison signals a zero crossing
	if (s_variableTriggerFreqCalcBuffer.t_sign ^ (((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->t & ACCURACY_16_BIT_MIDPOINT))
	{
		// Double the half wave count to get a full wave number of samples and check if a trigger is found
		s_variableTriggerFreqCalcBuffer.t[0].freq_count *= 2;
		checkVariableTriggerAndFreq(&s_variableTriggerFreqCalcBuffer.t[0]);
		if (g_externalTrigger == VARIABLE_TRIGGER_EVENT) { return; }

		// Store new sign for future zero crossing comparisons
		s_variableTriggerFreqCalcBuffer.t_sign = (uint16)(((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->t & ACCURACY_16_BIT_MIDPOINT);

		// Reset count since we crossed the zero boundary
		s_variableTriggerFreqCalcBuffer.t[0].freq_count = 1;

		// Reset peak and peak pointer to the current sample
		s_variableTriggerFreqCalcBuffer.t[0].peak = s_T_channelReading;
		s_variableTriggerFreqCalcBuffer.t[0].peakSamplePtr = g_tailOfPretriggerBuff;
	}
	else
	{
		// Check if the peak is high enough to change to the lower noise band filter otherwise check the noise band starting filter to prevent midpoint bias from bloating the frequency count
		if (((s_variableTriggerFreqCalcBuffer.t[0].peak > AD_PEAK_CHANGE_NOISE_THRESHOLD) && (s_T_channelReading > AD_NORMALIZED_NOISE_THRESHOLD)) || (s_T_channelReading > AD_NORMALIZED_NOISE_THRESHOLD_START))
		{
			// Increment count
			s_variableTriggerFreqCalcBuffer.t[0].freq_count++;
		}

		// Check if a new peak was found, and the store it
		if (s_T_channelReading > s_variableTriggerFreqCalcBuffer.t[0].peak)
		{
			s_variableTriggerFreqCalcBuffer.t[0].peak = s_T_channelReading;
			s_variableTriggerFreqCalcBuffer.t[0].peakSamplePtr = g_tailOfPretriggerBuff;
		}
	}
}

#if 0 /* Working but temp disabled Full wave count algorithm */
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static inline void processVariableTriggerDataFullWave_ISR_Inline(void)
{
	// ------------
	// All Channels
	// ------------
	// Check if the freq count signals initial sample
	if (s_variableTriggerFreqCalcBuffer.r_sign == 0xFFFF)
	{
		s_variableTriggerFreqCalcBuffer.r_sign = (uint16)(((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->r & ACCURACY_16_BIT_MIDPOINT);
		s_variableTriggerFreqCalcBuffer.v_sign = (uint16)(((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->v & ACCURACY_16_BIT_MIDPOINT);
		s_variableTriggerFreqCalcBuffer.t_sign = (uint16)(((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->t & ACCURACY_16_BIT_MIDPOINT);
	}

	// ------------------------
	// R channel crossover
	// ------------------------
	// Check if the stored sign comparison signals a zero crossing
	if (s_variableTriggerFreqCalcBuffer.r_sign ^ (((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->r & ACCURACY_16_BIT_MIDPOINT))
	{
		// Store new sign for future zero crossing comparisons
		s_variableTriggerFreqCalcBuffer.r_sign = (uint16)(((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->r & ACCURACY_16_BIT_MIDPOINT);

		if (s_variableTriggerFreqCalcBuffer.r_sign) { s_workingVTChanData = &s_variableTriggerFreqCalcBuffer.r[1]; }
		else { s_workingVTChanData = &s_variableTriggerFreqCalcBuffer.r[0]; }

		// Check the positive peak and frequency against the vibration curve
		checkVariableTriggerAndFreq(s_workingVTChanData);

		// Reset count since we crossed the zero boundary
		s_workingVTChanData->freq_count = 0;

		// Reset peak and peak pointer to the current sample
		s_workingVTChanData->peak = s_R_channelReading;
		s_workingVTChanData->peakSamplePtr = g_tailOfPretriggerBuff;

		// Check if a Variable Trigger event has been found and return if so
		if (g_externalTrigger == VARIABLE_TRIGGER_EVENT) { return; }
	}
	// ------------------
	// R channel peak
	// ------------------
	else
	{
		if (s_variableTriggerFreqCalcBuffer.r_sign) { s_workingVTChanData = &s_variableTriggerFreqCalcBuffer.r[1]; }
		else { s_workingVTChanData = &s_variableTriggerFreqCalcBuffer.r[0]; }

		// Check if a new peak was found, and the store it
		if (s_R_channelReading > s_workingVTChanData->peak)
		{
			s_workingVTChanData->peak = s_R_channelReading;
			s_workingVTChanData->peakSamplePtr = g_tailOfPretriggerBuff;
		}
	}

	// -----------------
	// R channel freq
	// -----------------
	// Check if the peak is high enough to change to the lower noise band filter otherwise check the noise band starting filter to prevent midpoint bias from bloating the frequency count
	if ((s_R_channelReading > AD_NORMALIZED_NOISE_THRESHOLD_START) || (s_variableTriggerFreqCalcBuffer.r[1].peak > AD_PEAK_CHANGE_NOISE_THRESHOLD))
	{
		// Increment count
		s_variableTriggerFreqCalcBuffer.r[1].freq_count++;
	}

	// Check if the peak is high enough to change to the lower noise band filter otherwise check the noise band starting filter to prevent midpoint bias from bloating the frequency count
	if ((s_R_channelReading > AD_NORMALIZED_NOISE_THRESHOLD_START) || (s_variableTriggerFreqCalcBuffer.r[0].peak > AD_PEAK_CHANGE_NOISE_THRESHOLD))
	{
		// Increment count
		s_variableTriggerFreqCalcBuffer.r[0].freq_count++;
	}

	// ------------------------
	// V channel crossover
	// ------------------------
	// Check if the stored sign comparison signals a zero crossing
	if (s_variableTriggerFreqCalcBuffer.v_sign ^ (((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->v & ACCURACY_16_BIT_MIDPOINT))
	{
		// Store new sign for future zero crossing comparisons
		s_variableTriggerFreqCalcBuffer.v_sign = (uint16)(((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->v & ACCURACY_16_BIT_MIDPOINT);

		if (s_variableTriggerFreqCalcBuffer.v_sign) { s_workingVTChanData = &s_variableTriggerFreqCalcBuffer.v[1]; }
		else { s_workingVTChanData = &s_variableTriggerFreqCalcBuffer.v[0]; }

		// Check the positive peak and frequency against the vibration curve
		checkVariableTriggerAndFreq(s_workingVTChanData);

		// Reset count since we crossed the zero boundary
		s_workingVTChanData->freq_count = 0;

		// Reset peak and peak pointer to the current sample
		s_workingVTChanData->peak = s_V_channelReading;
		s_workingVTChanData->peakSamplePtr = g_tailOfPretriggerBuff;

		// Check if a Variable Trigger event has been found and return if so
		if (g_externalTrigger == VARIABLE_TRIGGER_EVENT) { return; }
	}
	// ------------------
	// V channel peak
	// ------------------
	else
	{
		if (s_variableTriggerFreqCalcBuffer.v_sign) { s_workingVTChanData = &s_variableTriggerFreqCalcBuffer.v[1]; }
		else { s_workingVTChanData = &s_variableTriggerFreqCalcBuffer.v[0]; }

		// Check if a new peak was found, and the store it
		if (s_V_channelReading > s_workingVTChanData->peak)
		{
			s_workingVTChanData->peak = s_V_channelReading;
			s_workingVTChanData->peakSamplePtr = g_tailOfPretriggerBuff;
		}
	}

	// -----------------
	// V channel freq
	// -----------------
	// Check if the peak is high enough to change to the lower noise band filter otherwise check the noise band starting filter to prevent midpoint bias from bloating the frequency count
	if ((s_V_channelReading > AD_NORMALIZED_NOISE_THRESHOLD_START) || (s_variableTriggerFreqCalcBuffer.v[1].peak > AD_PEAK_CHANGE_NOISE_THRESHOLD))
	{
		// Increment count
		s_variableTriggerFreqCalcBuffer.v[1].freq_count++;
	}

	// Check if the peak is high enough to change to the lower noise band filter otherwise check the noise band starting filter to prevent midpoint bias from bloating the frequency count
	if ((s_V_channelReading > AD_NORMALIZED_NOISE_THRESHOLD_START) || (s_variableTriggerFreqCalcBuffer.v[0].peak > AD_PEAK_CHANGE_NOISE_THRESHOLD))
	{
		// Increment count
		s_variableTriggerFreqCalcBuffer.v[0].freq_count++;
	}

	// ------------------------
	// T channel crossover
	// ------------------------
	// Check if the stored sign comparison signals a zero crossing
	if (s_variableTriggerFreqCalcBuffer.t_sign ^ (((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->t & ACCURACY_16_BIT_MIDPOINT))
	{
		// Store new sign for future zero crossing comparisons
		s_variableTriggerFreqCalcBuffer.t_sign = (uint16)(((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->t & ACCURACY_16_BIT_MIDPOINT);

		if (s_variableTriggerFreqCalcBuffer.t_sign) { s_workingVTChanData = &s_variableTriggerFreqCalcBuffer.t[1]; }
		else { s_workingVTChanData = &s_variableTriggerFreqCalcBuffer.t[0]; }

		// Check the positive peak and frequency against the vibration curve
		checkVariableTriggerAndFreq(s_workingVTChanData);

		// Reset count since we crossed the zero boundary
		s_workingVTChanData->freq_count = 0;

		// Reset peak and peak pointer to the current sample
		s_workingVTChanData->peak = s_T_channelReading;
		s_workingVTChanData->peakSamplePtr = g_tailOfPretriggerBuff;

		// Check if a Variable Trigger event has been found and return if so
		if (g_externalTrigger == VARIABLE_TRIGGER_EVENT) { return; }
	}
	// ------------------
	// T channel peak
	// ------------------
	else
	{
		if (s_variableTriggerFreqCalcBuffer.t_sign) { s_workingVTChanData = &s_variableTriggerFreqCalcBuffer.t[1]; }
		else { s_workingVTChanData = &s_variableTriggerFreqCalcBuffer.t[0]; }

		// Check if a new peak was found, and the store it
		if (s_T_channelReading > s_workingVTChanData->peak)
		{
			s_workingVTChanData->peak = s_T_channelReading;
			s_workingVTChanData->peakSamplePtr = g_tailOfPretriggerBuff;
		}
	}

	// -----------------
	// T channel freq
	// -----------------
	// Check if the peak is high enough to change to the lower noise band filter otherwise check the noise band starting filter to prevent midpoint bias from bloating the frequency count
	if ((s_T_channelReading > AD_NORMALIZED_NOISE_THRESHOLD_START) || (s_variableTriggerFreqCalcBuffer.t[1].peak > AD_PEAK_CHANGE_NOISE_THRESHOLD))
	{
		// Increment count
		s_variableTriggerFreqCalcBuffer.t[1].freq_count++;
	}

	// Check if the peak is high enough to change to the lower noise band filter otherwise check the noise band starting filter to prevent midpoint bias from bloating the frequency count
	if ((s_T_channelReading > AD_NORMALIZED_NOISE_THRESHOLD_START) || (s_variableTriggerFreqCalcBuffer.t[0].peak > AD_PEAK_CHANGE_NOISE_THRESHOLD))
	{
		// Increment count
		s_variableTriggerFreqCalcBuffer.t[0].freq_count++;
	}
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static inline void processAndMoveManualCalData_ISR_Inline(void)
{
	// Wait 5 samples to start cal signaling (~5 ms)
	if (g_manualCalSampleCount == CAL_SAMPLE_COUNT_FIRST_TRANSITION_HIGH) { AdSetCalSignalHigh(); }	// (5 ms after start, run for 10 ms)
	if (g_manualCalSampleCount == CAL_SAMPLE_COUNT_SECOND_TRANSITION_LOW) { AdSetCalSignalLow(); }	// (15 ms after start, run for 20 ms)
	if (g_manualCalSampleCount == CAL_SAMPLE_COUNT_THIRD_TRANSITION_HIGH) { AdSetCalSignalHigh(); }	// (35 ms after start, run for 10 ms)
	if (g_manualCalSampleCount == CAL_SAMPLE_COUNT_FOURTH_TRANSITION_OFF) { AdSetCalSignalOff(); }	// (45 ms after start, run for 55 ms)

	// Check if the first time through
	if (g_manualCalSampleCount == MAX_CAL_SAMPLES)
	{
		// Setup the sample pointer to the start of the event buffer
		s_samplePtr = g_startOfEventBufferPtr;
	}

	// Move the samples into the event buffer
	*(SAMPLE_DATA_STRUCT*)s_samplePtr = *(SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff;

	s_samplePtr += NUMBER_OF_CHANNELS_DEFAULT;

	g_manualCalSampleCount--;

	// Check if done with the Manual Cal pulse
	if (g_manualCalSampleCount == 0)
	{
		// Signal the end of the Cal pulse
		raiseSystemEventFlag_ISR(MANUAL_CAL_EVENT);

		// Signal an event buffer has been used (gimmick to make processing work)
		g_freeEventBuffers--;

		g_manualCalFlag = FALSE;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static inline void processAndMoveWaveformData_ISR_Inline(void)
{
	//_____________________________________________________________________________________
	//___Three main stages to processing:
	//___1) Look for a trigger, either for the first time or within the window to process a consecutive event which postpones a cal pulse
	//___2) Capture event data since a trigger has been found
	//___3) Process the cal pulse to accompany the captured event data

	//_____________________________________________________________________________________
	//___Check if not recording _and_ no cal pulse, or if pending cal, and only if there are free event buffers
	if (((((s_recordingEvent == NO) && (s_calPulse == NO)) || (s_pendingCalCount))) && (g_freeEventBuffers))
	{
		//_____________________________________________________________________________________
		//___Check for triggers
		if (g_triggerRecord.trec.seismicTriggerLevel != NO_TRIGGER_CHAR)
		{
			if (s_R_channelReading > g_triggerRecord.trec.seismicTriggerLevel) { s_seismicTriggerSample = YES; }
			else if (s_V_channelReading > g_triggerRecord.trec.seismicTriggerLevel) { s_seismicTriggerSample = YES; }
			else if (s_T_channelReading > g_triggerRecord.trec.seismicTriggerLevel) { s_seismicTriggerSample = YES; }
					
			if (s_seismicTriggerSample == YES) { s_consecSeismicTriggerCount++; s_seismicTriggerSample = NO; }
			else {s_consecSeismicTriggerCount = 0; }
		}

		if (g_triggerRecord.trec.airTriggerLevel != NO_TRIGGER_CHAR)
		{
			if (s_A_channelReading > g_triggerRecord.trec.airTriggerLevel) { s_airTriggerSample = YES; }
						
			if (s_airTriggerSample == YES) { s_consecAirTriggerCount++; s_airTriggerSample = NO; }
			else { s_consecAirTriggerCount = 0; }
		}				

#if (!VT_FEATURE_DISABLED) // New VT feature
		if (g_triggerRecord.trec.variableTriggerEnable == YES)
		{
			processVariableTriggerDataHalfWave_ISR_Inline();
		}
#endif
		//___________________________________________________________________________________________
		//___Check if either a seismic or acoustic trigger threshold condition was achieved or an external trigger was found
		if ((s_consecSeismicTriggerCount == CONSECUTIVE_TRIGGERS_THRESHOLD) || (s_consecAirTriggerCount == CONSECUTIVE_TRIGGERS_THRESHOLD) || (g_externalTrigger))
		{
#if 0 /* Test */
			if (g_externalTrigger == EXTERNAL_TRIGGER_EVENT) { sprintf((char*)g_blmBuffer, "--> (ISR) External Trigger Found (ET: %d)\r\n", g_externalTrigger); } else
			sprintf((char*)g_blmBuffer, "(ISR) --> %s Trigger Found, %x %x %x (%x), %x (%x)\r\n", (g_externalTrigger == VARIABLE_TRIGGER_EVENT) ? "VT" : "Normal", s_R_channelReading, s_V_channelReading, s_T_channelReading,
					g_triggerRecord.trec.seismicTriggerLevel, s_A_channelReading, g_triggerRecord.trec.airTriggerLevel);
#endif
			//usart_write_char(&AVR32_USART1, '$');
			g_testTimeSinceLastTrigger = g_lifetimeHalfSecondTickCount;
			
#if (!VT_FEATURE_DISABLED) // New VT feature
			if (g_triggerRecord.trec.variableTriggerEnable == YES) // Variable trigger
			{
				uint32 pretriggerBufferSize;

				// Check if the trigger was above the fixed 2.0 IPS limit for variable trigger (skipping the frequency algorithm)
				if (g_externalTrigger != VARIABLE_TRIGGER_EVENT)
				{
					// Set variable trigger event pointer to the trigger sample in the pretrigger buffer
					s_variableEventPretriggerBuffPtr = g_tailOfPretriggerBuff;
				}

				// Calculate number of pretrigger samples (in words)
				pretriggerBufferSize = ((uint32)(g_triggerRecord.trec.sample_rate / g_unitConfig.pretrigBufferDivider) * g_sensorInfo.numOfChannels);

				// Check if the pretrigger start point wraps back in the pretrigger circular buffer
				if ((s_variableEventPretriggerBuffPtr - pretriggerBufferSize) < g_startOfPretriggerBuff)
				{
					// Calculate the wrapped pretrigger start location
					s_variablePretriggerBuffPtr = (g_endOfPretriggerBuff - (g_startOfPretriggerBuff - (s_variableEventPretriggerBuffPtr - pretriggerBufferSize)));

				}
				else // Pretrigger start does not wrap in the pretrigger circular buffer
				{
					s_variablePretriggerBuffPtr = (s_variableEventPretriggerBuffPtr - pretriggerBufferSize);
				}

				// Reset working freq calc buffer so that old results are not processed post event
				memset(&s_variableTriggerFreqCalcBuffer, 0, sizeof(s_variableTriggerFreqCalcBuffer));
				s_variableTriggerFreqCalcBuffer.r_sign = 0xFFFF;
			}
#endif

			// Check if this event was triggered by an external trigger signal
			if (g_externalTrigger == EXTERNAL_TRIGGER_EVENT)
			{
				// Flag as an External Trigger for handling the event
				raiseSystemEventFlag_ISR(EXT_TRIGGER_EVENT);

				// Reset the external trigger flag
				g_externalTrigger = NO;
			}
			// The trigger source was local (not an external trigger)
			else // ((g_externalTrigger == NO) || (g_externalTrigger == VARIABLE_TRIGGER_EVENT))
			{
				//debugRaw("+ET+");

				// Signal a trigger found for any external unit connected (Active high control)
				PowerControl(TRIGGER_OUT, ON);
					
				// Check if variable trigger event was the source and reset the flag
				if (g_externalTrigger) { g_externalTrigger = NO; }

				// Trigger out will be cleared when our own ISR catches the active high signal
			}

			s_consecSeismicTriggerCount = 0;
			s_consecAirTriggerCount = 0;

#if 1 /* New Adaptive sampling */
			// Check if Adaptive sampling is enabled, easiest to accomplish by checking adaptive state
			if (g_adaptiveState != ADAPTIVE_DISABLED)
			{
				g_adaptiveState = ADAPTIVE_MAX_RATE;
				g_adaptiveSampleDelay = ((g_triggerRecord.trec.record_time * g_triggerRecord.trec.sample_rate) + (g_triggerRecord.trec.sample_rate / 4) + 100); // Covers length of event + cal + 1/4 second
			}
#endif

#if 0 /* Need better management of check */
			// Don't worry about temp drift during an event
			s_checkForTempDrift = NO;
#endif

			//___________________________________________________________________________________________
			//___Check if there is an event buffer available and not marked done for taking events (stop trigger)
			if ((g_freeEventBuffers != 0) && (g_doneTakingEvents == NO))
			{
				//___________________________________________________________________________________________
				//__Save date and timestamp of new trigger
				g_eventDateTimeStampBuffer[g_eventBufferWriteIndex].triggerTime = GetCurrentTime();
#if 0 /* Re-enable if GPS time resource becomes available */
				if ((/* GPS time resource acitve */) && (g_epochTimeGPS))
				{
					g_eventDateTimeStampBuffer[g_eventBufferWriteIndex].gpsEpochTriggerTime = g_epochTimeGPS;
					g_eventDateTimeStampBuffer[g_eventBufferWriteIndex].gpsFractionalSecond = (/* CPU cycle count */ - s_fractionSecondMarker);
				}
#endif
				//___________________________________________________________________________________________
				//__Setup new event buffer pointers, counts, and flags
				s_pretrigPtr = g_startOfEventBufferPtr + (g_eventBufferWriteIndex * g_wordSizeInEvent);
				s_samplePtr = s_pretrigPtr + g_wordSizeInPretrig;

				s_pretrigCount = g_samplesInPretrig;
				s_sampleCount = g_samplesInBody;

				// Check if a cal pulse was already pending
				if (s_calPulse == PENDING)
				{
					// An event has already been captured without a cal, so inc the consecutive count
					s_consecEventsWithoutCal++;
				}

				// Set the cal pointer in the array corresponding to the appropriate event cal section buffer section
				s_calPtr[s_consecEventsWithoutCal] = s_pretrigPtr + g_wordSizeInPretrig + g_wordSizeInBody;
									
				s_recordingEvent = YES;
				s_calPulse = PENDING;
				
				// Global flag to signal handling an event
				g_busyProcessingEvent = YES;
				
				// Clear count (also needs to remain zero until event recording is done to prevent reentry into this 'if' section)
				s_pendingCalCount = 0;

#if VT_FEATURE_DISABLED
				//___________________________________________________________________________________________
				//___Copy current Pretrigger buffer sample to event
				*(SAMPLE_DATA_STRUCT*)s_samplePtr = *(SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff;

				//___________________________________________________________________________________________
				//___Copy oldest Pretrigger buffer sample to Pretrigger
				if ((g_tailOfPretriggerBuff + NUMBER_OF_CHANNELS_DEFAULT) >= g_endOfPretriggerBuff)
				{
					// Copy first (which is currently the oldest) Pretrigger buffer sample to Pretrigger buffer
					*(SAMPLE_DATA_STRUCT*)s_pretrigPtr = *(SAMPLE_DATA_STRUCT*)g_startOfPretriggerBuff;
				}
				else // Copy oldest Pretrigger buffer sample to Pretrigger
				{
					*(SAMPLE_DATA_STRUCT*)s_pretrigPtr = *(SAMPLE_DATA_STRUCT*)(g_tailOfPretriggerBuff + NUMBER_OF_CHANNELS_DEFAULT);
				}

#else
				// Check if using the standard trigger
				if (g_triggerRecord.trec.variableTriggerEnable != YES)
				{
					//___________________________________________________________________________________________
					//___Copy current Pretrigger buffer sample to event
					*(SAMPLE_DATA_STRUCT*)s_samplePtr = *(SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff;

					//___________________________________________________________________________________________
					//___Copy oldest Pretrigger buffer sample to Pretrigger
					if ((g_tailOfPretriggerBuff + NUMBER_OF_CHANNELS_DEFAULT) >= g_endOfPretriggerBuff)
					{
						// Copy first (which is currently the oldest) Pretrigger buffer sample to Pretrigger buffer
						*(SAMPLE_DATA_STRUCT*)s_pretrigPtr = *(SAMPLE_DATA_STRUCT*)g_startOfPretriggerBuff;
					}
					else // Copy oldest Pretrigger buffer sample to Pretrigger
					{
						*(SAMPLE_DATA_STRUCT*)s_pretrigPtr = *(SAMPLE_DATA_STRUCT*)(g_tailOfPretriggerBuff + NUMBER_OF_CHANNELS_DEFAULT);
					}
				}
				else // Variable trigger standard (USBM, OSM)
				{
					//___________________________________________________________________________________________
					//___Copy current Pretrigger buffer sample to event
					*(SAMPLE_DATA_STRUCT*)s_samplePtr = *(SAMPLE_DATA_STRUCT*)s_variableEventPretriggerBuffPtr;

					//___________________________________________________________________________________________
					//___Copy oldest Pretrigger buffer sample to Pretrigger
					*(SAMPLE_DATA_STRUCT*)s_pretrigPtr = *(SAMPLE_DATA_STRUCT*)s_variablePretriggerBuffPtr;

					// Increment both variable pretrigger pointers
					s_variableEventPretriggerBuffPtr += NUMBER_OF_CHANNELS_DEFAULT;
					s_variablePretriggerBuffPtr += NUMBER_OF_CHANNELS_DEFAULT;

					// Check if variable pretrigger pointers wrapped and adjust accordingly
					if (s_variableEventPretriggerBuffPtr >= g_endOfPretriggerBuff) { s_variableEventPretriggerBuffPtr = g_startOfPretriggerBuff; }
					if (s_variablePretriggerBuffPtr >= g_endOfPretriggerBuff) { s_variablePretriggerBuffPtr = g_startOfPretriggerBuff; }
				}
#endif
				//___________________________________________________________________________________________
				//___Advance data pointers and decrement counts
				s_samplePtr += NUMBER_OF_CHANNELS_DEFAULT;
				s_sampleCount--;
									
				s_pretrigPtr += NUMBER_OF_CHANNELS_DEFAULT;
				s_pretrigCount--;
			}
		}
		//___________________________________________________________________________________________
		//___Check if pending for a cal pulse if no consecutive trigger was found
		else if (s_pendingCalCount)
		{
			s_pendingCalCount--;
						
			// Check if done pending for a Cal pulse
			if (s_pendingCalCount == 0)
			{
				// Time to handle the Cal pulse
				s_calPulse = YES;
				s_calSampleCount = START_CAL_SIGNAL;
			}
		}
		//___________________________________________________________________________________________
		//___Check if waiting to finish monitoring
		else if (g_doneTakingEvents == PENDING)
		{
			g_doneTakingEvents = YES;
		}
	}
	//___________________________________________________________________________________________
	//___Check if handling event samples
	else if ((s_recordingEvent == YES) && (s_sampleCount))
	{
		//___________________________________________________________________________________________
		//___Check if pretrig data to copy
		if (s_pretrigCount)
		{
#if VT_FEATURE_DISABLED
			// Check if the end of the Pretrigger buffer has been reached
			if ((g_tailOfPretriggerBuff + NUMBER_OF_CHANNELS_DEFAULT) >= g_endOfPretriggerBuff)
			{
				// Copy oldest (which is currently the first) Pretrigger buffer samples to event Pretrigger
				*(SAMPLE_DATA_STRUCT*)s_pretrigPtr = *(SAMPLE_DATA_STRUCT*)g_startOfPretriggerBuff;
			}
			else // Copy oldest Pretrigger buffer samples to event Pretrigger
			{
				*(SAMPLE_DATA_STRUCT*)s_pretrigPtr = *(SAMPLE_DATA_STRUCT*)(g_tailOfPretriggerBuff + NUMBER_OF_CHANNELS_DEFAULT);
			}
#else
			// Check if using the standard trigger
			if (g_triggerRecord.trec.variableTriggerEnable != YES)
			{
				// Check if the end of the Pretrigger buffer has been reached
				if ((g_tailOfPretriggerBuff + NUMBER_OF_CHANNELS_DEFAULT) >= g_endOfPretriggerBuff)
				{
					// Copy oldest (which is currently the first) Pretrigger buffer samples to event Pretrigger
					*(SAMPLE_DATA_STRUCT*)s_pretrigPtr = *(SAMPLE_DATA_STRUCT*)g_startOfPretriggerBuff;
				}
				else // Copy oldest Pretrigger buffer samples to event Pretrigger
				{
					*(SAMPLE_DATA_STRUCT*)s_pretrigPtr = *(SAMPLE_DATA_STRUCT*)(g_tailOfPretriggerBuff + NUMBER_OF_CHANNELS_DEFAULT);
				}
			}
			else // Variable trigger standard (USBM, OSM)
			{
				// Copy oldest Pretrigger buffer sample to Pretrigger
				*(SAMPLE_DATA_STRUCT*)s_pretrigPtr = *(SAMPLE_DATA_STRUCT*)s_variablePretriggerBuffPtr;

				// Increment variable pretrigger pointer
				s_variablePretriggerBuffPtr += NUMBER_OF_CHANNELS_DEFAULT;

				// Check if variable pretrigger pointer wrapped and adjust accordingly
				if (s_variablePretriggerBuffPtr >= g_endOfPretriggerBuff) { s_variablePretriggerBuffPtr = g_startOfPretriggerBuff; }
			}
#endif
			s_pretrigPtr += NUMBER_OF_CHANNELS_DEFAULT;
			s_pretrigCount--;
		}

#if VT_FEATURE_DISABLED
		//___________________________________________________________________________________________
		//___Copy data samples to event buffer
		*(SAMPLE_DATA_STRUCT*)s_samplePtr = *(SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff;
#else
		// Check if using the standard trigger
		if (g_triggerRecord.trec.variableTriggerEnable != YES)
		{
			//___________________________________________________________________________________________
			//___Copy data samples to event buffer
			*(SAMPLE_DATA_STRUCT*)s_samplePtr = *(SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff;
		}
		else // Variable trigger standard (USBM, OSM)
		{
			//___________________________________________________________________________________________
			//___Copy data samples to event buffer
			*(SAMPLE_DATA_STRUCT*)s_samplePtr = *(SAMPLE_DATA_STRUCT*)s_variableEventPretriggerBuffPtr;

			// Increment variable pretrigger pointer
			s_variableEventPretriggerBuffPtr += NUMBER_OF_CHANNELS_DEFAULT;

			// Check if variable pretrigger pointer wrapped and adjust accordingly
			if (s_variableEventPretriggerBuffPtr >= g_endOfPretriggerBuff) { s_variableEventPretriggerBuffPtr = g_startOfPretriggerBuff; }
		}
#endif
		s_samplePtr += NUMBER_OF_CHANNELS_DEFAULT;
		s_sampleCount--;
					
		//___________________________________________________________________________________________
		//___Check if all the event samples have been handled
		if (s_sampleCount == 0)
		{
			//debug("--> Recording done\r\n");
			//usart_write_char(&AVR32_USART1, '%');

			s_recordingEvent = NO;

			// Mark one less event buffer available and inc the event buffer index for next event usage
			g_freeEventBuffers--;
			g_eventBufferWriteIndex++;

			// Check if the event buffer index matches the max
			if (g_eventBufferWriteIndex == g_maxEventBuffers)
			{
				// Reset event buffer index to the start
				g_eventBufferWriteIndex = 0;
			}

			// Check if maximum consecutive events have not been captured, therefore pend Cal pulse
			if (s_consecEventsWithoutCal < s_consecutiveEventsWithoutCalThreshold)
			{
				// Setup delay for Cal pulse (based on Pretrigger size)
				s_pendingCalCount = g_triggerRecord.trec.sample_rate / g_unitConfig.pretrigBufferDivider;
			}
			else // Max number of events have been captured, force a Cal pulse
			{
				// Time to handle the Cal pulse
				s_calPulse = YES;
				s_calSampleCount = START_CAL_SIGNAL;
			}
		}
	}
	//___________________________________________________________________________________________
	//___Check if handling cal pulse samples
	else if ((s_calPulse == YES) && (s_calSampleCount))
	{
		// Check if the SPI is available to access or already locked to handle a Cal pulse
		if ((g_spi1AccessLock == AVAILABLE) || (g_spi1AccessLock == CAL_PULSE_LOCK))
		{
			// If SPI lock is available, grab it and flag for cal pulse
			if (g_spi1AccessLock == AVAILABLE) { g_spi1AccessLock = CAL_PULSE_LOCK;	}
							
			// Check for the start of the Cal pulse and set low sensitivity and swap clock source for 1024 sample rate
			if (s_calSampleCount == START_CAL_SIGNAL)
			{
				// Check if on high sensitivity and if so set to low sensitivity for Cal pulse
				if (g_triggerRecord.srec.sensitivity == HIGH) { SetSeismicGainSelect(SEISMIC_GAIN_NORMAL); }

				g_testTimeSinceLastCalPulse = g_lifetimeHalfSecondTickCount;

				// Swap to alternate timer/counter for default 1024 sample rate for Cal
#if INTERNAL_SAMPLING_SOURCE
				SetupInteralSampleTimer(SAMPLE_RATE_1K);
				StartInteralSampleTimer();
#elif EXTERNAL_SAMPLING_SOURCE
				StartExternalRtcClock(SAMPLE_RATE_1K);
#endif
				s_calSampleCount = MAX_CAL_SAMPLES;
			}
			else // Cal pulse started
			{
				// Wait 5 samples to start cal signaling (~5 ms)
				if (s_calSampleCount == CAL_SAMPLE_COUNT_FIRST_TRANSITION_HIGH) { AdSetCalSignalHigh();	}	// (5 ms after start, run for 10 ms)
				if (s_calSampleCount == CAL_SAMPLE_COUNT_SECOND_TRANSITION_LOW) { AdSetCalSignalLow(); }	// (15 ms after start, run for 20 ms)
				if (s_calSampleCount == CAL_SAMPLE_COUNT_THIRD_TRANSITION_HIGH) { AdSetCalSignalHigh(); }	// (35 ms after start, run for 10 ms)
				if (s_calSampleCount == CAL_SAMPLE_COUNT_FOURTH_TRANSITION_OFF) { AdSetCalSignalOff(); }	// (45 ms after start, run for 55 ms)

				// Copy cal data to the event buffer cal section
				*(SAMPLE_DATA_STRUCT*)(s_calPtr[DEFAULT_CAL_BUFFER_INDEX]) = *(SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff;

				s_calPtr[DEFAULT_CAL_BUFFER_INDEX] += NUMBER_OF_CHANNELS_DEFAULT;
									
				// Check if a delayed cal pointer has been established
				if (s_calPtr[ONCE_DELAYED_CAL_BUFFER_INDEX] != NULL)
				{
					// Copy delayed cal data to event buffer cal section
					*(SAMPLE_DATA_STRUCT*)(s_calPtr[ONCE_DELAYED_CAL_BUFFER_INDEX]) = *(SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff;
					s_calPtr[ONCE_DELAYED_CAL_BUFFER_INDEX] += NUMBER_OF_CHANNELS_DEFAULT;
				}

				// Check if a second delayed cal pointer has been established
				if (s_calPtr[TWICE_DELAYED_CAL_BUFFER_INDEX] != NULL)
				{
					// Copy delayed cal data to event buffer cal section
					*(SAMPLE_DATA_STRUCT*)(s_calPtr[TWICE_DELAYED_CAL_BUFFER_INDEX]) = *(SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff;
					s_calPtr[TWICE_DELAYED_CAL_BUFFER_INDEX] += NUMBER_OF_CHANNELS_DEFAULT;
				}

				s_calSampleCount--;

				if (s_calSampleCount == 0)
				{
					//debug("\n--> Cal done\r\n");
					//usart_write_char(&AVR32_USART1, '&');
						
					// Reset all states and counters (that haven't already)
					s_calPulse = NO;
					s_consecSeismicTriggerCount = 0;
					s_consecAirTriggerCount = 0;
					s_consecEventsWithoutCal = 0;
					
					// Reset cal pointers to null
					s_calPtr[DEFAULT_CAL_BUFFER_INDEX] = NULL;
					s_calPtr[ONCE_DELAYED_CAL_BUFFER_INDEX] = NULL;
					s_calPtr[TWICE_DELAYED_CAL_BUFFER_INDEX] = NULL;

					// Signal for an event to be processed
					raiseSystemEventFlag_ISR(TRIGGER_EVENT);

					// Global flag to signal done handling an event
					g_busyProcessingEvent = NO;

					if (g_doneTakingEvents == PENDING) { g_doneTakingEvents = YES; }
					
					// Check if on high sensitivity and if so reset to high sensitivity after Cal pulse (done on low)
					if (g_triggerRecord.srec.sensitivity == HIGH) { SetSeismicGainSelect(SEISMIC_GAIN_HIGH); }

					// Swap back to original sampling rate
#if INTERNAL_SAMPLING_SOURCE
					SetupInteralSampleTimer(g_triggerRecord.trec.sample_rate);
					StartInteralSampleTimer();
#elif EXTERNAL_SAMPLING_SOURCE
					StartExternalRtcClock(g_triggerRecord.trec.sample_rate);
#endif
					// Invalidate the Pretrigger buffer until it's filled again
					s_pretriggerFull = NO;

					g_spi1AccessLock = AVAILABLE;
				}																
			}
		}
	}
	else
	{
#if 1 /* Test */
		if (g_freeEventBuffers)
		{
			// Should _never_ get here
			debugErr("Error in ISR processing\r\n");
		}
#else /* Normal */
		// Should _never_ get here
		debugErr("Error in ISR processing\r\n");
#endif
	}
}

#if 0 /* Test (First attempt to buffer waveform data for processing outside of the ISR) */
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static inline void moveWaveformData_ISR_Inline(void)
{
	// Copy sample over to buffer
	*(SAMPLE_DATA_STRUCT*) = *(SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff;

	// Increment the write pointer
	 += NUMBER_OF_CHANNELS_DEFAULT;
	
	// Check if write pointer is beyond the end of the circular bounds
	// add code

	// Alert system that we have data in ram buffer, raise flag to calculate and move data to flash.
	raiseSystemEventFlag_ISR();
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static inline void moveBargraphData_ISR_Inline(void)
{
	// Copy sample over to bargraph buffer
	*(SAMPLE_DATA_STRUCT*)g_bargraphDataWritePtr = *(SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff;

	// Increment the write pointer
	g_bargraphDataWritePtr += NUMBER_OF_CHANNELS_DEFAULT;
	
	// Check if write pointer is beyond the end of the circular bounds
	if (g_bargraphDataWritePtr >= g_bargraphDataEndPtr) g_bargraphDataWritePtr = g_bargraphDataStartPtr;

#if 1 /* Test if smaller BG data cache (due to much smaller sandbox) overruns */
	if (g_bargraphDataWritePtr == g_bargraphDataReadPtr) { debugWarn("Bargraph: BG data about to overrun\r\n"); }
#endif

	//________________________________________________________________________________________________
	//
	// Bargraph Bar Interval clocking changed to be time synced (instead of sample count synced)
	//________________________________________________________________________________________________

	// Check if the clock needs to be initialized
	if (g_bargraphBarIntervalClock == 0)
	{
		g_bargraphBarIntervalClock = g_lifetimeHalfSecondTickCount + (g_triggerRecord.bgrec.barInterval * TICKS_PER_SEC) - 1;
	}

	// Check if time signals end of a Bar Interval
	if ((volatile int32)g_lifetimeHalfSecondTickCount > (volatile int32)g_bargraphBarIntervalClock)
	{
		g_bargraphBarIntervalClock = g_lifetimeHalfSecondTickCount + (g_triggerRecord.bgrec.barInterval * TICKS_PER_SEC) - 1;

		// Signal end of Bar Interval with special key
		*(SAMPLE_DATA_STRUCT*)g_bargraphDataWritePtr = (SAMPLE_DATA_STRUCT)BAR_INTERVAL_END_KEY_SAMPLE;

		// Increment the write pointer
		g_bargraphDataWritePtr += NUMBER_OF_CHANNELS_DEFAULT;

		// Check if write pointer is beyond the end of the circular bounds
		if (g_bargraphDataWritePtr >= g_bargraphDataEndPtr) g_bargraphDataWritePtr = g_bargraphDataStartPtr;
	}

	// Alert system that we have data in ram buffer, raise flag to calculate and move data to flash.
	raiseSystemEventFlag_ISR(BARGRAPH_EVENT);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static inline void checkForTemperatureDrift_ISR_Inline(void)
{
	// Get the delta between the current and previous temperature reading
	s_temperatureDelta = abs((int16)(g_currentTempReading - g_previousTempReading));

	// Check if the delta in consecutive temp readings is greater than the max jump signifying a bogus temp reading that plagues this A/D part
	if (s_temperatureDelta < MAX_TEMPERATURE_JUMP_PER_SAMPLE)
	{
		// Update the previous temperature reading
		g_previousTempReading = g_currentTempReading;

		// Check if the stored temp reading is higher than the current reading
		if (g_storedTempReading > g_currentTempReading)
		{
			// Check if the delta is less than the margin of change required to signal an adjustment
			if ((g_storedTempReading - g_currentTempReading) > AD_TEMP_COUNT_FOR_ADJUSTMENT)
			{
				// Updated stored temp reading for future comparison
				g_storedTempReading = g_currentTempReading;

				// Re-init the update to get new offsets
				g_updateOffsetCount = 0;

				raiseSystemEventFlag_ISR(UPDATE_OFFSET_EVENT);
			}
		}
		// The current temp reading is equal or higher than the stored (result needs to be positive)
		else if ((g_currentTempReading - g_storedTempReading) > AD_TEMP_COUNT_FOR_ADJUSTMENT)
		{
			// Updated stored temp reading for future comparison
			g_storedTempReading = g_currentTempReading;

			// Re-init the update to get new offsets
			g_updateOffsetCount = 0;

			raiseSystemEventFlag_ISR(UPDATE_OFFSET_EVENT);
		}
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SensorCalibrationDataInit(void)
{
	uint16 i;

	memset(&s_tempSensorCalPeaks, 0, sizeof(s_tempSensorCalPeaks));
	memset(&s_tempChanMed[0][0], 0, sizeof(s_tempChanMed));
	memset(&s_tempSensorCalFreqCounter, 0, sizeof(s_tempSensorCalFreqCounter));
	memset(&s_tempSensorCalFreqCounts, 0, sizeof(s_tempSensorCalFreqCounts));

	// Reset the Min and Max to initial condition and Avg to zero
	for (i = 0; i < MAX_NUM_OF_CHANNELS; i++)
	{
		s_tempChanMin[i] = 0xFFFF;
		s_tempChanMax[i] = 0x0000;
		s_tempChanAvg[i] = 0;
	}

	s_sensorCalSampleCount = CALIBRATION_FIXED_SAMPLE_RATE;
	g_calibrationGeneratePulse = NO;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ProcessSensorCalibrationData(void)
{
	uint16 i, j;

	//=============================================================================================
	// Process data at end of 1 second cycle
	//=============================================================================================

	// Check if at the end of a 1 second cycle
	if (--s_sensorCalSampleCount == 0)
	{
		// Reset the counter
		s_sensorCalSampleCount = CALIBRATION_FIXED_SAMPLE_RATE;

		//=================
		// Sensor Cal Peaks
		//-----------------

		// Shift the Peaks out after a second (index 1 being the current peak for the last second, index 2 for the peak the second before, etc) and save frequency counts
		g_sensorCalPeaks[2] = g_sensorCalPeaks[1];
		g_sensorCalPeaks[1] = g_sensorCalPeaks[0];
		g_sensorCalPeaks[0] = s_tempSensorCalPeaks;
		g_sensorCalFreqCounts = s_tempSensorCalFreqCounts;

		// Clear out the first Peak hold values
		memset(&s_tempSensorCalPeaks, 0, sizeof(SAMPLE_DATA_STRUCT));

		//==============
		// Channel Noise
		//--------------

		// Calculate the channel med percentages
		for (i = 0; i < MAX_NUM_OF_CHANNELS; i++)
		{
			for (j = 0; j < 7; j++)
			{
				s_tempChanMed[i][j] *= 100;
				s_tempChanMed[i][j] /= CALIBRATION_FIXED_SAMPLE_RATE;
			}

			// Calculate the percentage of samples found withing the +/- 6 band
			s_tempChanMed[i][7] = CALIBRATION_FIXED_SAMPLE_RATE - s_tempChanMed[i][7];
			s_tempChanMed[i][7] *= 100;
			s_tempChanMed[i][7] /= CALIBRATION_FIXED_SAMPLE_RATE;
		}

		// Move the Med to display structure
		memcpy(&g_sensorCalChanMed[0][0], &s_tempChanMed[0][0], sizeof(s_tempChanMed));

		// Zero the Med
		memset(&s_tempChanMed[0][0], 0, sizeof(s_tempChanMed));

		//=================
		// Min, Max and Avg
		//-----------------

		// Remove midpoint from Min, Max and Avg calculations (to center around 0 instead of ACCURACY_16_BIT_MIDPOINT)
		for (i = 0; i < MAX_NUM_OF_CHANNELS; i++)
		{
			s_tempChanMin[i] -= ACCURACY_16_BIT_MIDPOINT;
			s_tempChanMax[i] -= ACCURACY_16_BIT_MIDPOINT;

			s_tempChanAvg[i] /= CALIBRATION_FIXED_SAMPLE_RATE;
			s_tempChanAvg[i] -= ACCURACY_16_BIT_MIDPOINT;
		}

		// Move the Min, Max and Avg to display struct
		memcpy(&g_sensorCalChanMin, &s_tempChanMin, sizeof(s_tempChanMin));
		memcpy(&g_sensorCalChanMax, &s_tempChanMax, sizeof(s_tempChanMax));
		memcpy(&g_sensorCalChanAvg, &s_tempChanAvg, sizeof(s_tempChanAvg));

		// Reset the Min and Max to initial condition and Avg to zero
		for (i = 0; i < MAX_NUM_OF_CHANNELS; i++)
		{
			s_tempChanMin[i] = 0xFFFF;
			s_tempChanMax[i] = 0x0000;
			s_tempChanAvg[i] = 0;
		}
	}

	//=============================================================================================
	// Generate cal pulse if selected
	//=============================================================================================
	if (g_calibrationGeneratePulse == YES)
	{
		// Process the cal in about the middle of the second
		if ((s_sensorCalSampleCount - (CALIBRATION_FIXED_SAMPLE_RATE / 2)) == CAL_SAMPLE_COUNT_FIRST_TRANSITION_HIGH) { AdSetCalSignalHigh(); }	// (5 ms after start, run for 10 ms)
		if ((s_sensorCalSampleCount - (CALIBRATION_FIXED_SAMPLE_RATE / 2)) == CAL_SAMPLE_COUNT_SECOND_TRANSITION_LOW) { AdSetCalSignalLow(); }	// (15 ms after start, run for 20 ms)
		if ((s_sensorCalSampleCount - (CALIBRATION_FIXED_SAMPLE_RATE / 2)) == CAL_SAMPLE_COUNT_THIRD_TRANSITION_HIGH) { AdSetCalSignalHigh(); }	// (35 ms after start, run for 10 ms)
		if ((s_sensorCalSampleCount - (CALIBRATION_FIXED_SAMPLE_RATE / 2)) == CAL_SAMPLE_COUNT_FOURTH_TRANSITION_OFF) { AdSetCalSignalOff(); }	// (45 ms after start, run for 55 ms)
	}

	//=============================================================================================
	// Process current sample data
	//=============================================================================================

	//========================
	// Process frequency count
	//------------------------

	// Check for initial condition
	if (s_tempSensorCalFreqCounter.a == 0)
	{
		// Init the sign and counter
		s_tempSensorCalFreqSign.r = (uint16)(s_R_channelReading & g_bitAccuracyMidpoint); s_tempSensorCalFreqCounter.r = 1;
		s_tempSensorCalFreqSign.v = (uint16)(s_V_channelReading & g_bitAccuracyMidpoint); s_tempSensorCalFreqCounter.v = 1;
		s_tempSensorCalFreqSign.t = (uint16)(s_T_channelReading & g_bitAccuracyMidpoint); s_tempSensorCalFreqCounter.t = 1;
		s_tempSensorCalFreqSign.a = (uint16)(s_A_channelReading & g_bitAccuracyMidpoint); s_tempSensorCalFreqCounter.a = 1;
	}

	// Check if the stored sign comparison signals a zero crossing and if so save new sign and reset freq counter otherwise increment count
	if (s_tempSensorCalFreqSign.r ^ (s_R_channelReading & g_bitAccuracyMidpoint)) { s_tempSensorCalFreqSign.r = (uint16)(s_R_channelReading & g_bitAccuracyMidpoint); s_tempSensorCalFreqCounter.r = 1; }
	else { if (s_tempSensorCalFreqCounter.r < 0xFFFF) { s_tempSensorCalFreqCounter.r++; } }

	if (s_tempSensorCalFreqSign.v ^ (s_V_channelReading & g_bitAccuracyMidpoint)) { s_tempSensorCalFreqSign.v = (uint16)(s_V_channelReading & g_bitAccuracyMidpoint); s_tempSensorCalFreqCounter.v = 1; }
	else { if (s_tempSensorCalFreqCounter.v < 0xFFFF) { s_tempSensorCalFreqCounter.v++; } }

	if (s_tempSensorCalFreqSign.t ^ (s_T_channelReading & g_bitAccuracyMidpoint)) { s_tempSensorCalFreqSign.t = (uint16)(s_T_channelReading & g_bitAccuracyMidpoint); s_tempSensorCalFreqCounter.t = 1; }
	else { if (s_tempSensorCalFreqCounter.t < 0xFFFF) { s_tempSensorCalFreqCounter.t++; } }

	if (s_tempSensorCalFreqSign.a ^ (s_A_channelReading & g_bitAccuracyMidpoint)) { s_tempSensorCalFreqSign.a = (uint16)(s_A_channelReading & g_bitAccuracyMidpoint); s_tempSensorCalFreqCounter.a = 1; }
	else { if (s_tempSensorCalFreqCounter.a < 0xFFFF) { s_tempSensorCalFreqCounter.a++; } }

	//=================
	// Min, Max and Avg
	//-----------------
	if (s_R_channelReading < s_tempChanMin[1]) { s_tempChanMin[1] = s_R_channelReading; }
	if (s_R_channelReading > s_tempChanMax[1]) { s_tempChanMax[1] = s_R_channelReading; }
	s_tempChanAvg[1] += s_R_channelReading;

	if (s_V_channelReading < s_tempChanMin[2]) { s_tempChanMin[2] = s_V_channelReading; }
	if (s_V_channelReading > s_tempChanMax[2]) { s_tempChanMax[2] = s_V_channelReading; }
	s_tempChanAvg[2] += s_V_channelReading;

	if (s_T_channelReading < s_tempChanMin[3]) { s_tempChanMin[3] = s_T_channelReading; }
	if (s_T_channelReading > s_tempChanMax[3]) { s_tempChanMax[3] = s_T_channelReading; }
	s_tempChanAvg[3] += s_T_channelReading;

	if (s_A_channelReading < s_tempChanMin[0]) { s_tempChanMin[0] = s_A_channelReading; }
	if (s_A_channelReading > s_tempChanMax[0]) { s_tempChanMax[0] = s_A_channelReading; }
	s_tempChanAvg[0] += s_A_channelReading;

	//======================
	// Normalize sample data
	//----------------------

	normalizeSampleData_ISR_Inline();

	// Get the max peak from the normalized data and store the current frequency count
	if (s_R_channelReading > s_tempSensorCalPeaks.r) { s_tempSensorCalPeaks.r = s_R_channelReading; s_tempSensorCalFreqCounts.r = s_tempSensorCalFreqCounter.r; }
	if (s_V_channelReading > s_tempSensorCalPeaks.v) { s_tempSensorCalPeaks.v = s_V_channelReading; s_tempSensorCalFreqCounts.v = s_tempSensorCalFreqCounter.v; }
	if (s_T_channelReading > s_tempSensorCalPeaks.t) { s_tempSensorCalPeaks.t = s_T_channelReading; s_tempSensorCalFreqCounts.t = s_tempSensorCalFreqCounter.t; }
	if (s_A_channelReading > s_tempSensorCalPeaks.a) { s_tempSensorCalPeaks.a = s_A_channelReading; s_tempSensorCalFreqCounts.a = s_tempSensorCalFreqCounter.a; }

	if (s_R_channelReading < 7) { s_tempChanMed[1][s_R_channelReading]++; } else { s_tempChanMed[1][7]++; }
	if (s_V_channelReading < 7) { s_tempChanMed[2][s_V_channelReading]++; } else { s_tempChanMed[2][7]++; }
	if (s_T_channelReading < 7) { s_tempChanMed[3][s_T_channelReading]++; } else { s_tempChanMed[3][7]++; }
	if (s_A_channelReading < 7)	{ s_tempChanMed[0][s_A_channelReading]++; } else { s_tempChanMed[0][7]++; }

#if 1 /* Test */
	ACC_DATA_STRUCT channelData;
#if 0 /* Original */
	GetAccelerometerChannelData(&channelData);
#else
	if (g_spi2InUseByLCD)
	{
		// Hopefully an updated Acc data cache is available (executed just prior to LCD write), otherwise it's worst case with no ability to get current Acc data so duplicate last sample
		channelData = g_accDataCache;
	}
	else
	{
		GetAccelerometerChannelData(&channelData);
		g_accDataCache = channelData;
	}
#endif

	if (channelData.x < s_tempChanMin[4]) { s_tempChanMin[4] = channelData.x; }
	if (channelData.x > s_tempChanMax[4]) { s_tempChanMax[4] = channelData.x; }
	s_tempChanAvg[4] += channelData.x;

	if (channelData.y < s_tempChanMin[5]) { s_tempChanMin[5] = channelData.y; }
	if (channelData.y > s_tempChanMax[5]) { s_tempChanMax[5] = channelData.y; }
	s_tempChanAvg[5] += channelData.y;

	if (channelData.z < s_tempChanMin[6]) { s_tempChanMin[6] = channelData.z; }
	if (channelData.z > s_tempChanMax[6]) { s_tempChanMax[6] = channelData.z; }
	s_tempChanAvg[6] += channelData.z;
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static inline void applyOffsetAndCacheSampleData_ISR_Inline(void)
{
	// Apply channel zero offset
#if 1 /* Normal */
	s_R_channelReading -= g_channelOffset.r_offset;
	s_V_channelReading -= g_channelOffset.v_offset;
	s_T_channelReading -= g_channelOffset.t_offset;
	s_A_channelReading -= g_channelOffset.a_offset;
#else /* Test (Mark cal pulse with identifiable data when no sensor connected) */
	if ((s_calPulse == YES) && (s_calSampleCount))
	{
		s_R_channelReading = (0x2000 + s_calSampleCount - 100);
		s_V_channelReading = (0x3000 + s_calSampleCount - 100);
		s_T_channelReading = (0x4000 + s_calSampleCount - 100);
		s_A_channelReading = (0x1000 + s_calSampleCount - 100);
	}
	else
	{
		s_R_channelReading -= g_channelOffset.r_offset;
		s_V_channelReading -= g_channelOffset.v_offset;
		s_T_channelReading -= g_channelOffset.t_offset;
		s_A_channelReading -= g_channelOffset.a_offset;
	}
#endif

	// Store the raw A/D data (adjusted for zero offset) into the Pretrigger buffer
	((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->r = s_R_channelReading;
	((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->v = s_V_channelReading;
	((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->t = s_T_channelReading;
	((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->a = s_A_channelReading;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static inline void getChannelDataWithReadbackWithTemp_ISR_Inline(void)
{
	uint8_t chanDataRaw[3];
#if 1 /* Test */
	uint8_t overVoltageAlert = NO;
	uint8_t ovAlertChannels = 0;
#endif

	// Todo: need variable channel config for selectable dynamic channels
	// Todo: verify channel inputs on which channel data lines

	// Conversion time max is 415ns (~50 clock cycles), normal SPI setup processing should take longer than that without requiring waiting on the ADC busy state (Port 0, Pin 17)

#if 0 /* Original */
	// Chan 0 - R
	SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING); SetAdcConversionState(OFF);
	s_R_channelReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]);
	if ((chanDataRaw[2] & 0x0F) != 0) { s_channelSyncError = YES; }

	// Chan 1 - T
	SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING); SetAdcConversionState(OFF);
	s_T_channelReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]);
	if ((chanDataRaw[2] & 0x0F) != 1) { s_channelSyncError = YES; }

	// Chan 2 - V
	SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING); SetAdcConversionState(OFF);
	s_V_channelReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]);
	if ((chanDataRaw[2] & 0x0F) != 2) { s_channelSyncError = YES; }

	// Chan 3 - A
	SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING); SetAdcConversionState(OFF);
	s_A_channelReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]);
	if ((chanDataRaw[2] & 0x0F) != 3) { s_channelSyncError = YES; }
#elif 0 /* Try active channels */
extern uint8_t chanActive[8];
	if (chanActive[0]) { SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING); SetAdcConversionState(OFF);
		s_R_channelReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]); if ((chanDataRaw[2] & 0x0F) != 0) { s_channelSyncError = YES; } }
	if (chanActive[1]) { SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING); SetAdcConversionState(OFF);
		s_T_channelReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]); if ((chanDataRaw[2] & 0x0F) != 1) { s_channelSyncError = YES; } }
	if (chanActive[2]) { SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING); SetAdcConversionState(OFF);
		s_V_channelReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]); if ((chanDataRaw[2] & 0x0F) != 2) { s_channelSyncError = YES; } }
	if (chanActive[3]) { SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING); SetAdcConversionState(OFF);
		s_A_channelReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]); if ((chanDataRaw[2] & 0x0F) != 3) { s_channelSyncError = YES; } }

	if (chanActive[4]) { SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING); SetAdcConversionState(OFF);
		s_R_channelReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]); if ((chanDataRaw[2] & 0x0F) != 4) { s_channelSyncError = YES; } }
	if (chanActive[5]) { SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING); SetAdcConversionState(OFF);
		s_T_channelReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]); if ((chanDataRaw[2] & 0x0F) != 5) { s_channelSyncError = YES; } }
	if (chanActive[6]) { SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING); SetAdcConversionState(OFF);
		s_V_channelReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]); if ((chanDataRaw[2] & 0x0F) != 6) { s_channelSyncError = YES; } }
	if (chanActive[7]) { SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING); SetAdcConversionState(OFF);
		s_A_channelReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]); if ((chanDataRaw[2] & 0x0F) != 7) { s_channelSyncError = YES; } }
#else /* Try active channels and monitor Over voltage Alert */
extern uint8_t chanActive[8];
	if (chanActive[0]) { SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING); SetAdcConversionState(OFF);
		s_R_channelReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]); if ((chanDataRaw[2] & 0x0F) != 0) { s_channelSyncError = YES; } if (chanDataRaw[2] & 0x10) { overVoltageAlert = YES; ovAlertChannels |= (chanDataRaw[2] & 0x0F); } }
	if (chanActive[1]) { SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING); SetAdcConversionState(OFF);
		s_T_channelReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]); if ((chanDataRaw[2] & 0x0F) != 1) { s_channelSyncError = YES; } if (chanDataRaw[2] & 0x10) { overVoltageAlert = YES; ovAlertChannels |= (chanDataRaw[2] & 0x0F); } }
	if (chanActive[2]) { SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING); SetAdcConversionState(OFF);
		s_V_channelReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]); if ((chanDataRaw[2] & 0x0F) != 2) { s_channelSyncError = YES; } if (chanDataRaw[2] & 0x10) { overVoltageAlert = YES; ovAlertChannels |= (chanDataRaw[2] & 0x0F); } }
	if (chanActive[3]) { SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING); SetAdcConversionState(OFF);
		s_A_channelReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]); if ((chanDataRaw[2] & 0x0F) != 3) { s_channelSyncError = YES; } if (chanDataRaw[2] & 0x10) { overVoltageAlert = YES; ovAlertChannels |= (chanDataRaw[2] & 0x0F); } }

	if (chanActive[4]) { SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING); SetAdcConversionState(OFF);
		s_R_channelReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]); if ((chanDataRaw[2] & 0x0F) != 4) { s_channelSyncError = YES; } if (chanDataRaw[2] & 0x10) { overVoltageAlert = YES; ovAlertChannels |= (chanDataRaw[2] & 0x0F); } }
	if (chanActive[5]) { SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING); SetAdcConversionState(OFF);
		s_T_channelReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]); if ((chanDataRaw[2] & 0x0F) != 5) { s_channelSyncError = YES; } if (chanDataRaw[2] & 0x10) { overVoltageAlert = YES; ovAlertChannels |= (chanDataRaw[2] & 0x0F); } }
	if (chanActive[6]) { SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING); SetAdcConversionState(OFF);
		s_V_channelReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]); if ((chanDataRaw[2] & 0x0F) != 6) { s_channelSyncError = YES; } if (chanDataRaw[2] & 0x10) { overVoltageAlert = YES; ovAlertChannels |= (chanDataRaw[2] & 0x0F); } }
	if (chanActive[7]) { SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING); SetAdcConversionState(OFF);
		s_A_channelReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]); if ((chanDataRaw[2] & 0x0F) != 7) { s_channelSyncError = YES; } if (chanDataRaw[2] & 0x10) { overVoltageAlert = YES; ovAlertChannels |= (chanDataRaw[2] & 0x0F); } }

	if (overVoltageAlert) { debugErr("AD Over Voltage Alert: Channel bit mask 0x%x\r\n", ovAlertChannels); }
#endif

	// Temperature
	SetAdcConversionState(ON);
	SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING);
	SetAdcConversionState(OFF);
	g_currentTempReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]);
	if ((chanDataRaw[2] & 0x0F) != 15) { s_channelSyncError = YES; } // An INx value of 15 corresponds to either IN15 or the temperature sensor
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static inline void getChannelDataNoReadbackWithTemp_ISR_Inline(void)
{
	uint8_t chanDataRaw[2];

	// Todo: need variable channel config for selectable dynamic channels

	// Conversion time max is 415ns (~50 clock cycles), normal SPI setup processing should take longer than that without requiring waiting on the ADC busy state (Port 0, Pin 17)

	// Chan 0 - R?
	SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE, BLOCKING); SetAdcConversionState(OFF);
	s_R_channelReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]);

	// Chan 1 - T?
	SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE, BLOCKING); SetAdcConversionState(OFF);
	s_T_channelReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]);

	// Chan 2 - V?
	SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE, BLOCKING); SetAdcConversionState(OFF);
	s_V_channelReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]);

	// Chan 3 - A
	SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE, BLOCKING); SetAdcConversionState(OFF);
	s_A_channelReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]);

	// Temp
	SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE, BLOCKING); SetAdcConversionState(OFF);
	g_currentTempReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static inline void getChannelDataNoReadbackNoTemp_ISR_Inline(void)
{
	uint8_t chanDataRaw[2];

	// Todo: need variable channel config for selectable dynamic channels

	// Conversion time max is 415ns (~50 clock cycles), normal SPI setup processing should take longer than that without requiring waiting on the ADC busy state (Port 0, Pin 17)

	// Chan 0 - R?
	SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE, BLOCKING); SetAdcConversionState(OFF);
	s_R_channelReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]);

	// Chan 1 - T?
	SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE, BLOCKING); SetAdcConversionState(OFF);
	s_T_channelReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]);

	// Chan 2 - V?
	SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE, BLOCKING); SetAdcConversionState(OFF);
	s_V_channelReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]);

	// Chan 3 - A
	SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE, BLOCKING); SetAdcConversionState(OFF);
	s_A_channelReading = ((chanDataRaw[0] << 8) | chanDataRaw[1]);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static inline void getChannelDataAcc_ISR_Inline(void)
{
#if 0 /* Original */
	ACC_DATA_STRUCT accData;
	GetAccelerometerChannelData(&accData);

	s_R_channelReading = accData.x;
	s_T_channelReading = accData.y;
	s_V_channelReading = accData.z;
	s_A_channelReading = 0x8000;
#else
	ACC_DATA_STRUCT accData;

	if (g_spi2InUseByLCD)
	{
		// Hopefully an updated Acc data cache is available (executed just prior to LCD write), otherwise it's worst case with no ability to get current Acc data so duplicate last sample
		accData = g_accDataCache;
	}
	else
	{
		GetAccelerometerChannelData(&accData);
		g_accDataCache = accData;
	}

	s_R_channelReading = accData.x;
	s_T_channelReading = accData.y;
	s_V_channelReading = accData.z;
	s_A_channelReading = 0x8000;
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static inline void HandleChannelSyncError_ISR_Inline(void)
{
	debugErr("AD Channel Sync Error\r\n");

	// Disable A/D due to error
	DisableSensorBlocks();
	PowerControl(ADC_RESET, ON);

	// Delay to allow power down
	SoftUsecWait(10 * SOFT_MSECS);

	// Re-Enable the Ext ADC
	WaitAnalogPower5vGood();
	AD4695_Init();

	// Setup the A/D Channel configuration
	SetupADChannelConfig(s_sampleRate, UNIT_CONFIG_CHANNEL_VERIFICATION);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DataIsrInit(uint16 sampleRate)
{
	s_sampleRate = sampleRate;
	s_pretriggerFull = NO;
	s_checkForTempDrift = NO;
	s_alarmOneCount = 0;
	s_alarmTwoCount = 0;
	s_channelSyncErrorCount = 0;

#if 1 /* Test */
	extern uint32 sampleProcessTiming;
	sampleProcessTiming = 0;
#endif

	if ((g_maxEventBuffers - 1) < CONSEC_EVENTS_WITHOUT_CAL_THRESHOLD)
	{
		s_consecutiveEventsWithoutCalThreshold = (g_maxEventBuffers - 1);
	}
	else { s_consecutiveEventsWithoutCalThreshold = CONSEC_EVENTS_WITHOUT_CAL_THRESHOLD; }

#if (!VT_FEATURE_DISABLED)
	memset(&s_variableTriggerFreqCalcBuffer, 0, sizeof(s_variableTriggerFreqCalcBuffer));

	// Seed the R sign to signal initialization
	s_variableTriggerFreqCalcBuffer.r_sign = 0xFFFF;

	// Set the divider for length conversion (Always as 16-bit)
	s_vtDiv = (float)(ACCURACY_16_BIT_MIDPOINT * g_sensorInfo.sensorAccuracy * ((g_triggerRecord.srec.sensitivity == LOW) ? 2 : 4)) / (float)(g_factorySetupRecord.seismicSensorType);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleActiveAlarmExtension(void)
{
	float secondsRemaining = 0;

	// Check if Alarm 1 is still active
	if (s_alarmOneCount)
	{
		// Calculate remaining time as count + round up (half second) divided by sample rate to get seconds
		secondsRemaining = (((float)s_alarmOneCount + (float)((float)s_sampleRate / 2)) / (float)s_sampleRate);

		// Assign soft timer to turn the Alarm 1 signal off
		AssignSoftTimer(ALARM_ONE_OUTPUT_TIMER_NUM, (uint32)(secondsRemaining * TICKS_PER_SEC), AlarmOneOutputTimerCallback);
	}

	// Check if Alarm 2 is still active
	if (s_alarmTwoCount)
	{
		// Calculate remaining time as count + round up (half second) divided by sample rate to get seconds
		secondsRemaining = (((float)s_alarmTwoCount + (float)((float)s_sampleRate / 2)) / (float)s_sampleRate);

		// Assign soft timer to turn the Alarm 2 signal off
		AssignSoftTimer(ALARM_TWO_OUTPUT_TIMER_NUM, (uint32)(secondsRemaining * TICKS_PER_SEC), AlarmTwoOutputTimerCallback);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static inline void processAdaptiveSamplingStateAndLogic(void)
{
	// 3 possible active states; ADAPTIVE_MAX_RATE, ADAPTIVE_MAX_WAITING_TO_DROP, ADAPTIVE_MIN_RATE
	if (g_adaptiveState == ADAPTIVE_MAX_RATE)
	{
		g_adaptiveSampleDelay--;

		if (g_adaptiveSampleDelay == 0)
		{
			g_adaptiveState = ADAPTIVE_MAX_WAITING_TO_DROP;
		}
	}
	else if (g_adaptiveState == ADAPTIVE_MAX_WAITING_TO_DROP)
	{
		if ((s_R_channelReading < g_adaptiveSeismicThreshold) && (s_V_channelReading < g_adaptiveSeismicThreshold) && (s_T_channelReading < g_adaptiveSeismicThreshold) && (s_A_channelReading < g_adaptiveAcousticThreshold))
		{
			g_adaptiveSampleDelay++;

			// Check if 1/10 of a second of consecutive samples is below the threshold
			if (g_adaptiveSampleDelay > (g_triggerRecord.trec.sample_rate / 10))
			{
				// Set the Adaptive state to the min rate
				g_adaptiveState = ADAPTIVE_MIN_RATE;

				// Save the current pretrigger buffer pointer in case this is the last real sample before adaptive rate drop
				g_adaptiveLastRealSamplePtr = g_tailOfPretriggerBuff;

				g_adaptiveBoundaryMarker = 1;
			}
		}
		else // Sample received above threshold signaling enough activity to continue monitoring max rate
		{
			g_adaptiveSampleDelay = 0;
		}
	}
	else // g_adaptiveState = ADAPTIVE_MIN_RATE
	{
		// Check if the Adaptive boundary marker is zero meaning a new sample was acquired (and not off boundary)
		if (g_adaptiveBoundaryMarker == 0)
		{
			// Check for threshold passing
			if ((s_R_channelReading > g_adaptiveSeismicThreshold) || (s_V_channelReading > g_adaptiveSeismicThreshold) || (s_T_channelReading > g_adaptiveSeismicThreshold) || (s_A_channelReading > g_adaptiveAcousticThreshold))
			{
				// Set state to Adaptive max waiting for something more significant or signal to cool off
				g_adaptiveState = ADAPTIVE_MAX_WAITING_TO_DROP;
				g_adaptiveSampleDelay = 0;
			}

			g_adaptiveLastRealSamplePtr = g_tailOfPretriggerBuff;
		}
		else
		{
			// Special condition skipping normal data collection, copy prior collected sample in pretrigger and either a) process normally or b) skip processing
			*(SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff = *(SAMPLE_DATA_STRUCT*)g_adaptiveLastRealSamplePtr; // same as memcpy(g_tailOfPretriggerBuff, g_adaptiveLastRealSamplePtr, 8);
		}

		// Increment Adaptive boundary marker and check if matching the total count, then reset counter
		g_adaptiveBoundaryMarker++;
		if (g_adaptiveBoundaryMarker == g_adaptiveBoundaryCount) { g_adaptiveBoundaryMarker = 0; }
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 1 /* Test */
uint32 sampleProcessTiming = 0;
#endif
__attribute__((__interrupt__))
void Sample_irq(void)
{
#if 1 /* Test */
	//SysTick->LOAD = 0xffffff; /* set reload register */
	SysTick->VAL = 0xffffff; /* Load the SysTick Counter Value */
	SysTick->CTRL = (SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk); /* Enable SysTick Timer */
#endif

#if 0 /* Test */
static uint32_t trash = 0;
	if (++trash % 1024 == 0) { debugRaw("-"); }
#endif

	//___________________________________________________________________________________________
	//___Test timing (throw away at some point)
	g_sampleCount++;
	//if (g_sampleCount % 1024 == 0) { debugRaw("+"); }

#if 0 /* Test sample count only */
	MXC_GPIO_ClearFlags(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
	return;
#endif

	//___________________________________________________________________________________________
	//___Check for Adaptive sampling w/ Min rate and off boundary
	if ((g_adaptiveState == ADAPTIVE_MIN_RATE) && (g_adaptiveBoundaryMarker != 0))
	{
		// Skip raw data collection, offset, and normalization, but process off boundary sample as established prior for frequency algorithms to work normally
		// I absolutely HATE using a goto here, however due to use of inline functions and logic structure, it's by far the easier method
		goto SKIP_PRIOR_PROCESSING_FOR_ADAPTIVE_MIN_RATE;
	}

#if 1 /* Test Accelerometer */
	//___________________________________________________________________________________________
	//___Internal Accelerometer raw data read 3 channels
	if (g_adChannelConfig == THREE_ACC_CHANNELS_NO_AIR)
	{
		getChannelDataAcc_ISR_Inline();
	}
	else
#endif
	//___________________________________________________________________________________________
	//___AD raw data read all 4 channels without config read back and no temp
	if (g_adChannelConfig == FOUR_AD_CHANNELS_NO_READBACK_NO_TEMP)
	{
		getChannelDataNoReadbackNoTemp_ISR_Inline();
	}
	//___________________________________________________________________________________________
	//___AD raw data read all 4 channels without config read back and temp
	else if (g_adChannelConfig == FOUR_AD_CHANNELS_NO_READBACK_WITH_TEMP)
	{
		getChannelDataNoReadbackWithTemp_ISR_Inline();

		//___________________________________________________________________________________________
		//___Check for temperature drift (only will start after Pretrigger buffer is full initially)
		if (s_checkForTempDrift == YES)
		{
			checkForTemperatureDrift_ISR_Inline();
		}
	}
	//___________________________________________________________________________________________
	//___AD raw data read all 4 channels with config read back and temp
	else // (g_adChannelConfig == FOUR_AD_CHANNELS_WITH_READBACK_WITH_TEMP)
	{
		getChannelDataWithReadbackWithTemp_ISR_Inline();
		
		//___________________________________________________________________________________________
		//___Check for channel sync error
		if (s_channelSyncError == YES)
		{
			if (s_channelSyncErrorCount)
			{
				g_breakpointCause = BP_AD_CHAN_SYNC_ERR;
				// Todo: Issue a breakpoint
			}
			else
			{
				s_channelSyncErrorCount++;

				HandleChannelSyncError_ISR_Inline();
			}

			// Clear the interrupt flag
#if INTERNAL_SAMPLING_SOURCE
			INTERNAL_SAMPLING_TIMER_NUM->intr = MXC_F_TMR_INTR_IRQ;
#elif EXTERNAL_SAMPLING_SOURCE
			MXC_GPIO_ClearFlags(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
#endif
			return;
		}

		//___________________________________________________________________________________________
		//___Check for temperature drift (only will start after Pretrigger buffer is full initially)
		if (s_checkForTempDrift == YES)
		{
			checkForTemperatureDrift_ISR_Inline();
		}
	} // End of reading raw data from External A/D

	//___________________________________________________________________________________________
	//___AD data read successfully, Normal operation
	applyOffsetAndCacheSampleData_ISR_Inline();

	//___________________________________________________________________________________________
	//___Check if not actively sampling
	if (g_sampleProcessing == IDLE_STATE)
	{
		// Idle and not processing, could adjust the channel offsets

		// Isolate processing for just the sensor calibration
		if (g_activeMenu == CAL_SETUP_MENU)
		{
			ProcessSensorCalibrationData();
		}
	}
	//___________________________________________________________________________________________
	//___Check if the Pretrigger buffer is not full, which is necessary for an event
	else if (s_pretriggerFull == NO)
	{
		fillPretriggerBufferUntilFull_ISR_Inline();
	}
	//___________________________________________________________________________________________
	//___Check if handling a Manual Cal
	else if (g_manualCalFlag == TRUE)
	{
		processAndMoveManualCalData_ISR_Inline();
	}
	//___________________________________________________________________________________________
	//___Handle all modes not manual cal
	else
	{
		// Normalize sample data to zero (positive) for comparison
		normalizeSampleData_ISR_Inline();
	
		// Alarm checking section
		checkAlarms_ISR_Inline();

SKIP_PRIOR_PROCESSING_FOR_ADAPTIVE_MIN_RATE:
		//___________________________________________________________________________________________
		//___Process and move the sample data for triggers in waveform or combo mode
		if ((g_triggerRecord.opMode == WAVEFORM_MODE) || (g_triggerRecord.opMode == COMBO_MODE))
		{
			processAndMoveWaveformData_ISR_Inline();
		}

		//___________________________________________________________________________________________
		//___Move the sample data for bar calculations in bargraph or combo mode (when combo mode isn't handling a cal pulse)
		if ((g_triggerRecord.opMode == BARGRAPH_MODE) || ((g_triggerRecord.opMode == COMBO_MODE) && (s_calPulse != YES)))
		{
			moveBargraphData_ISR_Inline();
		}

		//___________________________________________________________________________________________
		//___Handle Adaptive sampling logic if active
		if (g_adaptiveState != ADAPTIVE_DISABLED)
		{
			processAdaptiveSamplingStateAndLogic();
		}
	}			

	//___________________________________________________________________________________________
	//___Advance to the next sample in the buffer
	g_tailOfPretriggerBuff += g_sensorInfo.numOfChannels;

	// Check if the end of the Pretrigger buffer has been reached
	if (g_tailOfPretriggerBuff >= g_endOfPretriggerBuff) g_tailOfPretriggerBuff = g_startOfPretriggerBuff;

#if 1 /* Test */
	if (sampleProcessTiming) { sampleProcessTiming += (0xffffff - SysTick->VAL); sampleProcessTiming >>= 1; }
	else { sampleProcessTiming = (0xffffff - SysTick->VAL); }
    SysTick->CTRL = 0; /* Disable */
#endif

	// Clear the interrupt flag
#if INTERNAL_SAMPLING_SOURCE
	INTERNAL_SAMPLING_TIMER_NUM->intr = MXC_F_TMR_INTR_IRQ;
#elif EXTERNAL_SAMPLING_SOURCE
	MXC_GPIO_ClearFlags(GPIO_RTC_CLOCK_PORT, GPIO_RTC_CLOCK_PIN);
#endif
}
