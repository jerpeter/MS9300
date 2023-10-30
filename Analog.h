///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

#ifndef _ANALOG_H_
#define _ANALOG_H_

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include "Typedefs.h"
#include "Summary.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define ANALOG_R_CHANNEL_SELECT		0x02
#define ANALOG_V_CHANNEL_SELECT		0x00
#define ANALOG_T_CHANNEL_SELECT		0x06
#define ANALOG_A_CHANNEL_SELECT		0x04
#define ANALOG_CONTROL_DATA			0x01
#define ANALOG_CONTROL_SHIFT		0x02
#define ANALOG_CONTROL_STORAGE		0x04

#define SAMPLE_RATE_1K_PIT_DIVIDER		2
#define SAMPLE_RATE_2K_PIT_DIVIDER		4
#define SAMPLE_RATE_4K_PIT_DIVIDER		8
#define SAMPLE_RATE_8K_PIT_DIVIDER		16
#define SAMPLE_RATE_16K_PIT_DIVIDER		32
#define SAMPLE_RATE_32K_PIT_DIVIDER		64
#define SAMPLE_RATE_64K_PIT_DIVIDER		128
#define SAMPLE_RATE_PIT_MODULUS			62500

#define CAL_SAMPLE_COUNT_FIRST_TRANSITION_HIGH	95
#define CAL_SAMPLE_COUNT_SECOND_TRANSITION_LOW	85
#define CAL_SAMPLE_COUNT_THIRD_TRANSITION_HIGH	65
#define CAL_SAMPLE_COUNT_FOURTH_TRANSITION_OFF	55

#define AD_SPI_NPCS			0
#define AD_CTL_SPI_NPCS		3

#define GAIN_SELECT_x2	0x01
#define GAIN_SELECT_x4	0x00

#define CONSECUTIVE_TRIGGERS_THRESHOLD 2
#define CONSEC_EVENTS_WITHOUT_CAL_THRESHOLD 2 // Treating event + event as 1 consecutive, event + event + event as 2 consecutive
#define PENDING	2 // Anything above 1

#define AD_TEMP_COUNT_FOR_ADJUSTMENT	32		// Temperature adjustment range (about 10 degrees F)

#define	MAX_TEMPERATURE_JUMP_PER_SAMPLE	4		// Check for A/D temperature sample that is bogus

#define SENSOR_WARMUP_DELAY_IN_SECONDS	90		// Was 60 seconds, but not long enough for most sensors to stabilize from cold start, can take upwards of 120 seconds (this is where Zero Sensors comes in)
#define ZERO_SENSOR_MAX_TIME_IN_SECONDS	90		// Maximum time allowed for zero sensors before monitoring

#define AD_NORMALIZED_NOISE_THRESHOLD_START	7
#define AD_NORMALIZED_NOISE_THRESHOLD		3
#define AD_PEAK_CHANGE_NOISE_THRESHOLD		31

enum {
	DEFAULT_CAL_BUFFER_INDEX = 0,
	ONCE_DELAYED_CAL_BUFFER_INDEX = 1,
	TWICE_DELAYED_CAL_BUFFER_INDEX = 2
};

enum {
	SEISMIC_GAIN_LOW,
	SEISMIC_GAIN_HIGH,
	ACOUSTIC_GAIN_NORMAL,
	ACOUSTIC_GAIN_A_WEIGHTED
};

enum {
	ANALOG_CUTOFF_FREQ_LOW,	// Filters ~500 HZ and above 
	ANALOG_CUTOFF_FREQ_1,	// Filters ~1000 HZ and above 
	ANALOG_CUTOFF_FREQ_2,	// Filters ~2000 HZ and above 
	ANALOG_CUTOFF_FREQ_3,	// Filters ~4000 HZ and above 
	ANALOG_CUTOFF_FREQ_4	// Filters ~14000 HZ and above 
};

enum {
	FOUR_AD_CHANNELS_WITH_READBACK_WITH_TEMP,
	FOUR_AD_CHANNELS_NO_READBACK_WITH_TEMP,
	FOUR_AD_CHANNELS_NO_READBACK_NO_TEMP
};

enum {
	CHANNELS_R_AND_V_SCHEMATIC = 1,
	CHANNELS_R_AND_V_SWAPPED
};

enum {
	UNIT_CONFIG_CHANNEL_VERIFICATION = 1,
	OVERRIDE_ENABLE_CHANNEL_VERIFICATION
};

typedef union
{
	struct
	{
		bitfield calSignalEnable:			1;
		bitfield calSignal:					1;
		bitfield unused:					1;
		bitfield cutoffFreqSelectEnable:	1;
		bitfield cutoffFreqSelectHi:		1;
		bitfield cutoffFreqSelectLow:		1;
		bitfield acousticGainSelect:		1;
		bitfield seismicGainSelect:			1;
	} bit; 

	uint8 reg;
} ANALOG_CONTROL_STRUCT;

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------
void GetAnalogConfigReadback(void);
void ReadAnalogData(SAMPLE_DATA_STRUCT* dataPtr);
void InitAnalogControl(void);
void WriteAnalogControl(uint16 data);
void SetAnalogCutoffFrequency(uint8 freq);
void SetSeismicGainSelect(uint8 seismicGain);
void SetAcousticGainSelect(uint8 acousticGain);
void SetCalSignalEnable(uint8 enable);
void SetCalSignal(uint8 data);
void GenerateCalSignal(void);
void GetChannelOffsets(uint32 sampleRate);
void ZeroSensors(void);
void ZeroingSensorCalibration(void);
void UpdateChannelOffsetsForTempChange(void);
void AdSetCalSignalLow(void);
void AdSetCalSignalHigh(void);
void AdSetCalSignalOff(void);
void SetupADChannelConfig(uint32 sampleRate, uint8 channelVerification);

#endif //_ANALOG_H_
