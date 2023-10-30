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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "Typedefs.h"
#include "RealTimeClock.h"
#include "SoftTimer.h"
#include "OldUart.h"
#include "spi.h"

#include "ff.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define	RTC_SPI_NPCS	1

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
#include "Globals.h"

///----------------------------------------------------------------------------
///	Local Scope Globals
///----------------------------------------------------------------------------
DATE_TIME_STRUCT s_time = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
DATE_TIME_STRUCT s_currentTime = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
BOOLEAN ExternalRtcInit(void)
{
	// REDO & Update to new RTC
	DATE_TIME_STRUCT time;
	RTC_MEM_MAP_STRUCT rtcMap;

	// Initialize the soft timer array
	memset(&g_rtcTimerBank[0], 0, sizeof(g_rtcTimerBank));

	// Get the base of the external RTC memory map
	ExternalRtcRead(RTC_CONTROL_1_ADDR, 10, (uint8*)&rtcMap);
	
	if ((rtcMap.seconds & RTC_CLOCK_INTEGRITY) || (rtcMap.control_1 & RTC_CLOCK_STOPPED))
	{
		debug("Init RTC: Clock integrity not guaranteed or Clock stopped, setting default time and date\r\n");

		rtcMap.control_1 |= RTC_24_HOUR_MODE;
		ExternalRtcWrite(RTC_CONTROL_1_ADDR, 1, &rtcMap.control_1);

		// Setup an initial date and time
		time.year = 16;
		time.month = 1;
		time.day = 1;
		time.weekday = GetDayOfWeek(time.year, time.month, time.day);
		time.hour = 8;
		time.min = 1;
		time.sec = 1;

		SetExternalRtcDate(&time);
		SetExternalRtcTime(&time);
	}
	else
	{
		debug("Ext RTC: Clock running and intergrity validated\r\n");
	}

#if 1 /* Normal */
	// Clear all flags (write 0's) and disable interrupt generation for all but timestamp pin
	rtcMap.control_2 = (RTC_ALARM_INT_ENABLE);
	ExternalRtcWrite(RTC_CONTROL_2_ADDR, 1, &rtcMap.control_2);
#else /* Test */
	// Disable alarm settings and clear flags 
	DisableExternalRtcAlarm();
#endif

	// Need to initialize the global Current Time
	UpdateCurrentTime();

	// Check for RTC reset
	if ((g_lastReadExternalRtcTime.year == 0) || (g_lastReadExternalRtcTime.month == 0) || (g_lastReadExternalRtcTime.day == 0))
	{
		debugWarn("Warning: External RTC date not set, assuming power loss reset... applying a default date\r\n");
		// BCD formats
		g_lastReadExternalRtcTime.year = 0x14;
		g_lastReadExternalRtcTime.month = 0x01;
		g_lastReadExternalRtcTime.day = 0x01;

		SetExternalRtcDate(&g_lastReadExternalRtcTime);
	}

	// Set the clock out control to turn off any clock interrupt generation
	rtcMap.clock_out_control = (0x07);
	ExternalRtcWrite(RTC_CLOCK_OUT_CONTROL_ADDR, 1, &rtcMap.clock_out_control);

	return (TRUE);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void StartExternalRtcClock(uint16 sampleRate)
{
	RTC_MEM_MAP_STRUCT rtcMap;
	uint8 clockRate;

	switch (sampleRate)
	{
		case 32768	: clockRate = 0x00; break;
		case 16384	: clockRate = 0x01; break;
		case 8192	: clockRate = 0x02; break;
		case 4096	: clockRate = 0x03; break;
		case 2048	: clockRate = 0x04; break;
		case 1024	: clockRate = 0x05; break;
		case 512	: clockRate = 0x05; break;
		case 1		: clockRate = 0x06; break;
		default		: clockRate = 0x05; break; // set to 1024
	}

	rtcMap.clock_out_control = (clockRate);
	ExternalRtcWrite(RTC_CLOCK_OUT_CONTROL_ADDR, 1, &rtcMap.clock_out_control);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void StopExternalRtcClock(void)
{
	RTC_MEM_MAP_STRUCT rtcMap;

	debug("Stoping External RTC Interrupt\r\n");

	// Set the clock out control to turn off any clock interrupt generation
	rtcMap.clock_out_control = (0x07);
	ExternalRtcWrite(RTC_CLOCK_OUT_CONTROL_ADDR, 1, &rtcMap.clock_out_control);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 SetExternalRtcTime(DATE_TIME_STRUCT* time)
{
	RTC_TIME_STRUCT rtcTime;
	uint8 status = FAILED;

	if ((time->hour < 24) && (time->min < 60) && (time->sec < 60))
	{
		status = PASSED;

		// Setup time registers (24 Hour settings), BCD format
		rtcTime.hours = UINT8_CONVERT_TO_BCD(time->hour, RTC_BCD_HOURS_MASK);
		rtcTime.minutes = UINT8_CONVERT_TO_BCD(time->min, RTC_BCD_MINUTES_MASK);
		rtcTime.seconds = UINT8_CONVERT_TO_BCD(time->sec, RTC_BCD_SECONDS_MASK);

		ExternalRtcWrite(RTC_SECONDS_ADDR_TEMP, 3, (uint8*)&rtcTime);
	}

	return (status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 SetExternalRtcDate(DATE_TIME_STRUCT* time)
{
	RTC_DATE_STRUCT rtcDate;
	uint8 status = FAILED;

	// Check if years and months settings are valid
	if ((time->year <= 99) && (time->month > 0) && (time->month <= TOTAL_MONTHS))
	{
		// Check is the days setting is valid for the month given that month setting has been validated
		if ((time->day > 0) && (time->day <= GetDaysPerMonth(time->month, time->year)))
		{
			// Flag success since month, day and year settings are valid
			status = PASSED;

			// Setup time registers (24 Hour settings), BCD format
			rtcDate.years = UINT8_CONVERT_TO_BCD(time->year, RTC_BCD_YEARS_MASK);
			rtcDate.months = UINT8_CONVERT_TO_BCD(time->month, RTC_BCD_MONTHS_MASK);
			rtcDate.days = UINT8_CONVERT_TO_BCD(time->day, RTC_BCD_DAYS_MASK);

			// Calculate the weekday based on the new date
			rtcDate.weekdays = GetDayOfWeek(time->year, time->month, time->day);

			//debug("Ext RTC: Apply Date: %x-%x-%x\r\n", rtcDate.months, rtcDate.days, rtcDate.years);

			ExternalRtcWrite(RTC_DAYS_ADDR, 4, (uint8*)&rtcDate);
		}
	}

	return (status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 SetExternalRtcDateAndTimeByGpsUtcEpoch(time_t currentEpochTime)
{
	DATE_TIME_STRUCT currentDateTime;
	uint8 status = FAILED;

	// Adjust the GPS UTC by the current time zone offset
	currentEpochTime += (g_unitConfig.utcZoneOffset * 3600);

	currentDateTime = ConvertEpochTimeToDateTime(currentEpochTime);

	status = SetExternalRtcTime(&currentDateTime);
	if (status == PASSED) { status = SetExternalRtcDate(&currentDateTime); }

	if (status == PASSED) { status = UpdateCurrentTime(); }

	return (status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
DATE_TIME_STRUCT GetExternalRtcTime(void)
{
	RTC_DATE_TIME_STRUCT translateTime;

	ExternalRtcRead(RTC_SECONDS_ADDR_TEMP, 7, (uint8*)&translateTime);

	// Get time and date registers (24 Hour settings), BCD format
	s_time.year = BCD_CONVERT_TO_UINT8(translateTime.years, RTC_BCD_YEARS_MASK);
	s_time.month = BCD_CONVERT_TO_UINT8(translateTime.months, RTC_BCD_MONTHS_MASK);
	s_time.day = BCD_CONVERT_TO_UINT8(translateTime.days, RTC_BCD_DAYS_MASK);
	s_time.weekday = BCD_CONVERT_TO_UINT8(translateTime.weekdays, RTC_BCD_WEEKDAY_MASK);

	s_time.hour = BCD_CONVERT_TO_UINT8(translateTime.hours, RTC_BCD_HOURS_MASK);
	s_time.min = BCD_CONVERT_TO_UINT8(translateTime.minutes, RTC_BCD_MINUTES_MASK);
	s_time.sec = BCD_CONVERT_TO_UINT8(translateTime.seconds, RTC_BCD_SECONDS_MASK);

	//debug("Ext RTC: Get Time: %02d:%02d:%02d (%d), %02d-%02d-%02d\n\r\n", time.hour, time.min, time.sec, time.weekday, time.month, time.day, time.year);

	return (s_time);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 UpdateCurrentTime(void)
{
	uint8 status = FAILED;

	// Check if SPI1 access is available (actual lock is inside Get External RTC time)
	if (g_spi1AccessLock == AVAILABLE)
	{
		g_rtcTickCountSinceLastExternalUpdate = 0;
		g_lastReadExternalRtcTime = GetExternalRtcTime();
		status = PASSED;
	}

	return (status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
DATE_TIME_STRUCT GetCurrentTime(void)
{
	uint32 accumulatedSeconds = (g_rtcTickCountSinceLastExternalUpdate / 2);
	struct tm currentTime;
	struct tm *convertTime;
	time_t epochTime;

	s_currentTime = g_lastReadExternalRtcTime;

	if (accumulatedSeconds)
	{
		currentTime.tm_year = (g_lastReadExternalRtcTime.year + 100); // From 1900;
		currentTime.tm_mon = (g_lastReadExternalRtcTime.month - 1); // Month, 0 - jan
		currentTime.tm_mday = g_lastReadExternalRtcTime.day; // Day of the month
		currentTime.tm_hour = g_lastReadExternalRtcTime.hour;
		currentTime.tm_min = g_lastReadExternalRtcTime.min;
		currentTime.tm_sec = g_lastReadExternalRtcTime.sec;
		currentTime.tm_isdst = -1; // Is DST on? 1 = yes, 0 = no, -1 = unknown
		epochTime = mktime(&currentTime);

		epochTime += accumulatedSeconds;

		convertTime = localtime(&epochTime);

		s_currentTime.year = (convertTime->tm_year - 100);
		s_currentTime.month = (convertTime->tm_mon + 1);
		s_currentTime.day = convertTime->tm_mday;
		s_currentTime.hour = convertTime->tm_hour;
		s_currentTime.min = convertTime->tm_min;
		s_currentTime.sec = convertTime->tm_sec;
		s_currentTime.weekday = GetDayOfWeek(s_currentTime.year, s_currentTime.month, s_currentTime.day);
	}

	return (s_currentTime);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint32 GetCurrentEpochTime(void)
{
	uint32 accumulatedSeconds = (g_rtcTickCountSinceLastExternalUpdate / 2);
	struct tm convertTime;
	time_t epochTime;

	convertTime.tm_year = (g_lastReadExternalRtcTime.year + 100); // From 1900;
	convertTime.tm_mon = (g_lastReadExternalRtcTime.month - 1); // Month, 0 - jan
	convertTime.tm_mday = g_lastReadExternalRtcTime.day; // Day of the month
	convertTime.tm_hour = g_lastReadExternalRtcTime.hour;
	convertTime.tm_min = g_lastReadExternalRtcTime.min;
	convertTime.tm_sec = g_lastReadExternalRtcTime.sec;
	convertTime.tm_isdst = -1; // Is DST on? 1 = yes, 0 = no, -1 = unknown
	epochTime = mktime(&convertTime);

	epochTime += accumulatedSeconds;

	return (epochTime);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
DATE_TIME_STRUCT ConvertEpochTimeToDateTime(time_t epochTime)
{
	struct tm *convertTime;

	convertTime = localtime(&epochTime);

	s_currentTime.year = (convertTime->tm_year - 100);
	s_currentTime.month = (convertTime->tm_mon + 1);
	s_currentTime.day = convertTime->tm_mday;
	s_currentTime.hour = convertTime->tm_hour;
	s_currentTime.min = convertTime->tm_min;
	s_currentTime.sec = convertTime->tm_sec;
	s_currentTime.weekday = GetDayOfWeek(s_currentTime.year, s_currentTime.month, s_currentTime.day);

	return (s_currentTime);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
time_t ConvertDateTimeToEpochTime(DATE_TIME_STRUCT dateTime)
{
	struct tm currentTime;
	time_t epochTime;

	currentTime.tm_year = (dateTime.year + 100); // From 1900;
	currentTime.tm_mon = (dateTime.month - 1); // Month, 0 - jan
	currentTime.tm_mday = dateTime.day; // Day of the month
	currentTime.tm_hour = dateTime.hour;
	currentTime.tm_min = dateTime.min;
	currentTime.tm_sec = dateTime.sec;
	currentTime.tm_isdst = -1; // Is DST on? 1 = yes, 0 = no, -1 = unknown

	epochTime = mktime(&currentTime);

	return (epochTime);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ConvertCurrentTimeForFat(uint8* fatTimeField)
{
	DATE_TIME_STRUCT currentTime = GetCurrentTime();
	uint16 conversionTime;
	
/*
	The File Time:	The two bytes at offsets 0x16 and 0x17 are treated as a 16 bit value; remember that the least significant byte is at offset 0x16. They contain the time when the file was created or last updated. The time is mapped in the bits as follows; the first line indicates the byte's offset, the second line indicates (in decimal) individual bit numbers in the 16 bit value, and the third line indicates what is stored in each bit.
	<------- 0x17 --------> <------- 0x16 -------->
	15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
	h..h..h..h..h..m..m..m..m..m..m..x..x..x..x..x
	hhhhh	--> indicates the binary number of hours (0-23)
	mmmmmm	--> indicates the binary number of minutes (0-59)
	xxxxx	--> indicates the binary number of two-second periods (0-29), representing seconds 0 to 58.
*/
	
	conversionTime = ((currentTime.sec / 2) & 0x1f);
	conversionTime |= ((currentTime.min & 0x3f) << 5);
	conversionTime |= ((currentTime.hour & 0x1f) << 11);
	
	fatTimeField[0] = (uint8)(conversionTime);
	fatTimeField[1] = (uint8)(conversionTime >> 8);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ConvertCurrentDateForFat(uint8* fatDateField)
{
	DATE_TIME_STRUCT currentDate = GetCurrentTime();
	uint16 conversionDate;

/*
	The File Date:	The two bytes at offsets 0x18 and 0x19 are treated as a 16 bit value; remember that the least significant byte is at offset 0x18. They contain the date when the file was created or last updated. The date is mapped in the bits as follows; the first line indicates the byte's offset, the second line indicates (in decimal) individual bit numbers in the 16 bit value, and the third line indicates what is stored in each bit.
	<------- 0x19 --------> <------- 0x18 -------->
	15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
	y..y..y..y..y..y..y..m..m..m..m..d..d..d..d..d
	yyyyyyy	--> indicates the binary year offset from 1980 (0-119), representing the years 1980 to 2099
	mmmm	--> indicates the binary month number (1-12)
	ddddd	-->	indicates the binary day number (1-31) 
*/

	conversionDate = ((currentDate.day) & 0x1f);
	conversionDate |= ((currentDate.month & 0x0f) << 5);
	conversionDate |= (((currentDate.year + 20) & 0x7f) << 9);
	
	fatDateField[0] = (uint8)(conversionDate);
	fatDateField[1] = (uint8)(conversionDate >> 8);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DisableExternalRtcAlarm(void)
{
	RTC_ALARM_STRUCT disableAlarm;

	uint8 clearAlarmFlag = 0xEA; // Logic 0 on the bit will clear Alarm flag, bit 4

	disableAlarm.second_alarm = RTC_DISABLE_ALARM;
	disableAlarm.minute_alarm = RTC_DISABLE_ALARM;
	disableAlarm.hour_alarm = RTC_DISABLE_ALARM;
	disableAlarm.day_alarm = RTC_DISABLE_ALARM;
	disableAlarm.weekday_alarm = RTC_DISABLE_ALARM;
	
	// Turn off all the alarm enable flags
	ExternalRtcWrite(RTC_SECOND_ALARM_ADDR, 5, (uint8*)&disableAlarm);
	
	// Clear the Alarm flag
	ExternalRtcWrite(RTC_CONTROL_2_ADDR, 1, &clearAlarmFlag);
}

#if 1 /* Normal */
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void EnableExternalRtcAlarm(uint8 day, uint8 hour, uint8 minute, uint8 second)
{
	RTC_ALARM_STRUCT enableAlarm;

	uint8 clearAlarmFlag = 0xEA; // Logic 0 on the bit will clear Alarm flag, bit 4

	enableAlarm.day_alarm = (UINT8_CONVERT_TO_BCD(day, RTC_BCD_DAYS_MASK) | RTC_ENABLE_ALARM);
	enableAlarm.hour_alarm = (UINT8_CONVERT_TO_BCD(hour, RTC_BCD_HOURS_MASK) | RTC_ENABLE_ALARM);
	enableAlarm.minute_alarm = (UINT8_CONVERT_TO_BCD(minute, RTC_BCD_MINUTES_MASK) | RTC_ENABLE_ALARM);
	enableAlarm.second_alarm = (UINT8_CONVERT_TO_BCD(second, RTC_BCD_SECONDS_MASK) | RTC_ENABLE_ALARM);
	enableAlarm.weekday_alarm = RTC_DISABLE_ALARM;
	
	ExternalRtcWrite(RTC_SECOND_ALARM_ADDR, 5, (uint8*)&enableAlarm);

	// Clear the Alarm flag
	ExternalRtcWrite(RTC_CONTROL_2_ADDR, 1, &clearAlarmFlag);
	
	debug("Enable RTC Alarm with Day: %d, Hour: %d, Minute: %d and Second: %d\r\n", day, hour, minute, second);
}
#else /* Test */
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void EnableExternalRtcAlarm(uint8 day, uint8 hour, uint8 minute, uint8 second)
{
	RTC_ALARM_STRUCT enableAlarm;

	uint8 clearAlarmFlag = 0xEA; // Logic 0 on the bit will clear Alarm flag, bit 4

	enableAlarm.day_alarm = RTC_DISABLE_ALARM;
	enableAlarm.hour_alarm = RTC_DISABLE_ALARM;
	enableAlarm.minute_alarm = RTC_DISABLE_ALARM;
	enableAlarm.second_alarm = (UINT8_CONVERT_TO_BCD(second, RTC_BCD_SECONDS_MASK) | RTC_ENABLE_ALARM);
	enableAlarm.weekday_alarm = RTC_DISABLE_ALARM;
	
	ExternalRtcWrite(RTC_SECOND_ALARM_ADDR, 5, (uint8*)&enableAlarm);

	// Clear the Alarm flag
	ExternalRtcWrite(RTC_CONTROL_2_ADDR, 1, &clearAlarmFlag);
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ExternalRtcWrite(uint8 registerAddress, int length, uint8* data)
{
#if 0 /* temp remove while unsed */
	uint16 dataContainer = 0;
#endif

	if (g_spi1AccessLock != CAL_PULSE_LOCK)
	{
		GetSpi1MutexLock(RTC_TIME_LOCK);
	}

#if 0 /* old hw */
	spi_selectChip(&AVR32_SPI1, RTC_SPI_NPCS);

	// Small delay before the RTC device is accessible
	SoftUsecWait(RTC_ACCESS_DELAY);

	spi_write(&AVR32_SPI1, (RTC_WRITE_CMD | (registerAddress & 0x1F)));

	while (length--)
	{
		// Small delay before the RTC device is accessible
		SoftUsecWait(RTC_ACCESS_DELAY);

		dataContainer = *data++;
		spi_write(&AVR32_SPI1, dataContainer);
	}

	spi_unselectChip(&AVR32_SPI1, RTC_SPI_NPCS);
#endif

	if (g_spi1AccessLock != CAL_PULSE_LOCK)
	{
		ReleaseSpi1MutexLock();
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ExternalRtcRead(uint8 registerAddress, int length, uint8* data)
{
#if 0 /* temp remove while unused */
	uint16 dataContainer = 0;
#endif

	if (g_spi1AccessLock != CAL_PULSE_LOCK)
	{
		GetSpi1MutexLock(RTC_TIME_LOCK);
	}

#if 0 /* old hw */
	spi_selectChip(&AVR32_SPI1, RTC_SPI_NPCS);

	// Small delay before the RTC device is accessible
	SoftUsecWait(RTC_ACCESS_DELAY);

	spi_write(&AVR32_SPI1, (RTC_READ_CMD | (registerAddress & 0x1F)));

	while (length--)
	{
		// Small delay before the RTC device is accessible
		SoftUsecWait(RTC_ACCESS_DELAY);

		spi_write(&AVR32_SPI1, 0xFF);
		spi_read(&AVR32_SPI1, &dataContainer);

		*data++ = (uint8)dataContainer;
	}

	spi_unselectChip(&AVR32_SPI1, RTC_SPI_NPCS);
#endif

	if (g_spi1AccessLock != CAL_PULSE_LOCK)
	{
		ReleaseSpi1MutexLock();
	}
}
