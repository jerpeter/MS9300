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
#include <math.h>
#include "Typedefs.h"
#include "Board.h"
#include "Sensor.h"
#include "OldUart.h"
#include "Crc.h"
#include "string.h"
#include "PowerManagement.h"

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

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void OneWireInit(void)
{
#if 0 /* old hw */
	gpio_enable_gpio_pin(SMART_SENSOR_DATA);
#endif
	PowerControl(SEISMIC_SENSOR_DATA_CONTROL, OFF);
	PowerControl(ACOUSTIC_SENSOR_DATA_CONTROL, OFF);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 OneWireReset(SMART_SENSOR_TYPE sensor)
{
	//    500  30 110 (us)
	// __       _     ________
	//   |     | |   |
	//   |     | |   |
	//    -----   ---

	uint8 presenceDetect = NO;

	// Set data direction to output to drive a 0
	if (sensor == SEISMIC_SENSOR) { PowerControl(SEISMIC_SENSOR_DATA_CONTROL, ON); }
	else /* ACOUSTIC_SENSOR */ { PowerControl(ACOUSTIC_SENSOR_DATA_CONTROL, ON); }

	// Hold low for 500us
	//SoftUsecWait(500); // Looks like 540
	//SoftUsecWait(460); // Looks like 480
	SoftUsecWait(480);

	// Release line (allow pullup to take affect)
	if (sensor == SEISMIC_SENSOR) { PowerControl(SEISMIC_SENSOR_DATA_CONTROL, OFF); }
	else /* ACOUSTIC_SENSOR */ { PowerControl(ACOUSTIC_SENSOR_DATA_CONTROL, OFF); }

	// Wait 30us + 50us (80us total)
	//SoftUsecWait(80);
	//SoftUsecWait(74);
	SoftUsecWait(77);

	if (READ_SMART_SENSOR_ONE_WIRE_STATE() == LOW)
	{
		presenceDetect = YES;
	}

	// Wait 100us make sure device is not driving the line
	//SoftUsecWait(100);
	//SoftUsecWait(93);
	SoftUsecWait(97);

	return (presenceDetect);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void OneWireWriteByte(SMART_SENSOR_TYPE sensor, uint8 data)
{
	uint8 i;

	// Loop through all the bits starting with LSB
	for (i = 0; i <= 7; i++)
	{
		// Set data direction to output to drive a 0
		if (sensor == SEISMIC_SENSOR) { PowerControl(SEISMIC_SENSOR_DATA_CONTROL, ON); }
		else /* ACOUSTIC_SENSOR */ { PowerControl(ACOUSTIC_SENSOR_DATA_CONTROL, ON); }

		// Check if the bit is a 1
		if (data & 0x01)
		{
			// Hold low for 5us
			SoftUsecWait(5);

			// Release the line
			if (sensor == SEISMIC_SENSOR) { PowerControl(SEISMIC_SENSOR_DATA_CONTROL, OFF); }
			else /* ACOUSTIC_SENSOR */ { PowerControl(ACOUSTIC_SENSOR_DATA_CONTROL, OFF); }

			// Wait for 65us, recovery time
			//SoftUsecWait(65);
			//SoftUsecWait(60);
			SoftUsecWait(63);
		}
		else
		{
			// Hold low for 65us
			//SoftUsecWait(65);
			//SoftUsecWait(60);
			SoftUsecWait(63);

			// Release the line
			if (sensor == SEISMIC_SENSOR) { PowerControl(SEISMIC_SENSOR_DATA_CONTROL, OFF); }
			else /* ACOUSTIC_SENSOR */ { PowerControl(ACOUSTIC_SENSOR_DATA_CONTROL, OFF); }

			// Wait for 5us, recovery time
			SoftUsecWait(5);
		}

		// Shift the data over 1 bit
		data >>= 1;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 OneWireReadByte(SMART_SENSOR_TYPE sensor)
{
	uint8 data = 0;
	uint8 i;

	// Loop through all the bits starting with LSB
	for (i = 0; i <= 7; i++)
	{
		// Set data direction to output to drive a 0
		if (sensor == SEISMIC_SENSOR) { PowerControl(SEISMIC_SENSOR_DATA_CONTROL, ON); }
		else /* ACOUSTIC_SENSOR */ { PowerControl(ACOUSTIC_SENSOR_DATA_CONTROL, ON); }

		// Hold low for 5us
		SoftUsecWait(5);

		// Release the line
		if (sensor == SEISMIC_SENSOR) { PowerControl(SEISMIC_SENSOR_DATA_CONTROL, OFF); }
		else /* ACOUSTIC_SENSOR */ { PowerControl(ACOUSTIC_SENSOR_DATA_CONTROL, OFF); }

		// Wait for 5us
		SoftUsecWait(5);

		// Shift the data over 1 bit
		data >>= 1;

		// Check if the data bit is a 1
		if (READ_SMART_SENSOR_ONE_WIRE_STATE())
		{
			// Or in a 1
			data |= 0x80;
		}
		else
		{
			// And in a zero
			data &= 0x7f;
		}

		// Hold for 60us, recovery time
		//SoftUsecWait(60);
		//SoftUsecWait(56);
		SoftUsecWait(58);
	}

	return (data);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void OneWireTest(SMART_SENSOR_TYPE sensor)
{
	uint8 romData[8];
	uint8 i = 0;
	uint8 crc = 0;

	if (OneWireReset(sensor) == YES)
	{
		OneWireWriteByte(sensor, DS2431_READ_ROM);

		for (i = 0; i < 8; i++)
		{
#if 0 /* old hw */
			romData[i] = OneWireReadByte(sensor);
#endif
		}

		crc = CalcCrc8(&romData[0], 7, 0x00);

		debugRaw("\nOne Wire Rom Data: ");

		for (i = 0; i < 8; i++)
		{
			debugRaw("0x%x ", romData[i]);
		}

		if (crc == romData[7])
		{
			debugRaw("(CRC: %x, success)\r\n", crc);
			OneWireFunctions(sensor);
		}
		else
		{
			debugRaw("(CRC: %x, fail)\r\n", crc);
		}
	}
	else
	{
		debugRaw("\nOne Wire device not found\r\n");
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void OneWireFunctions(SMART_SENSOR_TYPE sensor)
{
	uint8 i = 0;
	uint16 crc16 = 0;
	uint16 crc16seed0 = 0;
	uint16 crc16invert = 0;
	uint16 returnCrc16 = 0;
	uint8 data;
	uint8 dataAdjust = 2;

	// Read Memory (0xF0), Address: 0x00 -> 0x1F (wrap)
	if (OneWireReset(sensor) == YES)
	{
		debugRaw("Read Memory\r\n");

		// Skip ROM
		OneWireWriteByte(sensor, DS2431_SKIP_ROM);

		// Read Memory
		OneWireWriteByte(sensor, DS2431_READ_MEMORY);

		// Address (Lower)
		OneWireWriteByte(sensor, 0x00);

		// Address (Upper)
		OneWireWriteByte(sensor, 0x00);

		debugRaw("  Data: ");

		// Data
		for (i = 0; i < 128; i++)
		{
#if 0 /* old hw */
			debugRaw("%02x ", OneWireReadByte(sensor));
#endif
			if (((i + 1) % 32) == 0)
			{
				debugRaw("\r\n\t");
			}
		}

		debugRaw("\r\n");

		OneWireReset(sensor);
	}
	else return;

	// Write Scratchpad (0x0F), Address: 0x00 -> 0x07
	if (OneWireReset(sensor) == YES)
	{
		debugRaw("Write Scratchpad\r\n");

		// Skip ROM
		OneWireWriteByte(sensor, DS2431_SKIP_ROM);

		// Write Scratchpad
		OneWireWriteByte(sensor, DS2431_WRITE_SCRATCHPAD);
		data = DS2431_WRITE_SCRATCHPAD; crc16 = CalcCrc16(&data, 1, 0xFFFF); crc16seed0 = CalcCrc16(&data, 1, 0); crc16invert = ~CalcCrc16(&data, 1, ~0);

		// Address (Lower)
		OneWireWriteByte(sensor, 0x00);
		data = 0x00; crc16 = CalcCrc16(&data, 1, crc16); crc16seed0 = CalcCrc16(&data, 1, crc16seed0); crc16invert = ~CalcCrc16(&data, 1, ~crc16invert);

		// Address (Upper)
		OneWireWriteByte(sensor, 0x00);
		data = 0x00; crc16 = CalcCrc16(&data, 1, crc16); crc16seed0 = CalcCrc16(&data, 1, crc16seed0); crc16invert = ~CalcCrc16(&data, 1, ~crc16invert);

		debugRaw("  Data: ");

		// Data
		for (i = 0; i < 8; i++)
		{
			OneWireWriteByte(sensor, (uint8)((i + 1) * dataAdjust));
			data = (uint8)((i + 1) * dataAdjust); crc16 = CalcCrc16(&data, 1, crc16); crc16seed0 = CalcCrc16(&data, 1, crc16seed0); crc16invert = ~CalcCrc16(&data, 1, ~crc16invert);
			debugRaw("%02x ", (uint8)((i + 1) * dataAdjust));
		}

		// Read CRC16 (only for full Scratchpad write)
#if 0 /* old hw */
		returnCrc16 = OneWireReadByte(sensor);
		returnCrc16 |= (OneWireReadByte(sensor) << 8);
#endif
		if (crc16 == returnCrc16) { debugRaw("(CRC16 match seed 0xFFFF)"); }
		else if (crc16seed0 == returnCrc16) { debugRaw("(CRC16 match seed 0)"); }
		else if (crc16invert == returnCrc16) { debugRaw("(CRC16 match invert)"); }
		else { debugRaw("(CRC16 0xFFFF: 0x%04x, CRC16 0: 0x%04x, CRC16 Invert: 0x%04x, Return CRC16: 0x%04x / 0x%04x)", crc16, crc16seed0, crc16invert, returnCrc16, ~returnCrc16); }

		debugRaw("\r\n");

		OneWireReset(sensor);
	}
	else return;

	// Read Scratchpad (0xAA), Address: 0x00 -> 0x1F (wrap)
	if (OneWireReset(sensor) == YES)
	{
		debugRaw("Read Scratchpad\r\n");

		// Skip ROM
		OneWireWriteByte(sensor, DS2431_SKIP_ROM);

		// Read Scratchpad
		OneWireWriteByte(sensor, DS2431_READ_SCRATCHPAD);

		// Address (Lower)
		OneWireWriteByte(sensor, 0x00);

		// Address (Upper)
		OneWireWriteByte(sensor, 0x00);

		// ES
		OneWireWriteByte(sensor, 0x00);

		debugRaw("  Data: ");

		// Data
		for (i = 0; i < 8; i++)
		{
#if 0 /* old hw */
			debugRaw("%02x ", OneWireReadByte(sensor));
#endif
		}

		debugRaw("\r\n");

		OneWireReset(sensor);
	}
	else return;

	// Copy Scratchpad (0x55), Validation key: 0xA5, Data line held for 10ms
	if (OneWireReset(sensor) == YES)
	{
		debugRaw("Copy Scratchpad\r\n");

		// Skip ROM
		OneWireWriteByte(sensor, DS2431_SKIP_ROM);

		// Copy Scratchpad
		OneWireWriteByte(sensor, DS2431_COPY_SCRATCHPAD);

		// Validation Key
		OneWireWriteByte(sensor, 0xA5);

		SoftUsecWait(10 * SOFT_MSECS);

		OneWireReset(sensor);
	}
	else return;

	// Write Application Register (0x99), Address: 0x00 -> 0x07 (wrap)
	if (OneWireReset(sensor) == YES)
	{
		debugRaw("Write App Register\r\n");

		// Skip ROM
		OneWireWriteByte(sensor, DS2431_SKIP_ROM);

		// Write App Register
		OneWireWriteByte(sensor, 0x99);

		// Address
		OneWireWriteByte(sensor, 0x00);

		// Data
		for (i = 0; i < 8; i++)
		{
			OneWireWriteByte(sensor, (uint8)((i + 1) * 4));
		}

		OneWireReset(sensor);
	}
	else return;

	// Read Status Register (0x66), Validation key: 0x00
	if (OneWireReset(sensor) == YES)
	{
		debugRaw("Read Status Register\r\n");

		// Skip ROM
		OneWireWriteByte(sensor, DS2431_SKIP_ROM);

		// Read Status Register
		OneWireWriteByte(sensor, 0x66);

		// Validation key
		OneWireWriteByte(sensor, 0x00);

		debugRaw("  Data: ");

		// Data
#if 0 /* old hw */
		debugRaw("%02x\r\n", OneWireReadByte(sensor));
#endif
		OneWireReset(sensor);
	}
	else return;

	// Read Application Register (0xC3), Address: 0x00 -> 0x07 (wrap)
	if (OneWireReset(sensor) == YES)
	{
		debugRaw("Read App Register\r\n");

		// Skip ROM
		OneWireWriteByte(sensor, DS2431_SKIP_ROM);

		// Read App Register
		OneWireWriteByte(sensor, 0xC3);

		// Address
		OneWireWriteByte(sensor, 0x00);

		debugRaw("  Data: ");

		// Data
		for (i = 0; i < 8; i++)
		{
#if 0 /* old hw */
			debugRaw("%02x ", OneWireReadByte(sensor));
#endif
		}

		debugRaw("\r\n");

		OneWireReset(sensor);
	}
	else return;

	// Copy and Lock Application Register (0x5A), Validation key: 0xA5, Only executed once
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 OneWireReadROM(SMART_SENSOR_TYPE sensor, SMART_SENSOR_ROM* romData)
{
	uint8 status = FAILED;
	uint8 i = 0;
	uint8* romDataPtr = (uint8*)romData;
	uint8 crc;

	// Read ROM
	if (OneWireReset(sensor) == YES)
	{
		// Read ROM command
		OneWireWriteByte(sensor, DS2431_READ_ROM);

		for (i = 0; i < 8; i++)
		{
#if 0 /* old hw */
			romDataPtr[i] = OneWireReadByte(sensor);
#endif
		}

		crc = CalcCrc8(romDataPtr, 7, 0x00);

#if EXTENDED_DEBUG
		debugRaw("\nOne Wire Rom Data: ");

		for (i = 0; i < 8; i++)
		{
			debugRaw("0x%x ", romDataPtr[i]);
		}

		if (crc == romDataPtr[7])
		{
			status = PASSED;
			debugRaw("(CRC: %x, success)\r\n", crc);
		}
		else
		{
			debugRaw("(CRC: %x, fail)\r\n", crc);
		}
#else
		if (crc == romDataPtr[7])
		{
			status = PASSED;
		}
#endif

		OneWireReset(sensor);
	}

	return (status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 OneWireReadMemory(SMART_SENSOR_TYPE sensor, uint16 address, uint8 length, uint8* data)
{
	uint8 status = FAILED;
	uint8 i = 0;

	if ((data != NULL) && (address <= 0x1F) && (length <= 128))
	{
		// Read Memory (0xF0), Address: 0x00 -> 0x1F (wrap)
		if (OneWireReset(sensor) == YES)
		{
			// Skip ROM
			OneWireWriteByte(sensor, DS2431_SKIP_ROM);

			// Read Memory
			OneWireWriteByte(sensor, DS2431_READ_MEMORY);

			// Address (Lower)
			OneWireWriteByte(sensor, (address & 0xFF));

			// Address (Upper)
			OneWireWriteByte(sensor, ((address >> 8) & 0xFF));

			// Data
			for (i = 0; i < length; i++)
			{
#if 0 /* old hw */
				data[i] = OneWireReadByte(sensor);
#endif
			}

			OneWireReset(sensor);

			status = PASSED;
		}
	}

	return (status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 OneWireWriteScratchpad(SMART_SENSOR_TYPE sensor, uint16 address, uint8 length, uint8* data)
{
	uint8 status = FAILED;
	uint8 i = 0;

	if ((data != NULL) && (address <= 0x1F) && (length <= 8))
	{
		// Write Scratchpad (0x0F), Address: 0x00 -> 0x1F (wrap)
		if (OneWireReset(sensor) == YES)
		{
			// Skip ROM
			OneWireWriteByte(sensor, DS2431_SKIP_ROM);

			// Write Scratchpad
			OneWireWriteByte(sensor, DS2431_WRITE_SCRATCHPAD);

			// Address (Lower)
			OneWireWriteByte(sensor, (address & 0xFF));

			// Address (Upper)
			OneWireWriteByte(sensor, ((address >> 8) & 0xFF));

			// Data
			for (i = 0; i < length; i++)
			{
				OneWireWriteByte(sensor, data[i]);
			}

			OneWireReset(sensor);

			status = PASSED;
		}
	}

	return(status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 OneWireReadScratchpad(SMART_SENSOR_TYPE sensor, uint16 address, uint8 length, uint8* data)
{
	uint8 status = FAILED;
	uint8 i = 0;

	if ((data != NULL) && (address <= 0x1F) && (length <= 8))
	{
		// Read Scratchpad (0xAA), Address: 0x00 -> 0x1F (wrap)
		if (OneWireReset(sensor) == YES)
		{
			// Skip ROM
			OneWireWriteByte(sensor, DS2431_SKIP_ROM);

			// Read Scratchpad
			OneWireWriteByte(sensor, DS2431_READ_SCRATCHPAD);

			// Address (Lower)
			OneWireWriteByte(sensor, (address & 0xFF));

			// Address (Upper)
			OneWireWriteByte(sensor, ((address >> 8) & 0xFF));

			// ES
			OneWireWriteByte(sensor, 0x00);

			// Data
			for (i = 0; i < length; i++)
			{
#if 0 /* old hw */
				data[i] = OneWireReadByte(sensor);
#endif
			}

			OneWireReset(sensor);

			status = PASSED;
		}
	}

	return (status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 OneWireCopyScratchpad(SMART_SENSOR_TYPE sensor)
{
	uint8 status = FAILED;
	//uint8 i = 0;

	// Copy Scratchpad (0x55), Validation key: 0xA5, Data line held for 10ms
	if (OneWireReset(sensor) == YES)
	{
		// Skip ROM
		OneWireWriteByte(sensor, DS2431_SKIP_ROM);

		// Copy Scratchpad
		OneWireWriteByte(sensor, DS2431_COPY_SCRATCHPAD);

		// Validation Key
		OneWireWriteByte(sensor, 0xA5);

		SoftUsecWait(10 * SOFT_MSECS);

		OneWireReset(sensor);

		status = PASSED;
	}

	return (status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 OneWireWriteAppRegister(SMART_SENSOR_TYPE sensor, uint16 address, uint8 length, uint8* data)
{
	uint8 status = FAILED;
	uint8 i = 0;

	if ((data != NULL) && (address <= 0x07) && (length <= 8))
	{
		// Write Application Register (0x99), Address: 0x00 -> 0x07 (wrap)
		if (OneWireReset(sensor) == YES)
		{
			// Skip ROM
			OneWireWriteByte(sensor, DS2431_SKIP_ROM);

			// Write App Register
			OneWireWriteByte(sensor, 0x99);

			// Address
			OneWireWriteByte(sensor, address);

			// Data
			for (i = 0; i < 8; i++)
			{
				OneWireWriteByte(sensor, data[i]);
			}

			OneWireReset(sensor);

			status = PASSED;
		}
	}

	return (status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 OneWireReadStatusRegister(SMART_SENSOR_TYPE sensor, uint8* data)
{
	uint8 status = FAILED;
	//uint8 i = 0;

	if (data != NULL)
	{
		// Read Status Register (0x66), Validation key: 0x00
		if (OneWireReset(sensor) == YES)
		{
			// Skip ROM
			OneWireWriteByte(sensor, DS2431_SKIP_ROM);

			// Read Status Register
			OneWireWriteByte(sensor, 0x66);

			// Validation key
			OneWireWriteByte(sensor, 0x00);

#if 0 /* old hw */
			data[0] = OneWireReadByte(sensor);
#endif
			OneWireReset(sensor);

			status = PASSED;
		}
	}

	return (status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 OneWireReadAppRegister(SMART_SENSOR_TYPE sensor, uint16 address, uint8 length, uint8* data)
{
	uint8 status = FAILED;
	uint8 i = 0;

	if ((data != NULL) && (address <= 0x07) && (length <= 8))
	{
		// Read Application Register (0xC3), Address: 0x00 -> 0x07 (wrap)
		if (OneWireReset(sensor) == YES)
		{
			// Skip ROM
			OneWireWriteByte(sensor, DS2431_SKIP_ROM);

			// Read App Register
			OneWireWriteByte(sensor, 0xC3);

			// Address
			OneWireWriteByte(sensor, address);

			// Data
			for (i = 0; i < length; i++)
			{
#if 0 /* old hw */
				data[i] = OneWireReadByte(sensor);
#endif
			}

			OneWireReset(sensor);

			status = PASSED;
		}
	}

	return (status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 OneWireCopyAndLockAppRegister(SMART_SENSOR_TYPE sensor)
{
	uint8 status = FAILED;
	//uint8 i = 0;
	uint8 lockStatus = 0;

	if (OneWireReadStatusRegister(sensor, &lockStatus) == PASSED)
	{
		// Check if the App register is unlocked
		if (lockStatus == 0xFF)
		{
			// Copy and Lock Application Register (0x5A), Validation key: 0xA5, Only executed once
			if (OneWireReset(sensor) == YES)
			{
				// Skip ROM
				OneWireWriteByte(sensor, DS2431_SKIP_ROM);

				// Copy and Lock App Register
				OneWireWriteByte(sensor, 0x5A);

				// Validation Key
				OneWireWriteByte(sensor, 0xA5);

				SoftUsecWait(10 * SOFT_MSECS);

				OneWireReset(sensor);

				status = PASSED;
			}
		}
	}

	return (status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SmartSensorDebug(SMART_SENSOR_TYPE sensor)
{
	SMART_SENSOR_STRUCT smartSensorData;
	uint16 zeroed = 0;
	uint16 i = 0;
	uint32 crc32 = 0;
	uint32 crc16 = 0;
	uint32 crc16seed0 = 0;
#if 1 /* temp */
	UNUSED(crc32);
	UNUSED(crc16);
	UNUSED(crc16seed0);
#endif

	debugRaw("\r\n--------%s Sensor Data-------\r\n\n", (sensor == SEISMIC_SENSOR) ? "Seismic" : "Acoustic");

	OneWireReadMemory(sensor, 0x0, sizeof(SMART_SENSOR_STRUCT), (uint8*)&smartSensorData);

	crc32 = CalcCCITT32((uint8*)&smartSensorData.serialNumber[0], smartSensorData.dataLength, 0xFFFFFFFF);
	crc16 = CalcCrc16((uint8*)&smartSensorData.currentCal, (sizeof(CALIBRATION_DATA_SET_STRUCT) - 2), 0xFFFF);
	crc16seed0 = CalcCrc16((uint8*)&smartSensorData.currentCal, (sizeof(CALIBRATION_DATA_SET_STRUCT) - 2), 0x0);

	debugRaw("\tSmart Sensor Version: 0x%x\r\n", smartSensorData.version);
	debugRaw("\tData Length: %d\r\n", smartSensorData.dataLength);
	debugRaw("\tCrc-32: 0x%x (Match: %s)\r\n", smartSensorData.crc, (crc32 == smartSensorData.crc) ? "YES": "NO");
	debugRaw("\tSerial Number: %02x-%02x-%02x-%02x-%02x-%02x\r\n", smartSensorData.serialNumber[0], smartSensorData.serialNumber[1], smartSensorData.serialNumber[2],
				smartSensorData.serialNumber[3], smartSensorData.serialNumber[4], smartSensorData.serialNumber[5]);
	debugRaw("\tSensor Type: %4.2f (0x%02x)\r\n", (smartSensorData.sensorType < 0x80) ? (pow(2,smartSensorData.sensorType) * 2.56) : ((pow(2, (smartSensorData.sensorType - 0x80)) * 65.535)),
				smartSensorData.sensorType);
	debugRaw("\tCalibration Count: 0x%x\r\n", smartSensorData.calCount);

	for (i = 0; i < 16; i++) { zeroed += smartSensorData.reserved[i]; }
	debugRaw("\tReserved Empty: %s\r\n", (zeroed == 0) ? "YES" : "NO");

	//convertTime = *localtime((time_t*)&smartSensorData.currentCal.calibrationDate);
	debugRaw("\tCurrent Calibration Date: %s/%d/%d (0x%08x)\r\n", (char*)g_monthTable[(smartSensorData.currentCal.calDate.month)].name,
				smartSensorData.currentCal.calDate.day, smartSensorData.currentCal.calDate.year, ((CALIBRATION_DATE_UNIVERSAL_STRUCT)smartSensorData.currentCal.calDate).epochDate);
	debugRaw("\tCurrent Calibration Facility: 0x%x\r\n", smartSensorData.currentCal.calFacility);
	debugRaw("\tCurrent Calibration Instrument: 0x%x\r\n", smartSensorData.currentCal.calInstrument);
	debugRaw("\tCurrent Calibration Crc-16: 0x%x (Match: %s (0x%04x), Match Seed 0: %s (0x%04x))\r\n", smartSensorData.currentCal.calCrc,
				(crc16 == smartSensorData.currentCal.calCrc) ? "YES": "NO", crc16,
				(crc16seed0 == smartSensorData.currentCal.calCrc) ? "YES": "NO", crc16seed0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SmartSensorReadRomAndMemory(SMART_SENSOR_TYPE sensor)
{
	SMART_SENSOR_ROM* smartSensorRom = ((sensor == SEISMIC_SENSOR) ? &g_seismicSmartSensorRom : &g_acousticSmartSensorRom);
	SMART_SENSOR_STRUCT* smartSensorData = ((sensor == SEISMIC_SENSOR) ? &g_seismicSmartSensorMemory : &g_acousticSmartSensorMemory);
	uint8 status = FAILED;

	if (OneWireReadROM(sensor, smartSensorRom) == PASSED)
	{
		if (OneWireReadMemory(sensor, 0x0, sizeof(SMART_SENSOR_STRUCT), (uint8*)smartSensorData) == PASSED)
		{
			if (smartSensorData->crc == CalcCCITT32((uint8*)&smartSensorData->serialNumber[0], smartSensorData->dataLength, 0xFFFFFFFF))
			{
				// Overlay key onto version information
				smartSensorData->version |= SMART_SENSOR_OVERLAY_KEY;

				status = PASSED;
			}
			else
			{
				debugErr("Failed CRC32 check on %s Smart Sensor memory\r\n", (sensor == SEISMIC_SENSOR) ? "Seismic" : "Acoustic");
			}
		}
		else // Failed to read sensor memory
		{
			debugErr("Failed memory read on %s Smart Sensor\r\n", (sensor == SEISMIC_SENSOR) ? "Seismic" : "Acoustic");
		}
	}
	else
	{
		debugErr("Failed ROM read on %s Smart Sensor\r\n", (sensor == SEISMIC_SENSOR) ? "Seismic" : "Acoustic");
	}

	if (status == FAILED)
	{
		memset(smartSensorData, 0x0, sizeof(SMART_SENSOR_STRUCT));
		memset(smartSensorRom, 0x0, sizeof(SMART_SENSOR_ROM));
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UpdateUnitSensorsWithSmartSensorTypes(void)
{
	if (g_seismicSmartSensorMemory.version & SMART_SENSOR_OVERLAY_KEY)
	{
		g_factorySetupRecord.seismicSensorType = (pow(2, g_seismicSmartSensorMemory.sensorType) * SENSOR_2_5_IN);
	}

	if (g_acousticSmartSensorMemory.version & SMART_SENSOR_OVERLAY_KEY)
	{
		if ((g_acousticSmartSensorMemory.sensorType == SENSOR_MIC_148_DB) || (g_acousticSmartSensorMemory.sensorType == SENSOR_MIC_160_DB) || (g_acousticSmartSensorMemory.sensorType == SENSOR_MIC_5_PSI) ||
			(g_acousticSmartSensorMemory.sensorType == SENSOR_MIC_10_PSI))
		{
			g_factorySetupRecord.acousticSensorType = g_acousticSmartSensorMemory.sensorType;
		}
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DisplaySmartSensorInfo(SMART_SENSOR_INFO situation)
{
	char airSensorTypeName[16];

	if (situation == INFO_ON_CHECK)
	{
		SmartSensorReadRomAndMemory(SEISMIC_SENSOR);
		SmartSensorReadRomAndMemory(ACOUSTIC_SENSOR);
	}

	//-------------------------------------------
	// Seismic sensor check
	//-------------------------------------------
	if (g_seismicSmartSensorMemory.version & SMART_SENSOR_OVERLAY_KEY)
	{
		// ST(0) = 2.5 = X8, ST(1) = 5.12 = X4, ST(2) = 10.24 = X2, ST(3) = 20.48 = X1
		sprintf((char*)g_spareBuffer, "%s: %s %s, %s: X%d (%4.2f %s)", getLangText(DISCOVERED_TEXT), getLangText(SEISMIC_TEXT), getLangText(SMART_SENSOR_TEXT), getLangText(TYPE_TEXT),
				(uint8)(8 / pow(2, g_seismicSmartSensorMemory.sensorType)),
				(g_seismicSmartSensorMemory.sensorType < 0x80) ? (pow(2,g_seismicSmartSensorMemory.sensorType) * (double)2.56) : ((pow(2, (g_seismicSmartSensorMemory.sensorType - 0x80)) * (double)65.535)),
				(g_seismicSmartSensorMemory.sensorType < 0x80) ? ("IN") : ("MM"));
		debug("Discovered: Seismic smart sensor, type: X%d (%4.2f %s)\r\n", (uint8)(8 / pow(2, g_seismicSmartSensorMemory.sensorType)), (pow(2,g_seismicSmartSensorMemory.sensorType) * 2.56), ("IN"));
	}
	else if (situation == INFO_ON_CHECK)
	{
		sprintf((char*)g_spareBuffer, "%s %s %s", getLangText(SEISMIC_TEXT), getLangText(SMART_SENSOR_TEXT), getLangText(NOT_FOUND_TEXT));
		debug("No Seismic smart sensor found\r\n");
	}

	// Allow faster UI menu traversal if not during initialization
	if (situation == INFO_ON_INIT) { OverlayMessage(getLangText(STATUS_TEXT), (char*)g_spareBuffer, (3 * SOFT_SECS)); }
	else { MessageBox(getLangText(STATUS_TEXT), (char*)g_spareBuffer, MB_OK); }

	//-------------------------------------------
	// Acoustic sensor check
	//-------------------------------------------
	if (g_acousticSmartSensorMemory.version & SMART_SENSOR_OVERLAY_KEY)
	{
		GetAirSensorTypeName(&airSensorTypeName[0]);
		sprintf((char*)g_spareBuffer, "%s: %s %s, %s: %s", getLangText(DISCOVERED_TEXT), getLangText(ACOUSTIC_TEXT), getLangText(SMART_SENSOR_TEXT),
				getLangText(TYPE_TEXT), airSensorTypeName);
		debug("Discovered: Acoustic smart sensor, type: %s\r\n", airSensorTypeName);
	}
	else if (situation == INFO_ON_CHECK)
	{
		sprintf((char*)g_spareBuffer, "%s %s %s", getLangText(ACOUSTIC_TEXT), getLangText(SMART_SENSOR_TEXT), getLangText(NOT_FOUND_TEXT));
		debug("No Acoustic smart sensor found\r\n");
	}

	// Allow faster UI menu traversal if not during initialization
	if (situation == INFO_ON_INIT) { OverlayMessage(getLangText(STATUS_TEXT), (char*)g_spareBuffer, (3 * SOFT_SECS)); }
	else { MessageBox(getLangText(STATUS_TEXT), (char*)g_spareBuffer, MB_OK); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DisplaySmartSensorSerialNumber(SMART_SENSOR_TYPE sensor)
{
	char serialNumber[10];
	uint8* serialNumBuffer = NULL;
	char airSensorTypeName[16];

	if ((sensor == SEISMIC_SENSOR) && (g_seismicSmartSensorMemory.version & SMART_SENSOR_OVERLAY_KEY))
	{
		serialNumBuffer = &g_seismicSmartSensorMemory.serialNumber[0];
		sprintf(serialNumber, "%d%d%d%d%d%d", serialNumBuffer[0], serialNumBuffer[1], serialNumBuffer[2], serialNumBuffer[3], serialNumBuffer[4], serialNumBuffer[5]);
		sprintf((char*)g_spareBuffer, "%s SN#: %s, %s: X%d (%4.2f %s)", getLangText(SEISMIC_TEXT), serialNumber, getLangText(TYPE_TEXT),
				(uint8)(8 / pow(2, g_seismicSmartSensorMemory.sensorType)),
				(g_seismicSmartSensorMemory.sensorType < 0x80) ? (pow(2,g_seismicSmartSensorMemory.sensorType) * (double)2.56) : ((pow(2, (g_seismicSmartSensorMemory.sensorType - 0x80)) * (double)65.535)),
				(g_seismicSmartSensorMemory.sensorType < 0x80) ? ("IN") : ("MM"));
		MessageBox(getLangText(STATUS_TEXT), (char*)g_spareBuffer, MB_OK);
	}
	else if ((sensor == ACOUSTIC_SENSOR) && (g_acousticSmartSensorMemory.version & SMART_SENSOR_OVERLAY_KEY))
	{
		serialNumBuffer = &g_acousticSmartSensorMemory.serialNumber[0];
		GetAirSensorTypeName(&airSensorTypeName[0]);
		sprintf(serialNumber, "%d%d%d%d%d%d", serialNumBuffer[0], serialNumBuffer[1], serialNumBuffer[2], serialNumBuffer[3], serialNumBuffer[4], serialNumBuffer[5]);
		sprintf((char*)g_spareBuffer, "%s SN#: %s, %s: %s", getLangText(ACOUSTIC_TEXT), serialNumber, getLangText(TYPE_TEXT), airSensorTypeName);
		MessageBox(getLangText(STATUS_TEXT), (char*)g_spareBuffer, MB_OK);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 CheckIfBothSmartSensorsPresent(void)
{
	if ((g_seismicSmartSensorMemory.version & SMART_SENSOR_OVERLAY_KEY) && (g_acousticSmartSensorMemory.version & SMART_SENSOR_OVERLAY_KEY))
	{
		return YES;
	}
	else
	{
		return NO;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 CheckIfNoSmartSensorsPresent(void)
{
	if (((g_acousticSmartSensorMemory.version & SMART_SENSOR_OVERLAY_KEY) != SMART_SENSOR_OVERLAY_KEY) && ((g_seismicSmartSensorMemory.version & SMART_SENSOR_OVERLAY_KEY) != SMART_SENSOR_OVERLAY_KEY))
	{
		return YES;
	}
	else
	{
		return NO;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UpdateWorkingCalibrationDate(void)
{
	// By default use the Unit Calibration date
	g_currentCalibration.source = UNIT_CAL_DATE;
	g_currentCalibration.date = g_factorySetupRecord.calDate;

	// Check if optioned to use Seismic Smart Sensor Calibration date and Seismic smart sensor was successfully read
	if ((g_factorySetupRecord.calibrationDateSource == SEISMIC_SMART_SENSOR_CAL_DATE) && (g_seismicSmartSensorMemory.version & SMART_SENSOR_OVERLAY_KEY))
	{
		// Check if Seismic Smart Sensor Calibration date CRC checks out
		if (CalcCCITT16((uint8*)&g_seismicSmartSensorMemory.currentCal, 6, 0xFFFF) == g_seismicSmartSensorMemory.currentCal.calCrc)
		{
			// Set Seismic Calibration date as working Calibration date and source
			g_currentCalibration.source = SEISMIC_SMART_SENSOR_CAL_DATE;
			g_currentCalibration.date = g_seismicSmartSensorMemory.currentCal.calDate;
		}
	}
	// Check if optioned to use Acoustic Smart Sensor Calibration date and Acoustic smart sensor was successfully read
	else if ((g_factorySetupRecord.calibrationDateSource == ACOUSTIC_SMART_SENSOR_CAL_DATE) && (g_acousticSmartSensorMemory.version & SMART_SENSOR_OVERLAY_KEY))
	{
		// Check if Acoustic Smart Sensor Calibration date CRC checks out
		if (CalcCCITT16((uint8*)&g_acousticSmartSensorMemory.currentCal, 6, 0xFFFF) == g_acousticSmartSensorMemory.currentCal.calCrc)
		{
			// Set Acoustic Calibration date as working Calibration date and source
			g_currentCalibration.source = ACOUSTIC_SMART_SENSOR_CAL_DATE;
			g_currentCalibration.date = g_acousticSmartSensorMemory.currentCal.calDate;
		}
	}
}

/*
1) Device Reset, Command: F0h, Param: None
2) Set Read Pointer, Command: E1h, Param: Pointer code
	-Pointer codes-
	Device Configuration Register = C3h
	Status Register = F0h
	Read Data Register = E1h
	Port Configuration Register = B4h
3) Write Device Configuration, Command: D2h, Param: Config byte
4) Adjust 1-Wire Port, Command: C3h, Param, Control byte
5) 1-Wire Reset, Command: B4h, Param: None
6) 1-Wire Single Bit, Command: 87h, Param: Bit byte
7) 1-Wire Write Byte, Command: A5h, Param: Data byte
8) 1-Wire Read Byte, Command: 96h, Param: None
	Generates 8 read data time slots on the 1-wire
9) 1-Wire Triplet, Command: 78h, Param: Direction byte
*/