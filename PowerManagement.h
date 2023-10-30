///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

#ifndef _POWER_MANAGEMENT_H_
#define _POWER_MANAGEMENT_H_

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include "Typedefs.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#if 0 /* old hw */
#define ALARM_1_GPIO_PIN	AVR32_PIN_PB06
#define ALARM_2_GPIO_PIN	AVR32_PIN_PB07
#else
#define ALARM_1_GPIO_PIN	1
#define ALARM_2_GPIO_PIN	2
#endif

typedef enum
{
	ALARM_2_ENABLE = 0,
	ALARM_1_ENABLE,
	SERIAL_485_RECEIVER_ENABLE,
	SERIAL_485_DRIVER_ENABLE,
	SERIAL_232_RECEIVER_ENABLE,
	SERIAL_232_DRIVER_ENABLE,
	POWER_OFF_PROTECTION_ENABLE,
	POWER_OFF,
	LCD_BACKLIGHT_HI_ENABLE,
	LCD_BACKLIGHT_ENABLE,
	LCD_CONTRAST_ENABLE,
	LCD_POWER_ENABLE,
	ANALOG_SLEEP_ENABLE,
	LAN_SLEEP_ENABLE,
	USB_DEVICE_SLEEP_ENABLE,
	USB_HOST_SLEEP_ENABLE,
	RTC_TIMESTAMP,
	TRIGGER_OUT,
	SEISMIC_SENSOR_DATA_CONTROL,
	ACOUSTIC_SENSOR_DATA_CONTROL,
	USB_LED,
	SD_POWER
} POWER_MGMT_OPTIONS;

#define ALARM_2_ENABLE_BIT					(1 << ALARM_2_ENABLE)
#define ALARM_1_ENABLE_BIT					(1 << ALARM_1_ENABLE)
#define SERIAL_485_RECEIVER_ENABLE_BIT		(1 << SERIAL_485_RECEIVER_ENABLE)
#define SERIAL_485_DRIVER_ENABLE_BIT		(1 << SERIAL_485_DRIVER_ENABLE)
#define SERIAL_232_RECEIVER_ENABLE_BIT		(1 << SERIAL_232_RECEIVER_ENABLE)
#define SERIAL_232_DRIVER_ENABLE_BIT		(1 << SERIAL_232_DRIVER_ENABLE)
#define POWER_OFF_PROTECTION_ENABLE_BIT		(1 << POWER_OFF_PROTECTION_ENABLE)
#define POWER_OFF_BIT						(1 << POWER_OFF)
#define LCD_BACKLIGHT_HI_ENABLE_BIT			(1 << LCD_BACKLIGHT_HI_ENABLE)
#define LCD_BACKLIGHT_ENABLE_BIT			(1 << LCD_BACKLIGHT_ENABLE)
#define LCD_CONTRAST_ENABLE_BIT				(1 << LCD_CONTRAST_ENABLE)
#define LCD_POWER_ENABLE_BIT				(1 << LCD_POWER_ENABLE)
#define ANALOG_SLEEP_ENABLE_BIT				(1 << ANALOG_SLEEP_ENABLE)
#define LAN_SLEEP_ENABLE_BIT				(1 << LAN_SLEEP_ENABLE)
#define USB_DEVICE_SLEEP_ENABLE_BIT			(1 << USB_DEVICE_SLEEP_ENABLE)
#define USB_HOST_SLEEP_ENABLE_BIT			(1 << USB_HOST_SLEEP_ENABLE)
#define RTC_TIMESTAMP_BIT					(1 << RTC_TIMESTAMP)
#define TRIGGER_OUT_BIT						(1 << TRIGGER_OUT)
#define SEISMIC_SENSOR_DATA_CONTROL_BIT		(1 << SEISMIC_SENSOR_DATA_CONTROL)
#define ACOUSTIC_SENSOR_DATA_CONTROL_BIT	(1 << ACOUSTIC_SENSOR_DATA_CONTROL)
#define USB_LED_BIT							(1 << USB_LED)
#define SD_POWER_BIT						(1 << SD_POWER)

enum {
	SHUTDOWN_UNIT = 1,
	RESET_UNIT
};

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------
void PowerControl(POWER_MGMT_OPTIONS option, BOOLEAN mode);
BOOLEAN GetPowerControlState(POWER_MGMT_OPTIONS option);
void setMcorePwMgntDefaults(void);
void PowerUnitOff(uint8 powerOffMode);

#endif //_POWER_MANAGEMENT_H_
