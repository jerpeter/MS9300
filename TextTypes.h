///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

#ifndef _TEXT_TYPES_H_
#define _TEXT_TYPES_H_

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include "Typedefs.h"

///----------------------------------------------------------------------------
///	Macros
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
extern char englishLanguageTable[];

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define getLangText(x)	g_languageLinkTable[x]

///----------------------------------------------------------------------------
///	Text Types - Language Table/Translation/Strings
///----------------------------------------------------------------------------
enum {
	A_WEIGHTING_TEXT = 0,
	A_WEIGHTING_OPTION_TEXT,
	A_D_CALIBRATED_OK_TEXT,
	ACTIVE_DATE_PERIOD_TEXT,
	ACTIVE_TIME_PERIOD_TEXT,
	AFTER_EVERY_24_HRS_TEXT,
	AFTER_EVERY_48_HRS_TEXT,
	AFTER_EVERY_72_HRS_TEXT,
	AIR_TEXT,
	AIR_CHANNEL_SCALE_TEXT,
	ALARM_1_TEXT,
	ALARM_1_AIR_LEVEL_TEXT,
	ALARM_1_SEISMIC_LVL_TEXT,
	ALARM_1_TIME_TEXT,
	ALARM_2_TEXT,
	ALARM_2_AIR_LEVEL_TEXT,
	ALARM_2_SEISMIC_LVL_TEXT,
	ALARM_2_TIME_TEXT,
	ALARM_OUTPUT_MODE_TEXT,
	ALL_RIGHTS_RESERVED_TEXT,
	ARE_YOU_DONE_WITH_CAL_SETUP_Q_TEXT,
	ATTEMPTING_TO_HANDLE_PARTIAL_EVENT_ERROR_TEXT,
	AUTO_CALIBRATION_TEXT,
	AUTO_MONITOR_TEXT,
	AUTO_MONITOR_AFTER_TEXT,
	BAR_GRAPH_TEXT,
	BAR_INTERVAL_TEXT,
	BAR_PRINTOUT_RESULTS_TEXT,
	BARGRAPH_ABORTED_TEXT,
	BARGRAPH_MODE_TEXT,
	BARS_TEXT,
	BATT_VOLTAGE_TEXT,
	BATTERY_TEXT,
	BATTERY_VOLTAGE_TEXT,
	BOTH_TEXT,
	CAL_DATE_STORED_TEXT,
	CAL_SETUP_TEXT,
	CALIBRATING_TEXT,
	CALIBRATION_TEXT,
	CALIBRATION_CHECK_TEXT,
	CALIBRATION_DATE_TEXT,
	CALIBRATION_DATE_NOT_SET_TEXT,
	CALIBRATION_GRAPH_TEXT,
	CALIBRATION_PULSE_TEXT,
	CANCEL_TIMER_MODE_Q_TEXT,
	CLIENT_TEXT,
	COMBO_TEXT,
	COMBO_MODE_TEXT,
	COMBO_MODE_NOT_IMPLEMENTED_TEXT,
	COMMENTS_TEXT,
	COMPANY_TEXT,
	CONFIG_AND_OPTIONS_TEXT,
	CONFIG_OPTIONS_MENU_TEXT,
	CONFIRM_TEXT,
	COPIES_TEXT,
	COPY_TEXT,
	CURRENTLY_NOT_IMPLEMENTED_TEXT,
	DAILY_EVERY_DAY_TEXT,
	DAILY_WEEKDAYS_TEXT,
	DARK_TEXT,
	DARKER_TEXT,
	DATE_TEXT,
	DATE_TIME_TEXT,
	DAY_MONTH_YEAR_TEXT,
	DEFAULT_TEXT,
	DEFAULT_BAR_TEXT,
	DEFAULT_COMBO_TEXT,
	DEFAULT_SELF_TRG_TEXT,
	DISABLED_TEXT,
	DISABLING_TIMER_MODE_TEXT,
	DISCOVERED_A_BROKEN_EVENT_LINK_TEXT,
	DISTANCE_TO_SOURCE_TEXT,
	DO_NOT_TURN_THE_UNIT_OFF_UNTIL_THE_OPERATION_IS_COMPLETE_TEXT,
	DO_YOU_WANT_TO_ENTER_MANUAL_TRIGGER_MODE_Q_TEXT,
	DO_YOU_WANT_TO_LEAVE_CAL_SETUP_MODE_Q_TEXT,
	DO_YOU_WANT_TO_LEAVE_MONITOR_MODE_Q_TEXT,
	DO_YOU_WANT_TO_SAVE_THE_CAL_DATE_Q_TEXT,
	EDIT_TEXT,
	EMPTY_TEXT,
	ENABLED_TEXT,
	ERASE_COMPLETE_TEXT,
	ERASE_EVENTS_TEXT,
	ERASE_MEMORY_TEXT,
	ERASE_OPERATION_IN_PROGRESS_TEXT,
	ERASE_SETTINGS_TEXT,
	ERROR_TEXT,
	ESC_HIT_TEXT,
	EVENT_TEXT,
	EVENT_NUMBER_ZEROING_COMPLETE_TEXT,
	EVENT_SUMMARY_TEXT,
	FACTORY_SETUP_COMPLETE_TEXT,
	FACTORY_SETUP_DATA_COULD_NOT_BE_FOUND_TEXT,
	FAILED_TEXT,
	FEET_TEXT,
	FREQUENCY_TEXT,
	FREQUENCY_PLOT_TEXT,
	FRQ_TEXT,
	FULL_TEXT,
	GRAPHICAL_RECORD_TEXT,
	HELP_INFO_MENU_TEXT,
	HELP_INFORMATION_TEXT,
	HELP_MENU_TEXT,
	HOUR_TEXT,
	HOURS_TEXT,
	HR_TEXT,
	HZ_TEXT,
	IMPERIAL_TEXT,
	INCLUDED_TEXT,
	INSTRUMENT_TEXT,
	JOB_NUMBER_TEXT,
	JOB_PEAK_RESULTS_TEXT,
	JOB_VECTOR_SUM_RESULTS_TEXT,
	LANGUAGE_TEXT,
	LAST_SETUP_TEXT,
	LCD_CONTRAST_TEXT,
	LIGHT_TEXT,
	LIGHTER_TEXT,
	LINEAR_TEXT,
	LIST_OF_SUMMARIES_TEXT,
	LOCATION_TEXT,
	LOW_TEXT,
	METRIC_TEXT,
	MIC_A_TEXT,
	MIC_B_TEXT,
	MINUTE_TEXT,
	MINUTES_TEXT,
	MMPS_DIV_TEXT,
	MONITOR_TEXT,
	MONITOR_BARGRAPH_TEXT,
	MONITORING_TEXT,
	MONTHLY_TEXT,
	NAME_TEXT,
	NEXT_EVENT_STORAGE_LOCATION_NOT_EMPTY_TEXT,
	NO_TEXT,
	NO_AUTO_CAL_TEXT,
	NO_AUTO_MONITOR_TEXT,
	NO_STORED_EVENTS_FOUND_TEXT,
	NOMIS_MAIN_MENU_TEXT,
	NOT_INCLUDED_TEXT,
	NOTES_TEXT,
	OFF_TEXT,
	ON_TEXT,
	ONE_TIME_TEXT,
	OPERATOR_TEXT,
	OVERWRITE_SETTINGS_TEXT,
	PARTIAL_RESULTS_TEXT,
	PASSED_TEXT,
	PEAK_TEXT,
	PEAK_AIR_TEXT,
	PLEASE_BE_PATIENT_TEXT,
	PLEASE_CONSIDER_ERASING_THE_FLASH_TO_FIX_PROBLEM_TEXT,
	PLEASE_POWER_OFF_UNIT_TEXT,
	PLEASE_PRESS_ENTER_TEXT,
	PLOT_STANDARD_TEXT,
	POWERING_UNIT_OFF_NOW_TEXT,
	PRINT_GRAPH_TEXT,
	PRINTER_TEXT,
	PRINTER_ON_TEXT,
	PRINTING_STOPPED_TEXT,
	PRINTOUT_TEXT,
	PROCEED_WITHOUT_SETTING_DATE_AND_TIME_Q_TEXT,
	PROCESSING_TEXT,
	RAD_TEXT,
	RADIAL_TEXT,
	RAM_SUMMARY_TABLE_IS_UNINITIALIZED_TEXT,
	RECORD_TIME_TEXT,
	REPORT_DISPLACEMENT_TEXT,
	RESULTS_TEXT,
	SAMPLE_RATE_TEXT,
	SAVE_CHANGES_TEXT,
	SAVE_SETUP_TEXT,
	SAVED_SETTINGS_TEXT,
	SCANNING_STORAGE_FOR_VALID_EVENTS_TEXT,
	SEC_TEXT,
	SECOND_TEXT,
	SECONDS_TEXT,
	SEIS_TRIG_TEXT,
	SEIS_LOCATION_TEXT,
	SEISMIC_TEXT,
	SEISMIC_TRIGGER_TEXT,
	SELECT_TEXT,
	SELF_TRIGGER_TEXT,
	SENSOR_A_TEXT,
	SENSOR_B_TEXT,
	SENSOR_CHECK_TEXT,
	SENSOR_GAIN_TYPE_TEXT,
	SENSOR_GAIN_TYPE_NOT_SET_TEXT,
	SENSOR_TYPE_TEXT,
	SERIAL_NUMBER_TEXT,
	SERIAL_NUMBER_NOT_SET_TEXT,
	SETTINGS_WILL_NOT_BE_LOADED_TEXT,
	SOFTWARE_VER_TEXT,
	SOUND_TEXT,
	AIR_TRIG_TEXT,
	AIR_TRIGGER_TEXT,
	SPANISH_TEXT,
	START_DATE_TEXT,
	START_TIME_TEXT,
	STATUS_TEXT,
	STOP_DATE_TEXT,
	STOP_PRINTING_TEXT,
	STOP_TIME_TEXT,
	SUCCESS_TEXT,
	EVENT_SUMMARIES_TEXT,
	SUMMARY_INTERVAL_TEXT,
	SUMMARY_INTERVAL_RESULTS_TEXT,
	TESTING_TEXT,
	THIS_FEATURE_IS_NOT_CURRENTLY_AVAILABLE_TEXT,
	TIME_TEXT,
	TIMER_FREQUENCY_TEXT,
	TIMER_MODE_TEXT,
	TIMER_MODE_DISABLED_TEXT,
	TIMER_MODE_NOW_ACTIVE_TEXT,
	TIMER_MODE_SETUP_NOT_COMPLETED_TEXT,
	TIMER_SETTINGS_INVALID_TEXT,
	TRAN_TEXT,
	TRANSVERSE_TEXT,
	UNIT_IS_IN_TIMER_MODE_TEXT,
	UNITS_OF_MEASURE_TEXT,
	USBM_OSMRE_REPORT_TEXT,
	VECTOR_SUM_TEXT,
	VERIFY_TEXT,
	VERT_TEXT,
	VERTICAL_TEXT,
	VOLTS_TEXT,
	WARNING_TEXT,
	WAVEFORM_MODE_TEXT,
	WEEKLY_TEXT,
	WEIGHT_PER_DELAY_TEXT,
	YES_TEXT,
	YOU_HAVE_ENTERED_THE_FACTORY_SETUP_TEXT,
	ZERO_EVENT_NUMBER_TEXT,
	ZEROING_SENSORS_TEXT,
	OK_TEXT,
	CANCEL_TEXT,
	MAX_TEXT,
	CHARS_TEXT,
	RANGE_TEXT,
	ENGLISH_TEXT,
	ITALIAN_TEXT,
	FRENCH_TEXT,
	BRITISH_TEXT,
	DIN_4150_TEXT,
	US_BOM_TEXT,
	X1_20_IPS_TEXT,
	X2_10_IPS_TEXT,
	X4_5_IPS_TEXT,
	X8_2_5_IPS_TEXT,
	BAR_TEXT,
	SELF_TRG_TEXT,
	END_TEXT,
	REALLY_ERASE_ALL_EVENTS_Q_TEXT,
	MODEM_SETUP_TEXT,
	MODEM_INIT_TEXT,
	MODEM_DIAL_TEXT,
	MODEM_RESET_TEXT,
	SENSITIVITY_TEXT,
	HIGH_TEXT,
	UNLOCK_CODE_TEXT,
	ACC_793L_TEXT,
	GERMAN_TEXT,
	BAUD_RATE_TEXT,
	POWER_UNIT_OFF_EARLY_Q_TEXT,
	NAME_ALREADY_USED_TEXT,
	DELETE_SAVED_SETUP_Q_TEXT,
	NAME_MUST_HAVE_AT_LEAST_ONE_CHARACTER_TEXT,
	BAR_SCALE_TEXT,
	REPORT_MILLIBARS_TEXT,
	LCD_TIMEOUT_TEXT,
	IMPULSE_RESULTS_TEXT,
	LCD_IMPULSE_TIME_TEXT,
	PRINT_MONITOR_LOG_TEXT,
	MONITOR_LOG_TEXT,
	EVENTS_RECORDED_TEXT,
	EVENT_NUMBERS_TEXT,
	CANCEL_ALL_PRINT_JOBS_Q_TEXT,
	CAL_SUMMARY_TEXT,
	MODEM_RETRY_TEXT,
	MODEM_RETRY_TIME_TEXT,
	FLASH_WRAPPING_TEXT,
	FLASH_STATS_TEXT,
	AUTO_DIAL_INFO_TEXT,
	PEAK_DISPLACEMENT_TEXT,
	WAVEFORM_AUTO_CAL_TEXT,
	START_OF_WAVEFORM_TEXT,
	CALIBRATED_ON_TEXT,
	VIEW_MONITOR_LOG_TEXT,
	PRINT_IN_MONITOR_TEXT,
	LOG_RESULTS_TEXT,
	MONITOR_LOG_RESULTS_TEXT,
	PRINT_MONITOR_LOG_ENTRY_Q_TEXT,
	PRINTER_DISABLED_TEXT,
	PRINTING_TEXT,
	BARGRAPH_REPORT_TEXT,
	NORMAL_FORMAT_TEXT,
	SHORT_FORMAT_TEXT,
	REPORT_PEAK_ACC_TEXT,
	PEAK_ACCELERATION_TEXT,
	EVENT_NUMBER_TEXT,
	START_OF_LOG_TEXT,
	END_OF_LOG_TEXT,
	USED_TEXT,
	FREE_TEXT,
	EVENT_DATA_TEXT,
	WRAPPED_TEXT,
	FLASH_USAGE_STATS_TEXT,
	SPACE_REMAINING_TEXT,
	BEFORE_OVERWRITE_TEXT,
	WAVEFORMS_TEXT,
	BAR_HOURS_TEXT,
	LAST_DIAL_EVENT_TEXT,
	LAST_RECEIVED_TEXT,
	LAST_CONNECTED_TEXT,
	AUTO_DIALOUT_INFO_TEXT,
	UNITS_OF_AIR_TEXT,
	DECIBEL_TEXT,
	MILLIBAR_TEXT,
	BAUD_RATE_115200_TEXT,
	BIT_ACCURACY_TEXT,
	BIT_TEXT,
	HOURLY_TEXT,
	RECALIBRATE_TEXT,
	ON_TEMP_CHANGE_TEXT,
	PRETRIGGER_SIZE_TEXT,
	QUARTER_SECOND_TEXT,
	HALF_SECOND_TEXT,
	FULL_SECOND_TEXT,
	POWER_SAVINGS_TEXT,
	NONE_TEXT,
	MINIMUM_TEXT,
	MOST_TEXT,
	ANALOG_CHANNEL_CONFIG_TEXT,
	CHANNELS_R_AND_V_SCHEMATIC_TEXT,
	CHANNELS_R_AND_V_SWAPPED_TEXT,
	NORMAL_DEFAULT_TEXT,
	NORM_AND_NO_USB_TEXT,
	NORM_AND_NO_USB_CRAFT_TEXT,
	SAVE_COMPRESSED_DATA_TEXT,
	YES_FASTER_DOWNLOAD_TEXT,
	FILE_SYSTEM_BUSY_DURING_TEXT,
	ERROR_TRYING_TO_ACCESS_TEXT,
	SUMMARY_LIST_TEXT,
	INITIALIZING_SUMMARY_LIST_WITH_STORED_EVENT_INFO_TEXT,
	FILE_NOT_FOUND_TEXT,
	FILE_CORRUPT_TEXT,
	UNABLE_TO_DELETE_TEXT,
	REMOVING_TEXT,
	UNABLE_TO_ACCESS_TEXT,
	DIR_TEXT,
	DELETE_EVENTS_TEXT,
	REMOVED_TEXT,
	EVENTS_TEXT,
	FAILED_TO_INIT_SD_CARD_TEXT,
	FAILED_TO_MOUNT_SD_CARD_TEXT,
	FAILED_TO_SELECT_SD_CARD_DRIVE_TEXT,
	SD_CARD_IS_NOT_PRESENT_TEXT,
	CALCULATING_EVENT_STORAGE_SPACE_FREE_TEXT,
	INITIALIZING_MONITOR_LOG_WITH_SAVED_ENTRIES_TEXT,
	PLEASE_CHARGE_BATTERY_TEXT,
	USB_THUMB_DRIVE_TEXT,
	USB_DEVICE_CABLE_WAS_REMOVED_TEXT,
	USB_HOST_OTG_CABLE_WAS_REMOVED_TEXT,
	DISCONNECTED_TEXT,
	DISCOVERED_TEXT,
	USB_DOWNLOAD_TEXT,
	SYNC_EVENTS_TO_USB_THUMB_DRIVE_Q_TEXT,
	SYNC_IN_PROGRESS_TEXT,
	SYNC_SUCCESSFUL_TEXT,
	TOTAL_EVENTS_TEXT,
	NEW_TEXT,
	EXISTING_TEXT,
	SYNC_ENCOUNTERED_AN_ERROR_TEXT,
	PLEASE_REMOVE_THUMB_DRIVE_TO_CONSERVE_POWER_TEXT,
	USB_DEVICE_STATUS_TEXT,
	USB_TO_PC_CABLE_WAS_CONNECTED_TEXT,
	USB_TO_PC_CABLE_WAS_DISCONNECTED_TEXT,
	USB_HOST_STATUS_TEXT,
	USB_DEVICE_WAS_CONNECTED_TEXT,
	USB_OTG_HOST_CABLE_WAS_CONNECTED_TEXT,
	USB_STATUS_TEXT,
	USB_CONNECTION_DISABLED_FOR_MONITORING_TEXT,
	USB_CONNECTION_DISABLED_FOR_FILE_OPERATION_TEXT,
	USB_DEVICE_WAS_REMOVED_TEXT,
	APP_LOADER_TEXT,
	STARTING_APP_LOADER_TEXT,
	FLASH_MEMORY_IS_FULL_TEXT,
	WRAPPING_TEXT,
	UNAVAILABLE_TEXT,
	STOPPED_TEXT,
	CLOSING_MONITOR_SESSION_TEXT,
	PERFORMING_CALIBRATION_TEXT,
	BEING_SAVED_TEXT,
	EVENT_COMPLETE_TEXT,
	WAVEFORM_TEXT,
	COMBO_WAVEFORM_TEXT,
	COMBO_BARGRAPH_TEXT,
	MAY_TAKE_TIME_TEXT,
	SYNC_PROGRESS_TEXT,
	EXISTS_TEXT,
	SYNCING_TEXT,
	ACOUSTIC_TEXT,
	TYPE_TEXT,
	STANDARD_TEXT,
	SMART_SENSOR_TEXT,
	NOT_FOUND_TEXT,
	START_CALIBRATION_DIAGNOSTICS_Q_TEXT,
	ERASE_FACTORY_SETUP_Q_TEXT,
	UNSUPPORTED_DEVICE_TEXT,
	PLEASE_REMOVE_IMMEDIATELY_BEFORE_SELECTING_OK_TEXT,
	UNIT_SET_FOR_EXTERNAL_TRIGGER_TEXT,
	SET_TO_NO_TRIGGER_TEXT,
	PLEASE_CHANGE_TEXT,
	SEISMIC_OR_AIR_TRIGGER_TEXT,
	FOR_HOURLY_MODE_THE_HOURS_AND_MINUTES_FIELDS_ARE_INDEPENDENT_TEXT,
	TIMER_MODE_HOURLY_TEXT,
	HOURS_ARE_ACTIVE_HOURS_MINS_ARE_ACTIVE_MIN_RANGE_EACH_HOUR_TEXT,
	FIRMWARE_TEXT,
	MODEM_SYNC_FAILED_TEXT,
	CRAFT_SERIAL_ERROR_TEXT,
	SENSOR_CALIBRATION_TEXT,
	DISPLAY_SUCCESSIVE_SAMPLES_TEXT,
	CHANNEL_NOISE_PERCENTAGES_TEXT,
	DISPLAY_CALIBRATED_TEXT,
	DISPLAY_NOT_CALIBRATED_TEXT,
	ZERO_TEXT,
	MIN_MAX_AVG_TEXT,
	CONNECT_USB_CABLE_TEXT,
	COPY_TO_SYSTEM_DIR_TEXT,
	REMOVE_CABLE_WHEN_DONE_TEXT,
	MENU_ACTIVATION_TEXT,
	SERIAL_ACTIVATION_TEXT,
	INITIALIZING_TEXT,
	EXTERNAL_TRIGGER_TEXT,
	RS232_POWER_SAVINGS_TEXT,
	BOTH_TRIGGERS_SET_TO_NO_AND_EXTERNAL_TRIGGER_DISABLED_TEXT,
	NO_TRIGGER_SOURCE_SELECTED_TEXT,
	ALARM_TESTING_TEXT,
	CHAN_VERIFICATION_TEXT,
	HARDWARE_ID_TEXT,
	REV_8_NORMAL_TEXT,
	REV_8_WITH_GPS_MOD_TEXT,
	REV_8_WITH_USART_TEXT,
	SYNC_FILE_EXISTS_TEXT,
	SKIP_TEXT,
	REPLACE_TEXT,
	DUPLICATE_TEXT,
	SKIP_ALL_TEXT,
	REPLACE_ALL_TEXT,
	DUPLICATE_ALL_TEXT,
	USB_SYNC_MODE_TEXT,
	PROMPT_ON_CONFLICT_TEXT,
	TOTAL_FILES_TEXT,
	FILE_TEXT,
	FILES_TEXT,
	REPLACED_TEXT,
	DUPLICATED_TEXT,
	SKIPPED_TEXT,
	SENSOR_WARMING_UP_FROM_COLD_START_TEXT,
	FORCED_POWER_OFF_TEXT,
	GPS_LOCATION_TEXT,
	GPS_LOCATION_FIX_TEXT,
	GPS_MODULE_NOT_INCLUDED_TEXT,
	EXCEPTION_REPORT_EXISTS_DUE_TO_ERROR_TEXT,
	PLEASE_CONTACT_SUPPORT_TEXT,
	RESTORE_FACTORY_SETUP_FROM_BACKUP_Q_TEXT,
	ALSO_ERASE_THE_REST_OF_THE_EEPROM_Q_TEXT,
	ERASE_FACTORY_SETUP_SHADOW_COPY_Q_TEXT,
	ERASE_ALL_NON_ESSENTIAL_SYSTEM_FILES_Q_TEXT,
	ACQUIRING_SATELLITE_SIGNALS_TEXT,
	CONTINUING_ACQUISITION_TEXT,
	TIMED_OUT_TEXT,
	SEISMIC_GAIN_TYPE_TEXT,
	ACOUSTIC_GAIN_TYPE_TEXT,
	MIC_148_DB_TEXT,
	MIC_160_DB_TEXT,
	CYCLE_END_TIME_24HR_TEXT,
	CHECK_SUMMARY_FILE_TEXT,
	COMPARING_EVENTS_TO_SUMMARY_LIST_TEXT,
	FOUND_TEXT,
	MERGE_TEXT,
	VALID_EVENTS_TEXT,
	INVALID_EVENTS_TEXT,
	MATCH_SERIAL_TEXT,
	WRONG_SERIAL_TEXT,
	NOT_IN_SUMMARY_TEXT,
	TO_SUMMARY_Q_TEXT,
	SUMMARY_FILE_MATCHES_EVENT_STORAGE_TEXT,
	NO_STRAY_EVENTS_TEXT,
	TWENTY_FOUR_HR_CYCLE_WILL_NOW_OCCUR_AT_TEXT,
	MIDNIGHT_TEXT,
	BAR_DATA_TO_STORE_TEXT,
	A_RVT_MAX_TEXT,
	A_R_V_T_MAX_TEXT,
	A_R_V_T_MAX_WITH_FREQ,
	X05_40_IPS_TEXT,
	X025_80_IPS_TEXT,
	LEGACY_DQM_LIMIT_TEXT,
	STORED_EVENTS_CAP_TEXT,
	MAX_EVTS_TO_KEEP_TEXT,
	ABOVE_EVT_CAP_TEXT,
	OLDEST_FIRST_TEXT,
	STORED_EVTS_MORE_THAN_MAX_TEXT,
	DELETE_OLD_EVTS_NOW_Q_TEXT,
	SENSOR_CAL_PULSE_TEXT,
	BAR_LIVE_MONITOR_TEXT,
	SEISMIC_TRIG_TYPE_TEXT,
	VARIABLE_USBM_OSM_TEXT,
	VIBRATION_STANDARD_TEXT,
	USBM_8507_DRYWALL_TEXT,
	USBM_8507_PLASTER_TEXT,
	OSM_REGULATIONS_TEXT,
	REMOTE_CONTROL_TEXT,
	BLIND_SEND_TEXT,
	VARIABLE_UNABLE_TO_SAMPLE_AT_16K_SETTING_TO_8K_TEXT,
	CUSTOM_CURVE_TEXT,
	STEP_THRESHOLD_TEXT,
	STEP_LIMITING_TEXT,
	FREQ_ALGORITHM_TEXT,
	HALF_WAVE_APPROX_TEXT,
	FULL_WAVE_COUNT_TEXT,
	DIAL_OUT_TYPE_TEXT,
	DIAL_OUT_CYCLE_TIME_TEXT,
	EVENTS_CONFIG_STATUS_TEXT,
	EVENTS_ONLY_TEXT,
	GPS_POWER_TEXT,
	NORMAL_SAVE_POWER_TEXT,
	ALWAYS_ON_ACQUIRING_TEXT,
	UTC_ZONE_OFFSET_TEXT,
	ADAPTIVE_SAMPLING_TEXT,
	SAMPLING_METHOD_TEXT,
	FIXED_NORMAL_TEXT,
	ADAPTIVE_SAVE_BATT_TEXT,
	SHOW_OPTION_IN_SETUP_TEXT,
	HIDE_OPTION_DISABLE_TEXT,
	MIC_5_PSI_TEXT,
	MIC_10_PSI_TEXT,
	PSI_TEXT,
	PERCENT_OF_LIMIT_TRIGGER_TEXT,
	NORMAL_TEXT,
	ACC_INT_8G_TEXT,
	ACC_INT_16G_TEXT,
	ACC_INT_32G_TEXT,
	ACC_INT_64G_TEXT,
	AUX_CHARGING_BYPASS_TEXT,
	NULL_TEXT,
	TOTAL_TEXT_STRINGS
};

#endif // _TEXT_TYPES_H_
