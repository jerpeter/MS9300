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
#include <string.h>
#include <stdlib.h>
#include "Typedefs.h"
#include "Common.h"
#include "Analog.h"
#include "Summary.h"
#include "Record.h"
#include "InitDataBuffers.h"
#include "PowerManagement.h"
#include "OldUart.h"
#include "Menu.h"
#include "rtc.h"
//#include "tc.h"
//#include "twi.h"
#include "spi.h"
#include "tmr.h"
#include "mxc_delay.h"
#include "gpio.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
typedef struct {
	uint32 rTotal;
	uint32 vTotal;
	uint32 tTotal;
	uint32 aTotal;
} CHANNEL_OFFSET_TOTALS;

typedef struct {
	uint32 rCount;
	uint32 vCount;
	uint32 tCount;
	uint32 aCount;
} CHANNEL_OFFSET_COUNTS;

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
#include "Globals.h"

///----------------------------------------------------------------------------
///	Local Scope Globals
///----------------------------------------------------------------------------
static uint16* s_zeroSensorPretriggerComparePtr;
static SAMPLE_DATA_STRUCT s_tempData;
static CHANNEL_OFFSET_TOTALS s_workingChannelOffset;
static CHANNEL_OFFSET_COUNTS s_workingChannelCounts;
static CHANNEL_OFFSET_TOTALS s_compareChannelOffset;

///----------------------------------------------------------------------------
///	Analog information
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* Unused at this time */
uint8_t GetAnalogConfigReadback(void)
{
	return(AD4695_GetStandardSequenceActiveChannels());
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16_t dataTemperature;
uint8_t chanActive[8];
void ReadAnalogData(SAMPLE_DATA_STRUCT* dataPtr)
{
	uint8_t chanDataRaw[3];
	uint8_t configError = NO;

	// Todo: need variable channel config for selectable dynamic channels

	if (g_adChannelConfig == FOUR_AD_CHANNELS_WITH_READBACK_WITH_TEMP)
	{
		// Conversion time max is 415ns (~50 clock cycles), normal SPI setup processing should take longer than that without requiring waiting on the ADC busy state (Port 0, Pin 17)

		// Chan 0 - Geo1/R
		if (chanActive[0]) { SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING); SetAdcConversionState(OFF);
			dataPtr->r = ((chanDataRaw[0] << 8) | chanDataRaw[1]); if ((chanDataRaw[2] & 0x0F) != 0) { configError = YES; debugErr("AD Channel config error: Expected %d, got %d\r\n", 0, (chanDataRaw[2] & 0x0F)); }
			if (chanDataRaw[2] & 0x10) { debugErr("AD Over Voltage Alert: Channel %d\r\n", (chanDataRaw[2] & 0x0F)); } }

		// Chan 1 - Geo1/T
		if (chanActive[1]) { SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING); SetAdcConversionState(OFF);
			dataPtr->t = ((chanDataRaw[0] << 8) | chanDataRaw[1]); if ((chanDataRaw[2] & 0x0F) != 1) { configError = YES; debugErr("AD Channel config error: Expected %d, got %d\r\n", 1, (chanDataRaw[2] & 0x0F)); }
			if (chanDataRaw[2] & 0x10) { debugErr("AD Over Voltage Alert: Channel %d\r\n", (chanDataRaw[2] & 0x0F)); } }

		// Chan 2 - Geo1/V
		if (chanActive[2]){ SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING); SetAdcConversionState(OFF);
			dataPtr->v = ((chanDataRaw[0] << 8) | chanDataRaw[1]); if ((chanDataRaw[2] & 0x0F) != 2) { configError = YES; debugErr("AD Channel config error: Expected %d, got %d\r\n", 2, (chanDataRaw[2] & 0x0F)); }
			if (chanDataRaw[2] & 0x10) { debugErr("AD Over Voltage Alert: Channel %d\r\n", (chanDataRaw[2] & 0x0F)); } }

		// Chan 3 - AOP1/A
		if (chanActive[3]) { SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING); SetAdcConversionState(OFF);
			dataPtr->a = ((chanDataRaw[0] << 8) | chanDataRaw[1]); if ((chanDataRaw[2] & 0x0F) != 3) { configError = YES; debugErr("AD Channel config error: Expected %d, got %d\r\n", 3, (chanDataRaw[2] & 0x0F)); }
			if (chanDataRaw[2] & 0x10) { debugErr("AD Over Voltage Alert: Channel %d\r\n", (chanDataRaw[2] & 0x0F)); } }

		// Chan 4 - Geo2/R
		if (chanActive[4]) { SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING); SetAdcConversionState(OFF);
			dataPtr->r = ((chanDataRaw[0] << 8) | chanDataRaw[1]); if ((chanDataRaw[2] & 0x0F) != 4) { configError = YES; debugErr("AD Channel config error: Expected %d, got %d\r\n", 4, (chanDataRaw[2] & 0x0F)); }
			if (chanDataRaw[2] & 0x10) { debugErr("AD Over Voltage Alert: Channel %d\r\n", (chanDataRaw[2] & 0x0F)); } }

		// Chan 5 - Geo2/T
		if (chanActive[5]) { SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING); SetAdcConversionState(OFF);
			dataPtr->t = ((chanDataRaw[0] << 8) | chanDataRaw[1]); if ((chanDataRaw[2] & 0x0F) != 5) { configError = YES; debugErr("AD Channel config error: Expected %d, got %d\r\n", 5, (chanDataRaw[2] & 0x0F)); }
			if (chanDataRaw[2] & 0x10) { debugErr("AD Over Voltage Alert: Channel %d\r\n", (chanDataRaw[2] & 0x0F)); } }

		// Chan 6 - Geo2/V
		if (chanActive[6]) { SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING); SetAdcConversionState(OFF);
			dataPtr->v = ((chanDataRaw[0] << 8) | chanDataRaw[1]); if ((chanDataRaw[2] & 0x0F) != 6) { configError = YES; debugErr("AD Channel config error: Expected %d, got %d\r\n", 6, (chanDataRaw[2] & 0x0F)); }
			if (chanDataRaw[2] & 0x10) { debugErr("AD Over Voltage Alert: Channel %d\r\n", (chanDataRaw[2] & 0x0F)); } }

		// Chan 7 - AOP2/A
		if (chanActive[7]) { SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING); SetAdcConversionState(OFF);
			dataPtr->a = ((chanDataRaw[0] << 8) | chanDataRaw[1]); if ((chanDataRaw[2] & 0x0F) != 7) { configError = YES; debugErr("AD Channel config error: Expected %d, got %d\r\n", 7, (chanDataRaw[2] & 0x0F)); }
			if (chanDataRaw[2] & 0x10) { debugErr("AD Over Voltage Alert: Channel %d\r\n", (chanDataRaw[2] & 0x0F)); } }

		// Temp
		SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS, BLOCKING); SetAdcConversionState(OFF);
#if 1 /* Test */
		dataTemperature = ((chanDataRaw[0] << 8) | chanDataRaw[1]);
#endif
		if ((chanDataRaw[2] & 0x0F) != 15) { configError = YES; debugErr("AD Channel config error: Expected %d, got %d\r\n", 15, (chanDataRaw[2] & 0x0F)); } // An INx value of 15 corresponds to either IN15 or the temperature sensor

		if (configError == YES)
		{
			debugErr("AD Channel config error! Channel data is not in sync\r\n");
		}
	}
	else if (g_adChannelConfig == FOUR_AD_CHANNELS_NO_READBACK_WITH_TEMP)
	{
		// Conversion time max is 415ns (~50 clock cycles), normal SPI setup processing should take longer than that without requiring waiting on the ADC busy state (Port 0, Pin 17)

		// Chan 0
		SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE, BLOCKING); SetAdcConversionState(OFF);
		dataPtr->r = ((chanDataRaw[0] << 8) | chanDataRaw[1]);

		// Chan 1
		SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE, BLOCKING); SetAdcConversionState(OFF);
		dataPtr->t = ((chanDataRaw[0] << 8) | chanDataRaw[1]);

		// Chan 2
		SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE, BLOCKING); SetAdcConversionState(OFF);
		dataPtr->v = ((chanDataRaw[0] << 8) | chanDataRaw[1]);

		// Chan 3
		SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE, BLOCKING); SetAdcConversionState(OFF);
		dataPtr->a = ((chanDataRaw[0] << 8) | chanDataRaw[1]);

		// Temp
		SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE, BLOCKING); SetAdcConversionState(OFF);
#if 1 /* Test */
		dataTemperature = ((chanDataRaw[0] << 8) | chanDataRaw[1]);
#endif
	}
	else // FOUR_AD_CHANNELS_NO_READBACK_NO_TEMP
	{
		// Chan 0
		SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE, BLOCKING); SetAdcConversionState(OFF);
		dataPtr->r = ((chanDataRaw[0] << 8) | chanDataRaw[1]);

		// Chan 1
		SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE, BLOCKING); SetAdcConversionState(OFF);
		dataPtr->t = ((chanDataRaw[0] << 8) | chanDataRaw[1]);

		// Chan 2
		SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE, BLOCKING); SetAdcConversionState(OFF);
		dataPtr->v = ((chanDataRaw[0] << 8) | chanDataRaw[1]);

		// Chan 3
		SetAdcConversionState(ON); SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, NULL, 0, chanDataRaw, AD4695_CHANNEL_DATA_READ_SIZE, BLOCKING); SetAdcConversionState(OFF);
		dataPtr->a = ((chanDataRaw[0] << 8) | chanDataRaw[1]);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AnalogControlInit(void)
{
	SetAnalogCutoffFrequency(ANALOG_CUTOFF_FREQ_500);
	debug("Analog Control: Cutoff Freq set @ 500 Hz\r\n");
	SetSeismicGainSelect(SEISMIC_GAIN_NORMAL);
	debug("Analog Control: Seismic gain set as Normal\r\n");
	SetAcousticPathSelect(ACOUSTIC_PATH_AOP);
	debug("Analog Control: Acoustic path set as AOP\r\n");
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupADChannelConfig(uint32 sampleRate, uint8 channelVerification)
{
	// Todo: make channel config dynamic
	
	// Enabled the specific sensor blocks
	if (g_currentSensorGroup == SENSOR_GROUP_A_1)
	{
#if 0 /* Original, Geo + AOP on one sensor group */
		MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_GEO1_PORT, GPIO_SENSOR_ENABLE_GEO1_PIN);
		MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_AOP1_PORT, GPIO_SENSOR_ENABLE_AOP1_PIN);
#else /* Standard config, Geo + AOP split on two sensor groups */
		MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_GEO1_PORT, GPIO_SENSOR_ENABLE_GEO1_PIN);
		MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_AOP2_PORT, GPIO_SENSOR_ENABLE_AOP2_PIN);
#endif
	}
	else if (g_currentSensorGroup == SENSOR_GROUP_B_2)
	{
#if 0 /* Original, Geo + AOP on one sensor group */
		MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_GEO2_PORT, GPIO_SENSOR_ENABLE_GEO2_PIN);
		MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_AOP2_PORT, GPIO_SENSOR_ENABLE_AOP2_PIN);
#else /* Standard config, Geo + AOP split on two sensor groups */
		MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_GEO2_PORT, GPIO_SENSOR_ENABLE_GEO2_PIN);
		MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_AOP1_PORT, GPIO_SENSOR_ENABLE_AOP1_PIN);
#endif
	}
	else // (g_currentSensorGroup == SENSOR_GROUP_BOTH)
	{
		MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_GEO1_PORT, GPIO_SENSOR_ENABLE_GEO1_PIN);
		MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_AOP1_PORT, GPIO_SENSOR_ENABLE_AOP1_PIN);
		MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_GEO2_PORT, GPIO_SENSOR_ENABLE_GEO2_PIN);
		MXC_GPIO_OutSet(GPIO_SENSOR_ENABLE_AOP2_PORT, GPIO_SENSOR_ENABLE_AOP2_PIN);
	}

	// Setup the stantard sequence channels to be monitored
	if (g_currentSensorGroup == SENSOR_GROUP_A_1)
	{
#if 0 /* Original, Geo + AOP on one sensor group */
		AD4695_SetStandardSequenceActiveChannels((ANALOG_GEO_1 | ANALOG_AOP_1));
#else /* Standard config, Geo + AOP split on two sensor groups */
		AD4695_SetStandardSequenceActiveChannels((ANALOG_GEO_1 | ANALOG_AOP_2));
#endif
	}
	else // SENSOR_GROUP_B_2
	{
#if 0 /* Original, Geo + AOP on one sensor group */
		AD4695_SetStandardSequenceActiveChannels((ANALOG_GEO_2 | ANALOG_AOP_2));
#else /* Standard config, Geo + AOP split on two sensor groups */
		AD4695_SetStandardSequenceActiveChannels((ANALOG_GEO_2 | ANALOG_AOP_1));
#endif
	}

	// For any sample rate 8K and below
	if (sampleRate <= SAMPLE_RATE_8K)
	{
		// Check if channel verification is not disabled or verification override is enabled to allow reading back the config
		if ((g_unitConfig.adChannelVerification != DISABLED) || (channelVerification == OVERRIDE_ENABLE_CHANNEL_VERIFICATION))
		{
			debug("ADC Channel Setup: 4 channels + Temperature + Readback\r\n");
			g_adChannelConfig = FOUR_AD_CHANNELS_WITH_READBACK_WITH_TEMP;
		}
		else // Verification disabled, don't read back config
		{
			debug("ADC Channel Setup: 4 channels + Temperature (No readback)\r\n");
			g_adChannelConfig = FOUR_AD_CHANNELS_NO_READBACK_WITH_TEMP;
		}
	}
	else // Sample rates 16384 and above take too long to read back config and temp, so skip them
	{
			debug("ADC Channel Setup: 4 channels only (No Temp, No readback)\r\n");
		g_adChannelConfig = FOUR_AD_CHANNELS_NO_READBACK_NO_TEMP;
	}

	// Enable temp sensor if channel config is anything but no temperature reading, otherwise disable
	AD4695_SetTemperatureSensorEnable(((g_adChannelConfig != FOUR_AD_CHANNELS_NO_READBACK_NO_TEMP) ? YES : NO));
	// Start conversion mode and enable status if readback is enabled, otherwise disable status
	AD4695_EnterConversionMode(((g_adChannelConfig == FOUR_AD_CHANNELS_WITH_READBACK_WITH_TEMP) ? YES : NO));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupAccChannelConfig(uint32 sampleRate)
{
	g_adChannelConfig = THREE_ACC_CHANNELS_NO_AIR;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DisableSensorBlocks(void)
{
#if 0 /* Normal */
	MXC_GPIO_OutClr(GPIO_SENSOR_ENABLE_GEO1_PORT, GPIO_SENSOR_ENABLE_GEO1_PIN);
	MXC_GPIO_OutClr(GPIO_SENSOR_ENABLE_AOP1_PORT, GPIO_SENSOR_ENABLE_AOP1_PIN);
	MXC_GPIO_OutClr(GPIO_SENSOR_ENABLE_GEO2_PORT, GPIO_SENSOR_ENABLE_GEO2_PIN);
	MXC_GPIO_OutClr(GPIO_SENSOR_ENABLE_AOP2_PORT, GPIO_SENSOR_ENABLE_AOP2_PIN);
#else /* Test leaving the sensor channels enabled */
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AdSetCalSignalLow(void)
{
	// Enable driving of the sensor check signal if not enabled
	if (GetPowerControlState(SENSOR_CHECK_ENABLE) == OFF) {	PowerControl(SENSOR_CHECK_ENABLE, ON); }

	// Set the sensor check drive signal low
	SetSensorCheckState(LOW);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AdSetCalSignalHigh(void)
{
	// Enable driving of the sensor check signal if not enabled
	if (GetPowerControlState(SENSOR_CHECK_ENABLE) == OFF) {	PowerControl(SENSOR_CHECK_ENABLE, ON); }

	// Set the sensor check drive signal high
	SetSensorCheckState(HIGH);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AdSetCalSignalOff(void)
{
	// Disable driving of the sensor check signal if not enabled
	PowerControl(SENSOR_CHECK_ENABLE, OFF);

	// Leave sensor state in the high position since this will be the desired state next sensor check
	SetSensorCheckState(HIGH);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetAnalogCutoffFrequency(uint8 freq)
{
	switch (freq)
	{
		case ANALOG_CUTOFF_FREQ_500: /* A1:X A0:X EN:0 */ SetNyquist1State(OFF); SetNyquist0State(OFF); SetNyquist2EnableState(OFF); break; // ~500 Hz
		case ANALOG_CUTOFF_FREQ_1K: /* A1:0 A0:0 EN:1 */ SetNyquist1State(OFF); SetNyquist0State(OFF); SetNyquist2EnableState(ON); break; // ~1 kHz
		case ANALOG_CUTOFF_FREQ_2K: /* A1:0 A0:1 EN:1 */ SetNyquist1State(OFF); SetNyquist0State(ON); SetNyquist2EnableState(ON); break; // ~2 kHz
		case ANALOG_CUTOFF_FREQ_4K: /* A1:1 A0:0 EN:1 */ SetNyquist1State(ON); SetNyquist0State(OFF); SetNyquist2EnableState(ON); break; // ~4 kHz
		case ANALOG_CUTOFF_FREQ_8K: /* A1:1 A0:1 EN:1 */ SetNyquist1State(ON); SetNyquist0State(ON); SetNyquist2EnableState(ON); break; // ~8 kHz
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetSeismicGainSelect(uint8 seismicGain)
{
	if (seismicGain == SEISMIC_GAIN_NORMAL)
	{
		// Logic 1 on Geo gain select line achieves normal gain
		SetGainGeo1State(HIGH);
		SetGainGeo2State(HIGH);
	}
	else // seismicGain == SEISMIC_GAIN_HIGH
	{
		// Logic 0 on Geo gain select line achieves high gain
		SetGainGeo1State(LOW);
		SetGainGeo2State(LOW);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetSeismicGainSelectSensor(uint8 sensorSelection, uint8 seismicGain)
{
	if (sensorSelection & ANALOG_SENSOR_GEO_1)
	{
		// Logic 1 on Geo gain select line achieves normal gain
		if (seismicGain == SEISMIC_GAIN_NORMAL) { SetGainGeo1State(HIGH); }
		else /* SEISMIC_GAIN_HIGH */ SetGainGeo1State(LOW);
	}

	if (sensorSelection & ANALOG_SENSOR_GEO_2)
	{
		// Logic 1 on Geo gain select line achieves normal gain
		if (seismicGain == SEISMIC_GAIN_NORMAL) { SetGainGeo2State(HIGH); }
		else /* SEISMIC_GAIN_HIGH */ SetGainGeo2State(LOW);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetAcousticPathSelect(uint8 acousticGain)
{
	if (acousticGain == ACOUSTIC_PATH_AOP)
	{
		// Logic 1 on AOP path select line achieves AOP
		SetPathSelectAop1State(HIGH);
		SetPathSelectAop2State(HIGH);
	}
	else // seismicGain == ACOUSTIC_PATH_A_WEIGHTED
	{
		// Logic 1 on AOP path select line achieves A-weighting
		SetPathSelectAop1State(LOW);
		SetPathSelectAop2State(LOW);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetAcousticPathSelectSensor(uint8 sensorSelection, uint8 acousticGain)
{
	if (sensorSelection & ANALOG_SENSOR_AOP_1)
	{
		// Logic 1 on AOP path select line achieves AOP
		if (acousticGain == ACOUSTIC_PATH_AOP) { SetPathSelectAop1State(HIGH); }
		else /* ACOUSTIC_PATH_A_WEIGHTED */ SetPathSelectAop1State(LOW);
	}

	if (sensorSelection & ANALOG_SENSOR_AOP_2)
	{
		// Logic 1 on AOP path select line achieves AOP
		if (acousticGain == ACOUSTIC_PATH_AOP) { SetPathSelectAop2State(HIGH); }
		else /* ACOUSTIC_PATH_A_WEIGHTED */ SetPathSelectAop2State(LOW);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetCalSignalEnable(uint8 enable)
{
	if (enable == ON) { PowerControl(SENSOR_CHECK_ENABLE, ON); }
	else /* (enable == OFF) */ { PowerControl(SENSOR_CHECK_ENABLE, OFF); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetCalSignal(uint8 state)
{
	if (state) { SetSensorCheckState(ON); }
	else /* (state == OFF) */ { SetSensorCheckState(OFF); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* Old timings */
void GenerateCalSignal(void)
{
	// Calibration signal timing
	// 1) Enable cal (cut off real channels) and delay 5ms
	// 2) Drive reference high for 10ms
	// 3) Drive reference low for 20ms
	// 4) Drive reference high for 10ms
	// 5) Disable cal and delay for 55ms and then off

	SetCalSignalEnable(ON);
	SoftUsecWait(5 * SOFT_MSECS);
	SetCalSignal(ON);
	SoftUsecWait(10 * SOFT_MSECS);
	SetCalSignal(OFF);
	SoftUsecWait(20 * SOFT_MSECS);
	SetCalSignal(ON);
	SoftUsecWait(10 * SOFT_MSECS);
	SetCalSignalEnable(OFF);
	SoftUsecWait(55 * SOFT_MSECS);
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitAndSeedChannelOffsetVariables(void)
{
	// Seed totals to ensure count is non-zero (count initialized to 1 for this reason)
	s_workingChannelOffset.rTotal = ACCURACY_16_BIT_MIDPOINT - g_channelOffset.r_offset;
	s_workingChannelOffset.vTotal = ACCURACY_16_BIT_MIDPOINT - g_channelOffset.v_offset;
	s_workingChannelOffset.tTotal = ACCURACY_16_BIT_MIDPOINT - g_channelOffset.t_offset;
	s_workingChannelOffset.aTotal = ACCURACY_16_BIT_MIDPOINT - g_channelOffset.a_offset;

	// Seed the counts
	s_workingChannelCounts.rCount = 1;
	s_workingChannelCounts.vCount = 1;
	s_workingChannelCounts.tCount = 1;
	s_workingChannelCounts.aCount = 1;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GetChannelOffsets(uint32 sampleRate)
{
	uint32 i = 0;
	uint32 timeDelay = (977 / (sampleRate / 1024));
	uint8 powerAnalogDown = NO;

#if 1 /* Test Accelerometer */
	if (g_adChannelConfig == THREE_ACC_CHANNELS_NO_AIR)
	{
		debug("Get Chan Offsets: Using Accelerometer\r\n");
	}
#endif

	// Check to see if the A/D is in sleep mode
	if (GetPowerControlState(ADC_RESET) == ON)
	{
		WaitAnalogPower5vGood();
		AD4695_Init();

		// Set flag to signal powering off the A/D when finished
		powerAnalogDown = YES;
	}

	// Reset offset values
	memset(&g_channelOffset, 0, sizeof(g_channelOffset));

	debug("Get Channel Offset: Read and pitch... (Address boundary: %s)\r\n", ((uint32)(&s_tempData) % 4 == 0) ? "YES" : "NO");
	// Read and pitch samples
	for (i = 0; i < (sampleRate * 1); i++)
	{
#if 1 /* Test Accelerometer */
		if (g_adChannelConfig == THREE_ACC_CHANNELS_NO_AIR)
		{
			ACC_DATA_STRUCT accData;
			GetAccelerometerChannelData(&accData);
			s_tempData.a = 0x8000; s_tempData.r = accData.x; s_tempData.t = accData.y; s_tempData.v = accData.z;
		}
		else { ReadAnalogData(&s_tempData); }
#else
		ReadAnalogData(&s_tempData);
#endif

		//debug("Offset throw away data: 0x%x, 0x%x, 0x%x, 0x%x\r\n", s_tempData.r, s_tempData.v, s_tempData.t, s_tempData.a);

		// Delay equivalent to the time in between gathering samples for the current sample rate
		SoftUsecWait(timeDelay);
	}

	// Initialize
	memset(&s_workingChannelOffset, 0, sizeof(s_workingChannelOffset));

	debug("Get Channel Offset: 1st Pass Read and sum...\r\n");
	// Read and sum samples
	for (i = 0; i < (sampleRate * 1); i++)
	{
#if 1 /* Test Accelerometer */
		if (g_adChannelConfig == THREE_ACC_CHANNELS_NO_AIR)
		{
			ACC_DATA_STRUCT accData;
			GetAccelerometerChannelData(&accData);
			s_tempData.a = 0x8000; s_tempData.r = accData.x; s_tempData.t = accData.y; s_tempData.v = accData.z;
		}
		else { ReadAnalogData(&s_tempData); }
#else
		ReadAnalogData(&s_tempData);
#endif

		//debug("Offset sum data: 0x%x, 0x%x, 0x%x, 0x%x\r\n", s_tempData.r, s_tempData.v, s_tempData.t, s_tempData.a);

		s_workingChannelOffset.rTotal += s_tempData.r;
		s_workingChannelOffset.vTotal += s_tempData.v;
		s_workingChannelOffset.tTotal += s_tempData.t;
		s_workingChannelOffset.aTotal += s_tempData.a;

		// Delay equivalent to the time in between gathering samples for the current sample rate
		SoftUsecWait(timeDelay);
	}

	// Average out the summations
	s_workingChannelOffset.rTotal /= (sampleRate * 1);
	s_workingChannelOffset.vTotal /= (sampleRate * 1);
	s_workingChannelOffset.tTotal /= (sampleRate * 1);
	s_workingChannelOffset.aTotal /= (sampleRate * 1);

	// Set the channel offsets
	g_channelOffset.r_offset = (int16)(s_workingChannelOffset.rTotal - ACCURACY_16_BIT_MIDPOINT);
	g_channelOffset.v_offset = (int16)(s_workingChannelOffset.vTotal - ACCURACY_16_BIT_MIDPOINT);
	g_channelOffset.t_offset = (int16)(s_workingChannelOffset.tTotal - ACCURACY_16_BIT_MIDPOINT);
	g_channelOffset.a_offset = (int16)(s_workingChannelOffset.aTotal - ACCURACY_16_BIT_MIDPOINT);

	debug("A/D Channel First Pass channel average: 0x%x, 0x%x, 0x%x, 0x%x\r\n", s_workingChannelOffset.rTotal, s_workingChannelOffset.vTotal, s_workingChannelOffset.tTotal, s_workingChannelOffset.aTotal);
	debug("A/D Channel First Pass channel offsets: %d, %d, %d, %d\r\n", g_channelOffset.r_offset, g_channelOffset.v_offset, g_channelOffset.t_offset, g_channelOffset.a_offset);

	memset(&s_workingChannelOffset, 0, sizeof(s_workingChannelOffset));
	memset(&s_workingChannelCounts, 0, sizeof(s_workingChannelCounts));

	debug("Get Channel Offset: 2nd Pass Read and sum...\r\n");
	// Read and sum samples
	for (i = 0; i < (sampleRate * 1); i++)
	{
#if 1 /* Test Accelerometer */
		if (g_adChannelConfig == THREE_ACC_CHANNELS_NO_AIR)
		{
			ACC_DATA_STRUCT accData;
			GetAccelerometerChannelData(&accData);
			s_tempData.a = 0x8000; s_tempData.r = accData.x; s_tempData.t = accData.y; s_tempData.v = accData.z;
		}
		else { ReadAnalogData(&s_tempData); }
#else
		ReadAnalogData(&s_tempData);
#endif

		//debug("Offset sum data: 0x%x, 0x%x, 0x%x, 0x%x\r\n", s_tempData.r, s_tempData.v, s_tempData.t, s_tempData.a);

		if (((s_tempData.r - g_channelOffset.r_offset) > (ACCURACY_16_BIT_MIDPOINT - SEISMIC_TRIGGER_MIN_VALUE)) && 
			(((s_tempData.r - g_channelOffset.r_offset) < (ACCURACY_16_BIT_MIDPOINT + SEISMIC_TRIGGER_MIN_VALUE))))
		{
			s_workingChannelOffset.rTotal += s_tempData.r;
			s_workingChannelCounts.rCount++;
		}

		if (((s_tempData.v - g_channelOffset.v_offset) > (ACCURACY_16_BIT_MIDPOINT - SEISMIC_TRIGGER_MIN_VALUE)) && 
			(((s_tempData.v - g_channelOffset.v_offset) < (ACCURACY_16_BIT_MIDPOINT + SEISMIC_TRIGGER_MIN_VALUE))))
		{
			s_workingChannelOffset.vTotal += s_tempData.v;
			s_workingChannelCounts.vCount++;
		}

		if (((s_tempData.t - g_channelOffset.t_offset) > (ACCURACY_16_BIT_MIDPOINT - SEISMIC_TRIGGER_MIN_VALUE)) && 
			(((s_tempData.t - g_channelOffset.t_offset) < (ACCURACY_16_BIT_MIDPOINT + SEISMIC_TRIGGER_MIN_VALUE))))
		{
			s_workingChannelOffset.tTotal += s_tempData.t;
			s_workingChannelCounts.tCount++;
		}

		if (((s_tempData.a - g_channelOffset.a_offset) > (ACCURACY_16_BIT_MIDPOINT - SEISMIC_TRIGGER_MIN_VALUE)) && 
			(((s_tempData.a - g_channelOffset.a_offset) < (ACCURACY_16_BIT_MIDPOINT + SEISMIC_TRIGGER_MIN_VALUE))))
		{
			s_workingChannelOffset.aTotal += s_tempData.a;
			s_workingChannelCounts.aCount++;
		}

		// Delay equivalent to the time in between gathering samples for the current sample rate
		SoftUsecWait(timeDelay);
	}

	// Check if count is non-zero, then average out the summations and set the channel offsets
	if (s_workingChannelCounts.rCount) { s_workingChannelOffset.rTotal /= s_workingChannelCounts.rCount; g_channelOffset.r_offset = (int16)(s_workingChannelOffset.rTotal - ACCURACY_16_BIT_MIDPOINT); }
	if (s_workingChannelCounts.vCount) { s_workingChannelOffset.vTotal /= s_workingChannelCounts.vCount; g_channelOffset.v_offset = (int16)(s_workingChannelOffset.vTotal - ACCURACY_16_BIT_MIDPOINT); }
	if (s_workingChannelCounts.tCount) { s_workingChannelOffset.tTotal /= s_workingChannelCounts.tCount; g_channelOffset.t_offset = (int16)(s_workingChannelOffset.tTotal - ACCURACY_16_BIT_MIDPOINT); }
	if (s_workingChannelCounts.aCount) { s_workingChannelOffset.aTotal /= s_workingChannelCounts.aCount; g_channelOffset.a_offset = (int16)(s_workingChannelOffset.aTotal - ACCURACY_16_BIT_MIDPOINT); }

	debug("A/D Channel Second Pass channel average: 0x%x, 0x%x, 0x%x, 0x%x\r\n", s_workingChannelOffset.rTotal, s_workingChannelOffset.vTotal, s_workingChannelOffset.tTotal, s_workingChannelOffset.aTotal);
	debug("A/D Channel Second Pass channel offsets: %d, %d, %d, %d\r\n", g_channelOffset.r_offset, g_channelOffset.v_offset, g_channelOffset.t_offset, g_channelOffset.a_offset);

	// If we had to power on the A/D here locally, then power it off
	if (powerAnalogDown == YES)
	{
		DisableSensorBlocks();
		PowerControl(ADC_RESET, ON);
	}		
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ZeroSensors(void)
{
	// Check if the count has been established which means init has processed
	if (g_updateOffsetCount)
	{
		// Check to make sure there is new data to process
		if (s_zeroSensorPretriggerComparePtr != g_tailOfPretriggerBuff)
		{
			// Update the comparator
			s_zeroSensorPretriggerComparePtr = g_tailOfPretriggerBuff;

			// Get data
			s_tempData.r = ((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->r;
			s_tempData.v = ((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->v;
			s_tempData.t = ((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->t;
			s_tempData.a = ((SAMPLE_DATA_STRUCT*)g_tailOfPretriggerBuff)->a;

			// Only capture samples that fall within the filter band (filtering spikes and seismic activity)
			if ((s_tempData.r > (ACCURACY_16_BIT_MIDPOINT - SEISMIC_TRIGGER_ADJUST_FILTER)) && ((s_tempData.r < (ACCURACY_16_BIT_MIDPOINT + SEISMIC_TRIGGER_ADJUST_FILTER))))
			{
				s_workingChannelOffset.rTotal += s_tempData.r;
				s_workingChannelCounts.rCount++;
			}

			if ((s_tempData.v > (ACCURACY_16_BIT_MIDPOINT - SEISMIC_TRIGGER_ADJUST_FILTER)) && ((s_tempData.v < (ACCURACY_16_BIT_MIDPOINT + SEISMIC_TRIGGER_ADJUST_FILTER))))
			{
				s_workingChannelOffset.vTotal += s_tempData.v;
				s_workingChannelCounts.vCount++;
			}

			if ((s_tempData.t > (ACCURACY_16_BIT_MIDPOINT - SEISMIC_TRIGGER_ADJUST_FILTER)) && ((s_tempData.t < (ACCURACY_16_BIT_MIDPOINT + SEISMIC_TRIGGER_ADJUST_FILTER))))
			{
				s_workingChannelOffset.tTotal += s_tempData.t;
				s_workingChannelCounts.tCount++;
			}

			if ((s_tempData.a > (ACCURACY_16_BIT_MIDPOINT - SEISMIC_TRIGGER_ADJUST_FILTER)) && ((s_tempData.a < (ACCURACY_16_BIT_MIDPOINT + SEISMIC_TRIGGER_ADJUST_FILTER))))
			{
				s_workingChannelOffset.aTotal += s_tempData.a;
				s_workingChannelCounts.aCount++;
			}

			// Done gathering offset data
			if (--g_updateOffsetCount == 0)
			{
				// Check if the majority of samples were within the band meaning little activity, seismic or spurious (~94% of the samples)
				if ((s_workingChannelCounts.rCount > (g_triggerRecord.trec.sample_rate - (g_triggerRecord.trec.sample_rate >> 4))) &&
					(s_workingChannelCounts.vCount > (g_triggerRecord.trec.sample_rate - (g_triggerRecord.trec.sample_rate >> 4))) &&
					(s_workingChannelCounts.tCount > (g_triggerRecord.trec.sample_rate - (g_triggerRecord.trec.sample_rate >> 4))) &&
					(s_workingChannelCounts.aCount > (g_triggerRecord.trec.sample_rate - (g_triggerRecord.trec.sample_rate >> 4))))
				{
					// Average out the summations
					s_workingChannelOffset.rTotal /= s_workingChannelCounts.rCount;
					s_workingChannelOffset.vTotal /= s_workingChannelCounts.vCount;
					s_workingChannelOffset.tTotal /= s_workingChannelCounts.tCount;
					s_workingChannelOffset.aTotal /= s_workingChannelCounts.aCount;

					// Check if the comparison structure is empty (unfilled)
					if (s_compareChannelOffset.rTotal == 0)
					{
						s_compareChannelOffset = s_workingChannelOffset;
					}
					else // Check if the working offsets are within range of the comparison to suggest stability on the signals
					{
						if ((abs(s_workingChannelOffset.rTotal - s_compareChannelOffset.rTotal) < 4) &&
							(abs(s_workingChannelOffset.vTotal - s_compareChannelOffset.vTotal) < 4) &&
							(abs(s_workingChannelOffset.tTotal - s_compareChannelOffset.tTotal) < 4) &&
							(abs(s_workingChannelOffset.aTotal - s_compareChannelOffset.aTotal) < 4))
						{
							debug("Zero Sensor - A/D Channel offsets (old): %d, %d, %d, %d\r\n", g_channelOffset.r_offset, g_channelOffset.v_offset, g_channelOffset.t_offset, g_channelOffset.a_offset);

							// Adjust the channel offsets
							g_channelOffset.r_offset += (int16)(s_workingChannelOffset.rTotal - ACCURACY_16_BIT_MIDPOINT);
							g_channelOffset.v_offset += (int16)(s_workingChannelOffset.vTotal - ACCURACY_16_BIT_MIDPOINT);
							g_channelOffset.t_offset += (int16)(s_workingChannelOffset.tTotal - ACCURACY_16_BIT_MIDPOINT);
							g_channelOffset.a_offset += (int16)(s_workingChannelOffset.aTotal - ACCURACY_16_BIT_MIDPOINT);

							debug("Zero Sensor - A/D Channel offsets (new): %d, %d, %d, %d\r\n", g_channelOffset.r_offset, g_channelOffset.v_offset, g_channelOffset.t_offset, g_channelOffset.a_offset);

							clearSystemEventFlag(UPDATE_OFFSET_EVENT);
						}

						// Clear the comparison structure (either case where a match is found or not, also initializes for next time)
						memset(&s_compareChannelOffset, 0, sizeof(s_compareChannelOffset));
					}
				}
				// else with the flag UPDATE_OFFSET_EVENT still set and g_updateOffsetCount == 0, the process will start over since majority of samples not within band (~94%)
			}
		}
	}
	else // Start processing counter for new zero crossing
	{
		// Check if the comparison structure is empty (unfilled)
		if (s_compareChannelOffset.rTotal == 0)
		{
			debug("Resume Offset adjustment for zero/temp drift (0x%x), First Pass...\r\n", g_storedTempReading);
		}
		else
		{
			debug("Resume Offset adjustment for zero/temp drift (0x%x), Second Pass...\r\n", g_storedTempReading);
		}
			
		// Initialize the counter for checking samples
		g_updateOffsetCount = g_triggerRecord.trec.sample_rate;
			
		InitAndSeedChannelOffsetVariables();
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UpdateChannelOffsetsForTempChange(void)
{
	// Make sure system isn't processing an event, not handling any system events but UPDATE_OFFSET_EVENT and not handling a manual cal pulse
	if ((g_busyProcessingEvent == NO) && (anySystemEventExcept(UPDATE_OFFSET_EVENT) == NO) && (g_triggerRecord.opMode != MANUAL_CAL_MODE))
	{
		ZeroSensors();
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ZeroingSensorCalibration(void)
{
	OFFSET_DATA_STRUCT zeroCheck = {0, 0, 0, 0};
	OFFSET_DATA_STRUCT zeroCheckCompare = {0, 0, 0, 0};
	uint32 startZeroSensorTime;
	uint32 lastHalfSecondTime;
	uint16 rDiff, vDiff, tDiff, aDiff;
	char spareBuffer[128]; // Using stack string buffer since prior use of the global spare buffer looks to be stepped on while zeroing

	//=========================================================================
	// Zero Sensor Calibration
	//-------------------------------------------------------------------------
	sprintf(spareBuffer, "%s (%s %d %s) ", getLangText(ZEROING_SENSORS_TEXT), getLangText(MAX_TEXT), ZERO_SENSOR_MAX_TIME_IN_SECONDS, getLangText(SEC_TEXT));
	OverlayMessage(getLangText(STATUS_TEXT), spareBuffer, 0);

	// Fool system and initialize buffers and pointers as if a waveform
	InitDataBuffs(WAVEFORM_MODE);

	StartADDataCollectionForCalibration(g_triggerRecord.trec.sample_rate);

	lastHalfSecondTime = startZeroSensorTime = g_lifetimeHalfSecondTickCount;

	raiseSystemEventFlag(UPDATE_OFFSET_EVENT);

	while (g_lifetimeHalfSecondTickCount < (startZeroSensorTime + (ZERO_SENSOR_MAX_TIME_IN_SECONDS * 2)))
	{
		ZeroSensors();

		// Check if the time changed, used for display purposes
		if (g_lifetimeHalfSecondTickCount != lastHalfSecondTime)
		{
			lastHalfSecondTime = g_lifetimeHalfSecondTickCount;

			if (lastHalfSecondTime % 4 == 0)
			{
				strcat(spareBuffer, ".");
				OverlayMessage(getLangText(STATUS_TEXT), spareBuffer, 0);
			}
		}

		// Check if Update Offset Event flag was cleared meaning a successful Zero Sensor cycle passed
		if (getSystemEventState(UPDATE_OFFSET_EVENT) == NO)
		{
			// Get differential from current Zero Sensor cycle and previous results
			rDiff = abs(g_channelOffset.r_offset - zeroCheck.r_offset);
			vDiff = abs(g_channelOffset.v_offset - zeroCheck.v_offset);
			tDiff = abs(g_channelOffset.t_offset - zeroCheck.t_offset);
			aDiff = abs(g_channelOffset.a_offset - zeroCheck.a_offset);

			// Check if differential is less than 2 counts for every channel for current and previous cycle
			if ((rDiff < 2) && (vDiff < 2) && (tDiff < 2) && (aDiff < 2))
			{
				// Get differential from current Zero Sensor cycle and previous
				rDiff = abs(zeroCheckCompare.r_offset - zeroCheck.r_offset);
				vDiff = abs(zeroCheckCompare.v_offset - zeroCheck.v_offset);
				tDiff = abs(zeroCheckCompare.t_offset - zeroCheck.t_offset);
				aDiff = abs(zeroCheckCompare.a_offset - zeroCheck.a_offset);

				// Check if differential is less than 2 counts for every channel for previous two cycles (more used to determine sensor drift during warm up)
				if ((rDiff < 2) && (vDiff < 2) && (tDiff < 2) && (aDiff < 2))
				{
					// Found a match close enough to proceed
					debug("Zero Sensor: 3 consecutive cycle match found\r\n");
					break;
				}
				else { debug("Zero Sensor: Greater than 2 count differential with a channel for previous two cycles\r\n"); }
			}
			else { debug("Zero Sensor: Greater than 2 count differential with a channel for current and previous cycle\r\n"); }

			// Save current Zero Sensor comparison next time
			zeroCheckCompare = zeroCheck;
			zeroCheck = g_channelOffset;

			// Raise the Update Offset flag
			raiseSystemEventFlag(UPDATE_OFFSET_EVENT);
		}
	}

#if 1 /* Test */
	if (g_lifetimeHalfSecondTickCount >= (startZeroSensorTime + (ZERO_SENSOR_MAX_TIME_IN_SECONDS * 2))) { debugWarn("Zero Sensors: Timed out finding stable offset\r\n"); }
#endif

	StopADDataCollectionForCalibration();

	clearSystemEventFlag(UPDATE_OFFSET_EVENT);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void WaitAnalogPower5vGood(void)
{
	uint8_t report = YES;

	while (GetPowerGood5vState() == NO)
	{
		if (report)
		{
			debug("Waiting for Analog 5V to come up\r\n");
			report = NO;
		}
	}

	// Bring External ADC out of reset
	PowerControl(ADC_RESET, OFF);

	// External ADC internal reference buffer turn on time for Cref = 1.1uF is ~11ms (should only needs ~3ms from cold power on for digital interface)
	SoftUsecWait(11 * SOFT_MSECS);

	// Any extra delay needed for the analog to power up/stabilize?
	SoftUsecWait(50 * SOFT_MSECS);
}

///============================================================================
///----------------------------------------------------------------------------
///	External ADC - AD4695
///----------------------------------------------------------------------------
///============================================================================

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AD4695_SpiWriteRegister(uint16_t reg_addr, uint8_t reg_data)
{
	uint8_t writeData[3];

	writeData[0] = ((reg_addr >> 8) & 0x7F);
	writeData[1] = 0xFF & reg_addr;
	writeData[2] = reg_data;

	SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, writeData, sizeof(writeData), NULL, 0, BLOCKING);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AD4695_SpiReadRegister(uint16_t reg_addr, uint8_t* reg_data)
{
	uint8_t writeData[3];
	uint8_t readData[3];

	writeData[0] = (1 << 7) | ((reg_addr >> 8) & 0x7F);
	writeData[1] = 0xFF & reg_addr;
	writeData[2] = 0xFF;

	SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, writeData, sizeof(writeData), readData, sizeof(readData), BLOCKING);
	*reg_data = readData[2];
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AD4695_SpiReadRegisterWithMask(uint16_t reg_addr, uint8_t mask, uint8_t* data)
{
	uint8_t regData[3];

	AD4695_SpiReadRegister(reg_addr, regData);
	*data = (regData[2] & mask);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t AD4695_SpiWriteRegisterWithMask(uint16_t reg_addr, uint8_t mask, uint8_t data)
{
	uint8_t regData;

	AD4695_SpiReadRegister(reg_addr, &regData);
	regData &= ~mask;
	regData |= data;
	AD4695_SpiWriteRegister(reg_addr, regData);

	return (regData);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AD4695_SetRegisterAccessMode(enum ad4695_reg_access access)
{
	uint8_t test, verify;

	test = AD4695_SpiWriteRegisterWithMask(AD4695_REG_SPI_CONFIG_C, AD4695_REG_SPI_CONFIG_C_MB_STRICT_MASK, AD4695_REG_SPI_CONFIG_C_MB_STRICT(access));
	AD4695_SpiReadRegister(AD4695_REG_SPI_CONFIG_C, &verify);

	if(test != verify) { debugErr("External ADC: Access mode error\r\n"); }
	//else debug("External ADC: Access mode updated\r\n");
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AD4695_SetBusyState(void)
{
	uint8_t test, verify;

	// Set BSY_ALT_GP0 GPO Enable bit (general-purpose output function on BSY_ALT_GP0 enabled)
	test = AD4695_SpiWriteRegisterWithMask(AD4695_REG_GPIO_CTRL, AD4695_GPIO_CTRL_GPO0_EN_MASK, AD4695_GPIO_CTRL_GPO0_EN_EN(1));
	AD4695_SpiReadRegister(AD4695_REG_GPIO_CTRL, &verify);
	if(test != verify) { debugErr("External ADC: Busy state error (1)\r\n"); }
	else
	{
		// Set BUSY_GP_EN bit (busy indicator on the general-purpose pin function enabled)
		test = AD4695_SpiWriteRegisterWithMask(AD4695_REG_GP_MODE, AD4695_GP_MODE_BUSY_GP_EN_MASK, AD4695_GP_MODE_BUSY_GP_EN(1));
		AD4695_SpiReadRegister(AD4695_REG_GP_MODE, &verify);
		if(test != verify) { debugErr("External ADC: Busy state error (1)\r\n"); }
		//else { debug("External ADC: Set Busy status on Busy/Alt GPIO pin\r\n"); }
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AD4695_SetSDOMode(uint8_t sdoMode)
{
	uint8_t test, verify;

	test = AD4695_SpiWriteRegisterWithMask(AD4695_REG_GP_MODE, AD4695_GP_MODE_SDO_MODE_MASK, AD4695_GP_MODE_SDO_MODE_SEL(sdoMode));
	AD4695_SpiReadRegister(AD4695_REG_GP_MODE, &verify);

	if(test != verify) { /* report error */ }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AD4695_SetStandardSequenceModeChannelOSR(enum ad4695_osr_ratios ratio) /*over sampling ratio in standard sequencer mode*/
{
	uint8_t test, verify;

	test = AD4695_SpiWriteRegisterWithMask(AD4695_REG_CONFIG_IN(0), AD4695_REG_CONFIG_IN_OSR_MASK, AD4695_REG_CONFIG_IN_OSR(ratio));
	AD4695_SpiReadRegister(AD4695_REG_CONFIG_IN(0), &verify);

	if(test != verify){ debugErr("External ADC: Channel input OSR setting error\r\n"); }
	//else { debug("External ADC: Channel input OSR set\r\n"); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AD4695_SetStandardSequenceModeChannelInputConfig(uint8_t regData)
{
	uint8_t verify;

	AD4695_SpiWriteRegister(AD4695_REG_CONFIG_IN(0), regData);
	AD4695_SpiReadRegister(AD4695_REG_CONFIG_IN(0), &verify);

	if(regData != verify){ debugErr("External ADC: Standard sequence mode channel input config error\r\n"); }
	//else { debug("External ADC: Standard sequence mode channel input config set\r\n"); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AD4695_SetStandardMode() /*Standard Sequencer Enable Bit*/
{
	uint8_t test, verify;

	test = AD4695_SpiWriteRegisterWithMask(AD4695_REG_SEQ_CTRL, AD4695_SEQ_CTRL_STD_SEQ_EN_MASK, AD4695_SEQ_CTRL_STD_SEQ_EN(1));
	AD4695_SpiReadRegister(AD4695_REG_SEQ_CTRL, &verify);

	if(test != verify) { debugErr("External ADC: Standard mode set error\r\n"); }
	//else { debug("External ADC: Standard mode set\r\n"); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AD4695_SetReferenceInputRange(enum ad4695_ref REF)
{
	uint8_t test, verify;

	/*Reference Input Range Control  0x0: 2.4 V ≤ VREF ≤ 2.75 V.
		0x1: 2.75 V < VREF ≤ 3.25 V.
		0x2: 3.25 V < VREF ≤ 3.75 V.
		0x3: 3.75 V < VREF ≤ 4.50 V.
		0x4: 4.5 V < VREF ≤ 5.10 V.*/

	test = AD4695_SpiWriteRegisterWithMask(AD4695_REG_REF_CTRL, AD4695_REG_REF_CTRL_MASK, AD4695_REG_REF_CTRL_EN(REF));
	AD4695_SpiReadRegister(AD4695_REG_REF_CTRL, &verify);

	if(test != verify) { debugErr("External ADC: Reference control error\r\n"); }
	//else { debug("External ADC: Reference control set\r\n"); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AD4695_SetStandardSequenceActiveChannels(uint8_t channels)
{
	uint8_t verify;

	// Setup channels to be sequenced
	AD4695_SpiWriteRegister(AD4695_REG_STD_SEQ_CONFIG, channels);
	AD4695_SpiReadRegister(AD4695_REG_STD_SEQ_CONFIG, &verify);

	if(channels != verify) { debugErr("External ADC: Set standard sequence active channels error (0x%x != 0x%x)\r\n", channels, verify); }
	else { debug("External ADC: Standard sequence active channels set (0x%x)\r\n", channels); }

#if 1 /* Temp method to dynamically change the channel verify, but fixed to either Geo1+AOP1 or Geo2+AOP2 (static 4 channels) */
	// Todo: Update to handle dynamic number of channels
	memset(chanActive, 0, sizeof(chanActive));
	for (uint8_t i = 0; i < 8; i++)
	{
		if (channels & (1 << i))
		{
			chanActive[i] = ON;
		}
	}
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t AD4695_GetStandardSequenceActiveChannels(void)
{
	uint8_t regData;

	// Setup channels to be sequenced
	AD4695_SpiReadRegister(AD4695_REG_STD_SEQ_CONFIG, &regData);

	return (regData);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AD4695_SetAutocycleControl(uint8_t regData)
{
	AD4695_SpiWriteRegister(AD4695_REG_AC_CTRL, regData);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AD4695_SetTemperatureSensorEnable(uint8_t mode)
{
	// Make sure mode is binary control
	if (mode > ON) { mode = ON; }

	if (mode) { debug("External ADC: Enabled reading Temperature sensor\r\n"); }
	else { debug("External ADC: Disabled reading Temperature sensor\r\n"); }

	AD4695_SpiWriteRegister(AD4695_REG_TEMP_CTRL, mode);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AD4695_DisableInternalLDO(void)
{
	AD4695_SpiWriteRegister(AD4695_REG_SETUP, 0x00);
	AD4695_SpiWriteRegisterWithMask(AD4695_REG_SETUP, AD4695_SETUP_LDO_ENABLE_MASK, AD4695_SETUP_LDO_ENABLE_EN(OFF));
	//debug("External ADC: Disabling Internal LDO...\r\n");
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AD4695_Init()
{
	uint8_t testData;
	debug("External ADC: Init\r\n");
#if 1 /* Test */
	AD4695_SpiReadRegister(AD4695_REG_VENDOR_L, &testData);
	if (testData != 0x56) { debug("External ADC: Vendor ID verification low byte failed (0x%x not 0x56)\r\n", testData); }
	else
	{
		AD4695_SpiReadRegister(AD4695_REG_VENDOR_H, &testData);
		if (testData != 0x04) { debug("External ADC: Vendor ID verification high byte failed (0x%x not 0x56)\r\n", testData); }
		else { debug("External ADC: Vendor ID verification passed\r\n"); }
	}

	testData = 0xAA; AD4695_SpiWriteRegister(AD4695_REG_SCRATCH_PAD, testData);
	testData = 0x00; AD4695_SpiReadRegister(AD4695_REG_SCRATCH_PAD, &testData);
	if (testData != 0xAA) { debugErr("External ADC: 1st Scratchpad test failed\r\n"); }
	//else debug("External ADC: 1st Scratchpad test passed\r\n");

	testData = 0x55; AD4695_SpiWriteRegister(AD4695_REG_SCRATCH_PAD, testData);
	testData = 0x00; AD4695_SpiReadRegister(AD4695_REG_SCRATCH_PAD, &testData);
	if (testData != 0x55) { debugErr("External ADC: 2nd Scratchpad test failed\r\n"); }
	//else debug("External ADC: 2nd Scratchpad test passed\r\n");
#endif

	AD4695_SetRegisterAccessMode(AD4695_BYTE_ACCESS); //individual bytes in multibyte registers are read from or written to in individual data phases
	AD4695_DisableInternalLDO();
	AD4695_SetBusyState();

	// In standard sequence mode only In channel 0 needs to be set for most of the settings
	// The default value of the In channel registers is pretty much what we want; Unipolar, paired with RefGND (same as COM for us), High-Z mode enabled, No oversampling, no threshold detection
	AD4695_SetStandardSequenceModeChannelOSR(AD4695_OSR_1); //16 bit not 17,18 or 19

	AD4695_SetStandardMode();
	AD4695_SetReferenceInputRange(R4V5_R5V1); // Setting the reference voltage range

	// Some combination of the following: ANALOG_GEO_1, ANALOG_AOP_1, ANALOG_GEO_2, ANALOG_AOP_2
	// Enable selected channels
	if (g_currentSensorGroup == SENSOR_GROUP_A_1)
	{
#if 0 /* Original, Geo + AOP on one sensor group */
		AD4695_SetStandardSequenceActiveChannels((ANALOG_GEO_1 | ANALOG_AOP_1));
#else /* Standard config, Geo + AOP split on two sensor groups */
		AD4695_SetStandardSequenceActiveChannels((ANALOG_GEO_1 | ANALOG_AOP_2));
#endif
	}
	else // SENSOR_GROUP_B_2
	{
#if 0 /* Original, Geo + AOP on one sensor group */
		AD4695_SetStandardSequenceActiveChannels((ANALOG_GEO_2 | ANALOG_AOP_2));
#else /* Standard config, Geo + AOP split on two sensor groups */
		AD4695_SetStandardSequenceActiveChannels((ANALOG_GEO_2 | ANALOG_AOP_1));
#endif
	}

#if 0 /* Not ready to enter conversion mode at this time */
	AD4695_EnterConversionMode(NO); /*Enters conversion mode*/

	// Delay 100ms?
	MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(100));
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AD4695_EnterConversionMode(uint8_t enableChannelStatus)
{
	if (enableChannelStatus)
	{
		AD4695_SpiWriteRegisterWithMask(AD4695_REG_SETUP, AD4695_SETUP_STATUS_ENABLE_MASK, AD4695_SETUP_STATUS_ENABLE_EN(ON));
		debug("External ADC: Enabling channel status\r\n");
	}
	else { debug("External ADC: Disabling channel status\r\n"); }

	// Enter conversion mode
	AD4695_SpiWriteRegisterWithMask(AD4695_REG_SETUP, AD4695_SETUP_IF_MODE_MASK, AD4695_SETUP_IF_MODE_EN(ON));

	debug("External ADC: Entering Conversion mode...\r\n");
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AD4695_ExitConversionMode()
{
	uint8_t command[3];
	command[0] = AD4695_CMD_REG_CONFIG_MODE;

	SetAdcConversionState(ON);
	SpiTransaction(SPI_ADC, SPI_8_BIT_DATA_SIZE, YES, command, 3, NULL, 0, BLOCKING);
	SetAdcConversionState(OFF);

	debug("External ADC: Exit Conversion mode...\r\n");
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int AD4695_TemperatureConversionCtoF(uint16_t tempCode)
{
	float temperature;
	float vTemp;

	vTemp = (float)((5000 * tempCode) / 65536);
	temperature = ((vTemp - 725) / (-1.8));

	// Conversion from C to F, (0°C × 9/5) + 32 = 32°F
	temperature = ((temperature * 9 / 5) + 32);

	return ((int)temperature);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AD4695_Test() //test the SPI com
{
	uint8_t testDataValue = 0x1f; //can be changed
	uint8_t testData;

	while(testData != testDataValue)
	{
		AD4695_SpiWriteRegister(AD4695_REG_SCRATCH_PAD, testDataValue);
		AD4695_SpiReadRegister(AD4695_REG_SCRATCH_PAD, &testData);

		// Delay 100ms?
		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(100));
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestExternalADC(void)
{
	uint8_t testData;

    debug("External ADC: Test device access...\r\n");

	// Check if External ADC is still in reset
	if (GetPowerControlState(ADC_RESET) == ON)
	{
		debug("Power Control: Bringing External ADC out of reset\r\n");
		WaitAnalogPower5vGood();
	}

	debug("External ADC: Initializing...\r\n");
	AD4695_Init();
	debug("External ADC: Init complete\r\n");

	AD4695_SpiReadRegister(AD4695_REG_VENDOR_L, &testData);
	if (testData != 0x56) { debug("External ADC: Vendor ID verification low byte failed (0x%x not 0x56)\r\n", testData); }
	else
	{
		AD4695_SpiReadRegister(AD4695_REG_VENDOR_H, &testData);
		if (testData != 0x04) { debug("External ADC: Vendor ID verification high byte failed (0x%x not 0x56)\r\n", testData); }
		else { debug("External ADC: Vendor ID verification passed\r\n"); }
	}

	testData = 0xAA; AD4695_SpiWriteRegister(AD4695_REG_SCRATCH_PAD, testData);
	testData = 0x00; AD4695_SpiReadRegister(AD4695_REG_SCRATCH_PAD, &testData);
	debug("External ADC: 1st Scratchpad test %s\r\n", (testData == 0xAA) ? "Passed" : "Failed");

	testData = 0x55; AD4695_SpiWriteRegister(AD4695_REG_SCRATCH_PAD, testData);
	testData = 0x00; AD4695_SpiReadRegister(AD4695_REG_SCRATCH_PAD, &testData);
	debug("External ADC: 2nd Scratchpad test %s\r\n", (testData == 0x55) ? "Passed" : "Failed");
}
