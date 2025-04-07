///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include "Typedefs.h"
#include "Analog.h"
#include "Summary.h"
#include "Common.h"
#include "RealTimeClock.h"
#include "Record.h"
#include "RemoteCommon.h"
#include "Menu.h"
#include "InitDataBuffers.h"
#include "Display.h"
#include "EventProcessing.h"
#include "SysEvents.h"
#include "Keypad.h"
#include "ProcessBargraph.h"
#include "OldUart.h"
#include "Sensor.h"
#include "TextTypes.h"
#include "ff.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Globals
///----------------------------------------------------------------------------

OFFSET_DATA_STRUCT g_channelOffset;
INPUT_MSG_STRUCT g_input_buffer[INPUT_BUFFER_SIZE];
//char g_appVersion[16];
//char g_appDate[16];
//char g_appTime[16];
MONTH_TABLE_STRUCT g_monthTable[] = { {0, "\0\0\0\0", 0}, {JAN, "JAN\0", 31}, {FEB, "FEB\0", 28}, {MAR, "MAR\0", 31}, {APR, "APR\0", 30}, {MAY, "MAY\0", 31},
	{JUN, "JUN\0", 30}, {JUL, "JUL\0", 31}, {AUG, "AUG\0", 31}, {SEP, "SEP\0", 30}, {OCT, "OCT\0", 31}, {NOV, "NOV\0", 30}, {DEC, "DEC\0", 31} }; // 156B
/* Original top left, row x column */
uint8 g_mmap[LCD_NUM_OF_ROWS][LCD_NUM_OF_BIT_COLUMNS]; // 1K
#if 1 /* New bottom left, scan line x heigth */
uint8 g_bitmap[LCD_MAP_SIZE_IN_BYTES]; // 1K
#endif
uint8 g_contrast_value;
uint8 g_powerSavingsForSleepEnabled = NO;
uint16 g_nextEventNumberToUse = 1;
uint32 __monitorLogTblKey;
uint16 __monitorLogTblIndex;
uint16 __monitorLogUniqueEntryId;
MONITOR_LOG_ENTRY_STRUCT __monitorLogTbl[TOTAL_MONITOR_LOG_ENTRIES]; // ~4K
uint32 __autoDialoutTblKey;
AUTODIALOUT_STRUCT __autoDialoutTbl;
uint32 g_eventNumberCacheValidKey;
uint16 g_eventNumberCacheValidEntries;
uint16 g_eventNumberCacheMaxIndex;
uint16 g_eventNumberCacheOldestIndex;
uint8 g_eventNumberCache[EVENT_NUMBER_CACHE_MAX_ENTRIES]; // 65K
uint16 g_pretriggerBuff[PRE_TRIG_BUFF_SIZE_IN_WORDS]; // ~131K
uint16* g_startOfPretriggerBuff;
uint16* g_tailOfPretriggerBuff;
uint16* g_endOfPretriggerBuff;
uint16 g_maxEventBuffers;
uint16 g_freeEventBuffers;
uint16 g_calTestExpected;
uint16 g_adChannelConfig;
uint32 g_samplesInBody;
uint32 g_samplesInPretrig;
uint32 g_samplesInCal;
uint32 g_samplesInEvent;
uint32 g_wordSizeInPretrig;
uint32 g_wordSizeInBody;
uint32 g_wordSizeInCal;
uint32 g_wordSizeInEvent;
uint16* g_startOfEventBufferPtr;
uint16* g_eventBufferPretrigPtr;
uint16* g_eventBufferBodyPtr;
uint16* g_eventBufferCalPtr;
uint16* g_delayedOneEventBufferCalPtr;
uint16* g_delayedTwoEventBufferCalPtr;
uint16* g_currentEventSamplePtr;
uint16* g_currentEventStartPtr;
uint16 g_eventBufferReadIndex;
uint16 g_bitShiftForAccuracy;
uint32 g_isTriggered = 0;
uint32 g_processingCal = 0;
uint16 g_testCounter = 0;
uint16* g_bargraphDataStartPtr;
uint16* g_bargraphDataWritePtr;
uint16* g_bargraphDataReadPtr;
uint16* g_bargraphDataEndPtr;
uint8 g_powerNoiseFlag = PRINTER_OFF;
uint8 g_doneTakingEvents = NO;
uint8 g_busyProcessingEvent = NO;
uint8 g_sampleProcessing = IDLE_STATE;
uint8 g_modemConnected = NO;
uint8 g_lcdBacklightFlag = ENABLED;
#if LCD_RESOURCE_UNAVAILABLE
uint8 g_lcdPowerFlag = DISABLED;
#else /* LCD available*/
uint8 g_lcdPowerFlag = ENABLED;
#endif
uint8 g_kpadProcessingFlag = DEACTIVATED;
uint8 g_kpadCheckForKeyFlag = DEACTIVATED;
uint8 g_factorySetupSequence = SEQ_NOT_STARTED;
uint8 g_kpadInterruptWhileProcessing = 0;
uint16 g_kpadLastKeyPressed = KEY_NONE;
uint16 g_kpadLastKeymap = 0;
uint16 g_kpadIsrKeymap = 0;
uint16 g_dynamicSoftKeyLayout[4];
volatile uint32 g_keypadTimerTicks = 0;
volatile uint32 g_msTimerTicks = 0;
uint32 g_kpadKeyRepeatCount = 0;
uint32 g_kpadDelayTickCount = 0;
uint32 g_keypadNumberSpeed = 1;
uint16 g_keypadTable[9] = {KB_SK_4, KB_SK_3, KB_SK_2, KB_SK_1, KB_ENTER, KB_RIGHT, KB_LEFT, KB_DOWN, KB_UP};
SENSOR_PARAMETERS_STRUCT g_sensorInfo;
EVT_RECORD g_pendingEventRecord;
EVT_RECORD g_pendingBargraphRecord;
EVT_RECORD g_resultsEventCache[50]; // ~34K
uint16 g_resultsCacheIndex = 0;
FACTORY_SETUP_STRUCT g_factorySetupRecord;
FACTORY_SETUP_STRUCT g_shadowFactorySetupRecord = {0xFFFF};
REC_EVENT_MN_STRUCT g_triggerRecord;
uint8 g_externalTriggerMenuActiveForSetup;
uint8 g_activeMenu;
MN_EVENT_STRUCT g_menuEventFlags = {0};
MN_TIMER_STRUCT g_timerEventFlags = {0};
SYS_EVENT_STRUCT g_systemEventFlags = {0};
MODEM_SETUP_STRUCT g_modemSetupRecord;
MODEM_STATUS_STRUCT g_modemStatus;
CMD_BUFFER_STRUCT g_isrMessageBufferStruct;
CMD_BUFFER_STRUCT* g_isrMessageBufferPtr = &g_isrMessageBufferStruct;
void (*g_menufunc_ptrs[TOTAL_NUMBER_OF_MENUS])(INPUT_MSG_STRUCT) = {MainMenu, LoadRecordMenu, SummaryMenu, MonitorMenu, ResultsMenu, OverwriteMenu,
	BatteryMn, DateTimeMn, LcdContrastMn, TimerModeTimeMenu, TimerModeDateMenu, CalSetupMn, UserMenu, MonitorLogMn};
MN_MEM_DATA_STRUCT g_menuPtr[DEFAULT_MN_SIZE]; // 500B
USER_MENU_TAGS_STRUCT g_menuTags[TOTAL_TAGS] = {
	{"",	NO_TAG},
	{"1. ",	ITEM_1},
	{"2. ",	ITEM_2},
	{"3. ",	ITEM_3},
	{"4. ",	ITEM_4},
	{"5. ",	ITEM_5},
	{"6. ",	ITEM_6},
	{"7. ",	ITEM_7},
	{"8. ",	ITEM_8},
	{"9. ",	ITEM_9},
	{"-=",	MAIN_PRE_TAG},
	{"=-",	MAIN_POST_TAG},
	{"-",	TITLE_PRE_TAG},
	{"-",	TITLE_POST_TAG},
	{"",	LOW_SENSITIVITY_MAX_TAG},
	{"",	HIGH_SENSITIVITY_MAX_TAG},
	{"",	BAR_SCALE_FULL_TAG},
	{"",	BAR_SCALE_HALF_TAG},
	{"",	BAR_SCALE_QUARTER_TAG},
	{"",	BAR_SCALE_EIGHTH_TAG},
	{" ENABLED",	ENABLED_TAG},
	{" DISABLED",	DISABLED_TAG},
	{"",	FILENAME_TAG},
	{"REV PROTOTYPE 1",		PROTOTYPE_1_TAG},
	{"DUMP BATTERY LOG", DUMP_BATTERY_LOG_TAG},
	{"BATTERY LOG TIMER", BATTERY_LOG_TAG},
	{" (A)", ALTERNATE_TAG},
	{"FCC TEST ALL", FCC_TEST_ALL_TAG}
};
uint8 g_monitorOperationMode;
uint8 g_waitForUser = FALSE;
uint8 g_promtForLeavingMonitorMode = FALSE;
uint8 g_promtForCancelingPrintJobs = FALSE;
uint8 g_monitorModeActiveChoice = MB_FIRST_CHOICE;
uint8 g_monitorEscapeCheck = YES;
uint8 g_displayBargraphResultsMode = SUMMARY_INTERVAL_RESULTS;
uint8 g_displayAlternateResultState = DEFAULT_RESULTS;
uint8 g_calDisplayAlternateResultState = DEFAULT_RESULTS;
uint8 g_allowQuickPowerOffForTimerModeSetup = NO;
uint32 g_barSampleCount = 0;
uint32 g_blmBarIntervalQueueCount = 0;
uint16 g_summaryCount = 0;
BARGRAPH_FREQ_CALC_BUFFER g_bargraphFreqCalcBuffer;
CALCULATED_DATA_STRUCT g_bargraphSummaryInterval;
BARGRAPH_BAR_INTERVAL_DATA* g_bargraphBarIntervalWritePtr;
BARGRAPH_BAR_INTERVAL_DATA* g_bargraphBarIntervalReadPtr;
BARGRAPH_BAR_INTERVAL_DATA* g_bargraphBarIntervalLiveMonitoringReadPtr;
BARGRAPH_BAR_INTERVAL_DATA* g_bargraphBarIntervalEndPtr;
uint32 g_bargraphBarIntervalClock = 0;
uint16 g_bargraphBarIntervalsCached = 0;
uint16 g_bitAccuracyMidpoint = 0x8000;
uint16 g_aImpulsePeak;
uint16 g_rImpulsePeak;
uint16 g_vImpulsePeak;
uint16 g_tImpulsePeak;
uint32 g_vsImpulsePeak;
uint16 g_impulseMenuCount;
uint16 g_aJobPeak;
uint16 g_aJobFreq;
uint16 g_rJobPeak;
uint16 g_rJobFreq;
uint16 g_vJobPeak;
uint16 g_vJobFreq;
uint16 g_tJobPeak;
uint16 g_tJobFreq;
uint32 g_vsJobPeak;
uint16 g_manualCalSampleCount = 0;
uint8 g_manualCalFlag = FALSE;
uint8 g_forcedCalibration = NO;
uint8 g_autoRetries;
DATE_TIME_STRUCT g_lastReadExternalRtcTime;
SOFT_TIMER_STRUCT g_rtcTimerBank[NUM_OF_SOFT_TIMERS];
uint32 g_lifetimeHalfSecondTickCount = 0;
volatile uint32 g_rtcTickCountSinceLastExternalUpdate = 0;
uint32 g_updateCounter = 0;
UNIT_CONFIG_STRUCT g_unitConfig;
uint8 g_disableDebugPrinting;
uint8 g_autoCalDaysToWait = 0;
uint8 g_autoDialoutState = AUTO_DIAL_IDLE;
uint8 g_modemDataTransfered = NO;
#if ENDIAN_CONVERSION
uint16 g_CRLF = 0x0A0D;
#else
uint16 g_CRLF = 0x0D0A;
#endif
CMD_BUFFER_STRUCT g_msgPool[CMD_MSG_POOL_SIZE];
DEMx_XFER_STRUCT g_demXferStruct;
DSMx_XFER_STRUCT g_dsmXferStruct;
DQMx_XFER_STRUCT g_dqmXferStruct;
COMMAND_MESSAGE_HEADER g_modemInputHeaderStruct;
COMMAND_MESSAGE_HEADER g_modemOutputHeaderStruct;
DEMx_XFER_STRUCT* g_demXferStructPtr = &g_demXferStruct;
DSMx_XFER_STRUCT* g_dsmXferStructPtr = &g_dsmXferStruct;
DQMx_XFER_STRUCT* g_dqmXferStructPtr = &g_dqmXferStruct;
COMMAND_MESSAGE_HEADER* g_inCmdHeaderPtr = &g_modemInputHeaderStruct;
COMMAND_MESSAGE_HEADER* g_outCmdHeaderPtr = &g_modemOutputHeaderStruct;
uint32 g_transferCount;
uint32 g_transmitCRC;
uint8 g_binaryXferFlag = CONVERT_DATA_TO_ASCII;
uint8 g_modemResetStage = 0;
uint8 g_updateResultsEventRecord = NO;
uint16 g_resultsEventIndex;
uint32 g_summaryEventNumber;
uint8 g_summaryListMenuActive = NO;
uint8 g_summaryListArrowChar = BOTH_ARROWS_CHAR;
volatile uint8 g_i2c1AccessLock = AVAILABLE;
volatile uint8 g_fileAccessLock = AVAILABLE;
volatile uint8 g_externalTrigger = NO;
uint8 g_lcdContrastChanged = NO;
uint32 g_cyclicEventDelay = 0;
uint32 g_updateOffsetCount = 0;
USER_MENU_CACHE_STRUCT g_userMenuCache[MAX_MENU_ENTRIES]; // 1.3K
USER_MENU_CACHE_STRUCT* g_userMenuCachePtr = &g_userMenuCache[0];
USER_MENU_CACHE_DATA g_userMenuCacheData;
void (*g_userMenuHandler)(uint16, void*);
uint8* g_compressedDataOutletPtr;
uint16 g_eventBufferWriteIndex;
uint8 g_timerModeLastRun = NO;
uint8 g_tcTypematicTimerActive = NO;
uint8 g_lowBatteryState = NO;
uint8 g_spi2State = 0;
uint32 g_sleepModeState = 0;
char* g_languageLinkTable[TOTAL_TEXT_STRINGS]; // 2.1K
uint32 g_tempTriggerLevelForMenuAdjustment;
volatile uint16 g_storedTempReading;
volatile uint16 g_currentTempReading;
volatile uint16 g_previousTempReading;
uint16 g_debugBufferCount = 0;
uint32 g_alarmOneSeismicMinLevel;
uint32 g_alarmOneAirMinLevel;
uint32 g_alarmTwoSeismicMinLevel;
uint32 g_alarmTwoAirMinLevel;
char g_languageTable[LANGUAGE_TABLE_MAX_SIZE]; // 16K
uint8 g_spareBuffer[SPARE_BUFFER_SIZE]; // 8K
uint32 g_spareBufferIndex = 0;
char g_spareFileName[MAX_FILE_NAME_CHARS];
char g_eventPathAndFilename[MAX_FILE_NAME_CHARS];
uint8 g_debugBuffer[(MAX_TEXT_LINE_CHARS + 1)];
uint8 g_blmBuffer[(MAX_TEXT_LINE_CHARS + 1)];
EVENT_TIMESTAMP_STRUCT g_eventDateTimeStampBuffer[MAX_EVENT_TIMESTAMP_BUFFERS]; // 340B
//uint16 g_eventDataBuffer[EVENT_BUFF_SIZE_IN_WORDS_PLUS_EVT_RECORD_PLUS_EVENT_LIST];
uint16 g_eventDataBuffer[EVENT_BUFF_SIZE_IN_WORDS_PLUS_EVT_RECORD]; // ~600K
EVENT_LIST_ENTRY_STRUCT g_eventListCache[EVENT_LIST_CACHE_ENTRIES_LIMIT]; // 32K
uint8 g_serialNumberCache[SERIAL_NUMBER_CACHE_ENTRIES][SERIAL_NUMBER_STRING_SIZE]; // 5K
uint32 g_execCycles = 0;
ACC_DATA_STRUCT g_accDataCache;
FLASH_USAGE_STRUCT g_sdCardUsageStats;
SUMMARY_LIST_FILE_DETAILS g_summaryList;
volatile uint32 g_sampleCount = 0;
uint32 g_sampleCountHold = 0;
uint8 g_channelSyncError = NO;
uint8 g_powerOffActivated = NO;
uint8 g_powerOffAttempted = NO;
uint8 g_usbMassStorageState = USB_INIT_DRIVER;
uint8 g_usbMode;
uint8 g_usbThumbDriveWasConnected = NO;
uint8 g_syncFileExistsAction = 0;
uint8 g_spi2InUseByLCD = NO;
uint8 g_calibrationGeneratePulse = NO;
uint8 g_bargraphLiveMonitoringBISendActive = NO;
uint8 g_blmAlertAlarmStatus = 0;
uint8* g_bargraphBarIntervalLiveMonitorBIDataPtr = g_blmBuffer;
SAMPLE_DATA_STRUCT g_sensorCalPeaks[3] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};
SAMPLE_DATA_STRUCT g_sensorCalFreqCounts;
int32 g_sensorCalChanMin[MAX_NUM_OF_CHANNELS];
int32 g_sensorCalChanMax[MAX_NUM_OF_CHANNELS];
int32 g_sensorCalChanAvg[MAX_NUM_OF_CHANNELS];
uint32 g_sensorCalChanMed[MAX_NUM_OF_CHANNELS][8];
SMART_SENSOR_ROM g_seismicSmartSensorRom;
SMART_SENSOR_ROM g_seismic2SmartSensorRom;
SMART_SENSOR_ROM g_acousticSmartSensorRom;
SMART_SENSOR_ROM g_acoustic2SmartSensorRom;
SMART_SENSOR_STRUCT g_seismicSmartSensorMemory;
SMART_SENSOR_STRUCT g_seismic2SmartSensorMemory;
SMART_SENSOR_STRUCT g_acousticSmartSensorMemory;
SMART_SENSOR_STRUCT g_acoustic2SmartSensorMemory;
WORKING_CAL_DATE_STRUCT g_currentCalibration;
FIL* g_globalFileHandle;
uint8 g_quickBootEntryJump = NO;
uint8 g_breakpointCause = 0;
uint8 g_currentSensorGroup = SENSOR_GROUP_A_1;
GPS_SERIAL_DATA g_gpsSerialData;
GPS_QUEUE g_gpsQueue;
GPS_BINARY_QUEUE g_gpsBinaryQueue;
GPS_POSITION g_gpsPosition = {0, 0, 0, 0, 0, 0, '-', '-', 0, 0, 0, 0, 0, NO, 0, 0, 0, 0};
uint8 g_gpsOutputToCraft = NO;
uint8 g_adaptiveState = ADAPTIVE_DISABLED;
uint8 g_adaptiveBoundaryCount;
uint8 g_adaptiveBoundaryMarker = 0;
uint16 g_adaptiveSeismicThreshold;
uint16 g_adaptiveAcousticThreshold;
uint32 g_adaptiveSampleDelay;
uint16* g_adaptiveLastRealSamplePtr;
time_t g_epochTimeGPS = 0;
uint32 g_testTimeSinceLastFSWrite = 0xffffffff;
uint32 g_testTimeSinceLastTrigger = 0xffffffff;
uint32 g_testTimeSinceLastCycleChange = 0xffffffff;
uint32 g_testTimeSinceLastCalPulse = 0xffffffff;

// End of the list
