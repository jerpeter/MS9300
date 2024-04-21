///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

#ifndef _MENU_H_
#define _MENU_H_ 

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include "Record.h"
#include "RealTimeClock.h"
#include "SoftTimer.h"
 
///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
/**** MENU MSG COMMANDS ****/
enum {
	ACTIVATE_MENU_CMD = 1,			// 1
	ACTIVATE_MENU_WITH_DATA_CMD,	// 2
	KEYPRESS_MENU_CMD, 				// 3
	MAIN_MENU_STARTUP_CMD, 			// 4
	ALARM_MENU_SLEEP_CMD, 			// 5
	STOP_MONITORING_CMD, 			// 6
	SENSOR_CK_ITEM_CMD, 			// 7
	SENSOR_CK_END_CMD, 				// 8
	START_AT_BOTTOM_OF_MENU_CMD,	// 9
	START_AT_TOP_OF_MENU_CMD, 		// 10
	START_AT_CURRENT_LINE_MENU_CMD,	// 11
	PAPER_FEED_CMD, 				// 12
	BACK_LIGHT_CMD, 				// 13
	POWER_OFF_CMD, 					// 14
	CTRL_CMD,						// 15
	// Add new commands before this line
	TOTAL_MENU_MSG_COMMANDS
};

// Menu Seperators
enum {
	NO_TAG = 0,
	ITEM_1,
	ITEM_2,
	ITEM_3,
	ITEM_4,
	ITEM_5,
	ITEM_6,
	ITEM_7,
	ITEM_8,
	ITEM_9,
	MAIN_PRE_TAG,
	MAIN_POST_TAG,
	TITLE_PRE_TAG,
	TITLE_POST_TAG,
	LOW_SENSITIVITY_MAX_TAG,
	HIGH_SENSITIVITY_MAX_TAG,
	BAR_SCALE_FULL_TAG,
	BAR_SCALE_HALF_TAG,
	BAR_SCALE_QUARTER_TAG,
	BAR_SCALE_EIGHTH_TAG,
	ENABLED_TAG,
	DISABLED_TAG,
	FILENAME_TAG,
	// Add new separators before this line
	TOTAL_TAGS
};

// User Select Default Item types
enum {
	DEFAULT_ITEM_1 = 1,
	DEFAULT_ITEM_2,
	DEFAULT_ITEM_3,
	DEFAULT_ITEM_4,
	DEFAULT_ITEM_5,
	DEFAULT_ITEM_6,
	DEFAULT_ITEM_7,
	DEFAULT_ITEM_8,
	DEFAULT_ITEM_9
};

// User Input Default Row types
enum {
	DEFAULT_ROW_1 = 1,
	DEFAULT_ROW_2,
	DEFAULT_ROW_3,
	DEFAULT_ROW_4,
	DEFAULT_ROW_5,
	DEFAULT_ROW_6,
	DEFAULT_ROW_7,
	DEFAULT_ROW_8,
	DEFAULT_ROW_9
};

// User Menu Specific Defines for Indicies
#define CURRENT_TEXT_INDEX		2
#define MAX_TEXT_CHARS			1
#define MENU_INFO				0
#define MENU_TYPE				3 // Was 0 originally on Big Endian
#define TOTAL_MENU_ITEMS		2 // Was 1 originally on Big Endian
#define MENU_TITLE_POSITION		1 // Was 2 originally on Big Endian
#define DEFAULT_ITEM			0 // Was 3 originally on Big Endian
#define DEFAULT_ROW				0 // Was 3 originally on Big Endian
#define INTEGER_RANGE			1
#define FLOAT_RANGE				1
#define FLOAT_INCREMENT			2
#define UNIT_TYPE				1
#define DEFAULT_TYPE			1 // Was 0 originally on Big Endian
#define ALT_TYPE				0 // Was 1 originally on Big Endian

// User Menu message indicies
#define CURRENT_USER_MENU		0
#define CURRENT_ITEM_VALUE		1
#define CURRENT_DATA_POINTER	1
#define CURRENT_DATA_SIZE		1
#define USER_MENU_ENTER_HANDLER	2
#define USER_MENU_ESC_HANDLER	3

// User Menu Macros
#define USER_MENU_TITLE(menu)			menu[MENU_INFO].text
#define USER_MENU_TYPE(menu)			menu[MENU_INFO].byteData[MENU_TYPE]
#define USER_MENU_TOTAL_ITEMS(menu)		menu[MENU_INFO].byteData[TOTAL_MENU_ITEMS]
#define USER_MENU_TITLE_POSITION(menu)	menu[MENU_INFO].byteData[MENU_TITLE_POSITION]
#define USER_MENU_DEFAULT_ITEM(menu)	menu[MENU_INFO].byteData[DEFAULT_ITEM]
#define USER_MENU_DEFAULT_ROW(menu)		menu[MENU_INFO].byteData[DEFAULT_ROW]
#define USER_MENU_DEFAULT_TYPE(menu)	menu[UNIT_TYPE].wordData[DEFAULT_TYPE]
#define USER_MENU_ALT_TYPE(menu)		menu[UNIT_TYPE].wordData[ALT_TYPE]
#define USER_MENU_DISPLAY_ITEMS(menu)	(menu[MENU_INFO].byteData[TOTAL_MENU_ITEMS] - 1)
#define USER_MENU_ACTIVE_ITEMS(menu)	(menu[MENU_INFO].byteData[TOTAL_MENU_ITEMS] - 2)

// Max number of menu entries for any single menu. The config menu currently has the most. 
#define MAX_MENU_ENTRIES	40

// User Menu types
enum {
	SELECT_TYPE = 1,
	SELECT_SPECIAL_TYPE,
	STRING_TYPE,
	STRING_SPECIAL_TYPE,
	INTEGER_BYTE_TYPE,
	INTEGER_BYTE_OFFSET_TYPE,
	INTEGER_WORD_TYPE,
	INTEGER_WORD_FIXED_TYPE,
	INTEGER_WORD_OFFSET_TYPE,
	INTEGER_LONG_TYPE,
	INTEGER_SPECIAL_TYPE,
	INTEGER_COUNT_TYPE,
	FLOAT_TYPE,
	FLOAT_SPECIAL_TYPE,
	FLOAT_WITH_N_TYPE,
	DATE_TIME_TYPE
};

// User Menu Unit Types
enum {
	NO_TYPE = 0,
	NO_ALT_TYPE,
	IN_TYPE,
	MM_TYPE,
	FT_TYPE,
	M_TYPE,
	LBS_TYPE,
	KG_TYPE,
	DB_TYPE,
	DBA_TYPE,
	MB_TYPE,
	SECS_TYPE,
	MINS_TYPE,
	MG_TYPE,
	HOUR_TYPE,
	PERCENT_TYPE,
	PSI_UNIT_TYPE,
	// Add types before this line
	TOTAL_TYPES
};

// User Menu Input Special number boundary cases
enum {
	NO_BOUNDARY = 0,
	TOP_BOUNDARY,
	BOTTOM_BOUNDARY
};

// Menu Scroll Direction
enum {
	UP = 0,
	DOWN
};

// Message display stuff
enum {
	NO_PROMPT,
	PROMPT,
	OVERLAY
};

// Unit operational Modes
enum {
	WAVEFORM_MODE = 10, 
	BARGRAPH_MODE, 			// 11 
	COMBO_MODE, 			// 12
	MANUAL_CAL_MODE, 		// 13
	MANUAL_TRIGGER_MODE		// 14
}; // UNIT_OPERATION_MODES

// Air Trigger stuff
#define AIR_TRIGGER_DEFAULT_VALUE			NO_TRIGGER_CHAR
#define AIR_TRIGGER_MIN_COUNT				64
#define AIR_TRIGGER_MAX_COUNT				0x8000

#if 1 /* Air trigger adheres to the general design all triggers start at 64 counts */
//---------------------------------------------------------------------------------------------------------------------
//--- Air Trigger Min/Max levels based on 64 A/D counts, adhering to 8100 design of 64/16/4 as minimum based on bit accuracy, except for the special case standard 148 dB mic and using a min of 92 dB (51 counts)
//---------------------------------------------------------------------------------------------------------------------
#if 1 /* Special case for only the standard 148 dB mic, using 92 dB (51 counts) as the min trigger level */
#define AIR_TRIGGER_MIC_148_DB_MIN_VALUE		92 // @ 51 counts = 92.00 (Special case allowing 92 dB as the minimum for the standard 148 dB mic)
#define AIR_TRIGGER_MIC_148_DB_MAX_VALUE		148 // 148.16, rounding down (below the cap/max level)
#define AIR_TRIGGER_MIC_148_DB_IN_MB_MIN_VALUE	80 // 0.00796875 * 10000 = 79.6875, rounding up (above the floor)
#define AIR_TRIGGER_MIC_148_DB_IN_MB_MAX_VALUE	51200 // 5.12 * 10000
#define AIR_TRIGGER_MIC_148_DB_IN_PSI_MIN_VALUE	2 // 0.00796875 * 10000 / 68.947572 = 1.15577, rounding up (above the floor)
#define AIR_TRIGGER_MIC_148_DB_IN_PSI_MAX_VALUE	742 // 5.12 * 10000 / 68.947572 = 742.593, rounding down (below the cap/max level)
#else /* Normal design based on 64 counts for standard 148 dB mic */
#define AIR_TRIGGER_MIC_148_DB_MIN_VALUE		94 // @ 64 counts = 93.97, rounding up (above the floor)
#define AIR_TRIGGER_MIC_148_DB_MAX_VALUE		148 // 148.16, rounding down (below the cap/max level)
#define AIR_TRIGGER_MIC_148_DB_IN_MB_MIN_VALUE	100 // 0.01 * 10000
#define AIR_TRIGGER_MIC_148_DB_IN_MB_MAX_VALUE	51200 // 5.12 * 10000
#define AIR_TRIGGER_MIC_148_DB_IN_PSI_MIN_VALUE	2 // 0.01 * 10000 / 68.947572 = 1.4503
#define AIR_TRIGGER_MIC_148_DB_IN_PSI_MAX_VALUE	742.59322 // 5.12 * 10000 / 68.947572
#endif

#define AIR_TRIGGER_MIC_160_DB_MIN_VALUE		106 // @ 64 counts = 106.0
#define AIR_TRIGGER_MIC_160_DB_MAX_VALUE		160 // 160.20, rounding down (below the cap/max level)
#define AIR_TRIGGER_MIC_160_DB_IN_MB_MIN_VALUE	400 // 0.04 * 10000
#define AIR_TRIGGER_MIC_160_DB_IN_MB_MAX_VALUE	204800 // 20.48 * 10000
#define AIR_TRIGGER_MIC_160_DB_IN_PSI_MIN_VALUE	6 // 0.04 * 10000 / 68.947572 = 5.8015, rounding up (above the floor)
#define AIR_TRIGGER_MIC_160_DB_IN_PSI_MAX_VALUE	2970 // 20.48 * 10000 / 68.947572 = 2970.372, rounding down (below the cap/max level)

#define AIR_TRIGGER_MIC_5_PSI_IN_DB_MIN_VALUE	131 // @ 64 counts = 130.5, rounding up (above the floor)
#define AIR_TRIGGER_MIC_5_PSI_IN_DB_MAX_VALUE	184 // 5 PSI to dB is 184.7291580458, rounding down (below the cap/max level)
#define AIR_TRIGGER_MIC_5_PSI_IN_MB_MIN_VALUE	6734 // 0.67332 * 10000 = 6733.2, rounding up (above the floor)
#define AIR_TRIGGER_MIC_5_PSI_IN_MB_MAX_VALUE	3447378 // 344.73786466 * 10000 = 3447378.6466, rounding down (bloew the cap/max level)
#define AIR_TRIGGER_MIC_5_PSI_MENU_MIN_VALUE	97 // 0.67332 * 10000 / 68.947572 = 97.656, rounding up (above the floor)
#define AIR_TRIGGER_MIC_5_PSI_MENU_MAX_VALUE	50000 // 344.73786466 * 10000 / 68.947572 = 50000

#define AIR_TRIGGER_MIC_10_PSI_IN_DB_MIN_VALUE	137 // @ 64 counts = 136.5, rounding up (above the floor)
#define AIR_TRIGGER_MIC_10_PSI_IN_DB_MAX_VALUE	190 // 10 PSI to dB is 190.7497797447, rounding down (bloew the cap/max level)
#define AIR_TRIGGER_MIC_10_PSI_IN_MB_MIN_VALUE	13467 // 1.34663 * 10000 = 13466.3, rounding up (above the floor)
#define AIR_TRIGGER_MIC_10_PSI_IN_MB_MAX_VALUE	6894757 // 689.47572932 * 10000 = 6894757.2932, rounding down (below the cap/max level)
#define AIR_TRIGGER_MIC_10_PSI_MENU_MIN_VALUE	196 // 1.34663 * 10000 / 68.947572 = 195.312, rounding up (above the floor)
#define AIR_TRIGGER_MIC_10_PSI_MENU_MAX_VALUE	100000 // 689.47572932 * 10000 / 68.947572 = 100000

#else /* Air trigger minimum adheres relative to standard 148 dB mic @ 92 dB */
//---------------------------------------------------------------------------------------------------------------------
//--- Air Trigger Min/Max levels based on ~51 (48 for PSI) A/D counts, adhering to 7K series legacy 92 dB lower limit
//---------------------------------------------------------------------------------------------------------------------
#define AIR_TRIGGER_MIC_148_DB_MIN_VALUE		92 // @ 51 counts
#define AIR_TRIGGER_MIC_148_DB_MAX_VALUE		148 // Max
#define AIR_TRIGGER_MIC_148_DB_IN_MB_MIN_VALUE	80 // 0.00796875 * 10000
#define AIR_TRIGGER_MIC_148_DB_IN_MB_MAX_VALUE	51200 // 5.12 * 10000
#define AIR_TRIGGER_MIC_148_DB_IN_PSI_MIN_VALUE	2 // 0.00796875 * 10000 / 68.947572 = 1.15577
#define AIR_TRIGGER_MIC_148_DB_IN_PSI_MAX_VALUE	742.59322 // 5.12 * 10000 / 68.947572

#define AIR_TRIGGER_MIC_160_DB_MIN_VALUE		104 // @ 51 counts
#define AIR_TRIGGER_MIC_160_DB_MAX_VALUE		160 // Max
#define AIR_TRIGGER_MIC_160_DB_IN_MB_MIN_VALUE	318 // 0.03188 * 10000 = 318.8
#define AIR_TRIGGER_MIC_160_DB_IN_MB_MAX_VALUE	204800 // 20.48 * 10000
#define AIR_TRIGGER_MIC_160_DB_IN_PSI_MIN_VALUE	5 // 0.03188 * 10000 / 68.947572 = 4.6238
#define AIR_TRIGGER_MIC_160_DB_IN_PSI_MAX_VALUE	2970.3728 // 20.48 * 10000 / 68.947572

#define AIR_TRIGGER_MIC_5_PSI_IN_DB_MIN_VALUE	128 // @ 48 counts
#define AIR_TRIGGER_MIC_5_PSI_IN_DB_MAX_VALUE	184 // 5 PSI to dB is 184.7291580458, but truncating for whole number
#define AIR_TRIGGER_MIC_5_PSI_IN_MB_MIN_VALUE	5050 // 0.50499 * 10000
#define AIR_TRIGGER_MIC_5_PSI_IN_MB_MAX_VALUE	3447378 // 344.73786466 * 10000
#define AIR_TRIGGER_MIC_5_PSI_MENU_MIN_VALUE	73 // 0.50499 * 10000 / 68.947572 = 73.242
#define AIR_TRIGGER_MIC_5_PSI_MENU_MAX_VALUE	50000 // 344.73786466 * 10000 / 68.947572

#define AIR_TRIGGER_MIC_10_PSI_IN_DB_MIN_VALUE	134 // @ 48 counts
#define AIR_TRIGGER_MIC_10_PSI_IN_DB_MAX_VALUE	190 // 10 PSI to dB is 190.7497797447, but truncating for whole number
#define AIR_TRIGGER_MIC_10_PSI_IN_MB_MIN_VALUE	10100 // 1.00997 * 10000
#define AIR_TRIGGER_MIC_10_PSI_IN_MB_MAX_VALUE	6894757 // 689.47572932 * 10000
#define AIR_TRIGGER_MIC_10_PSI_MENU_MIN_VALUE	146 // 1.00997 * 10000 / 68.947572 = 146.483
#define AIR_TRIGGER_MIC_10_PSI_MENU_MAX_VALUE	100000 // 689.47572932 * 10000 / 68.947572
#endif

#if 1 /* Air minimum trigger adheres to the general design that all triggers start at 64 counts, minus the special case for the standard 148 dB mic (51 counts) */
#define AIR_TRIGGER_MIN_COUNT_REMOTE_CONFIG					64 // 64 counts = 94 dB which adheres to the 8100 general design
#define AIR_TRIGGER_MIN_COUNT_REMOTE_CONFIG_SPECIAL_92_DB	51 // 51 counts = 92 dB, special case for the standard 148 dB mic to mimic the min on the 7K series
#else /* All Air mic trigger minimums adhere relative to standard 148 dB mic @ 92 dB */
#define AIR_TRIGGER_MIN_COUNT_REMOTE_CONFIG	51 // 51 counts = 92 dB which equals the 7K series lowest trigger level
#endif

//---------------------------------------------------------------------------------------------------------------------
//--- Air Trigger increment levels
//---------------------------------------------------------------------------------------------------------------------
#define AIR_INCREMENT_MIC_148_DB_IN_MB		2 // 1.562
#define AIR_INCREMENT_MIC_160_DB_IN_MB		6 // 6.25
#define AIR_INCREMENT_MIC_5_PSI_IN_MB		105 // 105.205
#define AIR_INCREMENT_MIC_10_PSI_IN_MB		210 // 210.411

#define AIR_INCREMENT_MIC_148_DB_IN_PSI		1 // 1.562 / 68.9475 = 0.0226
#define AIR_INCREMENT_MIC_160_DB_IN_PSI		1 // 6.25 / 68.9475 = 0.0906
#define AIR_INCREMENT_MIC_5_PSI_IN_PSI		2 // 105.205 / 68.9475 = 1.5258
#define AIR_INCREMENT_MIC_10_PSI_IN_PSI		3 // 210.411 / 68.9475 = 3.0517

// Alarm modes
#define ALARM_MODE_OFF		0x00
#define ALARM_MODE_SEISMIC	0x01
#define ALARM_MODE_AIR		0x02
#define ALARM_MODE_BOTH		(ALARM_MODE_SEISMIC + ALARM_MODE_AIR)

// BLM Alarm types
#define ALERT_ALARM_ONE		0x01
#define ALERT_ALARM_TWO		0x02

// Alarm 1 & 2 defaults
#define ALARM_ONE_SEIS_DEFAULT_TRIG_LVL	16384	//1024 // (2048 * 2 / 4) 50%
#define ALARM_ONE_AIR_DEFAULT_TRIG_LVL	120
#define ALARM_TWO_SEIS_DEFAULT_TRIG_LVL	24576	//1536 // (2048 * 3 / 4) 75%
#define ALARM_TWO_AIR_DEFAULT_TRIG_LVL	140

#define ALARM_SEIS_MIN_VALUE			64		//3
#define ALARM_SEIS_MAX_VALUE			0x8000	//2048

// Alarm Times
#define ALARM_OUTPUT_TIME_DEFAULT	5 		// secs
#define ALARM_OUTPUT_TIME_MIN		0.5		// secs
#define ALARM_OUTPUT_TIME_MAX		60		// secs
#define ALARM_OUTPUT_TIME_INCREMENT	0.5 	// secs

// Auto Monitor stuff
#define AUTO_TWO_MIN_TIMEOUT 	2 	// minutes
#define AUTO_THREE_MIN_TIMEOUT	3 	// minutes
#define AUTO_FOUR_MIN_TIMEOUT 	4 	// minutes
#define AUTO_NO_TIMEOUT 		0 	// Disabled

// Auto Cal stuff
#define AUTO_24_HOUR_TIMEOUT 	24	// hours
#define AUTO_48_HOUR_TIMEOUT	48 	// hours
#define AUTO_72_HOUR_TIMEOUT 	72 	// hours
#define AUTO_NO_CAL_TIMEOUT 	0 	// Disabled

// (Unused currently although text equivalent is available)
#define NORMAL_FORMAT		1
#define SHORT_FORMAT		2

// Bar Scale stuff
#define BAR_SCALE_FULL		1
#define BAR_SCALE_HALF		2
#define BAR_SCALE_QUARTER	4
#define BAR_SCALE_EIGHTH	8

// Copies stuff
#define COPIES_DEFAULT_VALUE	1
#define COPIES_MIN_VALUE		1
#define COPIES_MAX_VALUE		9

// LCD Impulse Time stuff
#define LCD_IMPULSE_TIME_DEFAULT_VALUE	2
#define LCD_IMPULSE_TIME_MIN_VALUE		1
#define LCD_IMPULSE_TIME_MAX_VALUE		15

// LCD Timeout stuff
#define LCD_TIMEOUT_DEFAULT_VALUE	2
#define LCD_TIMEOUT_MIN_VALUE		2
#define LCD_TIMEOUT_MAX_VALUE		60

// Cycle End Time Hour (24hr)
#define CYCLE_END_TIME_HOUR_DEFAULT_VALUE	0
#define CYCLE_END_TIME_HOUR_MIN_VALUE		0
#define CYCLE_END_TIME_HOUR_MAX_VALUE		23

// Stored Event Limits
#define STORED_EVENT_LIMIT_DEFAULT_VALUE	1000
#define STORED_EVENT_LIMIT_MIN_VALUE		25
#define STORED_EVENT_LIMIT_MAX_VALUE		65500

// Variable Trigger % of Limit stuff
#define VT_PERCENT_OF_LIMIT_DEFAULT_VALUE	100
#define VT_PERCENT_OF_LIMIT_MIN_VALUE		10
#define VT_PERCENT_OF_LIMIT_MAX_VALUE		100

// Language stuff
enum {
	ENGLISH_LANG = 1,
	FRENCH_LANG,
	SPANISH_LANG,
	ITALIAN_LANG,
	GERMAN_LANG
};

// Monitor Log stuff
enum {
	VIEW_LOG = 1,
	PRINT_LOG,
	LOG_RESULTS
};

// Modem Retries stuff
#define MODEM_RETRY_DEFAULT_VALUE	2
#define MODEM_RETRY_MIN_VALUE		1
#define MODEM_RETRY_MAX_VALUE		9

// Modem Retry Time stuff
#define MODEM_RETRY_TIME_DEFAULT_VALUE	2
#define MODEM_RETRY_TIME_MIN_VALUE		1
#define MODEM_RETRY_TIME_MAX_VALUE		60

// Modem Dial Out Timer stuff
#define MODEM_DIAL_OUT_TIMER_DEFAULT_VALUE	60
#define MODEM_DIAL_OUT_TIMER_MIN_VALUE		1
#define MODEM_DIAL_OUT_TIMER_MAX_VALUE		1440

// Distance to Source stuff
#define DISTANCE_TO_SOURCE_DEFAULT_VALUE	0.0
#define DISTANCE_TO_SOURCE_INCREMENT_VALUE	1.0
#define DISTANCE_TO_SOURCE_MIN_VALUE		0.0
#define DISTANCE_TO_SOURCE_MAX_VALUE		99999.0

// Weight per Delay stuff
#define WEIGHT_PER_DELAY_DEFAULT_VALUE		0.0
#define WEIGHT_PER_DELAY_INCREMENT_VALUE	1.0
#define WEIGHT_PER_DELAY_MIN_VALUE			0.0
#define WEIGHT_PER_DELAY_MAX_VALUE			99999.0

// Record Time stuff
#define RECORD_TIME_DEFAULT_VALUE	3 	// secs
#define RECORD_TIME_MIN_VALUE		1 	// secs
#define RECORD_TIME_MAX_VALUE		90 	// secs

// Summary Menu stuff
#define START_FROM_TOP	1

// Timer mode freq types
enum {
	TIMER_MODE_ONE_TIME = 1, // Specific values matter for UCM/Dave's SG (1..5)
	TIMER_MODE_DAILY,		// 2
	TIMER_MODE_WEEKDAYS,	// 3
	TIMER_MODE_WEEKLY,		// 4
	TIMER_MODE_MONTHLY,		// 5
	TIMER_MODE_HOURLY		// 6
};

// Seismic Trigger stuff
#define SEISMIC_TRIGGER_DEFAULT_VALUE	192
#define SEISMIC_TRIGGER_INCREMENT		1
#define SEISMIC_TRIGGER_MIN_VALUE		64		//3
#define SEISMIC_TRIGGER_MAX_VALUE		0x8000	//0x0800
#define SEISMIC_TRIGGER_ADJUST_FILTER	(SEISMIC_TRIGGER_MIN_VALUE * 32)

// Unlock Code stuff
#define UNLOCK_CODE_DEFAULT_VALUE	0
#define UNLOCK_CODE_MIN_VALUE		0
#define UNLOCK_CODE_MAX_VALUE		9999

// Freq Plot Standards types
enum {
	FREQ_PLOT_US_BOM_STANDARD = 1,
	FREQ_PLOT_FRENCH_STANDARD,
	FREQ_PLOT_DIN_4150_STANDARD,
	FREQ_PLOT_BRITISH_7385_STANDARD,
	FREQ_PLOT_SPANISH_STANDARD
};

// Bar Channel types
enum {
	BAR_BOTH_CHANNELS,
	BAR_SEISMIC_CHANNEL,
	BAR_AIR_CHANNEL
};

// Bar Result types
enum {
	BAR_RESULT_PEAK = 0,
	BAR_RESULT_VECTOR_SUM
};

// Bargraph Result Mode types
enum {
	SUMMARY_INTERVAL_RESULTS,
	JOB_PEAK_RESULTS,
	IMPULSE_RESULTS
};

// Alarm testing types
enum {
	ALARM_TESTING_DONE = 1,
	ALARM_1_TESTING_ENABLED,
	ALARM_1_TESTING_DISABLED,
	ALARM_2_TESTING_ENABLED,
	ALARM_2_TESTING_DISABLED
};

// Mode Menu types
enum {
	MONITOR = 1,
	EDIT
};

// Help Menu types
enum {
	CONFIG_CHOICE = 1,
	INFORMATION_CHOICE,
	SENSOR_CHECK_CHOICE,
	GPS_LOCATION_DISPLAY_CHOICE,
	CHECK_SUMMARY_FILE_CHOICE,
	TESTING_CHOICE
};

// Config Menu types
enum {
	ADAPTIVE_SAMPLE_RATE = 1,
	ALARM_OUTPUT_MODE,
	AUTO_CALIBRATION,
	AUTO_DIAL_INFO,
	AUTO_MONITOR,
	ANALOG_CUTOFF_FREQ,
	BATTERY,
	BAUD_RATE,
	BAR_LIVE_MONITOR,
	CALIBRATION_DATE,
	CHANNEL_VERIFICATION,
	COPIES,
	CYCLE_END_TIME_HOUR,
	DATE_TIME,
	ERASE_FLASH,
	EVENT_SUMMARIES,
	EXTERNAL_TRIGGER,
	FLASH_WRAPPING,
	FLASH_STATS,
	FREQUENCY_PLOT,
#if 1 /* Test */
	GPS_POWER,
#endif
	LANGUAGE,
	LCD_CONTRAST,
	LCD_TIMEOUT,
	LEGACY_DQM_LIMIT,
	MODEM_SETUP,
	MONITOR_LOG,
	PRETRIGGER_SIZE,
	PRINTER,
	PRINT_MONITOR_LOG,
	REPORT_DISPLACEMENT,
	REPORT_PEAK_ACC,
	RS232_POWER_SAVINGS,
	SAVE_COMPRESSED_DATA,
	SENSOR_GAIN_TYPE,
	SERIAL_NUMBER,
	STORED_EVENTS_CAP_MODE,
	TIMER_MODE,
	UNITS_OF_MEASURE,
	UNITS_OF_AIR,
	USB_SYNC_MODE,
	UTC_ZONE_OFFSET,
	VECTOR_SUM,
	WAVEFORM_AUTO_CAL,
	ZERO_EVENT_NUMBER,
	// Add new typed before this line
	TOTAL_CONFIG_MENU_TYPES
};

enum {
	BAUD_RATE_115200 = 0,
	BAUD_RATE_57600,
	BAUD_RATE_38400,
	BAUD_RATE_19200,
	BAUD_RATE_9600
};

// Alternate Results
enum {
	DEFAULT_RESULTS = 1, // Show Air channel results
	DEFAULT_ALTERNATE_RESULTS,
	SECOND_ALTERNATE_RESULTS,
	VECTOR_SUM_RESULTS,
	PEAK_DISPLACEMENT_RESULTS,
	PEAK_ACCELERATION_RESULTS
};

// Stop Monitoring operation types
enum {
	EVENT_PROCESSING,
	FINISH_PROCESSING
};

// Sampling methods
enum {
	FIXED_SAMPLING = 0,
	ADAPTIVE_SAMPLING
};

// Measurement types
enum {
	IMPERIAL_TYPE = 1,
	METRIC_TYPE
};

// Measurement conversion values
#define IMPERIAL		1
#define METRIC 			25.4

enum {
	DECIBEL_TYPE = 1,
	MILLIBAR_TYPE,
	PSI_TYPE
};

// Old Menu number types
#define FIXED_NUM_TYPE 				2
#define WHOLE_NUM_TYPE 				3
#define FIXED_NUM_ONE_DECMIAL_TYPE 	4
#define WHOLE_NUM_TO_STRING_TYPE 	5
#define WHOLE_NUM_TYPE_5 			6
#define WHOLE_NUM_TYPE_SPECIAL 		7
#define FIXED_NUM_TYPE_SPECIAL 		8
#define FIXED_TIME_TYPE_DAY 		9
#define FIXED_TIME_TYPE_MONTH 		10
#define FIXED_TIME_TYPE_YEAR 		11

// Menu types used as an index for the menufunc pointers
enum {
	MAIN_MENU = 0,
	LOAD_REC_MENU,
	SUMMARY_MENU,
	MONITOR_MENU,
	RESULTS_MENU,
	OVERWRITE_MENU,
	BATTERY_MENU,
	DATE_TIME_MENU,
	LCD_CONTRAST_MENU,
	TIMER_MODE_TIME_MENU,
	TIMER_MODE_DATE_MENU,
	CAL_SETUP_MENU,
	USER_MENU,
	VIEW_MONITOR_LOG_MENU,
	// Add new menu types here
	TOTAL_NUMBER_OF_MENUS
};

// Data values used in 430 messaging
#define NO_TRIGGER_CHAR 				0xEFFF
#define EXTERNAL_TRIGGER_CHAR			0xF000
#define MANUAL_TRIGGER_CHAR 			0xF001
#define VARIABLE_TRIGGER_CHAR_BASE		0xF100

// Manual Cal default rate
#define MANUAL_CAL_DEFAULT_SAMPLE_RATE			SAMPLE_RATE_1K
#define MANUAL_CAL_PRETRIGGER_BUFFER_DIVIDER	4

// Old Menu layout stuff
#define DEFAULT_MN_SIZE 		20
#define MAX_CHAR_PER_LN 		25 

// Old Menu row defines
#define DEFAULT_MENU_ROW_ZERO 	0
#define DEFAULT_MENU_ROW_ONE 	8
#define DEFAULT_MENU_ROW_TWO 	16
#define DEFAULT_MENU_ROW_THREE 	24
#define DEFAULT_MENU_ROW_FOUR 	32
#define DEFAULT_MENU_ROW_FIVE 	40
#define DEFAULT_MENU_ROW_SIX 	48
#define DEFAULT_MENU_ROW_SEVEN 	56

// Old Menu column defines
#define DEFAULT_COL_ZERO 		0
#define DEFAULT_COL_ONE 		1
#define DEFAULT_COL_TWO 		2
#define DEFAULT_COL_THREE 		3
#define DEFAULT_COL_FOUR 		4
#define DEFAULT_COL_FIVE 		5
#define DEFAULT_COL_SIX 		6

// Old Menu row and column end positions
#define DEFAULT_END_ROW 		63
#define DEFAULT_END_COL 		127

// Menu line types
enum {
	REG_LN = 1,
	CURSOR_LN,
	CURSOR_CHAR
};

// Monitor menu stuff
#define UP_ARROW_CHAR		0x01
#define DOWN_ARROW_CHAR		0x02
#define BOTH_ARROWS_CHAR	0x03

// Old Menu scroll logic stuff
#define SELECT_MN_WND_LNS 			6

// Old Menu input types
enum {
	INPUT_CHAR_STRING = 1,
	INPUT_NUM_STRING,
	INPUT_VALUE_LIST
};

// Font types
enum {
	SIX_BY_EIGHT_FONT = 1,
	EIGHT_BY_EIGHT_FONT,
	FOUR_BY_SIX_FONT
};

// Old Menu row and column sizes
#define FOUR_COL_SIZE 		4
#define SIX_ROW_SIZE 		6
#define SIX_COL_SIZE 		6
#define EIGHT_COL_SIZE 		8
#define EIGHT_ROW_SIZE 		8
#define FONT_MAX_COL_SIZE 	10

// Title Position
enum {
	TITLE_CENTERED,
	TITLE_LEFT_JUSTIFIED
};

typedef struct
{
	uint16 curr_item;
	uint16 num_items;
	uint16 tab_flag;
	uint16 font;
} WND_BACKGROUND_STRUCT;

typedef struct
{
	uint16 start_col;
	uint16 end_col;
	uint16 start_row;
	uint16 end_row;
	uint16 curr_row;
	uint16 curr_col;
	uint16 next_row;
	uint16 next_col;
	uint16 font;
	uint16 index;
	WND_BACKGROUND_STRUCT background;
} WND_LAYOUT_STRUCT;

typedef struct
{
	uint16 sub_ln;
	uint16 curr_ln;
	uint16 top_ln;
	uint16 dvlist;
} MN_LAYOUT_STRUCT;

// Menu Data structure
typedef struct
{
	uint8 data[MAX_CHAR_PER_LN];
} MN_MEM_DATA_STRUCT;

// Temp Menu Data structure
typedef struct
{
	uint16 preTag;
	uint16 textEntry;
	uint16 postTag;
} TEMP_MENU_DATA_STRUCT;

// User Menu Macros for combining bytes into a long
#define INSERT_USER_MENU_WORD_DATA(a, b)		((a<<16) | b)
#define INSERT_USER_MENU_BYTE_DATA(a, b, c, d)	((a<<24) | (b<<16) | (c<<8) | d)
#define INSERT_USER_MENU_INFO(a, b, c, d)		((a<<24) | (b<<16) | (c<<8) | d)

// User Menu stuff
#define END_OF_MENU	0

// User Type structure
typedef struct
{
	char text[MAX_CHAR_PER_LN];
	uint8 type;
	float conversion;
} USER_TYPE_STRUCT;

// User Menu Cache structure
typedef struct
{
	char text[MAX_CHAR_PER_LN];
	union 
	{
		uint32 data;
		uint16 wordData[2];
		uint8 byteData[4];
	};
} USER_MENU_CACHE_STRUCT;

// User Menu structure
typedef struct
{
	uint16 preTag;
	uint16 preNum;
	uint16 textEntry;
	uint16 postTag;
	union 
	{
		uint32 data;
		uint16 wordData[2];
		uint8 byteData[4];
	};
} USER_MENU_STRUCT;

// User Menu input cache struct
typedef struct {
	char text[128];
	float floatData;
	float floatDefault;
	float floatIncrement;
	float floatMinValue;
	float floatMaxValue;
	uint32 intDefault;
	uint32 intMinValue;
	uint32 intMaxValue;
	char* unitText;
	uint32 numLongData;
	uint16 numWordData;
	uint8 numByteData;
	uint8 boundary;
	uint16 currentIndex;
	DATE_TIME_STRUCT date;
} USER_MENU_CACHE_DATA;

#define MENU_TAGS_MAX_CHARS	22

// User Menu Tags
typedef struct 
{
	char text[MENU_TAGS_MAX_CHARS];
	uint8 type;
} USER_MENU_TAGS_STRUCT;

// MessageBox number of choices types
enum {
	MB_ONE_CHOICE = 0,
	MB_TWO_CHOICES
};

// MessageBox choice types
enum {
	MB_NO_ACTION = 0,
	MB_FIRST_CHOICE,
	MB_SECOND_CHOICE,
	MB_SPECIAL_ACTION
};

// MessageBox choices struct
typedef struct {
	BOOLEAN numChoices;
	uint16 firstTextEntry;
	uint16 secondTextEntry;
} MB_CHOICE;

// This enum serves as an index for MessageChoices (in the c file to prevent multiple definitions)
typedef enum {
	MB_OK = 0,
	MB_YESNO,
	MB_OKCANCEL,
	// Add a new MessageBox choice type above this line and then fill in a corresponding MessageChoices entry
	MB_TOTAL_CHOICES
} MB_CHOICE_TYPE;

// Menu Message Macros
#define JUMP_TO_ACTIVE_MENU()	(*g_menufunc_ptrs[g_activeMenu]) (mn_msg)

#define SETUP_MENU_MSG(m) \
	g_activeMenu = m; \
	mn_msg.cmd = ACTIVATE_MENU_CMD;\
	mn_msg.length = 0;\
	mn_msg.data[0] = 0;

#define SETUP_MENU_WITH_DATA_MSG(m, x) \
	g_activeMenu = m; \
	mn_msg.cmd = ACTIVATE_MENU_WITH_DATA_CMD;\
	mn_msg.length = 1;\
	mn_msg.data[0] = x;

#define SETUP_USER_MENU_MSG(a, b) \
	g_activeMenu = USER_MENU;\
	mn_msg.cmd = ACTIVATE_MENU_WITH_DATA_CMD;\
	mn_msg.length = 4;\
	mn_msg.data[0] = (uint32)a;\
	mn_msg.data[1] = (uint32)b;

#define SETUP_USER_MENU_FOR_FLOATS_MSG(a, b, c, d, e, f) \
	g_activeMenu = USER_MENU;\
	mn_msg.cmd = ACTIVATE_MENU_WITH_DATA_CMD;\
	mn_msg.length = 4;\
	mn_msg.data[0] = (uint32)a;\
	mn_msg.data[1] = (uint32)b;\
	g_userMenuCacheData.floatDefault = (float)c;\
	g_userMenuCacheData.floatIncrement = (float)d;\
	g_userMenuCacheData.floatMinValue = (float)e;\
	g_userMenuCacheData.floatMaxValue = (float)f;

#define SETUP_USER_MENU_FOR_INTEGERS_MSG(a, b, c, d, e) \
	g_activeMenu = USER_MENU;\
	mn_msg.cmd = ACTIVATE_MENU_WITH_DATA_CMD;\
	mn_msg.length = 4;\
	mn_msg.data[0] = (uint32)a;\
	mn_msg.data[1] = (uint32)b;\
	g_userMenuCacheData.intDefault = (uint32)c;\
	g_userMenuCacheData.intMinValue = (uint32)d;\
	g_userMenuCacheData.intMaxValue = (uint32)e;

#define SETUP_RESULTS_MENU_MONITORING_MSG(m) \
	g_activeMenu = m; \
	mn_msg.cmd = ACTIVATE_MENU_WITH_DATA_CMD;\
	mn_msg.length = 1;\
	mn_msg.data[0] = 12;

#define SETUP_RESULTS_MENU_MANUAL_CAL_MSG(m) \
	g_activeMenu = m; \
	mn_msg.cmd = ACTIVATE_MENU_WITH_DATA_CMD;\
	mn_msg.length = 1;\
	mn_msg.data[0] = 13;

enum {
	PRINTER_OFF,
	PRINTER_ON
};	

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------
void MainMenu(INPUT_MSG_STRUCT);
void LoadRecordMenu (INPUT_MSG_STRUCT);
void SummaryMenu (INPUT_MSG_STRUCT);
void MonitorMenu (INPUT_MSG_STRUCT);
void ResultsMenu (INPUT_MSG_STRUCT);
void OverwriteMenu (INPUT_MSG_STRUCT);
void BatteryMn (INPUT_MSG_STRUCT);
void DateTimeMn (INPUT_MSG_STRUCT);
void LcdContrastMn (INPUT_MSG_STRUCT);
void TimerModeTimeMenu (INPUT_MSG_STRUCT msg);
void TimerModeDateMenu (INPUT_MSG_STRUCT msg);
void CalSetupMn (INPUT_MSG_STRUCT msg);
void UserMenu (INPUT_MSG_STRUCT msg);
void MonitorLogMn (INPUT_MSG_STRUCT msg);

//----------------------------------------
// User Select Menu Enter and Esc Handlers
//----------------------------------------
void AcousticSensorTypeMenuHandler(uint8 key, void* data);
void AdaptiveSamplingMenuHandler(uint8 key, void* data);
void AdChannelVerificationMenuHandler(uint8 key, void* data);
void AirScaleMenuHandler(uint8 key, void* data);
void AirSetupMenuHandler(uint8 key, void* data);
void AlarmOneMenuHandler(uint8 key, void* data);
void AlarmTwoMenuHandler(uint8 key, void* data);
void AlarmOutputMenuHandler(uint8 key, void* data);
void AlarmTestingMenuHandler(uint8 key, void* data);
void AnalogChannelConfigMenuHandler(uint8 key, void* data);
void AutoCalMenuHandler(uint8 key, void* data);
void AutoMonitorMenuHandler(uint8 key, void* data);
void BarChannelMenuHandler(uint8 key, void* data);
void BarIntervalMenuHandler(uint8 key, void* data);
void BarIntervalDataTypeMenuHandler(uint8 key, void* data);
void BarLiveMonitorMenuHandler(uint8 key, void* data);
void BLMStartStopMsgMenuHandler(uint8 key, void* data);
void BarScaleMenuHandler(uint8 key, void* data);
void BarResultMenuHandler(uint8 key, void* data);
void BaudRateMenuHandler(uint8 key, void* data);
void BitAccuracyMenuHandler(uint8 key, void* data);
void CalibratonDateSourceMenuHandler(uint8 key, void* data);
void ConfigMenuHandler(uint8 key, void* data);
void CustomCurveMenuHandler(uint8 key, void* data);
void DisplacementMenuHandler(uint8 key, void* data);
void EraseEventsMenuHandler(uint8 key, void* data);
void EraseSettingsMenuHandler(uint8 key, void* data);
void ExternalTriggerMenuHandler(uint8 key, void* data);
void FlashWrappingMenuHandler(uint8 key, void* data);
void FreqPlotMenuHandler(uint8 key, void* data);
void FreqPlotStandardMenuHandler(uint8 key, void* data);
void GpsPowerMenuHandler(uint8 key, void* data);
void HardwareIDMenuHandler(uint8 key, void* data);
void HelpMenuHandler(uint8 key, void* data);
void InfoMenuHandler(uint8 key, void* data);
void LanguageMenuHandler(uint8 key, void* data);
void LegacyDqmLimitMenuHandler(uint8 key, void* data);
void ModeMenuHandler(uint8 key, void* data);
void ModemDialOutTypeMenuHandler(uint8 key, void* data);
void ModemSetupMenuHandler(uint8 key, void* data);
void MonitorLogMenuHandler(uint8 key, void* data);
void PeakAccMenuHandler(uint8 key, void* data);
void PretriggerSizeMenuHandler(uint8 key, void* data);
void PrinterEnableMenuHandler(uint8 key, void* data);
void PrintOutMenuHandler(uint8 key, void* data);
void PrintMonitorLogMenuHandler(uint8 key, void* data);
void Rs232PowerSavingsMenuHandler(uint8 key, void* data);
void RecalibrateMenuHandler(uint8 key, void* data);
void SaveCompressedDataMenuHandler(uint8 key, void* data);
void SampleRateMenuHandler(uint8 key, void* data);
void SamplingMethodMenuHandler(uint8 key, void* data);
void SaveSetupMenuHandler(uint8 key, void* data);
void SeismicFilteringMenuHandler(uint8 key, void* data);
void SeismicSensorTypeMenuHandler(uint8 key, void* data);
void SeismicTriggerTypeMenuHandler(uint8 keyPressed, void* data);
void SensitivityMenuHandler(uint8 key, void* data);
void StoredEventsCapModeMenuHandler(uint8 key, void* data);
void SummaryIntervalMenuHandler(uint8 key, void* data);
void SyncFileExistsMenuHandler(uint8 key, void* data);
void TimerModeMenuHandler(uint8 key, void* data);
void TimerModeFreqMenuHandler(uint8 key, void* data);
void UnitsOfMeasureMenuHandler(uint8 key, void* data);
void UnitsOfAirMenuHandler(uint8 key, void* data);
void UsbSyncModeMenuHandler(uint8 key, void* data);
void VectorSumMenuHandler(uint8 key, void* data);
void VibrationStandardMenuHandler(uint8 keyPressed, void* data);
void WaveformAutoCalMenuHandler(uint8 key, void* data);
void ZeroEventNumberMenuHandler(uint8 key, void* data);
//----------------------------------------
// End of User Select Menu Handlers
//----------------------------------------

//----------------------------------------
// User Input Menu Enter and Esc Handlers
//----------------------------------------
void AirTriggerMenuHandler(uint8 key, void* data);
void AlarmOneSeismicLevelMenuHandler(uint8 key, void* data);
void AlarmOneAirLevelMenuHandler(uint8 key, void* data);
void AlarmOneTimeMenuHandler(uint8 key, void* data);
void AlarmTwoSeismicLevelMenuHandler(uint8 key, void* data);
void AlarmTwoAirLevelMenuHandler(uint8 key, void* data);
void AlarmTwoTimeMenuHandler(uint8 key, void* data);
void CompanyMenuHandler(uint8 key, void* data);
void CopiesMenuHandler(uint8 key, void* data);
void CycleEndTimeMenuHandler(uint8 keyPressed, void* data);
void DistanceToSourceMenuHandler(uint8 key, void* data);
void LcdImpulseTimeMenuHandler(uint8 key, void* data);
void LcdTimeoutMenuHandler(uint8 key, void* data);
void ModemDialMenuHandler(uint8 key, void* data);
void ModemDialOutCycleTimeMenuHandler(uint8 key, void* data);
void ModemInitMenuHandler(uint8 key, void* data);
void ModemResetMenuHandler(uint8 key, void* data);
void ModemRetryMenuHandler(uint8 key, void* data);
void ModemRetryTimeMenuHandler(uint8 key, void* data);
void NotesMenuHandler(uint8 key, void* data);
void OperatorMenuHandler(uint8 key, void* data);
void PercentOfLimitTriggerMenuHandler(uint8 keyPressed, void* data);
void RecordTimeMenuHandler(uint8 key, void* data);
void SaveRecordMenuHandler(uint8 key, void* data);
void SeismicLocationMenuHandler(uint8 key, void* data);
void SeismicTriggerMenuHandler(uint8 key, void* data);
void SerialNumberMenuHandler(uint8 key, void* data);
void StoredEventLimitMenuHandler(uint8 key, void* data);
void UnlockCodeMenuHandler(uint8 key, void* data);
void UtcZoneOffsetMenuHandler(uint8 key, void* data);
void WeightPerDelayMenuHandler(uint8 key, void* data);
//----------------------------------------
// End of Menu Handlers
//----------------------------------------

///----------------------------------------------------------------------------
///	Other Prototypes
///----------------------------------------------------------------------------
// MessageBox headers and helper routines
void MessageBorder(void);
void MessageTitle(char* titleString);
void MessageText(char* textString);
void MessageChoice(MB_CHOICE_TYPE choiceType);
void MessageChoiceActiveSwap(MB_CHOICE_TYPE choiceType);
uint8 MessageBox(char* titleString, char* textString, MB_CHOICE_TYPE messageType);
void OverlayMessage(char* titleString, char* textString, uint32 usDisplayTime);
void DisplayLogoToLcd(void);

// Prototypes needed across menus
void LoadTempMenuTable(TEMP_MENU_DATA_STRUCT* currentMenu);
void WndMpWrtString(uint8*, WND_LAYOUT_STRUCT*, int, int);
void MenuScroll(char, char, MN_LAYOUT_STRUCT*);
void UserMenuScroll(uint32 direction, char wnd_size, MN_LAYOUT_STRUCT* mn_layout_ptr);
void DisplayUserMenu(WND_LAYOUT_STRUCT* wnd_layout_ptr, MN_LAYOUT_STRUCT* mn_layout_ptr, uint8 titlePosition);
void DisplaySelectMenu(WND_LAYOUT_STRUCT*, MN_LAYOUT_STRUCT*, uint8 titlePosition);
void StopDataCollection(void);
void StopDataClock(void);
void StartMonitoring(uint8 operationMode, TRIGGER_EVENT_DATA_STRUCT* opModeParamsPtr);
void StopMonitoring(uint8 mode, uint8 operation);
void HandleManualCalibration(void);
void ForcedCalibration(void);
void ProcessTimerModeSettings(uint8 mode);
void WaitForEventProcessingToFinish(void);
void UpdateModeMenuTitle(uint8 mode);
void DisplaySplashScreen(void);
void DisplayCalDate(void);
void DisplaySensorType(void);
void DisplaySerialNumber(void);
void DisplayTimerModeSettings(void);
void DisplayFlashUsageStats(void);
void DisplayAutoDialInfo(void);
uint32 GetAirDefaultValue(void);
uint32 GetAirMinValue(void);
uint32 GetAirMaxValue(void);
void GetAirSensorTypeName(char* airSensorTypeName);
void InitSensorParameters(uint16 seismicSensorType, uint8 sensitivity);
void StopMonitoringForLowPowerState(void);
uint8 CheckAndDisplayErrorThatPreventsMonitoring(uint8 messageType);
void CheckAndPromptUserWaitingForSensorWarmup(void);
void StartADDataCollectionForCalibration(uint16 sampleRate);
void StopADDataCollectionForCalibration(void);

#endif // _MENU_H_
