///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

#ifndef _SENSOR_H_
#define _SENSOR_H_

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include "Typedefs.h"
#include "Common.h"
#include "OldUart.h"
#include "Crc.h"
#include "Time.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
typedef enum {
	SEISMIC_SENSOR = 1,
	ACOUSTIC_SENSOR,
	SEISMIC_SENSOR_2,
	ACOUSTIC_SENSOR_2,
	TOTAL_SENSOR_TYPES
} SMART_SENSOR_TYPE;

typedef enum {
	INFO_ON_INIT = 1,
	INFO_ON_CHECK
} SMART_SENSOR_INFO;

typedef enum {
	UNIT_CAL_DATE = 1,
	SEISMIC_SMART_SENSOR_CAL_DATE,
	ACOUSTIC_SMART_SENSOR_CAL_DATE
} CAL_DATE_SOURCE;

#define SENSOR_WARMUP_PERIOD	(60) // Delay in seconds
//#define SENSOR_WARMUP_DELAY		(SENSOR_WARMUP_PERIOD * TICKS_PER_SEC) // Delay in seconds * 2 (ticks/sec)

#define SMART_SENSOR_OVERLAY_KEY	0xA8A8

#define SENSOR_SERIAL_NUMBER_SIZE	6
#define SENSOR_CAL_DATE_SIZE		4

// 1-Wire DS2431 ROM Function Commands
#define DS2431_READ_ROM				0x33 // #1 of 7
#define DS2431_MATCH_ROM			0x55 // #2 of 7
#define DS2431_SEARCH_ROM			0xF0 // #3 of 7
#define DS2431_SKIP_ROM				0xCC // #4 of 7
#define DS2431_RESUME				0xA5 // #5 of 7
#define DS2431_OD_SKIP_ROM			0x3C // #6 of 7
#define DS2431_OD_MATCH_ROM			0x69 // #7 of 7

#define DS2431_READ_SCRATCHPAD		0xAA
#define DS2431_WRITE_SCRATCHPAD		0x0F
#define DS2431_COPY_SCRATCHPAD		0x55
#define DS2431_READ_MEMORY			0xF0

///----------------------------------------------------------------------------
///	Device Information
///----------------------------------------------------------------------------
/*
1-Wire ROM FUNCTION COMMANDS
READ ROM - 64-BIT REG. #, RC-FLAG
MATCH ROM - 64-BIT REG. #, RC-FLAG
SEARCH ROM - 64-BIT REG. #, RC-FLAG
SKIP ROM - RC-FLAG
RESUME - RC-FLAG
OVERDRIVE-SKIP ROM - RC-FLAG, OD-FLAG
OVERDRIVE-MATCH ROM - 64-BIT REG. #, RC-FLAG, OD-FLAG

DS2431-SPECIFIC MEMORY FUNCTION COMMANDS
WRITE SCRATCHPAD - 64-BIT SCRATCHPAD, FLAGS
READ SCRATCHPAD - 64-BIT SCRATCHPAD
COPY SCRATCHPAD - DATA MEMORY, REGISTER PAGE
READ MEMORY - DATA MEMORY, REGISTER PAGE
*/

typedef enum {
	NTSI_2_56in_5v = 0x00,
	NTSI_5_12in_5v,
	NTSI_10_24in_5v,
	NTSI_20_48in_5v,
	NTSM_65_535in_5v = 0x80,
	NTSM_131_072in_5v,
	NTSM_262_144in_5v,
	NTSM_524_288in_5v
} SMART_SENSOR_SENSOR_TYPE;

typedef struct {
	CALIBRATION_DATE_STRUCT calDate;
	uint8 calFacility;
	uint8 calInstrument;
	uint16 calCrc;
} CALIBRATION_DATA_SET_STRUCT;

typedef struct {
	uint16 version;
	uint16 dataLength;
	uint32 crc;
	uint8 serialNumber[SENSOR_SERIAL_NUMBER_SIZE];
	uint8 sensorType;
	uint8 calCount;
	uint8 reserved[16];

	union {
		struct {
			CALIBRATION_DATA_SET_STRUCT currentCal;
			CALIBRATION_DATA_SET_STRUCT calHistory[7];
			uint8 userDefined[32];
		};
		struct {
			CALIBRATION_DATA_SET_STRUCT calDataSet[8];
			uint8 userDefinedData[32];
		};
		struct {
			CALIBRATION_DATA_SET_STRUCT calRawData[12];
		};
	};
} SMART_SENSOR_STRUCT;

typedef struct {
	uint8 family;
	uint8 serialNumber[SENSOR_SERIAL_NUMBER_SIZE];
	uint8 crc;
} SMART_SENSOR_ROM;

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------
void OneWireInit(void);
void OneWireResetAndConfigure(void);
uint8 OneWireReset(void);
void OneWireWriteByte(uint8 dataByte);
uint8 OneWireReadByte(void);
void OneWireTest(void);
void OneWireFunctions(void);
uint8 OneWireReadROM(SMART_SENSOR_ROM* romData);
uint8 OneWireReadMemory(uint16 address, uint8 length, uint8* data);
uint8 OneWireWriteScratchpad(uint16 address, uint8 length, uint8* data);
uint8 OneWireReadScratchpad(uint16 address, uint8 length, uint8* data);
uint8 OneWireCopyScratchpad(void);
uint8 OneWireWriteAppRegister(uint16 address, uint8 length, uint8* data);
uint8 OneWireReadStatusRegister(uint8* data);
uint8 OneWireReadAppRegister(uint16 address, uint8 length, uint8* data);
uint8 OneWireCopyAndLockAppRegister(void);
void SmartSensorDebug(void);
void SmartSensorTest(void);
void SmartSensorReadRomAndMemory(SMART_SENSOR_TYPE sensor);
void UpdateUnitSensorsWithSmartSensorTypes(void);
void DisplaySmartSensorInfo(SMART_SENSOR_INFO situation);
void DisplaySmartSensorSerialNumber(SMART_SENSOR_TYPE sensor);
uint8 CheckIfBothSmartSensorsPresent(void);
uint8 CheckIfNoSmartSensorsPresent(void);
uint8 CheckIfAcousticSensorTypeValid(uint8 airSensorType);
void UpdateWorkingCalibrationDate(void);
uint8_t SmartSensorMuxSelectAndDriverEnable(SMART_SENSOR_TYPE sensor);
void SmartSensorDisableMuxAndDriver(void);

///----------------------------------------------------------------------------
///	DS2484 Prototypes
///----------------------------------------------------------------------------
uint8_t ds2484_w1_reset_bus(void);
void ds2484_w1_write_byte(uint8_t byte);
uint8_t ds2484_w1_read_byte(void);

#endif //_SENSOR_H_
