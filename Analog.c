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
#include "Uart.h"
#include "Menu.h"
#include "rtc.h"
//#include "tc.h"
//#include "twi.h"
#include "spi.h"

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

// INA1 - R channel
// INA2 - T channel
// INB1 - V channel
// INB2 - A channel

// CNVST (A.L.) | A0 | A/B (A.L.)	|	Read	| Address Select
// -------------------------------------------------------------
//	inactive	| 0	 |		1		| INA1 (R)	|	0010 - 0x2
//	active		| 0	 |		0		| INB1 (V)	|	0000 - 0x0
//	inactive	| 1	 |		1		| INA2 (T)	|	0110 - 0x6
//	active		| 1	 |		0		| INB2 (A)	|	0100 - 0x4
//--------------------------------------------------------------
// Processor Pin| A2 |		A1		|

/*
Config readback reference for different reference configs

(Debug |      0s) Setup A/D config and channels (External Ref, Temp On) (Channel config: 0x39D4)
Chan 0 Config: 0xe150, Chan 1 Config: 0xe350, Chan 2 Config: 0xe550, Chan 3 Config: 0xe750, Temp Config: 0xb750

(Debug |      0s) Setup A/D config and channels (External Ref, Internal Buffer, Temp On) (Channel config: 0x39DC)
Chan 0 Config: 0xe170, Chan 1 Config: 0xe370, Chan 2 Config: 0xe570, Chan 3 Config: 0xe770, Temp Config: 0xb770

(Debug |      0s) Setup A/D config and channels (External Ref, Temp Off) (Channel config: 0x39F4)
Chan 0 Config: 0xe1d0, Chan 1 Config: 0xe3d0, Chan 2 Config: 0xe5d0, Chan 3 Config: 0xe7d0, Temp Config: 0xb7d0

(Debug |      0s) Setup A/D config and channels (External Ref, Internal Buffer, Temp Off) (Channel config: 0x39FC)
Chan 0 Config: 0xe1f0, Chan 1 Config: 0xe3f0, Chan 2 Config: 0xe5f0, Chan 3 Config: 0xe7f0, Temp Config: 0xb7f0
*/

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* Unused at this time */
void GetAnalogConfigReadback(void)
{
	SAMPLE_DATA_STRUCT dummyData;
	uint16 channelConfigReadback;

	if (g_adChannelConfig == FOUR_AD_CHANNELS_WITH_READBACK_WITH_TEMP)
	{
		// Chan 0
		spi_selectChip(&AVR32_SPI0, AD_SPI_NPCS);
		spi_write(&AVR32_SPI0, 0x0000);	spi_read(&AVR32_SPI0, &(dummyData.r));
		spi_write(&AVR32_SPI0, 0x0000);	spi_read(&AVR32_SPI0, &channelConfigReadback);
		spi_unselectChip(&AVR32_SPI0, AD_SPI_NPCS);
		debugRaw("\nChan 0 Config: 0x%x, ", channelConfigReadback);

		// Chan 1
		spi_selectChip(&AVR32_SPI0, AD_SPI_NPCS);
		spi_write(&AVR32_SPI0, 0x0000);	spi_read(&AVR32_SPI0, &(dummyData.t));
		spi_write(&AVR32_SPI0, 0x0000);	spi_read(&AVR32_SPI0, &channelConfigReadback);
		spi_unselectChip(&AVR32_SPI0, AD_SPI_NPCS);
		debugRaw("Chan 1 Config: 0x%x, ", channelConfigReadback);

		// Chan 2
		spi_selectChip(&AVR32_SPI0, AD_SPI_NPCS);
		spi_write(&AVR32_SPI0, 0x0000);	spi_read(&AVR32_SPI0, &(dummyData.v));
		spi_write(&AVR32_SPI0, 0x0000);	spi_read(&AVR32_SPI0, &channelConfigReadback);
		spi_unselectChip(&AVR32_SPI0, AD_SPI_NPCS);
		debugRaw("Chan 2 Config: 0x%x, ", channelConfigReadback);

		// Chan 3
		spi_selectChip(&AVR32_SPI0, AD_SPI_NPCS);
		spi_write(&AVR32_SPI0, 0x0000);	spi_read(&AVR32_SPI0, &(dummyData.a));
		spi_write(&AVR32_SPI0, 0x0000);	spi_read(&AVR32_SPI0, &channelConfigReadback);
		spi_unselectChip(&AVR32_SPI0, AD_SPI_NPCS);
		debugRaw("Chan 3 Config: 0x%x, ", channelConfigReadback);

		// Temp
		spi_selectChip(&AVR32_SPI0, AD_SPI_NPCS);
		spi_write(&AVR32_SPI0, 0x0000);	spi_read(&AVR32_SPI0, (uint16*)&g_currentTempReading);
		spi_write(&AVR32_SPI0, 0x0000);	spi_read(&AVR32_SPI0, &channelConfigReadback);
		spi_unselectChip(&AVR32_SPI0, AD_SPI_NPCS);
		debugRaw("Temp Config: 0x%x", channelConfigReadback);
	}
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ReadAnalogData(SAMPLE_DATA_STRUCT* dataPtr)
{
#if 0 /* old hw */
	uint16 channelConfigReadback;
	uint8 configError = NO;

	// Chan 0 Config: 0xe150, Chan 1 Config: 0xe350, Chan 2 Config: 0xe550, Chan 3 Config: 0xe750, Temp Config: 0xb750

	if (g_adChannelConfig == FOUR_AD_CHANNELS_WITH_READBACK_WITH_TEMP)
	{
		// Chan 0
		spi_selectChip(&AVR32_SPI0, AD_SPI_NPCS);
		if (g_factorySetupRecord.analogChannelConfig == CHANNELS_R_AND_V_SCHEMATIC)
			{ spi_write(&AVR32_SPI0, 0x0000); spi_read(&AVR32_SPI0, &(dataPtr->r)); }
		else // (g_factorySetupRecord.analogChannelConfig == CHANNELS_R_AND_V_SWAPPED)
			{ spi_write(&AVR32_SPI0, 0x0000); spi_read(&AVR32_SPI0, &(dataPtr->v)); }
		spi_write(&AVR32_SPI0, 0x0000);	spi_read(&AVR32_SPI0, &channelConfigReadback);
		spi_unselectChip(&AVR32_SPI0, AD_SPI_NPCS);
		//if (channelConfigReadback != 0xe0d0) { configError = YES; debug("Chan 0 Config: 0x%x\r\n", channelConfigReadback);}
		if (channelConfigReadback != 0xe150) { configError = YES; }

		// Chan 1
		spi_selectChip(&AVR32_SPI0, AD_SPI_NPCS);
		spi_write(&AVR32_SPI0, 0x0000);	spi_read(&AVR32_SPI0, &(dataPtr->t));
		spi_write(&AVR32_SPI0, 0x0000);	spi_read(&AVR32_SPI0, &channelConfigReadback);
		spi_unselectChip(&AVR32_SPI0, AD_SPI_NPCS);
		//if (channelConfigReadback != 0xe2d0) { configError = YES; debug("Chan 1 Config: 0x%x\r\n", channelConfigReadback);}
		if (channelConfigReadback != 0xe350) { configError = YES; }

		// Chan 2
		spi_selectChip(&AVR32_SPI0, AD_SPI_NPCS);
		if (g_factorySetupRecord.analogChannelConfig == CHANNELS_R_AND_V_SCHEMATIC)
			{ spi_write(&AVR32_SPI0, 0x0000); spi_read(&AVR32_SPI0, &(dataPtr->v)); }
		else // (g_factorySetupRecord.analogChannelConfig == CHANNELS_R_AND_V_SWAPPED)
			{ spi_write(&AVR32_SPI0, 0x0000); spi_read(&AVR32_SPI0, &(dataPtr->r)); }
		spi_write(&AVR32_SPI0, 0x0000);	spi_read(&AVR32_SPI0, &channelConfigReadback);
		spi_unselectChip(&AVR32_SPI0, AD_SPI_NPCS);
		//if (channelConfigReadback != 0xe4d0) { configError = YES; debug("Chan 2 Config: 0x%x\r\n", channelConfigReadback);}
		if (channelConfigReadback != 0xe550) { configError = YES; }

		// Chan 3
		spi_selectChip(&AVR32_SPI0, AD_SPI_NPCS);
		spi_write(&AVR32_SPI0, 0x0000);	spi_read(&AVR32_SPI0, &(dataPtr->a));
		spi_write(&AVR32_SPI0, 0x0000);	spi_read(&AVR32_SPI0, &channelConfigReadback);
		spi_unselectChip(&AVR32_SPI0, AD_SPI_NPCS);
		//if (channelConfigReadback != 0xe6d0) { configError = YES; debug("Chan 3 Config: 0x%x\r\n", channelConfigReadback);}
		if (channelConfigReadback != 0xe750) { configError = YES; }

		// Temp
		spi_selectChip(&AVR32_SPI0, AD_SPI_NPCS);
		spi_write(&AVR32_SPI0, 0x0000);	spi_read(&AVR32_SPI0, (uint16*)&g_currentTempReading);
		spi_write(&AVR32_SPI0, 0x0000);	spi_read(&AVR32_SPI0, &channelConfigReadback);
		spi_unselectChip(&AVR32_SPI0, AD_SPI_NPCS);
		//if (channelConfigReadback != 0xb6d0) { configError = YES; debug("Temp Config: 0x%x\r\n", channelConfigReadback);}
		if (channelConfigReadback != 0xb750) { configError = YES; }
			
		if (configError == YES)
		{
			debugErr("AD Channel config error! Channel data is not in sync\r\n");
		}
	}
	else if (g_adChannelConfig == FOUR_AD_CHANNELS_NO_READBACK_WITH_TEMP)
	{
		// Chan 0
		spi_selectChip(&AVR32_SPI0, AD_SPI_NPCS);
		if (g_factorySetupRecord.analogChannelConfig == CHANNELS_R_AND_V_SCHEMATIC)
			{ spi_write(&AVR32_SPI0, 0x0000); spi_read(&AVR32_SPI0, &(dataPtr->r)); }
		else // (g_factorySetupRecord.analogChannelConfig == CHANNELS_R_AND_V_SWAPPED)
			{ spi_write(&AVR32_SPI0, 0x0000); spi_read(&AVR32_SPI0, &(dataPtr->v)); }
		spi_unselectChip(&AVR32_SPI0, AD_SPI_NPCS);

		// Chan 1
		spi_selectChip(&AVR32_SPI0, AD_SPI_NPCS);
		spi_write(&AVR32_SPI0, 0x0000);	spi_read(&AVR32_SPI0, &(dataPtr->t));
		spi_unselectChip(&AVR32_SPI0, AD_SPI_NPCS);

		// Chan 2
		spi_selectChip(&AVR32_SPI0, AD_SPI_NPCS);
		if (g_factorySetupRecord.analogChannelConfig == CHANNELS_R_AND_V_SCHEMATIC)
			{ spi_write(&AVR32_SPI0, 0x0000); spi_read(&AVR32_SPI0, &(dataPtr->v)); }
		else // (g_factorySetupRecord.analogChannelConfig == CHANNELS_R_AND_V_SWAPPED)
			{ spi_write(&AVR32_SPI0, 0x0000); spi_read(&AVR32_SPI0, &(dataPtr->r)); }
		spi_unselectChip(&AVR32_SPI0, AD_SPI_NPCS);

		// Chan 3
		spi_selectChip(&AVR32_SPI0, AD_SPI_NPCS);
		spi_write(&AVR32_SPI0, 0x0000);	spi_read(&AVR32_SPI0, &(dataPtr->a));
		spi_unselectChip(&AVR32_SPI0, AD_SPI_NPCS);

		// Temp
		spi_selectChip(&AVR32_SPI0, AD_SPI_NPCS);
		spi_write(&AVR32_SPI0, 0x0000);	spi_read(&AVR32_SPI0, (uint16*)&g_currentTempReading);
		spi_unselectChip(&AVR32_SPI0, AD_SPI_NPCS);
	}
	else // FOUR_AD_CHANNELS_NO_READBACK_NO_TEMP
	{
		// Chan 0
		spi_selectChip(&AVR32_SPI0, AD_SPI_NPCS);
		if (g_factorySetupRecord.analogChannelConfig == CHANNELS_R_AND_V_SCHEMATIC)
			{ spi_write(&AVR32_SPI0, 0x0000); spi_read(&AVR32_SPI0, &(dataPtr->r)); }
		else // (g_factorySetupRecord.analogChannelConfig == CHANNELS_R_AND_V_SWAPPED)
			{ spi_write(&AVR32_SPI0, 0x0000); spi_read(&AVR32_SPI0, &(dataPtr->v)); }
		spi_unselectChip(&AVR32_SPI0, AD_SPI_NPCS);

		// Chan 1
		spi_selectChip(&AVR32_SPI0, AD_SPI_NPCS);
		spi_write(&AVR32_SPI0, 0x0000);	spi_read(&AVR32_SPI0, &(dataPtr->t));
		spi_unselectChip(&AVR32_SPI0, AD_SPI_NPCS);

		// Chan 2
		spi_selectChip(&AVR32_SPI0, AD_SPI_NPCS);
		if (g_factorySetupRecord.analogChannelConfig == CHANNELS_R_AND_V_SCHEMATIC)
			{ spi_write(&AVR32_SPI0, 0x0000); spi_read(&AVR32_SPI0, &(dataPtr->v)); }
		else // (g_factorySetupRecord.analogChannelConfig == CHANNELS_R_AND_V_SWAPPED)
			{ spi_write(&AVR32_SPI0, 0x0000); spi_read(&AVR32_SPI0, &(dataPtr->r)); }
		spi_unselectChip(&AVR32_SPI0, AD_SPI_NPCS);

		// Chan 3
		spi_selectChip(&AVR32_SPI0, AD_SPI_NPCS);
		spi_write(&AVR32_SPI0, 0x0000);	spi_read(&AVR32_SPI0, &(dataPtr->a));
		spi_unselectChip(&AVR32_SPI0, AD_SPI_NPCS);
	}	
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitAnalogControl(void)
{
	g_analogControl.reg = 0x0;

	SetAnalogCutoffFrequency(ANALOG_CUTOFF_FREQ_1);
	SetSeismicGainSelect(SEISMIC_GAIN_LOW);
	SetAcousticGainSelect(ACOUSTIC_GAIN_NORMAL);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void WriteADConfig(unsigned int config)
{
#if 0 /* old hw */
	spi_selectChip(&AVR32_SPI0, AD_SPI_NPCS);
	spi_write(&AVR32_SPI0, ((unsigned short) config << 2));
	spi_unselectChip(&AVR32_SPI0, AD_SPI_NPCS);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupADChannelConfig(uint32 sampleRate, uint8 channelVerification)
{
	// AD config all channels, with temp, with read back
	// Overwrite (set config), Unipolar, INx referenced to COM = GND � 0.1 V, Stop after Channel 3 (0 bias), Full BW, 
	//	External reference, temperature enabled (assumed internal buffer disabled), Scan IN0 to IN3 then read temp, Read back the CFG register
	// 00 1 110 011 1 010 10 0
	// 0011 1001 1101 0100 - 0x39D4
	
	// AD config all channels, with temp, no read back
	// Overwrite (set config), Unipolar, INx referenced to COM = GND � 0.1 V, Stop after Channel 3 (0 bias), Full BW,
	//	External reference, temperature enabled (assumed internal buffer disabled), Scan IN0 to IN3 then read temp, Do not read back the CFG register
	// 00 1 110 011 1 010 10 1
	// 0011 1001 1101 0101 - 0x39D5
	
	// AD config all channels, no temp, no read back
	// Overwrite (set config), Unipolar, INx referenced to COM = GND � 0.1 V, Stop after Channel 3 (0 bias), Full BW,
	//	External reference, temperature disabled (assumed internal buffer disabled), Scan IN0 to IN3 only, Do not read back the CFG register
	// 00 1 110 011 1 110 11 1
	// 0011 1001 1111 0111 - 0x39F7

	// For any sample rate 8K and below
	if (sampleRate <= SAMPLE_RATE_8K)
	{
		// Check if channel verification is not disabled or verification override is enabled to allow reading back the config
		if ((g_unitConfig.adChannelVerification != DISABLED) || (channelVerification == OVERRIDE_ENABLE_CHANNEL_VERIFICATION))
		{
			//===================================================
			// Setup config for 4 Chan, With read back, With temp
			//---------------------------------------------------
			WriteADConfig(0x39D4);
			WriteADConfig(0x39D4);
			WriteADConfig(0x39D4);
		
			g_adChannelConfig = FOUR_AD_CHANNELS_WITH_READBACK_WITH_TEMP;
		}
		else // Verification disabled, don't read back config
		{
			//=================================================
			// Setup config for 4 Chan, No read back, With temp
			//-------------------------------------------------
			WriteADConfig(0x39D5);
			WriteADConfig(0x39D5);
			WriteADConfig(0x39D5);
		
			g_adChannelConfig = FOUR_AD_CHANNELS_NO_READBACK_WITH_TEMP;
		}
	}
	else // Sample rates above 8192 take too long to read back config and temp, so skip them
	{
		//===============================================
		// Setup config for 4 Chan, No read back, No temp
		//-----------------------------------------------
		WriteADConfig(0x39F7);
		WriteADConfig(0x39F7);
		WriteADConfig(0x39F7);

		g_adChannelConfig = FOUR_AD_CHANNELS_NO_READBACK_NO_TEMP;
	}

	//Delay for 1.2us at least
	SoftUsecWait(2);

#if 0 /* old hw */
	spi_selectChip(&AVR32_SPI0, AD_SPI_NPCS);
	spi_write(&AVR32_SPI0, 0x0000);
	spi_unselectChip(&AVR32_SPI0, AD_SPI_NPCS);
#endif

	SoftUsecWait(2);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void WriteAnalogControl(uint16 control)
{
#if 0 /* old hw */
	spi_selectChip(&AVR32_SPI1, AD_CTL_SPI_NPCS);
	spi_write(&AVR32_SPI1, (unsigned short) control);
	spi_unselectChip(&AVR32_SPI1, AD_CTL_SPI_NPCS);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AdSetCalSignalLow(void)
{
	g_analogControl.bit.calSignal = 0;
	g_analogControl.bit.calSignalEnable = 1;

	WriteAnalogControl(g_analogControl.reg);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AdSetCalSignalHigh(void)
{
	g_analogControl.bit.calSignal = 1;
	g_analogControl.bit.calSignalEnable = 1;

	WriteAnalogControl(g_analogControl.reg);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AdSetCalSignalOff(void)
{
	g_analogControl.bit.calSignal = 0;
	g_analogControl.bit.calSignalEnable = 0;

	WriteAnalogControl(g_analogControl.reg);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetAnalogCutoffFrequency(uint8 freq)
{
	// Validated bit selection 8/20/2012
	
	switch (freq)
	{
		case ANALOG_CUTOFF_FREQ_LOW: // 500 Hz
			g_analogControl.bit.cutoffFreqSelectEnable = 1;
		break;

		case ANALOG_CUTOFF_FREQ_1: // 1K Hz
			g_analogControl.bit.cutoffFreqSelectLow = 1;
			g_analogControl.bit.cutoffFreqSelectHi = 1;
			g_analogControl.bit.cutoffFreqSelectEnable = 0;
		break;

		case ANALOG_CUTOFF_FREQ_2: // 2K Hz
			g_analogControl.bit.cutoffFreqSelectLow = 0;
			g_analogControl.bit.cutoffFreqSelectHi = 1;
			g_analogControl.bit.cutoffFreqSelectEnable = 0;
		break;

		case ANALOG_CUTOFF_FREQ_3: // 4K Hz
			g_analogControl.bit.cutoffFreqSelectLow = 1;
			g_analogControl.bit.cutoffFreqSelectHi = 0;
			g_analogControl.bit.cutoffFreqSelectEnable = 0;
		break;

		case ANALOG_CUTOFF_FREQ_4: // 14.3K Hz
			g_analogControl.bit.cutoffFreqSelectLow = 0;
			g_analogControl.bit.cutoffFreqSelectHi = 0;
			g_analogControl.bit.cutoffFreqSelectEnable = 0;
		break;
	}

	WriteAnalogControl(g_analogControl.reg);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetSeismicGainSelect(uint8 seismicGain)
{
	if (seismicGain == SEISMIC_GAIN_LOW)
	{
		// Control is swapped (Low is 1)
		g_analogControl.bit.seismicGainSelect = 1;
	}
	else // seismicGain == SEISMIC_GAIN_HIGH
	{
		// Control is swapped (High is 0)
		g_analogControl.bit.seismicGainSelect = 0;
	}

	WriteAnalogControl(g_analogControl.reg);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetAcousticGainSelect(uint8 acousticGain)
{
	if (acousticGain == ACOUSTIC_GAIN_NORMAL)
	{
		g_analogControl.bit.acousticGainSelect = 0;
	}
	else // seismicGain == ACOUSTIC_GAIN_A_WEIGHTED
	{
		g_analogControl.bit.acousticGainSelect = 1;
	}

	WriteAnalogControl(g_analogControl.reg);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetCalSignalEnable(uint8 enable)
{
	if (enable == ON)
	{
		g_analogControl.bit.calSignalEnable = 1;
	}
	else // enable == OFF
	{
		g_analogControl.bit.calSignalEnable = 0;
	}

	WriteAnalogControl(g_analogControl.reg);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetCalSignal(uint8 data)
{
	if (data)
	{
		g_analogControl.bit.calSignal = 1;
	}
	else // data == NULL
	{
		g_analogControl.bit.calSignal = 0;
	}

	WriteAnalogControl(g_analogControl.reg);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
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
	uint32 timeDelay = (977 / (sampleRate / 512) / 2);
	uint8 powerAnalogDown = NO;

	// Check to see if the A/D is in sleep mode
	if (GetPowerControlState(ANALOG_SLEEP_ENABLE) == ON)
	{
		// Power the A/D on to set the offsets
		PowerControl(ANALOG_SLEEP_ENABLE, OFF);

		// Set flag to signal powering off the A/D when finished
		powerAnalogDown = YES;
	}

	// Reset offset values
	memset(&g_channelOffset, 0, sizeof(g_channelOffset));

	debug("Get Channel Offset: Read and pitch... (Address boundary: %s)\r\n", ((uint32)(&s_tempData) % 4 == 0) ? "YES" : "NO");
	// Read and pitch samples
	for (i = 0; i < (sampleRate * 1); i++)
	{
		ReadAnalogData(&s_tempData);

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
		ReadAnalogData(&s_tempData);

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
		ReadAnalogData(&s_tempData);

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
		PowerControl(ANALOG_SLEEP_ENABLE, ON);
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
				// else with the flag UPDATE_OFFSET_EVENT still set and g_updateOffsetCount == 0, the process will start over
			}
		}
	}
	else // Start processing counter for new zero crossing
	{
		// Check if the comparison structure is empty (unfilled)
		if (s_compareChannelOffset.rTotal == 0)
		{
			debug("Resume Offset adjustment for temp drift (0x%x), First Pass...\r\n", g_storedTempReading);
		}
		else
		{
			debug("Resume Offset adjustment for temp drift (0x%x), Second Pass...\r\n", g_storedTempReading);
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
	if ((g_sleepModeEngaged == YES) && (g_busyProcessingEvent == NO) && (anySystemEventExcept(UPDATE_OFFSET_EVENT) == NO) && (g_triggerRecord.opMode != MANUAL_CAL_MODE))
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

	//=========================================================================
	// Zero Sensor Calibration
	//-------------------------------------------------------------------------
	sprintf((char*)g_spareBuffer, "%s (%s %d %s) ", getLangText(ZEROING_SENSORS_TEXT), getLangText(MAX_TEXT), ZERO_SENSOR_MAX_TIME_IN_SECONDS, getLangText(SEC_TEXT));
	OverlayMessage(getLangText(STATUS_TEXT), (char*)g_spareBuffer, 0);

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
				strcat((char*)g_spareBuffer, ".");
				OverlayMessage(getLangText(STATUS_TEXT), (char*)g_spareBuffer, 0);
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
					break;
				}
			}

			// Save current Zero Sensor comparison next time
			zeroCheckCompare = zeroCheck;
			zeroCheck = g_channelOffset;

			// Raise the Update Offset flag
			raiseSystemEventFlag(UPDATE_OFFSET_EVENT);
		}
	}

	StopADDataCollectionForCalibration();

	clearSystemEventFlag(UPDATE_OFFSET_EVENT);
}
