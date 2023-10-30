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

#define BATT_CHARGER_DEVICE_ADDRESS_SETTING					0X05	
#define BATT_CHARGER_INPUT_MINIMUM_VOLTAGE_LIMIT_SETTING	0X06	
#define BATT_CHARGER_INPUT_CURRENT_LIMIT_SETTING			0X08	
#define BATT_CHARGER_OUTPUT_VOLTAGE_SETTING_IN_SOURCE_MODE	0X09	
#define BATT_CHARGER_BATTERY_IMPEDANCE_COMPENSATION_AND_OUTPUT_CURRENT_LIMIT_SETTING_IN_SOURCE_MODE			0X0A	
#define BATT_CHARGER_BATTERY_LOW_VOLTAGE_THRESHOLD_AND_BATTERY_DISCHARGE_CURRENT_REGULATION_IN_SOURCE_MODE	0X0B	
#define BATT_CHARGER_JEITA_ACTION_SETTING					0X0C	
#define BATT_CHARGER_TEMPERATURE_PROTECTION_SETTING			0X0D	
#define BATT_CHARGER_CONFIGURATION_REGISTER_0				0X0E	
#define BATT_CHARGER_CONFIGURATION_REGISTER_1				0X0F	
#define BATT_CHARGER_CONFIGURATION_REGISTER_2				0X10	
#define BATT_CHARGER_CONFIGURATION_REGISTER_3				0X11	
#define BATT_CHARGER_CONFIGURATION_REGISTER_4				0X12	
#define BATT_CHARGER_CHARGE_CURRENT_SETTING					0X14	
#define BATT_CHARGER_BATTERY_REGULATION_VOLTAGE_SETTING		0X15	
#define BATT_CHARGER_STATUS_AND_FAULT_REGISTER_0			0X16	
#define BATT_CHARGER_STATUS_AND_FAULT_REGISTER_1			0X17	
#define BATT_CHARGER_INT_MASK_SETTING_REGISTER_0			0X18	
#define BATT_CHARGER_INT_MASK_SETTING_REGISTER_1			0X19	
#define BATT_CHARGER_INTERNAL_DAC_OUTPUT_OF_THE_INPUT_CURRENT_LIMIT_SETTING	0X22	
#define BATT_CHARGER_ADC_RESULT_OF_THE_INPUT_VOLTAGE		0X23	
#define BATT_CHARGER_ADC_RESULT_OF_THE_INPUT_CURRENT		0X24	
#define BATT_CHARGER_ADC_RESULT_OF_THE_BATTERY_VOLTAGE		0X25	
#define BATT_CHARGER_ADC_RESULT_OF_THE_BATTERY_CURRENT		0X27	
#define BATT_CHARGER_ADC_RESULT_OF_THE_NTC_VOLTAGE_RATIO	0X28	
#define BATT_CHARGER_ADC_RESULT_OF_THE_TS_VOLTAGE_RATIO		0X29	
#define BATT_CHARGER_ADC_RESULT_OF_THE_JUNCTION_TEMPERATURE	0X2A	
#define BATT_CHARGER_ADC_RESULT_OF_THE_BATTERY_DISCHARGE_CURRENT		0X2B	
#define BATT_CHARGER_ADC_RESULT_OF_THE_INPUT_VOLTAGE_IN_DISCHARGE_MODE	0X2C	
#define BATT_CHARGER_ADC_RESULT_OF_THE_OUTPUT_CURRENT_IN_DISCHARGE_MODE	0X2D	

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

void GetBattChargerRegister(uint8_t registerAddress, uint16_t* registerContents);
void SetBattChargerRegister(uint8_t registerAddress, uint16_t registerContents);
void InitBattChargerRegister(uint8_t registerAddress, uint16_t registerContents);
uint16_t GetBattChargerInputVoltage(void);
uint16_t GetBattChargerInputCurrent(void);
uint16_t GetBattChargerBatteryVoltagePerCell(void);
uint16_t GetBattChargerBatteryChargeCurrent(void);
uint16_t GetBattChargerNTCSenseRatio(void);
uint16_t GetBattChargerTSSenseRatio(void);
uint16_t GetBattChargerJunctionTemperature(void);
uint16_t GetBattChargerBatteryDischargeCurrent(void);
uint16_t GetBattChargerInputVoltageInDischargeMode(void);
uint16_t GetBattChargerOutputCurrentInDischargeMode(void);

#endif //_POWER_MANAGEMENT_H_
