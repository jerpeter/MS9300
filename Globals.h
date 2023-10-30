///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

#ifndef GLOBALS_H_
#define GLOBALS_H_

// Global Includes ---------------------------------------------------------------------

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
#include "Uart.h"
#include "Sensor.h"
#include "TextTypes.h"

// Global Externs ---------------------------------------------------------------------

extern ANALOG_CONTROL_STRUCT g_analogControl;
extern OFFSET_DATA_STRUCT g_channelOffset;
extern INPUT_MSG_STRUCT g_input_buffer[INPUT_BUFFER_SIZE];
extern MONTH_TABLE_STRUCT g_monthTable[];
extern uint8 g_mmap[LCD_NUM_OF_ROWS][LCD_NUM_OF_BIT_COLUMNS];
extern uint8 g_contrast_value;
extern uint8 g_powerSavingsForSleepEnabled;
extern uint16 g_nextEventNumberToUse;
extern uint32 __monitorLogTblKey;
extern uint16 __monitorLogTblIndex;
extern uint16 __monitorLogUniqueEntryId;
extern MONITOR_LOG_ENTRY_STRUCT __monitorLogTbl[TOTAL_MONITOR_LOG_ENTRIES];
extern uint32 __autoDialoutTblKey;
extern AUTODIALOUT_STRUCT __autoDialoutTbl;
extern uint32 g_eventNumberCacheValidKey;
extern uint16 g_eventNumberCacheValidEntries;
extern uint16 g_eventNumberCacheMaxIndex;
extern uint16 g_eventNumberCacheOldestIndex;
extern uint8 g_eventNumberCache[EVENT_NUMBER_CACHE_MAX_ENTRIES];
extern uint16 g_pretriggerBuff[PRE_TRIG_BUFF_SIZE_IN_WORDS];
extern uint16* g_startOfPretriggerBuff;
extern uint16* g_tailOfPretriggerBuff;
extern uint16* g_endOfPretriggerBuff;
extern uint16 g_maxEventBuffers;
extern uint16 g_freeEventBuffers;
extern uint16 g_calTestExpected;
extern uint16 g_adChannelConfig;
extern uint32 g_samplesInBody;
extern uint32 g_samplesInPretrig;
extern uint32 g_samplesInCal;
extern uint32 g_samplesInEvent;
extern uint32 g_wordSizeInPretrig;
extern uint32 g_wordSizeInBody;
extern uint32 g_wordSizeInCal;
extern uint32 g_wordSizeInEvent;
extern uint16* g_startOfEventBufferPtr;
extern uint16* g_eventBufferPretrigPtr;
extern uint16* g_eventBufferBodyPtr;
extern uint16* g_eventBufferCalPtr;
extern uint16* g_delayedOneEventBufferCalPtr;
extern uint16* g_delayedTwoEventBufferCalPtr;
extern uint16* g_currentEventSamplePtr;
extern uint16* g_currentEventStartPtr;
extern uint16 g_eventBufferReadIndex;
extern uint16 g_bitShiftForAccuracy;
extern uint32 g_isTriggered;
extern uint32 g_processingCal;
extern uint16 g_testCounter;
extern uint16* g_bargraphDataStartPtr;
extern uint16* g_bargraphDataWritePtr;
extern uint16* g_bargraphDataReadPtr;
extern uint16* g_bargraphDataEndPtr;
extern uint8 g_powerNoiseFlag;
extern uint8 g_doneTakingEvents;
extern uint8 g_busyProcessingEvent;
extern uint8 g_sampleProcessing;
extern uint8 g_modemConnected;
extern uint8 g_lcdBacklightFlag;
extern uint8 g_lcdPowerFlag;
extern uint8 g_kpadProcessingFlag;
extern uint8 g_kpadCheckForKeyFlag;
extern uint8 g_factorySetupSequence;
extern uint8 g_kpadLastKeyPressed;
extern uint8 g_kpadInterruptWhileProcessing;
extern uint8 g_kpadLastKeymap;
extern uint8 g_allowQuickPowerOffForTimerModeSetup;
extern volatile uint32 g_keypadTimerTicks;
extern volatile uint32 g_msTimerTicks;
extern uint32 g_kpadKeyRepeatCount;
extern uint32 g_kpadDelayTickCount;
extern uint32 g_keypadNumberSpeed;
extern uint8 g_keypadTable[8][8];
extern unsigned char g_smc_tab_cs_size[4];
extern SENSOR_PARAMETERS_STRUCT g_sensorInfo;
extern EVT_RECORD g_pendingEventRecord;
extern EVT_RECORD g_pendingBargraphRecord;
extern FACTORY_SETUP_STRUCT g_factorySetupRecord;
extern REC_EVENT_MN_STRUCT g_triggerRecord;
extern uint8 g_externalTriggerMenuActiveForSetup;
extern uint8 g_activeMenu;
extern MN_EVENT_STRUCT g_menuEventFlags;
extern MN_TIMER_STRUCT g_timerEventFlags;
extern SYS_EVENT_STRUCT g_systemEventFlags;
extern MODEM_SETUP_STRUCT g_modemSetupRecord;
extern MODEM_STATUS_STRUCT g_modemStatus;
extern CMD_BUFFER_STRUCT g_isrMessageBufferStruct;
extern CMD_BUFFER_STRUCT* g_isrMessageBufferPtr;
extern void (*g_menufunc_ptrs[TOTAL_NUMBER_OF_MENUS]) (INPUT_MSG_STRUCT);
extern MN_MEM_DATA_STRUCT g_menuPtr[DEFAULT_MN_SIZE];
extern USER_MENU_TAGS_STRUCT g_menuTags[TOTAL_TAGS];
extern uint8 g_monitorOperationMode;
extern uint8 g_waitForUser;
extern uint8 g_promtForLeavingMonitorMode;
extern uint8 g_promtForCancelingPrintJobs;
extern uint8 g_monitorModeActiveChoice;
extern uint8 g_monitorEscapeCheck;
extern uint8 g_displayBargraphResultsMode;
extern uint8 g_displayAlternateResultState;
extern uint8 g_calDisplayAlternateResultState;
extern uint32 g_barSampleCount;
extern uint32 g_blmBarIntervalQueueCount;
extern uint16 g_summaryCount;
extern BARGRAPH_FREQ_CALC_BUFFER g_bargraphFreqCalcBuffer;
extern CALCULATED_DATA_STRUCT g_bargraphSummaryInterval;
extern BARGRAPH_BAR_INTERVAL_DATA* g_bargraphBarIntervalWritePtr;
extern BARGRAPH_BAR_INTERVAL_DATA* g_bargraphBarIntervalReadPtr;
extern BARGRAPH_BAR_INTERVAL_DATA* g_bargraphBarIntervalLiveMonitoringReadPtr;
extern BARGRAPH_BAR_INTERVAL_DATA* g_bargraphBarIntervalEndPtr;
extern time_t g_bargraphBarIntervalClock;
extern uint16 g_bargraphBarIntervalsCached;
extern uint16 g_bitAccuracyMidpoint;
extern uint16 g_aImpulsePeak;
extern uint16 g_rImpulsePeak;
extern uint16 g_vImpulsePeak;
extern uint16 g_tImpulsePeak;
extern uint32 g_vsImpulsePeak;
extern uint16 g_impulseMenuCount;
extern uint16 g_aJobPeak;
extern uint16 g_aJobFreq;
extern uint16 g_rJobPeak;
extern uint16 g_rJobFreq;
extern uint16 g_vJobPeak;
extern uint16 g_vJobFreq;
extern uint16 g_tJobPeak;
extern uint16 g_tJobFreq;
extern uint32 g_vsJobPeak;
extern uint16 g_manualCalSampleCount;
extern uint8 g_manualCalFlag;
extern uint8 g_forcedCalibration;
extern uint8 g_autoRetries;
extern DATE_TIME_STRUCT g_lastReadExternalRtcTime;
extern SOFT_TIMER_STRUCT g_rtcTimerBank[NUM_OF_SOFT_TIMERS];
extern uint32 g_lifetimeHalfSecondTickCount;
extern volatile uint32 g_rtcTickCountSinceLastExternalUpdate;
extern uint32 g_updateCounter;
extern uint8 g_autoCalDaysToWait;
extern UNIT_CONFIG_STRUCT g_unitConfig;
extern uint8 g_autoDialoutState;
extern uint8 g_modemDataTransfered;
extern uint16 g_CRLF;
extern CMD_BUFFER_STRUCT g_msgPool[CMD_MSG_POOL_SIZE];
extern DERx_XFER_STRUCT g_derXferStruct;
extern DEMx_XFER_STRUCT* g_demXferStructPtr;
extern DSMx_XFER_STRUCT* g_dsmXferStructPtr;
extern DQMx_XFER_STRUCT* g_dqmXferStructPtr;
extern COMMAND_MESSAGE_HEADER* g_inCmdHeaderPtr;
extern COMMAND_MESSAGE_HEADER* g_outCmdHeaderPtr;
extern uint32 	g_transferCount;
extern uint32 	g_transmitCRC;
extern uint8	g_binaryXferFlag;
extern uint8 g_modemResetStage;
extern uint8 g_updateResultsEventRecord;
extern uint16 g_resultsEventIndex;
extern uint32 g_summaryEventNumber;
extern uint8 g_summaryListMenuActive;
extern uint8 g_summaryListArrowChar;
extern uint8 g_disableDebugPrinting;
extern uint8 g_lowBatteryState;
extern USER_MENU_CACHE_STRUCT g_userMenuCache[36];
extern USER_MENU_CACHE_STRUCT* g_userMenuCachePtr;
extern USER_MENU_CACHE_DATA g_userMenuCacheData;
extern void (*g_userMenuHandler)(uint8, void*);
extern uint8* g_compressedDataOutletPtr;
extern uint16 g_eventBufferWriteIndex;
extern uint8 g_spareBuffer[SPARE_BUFFER_SIZE];
extern uint32 g_spareBufferIndex;
extern char g_spareFileName[MAX_FILE_NAME_CHARS];
extern char g_eventPathAndFilename[MAX_FILE_NAME_CHARS];
extern uint8 g_debugBuffer[(MAX_TEXT_LINE_CHARS + 1)];
extern uint16 g_debugBufferCount;
extern uint8 g_blmBuffer[(MAX_TEXT_LINE_CHARS + 1)];
extern uint32 g_alarmOneSeismicMinLevel;
extern uint32 g_alarmOneAirMinLevel;
extern uint32 g_alarmTwoSeismicMinLevel;
extern uint32 g_alarmTwoAirMinLevel;
extern uint8 g_derCache[DER_CACHE_SIZE];
extern uint16 g_derCacheIndex;
extern uint8 g_timerModeLastRun;
extern uint8 g_tcSampleTimerActive;
extern uint8 g_tcTypematicTimerActive;
extern char* g_languageLinkTable[TOTAL_TEXT_STRINGS];
extern char g_languageTable[LANGUAGE_TABLE_MAX_SIZE];
extern uint32 g_sleepModeState;
extern uint8 g_sleepModeEngaged;
extern volatile uint8 g_spi1AccessLock;
extern volatile uint8 g_fileAccessLock;
extern volatile uint8 g_externalTrigger;
extern uint8 g_lcdContrastChanged;
extern EVT_RECORD g_resultsEventCache[50];
extern uint16 g_resultsCacheIndex;
extern uint32 g_cyclicEventDelay;
extern uint32 g_updateOffsetCount;
extern uint32 g_tempTriggerLevelForMenuAdjustment;
extern FLASH_USAGE_STRUCT g_sdCardUsageStats;
extern SUMMARY_LIST_FILE_DETAILS g_summaryList;
extern volatile uint16 g_storedTempReading;
extern volatile uint16 g_currentTempReading;
extern volatile uint16 g_previousTempReading;
extern EVENT_TIMESTAMP_STRUCT g_eventDateTimeStampBuffer[MAX_EVENT_TIMESTAMP_BUFFERS];
extern uint16 g_eventDataBuffer[EVENT_BUFF_SIZE_IN_WORDS_PLUS_EVT_RECORD_PLUS_EVENT_LIST];
extern uint8 g_serialNumberCache[SERIAL_NUMBER_CACHE_ENTRIES][SERIAL_NUMBER_STRING_SIZE];
extern const char g_buildVersion[];
extern const char g_buildDate[];
extern uint32 g_execCycles;
extern uint8 g_channelSyncError;
extern volatile uint32 g_sampleCount;
extern uint32 g_sampleCountHold;
extern uint8 g_powerOffActivated;
extern uint8 g_usbMassStorageState;
extern uint8 g_usbMode;
extern uint8 g_usbThumbDriveWasConnected;
extern uint8 g_syncFileExistsAction;
extern uint8 g_calibrationGeneratePulse;
extern uint8 g_bargraphLiveMonitoringBISendActive;
extern uint8 g_blmAlertAlarmStatus;
extern uint8* g_bargraphBarIntervalLiveMonitorBIDataPtr;
extern SAMPLE_DATA_STRUCT g_sensorCalPeaks[];
extern SAMPLE_DATA_STRUCT g_sensorCalFreqCounts;
extern int32 g_sensorCalChanMin[MAX_NUM_OF_CHANNELS];
extern int32 g_sensorCalChanMax[MAX_NUM_OF_CHANNELS];
extern int32 g_sensorCalChanAvg[MAX_NUM_OF_CHANNELS];
extern uint32 g_sensorCalChanMed[MAX_NUM_OF_CHANNELS][8];
extern SMART_SENSOR_ROM g_seismicSmartSensorRom;
extern SMART_SENSOR_ROM g_acousticSmartSensorRom;
extern SMART_SENSOR_STRUCT g_seismicSmartSensorMemory;
extern SMART_SENSOR_STRUCT g_acousticSmartSensorMemory;
extern WORKING_CAL_DATE_STRUCT g_currentCalibration;
extern int g_globalFileHandle;
extern uint8 g_quickBootEntryJump;
extern uint8 g_breakpointCause;
extern GPS_SERIAL_DATA g_gpsSerialData;
extern GPS_QUEUE g_gpsQueue;
extern GPS_BINARY_QUEUE g_gpsBinaryQueue;
extern GPS_POSITION g_gpsPosition;
extern uint8 g_gpsOutputToCraft;
extern uint8 g_adaptiveState;
extern uint8 g_adaptiveBoundaryCount;
extern uint8 g_adaptiveBoundaryMarker;
extern uint16 g_adaptiveSeismicThreshold;
extern uint16 g_adaptiveAcousticThreshold;
extern uint32 g_adaptiveSampleDelay;
extern uint16* g_adaptiveLastRealSamplePtr;
extern time_t g_epochTimeGPS;
extern uint32 g_testTimeSinceLastFSWrite;
extern uint32 g_testTimeSinceLastTrigger;
extern uint32 g_testTimeSinceLastCycleChange;
extern uint32 g_testTimeSinceLastCalPulse;

#endif /* GLOBALS_H_ */