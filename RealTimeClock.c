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

#include "mxc_errors.h"
#include "ff.h"
#include "i2c.h"
#include "mxc_delay.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------

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
	DATE_TIME_STRUCT time;
	uint8_t secondsReg;
	uint8_t stopEnableReg;
	uint8_t flagsReg;
	uint8_t ioConfig;
	uint8_t interruptReg;
	uint8_t controlReg;

	// Initialize the soft timer array
	memset(&g_rtcTimerBank[0], 0, sizeof(g_rtcTimerBank));

	// Get the Seconds register for the Oscillator stop bit
	GetRtcRegisters(PCF85263_RTC_SECONDS, (uint8_t*)&secondsReg, sizeof(secondsReg));
	// Get the Stop Enable register for the RTC clock stopped bit
	GetRtcRegisters(PCF85263_CTL_STOP_ENABLE, (uint8_t*)&stopEnableReg, sizeof(stopEnableReg));

	if ((secondsReg & PCF85263_RTC_SECONDS_OS) || (stopEnableReg & PCF85263_CTL_STOP))
	{
		debugWarn("External RTC: Clock integrity not guaranteed or Clock stopped, setting default time and date\r\n");

		// Desire 24HR mode, which is already the default mode for the PCF85263

		// Setup an initial date and time
		time.year = 24;
		time.month = 1;
		time.day = 1;
		time.weekday = GetDayOfWeek(time.year, time.month, time.day);
		time.hour = 8;
		time.min = 0;
		time.sec = 0;

		SetExternalRtcDate(&time);
		SetExternalRtcTime(&time);
	}
	else
	{
		debug("External RTC: Clock running and intergrity validated\r\n");
	}

#if 1 /* Normal */
	// Clear all flags
	flagsReg = 0;
	SetRtcRegisters(PCF85263_CTL_FLAGS, (uint8_t*)&flagsReg, sizeof(flagsReg));

	// Change Interrupt A to be an interrupt and not a mirrored clock
	ioConfig = PCF85263_CTL_INTAPM_INTA;
	SetRtcRegisters(PCF85263_CTL_PIN_IO, (uint8_t*)&ioConfig, sizeof(ioConfig));

	// Enable interrupts for the periodic and alarm 1
	interruptReg = (PCF85263_CTL_INTA_PIEA | PCF85263_CTL_INTA_A1IEA);
	SetRtcRegisters(PCF85263_CTL_INTA_ENABLE, (uint8_t*)&interruptReg, sizeof(interruptReg));

#if 0 /* Test 1 sec PI */
	// Turn on 1 second periodic interrupt
	debug("External RTC: Setting up 1 sec Periodic Int\r\n");
	controlReg = PCF85263_CTL_FUNC_PI_SEC;
	SetRtcRegisters(PCF85263_CTL_FUNCTION, (uint8_t*)&controlReg, sizeof(controlReg));
#elif 1 /* Test 1 min PI */
	// Turn on 1 second periodic interrupt
	debug("External RTC: Setting up 1 min Periodic Int\r\n");
	controlReg = PCF85263_CTL_FUNC_PI_MIN;
	SetRtcRegisters(PCF85263_CTL_FUNCTION, (uint8_t*)&controlReg, sizeof(controlReg));
#endif

#else /* Test */
	// Disable alarm settings and clear flags 
	DisableExternalRtcAlarm();
#endif

	// Need to initialize the global Current Time
	UpdateCurrentTime();

	// Check for RTC reset
	if ((g_lastReadExternalRtcTime.year == 0) && (g_lastReadExternalRtcTime.month == 1) && (g_lastReadExternalRtcTime.day == 1))
	{
		debugWarn("External RTC: Date not set, assuming power loss reset... applying a default date\r\n");

		g_lastReadExternalRtcTime.year = 24;
		g_lastReadExternalRtcTime.month = 1;
		g_lastReadExternalRtcTime.day = 1;

		SetExternalRtcDate(&g_lastReadExternalRtcTime);
	}

	// Set the clock out control to turn off any clock interrupt generation
#if 1 /* Normal */
	StopExternalRtcClock();
#else /* Test leaving interrupt active */
#endif

#if 1 /* Test */
	DATE_TIME_STRUCT testTime;
	uint8_t ramData;
	// Exercise the ram byte AKA scratchpad for comms validation
	ramData = 0xAA; SetRtcRegisters(PCF85263_CTL_RAM_BYTE, &ramData, sizeof(ramData));
	ramData = 0x00; GetRtcRegisters(PCF85263_CTL_RAM_BYTE, &ramData, sizeof(ramData));
	debug("External RTC: 1st RAM data (scratchpad) test %s\r\n", (ramData == 0xAA) ? "Passed" : "Failed");

	ramData = 0x55; SetRtcRegisters(PCF85263_CTL_RAM_BYTE, &ramData, sizeof(ramData));
	ramData = 0x00; GetRtcRegisters(PCF85263_CTL_RAM_BYTE, &ramData, sizeof(ramData));
	debug("External RTC: 2nd RAM data (scratchpad) test %s\r\n", (ramData == 0x55) ? "Passed" : "Failed");

	// Print time in succession with 1 second processor delays to show time increment
	testTime = GetExternalRtcTime(); debug("External RTC: Get time yields %02d:%02d:%02d, delaying 1 second for next check\r\n", testTime.hour, testTime.min, testTime.sec);
	MXC_Delay(MXC_DELAY_SEC(1)); //debug("MXC 1 second delay\r\n");
	testTime = GetExternalRtcTime(); debug("External RTC: Get time yields %02d:%02d:%02d, delaying 1 second for next check\r\n", testTime.hour, testTime.min, testTime.sec);
	MXC_Delay(MXC_DELAY_SEC(1)); //debug("MXC 1 second delay\r\n");
	testTime = GetExternalRtcTime(); debug("External RTC: Get time yields %02d:%02d:%02d\r\n", testTime.hour, testTime.min, testTime.sec);
#endif
	return (TRUE);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void StartExternalRtcClock(uint16 sampleRate)
{
	uint8_t clockOutControl;
	uint8_t pinIOControlReg;
	uint8_t controlReg;

	GetRtcRegisters(PCF85263_CTL_PIN_IO, &pinIOControlReg, 1);
	GetRtcRegisters(PCF85263_CTL_FUNCTION, &controlReg, 1);

	switch (sampleRate)
	{
		case 32768	: clockOutControl = PCF85263_CTL_FUNC_COF_32KHZ; break;
		case 16384	: clockOutControl = PCF85263_CTL_FUNC_COF_16KHZ; break;
		case 8192	: clockOutControl = PCF85263_CTL_FUNC_COF_8KHZ; break;
		case 4096	: clockOutControl = PCF85263_CTL_FUNC_COF_4KHZ; break;
		case 2048	: clockOutControl = PCF85263_CTL_FUNC_COF_2KHZ; break;
		case 1024	: clockOutControl = PCF85263_CTL_FUNC_COF_1KHZ; break;
		case 1		: clockOutControl = PCF85263_CTL_FUNC_COF_1HZ; break;
		case 0		: clockOutControl = PCF85263_CTL_FUNC_COF_LOW; break;
		default		: clockOutControl = PCF85263_CTL_FUNC_COF_1KHZ; break; // Set to 1024
	}

	// Set the clock output frequency
	controlReg &= (~PCF85263_CTL_FUNC_COF_MASK);
	controlReg |= clockOutControl;

	// Enable the clock pin (active low)
	pinIOControlReg &= (~PCF85263_CTL_CLKPM);

	debug("Starting External RTC Interrupt (SR: %d)\r\n", sampleRate);
#if 1 /* Test */
	debug("External RTC: Pin I/O is 0x%x, Function is 0x%x\r\n", pinIOControlReg, controlReg);
#endif
	SetRtcRegisters(PCF85263_CTL_FUNCTION, (uint8_t*)&controlReg, sizeof(controlReg));
	SetRtcRegisters(PCF85263_CTL_PIN_IO, (uint8_t*)&pinIOControlReg, sizeof(pinIOControlReg));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void StopExternalRtcClock(void)
{
	uint8_t pinIOControlReg;
	uint8_t controlReg;

	GetRtcRegisters(PCF85263_CTL_PIN_IO, &pinIOControlReg, 1);
	GetRtcRegisters(PCF85263_CTL_FUNCTION, &controlReg, 1);

	// Set the clock output frequency to low (off), mask not needed since all bits need to be set
	controlReg |= PCF85263_CTL_FUNC_COF_LOW;

	// Disable the clock pin (active low)
	pinIOControlReg |= PCF85263_CTL_CLKPM;

	SetRtcRegisters(PCF85263_CTL_FUNCTION, (uint8_t*)&controlReg, sizeof(controlReg));
	SetRtcRegisters(PCF85263_CTL_PIN_IO, (uint8_t*)&pinIOControlReg, sizeof(pinIOControlReg));

	debug("Stoping External RTC Interrupt\r\n");
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

		SetRtcRegisters(PCF85263_RTC_SECONDS, (uint8_t*)&rtcTime, sizeof(rtcTime));
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

			// Setup date registers, BCD format
			rtcDate.years = UINT8_CONVERT_TO_BCD(time->year, RTC_BCD_YEARS_MASK);
			rtcDate.months = UINT8_CONVERT_TO_BCD(time->month, RTC_BCD_MONTHS_MASK);
			rtcDate.days = UINT8_CONVERT_TO_BCD(time->day, RTC_BCD_DAYS_MASK);

			// Calculate the weekday based on the new date
			rtcDate.weekdays = GetDayOfWeek(time->year, time->month, time->day);

			//debug("Ext RTC: Apply Date: %x-%x-%x\r\n", rtcDate.months, rtcDate.days, rtcDate.years);

			SetRtcRegisters(PCF85263_RTC_DAYS, (uint8_t*)&rtcDate, sizeof(rtcDate));
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

	GetRtcRegisters(PCF85263_RTC_SECONDS, (uint8_t*)&translateTime, sizeof(translateTime));

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
	g_rtcTickCountSinceLastExternalUpdate = 0;
	g_lastReadExternalRtcTime = GetExternalRtcTime();

	return (PASSED);
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
void ClearAlarm1Flag(void)
{
	uint8 clearAlarmFlag = ~(PCF85263_CTL_FLAGS_A1F); // Logic 0 on the bit will clear Alarm flag, bit 4

	// Clear the Alarm flag
	SetRtcRegisters(PCF85263_CTL_FLAGS, (uint8_t*)&clearAlarmFlag, sizeof(clearAlarmFlag));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DisableExternalRtcAlarm(void)
{
	RTC_ALARM_STRUCT disableAlarm;

	disableAlarm.second_alarm = RTC_DISABLE_ALARM;
	disableAlarm.minute_alarm = RTC_DISABLE_ALARM;
	disableAlarm.hour_alarm = RTC_DISABLE_ALARM;
	disableAlarm.day_alarm = RTC_DISABLE_ALARM;
	disableAlarm.month_alarm = RTC_DISABLE_ALARM;
	
	// Turn off all the alarm enable flags
	SetRtcRegisters(PCF85263_RTC_SECOND_ALARM1, (uint8_t*)&disableAlarm, sizeof(disableAlarm));
	
	// Clear the Alarm flag
	ClearAlarm1Flag();
}

#if 1 /* Normal */
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void EnableExternalRtcAlarm(uint8 day, uint8 hour, uint8 minute, uint8 second)
{
	RTC_ALARM_STRUCT enableAlarm;

	enableAlarm.day_alarm = (UINT8_CONVERT_TO_BCD(day, RTC_BCD_DAYS_MASK) | RTC_ENABLE_ALARM);
	enableAlarm.hour_alarm = (UINT8_CONVERT_TO_BCD(hour, RTC_BCD_HOURS_MASK) | RTC_ENABLE_ALARM);
	enableAlarm.minute_alarm = (UINT8_CONVERT_TO_BCD(minute, RTC_BCD_MINUTES_MASK) | RTC_ENABLE_ALARM);
	enableAlarm.second_alarm = (UINT8_CONVERT_TO_BCD(second, RTC_BCD_SECONDS_MASK) | RTC_ENABLE_ALARM);
	enableAlarm.month_alarm = RTC_DISABLE_ALARM;
	
	SetRtcRegisters(PCF85263_RTC_SECOND_ALARM1, (uint8_t*)&enableAlarm, sizeof(enableAlarm));

	// Clear the Alarm flag
	ClearAlarm1Flag();
	
	debug("Enable RTC Alarm with Day: %d, Hour: %d, Minute: %d and Second: %d\r\n", day, hour, minute, second);
}
#else /* Test */
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void EnableExternalRtcAlarm(uint8 day, uint8 hour, uint8 minute, uint8 second)
{
	RTC_ALARM_STRUCT enableAlarm;

	enableAlarm.day_alarm = RTC_DISABLE_ALARM;
	enableAlarm.hour_alarm = RTC_DISABLE_ALARM;
	enableAlarm.minute_alarm = RTC_DISABLE_ALARM;
	enableAlarm.second_alarm = (UINT8_CONVERT_TO_BCD(second, RTC_BCD_SECONDS_MASK) | RTC_ENABLE_ALARM);
	enableAlarm.month_alarm = RTC_DISABLE_ALARM;
	
	SetRtcRegisters(PCF85263_RTC_SECOND_ALARM1, (uint8_t*)&enableAlarm, sizeof(enableAlarm));

	// Clear the Alarm flag
	ClearAlarm1Flag();
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GetRtcRegisters(uint8_t registerAddress, uint8_t* registerData, uint16_t dataLength)
{
    WriteI2CDevice(MXC_I2C1, I2C_ADDR_EXTERNAL_RTC, &registerAddress, sizeof(uint8_t), registerData, dataLength);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetRtcRegisters(uint8_t registerAddress, uint8_t* registerData, uint16_t dataLength)
{
	g_spareBuffer[0] = registerAddress;
	memcpy(&g_spareBuffer[1], registerData, dataLength);

    WriteI2CDevice(MXC_I2C1, I2C_ADDR_EXTERNAL_RTC, g_spareBuffer, (dataLength + 1), NULL, 0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestExternalRTC(void)
{
	DATE_TIME_STRUCT testTime;
	uint8_t ramData;
	uint8_t secondsReg;
	uint8_t stopEnableReg;
	uint8_t check = NO;

    debug("External RTC: Test device access...\r\n");

	// Expecting External RTC init to have been called prior

	// Get the Seconds register for the Oscillator stop bit
	GetRtcRegisters(PCF85263_RTC_SECONDS, (uint8_t*)&secondsReg, sizeof(secondsReg));
	// Get the Stop Enable register for the RTC clock stopped bit
	GetRtcRegisters(PCF85263_CTL_STOP_ENABLE, (uint8_t*)&stopEnableReg, sizeof(stopEnableReg));

	if (secondsReg & PCF85263_RTC_SECONDS_OS)
	{
		debugWarn("External RTC: Oscillator stopped, attempting enable...\r\n");
		secondsReg &= ~PCF85263_RTC_SECONDS_OS;
		SetRtcRegisters(PCF85263_RTC_SECONDS, (uint8_t*)&secondsReg, sizeof(secondsReg));
		check = YES;
	}

	if (stopEnableReg & PCF85263_CTL_STOP)
	{
		debugWarn("External RTC: Clock stopped, attempting enable...\r\n");
		stopEnableReg &= ~PCF85263_CTL_STOP;
		SetRtcRegisters(PCF85263_CTL_STOP_ENABLE, (uint8_t*)&stopEnableReg, sizeof(stopEnableReg));
		check = YES;
	}

	if (check == YES)
	{
		GetRtcRegisters(PCF85263_RTC_SECONDS, (uint8_t*)&secondsReg, sizeof(secondsReg));
		GetRtcRegisters(PCF85263_CTL_STOP_ENABLE, (uint8_t*)&stopEnableReg, sizeof(stopEnableReg));
	}

	if (((secondsReg & PCF85263_RTC_SECONDS_OS) == 0) && ((stopEnableReg & PCF85263_CTL_STOP) == 0))
	{
		debug("External RTC: Clock running and intergrity validated\r\n");
	}
	else
	{
		debugErr("External RTC: Unable to start Osc or Clock\r\n");
	}

	// Exercise the ram byte AKA scratchpad for comms validation
	ramData = 0xAA; SetRtcRegisters(PCF85263_CTL_RAM_BYTE, &ramData, sizeof(ramData));
	ramData = 0x00; GetRtcRegisters(PCF85263_CTL_RAM_BYTE, &ramData, sizeof(ramData));
	debug("External RTC: 1st RAM data (scratchpad) test %s\r\n", (ramData == 0xAA) ? "Passed" : "Failed");

	ramData = 0x55; SetRtcRegisters(PCF85263_CTL_RAM_BYTE, &ramData, sizeof(ramData));
	ramData = 0x00; GetRtcRegisters(PCF85263_CTL_RAM_BYTE, &ramData, sizeof(ramData));
	debug("External RTC: 2nd RAM data (scratchpad) test %s\r\n", (ramData == 0x55) ? "Passed" : "Failed");

	// Print time in succession with 1 second processor delays to show time increment
	testTime = GetExternalRtcTime(); debug("External RTC: Get time yields %02d:%02d:%02d\r\n", testTime.hour, testTime.min, testTime.sec);
	MXC_Delay(MXC_DELAY_SEC(1)); debug("MXC 1 second delay\r\n");
	testTime = GetExternalRtcTime(); debug("External RTC: Get time yields %02d:%02d:%02d\r\n", testTime.hour, testTime.min, testTime.sec);
	MXC_Delay(MXC_DELAY_SEC(1)); debug("MXC 1 second delay\r\n");
	testTime = GetExternalRtcTime(); debug("External RTC: Get time yields %02d:%02d:%02d\r\n", testTime.hour, testTime.min, testTime.sec);
}
