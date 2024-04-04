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
#define ALARM_1_GPIO_PIN	1
#define ALARM_2_GPIO_PIN	2

typedef enum
{
	ALARM_2_ENABLE = 0,
	ALARM_1_ENABLE,
	LCD_POWER_ENABLE,
	ANALOG_5V_ENABLE,
	TRIGGER_OUT,
	MCU_POWER_LATCH,
	ENABLE_12V,
	USB_SOURCE_ENABLE,
	USB_AUX_POWER_ENABLE,
	ADC_RESET,
	EXPANSION_ENABLE,
	SENSOR_CHECK_ENABLE,
	LTE_RESET,
	BLE_RESET,
	CELL_ENABLE,
	EXPANSION_RESET,
	LCD_POWER_DOWN,
	LED_1,
	LED_2,
	LED_3,
	LED_4
} POWER_MGMT_OPTIONS;

#define ALARM_2_ENABLE_BIT					(1 << ALARM_2_ENABLE)
#define ALARM_1_ENABLE_BIT					(1 << ALARM_1_ENABLE)
#define LCD_POWER_ENABLE_BIT				(1 << LCD_POWER_ENABLE)
#define ANALOG_5V_ENABLE_BIT				(1 << ANALOG_5V_ENABLE)
#define TRIGGER_OUT_BIT						(1 << TRIGGER_OUT)
#define MCU_POWER_LATCH_BIT					(1 << MCU_POWER_LATCH)
#define ENABLE_12V_BIT						(1 << ENABLE_12V)
#define USB_SOURCE_ENABLE_BIT				(1 << USB_SOURCE_ENABLE)
#define USB_AUX_POWER_ENABLE_BIT			(1 << USB_AUX_POWER_ENABLE)
#define ADC_RESET_BIT						(1 << ADC_RESET)
#define EXPANSION_ENABLE_BIT				(1 << EXPANSION_ENABLE)
#define SENSOR_CHECK_ENABLE_BIT				(1 << SENSOR_CHECK_ENABLE)
#define LTE_RESET_BIT						(1 << LTE_RESET)
#define BLE_RESET_BIT						(1 << BLE_RESET)
#define CELL_ENABLE_BIT						(1 << CELL_ENABLE)
#define EXPANSION_RESET_BIT					(1 << EXPANSION_RESET)
#define LCD_POWER_DOWN_BIT					(1 << LCD_POWER_DOWN)
#define LED_1_BIT							(1 << LED_1)
#define LED_2_BIT							(1 << LED_2)
#define LED_3_BIT							(1 << LED_3)
#define LED_4_BIT							(1 << LED_4) // 25th bit

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
BOOLEAN GetShadowPowerControlState(POWER_MGMT_OPTIONS option);
uint8_t GetCurrentLedStates(void);
void PowerUnitOff(uint8 powerOffMode);

void Analog5vPowerGpioSetup(uint8_t mode);
void LcdPowerGpioSetup(uint8_t mode);
void ExpansionPowerGpioSetup(uint8_t mode);
void CellPowerGpioSetup(uint8_t mode);

void GetBattChargerRegister(uint8_t registerAddress, uint16_t* registerContents);
void SetBattChargerRegister(uint8_t registerAddress, uint16_t registerContents);
void InitBattChargerRegister(void);
uint16_t GetBattChargerStatusReg0(void);
uint16_t GetBattChargerStatusReg1(void);
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

int Ltc2944_get_voltage(int* val);
void Ltc2944_i2c_shutdown(void);

void FuelGaugeDebugInfo(void);
char* FuelGaugeDebugString(void);
int FuelGaugeGetCurrent(void);

#endif //_POWER_MANAGEMENT_H_
