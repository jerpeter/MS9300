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
#include "OldUart.h"
#include "Keypad.h"
#include "SysEvents.h"
#include "EventProcessing.h"
#include "InitDataBuffers.h"
#include "RemoteCommon.h"
#include "PowerManagement.h"
#include "Sensor.h"
#include "TextTypes.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
#include "Globals.h"
extern USER_MENU_STRUCT acousticSensorTypeMenu[];
extern USER_MENU_STRUCT adChannelVerificationMenu[];
extern USER_MENU_STRUCT airTriggerMenu[];
extern USER_MENU_STRUCT alarmOneMenu[];
extern USER_MENU_STRUCT alarmTwoMenu[];
extern USER_MENU_STRUCT alarmOneTimeMenu[];
extern USER_MENU_STRUCT alarmTwoTimeMenu[];
extern USER_MENU_STRUCT alarmOneSeismicLevelMenu[];
extern USER_MENU_STRUCT alarmOneAirLevelMenu[];
extern USER_MENU_STRUCT alarmTwoSeismicLevelMenu[];
extern USER_MENU_STRUCT alarmTwoAirLevelMenu[];
extern USER_MENU_STRUCT alarmTestingMenu[];
extern USER_MENU_STRUCT analogChannelConfigMenu[];
extern USER_MENU_STRUCT barChannelMenu[];
extern USER_MENU_STRUCT barIntervalMenu[];
extern USER_MENU_STRUCT barIntervalDataTypeMenu[];
extern USER_MENU_STRUCT barLiveMonitorMenu[];
//extern USER_MENU_STRUCT blmStartStopMsgMenu[];
extern USER_MENU_STRUCT barScaleMenu[];
extern USER_MENU_STRUCT barResultMenu[];
extern USER_MENU_STRUCT baudRateMenu[];
extern USER_MENU_STRUCT bitAccuracyMenu[];
extern USER_MENU_STRUCT calibratonDateSourceMenu[];
extern USER_MENU_STRUCT companyMenu[];
extern USER_MENU_STRUCT copiesMenu[];
extern USER_MENU_STRUCT configMenu[];
extern USER_MENU_STRUCT cycleEndTimeMenu[];
extern USER_MENU_STRUCT displacementMenu[];
extern USER_MENU_STRUCT eraseEventsMenu[];
extern USER_MENU_STRUCT eraseSettingsMenu[];
extern USER_MENU_STRUCT externalTriggerMenu[];
extern USER_MENU_STRUCT flashWrappingMenu[];
extern USER_MENU_STRUCT freqPlotMenu[];
extern USER_MENU_STRUCT freqPlotStandardMenu[];
extern USER_MENU_STRUCT gpsPowerMenu[];
extern USER_MENU_STRUCT hardwareIDMenu[];
extern USER_MENU_STRUCT helpMenu[];
extern USER_MENU_STRUCT infoMenu[];
extern USER_MENU_STRUCT languageMenu[];
extern USER_MENU_STRUCT lcdImpulseTimeMenu[];
extern USER_MENU_STRUCT lcdTimeoutMenu[];
extern USER_MENU_STRUCT legacyDqmLimitMenu[];
extern USER_MENU_STRUCT modemDialMenu[];
extern USER_MENU_STRUCT modemDialOutCycleTimeMenu[];
extern USER_MENU_STRUCT modemInitMenu[];
extern USER_MENU_STRUCT modemResetMenu[];
extern USER_MENU_STRUCT modemSetupMenu[];
extern USER_MENU_STRUCT monitorLogMenu[];
extern USER_MENU_STRUCT operatorMenu[];
extern USER_MENU_STRUCT peakAccMenu[];
extern USER_MENU_STRUCT percentLimitTriggerMenu[];
extern USER_MENU_STRUCT pretriggerSizeMenu[];
extern USER_MENU_STRUCT printerEnableMenu[];
extern USER_MENU_STRUCT printMonitorLogMenu[];
extern USER_MENU_STRUCT rs232PowerSavingsMenu[];
extern USER_MENU_STRUCT recalibrateMenu[];
extern USER_MENU_STRUCT recordTimeMenu[];
extern USER_MENU_STRUCT saveCompressedDataMenu[];
extern USER_MENU_STRUCT saveRecordMenu[];
extern USER_MENU_STRUCT saveSetupMenu[];
extern USER_MENU_STRUCT sampleRateMenu[];
extern USER_MENU_STRUCT sampleRateBargraphMenu[];
extern USER_MENU_STRUCT sampleRateComboMenu[];
extern USER_MENU_STRUCT samplingMethodMenu[];
extern USER_MENU_STRUCT seismicFilteringMenu[];
extern USER_MENU_STRUCT seismicSensorTypeMenu[];
extern USER_MENU_STRUCT seismicTriggerMenu[];
#if (!VT_FEATURE_DISABLED)
extern USER_MENU_STRUCT seismicTriggerTypeMenu[];
#endif
extern USER_MENU_STRUCT sensitivityMenu[];
extern USER_MENU_STRUCT serialNumberMenu[];
extern USER_MENU_STRUCT storedEventsCapModeMenu[];
extern USER_MENU_STRUCT storedEventLimitMenu[];
extern USER_MENU_STRUCT summaryIntervalMenu[];
extern USER_MENU_STRUCT syncFileExistsMenu[];
extern USER_MENU_STRUCT timerModeFreqMenu[];
extern USER_MENU_STRUCT timerModeMenu[];
extern USER_MENU_STRUCT unitsOfMeasureMenu[];
extern USER_MENU_STRUCT unitsOfAirMenu[];
extern USER_MENU_STRUCT usbSyncModeMenu[];
extern USER_MENU_STRUCT utcZoneOffsetMenu[];
extern USER_MENU_STRUCT vectorSumMenu[];
#if (!VT_FEATURE_DISABLED)
extern USER_MENU_STRUCT vibrationStandardMenu[];
#endif
extern USER_MENU_STRUCT waveformAutoCalMenu[];
extern USER_MENU_STRUCT zeroEventNumberMenu[];

///----------------------------------------------------------------------------
///	Local Scope Globals
///----------------------------------------------------------------------------

//*****************************************************************************
//=============================================================================
// Adaptive Sample Rate Menu
//=============================================================================
//*****************************************************************************
#define ADAPTIVE_SAMPLING_MENU_ENTRIES 4
USER_MENU_STRUCT adaptiveSamplingMenu[ADAPTIVE_SAMPLING_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, ADAPTIVE_SAMPLING_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, ADAPTIVE_SAMPLING_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{NO_TAG, 0, SHOW_OPTION_IN_SETUP_TEXT,	NO_TAG,	{ENABLED}},
{NO_TAG, 0, HIDE_OPTION_DISABLE_TEXT,	NO_TAG,	{DISABLED}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&AdaptiveSamplingMenuHandler}}
};

//-------------------------------------
// Adaptive Sample Rate Menu Handler
//-------------------------------------
void AdaptiveSamplingMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_unitConfig.adaptiveSampling = (uint8)adaptiveSamplingMenu[newItemIndex].data;
		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		// Make sure if option is disabled that the current sampling method is set to fixed
		if (g_unitConfig.adaptiveSampling == DISABLED)
		{
			g_triggerRecord.trec.samplingMethod = FIXED_SAMPLING;
		}

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, ADAPTIVE_SAMPLING);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// AD Channel Verification Menu
//=============================================================================
//*****************************************************************************
#define AD_CHANNEL_VERIFICATION_MENU_ENTRIES 4
USER_MENU_STRUCT adChannelVerificationMenu[AD_CHANNEL_VERIFICATION_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, CHAN_VERIFICATION_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, AD_CHANNEL_VERIFICATION_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, ENABLED_TEXT,	NO_TAG,	{ENABLED}},
{ITEM_2, 0, DISABLED_TEXT,	NO_TAG,	{DISABLED}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&AdChannelVerificationMenuHandler}}
};

//-------------------------------------
// AD Channel Verification Menu Handler
//-------------------------------------
void AdChannelVerificationMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_unitConfig.adChannelVerification = (uint8)adChannelVerificationMenu[newItemIndex].data;
		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, CHANNEL_VERIFICATION);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Air Scale Menu
//=============================================================================
//*****************************************************************************
#define AIR_SCALE_MENU_ENTRIES 4
USER_MENU_STRUCT airScaleMenu[AIR_SCALE_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, AIR_CHANNEL_SCALE_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, AIR_SCALE_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, LINEAR_TEXT,		NO_TAG,	{AIR_SCALE_LINEAR}},
{ITEM_2, 0, A_WEIGHTING_TEXT,	NO_TAG,	{AIR_SCALE_A_WEIGHTING}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&AirScaleMenuHandler}}
};

//-----------------------
// Air Scale Menu Handler
//-----------------------
void AirScaleMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	g_unitConfig.airScale = (uint8)airScaleMenu[newItemIndex].data;
	
	if (keyPressed == ENTER_KEY)
	{
		if ((g_triggerRecord.opMode == WAVEFORM_MODE) || (g_triggerRecord.opMode == COMBO_MODE))
		{
			SETUP_USER_MENU_FOR_INTEGERS_MSG(&recordTimeMenu, &g_triggerRecord.trec.record_time, RECORD_TIME_DEFAULT_VALUE, RECORD_TIME_MIN_VALUE, g_triggerRecord.trec.record_time_max);
		}
		else // (g_triggerRecord.opMode == BARGRAPH_MODE)
		{
#if 0 /* Removing this option */
			SETUP_USER_MENU_MSG(&barResultMenu, g_unitConfig.vectorSum);
#else
			if ((g_unitConfig.alarmOneMode) || (g_unitConfig.alarmTwoMode))
			{
				SETUP_USER_MENU_MSG(&alarmOneMenu, g_unitConfig.alarmOneMode);
			}
			else // Save setup
			{
				SETUP_USER_MENU_MSG(&saveSetupMenu, YES);
			}
#endif
		}
	}
	else if (keyPressed == ESC_KEY)
	{
		if ((g_triggerRecord.opMode == WAVEFORM_MODE) || (g_triggerRecord.opMode == COMBO_MODE))
		{
			if (g_externalTriggerMenuActiveForSetup == YES)
			{
				// Jump to the External Trigger menu to allow the user to change if desired
				SETUP_USER_MENU_MSG(&externalTriggerMenu, g_unitConfig.externalTrigger);
			}
			else
			{
				g_tempTriggerLevelForMenuAdjustment = AirTriggerConvertToUnits(g_triggerRecord.trec.airTriggerLevel);
				SETUP_USER_MENU_FOR_INTEGERS_MSG(&airTriggerMenu, &g_tempTriggerLevelForMenuAdjustment, GetAirDefaultValue(), GetAirMinValue(), GetAirMaxValue());
			}
		}
		else // (g_triggerRecord.opMode == BARGRAPH_MODE)
		{
			SETUP_USER_MENU_FOR_INTEGERS_MSG(&lcdImpulseTimeMenu, &g_triggerRecord.berec.impulseMenuUpdateSecs,	LCD_IMPULSE_TIME_DEFAULT_VALUE, LCD_IMPULSE_TIME_MIN_VALUE, LCD_IMPULSE_TIME_MAX_VALUE);
		}
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Air Setup Menu
//=============================================================================
//*****************************************************************************
#define AIR_SETUP_MENU_ENTRIES 4
USER_MENU_STRUCT airSetupMenu[AIR_SETUP_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, A_WEIGHTING_OPTION_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, AIR_SETUP_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, INCLUDED_TEXT,		NO_TAG, {ENABLED}},
{ITEM_2, 0, NOT_INCLUDED_TEXT,	NO_TAG, {DISABLED}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&AirSetupMenuHandler}}
};

//-----------------------
// Air Setup Menu Handler
//-----------------------
void AirSetupMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_factorySetupRecord.aWeightOption = (uint8)airSetupMenu[newItemIndex].data;

		if ((g_factorySetupRecord.aWeightOption == DISABLED) || ((g_unitConfig.airScale != AIR_SCALE_LINEAR) && (g_unitConfig.airScale != AIR_SCALE_A_WEIGHTING)))
		{
			g_unitConfig.airScale = AIR_SCALE_LINEAR;
			SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);
		}

#if 0
		SETUP_MENU_MSG(CAL_SETUP_MENU);

		// Special call before Calibration setup to disable USB processing
		UsbDeviceManager();
#else
		SETUP_USER_MENU_MSG(&alarmTestingMenu, DEFAULT_ITEM_1);
#endif
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&calibratonDateSourceMenu, g_factorySetupRecord.calibrationDateSource);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Alarm Testing Menu
//=============================================================================
//*****************************************************************************
#define ALARM_TESTING_MENU_ENTRIES 7
USER_MENU_STRUCT alarmTestingMenu[ALARM_TESTING_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, ALARM_TESTING_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, ALARM_TESTING_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, END_TEXT,		NO_TAG,			{ALARM_TESTING_DONE}},
{ITEM_2, 0, ALARM_1_TEXT,	ENABLED_TAG,	{ALARM_1_TESTING_ENABLED}},
{ITEM_3, 0, ALARM_1_TEXT,	DISABLED_TAG,	{ALARM_1_TESTING_DISABLED}},
{ITEM_4, 0, ALARM_2_TEXT,	ENABLED_TAG,	{ALARM_2_TESTING_ENABLED}},
{ITEM_5, 0, ALARM_2_TEXT,	DISABLED_TAG,	{ALARM_2_TESTING_DISABLED}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&AlarmTestingMenuHandler}}
};

//---------------------------
// Alarm Testing Menu Handler
//---------------------------
void AlarmTestingMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		if ((uint8)alarmTestingMenu[newItemIndex].data == ALARM_TESTING_DONE)
		{
#if 0 /* old hw */
			// Clear Alarm 1
			gpio_clr_gpio_pin(ALARM_1_GPIO_PIN);
			// Clear Alarm 2
			gpio_clr_gpio_pin(ALARM_2_GPIO_PIN);
#endif
			SETUP_MENU_MSG(CAL_SETUP_MENU);

			// Special call before Calibration setup to disable USB processing
			UsbDeviceManager();
		}
		else
		{
			if ((uint8)alarmTestingMenu[newItemIndex].data == ALARM_1_TESTING_ENABLED)
			{
				// Start Alarm 1
#if 0 /* old hw */
				gpio_set_gpio_pin(ALARM_1_GPIO_PIN);
#endif
			}
			else if ((uint8)alarmTestingMenu[newItemIndex].data == ALARM_1_TESTING_DISABLED)
			{
				// Clear Alarm 1
#if 0 /* old hw */
				gpio_clr_gpio_pin(ALARM_1_GPIO_PIN);
#endif
			}
			else if ((uint8)alarmTestingMenu[newItemIndex].data == ALARM_2_TESTING_ENABLED)
			{
				// Start Alarm 2
#if 0 /* old hw */
				gpio_set_gpio_pin(ALARM_2_GPIO_PIN);
#endif
			}
			else if ((uint8)alarmTestingMenu[newItemIndex].data == ALARM_2_TESTING_DISABLED)
			{
				// Clear Alarm 2
#if 0 /* old hw */
				gpio_clr_gpio_pin(ALARM_2_GPIO_PIN);
#endif
			}
		}
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&airSetupMenu, g_factorySetupRecord.aWeightOption);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Alarm One Menu
//=============================================================================
//*****************************************************************************
#define ALARM_ONE_MENU_ENTRIES 6
USER_MENU_STRUCT alarmOneMenu[ALARM_ONE_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, ALARM_1_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, ALARM_ONE_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, OFF_TEXT,		NO_TAG, {ALARM_MODE_OFF}},
{ITEM_2, 0, SEISMIC_TEXT,	NO_TAG, {ALARM_MODE_SEISMIC}},
{ITEM_3, 0, AIR_TEXT,		NO_TAG, {ALARM_MODE_AIR}},
{ITEM_4, 0, BOTH_TEXT,		NO_TAG, {ALARM_MODE_BOTH}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&AlarmOneMenuHandler}}
};

//-----------------------
// Alarm One Menu Handler
//-----------------------
void AlarmOneMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_unitConfig.alarmOneMode = (uint8)alarmOneMenu[newItemIndex].data;

		switch (g_unitConfig.alarmOneMode)
		{
			case (ALARM_MODE_OFF):
				g_unitConfig.alarmOneSeismicLevel = NO_TRIGGER_CHAR;
				g_unitConfig.alarmOneAirLevel = NO_TRIGGER_CHAR;

				SETUP_USER_MENU_MSG(&alarmTwoMenu, g_unitConfig.alarmTwoMode);
			break;

			case (ALARM_MODE_SEISMIC):
				if ((g_triggerRecord.opMode == WAVEFORM_MODE) && (g_triggerRecord.trec.seismicTriggerLevel == NO_TRIGGER_CHAR))
				{
					sprintf((char*)g_spareBuffer, "%s %s. %s", getLangText(SEISMIC_TRIGGER_TEXT), getLangText(SET_TO_NO_TRIGGER_TEXT), getLangText(PLEASE_CHANGE_TEXT));
					MessageBox(getLangText(WARNING_TEXT), (char*)g_spareBuffer, MB_OK);

					SETUP_USER_MENU_MSG(&alarmOneMenu, g_unitConfig.alarmOneMode);
				}
				else
				{
					g_unitConfig.alarmOneAirLevel = NO_TRIGGER_CHAR;

					// Setup Alarm One Seismic Level
					if (g_unitConfig.alarmOneSeismicLevel == NO_TRIGGER_CHAR)
					{
						g_unitConfig.alarmOneSeismicLevel = ALARM_ONE_SEIS_DEFAULT_TRIG_LVL;
					}

					if (g_triggerRecord.opMode == WAVEFORM_MODE)
					{
						g_alarmOneSeismicMinLevel = g_triggerRecord.trec.seismicTriggerLevel;

						if (g_unitConfig.alarmOneSeismicLevel < g_triggerRecord.trec.seismicTriggerLevel)
						{
							g_unitConfig.alarmOneSeismicLevel = g_triggerRecord.trec.seismicTriggerLevel;
						}
					}
					else // (g_triggerRecord.opMode == BARGRAPH_MODE) || (g_triggerRecord.opMode == COMBO_MODE)
					{
						g_alarmOneSeismicMinLevel = ALARM_SEIS_MIN_VALUE;
					}

					// Down convert to current bit accuracy setting
					g_tempTriggerLevelForMenuAdjustment = SeismicTriggerConvertBitAccuracy(g_unitConfig.alarmOneSeismicLevel);

					// Call Alarm One Seismic Level
					SETUP_USER_MENU_FOR_INTEGERS_MSG(&alarmOneSeismicLevelMenu, &g_tempTriggerLevelForMenuAdjustment,
													(g_alarmOneSeismicMinLevel / (ALARM_SEIS_MAX_VALUE / g_bitAccuracyMidpoint)),
													(g_alarmOneSeismicMinLevel / (ALARM_SEIS_MAX_VALUE / g_bitAccuracyMidpoint)), g_bitAccuracyMidpoint);
				}
			break;

			case (ALARM_MODE_AIR):
				if ((g_triggerRecord.opMode == WAVEFORM_MODE) && (g_triggerRecord.trec.airTriggerLevel == NO_TRIGGER_CHAR))
				{
					sprintf((char*)g_spareBuffer, "%s %s. %s", getLangText(AIR_TRIGGER_TEXT), getLangText(SET_TO_NO_TRIGGER_TEXT), getLangText(PLEASE_CHANGE_TEXT));
					MessageBox(getLangText(WARNING_TEXT), (char*)g_spareBuffer, MB_OK);

					SETUP_USER_MENU_MSG(&alarmOneMenu, g_unitConfig.alarmOneMode);
				}
				else
				{
					g_unitConfig.alarmOneSeismicLevel = NO_TRIGGER_CHAR;

					// Setup Alarm One Air Level
					if (g_unitConfig.alarmOneAirLevel == NO_TRIGGER_CHAR)
					{
						g_unitConfig.alarmOneAirLevel = ALARM_ONE_AIR_DEFAULT_TRIG_LVL;
					}

					if (g_triggerRecord.opMode == WAVEFORM_MODE)
					{
						g_alarmOneAirMinLevel = (uint32)g_triggerRecord.trec.airTriggerLevel;

						if (g_unitConfig.alarmOneAirLevel < (uint32)g_triggerRecord.trec.airTriggerLevel)
						{
							g_unitConfig.alarmOneAirLevel = (uint32)g_triggerRecord.trec.airTriggerLevel;
						}
					}
					else // (g_triggerRecord.opMode == BARGRAPH_MODE) || (g_triggerRecord.opMode == COMBO_MODE)
					{
						g_alarmOneAirMinLevel = GetAirMinValue();
					}

					g_tempTriggerLevelForMenuAdjustment = AirTriggerConvertToUnits(g_unitConfig.alarmOneAirLevel);
					SETUP_USER_MENU_FOR_INTEGERS_MSG(&alarmOneAirLevelMenu, &g_tempTriggerLevelForMenuAdjustment, AirTriggerConvertToUnits(g_alarmOneAirMinLevel), AirTriggerConvertToUnits(g_alarmOneAirMinLevel), GetAirMaxValue());
				}
			break;

			case (ALARM_MODE_BOTH):
				if ((g_triggerRecord.opMode == WAVEFORM_MODE) && ((g_triggerRecord.trec.seismicTriggerLevel == NO_TRIGGER_CHAR) ||
					(g_triggerRecord.trec.airTriggerLevel == NO_TRIGGER_CHAR)))
				{
					sprintf((char*)g_spareBuffer, "%s %s. %s", getLangText(SEISMIC_OR_AIR_TRIGGER_TEXT), getLangText(SET_TO_NO_TRIGGER_TEXT), getLangText(PLEASE_CHANGE_TEXT));
					MessageBox(getLangText(WARNING_TEXT), (char*)g_spareBuffer, MB_OK);

					SETUP_USER_MENU_MSG(&alarmOneMenu, g_unitConfig.alarmOneMode);
				}
				else
				{
					// Setup Alarm One Seismic Level
					if (g_unitConfig.alarmOneSeismicLevel == NO_TRIGGER_CHAR)
					{
						g_unitConfig.alarmOneSeismicLevel = ALARM_ONE_SEIS_DEFAULT_TRIG_LVL;
					}

					if (g_triggerRecord.opMode == WAVEFORM_MODE)
					{
						g_alarmOneSeismicMinLevel = g_triggerRecord.trec.seismicTriggerLevel;

						if (g_unitConfig.alarmOneSeismicLevel < g_triggerRecord.trec.seismicTriggerLevel)
						{
							g_unitConfig.alarmOneSeismicLevel = g_triggerRecord.trec.seismicTriggerLevel;
						}
					}
					else // (g_triggerRecord.opMode == BARGRAPH_MODE) || (g_triggerRecord.opMode == COMBO_MODE)
					{
						g_alarmOneSeismicMinLevel = ALARM_SEIS_MIN_VALUE;
					}

					// Setup Alarm One Air Level
					if (g_unitConfig.alarmOneAirLevel == NO_TRIGGER_CHAR)
					{
						g_unitConfig.alarmOneAirLevel = ALARM_ONE_AIR_DEFAULT_TRIG_LVL;
					}

					if (g_triggerRecord.opMode == WAVEFORM_MODE)
					{
						g_alarmOneAirMinLevel = (uint16)g_triggerRecord.trec.airTriggerLevel;

						if (g_unitConfig.alarmOneAirLevel < (uint32)g_triggerRecord.trec.airTriggerLevel)
						{
							g_unitConfig.alarmOneAirLevel = (uint32)g_triggerRecord.trec.airTriggerLevel;
						}
					}
					else // (g_triggerRecord.opMode == BARGRAPH_MODE) || (g_triggerRecord.opMode == COMBO_MODE)
					{
						g_alarmOneAirMinLevel = GetAirMinValue();
					}

					// Down convert to current bit accuracy setting
					g_tempTriggerLevelForMenuAdjustment = SeismicTriggerConvertBitAccuracy(g_unitConfig.alarmOneSeismicLevel);

					// Call Alarm One Seismic Level
					SETUP_USER_MENU_FOR_INTEGERS_MSG(&alarmOneSeismicLevelMenu, &g_tempTriggerLevelForMenuAdjustment,
													(g_alarmOneSeismicMinLevel / (ALARM_SEIS_MAX_VALUE / g_bitAccuracyMidpoint)),
													(g_alarmOneSeismicMinLevel / (ALARM_SEIS_MAX_VALUE / g_bitAccuracyMidpoint)), g_bitAccuracyMidpoint);
				}
			break;
		}
	}
	else if (keyPressed == ESC_KEY)
	{
		if ((g_triggerRecord.opMode == WAVEFORM_MODE) || (g_triggerRecord.opMode == COMBO_MODE))
		{
			SETUP_USER_MENU_FOR_INTEGERS_MSG(&recordTimeMenu, &g_triggerRecord.trec.record_time,
				RECORD_TIME_DEFAULT_VALUE, RECORD_TIME_MIN_VALUE, g_triggerRecord.trec.record_time_max);
		}
		else if (g_triggerRecord.opMode == BARGRAPH_MODE)
		{
			if ((!g_factorySetupRecord.invalid) && (g_factorySetupRecord.aWeightOption == ENABLED))
			{
				SETUP_USER_MENU_MSG(&airScaleMenu, g_unitConfig.airScale);
			}
			else
			{
#if 0 /* Removing this option */
				SETUP_USER_MENU_MSG(&barResultMenu, g_unitConfig.vectorSum);
#else
				SETUP_USER_MENU_FOR_INTEGERS_MSG(&lcdImpulseTimeMenu, &g_triggerRecord.berec.impulseMenuUpdateSecs, LCD_IMPULSE_TIME_DEFAULT_VALUE, LCD_IMPULSE_TIME_MIN_VALUE, LCD_IMPULSE_TIME_MAX_VALUE);
#endif
			}
		}
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Alarm Two Menu
//=============================================================================
//*****************************************************************************
#define ALARM_TWO_MENU_ENTRIES 6
USER_MENU_STRUCT alarmTwoMenu[ALARM_TWO_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, ALARM_2_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, ALARM_TWO_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, OFF_TEXT,		NO_TAG, {ALARM_MODE_OFF}},
{ITEM_2, 0, SEISMIC_TEXT,	NO_TAG, {ALARM_MODE_SEISMIC}},
{ITEM_3, 0, AIR_TEXT,		NO_TAG, {ALARM_MODE_AIR}},
{ITEM_4, 0, BOTH_TEXT,		NO_TAG, {ALARM_MODE_BOTH}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&AlarmTwoMenuHandler}}
};

//-----------------------
// Alarm Two Menu Handler
//-----------------------
void AlarmTwoMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_unitConfig.alarmTwoMode = (uint8)alarmTwoMenu[newItemIndex].data;

		switch (g_unitConfig.alarmTwoMode)
		{
			case (ALARM_MODE_OFF):
				g_unitConfig.alarmTwoSeismicLevel = NO_TRIGGER_CHAR;
				g_unitConfig.alarmTwoAirLevel = NO_TRIGGER_CHAR;

				SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

				SETUP_USER_MENU_MSG(&saveSetupMenu, YES);
			break;

			case (ALARM_MODE_SEISMIC):
				if ((g_triggerRecord.opMode == WAVEFORM_MODE) && (g_triggerRecord.trec.seismicTriggerLevel == NO_TRIGGER_CHAR))
				{
					sprintf((char*)g_spareBuffer, "%s %s. %s", getLangText(SEISMIC_TRIGGER_TEXT), getLangText(SET_TO_NO_TRIGGER_TEXT), getLangText(PLEASE_CHANGE_TEXT));
					MessageBox(getLangText(WARNING_TEXT), (char*)g_spareBuffer, MB_OK);

					SETUP_USER_MENU_MSG(&alarmTwoMenu, g_unitConfig.alarmTwoMode);
				}
				else
				{
					g_unitConfig.alarmTwoAirLevel = NO_TRIGGER_CHAR;

					// Setup Alarm Two Seismic Level
					if (g_unitConfig.alarmTwoSeismicLevel == NO_TRIGGER_CHAR)
					{
						g_unitConfig.alarmTwoSeismicLevel = ALARM_ONE_SEIS_DEFAULT_TRIG_LVL;
					}

					if (g_triggerRecord.opMode == WAVEFORM_MODE)
					{
						g_alarmTwoSeismicMinLevel = g_triggerRecord.trec.seismicTriggerLevel;

						if (g_unitConfig.alarmTwoSeismicLevel < g_triggerRecord.trec.seismicTriggerLevel)
						{
							g_unitConfig.alarmTwoSeismicLevel = g_triggerRecord.trec.seismicTriggerLevel;
						}
					}
					else // (g_triggerRecord.opMode == BARGRAPH_MODE) || (g_triggerRecord.opMode == COMBO_MODE)
					{
						g_alarmTwoSeismicMinLevel = ALARM_SEIS_MIN_VALUE;
					}

					// Down convert to current bit accuracy setting
					g_tempTriggerLevelForMenuAdjustment = SeismicTriggerConvertBitAccuracy(g_unitConfig.alarmTwoSeismicLevel);

					// Call Alarm Two Seismic Level
					SETUP_USER_MENU_FOR_INTEGERS_MSG(&alarmTwoSeismicLevelMenu, &g_tempTriggerLevelForMenuAdjustment,
													(g_alarmTwoSeismicMinLevel / (ALARM_SEIS_MAX_VALUE / g_bitAccuracyMidpoint)),
													(g_alarmTwoSeismicMinLevel / (ALARM_SEIS_MAX_VALUE / g_bitAccuracyMidpoint)), g_bitAccuracyMidpoint);
				}
			break;

			case (ALARM_MODE_AIR):
				if ((g_triggerRecord.opMode == WAVEFORM_MODE) && (g_triggerRecord.trec.airTriggerLevel == NO_TRIGGER_CHAR))
				{
					sprintf((char*)g_spareBuffer, "%s %s. %s", getLangText(AIR_TRIGGER_TEXT), getLangText(SET_TO_NO_TRIGGER_TEXT), getLangText(PLEASE_CHANGE_TEXT));
					MessageBox(getLangText(WARNING_TEXT), (char*)g_spareBuffer, MB_OK);

					SETUP_USER_MENU_MSG(&alarmTwoMenu, g_unitConfig.alarmTwoMode);
				}
				else
				{
					g_unitConfig.alarmTwoSeismicLevel = NO_TRIGGER_CHAR;

					// Setup Alarm Two Air Level
					if (g_unitConfig.alarmTwoAirLevel == NO_TRIGGER_CHAR)
					{
						g_unitConfig.alarmTwoAirLevel = ALARM_TWO_AIR_DEFAULT_TRIG_LVL;
					}

					if (g_triggerRecord.opMode == WAVEFORM_MODE)
					{
						g_alarmTwoAirMinLevel = (uint16)g_triggerRecord.trec.airTriggerLevel;

						if (g_unitConfig.alarmTwoAirLevel < (uint32)g_triggerRecord.trec.airTriggerLevel)
						{
							g_unitConfig.alarmTwoAirLevel = (uint32)g_triggerRecord.trec.airTriggerLevel;
						}
					}
					else // (g_triggerRecord.opMode == BARGRAPH_MODE) || (g_triggerRecord.opMode == COMBO_MODE)
					{
						g_alarmTwoAirMinLevel = GetAirMinValue();
					}

					g_tempTriggerLevelForMenuAdjustment = AirTriggerConvertToUnits(g_unitConfig.alarmTwoAirLevel);
					SETUP_USER_MENU_FOR_INTEGERS_MSG(&alarmTwoAirLevelMenu, &g_tempTriggerLevelForMenuAdjustment, AirTriggerConvertToUnits(g_alarmTwoAirMinLevel), AirTriggerConvertToUnits(g_alarmTwoAirMinLevel), GetAirMaxValue());
				}
			break;

			case (ALARM_MODE_BOTH):
				if ((g_triggerRecord.opMode == WAVEFORM_MODE) && ((g_triggerRecord.trec.seismicTriggerLevel == NO_TRIGGER_CHAR) ||
					(g_triggerRecord.trec.airTriggerLevel == NO_TRIGGER_CHAR)))
				{
					sprintf((char*)g_spareBuffer, "%s %s. %s", getLangText(SEISMIC_OR_AIR_TRIGGER_TEXT), getLangText(SET_TO_NO_TRIGGER_TEXT), getLangText(PLEASE_CHANGE_TEXT));
					MessageBox(getLangText(WARNING_TEXT), (char*)g_spareBuffer, MB_OK);

					SETUP_USER_MENU_MSG(&alarmTwoMenu, g_unitConfig.alarmTwoMode);
				}
				else
				{
					// Setup Alarm Two Seismic Level
					if (g_unitConfig.alarmTwoSeismicLevel == NO_TRIGGER_CHAR)
					{
						g_unitConfig.alarmTwoSeismicLevel = ALARM_TWO_SEIS_DEFAULT_TRIG_LVL;
					}

					if (g_triggerRecord.opMode == WAVEFORM_MODE)
					{
						g_alarmTwoSeismicMinLevel = g_triggerRecord.trec.seismicTriggerLevel;

						if (g_unitConfig.alarmTwoSeismicLevel < g_triggerRecord.trec.seismicTriggerLevel)
						{
							g_unitConfig.alarmTwoSeismicLevel = g_triggerRecord.trec.seismicTriggerLevel;
						}
					}
					else // (g_triggerRecord.opMode == BARGRAPH_MODE) || (g_triggerRecord.opMode == COMBO_MODE)
					{
						g_alarmTwoSeismicMinLevel = ALARM_SEIS_MIN_VALUE;
					}

					// Setup Alarm Two Air Level
					if (g_unitConfig.alarmTwoAirLevel == NO_TRIGGER_CHAR)
					{
						g_unitConfig.alarmTwoAirLevel = ALARM_TWO_AIR_DEFAULT_TRIG_LVL;
					}

					if (g_triggerRecord.opMode == WAVEFORM_MODE)
					{
						g_alarmTwoAirMinLevel = (uint16)g_triggerRecord.trec.airTriggerLevel;

						if (g_unitConfig.alarmTwoAirLevel < (uint32)g_triggerRecord.trec.airTriggerLevel)
						{
							g_unitConfig.alarmTwoAirLevel = (uint32)g_triggerRecord.trec.airTriggerLevel;
						}
					}
					else // (g_triggerRecord.opMode == BARGRAPH_MODE) || (g_triggerRecord.opMode == COMBO_MODE)
					{
						g_alarmTwoAirMinLevel = GetAirMinValue();
					}

					// Down convert to current bit accuracy setting
					g_tempTriggerLevelForMenuAdjustment = SeismicTriggerConvertBitAccuracy(g_unitConfig.alarmTwoSeismicLevel);

					// Call Alarm Two Seismic Level
					SETUP_USER_MENU_FOR_INTEGERS_MSG(&alarmTwoSeismicLevelMenu, &g_tempTriggerLevelForMenuAdjustment,
													(g_alarmTwoSeismicMinLevel / (ALARM_SEIS_MAX_VALUE / g_bitAccuracyMidpoint)),
													(g_alarmTwoSeismicMinLevel / (ALARM_SEIS_MAX_VALUE / g_bitAccuracyMidpoint)), g_bitAccuracyMidpoint);
				}
			break;
		}
	}
	else if (keyPressed == ESC_KEY)
	{
		if (g_unitConfig.alarmOneMode == ALARM_MODE_OFF)
		{
			SETUP_USER_MENU_MSG(&alarmOneMenu, g_unitConfig.alarmOneMode);
		}
		else
		{
			SETUP_USER_MENU_FOR_FLOATS_MSG(&alarmOneTimeMenu, &g_unitConfig.alarmOneTime,
				ALARM_OUTPUT_TIME_DEFAULT, ALARM_OUTPUT_TIME_INCREMENT,
				ALARM_OUTPUT_TIME_MIN, ALARM_OUTPUT_TIME_MAX);
		}
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Alarm Output Menu
//=============================================================================
//*****************************************************************************
#define ALARM_OUTPUT_MENU_ENTRIES 4
USER_MENU_STRUCT alarmOutputMenu[ALARM_OUTPUT_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, ALARM_OUTPUT_MODE_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, ALARM_OUTPUT_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, DISABLED_TEXT,	NO_TAG, {DISABLED}},
{ITEM_2, 0, ENABLED_TEXT,	NO_TAG, {ENABLED}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&AlarmOutputMenuHandler}}
};

//--------------------------
// Alarm Output Menu Handler
//--------------------------
void AlarmOutputMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_unitConfig.alarmOneMode = (uint8)alarmOutputMenu[newItemIndex].data;
		g_unitConfig.alarmTwoMode = (uint8)alarmOutputMenu[newItemIndex].data;

		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, ALARM_OUTPUT_MODE);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Analog Channel Config Menu
//=============================================================================
//*****************************************************************************
#define ANALOG_CHANNEL_CONFIG_MENU_ENTRIES 4
USER_MENU_STRUCT analogChannelConfigMenu[ANALOG_CHANNEL_CONFIG_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, ANALOG_CHANNEL_CONFIG_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, ANALOG_CHANNEL_CONFIG_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_2)}},
{NO_TAG, 0, CHANNELS_R_AND_V_SCHEMATIC_TEXT,	NO_TAG, {CHANNELS_R_AND_V_SCHEMATIC}},
{NO_TAG, 0, CHANNELS_R_AND_V_SWAPPED_TEXT,		NO_TAG, {CHANNELS_R_AND_V_SWAPPED}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&AnalogChannelConfigMenuHandler}}
};

//-----------------------------------
// Analog Channel Config Menu Handler
//-----------------------------------
void AnalogChannelConfigMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_factorySetupRecord.analogChannelConfig = (uint8)analogChannelConfigMenu[newItemIndex].data;

		debug("Factory Setup: Channel R & V %s option selected\r\n", (g_factorySetupRecord.analogChannelConfig == CHANNELS_R_AND_V_SCHEMATIC) ? "Schematic" : "Swapped");

		SETUP_USER_MENU_MSG(&calibratonDateSourceMenu, g_factorySetupRecord.calibrationDateSource);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&acousticSensorTypeMenu, g_factorySetupRecord.acousticSensorType);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Auto Cal Menu
//=============================================================================
//*****************************************************************************
#define AUTO_CAL_MENU_ENTRIES 6
USER_MENU_STRUCT autoCalMenu[AUTO_CAL_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, AUTO_CALIBRATION_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, AUTO_CAL_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_4)}},
{ITEM_1, 0, AFTER_EVERY_24_HRS_TEXT,	NO_TAG, {AUTO_24_HOUR_TIMEOUT}},
{ITEM_2, 0, AFTER_EVERY_48_HRS_TEXT,	NO_TAG, {AUTO_48_HOUR_TIMEOUT}},
{ITEM_3, 0, AFTER_EVERY_72_HRS_TEXT,	NO_TAG, {AUTO_72_HOUR_TIMEOUT}},
{ITEM_4, 0, NO_AUTO_CAL_TEXT,			NO_TAG, {AUTO_NO_CAL_TIMEOUT}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&AutoCalMenuHandler}}
};

//----------------------
// Auto Cal Menu Handler
//----------------------
void AutoCalMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_unitConfig.autoCalMode = (uint8)autoCalMenu[newItemIndex].data;

		if (g_unitConfig.autoCalMode == AUTO_NO_CAL_TIMEOUT)
		{
			g_autoCalDaysToWait = 0;
		}
		else // Auto Cal enabled, set to process the first cycle change
		{
			g_autoCalDaysToWait = 1;
		}

		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, AUTO_CALIBRATION);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Auto Monitor Menu
//=============================================================================
//*****************************************************************************
#define AUTO_MONITOR_MENU_ENTRIES 6
USER_MENU_STRUCT autoMonitorMenu[AUTO_MONITOR_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, AUTO_MONITOR_AFTER_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, AUTO_MONITOR_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_4)}},
{ITEM_1, 2, MINUTES_TEXT,			NO_TAG, {AUTO_TWO_MIN_TIMEOUT}},
{ITEM_2, 3, MINUTES_TEXT,			NO_TAG, {AUTO_THREE_MIN_TIMEOUT}},
{ITEM_3, 4, MINUTES_TEXT,			NO_TAG, {AUTO_FOUR_MIN_TIMEOUT}},
{ITEM_4, 0, NO_AUTO_MONITOR_TEXT,	NO_TAG, {AUTO_NO_TIMEOUT}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&AutoMonitorMenuHandler}}
};

//--------------------------
// Auto Monitor Menu Handler
//--------------------------
void AutoMonitorMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_unitConfig.autoMonitorMode = (uint8)autoMonitorMenu[newItemIndex].data;

		AssignSoftTimer(AUTO_MONITOR_TIMER_NUM, (uint32)(g_unitConfig.autoMonitorMode * TICKS_PER_MIN), AutoMonitorTimerCallBack);

		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, AUTO_MONITOR);
	}

	JUMP_TO_ACTIVE_MENU();
}

#if 0 /* Removing this menu */
//*****************************************************************************
//=============================================================================
// Bar Channel Menu
//=============================================================================
//*****************************************************************************
#define BAR_CHANNEL_MENU_ENTRIES 5
USER_MENU_STRUCT barChannelMenu[BAR_CHANNEL_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, MONITOR_BARGRAPH_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, BAR_CHANNEL_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, BOTH_TEXT,		NO_TAG, {BAR_BOTH_CHANNELS}},
{ITEM_2, 0, SEISMIC_TEXT,	NO_TAG, {BAR_SEISMIC_CHANNEL}},
{ITEM_3, 0, AIR_TEXT,		NO_TAG, {BAR_AIR_CHANNEL}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&BarChannelMenuHandler}}
};

//-------------------------
// Bar Channel Menu Handler
//-------------------------
void BarChannelMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);
	uint16 gainFactor = (uint16)((g_triggerRecord.srec.sensitivity == LOW) ? 200 : 400);

	if (keyPressed == ENTER_KEY)
	{
		g_triggerRecord.berec.barChannel = (uint8)barChannelMenu[newItemIndex].data;

		if (g_factorySetupRecord.seismicSensorType == SENSOR_ACCELEROMETER)
		{
			sprintf((char*)&g_menuTags[BAR_SCALE_FULL_TAG].text, "100%% (%.0f mg)",
					(float)g_factorySetupRecord.seismicSensorType / (float)(gainFactor * BAR_SCALE_FULL));
			sprintf((char*)&g_menuTags[BAR_SCALE_HALF_TAG].text, " 50%% (%.0f mg)",
					(float)g_factorySetupRecord.seismicSensorType / (float)(gainFactor * BAR_SCALE_HALF));
			sprintf((char*)&g_menuTags[BAR_SCALE_QUARTER_TAG].text, " 25%% (%.0f mg)",
					(float)g_factorySetupRecord.seismicSensorType / (float)(gainFactor * BAR_SCALE_QUARTER));
			sprintf((char*)&g_menuTags[BAR_SCALE_EIGHTH_TAG].text, " 12%% (%.0f mg)",
					(float)g_factorySetupRecord.seismicSensorType / (float)(gainFactor * BAR_SCALE_EIGHTH));
		}
		if ((g_factorySetupRecord.seismicSensorType == SENSOR_ACC_832M1_0200) || (g_factorySetupRecord.seismicSensorType == SENSOR_ACC_832M1_0500))
		{
			sprintf((char*)&g_menuTags[BAR_SCALE_FULL_TAG].text, "100%% (%.0f mg)",
			(float)g_factorySetupRecord.seismicSensorType * ACC_832M1_SCALER / (float)(gainFactor * BAR_SCALE_FULL));
			sprintf((char*)&g_menuTags[BAR_SCALE_HALF_TAG].text, " 50%% (%.0f mg)",
			(float)g_factorySetupRecord.seismicSensorType * ACC_832M1_SCALER / (float)(gainFactor * BAR_SCALE_HALF));
			sprintf((char*)&g_menuTags[BAR_SCALE_QUARTER_TAG].text, " 25%% (%.0f mg)",
			(float)g_factorySetupRecord.seismicSensorType * ACC_832M1_SCALER / (float)(gainFactor * BAR_SCALE_QUARTER));
			sprintf((char*)&g_menuTags[BAR_SCALE_EIGHTH_TAG].text, " 12%% (%.0f mg)",
			(float)g_factorySetupRecord.seismicSensorType * ACC_832M1_SCALER / (float)(gainFactor * BAR_SCALE_EIGHTH));
		}
		else if (g_unitConfig.unitsOfMeasure == IMPERIAL_TYPE)
		{
			sprintf((char*)&g_menuTags[BAR_SCALE_FULL_TAG].text, "100%% (%.2f in/s)",
					(float)g_factorySetupRecord.seismicSensorType / (float)(gainFactor * BAR_SCALE_FULL));
			sprintf((char*)&g_menuTags[BAR_SCALE_HALF_TAG].text, " 50%% (%.2f in/s)",
					(float)g_factorySetupRecord.seismicSensorType / (float)(gainFactor * BAR_SCALE_HALF));
			sprintf((char*)&g_menuTags[BAR_SCALE_QUARTER_TAG].text, " 25%% (%.2f in/s)",
					(float)g_factorySetupRecord.seismicSensorType / (float)(gainFactor * BAR_SCALE_QUARTER));
			sprintf((char*)&g_menuTags[BAR_SCALE_EIGHTH_TAG].text, " 12%% (%.2f in/s)",
					(float)g_factorySetupRecord.seismicSensorType / (float)(gainFactor * BAR_SCALE_EIGHTH));
		}
		else // g_unitConfig.unitsOfMeasure == METRIC_TYPE
		{
			sprintf((char*)&g_menuTags[BAR_SCALE_FULL_TAG].text, "100%% (%lu mm/s)",
					(uint32)((float)g_factorySetupRecord.seismicSensorType * (float)25.4 /
					(float)(gainFactor * BAR_SCALE_FULL)));
			sprintf((char*)&g_menuTags[BAR_SCALE_HALF_TAG].text, " 50%% (%lu mm/s)",
					(uint32)((float)g_factorySetupRecord.seismicSensorType * (float)25.4 /
					(float)(gainFactor * BAR_SCALE_HALF)));
			sprintf((char*)&g_menuTags[BAR_SCALE_QUARTER_TAG].text, " 25%% (%lu mm/s)",
					(uint32)((float)g_factorySetupRecord.seismicSensorType * (float)25.4 /
					(float)(gainFactor * BAR_SCALE_QUARTER)));
			sprintf((char*)&g_menuTags[BAR_SCALE_EIGHTH_TAG].text, " 12%% (%lu mm/s)",
					(uint32)((float)g_factorySetupRecord.seismicSensorType * (float)25.4 /
					(float)(gainFactor * BAR_SCALE_EIGHTH)));
		}

#if 0 /* Removing Bar Scale menu (only applies to printing units) */
		SETUP_USER_MENU_MSG(&barScaleMenu, g_triggerRecord.berec.barScale);
#else
		SETUP_USER_MENU_MSG(&barIntervalMenu, g_triggerRecord.bgrec.barInterval);
#endif
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&sensitivityMenu, g_triggerRecord.srec.sensitivity);
	}

	JUMP_TO_ACTIVE_MENU();
}
#endif

//*****************************************************************************
//=============================================================================
// Bar Interval Menu
//=============================================================================
//*****************************************************************************
#define BAR_INTERVAL_MENU_ENTRIES 9
USER_MENU_STRUCT barIntervalMenu[BAR_INTERVAL_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, BAR_INTERVAL_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, BAR_INTERVAL_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_7)}},
{ITEM_1, 1,	SECOND_TEXT,	NO_TAG, {ONE_SEC_PRD}},
{ITEM_2, 10, SECONDS_TEXT,	NO_TAG, {TEN_SEC_PRD}},
{ITEM_3, 20, SECONDS_TEXT,	NO_TAG, {TWENTY_SEC_PRD}},
{ITEM_4, 30, SECONDS_TEXT,	NO_TAG, {THIRTY_SEC_PRD}},
{ITEM_5, 40, SECONDS_TEXT,	NO_TAG, {FOURTY_SEC_PRD}},
{ITEM_6, 50, SECONDS_TEXT,	NO_TAG, {FIFTY_SEC_PRD}},
{ITEM_7, 60, SECONDS_TEXT,	NO_TAG, {SIXTY_SEC_PRD}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&BarIntervalMenuHandler}}
};

//--------------------------
// Bar Interval Menu Handler
//--------------------------
void BarIntervalMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_triggerRecord.bgrec.barInterval = barIntervalMenu[newItemIndex].data;

		SETUP_USER_MENU_MSG(&summaryIntervalMenu, g_triggerRecord.bgrec.summaryInterval);
	}
	else if (keyPressed == ESC_KEY)
	{
#if 0 /* Removing Bar Scale menu (only applies to printing units) */
		SETUP_USER_MENU_MSG(&barScaleMenu, g_triggerRecord.berec.barScale);
#else
#if 0 /* Removing this option */
		SETUP_USER_MENU_MSG(&barChannelMenu, g_triggerRecord.berec.barChannel);
#else /* New path */
		SETUP_USER_MENU_MSG(&sensitivityMenu, g_triggerRecord.srec.sensitivity);
#endif
#endif
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Bar Interval Data Type Menu
//=============================================================================
//*****************************************************************************
#define BAR_INTERVAL_DATA_TYPE_MENU_ENTRIES 5
USER_MENU_STRUCT barIntervalDataTypeMenu[BAR_INTERVAL_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, BAR_DATA_TO_STORE_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, BAR_INTERVAL_DATA_TYPE_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0,	A_RVT_MAX_TEXT,			NO_TAG, {BAR_INTERVAL_ORIGINAL_DATA_TYPE_SIZE}},
{ITEM_2, 0, A_R_V_T_MAX_TEXT,		NO_TAG, {BAR_INTERVAL_A_R_V_T_DATA_TYPE_SIZE}},
{ITEM_3, 0, A_R_V_T_MAX_WITH_FREQ,	NO_TAG, {BAR_INTERVAL_A_R_V_T_WITH_FREQ_DATA_TYPE_SIZE}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&BarIntervalDataTypeMenuHandler}}
};

//------------------------------------
// Bar Interval Data Type Menu Handler
//------------------------------------
void BarIntervalDataTypeMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_triggerRecord.berec.barIntervalDataType = barIntervalDataTypeMenu[newItemIndex].data;

		SETUP_USER_MENU_FOR_INTEGERS_MSG(&lcdImpulseTimeMenu, &g_triggerRecord.berec.impulseMenuUpdateSecs, LCD_IMPULSE_TIME_DEFAULT_VALUE, LCD_IMPULSE_TIME_MIN_VALUE, LCD_IMPULSE_TIME_MAX_VALUE);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&summaryIntervalMenu, g_triggerRecord.bgrec.summaryInterval);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Bar Live Monitor Menu
//=============================================================================
//*****************************************************************************
#define BAR_LIVE_MONITOR_MENU_ENTRIES 4
USER_MENU_STRUCT barLiveMonitorMenu[BAR_LIVE_MONITOR_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, BAR_LIVE_MONITOR_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, BAR_LIVE_MONITOR_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, REMOTE_CONTROL_TEXT,	NO_TAG, {NO}},
{ITEM_2, 0, BLIND_SEND_TEXT,		NO_TAG,	{YES}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&BarLiveMonitorMenuHandler}}
};

//------------------------------
// Bar Live Monitor Menu Handler
//------------------------------
void BarLiveMonitorMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_unitConfig.barLiveMonitor = (uint8)barLiveMonitorMenu[newItemIndex].data;
		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);
		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, BAR_LIVE_MONITOR);
	}

	JUMP_TO_ACTIVE_MENU();
}

#if 0 /* Remvoing this menu */
//*****************************************************************************
//=============================================================================
// Bar Scale Menu
//=============================================================================
//*****************************************************************************
#define BAR_SCALE_MENU_ENTRIES 6
USER_MENU_STRUCT barScaleMenu[BAR_SCALE_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, BAR_SCALE_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, BAR_SCALE_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, NULL_TEXT,	BAR_SCALE_FULL_TAG,		{BAR_SCALE_FULL}},
{ITEM_2, 0, NULL_TEXT,	BAR_SCALE_HALF_TAG,		{BAR_SCALE_HALF}},
{ITEM_3, 0, NULL_TEXT,	BAR_SCALE_QUARTER_TAG,	{BAR_SCALE_QUARTER}},
{ITEM_4, 0, NULL_TEXT,	BAR_SCALE_EIGHTH_TAG,	{BAR_SCALE_EIGHTH}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&BarScaleMenuHandler}}
};

//--------------------------
// Bar Scale Menu Handler
//--------------------------
void BarScaleMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_triggerRecord.berec.barScale = (uint8)(barScaleMenu[newItemIndex].data);

		SETUP_USER_MENU_MSG(&barIntervalMenu, g_triggerRecord.bgrec.barInterval);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&barChannelMenu, g_triggerRecord.berec.barChannel);
	}

	JUMP_TO_ACTIVE_MENU();
}
#endif

//*****************************************************************************
//=============================================================================
// Calibration Date Source Menu
//=============================================================================
//*****************************************************************************
#define CALIBRATION_DATE_SOURCE_MENU_ENTRIES 5
USER_MENU_STRUCT calibratonDateSourceMenu[CALIBRATION_DATE_SOURCE_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, CAL_DATE_STORED_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, CALIBRATION_DATE_SOURCE_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, SENSOR_A_TEXT,	NO_TAG, {ACOUSTIC_SMART_SENSOR_CAL_DATE}},
{ITEM_2, 0, SENSOR_B_TEXT,	NO_TAG, {SEISMIC_SMART_SENSOR_CAL_DATE}},
{ITEM_3, 0, CALIBRATION_GRAPH_TEXT,	NO_TAG, {UNIT_CAL_DATE}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&CalibratonDateSourceMenuHandler}}
};

//-------------------------------------
// Calibration Date source Menu Handler
//-------------------------------------
void CalibratonDateSourceMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_factorySetupRecord.calibrationDateSource = calibratonDateSourceMenu[newItemIndex].data;

		if (g_factorySetupRecord.calibrationDateSource == UNIT_CAL_DATE) { debug("Factory Setup: Use Cal Date from: Unit\r\n"); }
		else { debug("Factory Setup: Use Cal Date from: %s Smart Sensor\r\n", (g_factorySetupRecord.calibrationDateSource == SEISMIC_SMART_SENSOR_CAL_DATE) ? "Seismic" : "Acoustic"); }

		SETUP_USER_MENU_MSG(&airSetupMenu, g_factorySetupRecord.aWeightOption);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&analogChannelConfigMenu, g_factorySetupRecord.analogChannelConfig);
	}

	JUMP_TO_ACTIVE_MENU();
}

#if 0 /* Removing this option */
//*****************************************************************************
//=============================================================================
// Bar Result Menu
//=============================================================================
//*****************************************************************************
#define BAR_RESULT_MENU_ENTRIES 4
USER_MENU_STRUCT barResultMenu[BAR_RESULT_MENU_ENTRIES] = {
{NO_TAG, 0, BAR_PRINTOUT_RESULTS_TEXT, NO_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, BAR_RESULT_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, PEAK_TEXT,			NO_TAG, {BAR_RESULT_PEAK}},
{ITEM_2, 0, VECTOR_SUM_TEXT,	NO_TAG, {BAR_RESULT_VECTOR_SUM}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&BarResultMenuHandler}}
};

//------------------------
// Bar Result Menu Handler
//------------------------
void BarResultMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_unitConfig.vectorSum = (uint8)barResultMenu[newItemIndex].data;
		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		// If Combo mode, jump back over to waveform specific settings
		if (g_triggerRecord.opMode == COMBO_MODE)
		{
#if VT_FEATURE_DISABLED // Original
			if (g_factorySetupRecord.seismicSensorType > SENSOR_ACC_RANGE_DIVIDER)
			{
				USER_MENU_DEFAULT_TYPE(seismicTriggerMenu) = MG_TYPE;
				USER_MENU_ALT_TYPE(seismicTriggerMenu) = MG_TYPE;
			}
			else
			{
				USER_MENU_DEFAULT_TYPE(seismicTriggerMenu) = IN_TYPE;
				USER_MENU_ALT_TYPE(seismicTriggerMenu) = MM_TYPE;
			}

			// Down convert to current bit accuracy setting
			g_tempTriggerLevelForMenuAdjustment = SeismicTriggerConvertBitAccuracy(g_triggerRecord.trec.seismicTriggerLevel);

			SETUP_USER_MENU_FOR_INTEGERS_MSG(&seismicTriggerMenu, &g_tempTriggerLevelForMenuAdjustment,
											(SEISMIC_TRIGGER_DEFAULT_VALUE / (SEISMIC_TRIGGER_MAX_VALUE / g_bitAccuracyMidpoint)),
											(SEISMIC_TRIGGER_MIN_VALUE / (SEISMIC_TRIGGER_MAX_VALUE / g_bitAccuracyMidpoint)), g_bitAccuracyMidpoint);
#else /* New VT feature */
			SETUP_USER_MENU_MSG(&seismicTriggerTypeMenu, g_triggerRecord.trec.variableTriggerEnable);
#endif
		}
		else if ((g_unitConfig.alarmOneMode) || (g_unitConfig.alarmTwoMode))
		{
			SETUP_USER_MENU_MSG(&alarmOneMenu, g_unitConfig.alarmOneMode);
		}
		else // Save setup
		{
			SETUP_USER_MENU_MSG(&saveSetupMenu, YES);
		}
	}
	else if (keyPressed == ESC_KEY)
	{
		if ((g_triggerRecord.opMode == BARGRAPH_MODE) && (!g_factorySetupRecord.invalid) && (g_factorySetupRecord.aWeightOption == ENABLED))
		{
			SETUP_USER_MENU_MSG(&airScaleMenu, g_unitConfig.airScale);
		}
		else
		{
			SETUP_USER_MENU_FOR_INTEGERS_MSG(&lcdImpulseTimeMenu, &g_triggerRecord.berec.impulseMenuUpdateSecs,	LCD_IMPULSE_TIME_DEFAULT_VALUE, LCD_IMPULSE_TIME_MIN_VALUE, LCD_IMPULSE_TIME_MAX_VALUE);
		}
	}

	JUMP_TO_ACTIVE_MENU();
}
#endif

//*****************************************************************************
//=============================================================================
// Baud Rate Menu
//=============================================================================
//*****************************************************************************
#define BAUD_RATE_MENU_ENTRIES 7
USER_MENU_STRUCT baudRateMenu[BAUD_RATE_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, BAUD_RATE_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, BAUD_RATE_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, BAUD_RATE_115200_TEXT,	NO_TAG, {BAUD_RATE_115200}},
{ITEM_2, 57600, BAUD_RATE_TEXT,	NO_TAG, {BAUD_RATE_57600}},
{ITEM_3, 38400, BAUD_RATE_TEXT,	NO_TAG, {BAUD_RATE_38400}},
{ITEM_4, 19200, BAUD_RATE_TEXT,	NO_TAG, {BAUD_RATE_19200}},
{ITEM_5, 9600, BAUD_RATE_TEXT,	NO_TAG, {BAUD_RATE_9600}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&BaudRateMenuHandler}}
};

//-----------------------
// Baud Rate Menu Handler
//-----------------------
#if 0 /* old hw */
#include "usart.h"
#endif
#include "RemoteHandler.h"
void BaudRateMenuHandler(uint8 keyPressed, void* data)
{
#if 0 /* temp remove while unused */
	uint32 usartRetries = 100; //USART_DEFAULT_TIMEOUT;
#endif
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);
#if 0 /* old hw */
	usart_options_t usart_1_rs232_options =
	{
		.charlength = 8,
		.paritytype = USART_NO_PARITY,
		.stopbits = USART_1_STOPBIT,
		.channelmode = USART_NORMAL_CHMODE
	};
#endif
	if (keyPressed == ENTER_KEY)
	{
		if (g_unitConfig.baudRate != baudRateMenu[newItemIndex].data)
		{
			g_unitConfig.baudRate = (uint8)baudRateMenu[newItemIndex].data;

			switch (baudRateMenu[newItemIndex].data)
			{
#if 0 /* old hw */
				case BAUD_RATE_115200: usart_1_rs232_options.baudrate = 115200; break;
				case BAUD_RATE_57600: usart_1_rs232_options.baudrate = 57600; break;
				case BAUD_RATE_38400: usart_1_rs232_options.baudrate = 38400; break;
				case BAUD_RATE_19200: usart_1_rs232_options.baudrate = 19200; break;
				case BAUD_RATE_9600: usart_1_rs232_options.baudrate = 9600; break;
#endif
			}

#if 0 /* old hw */
			// Check that receive is ready/idle
			while (((AVR32_USART1.csr & AVR32_USART_CSR_RXRDY_MASK) == 0) && usartRetries)
			{
				usartRetries--;
			}

			// Check that transmit is ready/idle
			usartRetries = 100; //USART_DEFAULT_TIMEOUT;
			while (((AVR32_USART1.csr & AVR32_USART_CSR_TXRDY_MASK) == 0) && usartRetries)
			{
				usartRetries--;
			}
#endif

#if 1 /* ns8100 (Added to help Dave's Supergraphics handle Baud change) */
			//-------------------------------------------------------------------------
			// Signal remote end that RS232 Comm is unavailable
			//-------------------------------------------------------------------------
			CLEAR_RTS; CLEAR_DTR;
#endif

#if 0 /* old hw */
			// Re-Initialize the RS232 with the new baud rate
			usart_init_rs232(&AVR32_USART1, &usart_1_rs232_options, FOSC0);
#endif

			InitCraftInterruptBuffers();

#if 1 /* ns8100 (Added to help Dave's Supergraphics handle Baud change) */
			//-------------------------------------------------------------------------
			// Signal remote end that RS232 Comm is available
			//-------------------------------------------------------------------------
			SET_RTS; SET_DTR;
#endif

			SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);
		}

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, BAUD_RATE);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Bit Accuracy Menu
//=============================================================================
//*****************************************************************************
#define BIT_ACCURACY_MENU_ENTRIES 6
USER_MENU_STRUCT bitAccuracyMenu[BIT_ACCURACY_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, BIT_ACCURACY_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, BIT_ACCURACY_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_3)}},
{ITEM_1, ACCURACY_16_BIT, BIT_TEXT, NO_TAG, {ACCURACY_16_BIT}},
{ITEM_2, ACCURACY_14_BIT, BIT_TEXT, NO_TAG, {ACCURACY_14_BIT}},
{ITEM_3, ACCURACY_12_BIT, BIT_TEXT, NO_TAG, {ACCURACY_12_BIT}},
{ITEM_4, ACCURACY_10_BIT, BIT_TEXT, NO_TAG, {ACCURACY_10_BIT}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&BitAccuracyMenuHandler}}
};

//-------------------------
// Bit Accuracy Menu Handler
//-------------------------
void BitAccuracyMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_triggerRecord.trec.bitAccuracy = bitAccuracyMenu[newItemIndex].data;

		switch (g_triggerRecord.trec.bitAccuracy)
		{
			case ACCURACY_10_BIT: { g_bitAccuracyMidpoint = ACCURACY_10_BIT_MIDPOINT; } break;
			case ACCURACY_12_BIT: { g_bitAccuracyMidpoint = ACCURACY_12_BIT_MIDPOINT; } break;
			case ACCURACY_14_BIT: { g_bitAccuracyMidpoint = ACCURACY_14_BIT_MIDPOINT; } break;
			default: { g_bitAccuracyMidpoint = ACCURACY_16_BIT_MIDPOINT; } break;
		}

		// Check if sample rate is 16K which can not process temp readings
		if (g_triggerRecord.trec.sample_rate == SAMPLE_RATE_16K)
		{
			SETUP_USER_MENU_MSG(&companyMenu, &g_triggerRecord.trec.client);
		}
		else // All other sample rates
		{
			SETUP_USER_MENU_MSG(&recalibrateMenu, g_triggerRecord.trec.adjustForTempDrift);
		}
	}
	else if (keyPressed == ESC_KEY)
	{
		// New Adaptive Sampling
		if ((g_triggerRecord.trec.sample_rate == SAMPLE_RATE_1K) || (g_unitConfig.adaptiveSampling != ENABLED))
		{
			if (g_triggerRecord.opMode == BARGRAPH_MODE)
			{
				SETUP_USER_MENU_MSG(&sampleRateBargraphMenu, g_triggerRecord.trec.sample_rate);
			}
			else if (g_triggerRecord.opMode == COMBO_MODE)
			{
				SETUP_USER_MENU_MSG(&sampleRateComboMenu, g_triggerRecord.trec.sample_rate);
			}
			else
			{
				SETUP_USER_MENU_MSG(&sampleRateMenu, g_triggerRecord.trec.sample_rate);
			}
		}
		else
		{
			SETUP_USER_MENU_MSG(&samplingMethodMenu, g_triggerRecord.trec.samplingMethod);
		}
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Config Menu
//=============================================================================
//*****************************************************************************
#define CONFIG_MENU_ENTRIES 38
USER_MENU_STRUCT configMenu[CONFIG_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, CONFIG_OPTIONS_MENU_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, CONFIG_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{NO_TAG, 0, ADAPTIVE_SAMPLING_TEXT,		NO_TAG, {ADAPTIVE_SAMPLING}},
{NO_TAG, 0, ALARM_OUTPUT_MODE_TEXT,		NO_TAG, {ALARM_OUTPUT_MODE}},
{NO_TAG, 0, AUTO_CALIBRATION_TEXT,		NO_TAG, {AUTO_CALIBRATION}},
{NO_TAG, 0, AUTO_DIAL_INFO_TEXT,		NO_TAG, {AUTO_DIAL_INFO}},
{NO_TAG, 0, AUTO_MONITOR_TEXT,			NO_TAG, {AUTO_MONITOR}},
{NO_TAG, 0, BAR_LIVE_MONITOR_TEXT,		NO_TAG, {BAR_LIVE_MONITOR}},
{NO_TAG, 0, BATTERY_TEXT,				NO_TAG, {BATTERY}},
{NO_TAG, 0, BAUD_RATE_TEXT,				NO_TAG, {BAUD_RATE}},
{NO_TAG, 0, CALIBRATION_DATE_TEXT,		NO_TAG, {CALIBRATION_DATE}},
{NO_TAG, 0, CHAN_VERIFICATION_TEXT,		NO_TAG, {CHANNEL_VERIFICATION}},
{NO_TAG, 0, CYCLE_END_TIME_24HR_TEXT,	NO_TAG, {CYCLE_END_TIME_HOUR}},
{NO_TAG, 0, DATE_TIME_TEXT,				NO_TAG, {DATE_TIME}},
{NO_TAG, 0, ERASE_MEMORY_TEXT,			NO_TAG, {ERASE_FLASH}},
{NO_TAG, 0, EVENT_SUMMARIES_TEXT,		NO_TAG, {EVENT_SUMMARIES}},
{NO_TAG, 0, EXTERNAL_TRIGGER_TEXT,		NO_TAG, {EXTERNAL_TRIGGER}},
{NO_TAG, 0, FLASH_WRAPPING_TEXT,		NO_TAG, {FLASH_WRAPPING}},
{NO_TAG, 0, FLASH_STATS_TEXT,			NO_TAG, {FLASH_STATS}},
{NO_TAG, 0, GPS_POWER_TEXT,				NO_TAG, {GPS_POWER}},
{NO_TAG, 0, LANGUAGE_TEXT,				NO_TAG, {LANGUAGE}},
{NO_TAG, 0, LCD_CONTRAST_TEXT,			NO_TAG, {LCD_CONTRAST}},
{NO_TAG, 0, LCD_TIMEOUT_TEXT,			NO_TAG, {LCD_TIMEOUT}},
#if 1 /* New */
{NO_TAG, 0, LEGACY_DQM_LIMIT_TEXT,		NO_TAG, {LEGACY_DQM_LIMIT}},
#endif
{NO_TAG, 0, MODEM_SETUP_TEXT,			NO_TAG, {MODEM_SETUP}},
{NO_TAG, 0, MONITOR_LOG_TEXT,			NO_TAG, {MONITOR_LOG}},
{NO_TAG, 0, PRETRIGGER_SIZE_TEXT,		NO_TAG, {PRETRIGGER_SIZE}},
{NO_TAG, 0, RS232_POWER_SAVINGS_TEXT,	NO_TAG, {RS232_POWER_SAVINGS}},
#if 0 /* No longer a unit configurable setting */
{NO_TAG, 0, REPORT_DISPLACEMENT_TEXT,	NO_TAG, {REPORT_DISPLACEMENT}},
{NO_TAG, 0, REPORT_PEAK_ACC_TEXT,		NO_TAG, {REPORT_PEAK_ACC}},
#endif
{NO_TAG, 0, SAVE_COMPRESSED_DATA_TEXT,	NO_TAG, {SAVE_COMPRESSED_DATA}},
{NO_TAG, 0, SENSOR_GAIN_TYPE_TEXT,		NO_TAG, {SENSOR_GAIN_TYPE}},
{NO_TAG, 0, SERIAL_NUMBER_TEXT,			NO_TAG, {SERIAL_NUMBER}},
#if 1 /* New */
{NO_TAG, 0, STORED_EVENTS_CAP_TEXT,		NO_TAG, {STORED_EVENTS_CAP_MODE}},
#endif
{NO_TAG, 0, TIMER_MODE_TEXT,			NO_TAG, {TIMER_MODE}},
{NO_TAG, 0, UNITS_OF_MEASURE_TEXT,		NO_TAG, {UNITS_OF_MEASURE}},
{NO_TAG, 0, UNITS_OF_AIR_TEXT,			NO_TAG, {UNITS_OF_AIR}},
{NO_TAG, 0, USB_SYNC_MODE_TEXT,			NO_TAG, {USB_SYNC_MODE}},
{NO_TAG, 0, UTC_ZONE_OFFSET_TEXT,		NO_TAG, {UTC_ZONE_OFFSET}},
#if 0 /* Removing this option */
{NO_TAG, 0, VECTOR_SUM_TEXT,			NO_TAG, {VECTOR_SUM}},
#endif
{NO_TAG, 0, WAVEFORM_AUTO_CAL_TEXT,		NO_TAG, {WAVEFORM_AUTO_CAL}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&ConfigMenuHandler}}
};

//--------------------
// Config Menu Handler
//--------------------
void ConfigMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		switch (configMenu[newItemIndex].data)
		{
			case (ADAPTIVE_SAMPLE_RATE):
				SETUP_USER_MENU_MSG(&adaptiveSamplingMenu, g_unitConfig.adaptiveSampling);
			break;

			case (ALARM_OUTPUT_MODE):
				SETUP_USER_MENU_MSG(&alarmOutputMenu, (g_unitConfig.alarmOneMode | g_unitConfig.alarmTwoMode));
			break;

			case (AUTO_CALIBRATION):
				SETUP_USER_MENU_MSG(&autoCalMenu, g_unitConfig.autoCalMode);
			break;

			case (AUTO_DIAL_INFO):
				DisplayAutoDialInfo();
			break;

			case (AUTO_MONITOR):
				SETUP_USER_MENU_MSG(&autoMonitorMenu, g_unitConfig.autoMonitorMode);
			break;

			case (BAR_LIVE_MONITOR):
				SETUP_USER_MENU_MSG(&barLiveMonitorMenu, g_unitConfig.barLiveMonitor);
			break;

			case (BATTERY):
				SETUP_MENU_MSG(BATTERY_MENU);
			break;

			case (BAUD_RATE):
				SETUP_USER_MENU_MSG(&baudRateMenu, g_unitConfig.baudRate);
			break;

			case (CALIBRATION_DATE):
				DisplayCalDate();
			break;

			case (CHANNEL_VERIFICATION):
				SETUP_USER_MENU_MSG(&adChannelVerificationMenu, g_unitConfig.adChannelVerification);
			break;

#if 0 /* Unused */
			case (COPIES):
				MessageBox(getLangText(STATUS_TEXT), getLangText(NOT_INCLUDED_TEXT), MB_OK);
			break;
#endif
			case (CYCLE_END_TIME_HOUR):
				SETUP_USER_MENU_FOR_INTEGERS_MSG(&cycleEndTimeMenu, &g_unitConfig.cycleEndTimeHour, CYCLE_END_TIME_HOUR_DEFAULT_VALUE, CYCLE_END_TIME_HOUR_MIN_VALUE, CYCLE_END_TIME_HOUR_MAX_VALUE);
			break;

			case (DATE_TIME):
				SETUP_MENU_MSG(DATE_TIME_MENU);
			break;

			case (ERASE_FLASH):
				SETUP_USER_MENU_MSG(&eraseEventsMenu, YES);
			break;

			case (EVENT_SUMMARIES):
				// Display a message to be patient while the software loads info from event files
				OverlayMessage(getLangText(STATUS_TEXT), getLangText(PLEASE_BE_PATIENT_TEXT), 0);

				SETUP_MENU_MSG(SUMMARY_MENU); mn_msg.data[0] = START_FROM_TOP;
			break;

			case (EXTERNAL_TRIGGER):
				g_externalTriggerMenuActiveForSetup = NO;
				SETUP_USER_MENU_MSG(&externalTriggerMenu, g_unitConfig.externalTrigger);
			break;

			case (FLASH_WRAPPING):
				SETUP_USER_MENU_MSG(&flashWrappingMenu, g_unitConfig.flashWrapping);
			break;

			case (FLASH_STATS):
				// Disable USB if there is an active connection
				UsbDisableIfActive();

				OverlayMessage(getLangText(STATUS_TEXT), getLangText(CALCULATING_EVENT_STORAGE_SPACE_FREE_TEXT), 0);
				GetSDCardUsageStats();
				DisplayFlashUsageStats();
			break;

			case (FREQUENCY_PLOT):
				MessageBox(getLangText(STATUS_TEXT), getLangText(NOT_INCLUDED_TEXT), MB_OK);
			break;

			case (GPS_POWER):
				SETUP_USER_MENU_MSG(&gpsPowerMenu, g_unitConfig.gpsPowerMode);
			break;

			case (LANGUAGE):
				SETUP_USER_MENU_MSG(&languageMenu, g_unitConfig.languageMode);
			break;

			case (LCD_CONTRAST):
				SETUP_MENU_MSG(LCD_CONTRAST_MENU);
			break;

			case (LCD_TIMEOUT):
				SETUP_USER_MENU_FOR_INTEGERS_MSG(&lcdTimeoutMenu, &g_unitConfig.lcdTimeout, LCD_TIMEOUT_DEFAULT_VALUE, LCD_TIMEOUT_MIN_VALUE, LCD_TIMEOUT_MAX_VALUE);
			break;

			case (LEGACY_DQM_LIMIT):
				SETUP_USER_MENU_MSG(&legacyDqmLimitMenu, g_unitConfig.legacyDqmLimit);
			break;

			case (MODEM_SETUP):
				SETUP_USER_MENU_MSG(&modemSetupMenu, g_modemSetupRecord.modemStatus);
			break;

			case (MONITOR_LOG):
#if 0 /* Original */
				SETUP_USER_MENU_MSG(&monitorLogMenu, DEFAULT_ITEM_1);
#else /* Jump right to the source */
				SETUP_MENU_MSG(VIEW_MONITOR_LOG_MENU);
#endif
			break;

			case (PRETRIGGER_SIZE):
				SETUP_USER_MENU_MSG(&pretriggerSizeMenu, g_unitConfig.pretrigBufferDivider);
			break;

			case (PRINTER):
				MessageBox(getLangText(STATUS_TEXT), getLangText(NOT_INCLUDED_TEXT), MB_OK);
			break;

			case (PRINT_MONITOR_LOG):
				MessageBox(getLangText(STATUS_TEXT), getLangText(NOT_INCLUDED_TEXT), MB_OK);
			break;

			case (RS232_POWER_SAVINGS):
				SETUP_USER_MENU_MSG(&rs232PowerSavingsMenu, g_unitConfig.rs232PowerSavings);
			break;
#if 0 /* No longer a unit configurable setting */
			case (REPORT_DISPLACEMENT):
				SETUP_USER_MENU_MSG(&displacementMenu, g_unitConfig.reportDisplacement);
			break;
#endif
#if 0 /* No longer a unit configurable setting */
			case (REPORT_PEAK_ACC):
				SETUP_USER_MENU_MSG(&peakAccMenu, g_unitConfig.reportPeakAcceleration);
			break;
#endif
			case (SAVE_COMPRESSED_DATA):
				SETUP_USER_MENU_MSG(&saveCompressedDataMenu, g_unitConfig.saveCompressedData);
			break;

			case (SENSOR_GAIN_TYPE):
				DisplaySensorType();
			break;

			case (SERIAL_NUMBER):
				DisplaySerialNumber();
			break;

			case (STORED_EVENTS_CAP_MODE):
				SETUP_USER_MENU_MSG(&storedEventsCapModeMenu, g_unitConfig.storedEventsCapMode);
			break;
			case (TIMER_MODE):
				if (g_unitConfig.timerMode == ENABLED)
				{
					// Show current Timer Mode settings
					DisplayTimerModeSettings();

					// Check if the user wants to cancel the current timer mode
					if (MessageBox(getLangText(STATUS_TEXT), getLangText(CANCEL_TIMER_MODE_Q_TEXT), MB_YESNO) == MB_FIRST_CHOICE)
					{
						g_unitConfig.timerMode = DISABLED;

						// Disable the Power Off timer if it's set
						ClearSoftTimer(POWER_OFF_TIMER_NUM);

						SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

						SETUP_USER_MENU_MSG(&timerModeMenu, g_unitConfig.timerMode);
					}
					else // User did not cancel timer mode
					{
						SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
					}
				}
				else // Timer mode is disabled
				{
					if (CheckAndDisplayErrorThatPreventsMonitoring(PROMPT) == NO)
					{
						SETUP_USER_MENU_MSG(&timerModeMenu, g_unitConfig.timerMode);
					}
				}
			break;

			case (UNITS_OF_MEASURE):
				SETUP_USER_MENU_MSG(&unitsOfMeasureMenu, g_unitConfig.unitsOfMeasure);
			break;

			case (UNITS_OF_AIR):
				SETUP_USER_MENU_MSG(&unitsOfAirMenu, g_unitConfig.unitsOfAir);
			break;

			case (USB_SYNC_MODE):
				SETUP_USER_MENU_MSG(&usbSyncModeMenu, g_unitConfig.usbSyncMode);
			break;

			case (UTC_ZONE_OFFSET):
				// Adjust the zone offset by the default to get to an all positive range
				g_unitConfig.utcZoneOffset += 12;
				SETUP_USER_MENU_FOR_INTEGERS_MSG(&utcZoneOffsetMenu, &g_unitConfig.utcZoneOffset, 12, 0, 24);
			break;

#if 0 /* Removing this option */
			case (VECTOR_SUM):
				SETUP_USER_MENU_MSG(&vectorSumMenu, g_unitConfig.vectorSum);
			break;
#endif
			case (WAVEFORM_AUTO_CAL):
				SETUP_USER_MENU_MSG(&waveformAutoCalMenu, g_unitConfig.autoCalForWaveform);
			break;
		}
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&helpMenu, CONFIG_CHOICE);
	}

	JUMP_TO_ACTIVE_MENU();
}

#if (!VT_FEATURE_DISABLED)
//*****************************************************************************
//=============================================================================
// Custom Curve Menu
//=============================================================================
//*****************************************************************************
#define CUSTOM_CURVE_MENU_ENTRIES 4
USER_MENU_STRUCT customCurveMenu[CUSTOM_CURVE_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, CUSTOM_CURVE_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, CUSTOM_CURVE_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, STEP_THRESHOLD_TEXT,	NO_TAG,	{CUSTOM_STEP_THRESHOLD}},
{ITEM_2, 0, STEP_LIMITING_TEXT,		NO_TAG,	{CUSTOM_STEP_LIMITING}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&CustomCurveMenuHandler}}
};

//----------------------------
// Custom Curve Menu Handler
//----------------------------
void CustomCurveMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);
#if 0 /* Removed for new Percent of Limit Trigger feature */
	float div = (float)(g_bitAccuracyMidpoint * g_sensorInfo.sensorAccuracy * ((g_triggerRecord.srec.sensitivity == LOW) ? 2 : 4)) / (float)(g_factorySetupRecord.seismicSensorType);
#endif

	if (keyPressed == ENTER_KEY)
	{
		g_triggerRecord.trec.variableTriggerVibrationStandard = (uint8)customCurveMenu[newItemIndex].data;

#if 0 /* Removed for new Percent of Limit Trigger feature */
		g_triggerRecord.trec.variableTriggerEnable = YES;

		if (g_triggerRecord.trec.variableTriggerVibrationStandard == CUSTOM_STEP_THRESHOLD)
		{
			// Set the fixed trigger level to 1 IPS for this custom curve
			g_triggerRecord.trec.seismicTriggerLevel = (uint32)(1.00 * div);
		}
		else // (g_triggerRecord.trec.variableTriggerVibrationStandard == CUSTOM_STEP_LIMITING)
		{
			// Set the fixed trigger level to 2 IPS for this custom curve
			g_triggerRecord.trec.seismicTriggerLevel = (uint32)(2.00 * div);
		}

		// Up convert to 16-bit since user selected level is based on selected bit accuracy
		g_triggerRecord.trec.seismicTriggerLevel *= (SEISMIC_TRIGGER_MAX_VALUE / g_bitAccuracyMidpoint);

		debug("Seismic Trigger: %d counts\r\n", g_triggerRecord.trec.seismicTriggerLevel);

		g_tempTriggerLevelForMenuAdjustment = AirTriggerConvertToUnits(g_triggerRecord.trec.airTriggerLevel);
		SETUP_USER_MENU_FOR_INTEGERS_MSG(&airTriggerMenu, &g_tempTriggerLevelForMenuAdjustment, GetAirDefaultValue(), GetAirMinValue(), GetAirMaxValue());
#else /* New Percent of Limit Trigger feature */
		SETUP_USER_MENU_FOR_INTEGERS_MSG(&percentLimitTriggerMenu, &g_triggerRecord.trec.variableTriggerPercentageLevel, VT_PERCENT_OF_LIMIT_DEFAULT_VALUE, VT_PERCENT_OF_LIMIT_MIN_VALUE, VT_PERCENT_OF_LIMIT_MAX_VALUE);
#endif
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&vibrationStandardMenu, START_OF_CUSTOM_CURVES_LIST);
	}

	JUMP_TO_ACTIVE_MENU();
}
#endif

#if 0 /* Unused */
//*****************************************************************************
//=============================================================================
// Displacement Menu
//=============================================================================
//*****************************************************************************
#define DISPLACEMENT_MENU_ENTRIES 4
USER_MENU_STRUCT displacementMenu[DISPLACEMENT_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, REPORT_DISPLACEMENT_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, DISPLACEMENT_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_2)}},
{ITEM_1, 0, YES_TEXT,	NO_TAG, {YES}},
{ITEM_2, 0, NO_TEXT,	NO_TAG, {NO}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&DisplacementMenuHandler}}
};

//------------------------------
// Displacement Menu Handler
//------------------------------
void DisplacementMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_unitConfig.reportDisplacement = (uint8)displacementMenu[newItemIndex].data;

		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, REPORT_DISPLACEMENT);
	}

	JUMP_TO_ACTIVE_MENU();
}
#endif

//*****************************************************************************
//=============================================================================
// Erase Events Menu
//=============================================================================
//*****************************************************************************
#define ERASE_EVENTS_MENU_ENTRIES 4
USER_MENU_STRUCT eraseEventsMenu[ERASE_EVENTS_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, ERASE_EVENTS_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, ERASE_EVENTS_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, YES_TEXT,	NO_TAG, {YES}},
{ITEM_2, 0, NO_TEXT,	NO_TAG, {NO}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&EraseEventsMenuHandler}}
};

//--------------------------
// Erase Events Menu Handler
//--------------------------
void EraseEventsMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);
	uint8 choice = MB_SECOND_CHOICE;

	if (keyPressed == ENTER_KEY)
	{
		if (eraseEventsMenu[newItemIndex].data == YES)
		{
			// Check if the user really wants to erase all the events
			choice = MessageBox(getLangText(CONFIRM_TEXT), getLangText(REALLY_ERASE_ALL_EVENTS_Q_TEXT), MB_YESNO);

			// User selected Yes
			if (choice == MB_FIRST_CHOICE)
			{
				// Warn the user that turning off the power at this point is bad
				MessageBox(getLangText(WARNING_TEXT), getLangText(DO_NOT_TURN_THE_UNIT_OFF_UNTIL_THE_OPERATION_IS_COMPLETE_TEXT), MB_OK);

				// Display a message alerting the user that the erase operation is in progress
				sprintf((char*)g_spareBuffer, "%s %s", getLangText(ERASE_OPERATION_IN_PROGRESS_TEXT), getLangText(PLEASE_BE_PATIENT_TEXT));
				OverlayMessage(getLangText(STATUS_TEXT), (char*)g_spareBuffer, 0);

				// Disable USB if there is an active connection
				UsbDisableIfActive();

				// Delete events, recalculate space and reinitialize tables
				DeleteEventFileRecords();

				// Recalculate free space and init buffers
				OverlayMessage(getLangText(STATUS_TEXT), getLangText(CALCULATING_EVENT_STORAGE_SPACE_FREE_TEXT), 0);
				GetSDCardUsageStats();
				InitEventNumberCache();

				// Display a message that the operation is complete
				OverlayMessage(getLangText(SUCCESS_TEXT), getLangText(ERASE_COMPLETE_TEXT), (2 * SOFT_SECS));

				// Call routine to reset the LCD display timers
				KeypressEventMgr();

				SETUP_USER_MENU_MSG(&zeroEventNumberMenu, YES);
			}
		}
		else
		{
			SETUP_USER_MENU_MSG(&eraseSettingsMenu, NO);
		}
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, ERASE_FLASH);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Erase Settings Menu
//=============================================================================
//*****************************************************************************
#define ERASE_SETTINGS_MENU_ENTRIES 4
USER_MENU_STRUCT eraseSettingsMenu[ERASE_SETTINGS_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, ERASE_SETTINGS_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, ERASE_SETTINGS_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_2)}},
{ITEM_1, 0, YES_TEXT,	NO_TAG, {YES}},
{ITEM_2, 0, NO_TEXT,	NO_TAG, {NO}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&EraseSettingsMenuHandler}}
};

//----------------------------
// Erase Settings Menu Handler
//----------------------------
void EraseSettingsMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);
	MONITOR_LOG_ID_STRUCT monitorLogRecord;

	if (keyPressed == ENTER_KEY)
	{
		if (eraseEventsMenu[newItemIndex].data == YES)
		{
			MessageBox(getLangText(WARNING_TEXT), getLangText(DO_NOT_TURN_THE_UNIT_OFF_UNTIL_THE_OPERATION_IS_COMPLETE_TEXT), MB_OK);

			OverlayMessage(getLangText(STATUS_TEXT), getLangText(ERASE_OPERATION_IN_PROGRESS_TEXT), 0);

			// Erase all used parameter memory (including factory setup to be copied right back after)
			EraseParameterMemory(0, ((sizeof(REC_EVENT_MN_STRUCT) * (MAX_NUM_OF_SAVED_SETUPS + 1)) +
									sizeof(UNIT_CONFIG_STRUCT) + sizeof(MODEM_SETUP_STRUCT)));

			// Reprogram Factory Setup record (re-write after erase)
			SaveRecordData(&g_factorySetupRecord, DEFAULT_RECORD, REC_FACTORY_SETUP_TYPE);
			// Just in case, re-init Sensor Parameters
			InitSensorParameters(g_factorySetupRecord.seismicSensorType, (uint8)g_triggerRecord.srec.sensitivity);

			// Load Defaults for Waveform
			LoadTrigRecordDefaults(&g_triggerRecord, WAVEFORM_MODE);
			SaveRecordData(&g_triggerRecord, DEFAULT_RECORD, REC_TRIGGER_USER_MENU_TYPE);

			// Load Unit Config defaults
			LoadUnitConfigDefaults((UNIT_CONFIG_STRUCT*)&g_unitConfig);
			SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

			// Clear out modem setup and save
			memset(&g_modemSetupRecord, 0, sizeof(g_modemSetupRecord));
			g_modemSetupRecord.modemStatus = NO;
			SaveRecordData(&g_modemSetupRecord, DEFAULT_RECORD, REC_MODEM_SETUP_TYPE);

			// Clear out Monitor Log unique ID
			memset(&monitorLogRecord, 0, sizeof(monitorLogRecord));
			SaveRecordData(&monitorLogRecord, DEFAULT_RECORD, REC_UNIQUE_MONITOR_LOG_ID_CLEAR_TYPE);
			InitMonitorLogUniqueEntryId();

			OverlayMessage(getLangText(SUCCESS_TEXT), getLangText(ERASE_COMPLETE_TEXT), (2 * SOFT_SECS));
		}

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&eraseEventsMenu, YES);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// External Trigger Menu
//=============================================================================
//*****************************************************************************
#define EXTERNAL_TRIGGER_MENU_ENTRIES 4
USER_MENU_STRUCT externalTriggerMenu[EXTERNAL_TRIGGER_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, EXTERNAL_TRIGGER_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, EXTERNAL_TRIGGER_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, ENABLED_TEXT,	NO_TAG, {ENABLED}},
{ITEM_2, 0, DISABLED_TEXT,	NO_TAG, {DISABLED}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&ExternalTriggerMenuHandler}}
};

//------------------------------
// External Trigger Menu Handler
//------------------------------
void ExternalTriggerMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		if ((g_externalTriggerMenuActiveForSetup == YES) && ((uint8)(externalTriggerMenu[newItemIndex].data) == DISABLED) &&
			(g_triggerRecord.trec.airTriggerLevel == NO_TRIGGER_CHAR) && (g_triggerRecord.trec.seismicTriggerLevel == NO_TRIGGER_CHAR))
		{
			MessageBox(getLangText(WARNING_TEXT), getLangText(BOTH_TRIGGERS_SET_TO_NO_AND_EXTERNAL_TRIGGER_DISABLED_TEXT), MB_OK);
			sprintf((char*)g_spareBuffer, "%s. %s", getLangText(NO_TRIGGER_SOURCE_SELECTED_TEXT), getLangText(PLEASE_CHANGE_TEXT));
			MessageBox(getLangText(WARNING_TEXT), (char*)g_spareBuffer, MB_OK);

			if (g_factorySetupRecord.seismicSensorType > SENSOR_ACC_RANGE_DIVIDER)
			{
				USER_MENU_DEFAULT_TYPE(seismicTriggerMenu) = MG_TYPE;
				USER_MENU_ALT_TYPE(seismicTriggerMenu) = MG_TYPE;
			}
			else
			{
				USER_MENU_DEFAULT_TYPE(seismicTriggerMenu) = IN_TYPE;
				USER_MENU_ALT_TYPE(seismicTriggerMenu) = MM_TYPE;
			}

			// Down convert to current bit accuracy setting
			g_tempTriggerLevelForMenuAdjustment = SeismicTriggerConvertBitAccuracy(g_triggerRecord.trec.seismicTriggerLevel);

			SETUP_USER_MENU_FOR_INTEGERS_MSG(&seismicTriggerMenu, &g_tempTriggerLevelForMenuAdjustment,
											(SEISMIC_TRIGGER_DEFAULT_VALUE / (SEISMIC_TRIGGER_MAX_VALUE / g_bitAccuracyMidpoint)),
											(SEISMIC_TRIGGER_MIN_VALUE / (SEISMIC_TRIGGER_MAX_VALUE / g_bitAccuracyMidpoint)), g_bitAccuracyMidpoint);
		}
		else
		{
			g_unitConfig.externalTrigger = (uint8)(externalTriggerMenu[newItemIndex].data);

			SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

			if (g_externalTriggerMenuActiveForSetup == NO)
			{
				SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
			}
			else // (g_externalTriggerMenuActiveForSetup == YES)
			{
				// Check if the A-weighting option is enabled
				if ((!g_factorySetupRecord.invalid) && (g_factorySetupRecord.aWeightOption == ENABLED))
				{
					SETUP_USER_MENU_MSG(&airScaleMenu, g_unitConfig.airScale);
				}
				else // A-weighting is not enabled
				{
					SETUP_USER_MENU_FOR_INTEGERS_MSG(&recordTimeMenu, &g_triggerRecord.trec.record_time,
					RECORD_TIME_DEFAULT_VALUE, RECORD_TIME_MIN_VALUE, g_triggerRecord.trec.record_time_max);
				}
			}
		}
	}
	else if (keyPressed == ESC_KEY)
	{
		if (g_externalTriggerMenuActiveForSetup == NO)
		{
			SETUP_USER_MENU_MSG(&configMenu, EXTERNAL_TRIGGER);
		}
		else // (g_externalTriggerMenuActiveForSetup == YES)
		{
			g_tempTriggerLevelForMenuAdjustment = AirTriggerConvertToUnits(g_triggerRecord.trec.airTriggerLevel);
			SETUP_USER_MENU_FOR_INTEGERS_MSG(&airTriggerMenu, &g_tempTriggerLevelForMenuAdjustment, GetAirDefaultValue(), GetAirMinValue(), GetAirMaxValue());
		}
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Flash Wrapping Menu
//=============================================================================
//*****************************************************************************
#define FLASH_WRAPPING_MENU_ENTRIES 4
USER_MENU_STRUCT flashWrappingMenu[FLASH_WRAPPING_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, FLASH_WRAPPING_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, FLASH_WRAPPING_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_2)}},
{ITEM_1, 0, YES_TEXT,	NO_TAG, {YES}},
{ITEM_2, 0, NO_TEXT,	NO_TAG, {NO}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&FlashWrappingMenuHandler}}
};

//----------------------------
// Flash Wrapping Menu Handler
//----------------------------
void FlashWrappingMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
#if 0 /* Normal */
		g_unitConfig.flashWrapping = (uint8)(flashWrappingMenu[newItemIndex].data);
#else /* Forcing flash wrapping to be disabled */
		if ((uint8)flashWrappingMenu[newItemIndex].data == YES)
		{
			MessageBox(getLangText(STATUS_TEXT), getLangText(CURRENTLY_NOT_IMPLEMENTED_TEXT), MB_OK);
		}

		g_unitConfig.flashWrapping = NO;
#endif
		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, FLASH_WRAPPING);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Freq Plot Menu
//=============================================================================
//*****************************************************************************
#define FREQ_PLOT_MENU_ENTRIES 4
USER_MENU_STRUCT freqPlotMenu[FREQ_PLOT_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, FREQUENCY_PLOT_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, FREQ_PLOT_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, YES_TEXT,	NO_TAG, {YES}},
{ITEM_2, 0, NO_TEXT,	NO_TAG, {NO}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&FreqPlotMenuHandler}}
};

//-----------------------
// Freq Plot Menu Handler
//-----------------------
void FreqPlotMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_unitConfig.freqPlotMode = (uint8)freqPlotMenu[newItemIndex].data;

		if (g_unitConfig.freqPlotMode == YES)
		{
			SETUP_USER_MENU_MSG(&freqPlotStandardMenu, g_unitConfig.freqPlotType);
		}
		else
		{
			SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

			SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
		}
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, FREQUENCY_PLOT);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Freq Plot Standard Menu
//=============================================================================
//*****************************************************************************
#define FREQ_PLOT_STANDARD_MENU_ENTRIES 7
USER_MENU_STRUCT freqPlotStandardMenu[FREQ_PLOT_STANDARD_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, PLOT_STANDARD_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, FREQ_PLOT_STANDARD_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, US_BOM_TEXT,	NO_TAG, {FREQ_PLOT_US_BOM_STANDARD}},
{ITEM_2, 0, FRENCH_TEXT,	NO_TAG, {FREQ_PLOT_FRENCH_STANDARD}},
{ITEM_3, 0, DIN_4150_TEXT,	NO_TAG, {FREQ_PLOT_DIN_4150_STANDARD}},
{ITEM_4, 0, BRITISH_TEXT,	NO_TAG, {FREQ_PLOT_BRITISH_7385_STANDARD}},
{ITEM_5, 0, SPANISH_TEXT,	NO_TAG, {FREQ_PLOT_SPANISH_STANDARD}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&FreqPlotStandardMenuHandler}}
};


//--------------------------------
// Freq Plot Standard Menu Handler
//--------------------------------
void FreqPlotStandardMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_unitConfig.freqPlotType = (uint8)freqPlotStandardMenu[newItemIndex].data;

		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&freqPlotMenu, g_unitConfig.freqPlotMode);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Hardware ID Menu
//=============================================================================
//*****************************************************************************
#define HARDWARE_ID_MENU_ENTRIES 5
USER_MENU_STRUCT hardwareIDMenu[HARDWARE_ID_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, HARDWARE_ID_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, HARDWARE_ID_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, REV_8_NORMAL_TEXT,			NO_TAG, {HARDWARE_ID_REV_8_NORMAL}},
{ITEM_2, 0, REV_8_WITH_GPS_MOD_TEXT,	NO_TAG, {HARDWARE_ID_REV_8_WITH_GPS_MOD}},
{ITEM_3, 0, REV_8_WITH_USART_TEXT,		NO_TAG, {HARDWARE_ID_REV_8_WITH_USART}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&HardwareIDMenuHandler}}
};

//------------------
// Help Menu Handler
//------------------
void HardwareIDMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_factorySetupRecord.hardwareID = hardwareIDMenu[newItemIndex].data;

		// Re-read and display Smart Sensor info
		DisplaySmartSensorInfo(INFO_ON_CHECK);

		SETUP_USER_MENU_MSG(&seismicSensorTypeMenu, g_factorySetupRecord.seismicSensorType);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&serialNumberMenu, &g_factorySetupRecord.unitSerialNumber);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Help Menu
//=============================================================================
//*****************************************************************************
#define HELP_MENU_ENTRIES 7
USER_MENU_STRUCT helpMenu[HELP_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, HELP_MENU_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, HELP_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, CONFIG_AND_OPTIONS_TEXT,	NO_TAG, {CONFIG_CHOICE}},
{ITEM_2, 0, HELP_INFORMATION_TEXT,		NO_TAG, {INFORMATION_CHOICE}},
{ITEM_3, 0, SENSOR_CHECK_TEXT,			NO_TAG, {SENSOR_CHECK_CHOICE}},
{ITEM_4, 0, GPS_LOCATION_TEXT,			NO_TAG, {GPS_LOCATION_DISPLAY_CHOICE}},
{ITEM_5, 0, CHECK_SUMMARY_FILE_TEXT,	NO_TAG, {CHECK_SUMMARY_FILE_CHOICE}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&HelpMenuHandler}}
};

//------------------
// Help Menu Handler
//------------------
void HelpMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);
	char buildString[50];

	if (keyPressed == ENTER_KEY)
	{
		if (helpMenu[newItemIndex].data == CONFIG_CHOICE)
		{
			SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
		}
		else if (helpMenu[newItemIndex].data == INFORMATION_CHOICE)
		{
			sprintf(buildString, "%s %s %s", getLangText(SOFTWARE_VER_TEXT), (char*)g_buildVersion, (char*)g_buildDate);
			if (MessageBox(getLangText(STATUS_TEXT), buildString, MB_OK) == MB_SPECIAL_ACTION)
			{
				g_quickBootEntryJump = QUICK_BOOT_ENTRY_FROM_MENU;
				BootLoadManager();
			}

			CheckExceptionReportLogExists();
		}
		else if (helpMenu[newItemIndex].data == SENSOR_CHECK_CHOICE)
		{
			DisplaySmartSensorInfo(INFO_ON_CHECK);

			// Update unit sensor types with any new information read from the Smart Sensors
			UpdateUnitSensorsWithSmartSensorTypes();
		}
		else if (helpMenu[newItemIndex].data == GPS_LOCATION_DISPLAY_CHOICE)
		{
			if (GET_HARDWARE_ID == HARDWARE_ID_REV_8_WITH_GPS_MOD)
			{
				sprintf((char*)g_spareBuffer, "LAT %02d%02d.%04d(%c) LON %02d%02d.%04d(%c)", g_gpsPosition.latDegrees, g_gpsPosition.latMinutes, g_gpsPosition.latSeconds, g_gpsPosition.northSouth,
						g_gpsPosition.longDegrees, g_gpsPosition.longMinutes, g_gpsPosition.longSeconds, g_gpsPosition.eastWest);
			}
			else
			{
				sprintf((char*)g_spareBuffer, "%s", getLangText(GPS_MODULE_NOT_INCLUDED_TEXT));
			}

			if (MessageBox(getLangText(GPS_LOCATION_TEXT), (char*)g_spareBuffer, MB_OK) == MB_SPECIAL_ACTION)
			{
				g_gpsOutputToCraft ^= YES;
				sprintf((char*)g_spareBuffer, "GPS OUTPUT TO CRAFT <%s>", ((g_gpsOutputToCraft == YES) ? "YES" : "NO"));
				OverlayMessage(getLangText(STATUS_TEXT), (char*)g_spareBuffer, (2 * SOFT_SECS));
			}
		}
		else if (helpMenu[newItemIndex].data == CHECK_SUMMARY_FILE_CHOICE)
		{
			ValidateSummaryListFileWithEventCache();
		}
		else // Testing
		{
#if 1 /* Smart Sensor testing */
#include "Sensor.h"
			if (OneWireReset(SEISMIC_SENSOR) == YES) { debug("Seismic Smart Sensor discovered\r\n"); }
			else { debug("Seismic Smart Sensor not found\r\n"); }

			if (OneWireReset(ACOUSTIC_SENSOR) == YES) { debug("Acoustic Smart Sensor discovered\r\n"); }
			else { debug("Acoustic Smart Sensor not found\r\n"); }

			debugRaw("\r\n----------Seismic Sensor----------\r\n");
			OneWireTest(SEISMIC_SENSOR);
			debugRaw("\r\n----------Acoustic Sensor----------\r\n");
			OneWireTest(ACOUSTIC_SENSOR);

			debugRaw("\r\n----------Seismic Sensor----------\r\n");
			OneWireFunctions(SEISMIC_SENSOR);
			debugRaw("\r\n----------Acoustic Sensor----------\r\n");
			OneWireFunctions(ACOUSTIC_SENSOR);

			SmartSensorDebug(SEISMIC_SENSOR);
			SmartSensorDebug(ACOUSTIC_SENSOR);
			debugRaw("\r\n----------End----------\r\n");
#endif
		}
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_MENU_MSG(MAIN_MENU);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Info Menu
//=============================================================================
//*****************************************************************************
#define INFO_MENU_ENTRIES 4
USER_MENU_STRUCT infoMenu[INFO_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, HELP_INFO_MENU_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, INFO_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{NO_TAG, 0, NULL_TEXT,	NO_TAG, {DEFAULT_ITEM_1}},
{NO_TAG, 0, NOT_INCLUDED_TEXT,	NO_TAG, {DEFAULT_ITEM_2}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&InfoMenuHandler}}
};

//------------------
// Info Menu Handler
//------------------
void InfoMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	//uint16 newItemIndex = *((uint16*)data);
	UNUSED(data);	

	if (keyPressed == ENTER_KEY)
	{
		SETUP_USER_MENU_MSG(&helpMenu, CONFIG_CHOICE);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&helpMenu, INFORMATION_CHOICE);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Language Menu
//=============================================================================
//*****************************************************************************
#define LANGUAGE_MENU_ENTRIES 7
USER_MENU_STRUCT languageMenu[LANGUAGE_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, LANGUAGE_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, LANGUAGE_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, ENGLISH_TEXT,	NO_TAG, {ENGLISH_LANG}},
{ITEM_2, 0, FRENCH_TEXT,	NO_TAG, {FRENCH_LANG}},
{ITEM_3, 0, SPANISH_TEXT,	NO_TAG, {SPANISH_LANG}},
{ITEM_4, 0, ITALIAN_TEXT,	NO_TAG, {ITALIAN_LANG}},
{ITEM_5, 0, GERMAN_TEXT,	NO_TAG, {GERMAN_LANG}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&LanguageMenuHandler}}
};

//----------------------
// Language Menu Handler
//----------------------
void LanguageMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		switch (languageMenu[newItemIndex].data)
		{
			case ENGLISH_LANG:
			case FRENCH_LANG:
			case ITALIAN_LANG:
			case GERMAN_LANG:
			case SPANISH_LANG:
					g_unitConfig.languageMode = (uint8)languageMenu[newItemIndex].data;
					BuildLanguageLinkTable(g_unitConfig.languageMode);
					SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);
					break;

			default:
					MessageBox(getLangText(STATUS_TEXT), getLangText(CURRENTLY_NOT_IMPLEMENTED_TEXT), MB_OK);
					break;
		}

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, LANGUAGE);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// GPS Power
//=============================================================================
//*****************************************************************************
#define GPS_POWER_MENU_ENTRIES 4
USER_MENU_STRUCT gpsPowerMenu[GPS_POWER_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, GPS_POWER_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, GPS_POWER_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{NO_TAG, 0, NORMAL_SAVE_POWER_TEXT,				NO_TAG, {GPS_POWER_NORMAL_SAVE_POWER}},
{NO_TAG, 0, ALWAYS_ON_ACQUIRING_TEXT,			NO_TAG, {GPS_POWER_ALWAYS_ON_ACQUIRING}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&GpsPowerMenuHandler}}
};

//------------------------------
// GPS Power Menu Handler
//------------------------------
void GpsPowerMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_unitConfig.gpsPowerMode = (uint8)gpsPowerMenu[newItemIndex].data;
		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		// Always on
		if (g_unitConfig.gpsPowerMode == GPS_POWER_ALWAYS_ON_ACQUIRING)
		{
#if 0 /* old hw */
			// Check if the GPS is powered off
			if (gpio_get_pin_value(AVR32_PIN_PB14) == 1)
#else
			if (0)
#endif
			{
				EnableGps();
			}
			else // GPS has power but there may be an active GPS power off timer
			{
				ClearSoftTimer(GPS_POWER_OFF_TIMER_NUM);
			}
		}
		else // normal save power
		{
#if 0 /* old hw */
			// Check if the GPS is powered on and the GPS power off timer is not active
			if ((gpio_get_pin_value(AVR32_PIN_PB14) == 0) && (IsSoftTimerActive(GPS_POWER_OFF_TIMER_NUM) == NO))
#else
			if (1)
#endif
			{
				// Add soft timer to power off
				AssignSoftTimer(GPS_POWER_OFF_TIMER_NUM, (GPS_ACTIVE_LOCATION_SEARCH_TIME * TICKS_PER_MIN), GpsPowerOffTimerCallBack);
			}
		}

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, GPS_POWER);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Legacy DQM Limit
//=============================================================================
//*****************************************************************************
#define LEGACY_DQM_LIMIT_MENU_ENTRIES 4
USER_MENU_STRUCT legacyDqmLimitMenu[LEGACY_DQM_LIMIT_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, LEGACY_DQM_LIMIT_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, LEGACY_DQM_LIMIT_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{NO_TAG, 0, DISABLED_TEXT,				NO_TAG, {DISABLED}},
{NO_TAG, 0, ENABLED_TEXT,				NO_TAG, {ENABLED}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&LegacyDqmLimitMenuHandler}}
};

//------------------------------
// Legacy DQM Limit Menu Handler
//------------------------------
void LegacyDqmLimitMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_unitConfig.legacyDqmLimit = (uint8)legacyDqmLimitMenu[newItemIndex].data;
		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, LEGACY_DQM_LIMIT);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Mode Menu
//=============================================================================
//*****************************************************************************
#define MODE_MENU_ENTRIES 4
USER_MENU_STRUCT modeMenu[MODE_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, NULL_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, MODE_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, MONITOR_TEXT,	NO_TAG, {MONITOR}},
{ITEM_2, 0, EDIT_TEXT,		NO_TAG, {EDIT}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&ModeMenuHandler}}
};

//------------------
// Mode Menu Handler
//------------------
void ModeMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		if (modeMenu[newItemIndex].data == MONITOR)
		{
			if (CheckAndDisplayErrorThatPreventsMonitoring(PROMPT) == NO)
			{
				// Safe to enter monitor mode
				SETUP_MENU_WITH_DATA_MSG(MONITOR_MENU, (uint32)g_triggerRecord.opMode);
			}
		}
		else // Mode is EDIT
		{
			if (g_triggerRecord.opMode == BARGRAPH_MODE)
			{
				SETUP_USER_MENU_MSG(&sampleRateBargraphMenu, g_triggerRecord.trec.sample_rate);
			}		
			else if (g_triggerRecord.opMode == COMBO_MODE)
			{
				SETUP_USER_MENU_MSG(&sampleRateComboMenu, g_triggerRecord.trec.sample_rate);
			}			
			else
			{
				SETUP_USER_MENU_MSG(&sampleRateMenu, g_triggerRecord.trec.sample_rate);
			}

			// Reset flag to display External Trigger menu
			g_externalTriggerMenuActiveForSetup = NO;
		}
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_MENU_MSG(MAIN_MENU); mn_msg.data[0] = ESC_KEY;
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Modem Dial Out Type Menu
//=============================================================================
//*****************************************************************************
#define MODEM_DIAL_OUT_TYPE_MENU_ENTRIES 4
USER_MENU_STRUCT modemDialOutTypeMenu[MODEM_DIAL_OUT_TYPE_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, DIAL_OUT_TYPE_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, MODEM_DIAL_OUT_TYPE_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, EVENTS_CONFIG_STATUS_TEXT,	NO_TAG, {AUTODIALOUT_EVENTS_CONFIG_STATUS}},
{ITEM_2, 0, EVENTS_ONLY_TEXT,			NO_TAG, {AUTODIALOUT_EVENTS_ONLY}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&ModemDialOutTypeMenuHandler}}
};

//---------------------------------
// Modem Dial Out Type Menu Handler
//---------------------------------
void ModemDialOutTypeMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_modemSetupRecord.dialOutType = (uint16)(modemDialOutTypeMenu[newItemIndex].data);

		if (g_modemSetupRecord.dialOutType == AUTODIALOUT_EVENTS_CONFIG_STATUS)
		{
			SETUP_USER_MENU_FOR_INTEGERS_MSG(&modemDialOutCycleTimeMenu, &g_modemSetupRecord.dialOutCycleTime, MODEM_DIAL_OUT_TIMER_DEFAULT_VALUE, MODEM_DIAL_OUT_TIMER_MIN_VALUE, MODEM_DIAL_OUT_TIMER_MAX_VALUE);
		}
		else
		{
			SETUP_USER_MENU_MSG(&modemResetMenu, &g_modemSetupRecord.reset);
		}
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&modemDialMenu, &g_modemSetupRecord.dial);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Modem Setup Menu
//=============================================================================
//*****************************************************************************
#define MODEM_SETUP_MENU_ENTRIES 4
USER_MENU_STRUCT modemSetupMenu[MODEM_SETUP_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, MODEM_SETUP_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, MODEM_SETUP_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, YES_TEXT,	NO_TAG, {YES}},
{ITEM_2, 0, NO_TEXT,	NO_TAG, {NO}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&ModemSetupMenuHandler}}
};

//-------------------------
// Modem Setup Menu Handler
//-------------------------
void ModemSetupMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_modemSetupRecord.modemStatus = (uint16)(modemSetupMenu[newItemIndex].data);

		if (g_modemSetupRecord.modemStatus == YES)
		{
			SETUP_USER_MENU_MSG(&modemInitMenu, &g_modemSetupRecord.init);

		}
		else // Modem Setup is NO
		{
			g_modemStatus.connectionState = NOP_CMD;
			g_modemStatus.modemAvailable = NO;

			g_modemStatus.craftPortRcvFlag = NO;		// Flag to indicate that incoming data has been received
			g_modemStatus.xferState = NOP_CMD;			// Flag for transmitting data to the craft
			g_modemStatus.xferMutex = NO;				// Flag to stop other message command from executing
			g_modemStatus.systemIsLockedFlag = YES;

			g_modemStatus.ringIndicator = 0;

			SaveRecordData(&g_modemSetupRecord, DEFAULT_RECORD, REC_MODEM_SETUP_TYPE);
			SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
		}
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, MODEM_SETUP);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Monitor Log Menu
//=============================================================================
//*****************************************************************************
#define MONITOR_LOG_MENU_ENTRIES 5
USER_MENU_STRUCT monitorLogMenu[MONITOR_LOG_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, MONITOR_LOG_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, MONITOR_LOG_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, VIEW_MONITOR_LOG_TEXT,	NO_TAG, {VIEW_LOG}},
{ITEM_2, 0, PRINT_MONITOR_LOG_TEXT,	NO_TAG, {PRINT_LOG}},
{ITEM_3, 0, LOG_RESULTS_TEXT,		NO_TAG, {LOG_RESULTS}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&MonitorLogMenuHandler}}
};

//-------------------------
// Monitor Log Menu Handler
//-------------------------
void MonitorLogMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		if (newItemIndex == VIEW_LOG)
		{
			SETUP_MENU_MSG(VIEW_MONITOR_LOG_MENU);
		}
		else if (newItemIndex == PRINT_LOG)
		{
			MessageBox(getLangText(STATUS_TEXT), getLangText(NOT_INCLUDED_TEXT), MB_OK);
		}
		else if (newItemIndex == LOG_RESULTS)
		{
			MessageBox(getLangText(STATUS_TEXT), getLangText(NOT_INCLUDED_TEXT), MB_OK);
		}
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, MONITOR_LOG);
	}

	JUMP_TO_ACTIVE_MENU();
}

#if 0 /* Unused */
//*****************************************************************************
//=============================================================================
// Peak Acceleration Menu
//=============================================================================
//*****************************************************************************
#define PEAK_ACC_MENU_ENTRIES 4
USER_MENU_STRUCT peakAccMenu[PEAK_ACC_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, REPORT_PEAK_ACC_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, PEAK_ACC_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_2)}},
{ITEM_1, 0, YES_TEXT, NO_TAG, {YES}},
{ITEM_2, 0, NO_TEXT, NO_TAG, {NO}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&PeakAccMenuHandler}}
};

//-------------------------------
// Peak Acceleration Menu Handler
//-------------------------------
void PeakAccMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_unitConfig.reportPeakAcceleration = (uint8)peakAccMenu[newItemIndex].data;

		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, REPORT_PEAK_ACC);
	}

	JUMP_TO_ACTIVE_MENU();
}
#endif

//*****************************************************************************
//=============================================================================
// Pretrigger Size Menu
//=============================================================================
//*****************************************************************************
#define PRETRIGGER_SIZE_MENU_ENTRIES 5
USER_MENU_STRUCT pretriggerSizeMenu[PRETRIGGER_SIZE_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, PRETRIGGER_SIZE_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, PRETRIGGER_SIZE_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, QUARTER_SECOND_TEXT,	NO_TAG, {4}},
{ITEM_2, 0, HALF_SECOND_TEXT,	NO_TAG, {2}},
{ITEM_3, 0, FULL_SECOND_TEXT,	NO_TAG, {1}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&PretriggerSizeMenuHandler}}
};

//-----------------------------
// Pretrigger Size Menu Handler
//-----------------------------
void PretriggerSizeMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_unitConfig.pretrigBufferDivider = (uint8)pretriggerSizeMenu[newItemIndex].data;

		debug("New Pretrigger divider: %d\r\n", g_unitConfig.pretrigBufferDivider);

		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, PRETRIGGER_SIZE);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Printer Enable Menu
//=============================================================================
//*****************************************************************************
#define PRINTER_ENABLE_MENU_ENTRIES 4
USER_MENU_STRUCT printerEnableMenu[PRINTER_ENABLE_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, PRINTER_ON_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, PRINTER_ENABLE_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, YES_TEXT,	NO_TAG, {YES}},
{ITEM_2, 0, NO_TEXT,	NO_TAG, {NO}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&PrinterEnableMenuHandler}}
};

//----------------------------
// Printer Enable Menu Handler
//----------------------------
void PrinterEnableMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_unitConfig.autoPrint = (uint8)printerEnableMenu[newItemIndex].data;

		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, PRINTER);
	}

	JUMP_TO_ACTIVE_MENU();
}

#if 0 /* Unused */
//*****************************************************************************
//=============================================================================
// Print Out Menu
//=============================================================================
//*****************************************************************************
#define PRINT_OUT_MENU_ENTRIES 4
USER_MENU_STRUCT printOutMenu[PRINT_OUT_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, PRINT_GRAPH_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, PRINT_OUT_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, YES_TEXT,	NO_TAG, {YES}},
{ITEM_2, 0, NO_TEXT,	NO_TAG, {NO}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&PrintOutMenuHandler}}
};

//-----------------------
// Print Out Menu Handler
//-----------------------
void PrintOutMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);
	uint8 tempCopies = COPIES_DEFAULT_VALUE;

	if (keyPressed == ENTER_KEY)
	{
		if (printOutMenu[newItemIndex].data == YES)
		{
			// Using tempCopies just to seed with a value of 1 with the understanding that this value will
			// be referenced before this routine exits (User Menu Handler) and not at all afterwards
			SETUP_USER_MENU_FOR_INTEGERS_MSG(&copiesMenu, &tempCopies,
				COPIES_DEFAULT_VALUE, COPIES_MIN_VALUE, COPIES_MAX_VALUE);
		}
		else
		{
			SETUP_MENU_MSG(SUMMARY_MENU); mn_msg.data[0] = ESC_KEY;
		}
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_MENU_MSG(SUMMARY_MENU); mn_msg.data[0] = ESC_KEY;
	}

	JUMP_TO_ACTIVE_MENU();
}
#endif

//*****************************************************************************
//=============================================================================
// Print Monitor Log Menu
//=============================================================================
//*****************************************************************************
#define PRINT_MONITOR_LOG_RESULTS_MENU_ENTRIES 4
USER_MENU_STRUCT printMonitorLogMenu[PRINT_MONITOR_LOG_RESULTS_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, PRINT_MONITOR_LOG_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, PRINT_MONITOR_LOG_RESULTS_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, YES_TEXT,	NO_TAG, {YES}},
{ITEM_2, 0, NO_TEXT,	NO_TAG, {NO}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&PrintMonitorLogMenuHandler}}
};

//---------------------------------------
// Print Monitor Log Menu Handler
//---------------------------------------
void PrintMonitorLogMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		if (printMonitorLogMenu[newItemIndex].data == YES)
		{
			MessageBox(getLangText(STATUS_TEXT), getLangText(CURRENTLY_NOT_IMPLEMENTED_TEXT), MB_OK);
		}

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, PRINT_MONITOR_LOG);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// RS232 Power Savings Menu
//=============================================================================
//*****************************************************************************
#define RS232_POWER_SAVINGS_MENU_ENTRIES 4
USER_MENU_STRUCT rs232PowerSavingsMenu[RS232_POWER_SAVINGS_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, POWER_SAVINGS_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, RS232_POWER_SAVINGS_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{NO_TAG, 0, ENABLED_TEXT,				NO_TAG, {ENABLED}},
{NO_TAG, 0, DISABLED_TEXT,				NO_TAG, {DISABLED}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&Rs232PowerSavingsMenuHandler}}
};

//---------------------------------------
// Power Savings Menu Handler
//---------------------------------------
void Rs232PowerSavingsMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_unitConfig.rs232PowerSavings = (uint8)rs232PowerSavingsMenu[newItemIndex].data;
		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, RS232_POWER_SAVINGS);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Recalibrate Menu
//=============================================================================
//*****************************************************************************
#define RECALIBRATE_MENU_ENTRIES 4
USER_MENU_STRUCT recalibrateMenu[RECALIBRATE_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, RECALIBRATE_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, RECALIBRATE_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, ON_TEMP_CHANGE_TEXT,	NO_TAG, {YES}},
{ITEM_2, 0, NO_TEXT,				NO_TAG, {NO}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&RecalibrateMenuHandler}}
};

//--------------------------------
// Recalibrate Menu Handler
//--------------------------------
void RecalibrateMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_triggerRecord.trec.adjustForTempDrift = (uint8)recalibrateMenu[newItemIndex].data;

		// Save the current trig record into the default location
		SaveRecordData(&g_triggerRecord, DEFAULT_RECORD, REC_TRIGGER_USER_MENU_TYPE);

		SETUP_USER_MENU_MSG(&companyMenu, &g_triggerRecord.trec.client);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&bitAccuracyMenu, g_triggerRecord.trec.bitAccuracy);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Sample Rate Menu
//=============================================================================
//*****************************************************************************
#define SAMPLE_RATE_MENU_ENTRIES 7
USER_MENU_STRUCT sampleRateMenu[SAMPLE_RATE_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, SAMPLE_RATE_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, SAMPLE_RATE_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_2)}},
{ITEM_1, SAMPLE_RATE_1K, NULL_TEXT, NO_TAG, {SAMPLE_RATE_1K}},
{ITEM_2, SAMPLE_RATE_2K, NULL_TEXT, NO_TAG, {SAMPLE_RATE_2K}},
{ITEM_3, SAMPLE_RATE_4K, NULL_TEXT, NO_TAG, {SAMPLE_RATE_4K}},
{ITEM_4, SAMPLE_RATE_8K, NULL_TEXT, NO_TAG, {SAMPLE_RATE_8K}},
{ITEM_5, SAMPLE_RATE_16K, NULL_TEXT, NO_TAG, {SAMPLE_RATE_16K}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&SampleRateMenuHandler}}
};

#define SAMPLE_RATE_BARGRAPH_MENU_ENTRIES 6
USER_MENU_STRUCT sampleRateBargraphMenu[SAMPLE_RATE_BARGRAPH_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, SAMPLE_RATE_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, SAMPLE_RATE_BARGRAPH_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_2)}},
{ITEM_1, SAMPLE_RATE_1K, NULL_TEXT, NO_TAG, {SAMPLE_RATE_1K}},
{ITEM_2, SAMPLE_RATE_2K, NULL_TEXT, NO_TAG, {SAMPLE_RATE_2K}},
{ITEM_3, SAMPLE_RATE_4K, NULL_TEXT, NO_TAG, {SAMPLE_RATE_4K}},
{ITEM_4, SAMPLE_RATE_8K, NULL_TEXT, NO_TAG, {SAMPLE_RATE_8K}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&SampleRateMenuHandler}}
};

#define SAMPLE_RATE_COMBO_MENU_ENTRIES 6
USER_MENU_STRUCT sampleRateComboMenu[SAMPLE_RATE_COMBO_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, SAMPLE_RATE_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, SAMPLE_RATE_COMBO_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_2)}},
{ITEM_1, SAMPLE_RATE_1K, NULL_TEXT, NO_TAG, {SAMPLE_RATE_1K}},
{ITEM_2, SAMPLE_RATE_2K, NULL_TEXT, NO_TAG, {SAMPLE_RATE_2K}},
{ITEM_3, SAMPLE_RATE_4K, NULL_TEXT, NO_TAG, {SAMPLE_RATE_4K}},
{ITEM_4, SAMPLE_RATE_8K, NULL_TEXT, NO_TAG, {SAMPLE_RATE_8K}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&SampleRateMenuHandler}}
};

//-------------------------
// Sample Rate Menu Handler
//-------------------------
void SampleRateMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		if ((sampleRateMenu[newItemIndex].data < sampleRateMenu[ITEM_1].data) || 
			(sampleRateMenu[newItemIndex].data > sampleRateMenu[ITEM_6].data) ||
			((sampleRateMenu[newItemIndex].data > sampleRateMenu[ITEM_4].data) && (g_triggerRecord.opMode == BARGRAPH_MODE)) ||
			((sampleRateMenu[newItemIndex].data > sampleRateMenu[ITEM_4].data) && (g_triggerRecord.opMode == COMBO_MODE)))
		{
			sprintf((char*)g_spareBuffer, "%lu %s", sampleRateMenu[newItemIndex].data,
					getLangText(CURRENTLY_NOT_IMPLEMENTED_TEXT));
			MessageBox(getLangText(STATUS_TEXT), (char*)g_spareBuffer, MB_OK);

			debug("Sample Rate: %d is not supported for this mode.\r\n", g_triggerRecord.trec.sample_rate);

			SETUP_USER_MENU_MSG(&sampleRateMenu, sampleRateMenu[DEFAULT_ITEM_2].data);
		}
		else
		{
			g_triggerRecord.trec.sample_rate = sampleRateMenu[newItemIndex].data;

			if (g_triggerRecord.opMode == COMBO_MODE)
			{
				g_triggerRecord.trec.record_time_max = (uint16)(((uint32)(((EVENT_BUFF_SIZE_IN_WORDS - COMBO_MODE_BARGRAPH_BUFFER_SIZE_WORDS) -
					((g_triggerRecord.trec.sample_rate / g_unitConfig.pretrigBufferDivider) * g_sensorInfo.numOfChannels) -
					((g_triggerRecord.trec.sample_rate / MIN_SAMPLE_RATE) * MAX_CAL_SAMPLES * g_sensorInfo.numOfChannels)) /
					(g_triggerRecord.trec.sample_rate * g_sensorInfo.numOfChannels))));
			}
			else // all other modes
			{
				g_triggerRecord.trec.record_time_max = (uint16)(((uint32)(((EVENT_BUFF_SIZE_IN_WORDS) -
					((g_triggerRecord.trec.sample_rate / g_unitConfig.pretrigBufferDivider) * g_sensorInfo.numOfChannels) -
					((g_triggerRecord.trec.sample_rate / MIN_SAMPLE_RATE) * MAX_CAL_SAMPLES * g_sensorInfo.numOfChannels)) /
					(g_triggerRecord.trec.sample_rate * g_sensorInfo.numOfChannels))));
			}

			debug("New Max Record Time: %d\r\n", g_triggerRecord.trec.record_time_max);

#if 0 /* Original */
			SETUP_USER_MENU_MSG(&bitAccuracyMenu, g_triggerRecord.trec.bitAccuracy);
#else /* New Adaptive Sampling */
			if ((g_triggerRecord.trec.sample_rate == SAMPLE_RATE_1K) || (g_unitConfig.adaptiveSampling != ENABLED))
			{
				SETUP_USER_MENU_MSG(&bitAccuracyMenu, g_triggerRecord.trec.bitAccuracy);
			}
			else
			{
				SETUP_USER_MENU_MSG(&samplingMethodMenu, g_triggerRecord.trec.samplingMethod);
			}
#endif
		}
	}
	else if (keyPressed == ESC_KEY)
	{
		UpdateModeMenuTitle(g_triggerRecord.opMode);
		SETUP_USER_MENU_MSG(&modeMenu, MONITOR);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Sampling Method
//=============================================================================
//*****************************************************************************
#define SAMPLING_METHOD_MENU_ENTRIES 4
USER_MENU_STRUCT samplingMethodMenu[SAMPLING_METHOD_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, SAMPLING_METHOD_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, SAMPLING_METHOD_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{NO_TAG, 0, FIXED_NORMAL_TEXT,			NO_TAG, {FIXED_SAMPLING}},
{NO_TAG, 0, ADAPTIVE_SAVE_BATT_TEXT,	NO_TAG, {ADAPTIVE_SAMPLING}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&SamplingMethodMenuHandler}}
};

//----------------------------------
// Menu Handler
//----------------------------------
void SamplingMethodMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_triggerRecord.trec.samplingMethod = (uint8)samplingMethodMenu[newItemIndex].data;

		SETUP_USER_MENU_MSG(&bitAccuracyMenu, g_triggerRecord.trec.bitAccuracy);
	}
	else if (keyPressed == ESC_KEY)
	{
		if (g_triggerRecord.opMode == BARGRAPH_MODE)
		{
			SETUP_USER_MENU_MSG(&sampleRateBargraphMenu, g_triggerRecord.trec.sample_rate);
		}
		else if (g_triggerRecord.opMode == COMBO_MODE)
		{
			SETUP_USER_MENU_MSG(&sampleRateComboMenu, g_triggerRecord.trec.sample_rate);
		}
		else
		{
			SETUP_USER_MENU_MSG(&sampleRateMenu, g_triggerRecord.trec.sample_rate);
		}
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Save Compressed Data
//=============================================================================
//*****************************************************************************
#define SAVE_COMPRESSED_DATA_MENU_ENTRIES 4
USER_MENU_STRUCT saveCompressedDataMenu[SAVE_COMPRESSED_DATA_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, SAVE_COMPRESSED_DATA_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, SAVE_COMPRESSED_DATA_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, YES_FASTER_DOWNLOAD_TEXT,	NO_TAG, {SAVE_EXTRA_FILE_COMPRESSED_DATA}},
{ITEM_2, 0, NO_TEXT,					NO_TAG, {DO_NOT_SAVE_EXTRA_FILE_COMPRESSED_DATA}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&SaveCompressedDataMenuHandler}}
};

//----------------------------------
// Save Compressed Data Menu Handler
//----------------------------------
void SaveCompressedDataMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_unitConfig.saveCompressedData = (uint8)saveCompressedDataMenu[newItemIndex].data;

		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, SAVE_COMPRESSED_DATA);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Save Setup Menu
//=============================================================================
//*****************************************************************************
#define SAVE_SETUP_MENU_ENTRIES 4
USER_MENU_STRUCT saveSetupMenu[SAVE_SETUP_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, SAVE_SETUP_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, SAVE_SETUP_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, YES_TEXT,	NO_TAG, {YES}},
{ITEM_2, 0, NO_TEXT,	NO_TAG, {NO}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&SaveSetupMenuHandler}}
};

//------------------------
// Save Setup Menu Handler
//------------------------
void SaveSetupMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		if (saveSetupMenu[newItemIndex].data == YES)
		{
			SETUP_USER_MENU_MSG(&saveRecordMenu, (uint8)0);
		}
		else // User selected NO
		{
			// Save the current trig record into the default location
			SaveRecordData(&g_triggerRecord, DEFAULT_RECORD, REC_TRIGGER_USER_MENU_TYPE);

			UpdateModeMenuTitle(g_triggerRecord.opMode);
			SETUP_USER_MENU_MSG(&modeMenu, MONITOR);
		}
	}
	else if (keyPressed == ESC_KEY)
	{
		if ((g_unitConfig.alarmOneMode) || (g_unitConfig.alarmTwoMode))
		{
			if (g_unitConfig.alarmTwoMode == ALARM_MODE_OFF)
			{
				SETUP_USER_MENU_MSG(&alarmTwoMenu, g_unitConfig.alarmTwoMode);
			}
			else
			{
				SETUP_USER_MENU_FOR_FLOATS_MSG(&alarmTwoTimeMenu, &g_unitConfig.alarmTwoTime, ALARM_OUTPUT_TIME_DEFAULT, ALARM_OUTPUT_TIME_INCREMENT, ALARM_OUTPUT_TIME_MIN, ALARM_OUTPUT_TIME_MAX);
			}
		}
		else // No alarms
		{
			if (g_triggerRecord.opMode == BARGRAPH_MODE)
			{
				if ((!g_factorySetupRecord.invalid) && (g_factorySetupRecord.aWeightOption == ENABLED))
				{
					SETUP_USER_MENU_MSG(&airScaleMenu, g_unitConfig.airScale);
				}
				else
				{
#if 0 /* Removing this option */
					SETUP_USER_MENU_MSG(&barResultMenu, g_unitConfig.vectorSum);
#else
					SETUP_USER_MENU_FOR_INTEGERS_MSG(&lcdImpulseTimeMenu, &g_triggerRecord.berec.impulseMenuUpdateSecs, LCD_IMPULSE_TIME_DEFAULT_VALUE, LCD_IMPULSE_TIME_MIN_VALUE, LCD_IMPULSE_TIME_MAX_VALUE);
#endif
				}
			}
			else // Waveform or Combo
			{
				SETUP_USER_MENU_FOR_INTEGERS_MSG(&recordTimeMenu, &g_triggerRecord.trec.record_time, RECORD_TIME_DEFAULT_VALUE, RECORD_TIME_MIN_VALUE, g_triggerRecord.trec.record_time_max);
			}
		}
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Sensitivity Menu
//=============================================================================
//*****************************************************************************
#define SENSITIVITY_MENU_ENTRIES 4
USER_MENU_STRUCT sensitivityMenu[SENSITIVITY_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, SENSITIVITY_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, SENSITIVITY_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, NORMAL_TEXT,	LOW_SENSITIVITY_MAX_TAG, {LOW}},
{ITEM_2, 0, HIGH_TEXT,		HIGH_SENSITIVITY_MAX_TAG, {HIGH}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&SensitivityMenuHandler}}
};

//-------------------------
// Sensitivity Menu Handler
//-------------------------
void SensitivityMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_triggerRecord.srec.sensitivity = (uint8)sensitivityMenu[newItemIndex].data;

#if VT_FEATURE_DISABLED // Normal
		if (g_triggerRecord.opMode == WAVEFORM_MODE)
		{
			if (g_factorySetupRecord.seismicSensorType > SENSOR_ACC_RANGE_DIVIDER)
			{
				USER_MENU_DEFAULT_TYPE(seismicTriggerMenu) = MG_TYPE;
				USER_MENU_ALT_TYPE(seismicTriggerMenu) = MG_TYPE;
			}
			else
			{
				USER_MENU_DEFAULT_TYPE(seismicTriggerMenu) = IN_TYPE;
				USER_MENU_ALT_TYPE(seismicTriggerMenu) = MM_TYPE;
			}

			// Down convert to current bit accuracy setting
			g_tempTriggerLevelForMenuAdjustment = SeismicTriggerConvertBitAccuracy(g_triggerRecord.trec.seismicTriggerLevel);

			SETUP_USER_MENU_FOR_INTEGERS_MSG(&seismicTriggerMenu, &g_tempTriggerLevelForMenuAdjustment,
											(SEISMIC_TRIGGER_DEFAULT_VALUE / (SEISMIC_TRIGGER_MAX_VALUE / g_bitAccuracyMidpoint)),
											(SEISMIC_TRIGGER_MIN_VALUE / (SEISMIC_TRIGGER_MAX_VALUE / g_bitAccuracyMidpoint)), g_bitAccuracyMidpoint);
		}
		else if ((g_triggerRecord.opMode == BARGRAPH_MODE) || (g_triggerRecord.opMode == COMBO_MODE))
		{
#if 0 /* Removing this option */
			SETUP_USER_MENU_MSG(&barChannelMenu, g_triggerRecord.berec.barChannel);
#else /* New path */
			SETUP_USER_MENU_MSG(&barIntervalMenu, g_triggerRecord.bgrec.barInterval);
#endif
		}
#else /* New VT feature */
		if (g_triggerRecord.opMode == WAVEFORM_MODE)
		{
			SETUP_USER_MENU_MSG(&seismicTriggerTypeMenu, g_triggerRecord.trec.variableTriggerEnable);
		}
		else if ((g_triggerRecord.opMode == BARGRAPH_MODE) || (g_triggerRecord.opMode == COMBO_MODE))
		{
#if 0 /* Removing this option */
			SETUP_USER_MENU_MSG(&barChannelMenu, g_triggerRecord.berec.barChannel);
#else /* New path */
			SETUP_USER_MENU_MSG(&barIntervalMenu, g_triggerRecord.bgrec.barInterval);
#endif
		}
#endif
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&operatorMenu, &g_triggerRecord.trec.oper);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Acoustic Sensor Type Menu
//=============================================================================
//*****************************************************************************
#define ACOUSTIC_SENSOR_TYPE_MENU_ENTRIES 6
USER_MENU_STRUCT acousticSensorTypeMenu[ACOUSTIC_SENSOR_TYPE_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, ACOUSTIC_GAIN_TYPE_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, ACOUSTIC_SENSOR_TYPE_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, MIC_148_DB_TEXT,	NO_TAG, {SENSOR_MIC_148_DB}},
{ITEM_2, 0, MIC_160_DB_TEXT,	NO_TAG, {SENSOR_MIC_160_DB}},
{ITEM_3, 0, MIC_5_PSI_TEXT,		NO_TAG, {SENSOR_MIC_5_PSI}},
{ITEM_2, 0, MIC_10_PSI_TEXT,	NO_TAG, {SENSOR_MIC_10_PSI}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&AcousticSensorTypeMenuHandler}}
};

//-------------------------
// Sensor Type Menu Handler
//-------------------------
void AcousticSensorTypeMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_factorySetupRecord.acousticSensorType = (uint8)acousticSensorTypeMenu[newItemIndex].data;

		SETUP_USER_MENU_MSG(&analogChannelConfigMenu, g_factorySetupRecord.analogChannelConfig);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&seismicSensorTypeMenu, g_factorySetupRecord.seismicSensorType);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Seismic Sensor Type Menu
//=============================================================================
//*****************************************************************************
#define SEISMIC_SENSOR_TYPE_MENU_ENTRIES 9
USER_MENU_STRUCT seismicSensorTypeMenu[SEISMIC_SENSOR_TYPE_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, SEISMIC_GAIN_TYPE_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, SEISMIC_SENSOR_TYPE_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_4)}},
{ITEM_1, 0, X025_80_IPS_TEXT,	NO_TAG, {SENSOR_80_IN}},
{ITEM_2, 0, X05_40_IPS_TEXT,	NO_TAG, {SENSOR_40_IN}},
{ITEM_3, 0, X1_20_IPS_TEXT,		NO_TAG, {SENSOR_20_IN}},
{ITEM_4, 0, X2_10_IPS_TEXT,		NO_TAG, {SENSOR_10_IN}},
{ITEM_5, 0, X4_5_IPS_TEXT,		NO_TAG, {SENSOR_5_IN}},
{ITEM_6, 0, X8_2_5_IPS_TEXT,	NO_TAG, {SENSOR_2_5_IN}},
{ITEM_7, 0, ACC_793L_TEXT,		NO_TAG, {SENSOR_ACCELEROMETER}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&SeismicSensorTypeMenuHandler}}
};

//---------------------------------
// Seismic Sensor Type Menu Handler
//---------------------------------
void SeismicSensorTypeMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_factorySetupRecord.seismicSensorType = (uint16)seismicSensorTypeMenu[newItemIndex].data;

		if (g_factorySetupRecord.seismicSensorType > SENSOR_ACC_RANGE_DIVIDER) { debug("Factory Setup: New Seismic sensor type: Accelerometer\r\n"); }
		else { debug("Factory Setup: New Seismic sensor type: %3.1f in\r\n", (double)((float)g_factorySetupRecord.seismicSensorType / (float)204.8)); }

		SETUP_USER_MENU_MSG(&acousticSensorTypeMenu, g_factorySetupRecord.acousticSensorType);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&hardwareIDMenu, g_factorySetupRecord.hardwareID);
	}

	JUMP_TO_ACTIVE_MENU();
}

#if 0 /* Test */
//*****************************************************************************
//=============================================================================
// Seismic Filtering Menu
//=============================================================================
//*****************************************************************************
#define SEISMIC_FILTERING_MENU_ENTRIES 7
USER_MENU_STRUCT seismicFilteringMenu[SEISMIC_FILTERING_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, SEISMIC_FILTERING_TAG, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, SEISMIC_FILTERING_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 960, HZ_TEXT, NO_TAG, {ANALOG_CUTOFF_FREQ_1K}},
{ITEM_2, 2100, HZ_TEXT, NO_TAG, {ANALOG_CUTOFF_FREQ_2K}},
{ITEM_3, 3900, HZ_TEXT, NO_TAG, {ANALOG_CUTOFF_FREQ_4K}},
{ITEM_4, 8000, HZ_TEXT, NO_TAG, {ANALOG_CUTOFF_FREQ_8K}},
{ITEM_5, 15800, HZ_TEXT, NO_TAG, {ANALOG_CUTOFF_FREQ_16K}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&SeismicFilteringMenuHandler}}
};

//-------------------------
// Seismic Filtering Menu Handler
//-------------------------
void SeismicFilteringMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_calSetupParameters.seismicFiltering = seismicFilteringMenu[newItemIndex].data;
		if (g_calSetupParameters.airFiltering == ACOUSTIC_PATH_A_WEIGHTED) { SETUP_USER_MENU_MSG(&airScaleMenu, AIR_SCALE_A_WEIGHTING); }
		else { SETUP_USER_MENU_MSG(&airScaleMenu, AIR_SCALE_LINEAR); }
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&sampleRateComboMenu, g_calSetupParameters.sampleRate);
	}

	JUMP_TO_ACTIVE_MENU();
}
#endif

#if (!VT_FEATURE_DISABLED)
//*****************************************************************************
//=============================================================================
// Seismic Trigger Type Menu
//=============================================================================
//*****************************************************************************
#define SEISMIC_TRIGGER_TYPE_MENU_ENTRIES 4
USER_MENU_STRUCT seismicTriggerTypeMenu[SEISMIC_TRIGGER_TYPE_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, SEISMIC_TRIG_TYPE_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, SEISMIC_TRIGGER_TYPE_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{NO_TAG, 0, NORMAL_DEFAULT_TEXT,	NO_TAG,	{NO}},
{NO_TAG, 0, VARIABLE_USBM_OSM_TEXT,	NO_TAG,	{YES}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&SeismicTriggerTypeMenuHandler}}
};

//----------------------------------
// Seismic Trigger Type Menu Handler
//----------------------------------
void SeismicTriggerTypeMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		if (seismicTriggerTypeMenu[newItemIndex].data == NO)
		{
			g_triggerRecord.trec.variableTriggerEnable = NO;

			if (g_factorySetupRecord.seismicSensorType > SENSOR_ACC_RANGE_DIVIDER)
			{
				USER_MENU_DEFAULT_TYPE(seismicTriggerMenu) = MG_TYPE;
				USER_MENU_ALT_TYPE(seismicTriggerMenu) = MG_TYPE;
			}
			else
			{
				USER_MENU_DEFAULT_TYPE(seismicTriggerMenu) = IN_TYPE;
				USER_MENU_ALT_TYPE(seismicTriggerMenu) = MM_TYPE;
			}

			// Down convert to current bit accuracy setting
			g_tempTriggerLevelForMenuAdjustment = SeismicTriggerConvertBitAccuracy(g_triggerRecord.trec.seismicTriggerLevel);

			SETUP_USER_MENU_FOR_INTEGERS_MSG(&seismicTriggerMenu, &g_tempTriggerLevelForMenuAdjustment,
											(SEISMIC_TRIGGER_DEFAULT_VALUE / (SEISMIC_TRIGGER_MAX_VALUE / g_bitAccuracyMidpoint)),
											(SEISMIC_TRIGGER_MIN_VALUE / (SEISMIC_TRIGGER_MAX_VALUE / g_bitAccuracyMidpoint)), g_bitAccuracyMidpoint);
		}
		else // Variable Trigger USBM/OSM selected
		{
#if 1 /* New */
			// Variable triggers increased processing does not allow the unit to achieve a 16K sample rate (limit is between 11K and 12K)
			if (g_triggerRecord.trec.sample_rate == SAMPLE_RATE_16K)
			{
				MessageBox(getLangText(WARNING_TEXT), getLangText(VARIABLE_UNABLE_TO_SAMPLE_AT_16K_SETTING_TO_8K_TEXT), MB_OK);

				g_triggerRecord.trec.sample_rate = SAMPLE_RATE_8K;
			}
#endif
			SETUP_USER_MENU_MSG(&vibrationStandardMenu, ((g_triggerRecord.trec.variableTriggerVibrationStandard < START_OF_CUSTOM_CURVES_LIST) ? g_triggerRecord.trec.variableTriggerVibrationStandard : START_OF_CUSTOM_CURVES_LIST));
		}
	}
	else if (keyPressed == ESC_KEY)
	{
		if (g_triggerRecord.opMode == COMBO_MODE)
		{
#if 0 /* Removing this option */
			SETUP_USER_MENU_MSG(&barResultMenu, g_unitConfig.vectorSum);
#else
			SETUP_USER_MENU_FOR_INTEGERS_MSG(&lcdImpulseTimeMenu, &g_triggerRecord.berec.impulseMenuUpdateSecs, LCD_IMPULSE_TIME_DEFAULT_VALUE, LCD_IMPULSE_TIME_MIN_VALUE, LCD_IMPULSE_TIME_MAX_VALUE);
#endif
		}
		else // WAVEFORM_MODE
		{
			SETUP_USER_MENU_MSG(&sensitivityMenu, g_triggerRecord.srec.sensitivity);
		}
	}

	JUMP_TO_ACTIVE_MENU();
}
#endif

//*****************************************************************************
//=============================================================================
// Stored Event Limit Menu
//=============================================================================
//*****************************************************************************
#define STORED_EVENTS_CAP_MODE_MENU_ENTRIES 4
USER_MENU_STRUCT storedEventsCapModeMenu[STORED_EVENTS_CAP_MODE_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, STORED_EVENTS_CAP_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, STORED_EVENTS_CAP_MODE_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, DISABLED_TEXT,	NO_TAG, {DISABLED}},
{ITEM_2, 0, ENABLED_TEXT,	NO_TAG, {ENABLED}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&StoredEventsCapModeMenuHandler}}
};

//------------------------------------
// Stored Events Cap Mode Menu Handler
//------------------------------------
void StoredEventsCapModeMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		if ((uint8)storedEventsCapModeMenu[newItemIndex].data == ENABLED)
		{
			SETUP_USER_MENU_FOR_INTEGERS_MSG(&storedEventLimitMenu, &g_unitConfig.storedEventLimit, STORED_EVENT_LIMIT_DEFAULT_VALUE, STORED_EVENT_LIMIT_MIN_VALUE, STORED_EVENT_LIMIT_MAX_VALUE);
		}
		else // Mode is disabled
		{
			g_unitConfig.storedEventsCapMode = DISABLED;

			SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

			SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
		}
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, STORED_EVENTS_CAP_MODE);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Summary Interval Menu
//=============================================================================
//*****************************************************************************
#define SUMMARY_INTERVAL_MENU_ENTRIES 10
USER_MENU_STRUCT summaryIntervalMenu[SUMMARY_INTERVAL_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, SUMMARY_INTERVAL_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, SUMMARY_INTERVAL_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_4)}},
{ITEM_1, 5, MINUTES_TEXT,	NO_TAG, {FIVE_MINUTE_INTVL}},
{ITEM_2, 15, MINUTES_TEXT,	NO_TAG, {FIFTEEN_MINUTE_INTVL}},
{ITEM_3, 30, MINUTES_TEXT,	NO_TAG, {THIRTY_MINUTE_INTVL}},
{ITEM_4, 1, HOUR_TEXT,		NO_TAG, {ONE_HOUR_INTVL}},
{ITEM_5, 2, HOURS_TEXT,		NO_TAG, {TWO_HOUR_INTVL}},
{ITEM_6, 4, HOURS_TEXT,		NO_TAG, {FOUR_HOUR_INTVL}},
{ITEM_7, 8, HOURS_TEXT,		NO_TAG, {EIGHT_HOUR_INTVL}},
{ITEM_8, 12, HOURS_TEXT,	NO_TAG, {TWELVE_HOUR_INTVL}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&SummaryIntervalMenuHandler}}
};

//------------------------------
// Summary Interval Menu Handler
//------------------------------
void SummaryIntervalMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_triggerRecord.bgrec.summaryInterval = summaryIntervalMenu[newItemIndex].data;

		SETUP_USER_MENU_MSG(&barIntervalDataTypeMenu, g_triggerRecord.berec.barIntervalDataType);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&barIntervalMenu, g_triggerRecord.bgrec.barInterval);
	}

	JUMP_TO_ACTIVE_MENU();
}

#if 1
//*****************************************************************************
//=============================================================================
// Usb Sync Mode Menu
//=============================================================================
//*****************************************************************************
#define USB_SYNC_MODE_MENU_ENTRIES 6
USER_MENU_STRUCT usbSyncModeMenu[USB_SYNC_MODE_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, USB_SYNC_MODE_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, USB_SYNC_MODE_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, PROMPT_ON_CONFLICT_TEXT,	NO_TAG, {PROMPT_OPTION}},
{ITEM_2, 0, SKIP_ALL_TEXT,				NO_TAG, {SKIP_ALL_OPTION}},
{ITEM_3, 0, REPLACE_ALL_TEXT,			NO_TAG, {REPLACE_ALL_OPTION}},
{ITEM_4, 0, DUPLICATE_ALL_TEXT,			NO_TAG, {DUPLICATE_ALL_OPTION}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&UsbSyncModeMenuHandler}}
};

//---------------------------
// Usb Sync Mode Menu Handler
//---------------------------
void UsbSyncModeMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_unitConfig.usbSyncMode = usbSyncModeMenu[newItemIndex].data;

		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, USB_SYNC_MODE);
	}

	JUMP_TO_ACTIVE_MENU();
}
#endif

#if 1
//*****************************************************************************
//=============================================================================
// Sync File Exists Menu
//=============================================================================
//*****************************************************************************
#define SYNC_FILE_EXISTS_MENU_ENTRIES 9
USER_MENU_STRUCT syncFileExistsMenu[SYNC_FILE_EXISTS_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, SYNC_FILE_EXISTS_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_SPECIAL_TYPE, SYNC_FILE_EXISTS_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_2)}},
{FILENAME_TAG, 0, NULL_TEXT,		NO_TAG, {0}},
{ITEM_1, 0, SKIP_TEXT,				NO_TAG, {SKIP_OPTION}},
{ITEM_2, 0, REPLACE_TEXT,			NO_TAG, {REPLACE_OPTION}},
{ITEM_3, 0, DUPLICATE_TEXT,			NO_TAG, {DUPLICATE_OPTION}},
{ITEM_4, 0, SKIP_ALL_TEXT,			NO_TAG, {SKIP_ALL_OPTION}},
{ITEM_5, 0, REPLACE_ALL_TEXT,		NO_TAG, {REPLACE_ALL_OPTION}},
{ITEM_6, 0, DUPLICATE_ALL_TEXT,		NO_TAG, {DUPLICATE_ALL_OPTION}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&SyncFileExistsMenuHandler}}
};

//--------------------------------
// Sync File Exists Menu Handler
//--------------------------------
void SyncFileExistsMenuHandler(uint8 keyPressed, void* data)
{
	//INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_syncFileExistsAction = syncFileExistsMenu[newItemIndex].data;
	}
	else if (keyPressed == ESC_KEY)
	{
		g_syncFileExistsAction = SKIP_OPTION;
	}

#if 0 /* Prevent recall */
	// Recall the current active menu to repaint the display
	mn_msg.cmd = 0; mn_msg.length = 0; mn_msg.data[0] = 0;
	JUMP_TO_ACTIVE_MENU();
#endif
}
#endif

//*****************************************************************************
//=============================================================================
// Timer Mode Menu
//=============================================================================
//*****************************************************************************
#define TIMER_MODE_MENU_ENTRIES 4
USER_MENU_STRUCT timerModeMenu[TIMER_MODE_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, TIMER_MODE_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, TIMER_MODE_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, DISABLED_TEXT,	NO_TAG, {DISABLED}},
{ITEM_2, 0, ENABLED_TEXT,	NO_TAG, {ENABLED}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&TimerModeMenuHandler}}
};

//------------------------
// Timer Mode Menu Handler
//------------------------
void TimerModeMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_unitConfig.timerMode = (uint8)timerModeMenu[newItemIndex].data;

		if (g_unitConfig.timerMode == ENABLED)
		{
			SETUP_USER_MENU_MSG(&timerModeFreqMenu, g_unitConfig.timerModeFrequency);
		}
		else
		{
			SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
		}

		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);
	}
	else if (keyPressed == ESC_KEY)
	{
		if (g_unitConfig.timerMode == ENABLED)
		{
			g_unitConfig.timerMode = DISABLED;

			SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

			// Disable the Power Off timer if it's set
			ClearSoftTimer(POWER_OFF_TIMER_NUM);

			MessageBox(getLangText(STATUS_TEXT), getLangText(TIMER_MODE_SETUP_NOT_COMPLETED_TEXT), MB_OK);
			MessageBox(getLangText(STATUS_TEXT), getLangText(DISABLING_TIMER_MODE_TEXT), MB_OK);
		}

		SETUP_USER_MENU_MSG(&configMenu, TIMER_MODE);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Timer Mode Freq Menu
//=============================================================================
//*****************************************************************************
#define TIMER_MODE_FREQ_MENU_ENTRIES 8
USER_MENU_STRUCT timerModeFreqMenu[TIMER_MODE_FREQ_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, TIMER_FREQUENCY_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, TIMER_MODE_FREQ_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, ONE_TIME_TEXT,			NO_TAG, {TIMER_MODE_ONE_TIME}},
{ITEM_2, 0, HOURLY_TEXT,			NO_TAG, {TIMER_MODE_HOURLY}},
{ITEM_3, 0, DAILY_EVERY_DAY_TEXT,	NO_TAG, {TIMER_MODE_DAILY}},
{ITEM_4, 0, DAILY_WEEKDAYS_TEXT,	NO_TAG, {TIMER_MODE_WEEKDAYS}},
{ITEM_5, 0, WEEKLY_TEXT,			NO_TAG, {TIMER_MODE_WEEKLY}},
{ITEM_6, 0, MONTHLY_TEXT,			NO_TAG, {TIMER_MODE_MONTHLY}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&TimerModeFreqMenuHandler}}
};

//-----------------------------
// Timer Mode Freq Menu Handler
//-----------------------------
void TimerModeFreqMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_unitConfig.timerModeFrequency = (uint8)timerModeFreqMenu[newItemIndex].data;

		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		if (g_unitConfig.timerModeFrequency == TIMER_MODE_HOURLY)
		{
			MessageBox(getLangText(TIMER_MODE_TEXT), getLangText(FOR_HOURLY_MODE_THE_HOURS_AND_MINUTES_FIELDS_ARE_INDEPENDENT_TEXT), MB_OK);
			MessageBox(getLangText(TIMER_MODE_HOURLY_TEXT), getLangText(HOURS_ARE_ACTIVE_HOURS_MINS_ARE_ACTIVE_MIN_RANGE_EACH_HOUR_TEXT), MB_OK);
		}
		
		SETUP_MENU_MSG(TIMER_MODE_TIME_MENU);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&timerModeMenu, g_unitConfig.timerMode);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Units Of Measure Menu
//=============================================================================
//*****************************************************************************
#define UNITS_OF_MEASURE_MENU_ENTRIES 4
USER_MENU_STRUCT unitsOfMeasureMenu[UNITS_OF_MEASURE_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, UNITS_OF_MEASURE_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, UNITS_OF_MEASURE_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, IMPERIAL_TEXT,	NO_TAG, {IMPERIAL_TYPE}},
{ITEM_2, 0, METRIC_TEXT,	NO_TAG, {METRIC_TYPE}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&UnitsOfMeasureMenuHandler}}
};

//------------------------------
// Units Of Measure Menu Handler
//------------------------------
void UnitsOfMeasureMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_unitConfig.unitsOfMeasure = (uint8)unitsOfMeasureMenu[newItemIndex].data;

		if (g_unitConfig.unitsOfMeasure == IMPERIAL_TYPE)
		{
			g_sensorInfo.unitsFlag = IMPERIAL_TYPE;
			g_sensorInfo.measurementRatio = (float)IMPERIAL;
		}
		else // Metric
		{
			g_sensorInfo.unitsFlag = METRIC_TYPE;
			g_sensorInfo.measurementRatio = (float)METRIC;
		}

		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, UNITS_OF_MEASURE);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Units Of Air Menu
//=============================================================================
//*****************************************************************************
#define UNITS_OF_AIR_MENU_ENTRIES 5
USER_MENU_STRUCT unitsOfAirMenu[UNITS_OF_AIR_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, UNITS_OF_AIR_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, UNITS_OF_AIR_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, DECIBEL_TEXT,	NO_TAG, {DECIBEL_TYPE}},
{ITEM_2, 0, MILLIBAR_TEXT,	NO_TAG, {MILLIBAR_TYPE}},
{ITEM_2, 0, PSI_TEXT,		NO_TAG, {PSI_TYPE}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&UnitsOfAirMenuHandler}}
};

//--------------------------
// Units Of Air Menu Handler
//--------------------------
void UnitsOfAirMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		if (g_unitConfig.unitsOfAir != (uint8)unitsOfAirMenu[newItemIndex].data)
		{
			g_unitConfig.unitsOfAir = (uint8)unitsOfAirMenu[newItemIndex].data;

			if (g_unitConfig.unitsOfAir == DECIBEL_TYPE) { g_sensorInfo.airUnitsFlag = DB_TYPE; }
			else if (g_unitConfig.unitsOfAir == MILLIBAR_TYPE) { g_sensorInfo.airUnitsFlag = MB_TYPE; }
			else /* (g_unitConfig.unitsOfAir == PSI_TYPE) */ { g_sensorInfo.airUnitsFlag = PSI_TYPE; }

			SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);
		}

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, UNITS_OF_AIR);
	}

	JUMP_TO_ACTIVE_MENU();
}

#if 0 /* Removing this option */
//*****************************************************************************
//=============================================================================
// Vector Sum Menu
//=============================================================================
//*****************************************************************************
#define VECTOR_SUM_MENU_ENTRIES 4
USER_MENU_STRUCT vectorSumMenu[VECTOR_SUM_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, VECTOR_SUM_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, VECTOR_SUM_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, OFF_TEXT,	NO_TAG, {DISABLED}},
{ITEM_2, 0, ON_TEXT,	NO_TAG, {ENABLED}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&VectorSumMenuHandler}}
};

//------------------------
// Vector Sum Menu Handler
//------------------------
void VectorSumMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_unitConfig.vectorSum = (uint8)vectorSumMenu[newItemIndex].data;

		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, VECTOR_SUM);
	}

	JUMP_TO_ACTIVE_MENU();
}
#endif

#if (!VT_FEATURE_DISABLED)
//*****************************************************************************
//=============================================================================
// Vibration Standard Menu
//=============================================================================
//*****************************************************************************
#define VIBRATION_STARDARD_MENU_ENTRIES 6
USER_MENU_STRUCT vibrationStandardMenu[VIBRATION_STARDARD_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, VIBRATION_STANDARD_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, VIBRATION_STARDARD_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, USBM_8507_DRYWALL_TEXT,	NO_TAG,	{USBM_RI_8507_DRYWALL_STANDARD}},
{ITEM_2, 0, USBM_8507_PLASTER_TEXT,	NO_TAG,	{USBM_RI_8507_PLASTER_STANDARD}},
{ITEM_3, 0, OSM_REGULATIONS_TEXT,	NO_TAG,	{OSM_REGULATIONS_STANDARD}},
{ITEM_4, 0, CUSTOM_CURVE_TEXT,		NO_TAG,	{START_OF_CUSTOM_CURVES_LIST}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&VibrationStandardMenuHandler}}
};

//--------------------------------
// Vibration Standard Menu Handler
//--------------------------------
void VibrationStandardMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);
#if 0 /* Removed for new Percent of Limit Trigger feature */
	float div = (float)(g_bitAccuracyMidpoint * g_sensorInfo.sensorAccuracy * ((g_triggerRecord.srec.sensitivity == LOW) ? 2 : 4)) / (float)(g_factorySetupRecord.seismicSensorType);
#endif

	if (keyPressed == ENTER_KEY)
	{
		if (vibrationStandardMenu[newItemIndex].data != START_OF_CUSTOM_CURVES_LIST)
		{
			g_triggerRecord.trec.variableTriggerVibrationStandard = (uint8)vibrationStandardMenu[newItemIndex].data;

#if 0 /* Removed for new % of Limit Trigger feature */
			g_triggerRecord.trec.variableTriggerEnable = YES;

			// Set the fixed trigger level to 2 IPS since anything above this level is an automatic trigger for all vibration standards
			g_triggerRecord.trec.seismicTriggerLevel = (uint32)(2.00 * div);

			// Up convert to 16-bit since user selected level is based on selected bit accuracy
			g_triggerRecord.trec.seismicTriggerLevel *= (SEISMIC_TRIGGER_MAX_VALUE / g_bitAccuracyMidpoint);

			debug("Seismic Trigger: %d counts\r\n", g_triggerRecord.trec.seismicTriggerLevel);

			g_tempTriggerLevelForMenuAdjustment = AirTriggerConvertToUnits(g_triggerRecord.trec.airTriggerLevel);
			SETUP_USER_MENU_FOR_INTEGERS_MSG(&airTriggerMenu, &g_tempTriggerLevelForMenuAdjustment, GetAirDefaultValue(), GetAirMinValue(), GetAirMaxValue());
#else /* New Percent of Limit Trigger feature */
			SETUP_USER_MENU_FOR_INTEGERS_MSG(&percentLimitTriggerMenu, &g_triggerRecord.trec.variableTriggerPercentageLevel, VT_PERCENT_OF_LIMIT_DEFAULT_VALUE, VT_PERCENT_OF_LIMIT_MIN_VALUE, VT_PERCENT_OF_LIMIT_MAX_VALUE);
#endif
		}
		else // (newItemIndex == START_OF_CUSTOM_CURVES_LIST)
		{
			SETUP_USER_MENU_MSG(&customCurveMenu, g_triggerRecord.trec.variableTriggerVibrationStandard);
		}
	}
	else if (keyPressed == ESC_KEY)
	{
		// Return to Seismic Trigger type menu passing a 1 for Variable Trigger Enable (although it isn't actually set until this menu completes)
		SETUP_USER_MENU_MSG(&seismicTriggerTypeMenu, 1);
	}

	JUMP_TO_ACTIVE_MENU();
}
#endif

//*****************************************************************************
//=============================================================================
// Waveform Auto Cal Menu
//=============================================================================
//*****************************************************************************
#define WAVEFORM_AUTO_CAL_MENU_ENTRIES 4
USER_MENU_STRUCT waveformAutoCalMenu[WAVEFORM_AUTO_CAL_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, WAVEFORM_AUTO_CAL_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, WAVEFORM_AUTO_CAL_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, DISABLED_TEXT,				NO_TAG, {DISABLED}},
{ITEM_2, 0, START_OF_WAVEFORM_TEXT,	NO_TAG, {ENABLED}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&WaveformAutoCalMenuHandler}}
};

//-------------------------------
// Waveform Auto Cal Menu Handler
//-------------------------------
void WaveformAutoCalMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);

	if (keyPressed == ENTER_KEY)
	{
		g_unitConfig.autoCalForWaveform = (uint8)waveformAutoCalMenu[newItemIndex].data;

		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, WAVEFORM_AUTO_CAL);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Zero Event Number Menu
//=============================================================================
//*****************************************************************************
#define ZERO_EVENT_NUMBER_MENU_ENTRIES 4
USER_MENU_STRUCT zeroEventNumberMenu[ZERO_EVENT_NUMBER_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, ZERO_EVENT_NUMBER_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(SELECT_TYPE, ZERO_EVENT_NUMBER_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ITEM_1)}},
{ITEM_1, 0, YES_TEXT,	NO_TAG, {YES}},
{ITEM_2, 0, NO_TEXT,	NO_TAG, {NO}},
{END_OF_MENU, (uint8)0, (uint8)0, (uint8)0, {(uint32)&ZeroEventNumberMenuHandler}}
};

//-------------------------------
// Zero Event Number Menu Handler
//-------------------------------
void ZeroEventNumberMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint16 newItemIndex = *((uint16*)data);
	CURRENT_EVENT_NUMBER_STRUCT eventNumberRecord;

	if (keyPressed == ENTER_KEY)
	{
		if (zeroEventNumberMenu[newItemIndex].data == YES)
		{
			eventNumberRecord.currentEventNumber = 0;
			SaveRecordData(&eventNumberRecord, DEFAULT_RECORD, REC_UNIQUE_EVENT_ID_TYPE);

			InitCurrentEventNumber();

			__autoDialoutTbl.lastDownloadedEvent = 0;

			OverlayMessage(getLangText(SUCCESS_TEXT), getLangText(EVENT_NUMBER_ZEROING_COMPLETE_TEXT), (2 * SOFT_SECS));
		}

		SETUP_USER_MENU_MSG(&eraseSettingsMenu, NO);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&eraseEventsMenu, YES);
	}

	JUMP_TO_ACTIVE_MENU();
}
