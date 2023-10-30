///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

#ifndef _SUMMARY_CMMN_H_
#define _SUMMARY_CMMN_H_

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include "Typedefs.h"
#include "Common.h"
#include "Sensor.h"
#include "time.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define VERSION_STRING_SIZE				8
#define MODEL_STRING_SIZE				20
#define SERIAL_NUMBER_STRING_SIZE		20
#define SERIAL_NUMBER_CACHE_ENTRIES		256
#define FACTORY_SERIAL_NUMBER_SIZE		16

#define COMPANY_NAME_STRING_SIZE		32
#define SEISMIC_OPERATOR_STRING_SIZE	32
#define SESSION_LOCATION_STRING_SIZE	32
#define SESSION_COMMENTS_STRING_SIZE	102

#define PRETRIGGER_BUFFER_QUARTER_SEC_DIV	4
#define PRETRIGGER_BUFFER_HALF_SEC_DIV		2
#define PRETRIGGER_BUFFER_FULL_SEC_DIV		1

//-------------------------------------------------------------------------------------
typedef struct
{
	uint16 a;
	uint16 r;
	uint16 v;
	uint16 t;
} SAMPLE_DATA_STRUCT;

//-------------------------------------------------------------------------------------
typedef struct
{
	int16 a_offset;
	int16 r_offset;
	int16 v_offset;
	int16 t_offset;
} OFFSET_DATA_STRUCT;

//-------------------------------------------------------------------------------------
#pragma pack(1)
typedef struct
{
	uint16 air;	// Max Air peak for an interval
	uint16 rvt;	// Max RVT peak for all channels for an interval
	uint32 vs;	// Max Vector sum data (squared)
} BAR_INTERVAL_DATA_STRUCT;
#pragma pack()

//-------------------------------------------------------------------------------------
typedef struct
{
	uint16 peak; // Max peak, kept in raw A/D value (not in units of measurement)
	uint16 frequency; // The count for a period (frequency will later be calculated from this)
	uint32 displacement; // Peak Displacement = Peak / (2 * PI * Freq)
	uint32 acceleration; // Peak Acceleration = 2 * PI * Peak * Freq / 386.4 in/sec^2 (9814.6 mm/sec^2)
} CHANNEL_CALCULATED_DATA_STRUCT;

//-------------------------------------------------------------------------------------
typedef struct
{
	union
	{
		CHANNEL_CALCULATED_DATA_STRUCT chan[4];
		struct {
			CHANNEL_CALCULATED_DATA_STRUCT a;
			CHANNEL_CALCULATED_DATA_STRUCT r;
			CHANNEL_CALCULATED_DATA_STRUCT v;
			CHANNEL_CALCULATED_DATA_STRUCT t;
		};
	};
} CHANNEL_SUMMARY;

//-------------------------------------------------------------------------------------
typedef struct
{
	uint16 eventNumber;
	uint8 mode;
	uint8 subMode;
	CHANNEL_SUMMARY channelSummary;
	DATE_TIME_STRUCT eventTime;
	uint8 serialNumber[SERIAL_NUMBER_STRING_SIZE];
	uint16 seismicSensorType;
	uint16 sampleRate;
	uint8 unitsOfMeasure;
	uint8 unitsOfAir;
	uint8 gainSelect;
	uint8 bitAccuracy;
	uint32 vectorSumPeak;
} SUMMARY_LIST_ENTRY_STRUCT; // 96 bytes

//-------------------------------------------------------------------------------------
typedef struct
{
	uint8 mode;
	uint8 subMode;
	uint8 serialNumberCacheIndex;
	uint8 spare; // Needed to make structure even and keep time long bounded
	time_t epochEventTime; // 4 bytes
} EVENT_LIST_ENTRY_STRUCT; // 8 bytes

//-------------------------------------------------------------------------------------
typedef struct
{
	uint16 sign;
	uint16 freq_count;
	uint8 updateFlag;
	uint8 matchFlag;
} BARGRAPH_FREQ_CHANNEL_BUFFER;

//-------------------------------------------------------------------------------------
typedef struct
{
	BARGRAPH_FREQ_CHANNEL_BUFFER a;
	BARGRAPH_FREQ_CHANNEL_BUFFER r;
	BARGRAPH_FREQ_CHANNEL_BUFFER v;
	BARGRAPH_FREQ_CHANNEL_BUFFER t;
} BARGRAPH_FREQ_CALC_BUFFER;

//-------------------------------------------------------------------------------------
typedef struct 
{
	uint16 aMax;
	uint16 rvtMax;
	uint32 vsMax;
	uint16 rMax;
	uint16 vMax;
	uint16 tMax;
	uint16 aFreq;
	uint16 rFreq;
	uint16 vFreq;
	uint16 tFreq;
#if 1 /* Bargraph live monitoring */
	uint16 barIntervalCount;
	uint16 summaryIntervalCount;
	uint16 currentBargraphEventNumber;
	uint16 currentComboWaveformEventNumber;
	uint32 epochTime;
#endif
} BARGRAPH_BAR_INTERVAL_DATA;

//-------------------------------------------------------------------------------------
typedef struct
{
	uint8 type;
	uint8 input;
	uint8 group;
	uint8 options;
} SEISMIC_CHANNEL_INFO_STRUCT;

//-------------------------------------------------------------------------------------
typedef struct
{
	DATE_TIME_STRUCT triggerTime;
	time_t gpsEpochTriggerTime;
	uint32 gpsFractionalSecond;
} EVENT_TIMESTAMP_STRUCT;

//-------------------------------------------------------------------------------------
// Version sub-structure
//-------------------------------------------------------------------------------------
#define UNUSED_VERSION_SIZE		3
typedef struct
{
	uint8 modelNumber[MODEL_STRING_SIZE]; // 0x18 (from 0xA55A)
	uint8 serialNumber[SERIAL_NUMBER_STRING_SIZE]; // 0x2C (from 0xA55A)
	uint8 softwareVersion[VERSION_STRING_SIZE]; // 0x40 (from 0xA55A)
	SMART_SENSOR_ROM seismicSensorRom; // 0x48 (from 0xA55A)
	SMART_SENSOR_ROM acousticSensorRom; // 0x50 (from 0xA55A)
	uint8 hardwareId; // 0x58 (from 0xA55A)
	uint8 unused[UNUSED_VERSION_SIZE]; // 0x59 (from 0xA55A)
} VERSION_INFO_STRUCT; // 68 bytes

#pragma pack(1)
//-------------------------------------------------------------------------------------
// Parameters sub-structure
//-------------------------------------------------------------------------------------
typedef struct
{
	uint32	distToSource; // 0x5C (from 0xA55A)
	uint32	weightPerDelay; // 0x60 (from 0xA55A)
	uint16	sampleRate; // 0x64 (from 0xA55A)
	uint16	seismicSensorType; // 0x66 (from 0xA55A)
	uint16	airSensorType; // 0x68 (from 0xA55A)
	uint8	bitAccuracy; // 0x6A (from 0xA55A)
	uint8	aWeighting; // 0x6B (from 0xA55A)
	uint8	numOfChannels; // 0x6C (from 0xA55A)
	uint8	activeChannels; // 0x6D (from 0xA55A)
	uint8	appMajorVersion; // 0x6E (from 0xA55A) // Used for modem config
	uint8	appMinorVersion; // 0x6F (from 0xA55A) // Used for modem config
	SEISMIC_CHANNEL_INFO_STRUCT	channel[8]; // 0x70 (from 0xA55A)

	// Waveform specific - Initial conditions.
	uint32	seismicTriggerLevel; // 0x90 (from 0xA55A)
	uint32	airTriggerLevel; // 0x94 (from 0xA55A)
	uint32	recordTime; // 0x98 (from 0xA55A)
	uint16	numOfSamples; // 0x9C (from 0xA55A)
	uint16	preBuffNumOfSamples; // 0x9E (from 0xA55A)
	uint16	calDataNumOfSamples; // 0xA0 (from 0xA55A)

	// Bargraph specific - Initial conditions.
	uint16	barInterval; // 0xA2 (from 0xA55A)
	uint16	summaryInterval; // 0xA4 (from 0xA55A)

	uint8	companyName[COMPANY_NAME_STRING_SIZE]; // 0xA6 (from 0xA55A)
	uint8	seismicOperator[SEISMIC_OPERATOR_STRING_SIZE]; // 0xC6 (from 0xA55A)
	uint8	sessionLocation[SESSION_LOCATION_STRING_SIZE]; // 0xE6 (from 0xA55A)
	uint8	sessionComments[SESSION_COMMENTS_STRING_SIZE]; // 0x106 (from 0xA55A)
	
	uint8	adjustForTempDrift; // 0x16C (from 0xA55A)
	uint8	pretrigBufferDivider; // 0x16D (from 0xA55A)
	uint8	seismicUnitsOfMeasure; // 0x16E (from 0xA55A)
	uint8	airUnitsOfMeasure; // 0x16F (from 0xA55A)
	uint32	seismicTriggerInUnits; // 0x170 (from 0xA55A)
	uint32	airTriggerInUnits; // 0x174 (from 0xA55A)

	uint8 seismicSensorSerialNumber[6]; // 0x178 (from 0xA55A)
	CALIBRATION_DATE_STRUCT seismicSensorCurrentCalDate; // 0x17E (from 0xA55A)
	uint8 seismicSensorFacility; // 0x182 (from 0xA55A)
	uint8 seismicSensorInstrument; // 0x183 (from 0xA55A)

	uint8 acousticSensorSerialNumber[6]; // 0x184 (from 0xA55A)
	CALIBRATION_DATE_STRUCT acousticSensorCurrentCalDate; // 0x18A (from 0xA55A)
	uint8 acousticSensorFacility; // 0x18E (from 0xA55A)
	uint8 acousticSensorInstrument; // 0x18F (from 0xA55A)

	uint8 calibrationDateSource; // 0x190 (from 0xA55A)
	uint8 adChannelVerification; // 0x191 (from 0xA55A)

	uint8 barIntervalDataType; // 0x192 (form 0xA55A)
	int8 utcZoneOffset;	// 0x193 (from 0xA55A)
} PARAMETERS_STRUCT; // 312 bytes
#pragma pack()

#pragma pack(1)
//-------------------------------------------------------------------------------------
// Capture sub-structure
//-------------------------------------------------------------------------------------
#define UNUSED_CAPTURE_SIZE	4
typedef struct
{
	DATE_TIME_STRUCT	calDateTime; // 0x194 (from 0xA55A)	// Calibration date
	uint32				batteryLevel; // 0x1A0 (from 0xA55A) // Battery Level
	uint8				bargraphSessionComplete; // 0x1A4 (from 0xA55A) // Session status
	uint8				externalTrigger; // 0x1A5 (from 0xA55A) // Mark if triggered with an External signal
	uint16				comboEventsRecordedDuringSession; // 0x1A6 (from 0xA55A) // C-Wave events during C-Bar session
	DATE_TIME_STRUCT	eventTime; // 0x1A8 (from 0xA55A) // Waveform and bargraph start information. 
	DATE_TIME_STRUCT	endTime; // 0x1B4 (from 0xA55A) // Bargraph specific

	uint16				comboEventsRecordedStartNumber; // 0x1C0 (from 0xA55A)
	uint16				comboEventsRecordedEndNumber; // 0x1C2 (from 0xA55A)
	uint16				comboBargraphEventNumberLink; // 0x1C4 (from 0xA55A)
	uint8				variableTriggerPercentageLevel; // 0x1C6 (from 0xA55A)
	uint8				unused1; // 0x1C7 (from 0xA55A) // Space for expansion
	time_t				gpsEpochTriggerTime; // 0x1C8 (from 0xA55A)
	uint32				gpsFractionalSecond; // 0x1CC (from 0xA55A)
	uint8				unused[UNUSED_CAPTURE_SIZE]; // 0x1D0 (from 0xA55A) // Space for expansion
} CAPTURE_INFO_STRUCT; // 64 bytes
#pragma pack()

#pragma pack(1)
//-------------------------------------------------------------------------------------
// Calculated sub-structure
//-------------------------------------------------------------------------------------
#define UNUSED_CALCULATED_SIZE	(54 - sizeof(GPS_POSITION))
typedef struct
{
	// Used for both Waveform and Bargraph (and Bargraph Summaries), 0xCC in size (204 bytes)
	CHANNEL_CALCULATED_DATA_STRUCT a; // 0x1D4 (from 0xA55A), 0x0 (from struct start)
	CHANNEL_CALCULATED_DATA_STRUCT r; // 0x1E0 (from 0xA55A), 0xC (from struct start)
	CHANNEL_CALCULATED_DATA_STRUCT v; // 0x1EC (from 0xA55A), 0x18 (from struct start)
	CHANNEL_CALCULATED_DATA_STRUCT t; // 0x1F8 (from 0xA55A), 0x24 (from struct start)

	uint16 bargraphEffectiveSampleRate;
	uint16 unused1;
	uint32 unused2;
	uint32 unused3;
	uint32 vectorSumPeak; // 0x210 (from 0xA55A), 0x3C (from struct start)

	// Bargraph specific variables.
	DATE_TIME_STRUCT a_Time; // 0x214 (from 0xA55A), 0x40 (from struct start)
	DATE_TIME_STRUCT r_Time; // 0x220 (from 0xA55A), 0x4C (from struct start)
	DATE_TIME_STRUCT v_Time; // 0x22C (from 0xA55A), 0x58 (from struct start)
	DATE_TIME_STRUCT t_Time; // 0x238 (from 0xA55A), 0x64 (from struct start)
	DATE_TIME_STRUCT vs_Time; // 0x244 (from 0xA55A), 0x70 (from struct start)
	DATE_TIME_STRUCT intervalEnd_Time; // 0x250 (from 0xA55A), 0x7C (from struct start)

	uint32 batteryLevel; // 0x25C (from 0xA55A), 0x88 (from struct start)
	uint32 barIntervalsCaptured; // 0x260 (from 0xA55A), 0x8C (from struct start)
	uint16 summariesCaptured; // 0x264 (from 0xA55A), 0x90 (from struct start)

	GPS_POSITION		gpsPosition; // 0x266 (from 0xA55A) // 0x92 (from struct start)

	uint8				unused[UNUSED_CALCULATED_SIZE]; // 0x27C (from 0xA55A) // 0xA8 (from struct start)
	uint32 calcStructEndFlag; // 0x29C (from 0xA55A), 0xC8 (from struct start)
} CALCULATED_DATA_STRUCT; // 204 bytes
#pragma pack()

typedef struct
{
	uint16 startFlag; // 0x0 (from 0xA55A)
	uint16 recordVersion; // 0x2 (from 0xA55A)
	uint16 headerLength; // 0x4 (from 0xA55A)
	uint16 summaryLength; // 0x6 (from 0xA55A)
	uint32 dataLength; // 0x8 (from 0xA55A)
	uint16 dataCompression; // 0xC (from 0xA55A)
	uint16 summaryChecksum; // 0xE (from 0xA55A)
	uint16 dataChecksum; // 0x10 (from 0xA55A)
	uint16 unused1; // 0x12 (from 0xA55A)
	uint16 unused2; // 0x14 (from 0xA55A)
	uint16 unused3; // 0x16 (from 0xA55A)
} EVENT_HEADER_STRUCT; // 24 bytes

typedef struct
{
	VERSION_INFO_STRUCT		version; // 0x18 (from 0xA55A)
	PARAMETERS_STRUCT		parameters; // 0x5C (from 0xA55A)
	CAPTURE_INFO_STRUCT		captured; // 0x194 (from 0xA55A)
	CALCULATED_DATA_STRUCT	calculated; // 0x1D4 (from 0xA55A)
	uint16 					eventNumber; // 0x2A0 (from 0xA55A)
	uint8					mode; // 0x2A2 (from 0xA55A)
	uint8					subMode; // 0x2A3 (from 0xA55A)
} EVENT_SUMMARY_STRUCT; // 652 bytes

//#pragma pack(4)
typedef struct
{
	EVENT_HEADER_STRUCT 		header;
	EVENT_SUMMARY_STRUCT		summary;
} EVT_RECORD; // 676 bytes
//#pragma pack()



#endif //_SUMMARY_CMMN_H_

