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
//#include <math.h>
//#include "Typedefs.h"
//#include "Board.h"
//#include "Sensor.h"
//#include "OldUart.h"
//#include "Crc.h"
//#include "string.h"
#include "PowerManagement.h"
#include "tmr.h"
#include "mxc_delay.h"

#include "lcd.h"
#include "spi.h"

#if /* Old board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN) /* KX134 */
///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define ACC_MAN_ID_REGISTER 0x00
#define ACC_CHANNEL_DATA_START_REGISTER 0x08
#define ACC_CHANNEL_DATA_SIZE   6
#define ACC_COMMAND_TEST_RESPONSE_REGISTER  0x12
#define ACC_INTERRUPT_SOURCE_1_REGISTER  0x16
#define ACC_INTERRUPT_SOURCE_2_REGISTER  0x17
#define ACC_INTERRUPT_SOURCE_3_REGISTER  0x18
#define ACC_INTERRUPT_RELEASE_REGISTER  0x1A
#define ACC_CONTROL_1_REGISTER  0x1B
#define ACC_CONTROL_2_REGISTER  0x1C
#define ACC_CONTROL_3_REGISTER  0x1D
#define ACC_CONTROL_4_REGISTER  0x1E
#define ACC_CONTROL_5_REGISTER  0x1F
#define ACC_CONTROL_6_REGISTER  0x20
#define ACC_OUTPUT_DATA_CONTROL_REGISTER  0x21  
#define ACC_INTERRUPT_CONTROL_1_REGISTER  0x22
#define ACC_INTERRUPT_CONTROL_2_REGISTER  0x23
#define ACC_INTERRUPT_CONTROL_3_REGISTER  0x24
#define ACC_INTERRUPT_CONTROL_4_REGISTER  0x25
#define ACC_INTERRUPT_CONTROL_5_REGISTER  0x26
#define ACC_INTERRUPT_CONTROL_6_REGISTER  0x27
#define ACC_SELF_TEST_REGISTER  0x5D

enum {
	ACC_ODR_0_781,
	ACC_ODR_1_563,
	ACC_ODR_3_125,
	ACC_ODR_6_25,
	ACC_ODR_12_5,
	ACC_ODR_25,
	ACC_ODR_50,
	ACC_ODR_100,
	ACC_ODR_200,
	ACC_ODR_400,
	ACC_ODR_800,
	ACC_ODR_1600,
	ACC_ODR_3200,
	ACC_ODR_6400,
	ACC_ODR_12800,
	ACC_ODR_25600
};

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
#include "Globals.h"

///----------------------------------------------------------------------------
///	Local Scope Globals
///----------------------------------------------------------------------------
uint8_t accelerometerI2CAddr = I2C_ADDR_ACCELEROMETER;

///----------------------------------------------------------------------------
///	Device Info - KX134-1211
///----------------------------------------------------------------------------
/*
Address	Register Name	R/W
00	MAN_ID	R
01	PART_ID	R
02	XADP_L	R
03	XADP_H	R
04	YADP_L	R
05	YADP_H	R
06	ZADP_L	R
07	ZADP_H	R

08	XOUT_L	R // Acc output 2's compliment
09	XOUT_H	R // Acc output 2's compliment
0A	YOUT_L	R // Acc output 2's compliment
0B	YOUT_H	R // Acc output 2's compliment
0C	ZOUT_L	R // Acc output 2's compliment
0D	ZOUT_H	R // Acc output 2's compliment

12	COTR	R
13	WHO_AM_I	R
14	TSCP	R
15	TSPP	R
16	INS1	R
17	INS2	R
18	INS3	R
19	STATUS_REG	R
1A	INT_REL	R
1B	CNTL11	R/W
1C	CNTL22	R/W
1D	CNTL31	R/W
1E	CNTL41	R/W
1F	CNTL52	R/W
20	CNTL62	R/W
21	ODCNTL1	R/W
22	INC11	R/W
23	INC21	R/W
24	INC31	R/W
25	INC41	R/W
26	INC51	R/W
27	INC61	R/W

29	TILT_TIMER2	R/W
2A	TDTRC2	R/W
2B	TDTC2	R/W
2C	TTH2	R/W
2D	TTL2	R/W
2E	FTD2	R/W
2F	STD2	R/W
30	TLT2	R/W
31	TWS2	R/W
32	FFTH2	R/W
33	FFC2	R/W
34	FFCNTL1	R/W

37	TILT_ANGLE_LL2	R/W
38	TILT_ANGLE_HL2	R/W
39	HYST_SET2	R/W
3A	LP_CNTL11	R/W
3B	LP_CNTL21	R/W

49	WUFTH2	R/W
4A	BTSWUFTH2	R/W
4B	BTSTH2	R/W
4C	BTSC2	R/W
4D	WUFC2	R/W

5D	SELF_TEST	W
5E	BUF_CNTL12	R/W
5F	BUF_CNTL22	R/W
60	BUF_STATUS_1	R
61	BUF_STATUS_2	R
62	BUF_CLEAR2	W
63	BUF_READ	R
64-76	ADP_CNTL(1-19)2	R/W
*/

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int GetAccRegister(uint8_t registerAddress, uint8_t* registerData, uint8_t dataSize)
{
	int status = E_SUCCESS;
#if /* New board */ ((HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN) || (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_PRODUCTION))
	if (dataSize > 7) { return (E_BAD_PARAM); }
	uint8_t readData[8];
	uint8_t writeData[8];
	writeData[0] = registerAddress | 0x80; // Set the MSB to 1 to signify a read
	memset(&writeData[1], 0, dataSize);

	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { MXC_GPIO_OutClr(GPIO_SPI2_SS1_ACC_PORT, GPIO_SPI2_SS1_ACC_PIN); }
	SpiTransaction(SPI_ACC, SPI_8_BIT_DATA_SIZE, YES, &writeData[0], sizeof(writeData), readData, sizeof(readData), BLOCKING);
	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { MXC_GPIO_OutSet(GPIO_SPI2_SS1_ACC_PORT, GPIO_SPI2_SS1_ACC_PIN); }

	memcpy(registerData, &readData[1], dataSize);
#else /* HARDWARE_ID_REV_PROTOTYPE_1 */
#if 0 /* Original */
	status = WriteI2CDevice(MXC_I2C0, I2C_ADDR_ACCELEROMETER, &registerAddress, sizeof(uint8_t), registerData, sizeof(uint8_t));
#else /* Use variable address since the Acc slave address sometimes changes */
	status = WriteI2CDevice(MXC_I2C0, accelerometerI2CAddr, &registerAddress, sizeof(uint8_t), registerData, dataSize);
#endif
#endif
	return (status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int SetAccRegister(uint8_t registerAddress, uint8_t registerData)
{
	int status = E_SUCCESS;

	uint8_t writeData[2];
	writeData[0] = registerAddress;
	writeData[1] = registerData;

#if /* New board */ ((HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN) || (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_PRODUCTION))
	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { MXC_GPIO_OutClr(GPIO_SPI2_SS1_ACC_PORT, GPIO_SPI2_SS1_ACC_PIN); }
	SpiTransaction(SPI_ACC, SPI_8_BIT_DATA_SIZE, YES, &writeData[0], sizeof(writeData), NULL, 0, BLOCKING);
	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { MXC_GPIO_OutSet(GPIO_SPI2_SS1_ACC_PORT, GPIO_SPI2_SS1_ACC_PIN); }
#else /* HARDWARE_ID_REV_PROTOTYPE_1 */
#if 0 /* Original */
	status = WriteI2CDevice(MXC_I2C0, I2C_ADDR_ACCELEROMETER, writeData, sizeof(writeData), NULL, 0);
#else /* Use variable address since the Acc slave address sometimes changes */
	status = WriteI2CDevice(MXC_I2C0, accelerometerI2CAddr, writeData, sizeof(writeData), NULL, 0);
#endif
#endif
	return (status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int SetAndReadAccRegister(uint8_t registerAddress, uint8_t registerData, uint8_t* readData, uint8_t dataSize)
{
	uint8_t writeData[2];
	int status = E_SUCCESS;

	writeData[0] = registerAddress;
	writeData[1] = registerData;

#if /* New board */ ((HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN) || (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_PRODUCTION))
	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { MXC_GPIO_OutClr(GPIO_SPI2_SS1_ACC_PORT, GPIO_SPI2_SS1_ACC_PIN); }
	SpiTransaction(SPI_ACC, SPI_8_BIT_DATA_SIZE, YES, &writeData[0], sizeof(writeData), readData, dataSize, BLOCKING);
	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { MXC_GPIO_OutSet(GPIO_SPI2_SS1_ACC_PORT, GPIO_SPI2_SS1_ACC_PIN); }
#else /* HARDWARE_ID_REV_PROTOTYPE_1 */
#if 0 /* Original */
	status = WriteI2CDevice(MXC_I2C0, I2C_ADDR_ACCELEROMETER, writeData, sizeof(writeData), NULL, 0);
#else /* Use variable address since the Acc slave address sometimes changes */
	status = WriteI2CDevice(MXC_I2C0, accelerometerI2CAddr, writeData, sizeof(writeData), readData, dataSize);
#endif
#endif
	return (status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int SetAccFullScaleRange(uint16_t rangeInGravity)
{
	uint8_t controlReg1;
	uint8_t rangeBitSelection;

	// Range check and conversion from 8G, 16G, 32G and 64G to 0x00, 0x08, 0x10 and 0x18 bits respectively
	if (rangeInGravity == 16) { rangeBitSelection = 0x00; }
	else if (rangeInGravity == 16) { rangeBitSelection = 0x08; }
	else if (rangeInGravity == 32) { rangeBitSelection = 0x10; }
	else if (rangeInGravity == 64) { rangeBitSelection = 0x18; }
	else return (E_BAD_PARAM);

	GetAccRegister(ACC_CONTROL_1_REGISTER, &controlReg1, sizeof(uint8_t));
	controlReg1 &= 0xE7; // Clear GSEL1 and GSEL0 bits
	controlReg1 |= rangeInGravity;
	SetAccRegister(ACC_CONTROL_1_REGISTER, controlReg1);

	// Verify
	controlReg1 = 0;
	GetAccRegister(ACC_CONTROL_1_REGISTER, &controlReg1, sizeof(uint8_t));
	if ((controlReg1 & 0x18) == rangeBitSelection) { return (E_SUCCESS); }
	else return (E_FAIL);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t VerifyAccManuIDAndPartID(void)
{
	uint8_t manuIDAndPartID[6];
	uint8_t registerAddress = ACC_MAN_ID_REGISTER;
	uint8_t status = FAILED;

#if 0 /* Original */
	WriteI2CDevice(MXC_I2C0, I2C_ADDR_ACCELEROMETER, &registerAddress, sizeof(uint8_t), manuIDAndPartID, sizeof(manuIDAndPartID));
#else /* Use variable address since the Acc slave address sometimes changes */
	//WriteI2CDevice(MXC_I2C0, accelerometerI2CAddr, &registerAddress, sizeof(uint8_t), manuIDAndPartID, sizeof(manuIDAndPartID));
	GetAccRegister(registerAddress, &manuIDAndPartID[0], sizeof(manuIDAndPartID));
#endif

	if ((manuIDAndPartID[0] == 'K') && (manuIDAndPartID[1] == 'i') && (manuIDAndPartID[2] == 'o') && (manuIDAndPartID[3] == 'n'))
	{
		status = PASSED;
		debug("Accelerometer Man ID verified, Part ID: 0x%x 0x%x (Slave Addr: %02x)\r\n", manuIDAndPartID[4], manuIDAndPartID[5], accelerometerI2CAddr);
	}
	else
	{
		debugErr("Accelerometer Man ID failed verified, 0x%x 0x%x 0x%x 0x%x (Slave Addr: %02x)\r\n", manuIDAndPartID[0], manuIDAndPartID[1], manuIDAndPartID[2], manuIDAndPartID[3], accelerometerI2CAddr);
	}

	return (status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GetAccelerometerChannelData(ACC_DATA_STRUCT* channelData)
{
	uint8_t registerAddress = ACC_CHANNEL_DATA_START_REGISTER;
	uint8_t* chanDataBytePtr = (uint8_t*)channelData;

   	// Data read in LSB/MSB (little endian), so no conversion necessary
#if 0 /* Original */
	WriteI2CDevice(MXC_I2C0, I2C_ADDR_ACCELEROMETER, &registerAddress, sizeof(uint8_t), chanDataBytePtr, ACC_CHANNEL_DATA_SIZE);
#else /* Use variable address since the Acc slave address sometimes changes */
	//WriteI2CDevice(MXC_I2C0, accelerometerI2CAddr, &registerAddress, sizeof(uint8_t), chanDataBytePtr, ACC_CHANNEL_DATA_SIZE);
	GetAccRegister(registerAddress, chanDataBytePtr, ACC_CHANNEL_DATA_SIZE);
	//UNUSED(registerAddress);
	//UNUSED(chanDataBytePtr);
#endif

#if 0 /* Should be unnecessary since data comes across Little Endian */
	channelData->x = (chanDataBytePtr[0] | (chanDataBytePtr[1] << 8));
	channelData->y = (chanDataBytePtr[2] | (chanDataBytePtr[3] << 8));
	channelData->z = (chanDataBytePtr[4] | (chanDataBytePtr[5] << 8));
#endif

	// Normalize around 0x8000
	channelData->x = ((int16_t)channelData->x + 0x8000);
	channelData->y = ((int16_t)channelData->y + 0x8000);
	channelData->z = ((int16_t)channelData->z + 0x8000);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t VerifyAccCommandTestResponse(void)
{
	uint8_t status = FAILED;
	uint8_t ctrData;
	uint8_t registerData;

	// Read CTR register
	GetAccRegister(ACC_COMMAND_TEST_RESPONSE_REGISTER, &ctrData, sizeof(uint8_t));

	if (ctrData == 0x55)
	{
		// Write COTC bit in Control 2 register
		GetAccRegister(ACC_CONTROL_2_REGISTER, &registerData, sizeof(uint8_t));
		registerData |= 0x40; // Enable PC1 bit
		SetAccRegister(ACC_CONTROL_2_REGISTER, registerData);

		// Read CTR register again
		GetAccRegister(ACC_COMMAND_TEST_RESPONSE_REGISTER, &ctrData, sizeof(uint8_t));

		if (ctrData == 0xAA)
		{
			// Read CTR register again
			GetAccRegister(ACC_COMMAND_TEST_RESPONSE_REGISTER, &ctrData, sizeof(uint8_t));

			if (ctrData == 0x55)
			{
				status = PASSED;
			}
		}
	}

	return (status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void StartAccelerometerAquisition(void)
{
	uint8_t registerData;

	SetupSPI2_Accelerometer(ON);

	// Set the ODR
	GetAccRegister(ACC_OUTPUT_DATA_CONTROL_REGISTER, &registerData, sizeof(uint8_t));
	registerData &= 0xF0; // Clear out OSA bits
	registerData |= ACC_ODR_1600; // Enable the output data rate
	SetAccRegister(ACC_OUTPUT_DATA_CONTROL_REGISTER, registerData);

	// Start operating mode and high performance
	GetAccRegister(ACC_CONTROL_1_REGISTER, &registerData, sizeof(uint8_t));
	registerData |= 0xC0; // Enable PC1 and RES bits
	SetAccRegister(ACC_CONTROL_1_REGISTER, registerData);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void StopAccelerometerAquisition(void)
{
	uint8_t registerData;

	GetAccRegister(ACC_CONTROL_1_REGISTER, &registerData, sizeof(uint8_t));
	registerData &= 0x7F; // Disable PC1 bit
	SetAccRegister(ACC_CONTROL_1_REGISTER, registerData);

	SetupSPI2_Accelerometer(OFF);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitAccControl(void)
{
	uint8_t registerData;

	GetAccRegister(ACC_CONTROL_1_REGISTER, &registerData, sizeof(uint8_t));

	// Check if PC1 shows in operation
	if (registerData & 0x80)
	{
		// Put the Acc in standby mode to allow changing control register values
		SetAccRegister(ACC_CONTROL_1_REGISTER, 0x00);
	}

	SetAccRegister(ACC_CONTROL_1_REGISTER, 0x00); // Default/reset value
	SetAccRegister(ACC_CONTROL_2_REGISTER, 0x3F); // Default/reset value
	SetAccRegister(ACC_CONTROL_3_REGISTER, 0xA8); // Default/reset value
	SetAccRegister(ACC_CONTROL_4_REGISTER, 0x40); // Default/reset value
	SetAccRegister(ACC_CONTROL_5_REGISTER, 0x00); // Default/reset value
	SetAccRegister(ACC_CONTROL_6_REGISTER, 0x00); // Default/reset value
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestAccelerometer(void)
{
	debug("Accelerometer: Test device access...\r\n");

	VerifyAccManuIDAndPartID();

	if (VerifyAccCommandTestResponse() == PASSED) { debug("Acc: Command Test response passed\r\n"); }
	else { debugErr("Acc: Command Test response failed\r\n"); }

	debug("Accelerometer: Setting manual sleep\r\n");
	SetAccRegister(ACC_CONTROL_5_REGISTER, 0x01);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void IssueAccSoftwareReset(void)
{
	uint8_t registerAddrAndData[2];
	uint8_t readVal;

	registerAddrAndData[0] = 0x7F;
	registerAddrAndData[1] = 0x00;

	debug("Accelerometer: Attempting software reset to get slave address back to it's default...\r\n");

	// Write 0x00 to addr 0x7F for ack/nack
	//if (WriteI2CDevice(MXC_I2C0, accelerometerI2CAddr, &registerAddrAndData[0], sizeof(registerAddrAndData), NULL, 0) == E_SUCCESS)
	if (SetAccRegister(registerAddrAndData[0], registerAddrAndData[1]) == E_SUCCESS)
	{
		// Write 0x00 to addr 0x1C looking for ack
		registerAddrAndData[0] = 0x1C;
		registerAddrAndData[1] = 0x00;

		//if (WriteI2CDevice(MXC_I2C0, accelerometerI2CAddr, &registerAddrAndData[0], sizeof(registerAddrAndData), NULL, 0) == E_SUCCESS)
		if (SetAccRegister(registerAddrAndData[0], registerAddrAndData[1]) == E_SUCCESS)
		{
			// Write 0x80 to addr 0x1C looking for ack
			registerAddrAndData[0] = 0x1C;
			registerAddrAndData[1] = 0x80;

			//if (WriteI2CDevice(MXC_I2C0, accelerometerI2CAddr, &registerAddrAndData[0], sizeof(registerAddrAndData), NULL, 0) == E_SUCCESS)
			if (SetAccRegister(registerAddrAndData[0], registerAddrAndData[1]) == E_SUCCESS)
			{
				// Wait for device reset
				MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(50));

				// Read addr 0x13
				registerAddrAndData[0] = 0x13;

				//if (WriteI2CDevice(MXC_I2C0, I2C_ADDR_ACCELEROMETER, &registerAddrAndData[0], sizeof(uint8_t), &readVal, sizeof(uint8_t)) == E_SUCCESS)
				if (SetAndReadAccRegister(I2C_ADDR_ACCELEROMETER, registerAddrAndData[0], &readVal, sizeof(uint8_t)) == E_SUCCESS)
				{
					if (readVal == 0x46)
					{
						debug("Accelerometer: Slave address changed to default\r\n");
						accelerometerI2CAddr = I2C_ADDR_ACCELEROMETER;

						registerAddrAndData[0] = 0x12;

						//if (WriteI2CDevice(MXC_I2C0, I2C_ADDR_ACCELEROMETER, &registerAddrAndData[0], sizeof(uint8_t), &readVal, sizeof(uint8_t)) == E_SUCCESS)
						if (SetAndReadAccRegister(I2C_ADDR_ACCELEROMETER, registerAddrAndData[0], &readVal, sizeof(uint8_t)) == E_SUCCESS)
						{
							if (readVal == 0x55)
							{
							   debug("Accelerometer: Device operation can be started\r\n");
							}
							else { debugErr("Accelerometer: Failed software reset step 7r\n"); }
						}
						else { debugErr("Accelerometer: Failed software reset step 6r\n"); }
					}
					else { debugErr("Accelerometer: Failed software reset step 5r\n"); }
				}
				else { debugErr("Accelerometer: Failed software reset step 4r\n"); }
			}
			else { debugErr("Accelerometer: Failed software reset step 3\r\n"); }
		}
		else { debugErr("Accelerometer: Failed software reset step 2\r\n"); }
	}
	else { debugErr("Accelerometer: Failed software reset step 1\r\n"); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AccelerometerInit(void)
{
	SetupSPI2_Accelerometer(ON);

#if /* Old board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_PROTOTYPE_1)
	// Issue software reset if the Acc slave address is anything other than the primary defualt 0x1E */
	if (accelerometerI2CAddr != I2C_ADDR_ACCELEROMETER)
	{
		debug("Accelerometer: Attempting software reset...\r\n");
		IssueAccSoftwareReset();
	}
#endif

#if 0 /* Test */
	//while (1)
	{
		VerifyAccManuIDAndPartID();
		SoftUsecWait(1 * SOFT_SECS);
	}
#endif

#if 0 /* Test */
	uint8_t manuIDAndPartID[6];
	uint8_t registerAddress = ACC_MAN_ID_REGISTER;
	//uint8_t status = FAILED;

	debug("Accelerometer: Attempting read of ManID...\r\n");
	while (1)
	{
		VerifyAccManuIDAndPartID();
		SoftUsecWait(1 * SOFT_SECS);
#if 0
	MXC_SPI_SetMode(MXC_SPI2, SPI_MODE_0);
	GetAccRegister(registerAddress, &manuIDAndPartID[0], sizeof(uint8_t));
	if (manuIDAndPartID[0] == 'K') { debug("Accelerometer: ManID first letter read successfully\r\n"); }
	else { debug("Accelerometer: ManID first letter read failed with 0x%02x\r\n", manuIDAndPartID[0]); }
	SoftUsecWait(1 * SOFT_SECS);
#endif
#if 0
	MXC_SPI_SetMode(MXC_SPI2, SPI_MODE_1);
	GetAccRegister(registerAddress, &manuIDAndPartID[0], sizeof(uint8_t));
	if (manuIDAndPartID[0] == 'K') { debug("Accelerometer: ManID first letter read successfully\r\n"); }
	else { debug("Accelerometer: ManID first letter read failed with 0x%02x\r\n", manuIDAndPartID[0]); }
	SoftUsecWait(1 * SOFT_SECS);
#endif
#if 0
	MXC_SPI_SetMode(MXC_SPI2, SPI_MODE_2);
	GetAccRegister(registerAddress, &manuIDAndPartID[0], sizeof(uint8_t));
	if (manuIDAndPartID[0] == 'K') { debug("Accelerometer: ManID first letter read successfully\r\n"); }
	else { debug("Accelerometer: ManID first letter read failed with 0x%02x\r\n", manuIDAndPartID[0]); }
	SoftUsecWait(1 * SOFT_SECS);
#endif
#if 0
	MXC_SPI_SetMode(MXC_SPI2, SPI_MODE_3);
	GetAccRegister(registerAddress, &manuIDAndPartID[0], sizeof(uint8_t));
	if (manuIDAndPartID[0] == 'K') { debug("Accelerometer: ManID first letter read successfully\r\n"); }
	else { debug("Accelerometer: ManID first letter read failed with 0x%02x\r\n", manuIDAndPartID[0]); }
	SoftUsecWait(1 * SOFT_SECS);
#endif
	}
	// LCD controller uses SPI Mode 0 only, so reset
	MXC_SPI_SetMode(MXC_SPI2, SPI_MODE_0);
#endif

	// Attempt to verify the part is present
	if (VerifyAccManuIDAndPartID() == PASSED)
	{
		// Put the Acc in standby mode to allow changing control register values
		SetAccRegister(ACC_CONTROL_1_REGISTER, 0x00);

#if 0 /* Test */
		// Reset interrupt controls
		SetAccRegister(ACC_INTERRUPT_CONTROL_1_REGISTER, 0x10);
		SetAccRegister(ACC_INTERRUPT_CONTROL_5_REGISTER, 0x10);
#endif

		uint8_t testData;
		GetAccRegister(ACC_CONTROL_2_REGISTER, &testData, sizeof(uint8_t));
		if (testData == 0x3F) { debug("Accelerometer: Ctrl2 is default value\r\n"); }
		else { debug("Accelerometer: Ctrl2 is non-default value of 0x%x\r\n", testData); }

		GetAccRegister(ACC_CONTROL_3_REGISTER, &testData, sizeof(uint8_t));
		if (testData == 0xA8) { debug("Accelerometer: Ctrl3 is default value\r\n"); }
		else { debug("Accelerometer: Ctrl3 is non-default value of 0x%x\r\n", testData); }
		SetAccRegister(ACC_CONTROL_3_REGISTER, 0x00);
		GetAccRegister(ACC_CONTROL_3_REGISTER, &testData, sizeof(uint8_t));
		if (testData == 0) { debug("Accelerometer: Ctrl3 was changed successfully\r\n"); }
		else { debug("Accelerometer: Ctrl3 not changed correctly, returns a non-zero value of 0x%x\r\n", testData); }
		//SetAccRegister(ACC_CONTROL_3_REGISTER, 0xA8);

		GetAccRegister(ACC_CONTROL_4_REGISTER, &testData, sizeof(uint8_t));
		if (testData == 0x40) { debug("Accelerometer: Ctrl4 is default value\r\n"); }
		else { debug("Accelerometer: Ctrl4 is non-default value of 0x%x\r\n", testData); }

#if 0 /* Test max Acc sample rate over I2C */
		StartAccelerometerAquisition();
extern volatile uint32_t g_lifetimePeriodicSecondCount;
		volatile uint32_t secCount = g_lifetimePeriodicSecondCount;
		uint32_t loopCount = 0;
		ACC_DATA_STRUCT channelData;

		while (secCount == g_lifetimePeriodicSecondCount) {}
		secCount = g_lifetimePeriodicSecondCount + 3;

		while (secCount != g_lifetimePeriodicSecondCount)
		{
			GetAccelerometerChannelData(&channelData);
			loopCount++;
		}
		StopAccelerometerAquisition();

		debug("Accelerometer: Fastest sample rate is %0.2f\r\n", (double)((float)loopCount / (float)3));
#endif

#if 0 /* Attempt Self test */
		// 1. Set STPOL bit to 1 in INC1 register to set the polarity of the self-test mode to positive.
		// 2. Set PC1 bit to 1 in CNTL1 register to enable KX134-1211.
		// 3. Write 0xCA to this register to enable the MEMS self-test function.
		debug("Accelerometer: Starting Self-test...\r\n");
		GetAccRegister(ACC_INTERRUPT_CONTROL_1_REGISTER, &testData, sizeof(uint8_t));
		testData |= 0x02;
		SetAccRegister(ACC_INTERRUPT_CONTROL_1_REGISTER, testData);

		GetAccRegister(ACC_CONTROL_1_REGISTER, &testData, sizeof(uint8_t));
		testData |= 0x80;
		SetAccRegister(ACC_CONTROL_1_REGISTER, testData);

		SetAccRegister(ACC_SELF_TEST_REGISTER, 0xCA);

		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(5));

		SetAccRegister(ACC_CONTROL_1_REGISTER, 0x00);
		SetAccRegister(ACC_SELF_TEST_REGISTER, 0x00);
		debug("Accelerometer: Self-test disabled\r\n");
		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(1));
#endif

#if 0 /* Test reading data */
		StartAccelerometerAquisition();
		ACC_DATA_STRUCT channelData;

		//for (uint8_t i = 0; i < 4; i++)
		while (1)
		{
			GetAccelerometerChannelData(&channelData);
			debug("Accelerometer (%s): X:%04x Y:%04x Z:%04x\r\n", FuelGaugeDebugString(), channelData.x, channelData.y, channelData.z);
			MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(250));
		}

		StopAccelerometerAquisition();
#endif

#if 0 /* Test Accel Trig control */
		while (1)
		{
			SetAccelerometerTriggerState(ON);
			MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(1));
			SetAccelerometerTriggerState(OFF);
			MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(1));
		}
#endif

#if 0 /* Test interrupts */
		debug("Accelerometer: Both interrupts setup\r\n");
		SetAccRegister(ACC_INTERRUPT_CONTROL_1_REGISTER, 0x38);
		SetAccRegister(ACC_INTERRUPT_CONTROL_4_REGISTER, 0xFF);
		SetAccRegister(ACC_INTERRUPT_CONTROL_5_REGISTER, 0x38);
		SetAccRegister(ACC_INTERRUPT_CONTROL_6_REGISTER, 0xFF);

		SetAccRegister(ACC_CONTROL_4_REGISTER, 0x70);

		GetAccRegister(ACC_INTERRUPT_RELEASE_REGISTER, &testData);

		debug("Accelerometer: Start operating...\r\n");
		GetAccRegister(ACC_CONTROL_1_REGISTER, &testData, sizeof(uint8_t));
		testData |= 0xE5;
		SetAccRegister(ACC_CONTROL_1_REGISTER, testData);

		//MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_SEC(8));
extern volatile uint8_t testAccInt;
		uint8_t is1, is2, is3, count = 0;
		SetAccelerometerTriggerState(ON);
		MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(5));
		SetAccelerometerTriggerState(OFF);
		while (1)
		{
			if (testAccInt)
			{
				GetAccRegister(ACC_INTERRUPT_SOURCE_1_REGISTER, &is1, sizeof(uint8_t));
				GetAccRegister(ACC_INTERRUPT_SOURCE_2_REGISTER, &is2, sizeof(uint8_t));
				GetAccRegister(ACC_INTERRUPT_SOURCE_3_REGISTER, &is3, sizeof(uint8_t));
				GetAccRegister(ACC_INTERRUPT_RELEASE_REGISTER, &testData, sizeof(uint8_t));
				debug("Accelerometer: Interrupt source regs: 0x%x 0x%x 0x%x\r\n", is1, is2, is3);
				testAccInt = 0;

				if (++count > 20) break;
			}
		}
#endif

#if 0 /* Test */
		// Reset interrupt controls
		SetAccRegister(ACC_INTERRUPT_CONTROL_1_REGISTER, 0x10);
		SetAccRegister(ACC_INTERRUPT_CONTROL_5_REGISTER, 0x10);
#endif

		// Put the Accelerometer in manual sleep
		SetAccRegister(ACC_CONTROL_5_REGISTER, 0x01);
		debug("Accelerometer: Manual sleep engaged\r\n");
	}

	SetupSPI2_Accelerometer(OFF);
}
#endif /* KX134 */

#if /* New board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_PRODUCTION) /* IIM-42352 */
/*
 * ________________________________________________________________________________________________________
   Copyright (C) [2022] by InvenSense, Inc.
   Permission to use, copy, modify, and/or distribute this software for any purpose
   with or without fee is hereby granted.

   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
   REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
   FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
   INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
   OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
   TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
   THIS SOFTWARE.
 * ________________________________________________________________________________________________________
 */

#ifndef _INV_IIM423XX_DEFS_H_
#define _INV_IIM423XX_DEFS_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @file Iim423xxDefs.h
 * File exposing the device register map
 */

#include <stdint.h>

/* List whoami values for all iim423xx variants*/
#define IIM42352_WHOAMI 0x6D

/* Define whoami value for the targeted product and make sure the target is valid */
#define ICM_WHOAMI IIM42352_WHOAMI


/** @brief ICM family definition
 *  Possible values are ICM_FAMILY_CPLUS or ICM_FAMILY_BPLUS
 */

	#define ICM_FAMILY_CPLUS



/* ----------------------------------------------------------------------------
 * Device Register map
 *
 * Next macros defines address for all iim423xx registers as listed by device
 * datasheet.
 * Macros name is MPUREG_<REGISTER_NAME> with REGISTER_NAME being the name of
 * the corresponding register in datasheet.
 * Note that macro name is MPUREG_<REGISTER_NAME>_Bx with x being the bank
 * number for registers that are in bank 1 and 2 (suffix is ommitted for
 * bank0 registers)
 * ---------------------------------------------------------------------------- */

/* Bank 0 */
#define MPUREG_DEVICE_CONFIG      0x11
#define MPUREG_CHIP_CONFIG        MPUREG_DEVICE_CONFIG // Retro-compatibility
#define MPUREG_DRIVE_CONFIG       0x13
#define MPUREG_INT_CONFIG         0x14
#define MPUREG_FIFO_CONFIG        0x16
#define MPUREG_TEMP_DATA1_UI      0x1D
#define MPUREG_ACCEL_DATA_X0_UI   0x1F

#define MPUREG_TMST_FSYNCH        0x2B
#define MPUREG_TMST_FSYNC1        MPUREG_TMST_FSYNCH // Retro-compatibility
#define MPUREG_INT_STATUS         0x2D
#define MPUREG_FIFO_COUNTH        0x2E
#define MPUREG_FIFO_BYTE_COUNT1   MPUREG_FIFO_COUNTH // Retro-compatibility
#define MPUREG_FIFO_COUNTL        0x2F
#define MPUREG_FIFO_BYTE_COUNT2   MPUREG_FIFO_COUNTL // Retro-compatibility
#define MPUREG_FIFO_DATA          0x30
#define MPUREG_APEX_DATA0         0x31
#define MPUREG_APEX_DATA1         0x32
#define MPUREG_APEX_DATA2         0x33
#define MPUREG_APEX_DATA3         0x34
#define MPUREG_APEX_DATA4         0x35
#define MPUREG_APEX_DATA5         0x36
#define MPUREG_INT_STATUS2        0x37
#define MPUREG_INT_STATUS3        0x38
#define MPUREG_SIGNAL_PATH_RESET  0x4B
#define MPUREG_INTF_CONFIG0       0x4C
#define MPUREG_INTF_CONFIG1       0x4D
#define MPUREG_PWR_MGMT_0         0x4E

#define MPUREG_ACCEL_CONFIG0      0x50

#define MPUREG_TEMP_FILT_CONFIG       0x51

#define MPUREG_ACCEL_FILT_CONFIG 0x52
#define MPUREG_ACCEL_CONFIG1      0x53
#define MPUREG_TMST_CONFIG        0x54
#define MPUREG_APEX_CONFIG0       0x56
#define MPUREG_SMD_CONFIG         0x57
#define MPUREG_FIFO_CONFIG1       0x5F
#define MPUREG_FIFO_CONFIG2       0x60
#define MPUREG_FIFO_CONFIG3       0x61
#define MPUREG_FSYNC_CONFIG       0x62
#define MPUREG_INT_CONFIG0        0x63
#define MPUREG_INT_CONFIG1        0x64
#define MPUREG_INT_SOURCE0        0x65
#define MPUREG_INT_SOURCE1        0x66
#define MPUREG_INT_SOURCE2        0x67
#define MPUREG_INT_SOURCE3        0x68
#define MPUREG_INT_SOURCE4        0x69
#define MPUREG_FIFO_LOST_PKT0     0x6C
#define MPUREG_FIFO_LOST_PKT1     0x6D
#define MPUREG_SELF_TEST_CONFIG   0x70
#define MPUREG_WHO_AM_I           0x75
#define MPUREG_REG_BANK_SEL       0x76

/* Bank 1 */
#define MPUREG_SENSOR_CONFIG0_B1      0x03

#define MPUREG_TMST_VAL0_B1           0x62
#define MPUREG_TMST_VAL1_B1           0x63
#define MPUREG_TMST_VAL2_B1           0x64
#define MPUREG_INTF_CONFIG4_B1        0x7A
#define MPUREG_INTF_CONFIG5_B1        0x7B
#define MPUREG_INTF_CONFIG6_B1        0x7C

/* Bank 2 */
#define MPUREG_ACCEL_CONFIG_STATIC2_B2 0x03
#define MPUREG_ACCEL_CONFIG_STATIC3_B2 0x04
#define MPUREG_ACCEL_CONFIG_STATIC4_B2 0x05
#define MPUREG_XA_ST_DATA_B2           0x3B
#define MPUREG_YA_ST_DATA_B2           0x3C
#define MPUREG_ZA_ST_DATA_B2           0x3D


/* Bank 3 */
#define MPUREG_PU_PD_CONFIG1_B3   0x06
#define MPUREG_PU_PD_CONFIG2_B3   0x0E

/* Bank 4 */
#define MPUREG_FDR_CONFIG_B4      0x09
#define MPUREG_APEX_CONFIG1_B4    0x40
#define MPUREG_APEX_CONFIG2_B4    0x41
#define MPUREG_APEX_CONFIG3_B4    0x42
#define MPUREG_APEX_CONFIG4_B4    0x43
#define MPUREG_APEX_CONFIG5_B4    0x44
#define MPUREG_APEX_CONFIG6_B4    0x45
#define MPUREG_APEX_CONFIG7_B4    0x46
#define MPUREG_APEX_CONFIG8_B4    0x47
#define MPUREG_APEX_CONFIG9_B4    0x48
#define MPUREG_APEX_CONFIG10_B4   0x49
#define MPUREG_ACCEL_WOM_X_THR_B4 0x4A
#define MPUREG_ACCEL_WOM_Y_THR_B4 0x4B
#define MPUREG_ACCEL_WOM_Z_THR_B4 0x4C
#define MPUREG_INT_SOURCE6_B4     0x4D
#define MPUREG_INT_SOURCE7_B4     0x4E
#define MPUREG_INT_SOURCE8_B4     0x4F
#define MPUREG_INT_SOURCE9_B4     0x50
#define MPUREG_INT_SOURCE10_B4    0x51
#define MPUREG_OFFSET_USER_4_B4   0x7B
#define MPUREG_OFFSET_USER_5_B4   0x7C
#define MPUREG_OFFSET_USER_6_B4   0x7D
#define MPUREG_OFFSET_USER_7_B4   0x7E
#define MPUREG_OFFSET_USER_8_B4   0x7F

/* ----------------------------------------------------------------------------
 * Device features
 *
 * Next macros define some of the device features such as FIFO, sensor data
 * size or whoami value.
 * ---------------------------------------------------------------------------- */

#define ACCEL_DATA_SIZE               6

#define RESERVED_DATA_SIZE                6
#define TEMP_DATA_SIZE                2

#define FIFO_HEADER_SIZE              1
#define FIFO_ACCEL_DATA_SIZE          ACCEL_DATA_SIZE

#define FIFO_RESERVED_DATA_SIZE           RESERVED_DATA_SIZE
#define FIFO_TEMP_DATA_SIZE           1
#define FIFO_TS_FSYNC_SIZE            2
#define FIFO_TEMP_HIGH_RES_SIZE       1

#define FIFO_ACCEL_RESERVED_HIGH_RES_SIZE 3

#define FIFO_16BYTES_PACKET_SIZE      (FIFO_HEADER_SIZE + FIFO_ACCEL_DATA_SIZE + FIFO_RESERVED_DATA_SIZE + FIFO_TEMP_DATA_SIZE + FIFO_TS_FSYNC_SIZE)
#define FIFO_20BYTES_PACKET_SIZE      (FIFO_HEADER_SIZE + FIFO_ACCEL_DATA_SIZE + FIFO_RESERVED_DATA_SIZE + FIFO_TEMP_DATA_SIZE + FIFO_TS_FSYNC_SIZE +\
										FIFO_TEMP_HIGH_RES_SIZE + FIFO_ACCEL_RESERVED_HIGH_RES_SIZE)

#define FIFO_HEADER_ODR_ACCEL         0x01

#define FIFO_HEADER_FSYNC             0x04
#define FIFO_HEADER_TMST              0x08
#define FIFO_HEADER_HEADER_20         0x10

#define FIFO_HEADER_ACC               0x40
#define FIFO_HEADER_MSG               0x80


#define INVALID_VALUE_FIFO              ((int16_t)0x8000)
#define INVALID_VALUE_FIFO_1B           ((int8_t)0x80)

/** Describe the content of the FIFO header */
typedef union
{
	unsigned char Byte;
	struct
	{
		unsigned char reserved_odr_different : 1;
		unsigned char accel_odr_different : 1;
		unsigned char fsync_bit : 1;
		unsigned char timestamp_bit : 1;
		unsigned char twentybits_bit : 1;
		unsigned char reserved_bit : 1;
		unsigned char accel_bit : 1;
		unsigned char msg_bit : 1;
	}bits;
} fifo_header_t;

#define I3C_IBI_PAYLOAD_ALL           0xFF
#define I3C_IBI_PAYLOAD_TIMEC         0x80
#define I3C_IBI_PAYLOAD_CAT_MISC      0x40
#define I3C_IBI_PAYLOAD_CAT_ERROR     0x20
#define I3C_IBI_PAYLOAD_CAT_APEX2     0x10
#define I3C_IBI_PAYLOAD_CAT_APEX1     0x08
#define I3C_IBI_PAYLOAD_CAT_OIS1_DRDY 0x04
#define I3C_IBI_PAYLOAD_CAT_FIFO      0x02
#define I3C_IBI_PAYLOAD_CAT_UI_DRDY   0x01


/* ----------------------------------------------------------------------------
 * Device registers description
 *
 * Next section defines some of the registers bitfield and declare corresponding
 * accessors.
 * Note that descriptors and accessors are not provided for all the registers
 * but only for the most useful ones.
 * For all details on registers and bitfields functionalities please refer to
 * the device datasheet.
 * ---------------------------------------------------------------------------- */


/* ---------------------------------------------------------------------------
 * register bank 0
 * ---------------------------------------------------------------------------- */

/*
 * MPUREG_DEVICE_CONFIG
 * Register Name : DEVICE_CONFIG
 */

/* SPI_MODE */
#define BIT_DEVICE_CONFIG_SPI_MODE_POS     4
#define BIT_DEVICE_CONFIG_SPI_MODE_MASK    (0x1 << BIT_DEVICE_CONFIG_SPI_MODE_POS)
#define BIT_CHIP_CONFIG_SPI_MODE_MASK      BIT_DEVICE_CONFIG_SPI_MODE_MASK			//retro-compatibility
#define BIT_CHIP_CONFIG_SPI_MODE_POS       BIT_DEVICE_CONFIG_SPI_MODE_POS			//retro-compatibility

typedef enum
{
	IIM423XX_DEVICE_CONFIG_SPI_MODE_1_2 = (0x1 << BIT_DEVICE_CONFIG_SPI_MODE_POS),
	IIM423XX_DEVICE_CONFIG_SPI_MODE_0_3 = (0x0 << BIT_DEVICE_CONFIG_SPI_MODE_POS),
} IIM423XX_DEVICE_CONFIG_SPI_MODE_t;

typedef enum
{
	IIM423XX_CHIP_CONFIG_SPI_MODE_1_2 = (0x1 << BIT_CHIP_CONFIG_SPI_MODE_POS),
	IIM423XX_CHIP_CONFIG_SPI_MODE_0_3 = (0x0 << BIT_CHIP_CONFIG_SPI_MODE_POS),
} IIM423XX_CHIP_CONFIG_SPI_MODE_t;													//retro-compatibility


/* SOFT_RESET */
#define BIT_DEVICE_CONFIG_RESET_POS     0
#define BIT_DEVICE_CONFIG_RESET_MASK    0x01
#define BIT_CHIP_CONFIG_RESET_MASK      BIT_DEVICE_CONFIG_RESET_MASK				//retro-compatibility
#define BIT_CHIP_CONFIG_RESET_POS       BIT_DEVICE_CONFIG_RESET_POS					//retro-compatibility

typedef enum
{
	IIM423XX_DEVICE_CONFIG_RESET_EN   = 0x01,
	IIM423XX_DEVICE_CONFIG_RESET_NONE = 0x00,
} IIM423XX_DEVICE_CONFIG_RESET_t;

typedef enum
{
	IIM423XX_CHIP_CONFIG_RESET_EN   = 0x01,
	IIM423XX_CHIP_CONFIG_RESET_NONE = 0x00,
} IIM423XX_CHIP_CONFIG_RESET_t;														//retro-compatibility

/*
 * MPUREG_INT_CONFIG
 * Register Name: INT_CONFIG
 */

/* INT2_DRIVE_CIRCUIT */
#define BIT_INT_CONFIG_INT2_DRIVE_CIRCUIT_POS        4
#define BIT_INT_CONFIG_INT2_DRIVE_CIRCUIT_MASK   (0x01 << BIT_INT_CONFIG_INT2_DRIVE_CIRCUIT_POS)

typedef enum
{
	IIM423XX_INT_CONFIG_INT2_DRIVE_CIRCUIT_PP = (0x01 << BIT_INT_CONFIG_INT2_DRIVE_CIRCUIT_POS),
	IIM423XX_INT_CONFIG_INT2_DRIVE_CIRCUIT_OD = (0x00 << BIT_INT_CONFIG_INT2_DRIVE_CIRCUIT_POS),
} IIM423XX_INT_CONFIG_INT2_DRIVE_CIRCUIT_t;

/* INT2_POLARITY */
#define BIT_INT_CONFIG_INT2_POLARITY_POS        3
#define BIT_INT_CONFIG_INT2_POLARITY_MASK   (0x01 << BIT_INT_CONFIG_INT2_POLARITY_POS)

typedef enum
{
	IIM423XX_INT_CONFIG_INT2_POLARITY_HIGH = (0x01 << BIT_INT_CONFIG_INT2_POLARITY_POS),
	IIM423XX_INT_CONFIG_INT2_POLARITY_LOW  = (0x00 << BIT_INT_CONFIG_INT2_POLARITY_POS),
} IIM423XX_INT_CONFIG_INT2_POLARITY_t;

/* INT1_DRIVE_CIRCUIT */
#define BIT_INT_CONFIG_INT1_DRIVE_CIRCUIT_POS        1
#define BIT_INT_CONFIG_INT1_DRIVE_CIRCUIT_MASK   (0x01 << BIT_INT_CONFIG_INT1_DRIVE_CIRCUIT_POS)

typedef enum
{
	IIM423XX_INT_CONFIG_INT1_DRIVE_CIRCUIT_PP = (0x01 << BIT_INT_CONFIG_INT1_DRIVE_CIRCUIT_POS),
	IIM423XX_INT_CONFIG_INT1_DRIVE_CIRCUIT_OD = (0x00 << BIT_INT_CONFIG_INT1_DRIVE_CIRCUIT_POS),
} IIM423XX_INT_CONFIG_INT1_DRIVE_CIRCUIT_t;

/* INT1_POLARITY */
#define BIT_INT_CONFIG_INT1_POLARITY_POS       0
#define BIT_INT_CONFIG_INT1_POLARITY_MASK   0x01

typedef enum
{
	IIM423XX_INT_CONFIG_INT1_POLARITY_HIGH = 0x01,
	IIM423XX_INT_CONFIG_INT1_POLARITY_LOW  = 0x00,
} IIM423XX_INT_CONFIG_INT1_POLARITY_t;

/*
 * MPUREG_FIFO_CONFIG
 * Register Name: FIFO_CONFIG
 */

/* FIFO_MODE */
#define BIT_FIFO_CONFIG_MODE_POS        6
#define BIT_FIFO_CONFIG_MODE_MASK   (0x03 << BIT_FIFO_CONFIG_MODE_POS)

typedef enum
{
	IIM423XX_FIFO_CONFIG_MODE_STOP_ON_FULL  = (0x02 << BIT_FIFO_CONFIG_MODE_POS),
	IIM423XX_FIFO_CONFIG_MODE_STREAM        = (0x01 << BIT_FIFO_CONFIG_MODE_POS),
	IIM423XX_FIFO_CONFIG_MODE_BYPASS        = (0x00 << BIT_FIFO_CONFIG_MODE_POS),
} IIM423XX_FIFO_CONFIG_MODE_t;

#define IIM423XX_FIFO_CONFIG_MODE_SNAPSHOT = IIM423XX_FIFO_CONFIG_MODE_STOP_ON_FULL; // For retro-compatibility
/*
 * MPUREG_INT_STATUS
 * Register Name: INT_STATUS
 */
#define BIT_INT_STATUS_UI_FSYNC   0x40
#define BIT_INT_STATUS_PLL_RDY    0x20
#define BIT_INT_STATUS_RESET_DONE 0x10
#define BIT_INT_STATUS_DRDY       0x08
#define BIT_INT_STATUS_FIFO_THS   0x04
#define BIT_INT_STATUS_FIFO_FULL  0x02
#define BIT_INT_STATUS_AGC_RDY    0x01

/*
 * MPUREG_APEX_DATA0
 * MPUREG_APEX_DATA1
 * MPUREG_APEX_DATA2
 * MPUREG_APEX_DATA3
 * Register Name: APEX_DATA0
 * Register Name: APEX_DATA1
 * Register Name: APEX_DATA2
 * Register Name: APEX_DATA3
 */

/* ACTIVITY_CLASS */
#define BIT_APEX_DATA3_ACTIVITY_CLASS_POS       0
#define BIT_APEX_DATA3_ACTIVITY_CLASS_MASK   0x03

typedef enum
{
	IIM423XX_APEX_DATA3_ACTIVITY_CLASS_OTHER = 0x0,
	IIM423XX_APEX_DATA3_ACTIVITY_CLASS_WALK  = 0x1,
	IIM423XX_APEX_DATA3_ACTIVITY_CLASS_RUN   = 0x2,
} IIM423XX_APEX_DATA3_ACTIVITY_CLASS_t;

/* DMP_IDLE */
#define BIT_APEX_DATA3_DMP_IDLE_POS        2
#define BIT_APEX_DATA3_DMP_IDLE_MASK   (0x01 << BIT_APEX_DATA3_DMP_IDLE_POS)

typedef enum
{
	IIM423XX_APEX_DATA3_DMP_IDLE_ON     = (0x01 << BIT_APEX_DATA3_DMP_IDLE_POS),
	IIM423XX_APEX_DATA3_DMP_IDLE_OFF    = (0x00 << BIT_APEX_DATA3_DMP_IDLE_POS),
} IIM423XX_APEX_DATA3_DMP_IDLE_OFF_t;

/*
 * MPUREG_APEX_DATA4
 * Register Name: APEX_DATA4
 */

/** TAP status flags: non-zero value - tap detected
bit0 - positive or negative edge
bit1 and 2 - axis detected : 0-X ; 1-Y ; 2-Z
bit3 and 4 - tap type : 1-single ; 2 -double
*/

/* TAP_NUM */
#define BIT_APEX_DATA4_TAP_NUM_POS        3
#define BIT_APEX_DATA4_TAP_NUM_MASK   (0x03 << BIT_APEX_DATA4_TAP_NUM_POS)
typedef enum
{
	IIM423XX_APEX_DATA4_TAP_NUM_DOUBLE = (0x02 << BIT_APEX_DATA4_TAP_NUM_POS),
	IIM423XX_APEX_DATA4_TAP_NUM_SINGLE = (0x01 << BIT_APEX_DATA4_TAP_NUM_POS),
} IIM423XX_APEX_DATA4_TAP_NUM_t;

/* TAP_AXIS */
#define BIT_APEX_DATA4_TAP_AXIS_POS        1
#define BIT_APEX_DATA4_TAP_AXIS_MASK   (0x03 << BIT_APEX_DATA4_TAP_AXIS_POS)
typedef enum
{
	IIM423XX_APEX_DATA4_TAP_AXIS_Z = (0x02 << BIT_APEX_DATA4_TAP_AXIS_POS),
	IIM423XX_APEX_DATA4_TAP_AXIS_Y = (0x01 << BIT_APEX_DATA4_TAP_AXIS_POS),
	IIM423XX_APEX_DATA4_TAP_AXIS_X = (0x00 << BIT_APEX_DATA4_TAP_AXIS_POS),
} IIM423XX_APEX_DATA4_TAP_AXIS_t;

/* TAP_DIR */
#define BIT_APEX_DATA4_TAP_DIR_POS       0
#define BIT_APEX_DATA4_TAP_DIR_MASK   0x01
typedef enum
{
	IIM423XX_APEX_DATA4_TAP_DIR_POSITIVE = (0x01 << BIT_APEX_DATA4_TAP_DIR_POS),
	IIM423XX_APEX_DATA4_TAP_DIR_NEGATIVE = (0x00 << BIT_APEX_DATA4_TAP_DIR_POS),
} IIM423XX_APEX_DATA4_TAP_DIR_t;

/*
 * MPUREG_APEX_DATA5
 * Register Name: APEX_DATA5
 */

/* DOUBLE_TAP_TIMING */
#define BIT_APEX_DATA5_DOUBLE_TAP_TIMING_POS      0
#define BIT_APEX_DATA5_DOUBLE_TAP_TIMING_MASK (0x3F << BIT_APEX_DATA5_DOUBLE_TAP_TIMING_POS)

/*
 * MPUREG_INT_STATUS2
 * Register Name: INT_STATUS2
 */
#define BIT_INT_STATUS2_SMD_INT        0x08
#define BIT_INT_STATUS2_WOM_Z_INT      0x04
#define BIT_INT_STATUS2_WOM_Y_INT      0x02
#define BIT_INT_STATUS2_WOM_X_INT      0x01

/*
 * MPUREG_INT_STATUS3
 * Register Name: INT_STATUS3
 */
#define BIT_INT_STATUS3_STEP_DET        0x20
#define BIT_INT_STATUS3_STEP_CNT_OVFL   0x10
#define BIT_INT_STATUS3_TILT_DET        0x08
#if defined(ICM_FAMILY_BPLUS)
#define BIT_INT_STATUS3_WAKE_DET        0x04
#define BIT_INT_STATUS3_SLEEP_DET       0x02
#elif defined(ICM_FAMILY_CPLUS)
#define BIT_INT_STATUS3_LOWG_DET        0x04
#define BIT_INT_STATUS3_FF_DET          0x02
#endif
#define BIT_INT_STATUS3_TAP_DET         0x01

/*
 * MPUREG_SIGNAL_PATH_RESET
 * Register Name: SIGNAL_PATH_RESET
 */

/* DMP_INIT_EN */
#define BIT_SIGNAL_PATH_RESET_DMP_INIT_POS       6
#define BIT_SIGNAL_PATH_RESET_DMP_INIT_MASK  (0x01 << BIT_SIGNAL_PATH_RESET_DMP_INIT_POS)

typedef enum
{
	IIM423XX_SIGNAL_PATH_RESET_DMP_INIT_EN  = (0x01 << BIT_SIGNAL_PATH_RESET_DMP_INIT_POS),
	IIM423XX_SIGNAL_PATH_RESET_DMP_INIT_DIS = (0x00 << BIT_SIGNAL_PATH_RESET_DMP_INIT_POS),
} IIM423XX_SIGNAL_PATH_RESET_DMP_INIT_t;

/* DMP_MEM_RESET_EN */
#define BIT_SIGNAL_PATH_RESET_DMP_MEM_RESET_POS       5
#define BIT_SIGNAL_PATH_RESET_DMP_MEM_RESET_MASK  (0x01 << BIT_SIGNAL_PATH_RESET_DMP_MEM_RESET_POS)

typedef enum
{
	IIM423XX_SIGNAL_PATH_RESET_DMP_MEM_RESET_EN  = (0x01 << BIT_SIGNAL_PATH_RESET_DMP_MEM_RESET_POS),
	IIM423XX_SIGNAL_PATH_RESET_DMP_MEM_RESET_DIS = (0x00 << BIT_SIGNAL_PATH_RESET_DMP_MEM_RESET_POS),
} IIM423XX_SIGNAL_PATH_RESET_DMP_MEM_RESET_t;

/* TMST_STROBE */
#define BIT_SIGNAL_PATH_RESET_TMST_STROBE_POS       2
#define BIT_SIGNAL_PATH_RESET_TMST_STROBE_MASK  (0x01 << BIT_SIGNAL_PATH_RESET_TMST_STROBE_POS)

typedef enum
{
	IIM423XX_SIGNAL_PATH_RESET_TMST_STROBE_EN  = (0x01 << BIT_SIGNAL_PATH_RESET_TMST_STROBE_POS),
	IIM423XX_SIGNAL_PATH_RESET_TMST_STROBE_DIS = (0x00 << BIT_SIGNAL_PATH_RESET_TMST_STROBE_POS),
} IIM423XX_SIGNAL_PATH_RESET_TMST_STROBE_t;

/* FIFO_FLUSH */
#define BIT_SIGNAL_PATH_RESET_FIFO_FLUSH_POS        1
#define BIT_SIGNAL_PATH_RESET_FIFO_FLUSH_MASK   (0x01 << BIT_SIGNAL_PATH_RESET_FIFO_FLUSH_POS)

typedef enum
{
	IIM423XX_SIGNAL_PATH_RESET_FIFO_FLUSH_EN  = (0x01 << BIT_SIGNAL_PATH_RESET_FIFO_FLUSH_POS),
	IIM423XX_SIGNAL_PATH_RESET_FIFO_FLUSH_DIS = (0x00 << BIT_SIGNAL_PATH_RESET_FIFO_FLUSH_POS),
} IIM423XX_SIGNAL_PATH_RESET_FIFO_FLUSH_t;

/*
 * MPUREG_INTF_CONFIG0
 * Register Name: INTF_CONFIG0
 */

/* FIFO_SREG_INVALID_IND */
#define BIT_FIFO_SREG_INVALID_IND_POS        7
#define BIT_FIFO_SREG_INVALID_IND_MASK   (0x01 << BIT_FIFO_SREG_INVALID_IND_POS)

typedef enum
{
	IIM423XX_INTF_CONFIG0_FIFO_SREG_INVALID_IND_DIS = (0x01 << BIT_FIFO_SREG_INVALID_IND_POS),
	IIM423XX_INTF_CONFIG0_FIFO_SREG_INVALID_IND_EN  = (0x00 << BIT_FIFO_SREG_INVALID_IND_POS),
} IIM423XX_INTF_CONFIG0_FIFO_SREG_INVALID_IND_t;

/* FIFO_COUNT_REC */
#define BIT_FIFO_COUNT_REC_POS               6
#define BIT_FIFO_COUNT_REC_MASK          (0x01 << BIT_FIFO_COUNT_REC_POS)

typedef enum
{
	IIM423XX_INTF_CONFIG0_FIFO_COUNT_REC_RECORD = (0x01 << BIT_FIFO_COUNT_REC_POS),
	IIM423XX_INTF_CONFIG0_FIFO_COUNT_REC_BYTE   = (0x00 << BIT_FIFO_COUNT_REC_POS),
} IIM423XX_INTF_CONFIG0_FIFO_COUNT_REC_t;

/* FIFO_COUNT_ENDIAN */
#define BIT_FIFO_COUNT_ENDIAN_POS           5
#define BIT_FIFO_COUNT_ENDIAN_MASK      (0x01 << BIT_FIFO_COUNT_ENDIAN_POS)

typedef enum
{
	IIM423XX_INTF_CONFIG0_FIFO_COUNT_BIG_ENDIAN    = (0x01 << BIT_FIFO_COUNT_ENDIAN_POS),
	IIM423XX_INTF_CONFIG0_FIFO_COUNT_LITTLE_ENDIAN = (0x00 << BIT_FIFO_COUNT_ENDIAN_POS),
} IIM423XX_INTF_CONFIG0_FIFO_COUNT_ENDIAN_t;

/* DATA_ENDIAN */
#define BIT_DATA_ENDIAN_POS                 4
#define BIT_DATA_ENDIAN_MASK            (0x01 << BIT_DATA_ENDIAN_POS)

typedef enum
{
	IIM423XX_INTF_CONFIG0_DATA_BIG_ENDIAN    = (0x01 << BIT_DATA_ENDIAN_POS),
	IIM423XX_INTF_CONFIG0_DATA_LITTLE_ENDIAN = (0x00 << BIT_DATA_ENDIAN_POS),
} IIM423XX_INTF_CONFIG0_DATA_ENDIAN_t;

/* SPI_MODE_OIS2 */
#define BIT_SPI_MODE_OIS2_POS               3
#define BIT_SPI_MODE_OIS2_MASK          (0x01 << BIT_SPI_MODE_OIS2_POS)

typedef enum
{
	IIM423XX_INTF_CONFIG0_SPI_MODE_OIS2_1_2 = (0x01 << BIT_SPI_MODE_OIS2_POS),
	IIM423XX_INTF_CONFIG0_SPI_MODE_OIS2_0_3 = (0x00 << BIT_SPI_MODE_OIS2_POS),
} IIM423XX_INTF_CONFIG0_SPI_MODE_OIS2_t;

/* SPI_MODE_AUX1 */
#define BIT_SPI_MODE_OIS1_POS               2
#define BIT_SPI_MODE_OIS1_MASK          (0x01 << BIT_SPI_MODE_OIS1_POS)

typedef enum
{
	IIM423XX_INTF_CONFIG0_SPI_MODE_OIS1_1_2 = (0x01 << BIT_SPI_MODE_OIS1_POS),
	IIM423XX_INTF_CONFIG0_SPI_MODE_OIS1_0_3 = (0x00 << BIT_SPI_MODE_OIS1_POS),
} IIM423XX_INTF_CONFIG0_SPI_MODE_OIS1_t;

/*
 * MPUREG_INTF_CONFIG1
 * Register Name: INTF_CONFIG1
 */

/* ACCEL_LP_CLK_SEL */
#define BIT_ACCEL_LP_CLK_SEL_POS        3
#define BIT_ACCEL_LP_CLK_SEL_MASK   (0x01 << BIT_ACCEL_LP_CLK_SEL_POS)

typedef enum
{
	IIM423XX_INTF_CONFIG1_ACCEL_LP_CLK_WUOSC = (0x00 << BIT_ACCEL_LP_CLK_SEL_POS),
	IIM423XX_INTF_CONFIG1_ACCEL_LP_CLK_RCOSC = (0x01 << BIT_ACCEL_LP_CLK_SEL_POS),
} IIM423XX_INTF_CONFIG1_ACCEL_LP_CLK_t;

/* RTC_MODE */
#define BIT_RTC_MODE_POS        2
#define BIT_RTC_MODE_MASK   (0x01 << BIT_RTC_MODE_POS)

typedef enum
{
	IIM423XX_INTF_CONFIG1_RTC_MODE_DIS = (0x00 << BIT_RTC_MODE_POS),
	IIM423XX_INTF_CONFIG1_RTC_MODE_EN  = (0x01 << BIT_RTC_MODE_POS),
} IIM423XX_INTF_CONFIG1_RTC_MODE_t;

/*
 * MPUREG_PWR_MGMT_0
 * Register Name: PWR_MGMT_0
 */

/* TEMP_DIS */
#define BIT_PWR_MGMT_0_TEMP_POS        5
#define BIT_PWR_MGMT_0_TEMP_MASK   (0x01 << BIT_PWR_MGMT_0_TEMP_POS)

typedef enum
{
	IIM423XX_PWR_MGMT_0_TEMP_DIS = (0x01 << BIT_PWR_MGMT_0_TEMP_POS),
	IIM423XX_PWR_MGMT_0_TEMP_EN  = (0x00 << BIT_PWR_MGMT_0_TEMP_POS),
} IIM423XX_PWR_MGMT_0_TEMP_t;

/* IDLE */
#define BIT_PWR_MGMT_0_IDLE_POS        4
#define BIT_PWR_MGMT_0_IDLE_MASK   (0x01 << BIT_PWR_MGMT_0_IDLE_POS)

typedef enum
{
	IIM423XX_PWR_MGMT_0_IDLE_DIS = (0x01 << BIT_PWR_MGMT_0_IDLE_POS),
	IIM423XX_PWR_MGMT_0_IDLE_EN  = (0x00 << BIT_PWR_MGMT_0_IDLE_POS),
} IIM423XX_PWR_MGMT_0_IDLE_t;



/* ACCEL_MODE */
#define BIT_PWR_MGMT_0_ACCEL_MODE_POS       0
#define BIT_PWR_MGMT_0_ACCEL_MODE_MASK   0x03

typedef enum
{
	IIM423XX_PWR_MGMT_0_ACCEL_MODE_LN  = 0x03,
	IIM423XX_PWR_MGMT_0_ACCEL_MODE_LP  = 0x02,
	IIM423XX_PWR_MGMT_0_ACCEL_MODE_OFF = 0x00,
} IIM423XX_PWR_MGMT_0_ACCEL_MODE_t;



/*
 * MPUREG_ACCEL_CONFIG0
 * Register Name: ACCEL_CONFIG0
 */

/* ACCEL_FS_SEL */
#define BIT_ACCEL_CONFIG0_FS_SEL_POS       5
#define BIT_ACCEL_CONFIG0_FS_SEL_MASK   (0x7 << BIT_ACCEL_CONFIG0_FS_SEL_POS)

/** @brief Accelerometer FSR selection
 */
typedef enum
{

	IIM423XX_ACCEL_CONFIG0_FS_SEL_RESERVED = (0x4 << BIT_ACCEL_CONFIG0_FS_SEL_POS),
	IIM423XX_ACCEL_CONFIG0_FS_SEL_2g       = (0x3 << BIT_ACCEL_CONFIG0_FS_SEL_POS),  /*!< 2g*/
	IIM423XX_ACCEL_CONFIG0_FS_SEL_4g       = (0x2 << BIT_ACCEL_CONFIG0_FS_SEL_POS),  /*!< 4g*/
	IIM423XX_ACCEL_CONFIG0_FS_SEL_8g       = (0x1 << BIT_ACCEL_CONFIG0_FS_SEL_POS),  /*!< 8g*/
	IIM423XX_ACCEL_CONFIG0_FS_SEL_16g      = (0x0 << BIT_ACCEL_CONFIG0_FS_SEL_POS),  /*!< 16g*/

} IIM423XX_ACCEL_CONFIG0_FS_SEL_t;

/* ACCEL_ODR */
#define BIT_ACCEL_CONFIG0_ODR_POS       0
#define BIT_ACCEL_CONFIG0_ODR_MASK   0x0F

/** @brief Accelerometer ODR selection
 */
typedef enum
{
	IIM423XX_ACCEL_CONFIG0_ODR_500_HZ    = 0xF,  /*!< 500 Hz (2 ms)*/
	IIM423XX_ACCEL_CONFIG0_ODR_1_5625_HZ = 0xE,  /*!< 1.5625 Hz (640 ms)*/
	IIM423XX_ACCEL_CONFIG0_ODR_3_125_HZ  = 0xD,  /*!< 3.125 Hz (320 ms)*/
	IIM423XX_ACCEL_CONFIG0_ODR_6_25_HZ   = 0xC,  /*!< 6.25 Hz (160 ms)*/
	IIM423XX_ACCEL_CONFIG0_ODR_12_5_HZ   = 0xB,  /*!< 12.5 Hz (80 ms)*/
	IIM423XX_ACCEL_CONFIG0_ODR_25_HZ     = 0xA,  /*!< 25 Hz (40 ms)*/
	IIM423XX_ACCEL_CONFIG0_ODR_50_HZ     = 0x9,  /*!< 50 Hz (20 ms)*/
	IIM423XX_ACCEL_CONFIG0_ODR_100_HZ    = 0x8,  /*!< 100 Hz (10 ms)*/
	IIM423XX_ACCEL_CONFIG0_ODR_200_HZ    = 0x7,  /*!< 200 Hz (5 ms)*/
	IIM423XX_ACCEL_CONFIG0_ODR_1_KHZ     = 0x6,  /*!< 1 KHz (1 ms)*/
	IIM423XX_ACCEL_CONFIG0_ODR_2_KHZ     = 0x5,  /*!< 2 KHz (500 us)*/
	IIM423XX_ACCEL_CONFIG0_ODR_4_KHZ     = 0x4,  /*!< 4 KHz (250 us)*/
	IIM423XX_ACCEL_CONFIG0_ODR_8_KHZ     = 0x3,  /*!< 8 KHz (125 us)*/

	IIM423XX_ACCEL_CONFIG0_ODR_16_KHZ    = 0x2,  /*!< 16 KHz (62.5 us)*/
	IIM423XX_ACCEL_CONFIG0_ODR_32_KHZ    = 0x1,  /*!< 32 KHz (31.25 us)*/

} IIM423XX_ACCEL_CONFIG0_ODR_t;

/*
 * MPUREG_TEMP_FILT_CONFIG
 * Register Name: TEMP_FILT_CONFIG
 */

///* TEMP_FILT_BW */
#define BIT_TEMP_FILT_CONFIG_TEMP_FILT_BW_POS        5
#define BIT_TEMP_FILT_CONFIG_TEMP_FILT_BW_MASK    (0x7 << BIT_TEMP_FILT_CONFIG_TEMP_FILT_BW_POS)



/*
 * MPUREG_ACCEL_FILT_CONFIG
 * Register Name: ACCEL_FILT_CONFIG
 */

/* ACCEL_UI_FILT_BW_IND */
#define BIT_ACCEL_FILT_CONFIG_FILT_POS       4
#define BIT_ACCEL_FILT_CONFIG_FILT_MASK   (0xF << BIT_ACCEL_FILT_CONFIG_FILT_POS)

typedef enum
{
	IIM423XX_ACCEL_FILT_CONFIG_FILT_BW_40 = (0x7 << BIT_ACCEL_FILT_CONFIG_FILT_POS),
	IIM423XX_ACCEL_FILT_CONFIG_FILT_BW_20 = (0x6 << BIT_ACCEL_FILT_CONFIG_FILT_POS),
	IIM423XX_ACCEL_FILT_CONFIG_FILT_BW_16 = (0x5 << BIT_ACCEL_FILT_CONFIG_FILT_POS),
	IIM423XX_ACCEL_FILT_CONFIG_FILT_BW_10 = (0x4 << BIT_ACCEL_FILT_CONFIG_FILT_POS),
	IIM423XX_ACCEL_FILT_CONFIG_FILT_BW_8  = (0x3 << BIT_ACCEL_FILT_CONFIG_FILT_POS),
	IIM423XX_ACCEL_FILT_CONFIG_FILT_BW_5  = (0x2 << BIT_ACCEL_FILT_CONFIG_FILT_POS),
	IIM423XX_ACCEL_FILT_CONFIG_FILT_BW_4  = (0x1 << BIT_ACCEL_FILT_CONFIG_FILT_POS),
	IIM423XX_ACCEL_FILT_CONFIG_FILT_BW_2  = (0x0 << BIT_ACCEL_FILT_CONFIG_FILT_POS),
} IIM423XX_ACCEL_FILT_CONFIG_FILT_BW_t;

typedef enum
{
	IIM423XX_ACCEL_FILT_CONFIG_FILT_AVG_16  = (0x6 << BIT_ACCEL_FILT_CONFIG_FILT_POS),
	IIM423XX_ACCEL_FILT_CONFIG_FILT_AVG_1   = (0x1 << BIT_ACCEL_FILT_CONFIG_FILT_POS),
} IIM423XX_ACCEL_FILT_CONFIG_FILT_AVG_t;


/*
 * MPUREG_ACCEL_CONFIG1
 * Register Name: ACCEL_CONFIG1
 */

/* ACCEL_UI_FILT_ORD */
#define BIT_ACCEL_CONFIG1_ACCEL_UI_FILT_ORD_POS       3
#define BIT_ACCEL_CONFIG1_ACCEL_UI_FILT_ORD_MASK    (0x3 << BIT_ACCEL_CONFIG1_ACCEL_UI_FILT_ORD_POS)

typedef enum
{
	IIM423XX_ACCEL_CONFIG_ACCEL_UI_FILT_ORD_1ST_ORDER = (0x0 << BIT_ACCEL_CONFIG1_ACCEL_UI_FILT_ORD_POS),
	IIM423XX_ACCEL_CONFIG_ACCEL_UI_FILT_ORD_2ND_ORDER = (0x1 << BIT_ACCEL_CONFIG1_ACCEL_UI_FILT_ORD_POS),
	IIM423XX_ACCEL_CONFIG_ACCEL_UI_FILT_ORD_3RD_ORDER = (0x2 << BIT_ACCEL_CONFIG1_ACCEL_UI_FILT_ORD_POS),
} IIM423XX_ACCEL_CONFIG_ACCEL_UI_FILT_ORD_t;

/* ACCEL_DEC2_M2_ORD */
#define BIT_ACCEL_CONFIG1_ACCEL_DEC2_M2_ORD_POS       1
#define BIT_ACCEL_CONFIG1_ACCEL_DEC2_M2_ORD_MASK    (0x3 << BIT_ACCEL_CONFIG1_ACCEL_DEC2_M2_ORD_POS)

/*
 * MPUREG_TMST_CONFIG
 * Register Name: TMST_CONFIG
 */

/* TMST_TO_REGS */
#define BIT_TMST_CONFIG_TMST_TO_REGS_EN_POS       4
#define BIT_TMST_CONFIG_TMST_TO_REGS_EN_MASK   (0x1 << BIT_TMST_CONFIG_TMST_TO_REGS_EN_POS)

 typedef enum
{
	IIM423XX_TMST_CONFIG_TMST_TO_REGS_EN   = (0x1 << BIT_TMST_CONFIG_TMST_TO_REGS_EN_POS),
	IIM423XX_TMST_CONFIG_TMST_TO_REGS_DIS  = (0x0 << BIT_TMST_CONFIG_TMST_TO_REGS_EN_POS),
} IIM423XX_TMST_CONFIG_TMST_TO_REGS_EN_t;

/* TMST_RES */
#define BIT_TMST_CONFIG_RESOL_POS        3
#define BIT_TMST_CONFIG_RESOL_MASK    (0x1 << BIT_TMST_CONFIG_RESOL_POS)

typedef enum
{
	IIM423XX_TMST_CONFIG_RESOL_16us = (0x01 << BIT_TMST_CONFIG_RESOL_POS),
	IIM423XX_TMST_CONFIG_RESOL_1us  = (0x00 << BIT_TMST_CONFIG_RESOL_POS),
} IIM423XX_TMST_CONFIG_RESOL_t;

/* TMST_FSYNC */
#define BIT_TMST_CONFIG_TMST_FSYNC_POS        1
#define BIT_TMST_CONFIG_TMST_FSYNC_MASK    (0x1 << BIT_TMST_CONFIG_TMST_FSYNC_POS)

typedef enum
{
	IIM423XX_TMST_CONFIG_TMST_FSYNC_EN  = (0x01 << BIT_TMST_CONFIG_TMST_FSYNC_POS),
	IIM423XX_TMST_CONFIG_TMST_FSYNC_DIS = (0x00 << BIT_TMST_CONFIG_TMST_FSYNC_POS),
} IIM423XX_TMST_CONFIG_TMST_FSYNC_EN_t;

/* TMST_EN */
#define BIT_TMST_CONFIG_TMST_EN_POS       0
#define BIT_TMST_CONFIG_TMST_EN_MASK    0x1

typedef enum
{
	IIM423XX_TMST_CONFIG_TMST_EN  = 0x01,
	IIM423XX_TMST_CONFIG_TMST_DIS = 0x00,
} IIM423XX_TMST_CONFIG_TMST_EN_t;

/*
 * MPUREG_APEX_CONFIG0
 * Register Name: APEX_CONFIG0
 */

/* DMP_POWER_SAVE_EN */
#define BIT_APEX_CONFIG0_DMP_POWER_SAVE_POS       7
#define BIT_APEX_CONFIG0_DMP_POWER_SAVE_MASK   (0x1 << BIT_APEX_CONFIG0_DMP_POWER_SAVE_POS)

 typedef enum
{
	IIM423XX_APEX_CONFIG0_DMP_POWER_SAVE_EN   = (0x1 << BIT_APEX_CONFIG0_DMP_POWER_SAVE_POS),
	IIM423XX_APEX_CONFIG0_DMP_POWER_SAVE_DIS  = (0x0 << BIT_APEX_CONFIG0_DMP_POWER_SAVE_POS),
} IIM423XX_APEX_CONFIG0_DMP_POWER_SAVE_t;

/* TAP_ENABLE */
#define BIT_APEX_CONFIG0_TAP_ENABLE_POS       6
#define BIT_APEX_CONFIG0_TAP_ENABLE_MASK   (0x1 << BIT_APEX_CONFIG0_TAP_ENABLE_POS)

 typedef enum
{
	IIM423XX_APEX_CONFIG0_TAP_ENABLE_EN   = (0x1 << BIT_APEX_CONFIG0_TAP_ENABLE_POS),
	IIM423XX_APEX_CONFIG0_TAP_ENABLE_DIS  = (0x0 << BIT_APEX_CONFIG0_TAP_ENABLE_POS),
} IIM423XX_APEX_CONFIG0_TAP_ENABLE_t;

/* PEDO_EN */
#define BIT_APEX_CONFIG0_PEDO_EN_POS       5
#define BIT_APEX_CONFIG0_PEDO_EN_MASK   (0x1 << BIT_APEX_CONFIG0_PEDO_EN_POS)

 typedef enum
{
	IIM423XX_APEX_CONFIG0_PEDO_EN_EN   = (0x1 << BIT_APEX_CONFIG0_PEDO_EN_POS),
	IIM423XX_APEX_CONFIG0_PEDO_EN_DIS  = (0x0 << BIT_APEX_CONFIG0_PEDO_EN_POS),
} IIM423XX_APEX_CONFIG0_PEDO_EN_t;

#if defined(ICM_FAMILY_BPLUS)

/* R2W_EN */
#define BIT_APEX_CONFIG0_R2W_EN_POS       3
#define BIT_APEX_CONFIG0_R2W_EN_MASK   (0x1 << BIT_APEX_CONFIG0_R2W_EN_POS)

 typedef enum
{
	IIM423XX_APEX_CONFIG0_R2W_EN_EN   = (0x1 << BIT_APEX_CONFIG0_R2W_EN_POS),
	IIM423XX_APEX_CONFIG0_R2W_EN_DIS  = (0x0 << BIT_APEX_CONFIG0_R2W_EN_POS),
} IIM423XX_APEX_CONFIG0_R2W_EN_t;

#elif defined(ICM_FAMILY_CPLUS)

/* FF_EN */
#define BIT_APEX_CONFIG0_FF_EN_POS       2
#define BIT_APEX_CONFIG0_FF_EN_MASK   (0x1 << BIT_APEX_CONFIG0_FF_EN_POS)

 typedef enum
{
	IIM423XX_APEX_CONFIG0_FF_EN_EN   = (0x1 << BIT_APEX_CONFIG0_FF_EN_POS),
	IIM423XX_APEX_CONFIG0_FF_EN_DIS  = (0x0 << BIT_APEX_CONFIG0_FF_EN_POS),
} IIM423XX_APEX_CONFIG0_FF_EN_t;

/* LOWG_EN */
#define BIT_APEX_CONFIG0_LOWG_EN_POS       3
#define BIT_APEX_CONFIG0_LOWG_EN_MASK   (0x1 << BIT_APEX_CONFIG0_LOWG_EN_POS)

 typedef enum
{
	IIM423XX_APEX_CONFIG0_LOWG_EN_EN   = (0x1 << BIT_APEX_CONFIG0_LOWG_EN_POS),
	IIM423XX_APEX_CONFIG0_LOWG_EN_DIS  = (0x0 << BIT_APEX_CONFIG0_LOWG_EN_POS),
} IIM423XX_APEX_CONFIG0_LOWG_EN_t;

#endif

/* TILT_EN */
#define BIT_APEX_CONFIG0_TILT_EN_POS       4
#define BIT_APEX_CONFIG0_TILT_EN_MASK   (0x1 << BIT_APEX_CONFIG0_TILT_EN_POS)

 typedef enum
{
	IIM423XX_APEX_CONFIG0_TILT_EN_EN   = (0x1 << BIT_APEX_CONFIG0_TILT_EN_POS),
	IIM423XX_APEX_CONFIG0_TILT_EN_DIS  = (0x0 << BIT_APEX_CONFIG0_TILT_EN_POS),
} IIM423XX_APEX_CONFIG0_TILT_EN_t;

/* DMP_ODR */
#define BIT_APEX_CONFIG0_DMP_ODR_POS       0
#define BIT_APEX_CONFIG0_DMP_ODR_MASK   (0x3 << BIT_APEX_CONFIG0_DMP_ODR_POS)

/** @brief DMP ODR selection
 */
typedef enum
{
	IIM423XX_APEX_CONFIG0_DMP_ODR_25Hz     = (0x0 << BIT_APEX_CONFIG0_DMP_ODR_POS), /**< 25Hz (40ms) */
	IIM423XX_APEX_CONFIG0_DMP_ODR_50Hz     = (0x2 << BIT_APEX_CONFIG0_DMP_ODR_POS), /**< 50Hz (20ms) */
	IIM423XX_APEX_CONFIG0_DMP_ODR_100Hz    = (0x3 << BIT_APEX_CONFIG0_DMP_ODR_POS), /**< 100Hz (10ms) */
	IIM423XX_APEX_CONFIG0_DMP_ODR_500Hz    = (0x1 << BIT_APEX_CONFIG0_DMP_ODR_POS), /**< 500Hz (40ms) */
} IIM423XX_APEX_CONFIG0_DMP_ODR_t;

/*
 * MPUREG_SMD_CONFIG
 * Register Name: SMD_CONFIG
 */

/* WOM_INT_MODE */
#define BIT_SMD_CONFIG_WOM_INT_MODE_POS       3
#define BIT_SMD_CONFIG_WOM_INT_MODE_MASK   (0x1 << BIT_SMD_CONFIG_WOM_INT_MODE_POS)

typedef enum
{
	IIM423XX_SMD_CONFIG_WOM_INT_MODE_ANDED = (0x01 << BIT_SMD_CONFIG_WOM_INT_MODE_POS),
	IIM423XX_SMD_CONFIG_WOM_INT_MODE_ORED  = (0x00 << BIT_SMD_CONFIG_WOM_INT_MODE_POS),
} IIM423XX_SMD_CONFIG_WOM_INT_MODE_t;

/* WOM_MODE */
#define BIT_SMD_CONFIG_WOM_MODE_POS       2
#define BIT_SMD_CONFIG_WOM_MODE_MASK   (0x1 << BIT_SMD_CONFIG_WOM_MODE_POS)

typedef enum
{
	IIM423XX_SMD_CONFIG_WOM_MODE_CMP_PREV = (0x01 << BIT_SMD_CONFIG_WOM_MODE_POS),
	IIM423XX_SMD_CONFIG_WOM_MODE_CMP_INIT = (0x00 << BIT_SMD_CONFIG_WOM_MODE_POS),
} IIM423XX_SMD_CONFIG_WOM_MODE_t;

/* SMD_MODE */
#define BIT_SMD_CONFIG_SMD_MODE_POS       0
#define BIT_SMD_CONFIG_SMD_MODE_MASK    0x3

typedef enum
{
	IIM423XX_SMD_CONFIG_SMD_MODE_LONG     = 0x03,
	IIM423XX_SMD_CONFIG_SMD_MODE_SHORT    = 0x02,
	IIM423XX_SMD_CONFIG_SMD_MODE_WOM      = 0x01,
	IIM423XX_SMD_CONFIG_SMD_MODE_DISABLED = 0x00,
} IIM423XX_SMD_CONFIG_SMD_MODE_t;

/*
 * MPUREG_FIFO_CONFIG1
 * Register Name: FIFO_CONFIG1
 */
#define BIT_FIFO_CONFIG1_RESUME_PARTIAL_RD_POS       6
#define BIT_FIFO_CONFIG1_RESUME_PARTIAL_RD_MASK   (0x1 << BIT_FIFO_CONFIG1_RESUME_PARTIAL_RD_POS)

/* FIFO_WM_GT_TH */
#define BIT_FIFO_CONFIG1_WM_GT_TH_POS       5
#define BIT_FIFO_CONFIG1_WM_GT_TH_MASK   (0x1 << BIT_FIFO_CONFIG1_WM_GT_TH_POS)

typedef enum
{
	IIM423XX_FIFO_CONFIG1_WM_GT_TH_EN  = (0x1 << BIT_FIFO_CONFIG1_WM_GT_TH_POS),
	IIM423XX_FIFO_CONFIG1_WM_GT_TH_DIS = (0x0 << BIT_FIFO_CONFIG1_WM_GT_TH_POS),
} IIM423XX_FIFO_CONFIG1_WM_GT_t;

/* FIFO_HIRES_EN */
#define BIT_FIFO_CONFIG1_HIRES_POS       4
#define BIT_FIFO_CONFIG1_HIRES_MASK   (0x1 << BIT_FIFO_CONFIG1_HIRES_POS)

typedef enum
{
	IIM423XX_FIFO_CONFIG1_HIRES_EN  = (0x1 << BIT_FIFO_CONFIG1_HIRES_POS),
	IIM423XX_FIFO_CONFIG1_HIRES_DIS = (0x0 << BIT_FIFO_CONFIG1_HIRES_POS),
} IIM423XX_FIFO_CONFIG1_HIRES_t;

/* FIFO_TMST_FSYNC_EN */
#define BIT_FIFO_CONFIG1_TMST_FSYNC_POS       3
#define BIT_FIFO_CONFIG1_TMST_FSYNC_MASK   (0x1 << BIT_FIFO_CONFIG1_TMST_FSYNC_POS)

typedef enum
{
	IIM423XX_FIFO_CONFIG1_TMST_FSYNC_EN  = (0x1 << BIT_FIFO_CONFIG1_TMST_FSYNC_POS),
	IIM423XX_FIFO_CONFIG1_TMST_FSYNC_DIS = (0x0 << BIT_FIFO_CONFIG1_TMST_FSYNC_POS),
} IIM423XX_FIFO_CONFIG1_TMST_FSYNC_t;

/* FIFO_TEMP_EN */
#define BIT_FIFO_CONFIG1_TEMP_POS       2
#define BIT_FIFO_CONFIG1_TEMP_MASK   (0x1 << BIT_FIFO_CONFIG1_TEMP_POS)

typedef enum
{
	IIM423XX_FIFO_CONFIG1_TEMP_EN  = (0x1 << BIT_FIFO_CONFIG1_TEMP_POS),
	IIM423XX_FIFO_CONFIG1_TEMP_DIS = (0x0 << BIT_FIFO_CONFIG1_TEMP_POS),
} IIM423XX_FIFO_CONFIG1_TEMP_t;

/* FIFO_RESERVED_EN */
#define BIT_FIFO_CONFIG1_RESERVED_POS       1
#define BIT_FIFO_CONFIG1_RESERVED_MASK   (0x1 << BIT_FIFO_CONFIG1_RESERVED_POS)

typedef enum
{
	IIM423XX_FIFO_CONFIG1_RESERVED_EN  = (0x1 << BIT_FIFO_CONFIG1_RESERVED_POS),
	IIM423XX_FIFO_CONFIG1_RESERVED_DIS = (0x0 << BIT_FIFO_CONFIG1_RESERVED_POS),
} IIM423XX_FIFO_CONFIG1_RESERVED_t;

/* FIFO_ACCEL_EN*/
#define BIT_FIFO_CONFIG1_ACCEL_POS       0
#define BIT_FIFO_CONFIG1_ACCEL_MASK    0x1

typedef enum
{
	IIM423XX_FIFO_CONFIG1_ACCEL_EN  = 0x01,
	IIM423XX_FIFO_CONFIG1_ACCEL_DIS = 0x00,
} IIM423XX_FIFO_CONFIG1_ACCEL_t;

/*
 * MPUREG_FSYNC_CONFIG
 * Register Name: FSYNC_CONFIG
 */

/* FSYNC_UI_SEL */
#define BIT_FSYNC_CONFIG_UI_SEL_POS       4
#define BIT_FSYNC_CONFIG_UI_SEL_MASK   (0x7 << BIT_FSYNC_CONFIG_UI_SEL_POS)

typedef enum
{
	IIM423XX_FSYNC_CONFIG_UI_SEL_NO      = (0x0 << BIT_FSYNC_CONFIG_UI_SEL_POS),
	IIM423XX_FSYNC_CONFIG_UI_SEL_TEMP    = (0x1 << BIT_FSYNC_CONFIG_UI_SEL_POS),

	IIM423XX_FSYNC_CONFIG_UI_SEL_ACCEL_X = (0x5 << BIT_FSYNC_CONFIG_UI_SEL_POS),
	IIM423XX_FSYNC_CONFIG_UI_SEL_ACCEL_Y = (0x6 << BIT_FSYNC_CONFIG_UI_SEL_POS),
	IIM423XX_FSYNC_CONFIG_UI_SEL_ACCEL_Z = (0x7 << BIT_FSYNC_CONFIG_UI_SEL_POS),
} IIM423XX_FSYNC_CONFIG_UI_SEL_t;

/*
 * MPUREG_INT_CONFIG1
 * Register Name: INT_CONFIG1
 */

/* INT_TPULSE_DURATION */
#define BIT_INT_TPULSE_DURATION_POS      6
#define BIT_INT_TPULSE_DURATION_MASK  (0x1 << BIT_INT_TPULSE_DURATION_POS)

typedef enum
{
  IIM423XX_INT_TPULSE_DURATION_8_US    = (0x1 << BIT_INT_TPULSE_DURATION_POS),
  IIM423XX_INT_TPULSE_DURATION_100_US  = (0x0 << BIT_INT_TPULSE_DURATION_POS),
} IIM423XX_INT_TPULSE_DURATION_t;

/* INT_TDEASSERT_DISABLE */
#define BIT_INT_TDEASSERT_POS      5
#define BIT_INT_TDEASSERT_MASK  (0x1 << BIT_INT_TDEASSERT_POS)

typedef enum
{
  IIM423XX_INT_TDEASSERT_DISABLED = (0x1 << BIT_INT_TDEASSERT_POS),
  IIM423XX_INT_TDEASSERT_ENABLED  = (0x0 << BIT_INT_TDEASSERT_POS),
} IIM423XX_INT_TDEASSERT_t;

/* ASY_RESET_DISABLE */
#define BIT_INT_CONFIG1_ASY_RST_POS      4
#define BIT_INT_CONFIG1_ASY_RST_MASK  (0x1 << BIT_INT_CONFIG1_ASY_RST_POS)

typedef enum
{
  IIM423XX_INT_CONFIG1_ASY_RST_DISABLED = (0x1 << BIT_INT_CONFIG1_ASY_RST_POS),
  IIM423XX_INT_CONFIG1_ASY_RST_ENABLED  = (0x0 << BIT_INT_CONFIG1_ASY_RST_POS),
} IIM423XX_INT_CONFIG1_ASY_RST_t;

/*
 * MPUREG_INT_SOURCE0
 * Register Name: INT_SOURCE0
 */
#define BIT_INT_UI_FSYNC_INT_EN_POS         6
#define BIT_INT_PLL_RDY_INT_EN_POS          5
#define BIT_INT_RESET_DONE_INT_EN_POS       4
#define BIT_INT_UI_DRDY_INT_EN_POS          3
#define BIT_INT_FIFO_THS_INT_EN_POS         2
#define BIT_INT_FIFO_FULL_INT_EN_POS        1
#define BIT_INT_UI_AGC_RDY_INT_EN_POS       0

#define BIT_INT_SOURCE0_UI_FSYNC_INT1_EN    0x40
#define BIT_INT_SOURCE0_PLL_RDY_INT1_EN     0x20
#define BIT_INT_SOURCE0_RESET_DONE_INT1_EN  0x10
#define BIT_INT_SOURCE0_UI_DRDY_INT1_EN     0x08
#define BIT_INT_SOURCE0_FIFO_THS_INT1_EN    0x04
#define BIT_INT_SOURCE0_FIFO_FULL_INT1_EN   0x02
#define BIT_INT_SOURCE0_UI_AGC_RDY_INT1_EN  0x01

/*
 * MPUREG_INT_SOURCE1
 * Register Name: INT_SOURCE1
 */
#define BIT_INT_SMD_INT_EN_POS              3
#define BIT_INT_WOM_Z_INT_EN_POS            2
#define BIT_INT_WOM_Y_INT_EN_POS            1
#define BIT_INT_WOM_X_INT_EN_POS            0

#define BIT_INT_SOURCE1_SMD_INT1_EN         0x08
#define BIT_INT_SOURCE1_WOM_Z_INT1_EN       0x04
#define BIT_INT_SOURCE1_WOM_Y_INT1_EN       0x02
#define BIT_INT_SOURCE1_WOM_X_INT1_EN       0x01

/*
 * MPUREG_INT_SOURCE2
 * Register Name: INT_SOURCE2
 */
#define BIT_INT_SOURCE2_OIS2_AGC_RDY_INT1_EN 0x20
#define BIT_INT_SOURCE2_OIS2_FSYNC_INT1_EN   0x10
#define BIT_INT_SOURCE2_OIS2_DRDY_INT1_EN    0x08
#define BIT_INT_SOURCE2_OIS1_AGC_RDY_INT1_EN 0x04
#define BIT_INT_SOURCE2_OIS1_FSYNC_INT1_EN   0x02
#define BIT_INT_SOURCE2_OIS1_DRDY_INT1_EN    0x01

/*
 * MPUREG_INT_SOURCE3
 * Register Name: INT_SOURCE3
 */
#define BIT_INT_SOURCE3_UI_FSYNC_INT2_EN    0x40
#define BIT_INT_SOURCE3_PLL_RDY_INT2_EN     0x20
#define BIT_INT_SOURCE3_RESET_DONE_INT2_EN  0x10
#define BIT_INT_SOURCE3_UI_DRDY_INT2_EN     0x08
#define BIT_INT_SOURCE3_FIFO_THS_INT2_EN    0x04
#define BIT_INT_SOURCE3_FIFO_FULL_INT2_EN   0x02
#define BIT_INT_SOURCE3_UI_AGC_RDY_INT2_EN  0x01

/*
 * MPUREG_INT_SOURCE4
 * Register Name: INT_SOURCE4
 */
#define BIT_INT_SOURCE4_SMD_INT2_EN         0x08
#define BIT_INT_SOURCE4_WOM_Z_INT2_EN       0x04
#define BIT_INT_SOURCE4_WOM_Y_INT2_EN       0x02
#define BIT_INT_SOURCE4_WOM_X_INT2_EN       0x01

/*
 * MPUREG_INT_SOURCE5
 * Register Name: INT_SOURCE5
 */
#define BIT_INT_SOURCE5_OIS2_AGC_RDY_INT2_EN 0x20
#define BIT_INT_SOURCE5_OIS2_FSYNC_INT2_EN   0x10
#define BIT_INT_SOURCE5_OIS2_DRDY_INT2_EN    0x08
#define BIT_INT_SOURCE5_OIS1_AGC_RDY_INT2_EN 0x04
#define BIT_INT_SOURCE5_OIS1_FSYNC_INT2_EN   0x02
#define BIT_INT_SOURCE5_OIS1_DRDY_INT2_EN    0x01

/*
 * MPUREG_SELF_TEST_CONFIG
 * Register Name: SELF_TEST_CONFIG
*/
#define BIT_ST_REGULATOR_EN                 0x40
#define BIT_ACCEL_Z_ST_EN                   0x20
#define BIT_ACCEL_Y_ST_EN                   0x10
#define BIT_ACCEL_X_ST_EN                   0x08



#define BIT_DMP_MEM_ACCESS_EN		0x08
#define BIT_MEM_OTP_ACCESS_EN		0x04
#define BIT_FIFO_MEM_RD_SYS			0x02
#define BIT_FIFO_MEM_WR_SER			0x01

/* ----------------------------------------------------------------------------
 * Register bank 1
 * ---------------------------------------------------------------------------- */

/*
 * MPUREG_INTF_CONFIG4_B1
 * Register Name: INTF_CONFIG4
 */

/* SPI_AP_4WIRE */
#define BIT_INTF_CONFIG4_AP_SPI_POS       1
#define BIT_INTF_CONFIG4_AP_SPI_MASK   (0x1 << BIT_INTF_CONFIG4_AP_SPI_POS)

typedef enum
{
	IIM423XX_INTF_CONFIG4_AP_SPI4W = (0x1 << BIT_INTF_CONFIG4_AP_SPI_POS),
	IIM423XX_INTF_CONFIG4_AP_SPI3W = (0x0 << BIT_INTF_CONFIG4_AP_SPI_POS),
} IIM423XX_INTF_CONFIG4_AP_SPI_t;

/* SPI_AUX1_4WIRE */
#define BIT_INTF_CONFIG4_AUX1_SPI_POS       2
#define BIT_INTF_CONFIG4_AUX1_SPI_MASK   (0x1 << BIT_INTF_CONFIG4_AUX1_SPI_POS)

typedef enum
{
	IIM423XX_INTF_CONFIG4_AUX1_SPI4W = (0x1 << BIT_INTF_CONFIG4_AUX1_SPI_POS),
	IIM423XX_INTF_CONFIG4_AUX1_SPI3W = (0x0 << BIT_INTF_CONFIG4_AUX1_SPI_POS),
} IIM423XX_INTF_CONFIG4_AUX1_SPI_t;

/*
 * MPUREG_INTF_CONFIG5_B1
 * Register Name: INTF_CONFIG5
 */

/* GPIO_PAD_SEL */
#define BIT_INTF_CONFIG5_GPIO_PAD_SEL_POS       1
#define BIT_INTF_CONFIG5_GPIO_PAD_SEL_MASK   (0x3 << BIT_INTF_CONFIG5_GPIO_PAD_SEL_POS)

/*
 * MPUREG_INTF_CONFIG6_B1
 * Register Name: INTF_CONFIG6
 */

/* I3C_DDR_EN */
#define BIT_INTF_CONFIG6_I3C_DDR_EN_POS       1
#define BIT_INTF_CONFIG6_I3C_DDR_EN_MASK   (0x1 << BIT_INTF_CONFIG6_I3C_DDR_EN_POS)

/* I3C_SDR_EN */
#define BIT_INTF_CONFIG6_I3C_SDR_EN_POS       0
#define BIT_INTF_CONFIG6_I3C_SDR_EN_MASK   (0x1 << BIT_INTF_CONFIG6_I3C_SDR_EN_POS)

/*
 * MPUREG_INTF_CONFIG6_B1
 * Register Name: INTF_CONFIG6
 */

/* I3C_IBI_BYTE_EN */
#define BIT_INTF_CONFIG6_I3C_IBI_BYTE_EN_POS       3
#define BIT_INTF_CONFIG6_I3C_IBI_BYTE_EN_MASK   (0x1 << BIT_INTF_CONFIG6_I3C_IBI_BYTE_EN_POS)

/* I3C_IBI_EN */
#define BIT_INTF_CONFIG6_I3C_IBI_EN_POS       2
#define BIT_INTF_CONFIG6_I3C_IBI_EN_MASK   (0x1 << BIT_INTF_CONFIG6_I3C_IBI_EN_POS)

/* ----------------------------------------------------------------------------
 * Register bank 2
 * ---------------------------------------------------------------------------- */

/*
 * MPUREG_ACCEL_CONFIG_STATIC2_B2
 * Register Name: ACCEL_CONFIG_STATIC2
 */

 /* ACCEL_AAF_DIS */
#define BIT_ACCEL_AAF_DIS_POS        0
#define BIT_ACCEL_AAF_DIS_MASK   (0x01 << BIT_ACCEL_AAF_DIS_POS)

typedef enum
{
	IIM423XX_ACCEL_AAF_EN  = (0x0 << BIT_ACCEL_AAF_DIS_POS),
	IIM423XX_ACCEL_AAF_DIS = (0x1 << BIT_ACCEL_AAF_DIS_POS),
} IIM423XX_ACCEL_AAF_DIS_t;

/* ACCEL_AAF_DELT */
#define BIT_ACCEL_AAF_DELT_POS        1
#define BIT_ACCEL_AAF_DELT_MASK   (0x3F << BIT_ACCEL_AAF_DELT_POS)

/*
 * MPUREG_ACCEL_CONFIG_STATIC3_B2
 * Register Name: ACCEL_CONFIG_STATIC3
 */

/* ACCEL_AAF_DELTSQR */
#define BIT_ACCEL_AAF_DELTSQR_POS_LO        0
#define BIT_ACCEL_AAF_DELTSQR_MASK_LO   (0xFF << BIT_ACCEL_AAF_DELTSQR_POS_LO)

/*
 * MPUREG_ACCEL_CONFIG_STATIC4_B2
 * Register Name: ACCEL_CONFIG_STATIC4
 */

/* ACCEL_AAF_DELTSQR */
#define BIT_ACCEL_AAF_DELTSQR_POS_HI        0
#define BIT_ACCEL_AAF_DELTSQR_MASK_HI   (0x0F << BIT_ACCEL_AAF_DELTSQR_POS_HI)

/* ACCEL_AAF_BITSHIFT */
#define BIT_ACCEL_AAF_BITSHIFT_POS     4
#define BIT_ACCEL_AAF_BITSHIFT_MASK   (0x0F << BIT_ACCEL_AAF_BITSHIFT_POS)




/* ----------------------------------------------------------------------------
 * Register bank 4
 * ---------------------------------------------------------------------------- */

/*
 * MPUREG_FDR_CONFIG_B4
 * Register Name: FDR_CONFIG
*/

/* FDR_SEL */

#define BIT_FDR_CONFIG_FDR_SEL_POS   0
#define BIT_FDR_CONFIG_FDR_SEL_MASK  0x7F

/*
 * MPUREG_APEX_CONFIG1_B4
 * Register Name: APEX_CONFIG1
*/

/* DMP_POWER_SAVE_TIME_SEL */

#define BIT_APEX_CONFIG1_DMP_POWER_SAVE_TIME_SEL_POS   0
#define BIT_APEX_CONFIG1_DMP_POWER_SAVE_TIME_SEL_MASK  0x0F

typedef enum
{
	IIM423XX_APEX_CONFIG1_DMP_POWER_SAVE_TIME_SEL_0S  = 0x0,
	IIM423XX_APEX_CONFIG1_DMP_POWER_SAVE_TIME_SEL_4S  = 0x1,
	IIM423XX_APEX_CONFIG1_DMP_POWER_SAVE_TIME_SEL_8S  = 0x2,
	IIM423XX_APEX_CONFIG1_DMP_POWER_SAVE_TIME_SEL_12S = 0x3,
	IIM423XX_APEX_CONFIG1_DMP_POWER_SAVE_TIME_SEL_16S = 0x4,
	IIM423XX_APEX_CONFIG1_DMP_POWER_SAVE_TIME_SEL_20S = 0x5,
	IIM423XX_APEX_CONFIG1_DMP_POWER_SAVE_TIME_SEL_24S = 0x6,
	IIM423XX_APEX_CONFIG1_DMP_POWER_SAVE_TIME_SEL_28S = 0x7,
	IIM423XX_APEX_CONFIG1_DMP_POWER_SAVE_TIME_SEL_32S = 0x8,
	IIM423XX_APEX_CONFIG1_DMP_POWER_SAVE_TIME_SEL_36S = 0x9,
	IIM423XX_APEX_CONFIG1_DMP_POWER_SAVE_TIME_SEL_40S = 0xA,
	IIM423XX_APEX_CONFIG1_DMP_POWER_SAVE_TIME_SEL_44S = 0xB,
	IIM423XX_APEX_CONFIG1_DMP_POWER_SAVE_TIME_SEL_48S = 0xC,
	IIM423XX_APEX_CONFIG1_DMP_POWER_SAVE_TIME_SEL_52S = 0xD,
	IIM423XX_APEX_CONFIG1_DMP_POWER_SAVE_TIME_SEL_56S = 0xE,
	IIM423XX_APEX_CONFIG1_DMP_POWER_SAVE_TIME_SEL_60S = 0xF
} IIM423XX_APEX_CONFIG1_DMP_POWER_SAVE_TIME_t;

/* LOW_ENERGY_AMP_TH_SEL */

#define BIT_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_POS    4
#define BIT_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_MASK  (0x0F << BIT_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_POS)

typedef enum
{
	IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_30MG  = (0 << BIT_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_POS),
	IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_35MG  = (1 << BIT_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_POS),
	IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_40MG  = (2 << BIT_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_POS),
	IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_45MG  = (3 << BIT_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_POS),
	IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_50MG  = (4 << BIT_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_POS),
	IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_55MG  = (5 << BIT_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_POS),
	IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_60MG  = (6 << BIT_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_POS),
	IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_65MG  = (7 << BIT_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_POS),
	IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_70MG  = (8 << BIT_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_POS),
	IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_75MG  = (9 << BIT_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_POS),
	IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_80MG  = (10 << BIT_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_POS),
	IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_85MG  = (11 << BIT_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_POS),
	IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_90MG  = (12 << BIT_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_POS),
	IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_95MG  = (13 << BIT_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_POS),
	IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_100MG = (14 << BIT_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_POS),
	IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_105MG = (15 << BIT_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_POS)
} IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_t;

// Retro-compatibility
#define IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_1006632MG IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_30MG
#define IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_1174405MG IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_35MG
#define IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_1342177MG IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_40MG
#define IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_1509949MG IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_45MG
#define IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_1677721MG IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_50MG
#define IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_1845493MG IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_55MG
#define IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_2013265MG IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_60MG
#define IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_2181038MG IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_65MG
#define IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_2348810MG IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_70MG
#define IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_2516582MG IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_75MG
#define IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_2684354MG IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_80MG
#define IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_2852126MG IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_85MG
#define IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_3019898MG IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_90MG
#define IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_3187671MG IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_95MG
#define IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_3355443MG IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_100MG
#define IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_3523215MG IIM423XX_APEX_CONFIG1_LOW_ENERGY_AMP_TH_SEL_105MG

/*
 * MPUREG_APEX_CONFIG2_B4
 * Register Name: APEX_CONFIG2
*/

/* PEDO_AMP_TH_SEL */

#define BIT_APEX_CONFIG2_PEDO_AMP_TH_POS   4
#define BIT_APEX_CONFIG2_PEDO_AMP_TH_MASK (0x0F<<BIT_APEX_CONFIG2_PEDO_AMP_TH_POS)

typedef enum
{
	IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_30MG = (0  << BIT_APEX_CONFIG2_PEDO_AMP_TH_POS),
	IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_34MG = (1  << BIT_APEX_CONFIG2_PEDO_AMP_TH_POS),
	IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_38MG = (2  << BIT_APEX_CONFIG2_PEDO_AMP_TH_POS),
	IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_42MG = (3  << BIT_APEX_CONFIG2_PEDO_AMP_TH_POS),
	IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_46MG = (4  << BIT_APEX_CONFIG2_PEDO_AMP_TH_POS),
	IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_50MG = (5  << BIT_APEX_CONFIG2_PEDO_AMP_TH_POS),
	IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_54MG = (6  << BIT_APEX_CONFIG2_PEDO_AMP_TH_POS),
	IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_58MG = (7  << BIT_APEX_CONFIG2_PEDO_AMP_TH_POS),
	IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_62MG = (8  << BIT_APEX_CONFIG2_PEDO_AMP_TH_POS),
	IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_66MG = (9  << BIT_APEX_CONFIG2_PEDO_AMP_TH_POS),
	IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_70MG = (10 << BIT_APEX_CONFIG2_PEDO_AMP_TH_POS),
	IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_74MG = (11 << BIT_APEX_CONFIG2_PEDO_AMP_TH_POS),
	IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_78MG = (12 << BIT_APEX_CONFIG2_PEDO_AMP_TH_POS),
	IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_82MG = (13 << BIT_APEX_CONFIG2_PEDO_AMP_TH_POS),
	IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_86MG = (14 << BIT_APEX_CONFIG2_PEDO_AMP_TH_POS),
	IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_90MG = (15 << BIT_APEX_CONFIG2_PEDO_AMP_TH_POS)
} IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_t;

// Retro-compatibility
#define IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_1006632_MG IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_30MG
#define IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_1140850_MG IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_34MG
#define IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_1275068_MG IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_38MG
#define IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_1409286_MG IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_42MG
#define IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_1543503_MG IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_46MG
#define IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_1677721_MG IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_50MG
#define IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_1811939_MG IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_54MG
#define IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_1946157_MG IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_58MG
#define IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_2080374_MG IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_62MG
#define IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_2214592_MG IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_66MG
#define IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_2348810_MG IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_70MG
#define IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_2483027_MG IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_74MG
#define IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_2617245_MG IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_78MG
#define IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_2751463_MG IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_82MG
#define IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_2885681_MG IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_86MG
#define IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_3019898_MG IIM423XX_APEX_CONFIG2_PEDO_AMP_TH_90MG


/* PEDO_STEP_CNT_TH_SEL */

#define BIT_APEX_CONFIG2_PEDO_STEP_CNT_TH_POS  0
#define BIT_APEX_CONFIG2_PEDO_STEP_CNT_TH_MASK 0x0F

/*
 * MPUREG_APEX_CONFIG3_B4
 * Register Name: APEX_CONFIG3
*/

/* PEDO_STEP_DET_TH_SEL */

#define BIT_APEX_CONFIG3_PEDO_STEP_DET_TH_POS   5
#define BIT_APEX_CONFIG3_PEDO_STEP_DET_TH_MASK (0x07<<BIT_APEX_CONFIG3_PEDO_STEP_DET_TH_POS)

/* PEDO_SB_TIMER_TH_SEL */

#define BIT_APEX_CONFIG3_PEDO_SB_TIMER_TH_POS   2
#define BIT_APEX_CONFIG3_PEDO_SB_TIMER_TH_MASK (0x07<<BIT_APEX_CONFIG3_PEDO_SB_TIMER_TH_POS)

typedef enum
{
	IIM423XX_APEX_CONFIG3_PEDO_SB_TIMER_TH_50_SAMPLES  = (0  << BIT_APEX_CONFIG3_PEDO_SB_TIMER_TH_POS),
	IIM423XX_APEX_CONFIG3_PEDO_SB_TIMER_TH_75_SAMPLES  = (1  << BIT_APEX_CONFIG3_PEDO_SB_TIMER_TH_POS),
	IIM423XX_APEX_CONFIG3_PEDO_SB_TIMER_TH_100_SAMPLES = (2  << BIT_APEX_CONFIG3_PEDO_SB_TIMER_TH_POS),
	IIM423XX_APEX_CONFIG3_PEDO_SB_TIMER_TH_125_SAMPLES = (3  << BIT_APEX_CONFIG3_PEDO_SB_TIMER_TH_POS),
	IIM423XX_APEX_CONFIG3_PEDO_SB_TIMER_TH_150_SAMPLES = (4  << BIT_APEX_CONFIG3_PEDO_SB_TIMER_TH_POS),
	IIM423XX_APEX_CONFIG3_PEDO_SB_TIMER_TH_175_SAMPLES = (5  << BIT_APEX_CONFIG3_PEDO_SB_TIMER_TH_POS),
	IIM423XX_APEX_CONFIG3_PEDO_SB_TIMER_TH_200_SAMPLES = (6  << BIT_APEX_CONFIG3_PEDO_SB_TIMER_TH_POS),
	IIM423XX_APEX_CONFIG3_PEDO_SB_TIMER_TH_225_SAMPLES = (7  << BIT_APEX_CONFIG3_PEDO_SB_TIMER_TH_POS)
} IIM423XX_APEX_CONFIG3_PEDO_SB_TIMER_TH_t;

/* PEDO_HI_ENRGY_TH_SEL */

#define BIT_APEX_CONFIG3_PEDO_HI_ENRGY_TH_POS   0
#define BIT_APEX_CONFIG3_PEDO_HI_ENRGY_TH_MASK  0x03

typedef enum
{
	IIM423XX_APEX_CONFIG3_PEDO_HI_ENRGY_TH_90  = 0x0,
	IIM423XX_APEX_CONFIG3_PEDO_HI_ENRGY_TH_107 = 0x1,
	IIM423XX_APEX_CONFIG3_PEDO_HI_ENRGY_TH_136 = 0x2,
	IIM423XX_APEX_CONFIG3_PEDO_HI_ENRGY_TH_159 = 0x3
} IIM423XX_APEX_CONFIG3_PEDO_HI_ENRGY_TH_t;

/*
 * MPUREG_APEX_CONFIG4_B4
 * Register Name: APEX_CONFIG4
*/

/* TILT_WAIT_TIME_SEL */

#define BIT_APEX_CONFIG4_TILT_WAIT_TIME_POS   6
#define BIT_APEX_CONFIG4_TILT_WAIT_TIME_MASK (0x03<<BIT_APEX_CONFIG4_TILT_WAIT_TIME_POS)

typedef enum
{
	IIM423XX_APEX_CONFIG4_TILT_WAIT_TIME_0S = (0  << BIT_APEX_CONFIG4_TILT_WAIT_TIME_POS),
	IIM423XX_APEX_CONFIG4_TILT_WAIT_TIME_2S = (1  << BIT_APEX_CONFIG4_TILT_WAIT_TIME_POS),
	IIM423XX_APEX_CONFIG4_TILT_WAIT_TIME_4S = (2  << BIT_APEX_CONFIG4_TILT_WAIT_TIME_POS),
	IIM423XX_APEX_CONFIG4_TILT_WAIT_TIME_6S = (3  << BIT_APEX_CONFIG4_TILT_WAIT_TIME_POS)
} IIM423XX_APEX_CONFIG4_TILT_WAIT_TIME_t;

#if defined(ICM_FAMILY_BPLUS)
/* R2W_SLEEP_TIME_OUT */

#define BIT_APEX_CONFIG4_R2W_SLEEP_TIME_OUT_POS   3
#define BIT_APEX_CONFIG4_R2W_SLEEP_TIME_OUT_MASK (0x07<<BIT_APEX_CONFIG4_R2W_SLEEP_TIME_OUT_POS)

typedef enum
{
	IIM423XX_APEX_CONFIG4_R2W_SLEEP_TIME_OUT_1_28S  = (0  << BIT_APEX_CONFIG4_R2W_SLEEP_TIME_OUT_POS),
	IIM423XX_APEX_CONFIG4_R2W_SLEEP_TIME_OUT_2_56S  = (1  << BIT_APEX_CONFIG4_R2W_SLEEP_TIME_OUT_POS),
	IIM423XX_APEX_CONFIG4_R2W_SLEEP_TIME_OUT_3_84S  = (2  << BIT_APEX_CONFIG4_R2W_SLEEP_TIME_OUT_POS),
	IIM423XX_APEX_CONFIG4_R2W_SLEEP_TIME_OUT_5_12S  = (3  << BIT_APEX_CONFIG4_R2W_SLEEP_TIME_OUT_POS),
	IIM423XX_APEX_CONFIG4_R2W_SLEEP_TIME_OUT_6_4S   = (4  << BIT_APEX_CONFIG4_R2W_SLEEP_TIME_OUT_POS),
	IIM423XX_APEX_CONFIG4_R2W_SLEEP_TIME_OUT_7_68S  = (5  << BIT_APEX_CONFIG4_R2W_SLEEP_TIME_OUT_POS),
	IIM423XX_APEX_CONFIG4_R2W_SLEEP_TIME_OUT_8_96S  = (6  << BIT_APEX_CONFIG4_R2W_SLEEP_TIME_OUT_POS),
	IIM423XX_APEX_CONFIG4_R2W_SLEEP_TIME_OUT_10_24S = (7  << BIT_APEX_CONFIG4_R2W_SLEEP_TIME_OUT_POS)
} IIM423XX_APEX_CONFIG4_R2W_SLEEP_TIME_OUT_t;

/*
 * MPUREG_APEX_CONFIG5_B4
 * Register Name: APEX_CONFIG5
*/

/* R2W_MOUNTING_MATRIX */

#define BIT_APEX_CONFIG5_R2W_MOUNTING_MATRIX_POS   0
#define BIT_APEX_CONFIG5_R2W_MOUNTING_MATRIX_MASK (0x07<<BIT_APEX_CONFIG5_R2W_MOUNTING_MATRIX_POS)

typedef enum
{
	IIM423XX_APEX_CONFIG5_R2W_MOUNTING_MATRIX_0 = (0  << BIT_APEX_CONFIG5_R2W_MOUNTING_MATRIX_POS),
	IIM423XX_APEX_CONFIG5_R2W_MOUNTING_MATRIX_1 = (1  << BIT_APEX_CONFIG5_R2W_MOUNTING_MATRIX_POS),
	IIM423XX_APEX_CONFIG5_R2W_MOUNTING_MATRIX_2 = (2  << BIT_APEX_CONFIG5_R2W_MOUNTING_MATRIX_POS),
	IIM423XX_APEX_CONFIG5_R2W_MOUNTING_MATRIX_3 = (3  << BIT_APEX_CONFIG5_R2W_MOUNTING_MATRIX_POS),
	IIM423XX_APEX_CONFIG5_R2W_MOUNTING_MATRIX_4 = (4  << BIT_APEX_CONFIG5_R2W_MOUNTING_MATRIX_POS),
	IIM423XX_APEX_CONFIG5_R2W_MOUNTING_MATRIX_5 = (5  << BIT_APEX_CONFIG5_R2W_MOUNTING_MATRIX_POS),
	IIM423XX_APEX_CONFIG5_R2W_MOUNTING_MATRIX_6 = (6  << BIT_APEX_CONFIG5_R2W_MOUNTING_MATRIX_POS),
	IIM423XX_APEX_CONFIG5_R2W_MOUNTING_MATRIX_7 = (7  << BIT_APEX_CONFIG5_R2W_MOUNTING_MATRIX_POS)
} IIM423XX_APEX_CONFIG5_R2W_MOUNTING_MATRIX_t;

/*
 * MPUREG_APEX_CONFIG6_B4
 * Register Name: APEX_CONFIG6
*/

/* R2W_SLEEP_GEST_DELAY */

#define BIT_APEX_CONFIG6_R2W_SLEEP_GEST_DELAY_POS   0
#define BIT_APEX_CONFIG6_R2W_SLEEP_GEST_DELAY_MASK (0x07<<BIT_APEX_CONFIG6_R2W_SLEEP_GEST_DELAY_POS)

typedef enum
{
	IIM423XX_APEX_CONFIG6_R2W_SLEEP_GEST_DELAY_0_32S  = (0  << BIT_APEX_CONFIG6_R2W_SLEEP_GEST_DELAY_POS),
	IIM423XX_APEX_CONFIG6_R2W_SLEEP_GEST_DELAY_0_64S  = (1  << BIT_APEX_CONFIG6_R2W_SLEEP_GEST_DELAY_POS),
	IIM423XX_APEX_CONFIG6_R2W_SLEEP_GEST_DELAY_0_96S  = (2  << BIT_APEX_CONFIG6_R2W_SLEEP_GEST_DELAY_POS),
	IIM423XX_APEX_CONFIG6_R2W_SLEEP_GEST_DELAY_1_28S  = (3  << BIT_APEX_CONFIG6_R2W_SLEEP_GEST_DELAY_POS),
	IIM423XX_APEX_CONFIG6_R2W_SLEEP_GEST_DELAY_1_6S   = (4  << BIT_APEX_CONFIG6_R2W_SLEEP_GEST_DELAY_POS),
	IIM423XX_APEX_CONFIG6_R2W_SLEEP_GEST_DELAY_1_92S  = (5  << BIT_APEX_CONFIG6_R2W_SLEEP_GEST_DELAY_POS),
	IIM423XX_APEX_CONFIG6_R2W_SLEEP_GEST_DELAY_2_24S  = (6  << BIT_APEX_CONFIG6_R2W_SLEEP_GEST_DELAY_POS),
	IIM423XX_APEX_CONFIG6_R2W_SLEEP_GEST_DELAY_2_56S  = (7  << BIT_APEX_CONFIG6_R2W_SLEEP_GEST_DELAY_POS)
} IIM423XX_APEX_CONFIG6_R2W_SLEEP_GEST_DELAY_t;

#elif defined(ICM_FAMILY_CPLUS)

/* LOWG_PEAK_TH_HYST */

#define BIT_APEX_CONFIG4_LOWG_PEAK_TH_HYST_POS   3
#define BIT_APEX_CONFIG4_LOWG_PEAK_TH_HYST_MASK (0x07<<BIT_APEX_CONFIG4_LOWG_PEAK_TH_HYST_POS)

typedef enum
{
	IIM423XX_APEX_CONFIG4_LOWG_PEAK_TH_HYST_31MG  = (0  << BIT_APEX_CONFIG4_LOWG_PEAK_TH_HYST_POS),
	IIM423XX_APEX_CONFIG4_LOWG_PEAK_TH_HYST_63MG  = (1  << BIT_APEX_CONFIG4_LOWG_PEAK_TH_HYST_POS),
	IIM423XX_APEX_CONFIG4_LOWG_PEAK_TH_HYST_94MG  = (2  << BIT_APEX_CONFIG4_LOWG_PEAK_TH_HYST_POS),
	IIM423XX_APEX_CONFIG4_LOWG_PEAK_TH_HYST_125MG = (3  << BIT_APEX_CONFIG4_LOWG_PEAK_TH_HYST_POS),
	IIM423XX_APEX_CONFIG4_LOWG_PEAK_TH_HYST_156MG = (4  << BIT_APEX_CONFIG4_LOWG_PEAK_TH_HYST_POS),
	IIM423XX_APEX_CONFIG4_LOWG_PEAK_TH_HYST_188MG = (5  << BIT_APEX_CONFIG4_LOWG_PEAK_TH_HYST_POS),
	IIM423XX_APEX_CONFIG4_LOWG_PEAK_TH_HYST_219MG = (6  << BIT_APEX_CONFIG4_LOWG_PEAK_TH_HYST_POS),
	IIM423XX_APEX_CONFIG4_LOWG_PEAK_TH_HYST_250MG = (7  << BIT_APEX_CONFIG4_LOWG_PEAK_TH_HYST_POS)
} IIM423XX_APEX_CONFIG4_LOWG_PEAK_TH_HYST_t;

/* HIGHG_PEAK_TH_HYST */

#define BIT_APEX_CONFIG4_HIGHG_PEAK_TH_HYST_POS   0
#define BIT_APEX_CONFIG4_HIGHG_PEAK_TH_HYST_MASK (0x07<<BIT_APEX_CONFIG4_HIGHG_PEAK_TH_HYST_POS)

typedef enum
{
	IIM423XX_APEX_CONFIG4_HIGHG_PEAK_TH_HYST_31MG  = (0  << BIT_APEX_CONFIG4_HIGHG_PEAK_TH_HYST_POS),
	IIM423XX_APEX_CONFIG4_HIGHG_PEAK_TH_HYST_63MG  = (1  << BIT_APEX_CONFIG4_HIGHG_PEAK_TH_HYST_POS),
	IIM423XX_APEX_CONFIG4_HIGHG_PEAK_TH_HYST_94MG  = (2  << BIT_APEX_CONFIG4_HIGHG_PEAK_TH_HYST_POS),
	IIM423XX_APEX_CONFIG4_HIGHG_PEAK_TH_HYST_125MG = (3  << BIT_APEX_CONFIG4_HIGHG_PEAK_TH_HYST_POS),
	IIM423XX_APEX_CONFIG4_HIGHG_PEAK_TH_HYST_156MG = (4  << BIT_APEX_CONFIG4_HIGHG_PEAK_TH_HYST_POS),
	IIM423XX_APEX_CONFIG4_HIGHG_PEAK_TH_HYST_188MG = (5  << BIT_APEX_CONFIG4_HIGHG_PEAK_TH_HYST_POS),
	IIM423XX_APEX_CONFIG4_HIGHG_PEAK_TH_HYST_219MG = (6  << BIT_APEX_CONFIG4_HIGHG_PEAK_TH_HYST_POS),
	IIM423XX_APEX_CONFIG4_HIGHG_PEAK_TH_HYST_250MG = (7  << BIT_APEX_CONFIG4_HIGHG_PEAK_TH_HYST_POS)
} IIM423XX_APEX_CONFIG4_HIGHG_PEAK_TH_HYST_t;

/*
 * MPUREG_APEX_CONFIG5_B4
 * Register Name: APEX_CONFIG5
*/

/* LOWG_PEAK_TH */

#define BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS   3
#define BIT_APEX_CONFIG5_LOWG_PEAK_TH_MASK (0x1f<<BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS)

typedef enum
{
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_31MG    = (0x00 << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_63MG    = (0x01 << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_94MG    = (0x02 << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_125MG   = (0x03 << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_156MG   = (0x04 << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_188MG   = (0x05 << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_219MG   = (0x06 << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_250MG   = (0x07 << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_281MG   = (0x08 << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_313MG   = (0x09 << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_344MG   = (0x0A << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_375MG   = (0x0B << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_406MG   = (0x0C << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_438MG   = (0x0D << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_469MG   = (0x0E << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_500MG   = (0x0F << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_531MG   = (0x10 << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_563MG   = (0x11 << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_594MG   = (0x12 << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_625MG   = (0x13 << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_656MG   = (0x14 << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_688MG   = (0x15 << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_719MG   = (0x16 << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_750MG   = (0x17 << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_781MG   = (0x18 << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_813MG   = (0x19 << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_844MG   = (0x1A << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_875MG   = (0x1B << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_906MG   = (0x1C << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_938MG   = (0x1D << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_969MG   = (0x1E << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_1000MG  = (0x1F << BIT_APEX_CONFIG5_LOWG_PEAK_TH_POS)
} IIM423XX_APEX_CONFIG5_LOWG_PEAK_TH_t;

/* LOWG_TIME_TH */

#define BIT_APEX_CONFIG5_LOWG_TIME_TH_POS   0
#define BIT_APEX_CONFIG5_LOWG_TIME_TH_MASK (0x07<<BIT_APEX_CONFIG5_LOWG_TIME_TH_POS)

typedef enum
{
	IIM423XX_APEX_CONFIG5_LOWG_TIME_TH_1_SAMPLE  = (0x00 << BIT_APEX_CONFIG5_LOWG_TIME_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_TIME_TH_2_SAMPLES = (0x01 << BIT_APEX_CONFIG5_LOWG_TIME_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_TIME_TH_3_SAMPLES = (0x02 << BIT_APEX_CONFIG5_LOWG_TIME_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_TIME_TH_4_SAMPLES = (0x03 << BIT_APEX_CONFIG5_LOWG_TIME_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_TIME_TH_5_SAMPLES = (0x04 << BIT_APEX_CONFIG5_LOWG_TIME_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_TIME_TH_6_SAMPLES = (0x05 << BIT_APEX_CONFIG5_LOWG_TIME_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_TIME_TH_7_SAMPLES = (0x06 << BIT_APEX_CONFIG5_LOWG_TIME_TH_POS),
	IIM423XX_APEX_CONFIG5_LOWG_TIME_TH_8_SAMPLES = (0x07 << BIT_APEX_CONFIG5_LOWG_TIME_TH_POS)
} IIM423XX_APEX_CONFIG5_LOWG_TIME_TH_SAMPLES_t;

/*
 * MPUREG_APEX_CONFIG6_B4
 * Register Name: APEX_CONFIG6
*/
/* HIGHG_PEAK_TH */

#define BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS   3
#define BIT_APEX_CONFIG6_HIGHG_PEAK_TH_MASK (0x1f<<BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS)

typedef enum
{
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_250MG  = (0x00 << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_500MG  = (0x01 << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_750MG  = (0x02 << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_1000MG = (0x03 << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_1250MG = (0x04 << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_1500MG = (0x05 << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_1750MG = (0x06 << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_2000MG = (0x07 << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_2250MG = (0x08 << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_2500MG = (0x09 << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_2750MG = (0x0A << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_3000MG = (0x0B << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_3250MG = (0x0C << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_3500MG = (0x0D << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_3750MG = (0x0E << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_4000MG = (0x0F << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_4250MG = (0x10 << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_4500MG = (0x11 << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_4750MG = (0x12 << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_5000MG = (0x13 << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_5250MG = (0x14 << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_5500MG = (0x15 << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_5750MG = (0x16 << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_6000MG = (0x17 << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_6250MG = (0x18 << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_6500MG = (0x19 << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_6750MG = (0x1A << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_7000MG = (0x1B << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_7250MG = (0x1C << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_7500MG = (0x1D << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_7750MG = (0x1E << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_8000MG = (0x1F << BIT_APEX_CONFIG6_HIGHG_PEAK_TH_POS)
} IIM423XX_APEX_CONFIG6_HIGHG_PEAK_TH_t;

/* HIGHG_TIME_TH */

#define BIT_APEX_CONFIG6_HIGHG_TIME_TH_POS   0
#define BIT_APEX_CONFIG6_HIGHG_TIME_TH_MASK (0x07<<BIT_APEX_CONFIG6_HIGHG_TIME_TH_POS)

typedef enum
{
	IIM423XX_APEX_CONFIG6_HIGHG_TIME_TH_1_SAMPLE  = (0x00 << BIT_APEX_CONFIG6_HIGHG_TIME_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_TIME_TH_2_SAMPLES = (0x01 << BIT_APEX_CONFIG6_HIGHG_TIME_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_TIME_TH_3_SAMPLES = (0x02 << BIT_APEX_CONFIG6_HIGHG_TIME_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_TIME_TH_4_SAMPLES = (0x03 << BIT_APEX_CONFIG6_HIGHG_TIME_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_TIME_TH_5_SAMPLES = (0x04 << BIT_APEX_CONFIG6_HIGHG_TIME_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_TIME_TH_6_SAMPLES = (0x05 << BIT_APEX_CONFIG6_HIGHG_TIME_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_TIME_TH_7_SAMPLES = (0x06 << BIT_APEX_CONFIG6_HIGHG_TIME_TH_POS),
	IIM423XX_APEX_CONFIG6_HIGHG_TIME_TH_8_SAMPLES = (0x07 << BIT_APEX_CONFIG6_HIGHG_TIME_TH_POS)
} IIM423XX_APEX_CONFIG6_HIGHG_TIME_TH_SAMPLES_t;

#endif

/*
 * MPUREG_APEX_CONFIG7_B4
 * Register Name: APEX_CONFIG7
*/

/* TAP_MIN_JERK_THR */
#define BIT_APEX_CONFIG7_TAP_MIN_JERK_THR_POS        2
#define BIT_APEX_CONFIG7_TAP_MIN_JERK_THR_MASK   (0x3F << BIT_APEX_CONFIG7_TAP_MIN_JERK_THR_POS)

#define IIM423XX_APEX_CONFIG7_TAP_MIN_JERK_THR_281MG_DEFAULT    0x11

/* TAP_MAX_PEAK_TOL */
#define BIT_APEX_CONFIG7_TAP_MAX_PEAK_TOL_POS      0
#define BIT_APEX_CONFIG7_TAP_MAX_PEAK_TOL_MASK   0x3

typedef enum
{
	IIM423XX_APEX_CONFIG7_TAP_MAX_PEAK_TOL_12 = 0x0,
	IIM423XX_APEX_CONFIG7_TAP_MAX_PEAK_TOL_25 = 0x1,
	IIM423XX_APEX_CONFIG7_TAP_MAX_PEAK_TOL_37 = 0x2,
	IIM423XX_APEX_CONFIG7_TAP_MAX_PEAK_TOL_50 = 0x3
} IIM423XX_APEX_CONFIG7_TAP_MAX_PEAK_TOL_t;

/*
 * MPUREG_APEX_CONFIG8_B4
 * Register Name: APEX_CONFIG8
*/

/* TAP_TMAX */
#define BIT_APEX_CONFIG8_TAP_TMAX_POS       5
#define BIT_APEX_CONFIG8_TAP_TMAX_MASK   (0x03 << BIT_APEX_CONFIG8_TAP_TMAX_POS)

typedef enum
{
	IIM423XX_APEX_CONFIG8_TAP_TMAX_250MS = (0 << BIT_APEX_CONFIG8_TAP_TMAX_POS),
	IIM423XX_APEX_CONFIG8_TAP_TMAX_375MS = (1 << BIT_APEX_CONFIG8_TAP_TMAX_POS),
	IIM423XX_APEX_CONFIG8_TAP_TMAX_500MS = (2 << BIT_APEX_CONFIG8_TAP_TMAX_POS),
	IIM423XX_APEX_CONFIG8_TAP_TMAX_625MS = (3 << BIT_APEX_CONFIG8_TAP_TMAX_POS)
} IIM423XX_APEX_CONFIG8_TAP_TMAX_t;

/* TAP_TAVG */
#define BIT_APEX_CONFIG8_TAP_TAVG_POS       3
#define BIT_APEX_CONFIG8_TAP_TAVG_MASK   (0x03 << BIT_APEX_CONFIG8_TAP_TAVG_POS)

typedef enum
{
	IIM423XX_APEX_CONFIG8_TAP_TAVG_1SAMPLE = (0 << BIT_APEX_CONFIG8_TAP_TAVG_POS),
	IIM423XX_APEX_CONFIG8_TAP_TAVG_2SAMPLE = (1 << BIT_APEX_CONFIG8_TAP_TAVG_POS),
	IIM423XX_APEX_CONFIG8_TAP_TAVG_4SAMPLE = (2 << BIT_APEX_CONFIG8_TAP_TAVG_POS),
	IIM423XX_APEX_CONFIG8_TAP_TAVG_8SAMPLE = (3 << BIT_APEX_CONFIG8_TAP_TAVG_POS)
} IIM423XX_APEX_CONFIG8_TAP_TAVG_t;

/* TAP_TMIN */
#define BIT_APEX_CONFIG8_TAP_TMIN_POS       0
#define BIT_APEX_CONFIG8_TAP_TMIN_MASK   0x07

typedef enum
{
	IIM423XX_APEX_CONFIG8_TAP_TMIN_125MS = 0x0,
	IIM423XX_APEX_CONFIG8_TAP_TMIN_140MS = 0x1,
	IIM423XX_APEX_CONFIG8_TAP_TMIN_156MS = 0x2,
	IIM423XX_APEX_CONFIG8_TAP_TMIN_171MS = 0x3,
	IIM423XX_APEX_CONFIG8_TAP_TMIN_187MS = 0x4,
	IIM423XX_APEX_CONFIG8_TAP_TMIN_203MS = 0x5,
	IIM423XX_APEX_CONFIG8_TAP_TMIN_218MS = 0x6,
	IIM423XX_APEX_CONFIG8_TAP_TMIN_234MS = 0x7
} IIM423XX_APEX_CONFIG8_TAP_TMIN_t;

/*
 * MPUREG_APEX_CONFIG9_B4
 * Register Name: APEX_CONFIG9
*/
#define BIT_APEX_CONFIG9_SENSITIVITY_MODE_POS       0
#define BIT_APEX_CONFIG9_SENSITIVITY_MODE_MASK   0x01
typedef enum
{
	IIM423XX_APEX_CONFIG9_SENSITIVITY_MODE_NORMAL   = 0x00,
	IIM423XX_APEX_CONFIG9_SENSITIVITY_MODE_RESERVED = 0x01
} IIM423XX_APEX_CONFIG9_SENSITIVITY_MODE_t;

#if defined(ICM_FAMILY_CPLUS)

/*
 * MPUREG_APEX_CONFIG10_B4
 * Register Name: APEX_CONFIG10
*/

/* FF_DEBOUNCE_DURATION */
#define BIT_APEX_CONFIG10_FF_DEBOUNCE_DURATION_POS     0
#define BIT_APEX_CONFIG10_FF_DEBOUNCE_DURATION_MASK   (0x03 << BIT_APEX_CONFIG10_FF_DEBOUNCE_DURATION_POS)

typedef enum
{
	IIM423XX_APEX_CONFIG10_FF_DEBOUNCE_DURATION_0_MS = (0  << BIT_APEX_CONFIG10_FF_DEBOUNCE_DURATION_POS),
	IIM423XX_APEX_CONFIG10_FF_DEBOUNCE_DURATION_1000_MS = (1  << BIT_APEX_CONFIG10_FF_DEBOUNCE_DURATION_POS),
	IIM423XX_APEX_CONFIG10_FF_DEBOUNCE_DURATION_2000_MS = (2  << BIT_APEX_CONFIG10_FF_DEBOUNCE_DURATION_POS),
	IIM423XX_APEX_CONFIG10_FF_DEBOUNCE_DURATION_3000_MS = (3  << BIT_APEX_CONFIG10_FF_DEBOUNCE_DURATION_POS)
} IIM423XX_APEX_CONFIG10_FF_DEBOUNCE_DURATION_t;

/* FF_MAX_DURATION */
#define BIT_APEX_CONFIG10_FF_MAX_DURATION_POS     2
#define BIT_APEX_CONFIG10_FF_MAX_DURATION_MASK   (0x07 << BIT_APEX_CONFIG10_FF_MAX_DURATION_POS)

typedef enum
{
	IIM423XX_APEX_CONFIG10_FF_MAX_DURATION_113_CM = (0  << BIT_APEX_CONFIG10_FF_MAX_DURATION_POS),
	IIM423XX_APEX_CONFIG10_FF_MAX_DURATION_154_CM = (1  << BIT_APEX_CONFIG10_FF_MAX_DURATION_POS),
	IIM423XX_APEX_CONFIG10_FF_MAX_DURATION_201_CM = (2  << BIT_APEX_CONFIG10_FF_MAX_DURATION_POS),
	IIM423XX_APEX_CONFIG10_FF_MAX_DURATION_255_CM = (3  << BIT_APEX_CONFIG10_FF_MAX_DURATION_POS),
	IIM423XX_APEX_CONFIG10_FF_MAX_DURATION_314_CM = (4  << BIT_APEX_CONFIG10_FF_MAX_DURATION_POS),
	IIM423XX_APEX_CONFIG10_FF_MAX_DURATION_380_CM = (5  << BIT_APEX_CONFIG10_FF_MAX_DURATION_POS),
	IIM423XX_APEX_CONFIG10_FF_MAX_DURATION_452_CM = (6  << BIT_APEX_CONFIG10_FF_MAX_DURATION_POS),
	IIM423XX_APEX_CONFIG10_FF_MAX_DURATION_531_CM = (7  << BIT_APEX_CONFIG10_FF_MAX_DURATION_POS)
} IIM423XX_APEX_CONFIG10_FF_MAX_DURATION_t;

/* FF_MIN_DURATION */
#define BIT_APEX_CONFIG10_FF_MIN_DURATION_POS     5
#define BIT_APEX_CONFIG10_FF_MIN_DURATION_MASK   (0x07 << BIT_APEX_CONFIG10_FF_MIN_DURATION_POS)

typedef enum
{
	IIM423XX_APEX_CONFIG10_FF_MIN_DURATION_13_CM = (0  << BIT_APEX_CONFIG10_FF_MIN_DURATION_POS),
	IIM423XX_APEX_CONFIG10_FF_MIN_DURATION_19_CM = (1  << BIT_APEX_CONFIG10_FF_MIN_DURATION_POS),
	IIM423XX_APEX_CONFIG10_FF_MIN_DURATION_28_CM = (2  << BIT_APEX_CONFIG10_FF_MIN_DURATION_POS),
	IIM423XX_APEX_CONFIG10_FF_MIN_DURATION_38_CM = (3  << BIT_APEX_CONFIG10_FF_MIN_DURATION_POS),
	IIM423XX_APEX_CONFIG10_FF_MIN_DURATION_50_CM = (4  << BIT_APEX_CONFIG10_FF_MIN_DURATION_POS),
	IIM423XX_APEX_CONFIG10_FF_MIN_DURATION_64_CM = (5  << BIT_APEX_CONFIG10_FF_MIN_DURATION_POS),
	IIM423XX_APEX_CONFIG10_FF_MIN_DURATION_78_CM = (6  << BIT_APEX_CONFIG10_FF_MIN_DURATION_POS),
	IIM423XX_APEX_CONFIG10_FF_MIN_DURATION_95_CM = (7  << BIT_APEX_CONFIG10_FF_MIN_DURATION_POS)
} IIM423XX_APEX_CONFIG10_FF_MIN_DURATION_t;

#endif

/*
 * MPUREG_INT_SOURCE6_B4
 * Register Name: INT_SOURCE6
 */
#define BIT_INT_STEP_DET_INT_EN_POS      5
#define BIT_INT_STEP_CNT_OVFL_INT_EN_POS 4
#define BIT_INT_TILT_DET_INT_EN_POS      3
#if defined(ICM_FAMILY_BPLUS)
#define BIT_INT_WAKE_DET_INT_EN_POS      2
#define BIT_INT_SLEEP_DET_INT_EN_POS     1
#elif defined(ICM_FAMILY_CPLUS)
#define BIT_INT_LOWG_DET_INT_EN_POS      2
#define BIT_INT_FF_DET_INT_EN_POS        1
#endif
#define BIT_INT_TAP_DET_INT_EN_POS       0

#define BIT_INT_SOURCE6_STEP_DET_INT1_EN      0x20
#define BIT_INT_SOURCE6_STEP_CNT_OVFL_INT1_EN 0x10
#define BIT_INT_SOURCE6_TILT_DET_INT1_EN      0x8
#if defined(ICM_FAMILY_BPLUS)
#define BIT_INT_SOURCE6_WAKE_DET_INT1_EN      0x4
#define BIT_INT_SOURCE6_SLEEP_DET_INT1_EN     0x2
#elif defined(ICM_FAMILY_CPLUS)
#define BIT_INT_SOURCE6_LOWG_DET_INT1_EN      0x4
#define BIT_INT_SOURCE6_FF_DET_INT1_EN        0x2
#endif
#define BIT_INT_SOURCE6_TAP_DET_INT1_EN       0x1

/*
 * MPUREG_INT_SOURCE7_B4
 * Register Name: INT_SOURCE7
 */
#define BIT_INT_SOURCE7_STEP_DET_INT2_EN      0x20
#define BIT_INT_SOURCE7_STEP_CNT_OVFL_INT2_EN 0x10
#define BIT_INT_SOURCE7_TILT_DET_INT2_EN      0x8
#if defined(ICM_FAMILY_BPLUS)
#define BIT_INT_SOURCE7_WAKE_DET_INT2_EN      0x4
#define BIT_INT_SOURCE7_SLEEP_DET_INT2_EN     0x2
#elif defined(ICM_FAMILY_CPLUS)
#define BIT_INT_SOURCE7_LOWG_DET_INT2_EN      0x4
#define BIT_INT_SOURCE7_FF_DET_INT2_EN        0x2
#endif
#define BIT_INT_SOURCE7_TAP_DET_INT2_EN       0x1

/*
 * MPUREG_INT_SOURCE8_B4
 * Register Name: INT_SOURCE8
 */
#define BIT_INT_OIS1_DRDY_IBI_EN_POS  6
#define BIT_INT_UI_FSYNC_IBI_EN_POS   5
#define BIT_INT_PLL_RDY_IBI_EN_POS    4
#define BIT_INT_UI_DRDY_IBI_EN_POS    3
#define BIT_INT_FIFO_THS_IBI_EN_POS   2
#define BIT_INT_FIFO_FULL_IBI_EN_POS  1
#define BIT_INT_UI_AGC_RDY_IBI_EN_POS 0

#define BIT_INT_SOURCE8_OIS1_DRDY_IBI_EN  0x40
#define BIT_INT_SOURCE8_UI_FSYNC_IBI_EN   0x20
#define BIT_INT_SOURCE8_PLL_RDY_IBI_EN    0x10
#define BIT_INT_SOURCE8_UI_DRDY_IBI_EN    0x08
#define BIT_INT_SOURCE8_FIFO_THS_IBI_EN   0x04
#define BIT_INT_SOURCE8_FIFO_FULL_IBI_EN  0x02
#define BIT_INT_SOURCE8_UI_AGC_RDY_IBI_EN 0x01

/*
 * MPUREG_INT_SOURCE9_B4
 * Register Name: INT_SOURCE9
 */
#define BIT_INT_SMD_IBI_EN_POS                 4
#define BIT_INT_WOM_Z_IBI_EN_POS               3
#define BIT_INT_WOM_Y_IBI_EN_POS               2
#define BIT_INT_WOM_X_IBI_EN_POS               1

#define BIT_INT_SOURCE9_SMD_IBI_EN             0x10
#define BIT_INT_SOURCE9_WOM_Z_IBI_EN           0x08
#define BIT_INT_SOURCE9_WOM_Y_IBI_EN           0x04
#define BIT_INT_SOURCE9_WOM_X_IBI_EN           0x02

/*
 * MPUREG_INT_SOURCE10_B4
 * Register Name: INT_SOURCE10
 */
#define BIT_INT_STEP_DET_IBI_EN_POS      5
#define BIT_INT_STEP_CNT_OVFL_IBI_EN_POS 4
#define BIT_INT_TILT_DET_IBI_EN_POS      3
#if defined(ICM_FAMILY_BPLUS)
#define BIT_INT_WAKE_DET_IBI_EN_POS      2
#define BIT_INT_SLEEP_DET_IBI_EN_POS     1
#elif defined(ICM_FAMILY_CPLUS)
#define BIT_INT_LOWG_DET_IBI_EN_POS      2
#define BIT_INT_FF_DET_IBI_EN_POS        1
#endif
#define BIT_INT_TAP_DET_IBI_EN_POS       0

#define BIT_INT_SOURCE10_STEP_DET_IBI_EN      0x20
#define BIT_INT_SOURCE10_STEP_CNT_OVFL_IBI_EN 0x10
#define BIT_INT_SOURCE10_TILT_DET_IBI_EN      0x08
#if defined(ICM_FAMILY_BPLUS)
#define BIT_INT_SOURCE10_WAKE_DET_IBI_EN      0x04
#define BIT_INT_SOURCE10_SLEEP_DET_IBI_EN     0x02
#elif defined(ICM_FAMILY_CPLUS)
#define BIT_INT_SOURCE10_LOWG_DET_IBI_EN      0x04
#define BIT_INT_SOURCE10_FF_DET_IBI_EN        0x02
#endif
#define BIT_INT_SOURCE10_TAP_DET_IBI_EN       0x01



/*
 * MPUREG_OFFSET_USER_4_B4
 * Register Name: OFFSET_USER4
 */



/* ACCEL_X_OFFUSER */
#define BIT_ACCEL_X_OFFUSER_POS_HI        4
#define BIT_ACCEL_X_OFFUSER_MASK_HI   (0x0F << BIT_ACCEL_X_OFFUSER_POS_HI)

/*
 * MPUREG_OFFSET_USER_5_B4
 * Register Name: OFFSET_USER5
 */

#define BIT_ACCEL_X_OFFUSER_POS_LO        0
#define BIT_ACCEL_X_OFFUSER_MASK_LO   (0xFF << BIT_ACCEL_X_OFFUSER_POS_LO)

/*
 * MPUREG_OFFSET_USER_6_B4
 * Register Name: OFFSET_USER_6
 */

/* ACCEL_Y_OFFUSER */
#define BIT_ACCEL_Y_OFFUSER_POS_LO       0
#define BIT_ACCEL_Y_OFFUSER_MASK_LO   (0xFF << BIT_ACCEL_Y_OFFUSER_POS_LO)


/*
 * MPUREG_OFFSET_USER_7_B4
 * Register Name: OFFSET_USER_7
 */

#define BIT_ACCEL_Y_OFFUSER_POS_HI        0
#define BIT_ACCEL_Y_OFFUSER_MASK_HI   (0x0F << BIT_ACCEL_Y_OFFUSER_POS_HI)

/* ACCEL_Z_OFFUSER */
#define BIT_ACCEL_Z_OFFUSER_POS_HI        4
#define BIT_ACCEL_Z_OFFUSER_MASK_HI   (0x0F << BIT_ACCEL_Z_OFFUSER_POS_HI)

/*
 * MPUREG_OFFSET_USER_8_B4
 * Register Name: OFFSET_USER_8
 */

/* ACCEL_Z_OFFUSER_L */
#define BIT_ACCEL_Z_OFFUSER_POS_LO        0
#define BIT_ACCEL_Z_OFFUSER_MASK_LO   (0xFF << BIT_ACCEL_Z_OFFUSER_POS_LO)

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef _INV_IIM423XX_DEFS_H_ */

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int GetAccRegister(uint8_t registerAddress, uint8_t* registerData, uint8_t dataSize)
{
	int status = E_SUCCESS;

	if (dataSize > 7) { return (E_BAD_PARAM); }
	uint8_t readData[8];
	uint8_t writeData[8];
	writeData[0] = registerAddress | 0x80; // Set the MSB to 1 to signify a read
	memset(&writeData[1], 0, dataSize);

	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { MXC_GPIO_OutClr(GPIO_SPI2_SS1_ACC_PORT, GPIO_SPI2_SS1_ACC_PIN); }
	SpiTransaction(SPI_ACC, SPI_8_BIT_DATA_SIZE, YES, &writeData[0], (dataSize + 1), readData, (dataSize + 1), BLOCKING);
	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { MXC_GPIO_OutSet(GPIO_SPI2_SS1_ACC_PORT, GPIO_SPI2_SS1_ACC_PIN); }

	memcpy(registerData, &readData[1], dataSize);

	return (status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int SetAccRegister(uint8_t registerAddress, uint8_t registerData)
{
	int status = E_SUCCESS;

	uint8_t writeData[2];
	writeData[0] = registerAddress;
	writeData[1] = registerData;

	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { MXC_GPIO_OutClr(GPIO_SPI2_SS1_ACC_PORT, GPIO_SPI2_SS1_ACC_PIN); }
	SpiTransaction(SPI_ACC, SPI_8_BIT_DATA_SIZE, YES, &writeData[0], sizeof(writeData), NULL, 0, BLOCKING);
	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { MXC_GPIO_OutSet(GPIO_SPI2_SS1_ACC_PORT, GPIO_SPI2_SS1_ACC_PIN); }

	return (status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int SetAndReadAccRegister(uint8_t registerAddress, uint8_t registerData, uint8_t* readData, uint8_t dataSize)
{
	uint8_t writeData[2];
	int status = E_SUCCESS;

	writeData[0] = registerAddress;
	writeData[1] = registerData;

	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { MXC_GPIO_OutClr(GPIO_SPI2_SS1_ACC_PORT, GPIO_SPI2_SS1_ACC_PIN); }
	SpiTransaction(SPI_ACC, SPI_8_BIT_DATA_SIZE, YES, &writeData[0], sizeof(writeData), readData, dataSize, BLOCKING); // Does read data size need to be incremented by 1 for dummy byte?
	if (FT81X_SPI_2_SS_CONTROL_MANUAL) { MXC_GPIO_OutSet(GPIO_SPI2_SS1_ACC_PORT, GPIO_SPI2_SS1_ACC_PIN); }

	return (status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int SetAccFullScaleRange(uint16_t rangeInGravity)
{
	return (E_SUCCESS);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t VerifyAccManuIDAndPartID(void)
{
	return (E_SUCCESS);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#define ACC_CHANNEL_DATA_START_REGISTER 0x1F
#define ACC_CHANNEL_DATA_SIZE			6
void GetAccelerometerChannelData(ACC_DATA_STRUCT* channelData)
{
	uint8_t registerAddress = ACC_CHANNEL_DATA_START_REGISTER;
	uint8_t chanData[6];
	int x, y, z;

	// Data read in MSB/LSB (big endian), so conversion necessary
	GetAccRegister(registerAddress, &chanData[0], ACC_CHANNEL_DATA_SIZE);

	x = (chanData[0] << 8) | (chanData[1]);
	y = (chanData[2] << 8) | (chanData[3]);
	z = (chanData[4] << 8) | (chanData[5]);

	// Endian swap and normalize around 0x8000
	channelData->x = ((uint16_t)x + 0x8000);
	channelData->y = ((uint16_t)y + 0x8000);
	channelData->z = ((uint16_t)z + 0x8000);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t VerifyAccCommandTestResponse(void)
{
	return (E_SUCCESS);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void StartAccelerometerAquisition(void)
{
	SetupSPI2_Accelerometer(ON);

	// Start operating
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void StopAccelerometerAquisition(void)
{
	// Stop operating

	SetupSPI2_Accelerometer(OFF);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitAccControl(void)
{

}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestAccelerometer(void)
{
	debug("Accelerometer: Test device access...\r\n");

	VerifyAccManuIDAndPartID();

	if (VerifyAccCommandTestResponse() == PASSED) { debug("Acc: Command Test response passed\r\n"); }
	else { debugErr("Acc: Command Test response failed\r\n"); }

	//debug("Accelerometer: Setting manual sleep\r\n");
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void IssueAccSoftwareReset(void)
{

}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AccelerometerInit(void)
{
	uint8_t deviceConfig = 0;
	uint8_t driveConfig = 0;

	SetupSPI2_Accelerometer(ON);

	GetAccRegister(MPUREG_DEVICE_CONFIG, &deviceConfig, 1);
	GetAccRegister(MPUREG_DRIVE_CONFIG, &driveConfig, 1);

	if ((deviceConfig == 0x00) && (driveConfig == 0x05)) { debug("Accelerometer device and comms verified\r\n"); }
	else { debugWarn("Accelerometer device or comms error\r\n"); }

	uint8_t powerMgmt = 0;
	uint8_t accelConfig = 0;

	GetAccRegister(MPUREG_PWR_MGMT_0, &powerMgmt, 1);
	GetAccRegister(MPUREG_ACCEL_CONFIG0, &accelConfig, 1);

	debug("Accelerometer power mgmt is 0x%x\r\n", powerMgmt);
	debug("Accelerometer config is 0x%x\r\n", accelConfig);

#if 0 /* Test setting scale to 2G */
	accelConfig = 0x66;
	SetAccRegister(MPUREG_ACCEL_CONFIG0, accelConfig);
#endif

	uint8_t tempData[8];
	float temperature;

	GetAccRegister(MPUREG_TEMP_DATA1_UI, &tempData[0], 2);

	// Temperature in Degrees Centigrade = (TEMP_DATA / 132.48) + 25
	temperature = tempData[1] << 8 | tempData[0];
	temperature = ((temperature / 132.48) + 25);
	debug("Accelerometer: Temperature reading %0.2fC (%0.2fF)\r\n", (double)temperature, (double)(((temperature * 9) / 5) + 32));

	powerMgmt = 0x03;
	SetAccRegister(MPUREG_PWR_MGMT_0, powerMgmt);
	debug("Accelerometer: Turning on mode Low Noise (LN)\r\n");

	ACC_DATA_STRUCT accData;
	GetAccelerometerChannelData(&accData);
	debug("Accelerometer: X %x Y %x Z %x\r\n", accData.x, accData.y, accData.z);
	GetAccelerometerChannelData(&accData);
	debug("Accelerometer: X %x Y %x Z %x\r\n", accData.x, accData.y, accData.z);
	GetAccelerometerChannelData(&accData);
	debug("Accelerometer: X %x Y %x Z %x\r\n", accData.x, accData.y, accData.z);

#if 0 /* Power down */
	powerMgmt = 0x01;
	SetAccRegister(MPUREG_PWR_MGMT_0, powerMgmt);
	debug("Accelerometer: Turning off\r\n");

	SetupSPI2_Accelerometer(OFF);
#endif
}
#endif /* IIM-42352 */
