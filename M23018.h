#ifndef M23018_H_
#define M23018_H_

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include "Typedefs.h"
//#include "twi.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define IO_ADDRESS_BASE		0x20 // IO's TWI address
#define IO_ADDRESS_KPD		0x27 // IO's TWI address
#define IO_ADDR_LGT			1 // Address length of the IO chip

#if 1 /* Original (normal) speed */
#define TWI_SPEED			400000 // 400 KHz (works)
#define RX_RDY_TESTED_COUNTER_INCREMENT_COUNT	1396
#define RX_COMP_TESTED_COUNTER_INCREMENT_COUNT	31
#define TX_RDY_TESTED_COUNTER_INCREMENT_COUNT	1001
#define TX_COMP_TESTED_COUNTER_INCREMENT_COUNT	206
#else /* New (fast) speed */
#define TWI_SPEED			3400000 // 3.4 MHz (works with strong enough TWI pull ups)
#define RX_RDY_TESTED_COUNTER_INCREMENT_COUNT	203
#define RX_COMP_TESTED_COUNTER_INCREMENT_COUNT	3
#define TX_RDY_TESTED_COUNTER_INCREMENT_COUNT	134
#define TX_COMP_TESTED_COUNTER_INCREMENT_COUNT	34
#endif

#define TWI_TIMEOUT_DELAY_MULTIPLIER	25

#define IODIRA		0x00
#define IODIRB		0x01
#define IOPOLA		0x02
#define IOPOLB		0x03
#define GPINTENA	0x04
#define GPINTENB	0x05
#define DEFVALA		0x06
#define DEFVALB		0x07
#define INTCONA		0x08
#define INTCONB		0x09
#define IOCONA		0x0A
#define IOCONB		0x0B
#define GPPUA		0x0C
#define GPPUB		0x0D
#define INTFA		0x0E
#define INTFB		0x0F
#define INTCAPA		0x10
#define INTCAPB		0x11
#define GPIOA		0x12
#define GPIOB		0x13
#define OLATA		0x14
#define OLATB		0x15

#define GREEN_LED_PIN	0x10
#define RED_LED_PIN		0x20
#define NO_LED_PINS		0x00

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------
void InitTWI(void);
void InitMcp23018(void);
void ResetTWI(void);
void WriteMcp23018(uint8 chip, uint8 address, uint8 data);
uint8 ReadMcp23018(uint8 chip, uint8 address);
void EnableMcp23018Interrupts(void);
void DisableMcp23018Interrupts(void);

#endif /* M23018_H_ */
