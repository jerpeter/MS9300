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
#define ANALOG_SENSOR_GEO_1		0x01
#define ANALOG_SENSOR_GEO_2		0x02
#define ANALOG_SENSOR_AOP_1		0x04
#define ANALOG_SENSOR_AOP_2		0x08

#define CAL_SAMPLE_COUNT_FIRST_TRANSITION_HIGH	95
#define CAL_SAMPLE_COUNT_SECOND_TRANSITION_LOW	85
#define CAL_SAMPLE_COUNT_THIRD_TRANSITION_HIGH	65
#define CAL_SAMPLE_COUNT_FOURTH_TRANSITION_OFF	55

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
	SEISMIC_GAIN_NORMAL,
	SEISMIC_GAIN_HIGH,
	ACOUSTIC_PATH_AOP,
	ACOUSTIC_PATH_A_WEIGHTED
};

enum {
	ANALOG_CUTOFF_FREQ_500,	// Filters ~500 Hz and above
	ANALOG_CUTOFF_FREQ_1K,	// Filters ~1 KHz and above
	ANALOG_CUTOFF_FREQ_2K,	// Filters ~2 kHz and above
	ANALOG_CUTOFF_FREQ_4K,	// Filters ~4 kHz and above
	ANALOG_CUTOFF_FREQ_8K,	// Filters ~8 kHz and above
	ANALOG_CUTOFF_FREQ_16K	// Filters ~16 kHz and above
};

enum {
	FOUR_AD_CHANNELS_WITH_READBACK_WITH_TEMP,
	FOUR_AD_CHANNELS_NO_READBACK_WITH_TEMP,
	FOUR_AD_CHANNELS_NO_READBACK_NO_TEMP,
	THREE_ACC_CHANNELS_NO_AIR
};

enum {
	UNIT_CONFIG_CHANNEL_VERIFICATION = 1,
	OVERRIDE_ENABLE_CHANNEL_VERIFICATION
};

enum {
	CAL_MUX_SELECT_SENSOR_GROUP_A = 1, 	// Geo1 + AOP1
	CAL_MUX_SELECT_SENSOR_GROUP_B 		// Geo2 + AOP2
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
uint8_t GetAnalogConfigReadback(void);
void ReadAnalogData(SAMPLE_DATA_STRUCT* dataPtr);
void AnalogControlInit(void);
void SetAnalogCutoffFrequency(uint8 freq);
void SetSeismicGainSelect(uint8 seismicGain);
void SetSeismicGainSelectSensor(uint8 sensorSelection, uint8 seismicGain);
void SetAcousticPathSelect(uint8 acousticGain);
void SetAcousticPathSelectSensor(uint8 sensorSelection, uint8 acousticGain);
void SetCalSignalEnable(uint8 enable);
void SetCalSignal(uint8 state);
void GenerateCalSignal(void);
void GetChannelOffsets(uint32 sampleRate);
void ZeroSensors(void);
void ZeroingSensorCalibration(void);
void UpdateChannelOffsetsForTempChange(void);
void AdSetCalSignalLow(void);
void AdSetCalSignalHigh(void);
void AdSetCalSignalOff(void);
void SetupADChannelConfig(uint32 sampleRate, uint8 channelVerification);
void DisableSensorBlocks(void);
void WaitAnalogPower5vGood(void);

#endif //_ANALOG_H_

///============================================================================
///----------------------------------------------------------------------------
///	External ADC - AD4695
///----------------------------------------------------------------------------
///============================================================================
/*
 * ad4695.h
 *
 *  Created on: May 5, 2022
 *      Author: emre.ozdemir
 */

#ifndef INC_AD4695_H_
#define INC_AD4695_H_

#define AD4695_REG_SPI_CONFIG_A		0x000 // R/W
#define AD4695_REG_SPI_CONFIG_B		0x001 // R/W
#define AD4695_REG_DEVICE_TYPE		0x003 // R  read the 0x7 value(fixed)
#define AD4695_REG_SCRATCH_PAD		0x00A // R/W to test the SPI communications
#define AD4695_REG_VENDOR_L			0x00C // R just an ID 0x0456
#define AD4695_REG_VENDOR_H			0x00D // R just an ID
#define AD4695_REG_LOOP_MODE		0x00E // R/W
#define AD4695_REG_SPI_CONFIG_C		0x010 // R/W
#define AD4695_REG_SPI_STATUS		0x011 // R/W
#define AD4695_REG_STATUS			0x014 // R
#define AD4695_REG_ALERT_STATUS1	0x015 // R
#define AD4695_REG_ALERT_STATUS2	0x016 // R
#define AD4695_REG_ALERT_STATUS3	0x017 // R
#define AD4695_REG_ALERT_STATUS4	0x018 // R
#define AD4695_REG_CLAMP_STATUS1	0x01A // R
#define AD4695_REG_CLAMP_STATUS2	0x01B // R
#define AD4695_REG_SETUP			0x020 // R/W
#define AD4695_REG_REF_CTRL			0x021 // R/W
#define AD4695_REG_SEQ_CTRL			0x022 // R/W
#define AD4695_REG_AC_CTRL			0x023 // R/W
#define AD4695_REG_STD_SEQ_CONFIG	0x024 // R/W
#define AD4695_REG_GPIO_CTRL		0x026 // R/W
#define AD4695_REG_GP_MODE			0x027 // R/W
#define AD4695_REG_GPIO_STATE		0x028 // R/W
#define AD4695_REG_TEMP_CTRL		0x029 // R/W
#define AD4695_REG_CONFIG_IN(x)		((x & 0x0F) | 0x30)
#define AD4695_REG_UPPER_IN(x)		((x & 0x5E) | 0x40)
#define AD4695_REG_LOWER_IN(x)		((x & 0x7E) | 0x60)
#define AD4695_REG_HYST_IN(x)		((x & 0x9E) | 0x80)
#define AD4695_REG_OFFSET_IN(x)		((x & 0xBE) | 0xA0)
#define AD4695_REG_GAIN_IN(x)		((x & 0xDE) | 0xC0)
#define AD4695_REG_AS_SLOT(x)		((x & 0x7F) | 0x100)

#define AD4695_REG_REF_CTRL_MASK			(0x07 << 2)
#define AD4695_REG_REF_CTRL_EN(x)			((x & 0x07) << 2)
/* 5-bit SDI Conversion Mode Commands */
#define AD4695_CMD_REG_CONFIG_MODE			(0x0A << 3)
#define AD4695_CMD_SEL_TEMP_SNSOR_CH		(0x0F << 3)
#define AD4695_CMD_CONFIG_CH_SEL(x)			((0x10 | (0x0F & x)) << 3)

/* AD4695_REG_SETUP */
#define AD4695_SETUP_CYC_CTRL_MASK			(0x01 << 1)
#define AD4695_SETUP_CYC_CTRL_EN(x)			((x & 0x01) << 1)
#define AD4695_SETUP_IF_MODE_MASK			(0x01 << 2)
#define AD4695_SETUP_IF_MODE_EN(x)			((x & 0x01) << 2)
#define AD4695_SETUP_LDO_ENABLE_MASK		(0x01 << 4)
#define AD4695_SETUP_LDO_ENABLE_EN(x)		((x & 0x01) << 4)
#define AD4695_SETUP_STATUS_ENABLE_MASK		(0x01 << 5)
#define AD4695_SETUP_STATUS_ENABLE_EN(x)	((x & 0x01) << 5)
#define AD4695_SETUP_SDO_STATE_MASK			(0x01 << 6)
#define AD4695_SETUP_SDO_STATE_EN(x)		((x & 0x01) << 6)
#define AD4695_SETUP_ALERT_MODE_MASK		(0x01 << 7)
#define AD4695_SETUP_ALERT_MODE_EN(x)		((x & 0x01) << 7)

/* AD4695_REG_GPIO_CTRL */
#define AD4695_GPIO_CTRL_GPO0_EN_MASK		(0x01 << 0)
#define AD4695_GPIO_CTRL_GPO0_EN_EN(x)		((x & 0x01) << 0)

/* AD4695_REG_GP_MODE */
#define AD4695_GP_MODE_BUSY_GP_EN_MASK		(0x01 << 1)
#define AD4695_GP_MODE_BUSY_GP_EN(x)		((x & 0x01) << 1)
#define AD4695_GP_MODE_SDO_MODE_MASK		(0x03 << 2)
#define AD4695_GP_MODE_SDO_MODE_SEL(x)		((x & 0x03) << 2)
#define AD4695_GP_MODE_BUSY_GP_SEL_MASK		(0x01 << 4)
#define AD4695_GP_MODE_BUSY_GP_SEL(x)		((x & 0x01) << 4)

/* AD4695_REG_SEQ_CTRL */
#define AD4695_SEQ_CTRL_STD_SEQ_EN_MASK		(0x01 << 7)
#define AD4695_SEQ_CTRL_STD_SEQ_EN(x)		((x & 0x01) << 7)
#define AD4695_SEQ_CTRL_NUM_SLOTS_AS_MASK	(0x7F << 0)
#define AD4695_SEQ_CTRL_NUM_SLOTS_AS(x)		((x & 0x7F) << 0)

/* AD4695_REG_TEMP_CTRL */
#define AD4695_REG_TEMP_CTRL_TEMP_EN_MASK	(0x01 << 0)
#define AD4695_REG_TEMP_CTRL_TEMP_EN(x)		((x & 0x01) << 0)

/* AD4695_REG_AS_SLOT */
#define AD4695_REG_AS_SLOT_INX(x)			((x & 0x0F) << 0)

/* AD4695_REG_SPI_CONFIG_C */
#define AD4695_REG_SPI_CONFIG_C_MB_STRICT_MASK		(0x01 << 5)
#define AD4695_REG_SPI_CONFIG_C_MB_STRICT(x)		((x & 0x01) << 5)

/* AD4695_REG_CONFIG_INn */
#define AD4695_REG_CONFIG_IN_OSR_MASK		(0x03 << 0)
#define AD4695_REG_CONFIG_IN_OSR(x)			((x & 0x03) << 0)
#define AD4695_REG_CONFIG_IN_HIZ_EN_MASK	(0x01 << 3)
#define AD4695_REG_CONFIG_IN_HIZ_EN(x)		((x & 0x01) << 3)
#define AD4695_REG_CONFIG_IN_PAIR_MASK		(0x03 << 4)
#define AD4695_REG_CONFIG_IN_PAIR(x)		((x & 0x03) << 4)
#define AD4695_REG_CONFIG_IN_MODE_MASK		(0x01 << 6)
#define AD4695_REG_CONFIG_IN_MODE(x)		((x & 0x01) << 6)
#define AD4695_REG_CONFIG_IN_TD_EN_MASK		(0x01 << 7)
#define AD4695_REG_CONFIG_IN_TD_EN(x)		((x & 0x01) << 7)
#define AD4695_CHANNEL(x)					(BIT(x) & 0xFFFF)
#define AD4695_CHANNEL_NO					16
#define AD4695_SLOTS_NO						0x80
#define AD4695_CHANNEL_TEMP					16

#define ANALOG_GEO_1	0x07
#define ANALOG_AOP_1	0x08
#define ANALOG_GEO_2	0x70
#define ANALOG_AOP_2	0x80

#define AD4695_SINGLE_SDO_MODE	0x00
#define AD4695_DUAL_SDO_MODE	0x01

#define AD4695_CHANNEL_DATA_READ_SIZE				2
#define AD4695_CHANNEL_DATA_READ_SIZE_PLUS_STATUS	3

#if 0
typedef enum {
	FALSE = 0,
	TRUE = 1
} Bool;
#endif

enum ad4695_channel_sequencing {
	AD4695_single_cycle = 0,
	AD4695_two_cycle = 1,
	AD4695_standard_seq = 2,
	AD4695_advanced_seq = 3,
};

enum ad4695_ref {
	 R2V4_2V7 = 0,
	 R2V7_3V2 = 1,
	 R3V2_3V7 = 2,
	 R3V7_4V5 = 3,
	 R4V5_R5V1 = 4,
};

enum ad4695_busy_gpio_sel {
	AD4695_busy_gp0 = 0,
	AD4695_busy_gp3 = 1,
};

enum ad4695_reg_access {
	AD4695_BYTE_ACCESS = 0,
	AD4695_WORD_ACCESS = 1,
};

enum ad4695_osr_ratios { /* oversampling */

	AD4695_OSR_1 = 0,
	AD4695_OSR_4 = 1,
	AD4695_OSR_16 = 2,
	AD4695_OSR_64 = 3,
};

void AD4695_Test(void);
void AD4695_SpiWriteRegister(uint16_t reg_addr,uint8_t reg_data);
void AD4695_SpiReadRegister(uint16_t reg_addr,uint8_t *reg_data);
void AD4695_SpiReadRegisterWithMask(uint16_t reg_addr, uint8_t mask, uint8_t *data);
uint8_t AD4695_SpiWriteRegisterWithMask(uint16_t reg_addr, uint8_t mask, uint8_t data);
void AD4695_SetRegisterAccessMode( enum ad4695_reg_access access);
void AD4695_Init();
void AD4695_SetBusyState(void);
void AD4695_SetStandardSequenceModeChannelOSR(enum ad4695_osr_ratios ratio);
void AD4695_SetStandardSequenceModeChannelInputConfig(uint8_t regData);
void AD4695_EnterConversionMode(uint8_t enableStatus);
void AD4695_ExitConversionMode(void);
void AD4695_SetStandardMode(void);
void AD4695_SetStandardSequenceActiveChannels(uint8_t channels);
uint8_t AD4695_GetStandardSequenceActiveChannels(void);
void AD4695_SetTemperatureSensorEnable(uint8_t mode);
void AD4695_DisableInternalLDO(void);
int AD4695_TemperatureConversionCtoF(uint16_t tempCode);

#endif /* INC_AD4695_H_ */
