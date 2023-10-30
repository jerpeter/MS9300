///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

#ifndef _RTC_H_
#define _RTC_H_

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include "Typedefs.h"
#include "Common.h"
#include "time.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define TOTAL_MONTHS	12

// Months
enum {
	JAN = 1,
	FEB, //	2
	MAR, //	3
	APR, //	4
	MAY, //	5
	JUN, //	6
	JUL, //	7
	AUG, //	8
	SEP, //	9
	OCT, //	10
	NOV, //	11
	DEC //	12
};

// Days
enum {
	SUN = 0,
	MON, // 1
	TUE, // 2
	WED, // 3
	THU, // 4
	FRI, // 5
	SAT // 6
};

typedef struct
{
	uint32 monthNumber;
	uint8 name[4];
	uint32 days;
} MONTH_TABLE_STRUCT;

// Alarm Frequency
enum {
	ONCE_PER_SECOND = 0,
	ONCE_PER_MINUTE_WHEN_SECONDS_MATCH,
	ONCE_PER_HOUR_WHEN_MINUTES_AND_SECONDS_MATCH,
	ONCE_PER_DAY_WHEN_HOURS_MINUTES_AND_SECONDS_MATCH,
	WHEN_DAY_HOURS_MINUTES_AND_SECONDS_MATCH
};

//=============================================================================
//=== RTC Defines =============================================================
//=============================================================================

#define RTC_WRITE_CMD				0x20
#define RTC_READ_CMD				0xA0
#define RTC_ACCESS_DELAY			100

#define RTC_CLOCK_STOPPED			0x20
#define RTC_CLOCK_INTEGRITY			0x80
#define RTC_24_HOUR_MODE			0x00
#define RTC_12_HOUR_MODE			0x04
#define RTC_TIMESTAMP_INT_ENABLE	0x04
#define RTC_ALARM_INT_ENABLE		0x02
#define RTC_COUNTDOWN_INT_ENABLE	0x01
#define RTC_BCD_SECONDS_MASK		0x7F
#define RTC_BCD_MINUTES_MASK		0x7F
#define RTC_BCD_HOURS_MASK			0x3F
#define RTC_BCD_DAYS_MASK			0x3F
#define RTC_BCD_WEEKDAY_MASK		0x07
#define RTC_BCD_MONTHS_MASK			0x1F
#define RTC_BCD_YEARS_MASK			0xFF
#define RTC_DISABLE_ALARM			0x80
#define RTC_ENABLE_ALARM			0x00

#define BCD_CONVERT_TO_UINT8(x, filter)	((((x & filter) >> 4) * 10) + (x & 0x0F))
#define UINT8_CONVERT_TO_BCD(x, filter)	((((uint8)(x / 10) << 4) & filter) | ((x % 10) & 0x0F))

// ===========================
// RTC Registers
// ===========================

// ---------------------------
// Control register 1
// Address: 0x0
// ---------------------------
typedef union
{
	struct
	{
		bitfield ext_test:	 	1;
		bitfield t:			 	1;
		bitfield stop:			1;
		bitfield tsf1:		 	1;
		bitfield por_ovrd:		1;
		bitfield format_12_24:	1;
		bitfield mi:			1;
		bitfield si:			1;
	} bit;

	uint8 reg;
} RTC_CONTROL_1_STRUCT;

// ---------------------------
// Control register 2
// RTC Address: 0x1
// ---------------------------
typedef union
{
	struct
	{
		bitfield msf:		 	1;
		bitfield wdtf:			1;
		bitfield tsf2:			1;
		bitfield af:		 	1;
		bitfield cdtf:			1;
		bitfield tsie:			1;
		bitfield aie:			1;
		bitfield cdtie:			1;
	} bit;

	uint8 reg;
} RTC_CONTROL_2_STRUCT;

// ---------------------------
// Control register 3
// RTC Address: 0x2
// ---------------------------
typedef union
{
	struct
	{
		bitfield pwrmng:		3;
		bitfield btse:			1;
		bitfield bf:			1;
		bitfield blf:			1;
		bitfield bie:			1;
		bitfield blie:			1;
	} bit; 

	uint8 reg;
} RTC_CONTROL_3_STRUCT;

// ---------------------------
// Seconds register
// RTC Address: 0x3
// ---------------------------
typedef union
{
	struct
	{
		bitfield osf:		1;
		bitfield tenSecond:	3;
		bitfield oneSecond:	4;
	} bit; 

	uint8 reg;
} RTC_SECONDS_STRUCT_TEMP;

// ---------------------------
// Minutes register
// RTC Address: 0x4
// ---------------------------
typedef union
{
	struct
	{
		bitfield unused: 	1;
		bitfield tenMinute:	3;
		bitfield oneMinute:	4;
	} bit; 

	uint8 reg;
} RTC_MINUTES_STRUCT_TEMP;

// ---------------------------
// Hours register
// RTC Address: 0x5
// ---------------------------
typedef union
{
	struct
	{
		bitfield unused:	2;
		bitfield ampm: 		1;
		bitfield tenHour:	1;
		bitfield oneHour:	4;
	} bit; 

	struct
	{
		bitfield unused:	2;
		bitfield tenHour:	2;
		bitfield oneHour:	4;
	} hours_24;

	uint8 reg;
} RTC_HOURS_STRUCT_TEMP;

// ---------------------------
// Days register
// RTC Address: 0x6
// ---------------------------
typedef union
{
	struct
	{
		bitfield unused:	2;
		bitfield tenDay:	2;
		bitfield oneDay:	4;
	} bit; 

	uint8 reg;
} RTC_DAYS_STRUCT;

// ---------------------------
// Weekdays register
// RTC Address: 0x7
// ---------------------------
typedef union
{
	struct
	{
		bitfield unused: 	5;
		bitfield weekday:	3;
	} bit; 

	uint8 reg;
} RTC_WEEKDAYS_STRUCT;

// ---------------------------
// Months register
// RTC Address: 0x8
// ---------------------------
typedef union
{
	struct
	{
		bitfield unused:	3;
		bitfield tenMonth:	1;
		bitfield oneMonth:	4;
	} bit; 

	uint8 reg;
} RTC_MONTHS_STRUCT;

// ---------------------------
// Years register
// RTC Address: 0x9
// ---------------------------
typedef union
{
	struct
	{
		bitfield tenYear:	4;
		bitfield oneYear:	4;
	} bit; 

	uint8 reg;
} RTC_YEARS_STRUCT;

// ---------------------------
// Second alarm register
// RTC Address: 0xA
// ---------------------------
typedef union
{
	struct
	{
		bitfield ae_s:				1;
		bitfield tenSecond_alarm:	3;
		bitfield oneSecond_alarm:	4;
	} bit; 

	uint8 reg;
} RTC_SECOND_ALARM_STRUCT;

// ---------------------------
// Minute alarm register
// RTC Address: 0xB
// ---------------------------
typedef union
{
	struct
	{
		bitfield ae_m:				1;
		bitfield tenMinute_alarm:	3;
		bitfield oneMinute_alarm:	4;
	} bit; 

	uint8 reg;
} RTC_MINUTE_ALARM_STRUCT;

// ---------------------------
// Hour alarm register
// RTC Address: 0xC
// ---------------------------
typedef union
{
	struct
	{
		bitfield ae_h:			1;
		bitfield unused:		1;
		bitfield ampm:			1;
		bitfield tenHour_alarm:	1;
		bitfield oneHour_alarm:	4;
	} bit; 

	struct
	{
		bitfield ae_h:			1;
		bitfield unused:		1;
		bitfield tenHour_alarm:	2;
		bitfield oneHour_alarm:	4;
	} hours_24;

	uint8 reg;
} RTC_HOUR_ALARM_STRUCT;

// ---------------------------
// Day alarm register
// RTC Address: 0xD
// ---------------------------
typedef union
{
	struct
	{
		bitfield ae_d:			1;
		bitfield unused:		1;
		bitfield tenDay_alarm:	2;
		bitfield oneDay_alarm:	4;
	} bit; 

	uint8 reg;
} RTC_DAY_ALARM_STRUCT_TEMP;

// ---------------------------
// Weekday alarm register
// RTC Address: 0xE
// ---------------------------
typedef union
{
	struct
	{
		bitfield ae_w:			1;
		bitfield unused:		4;
		bitfield weekday_alarm:	3;
	} bit; 

	uint8 reg;
} RTC_WEEKDAY_ALARM_STRUCT;

// ---------------------------
// Clock out control register
// RTC Address: 0xF
// ---------------------------
typedef union
{
	struct
	{
		bitfield tcr:			2;
		bitfield unused:		3;
		bitfield cof:			3;
	} bit; 

	uint8 reg;
} RTC_CLOCK_OUT_CONTROL_STRUCT;

// ---------------------------
// Watchdog control register
// RTC Address: 0x10
// ---------------------------
typedef union
{
	struct
	{
		bitfield wd_cd:			2;
		bitfield ti_tp:			1;
		bitfield unused:		3;
		bitfield tf:			2;
	} bit; 

	uint8 reg;
} RTC_WATCHDOG_CONTROL_STRUCT;

// ---------------------------
// Watchdog register
// RTC Address: 0x11
// ---------------------------
typedef union
{
	struct
	{
		bitfield watchdog:		8;
	} bit; 

	uint8 reg;
} RTC_WATCHDOG_STRUCT;

// ---------------------------
// Timestamp control register
// RTC Address: 0x12
// ---------------------------
typedef union
{
	struct
	{
		bitfield tsm:			1;
		bitfield tsoff:			1;
		bitfield unused:		1;
		bitfield one_O_16_ts:	5;
	} bit; 

	uint8 reg;
} RTC_TIMESTAMP_CONTROL_STRUCT;

// ---------------------------
// Timestamp seconds register
// RTC Address: 0x13
// ---------------------------
typedef union
{
	struct
	{
		bitfield unused:		1;
		bitfield seconds:		7;
	} bit; 

	uint8 reg;
} RTC_TIMESTAMP_SECONDS_STRUCT;

// ---------------------------
// Timestamp minutes register
// RTC Address: 0x14
// ---------------------------
typedef union
{
	struct
	{
		bitfield unused:		1;
		bitfield minutes:		7;
	} bit; 

	uint8 reg;
} RTC_TIMESTAMP_MINUTES_STRUCT;

// ---------------------------
// Timestamp hours register
// RTC Address: 0x15
// ---------------------------
typedef union
{
	struct
	{
		bitfield unused:		2;
		bitfield ampm:			1;
		bitfield hours:			5;
	} bit; 

	struct
	{
		bitfield unused:		2;
		bitfield hours:			6;
	} hours_24; 

	uint8 reg;
} RTC_TIMESTAMP_HOURS_STRUCT;

// ---------------------------
// Timestamp days register
// RTC Address: 0x16
// ---------------------------
typedef union
{
	struct
	{
		bitfield unused:		2;
		bitfield days:			6;
	} bit; 

	uint8 reg;
} RTC_TIMESTAMP_DAYS_STRUCT;

// ---------------------------
// Timestamp months register
// RTC Address: 0x17
// ---------------------------
typedef union
{
	struct
	{
		bitfield unused:		3;
		bitfield months:		5;
	} bit; 

	uint8 reg;
} RTC_TIMESTAMP_MONTHS_STRUCT;

// ---------------------------
// Timestamp years register
// RTC Address: 0x18
// ---------------------------
typedef union
{
	struct
	{
		bitfield years:			8;
	} bit; 

	uint8 reg;
} RTC_TIMESTAMP_YEARS_STRUCT;

// ---------------------------
// Aging offset register
// RTC Address: 0x19
// ---------------------------
typedef union
{
	struct
	{
		bitfield unused:		4;
		bitfield ao:			4;
	} bit; 

	uint8 reg;
} RTC_AGING_OFFSET_STRUCT;

// ---------------------------
// RAM address MSB register
// RTC Address: 0x1A
// ---------------------------
typedef union
{
	struct
	{
		bitfield unused:		7;
		bitfield ra8:			1;
	} bit; 

	uint8 reg;
} RTC_RAM_ADDRESS_MSB_STRUCT;

// ---------------------------
// RAM address LSB register
// RTC Address: 0x1B
// ---------------------------
typedef union
{
	struct
	{
		bitfield ra:			8;
	} bit; 

	uint8 reg;
} RTC_RAM_ADDRESS_LSB_STRUCT;

// ---------------------------
// RAM write register
// RTC Address: 0x1C
// ---------------------------
typedef union
{
	struct
	{
		bitfield write_data:	8;
	} bit; 

	uint8 reg;
} RTC_RAM_WRITE_STRUCT;

// ---------------------------
// RAM read register
// RTC Address: 0x1D
// ---------------------------
typedef union
{
	struct
	{
		bitfield read_data:		8;
	} bit; 

	uint8 reg;
} RTC_RAM_READ_STRUCT;

// ===========================
// RTC Memory Location Defines
// ===========================
#define RTC_CONTROL_1_ADDR			0x00
#define RTC_CONTROL_2_ADDR			0x01
#define RTC_CONTROL_3_ADDR			0x02
#define RTC_SECONDS_ADDR_TEMP		0x03
#define RTC_MINUTES_ADDR_TEMP		0x04
#define RTC_HOURS_ADDR_TEMP			0x05
#define RTC_DAYS_ADDR				0x06
#define RTC_WEEKDAYS_ADDR			0x07
#define RTC_MONTHS_ADDR				0x08
#define RTC_YEARS_ADDR				0x09
#define RTC_SECOND_ALARM_ADDR		0x0a
#define RTC_MINUTE_ALARM_ADDR		0x0b
#define RTC_HOUR_ALARM_ADDR			0x0c
#define RTC_DAY_ALARM_ADDR_TEMP		0x0d
#define RTC_WEEKDAY_ALARM_ADDR		0x0e
#define RTC_CLOCK_OUT_CONTROL_ADDR	0x0f
#define RTC_WATCHDOG_CONTROL_ADDR	0x10
#define RTC_WATCHDOG_ADDR			0x11
#define RTC_TIMESTAMP_CONTROL_ADDR	0x12
#define RTC_TIMESTAMP_SECONDS_ADDR	0x13
#define RTC_TIMESTAMP_MINUTES_ADDR	0x14
#define RTC_TIMESTAMP_HOURS_ADDR	0x15
#define RTC_TIMESTAMP_DAYS_ADDR		0x16
#define RTC_TIMESTAMP_MONTHS_ADDR	0x17
#define RTC_TIMESTAMP_YEARS_ADDR	0x18
#define RTC_AGING_OFFSET_ADDR		0x19
#define RTC_RAM_ADDRESS_MSB_ADDR	0x1a
#define RTC_RAM_ADDRESS_LSB_ADDR	0x1b
#define RTC_RAM_WRITE_ADDR			0x1c
#define RTC_RAM_READ_ADDR			0x1d

typedef struct
{
	uint8 control_1;
	uint8 control_2;
	uint8 control_3;
	uint8 seconds;
	uint8 minutes;
	uint8 hours;
	uint8 days;
	uint8 weekdays;
	uint8 months;
	uint8 years;
	uint8 second_alarm;
	uint8 minute_alarm;
	uint8 hour_alarm;
	uint8 day_alarm;
	uint8 weekday_alarm;
	uint8 clock_out_control;
	uint8 watchdog_control;
	uint8 watchdog;
	uint8 timestamp_control;
	uint8 timestamp_seconds;
	uint8 timestamp_minutes;
	uint8 timestamp_hours;
	uint8 timestamp_days;
	uint8 timestamp_months;
	uint8 timestamp_years;
	uint8 aging_offset;
	uint8 ram_address_msb;
	uint8 ram_address_lsb;
	uint8 ram_write;
	uint8 ram_read;
} RTC_MEM_MAP_STRUCT;

typedef struct
{
	uint8 seconds;
	uint8 minutes;
	uint8 hours;
	uint8 days;
	uint8 weekdays;
	uint8 months;
	uint8 years;
} RTC_DATE_TIME_STRUCT;

typedef struct
{
	uint8 days;
	uint8 weekdays;
	uint8 months;
	uint8 years;
} RTC_DATE_STRUCT;

typedef struct
{
	uint8 seconds;
	uint8 minutes;
	uint8 hours;
} RTC_TIME_STRUCT;

typedef struct
{
	uint8 second_alarm;
	uint8 minute_alarm;
	uint8 hour_alarm;
	uint8 day_alarm;
	uint8 weekday_alarm;
} RTC_ALARM_STRUCT;

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------
BOOLEAN ExternalRtcInit(void);
uint8 SetExternalRtcTime(DATE_TIME_STRUCT* time);
uint8 SetExternalRtcDate(DATE_TIME_STRUCT* time);
uint8 SetExternalRtcDateAndTimeByGpsUtcEpoch(time_t currentEpochTime);
DATE_TIME_STRUCT GetExternalRtcTime(void);
uint8 UpdateCurrentTime(void);
DATE_TIME_STRUCT GetCurrentTime(void);
uint32 GetCurrentEpochTime(void);
void DisableExternalRtcAlarm(void);
void EnableExternalRtcAlarm(uint8 day, uint8 hour, uint8 minute, uint8 second);
void ConvertCurrentTimeForFat(uint8* fatTimeField);
void ConvertCurrentDateForFat(uint8* fatTimeDate);
DATE_TIME_STRUCT ConvertEpochTimeToDateTime(time_t epochTime);
time_t ConvertDateTimeToEpochTime(DATE_TIME_STRUCT dateTime);
void ExternalRtcWrite(uint8 register_address, int length, uint8* data);
void ExternalRtcRead(uint8 register_address, int length, uint8* data);
void StartExternalRtcClock(uint16 sampleRate);
void StopExternalRtcClock(void);

// Timer mode related
void TimerModeActiveMinutes(void);
uint8 ValidateTimerModeSettings(void);

#endif // _RTC_H_
