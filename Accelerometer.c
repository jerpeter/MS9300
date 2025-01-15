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
#if /* New board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN)
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

#if /* New board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN)
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

#if /* New board */ (HARDWARE_BOARD_REVISION == HARDWARE_ID_REV_BETA_RESPIN)
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
void GetAccChannelData(ACC_DATA_STRUCT* channelData)
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
void StartAccAquisition(void)
{
    uint8_t registerData;

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
void StopAccAquisition(void)
{
    uint8_t registerData;

    GetAccRegister(ACC_CONTROL_1_REGISTER, &registerData, sizeof(uint8_t));
    registerData &= 0x7F; // Disable PC1 bit
    SetAccRegister(ACC_CONTROL_1_REGISTER, registerData);
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
    g_spi2State |= SPI2_ACC_ON;
    if ((g_spi2State & SPI2_OPERAITONAL) == NO) { SetupSPI2_LCDAndAcc(); g_spi2State |= SPI2_OPERAITONAL; }

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
        StartAccAquisition();
extern volatile uint32_t g_lifetimePeriodicSecondCount;
        volatile uint32_t secCount = g_lifetimePeriodicSecondCount;
        uint32_t loopCount = 0;
        ACC_DATA_STRUCT channelData;

        while (secCount == g_lifetimePeriodicSecondCount) {}
        secCount = g_lifetimePeriodicSecondCount + 3;

        while (secCount != g_lifetimePeriodicSecondCount)
        {
            GetAccChannelData(&channelData);
            loopCount++;
        }
        StopAccAquisition();

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
        StartAccAquisition();
        ACC_DATA_STRUCT channelData;

        //for (uint8_t i = 0; i < 4; i++)
        while (1)
        {
            GetAccChannelData(&channelData);
            debug("Accelerometer (%s): X:%04x Y:%04x Z:%04x\r\n", FuelGaugeDebugString(), channelData.x, channelData.y, channelData.z);
            MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(250));
        }

        StopAccAquisition();
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

    g_spi2State &= ~SPI2_ACC_ON;
	// Check if SPI2 is not active for the LCD
	if ((g_spi2State & SPI2_LCD_ON) == NO) { MXC_SPI_Shutdown(MXC_SPI2); g_spi2State &= ~SPI2_OPERAITONAL; } // Mark SPI2 state shutdown
}
