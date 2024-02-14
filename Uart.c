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
#include <stdarg.h>
#include <string.h>
#include "Typedefs.h"
#include "OldUart.h"
#include "Menu.h"
#include "RemoteCommon.h"
#include "PowerManagement.h"

#include <stdint.h>
#include "mxc_errors.h"
#include "i2c.h"
#include "uart.h"
#include "mxc_delay.h"
#include "cdc_acm.h"

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
// Make sure there is enough buffer space for comparing remote command 1K message sizes
static char s_uartBuffer[(1024 + 256)];

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 NibbleToA(uint8 hexData)
{
	uint8 hexNib = (uint8)(0x0F & hexData);

	if (hexNib <= 0x9)
	{
		// Got a hexNibble between '0' and '9', convert to the ascii representation.
		return (uint8)(hexNib + 0x30);
	}
	else if ((hexNib >= 0xA) && (hexNib <= 0xF))
	{
		// Got a hexNibble between 'A' and 'F', convert to the ascii representation.
		return (uint8)(hexNib + 0x37);
	}
	else // Should get here
	{
		return (0);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 ModemPutc(uint8 byteData, uint8 convertAsciiFlag)
{
	uint8 status = MODEM_SEND_FAILED;
	uint8 hexData;
	uint8 asciiData;
	volatile uint32 timeout = g_lifetimeHalfSecondTickCount + (25 * 2); //Set timeout to 25 secs

	// Make sure a modem is connected
	if (READ_DSR == MODEM_CONNECTED)
	{
		// Check clear to send and check if the timeout has been exceeded
		while ((NOT_READY_TO_SEND == READ_CTS) && (timeout > g_lifetimeHalfSecondTickCount))
		{
			// Check if the connection has been lost
			if (READ_DCD == NO_CONNECTION)
			{
				// Return immediately
				return (status);
			}
		}

		// Check if the timeout condition hasn't been exceeded yet
		if (timeout > g_lifetimeHalfSecondTickCount)
		{
			if (convertAsciiFlag == CONVERT_DATA_TO_ASCII)
			{
				// Convert the top nibble to hex
				hexData = (uint8)((0xF0 & byteData) >> 4);
				asciiData = NibbleToA(hexData);

				// Send the top nibble
				UartPutc(asciiData, CRAFT_COM_PORT);

				// Convert the bottom nibble to hex
				hexData = (uint8)(0x0F & byteData);
				asciiData = NibbleToA(hexData);

				// Send the bottom nibble
				UartPutc(asciiData, CRAFT_COM_PORT);
			}
			else
			{
				// Send the byte of data
				UartPutc(byteData, CRAFT_COM_PORT);
			}

			// Set status to success because data has been sent
			status = MODEM_SEND_SUCCESS;
		}
	}

	return (status);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 ModemPuts(uint8* byteData, uint32 dataLength, uint8 convertAsciiFlag)
{
	uint32 dataDex;
	uint8* theData = byteData;

	// Sending modem data, signal that data is being transfered
	g_modemDataTransfered = YES;

	for (dataDex = 0; dataDex < dataLength; dataDex++)
	{
#if 0 /* Exception testing (Prevent non-ISR soft loop watchdog from triggering) */
		g_execCycles++;
#endif
		if (MODEM_SEND_FAILED == ModemPutc(*theData, convertAsciiFlag))
		{
			return (MODEM_SEND_FAILED);
		}

		theData++;
	}
	return (MODEM_SEND_SUCCESS);
}


///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UartPutc(uint8 c, int32 channel)
{
	mxc_uart_regs_t* port;

	// Check if channel is USB CDC/ACM serial
	if (channel == CRAFT_COM_PORT)
	{
        // Verfiy USB serial channel is available
		if (acm_present())
		{
            if (acm_write(&c, sizeof(c)) != sizeof(c))
			{
				debugErr("USB CDC/ACM serial transfer failed trying to send <%c>\r\n", c);
			}
		}
		else { debugErr("USB CDC/ACM serial unavailable\r\n"); }
	}
	else // channel is UART serial
	{
		if (channel == LTE_COM_PORT) { port = MXC_UART0; }
		else if (channel == BLE_COM_PORT) { port = MXC_UART1; }
		else /* (channel == GLOBAL_DEBUG_PRINT_PORT) */ { port = MXC_UART2; }

#if 1 /* Framework driver blocks waiting forever for TX FIFO space to be available */
		MXC_UART_WriteCharacter(port, c);
#else /* Manage timeout ourselves */
		uint32 retries = 100; //USART_DEFAULT_TIMEOUT;
		int status;

		// Dump the character to the serial port
		status = MXC_UART_RevA_WriteCharacterRaw(port, c);

		while ((status == E_OVERFLOW) && (retries))
		{
			retries--;
			status = MXC_UART_RevA_WriteCharacterRaw(port, c);
		}
#endif
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UartWrite(void* b, int32 n, int32 channel)
{
	char* s = (char*)b;
	while (n--)
	{
		if (*s == '\n')
		{
			UartPutc('\r', channel);
		}

		UartPutc(*s++, channel);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UartPuts(char* s, int32 channel)
{
	while (*s)
	{
		if (*s == '\n')
		{
			UartPutc('\r', channel);
		}

		UartPutc(*s++, channel);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 UartCharWaiting(int32 channel)
{
	mxc_uart_regs_t* port;

	if (channel == LTE_COM_PORT) { port = MXC_UART0; }
	else if (channel == BLE_COM_PORT) { port = MXC_UART1; }
	else /* (channel == GLOBAL_DEBUG_PRINT_PORT) */ { port = MXC_UART2; }

	return (MXC_UART_GetRXFIFOAvailable(port));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 UartGetc(int32 channel, uint8 mode)
{
	mxc_uart_regs_t* port;
	volatile uint32 uartTimeout = UART_TIMEOUT_COUNT;

	if (channel == LTE_COM_PORT) { port = MXC_UART0; }
	else if (channel == BLE_COM_PORT) { port = MXC_UART1; }
	else /* (channel == GLOBAL_DEBUG_PRINT_PORT) */ { port = MXC_UART2; }

	if (mode == UART_BLOCK)
	{
		// Read char is a forever blocking call
		return (MXC_UART_ReadCharacter(port));
	}
	else // mode == UART_TIMEOUT
	{
		while (!UartCharWaiting(channel))
		{
			if ((mode == UART_TIMEOUT) && (uartTimeout-- == 0))
				return (UART_TIMED_OUT);
		}

		return (MXC_UART_ReadCharacterRaw(port));
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
char* UartGets(char* s, int32 channel)
{
	char* b = s;
	BOOLEAN end = FALSE;
	int32 data;
	int32 count = 0;

	do
	{
		data = UartGetc(channel, UART_BLOCK);
		switch (data)
		{
			case '\b':
			case 0x7e:
				if (count)
				{
					count--;
					b--;
					if (channel != CRAFT_COM_PORT)
					{
						UartPuts("\b \b", channel);
					}
				}
				break;
			case '\r':
			case '\n':
				if (count)
				{
					*b = 0;
					if (channel != CRAFT_COM_PORT)
					{
						UartPuts("\r\n", channel);
					}
					end = TRUE;
				}
				break;
			case CAN_CHAR:
				*b = CAN_CHAR;
				*s = CAN_CHAR;
				end = TRUE;
				break;
			default:
				if (count < 255)
				{
					count++;
					*b++ = (char)data;
					if (channel != CRAFT_COM_PORT)
					{
						UartPutc((uint8)data, channel); // Echo the data back
					}
				}
				break;
		}
	} while (!end);
	if (*b != CAN_CHAR)
	{
		*b = 0;
	}
	return (s);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
short Craft(char* fmt, ...)
{
	va_list arg_ptr;
	short l;
#if 0 /* Original */
	char buf[256];
#else /* Make sure there is enough buffer space for printing remote command 1K message sizes */
	char buf[(1024 + 256)];
#endif
	static uint32 repeatingBuf = 0;
	char repeatCountStr[10];

	// Initialize arg_ptr to the begenning of the variable argument list
	va_start(arg_ptr, fmt);

	// Build the string in buf with the format fmt and arguments in arg_ptr
	l = (short)vsprintf(buf, fmt, arg_ptr);

	// Clean up. Invalidates arg_ptr from use again
	va_end(arg_ptr);

	// Check if the current string to be printed was different than the last
	if (strncmp(buf, s_uartBuffer, l))
	{
		// Check if the previous string repeated at all
		if (repeatingBuf > 0)
		{
			// Print the repeat count of the previous repeated string
			sprintf(repeatCountStr, "(%d)\n", (int)repeatingBuf);
			UartPuts(repeatCountStr, CRAFT_COM_PORT);

			// Reset the counter
			repeatingBuf = 0;
		}

		// Copy the new string into the global buffer
		strncpy(s_uartBuffer, buf, l);

		// Print the new string
		UartWrite(buf, l, CRAFT_COM_PORT);
	}
	else // Strings are equal
	{
		// Increment the repeat count
		repeatingBuf++;

		// Print a '!' (bang) so signify that the output was repeated
		UartPutc('!', CRAFT_COM_PORT);
	}

	// Return the number of characters
	return (l);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
short DebugPrint(uint8_t mode, char* fmt, ...)
{
	va_list arg_ptr;
	short length = 0;
#if 0 /* Original */
	char buf[256];
#else /* Make sure there is enough buffer space for printing remote command 1K message sizes */
	char buf[(1024 + 256)];
#endif
	static uint32 repeatingBuf = 0;
	static uint8 strippedNewline = 0;
	char repeatCountStr[10];
	char timestampStr[8];
	int32 tempTime;

	//if (g_disableDebugPrinting == YES)
	//	return (0);

	// Initialize the buffer
	memset(&buf[0], 0, sizeof(buf));

	// Initialize arg_ptr to the beginning of the variable argument list
	va_start(arg_ptr, fmt);

	switch (mode)
	{
		case RAW: break;
		case NORM:	strcpy(buf, "(Debug |        ) "); break;
		case WARN:	strcpy(buf, "(Warn  -        ) "); break;
		case ERR:	strcpy(buf, "(Error *        ) "); break;
		default: break;
	}

	if (mode == RAW)
	{
		// Build the string in buf with the format fmt and arguments in arg_ptr
		length = (short)vsprintf(buf, fmt, arg_ptr);
	}
	else
	{
		// Build the string in buf with the format fmt and arguments in arg_ptr

		// Initialize the length to the number of chars in the debug string
		length = 18;

		// Offset the buf array to start after debug string section
		length += (short)vsprintf(&buf[length], fmt, arg_ptr);
	}

	// Clean up. Invalidates arg_ptr from use again
	va_end(arg_ptr);

	if (mode == RAW)
	{
		// Print the raw string
		UartWrite(buf, length, GLOBAL_DEBUG_PRINT_PORT);
	}
	// Check if the current string to be printed was different than the last
	else if (strncmp(buf, s_uartBuffer, length))
	{
		// Check if the previous string repeated at all
		if (repeatingBuf > 0)
		{
			// Print the repeat count of the previous repeated string
			sprintf(repeatCountStr, "(%d)\n", (int)repeatingBuf);
			UartPuts(repeatCountStr, GLOBAL_DEBUG_PRINT_PORT);

			// Reset the counter
			repeatingBuf = 0;
		}
		else if (strippedNewline == YES)
		{
			// Issue a carrige return and a line feed
			UartPutc('\r', GLOBAL_DEBUG_PRINT_PORT);
			UartPutc('\n', GLOBAL_DEBUG_PRINT_PORT);

			// Reset the flag
			strippedNewline = NO;
		}

		// Copy the new string into the global buffer
		strncpy(s_uartBuffer, buf, length);

		tempTime = g_lifetimeHalfSecondTickCount >> 1;

		// Put timestamp into a formatted string
		if (tempTime < 60)
		{
			sprintf(timestampStr, "%6ds", (int)tempTime);
		}
		else if (tempTime < 3600)
		{
			sprintf(timestampStr, "%2dm,%2ds", (int)(tempTime/60), (int)(tempTime%60));
		}
		else if (tempTime < 86400)
		{
			sprintf(timestampStr, "%2dh,%2dm", (int)(tempTime/3600), (int)((tempTime%3600)/60));
		}
		else
		{
			sprintf(timestampStr, "%2dd,%2dh", (int)(tempTime/86400), (int)((tempTime%86400)/3600));
		}

		// Add in the timestamp at print buffer location offset 9
		strncpy(&buf[9], timestampStr, 7);

		// For repeat '!' (bang) processing, look for an ending newline
		if (buf[length - 1] == '\n')
		{
			// Reduce length by one
			length--;

			// Check for a preceding carriage return
			if (buf[length - 1] == '\r')
			{
				length--;
			}

			// Strip trailing newline and replace with a null
			buf[length] = '\0';

			// Set the flag
			strippedNewline = YES;
		}
		// Check for an ending newline carriage return combo
		else if ((buf[length - 2] == '\n') && (buf[length - 1] == '\r'))
		{
			// Reduce length by one
			length -= 2;

			// Strip trailing newline and replace with a null
			buf[length] = '\0';

			// Set the flag
			strippedNewline = YES;
		}

		// Print the new string
		UartWrite(buf, length, GLOBAL_DEBUG_PRINT_PORT);
	}
	else // Strings are equal
	{
		// Increment the repeat count
		repeatingBuf++;

		// Print a '!' (bang) so signify that the output was repeated
		UartPutc('!', GLOBAL_DEBUG_PRINT_PORT);
	}

	// Return the number of characters
	return (length);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DebugPrintChar(uint8 charData)
{
	if (g_disableDebugPrinting == NO)
	{
		UartPutc(charData, GLOBAL_DEBUG_PRINT_PORT);
	}
}

///============================================================================
///----------------------------------------------------------------------------
///	PI7C9X760 - I2C-bus/SPI to UART Bridge Controller w/ 64 bytes of TX/RX FIFOs
///----------------------------------------------------------------------------
///============================================================================
#define PI7C9X760_REG_RHR		0x00 // Receive Holding Register, 		Notes: Accessible when LCR[7]=0. Default=00
#define PI7C9X760_REG_THR		0x00 // Transmit Holding Register, 		Notes: Accessible when LCR[7]=0. Default=00
#define PI7C9X760_REG_DLL		0x00 // Divisor Latch LSB, 				Notes: Accessible when LCR[7]=1 and LCR!=0xBF. Default=01
#define PI7C9X760_REG_IER		0x01 // Interrupt Enable Register, 		Notes: Accessible when LCR[7]=0. Default=00
#define PI7C9X760_REG_DLH		0x01 // Divisor Latch MSB, 				Notes: Accessible when LCR[7]=1 and LCR!=0xBF. Default=00
#define PI7C9X760_REG_IIR		0x02 // Interrupt Id Register, 			Notes: Accessible when LCR[7]=0. Default=01
#define PI7C9X760_REG_FCR		0x02 // FIFO Control Register, 			Notes: Accessible when LCR[7]=0. Default=00
#define PI7C9X760_REG_EFR		0x02 // Enhanced Feature Register, 		Notes: Accessible when LCR=0xBF and SFR[2]=0. Default=00
#define PI7C9X760_REG_LCR		0x03 // Line Control Register, 			Notes: Default=1D (I argue this is supposed to be 0x1B)
#define PI7C9X760_REG_MCR		0x04 // Modem Control Register, 		Notes: Accessible when LCR[7]=0. Default=00
#define PI7C9X760_REG_XON1		0x04 // XON1 Character Register, 		Notes: Accessible when LCR=0xBF and SFR[2]=0. Default=00
#define PI7C9X760_REG_LSR		0x05 // Line Status Register, 			Notes: Accessible when LCR[7]=0. Default=60
#define PI7C9X760_REG_XON2		0x05 // XON2 Character Register, 		Notes: Accessible when LCR=0xBF and SFR[2]=0. Default=00
#define PI7C9X760_REG_MSR		0x06 // Modem Status Register, 			Notes: Accessible when LCR[7]=0 and MCR[2]=0 and SFR[2]=0. Default=00
#define PI7C9X760_REG_TCR		0x06 // Transmission Control Register, 	Notes: Accessible when EFR[4]=1 and MCR[2]=1 and SFR[2]=0. Default=00
#define PI7C9X760_REG_XOFF1		0x06 // XOFF1 Character Register, 		Notes: Accessible when LCR=0xBF and SFR[2]=0. Default=00
#define PI7C9X760_REG_SPR		0x07 // Scratch Pad Register, 			Notes: Accessible when LCR[7]=0 and MCR[2]=0. Default=FF
#define PI7C9X760_REG_TLR		0x07 // Trigger Level Register, 		Notes: Accessible when EFR[4]=1 and MCR[2]=1. Default=00
#define PI7C9X760_REG_XOFF2		0x07 // XOFF2 Character Register,		Notes: Accessible when LCR=0xBF and SFREN!=0x5A. Default=00
#define PI7C9X760_REG_TXLVL		0x08 // Transmit FIFO Level Register,	Notes: Accessible when SFR[2]=0. Default=40
#define PI7C9X760_REG_RXLVL		0x09 // Receive FIFO Level Register,	Notes: Accessible when SFR[2]=0. Default=00
#define PI7C9X760_REG_IODIR		0x0A // GPIO Direction Register,		Notes: Default=00
#define PI7C9X760_REG_IOSTATE	0x0B // GPIO State Register,			Notes: Default=FF
#define PI7C9X760_REG_IOINTEN	0x0C // GPIO Interrupt Enable Register,	Notes: Default=00
#define PI7C9X760_REG_IOCONTROL	0x0E // GPIO Control Register,			Notes: Default=00
#define PI7C9X760_REG_EFCR		0x0F // Extra Features Control Reg,		Notes: Accessable when SFR[2]=0, Default=00
#define PI7C9X760_REG_SFREN		0x0D // Special Features Enable Ctrl,	Notes: Accessible when LCR==8'hBF. Default=00 (Spec typo?)
#define PI7C9X760_REG_ASR		0x02 // Advance Status Register,		Notes: Accessible when LCR=0xBF and SFR[2]=1. Default=00
#define PI7C9X760_REG_CPR		0x04 // Clock Prescale Register,		Notes: Accessible when LCR=0xBF and SFR[2]=1. Default=10
#define PI7C9X760_REG_RFD		0x05 // Rcv FIFO Data Count Register,	Notes: Accessible when LCR=0xBF and SFR[2]=1, SFR[6]=0. Default=00
#define PI7C9X760_REG_RLS		0x05 // Receive Line Error Status Cnt,	Notes: Accessible when LCR=0xBF and SFR[2]=1, SFR[6]=1. Default=00
#define PI7C9X760_REG_TFD		0x06 // Tx FIFO Data Count Register,	Notes: Accessible when LCR=0xBF and SFR[2]=1. Default=00
#define PI7C9X760_REG_SFR		0x07 // Special Function Register,		Notes: Accessible when LCR=0xBF and SFREN==0x5A. Default=00
#define PI7C9X760_REG_TIIDLE	0x08 // Tx Idle Time Count Register,	Notes: Accessible when LCR=0xBF and SFR[2]=1. Default=00
#define PI7C9X760_REG_TRCTL		0x09 // Tx/Rx Control Register,			Notes: Accessible when LCR=0xBF and SFR[2]=1. Default=06
#define PI7C9X760_REG_ISCR		0x0F // Interrupt Status/Clear Reg,		Notes: Accessible when LCR=0xBF and SFR[2]=1. Default=00

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GetUartBridgeRegisters(uint8_t registerAddress, uint8_t* registerData, uint16_t dataLength)
{
    WriteI2CDevice(MXC_I2C1, I2C_ADDR_EXPANSION, &registerAddress, sizeof(uint8_t), registerData, dataLength);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetUartBridgeRegisters(uint8_t registerAddress, uint8_t* registerData, uint16_t dataLength)
{
	g_spareBuffer[0] = registerAddress;
	memcpy(&g_spareBuffer[1], registerData, dataLength);

    WriteI2CDevice(MXC_I2C1, I2C_ADDR_EXPANSION, g_spareBuffer, (dataLength + 1), NULL, 0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void WriteUartBridgeControlRegister(uint8_t registerAddress, uint8_t registerData)
{
	uint8_t writeData[2];

	writeData[0] = registerAddress;
	writeData[1] = registerData;

    WriteI2CDevice(MXC_I2C1, I2C_ADDR_EXPANSION, writeData, sizeof(writeData), NULL, 0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t ReadUartBridgeControlRegister(uint8_t registerAddress)
{
	uint8_t readData;

    WriteI2CDevice(MXC_I2C1, I2C_ADDR_EXPANSION, &registerAddress, sizeof(registerAddress), &readData, sizeof(readData));

	return (readData);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestUartBridgeScratchpad(void)
{
	uint8_t reg;

	//debug("Expansion I2C Uart Bridge: Scratchpad test\r\n");

	// Offset 07H: Scratch Pad Register (SPR). Accessible when LCR[7]=0 and MCR[2]=0. Default=FF

	reg = ReadUartBridgeControlRegister(PI7C9X760_REG_LCR);
	debug("Expansion: LCR Register is 0x%x\r\n", reg);
	// Check if LCR bit 7 is enabled
	if (reg & 0x80)
	{
		debug("Expansion: LCR bit enabled\r\n");
		// Disable LCR bit 7 so that SPR is available
		reg &= 0x80;
		WriteUartBridgeControlRegister(PI7C9X760_REG_LCR, reg);

		reg = ReadUartBridgeControlRegister(PI7C9X760_REG_LCR);
		if (reg & 0x80) { debugErr("Expansion: LCR bit disable failed\r\n"); }
	}
	else { debug("Expansion: LCR bit disabled\r\n"); }

	reg = ReadUartBridgeControlRegister(PI7C9X760_REG_MCR);
	debug("Expansion: MCR Register is 0x%x\r\n", reg);
	// Check if MCR bit 2 is enabled
	if (reg & 0x04)
	{
		debug("Expansion: MCR bit enabled\r\n");
		// Disable MCR bit 2 so that SPR is available
		reg &= 0x04;
		WriteUartBridgeControlRegister(PI7C9X760_REG_MCR, reg);

		reg = ReadUartBridgeControlRegister(PI7C9X760_REG_MCR);
		if (reg & 0x04) { debugErr("Expansion: MCR bit disable failed\r\n"); }
	}
	else { debug("Expansion: MCR bit disabled\r\n"); }

	reg = ReadUartBridgeControlRegister(PI7C9X760_REG_SPR);
	if (reg != 0xFF) { debugErr("Expansion I2C Uart Bridge: Scratchpad default error (0x%x)\r\n", reg); }
	else { debug("Expansion I2C Uart Bridge: Scratchpad default correct\r\n"); }
	WriteUartBridgeControlRegister(PI7C9X760_REG_SPR, 0xAA);
	reg = ReadUartBridgeControlRegister(PI7C9X760_REG_SPR);
#if 0 /* Shorter */
	debug("Expansion I2C Uart Bridge: 1st scracth test: %s\r\n", (reg == 0xAA) ? "Passed" : "Failed");
#else
	if (ReadUartBridgeControlRegister(PI7C9X760_REG_SPR) == 0xAA) { debug("Expansion I2C Uart Bridge: 1st scracth test passed\r\n"); }
	else { debugErr("Expansion I2C Uart Bridge: 1st scracth test failed (0x%x)\r\n", reg); }
#endif

	WriteUartBridgeControlRegister(PI7C9X760_REG_SPR, 0x55);
	reg = ReadUartBridgeControlRegister(PI7C9X760_REG_SPR);
#if 0 /* Shorter */
	debug("Expansion I2C Uart Bridge: 2nd scracth test: %s\r\n", (reg == 0x55) ? "Passed" : "Failed");
#else
	if (ReadUartBridgeControlRegister(PI7C9X760_REG_SPR) == 0x55) { debug("Expansion I2C Uart Bridge: 2nd scracth test passed\r\n"); }
	else { debugErr("Expansion I2C Uart Bridge: 2nd scracth test failed (0x%x)\r\n", reg); }
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SleepUartBridgeDevice(void)
{
	/*
		The UART may enter sleep mode when all conditions met:
		- no interrupts pending
		- modem inputs are not toggled
		- RX input pin is idling HIGH
		- TX/RX FIFO are empty

		It will exit from sleep mode when any below condition met:
		- modem inputs are toggling
		- RX input pin changed to LOW
		- A data byte is loaded to the TX FIFO

		In sleep mode, Crystal is stopped and no UART clock

		Sleep mode enabling requires EFR[4] = 1
	*/

	// Need external pull up on RX input pin to allow sleep, fortunately there are two other ways to sleep the device (Exp enable and Exp reset)

	// Clear Interrupts
	//Offset 0FH: Interrupt Status and Clear Register (ISCR). Accessible when LCR=0xBF and SFR[2]=1. Default=00
	//	Bit 0 set to logic 1
	WriteUartBridgeControlRegister(PI7C9X760_REG_LCR, 0xBF);
	WriteUartBridgeControlRegister(PI7C9X760_REG_SFR, 0x04);
	WriteUartBridgeControlRegister(PI7C9X760_REG_ISCR, 0x01);

	// Reset FIFO's
	//Offset 02H: FIFO Control Register (FCR). Accessible when LCR[7]=0. Default=00
	//	Bits 1 & 2 setting logic 1 resets Tx/Rx FIFOs
	WriteUartBridgeControlRegister(PI7C9X760_REG_LCR, 0x1D);
	WriteUartBridgeControlRegister(PI7C9X760_REG_FCR, 0x06);

	// Enable access to sleep mode (IER bit 4)
	//Offset 02H: Enhanced Feature Register (EFR). Accessible when LCR=0xBF and SFR[2]=0. Default=00
	//	Bit 4 set to logic 1
	WriteUartBridgeControlRegister(PI7C9X760_REG_LCR, 0xBF);
	WriteUartBridgeControlRegister(PI7C9X760_REG_SFR, 0x00);
	WriteUartBridgeControlRegister(PI7C9X760_REG_EFR, 0x10);

	// Enable sleep
	//Offset 01H: Interrupt Enable Register (IER). Accessible when LCR[7]=0. Default=00
	//	Bit 4 set to logic 1
	WriteUartBridgeControlRegister(PI7C9X760_REG_LCR, 0x1D);
	WriteUartBridgeControlRegister(PI7C9X760_REG_IER, 0x10);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestExpansionI2CBridge(void)
{
    debug("Expansion I2C Uart Bridge: Test device access...\r\n");

    if (GetPowerControlState(EXPANSION_ENABLE) == OFF)
	{
		debug("Power Control: Expansion I2C UART bridge enable being turned on\r\n");
		PowerControl(EXPANSION_ENABLE, ON);
		MXC_Delay(MXC_DELAY_MSEC(500));
		PowerControl(EXPANSION_RESET, OFF);
	}
	else { debugWarn("Power Control: Expansion I2C UART bridge enable already on\r\n"); }

	debug("Expansion I2C Uart Bridge: Scratchpad test...\r\n");
	TestUartBridgeScratchpad();
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ExpansionBridgeInit(void)
{
	PowerControl(EXPANSION_ENABLE, ON);
	MXC_Delay(MXC_DELAY_MSEC(500));
	PowerControl(EXPANSION_RESET, OFF);
	MXC_Delay(MXC_DELAY_MSEC(250));

#if 0 /* Test read for scope */
	debug("Expansion I2C Uart Bridge: Forever read LCD register...\r\n");
	while (1)
	{
		ReadUartBridgeControlRegister(PI7C9X760_REG_LCR);
		MXC_Delay(50);
	}
#endif

	debug("Expansion I2C Uart Bridge: Powered on, Scratchpad test...\r\n");
	TestUartBridgeScratchpad();

	// Make sure Expansion bridge is turned off
	if (GetPowerControlState(EXPANSION_ENABLE) == ON)
	{
		PowerControl(EXPANSION_RESET, ON);
		debug("Power Control: Expansion I2C UART bridge being turned off\n");
		PowerControl(EXPANSION_ENABLE, OFF);
	}
}
