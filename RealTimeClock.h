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
	uint8 month_alarm;
} RTC_ALARM_STRUCT;

/****************************************************************************
 * drivers/timers/pcf85263.h
 *
 *   Copyright (C) 2015 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __DRIVERS_TIMERS_PCF85263_H
#define __DRIVERS_TIMERS_PCF85263_H

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* RTC time and date registers */

#define PCF85263_RTC_100TH_SECONDS         0x00      /* RTC 100ths of seconds register */
                                                     /* Bits 0-7: 100ths of seconds register (0-99 BCD) */

#define PCF85263_RTC_SECONDS               0x01      /* RTC seconds register */
#define PCF85263_RTC_SECONDS_MASK        (0x7f)    /* Bits 0-6: Seconds (0-59 BCD) */
#define PCF85263_RTC_SECONDS_OS          (1 << 7)  /* Bit 7: Oscillator stop */

#define PCF85263_RTC_MINUTES               0x02      /* RTC minutes register */
#define PCF85263_RTC_MINUTES_MASK        (0x7f)    /* Bits 0-6: Minutes (0-59 BCD) */
#define PCF85263_RTC_MINUTES_EMON        (1 << 7)  /* Bit 7: Event monitor */

#define PCF85263_RTC_HOURS                 0x03      /* RTC hours register */
#define PCF85263_RTC_HOURS12_MASK        0x1f      /* Bits 0-4: Hours (1-12 BCD) */
#define PCF85263_RTC_AMPM                (1 << 5)  /* Bit 5: AM/PM */
#define PCF85263_RTC_HOURS24_MASK        0x3f      /* Bits 0-5: Hours (0-23  BCD) */

#define PCF85263_RTC_DAYS                  0x04      /* RTC days register */
#define PCF85263_RTC_DAYS_MASK           0x3f      /* Bit 0-5: Day of the month (1-31 BCD) */

#define PCF85263_RTC_WEEKDAYS              0x05      /* RTC day-of-week register */
#define PCF85263_RTC_WEEKDAYS_MASK       0x07      /* Bits 0-2: Day of the week (0-6) */

#define PCF85263_RTC_MONTHS                0x06      /* RTC month register */
#define PCF85263_RTC_MONTHS_MASK         0x1f      /* Bits 0-4: Month (1-12 BCD) */

#define PCF85263_RTC_YEARS                 0x07      /* RTC year register */
                                                     /* Bits 0-7: Year (0-99 BCD) */

/* RTC alarm1 */

#define PCF85263_RTC_SECOND_ALARM1         0x08      /* RTC alarm1 seconds register */
#define PCF85263_RTC_SECOND_ALARM1_MASK  0x7f      /* Bits 0-6:  Seconds (0-59 BCD) */

#define PCF85263_RTC_MINUTE_ALARM1         0x09      /* RTC alarm1 minutes register */
#define PCF85263_RTC_MINUTE_ALARM1_MASK  0x7f      /* Bits 0-6:  Minutes (0-59 BCD) */

#define PCF85263_RTC_HOUR_ALARM1           0x0a      /* RTC alarm1 hours register */
#define PCF85263_RTC_HOURS12_ALARM1_MASK 0x1f      /* Bits 0-4: Hours (1-12 BCD) */
#define PCF85263_RTC_AMPM_ALARM1         (1 << 5)  /* Bit 5: AM/PM */
#define PCF85263_RTC_HOURS24_ALARM1_MASK 0x3f      /* Bits 0-5: Hours (0-23  BCD) */

#define PCF85263_RTC_DAY_ALARM1            0x0b      /* RTC alarm1 days register */
#define PCF85263_RTC_DAY_ALARM1_MASK     0x3f      /* Bits 0-5: Days (1-31 BCD) */

#define PCF85263_RTC_MONTH_ALARM1          0x0c      /* RTC alarm1 month register */
#define PCF85263_RTC_MONTH_ALARM1_MASK   0x1f      /* Bits 0-4: Month (1-12 BCD) */

/* RTC alarm2 */

#define PCF85263_RTC_MINUTE_ALARM2         0x0d      /* RTC alarm2 seconds register */
#define PCF85263_RTC_MINUTE_ALARM2_MASK  0x7f      /* Bits 0-6:  Minutes (0-59 BCD) */

#define PCF85263_RTC_HOUR_ALARM2           0x0e      /* RTC alarm1 minutes register */
#define PCF85263_RTC_HOURS12_ALARM2_MASK 0x1f      /* Bits 0-4: Hours (1-12 BCD) */
#define PCF85263_RTC_AMPM_ALARM2         (1 << 5)  /* Bit 5: AM/PM */
#define PCF85263_RTC_HOURS24_ALARM2_MASK 0x3f      /* Bits 0-5: Hours (0-23  BCD) */

#define PCF85263_RTC_WEEKDAY_ALARM         0x0f      /* RTC alarm1 day-of-week register */
#define PCF85263_RTC_WEEKDAY_ALARM_MASK  0x07      /* Bits 0-2: Day-of-week (0-6) */

/* RTC alarm enables */

#define PCF85263_RTC_ALARM_ENABLES         0x10      /* RTC alarem enables */
#define PCF85263_RTC_ALARM_SEC_A1E       (1 << 0)  /* Second alarm1 enable */
#define PCF85263_RTC_ALARM_MIN_A1E       (1 << 1)  /* Minute alarm1 enable */
#define PCF85263_RTC_ALARM_HR_A1E        (1 << 2)  /* Hour alarm1 enable */
#define PCF85263_RTC_ALARM_DAY_A1E       (1 << 3)  /* Day alarm1 enable */
#define PCF85263_RTC_ALARM_MON_A1E       (1 << 4)  /* Month alarm1 enable */
#define PCF85263_RTC_ALARM_MIN_A2E       (1 << 5)  /* Minute alarm2 enable */
#define PCF85263_RTC_ALARM_HR_A2E        (1 << 6)  /* Hour alarm2 enable */
#define PCF85263_RTC_ALARM_WDAY_A2E      (1 << 7)  /* Day-of-week alarm2 enable */

/* RTC timestamp1 (TSR1) */

#define PCF85263_RTC_TSR1_SECONDS          0x11      /* TSR1 seconds register */
#define PCF85263_RTC_TSR1_SECONDS_MASK   (0x7f)    /* Bits 0-6: Seconds (0-59 BCD) */
#define PCF85263_RTC_SECONDS_OS          (1 << 7)  /* Bit 7: Oscillator stop */

#define PCF85263_RTC_TSR1_MINUTES          0x12      /* TSR1 minutes register */
#define PCF85263_RTC_TSR1_MINUTES_MASK   (0x7f)    /* Bits 0-6: Minutes (0-59 BCD) */

#define PCF85263_RTC_TSR1_HOURS            0x13      /* TSR1 hours register */
#define PCF85263_RTC_TSR1_HOURS12_MASK   0x1f      /* Bits 0-4: Hours (1-12 BCD) */
#define PCF85263_RTC_TSR1_AMPM           (1 << 5)  /* Bit 5: AM/PM */
#define PCF85263_RTC_TSR1_HOURS24_MASK   0x3f      /* Bits 0-5: Hours (0-23  BCD) */

#define PCF85263_RTC_TSR1_DAYS             0x14      /* TSR1 days register */
#define PCF85263_RTC_TSR1_DAYS_MASK      0x3f      /* Bits 0-5: Day of the month (1-31 BCD) */

#define PCF85263_RTC_TSR1_MONTHS           0x15      /* TSR1 month register */
#define PCF85263_RTC_TSR1_MONTHS_MASK    0x1f      /* Bits 0-4: Month (1-12 BCD) */

#define PCF85263_RTC_TSR1_YEARS            0x16      /* TSR1 year register */
                                                     /* Bits 0-7: Year (0-99 BCD) */

/* RTC timestamp2 (TSR2) */

#define PCF85263_RTC_TSR2_SECONDS          0x17      /* TSR2 seconds register */
#define PCF85263_RTC_TSR2_SECONDS_MASK   (0x7f)    /* Bits 0-6: Seconds (0-59 BCD) */

#define PCF85263_RTC_TSR2_MINUTES          0x18      /* TSR2 minutes register */
#define PCF85263_RTC_TSR2_MINUTES_MASK   (0x7f)    /* Bits 0-6: Minutes (0-59 BCD) */

#define PCF85263_RTC_TSR2_HOURS            0x19      /* TSR2 hours register */
#define PCF85263_RTC_TSR2_HOURS12_MASK   0x1f      /* Bits 0-4: Hours (1-12 BCD) */
#define PCF85263_RTC_TSR2_AMPM           (1 << 5)  /* Bit 5: AM/PM */
#define PCF85263_RTC_TSR2_HOURS24_MASK   0x3f      /* Bits 0-5: Hours (0-23  BCD) */

#define PCF85263_RTC_TSR2_DAYS             0x1a      /* TSR2 days register */
#define PCF85263_RTC_TSR2_DAYS_MASK      0x3f      /* Bits 0-5: Day of the month (1-31 BCD) */

#define PCF85263_RTC_TSR2_MONTHS           0x1b      /* TSR2 month register */
#define PCF85263_RTC_TSR2_MONTHS_MASK    0x1f      /* Bits 0-4: Month (1-12 BCD) */

#define PCF85263_RTC_TSR2_YEARS            0x1c      /* TSR2 year register */
                                                     /* Bits 0-7: Year (0-99 BCD) */

/* RTC timestamp3 (TSR3) */

#define PCF85263_RTC_TSR3_SECONDS          0x1d      /* TSR3 seconds register */
#define PCF85263_RTC_TSR3_SECONDS_MASK   (0x7f)    /* Bits 0-6: Seconds (0-59 BCD) */

#define PCF85263_RTC_TSR3_MINUTES          0x1e      /* TSR3 minutes register */
#define PCF85263_RTC_TSR3_MINUTES_MASK   (0x7f)    /* Bits 0-6: Minutes (0-59 BCD) */

#define PCF85263_RTC_TSR3_HOURS            0x1f      /* TSR3 hours register */
#define PCF85263_RTC_TSR3_HOURS12_MASK   0x1f      /* Bits 0-4: Hours (1-12 BCD) */
#define PCF85263_RTC_TSR3_AMPM           (1 << 5)  /* Bit 5: AM/PM */
#define PCF85263_RTC_TSR3_HOURS24_MASK   0x3f      /* Bits 0-5: Hours (0-23  BCD) */

#define PCF85263_RTC_TSR3_DAYS             0x20      /* TSR3 days register */
#define PCF85263_RTC_TSR3_DAYS_MASK      0x3f      /* Bits 0-5: Day of the month (1-31 BCD) */

#define PCF85263_RTC_TSR3_MONTHS           0x21      /* TSR3 month register */
#define PCF85263_RTC_TSR3_MONTHS_MASK    0x1f      /* Bits 0-4: Month (1-12 BCD) */

#define PCF85263_RTC_TSR3_YEARS            0x22      /* TSR3 year register */
                                                     /* Bits 0-7: Year (0-99 BCD) */

/* RTC timestamp mode control */

#define PCF85263_RTC_TSR_MODE              0x23      /* Timestamp mode control register */
#define PCF85263_RTC_TSR_TSR1M_SHIFT     (0)       /* Bit 0-1: Timestamp register 1 mode */
#define PCF85263_RTC_TSR_TSR1M_MASK      (3 << PCF85263_RTC_TSR_TSR1M_SHIFT)
#define PCF85263_RTC_TSR_TSR1M_NONE    (0 << PCF85263_RTC_TSR_TSR1M_SHIFT) /* No timestamp */
#define PCF85263_RTC_TSR_TSR1M_FE      (1 << PCF85263_RTC_TSR_TSR1M_SHIFT) /* Record First TS pin Event */
#define PCF85263_RTC_TSR_TSR1M_LE      (2 << PCF85263_RTC_TSR_TSR1M_SHIFT) /* Record Last TS pin Event */
#define PCF85263_RTC_TSR_TSR2M_SHIFT     (2)       /* Bit 2-4: Timestamp register 2 mode */
#define PCF85263_RTC_TSR_TSR2M_MASK      (7 << PCF85263_RTC_TSR_TSR2M_SHIFT)
#define PCF85263_RTC_TSR_TSR2M_NONE    (0 << PCF85263_RTC_TSR_TSR2M_SHIFT) /* No timestamp */
#define PCF85263_RTC_TSR_TSR2M_FB      (1 << PCF85263_RTC_TSR_TSR2M_SHIFT) /* Record First time switch to Battery event */
#define PCF85263_RTC_TSR_TSR2M_LB      (2 << PCF85263_RTC_TSR_TSR2M_SHIFT) /* Record Last time switch to Battery event */
#define PCF85263_RTC_TSR_TSR2M_LV      (3 << PCF85263_RTC_TSR_TSR2M_SHIFT) /* Record Last time switch to VDD event */
#define PCF85263_RTC_TSR_TSR2M_FE      (4 << PCF85263_RTC_TSR_TSR2M_SHIFT) /* Record First TS pin Event */
#define PCF85263_RTC_TSR_TSR2M_LE      (5 << PCF85263_RTC_TSR_TSR2M_SHIFT) /* Record Last TS pin Event */
#define PCF85263_RTC_TSR_TSR3M_SHIFT     (6)       /* Bit 6-7: Timestamp register 3 mode */
#define PCF85263_RTC_TSR_TSR3M_MASK      (3 << PCF85263_RTC_TSR_TSR3M_SHIFT)
#define PCF85263_RTC_TSR_TSR3M_NONE    (0 << PCF85263_RTC_TSR_TSR3M_SHIFT) /* No timestamp */
#define PCF85263_RTC_TSR_TSR3M_FB      (1 << PCF85263_RTC_TSR_TSR3M_SHIFT) /* Record First time switch to Battery event */
#define PCF85263_RTC_TSR_TSR3M_LB      (2 << PCF85263_RTC_TSR_TSR3M_SHIFT) /* Record Last time switch to Battery event */
#define PCF85263_RTC_TSR_TSR3M_LV      (3 << PCF85263_RTC_TSR_TSR3M_SHIFT) /* Record Last time switch to VDD event */

/* Stop-watch time registers */

#define PCF85263_STW_100TH_SECONDS         0x00      /* Stopwatch 100ths of seconds register */
                                                     /* Bits 0-7: 100ths of seconds register (0-99 BCD) */
#define PCF85263_STW_SECONDS               0x01      /* Stopwatch seconds register */
#define PCF85263_STW_SECONDS_MASK        (0x7f)    /* Bits 0-6: Seconds (0-59 BCD) */
#define PCF85263_STW_SECONDS_OS          (1 << 7)  /* Bit 7: Oscillator stop */

#define PCF85263_STW_MINUTES               0x02      /* Stopwatch minutes register */
#define PCF85263_STW_MINUTES_MASK        (0x7f)    /* Bits 0-6: Minutes (0-59 BCD) */

#define PCF85263_STW_HOURS_XX_XX_00        0x03      /* Stopwatch hours register xx_xx_00 */
                                                     /* Bits 0-7: hours (0-99 BCD) */
#define PCF85263_STW_HOURS_XX_00_XX        0x04      /* Stopwatch hours register xx_00_xx */
                                                     /* Bits 0-7: hours (0-99 BCD) */
#define PCF85263_STW_HOURS_00_XX_XX        0x05      /* Stopwatch hours register 00_xx_xx */
                                                     /* Bits 0-7: hours (0-99 BCD) */

/* Stop-watch alarm1 */

#define PCF85263_STW_SECOND_ALM1           0x08      /* Stopwatch alarm1 seconds register */
#define PCF85263_STW_SECOND_ALM1_MASK    (0x7f)    /* Bits 0-6: Seconds (0-59 BCD) */

#define PCF85263_STW_MINUTE_ALM1           0x09      /* Stopwatch alarm1 minutes register */
#define PCF85263_STW_MINUTE_ALM1_MASK    (0x7f)    /* Bits 0-6: Minutes (0-59 BCD) */

#define PCF85263_STW_HR_XX_XX_00_ALM1      0x0a      /* Stopwatch alarm1 hours register xx_xx_00 */
                                                     /* Bits 0-7: hours (0-99 BCD) */
#define PCF85263_STW_HR_XX_00_XX_ALM1      0x0b      /* Stopwatch alarm1 hours register xx_00_xx */
                                                     /* Bits 0-7: hours (0-99 BCD) */
#define PCF85263_STW_HR_00_XX_XX_ALM1      0x0c      /* Stopwatch alarm1 hours register 00_xx_xx */
                                                     /* Bits 0-7: hours (0-99 BCD) */

/* Stop-watch alarm2 */

#define PCF85263_STW_MINUTE_ALM2           0x0d      /* Stopwatch alarm2 minutes register */
#define PCF85263_STW_MINUTE_ALM2_MASK    (0x7f)    /* Bits 0-6: Minutes (0-59 BCD) */

#define PCF85263_STW_HR_XX_00_ALM2         0x0e      /* Stopwatch alarm2 hours register xx_00_xx */
                                                     /* Bits 0-7: hours (0-99 BCD) */
#define PCF85263_STW_HR_00_XX_ALM2         0x0f      /* Stopwatch alarm2 hours register 00_xx_xx */
                                                     /* Bits 0-7: hours (0-99 BCD) */

/* Stop-watch alarm enables */

#define PCF85263_STW_ALARM_ENABLES         0x10      /* Alarm enable control register */
#define PCF85263_STW_SEC_A1E             (1 << 0)  /* Bit 0: Second alarm1 enable */
#define PCF85263_STW_MIN_A1E             (1 << 1)  /* Bit 1: Minute alarm1 enable */
#define PCF85263_STW_HR_XX_XX_00_A1E     (1 << 2)  /* Bit 2: Tens of hour alarm1 enable */
#define PCF85263_STW_HR_XX_00_XX_A1E     (1 << 3)  /* Bit 3: Thousands of hours alarm1 enable */
#define PCF85263_STW_HR_00_XX_XX_A1E     (1 << 4)  /* Bit 4: 100 thousands of hours alarm1 enable */
#define PCF85263_STW_MIN_A2E             (1 << 5)  /* Bit 5: Minute alarm2 enable */
#define PCF85263_STW_HR_XX_00_A2E        (1 << 6)  /* Bit 6: Tens of hours alarm2 enable */
#define PCF85263_STW_HR_00_XX_A2E        (1 << 7)  /* Bit 7: Thousands of hours alarm2 enable */

/* Stop-watch timestamp1 (TSR1) */

#define PCF85263_STW_TSR1_SECONDS          0x11      /* TSR1 seconds register */
#define PCF85263_STW_TSR1_SECONDS_MASK   (0x7f)    /* Bits 0-6: Seconds (0-59 BCD) */

#define PCF85263_STW_TSR1_MINUTES          0x12      /* TSR1 minutes register */
#define PCF85263_STW_TSR1_MINUTES_MASK   (0x7f)    /* Bits 0-6: Minutes (0-59 BCD) */

#define PCF85263_STW_TSR1_HR_XX_XX_00      0x13      /* Stopwatch TSR1 hours register xx_xx_00 */
                                                     /* Bits 0-7: hours (0-99 BCD) */
#define PCF85263_STW_TSR1_HR_XX_00_XX      0x14      /* Stopwatch TSR1 hours register xx_00_xx */
                                                     /* Bits 0-7: hours (0-99 BCD) */
#define PCF85263_STW_TSR1_HR_00_XX_XX      0x15      /* Stopwatch TSR1 hours register 00_xx_xx */
                                                     /* Bits 0-7: hours (0-99 BCD) */

/* Stop-watch timestamp2 (TSR2) */

#define PCF85263_STW_TSR2_SECONDS          0x17      /* TSR2 seconds register */
#define PCF85263_STW_TSR2_SECONDS_MASK   (0x7f)    /* Bits 0-6: Seconds (0-59 BCD) */

#define PCF85263_STW_TSR2_MINUTES          0x18      /* TSR2 minutes register */
#define PCF85263_RTC_TSR2_MINUTES_MASK   (0x7f)    /* Bits 0-6: Minutes (0-59 BCD) */

#define PCF85263_STW_TSR2_HR_XX_XX_00      0x19      /* Stopwatch TSR2 hours register xx_xx_00 */
                                                     /* Bits 0-7: hours (0-99 BCD) */
#define PCF85263_STW_TSR2_HR_XX_00_XX      0x1a      /* Stopwatch TSR2 hours register xx_00_xx */
                                                     /* Bits 0-7: hours (0-99 BCD) */
#define PCF85263_STW_TSR2_HR_00_XX_XX      0x1b      /* Stopwatch TSR2 hours register 00_xx_xx */
                                                     /* Bits 0-7: hours (0-99 BCD) */

/* Stop-watch timestamp3 (TSR3) */

#define PCF85263_STW_TSR3_SECONDS          0x1d      /* TSR3 seconds register */
#define PCF85263_STW_TSR3_SECONDS_MASK   (0x7f)    /* Bits 0-6: Seconds (0-59 BCD) */

#define PCF85263_STW_TSR3_MINUTES          0x1e      /* TSR3 minutes register */
#define PCF85263_RTC_TSR3_MINUTES_MASK   (0x7f)    /* Bits 0-6: Minutes (0-59 BCD) */

#define PCF85263_STW_TSR3_HR_XX_XX_00      0x1f      /* Stopwatch TSR3 hours register xx_xx_00 */
                                                     /* Bits 0-7: hours (0-99 BCD) */
#define PCF85263_STW_TSR3_HR_XX_00_XX      0x20      /* Stopwatch TSR3 hours register xx_00_xx */
                                                     /* Bits 0-7: hours (0-99 BCD) */
#define PCF85263_STW_TSR3_HR_00_XX_XX      0x21      /* Stopwatch TSR3 hours register 00_xx_xx */
                                                     /* Bits 0-7: hours (0-99 BCD) */

/* Stop-watch timestamp mode control */

#define PCF85263_STW_TSR_MODE              0x23      /* Timestamp mode control register */
#define PCF85263_STW_TSR_TSR1M_SHIFT     (0)       /* Bit 0-1: Timestamp register 1 mode */
#define PCF85263_STW_TSR_TSR1M_MASK      (3 << PCF85263_STW_TSR_TSR1M_SHIFT)
#define PCF85263_STW_TSR_TSR1M_NONE    (0 << PCF85263_STW_TSR_TSR1M_SHIFT) /* No timestamp */
#define PCF85263_STW_TSR_TSR1M_FE      (1 << PCF85263_STW_TSR_TSR1M_SHIFT) /* Record First TS pin Event */
#define PCF85263_STW_TSR_TSR1M_LE      (2 << PCF85263_STW_TSR_TSR1M_SHIFT) /* Record Last TS pin Event */
#define PCF85263_STW_TSR_TSR2M_SHIFT     (2)       /* Bit 2-4: Timestamp register 2 mode */
#define PCF85263_STW_TSR_TSR2M_MASK      (7 << PCF85263_STW_TSR_TSR2M_SHIFT)
#define PCF85263_STW_TSR_TSR2M_NONE    (0 << PCF85263_STW_TSR_TSR2M_SHIFT) /* No timestamp */
#define PCF85263_STW_TSR_TSR2M_FB      (1 << PCF85263_STW_TSR_TSR2M_SHIFT) /* Record First time switch to Battery event */
#define PCF85263_STW_TSR_TSR2M_LB      (2 << PCF85263_STW_TSR_TSR2M_SHIFT) /* Record Last time switch to Battery event */
#define PCF85263_STW_TSR_TSR2M_LV      (3 << PCF85263_STW_TSR_TSR2M_SHIFT) /* Record Last time switch to VDD event */
#define PCF85263_STW_TSR_TSR2M_FE      (4 << PCF85263_STW_TSR_TSR2M_SHIFT) /* Record First TS pin Event */
#define PCF85263_STW_TSR_TSR2M_LE      (5 << PCF85263_STW_TSR_TSR2M_SHIFT) /* Record Last TS pin Event */
#define PCF85263_STW_TSR_TSR3M_SHIFT     (6)       /* Bit 6-7: Timestamp register 3 mode */
#define PCF85263_STW_TSR_TSR3M_MASK      (3 << PCF85263_STW_TSR_TSR3M_SHIFT)
#define PCF85263_STW_TSR_TSR3M_NONE    (0 << PCF85263_STW_TSR_TSR3M_SHIFT) /* No timestamp */
#define PCF85263_STW_TSR_TSR3M_FB      (1 << PCF85263_STW_TSR_TSR3M_SHIFT) /* Record First time switch to Battery event */
#define PCF85263_STW_TSR_TSR3M_LB      (2 << PCF85263_STW_TSR_TSR3M_SHIFT) /* Record Last time switch to Battery event */
#define PCF85263_STW_TSR_TSR3M_LV      (3 << PCF85263_STW_TSR_TSR3M_SHIFT) /* Record Last time switch to VDD event */

/* Offset register */

#define PCF85263_CTL_OFFSET                0x24      /* Offset regsiter */
                                                     /* Bits 0-7: Offset value */

/* Control registers */

#define PCF85263_CTL_OSCILLATOR            0x25      /* Oscillator control register */
#define PCF85263_CTL_OSC_CL_SHIFT        (0)       /* Bits 0-1: Quartz oscillator load capacitance */
#define PCF85263_CTL_OSC_CL_MASK         (3 << PCF85263_CTL_OSC_CL_SHIFT)
#define PCF85263_CTL_OSC_CL_7PF        (0 << PCF85263_CTL_OSC_CL_SHIFT) /* 7.0 pF */
#define PCF85263_CTL_OSC_CL_6PF        (1 << PCF85263_CTL_OSC_CL_SHIFT) /* 6.0 pF */
#define PCF85263_CTL_OSC_CL_12p5PF     (2 << PCF85263_CTL_OSC_CL_SHIFT) /* 12.5 pF */
#define PCF85263_CTL_OSC_OSCD_SHIFT      (2)       /* Bits 1-2: Oscillator driver bits */
#define PCF85263_CTL_OSC_OSCD_MASK       (3 << PCF85263_CTL_OSC_OSCD_SHIFT)
#define PCF85263_CTL_OSC_OSCD_NORMAL   (0 << PCF85263_CTL_OSC_OSCD_SHIFT) /* Normal drive; RS(max): 100 kohm */
#define PCF85263_CTL_OSC_OSCD_LOW      (1 << PCF85263_CTL_OSC_OSCD_SHIFT) /* Low drive; RS(max): 60 kohm; reduced IDD */
#define PCF85263_CTL_OSC_OSCD_HIGH     (2 << PCF85263_CTL_OSC_OSCD_SHIFT) /* High drive; RS(max): 500 kohm; increased IDD */
#define PCF85263_CTL_OSC_LOWJ            (1 << 4)  /* Bit 4:  Low jitter mode */
#define PCF85263_CTL_OSC_12_24           (1 << 5)  /* Bit 5:  12-/24-hour mode */
#define PCF85263_CTL_OSC_OFFM            (1 << 6)  /* Bit 6:  Offset calibration mode */
#define PCF85263_CTL_OSC_CLKIV           (1 << 7)  /* Bit 7:  Output clock inversion */

#define PCF85263_CTL_BATTERY_SWITCH        0x26      /* Battery switch control register */
#define PCF85263_CTL_BATTERY_BSTH        (1 << 0)  /* Bit 0: Threshold voltage control */
#define PCF85263_CTL_BATTERY_BSM_SHIFT   (1)       /* Bits 1-2: Battery switch mode bits */
#define PCF85263_CTL_BATTERY_BSM_MASK    (3 << PCF85263_CTL_BATTERY_BSM_SHIFT)
#define PCF85263_CTL_BATTERY_BSM_VTH   (0 << PCF85263_CTL_BATTERY_BSM_SHIFT) /* Switching at the Vth level */
#define PCF85263_CTL_BATTERY_BSM_VBAT  (1 << PCF85263_CTL_BATTERY_BSM_SHIFT) /* Switching at the VBAT level */
#define PCF85263_CTL_BATTERY_BSM_MAX   (2 << PCF85263_CTL_BATTERY_BSM_SHIFT) /* Switching at the higher level of Vth or VBAT */
#define PCF85263_CTL_BATTERY_BSM_MIN   (3 << PCF85263_CTL_BATTERY_BSM_SHIFT) /* Switching at the lower level of Vth or VBAT */
#define PCF85263_CTL_BATTERY_BSRR        (1 << 3)  /* Bit 3:  Battery switch refresh rate */
#define PCF85263_CTL_BATTERY_BSOFF       (1 << 4)  /* Bit 4:  Battery switch on/off */

#define PCF85263_CTL_PIN_IO                0x27      /* Pin input/output control register */
#define PCF85263_CTL_INTAPM_SHIFT        (0)       /* Bits 0-1: INTA pin mode */
#define PCF85263_CTL_INTAPM_MASK         (3 << PCF85263_CTL_INTAPM_SHIFT)
#define PCF85263_CTL_INTAPM_CLK        (0 << PCF85263_CTL_INTAPM_SHIFT) /* CLK output mode */
#define PCF85263_CTL_INTAPM_BAT        (1 << PCF85263_CTL_INTAPM_SHIFT) /* Battery mode indication */
#define PCF85263_CTL_INTAPM_INTA       (2 << PCF85263_CTL_INTAPM_SHIFT) /* INTA output */
#define PCF85263_CTL_INTAPM_HIZ        (3 << PCF85263_CTL_INTAPM_SHIFT) /* Hi-Z */
#define PCF85263_CTL_TSPM_SHIFT          (2)       /* Bits 2-3: TS pin I/O control */
#define PCF85263_CTL_TSPM_MASK           (3 << PCF85263_CTL_TSPM_SHIFT)
#define PCF85263_CTL_TSPM_DISABLED     (0 << PCF85263_CTL_TSPM_SHIFT) /* Disabled; input can be left floating */
#define PCF85263_CTL_TSPM_INTB         (1 << PCF85263_CTL_TSPM_SHIFT) /* INTB output; push-pull */
#define PCF85263_CTL_TSPM_CLK          (2 << PCF85263_CTL_TSPM_SHIFT) /* CLK output; push-pull */
#define PCF85263_CTL_TSPM_INPUT        (3 << PCF85263_CTL_TSPM_SHIFT) /* Input mode */
#define PCF85263_CTL_TSIM                (1 << 4)  /* Bit 4:  TS pin input mode */
#define PCF85263_CTL_TSL                 (1 << 5)  /* Bit 5:  TS pin input sense */
#define PCF85263_CTL_TSPULL              (1 << 6)  /* Bit 6:  TS pin pull-up resistor value */
#define PCF85263_CTL_CLKPM               (1 << 7)  /* Bit 7:  CLK pin mode */

#define PCF85263_CTL_FUNCTION              0x28      /* Function control register */
#define PCF85263_CTL_FUNC_COF_SHIFT      (0)       /* Bits 0-2: Clock output frequency */
#define PCF85263_CTL_FUNC_COF_MASK       (7 << PCF85263_CTL_FUNC_COF_SHIFT) /* CLK pin    TS pin     INTA pin */
#define PCF85263_CTL_FUNC_COF_32KHZ    (0 << PCF85263_CTL_FUNC_COF_SHIFT) /* 32768      32768      32768    */
#define PCF85263_CTL_FUNC_COF_16KHZ    (1 << PCF85263_CTL_FUNC_COF_SHIFT) /* 16384      16384      16384    */
#define PCF85263_CTL_FUNC_COF_8KHZ     (2 << PCF85263_CTL_FUNC_COF_SHIFT) /* 8192       8192       8192     */
#define PCF85263_CTL_FUNC_COF_4KHZ     (3 << PCF85263_CTL_FUNC_COF_SHIFT) /* 4096       4096       4096     */
#define PCF85263_CTL_FUNC_COF_2KHZ     (4 << PCF85263_CTL_FUNC_COF_SHIFT) /* 2048       2048       2048     */
#define PCF85263_CTL_FUNC_COF_1KHZ     (5 << PCF85263_CTL_FUNC_COF_SHIFT) /* 1024       1024       1024     */
#define PCF85263_CTL_FUNC_COF_1HZ      (6 << PCF85263_CTL_FUNC_COF_SHIFT) /* 1          1          1        */
#define PCF85263_CTL_FUNC_COF_LOW      (7 << PCF85263_CTL_FUNC_COF_SHIFT) /* static LOW static LOW Hi-Z     */
#define PCF85263_CTL_FUNC_STOPM          (1 << 3)  /* Bit 3:  STOP mode */
#define PCF85263_CTL_FUNC_RTCM           (1 << 4)  /* Bit 4:  RTC mode */
#define PCF85263_CTL_FUNC_PI_SHIFT       (5)       /* Bits 5-6: Periodic interrupt */
#define PCF85263_CTL_FUNC_PI_MASK        (3 << PCF85263_CTL_FUNC_PI_SHIFT)
#define PCF85263_CTL_FUNC_PI_NONE      (0 << PCF85263_CTL_FUNC_PI_SHIFT) /* No periodic interrupt */
#define PCF85263_CTL_FUNC_PI_SEC       (1 << PCF85263_CTL_FUNC_PI_SHIFT) /* Once per second */
#define PCF85263_CTL_FUNC_PI_MIN       (2 << PCF85263_CTL_FUNC_PI_SHIFT) /* Once per minute */
#define PCF85263_CTL_FUNC_PI_HOUR      (3 << PCF85263_CTL_FUNC_PI_SHIFT) /* Once per hour */
#define PCF85263_CTL_FUNC_100TH          (1 << 7)  /* Bit 7:  100th seconds mode */

#define PCF85263_CTL_INTA_ENABLE           0x29      /* Interrupt A control bits */
#define PCF85263_CTL_INTA_WDIEA          (1 << 0)  /* Bit 0:  Watchdog interrupt enable */
#define PCF85263_CTL_INTA_BSIEA          (1 << 1)  /* Bit 1:  Battery switch interrupt enable */
#define PCF85263_CTL_INTA_TSRIEA         (1 << 2)  /* Bit 2:  Timestamp register interrupt enable */
#define PCF85263_CTL_INTA_A2IEA          (1 << 3)  /* Bit 3:  Alarm2 interrupt enable */
#define PCF85263_CTL_INTA_A1IEA          (1 << 4)  /* Bit 4:  Alarm1 interrupt enable */
#define PCF85263_CTL_INTA_OIEA           (1 << 5)  /* Bit 5:  Offset correction interrupt enable */
#define PCF85263_CTL_INTA_PIEA           (1 << 6)  /* Bit 6:  Periodic interrupt enable */
#define PCF85263_CTL_INTA_ILPA           (1 << 7)  /* Bit 7:  Interrupt generates a pulse */

#define PCF85263_CTL_INTB_ENABLE           0x2a      /* Interrupt B control bits */
#define PCF85263_CTL_INTB_WDIEB          (1 << 0)  /* Bit 0:  Watchdog interrupt enable */
#define PCF85263_CTL_INTB_BSIEB          (1 << 1)  /* Bit 1:  Battery switch interrupt enable */
#define PCF85263_CTL_INTB_TSRIEB         (1 << 2)  /* Bit 2:  Timestamp register interrupt enable */
#define PCF85263_CTL_INTB_A2IEB          (1 << 3)  /* Bit 3:  Alarm2 interrupt enable */
#define PCF85263_CTL_INTB_A1IEB          (1 << 4)  /* Bit 4:  Alarm1 interrupt enable */
#define PCF85263_CTL_INTB_OIEB           (1 << 5)  /* Bit 5:  Offset correction interrupt enable */
#define PCF85263_CTL_INTB_PIEB           (1 << 6)  /* Bit 6:  Periodic interrupt enable */
#define PCF85263_CTL_INTB_ILPB           (1 << 7)  /* Bit 7:  Interrupt generates a pulse */

#define PCF85263_CTL_FLAGS                 0x2b      /* Flag status register */
#define PCF85263_CTL_FLAGS_TSR1F         (1 << 0)  /* Bit 0:  Timestamp register 1 event flag */
#define PCF85263_CTL_FLAGS_TSR2F         (1 << 1)  /* Bit 1:  Timestamp register 2 event flag */
#define PCF85263_CTL_FLAGS_TSR3F         (1 << 2)  /* Bit 2:  Timestamp register 3 event flag */
#define PCF85263_CTL_FLAGS_BSF           (1 << 3)  /* Bit 3:  Battery switch flag */
#define PCF85263_CTL_FLAGS_WDF           (1 << 4)  /* Bit 4:  Watchdog flag */
#define PCF85263_CTL_FLAGS_A1F           (1 << 5)  /* Bit 5:  Alarm1 flag */
#define PCF85263_CTL_FLAGS_A2F           (1 << 6)  /* Bit 6:  Alarm2 flag */
#define PCF85263_CTL_FLAGS_PIF           (1 << 7)  /* Bit 7:  Periodic interrupt flag */

/* RAM byte */

#define PCF85263_CTL_RAM_BYTE              0x2c      /* RAM byte register */
                                                     /* Bits 0-7: RAM data */

/* Watchdog registers */

#define PCF85263_CTL_WATCHDOG              0x2d      /* Watchdog control and status register */
#define PCF85263_CTL_WDS_SHIFT           (0)       /* Bits 0-1: Watchdog step size (source clock) */
#define PCF85263_CTL_WDS_MASK            (3 << PCF85263_CTL_WDS_SHIFT)
#define PCF85263_CTL_WDS_4SEC          (0 << PCF85263_CTL_WDS_SHIFT) /* 4 seconds (0.25 Hz) */
#define PCF85263_CTL_WDS_1SEC          (1 << PCF85263_CTL_WDS_SHIFT) /* 1 second (1 Hz) */
#define PCF85263_CTL_WDS_250MSEC       (2 << PCF85263_CTL_WDS_SHIFT) /* 1⁄4 second (4 Hz) */
#define PCF85263_CTL_WDS_67MSEC        (3 << PCF85263_CTL_WDS_SHIFT) /* 1⁄16 second (16 Hz) */
#define PCF85263_CTL_WDR_SHIFT           (2)       /* Bits 2-6: Watchdog register bits */
#define PCF85263_CTL_WDR_MASK            (31 << PCF85263_CTL_WDR_SHIFT)
#define PCF85263_CTL_WDR(n)            ((uint9_t)(n) << PCF85263_CTL_WDR_SHIFT)
#define PCF85263_CTL_WDM                 (1 << 7)  /* Bit 7:  Watchdog mode */

/* Stop */

#define PCF85263_CTL_STOP_ENABLE           0x2e      /* Stop enable register */
#define PCF85263_CTL_STOP                (1 << 0)  /* Bit 0:  Stop bit */

/* Reset */

#define PCF85263_CTL_RESETS                0x2f      /* Software reset control register */
#define PCF85263_CTL_CTS                 (1 << 0)  /* Bit 0:  Clear timestamp */
#define PCF85263_CTL_SR                  (1 << 3)  /* Bit 3:  Software reset */
#define PCF85263_CTL_CPR                 (1 << 7)  /* Bit 7:  Clear prescaler */
#define PCF85263_CTL_RESETS_BITS         0x24      /* Fixed register bits */

#endif /* __DRIVERS_TIMERS_PCF85263_H */

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

void GetRtcRegisters(uint8_t registerAddress, uint8_t* registerData, uint16_t dataLength);
void SetRtcRegisters(uint8_t registerAddress, uint8_t* registerData, uint16_t dataLength);

// Timer mode related
void TimerModeActiveMinutes(void);
uint8 ValidateTimerModeSettings(void);

#endif // _RTC_H_
