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
#include "RemoteCommon.h"
#include "RemoteHandler.h"
#include "TextTypes.h"
#include "Sensor.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
#include "Globals.h"
extern USER_MENU_STRUCT alarmTwoMenu[];
extern USER_MENU_STRUCT alarmOneSeismicLevelMenu[];
extern USER_MENU_STRUCT alarmOneAirLevelMenu[];
extern USER_MENU_STRUCT alarmTwoSeismicLevelMenu[];
extern USER_MENU_STRUCT alarmTwoAirLevelMenu[];
extern USER_MENU_STRUCT alarmOneTimeMenu[];
extern USER_MENU_STRUCT alarmTwoTimeMenu[];
extern USER_MENU_STRUCT alarmOneMenu[];
extern USER_MENU_STRUCT alarmTwoMenu[];
extern USER_MENU_STRUCT airScaleMenu[];
extern USER_MENU_STRUCT barChannelMenu[];
extern USER_MENU_STRUCT barResultMenu[];
extern USER_MENU_STRUCT barIntervalDataTypeMenu[];
extern USER_MENU_STRUCT bitAccuracyMenu[];
extern USER_MENU_STRUCT configMenu[];
extern USER_MENU_STRUCT customCurveMenu[];
extern USER_MENU_STRUCT distanceToSourceMenu[];
extern USER_MENU_STRUCT externalTriggerMenu[];
extern USER_MENU_STRUCT hardwareIDMenu[];
extern USER_MENU_STRUCT modemSetupMenu[];
extern USER_MENU_STRUCT modemDialMenu[];
extern USER_MENU_STRUCT modemDialOutTypeMenu[];
extern USER_MENU_STRUCT modemInitMenu[];
extern USER_MENU_STRUCT modemResetMenu[];
extern USER_MENU_STRUCT modemRetryMenu[];
extern USER_MENU_STRUCT modemRetryTimeMenu[];
extern USER_MENU_STRUCT modeMenu[];
extern USER_MENU_STRUCT notesMenu[];
extern USER_MENU_STRUCT operatorMenu[];
extern USER_MENU_STRUCT percentLimitTriggerMenu[];
extern USER_MENU_STRUCT recalibrateMenu[];
extern USER_MENU_STRUCT recordTimeMenu[];
extern USER_MENU_STRUCT saveSetupMenu[];
extern USER_MENU_STRUCT seismicTriggerMenu[];
extern USER_MENU_STRUCT seismicLocationMenu[];
extern USER_MENU_STRUCT seismicSensorTypeMenu[];
#if (!VT_FEATURE_DISABLED)
extern USER_MENU_STRUCT seismicTriggerTypeMenu[];
#endif
extern USER_MENU_STRUCT sensitivityMenu[];
extern USER_MENU_STRUCT storedEventsCapModeMenu[];
extern USER_MENU_STRUCT storedEventLimitMenu[];
extern USER_MENU_STRUCT summaryIntervalMenu[];
extern USER_MENU_STRUCT unlockCodeMenu[];
extern USER_MENU_STRUCT vibrationStandardMenu[];
extern USER_MENU_STRUCT weightPerDelayMenu[];

///----------------------------------------------------------------------------
///	Local Scope Globals
///----------------------------------------------------------------------------

//*****************************************************************************
//=============================================================================
// Air Trigger Menu
//=============================================================================
//*****************************************************************************
#define AIR_TRIGGER_MENU_ENTRIES 4
USER_MENU_STRUCT airTriggerMenu[AIR_TRIGGER_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, AIR_TRIGGER_TEXT, TITLE_POST_TAG, 
	{INSERT_USER_MENU_INFO(INTEGER_SPECIAL_TYPE, AIR_TRIGGER_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_2)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {INSERT_USER_MENU_WORD_DATA(DB_TYPE, MB_TYPE)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&AirTriggerMenuHandler}}
};

//-------------------------
// Air Trigger Menu Handler
//-------------------------
void AirTriggerMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};

	if (keyPressed == ENTER_KEY)
	{	
		g_triggerRecord.trec.airTriggerLevel = AirTriggerConvert(*((uint32*)data));

		if (g_triggerRecord.trec.airTriggerLevel == NO_TRIGGER_CHAR)
		{
			debug("Air Trigger: No Trigger\r\n");
		}
		else
		{
			if (g_unitConfig.unitsOfAir == DECIBEL_TYPE) { debug("Air Trigger: %d dB\r\n", *((uint32*)data)); }
			else // (g_unitConfig.unitsOfAir == MILLIBAR_TYPE) || (g_unitConfig.unitsOfAir == PSI_TYPE)
			{
				// Millibar and PSI true values has been shifted up by 10,000
				debug("Air Trigger: %f %s\r\n", (double)(((float)(*((uint32*)data)) / (float)10000)), ((g_unitConfig.unitsOfAir == MILLIBAR_TYPE) ? "mb" : "pis"));
			}
		}

		// Check if seismic and air triggers are set to No trigger
		if (((g_triggerRecord.trec.airTriggerLevel == NO_TRIGGER_CHAR) && (g_triggerRecord.trec.seismicTriggerLevel == NO_TRIGGER_CHAR)) ||
			(g_externalTriggerMenuActiveForSetup == YES))
		{
			// Jump to the External Trigger menu (flagged active) to allow the user to change if desired
			g_externalTriggerMenuActiveForSetup = YES;
			SETUP_USER_MENU_MSG(&externalTriggerMenu, g_unitConfig.externalTrigger);
		}
		// Check if the A-weighting option is enabled
		else if ((!g_factorySetupRecord.invalid) && (g_factorySetupRecord.aWeightOption == ENABLED))
		{
			SETUP_USER_MENU_MSG(&airScaleMenu, g_unitConfig.airScale);
		}
		else // A-weighting is not enabled
		{
			SETUP_USER_MENU_FOR_INTEGERS_MSG(&recordTimeMenu, &g_triggerRecord.trec.record_time,
				RECORD_TIME_DEFAULT_VALUE, RECORD_TIME_MIN_VALUE, g_triggerRecord.trec.record_time_max);
		}
	}
	else if (keyPressed == ESC_KEY)
	{
#if VT_FEATURE_DISABLED // Original
		if (IsSeismicSensorInternalAccelerometer(g_factorySetupRecord.seismicSensorType))
		{
			USER_MENU_DEFAULT_TYPE(seismicTriggerMenu) = G_TYPE;
			USER_MENU_ALT_TYPE(seismicTriggerMenu) = G_TYPE;
		}
		else if (IsSeismicSensorAnAccelerometer(g_factorySetupRecord.seismicSensorType))
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
#else /* New Variable Trigger feature */
		if (g_triggerRecord.trec.variableTriggerEnable == YES)
		{
#if 0 /* Removed for new Percent of Limit Trigger feature */
			if (g_triggerRecord.trec.variableTriggerVibrationStandard < START_OF_CUSTOM_CURVES_LIST)
			{
				// Standard Vibration
				SETUP_USER_MENU_MSG(&vibrationStandardMenu, g_triggerRecord.trec.variableTriggerVibrationStandard);
			}
			else // Custom curve
			{
				SETUP_USER_MENU_MSG(&customCurveMenu, g_triggerRecord.trec.variableTriggerVibrationStandard);
			}
#else /* New Percent of Limit Trigger feature */
			SETUP_USER_MENU_FOR_INTEGERS_MSG(&percentLimitTriggerMenu, &g_triggerRecord.trec.variableTriggerPercentageLevel, VT_PERCENT_OF_LIMIT_DEFAULT_VALUE, VT_PERCENT_OF_LIMIT_MIN_VALUE, VT_PERCENT_OF_LIMIT_MAX_VALUE);
#endif
		}
		else // (g_triggerRecord.trec.variableTriggerEnable != YES)
		{
			if (IsSeismicSensorInternalAccelerometer(g_factorySetupRecord.seismicSensorType))
			{
				USER_MENU_DEFAULT_TYPE(seismicTriggerMenu) = G_TYPE;
				USER_MENU_ALT_TYPE(seismicTriggerMenu) = G_TYPE;
			}
			else if (IsSeismicSensorAnAccelerometer(g_factorySetupRecord.seismicSensorType))
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
#endif
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Alarm One Seismic Level Menu
//=============================================================================
//*****************************************************************************
#define ALARM_ONE_SEISMIC_LEVEL_MENU_ENTRIES 5
USER_MENU_STRUCT alarmOneSeismicLevelMenu[ALARM_ONE_SEISMIC_LEVEL_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, ALARM_1_SEISMIC_LVL_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(INTEGER_COUNT_TYPE, ALARM_ONE_SEISMIC_LEVEL_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_3)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {INSERT_USER_MENU_WORD_DATA(IN_TYPE, MM_TYPE)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&AlarmOneSeismicLevelMenuHandler}}
};

//-------------------------------------
// Alarm One Seismic Level Menu Handler
//-------------------------------------
void AlarmOneSeismicLevelMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	
	if (keyPressed == ENTER_KEY)
	{	
		g_unitConfig.alarmOneSeismicLevel = *((uint32*)data);
		
		if (g_unitConfig.alarmOneSeismicLevel == NO_TRIGGER_CHAR)
		{
			debug("Alarm 1 Seismic Trigger: No Trigger\r\n");
		}
		else
		{
			// Up convert to 16-bit since user selected level is based on selected bit accuracy
			g_unitConfig.alarmOneSeismicLevel *= (ALARM_SEIS_MAX_VALUE / g_bitAccuracyMidpoint);

			debug("Alarm 1 Seismic Level Count: %d\r\n", g_unitConfig.alarmOneSeismicLevel);
		}

		if (g_unitConfig.alarmOneMode == ALARM_MODE_BOTH)
		{
			g_tempTriggerLevelForMenuAdjustment = AirTriggerConvertToUnits(g_unitConfig.alarmOneAirLevel);
			SETUP_USER_MENU_FOR_INTEGERS_MSG(&alarmOneAirLevelMenu, &g_tempTriggerLevelForMenuAdjustment, AirTriggerConvertToUnits(g_alarmOneAirMinLevel), AirTriggerConvertToUnits(g_alarmOneAirMinLevel), GetAirMaxValue());
		}
		else // g_unitConfig.alarmOneMode == ALARM_MODE_SEISMIC
		{
			SETUP_USER_MENU_FOR_FLOATS_MSG(&alarmOneTimeMenu, &g_unitConfig.alarmOneTime, ALARM_OUTPUT_TIME_DEFAULT, ALARM_OUTPUT_TIME_INCREMENT,
											ALARM_OUTPUT_TIME_MIN, ALARM_OUTPUT_TIME_MAX);
		}
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&alarmOneMenu, g_unitConfig.alarmOneMode);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Alarm One Air Level Menu
//=============================================================================
//*****************************************************************************
#define ALARM_ONE_AIR_LEVEL_MENU_ENTRIES 4
USER_MENU_STRUCT alarmOneAirLevelMenu[ALARM_ONE_AIR_LEVEL_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, ALARM_1_AIR_LEVEL_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(INTEGER_SPECIAL_TYPE, ALARM_ONE_AIR_LEVEL_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_2)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {INSERT_USER_MENU_WORD_DATA(DB_TYPE, MB_TYPE)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&AlarmOneAirLevelMenuHandler}}
};

//---------------------------------
// Alarm One Air Level Menu Handler
//---------------------------------
void AlarmOneAirLevelMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	
	if (keyPressed == ENTER_KEY)
	{	
		g_unitConfig.alarmOneAirLevel = AirTriggerConvert(*((uint32*)data));
		
		debug("Alarm 1 Air Level: %d\r\n", g_unitConfig.alarmOneAirLevel);

		SETUP_USER_MENU_FOR_FLOATS_MSG(&alarmOneTimeMenu, &g_unitConfig.alarmOneTime, ALARM_OUTPUT_TIME_DEFAULT, ALARM_OUTPUT_TIME_INCREMENT,
										ALARM_OUTPUT_TIME_MIN, ALARM_OUTPUT_TIME_MAX);
	}
	else if (keyPressed == ESC_KEY)
	{
		if (g_unitConfig.alarmOneMode == ALARM_MODE_BOTH)
		{
			// Down convert to current bit accuracy setting
			g_tempTriggerLevelForMenuAdjustment = SeismicTriggerConvertBitAccuracy(g_unitConfig.alarmOneSeismicLevel);

			SETUP_USER_MENU_FOR_INTEGERS_MSG(&alarmOneSeismicLevelMenu, &g_tempTriggerLevelForMenuAdjustment,
											(g_alarmOneSeismicMinLevel / (ALARM_SEIS_MAX_VALUE / g_bitAccuracyMidpoint)),
											(g_alarmOneSeismicMinLevel / (ALARM_SEIS_MAX_VALUE / g_bitAccuracyMidpoint)), g_bitAccuracyMidpoint);
		}
		else
		{
			SETUP_USER_MENU_MSG(&alarmOneMenu, g_unitConfig.alarmOneMode);
		}
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Alarm One Time Menu
//=============================================================================
//*****************************************************************************
#define ALARM_ONE_TIME_MENU_ENTRIES 4
USER_MENU_STRUCT alarmOneTimeMenu[ALARM_ONE_TIME_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, ALARM_1_TIME_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(FLOAT_TYPE, ALARM_ONE_TIME_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_2)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {INSERT_USER_MENU_WORD_DATA(SECS_TYPE, NO_ALT_TYPE)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&AlarmOneTimeMenuHandler}}
};

//----------------------------
// Alarm One Time Menu Handler
//----------------------------
void AlarmOneTimeMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	
	if (keyPressed == ENTER_KEY)
	{	
		g_unitConfig.alarmOneTime = *((float*)data);
		
		debug("Alarm 1 Time: %f\r\n", (double)g_unitConfig.alarmOneTime);

		SETUP_USER_MENU_MSG(&alarmTwoMenu, g_unitConfig.alarmTwoMode);
	}
	else if (keyPressed == ESC_KEY)
	{
		if ((g_unitConfig.alarmOneMode == ALARM_MODE_BOTH) || (g_unitConfig.alarmOneMode == ALARM_MODE_AIR))
		{
			g_tempTriggerLevelForMenuAdjustment = AirTriggerConvertToUnits(g_unitConfig.alarmOneAirLevel);
			SETUP_USER_MENU_FOR_INTEGERS_MSG(&alarmOneAirLevelMenu, &g_tempTriggerLevelForMenuAdjustment, AirTriggerConvertToUnits(g_alarmOneAirMinLevel), AirTriggerConvertToUnits(g_alarmOneAirMinLevel), GetAirMaxValue());
		}
		else if (g_unitConfig.alarmOneMode == ALARM_MODE_SEISMIC)
		{
			// Down convert to current bit accuracy setting
			g_tempTriggerLevelForMenuAdjustment = SeismicTriggerConvertBitAccuracy(g_unitConfig.alarmOneSeismicLevel);

			SETUP_USER_MENU_FOR_INTEGERS_MSG(&alarmOneSeismicLevelMenu, &g_tempTriggerLevelForMenuAdjustment,
											(g_alarmOneSeismicMinLevel / (ALARM_SEIS_MAX_VALUE / g_bitAccuracyMidpoint)),
											(g_alarmOneSeismicMinLevel / (ALARM_SEIS_MAX_VALUE / g_bitAccuracyMidpoint)), g_bitAccuracyMidpoint);
		}
		else // g_unitConfig.alarmOneMode == ALARM_MODE_OFF
		{
			SETUP_USER_MENU_MSG(&alarmOneMenu, g_unitConfig.alarmOneMode);
		}
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Alarm Two Seismic Level Menu
//=============================================================================
//*****************************************************************************
#define ALARM_TWO_SEISMIC_LEVEL_MENU_ENTRIES 5
USER_MENU_STRUCT alarmTwoSeismicLevelMenu[ALARM_TWO_SEISMIC_LEVEL_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, ALARM_2_SEISMIC_LVL_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(INTEGER_COUNT_TYPE, ALARM_TWO_SEISMIC_LEVEL_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_3)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {INSERT_USER_MENU_WORD_DATA(IN_TYPE, MM_TYPE)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&AlarmTwoSeismicLevelMenuHandler}}
};

//-------------------------------------
// Alarm Two Seismic Level Menu Handler
//-------------------------------------
void AlarmTwoSeismicLevelMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	
	if (keyPressed == ENTER_KEY)
	{	
		g_unitConfig.alarmTwoSeismicLevel = *((uint32*)data);
		
		if (g_unitConfig.alarmTwoSeismicLevel == NO_TRIGGER_CHAR)
		{
			debug("Alarm 2 Seismic Trigger: No Trigger\r\n");
		}
		else
		{
			// Up convert to 16-bit since user selected level is based on selected bit accuracy
			g_unitConfig.alarmTwoSeismicLevel *= (ALARM_SEIS_MAX_VALUE / g_bitAccuracyMidpoint);

			debug("Alarm 2 Seismic Level Count: %d\r\n", g_unitConfig.alarmTwoSeismicLevel);
		}

		if (g_unitConfig.alarmTwoMode == ALARM_MODE_BOTH)
		{
			g_tempTriggerLevelForMenuAdjustment = AirTriggerConvertToUnits(g_unitConfig.alarmTwoAirLevel);
			SETUP_USER_MENU_FOR_INTEGERS_MSG(&alarmTwoAirLevelMenu, &g_tempTriggerLevelForMenuAdjustment, AirTriggerConvertToUnits(g_alarmTwoAirMinLevel), AirTriggerConvertToUnits(g_alarmTwoAirMinLevel), GetAirMaxValue());
		}
		else // g_unitConfig.alarmTwoMode == ALARM_MODE_SEISMIC
		{
			SETUP_USER_MENU_FOR_FLOATS_MSG(&alarmTwoTimeMenu, &g_unitConfig.alarmTwoTime, ALARM_OUTPUT_TIME_DEFAULT, ALARM_OUTPUT_TIME_INCREMENT,
											ALARM_OUTPUT_TIME_MIN, ALARM_OUTPUT_TIME_MAX);
		}
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&alarmTwoMenu, g_unitConfig.alarmTwoMode);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Alarm Two Air Level Menu
//=============================================================================
//*****************************************************************************
#define ALARM_TWO_AIR_LEVEL_MENU_ENTRIES 4
USER_MENU_STRUCT alarmTwoAirLevelMenu[ALARM_TWO_AIR_LEVEL_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, ALARM_2_AIR_LEVEL_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(INTEGER_SPECIAL_TYPE, ALARM_TWO_AIR_LEVEL_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_2)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {INSERT_USER_MENU_WORD_DATA(DB_TYPE, MB_TYPE)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&AlarmTwoAirLevelMenuHandler}}
};

//---------------------------------
// Alarm Two Air Level Menu Handler
//---------------------------------
void AlarmTwoAirLevelMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	
	if (keyPressed == ENTER_KEY)
	{	
		g_unitConfig.alarmTwoAirLevel = AirTriggerConvert(*((uint32*)data));
		
		debug("Alarm 2 Air Level: %d\r\n", g_unitConfig.alarmTwoAirLevel);

		SETUP_USER_MENU_FOR_FLOATS_MSG(&alarmTwoTimeMenu, &g_unitConfig.alarmTwoTime, ALARM_OUTPUT_TIME_DEFAULT, ALARM_OUTPUT_TIME_INCREMENT,
										ALARM_OUTPUT_TIME_MIN, ALARM_OUTPUT_TIME_MAX);
	}
	else if (keyPressed == ESC_KEY)
	{
		if (g_unitConfig.alarmTwoMode == ALARM_MODE_BOTH)
		{
			// Down convert to current bit accuracy setting
			g_tempTriggerLevelForMenuAdjustment = SeismicTriggerConvertBitAccuracy(g_unitConfig.alarmTwoSeismicLevel);

			SETUP_USER_MENU_FOR_INTEGERS_MSG(&alarmTwoSeismicLevelMenu, &g_tempTriggerLevelForMenuAdjustment,
											(g_alarmTwoSeismicMinLevel / (ALARM_SEIS_MAX_VALUE / g_bitAccuracyMidpoint)),
											(g_alarmTwoSeismicMinLevel / (ALARM_SEIS_MAX_VALUE / g_bitAccuracyMidpoint)), g_bitAccuracyMidpoint);
		}
		else
		{
			SETUP_USER_MENU_MSG(&alarmTwoMenu, g_unitConfig.alarmTwoMode);
		}
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Alarm Two Time Menu
//=============================================================================
//*****************************************************************************
#define ALARM_TWO_TIME_MENU_ENTRIES 4
USER_MENU_STRUCT alarmTwoTimeMenu[ALARM_TWO_TIME_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, ALARM_2_TIME_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(FLOAT_TYPE, ALARM_TWO_TIME_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_2)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {INSERT_USER_MENU_WORD_DATA(SECS_TYPE, NO_ALT_TYPE)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&AlarmTwoTimeMenuHandler}}
};

//----------------------------
// Alarm Two Time Menu Handler
//----------------------------
void AlarmTwoTimeMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	
	if (keyPressed == ENTER_KEY)
	{	
		g_unitConfig.alarmTwoTime = *((float*)data);
		
		debug("Alarm 2 Time: %f\r\n", (double)g_unitConfig.alarmTwoTime);

		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		SETUP_USER_MENU_MSG(&saveSetupMenu, YES);
	}
	else if (keyPressed == ESC_KEY)
	{
		if ((g_unitConfig.alarmTwoMode == ALARM_MODE_BOTH) || (g_unitConfig.alarmTwoMode == ALARM_MODE_AIR))
		{
			g_tempTriggerLevelForMenuAdjustment = AirTriggerConvertToUnits(g_unitConfig.alarmTwoAirLevel);
			SETUP_USER_MENU_FOR_INTEGERS_MSG(&alarmTwoAirLevelMenu, &g_tempTriggerLevelForMenuAdjustment, AirTriggerConvertToUnits(g_alarmTwoAirMinLevel), AirTriggerConvertToUnits(g_alarmTwoAirMinLevel), GetAirMaxValue());
		}
		else if (g_unitConfig.alarmTwoMode == ALARM_MODE_SEISMIC)
		{
			// Down convert to current bit accuracy setting
			g_tempTriggerLevelForMenuAdjustment = SeismicTriggerConvertBitAccuracy(g_unitConfig.alarmTwoSeismicLevel);

			SETUP_USER_MENU_FOR_INTEGERS_MSG(&alarmTwoSeismicLevelMenu, &g_tempTriggerLevelForMenuAdjustment,
											(g_alarmTwoSeismicMinLevel / (ALARM_SEIS_MAX_VALUE / g_bitAccuracyMidpoint)),
											(g_alarmTwoSeismicMinLevel / (ALARM_SEIS_MAX_VALUE / g_bitAccuracyMidpoint)), g_bitAccuracyMidpoint);
		}
		else // g_unitConfig.alarmTwoMode == ALARM_MODE_OFF
		{
			SETUP_USER_MENU_MSG(&alarmTwoMenu, g_unitConfig.alarmTwoMode);
		}
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Company Menu
//=============================================================================
//*****************************************************************************
#define COMPANY_MENU_ENTRIES 5
#define MAX_COMPANY_CHARS 30
USER_MENU_STRUCT companyMenu[COMPANY_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, COMPANY_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(STRING_TYPE, COMPANY_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_2)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {MAX_COMPANY_CHARS}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&CompanyMenuHandler}}
};

//---------------------
// Company Menu Handler
//---------------------
void CompanyMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	
	if (keyPressed == ENTER_KEY)
	{	
		strcpy((char*)(&g_triggerRecord.trec.client), (char*)data);
		debug("Company: <%s>, Length: %d\r\n", g_triggerRecord.trec.client, strlen((char*)g_triggerRecord.trec.client));
		
		SETUP_USER_MENU_MSG(&seismicLocationMenu, &g_triggerRecord.trec.loc);
	}
	else if (keyPressed == ESC_KEY)
	{
		if (g_triggerRecord.trec.sample_rate == SAMPLE_RATE_16K)
		{
			SETUP_USER_MENU_MSG(&bitAccuracyMenu, g_triggerRecord.trec.bitAccuracy);
		}
		else // All other sample rates
		{
			SETUP_USER_MENU_MSG(&recalibrateMenu, g_triggerRecord.trec.adjustForTempDrift);
		}
	}

	JUMP_TO_ACTIVE_MENU();
}

#if 0 /* Unused */
//*****************************************************************************
//=============================================================================
// Copies Menu
//=============================================================================
//*****************************************************************************
#define COPIES_MENU_ENTRIES 4
USER_MENU_STRUCT copiesMenu[COPIES_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, COPIES_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(INTEGER_BYTE_TYPE, COPIES_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_2)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {INSERT_USER_MENU_WORD_DATA(NO_TYPE, NO_ALT_TYPE)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&CopiesMenuHandler}}
};

//--------------------
// Copies Menu Handler
//--------------------
void CopiesMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	
	if (keyPressed == ENTER_KEY)
	{	
		// Check if the user is printing an event from the summary list
		if (g_summaryListMenuActive == YES)
		{
			SETUP_MENU_MSG(SUMMARY_MENU); mn_msg.data[0] = ESC_KEY;
		}
		else // g_summaryListMenuActive == NO
		{
			g_unitConfig.copies = *((uint8*)data);
			debug("Copies: %d\r\n", g_unitConfig.copies);

			SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

			SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
		}
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, COPIES);
	}

	JUMP_TO_ACTIVE_MENU();
}
#endif

//*****************************************************************************
//=============================================================================
// Cycle End Time Menu
//=============================================================================
//*****************************************************************************
#define CYCLE_END_TIME_HOUR_MENU_ENTRIES 4
USER_MENU_STRUCT cycleEndTimeMenu[CYCLE_END_TIME_HOUR_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, CYCLE_END_TIME_24HR_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(INTEGER_BYTE_TYPE, CYCLE_END_TIME_HOUR_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_2)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {INSERT_USER_MENU_WORD_DATA(HOUR_TYPE, NO_ALT_TYPE)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&CycleEndTimeMenuHandler}}
};

//----------------------------
// Cycle End Time Menu Handler
//----------------------------
void CycleEndTimeMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};

	if (keyPressed == ENTER_KEY)
	{
		if (g_unitConfig.cycleEndTimeHour != *((uint8*)data))
		{
			g_unitConfig.cycleEndTimeHour = *((uint8*)data);
			debug("Cycle End Time Hour: %d\r\n", g_unitConfig.cycleEndTimeHour);

			SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

			if (g_unitConfig.cycleEndTimeHour == 0)
			{
				sprintf((char*)g_spareBuffer, "%s %s", getLangText(TWENTY_FOUR_HR_CYCLE_WILL_NOW_OCCUR_AT_TEXT), getLangText(MIDNIGHT_TEXT));
			}
			else
			{
				sprintf((char*)g_spareBuffer, "%s %d %s", getLangText(TWENTY_FOUR_HR_CYCLE_WILL_NOW_OCCUR_AT_TEXT), ((g_unitConfig.cycleEndTimeHour > 12) ? (g_unitConfig.cycleEndTimeHour - 12) : g_unitConfig.cycleEndTimeHour),
						((g_unitConfig.cycleEndTimeHour > 12) ? "PM" : "AM"));
			}
			MessageBox(getLangText(STATUS_TEXT), (char*)g_spareBuffer, MB_OK);
		}

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, CYCLE_END_TIME_HOUR);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Distance to Source Menu
//=============================================================================
//*****************************************************************************
#define DISTANCE_TO_SOURCE_MENU_ENTRIES 4
USER_MENU_STRUCT distanceToSourceMenu[DISTANCE_TO_SOURCE_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, DISTANCE_TO_SOURCE_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(FLOAT_WITH_N_TYPE, DISTANCE_TO_SOURCE_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_2)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {INSERT_USER_MENU_WORD_DATA(FT_TYPE, M_TYPE)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&DistanceToSourceMenuHandler}}
};

//--------------------------------
// Distance to Source Menu Handler
//--------------------------------
void DistanceToSourceMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	
	if (keyPressed == ENTER_KEY)
	{	
		g_triggerRecord.trec.dist_to_source = *((float*)data);

		if (g_unitConfig.unitsOfMeasure == METRIC_TYPE)
			g_triggerRecord.trec.dist_to_source *= FT_PER_METER;

		debug("Distance to Source: %f ft\r\n", (double)g_triggerRecord.trec.dist_to_source);

		if ((g_triggerRecord.opMode == WAVEFORM_MODE) || (g_triggerRecord.opMode == COMBO_MODE))
		{
			SETUP_USER_MENU_FOR_FLOATS_MSG(&weightPerDelayMenu, &g_triggerRecord.trec.weight_per_delay,
				WEIGHT_PER_DELAY_DEFAULT_VALUE, WEIGHT_PER_DELAY_INCREMENT_VALUE,
				WEIGHT_PER_DELAY_MIN_VALUE, WEIGHT_PER_DELAY_MAX_VALUE);
		}
		else if (g_triggerRecord.opMode == BARGRAPH_MODE)
		{
			SETUP_USER_MENU_MSG(&operatorMenu, &g_triggerRecord.trec.oper);
		}
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&notesMenu, &g_triggerRecord.trec.comments);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Gps Timeout Menu
//=============================================================================
//*****************************************************************************
#if 0
#define GPS_TIMEOUT_MENU_ENTRIES 4
USER_MENU_STRUCT gpsTimeoutMenu[GPS_TIMEOUT_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, NULL_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(INTEGER_LONG_TYPE, GPS_TIMEOUT_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_2)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {INSERT_USER_MENU_WORD_DATA(SECS_TYPE, NO_ALT_TYPE)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&GpsTimeoutMenuHandler}}
};

//-------------------------
// Record Time Menu Handler
//-------------------------
void GpsTimeoutMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};

	if (keyPressed == ENTER_KEY)
	{
		//GPS retryForPositionTimeoutInSeconds = *((uint32*)data);

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu GPS_TIMEOUT);
	}

	JUMP_TO_ACTIVE_MENU();
}
#endif

//*****************************************************************************
//=============================================================================
// Lcd Impulse Time Menu
//=============================================================================
//*****************************************************************************
#define LCD_IMPULSE_TIME_MENU_ENTRIES 4
USER_MENU_STRUCT lcdImpulseTimeMenu[LCD_IMPULSE_TIME_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, LCD_IMPULSE_TIME_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(INTEGER_BYTE_TYPE, LCD_IMPULSE_TIME_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_2)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {INSERT_USER_MENU_WORD_DATA(SECS_TYPE, NO_ALT_TYPE)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&LcdImpulseTimeMenuHandler}}
};

//------------------------------
// Lcd Impulse Time Menu Handler
//------------------------------
void LcdImpulseTimeMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	
	if (keyPressed == ENTER_KEY)
	{	
		g_triggerRecord.berec.impulseMenuUpdateSecs = *((uint8*)data);

		debug("LCD Impulse Menu Update Seconds: %d\r\n", g_triggerRecord.berec.impulseMenuUpdateSecs);

		SaveRecordData(&g_triggerRecord, DEFAULT_RECORD, REC_TRIGGER_USER_MENU_TYPE);

		// Check if Bargraph mode and A-weighting is enabled
		if ((g_triggerRecord.opMode == BARGRAPH_MODE) && (!g_factorySetupRecord.invalid) && (g_factorySetupRecord.aWeightOption == ENABLED))
		{
			SETUP_USER_MENU_MSG(&airScaleMenu, g_unitConfig.airScale);
		}
		else
		{
#if 0 /* Removing this option */
			SETUP_USER_MENU_MSG(&barResultMenu, g_unitConfig.vectorSum);
#else
			// If Combo mode, jump back over to waveform specific settings
			if (g_triggerRecord.opMode == COMBO_MODE)
			{
#if VT_FEATURE_DISABLED // Original
				if (IsSeismicSensorInternalAccelerometer(g_factorySetupRecord.seismicSensorType))
				{
					USER_MENU_DEFAULT_TYPE(seismicTriggerMenu) = G_TYPE;
					USER_MENU_ALT_TYPE(seismicTriggerMenu) = G_TYPE;
				}
				else if (IsSeismicSensorAnAccelerometer(g_factorySetupRecord.seismicSensorType))
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

				SETUP_USER_MENU_FOR_INTEGERS_MSG(&seismicTriggerMenu, &g_tempTriggerLevelForMenuAdjustment, (SEISMIC_TRIGGER_DEFAULT_VALUE / (SEISMIC_TRIGGER_MAX_VALUE / g_bitAccuracyMidpoint)),
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
#endif
		}
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&barIntervalDataTypeMenu, g_triggerRecord.berec.barIntervalDataType);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Lcd Timeout Menu
//=============================================================================
//*****************************************************************************
#define LCD_TIMEOUT_MENU_ENTRIES 4
USER_MENU_STRUCT lcdTimeoutMenu[LCD_TIMEOUT_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, LCD_TIMEOUT_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(INTEGER_BYTE_TYPE, LCD_TIMEOUT_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_2)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {INSERT_USER_MENU_WORD_DATA(MINS_TYPE, NO_ALT_TYPE)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&LcdTimeoutMenuHandler}}
};

//-------------------------
// Lcd Timeout Menu Handler
//-------------------------
void LcdTimeoutMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	
	if (keyPressed == ENTER_KEY)
	{	
		g_unitConfig.lcdTimeout = *((uint8*)data);

		debug("LCD Timeout: %d\r\n", g_unitConfig.lcdTimeout);

		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, LCD_TIMEOUT);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Modem Dial Menu
//=============================================================================
//*****************************************************************************
#define MODEM_DIAL_MENU_ENTRIES 5
#define MAX_MODEM_DIAL_CHARS 30
USER_MENU_STRUCT modemDialMenu[MODEM_DIAL_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, MODEM_DIAL_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(STRING_TYPE, MODEM_DIAL_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_2)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {MAX_MODEM_DIAL_CHARS}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&ModemDialMenuHandler}}
};

//------------------------
// Modem Dial Menu Handler
//------------------------
void ModemDialMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	
	if (keyPressed == ENTER_KEY)
	{	
		strcpy((char*)(&g_modemSetupRecord.dial), (char*)data);
		debug("Modem Dial: <%s>, Length: %d\r\n", g_modemSetupRecord.dial, strlen((char*)g_modemSetupRecord.dial));

#if 0 /* Original */
		SETUP_USER_MENU_MSG(&modemResetMenu, &g_modemSetupRecord.reset);
#else /* New AutoDialOut for Config/Status */
		if (strlen((char*)g_modemSetupRecord.dial))
		{
			SETUP_USER_MENU_MSG(&modemDialOutTypeMenu, g_modemSetupRecord.dialOutType);
		}
		else
		{
			SETUP_USER_MENU_MSG(&modemResetMenu, &g_modemSetupRecord.reset);
		}
#endif
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&modemInitMenu, &g_modemSetupRecord.init);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Modem Dial Out Cycle Time Menu
//=============================================================================
//*****************************************************************************
#define MODEM_DIAL_OUT_CYCLE_TIME_MENU_ENTRIES 4
USER_MENU_STRUCT modemDialOutCycleTimeMenu[MODEM_DIAL_OUT_CYCLE_TIME_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, DIAL_OUT_CYCLE_TIME_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(INTEGER_WORD_TYPE, MODEM_DIAL_OUT_CYCLE_TIME_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_2)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {INSERT_USER_MENU_WORD_DATA(MINS_TYPE, NO_ALT_TYPE)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&ModemDialOutCycleTimeMenuHandler}}
};

//---------------------------------------
// Modem Dial Out Cycle Time Menu Handler
//---------------------------------------
void ModemDialOutCycleTimeMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};

	if (keyPressed == ENTER_KEY)
	{
		g_modemSetupRecord.dialOutCycleTime = *((uint16*)data);

		SETUP_USER_MENU_MSG(&modemResetMenu, &g_modemSetupRecord.reset);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&modemDialOutTypeMenu, g_modemSetupRecord.dialOutType);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Modem Init Menu
//=============================================================================
//*****************************************************************************
#define MODEM_INIT_MENU_ENTRIES 6
#define MAX_MODEM_INIT_CHARS 60
USER_MENU_STRUCT modemInitMenu[MODEM_INIT_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, MODEM_INIT_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(STRING_TYPE, MODEM_INIT_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_2)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {MAX_MODEM_INIT_CHARS}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&ModemInitMenuHandler}}
};

//------------------------
// Modem Init Menu Handler
//------------------------
void ModemInitMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	
	if (keyPressed == ENTER_KEY)
	{	
		strcpy((char*)(&g_modemSetupRecord.init), (char*)data);
		debug("Modem Init: <%s>, Length: %d\r\n", g_modemSetupRecord.init, strlen((char*)g_modemSetupRecord.init));

		SETUP_USER_MENU_MSG(&modemDialMenu, &g_modemSetupRecord.dial);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&modemSetupMenu, g_modemSetupRecord.modemStatus);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Modem Reset Menu
//=============================================================================
//*****************************************************************************
#define MODEM_RESET_MENU_ENTRIES 4
#define MAX_MODEM_RESET_CHARS 15
USER_MENU_STRUCT modemResetMenu[MODEM_RESET_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, MODEM_RESET_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(STRING_TYPE, MODEM_RESET_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_2)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {MAX_MODEM_RESET_CHARS}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&ModemResetMenuHandler}}
};

//-------------------------
// Modem Reset Menu Handler
//-------------------------
void ModemResetMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	
	if (keyPressed == ENTER_KEY)
	{	
		strcpy((char*)(&g_modemSetupRecord.reset), (char*)data);
		debug("Modem Reset: <%s>, Length: %d\r\n", g_modemSetupRecord.reset, strlen((char*)g_modemSetupRecord.reset));

		SETUP_USER_MENU_FOR_INTEGERS_MSG(&modemRetryMenu, &g_modemSetupRecord.retries,
			MODEM_RETRY_DEFAULT_VALUE, MODEM_RETRY_MIN_VALUE, MODEM_RETRY_MAX_VALUE);
	}
	else if (keyPressed == ESC_KEY)
	{
#if 0 /* Original */
		SETUP_USER_MENU_MSG(&modemDialMenu, &g_modemSetupRecord.dial);
#else /* New ADO feature */
		if (strlen((char*)g_modemSetupRecord.dial) == 0)
		{
			SETUP_USER_MENU_MSG(&modemDialMenu, &g_modemSetupRecord.dial);
		}
		else if (g_modemSetupRecord.dialOutType == AUTODIALOUT_EVENTS_CONFIG_STATUS)
		{
			SETUP_USER_MENU_FOR_INTEGERS_MSG(&modemDialOutCycleTimeMenu, &g_modemSetupRecord.dialOutCycleTime, MODEM_DIAL_OUT_TIMER_DEFAULT_VALUE, MODEM_DIAL_OUT_TIMER_MIN_VALUE, MODEM_DIAL_OUT_TIMER_MAX_VALUE);
		}
		else
		{
			SETUP_USER_MENU_MSG(&modemDialOutTypeMenu, g_modemSetupRecord.dialOutType);
		}
#endif
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Modem Retry Menu
//=============================================================================
//*****************************************************************************
#define MODEM_RETRY_MENU_ENTRIES 4
USER_MENU_STRUCT modemRetryMenu[MODEM_RETRY_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, MODEM_RETRY_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(INTEGER_BYTE_TYPE, MODEM_RETRY_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_2)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {INSERT_USER_MENU_WORD_DATA(NO_TYPE, NO_ALT_TYPE)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&ModemRetryMenuHandler}}
};

//-------------------------
// Modem Retry Menu Handler
//-------------------------
void ModemRetryMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	
	if (keyPressed == ENTER_KEY)
	{	
		g_modemSetupRecord.retries = *((uint8*)data);
		debug("Modem Retries: %d\r\n", g_modemSetupRecord.retries);
		
		SETUP_USER_MENU_FOR_INTEGERS_MSG(&modemRetryTimeMenu, &g_modemSetupRecord.retryTime,
			MODEM_RETRY_TIME_DEFAULT_VALUE, MODEM_RETRY_TIME_MIN_VALUE, MODEM_RETRY_TIME_MAX_VALUE);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&modemResetMenu, &g_modemSetupRecord.reset);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Modem Retry Time Menu
//=============================================================================
//*****************************************************************************
#define MODEM_RETRY_TIME_MENU_ENTRIES 4
USER_MENU_STRUCT modemRetryTimeMenu[MODEM_RETRY_TIME_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, MODEM_RETRY_TIME_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(INTEGER_BYTE_TYPE, MODEM_RETRY_TIME_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_2)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {INSERT_USER_MENU_WORD_DATA(MINS_TYPE, NO_ALT_TYPE)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&ModemRetryTimeMenuHandler}}
};

//------------------------------
// Modem Retry Time Menu Handler
//------------------------------
void ModemRetryTimeMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	
	if (keyPressed == ENTER_KEY)
	{	
		g_modemSetupRecord.retryTime = *((uint8*)data);
		debug("Modem Retry Time: %d\r\n", g_modemSetupRecord.retryTime);
		
		SETUP_USER_MENU_FOR_INTEGERS_MSG(&unlockCodeMenu, &g_modemSetupRecord.unlockCode,
			UNLOCK_CODE_DEFAULT_VALUE, UNLOCK_CODE_MIN_VALUE, UNLOCK_CODE_MAX_VALUE);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_FOR_INTEGERS_MSG(&modemRetryMenu, &g_modemSetupRecord.retries,
			MODEM_RETRY_DEFAULT_VALUE, MODEM_RETRY_MIN_VALUE, MODEM_RETRY_MAX_VALUE);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Notes Menu
//=============================================================================
//*****************************************************************************
#define NOTES_MENU_ENTRIES 8
#define MAX_NOTES_CHARS 100
USER_MENU_STRUCT notesMenu[NOTES_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, NOTES_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(STRING_TYPE, NOTES_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_2)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {MAX_NOTES_CHARS}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&NotesMenuHandler}}
};

//-------------------
// Notes Menu Handler
//-------------------
void NotesMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	
	if (keyPressed == ENTER_KEY)
	{	
		strcpy((char*)(&g_triggerRecord.trec.comments), (char*)data);
		debug("Notes: <%s>, Length: %d\r\n", g_triggerRecord.trec.comments, strlen((char*)g_triggerRecord.trec.comments));

		SETUP_USER_MENU_FOR_FLOATS_MSG(&distanceToSourceMenu, &g_triggerRecord.trec.dist_to_source,
			DISTANCE_TO_SOURCE_DEFAULT_VALUE, DISTANCE_TO_SOURCE_INCREMENT_VALUE,
			DISTANCE_TO_SOURCE_MIN_VALUE, DISTANCE_TO_SOURCE_MAX_VALUE);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&seismicLocationMenu, &g_triggerRecord.trec.loc);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Operator Menu
//=============================================================================
//*****************************************************************************
#define OPERATOR_MENU_ENTRIES 5
#define MAX_OPERATOR_CHARS 30
USER_MENU_STRUCT operatorMenu[OPERATOR_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, OPERATOR_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(STRING_TYPE, OPERATOR_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_2)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {MAX_OPERATOR_CHARS}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&OperatorMenuHandler}}
};

//----------------------
// Operator Menu Handler
//----------------------
void OperatorMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	
	if (keyPressed == ENTER_KEY)
	{	
		strcpy((char*)(&g_triggerRecord.trec.oper), (char*)data);
		debug("Operator: <%s>, Length: %d\r\n", g_triggerRecord.trec.oper, strlen((char*)g_triggerRecord.trec.oper));

		if (IsSeismicSensorInternalAccelerometer(g_factorySetupRecord.seismicSensorType))
		{
			sprintf((char*)&g_menuTags[LOW_SENSITIVITY_MAX_TAG].text, " (%.0fg)", (double)((float)g_factorySetupRecord.seismicSensorType / (float)200));
			sprintf((char*)&g_menuTags[HIGH_SENSITIVITY_MAX_TAG].text, " (%.0fg)", (double)((float)g_factorySetupRecord.seismicSensorType / (float)400));
		}
		else if (g_factorySetupRecord.seismicSensorType == SENSOR_ACCELEROMETER)
		{
			sprintf((char*)&g_menuTags[LOW_SENSITIVITY_MAX_TAG].text, " (%.0fmg)", (double)((float)g_factorySetupRecord.seismicSensorType / (float)200));
			sprintf((char*)&g_menuTags[HIGH_SENSITIVITY_MAX_TAG].text, " (%.0fmg)", (double)((float)g_factorySetupRecord.seismicSensorType / (float)400));
		}
		else if ((g_factorySetupRecord.seismicSensorType == SENSOR_ACC_832M1_0200) || (g_factorySetupRecord.seismicSensorType == SENSOR_ACC_832M1_0500))
		{
			sprintf((char*)&g_menuTags[LOW_SENSITIVITY_MAX_TAG].text, " (%.0fmg)", (double)((float)g_factorySetupRecord.seismicSensorType * ACC_832M1_SCALER / (float)200));
			sprintf((char*)&g_menuTags[HIGH_SENSITIVITY_MAX_TAG].text, " (%.0fmg)",	(double)((float)g_factorySetupRecord.seismicSensorType * ACC_832M1_SCALER / (float)400));
		}
		else if (g_unitConfig.unitsOfMeasure == IMPERIAL_TYPE)
		{
			sprintf((char*)&g_menuTags[LOW_SENSITIVITY_MAX_TAG].text, " (%.2fin)", 
					(double)((float)g_factorySetupRecord.seismicSensorType / (float)200));
			sprintf((char*)&g_menuTags[HIGH_SENSITIVITY_MAX_TAG].text, " (%.2fin)", 
					(double)((float)g_factorySetupRecord.seismicSensorType / (float)400));
		}
		else // g_unitConfig.unitsOfMeasure == METRIC_TYPE
		{
			sprintf((char*)&g_menuTags[LOW_SENSITIVITY_MAX_TAG].text, " (%.2fmm)", 
					(double)((float)g_factorySetupRecord.seismicSensorType * (float)25.4 / (float)200));
			sprintf((char*)&g_menuTags[HIGH_SENSITIVITY_MAX_TAG].text, " (%.2fmm)", 
					(double)((float)g_factorySetupRecord.seismicSensorType * (float)25.4 / (float)400));
		}

		SETUP_USER_MENU_MSG(&sensitivityMenu, g_triggerRecord.srec.sensitivity);
	}
	else if (keyPressed == ESC_KEY)
	{
		if ((g_triggerRecord.opMode == WAVEFORM_MODE) || (g_triggerRecord.opMode == COMBO_MODE))
		{
			SETUP_USER_MENU_FOR_FLOATS_MSG(&weightPerDelayMenu, &g_triggerRecord.trec.weight_per_delay,
				WEIGHT_PER_DELAY_DEFAULT_VALUE, WEIGHT_PER_DELAY_INCREMENT_VALUE,
				WEIGHT_PER_DELAY_MIN_VALUE, WEIGHT_PER_DELAY_MAX_VALUE);
		}
		else if (g_triggerRecord.opMode == BARGRAPH_MODE)
		{
			SETUP_USER_MENU_FOR_FLOATS_MSG(&distanceToSourceMenu, &g_triggerRecord.trec.dist_to_source,
				DISTANCE_TO_SOURCE_DEFAULT_VALUE, DISTANCE_TO_SOURCE_INCREMENT_VALUE,
				DISTANCE_TO_SOURCE_MIN_VALUE, DISTANCE_TO_SOURCE_MAX_VALUE);
		}
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Percent Of Limit Trigger
//=============================================================================
//*****************************************************************************
#define PERCENT_OF_LIMIT_TRIGGER_MENU_ENTRIES 4
USER_MENU_STRUCT percentLimitTriggerMenu[PERCENT_OF_LIMIT_TRIGGER_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, PERCENT_OF_LIMIT_TRIGGER_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(INTEGER_BYTE_TYPE, PERCENT_OF_LIMIT_TRIGGER_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_2)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {INSERT_USER_MENU_WORD_DATA(PERCENT_TYPE, NO_ALT_TYPE)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&PercentOfLimitTriggerMenuHandler}}
};

//-----------------------------------
// Percent Of Limit Trigger Menu Handler
//-----------------------------------
void PercentOfLimitTriggerMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	float div = (float)(g_bitAccuracyMidpoint * g_sensorInfo.sensorAccuracy * ((g_triggerRecord.srec.sensitivity == LOW) ? 2 : 4)) / (float)(g_factorySetupRecord.seismicSensorType);

	if (keyPressed == ENTER_KEY)
	{
		g_triggerRecord.trec.variableTriggerPercentageLevel = *((uint8*)data);
		debug("Percent of Limit Trigger: %d\r\n", g_triggerRecord.trec.variableTriggerPercentageLevel);

		g_triggerRecord.trec.variableTriggerEnable = YES;

		if (g_triggerRecord.trec.variableTriggerVibrationStandard == CUSTOM_STEP_THRESHOLD)
		{
			// Set the fixed trigger level to 1 IPS for this custom curve
			g_triggerRecord.trec.seismicTriggerLevel = (uint32)(1.00 * div);
		}
		else // All the other standards; USBM (Drywall and Plaster), OSM, and the CUSTOM_STEP_LIMITING
		{
			// Set the fixed trigger level to 2 IPS since anything above this level is an automatic trigger for all the other vibration standards
			g_triggerRecord.trec.seismicTriggerLevel = (uint32)(2.00 * div);
		}

		// Up convert to 16-bit since user selected level is based on selected bit accuracy
		g_triggerRecord.trec.seismicTriggerLevel *= (SEISMIC_TRIGGER_MAX_VALUE / g_bitAccuracyMidpoint);

		// Factor in % of Limit choice at 16-bit for better accuracy
		g_triggerRecord.trec.seismicTriggerLevel = (uint32)(g_triggerRecord.trec.seismicTriggerLevel * (float)((float)g_triggerRecord.trec.variableTriggerPercentageLevel / (float)100));

		debug("Seismic Trigger: %d counts\r\n", g_triggerRecord.trec.seismicTriggerLevel);

#if 0 /* Test */
		float unitsDiv = (float)(g_bitAccuracyMidpoint * SENSOR_ACCURACY_100X_SHIFT * ((g_triggerRecord.srec.sensitivity == LOW) ? 2 : 4)) / (float)(g_factorySetupRecord.seismicSensorType);
		float tempSesmicTriggerInUnits = (float)(g_triggerRecord.trec.seismicTriggerLevel >> g_bitShiftForAccuracy) / (float)unitsDiv;
		sprintf((char*)g_spareBuffer, "New VT Seismic trigger fixed level: 0x%04x of 0x8000, STIU: %f\r\n", (uint16)g_triggerRecord.trec.seismicTriggerLevel, tempSesmicTriggerInUnits); ModemPuts(g_spareBuffer, strlen((char*)g_spareBuffer), NO_CONVERSION);
#endif
		g_tempTriggerLevelForMenuAdjustment = AirTriggerConvertToUnits(g_triggerRecord.trec.airTriggerLevel);
		SETUP_USER_MENU_FOR_INTEGERS_MSG(&airTriggerMenu, &g_tempTriggerLevelForMenuAdjustment, GetAirDefaultValue(), GetAirMinValue(), GetAirMaxValue());
	}
	else if (keyPressed == ESC_KEY)
	{
		if (g_triggerRecord.trec.variableTriggerVibrationStandard < START_OF_CUSTOM_CURVES_LIST)
		{
			// Standard Vibration
			SETUP_USER_MENU_MSG(&vibrationStandardMenu, g_triggerRecord.trec.variableTriggerVibrationStandard);
		}
		else // Custom curve
		{
			SETUP_USER_MENU_MSG(&customCurveMenu, g_triggerRecord.trec.variableTriggerVibrationStandard);
		}
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Record Time Menu
//=============================================================================
//*****************************************************************************
#define RECORD_TIME_MENU_ENTRIES 4
USER_MENU_STRUCT recordTimeMenu[RECORD_TIME_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, RECORD_TIME_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(INTEGER_LONG_TYPE, RECORD_TIME_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_2)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {INSERT_USER_MENU_WORD_DATA(SECS_TYPE, NO_ALT_TYPE)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&RecordTimeMenuHandler}}
};

//-------------------------
// Record Time Menu Handler
//-------------------------
void RecordTimeMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	
	if (keyPressed == ENTER_KEY)
	{	
		g_triggerRecord.trec.record_time = *((uint32*)data);
		debug("Record Time: %d\r\n", g_triggerRecord.trec.record_time);

		if ((g_unitConfig.alarmOneMode) || (g_unitConfig.alarmTwoMode))
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
		// Check if the A-weighting option is enabled
		if ((!g_factorySetupRecord.invalid) && (g_factorySetupRecord.aWeightOption == ENABLED))
		{
			SETUP_USER_MENU_MSG(&airScaleMenu, g_unitConfig.airScale);
		}
		else
		{
			g_tempTriggerLevelForMenuAdjustment = AirTriggerConvertToUnits(g_triggerRecord.trec.airTriggerLevel);
			SETUP_USER_MENU_FOR_INTEGERS_MSG(&airTriggerMenu, &g_tempTriggerLevelForMenuAdjustment, GetAirDefaultValue(), GetAirMinValue(), GetAirMaxValue());
		}
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Save Record Menu
//=============================================================================
//*****************************************************************************
#define SAVE_RECORD_MENU_ENTRIES 4
#define MAX_SAVE_RECORD_CHARS 8
USER_MENU_STRUCT saveRecordMenu[SAVE_RECORD_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, NAME_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(STRING_TYPE, SAVE_RECORD_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_2)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {MAX_SAVE_RECORD_CHARS}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&SaveRecordMenuHandler}}
};

//-------------------------
// Save Record Menu Handler
//-------------------------
void SaveRecordMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	uint8 availableLocation = 0;
	uint8 choice;
	uint8 match = NO;
	
	if (keyPressed == ENTER_KEY)
	{	
		if (strlen((char*)data) != 0)
		{
			debug("Save Record Name: <%s>\r\n", (char*)data);
			availableLocation = CheckForAvailableTriggerRecordEntry((char*)data, &match);

			if (match == YES)
			{
				// Found another saved record with the same name

				// Prompt to alert the user the name has been used already
				OverlayMessage(getLangText(WARNING_TEXT), getLangText(NAME_ALREADY_USED_TEXT), 2 * SOFT_SECS);
				
				// Ask if they want to overwrite the saved setup
				choice = MessageBox(getLangText(WARNING_TEXT), getLangText(OVERWRITE_SETTINGS_TEXT), MB_YESNO);
				
				// Check if user selected YES
				if (choice == MB_FIRST_CHOICE)
				{
					// Copy over the record name
					strcpy((char*)g_triggerRecord.name, (char*)data);

					// Save the trigger record in the available location
					SaveRecordData(&g_triggerRecord, availableLocation, REC_TRIGGER_USER_MENU_TYPE);

					// Also save the trigger record in the default record for future use
					SaveRecordData(&g_triggerRecord, DEFAULT_RECORD, REC_TRIGGER_USER_MENU_TYPE);

					// Update the title based on the current mode
					UpdateModeMenuTitle(g_triggerRecord.opMode);
					SETUP_USER_MENU_MSG(&modeMenu, MONITOR);
				}
				else
				{
					// Let the code recall the current menu
					// If that doesn't work, then uncomment the following line to call the menu from start
					//SETUP_USER_MENU_MSG(&saveRecordMenu, (uint8)NULL);
				}
			}
			else if (availableLocation == 0)
			{
				// Prompt to alert the user that the saved settings are full
				sprintf((char*)g_spareBuffer, "%s %s", getLangText(SAVED_SETTINGS_TEXT), getLangText(FULL_TEXT));
				OverlayMessage(getLangText(WARNING_TEXT), (char*)g_spareBuffer, 2 * SOFT_SECS);

				// Copy over the record name
				strcpy((char*)g_triggerRecord.name, (char*)data);

				// Out of empty record slots, goto the Overwrite menu
				SETUP_MENU_WITH_DATA_MSG(OVERWRITE_MENU, g_triggerRecord.opMode);
			}
			else // Found an empty record slot
			{
				// Copy over the record name
				strcpy((char*)g_triggerRecord.name, (char*)data);

				// Save the trigger record in the available location
				SaveRecordData(&g_triggerRecord, availableLocation, REC_TRIGGER_USER_MENU_TYPE);

				// Also save the trigger record in the default record for future use
				SaveRecordData(&g_triggerRecord, DEFAULT_RECORD, REC_TRIGGER_USER_MENU_TYPE);

				// Update the title based on the current mode
				UpdateModeMenuTitle(g_triggerRecord.opMode);
				SETUP_USER_MENU_MSG(&modeMenu, MONITOR);
			}
		}
		else // Name was empty, don't allow it to be saved
		{
			MessageBox(getLangText(WARNING_TEXT), getLangText(NAME_MUST_HAVE_AT_LEAST_ONE_CHARACTER_TEXT), MB_OK);
			
			// Let the code recall the current menu
			// If that doesn't work, then uncomment the following line to call the menu from start
			//SETUP_USER_MENU_MSG(&saveRecordMenu, (uint8)NULL);
		}
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&saveSetupMenu, YES);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Seismic Location Menu
//=============================================================================
//*****************************************************************************
#define SEISMIC_LOCATION_MENU_ENTRIES 5
#define MAX_SEISMIC_LOCATION_CHARS 30
USER_MENU_STRUCT seismicLocationMenu[SEISMIC_LOCATION_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, SEIS_LOCATION_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(STRING_TYPE, SEISMIC_LOCATION_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_2)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {MAX_SEISMIC_LOCATION_CHARS}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&SeismicLocationMenuHandler}}
};

//------------------------------
// Seismic Location Menu Handler
//------------------------------
void SeismicLocationMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	
	if (keyPressed == ENTER_KEY)
	{	
		strcpy((char*)(&g_triggerRecord.trec.loc), (char*)data);
		debug("Seismic Location: <%s>, Length: %d\r\n", g_triggerRecord.trec.loc, strlen((char*)g_triggerRecord.trec.loc));

		SETUP_USER_MENU_MSG(&notesMenu, &g_triggerRecord.trec.comments);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&companyMenu, &g_triggerRecord.trec.client);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Seismic Trigger Level Menu
//=============================================================================
//*****************************************************************************
#define SEISMIC_TRIGGER_MENU_ENTRIES 5
USER_MENU_STRUCT seismicTriggerMenu[SEISMIC_TRIGGER_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, SEISMIC_TRIGGER_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(INTEGER_COUNT_TYPE, SEISMIC_TRIGGER_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_3)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {INSERT_USER_MENU_WORD_DATA(IN_TYPE, MM_TYPE)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&SeismicTriggerMenuHandler}}
};

//-----------------------------
// Seismic Trigger Menu Handler
//-----------------------------
void SeismicTriggerMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	
	if (keyPressed == ENTER_KEY)
	{	
		g_triggerRecord.trec.seismicTriggerLevel = *((uint32*)data);

		if (g_triggerRecord.trec.seismicTriggerLevel == NO_TRIGGER_CHAR)
		{
			debug("Seismic Trigger: No Trigger\r\n");
		}
		else
		{
			// Up convert to 16-bit since user selected level is based on selected bit accuracy
			g_triggerRecord.trec.seismicTriggerLevel *= (SEISMIC_TRIGGER_MAX_VALUE / g_bitAccuracyMidpoint);

			debug("Seismic Trigger: %d counts\r\n", g_triggerRecord.trec.seismicTriggerLevel);
		}

		g_tempTriggerLevelForMenuAdjustment = AirTriggerConvertToUnits(g_triggerRecord.trec.airTriggerLevel);
		SETUP_USER_MENU_FOR_INTEGERS_MSG(&airTriggerMenu, &g_tempTriggerLevelForMenuAdjustment, GetAirDefaultValue(), GetAirMinValue(), GetAirMaxValue());
	}
	else if (keyPressed == ESC_KEY)
	{
#if VT_FEATURE_DISABLED // Original
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
#else /* New Variable Trigger feature */
		SETUP_USER_MENU_MSG(&seismicTriggerTypeMenu, g_triggerRecord.trec.variableTriggerEnable);
#endif
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Serial Number Menu
//=============================================================================
//*****************************************************************************
#define SERIAL_NUMBER_MENU_ENTRIES 4
#define MAX_SERIAL_NUMBER_CHARS 15
USER_MENU_STRUCT serialNumberMenu[SERIAL_NUMBER_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, SERIAL_NUMBER_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(STRING_SPECIAL_TYPE, SERIAL_NUMBER_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_2)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {MAX_SERIAL_NUMBER_CHARS}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&SerialNumberMenuHandler}}
};

//---------------------------
// Serial Number Menu Handler
//---------------------------
void SerialNumberMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	
	if (keyPressed == ENTER_KEY)
	{	
		debug("Serial #: <%s>, Length: %d\r\n", (char*)data, strlen((char*)data));
		strcpy((char*)g_factorySetupRecord.unitSerialNumber, (char*)data);

#if 0 /* Original */
		// Re-read and display Smart Sensor info
		DisplaySmartSensorInfo(INFO_ON_CHECK);

		SETUP_USER_MENU_MSG(&seismicSensorTypeMenu, g_factorySetupRecord.seismicSensorType);
#else /* Add Hardware ID */
		SETUP_USER_MENU_MSG(&hardwareIDMenu, g_factorySetupRecord.hardwareID);
#endif
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_MENU_MSG(DATE_TIME_MENU);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Stored Event Limit Menu
//=============================================================================
//*****************************************************************************
#define STORED_EVENT_LIMIT_MENU_ENTRIES 4
USER_MENU_STRUCT storedEventLimitMenu[STORED_EVENT_LIMIT_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, MAX_EVTS_TO_KEEP_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(INTEGER_WORD_TYPE, STORED_EVENT_LIMIT_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_2)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {INSERT_USER_MENU_WORD_DATA(NO_TYPE, NO_ALT_TYPE)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&StoredEventLimitMenuHandler}}
};

//--------------------------------
// Stored Event Limit Menu Handler
//--------------------------------
void StoredEventLimitMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};

	if (keyPressed == ENTER_KEY)
	{
		g_unitConfig.storedEventsCapMode = ENABLED;
		g_unitConfig.storedEventLimit = *((uint16*)data);

		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		if (g_summaryList.validEntries > g_unitConfig.storedEventLimit)
		{
			sprintf((char*)g_spareBuffer, "%s. %s", getLangText(STORED_EVTS_MORE_THAN_MAX_TEXT), getLangText(DELETE_OLD_EVTS_NOW_Q_TEXT));
			if (MessageBox(getLangText(WARNING_TEXT), (char*)g_spareBuffer, MB_YESNO) == MB_FIRST_CHOICE)
			{
				// Remove excess events above max now
				RemoveExcessEventsAboveCap();
			}
		}

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&storedEventsCapModeMenu, g_unitConfig.storedEventsCapMode);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Weight per Delay Menu
//=============================================================================
//*****************************************************************************
#define WEIGHT_PER_DELAY_MENU_ENTRIES 4
USER_MENU_STRUCT weightPerDelayMenu[WEIGHT_PER_DELAY_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, WEIGHT_PER_DELAY_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(FLOAT_WITH_N_TYPE, WEIGHT_PER_DELAY_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_2)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {INSERT_USER_MENU_WORD_DATA(LBS_TYPE, KG_TYPE)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&WeightPerDelayMenuHandler}}
};

//------------------------------
// Weight per Delay Menu Handler
//------------------------------
void WeightPerDelayMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	
	if (keyPressed == ENTER_KEY)
	{	
		g_triggerRecord.trec.weight_per_delay = *((float*)data);

		if (g_unitConfig.unitsOfMeasure == METRIC_TYPE)
			g_triggerRecord.trec.weight_per_delay *= LBS_PER_KG;

		debug("Weight per Delay: %.1f lbs\r\n", (double)g_triggerRecord.trec.weight_per_delay);

		SETUP_USER_MENU_MSG(&operatorMenu, &g_triggerRecord.trec.oper);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_FOR_FLOATS_MSG(&distanceToSourceMenu, &g_triggerRecord.trec.dist_to_source,
			DISTANCE_TO_SOURCE_DEFAULT_VALUE, DISTANCE_TO_SOURCE_INCREMENT_VALUE,
			DISTANCE_TO_SOURCE_MIN_VALUE, DISTANCE_TO_SOURCE_MAX_VALUE);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// Unlock Code Menu
//=============================================================================
//*****************************************************************************
#define UNLOCK_CODE_MENU_ENTRIES 4
USER_MENU_STRUCT unlockCodeMenu[UNLOCK_CODE_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, UNLOCK_CODE_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(INTEGER_WORD_FIXED_TYPE, UNLOCK_CODE_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_2)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {INSERT_USER_MENU_WORD_DATA(NO_TYPE, NO_ALT_TYPE)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&UnlockCodeMenuHandler}}
};

//-------------------------
// Unlock Code Menu Handler
//-------------------------
void UnlockCodeMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};
	
	if (keyPressed == ENTER_KEY)
	{	
		g_modemSetupRecord.unlockCode = *((uint16*)data);

		debug("Modem Unlock Code: %4d\r\n", g_modemSetupRecord.unlockCode);

		SaveRecordData(&g_modemSetupRecord, DEFAULT_RECORD, REC_MODEM_SETUP_TYPE);

		if (READ_DCD == NO_CONNECTION)
		{
			CraftInitStatusFlags();
		}

		if (g_modemSetupRecord.dialOutType == AUTODIALOUT_EVENTS_CONFIG_STATUS)
		{
			AssignSoftTimer(AUTO_DIAL_OUT_CYCLE_TIMER_NUM, (uint32)(g_modemSetupRecord.dialOutCycleTime * TICKS_PER_MIN), AutoDialOutCycleTimerCallBack);
		}

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_FOR_INTEGERS_MSG(&modemRetryTimeMenu, &g_modemSetupRecord.retryTime,
			MODEM_RETRY_TIME_DEFAULT_VALUE, MODEM_RETRY_TIME_MIN_VALUE, MODEM_RETRY_TIME_MAX_VALUE);
	}

	JUMP_TO_ACTIVE_MENU();
}

//*****************************************************************************
//=============================================================================
// UTC Offset Menu
//=============================================================================
//*****************************************************************************
#define UTC_ZONE_OFFSET_MENU_ENTRIES 4
USER_MENU_STRUCT utcZoneOffsetMenu[UTC_ZONE_OFFSET_MENU_ENTRIES] = {
{TITLE_PRE_TAG, 0, UTC_ZONE_OFFSET_TEXT, TITLE_POST_TAG,
	{INSERT_USER_MENU_INFO(INTEGER_BYTE_OFFSET_TYPE, UTC_ZONE_OFFSET_MENU_ENTRIES, TITLE_CENTERED, DEFAULT_ROW_2)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {INSERT_USER_MENU_WORD_DATA(NO_TYPE, NO_ALT_TYPE)}},
{NO_TAG, 0, NULL_TEXT, NO_TAG, {}},
{END_OF_MENU, (uint16_t)BACKLIGHT_KEY, (uint16_t)HELP_KEY, (uint16_t)ESC_KEY, {(uint32)&UtcZoneOffsetMenuHandler}}
};

//------------------------
// UTC Offset Menu Handler
//------------------------
void UtcZoneOffsetMenuHandler(uint8 keyPressed, void* data)
{
	INPUT_MSG_STRUCT mn_msg = {0, 0, {}};

	if (keyPressed == ENTER_KEY)
	{
		g_unitConfig.utcZoneOffset = (int8)(*((uint8*)data) - 12);

		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		SETUP_USER_MENU_MSG(&configMenu, DEFAULT_ITEM_1);
	}
	else if (keyPressed == ESC_KEY)
	{
		SETUP_USER_MENU_MSG(&configMenu, UTC_ZONE_OFFSET);
	}

	JUMP_TO_ACTIVE_MENU();
}
