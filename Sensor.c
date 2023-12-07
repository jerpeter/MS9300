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

#include "i2c.h"
#include "gpio.h"
#include "mxc_errors.h"
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
#if 0 /* old hw */
	if (sensor == SEISMIC_SENSOR) { PowerControl(SEISMIC_SENSOR_DATA_CONTROL, ON); }
	else /* ACOUSTIC_SENSOR */ { PowerControl(ACOUSTIC_SENSOR_DATA_CONTROL, ON); }
#endif

	// Hold low for 500us
	//SoftUsecWait(500); // Looks like 540
	//SoftUsecWait(460); // Looks like 480
	SoftUsecWait(480);

	// Release line (allow pullup to take affect)
#if 0 /* old hw */
	if (sensor == SEISMIC_SENSOR) { PowerControl(SEISMIC_SENSOR_DATA_CONTROL, OFF); }
	else /* ACOUSTIC_SENSOR */ { PowerControl(ACOUSTIC_SENSOR_DATA_CONTROL, OFF); }
#endif

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
#if 0 /* old hw */
		if (sensor == SEISMIC_SENSOR) { PowerControl(SEISMIC_SENSOR_DATA_CONTROL, ON); }
		else /* ACOUSTIC_SENSOR */ { PowerControl(ACOUSTIC_SENSOR_DATA_CONTROL, ON); }
#endif
		// Check if the bit is a 1
		if (data & 0x01)
		{
			// Hold low for 5us
			SoftUsecWait(5);

			// Release the line
#if 0 /* old hw */
			if (sensor == SEISMIC_SENSOR) { PowerControl(SEISMIC_SENSOR_DATA_CONTROL, OFF); }
			else /* ACOUSTIC_SENSOR */ { PowerControl(ACOUSTIC_SENSOR_DATA_CONTROL, OFF); }
#endif
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
#if 0 /* old hw */
			if (sensor == SEISMIC_SENSOR) { PowerControl(SEISMIC_SENSOR_DATA_CONTROL, OFF); }
			else /* ACOUSTIC_SENSOR */ { PowerControl(ACOUSTIC_SENSOR_DATA_CONTROL, OFF); }
#endif
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
#if 0 /* old hw */
		if (sensor == SEISMIC_SENSOR) { PowerControl(SEISMIC_SENSOR_DATA_CONTROL, ON); }
		else /* ACOUSTIC_SENSOR */ { PowerControl(ACOUSTIC_SENSOR_DATA_CONTROL, ON); }
#endif
		// Hold low for 5us
		SoftUsecWait(5);

		// Release the line
#if 0 /* old hw */
		if (sensor == SEISMIC_SENSOR) { PowerControl(SEISMIC_SENSOR_DATA_CONTROL, OFF); }
		else /* ACOUSTIC_SENSOR */ { PowerControl(ACOUSTIC_SENSOR_DATA_CONTROL, OFF); }
#endif
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
#if 0 /* old hw */
	if (OneWireReset(sensor) == YES)
#else
	if (ds2484_w1_reset_bus() == 0) // 0 = device present, 1 = no device
#endif
	{
		// Read ROM command
#if 0 /* old hw */
		OneWireWriteByte(sensor, DS2431_READ_ROM);
#else
		ds2484_w1_write_byte(DS2431_READ_ROM);
#endif

		for (i = 0; i < 8; i++)
		{
#if 0 /* old hw */
			romDataPtr[i] = OneWireReadByte(sensor);
#else
			romDataPtr[i] = ds2484_w1_read_byte();
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

#if 0 /* old hw */
		OneWireReset(sensor);
#else
		ds2484_w1_reset_bus();
#endif
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
#if 0 /* old hw */
		if (OneWireReset(sensor) == YES)
#else
		if (ds2484_w1_reset_bus() == 0) // 0 = device found
#endif
		{
			// Skip ROM
#if 0 /* old hw */
			OneWireWriteByte(sensor, DS2431_SKIP_ROM);
#else
			ds2484_w1_write_byte(DS2431_SKIP_ROM);
#endif

			// Read Memory
#if 0 /* old hw */
			OneWireWriteByte(sensor, DS2431_READ_MEMORY);
#else
			ds2484_w1_write_byte(DS2431_READ_MEMORY);
#endif

			// Address (Lower)
#if 0 /* old hw */
			OneWireWriteByte(sensor, (address & 0xFF));
#else
			ds2484_w1_write_byte((address & 0xFF));
#endif

			// Address (Upper)
#if 0 /* old hw */
			OneWireWriteByte(sensor, ((address >> 8) & 0xFF));
#else
			ds2484_w1_write_byte(((address >> 8) & 0xFF));
#endif

			// Data
			for (i = 0; i < length; i++)
			{
#if 0 /* old hw */
				data[i] = OneWireReadByte(sensor);
#else
				data[i] = ds2484_w1_read_byte();
#endif
			}

#if 0 /* old hw */
			OneWireReset(sensor);
#else
			ds2484_w1_reset_bus();
#endif

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
	debugRaw("\tSensor Type: %4.2f (0x%02x)\r\n", ((smartSensorData.sensorType < 0x80) ? (double)(pow(2,smartSensorData.sensorType) * (double)2.56) : (double)((pow(2, (smartSensorData.sensorType - 0x80)) * (double)65.535))),
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
void SmartSensorTest(void)
{
	SmartSensorMuxSelectAndDriverEnable(SEISMIC_SENSOR);
	if (OneWireReset(SEISMIC_SENSOR) == YES) { debug("Seismic Smart Sensor discovered\r\n"); }
	else { debug("Seismic Smart Sensor not found\r\n"); }

	debugRaw("\r\n----------Seismic Sensor----------\r\n");
	OneWireTest(SEISMIC_SENSOR);
	SmartSensorDebug(SEISMIC_SENSOR);

	SmartSensorMuxSelectAndDriverEnable(ACOUSTIC_SENSOR);
	if (OneWireReset(ACOUSTIC_SENSOR) == YES) { debug("Acoustic Smart Sensor discovered\r\n"); }
	else { debug("Acoustic Smart Sensor not found\r\n"); }

	debugRaw("\r\n----------Acoustic Sensor----------\r\n");
	OneWireTest(ACOUSTIC_SENSOR);
	SmartSensorDebug(ACOUSTIC_SENSOR);

	// Todo: Possibly add in second set of sensors, Geo2, AOP2

	debugRaw("\r\n----------End----------\r\n");

	SmartSensorDisableMuxAndDriver();
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SmartSensorMuxSelectAndDriverEnable(SMART_SENSOR_TYPE sensor)
{
	uint8_t MuxA0, MuxA1;
	uint8_t analog5vPoweredUp = NO;

	if (sensor == SEISMIC_SENSOR_2) { MuxA1 = 0; MuxA0 = 1; }
	else if (sensor == ACOUSTIC_SENSOR) { MuxA1 = 1; MuxA0 = 0; }
	else if (sensor == ACOUSTIC_SENSOR_2) { MuxA1 = 1; MuxA0 = 1; }
	else /* (sensor == SEISMIC_SENSOR) */ { MuxA1 = 0; MuxA0 = 0; }

    if (GetPowerControlState(ANALOG_5V_ENABLE) == OFF)
	{
		debug("Power Control: Analog 5V enable being turned on\r\n");
		analog5vPoweredUp = YES;
		PowerUpAnalog5VandExternalADC();
	}

	// Check if the Smart Sensor Mux Enable is active (swapping channels)
	if (GetSmartSensorMuxEnableState() == ON)
	{
		// Make sure Mux is disabled before changing mux address lines
		SetSmartSensorMuxEnableState(OFF);
		// Delay before changing? 250ns max
		MXC_Delay(1);
	}

	// Set the 1-Write Mux to the specified sensor
	SetSmartSensorMuxA0State(MuxA0);
	SetSmartSensorMuxA1State(MuxA1);
	SetSmartSensorMuxEnableState(ON);

	// Activate the 1-Wire driver
	SetSmartSensorSleepState(OFF);
	MXC_Delay(MXC_DELAY_MSEC(2));

	// Reconfigure 1-Wire driver since it's possible that the analog 5V was removed since setup
	OneWireResetAndConfigure();

	if(analog5vPoweredUp)
	{
		PowerControl(ANALOG_5V_ENABLE, OFF);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SmartSensorDisableMuxAndDriver(void)
{
	// Disable the 1-Wire drvier (sleep)
	SetSmartSensorSleepState(ON);

	// Disable the 1-Wire mux
	SetSmartSensorMuxEnableState(OFF);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SmartSensorReadRomAndMemory(SMART_SENSOR_TYPE sensor)
{
	SMART_SENSOR_ROM* smartSensorRom = ((sensor == SEISMIC_SENSOR) ? &g_seismicSmartSensorRom : &g_acousticSmartSensorRom);
	SMART_SENSOR_STRUCT* smartSensorData = ((sensor == SEISMIC_SENSOR) ? &g_seismicSmartSensorMemory : &g_acousticSmartSensorMemory);
	uint8 status = FAILED;

	SmartSensorMuxSelectAndDriverEnable(sensor);

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

	SmartSensorDisableMuxAndDriver();
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
		debug("Discovered: Seismic smart sensor, type: X%d (%4.2f %s)\r\n", (uint8)(8 / pow(2, g_seismicSmartSensorMemory.sensorType)), (double)(pow(2,g_seismicSmartSensorMemory.sensorType) * (double)2.56), ("IN"));
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

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
extern mxc_gpio_cfg_t g_SmartSensorMuxEnable;
extern mxc_gpio_cfg_t g_SmartSensorMux_A0;
extern mxc_gpio_cfg_t g_SmartSensorMux_A1;

void EnableAndSelectSmartSensorMux(SMART_SENSOR_TYPE sensor)
{
	if (sensor < TOTAL_SENSOR_TYPES)
	{
		MXC_GPIO_OutSet(g_SmartSensorMuxEnable.port, g_SmartSensorMuxEnable.mask);
		
		// Delay 480ns @ 5V after enabling
		MXC_Delay(1); // 1us

		// Set the upper address bit, logic 0 for either seismic or logic 1 for either acoustic 
		if ((sensor == SEISMIC_SENSOR) || (sensor == SEISMIC_SENSOR_2)) { MXC_GPIO_OutClr(g_SmartSensorMux_A1.port, g_SmartSensorMux_A1.mask); }
		else /* ACOUSTIC_SENSOR or ACOUSTIC_SENSOR_2 */ { MXC_GPIO_OutSet(g_SmartSensorMux_A1.port, g_SmartSensorMux_A1.mask); }

		// Set the lower address bit, logic 0 for first sensor group or logic 1 for second sensor group
		if ((sensor == SEISMIC_SENSOR) || (sensor == ACOUSTIC_SENSOR)) { MXC_GPIO_OutClr(g_SmartSensorMux_A0.port, g_SmartSensorMux_A0.mask); }
		else /* SEISMIC_SENSOR_2 or ACOUSTIC_SENSOR_2 */ { MXC_GPIO_OutSet(g_SmartSensorMux_A0.port, g_SmartSensorMux_A0.mask); }
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DisableSmartSensorMux(void)
{
	MXC_GPIO_OutClr(g_SmartSensorMuxEnable.port, g_SmartSensorMuxEnable.mask);

	// Delay 400ns @ 5V after disabling
	MXC_Delay(1); // 1us
}

///============================================================================
///----------------------------------------------------------------------------
///	1-Wire Master DS2484
///----------------------------------------------------------------------------
///============================================================================
/* DS4 datasheet note: The APU bit controls whether an active pullup (low impedance transistor) or a passive pullup (RWPU resistor) is used to drive a 1-Wire line from low to high.
						When APU = 0, active pullup is disabled (resistor mode). Enabling active pullup is generally recommended for best 1-Wire bus performance. */
static int ds2484_active_pullup = 1;

// 1-Wire Speed: Standard speed = 0, Overdrive speed = 1;
static int ds2484_1wire_speed = 0;

// DS2484 registers: 4 registers are addressed by a read pointer, the read pointer is set by the last command executed
// To read the data, issue a register read for any address
#define DS2484_CMD_RESET				0xF0	// No param
#define DS2484_CMD_SET_READ_PTR			0xE1	// Param: DS2484_PTR_CODE_xxx
#define DS2484_CMD_WRITE_CONFIG			0xD2	// Param: Config byte
#define DS2484_CMD_ADJUST_1WIRE_PORT	0xC3	// Param: Control byte
#define DS2484_CMD_1WIRE_RESET			0xB4	// Param: None
#define DS2484_CMD_1WIRE_SINGLE_BIT		0x87	// Param: Bit byte (bit7)
#define DS2484_CMD_1WIRE_WRITE_BYTE		0xA5	// Param: Data byte
#define DS2484_CMD_1WIRE_READ_BYTE		0x96	// Param: None
// Note to read the byte, Set the ReadPtr to Data then read (any addr)
#define DS2484_CMD_1WIRE_TRIPLET		0x78	// Param: Dir byte (bit7)

// Values for DS2484_CMD_SET_READ_PTR
#define DS2484_PTR_CODE_DEVICE_CONFIG	0xC3
#define DS2484_PTR_CODE_STATUS			0xF0
#define DS2484_PTR_CODE_READ_DATA		0xE1
#define DS2484_PTR_CODE_PORT_CONFIG		0xB4

// DS2484 Config Register bit definitions, the top 4 bits always read 0
// When writing, the top nibble must be the 1's compliment of the low nibble
#define DS2484_REG_CFG_1WS		0x08	// 1-wire speed
#define DS2484_REG_CFG_SPU		0x04	// Strong pull-up
#define DS2484_REG_CFG_PDN		0x02	// 1-wire power down
#define DS2484_REG_CFG_APU		0x01	// Active pull-up

// DS2484 Status Register bit definitions (read only)
#define DS2484_REG_STS_DIR		0x80	// Branch direction taken
#define DS2484_REG_STS_TSB		0x40	// Triple second bit
#define DS2484_REG_STS_SBR		0x20	// Single bit result
#define DS2484_REG_STS_RST		0x10	// Device reset
#define DS2484_REG_STS_LL		0x08	// Logic level
#define DS2484_REG_STS_SD		0x04	// Short detected
#define DS2484_REG_STS_PPD		0x02	// Presence pulse detect
#define DS2484_REG_STS_1WB		0x01	// 1-wire busy

#define DS2484_PARAM_RSTL		0x000
#define DS2484_PARAM_MSP		0x001
#define DS2484_PARAM_W0L		0x010
#define DS2484_PARAM_REC0		0x011
#define DS2484_PARAM_R_WPU		0x100

typedef struct {
	uint8_t			read_pointer;	/* see DS2484_PTR_CODE_xxx */
	uint8_t			reg_config;
} ds2484_data;

ds2484_data s_ds2484_data;

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
/**
 * ds2484_calculate_config - Helper to calculate values for configuration register
 * @conf: the raw config value
 * Return: the value w/ complements that can be written to register
 */
static inline uint8_t ds2484_calculate_config(uint8_t conf)
{
	if (ds2484_1wire_speed) { conf |= DS2484_REG_CFG_1WS; }
	if (ds2484_active_pullup) { conf |= DS2484_REG_CFG_APU; }

	return conf | ((~conf & 0x0f) << 4);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
/**
 * ds2484_select_register - Sets the read pointer.
 * @pdev:		The ds2484 client pointer
 * @read_ptr:	see DS2484_PTR_CODE_xxx above
 * Return: -1 on failure, 0 on success
 */
static inline int ds2484_select_register(ds2484_data *pdev, uint8_t read_ptr)
{
	uint8_t data[2] = {DS2484_CMD_SET_READ_PTR, read_ptr};

	if (pdev->read_pointer != read_ptr)
	{
		if (WriteI2CDevice(MXC_I2C0, I2C_ADDR_1_WIRE, data, sizeof(data), NULL, 0) != E_SUCCESS) { return -1; }

		pdev->read_pointer = read_ptr;
	}

	return (0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
/**
 * ds2484_send_cmd - Sends a command without a parameter
 * @pdev:	The ds2484 client pointer
 * @cmd:	DS2484_CMD_RESET,
 *		DS2484_CMD_1WIRE_RESET,
 *		DS2484_CMD_1WIRE_READ_BYTE
 * Return: -1 on failure, 0 on success
 */
static inline int ds2484_send_cmd(ds2484_data *pdev, uint8_t cmd)
{
	if (WriteI2CDevice(MXC_I2C0, I2C_ADDR_1_WIRE, &cmd, sizeof(cmd), NULL, 0) != E_SUCCESS) { return -1; }

	pdev->read_pointer = DS2484_PTR_CODE_STATUS;
	return (0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
/**
 * ds2484_send_cmd_data - Sends a command with a parameter
 * @pdev:	The ds2484 client pointer
 * @cmd:	DS2484_CMD_WRITE_CONFIG, DS2484_CMD_ADJUST_1WIRE_PORT, DS2484_CMD_1WIRE_SINGLE_BIT, DS2484_CMD_1WIRE_WRITE_BYTE, DS2484_CMD_1WIRE_TRIPLET
 * @byte:	The data to send
 * Return: -1 on failure, 0 on success
 */
static inline int ds2484_send_cmd_data(ds2484_data *pdev, uint8_t cmd, uint8_t byte)
{
	uint8_t data[2] = {cmd, byte};

	if (WriteI2CDevice(MXC_I2C0, I2C_ADDR_1_WIRE, data, sizeof(data), NULL, 0) != E_SUCCESS) { return -1; }

	// Adjust where the read pointer rests after a write
	if (cmd == DS2484_CMD_WRITE_CONFIG) { pdev->read_pointer = DS2484_CMD_WRITE_CONFIG; }
	else if (cmd == DS2484_CMD_ADJUST_1WIRE_PORT) { pdev->read_pointer = DS2484_CMD_ADJUST_1WIRE_PORT; }
	else { pdev->read_pointer = DS2484_PTR_CODE_STATUS; }

	return (0);
}

#define DS2484_WAIT_IDLE_TIMEOUT	100

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
/**
 * ds2484_wait_1wire_idle - Waits until the 1-wire interface is idle (not busy)
 *
 * @pdev: Pointer to the device structure
 * Return: the last value read from status or -1 (failure)
 */
static int ds2484_wait_1wire_idle(ds2484_data *pdev)
{
	uint8_t temp;
	int retries = 0;

	if (!ds2484_select_register(pdev, DS2484_PTR_CODE_STATUS))
	{
		do {
			WriteI2CDevice(MXC_I2C0, I2C_ADDR_1_WIRE, NULL, 0, &temp, sizeof(temp));
		} while ((temp & DS2484_REG_STS_1WB) && (++retries < DS2484_WAIT_IDLE_TIMEOUT));
	}

	if (retries >= DS2484_WAIT_IDLE_TIMEOUT) { debugErr("1-Wire Master: timed out\n"); }

	// Todo: Fix reference driver return value that is unsigned but claims -1 designates failure
	return (temp);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ds2484_adjust_port(uint8_t timingValue, uint8_t overdriveControl)
{
	ds2484_data* pdev = &s_ds2484_data;
	uint8_t paramData;

	/*
		Bits 7:5 (top 3) are Parameter Selection of the Control byte
		000: selects tRSTL
		001: selects tMSP
		010: selects tW0L
		011: selects tREC0; the OD flag does not apply (don’t care)
		100: selects RWPU; the OD flag does not apply (don’t care)
	*/

	// RSTL
	paramData = ((DS2484_PARAM_RSTL < 5) | (overdriveControl << 4) | (timingValue));
	ds2484_send_cmd_data(pdev, DS2484_CMD_ADJUST_1WIRE_PORT, paramData);

	// MSP
	paramData = ((DS2484_PARAM_MSP < 5) | (overdriveControl << 4) | (timingValue));
	ds2484_send_cmd_data(pdev, DS2484_CMD_ADJUST_1WIRE_PORT, paramData);

	// W0L
	paramData = ((DS2484_PARAM_W0L < 5) | (overdriveControl << 4) | (timingValue));
	ds2484_send_cmd_data(pdev, DS2484_CMD_ADJUST_1WIRE_PORT, paramData);

	// REC0
	paramData = ((DS2484_PARAM_REC0 < 5) | (overdriveControl << 4) | (timingValue));
	ds2484_send_cmd_data(pdev, DS2484_CMD_ADJUST_1WIRE_PORT, paramData);

	// R_WPU
	paramData = ((DS2484_PARAM_R_WPU < 5) | (overdriveControl << 4) | (timingValue));
	ds2484_send_cmd_data(pdev, DS2484_CMD_ADJUST_1WIRE_PORT, paramData);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ds2484_adjust_port_param(uint8_t param, uint8_t timingValue, uint8_t overdriveControl)
{
	ds2484_data* pdev = &s_ds2484_data;

	ds2484_send_cmd_data(pdev, DS2484_CMD_ADJUST_1WIRE_PORT, ((param < 5) | (overdriveControl << 4) | (timingValue)));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
/**
 * ds2484_w1_touch_bit - Performs the touch-bit function, which writes a 0 or 1 and reads the level.
 *
 * @bit:	The level to write: 0 or non-zero
 * Return:	The level read: 0 or 1
 */
uint8_t ds2484_w1_touch_bit(uint8_t bit)
{
	ds2484_data* pdev = &s_ds2484_data;
	int status = -1;

	/* Send the touch command, wait until 1WB == 0, return the status */
	if (!ds2484_send_cmd_data(pdev, DS2484_CMD_1WIRE_SINGLE_BIT, bit ? 0xFF : 0))
	{
		status = ds2484_wait_1wire_idle(pdev);
	}

	return ((status & DS2484_REG_STS_SBR) ? 1 : 0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
/**
 * ds2484_w1_triplet - Performs the triplet function, which reads two bits and writes a bit.
 * The bit written is determined by the two reads:
 *   00 => dbit, 01 => 0, 10 => 1
 *
 * @dbit:	The direction to choose if both branches are valid
 * Return:	b0=read1 b1=read2 b3=bit written
 */
uint8_t ds2484_w1_triplet(uint8_t dbit)
{
	ds2484_data* pdev = &s_ds2484_data;
	int status = (3 << 5);

	/* Send the triplet command, wait until 1WB == 0, return the status */
	if (!ds2484_send_cmd_data(pdev, DS2484_CMD_1WIRE_TRIPLET, dbit ? 0xFF : 0))
	{
		status = ds2484_wait_1wire_idle(pdev);
	}

	/* Decode the status */
	return (status >> 5);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
/**
 * ds2484_w1_write_byte - Performs the write byte function.
 *
 * @byte:	The value to write
 */
void ds2484_w1_write_byte(uint8_t byte)
{
	ds2484_data* pdev = &s_ds2484_data;

	/* Send the write byte command */
	ds2484_send_cmd_data(pdev, DS2484_CMD_1WIRE_WRITE_BYTE, byte);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
/**
 * ds2484_w1_read_byte - Performs the read byte function.
 *
 * Return:	The value read
 */
uint8_t ds2484_w1_read_byte(void)
{
	ds2484_data* pdev = &s_ds2484_data;
	uint8_t result;

	/* Send the read byte command */
	ds2484_send_cmd(pdev, DS2484_CMD_1WIRE_READ_BYTE);

	/* Wait until 1WB == 0 */
	ds2484_wait_1wire_idle(pdev);

	/* Select the data register */
	ds2484_select_register(pdev, DS2484_PTR_CODE_READ_DATA);

	/* Read the data byte */
	WriteI2CDevice(MXC_I2C0, I2C_ADDR_1_WIRE, NULL, 0, &result, sizeof(result));

	return (result);
}


///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
/**
 * ds2484_w1_reset_bus - Sends a reset on the 1-wire interface
 *
 * Return:	0=Device present, 1=No device present or error
 */
uint8_t ds2484_w1_reset_bus(void)
{
	ds2484_data* pdev = &s_ds2484_data;
	int err;
	uint8_t retval = 1;

	/* Send the reset command */
	err = ds2484_send_cmd(pdev, DS2484_CMD_1WIRE_RESET);
	if (err >= 0)
	{
		/* Wait until the reset is complete */
		err = ds2484_wait_1wire_idle(pdev);
		retval = !(err & DS2484_REG_STS_PPD);

		/* If the chip did reset since detect, re-config it */
		if (err & DS2484_REG_STS_RST)
		{
			ds2484_send_cmd_data(pdev, DS2484_CMD_WRITE_CONFIG, ds2484_calculate_config(0x00));
		}
	}

	return (retval);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t ds2484_w1_set_pullup(int delay)
{
	ds2484_data* pdev = &s_ds2484_data;
	uint8_t retval = 1;

	/* if delay is non-zero activate the pullup,
	 * the strong pullup will be automatically deactivated
	 * by the master, so do not explicitly deactive it
	 */
	if (delay)
	{
		/* both waits are crucial, otherwise devices might not be
		 * powered long enough, causing e.g. a w1_therm sensor to
		 * provide wrong conversion results
		 */
		ds2484_wait_1wire_idle(pdev);
		/* note: it seems like both SPU and APU have to be set! */
		retval = ds2484_send_cmd_data(pdev, DS2484_CMD_WRITE_CONFIG, ds2484_calculate_config(DS2484_REG_CFG_SPU | DS2484_REG_CFG_APU));
		ds2484_wait_1wire_idle(pdev);
	}

	return (retval);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int ds2484_probe(void)
{
	ds2484_data* data = &s_ds2484_data;
	int err = E_NO_DEVICE;
	uint8_t temp1;

	/* Reset the device (sets the read_ptr to status) */
	if (ds2484_send_cmd(data, DS2484_CMD_RESET) < 0)
	{
		debugWarn("1-wire Master: DS2484 reset failed\r\n");
		goto exit_free;
	}

	/* Sleep at least 525ns to allow the reset to complete */
	MXC_Delay(1);

	/* Read the status byte - only reset bit and line should be set */
	WriteI2CDevice(MXC_I2C0, I2C_ADDR_1_WIRE, NULL, 0, &temp1, sizeof(temp1));
	if (temp1 != (DS2484_REG_STS_LL | DS2484_REG_STS_RST))
	{
		debugWarn("1-wire Master: DS2484 reset status 0x%02X\r\n", temp1);
		goto exit_free;
	}

	/* Set all config items to 0 (off) */
	ds2484_send_cmd_data(data, DS2484_CMD_WRITE_CONFIG, ds2484_calculate_config(0x00));

	return (0);

exit_free:
	return (err);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Test1Wire(void)
{
	ds2484_data* data = &s_ds2484_data;
	uint8_t temp1;

    debug("1-Wire Master: Test device access...\r\n");

    if (GetPowerControlState(ANALOG_5V_ENABLE) == OFF)
	{
		debug("Power Control: Analog 5V enable being turned on\r\n");
		PowerUpAnalog5VandExternalADC();
	}
	else { debug("Power Control: Analog 5V enable already on\r\n"); }

	debug("1-Wire Mux: Setting Mux for Smart Sensor: Geophone 1\r\n");
	SetSmartSensorMuxA0State(0);
	SetSmartSensorMuxA1State(0);
	debug("1-Wire Mux: Enabling Mux\r\n");
	SetSmartSensorMuxEnableState(ON);

    debug("1-Wire Master: Powering up (disabling sleep)\r\n");
	SetSmartSensorSleepState(OFF);
	MXC_Delay(MXC_DELAY_MSEC(10));

	if (ds2484_send_cmd(data, DS2484_CMD_RESET) < 0) { debugWarn("1-Wire Master: Reset failed\r\n"); }
	else { debug("1-Wire Master: Reset successful\r\n"); }

	/* Sleep at least 525ns to allow the reset to complete */
	MXC_Delay(1);

	/* Read the status byte - only reset bit and line should be set */
	WriteI2CDevice(MXC_I2C0, I2C_ADDR_1_WIRE, NULL, 0, &temp1, sizeof(temp1));
	if (temp1 != (DS2484_REG_STS_LL | DS2484_REG_STS_RST)) { debugWarn("1-Wire Master: Reset status 0x%02X\r\n", temp1); }
	else { debug("1-Wire Master: Status is 0x%x\r\n", temp1); }

	debug("1-Wire Master: Set device config\r\n");
	ds2484_send_cmd_data(data, DS2484_CMD_WRITE_CONFIG, ds2484_calculate_config(0x00));

	MXC_Delay(MXC_DELAY_MSEC(500));

	ds2484_select_register(data, DS2484_PTR_CODE_STATUS);
	WriteI2CDevice(MXC_I2C0, I2C_ADDR_1_WIRE, NULL, 0, &temp1, sizeof(temp1));
	debug("1-Wire Master: Status after config and delay is 0x%x\r\n", temp1);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void OneWireResetAndConfigure(void)
{
	ds2484_data* data = &s_ds2484_data;

	// Issue device reset (also sets read pointer at the status register)
	if (ds2484_send_cmd(data, DS2484_CMD_RESET) < 0) { debugErr("1-Wire Master: Reset failed\r\n"); }

	/* Sleep at least 525ns to allow the reset to complete */
	MXC_Delay(1);

	// Write device config, enabling active pullup (generally recommended for best 1-Wire bus performance)
	ds2484_send_cmd_data(data, DS2484_CMD_WRITE_CONFIG, ds2484_calculate_config(DS2484_REG_CFG_APU));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void OneWireInit(void)
{
	ds2484_data* data = &s_ds2484_data;
	uint8_t analog5vPoweredUp = NO;
	uint8_t status;

    if (GetPowerControlState(ANALOG_5V_ENABLE) == OFF)
	{
		analog5vPoweredUp = YES;
		PowerUpAnalog5VandExternalADC();
	}

	// No need to set and enable the 1-Wire mux since we're only going to write 1-Wire driver config
	//SetSmartSensorMuxA0State(0); SetSmartSensorMuxA1State(0); SetSmartSensorMuxEnableState(ON);

	// Enable the 1-Wire driver
	SetSmartSensorSleepState(OFF);
	MXC_Delay(MXC_DELAY_MSEC(2)); // Per datasheet delay coming out of sleep

	// Issue device reset (also sets read pointer at the status register)
	if (ds2484_send_cmd(data, DS2484_CMD_RESET) < 0) { debugErr("1-Wire Master: Reset failed\r\n"); }

	/* Sleep at least 525ns to allow the reset to complete */
	MXC_Delay(1);

	/* Read the status byte - only reset bit and line should be set */
	WriteI2CDevice(MXC_I2C0, I2C_ADDR_1_WIRE, NULL, 0, &status, sizeof(status));
	if (status != (DS2484_REG_STS_LL | DS2484_REG_STS_RST)) { debugWarn("1-Wire Master: Reset status different than expected (0x%02X)\r\n", status); }

	// Write device config, enabling power down for now, enabling active pullup (generally recommended for best 1-Wire bus performance)
	ds2484_send_cmd_data(data, DS2484_CMD_WRITE_CONFIG, ds2484_calculate_config(DS2484_REG_CFG_PDN | DS2484_REG_CFG_APU));

	ds2484_select_register(data, DS2484_PTR_CODE_STATUS);
	WriteI2CDevice(MXC_I2C0, I2C_ADDR_1_WIRE, NULL, 0, &status, sizeof(status));
	if (status & DS2484_REG_STS_RST) { debugWarn("1-Wire Master: Reset status should have been cleared (0x%02X)\r\n", status); }

	if (analog5vPoweredUp)
	{
		PowerControl(ANALOG_5V_ENABLE, OFF);
	}
}
