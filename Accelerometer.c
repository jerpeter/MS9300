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
#include "mxc_delay.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define ACC_MAN_ID_REGISTER 0x00
#define ACC_CHANNEL_DATA_START_REGISTER 0x08
#define ACC_CHANNEL_DATA_SIZE   6
#define ACC_COMMAND_TEST_RESPONSE_REGISTER  0x12
#define ACC_CONTROL_1_REGISTER  0x1B
#define ACC_CONTROL_2_REGISTER  0x1C
#define ACC_CONTROL_3_REGISTER  0x1D
#define ACC_CONTROL_4_REGISTER  0x1E
#define ACC_CONTROL_5_REGISTER  0x1F
#define ACC_CONTROL_6_REGISTER  0x20
#define ACC_INTERRUPT_CONTROL_1_REGISTER  0x22
#define ACC_SELF_TEST_REGISTER  0x5D

typedef struct
{
	uint16_t x;
	uint16_t y;
    uint16_t z;
} ACC_DATA_STRUCT;

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
#include "Globals.h"

///----------------------------------------------------------------------------
///	Local Scope Globals
///----------------------------------------------------------------------------

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
void GetAccRegister(uint8_t registerAddress, uint8_t* registerData)
{
    WriteI2CDevice(MXC_I2C0, I2C_ADDR_ACCELEROMETER, &registerAddress, sizeof(uint8_t), registerData, sizeof(uint8_t));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetAccRegister(uint8_t registerAddress, uint8_t registerData)
{
    uint8_t writeData[2];

    writeData[0] = registerAddress;
    writeData[1] = registerData;

    WriteI2CDevice(MXC_I2C0, I2C_ADDR_ACCELEROMETER, writeData, sizeof(writeData), NULL, 0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t VerifyAccManuIDAndPartID(void)
{
    uint8_t manuIDAndPartID[6];
    uint8_t registerAddress = ACC_MAN_ID_REGISTER;
    uint8_t status = FAILED;

   	WriteI2CDevice(MXC_I2C0, I2C_ADDR_ACCELEROMETER, &registerAddress, sizeof(uint8_t), manuIDAndPartID, sizeof(manuIDAndPartID));

    if ((manuIDAndPartID[0] == 'K') && (manuIDAndPartID[1] == 'i') && (manuIDAndPartID[2] == 'o') && (manuIDAndPartID[3] == 'n'))
    {
        status = PASSED;
        debug("Accelerometer Man ID verified, Part ID: 0x%x 0x%x\r\n", manuIDAndPartID[4], manuIDAndPartID[5]);
    }
    else
    {
        debugErr("Accelerometer Man ID failed verified, 0x%x, 0x%x 0x%x 0x%x\r\n", manuIDAndPartID[0], manuIDAndPartID[1], manuIDAndPartID[2], manuIDAndPartID[3]);
    }

    return (status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GetAccChannelData(ACC_DATA_STRUCT* channelData)
{
    uint8_t registerAddress = ACC_CHANNEL_DATA_START_REGISTER;
    uint8_t* chanDataBytePtr = (uint8_t*)channelData;

   	// Data read in LSB/MSB (little endian), so no conversion necessary
    WriteI2CDevice(MXC_I2C0, I2C_ADDR_ACCELEROMETER, &registerAddress, sizeof(uint8_t), chanDataBytePtr, ACC_CHANNEL_DATA_SIZE);

    channelData->x = (chanDataBytePtr[0] | (chanDataBytePtr[1] << 8));
    channelData->y = (chanDataBytePtr[2] | (chanDataBytePtr[3] << 8));
    channelData->z = (chanDataBytePtr[4] | (chanDataBytePtr[5] << 8));

    // Normalize around 0x8000
    if (channelData->x & 0x8000) { channelData->x = (0x7FFF - ~channelData->x); } else { channelData->x += 0x8000; }
    if (channelData->y & 0x8000) { channelData->y = (0x7FFF - ~channelData->y); } else { channelData->y += 0x8000; }
    if (channelData->z & 0x8000) { channelData->z = (0x7FFF - ~channelData->z); } else { channelData->z += 0x8000; }
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
    //WriteI2CDevice(MXC_I2C0, I2C_ADDR_ACCELEROMETER, &registerAddress, sizeof(uint8_t), &ctrData, sizeof(ctrData));
    GetAccRegister(ACC_COMMAND_TEST_RESPONSE_REGISTER, &ctrData);

    if (ctrData == 0x55)
    {
        // Write COTC bit in Control 2 register
        GetAccRegister(ACC_CONTROL_2_REGISTER, &registerData);
        registerData |= 0x40; // Enable PC1 bit
        SetAccRegister(ACC_CONTROL_2_REGISTER, registerData);

        // Read CTR register again
        //WriteI2CDevice(MXC_I2C0, I2C_ADDR_ACCELEROMETER, &registerAddress, sizeof(uint8_t), &ctrData, sizeof(ctrData));
        GetAccRegister(ACC_COMMAND_TEST_RESPONSE_REGISTER, &ctrData);

        if (ctrData == 0xAA)
        {
            // Read CTR register again
            //WriteI2CDevice(MXC_I2C0, I2C_ADDR_ACCELEROMETER, &registerAddress, sizeof(uint8_t), &ctrData, sizeof(ctrData));
            GetAccRegister(ACC_COMMAND_TEST_RESPONSE_REGISTER, &ctrData);

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
void StartAccAquisition(void)
{
    uint8_t registerData;

    GetAccRegister(ACC_CONTROL_1_REGISTER, &registerData);
    registerData |= 0x80; // Enable PC1 bit
    SetAccRegister(ACC_CONTROL_1_REGISTER, registerData);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void StopAccAquisition(void)
{
    uint8_t registerData;

    GetAccRegister(ACC_CONTROL_1_REGISTER, &registerData);
    registerData &= 0x7F; // Disable PC1 bit
    SetAccRegister(ACC_CONTROL_1_REGISTER, registerData);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitAccControl(void)
{
    uint8_t registerData;

    GetAccRegister(ACC_CONTROL_1_REGISTER, &registerData);

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
void AccelerometerInit(void)
{
    // Attempt to verify the part is present
    if (VerifyAccManuIDAndPartID() == PASSED)
    {
        // Put the Acc in standby mode to allow changing control register values
        SetAccRegister(ACC_CONTROL_1_REGISTER, 0x00);

        uint8_t testData;
        GetAccRegister(ACC_CONTROL_2_REGISTER, &testData);
        if (testData == 0x3F) { debug("Accelerometer: Ctrl2 is default value\r\n"); }
        else { debug("Accelerometer: Ctrl2 is non-default value of 0x%x\r\n", testData); }

        GetAccRegister(ACC_CONTROL_3_REGISTER, &testData);
        if (testData == 0xA8) { debug("Accelerometer: Ctrl3 is default value\r\n"); }
        else { debug("Accelerometer: Ctrl3 is non-default value of 0x%x\r\n", testData); }
        SetAccRegister(ACC_CONTROL_3_REGISTER, 0x00);
        GetAccRegister(ACC_CONTROL_3_REGISTER, &testData);
        if (testData == 0) { debug("Accelerometer: Ctrl3 was changed successfully\r\n"); }
        else { debug("Accelerometer: Ctrl3 not changed correctly, returns a non-zero value of 0x%x\r\n", testData); }
        //SetAccRegister(ACC_CONTROL_3_REGISTER, 0xA8);

        GetAccRegister(ACC_CONTROL_4_REGISTER, &testData);
        if (testData == 0x40) { debug("Accelerometer: Ctrl4 is default value\r\n"); }
        else { debug("Accelerometer: Ctrl4 is non-default value of 0x%x\r\n", testData); }

#if 0 /* Attempt Self test */
        // 1. Set STPOL bit to 1 in INC1 register to set the polarity of the self-test mode to positive.
        // 2. Set PC1 bit to 1 in CNTL1 register to enable KX134-1211.
        // 3. Write 0xCA to this register to enable the MEMS self-test function.
        debug("Accelerometer: Starting Self-test...\r\n");
        GetAccRegister(ACC_INTERRUPT_CONTROL_1_REGISTER, &testData);
        testData |= 0x02;
        SetAccRegister(ACC_INTERRUPT_CONTROL_1_REGISTER, testData);

        GetAccRegister(ACC_CONTROL_1_REGISTER, &testData);
        testData |= 0x80;
        SetAccRegister(ACC_CONTROL_1_REGISTER, testData);

        SetAccRegister(ACC_SELF_TEST_REGISTER, 0xCA);

        MXC_Delay(MXC_DELAY_SEC(5));

        SetAccRegister(ACC_CONTROL_1_REGISTER, 0x00);
        SetAccRegister(ACC_SELF_TEST_REGISTER, 0x00);
        debug("Accelerometer: Self-test disabled\r\n");
        MXC_Delay(MXC_DELAY_SEC(1));
#endif

#if 0 /* Test reading data */
        StartAccAquisition();
        ACC_DATA_STRUCT channelData;

        //for (uint8_t i = 0; i < 4; i++)
        while (1)
        {
            GetAccChannelData(&channelData);
            debug("Accelerometer (%s): X:%04x Y:%04x Z:%04x\r\n", FuelGaugeDebugString(), channelData.x, channelData.y, channelData.z);
            MXC_Delay(MXC_DELAY_MSEC(250));
        }

        StopAccAquisition();
#endif

        // Put the Accelerometer in manual sleep
        SetAccRegister(ACC_CONTROL_5_REGISTER, 0x01);
        debug("Accelerometer: Manual sleep engaged\r\n");
    }
}
