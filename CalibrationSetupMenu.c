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
#include "Menu.h"
#include "OldUart.h"
#include "Display.h"
#include "Common.h"
#include "InitDataBuffers.h"
#include "Summary.h"
#include "SysEvents.h"
#include "Board.h"
#include "PowerManagement.h"
#include "Record.h"
#include "SoftTimer.h"
#include "Keypad.h"
#include "TextTypes.h"
#include "Analog.h"
#include "RealTimeClock.h"
#include "Sensor.h"
#include "stdlib.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define CAL_SETUP_MN_TABLE_SIZE 8
#define CAL_SETUP_WND_STARTING_COL DEFAULT_COL_THREE
#define CAL_SETUP_WND_END_COL DEFAULT_END_COL
#define CAL_SETUP_WND_STARTING_ROW DEFAULT_MENU_ROW_ZERO // DEFAULT_MENU_ROW_TWO
#define CAL_SETUP_WND_END_ROW (DEFAULT_MENU_ROW_SEVEN)
#define CAL_SETUP_MN_TBL_START_LINE 0

enum {
	CAL_MENU_DEFAULT_NON_CALIBRATED_DISPLAY,
	CAL_MENU_CALIBRATED_DISPLAY,
	CAL_MENU_CALIBRATED_CHAN_NOISE_PERCENT_DISPLAY,
	CAL_MENU_CALIBRATE_SENSOR,
	CAL_MENU_CALIBRATION_PULSE
};

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
#include "Globals.h"
extern USER_MENU_STRUCT helpMenu[];

///----------------------------------------------------------------------------
///	Local Scope Globals
///----------------------------------------------------------------------------
static uint32 s_calDisplayScreen = CAL_MENU_DEFAULT_NON_CALIBRATED_DISPLAY;
static uint32 s_calSavedSampleRate = 0;
static uint8 s_pauseDisplay = NO;

typedef struct {
	uint16 chan[4];
} CALIBRATION_DATA;

#if 1 /* Test */
static uint8_t cmState = OFF;
#endif

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------
void CalSetupMn(INPUT_MSG_STRUCT);
void CalSetupMnDsply(WND_LAYOUT_STRUCT*);
void CalSetupMnProc(INPUT_MSG_STRUCT, WND_LAYOUT_STRUCT*, MN_LAYOUT_STRUCT*);

#if 1 /* Test */
static uint8_t aCutoffState = ANALOG_CUTOFF_FREQ_500;
static uint8_t gainState = SEISMIC_GAIN_NORMAL;
static uint8_t pathState = ACOUSTIC_PATH_AOP;
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void CalSetupMn(INPUT_MSG_STRUCT msg)
{
	static WND_LAYOUT_STRUCT wnd_layout;
	static MN_LAYOUT_STRUCT mn_layout;
	static uint8 s_pausedDisplay = NO;
	volatile uint32 tempTicks = 0;
	//uint8 cursorLine = 0;
	volatile uint32 key = 0;
	uint8 mbChoice = 0;
	INPUT_MSG_STRUCT mn_msg;
	DATE_TIME_STRUCT tempTime;
	uint8 previousMode = g_triggerRecord.opMode;
	uint8 clearedFSRecord = NO;
	uint8 choice = MessageBox(getLangText(VERIFY_TEXT), getLangText(START_CALIBRATION_DIAGNOSTICS_Q_TEXT), MB_YESNO);

	if (choice == MB_FIRST_CHOICE)
	{
		CalSetupMnProc(msg, &wnd_layout, &mn_layout);
	}

	if (g_activeMenu == CAL_SETUP_MENU)
	{
		if (choice == MB_FIRST_CHOICE)
		{
			while ((key != ENTER_KEY) && (key != ESC_KEY))
			{
				// Check if a half second has gone by
				if (tempTicks != g_lifetimeHalfSecondTickCount)
				{
					// Update the current time since we never leave the loop
					UpdateCurrentTime();

					// Create the Cal Setup menu
					if (s_pausedDisplay == NO)
					{
						CalSetupMnDsply(&wnd_layout);
					}

#if 1 /* Test manual cyclic current average management */
					static uint32_t cyclicCheck = 0;
extern int32_t testLifetimeCurrentAvg;
extern uint32_t testLifetimeCurrentAvgCount;
					if ((!cyclicCheck) || (cyclicCheck == g_lifetimeHalfSecondTickCount))
					{
						testLifetimeCurrentAvg += (FuelGaugeGetCurrent() / 1000);
						testLifetimeCurrentAvgCount++;
						cyclicCheck = g_lifetimeHalfSecondTickCount + 8;
					}
#endif

#if 1 /* Test to see A/D output */
					if (g_lifetimeHalfSecondTickCount & 0x1)
					{
						//debug("CS A/D: R:%+5ld/%+5ld V:%+5ld/%+5ld T:%+5ld/%+5ld A:%+5ld/%+5ld\r\n", g_sensorCalChanMin[1], g_sensorCalChanMax[1], g_sensorCalChanMin[2], g_sensorCalChanMax[2], g_sensorCalChanMin[3], g_sensorCalChanMax[3], g_sensorCalChanMin[0], g_sensorCalChanMax[0]);
						float rCalc, tCalc, vCalc, xCalc, yCalc, zCalc, aCalc;
						rCalc = (float)10.24 / (float)32768 * (float)(abs(g_sensorCalChanMin[1]) > g_sensorCalChanMax[1] ? abs(g_sensorCalChanMin[1]) : g_sensorCalChanMax[1]);
						tCalc = (float)10.24 / (float)32768 * (float)(abs(g_sensorCalChanMin[3]) > g_sensorCalChanMax[3] ? abs(g_sensorCalChanMin[3]) : g_sensorCalChanMax[3]);
						vCalc = (float)10.24 / (float)32768 * (float)(abs(g_sensorCalChanMin[2]) > g_sensorCalChanMax[2] ? abs(g_sensorCalChanMin[2]) : g_sensorCalChanMax[2]);
						aCalc = (float)5.120 / (float)32768 * (float)(abs(g_sensorCalChanMin[0]) > g_sensorCalChanMax[0] ? abs(g_sensorCalChanMin[0]) : g_sensorCalChanMax[0]);
						xCalc = (float)8.000 / (float)32768 * (float)(abs(g_sensorCalChanMin[4]) > g_sensorCalChanMax[4] ? abs(g_sensorCalChanMin[4]) : g_sensorCalChanMax[4]);
						yCalc = (float)8.000 / (float)32768 * (float)(abs(g_sensorCalChanMin[5]) > g_sensorCalChanMax[5] ? abs(g_sensorCalChanMin[5]) : g_sensorCalChanMax[5]);
						zCalc = (float)8.000 / (float)32768 * (float)(abs(g_sensorCalChanMin[6]) > g_sensorCalChanMax[6] ? abs(g_sensorCalChanMin[6]) : g_sensorCalChanMax[6]);
						debug("CS A/D: R %2.3f T %2.3f V %2.3f (IPS), A %1.3f (mB), X %1.4f Y %1.4f Z %1.4f (G)\r\n", (double)(rCalc), (double)(tCalc), (double)(vCalc), (double)(aCalc), (double)(xCalc), (double)(yCalc), (double)(zCalc));
					}
#endif
					WriteMapToLcd(g_mmap);

#if 0 /* Exception testing (Prevent non-ISR soft loop watchdog from triggering) */
					g_execCycles++;
#endif
					// Set to current half second
					tempTicks = g_lifetimeHalfSecondTickCount;
				}

				key = GetKeypadKey(CHECK_ONCE_FOR_KEY);

				if (key == KEY_NONE) { SoftUsecWait(1 * SOFT_MSECS); } // Delay 1ms between key checks
				else /* Key press found */ { SoftUsecWait(250 * SOFT_MSECS); } // Generic delay to prevent processing the same key twice

#if 1 /* Test */
				// Check if the LCD Power was turned off
				if ((key != KEY_NONE) && (g_lcdPowerFlag == DISABLED)) { KeypressEventMgr(); key = KEY_NONE; }
#endif

#if 1 /* Test */
				char filterText[16];
#endif
				switch (key)
				{
					case UP_ARROW_KEY:
#if 1 /* Normal */
						if (s_calDisplayScreen == CAL_MENU_CALIBRATION_PULSE)
						{
							s_pauseDisplay = NO;
							g_calibrationGeneratePulse = NO;
							OverlayMessage(getLangText(STATUS_TEXT), getLangText(SENSOR_CALIBRATION_TEXT), (1 * SOFT_SECS));
							s_calDisplayScreen = CAL_MENU_CALIBRATE_SENSOR;
						}
						else if (s_calDisplayScreen == CAL_MENU_CALIBRATE_SENSOR)
						{
							s_pauseDisplay = NO;
							OverlayMessage(getLangText(STATUS_TEXT), getLangText(CHANNEL_NOISE_PERCENTAGES_TEXT), (1 * SOFT_SECS));
							s_calDisplayScreen = CAL_MENU_CALIBRATED_CHAN_NOISE_PERCENT_DISPLAY;
						}
						else if (s_calDisplayScreen == CAL_MENU_CALIBRATED_CHAN_NOISE_PERCENT_DISPLAY)
						{
							sprintf((char*)g_spareBuffer, "%s (%s) %s", getLangText(DISPLAY_CALIBRATED_TEXT), getLangText(ZERO_TEXT), getLangText(MIN_MAX_AVG_TEXT));
							OverlayMessage(getLangText(STATUS_TEXT), (char*)g_spareBuffer, (1 * SOFT_SECS));
							s_calDisplayScreen = CAL_MENU_CALIBRATED_DISPLAY;
						}
						else if (s_calDisplayScreen == CAL_MENU_CALIBRATED_DISPLAY)
						{
							// Clear the stored offsets so that the A/D channel data is raw
							memset(&g_channelOffset, 0, sizeof(g_channelOffset));
							
							// Clear the Pretrigger buffer
							SoftUsecWait(250 * SOFT_MSECS);
							
							sprintf((char*)g_spareBuffer, "%s (%s) %s", getLangText(DISPLAY_NOT_CALIBRATED_TEXT), getLangText(ZERO_TEXT), getLangText(MIN_MAX_AVG_TEXT));
							OverlayMessage(getLangText(STATUS_TEXT), (char*)g_spareBuffer, (1 * SOFT_SECS));
							s_calDisplayScreen = CAL_MENU_DEFAULT_NON_CALIBRATED_DISPLAY;
						}

						key = 0;
#else /* Test */
						if (scEnable == ON) { scEnable = OFF; } else { scEnable = ON; }
						if (scEnable) { PowerControl(SENSOR_CHECK_ENABLE, ON); sprintf((char*)g_debugBuffer, "Cal Setup: Sensor Check Enabled"); }
						else{ PowerControl(SENSOR_CHECK_ENABLE, OFF); sprintf((char*)g_debugBuffer, "Cal Setup: Sensor Check Disabled"); }
						debug("%s\r\n", (char*)g_debugBuffer);
						OverlayMessage(getLangText(STATUS_TEXT), (char*)g_debugBuffer, (2 * SOFT_SECS));
#endif
						break;

					case DOWN_ARROW_KEY:
#if 1 /* Normal */
						if (s_calDisplayScreen == CAL_MENU_DEFAULT_NON_CALIBRATED_DISPLAY)
						{
							// Stop A/D data collection clock
#if INTERNAL_SAMPLING_SOURCE
							StopInteralSampleTimer();
#elif EXTERNAL_SAMPLING_SOURCE
							StopExternalRtcClock();
#endif
							
							// Alert the system operator that the unit is calibrating
							OverlayMessage(getLangText(STATUS_TEXT), getLangText(CALIBRATING_TEXT), 0);
							
							// Get new channel offsets
							GetChannelOffsets(CALIBRATION_FIXED_SAMPLE_RATE);
							
							// Restart the data collection clock
#if INTERNAL_SAMPLING_SOURCE
							StartInteralSampleTimer();
#elif EXTERNAL_SAMPLING_SOURCE
							StartExternalRtcClock(CALIBRATION_FIXED_SAMPLE_RATE);
#endif
							
							// Clear the Pretrigger buffer
							SoftUsecWait(250 * SOFT_MSECS);

							s_calDisplayScreen = CAL_MENU_CALIBRATED_DISPLAY;
						}
						else if (s_calDisplayScreen == CAL_MENU_CALIBRATED_DISPLAY)
						{
							OverlayMessage(getLangText(STATUS_TEXT), getLangText(CHANNEL_NOISE_PERCENTAGES_TEXT), (1 * SOFT_SECS));
							s_calDisplayScreen = CAL_MENU_CALIBRATED_CHAN_NOISE_PERCENT_DISPLAY;
						}
						else if (s_calDisplayScreen == CAL_MENU_CALIBRATED_CHAN_NOISE_PERCENT_DISPLAY)
						{
							OverlayMessage(getLangText(STATUS_TEXT), getLangText(SENSOR_CALIBRATION_TEXT), (1 * SOFT_SECS));
							s_pauseDisplay = NO;
							s_calDisplayScreen = CAL_MENU_CALIBRATE_SENSOR;
						}
						else if (s_calDisplayScreen == CAL_MENU_CALIBRATE_SENSOR)
						{
							OverlayMessage(getLangText(STATUS_TEXT), getLangText(SENSOR_CAL_PULSE_TEXT), (1 * SOFT_SECS));
							s_pauseDisplay = NO;
							g_calibrationGeneratePulse = YES;
							s_calDisplayScreen = CAL_MENU_CALIBRATION_PULSE;
						}

						key = 0;
#else /* Test */
						if (scState == ON) { scState = OFF; } else { scState = ON; }
						if (scState) { SetSensorCheckState(ON); sprintf((char*)g_debugBuffer, "Cal Setup: Sensor Check state High"); }
						else { SetSensorCheckState(OFF); sprintf((char*)g_debugBuffer, "Cal Setup: Sensor Check state Low"); }
						debug("%s\r\n", (char*)g_debugBuffer);
						OverlayMessage(getLangText(STATUS_TEXT), (char*)g_debugBuffer, (2 * SOFT_SECS));
#endif
						break;

					case (RIGHT_ARROW_KEY):
#if 0 /* Normal */
						if (g_calDisplayAlternateResultState == DEFAULT_RESULTS) { g_calDisplayAlternateResultState = DEFAULT_ALTERNATE_RESULTS; }
						else { g_calDisplayAlternateResultState = DEFAULT_RESULTS; }
#else
						if (GetPowerOnButtonState() == ON)
						{
							sprintf((char*)g_debugBuffer, "Cal Setup: Geo1 + Geo2 Sensor disabled");
							MXC_GPIO_OutClr(GPIO_SENSOR_ENABLE_GEO1_PORT, GPIO_SENSOR_ENABLE_GEO1_PIN);
							MXC_GPIO_OutClr(GPIO_SENSOR_ENABLE_GEO2_PORT, GPIO_SENSOR_ENABLE_GEO2_PIN);
						}
						else
						if (g_currentSensorGroup == SENSOR_GROUP_A_1)
						{
							sprintf((char*)g_debugBuffer, "Cal Setup: Geo1 Sensor disabled");
							MXC_GPIO_OutClr(GPIO_SENSOR_ENABLE_GEO1_PORT, GPIO_SENSOR_ENABLE_GEO1_PIN);
						}
						else if (g_currentSensorGroup == SENSOR_GROUP_B_2)
						{
							sprintf((char*)g_debugBuffer, "Cal Setup: Geo2 Sensor disabled");
							MXC_GPIO_OutClr(GPIO_SENSOR_ENABLE_GEO2_PORT, GPIO_SENSOR_ENABLE_GEO2_PIN);
						}
						else // (g_currentSensorGroup == SENSOR_GROUP_BOTH)
						{
							sprintf((char*)g_debugBuffer, "Cal Setup: Geo1 + Geo2 Sensor disabled");
							MXC_GPIO_OutClr(GPIO_SENSOR_ENABLE_GEO1_PORT, GPIO_SENSOR_ENABLE_GEO1_PIN);
							MXC_GPIO_OutClr(GPIO_SENSOR_ENABLE_GEO2_PORT, GPIO_SENSOR_ENABLE_GEO2_PIN);
						}

						debug("%s\r\n", (char*)g_debugBuffer);
						OverlayMessage(getLangText(STATUS_TEXT), (char*)g_debugBuffer, (2 * SOFT_SECS));
#endif
						break;

					case (LEFT_ARROW_KEY):
#if 0 /* Normal */
						if (g_displayAlternateResultState == DEFAULT_ALTERNATE_RESULTS) { g_displayAlternateResultState = DEFAULT_RESULTS; }
						else { g_displayAlternateResultState = DEFAULT_ALTERNATE_RESULTS; }
#else
						if (GetPowerOnButtonState() == ON)
						{
							sprintf((char*)g_debugBuffer, "Cal Setup: AOP1 + AOP2 Sensor disabled");
							MXC_GPIO_OutClr(GPIO_SENSOR_ENABLE_AOP1_PORT, GPIO_SENSOR_ENABLE_AOP1_PIN);
							MXC_GPIO_OutClr(GPIO_SENSOR_ENABLE_AOP2_PORT, GPIO_SENSOR_ENABLE_AOP2_PIN);
						}
						else
						if (g_currentSensorGroup == SENSOR_GROUP_A_1)
						{
#if 0 /* Original, Geo + AOP on one sensor group */
							sprintf((char*)g_debugBuffer, "Cal Setup: AOP1 Sensor disabled");
							MXC_GPIO_OutClr(GPIO_SENSOR_ENABLE_AOP1_PORT, GPIO_SENSOR_ENABLE_AOP1_PIN);
#else /* Standard config, Geo + Aop on separate sensors */
							sprintf((char*)g_debugBuffer, "Cal Setup: AOP2 Sensor disabled");
							MXC_GPIO_OutClr(GPIO_SENSOR_ENABLE_AOP2_PORT, GPIO_SENSOR_ENABLE_AOP2_PIN);
#endif
						}
						else if (g_currentSensorGroup == SENSOR_GROUP_B_2)
						{
#if 0 /* Original, Geo + AOP on one sensor group */
							sprintf((char*)g_debugBuffer, "Cal Setup: AOP2 Sensor disabled");
							MXC_GPIO_OutClr(GPIO_SENSOR_ENABLE_AOP2_PORT, GPIO_SENSOR_ENABLE_AOP2_PIN);
#else /* Standard config, Geo + Aop on separate sensors */
							sprintf((char*)g_debugBuffer, "Cal Setup: AOP1 Sensor disabled");
							MXC_GPIO_OutClr(GPIO_SENSOR_ENABLE_AOP1_PORT, GPIO_SENSOR_ENABLE_AOP1_PIN);
#endif
						}
						else // (g_currentSensorGroup == SENSOR_GROUP_BOTH)
						{
							sprintf((char*)g_debugBuffer, "Cal Setup: AOP1 + AOP2 Sensor disabled");
							MXC_GPIO_OutClr(GPIO_SENSOR_ENABLE_AOP1_PORT, GPIO_SENSOR_ENABLE_AOP1_PIN);
							MXC_GPIO_OutClr(GPIO_SENSOR_ENABLE_AOP2_PORT, GPIO_SENSOR_ENABLE_AOP2_PIN);
						}

						debug("%s\r\n", (char*)g_debugBuffer);
						OverlayMessage(getLangText(STATUS_TEXT), (char*)g_debugBuffer, (2 * SOFT_SECS));
#endif
						break;

					case HELP_KEY:
#if 0 /* Normal */
						if (s_pauseDisplay == NO) { s_pauseDisplay = YES; }
						else { s_pauseDisplay = NO; }
#else
						if (GetPowerOnButtonState() == ON)
						{
							sprintf((char*)g_debugBuffer, "Cal Setup: Geo1 + Geo2 + AOP1 + AOP2 Sensors enabled");
							MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_GEO1_PORT, GPIO_SENSOR_ENABLE_GEO1_PIN);
							MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_AOP1_PORT, GPIO_SENSOR_ENABLE_AOP1_PIN);
							MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_GEO2_PORT, GPIO_SENSOR_ENABLE_GEO2_PIN);
							MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_AOP2_PORT, GPIO_SENSOR_ENABLE_AOP2_PIN);
						}
						else
						if (g_currentSensorGroup == SENSOR_GROUP_A_1)
						{
#if 0 /* Original, Geo + AOP on one sensor group */
							sprintf((char*)g_debugBuffer, "Cal Setup: Geo1 + AOP1 Sensors enabled");
							MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_GEO1_PORT, GPIO_SENSOR_ENABLE_GEO1_PIN);
							MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_AOP1_PORT, GPIO_SENSOR_ENABLE_AOP1_PIN);
#else /* Standard config, Geo + Aop on separate sensors */
							sprintf((char*)g_debugBuffer, "Cal Setup: Geo1 + AOP2 Sensors enabled");
							MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_GEO1_PORT, GPIO_SENSOR_ENABLE_GEO1_PIN);
							MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_AOP2_PORT, GPIO_SENSOR_ENABLE_AOP2_PIN);
#endif
						}
						else if (g_currentSensorGroup == SENSOR_GROUP_B_2)
						{
#if 0 /* Original, Geo + AOP on one sensor group */
							sprintf((char*)g_debugBuffer, "Cal Setup: Geo2 + AOP2 Sensors enabled");
							MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_GEO2_PORT, GPIO_SENSOR_ENABLE_GEO2_PIN);
							MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_AOP2_PORT, GPIO_SENSOR_ENABLE_AOP2_PIN);
#else /* Standard config, Geo + Aop on separate sensors */
							sprintf((char*)g_debugBuffer, "Cal Setup: Geo2 + AOP1 Sensors enabled");
							MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_GEO2_PORT, GPIO_SENSOR_ENABLE_GEO2_PIN);
							MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_AOP1_PORT, GPIO_SENSOR_ENABLE_AOP1_PIN);
#endif
						}
						else // (g_currentSensorGroup == SENSOR_GROUP_BOTH)
						{
							sprintf((char*)g_debugBuffer, "Cal Setup: Geo1 + Geo2 + AOP1 + AOP2 Sensors enabled");
							MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_GEO1_PORT, GPIO_SENSOR_ENABLE_GEO1_PIN);
							MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_AOP1_PORT, GPIO_SENSOR_ENABLE_AOP1_PIN);
							MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_GEO2_PORT, GPIO_SENSOR_ENABLE_GEO2_PIN);
							MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_AOP2_PORT, GPIO_SENSOR_ENABLE_AOP2_PIN);
						}

						debug("%s\r\n", (char*)g_debugBuffer);
						OverlayMessage(getLangText(STATUS_TEXT), (char*)g_debugBuffer, (2 * SOFT_SECS));
#endif
						break;

					case KB_SK_1:
#if 1 /* Test */
						if (GetPowerOnButtonState() == ON)
						{
							LcdPwTimerCallBack();
							SoftUsecWait(1 * SOFT_SECS);
							break;
						}
#endif
#if 1 /* Test */
						if (aCutoffState == ANALOG_CUTOFF_FREQ_500) { aCutoffState = ANALOG_CUTOFF_FREQ_1K; strcpy(filterText, "1K"); }
						else if (aCutoffState == ANALOG_CUTOFF_FREQ_1K) { aCutoffState = ANALOG_CUTOFF_FREQ_2K; strcpy(filterText, "2K"); }
						else if (aCutoffState == ANALOG_CUTOFF_FREQ_2K) { aCutoffState = ANALOG_CUTOFF_FREQ_4K; strcpy(filterText, "4K"); }
						else if (aCutoffState == ANALOG_CUTOFF_FREQ_4K) { aCutoffState = ANALOG_CUTOFF_FREQ_8K; strcpy(filterText, "8K"); }
						else if (aCutoffState == ANALOG_CUTOFF_FREQ_8K) { aCutoffState = ANALOG_CUTOFF_FREQ_500; strcpy(filterText, "500"); }
						//else if (aCutoffState == ANALOG_CUTOFF_FREQ_16K) { aCutoffState = ANALOG_CUTOFF_FREQ_1K; strcpy(filterText, "1K"); }
						sprintf((char*)g_debugBuffer, "Cal Setup: Changing Nyquist filter (%s)", filterText);
						debug("%s\r\n", (char*)g_debugBuffer);
						OverlayMessage(getLangText(STATUS_TEXT), (char*)g_debugBuffer, (2 * SOFT_SECS));
						SetAnalogCutoffFrequency(aCutoffState);
#endif
						break;
					case KB_SK_2:
#if 1 /* Test */
						if (GetPowerOnButtonState() == ON)
						{
							if (gainState == SEISMIC_GAIN_NORMAL) { gainState = SEISMIC_GAIN_HIGH; } else { gainState = SEISMIC_GAIN_NORMAL; }
							if ((g_currentSensorGroup == SENSOR_GROUP_A_1) || (g_currentSensorGroup == SENSOR_GROUP_BOTH))
							{
								if (gainState == SEISMIC_GAIN_NORMAL){ strcpy(filterText, "Normal"); SetGainGeo1State(HIGH); } else { strcpy(filterText, "High"); SetGainGeo1State(LOW); }
								sprintf((char*)g_debugBuffer, "Cal Setup: Changing Geo1 gain (%s)", filterText);
							}
							else if (g_currentSensorGroup == SENSOR_GROUP_B_2)
							{
								if (gainState == SEISMIC_GAIN_NORMAL){ strcpy(filterText, "Normal"); SetGainGeo2State(HIGH); } else { strcpy(filterText, "High"); SetGainGeo2State(LOW); }
								sprintf((char*)g_debugBuffer, "Cal Setup: Changing Geo2 gain (%s)", filterText);
							}
						}
						else
						{
							if (pathState == ACOUSTIC_PATH_AOP) { pathState = ACOUSTIC_PATH_A_WEIGHTED; } else { pathState = ACOUSTIC_PATH_AOP; }
							if ((g_currentSensorGroup == SENSOR_GROUP_A_1) || (g_currentSensorGroup == SENSOR_GROUP_BOTH))
							{
#if 0 /* Original, Geo + AOP on one sensor group */
								if (pathState == ACOUSTIC_PATH_AOP) { strcpy(filterText, "AOP"); SetPathSelectAop1State(HIGH); } else { strcpy(filterText, "C-weight"); SetPathSelectAop1State(LOW); }
								sprintf((char*)g_debugBuffer, "Cal Setup: Changing AOP1 path (%s)", filterText);
#else /* Standard config, Geo + Aop on separate sensors */
								if (pathState == ACOUSTIC_PATH_AOP) { strcpy(filterText, "AOP"); SetPathSelectAop2State(HIGH); } else { strcpy(filterText, "A-weight"); SetPathSelectAop2State(LOW); }
								sprintf((char*)g_debugBuffer, "Cal Setup: Changing AOP2 path (%s)", filterText);
#endif
							}
							else if (g_currentSensorGroup == SENSOR_GROUP_B_2)
							{
#if 0 /* Original, Geo + AOP on one sensor group */
								if (pathState == ACOUSTIC_PATH_AOP) { strcpy(filterText, "AOP"); SetPathSelectAop2State(HIGH); } else { strcpy(filterText, "A-weight"); SetPathSelectAop2State(LOW); }
								sprintf((char*)g_debugBuffer, "Cal Setup: Changing AOP2 path (%s)", filterText);
#else /* Standard config, Geo + Aop on separate sensors */
								if (pathState == ACOUSTIC_PATH_AOP) { strcpy(filterText, "AOP"); SetPathSelectAop1State(HIGH); } else { strcpy(filterText, "C-weight"); SetPathSelectAop1State(LOW); }
								sprintf((char*)g_debugBuffer, "Cal Setup: Changing AOP1 path (%s)", filterText);
#endif
							}
						}

						debug("%s\r\n", (char*)g_debugBuffer);
						OverlayMessage(getLangText(STATUS_TEXT), (char*)g_debugBuffer, (2 * SOFT_SECS));
#endif
						break;

#if 0 /* Fill in with correct control, maybe a soft key named swap */
					case /* Which key? */:
						if (GetCalMuxPreADSelectState() == CAL_MUX_SELECT_SENSOR_GROUP_A)
						{
							SetCalMuxPreADSelectState(CAL_MUX_SELECT_SENSOR_GROUP_B);
							OverlayMessage(getLangText(STATUS_TEXT), "SWAPPED TO SENSOR GROUP B (GEO2 + AOP2)", (1 * SOFT_SECS));
						}
						else // Currently sensor group B
						{
							SetCalMuxPreADSelectState(CAL_MUX_SELECT_SENSOR_GROUP_A);
							OverlayMessage(getLangText(STATUS_TEXT), "SWAPPED TO SENSOR GROUP A (GEO1 + AOP1)", (1 * SOFT_SECS));
						}
						break;
#endif

					case KB_SK_4:
#if 1 /* Test */
						if (GetPowerOnButtonState() == ON)
						{
							if (g_lcdBacklightFlag == ENABLED)
							{
								sprintf((char*)g_debugBuffer, "Cal Setup: Turning Backlight Off");
								debug("%s\r\n", (char*)g_debugBuffer);
								OverlayMessage(getLangText(STATUS_TEXT), (char*)g_debugBuffer, (2 * SOFT_SECS));
								SetLcdBacklightState(BACKLIGHT_OFF);
								g_lcdBacklightFlag = DISABLED;
							}
							else
							{
								SetLcdBacklightState(BACKLIGHT_MID);
								g_lcdBacklightFlag = ENABLED;
								sprintf((char*)g_debugBuffer, "Cal Setup: Turning Backlight On");
								debug("%s\r\n", (char*)g_debugBuffer);
								OverlayMessage(getLangText(STATUS_TEXT), (char*)g_debugBuffer, (2 * SOFT_SECS));
							}
						}
						else
#if 0 /* Original */
						if (cmState) // Check if on
						{
							cmState = OFF; SetCalMuxPreADEnableState(OFF);
							sprintf((char*)g_debugBuffer, "Cal Setup: Turning Cal Mux Enable Off");
							debug("%s\r\n", (char*)g_debugBuffer);
							OverlayMessage(getLangText(STATUS_TEXT), (char*)g_debugBuffer, (2 * SOFT_SECS));
						}
						else // Off, turn on the selected group
						{
							if (g_currentSensorGroup == SENSOR_GROUP_A_1)
							{
								cmState = CAL_MUX_SELECT_SENSOR_GROUP_A; SetCalMuxPreADSelectState(CAL_MUX_SELECT_SENSOR_GROUP_A); SetCalMuxPreADEnableState(ON);
								sprintf((char*)g_debugBuffer, "Cal Setup: Enabling Cal Mux Sensor Group A/1");
								debug("%s\r\n", (char*)g_debugBuffer);
								OverlayMessage(getLangText(STATUS_TEXT), (char*)g_debugBuffer, (2 * SOFT_SECS));
							}
							else if (g_currentSensorGroup == SENSOR_GROUP_B_2)
							{
								cmState = CAL_MUX_SELECT_SENSOR_GROUP_B; SetCalMuxPreADSelectState(CAL_MUX_SELECT_SENSOR_GROUP_B); SetCalMuxPreADEnableState(ON);
								sprintf((char*)g_debugBuffer, "Cal Setup: Enabling Cal Mux Sensor Group B/2");
								debug("%s\r\n", (char*)g_debugBuffer);
								OverlayMessage(getLangText(STATUS_TEXT), (char*)g_debugBuffer, (2 * SOFT_SECS));
							}
							else // (g_currentSensorGroup == SENSOR_GROUP_BOTH)
							{
								cmState = CAL_MUX_SELECT_SENSOR_GROUP_A; SetCalMuxPreADSelectState(CAL_MUX_SELECT_SENSOR_GROUP_A); SetCalMuxPreADEnableState(ON);
								sprintf((char*)g_debugBuffer, "Cal Setup: Enabling Cal Mux Sensor Group A/1");
								debug("%s\r\n", (char*)g_debugBuffer);
								OverlayMessage(getLangText(STATUS_TEXT), (char*)g_debugBuffer, (2 * SOFT_SECS));
							}
						}
#else /* Cycle through CM states */
						{
							if (cmState == OFF)
							{
								cmState = CAL_MUX_SELECT_SENSOR_GROUP_A; SetCalMuxPreADSelectState(CAL_MUX_SELECT_SENSOR_GROUP_A); SetCalMuxPreADEnableState(ON);
								sprintf((char*)g_debugBuffer, "Cal Setup: Enabling Cal Mux Sensor Group A/1");
								debug("%s\r\n", (char*)g_debugBuffer);
								OverlayMessage(getLangText(STATUS_TEXT), (char*)g_debugBuffer, (2 * SOFT_SECS));
							}
							else if (cmState == CAL_MUX_SELECT_SENSOR_GROUP_A)
							{
								cmState = CAL_MUX_SELECT_SENSOR_GROUP_B; SetCalMuxPreADSelectState(CAL_MUX_SELECT_SENSOR_GROUP_B); SetCalMuxPreADEnableState(ON);
								sprintf((char*)g_debugBuffer, "Cal Setup: Enabling Cal Mux Sensor Group B/2");
								debug("%s\r\n", (char*)g_debugBuffer);
								OverlayMessage(getLangText(STATUS_TEXT), (char*)g_debugBuffer, (2 * SOFT_SECS));
							}
							else // (cmState == CAL_MUX_SELECT_SENSOR_GROUP_B)
							{
								cmState = OFF; SetCalMuxPreADEnableState(OFF);
								sprintf((char*)g_debugBuffer, "Cal Setup: Turning Cal Mux Enable Off");
								debug("%s\r\n", (char*)g_debugBuffer);
								OverlayMessage(getLangText(STATUS_TEXT), (char*)g_debugBuffer, (2 * SOFT_SECS));
							}
						}
#endif
#endif
						break;

					case ENTER_KEY:
					case ESC_KEY:
						// Not really used, keys are grabbed in CalSetupMn
						mbChoice = MessageBox(getLangText(STATUS_TEXT), getLangText(ARE_YOU_DONE_WITH_CAL_SETUP_Q_TEXT), MB_YESNO);
						if (mbChoice != MB_FIRST_CHOICE)
						{
							// Don't leave
							key = 0;
						}
						break;
				}
			}

			StopADDataCollectionForCalibration();
#if 1 /* Test */
			StopAccelerometerAquisition();
#endif
			// Reset the current sensor group selection to the default
			g_currentSensorGroup = SENSOR_GROUP_A_1;

			// Reestablish the previously stored sample rate
			g_triggerRecord.trec.sample_rate = s_calSavedSampleRate;

			// Check if going through Factory Setup to allow the following operations
			if (g_factorySetupSequence != SEQ_NOT_STARTED)
			{
				if (MessageBox(getLangText(CONFIRM_TEXT), getLangText(DO_YOU_WANT_TO_SAVE_THE_CAL_DATE_Q_TEXT), MB_YESNO) == MB_FIRST_CHOICE)
				{
					// Store Calibration Date
					tempTime = GetCurrentTime();
					ConvertDateTimeToCalDate(&g_factorySetupRecord.calDate, &tempTime);
				}
				// Check if no Smart sensor is connected
				else if (CheckIfNoSmartSensorsPresent() == YES)
				{
					if (MessageBox(getLangText(CONFIRM_TEXT), getLangText(ERASE_FACTORY_SETUP_Q_TEXT), MB_YESNO) == MB_FIRST_CHOICE)
					{
						if (MessageBox(getLangText(CONFIRM_TEXT), getLangText(ALSO_ERASE_THE_REST_OF_THE_EEPROM_Q_TEXT), MB_YESNO) == MB_FIRST_CHOICE)
						{
							// Erase entire EEPROM
							memset(g_spareBuffer, 0xFF, sizeof(g_spareBuffer));
							SaveParameterMemory(g_spareBuffer, 0, EEPROM_AT25640_TOTAL_BYTES);
						}
						else
						{
							memset(&g_factorySetupRecord, 0xFF, sizeof(g_factorySetupRecord));
							SaveRecordData(&g_factorySetupRecord, DEFAULT_RECORD, REC_FACTORY_SETUP_CLEAR_TYPE);
						}

						if (MessageBox(getLangText(CONFIRM_TEXT), getLangText(ERASE_FACTORY_SETUP_SHADOW_COPY_Q_TEXT), MB_YESNO) == MB_FIRST_CHOICE)
						{
							// Erase Factory Setup shadow copy in the Flash User Page
							EraseFlashUserPageFactorySetup();
						}

						clearedFSRecord = YES;
					}

					if (MessageBox(getLangText(CONFIRM_TEXT), getLangText(ERASE_ALL_NON_ESSENTIAL_SYSTEM_FILES_Q_TEXT), MB_YESNO) == MB_FIRST_CHOICE)
					{
						// Delete Non-Essential files
						DeleteNonEssentialFiles();
					}
				}
			}
		}

		// Check if going through Factory Setup to allow the following operations
		if (g_factorySetupSequence != SEQ_NOT_STARTED)
		{
			// Check that the Factory setup wasn't cleared
			if (clearedFSRecord == NO)
			{
				SaveRecordData(&g_factorySetupRecord, DEFAULT_RECORD, REC_FACTORY_SETUP_TYPE);
#if 0 /* Original */
				SaveFlashUserPageFactorySetup(&g_factorySetupRecord);
#else /* EEPROM user page currently not accessible */
#endif
				UpdateUnitSensorsWithSmartSensorTypes();
#if 1 /* Test reporting the Seismic and Air sensor types */
				debug("CS Sensor Types Selected: %0.2f Seismic, %x Air\r\n", (double)((float)g_factorySetupRecord.seismicSensorType / (float)204.8), g_factorySetupRecord.acousticSensorType);
#endif
				LoadTrigRecordDefaults(&g_triggerRecord, WAVEFORM_MODE);
				SaveRecordData(&g_triggerRecord, DEFAULT_RECORD, REC_TRIGGER_USER_MENU_TYPE);
			}

			UpdateWorkingCalibrationDate();
			MessageBox(getLangText(STATUS_TEXT), getLangText(FACTORY_SETUP_COMPLETE_TEXT), MB_OK);
		}

		g_factorySetupSequence = SEQ_NOT_STARTED;

		// Disable the Cal Mux
		SetCalMuxPreADEnableState(OFF);

		// Restore the previous mode
		g_triggerRecord.opMode = previousMode;

		// Reset display state
		g_calDisplayAlternateResultState = DEFAULT_RESULTS;
		g_displayAlternateResultState = DEFAULT_RESULTS;

		// Reset default screen to non calibrated
		//debug("Cal Menu Screen 1 selected\r\n");
		s_calDisplayScreen = CAL_MENU_DEFAULT_NON_CALIBRATED_DISPLAY;

#if 1 /* Test revert soft keys */
		g_keypadTable[SOFT_KEY_1] = LCD_OFF_KEY;
		g_keypadTable[SOFT_KEY_2] = BACKLIGHT_KEY;
		g_keypadTable[SOFT_KEY_4] = ESC_KEY;
#endif
		SETUP_MENU_MSG(MAIN_MENU);
		JUMP_TO_ACTIVE_MENU();
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void CalSetupMnProc(INPUT_MSG_STRUCT msg,
	WND_LAYOUT_STRUCT *wnd_layout_ptr, MN_LAYOUT_STRUCT *mn_layout_ptr)
{
	INPUT_MSG_STRUCT mn_msg;
	uint32 data;
	uint8 mbChoice;

	switch (msg.cmd)
	{
		case (ACTIVATE_MENU_CMD):
			wnd_layout_ptr->start_col = CAL_SETUP_WND_STARTING_COL;
			wnd_layout_ptr->end_col = CAL_SETUP_WND_END_COL;
			wnd_layout_ptr->start_row = CAL_SETUP_WND_STARTING_ROW;
			wnd_layout_ptr->end_row = CAL_SETUP_WND_END_ROW;
			mn_layout_ptr->curr_ln = CAL_SETUP_MN_TBL_START_LINE;
			mn_layout_ptr->top_ln = CAL_SETUP_MN_TBL_START_LINE;

			// Enable the Cal Mux for Sensor group A (Geo1 + AOP1)
#if 1 /* Prompt for Cal Mux choice */
			mbChoice = MessageBox(getLangText(SELECT_TEXT), "TEST WITH SENSOR GROUP A/1? NO WILL SWAP TO SENSOR GROUP B/2", MB_YESNO);
			if (mbChoice == MB_FIRST_CHOICE) { g_currentSensorGroup = SENSOR_GROUP_A_1; }
			else if (mbChoice == MB_SECOND_CHOICE) { g_currentSensorGroup = SENSOR_GROUP_B_2; }
			else /* (mbChoice == MB_SPECIAL_ACTION) */ { g_currentSensorGroup = SENSOR_GROUP_BOTH; }
#endif
			OverlayMessage(getLangText(STATUS_TEXT), getLangText(PLEASE_BE_PATIENT_TEXT), 0);

			// Save the currently stored sample rate to later be reverted
			s_calSavedSampleRate = g_triggerRecord.trec.sample_rate;
			
			// Set the sample rate to a fixed 1K
			g_triggerRecord.trec.sample_rate = SAMPLE_RATE_1K;

#if 1 /* Test starting the Accelerometer */
			StartAccelerometerAquisition();
#endif
			// Fool system and initialize buffers and pointers as if a waveform
			InitDataBuffs(WAVEFORM_MODE);

			// Clear out Sensor Cal data structures
			SensorCalibrationDataInit();

			// Hand setup A/D data collection and start the data clock
#if 1 /* Normal */
			StartADDataCollectionForCalibration(CALIBRATION_FIXED_SAMPLE_RATE);
#else /* Test */
			StartADDataCollectionForCalibration(SAMPLE_RATE_8K);
#endif

#if 1 /* Test addition */
			// Reset static state on for re-entry
			aCutoffState = ANALOG_CUTOFF_FREQ_500;
			gainState = SEISMIC_GAIN_NORMAL;
			pathState = ACOUSTIC_PATH_AOP;
#endif

			// Set the Cal Mux to the correct sensor group
			if (g_currentSensorGroup == SENSOR_GROUP_A_1) { SetCalMuxPreADSelectState(CAL_MUX_SELECT_SENSOR_GROUP_A); cmState = CAL_MUX_SELECT_SENSOR_GROUP_A; }
			else if (g_currentSensorGroup == SENSOR_GROUP_B_2) { SetCalMuxPreADSelectState(CAL_MUX_SELECT_SENSOR_GROUP_B); cmState = CAL_MUX_SELECT_SENSOR_GROUP_B; }
			else /* (g_currentSensorGroup == SENSOR_GROUP_BOTH) */ { SetCalMuxPreADSelectState(CAL_MUX_SELECT_SENSOR_GROUP_A); cmState = CAL_MUX_SELECT_SENSOR_GROUP_A; } // Just use A/1
			SetCalMuxPreADEnableState(ON);

			// Clear channel offsets since uncalibrated is the first menu
			memset(&g_channelOffset, 0, sizeof(g_channelOffset));
			
			// Allow 1 second of data processing
			SoftUsecWait(1 * SOFT_SECS);

			// Set display for Air to represent MB by default
			g_displayAlternateResultState = DEFAULT_RESULTS;
		break;

		case (KEYPRESS_MENU_CMD):
			data = msg.data[0];

			switch (data)
			{
				case (ENTER_KEY):
					break;

				case (ESC_KEY):
					mbChoice = MessageBox(getLangText(WARNING_TEXT), getLangText(DO_YOU_WANT_TO_LEAVE_CAL_SETUP_MODE_Q_TEXT), MB_YESNO);
					if (mbChoice != MB_FIRST_CHOICE)
					{
						// Dont leave, escape to continue
						break;
					}

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

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void CalSetupMnDsply(WND_LAYOUT_STRUCT *wnd_layout_ptr)
{
	uint8 buff[50];
	uint8 buff1[12], buff2[12], buff3[12];
	uint8 length;
	DATE_TIME_STRUCT time;
	float div, r1, r2, r3, v1, v2, v3, t1, t2, t3;
	uint16 sensorType = g_factorySetupRecord.seismicSensorType;
	uint8 acousticSensorType = g_factorySetupRecord.acousticSensorType;

	wnd_layout_ptr->curr_row = wnd_layout_ptr->start_row;
	wnd_layout_ptr->curr_col = wnd_layout_ptr->start_col;
	wnd_layout_ptr->next_row = wnd_layout_ptr->start_row;
	wnd_layout_ptr->next_col = wnd_layout_ptr->start_col;

#if 0 /* Old */
	uint8 gainFactor = (uint8)((g_triggerRecord.srec.sensitivity == LOW) ? 2 : 4);
	div = (float)(g_bitAccuracyMidpoint * g_sensorInfo.sensorAccuracy * gainFactor) / (float)(g_factorySetupRecord.seismicSensorType);
#else
	if (g_currentSensorGroup == SENSOR_GROUP_A_1)
	{
		// Check if the Seismic 1 Smart Sensor if found
		if (g_seismicSmartSensorMemory.version & SMART_SENSOR_OVERLAY_KEY)
		{
			sensorType = (pow(2, g_seismicSmartSensorMemory.sensorType) * SENSOR_2_5_IN);
		}

		// Check if the Acoustic 2 Smart Sensor if found
		if ((g_acoustic2SmartSensorMemory.version & SMART_SENSOR_OVERLAY_KEY) && CheckIfAcousticSensorTypeValid(g_acoustic2SmartSensorMemory.sensorType))
		{
			acousticSensorType = g_acoustic2SmartSensorMemory.sensorType;
		}
	}
	else // g_currentSensorGroup == SENSOR_GROUP_B_2
	{
		// Check if the Seismic 2 Smart Sensor if found
		if (g_seismic2SmartSensorMemory.version & SMART_SENSOR_OVERLAY_KEY)
		{
			sensorType = (pow(2, g_seismic2SmartSensorMemory.sensorType) * SENSOR_2_5_IN);
		}

		// Check if the Acoustic 1 Smart Sensor if found
		if ((g_acousticSmartSensorMemory.version & SMART_SENSOR_OVERLAY_KEY) && CheckIfAcousticSensorTypeValid(g_acousticSmartSensorMemory.sensorType))
		{
			acousticSensorType = g_acousticSmartSensorMemory.sensorType;
		}
	}

	if (g_unitConfig.unitsOfMeasure == METRIC_TYPE)
	{
		div = (float)(ACCURACY_16_BIT_MIDPOINT * g_sensorInfo.sensorAccuracy * 2) / (float)(sensorType * METRIC);
	}
	else
	{
		div = (float)(ACCURACY_16_BIT_MIDPOINT * g_sensorInfo.sensorAccuracy * 2) / (float)(sensorType);
	}
#endif

	// Allow the display to be maintained on the screen if in calibrate sensor menu and pause display selected
	if (((s_calDisplayScreen == CAL_MENU_CALIBRATE_SENSOR) || (s_calDisplayScreen == CAL_MENU_CALIBRATION_PULSE)) && (s_pauseDisplay == YES))
	{
		memset(&(g_mmap[1][0]), 0, (sizeof(g_mmap) / 8));
		memset(&(g_mmap[2][0]), 0, (sizeof(g_mmap) / 8));
		//memset(&(g_mmap[7][0]), 0, (sizeof(g_mmap) / 8));
	}
	else // Clear the whole map
	{
		ClearLcdMap();
	}

	if ((s_calDisplayScreen == CAL_MENU_DEFAULT_NON_CALIBRATED_DISPLAY) || (s_calDisplayScreen == CAL_MENU_CALIBRATED_DISPLAY) ||
		(s_calDisplayScreen == CAL_MENU_CALIBRATE_SENSOR) || (s_calDisplayScreen == CAL_MENU_CALIBRATION_PULSE))
	{
		// PRINT CAL_SETUP
		memset(&buff[0], 0, sizeof(buff));
		length = (uint8)sprintf((char*)buff, "-%s-", getLangText(CAL_SETUP_TEXT));
		wnd_layout_ptr->curr_col = (uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));
		wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_ZERO;
		WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

		// DATE AND TIME
		memset(&buff[0], 0, sizeof(buff));
		time = GetCurrentTime();
		ConvertTimeStampToString((char*)buff, &time, REC_DATE_TIME_TYPE);
		length = (uint8)strlen((char*)buff);
		wnd_layout_ptr->curr_col = (uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));
		wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_ONE;
		WndMpWrtString(buff,wnd_layout_ptr, SIX_BY_EIGHT_FONT,REG_LN);

#if 1 /* Test addition */
		char filterText[8];
		char weightText[8];
		if (aCutoffState == ANALOG_CUTOFF_FREQ_500) { strcpy(filterText, "5H"); }
		if (aCutoffState == ANALOG_CUTOFF_FREQ_1K) { strcpy(filterText, "1K"); }
		if (aCutoffState == ANALOG_CUTOFF_FREQ_2K) { strcpy(filterText, "2K"); }
		if (aCutoffState == ANALOG_CUTOFF_FREQ_4K) { strcpy(filterText, "4K"); }
		if (aCutoffState == ANALOG_CUTOFF_FREQ_8K) { strcpy(filterText, "8K"); }

		if (g_currentSensorGroup == SENSOR_GROUP_A_1) { if (pathState == ACOUSTIC_PATH_AOP) { strcpy(weightText, "AOP"); } else { strcpy(weightText, "C-W"); } }
		if (g_currentSensorGroup == SENSOR_GROUP_B_2) { if (pathState == ACOUSTIC_PATH_AOP) { strcpy(weightText, "AOP"); } else { strcpy(weightText, "A-W"); } }
#endif

		if (s_calDisplayScreen == CAL_MENU_DEFAULT_NON_CALIBRATED_DISPLAY)
		{
			// PRINT Table separator
			memset(&buff[0], 0, sizeof(buff));
			//sprintf((char*)buff, "--------------------");
#if 0 /* Normal */
			sprintf((char*)buff, "-----RAW NO CAL-----");
#else
			sprintf((char*)buff, "-----RAW NO CAL----- (%s/%s)", filterText, weightText);
#endif
			wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_TWO;
			WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT,REG_LN);
		}
		else if (s_calDisplayScreen == CAL_MENU_CALIBRATED_DISPLAY)
		{
			// PRINT Table separator
			memset(&buff[0], 0, sizeof(buff));
			//sprintf((char*)buff, "--------------------");
#if 0 /* Normal */
			sprintf((char*)buff, "-----CALIBRATED-----");
#else
			sprintf((char*)buff, "-----CALIBRATED----- (%s/%s)", filterText, weightText);
#endif
			wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_TWO;
			WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT,REG_LN);
		}
		else // CAL_MENU_CALIBRATE_SENSOR || CAL_MENU_CALIBRATION_PULSE
		{
			if (s_calDisplayScreen == CAL_MENU_CALIBRATE_SENSOR) { sprintf((char*)buff1, "CAL SENSOR"); }
			else { sprintf((char*)buff1, "CAL PULSE"); }

			if (s_pauseDisplay == YES)
			{
				// PRINT Table separator
				memset(&buff[0], 0, sizeof(buff));
				//sprintf((char*)buff, "--------------------");
				sprintf((char*)buff, "-%s: PAUSED-", (char*)buff1);
				wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_TWO;
				WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT,REG_LN);
			}
			else
			{
				// PRINT Table separator
				memset(&buff[0], 0, sizeof(buff));
				//sprintf((char*)buff, "--------------------");
				if (g_unitConfig.unitsOfMeasure == METRIC_TYPE)
				{
					if (g_displayAlternateResultState == DEFAULT_RESULTS) { sprintf((char*)buff, "-%s: MM/MB-", (char*)buff1); }
					else { sprintf((char*)buff, "-%s: MM/DB-", (char*)buff1); }
				}
				else // IMPERIAL_TYPE
				{
					if (g_displayAlternateResultState == DEFAULT_RESULTS) { sprintf((char*)buff, "-%s: IN/MB-", (char*)buff1); }
					else { sprintf((char*)buff, "-%s: IN/DB-", (char*)buff1); }
				}
				wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_TWO;
				WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT,REG_LN);
			}
		}

		if ((s_calDisplayScreen == CAL_MENU_CALIBRATE_SENSOR) || (s_calDisplayScreen == CAL_MENU_CALIBRATION_PULSE))
		{
			if (s_pauseDisplay == NO)
			{
				// PRINT Table header
				memset(&buff[0], 0, sizeof(buff));
				if ((g_calDisplayAlternateResultState == DEFAULT_RESULTS) && (s_calDisplayScreen == CAL_MENU_CALIBRATE_SENSOR)) { sprintf((char*)buff, "C| Peak| P2PA| Freq|"); }
				else if ((g_calDisplayAlternateResultState == DEFAULT_RESULTS) && (s_calDisplayScreen == CAL_MENU_CALIBRATION_PULSE)) { sprintf((char*)buff, "C| Peak|  P2P| Freq|"); }
				else { sprintf((char*)buff, "C| Curr|  -1s|  -2s|"); }
				wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_THREE;
				WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT,REG_LN);

				// PRINT R,V,T,A Min, Max and Avg
				memset(&buff[0], 0, sizeof(buff));
				// Setup R value for Column 1
				r1 = (float)((float)g_sensorCalPeaks[0].r / (float)div);
				// Setup R value for Column 2
				if ((g_calDisplayAlternateResultState == DEFAULT_RESULTS) && (s_calDisplayScreen == CAL_MENU_CALIBRATION_PULSE)) { r2 = (float)((float)(g_sensorCalChanMax[1] - g_sensorCalChanMin[1]) / (float)div); }
				else if ((g_calDisplayAlternateResultState == DEFAULT_RESULTS) && (s_calDisplayScreen == CAL_MENU_CALIBRATE_SENSOR)) { r2 = (float)((float)(g_sensorCalChanMax[1] - g_sensorCalChanMin[1]) / (2 * (float)div)); }
				else { r2 = (float)((float)g_sensorCalPeaks[1].r / (float)div); }
				// Setup R value for Column 3
				if (g_calDisplayAlternateResultState == DEFAULT_RESULTS) { r3 = (float)((float)CALIBRATION_FIXED_SAMPLE_RATE / (float)(g_sensorCalFreqCounts.r * 4)); }
				else { r3 = (float)((float)g_sensorCalPeaks[2].r / (float)div); }

				if (r1 < 10) { sprintf((char*)buff1, "%01.3f", (double)r1); } else if (r1 < 100) { sprintf((char*)buff1, "%02.2f", (double)r1); } else { sprintf((char*)buff1, "%03.1f", (double)r1); }
				if (r2 < 10) { sprintf((char*)buff2, "%01.3f", (double)r2); } else if (r2 < 100) { sprintf((char*)buff2, "%02.2f", (double)r2); } else { sprintf((char*)buff2, "%03.1f", (double)r2); }
				if (g_calDisplayAlternateResultState == DEFAULT_RESULTS) { sprintf((char*)buff3, "%5d", (int)r3); }
				else { if (r3 < 10) { sprintf((char*)buff3, "%01.3f", (double)r3); } else if (r3 < 100) { sprintf((char*)buff3, "%02.2f", (double)r3); } else { sprintf((char*)buff3, "%03.1f", (double)r3); } }

				sprintf((char*)buff, "R|%s|%s|%s|", (char*)buff1, (char*)buff2, (char*)buff3);
				wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_FOUR;
				WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

				memset(&buff[0], 0, sizeof(buff));
				// Setup V value for Column 1
				v1 = (float)((float)g_sensorCalPeaks[0].v / (float)div);
				// Setup V value for Column 2
				if ((g_calDisplayAlternateResultState == DEFAULT_RESULTS) && (s_calDisplayScreen == CAL_MENU_CALIBRATION_PULSE)) { v2 = (float)((float)(g_sensorCalChanMax[2] - g_sensorCalChanMin[2]) / (float)div); }
				else if ((g_calDisplayAlternateResultState == DEFAULT_RESULTS) && (s_calDisplayScreen == CAL_MENU_CALIBRATE_SENSOR)) { v2 = (float)((float)(g_sensorCalChanMax[2] - g_sensorCalChanMin[2]) / (2 * (float)div)); }
				else { v2 = (float)((float)g_sensorCalPeaks[1].v / (float)div); }
				// Setup V value for Column 3
				if (g_calDisplayAlternateResultState == DEFAULT_RESULTS) { v3 = (float)((float)CALIBRATION_FIXED_SAMPLE_RATE / (float)(g_sensorCalFreqCounts.v * 4)); }
				else { v3 = (float)((float)g_sensorCalPeaks[2].v / (float)div); }

				if (v1 < 10) { sprintf((char*)buff1, "%01.3f", (double)v1); } else if (v1 < 100) { sprintf((char*)buff1, "%02.2f", (double)v1); } else { sprintf((char*)buff1, "%03.1f", (double)v1); }
				if (v2 < 10) { sprintf((char*)buff2, "%01.3f", (double)v2); } else if (v2 < 100) { sprintf((char*)buff2, "%02.2f", (double)v2); } else { sprintf((char*)buff2, "%03.1f", (double)v2); }
				if (g_calDisplayAlternateResultState == DEFAULT_RESULTS) { sprintf((char*)buff3, "%5d", (int)v3); }
				else { if (v3 < 10) { sprintf((char*)buff3, "%01.3f", (double)v3); } else if (v3 < 100) { sprintf((char*)buff3, "%02.2f", (double)v3); } else { sprintf((char*)buff3, "%03.1f", (double)v3); } }

				sprintf((char*)buff, "V|%s|%s|%s|", (char*)buff1, (char*)buff2, (char*)buff3);
				wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_FIVE;
				WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

				memset(&buff[0], 0, sizeof(buff));
				// Setup T value for Column 1
				t1 = (float)((float)g_sensorCalPeaks[0].t / (float)div);
				// Setup T value for Column 2
				if ((g_calDisplayAlternateResultState == DEFAULT_RESULTS) && (s_calDisplayScreen == CAL_MENU_CALIBRATION_PULSE)) { t2 = (float)((float)(g_sensorCalChanMax[3] - g_sensorCalChanMin[3]) / (float)div); }
				else if ((g_calDisplayAlternateResultState == DEFAULT_RESULTS) && (s_calDisplayScreen == CAL_MENU_CALIBRATE_SENSOR)) { t2 = (float)((float)(g_sensorCalChanMax[3] - g_sensorCalChanMin[3]) / (2 * (float)div)); }
				else { t2 = (float)((float)g_sensorCalPeaks[1].t / (float)div); }
				// Setup T value for Column 3
				if (g_calDisplayAlternateResultState == DEFAULT_RESULTS) { t3 = (float)((float)CALIBRATION_FIXED_SAMPLE_RATE / (float)(g_sensorCalFreqCounts.t * 4)); }
				else { t3 = (float)((float)g_sensorCalPeaks[2].t / (float)div); }

				if (t1 < 10) { sprintf((char*)buff1, "%01.3f", (double)t1); } else if (t1 < 100) { sprintf((char*)buff1, "%02.2f", (double)t1); } else { sprintf((char*)buff1, "%03.1f", (double)t1); }
				if (t2 < 10) { sprintf((char*)buff2, "%01.3f", (double)t2); } else if (t2 < 100) { sprintf((char*)buff2, "%02.2f", (double)t2); } else { sprintf((char*)buff2, "%03.1f", (double)t2); }
				if (g_calDisplayAlternateResultState == DEFAULT_RESULTS) { sprintf((char*)buff3, "%5d", (int)t3); }
				else { if (t3 < 10) { sprintf((char*)buff3, "%01.3f", (double)t3); } else if (t3 < 100) { sprintf((char*)buff3, "%02.2f", (double)t3); } else { sprintf((char*)buff3, "%03.1f", (double)t3); } }

				sprintf((char*)buff, "T|%s|%s|%s|", (char*)buff1, (char*)buff2, (char*)buff3);
				wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_SIX;
				WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

				memset(&buff[0], 0, sizeof(buff));
				if (g_displayAlternateResultState == DEFAULT_RESULTS)
				{
					// Setup A value for Column 1
					sprintf((char*)buff1, "%05.3f", (double)HexToMB(g_sensorCalPeaks[0].a, DATA_NORMALIZED, ACCURACY_16_BIT_MIDPOINT, acousticSensorType));
					// Setup A value for Column 2
					if ((g_calDisplayAlternateResultState == DEFAULT_RESULTS) && (s_calDisplayScreen == CAL_MENU_CALIBRATION_PULSE))
						{ sprintf((char*)buff2, "%05.3f", (double)HexToMB((g_sensorCalChanMax[0] - g_sensorCalChanMin[0]), DATA_NORMALIZED, ACCURACY_16_BIT_MIDPOINT, acousticSensorType)); }
					else if ((g_calDisplayAlternateResultState == DEFAULT_RESULTS) && (s_calDisplayScreen == CAL_MENU_CALIBRATE_SENSOR))
						{ sprintf((char*)buff2, "%05.3f", (double)HexToMB(((g_sensorCalChanMax[0] - g_sensorCalChanMin[0]) / 2), DATA_NORMALIZED, ACCURACY_16_BIT_MIDPOINT, acousticSensorType)); }
					else { sprintf((char*)buff2, "%05.3f", (double)HexToMB(g_sensorCalPeaks[1].a, DATA_NORMALIZED, ACCURACY_16_BIT_MIDPOINT, acousticSensorType)); }
					// Setup R value for Column 3
					if (g_calDisplayAlternateResultState == DEFAULT_RESULTS) { sprintf((char*)buff3, "%5d", (int)(CALIBRATION_FIXED_SAMPLE_RATE / (g_sensorCalFreqCounts.a * 4))); }
					else { sprintf((char*)buff3, "%05.3f", (double)HexToMB(g_sensorCalPeaks[2].a, DATA_NORMALIZED, ACCURACY_16_BIT_MIDPOINT, acousticSensorType)); }

					sprintf((char*)buff, "A|%s|%s|%s|", (char*)buff1, (char*)buff2, (char*)buff3);
					//strcpy((char*)g_spareBuffer, (char*)buff);
				}
				else
				{
					// Setup A value for Column 1
					sprintf((char*)buff1, "%5.1f", (double)HexToDB(g_sensorCalPeaks[0].a, DATA_NORMALIZED, ACCURACY_16_BIT_MIDPOINT, acousticSensorType));
					// Setup A value for Column 2
					if ((g_calDisplayAlternateResultState == DEFAULT_RESULTS) && (s_calDisplayScreen == CAL_MENU_CALIBRATION_PULSE))
						{ sprintf((char*)buff2, "%5.1f", (double)HexToDB((g_sensorCalChanMax[0] - g_sensorCalChanMin[0]), DATA_NORMALIZED, ACCURACY_16_BIT_MIDPOINT, acousticSensorType)); }
					else { sprintf((char*)buff2, "%5.1f", (double)HexToDB(g_sensorCalPeaks[1].a, DATA_NORMALIZED, ACCURACY_16_BIT_MIDPOINT, acousticSensorType)); }
					// Setup A value for Column 3
					if (g_calDisplayAlternateResultState == DEFAULT_RESULTS) { sprintf((char*)buff3, "%5d", (int)(CALIBRATION_FIXED_SAMPLE_RATE / (g_sensorCalFreqCounts.a * 4))); }
						//{ sprintf((char*)buff3, "%5d", (int)(g_sensorCalFreqCounts.a)); }
					else { sprintf((char*)buff3, "%5.1f", (double)HexToDB(g_sensorCalPeaks[2].a, DATA_NORMALIZED, ACCURACY_16_BIT_MIDPOINT, acousticSensorType)); }

					sprintf((char*)buff, "A|%s|%s|%s|", (char*)buff1, (char*)buff2, (char*)buff3);
					//strcpy((char*)&g_spareBuffer[50], (char*)buff);
				}
				wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_SEVEN;
				WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
			}
			else // (s_pauseDisplay == YES)
			{
#if 0 /* Try to remove */
				wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_SEVEN;

				if (g_displayAlternateResultState == DEFAULT_RESULTS) { WndMpWrtString(g_spareBuffer, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN); }
				else { WndMpWrtString(&g_spareBuffer[50], wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN); }
#endif
			}
		}
		else // ((s_calDisplayScreen == CAL_MENU_DEFAULT_NON_CALIBRATED_DISPLAY) || (s_calDisplayScreen == CAL_MENU_CALIBRATED_DISPLAY))
		{
			// PRINT Table header
			memset(&buff[0], 0, sizeof(buff));
			//sprintf((char*)buff, "C|  Min|  Max|  Avg|");
#if 0 /* Normal */
			sprintf((char*)buff, "Chan   Min   Max   Units   Acc");
#else /* Test Addition */
			sprintf((char*)buff, "Chan Min Max Units Acc (%c/CM:%s)", ((gainState == SEISMIC_GAIN_NORMAL) ? 'N' : 'H'), ((cmState == OFF) ? "OFF" : "ON"));
#endif
			wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_THREE;
			WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT,REG_LN);

			// PRINT R,V,T,A Min, Max and Avg
			memset(&buff[0], 0, sizeof(buff));
#if 0 /* Normal */
			sprintf((char*)buff, "R|%+5ld|%+5ld|%+5ld|", g_sensorCalChanMin[1], g_sensorCalChanMax[1], g_sensorCalChanAvg[1]);
			wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_FOUR;
			WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

			memset(&buff[0], 0, sizeof(buff));
			sprintf((char*)buff, "V|%+5ld|%+5ld|%+5ld|", g_sensorCalChanMin[2], g_sensorCalChanMax[2], g_sensorCalChanAvg[2]);
			wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_FIVE;
			WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

			memset(&buff[0], 0, sizeof(buff));
			sprintf((char*)buff, "T|%+5ld|%+5ld|%+5ld|", g_sensorCalChanMin[3], g_sensorCalChanMax[3], g_sensorCalChanAvg[3]);
			wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_SIX;
			WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
#else /* Test adding in Acc results */
			float rCalc, tCalc, vCalc, xCalc, yCalc, zCalc, aCalc;
			rCalc = (float)((float)((abs(g_sensorCalChanMin[1]) > g_sensorCalChanMax[1] ? abs(g_sensorCalChanMin[1]) : g_sensorCalChanMax[1])) / (float)div);
			tCalc = (float)((float)((abs(g_sensorCalChanMin[3]) > g_sensorCalChanMax[3] ? abs(g_sensorCalChanMin[3]) : g_sensorCalChanMax[3])) / (float)div);
			vCalc = (float)((float)((abs(g_sensorCalChanMin[2]) > g_sensorCalChanMax[2] ? abs(g_sensorCalChanMin[2]) : g_sensorCalChanMax[2])) / (float)div);
			aCalc = (float)HexToMB((float)(abs(g_sensorCalChanMin[0]) > g_sensorCalChanMax[0] ? abs(g_sensorCalChanMin[0]) : g_sensorCalChanMax[0]), DATA_NORMALIZED, ACCURACY_16_BIT_MIDPOINT, acousticSensorType);
			xCalc = (float)8.000 / (float)32768 * (float)(abs(g_sensorCalChanMin[4]) > g_sensorCalChanMax[4] ? abs(g_sensorCalChanMin[4]) : g_sensorCalChanMax[4]);
			yCalc = (float)8.000 / (float)32768 * (float)(abs(g_sensorCalChanMin[5]) > g_sensorCalChanMax[5] ? abs(g_sensorCalChanMin[5]) : g_sensorCalChanMax[5]);
			zCalc = (float)8.000 / (float)32768 * (float)(abs(g_sensorCalChanMin[6]) > g_sensorCalChanMax[6] ? abs(g_sensorCalChanMin[6]) : g_sensorCalChanMax[6]);
			sprintf((char*)buff, "(R) %+5ld  %+5ld   (R) %2.3f IPS   (X) %1.4f G", g_sensorCalChanMin[1], g_sensorCalChanMax[1], (double)rCalc, (double)xCalc);
/* 			sprintf((char*)buff, "R|%+1.4f|%+1.4f|%+5ld| X|%+1.4f|%+1.4f|%+5ld|",
					(double)((float)10.24 / (float)32768 * (float)g_sensorCalChanMin[1]),
					(double)((float)10.24 / (float)32768 * (float)g_sensorCalChanMax[1]),
					g_sensorCalChanAvg[1],
					(double)((float)8 / (float)32768 * (float)g_sensorCalChanMin[4]),
					(double)((float)8 / (float)32768 * (float)g_sensorCalChanMax[4]),
					g_sensorCalChanAvg[4]);
 */
			wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_FOUR;
			WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

			memset(&buff[0], 0, sizeof(buff));
			sprintf((char*)buff, "(T) %+5ld  %+5ld   (T) %2.3f IPS   (Y) %1.4f G", g_sensorCalChanMin[3], g_sensorCalChanMax[3], (double)tCalc, (double)yCalc);
/* 			sprintf((char*)buff, "V|%+1.4f|%+1.4f|%+5ld| Z|%+1.4f|%+1.4f|%+5ld|",
					(double)((float)10.24 / (float)32768 * (float)g_sensorCalChanMin[2]),
					(double)((float)10.24 / (float)32768 * (float)g_sensorCalChanMax[2]),
					g_sensorCalChanAvg[2],
					(double)((float)8 / (float)32768 * (float)g_sensorCalChanMin[6]),
					(double)((float)8 / (float)32768 * (float)g_sensorCalChanMax[6]),
					g_sensorCalChanAvg[6]);
 */
			wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_FIVE;
			WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

			memset(&buff[0], 0, sizeof(buff));
			sprintf((char*)buff, "(V) %+5ld  %+5ld   (V) %2.3f IPS   (Z) %1.4f G", g_sensorCalChanMin[2], g_sensorCalChanMax[2], (double)vCalc, (double)zCalc);
/* 			sprintf((char*)buff, "T|%+1.4f|%+1.4f|%+5ld| Y|%+1.4f|%+1.4f|%+5ld|",
					(double)((float)10.24 / (float)32768 * (float)g_sensorCalChanMin[3]),
					(double)((float)10.24 / (float)32768 * (float)g_sensorCalChanMax[3]),
					g_sensorCalChanAvg[3],
					(double)((float)8 / (float)32768 * (float)g_sensorCalChanMin[5]),
					(double)((float)8 / (float)32768 * (float)g_sensorCalChanMax[5]),
					g_sensorCalChanAvg[5]);
 */
			wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_SIX;
			WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
#endif
			memset(&buff[0], 0, sizeof(buff));
			sprintf((char*)buff, "(A) %+5ld  %+5ld   (A) %1.3f mB", g_sensorCalChanMin[0], g_sensorCalChanMax[0], (double)aCalc);
			wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_SEVEN;
			WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
		}
	}
	else // s_calDisplayScreen == CAL_MENU_CALIBRATED_CHAN_NOISE_PERCENT_DISPLAY
	{
#if 0 /* Original */
		// R
		memset(&buff[0], 0, sizeof(buff));
		sprintf((char*)buff, "R%%|+1|+2|+3|+4|+5|+6");
		wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_ZERO;
		WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

		memset(&buff[0], 0, sizeof(buff));
		sprintf((char*)buff, "%2d|%2d|%2d|%2d|%2d|%2d|%2d", g_sensorCalChanMed[1][0], g_sensorCalChanMed[1][1], g_sensorCalChanMed[1][2], g_sensorCalChanMed[1][3], g_sensorCalChanMed[1][4], g_sensorCalChanMed[1][5], g_sensorCalChanMed[1][6]);
		wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_ONE;
		WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

		// V
		memset(&buff[0], 0, sizeof(buff));
		sprintf((char*)buff, "V%%|  |  |  |  |  |  ");
		wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_TWO;
		WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

		memset(&buff[0], 0, sizeof(buff));
		sprintf((char*)buff, "%2d|%2d|%2d|%2d|%2d|%2d|%2d", g_sensorCalChanMed[2][0], g_sensorCalChanMed[2][1], g_sensorCalChanMed[2][2], g_sensorCalChanMed[2][3], g_sensorCalChanMed[2][4], g_sensorCalChanMed[2][5], g_sensorCalChanMed[2][6]);
		wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_THREE;
		WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

		// T
		memset(&buff[0], 0, sizeof(buff));
		sprintf((char*)buff, "T%%|  |  |  |  |  |  ");
		wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_FOUR;
		WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

		memset(&buff[0], 0, sizeof(buff));
		sprintf((char*)buff, "%2d|%2d|%2d|%2d|%2d|%2d|%2d", g_sensorCalChanMed[3][0], g_sensorCalChanMed[3][1], g_sensorCalChanMed[3][2], g_sensorCalChanMed[3][3], g_sensorCalChanMed[3][4], g_sensorCalChanMed[3][5], g_sensorCalChanMed[3][6]);
		wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_FIVE;
		WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

		// A
		memset(&buff[0], 0, sizeof(buff));
		sprintf((char*)buff, "A%%|  |  |  |  |  |  ");
		wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_SIX;
		WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

		memset(&buff[0], 0, sizeof(buff));
		sprintf((char*)buff, "%2d|%2d|%2d|%2d|%2d|%2d|%2d", g_sensorCalChanMed[0][0], g_sensorCalChanMed[0][1], g_sensorCalChanMed[0][2], g_sensorCalChanMed[0][3], g_sensorCalChanMed[0][4], g_sensorCalChanMed[0][5], g_sensorCalChanMed[0][6]);
		wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_SEVEN;
		WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
#else
		// PRINT CAL_SETUP
		memset(&buff[0], 0, sizeof(buff));
		length = (uint8)sprintf((char*)buff, "-%s-", getLangText(CAL_SETUP_TEXT));
		wnd_layout_ptr->curr_col = (uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));
		wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_ZERO;
		WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

		// DATE AND TIME
		memset(&buff[0], 0, sizeof(buff));
		time = GetCurrentTime();
		ConvertTimeStampToString((char*)buff, &time, REC_DATE_TIME_TYPE);
		length = (uint8)strlen((char*)buff);
		wnd_layout_ptr->curr_col = (uint16)(((wnd_layout_ptr->end_col)/2) - ((length * SIX_COL_SIZE)/2));
		wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_ONE;
		WndMpWrtString(buff,wnd_layout_ptr, SIX_BY_EIGHT_FONT,REG_LN);

		// PRINT Table separator
		memset(&buff[0], 0, sizeof(buff));
		//sprintf((char*)buff, "--------------------");
		sprintf((char*)buff, "--CHANNEL NOISE %%--");
		wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_TWO;
		WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT,REG_LN);

		memset(&buff[0], 0, sizeof(buff));
		sprintf((char*)buff, "CH|+0|+1|+2|+3|+4|+5");
		wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_THREE;
		WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

		// R
		memset(&buff[0], 0, sizeof(buff));
		sprintf((char*)buff, "R%%|%2d|%2d|%2d|%2d|%2d|%2d", (int)g_sensorCalChanMed[1][0], (int)g_sensorCalChanMed[1][1], (int)g_sensorCalChanMed[1][2], (int)g_sensorCalChanMed[1][3], (int)g_sensorCalChanMed[1][4], (int)g_sensorCalChanMed[1][5]);
		wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_FOUR;
		WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

		// V
		memset(&buff[0], 0, sizeof(buff));
		sprintf((char*)buff, "V%%|%2d|%2d|%2d|%2d|%2d|%2d", (int)g_sensorCalChanMed[2][0], (int)g_sensorCalChanMed[2][1], (int)g_sensorCalChanMed[2][2], (int)g_sensorCalChanMed[2][3], (int)g_sensorCalChanMed[2][4], (int)g_sensorCalChanMed[2][5]);
		wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_FIVE;
		WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

		// T
		memset(&buff[0], 0, sizeof(buff));
		sprintf((char*)buff, "T%%|%2d|%2d|%2d|%2d|%2d|%2d", (int)g_sensorCalChanMed[3][0], (int)g_sensorCalChanMed[3][1], (int)g_sensorCalChanMed[3][2], (int)g_sensorCalChanMed[3][3], (int)g_sensorCalChanMed[3][4], (int)g_sensorCalChanMed[3][5]);
		wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_SIX;
		WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);

		// A
		memset(&buff[0], 0, sizeof(buff));
		sprintf((char*)buff, "A%%|%2d|%2d|%2d|%2d|%2d|%2d", (int)g_sensorCalChanMed[0][0], (int)g_sensorCalChanMed[0][1], (int)g_sensorCalChanMed[0][2], (int)g_sensorCalChanMed[0][3], (int)g_sensorCalChanMed[0][4], (int)g_sensorCalChanMed[0][5]);
		wnd_layout_ptr->curr_row = DEFAULT_MENU_ROW_SEVEN;
		WndMpWrtString(buff, wnd_layout_ptr, SIX_BY_EIGHT_FONT, REG_LN);
#endif
	}
}
