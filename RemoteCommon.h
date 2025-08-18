///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

#ifndef _REMOTE_COMMON_H_
#define _REMOTE_COMMON_H_

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include "Common.h"
#include "Summary.h"
#include "Record.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------

#define ATZ_CMD_STRING			"ATZ"
#define INIT_CMD_STRING			"AT"
#define INIT_CMD_STRING1		"ATS0=3S109=2&C1&K4\\X1"
#define ATA_CMD_STRING			"ATA"
#define ATH_CMD_STRING			"ATH"
#define CMDMODE_CMD_STRING		"+++"

#define OK_RESP_STRING			"OK"
#define READY_RESP_STRING		"READY"
#define READY_RESP_STRING_ALT	"REAdy"
#define CONNECT_RESP_STRING		"CONNECT"
#define CONNECT_RESP_STRING_2	"#XTCPCLI: 0,\"connected\""
#define TCP_CLIENT_STRING		"#XTCPCLI:"
#define TCP_SERVER_STRING		"#XTCPSVR:"
#define NO_ANSWER_RESP_STRING	"NO ANSWER"
#define RING_CMD_STRING			"RING"
#define SUPER_STATUS_STRING		"NOMIS SUPERGRAPH"
#define MINI_STATUS_STRING		"NOMIS MINIGRAPH"
#define UNL_USER_RESP_STRING	"UNLx81"
#define UNL_ADMIN_RESP_STRING	"UNLx82"
#define UNL_PROG_RESP_STRING	"UNLx83"
#define DAI_ERR_RESP_STRING		"DAIx0101"

#define AT_CMD_GET_SYSTEMMODE	"AT%%XSYSTEMMODE?"
#define AT_CMD_SET_PDP_CONTEXT	"AT+CGDCONT=0,\"IP\","
#define AT_CMD_PDP_CONTEXT		"AT+CGDCONT?"
#define AT_CMD_MODEM_START		"AT+CFUN=1"
#define AT_CMD_TCP_CLI_START	"AT#XTCPCLI=1,"
#define AT_CMD_TCP_CLI_STOP		"AT#XTCPCLI=0"
#define AT_CMD_TCP_SVR_START	"AT#XTCPSVR=1,"
#define AT_CMD_TCP_SVR_STOP		"AT#XTCPSVR=0"
#define AT_CMD_ENTER_DATA_MODE	"AT#XTCPSEND"
#define AT_CMD_DATAMODE_EXIT	"#XDATAMODE: 0"
#define MODEM_HOME_NETWORK		"+CEREG: 0,1"
#define MODEM_ROAMING_NETWORK	"+CEREG: 0,5"
#define AT_CMD_ERROR			"ERROR"
#define AT_CMD_XMODEM			"#XMODEM:"

#define CMD_MSG_NO_ERR				0x00
#define CMD_MSG_FULLBUFFER_ERR		0x01
#define CMD_MSG_MSGPOOL_ERR 		0x02
#define CMD_MSG_OVERFLOW_ERR		0x04
#define CMD_MSG_DEFAULT_4 			0x08
#define CMD_MSG_DEFAULT_5 			0x10
#define CMD_MSG_DEFAULT_6 			0x20
#define CMD_MSG_DEFAULT_7			0x40
#define CMD_MSG_READY_TO_PROCESS	0x80
#define CMD_MSG_ANY					0xFF

#define CMD_BUFFER_SIZE 			1024	// was 2048, Originally, 256

#define XMIT_SIZE_MONITORING		128		// 128
#define XMIT_SIZE_NOT_MONITORING	1024

#define CMD_MSG_POOL_SIZE 		5

#define UNLOCK_CODE_SIZE 		4
#define UNLOCK_FLAG_LOC			7
#define UNLOCK_STR_SIZE 		10
#define UNLOCK_CODE_DEFAULT		"0000"
#define UNLOCK_CODE_ADMIN		"3728"
#define UNLOCK_CODE_PROG		"ALGS"

#define CHAR_UPPER_CASE(DATA)						\
			if ((DATA >= 'a') && (DATA <= 'z'))		\
				DATA = (uint8)(DATA - 32);

//#ifndef CRLF
//	#define CRLF 0x0D0A
//#endif

// This is the length of the message header
#define MESSAGE_HEADER_LENGTH	38
#define MESSAGE_FOOTER_LENGTH	4

#define HDR_CMD_LEN 			4
#define HDR_TYPE_LEN 			2
#define HDR_DATALENGTH_LEN 		8
#define HDR_UNITMODEL_LEN		8
#define HDR_SERIALNUMBER_LEN 	8
#define HDR_COMPRESSCRC_LEN 	2
#define HDR_SOFTWAREVERSION_LEN 2
#define HDR_DATAVERSION_LEN 	2
#define HDR_SPARE_LEN			2

#define DATA_FIELD_LEN			6

#define DQM_XFER_SIZE			10

#define DQM_LEGACY_EVENT_LIMIT	800

#define VML_DATA_LOG_ENTRIES	4

#define START_DLOAD_FLAG	0xAABBBBAA		// Do not change - flags are used in the supergraphics app
#define END_DLOAD_FLAG		0xCCDDDDCC		// Do not change - flags are used in the supergraphics app

#define MESSAGE_HEADER_SIMPLE_LENGTH	HDR_CMD_LEN + HDR_TYPE_LEN + HDR_DATALENGTH_LEN + HDR_SPARE_LEN
#define MESSAGE_SIMPLE_TOTAL_LENGTH		MESSAGE_HEADER_SIMPLE_LENGTH + MESSAGE_FOOTER_LENGTH

enum {
	MSGTYPE_REQUEST 			= 0,
	MSGTYPE_RESPONSE			= 1,
	MSGTYPE_ERROR_MSGLENGTH		= 2,
	MSGTYPE_ERROR_NO_EVENT		= 3,
	MSGTYPE_ERROR_INVALID_DATA	= 4,
	MSGTYPE_RESPONSE_REV1		= 5
};


enum {
	COMPRESS_NOP			= 0,	// Starting value
	COMPRESS_NONE			= 1,	// Initial Release
	COMPRESS_MINILZO 		= 2,	// MINILZO Compression
	COMPRESS_NOMIS_v1		= 3,	// NOMIS v01 Compression
	COMPRESS_MININOMIS_v1	= 4		// BOth MINILZO and NOMIS v01
};

enum {
	CRC_NOP			= 0,	// Starting value
	CRC_NONE		= 1,	// Initial Release
	CRC_16BIT 		= 2,	//
	CRC_32BIT		= 3,	//
	CHECKSUM_8BIT	= 4,	//
	CHECKSUM_16BIT	= 5,	//
	CHECKSUM_32BIT	= 6		//
};


enum {
	CFG_ERR_NONE = 1,
	CFG_ERR_MSG_LENGTH = 2,
	CFG_ERR_MONITORING_STATE,
	CFG_ERR_SENSITIVITY = 7,		// 7
	CFG_ERR_SCALING,				// 8
	CFG_ERR_TRIGGER_MODE,			// 9
	CFG_ERR_DIST_TO_SRC,			// 10
	CFG_ERR_WEIGHT_DELAY,			// 11
	CFG_ERR_SAMPLE_RATE,			// 12
	CFG_ERR_SEISMIC_TRIG_LVL,		// 13
	CFG_ERR_SOUND_TRIG_LVL,			// 14
	CFG_ERR_RECORD_TIME,			// 15
	CFG_ERR_BAR_INTERVAL,			// 16
	CFG_ERR_SUM_INTERVAL,			// 17
	CFG_ERR_AUTO_MON_MODE,			// 18
	CFG_ERR_AUTO_CAL_MODE,			// 19
	CFG_ERR_AUTO_PRINT,				// 20
	CFG_ERR_LANGUAGE_MODE,			// 21
	CFG_ERR_VECTOR_SUM,				// 22
	CFG_ERR_UNITS_OF_MEASURE,		// 23
	CFG_ERR_FREQ_PLOT_MODE,			// 24
	CFG_ERR_FREQ_PLOT_TYPE,			// 25
	CFG_ERR_ALARM_ONE_MODE,			// 26
	CFG_ERR_ALARM_ONE_SEISMIC_LVL,	// 27
	CFG_ERR_ALARM_ONE_SOUND_LVL,	// 28
	CFG_ERR_ALARM_ONE_TIME,			// 29
	CFG_ERR_ALARM_TWO_MODE,			// 30
	CFG_ERR_ALARM_TWO_SEISMIC_LVL,	// 31
	CFG_ERR_ALARM_TWO_SOUND_LVL,	// 32
	CFG_ERR_ALARM_TWO_TIME,			// 33
	CFG_ERR_TIMER_MODE,				// 34
	CFG_ERR_TIMER_MODE_FREQ,		// 35
	CFG_ERR_TIMER_ACTIVE_MIN, 		// 36
	CFG_ERR_START_TIME,				// 37
	CFG_ERR_STOP_TIME,				// 38
	CFG_ERR_SYSTEM_TIME,			// 39
	CFG_ERR_SYSTEM_DATE,			// 40
	CFG_ERR_BAR_PRINT_CHANNEL,		// 41
	CFG_ERR_FLASH_WRAPPING,			// 42
	CFG_ERR_BAD_CRC,				// 43
	CFG_ERR_MODEM_CONFIG,			// 44
	CFG_ERR_PRETRIG_BUFFER_DIV,		// 45
	CFG_ERR_BIT_ACCURACY,			// 46
	CFG_ERR_A_WEIGHTING,			// 47
	CFG_ERR_TEMP_ADJUST,			// 48
	CFG_ERR_EXTERNAL_TRIGGER,		// 49
	CFG_ERR_RS232_POWER_SAVINGS,	// 50
	CFG_ERR_NO_TRIGGER_SOURCE,		// 51
	CFG_ERR_CHANNEL_VERIFICATION,	// 52
	CFG_ERR_BAD_SUBSET,				// 53
	CFG_ERR_BAD_DER_REQUEST,		// 54
	CFG_ERR_STORED_EVT_LIMIT_RANGE,	// 55
	CFG_ERR_END						// END
};

enum {
	NOP_CMD = 0,
	ATZ_CMD,
	INIT_CMD,
	CONNECTED,
	WAIT_FOR_CONNECT_CMD,

/*
	ATA_CMD,
	SYSTEM_ONLINE_CMD,
	ATZ2_CMD,

	WAIT_FOR_ATZ_OK_CMD,
	WAIT_FOR_INIT_OK_CMD,
	WAIT_FOR_ATA_OK_CMD,
	WAIT_FOR_RING_CMD,
*/

	NOP_XFER_STATE,
	HEADER_XFER_STATE,
	SUMMARY_TABLE_SEARCH_STATE,
	EVENTREC_XFER_STATE,
	DATA_XFER_STATE,
	FOOTER_XFER_STATE,

	CACHE_PACKETS_STATE,
	SEND_PACKETS_STATE,
	WAIT_REMOTE_STATUS_STATE,

	DEMx_CMD,
	DSMx_CMD,
	DQMx_CMD,
	VMLx_CMD,
	END_OF_COMMAND_STATE_LIST
};

// Auto Dialout States
enum {
	AUTO_DIAL_IDLE = 0,
	AUTO_DIAL_INIT,
	AUTO_DIAL_CONNECTING,
	AUTO_DIAL_CONNECTED,
	AUTO_DIAL_RESPONSE,
	AUTO_DIAL_WAIT,
	AUTO_DIAL_RESET,
	AUTO_DIAL_RESET_WAIT,
	AUTO_DIAL_RETRY,
	AUTO_DIAL_SLEEP,
	AUTO_DIAL_ACTIVE,
	AUTO_DIAL_FINISH
};

#define ADO_IDLE		"Idle"
#define ADO_INIT		"Init"
#define ADO_CONNECTING	"Connecting"
#define ADO_CONNECTED	"Connected"
#define ADO_RESPONSE	"Response"
#define ADO_WAIT		"Wait"
#define ADO_RESET		"Reset"
#define ADO_RESET_WAIT	"Reset Wait"
#define ADO_RETRY		"Retry"
#define ADO_SLEEP		"Sleep"
#define ADO_ACTIVE		"Active"
#define ADO_FINISH		"Finish"

#define TCP_SVR_IDLE	"Idle"
#define ADO_INIT		"Init"
#define ADO_CONNECTING	"Connecting"
#define ADO_CONNECTED	"Connected"
#define ADO_RESPONSE	"Response"
#define ADO_WAIT		"Wait"
#define ADO_RESET		"Reset"
#define ADO_RESET_WAIT	"Reset Wait"
#define ADO_RETRY		"Retry"
#define ADO_SLEEP		"Sleep"
#define ADO_ACTIVE		"Active"
#define ADO_FINISH		"Finish"

enum {
	NO_RESPONSE = 0,
	WAITING_FOR_STATUS = 1,
	ACKNOWLEDGE_PACKET,
	NACK_PACKET,
	CANCEL_COMMAND,
	ESCAPE_SEQUENCE,
	OK_RESPONSE,
	CONNECT_RESPONSE,
	READY_RESPONSE,
	MODEM_CELL_NETWORK_REGISTERED,
	DATAMODE_EXIT,
	TCP_CLIENT_DISCONNECT,
	TCP_CLIENT_NOT_CONNECTED,
	TCP_SERVER_STARTED,
	TCP_SERVER_NOT_STARTED,
	ERROR_RESPONSE,
	UNKNOWN_RESPONSE
};

enum {
	CSV_FULL = 0,
	CSV_SUMMARY,
	CSV_DATA,
	CSV_BARS
};

#define AUTODIALOUT_EVENTS_ONLY				0
#define AUTODIALOUT_EVENTS_CONFIG_STATUS	1

#define	FIELD_LEN_02			4
#define	FIELD_LEN_04			4
#define	FIELD_LEN_06			6
#define	FIELD_LEN_08			8

#define BAR_LIVE_MONITORING_OVERRIDE_STOP	2

#define REMOTE_SYSTEM_LOCK_TIMEOUT	(5 * TICKS_PER_MIN)

typedef struct
{
	uint8 modemAvailable;		// Flag to indicate modem is available.
	uint8 connectionState;		// State flag to indicate which modem command to handle.
	uint8 craftPortRcvFlag;		// Flag to indicate that incomming data is not modem commands.
	uint8 remoteResponse;

	uint8 xferState;			// Flag for xmitting data to the craft.
	uint8 xferMutex;			// Flag to stop other message command from executing.
	uint8 xferPrintState;		// Turn printing off when xfering data, Store the previous state.
	uint8 ringIndicator;

	uint8 systemIsLockedFlag;
	uint8 numberOfRings;
	uint8 firstConnection;
	uint8 barLiveMonitorOverride;

	uint8 remoteConnectionActive;
	uint8 spare;

} MODEM_STATUS_STRUCT;

// Command processing structure
typedef struct
{
	uint8 cmd[HDR_CMD_LEN + 1]; // 4
	uint8 type[HDR_TYPE_LEN + 1]; // 2
	uint8 dataLength[HDR_DATALENGTH_LEN + 1]; // 8
	uint8 unitModel[HDR_UNITMODEL_LEN + 1]; // 8
	uint8 unitSn[HDR_SERIALNUMBER_LEN + 1]; // 8
	uint8 compressCrcFlags[HDR_COMPRESSCRC_LEN + 1]; // 2
	uint8 softwareVersion[HDR_SOFTWAREVERSION_LEN + 1]; // 2
	uint8 dataVersion[HDR_DATAVERSION_LEN + 1]; // 2
	uint8 spare[HDR_SPARE_LEN + 1]; // 2
} COMMAND_MESSAGE_HEADER;

// Command processing structure
typedef struct
{
	uint16	size;			// Size of msg with data and CRLF.
	uint8	status;			// Status of buffer.

	uint8*	readPtr;		// Start of data in msg.
	uint8*	writePtr;		// Element in array to be written.
	uint8	msg[CMD_BUFFER_SIZE];
	uint16	overRunCheck;
} CMD_BUFFER_STRUCT;

#pragma pack(1)
typedef struct
{
	uint32				structureFlag;
	DATE_TIME_STRUCT	downloadDate;
	EVT_RECORD			eventRecord;
	uint32				endFlag;
} EVENT_RECORD_DOWNLOAD_STRUCT;
#pragma pack()

typedef struct
{
	uint8 	xferStateFlag;
	uint8 	msgHdr[MESSAGE_HEADER_LENGTH+1];
	EVENT_RECORD_DOWNLOAD_STRUCT	dloadEventRec;

	uint8* startDloadPtr;
	uint8* dloadPtr;
	uint8* endDloadPtr;

	uint8* startDataPtr;
	uint8* dataPtr;
	uint8* endDataPtr;
	uint8 errorStatus;
	uint8 downloadMethod;
	uint8 compressedEventDataFilePresent;
	uint8 	xmitBuffer[ CMD_BUFFER_SIZE ];
	uint32 	xmitSize;
} DEMx_XFER_STRUCT;

typedef struct
{
	uint8 xferStateFlag;
	uint8 msgHdr[MESSAGE_HEADER_LENGTH+1];
	uint8 numOfRecStr[DATA_FIELD_LEN+1];
	uint8 spare;
	EVENT_RECORD_DOWNLOAD_STRUCT	dloadEventRec;

	uint16	currentEventNumber;
	uint16	numOfSummaries;

	uint8* startDloadPtr;
	uint8* dloadPtr;
	uint8* endDloadPtr;

} DSMx_XFER_STRUCT;

#pragma pack(1)
typedef struct
{
	uint8 	dqmxFlag;			// CC
	uint8	mode;				// mode of the event
	uint16	eventNumber;		// event number
	uint8	serialNumber[SERIAL_NUMBER_STRING_SIZE];
	DATE_TIME_STRUCT	eventTime;
	uint8	subMode;			// EE
	uint8	endFlag;			// EE
	uint8	spare[2];			// Added spacing to match the NS7100
} DQMx_DATA_STRUCT;
#pragma pack()

typedef struct
{
	uint8 	xferStateFlag;
	uint8 	dqmHdr[MESSAGE_HEADER_SIMPLE_LENGTH+10];
	uint16	ramTableIndex;
	uint16	numOfRecs;
	DQMx_DATA_STRUCT dqmData[DQM_XFER_SIZE];
} DQMx_XFER_STRUCT;

typedef struct
{
	uint8 	xferStateFlag;
	uint8 	vmlHdr[MESSAGE_HEADER_SIMPLE_LENGTH];
	uint16	lastDlUniqueEntryId;
	uint16	startMonitorLogTableIndex;
	uint16	tempMonitorLogTableIndex;
	MONITOR_LOG_ENTRY_STRUCT vmlData[VML_DATA_LOG_ENTRIES];
} VMLx_XFER_STRUCT;

// Command processing structure

typedef struct
{
	uint8 	cmdChar1;
	uint8 	cmdChar2;
	uint8 	cmdChar3;
	void 	(* cmdFunction)(CMD_BUFFER_STRUCT *);

} COMMAND_MESSAGE_STRUCT;



////////////////////////////////////////////////////////////
// structs for transfering configuration data remotely
#pragma pack(1)
typedef struct
{
	uint8	autoMonitorMode;
	uint8	autoCalMode;
	uint8	externalTrigger;
	uint8	rs232PowerSavings;
} AUTO_CAL_MON_CFG;
#pragma pack()


#pragma pack(1)
typedef struct
{
	uint8 autoPrint;
	uint8 languageMode;
	uint8 vectorSum;
	uint8 unitsOfMeasure;
	uint8 freqPlotMode;
	uint8 freqPlotType;
	uint8 barChannel;
	uint8 barScale;
	uint8 sensitivity;
	uint8 unitsOfAir;
} EXTRA_UNIT_CFG;
#pragma pack()


#pragma pack(1)
typedef struct
{
	uint8 alarmOneMode;
	uint8 alarmTwoMode;
	uint32 alarmOneSeismicLevel;
	uint32 alarmOneSeismicMinLevel;
	uint32 alarmOneAirLevel;
	uint32 alarmOneAirMinLevel;
	uint32 alarmTwoSeismicLevel;
	uint32 alarmTwoSeismicMinLevel;
	uint32 alarmTwoAirLevel;
	uint32 alarmTwoAirMinLevel;
	uint32 alarmOneTime;
	uint32 alarmTwoTime;
	uint8 legacyDqmLimit;
	uint8 storedEventsCapMode;
	uint16 storedEventLimit;
} ALARM_CFG;
#pragma pack()


#pragma pack(1)
typedef struct
{
	uint8 	timerMode;
	uint8 	timerModeFrequency;
	uint8 	cycleEndTimeHour;
	uint8 	unused1;
	DATE_TIME_STRUCT timer_start;
	DATE_TIME_STRUCT timer_stop;
} TIMER_CFG;
#pragma pack()


#pragma pack(1)
typedef struct
{
	uint8				mode;
	uint8				monitorStatus;
	uint16				sgVersion;
	DATE_TIME_STRUCT	currentTime;
	PARAMETERS_STRUCT	eventCfg;
	AUTO_CAL_MON_CFG	autoCfg;
	EXTRA_UNIT_CFG		extraUnitCfg;
	ALARM_CFG			alarmCfg;
	TIMER_CFG			timerCfg;
	uint8				flashWrapping;
	uint8				appBuildVersion;
	uint16				batteryLevel;
	uint8				variableTriggerPercentageLevel;
	uint8				unused[3];
} SYSTEM_CFG; // Remote DCM/UCM only
#pragma pack()





// enum list for command index.
enum CMD_MESSAGE_INDEX {
	AAA = 0,	// Dummy function call.
	MRS,		// Modem reset.

#if 0
	MVS,		// Modem view settings.
	MPO,		// Toggle modem on/off.
	MMO,		// Toggle modem mode transmit/receive.
	MNO,		// Toggle modem phone number A/B/C.
	MTO,		// Toggle modem log on/off.
	MSD,		// Modem set default initialization string.
	MSR,		// Modem set receive initialization string.
	MST,		// Modem set transmit initialization string.
	MSA,		// Modem set phone number A.
	MSB,		// Modem set phone number B.
	MSC,		// Modem set phone number C.
	MVI,		// Modem view last call in detail.
	MVO,		// Modem view last call out detail.
	VTI,		// View time.
	STI,		// Set time.
	VDA,		// View date.
	SDA,		// Set date.
#endif

	// Immediate commands
	UNL,		// Unlock unit.
	RST,		// Reset the unit.
	DDP,		// Disable Debug Printing Toggle.
	DAI,		// Disable Debug Printing Toggle.

#if 0
	ZRO,		// Zero sensors.
	TTO,		// Toggle test mode on/off.
#endif
	CAL,		// Calibrate sensors with cal pulse.
#if 0
	VOL,		// View on/off log.
	VCL,		// View command log.
	VSL,		// View summary log.
	VEL,		// View event log.
	ESM,		// Erase summary memory.
	ECM,		// Erase configuration memory.
	TRG,		// Trigger an event.
#endif

	VML,		// View Monitor Log
	DQM,		// Download Quick summary memory.
	DSM,		// Download summary memory.
	DEM,		// Download event memory.
	DET,		// Download event in CSV text
	EEM,		// Erase event memory.
	DCM,		// Download configuration memory.
	UCM,		// Upload configuration memory.
	DMM,		// Download modem configuration memory.
	UMM,		// Upload modem configuration memory.
	GMN,		// Go start Monitoring waveform/bargraph/combo.
	HLT,		// Halt Monitoring waveform/bargraph/combo.
	GLM,		// Get/Start Bar Live Monitoring
	HLM,		// Halt Bar Live Monitoring
	DLM,		// Download Bar Live Monitoring pending event record
	UDE,		// Update last Download Event number
	GAD,		// Get Auto-Dialout/Download information
	GFS,		// Get flash stats
	VFV,		// View firmware version
	ACK,		// Acknowledge
	NAK,		// Nack
	CAN,		// Cancel
	ESC,		// Escape sequence
#if 1 /* Test */
	DBL,
#endif
	ZZZ,
	TOTAL_COMMAND_MESSAGES
};

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------
uint8 ParseIncommingMsgHeader(CMD_BUFFER_STRUCT*, COMMAND_MESSAGE_HEADER*);
uint8 ParseIncommingMsgCmd(CMD_BUFFER_STRUCT*, COMMAND_MESSAGE_HEADER*);

void BuildOutgoingHeaderBuffer(COMMAND_MESSAGE_HEADER*, uint8*);
void BuildOutgoingSimpleHeaderBuffer(uint8*, uint8*, uint8*, uint32, uint8, uint8);
void SendErrorMsg(uint8*, uint8*);
uint16 GetInt16Field(uint8*);
void BuildIntDataField(char*, uint32, uint8);
uint32 DataLengthStrToUint32(uint8*);
void WriteCompressedData(uint8 compressedData, uint8 outMode);
void InitTcpListenServer(void);
void InitAutoDialout(void);
uint8 CheckAutoDialoutStatusAndFlagIfAvailable(void);
void StartAutoDialoutProcess(void);
void AutoDialoutStateMachine(void);
void ShutdownPdnAndCellModem(void);
char* AdoStateGetDebugString(void);
void SetStartCellConnectTime(void);
void CellConnectStatsUpdate(void);
uint32 GetCurrentCellConnectTime(void);
uint32 GetCellConnectStatsLastConenct(void);
uint32 GetCellConnectStatsAverage(void);

uint8 FirstPassValidateCommandString(char* command);

#endif // _REMOTE_COMMON_H_

