///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

#ifndef _UART_H_
#define _UART_H_

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include "Typedefs.h" 
#include "gpio.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define BAUD_TEST_57600		0x55	/* 57600 autobaud test character */
#define BAUD_TEST_38400		0x92	/* 38400 autobaud test character */
#define BAUD_TEST_19200		0x1C	/* 19200 autobaud test character */
#define BAUD_TEST_9600		0xE0	/* 9600 autobaud test character */
#define UART_BLOCK			0
#define UART_TIMEOUT		1
#define UART_TIMEOUT_COUNT	50000	// Approx 250ms to process the code to wait
#define UART_TIMED_OUT		0x80

enum {
	RX_ENABLE,
	RX_DISABLE,
	TX_ENABLE,
	TX_DISABLE
};

enum {
	CONVERT_DATA_TO_ASCII = 1,
	NO_CONVERSION
};

#define CTRL_B  02

#define EOT_CHAR		0x04
#define ACK_CHAR		0x06
#define XON_CHAR		0x11
#define XOFF_CHAR		0x13
#define NACK_CHAR		0x15
#define CAN_CHAR		0x18

#if 0 /* old hw */
#define CLEAR_DTR	(AVR32_USART1.cr = (1 << AVR32_USART_DTRDIS))
#define SET_DTR		(AVR32_USART1.cr = (1 << AVR32_USART_DTREN))

#define CLEAR_RTS	(AVR32_USART1.cr = (1 << AVR32_USART_RTSDIS))
#define SET_RTS		(AVR32_USART1.cr = (1 << AVR32_USART_RTSEN))

#define READ_DSR 	((AVR32_USART1.csr & (1 << AVR32_USART_CSR_DSR)) ? (uint8)1 : (uint8)0)
#define READ_DCD 	((AVR32_USART1.csr & (1 << AVR32_USART_CSR_DCD)) ? (uint8)1 : (uint8)0)
#define READ_RI 	((AVR32_USART1.csr & (1 << AVR32_USART_CSR_RI)) ? (uint8)1 : (uint8)0)
#define READ_CTS 	((AVR32_USART1.csr & (1 << AVR32_USART_CSR_CTS)) ? (uint8)1 : (uint8)0)
#else
#define CLEAR_DTR	{}
#define SET_DTR		{}

#define CLEAR_RTS	{}
#define SET_RTS		{}

#define READ_DSR 	1
#define READ_DCD 	1
#define READ_RI 	1
#define READ_CTS 	1
#endif

enum {
	DCD_ACTIVE = 0,
	DCD_INACTIVE
};

enum {
	READY_TO_SEND = 0,
	NOT_READY_TO_SEND
};

enum {
	CONNECTION_ESTABLISHED = 0,
	NO_CONNECTION
};

enum {
	RING = 0,
	NO_RING
};

enum {
	MODEM_CONNECTED = 0,
	MODEM_NOT_CONNECTED
};

enum {
	MODEM_SEND_FAILED = 0,
	MODEM_SEND_SUCCESS
};

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------
short Craft(char* fmt, ...);
short DebugPrint(uint8 mode, char* fmt, ...);
void DebugPrintChar(uint8 charData);
char* UartGets(char* s, int32 channel);
uint8 UartGetc(int32 channel, uint8 mode);
uint8 NibbleToA(uint8 hexData);
uint8 ModemPutc(uint8 , uint8);
uint8 ModemPuts(uint8* , uint32 , uint8);
void UartPuts(char* s, int32 channel);
void UartWrite(void* b, int32 n, int32 channel);
void UartPutc(uint8 c, int32 channel);
uint8 UartCharWaiting(int32 channel);

#endif // _UART_H_
