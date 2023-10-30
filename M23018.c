///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include "board.h"
//#include "twi.h"
#include "m23018.h"
#include "Typedefs.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define	TWI_DATA_LENGTH		(sizeof(s_twiData)/sizeof(U8))

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
#include "Globals.h"

///----------------------------------------------------------------------------
///	Local Scope Globals
///----------------------------------------------------------------------------
static uint8 s_twiData[10];
#if 0 /* old hw */
static twi_package_t s_twiPacket;
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* old hw */
uint8 TwiMasterReadNoInterrupts(volatile avr32_twi_t *twi, const twi_package_t *package)
{
	uint32 rxRdyTimeoutCount = (RX_RDY_TESTED_COUNTER_INCREMENT_COUNT * TWI_TIMEOUT_DELAY_MULTIPLIER); // Max tested counter time * multiplier
	uint32 rxCompTimeoutCount = (RX_COMP_TESTED_COUNTER_INCREMENT_COUNT * TWI_TIMEOUT_DELAY_MULTIPLIER); // Max tested counter time * multiplier
	uint8 rxCommTimedOut = NO;
	uint8* twiRxData = package->buffer;

	// Enable master transfer, disable slave
	twi->cr = AVR32_TWI_CR_MSEN_MASK | AVR32_TWI_CR_SVDIS_MASK;

	// set read mode, slave address and 3 internal address byte length
	twi->mmr = (package->chip << AVR32_TWI_MMR_DADR_OFFSET) | ((package->addr_length << AVR32_TWI_MMR_IADRSZ_OFFSET) & AVR32_TWI_MMR_IADRSZ_MASK) |
				(1 << AVR32_TWI_MMR_MREAD_OFFSET);

	// set internal address for remote chip
	twi->iadr = package->addr;

	// Send start condition
	twi->cr = (AVR32_TWI_START_MASK | AVR32_TWI_STOP_MASK);

	// Wait for data to be available
	while((twi->sr & AVR32_TWI_SR_RXRDY_MASK) == 0) { if (--rxRdyTimeoutCount == 0) { rxCommTimedOut = YES; break; } /* spin */ }
	*twiRxData = twi->rhr;
	while((twi->sr & AVR32_TWI_SR_TXCOMP_MASK) == 0) { if (--rxCompTimeoutCount == 0) { rxCommTimedOut = YES; break; } /* spin */ }

	// Disable master transfer
	twi->cr = AVR32_TWI_CR_MSDIS_MASK;

	return (rxCommTimedOut);
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* old hw */
uint8 TwiMasterWriteNoInterrupts(volatile avr32_twi_t *twi, const twi_package_t *package)
{
	uint32 txRdyTimeoutCount = (TX_RDY_TESTED_COUNTER_INCREMENT_COUNT * TWI_TIMEOUT_DELAY_MULTIPLIER); // Max tested counter time * multiplier
	uint32 txCompTimeoutCount = (TX_COMP_TESTED_COUNTER_INCREMENT_COUNT * TWI_TIMEOUT_DELAY_MULTIPLIER); // Max tested counter time * multiplier
	uint8 txCommTimedOut = NO;
	uint8* twiTxData = package->buffer;

	// Enable master transfer, disable slave
	twi->cr = AVR32_TWI_CR_MSEN_MASK | AVR32_TWI_CR_SVDIS_MASK;

	// set write mode, slave address and 3 internal address byte length
	twi->mmr = (0 << AVR32_TWI_MMR_MREAD_OFFSET) | (package->chip << AVR32_TWI_MMR_DADR_OFFSET) | ((package->addr_length << AVR32_TWI_MMR_IADRSZ_OFFSET) & AVR32_TWI_MMR_IADRSZ_MASK);

	// set internal address for remote chip
	twi->iadr = package->addr;

	// put the first byte in the Transmit Holding Register
	twi->thr = *twiTxData;

	while((twi->sr & AVR32_TWI_SR_TXRDY_MASK) == 0) { if (--txRdyTimeoutCount == 0) { txCommTimedOut = YES; break; } /* spin */ }
	while((twi->sr & AVR32_TWI_SR_TXCOMP_MASK) == 0) { if (--txCompTimeoutCount == 0) { txCommTimedOut = YES; break; } /* spin */ }

	// Disable master transfer
	twi->cr = AVR32_TWI_CR_MSDIS_MASK;

	return (txCommTimedOut);
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void WriteMcp23018(unsigned char chip, unsigned char address, unsigned char data)
{
#if 0 /* old hw */
	unsigned char globalInterruptsEnabled = Is_global_interrupt_enabled();
	uint8 errorStatus;

	s_twiData[0] = data;
	s_twiPacket.chip = chip;
	s_twiPacket.addr = address;
	s_twiPacket.addr_length = IO_ADDR_LGT;
	s_twiPacket.buffer = (void*) s_twiData;
	s_twiPacket.length = 1;

	if (globalInterruptsEnabled) { Disable_global_interrupt(); }

	errorStatus = TwiMasterWriteNoInterrupts(&AVR32_TWI, &s_twiPacket);

	if (globalInterruptsEnabled) { Enable_global_interrupt(); }

	// If communication failed try to reset the TWI
	if (errorStatus)
	{
		ResetTWI();
	}
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
unsigned char ReadMcp23018(unsigned char chip, unsigned char address)
{
#if 0 /* old hw */
	unsigned char globalInterruptsEnabled = Is_global_interrupt_enabled();
	uint8 errorStatus;

	s_twiData[0] = 0;
	s_twiPacket.chip = chip;
	s_twiPacket.addr = address;
	s_twiPacket.addr_length = IO_ADDR_LGT;
	s_twiPacket.buffer = (void*) s_twiData;
	s_twiPacket.length = 1;

	if (globalInterruptsEnabled) { Disable_global_interrupt(); }

	errorStatus = TwiMasterReadNoInterrupts(&AVR32_TWI, &s_twiPacket);

	if (globalInterruptsEnabled) { Enable_global_interrupt(); }

	// If communication failed try to reset the TWI
	if (errorStatus)
	{
		ResetTWI();
	}
#endif

	return(s_twiData[0]);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ResetTWI(void)
{
#if 0 /* old hw */
	unsigned int cldiv;
	unsigned int ckdiv = 0;
	volatile avr32_twi_t *twi = &AVR32_TWI;
	int status;

	// Reset TWI
	twi->cr = AVR32_TWI_CR_SWRST_MASK;

	// Dummy reads of status register
	status = twi->sr;
	status = twi->sr;
	status = twi->sr;
	status = twi->sr;
	status = twi->sr;

	// Get clock dividers
	cldiv = (FOSC0 / TWI_SPEED) - 4;

	// cldiv must fit in 8 bits, ckdiv must fit in 3 bits
	while ((cldiv > 0xFF) && (ckdiv < 0x7))
	{
		// increase clock divider
		ckdiv++;
		// divide cldiv value
		cldiv /= 2;
	}

	// Set clock waveform generator register
	twi->cwgr = (cldiv | (cldiv << AVR32_TWI_CWGR_CHDIV_OFFSET) | (ckdiv << AVR32_TWI_CWGR_CKDIV_OFFSET));
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitTWI(void)
{
#if 0 /* old hw */
	// TWI options.
	twi_package_t package;
	unsigned int cldiv;
	unsigned int ckdiv = 0;
	char data[1] = {0};
	int status;
	volatile avr32_twi_t *twi = &AVR32_TWI;

	status = twi->rhr;
	twi->idr = ~0UL;
	status = twi->sr;

	// Reset TWI
	twi->cr = AVR32_TWI_CR_SWRST_MASK;

	// Dummy reads of status register
	status = twi->sr;
	status = twi->sr;
	status = twi->sr;
	status = twi->sr;
	status = twi->sr;

	// Get clock dividers
	cldiv = (FOSC0 / TWI_SPEED) - 4;

	// cldiv must fit in 8 bits, ckdiv must fit in 3 bits
	while ((cldiv > 0xFF) && (ckdiv < 0x7))
	{
		// increase clock divider
		ckdiv++;
		// divide cldiv value
		cldiv /= 2;
	}

	// Set clock waveform generator register
	twi->cwgr = (cldiv | (cldiv << AVR32_TWI_CWGR_CHDIV_OFFSET) | (ckdiv << AVR32_TWI_CWGR_CKDIV_OFFSET));

	// Setup dummy package to probe the component
	package.buffer = data;
	package.chip = IO_ADDRESS_KPD;
	package.length = 1;
	package.addr_length = 0;
	package.addr = 0;

	// Probe
	TwiMasterWriteNoInterrupts(&AVR32_TWI, &package);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitMcp23018(void)
{
	// I/O Config
	WriteMcp23018(IO_ADDRESS_KPD, IOCONA, 0x20);
	WriteMcp23018(IO_ADDRESS_KPD, IOCONB, 0x20);

	// Port Value
	WriteMcp23018(IO_ADDRESS_KPD, GPIOA, 0x00);
	WriteMcp23018(IO_ADDRESS_KPD, GPIOB, 0x00);

	// Port Direction
	WriteMcp23018(IO_ADDRESS_KPD, IODIRA, 0x0F);
	WriteMcp23018(IO_ADDRESS_KPD, IODIRB, 0xFF);

	// Pullup (Open drain outputs only, without pullups you can't drive)
	WriteMcp23018(IO_ADDRESS_KPD, GPPUA, 0xFF);
	WriteMcp23018(IO_ADDRESS_KPD, GPPUB, 0xFF);

	// Polarity
	WriteMcp23018(IO_ADDRESS_KPD, IOPOLA, 0x0E);
	WriteMcp23018(IO_ADDRESS_KPD, IOPOLB, 0xFF);

	// Default Value
	WriteMcp23018(IO_ADDRESS_KPD, DEFVALA, 0x00);
	WriteMcp23018(IO_ADDRESS_KPD, DEFVALB, 0x00);

	// Interrupt on Change Compare
	WriteMcp23018(IO_ADDRESS_KPD, INTCONA, 0x00);
	WriteMcp23018(IO_ADDRESS_KPD, INTCONB, 0x00);

	// Disable Interrupt Enable on Change for now (enabled at the end of Software Init now)
	WriteMcp23018(IO_ADDRESS_KPD, GPINTENA, 0x00);
	WriteMcp23018(IO_ADDRESS_KPD, GPINTENB, 0x00);

	// Clear any interrupt generation
	ReadMcp23018(IO_ADDRESS_KPD, INTFA);
	ReadMcp23018(IO_ADDRESS_KPD, GPIOA);

	ReadMcp23018(IO_ADDRESS_KPD, INTFB);
	ReadMcp23018(IO_ADDRESS_KPD, GPIOB);

#if 0 /* old hw */
	// clear the interrupt flag in the processor
	AVR32_EIC.ICR.int4 = 1;
	AVR32_EIC.ICR.int5 = 1;
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void EnableMcp23018Interrupts(void)
{
	// Disable Interrupt Enable on Change
	WriteMcp23018(IO_ADDRESS_KPD, GPINTENA, 0x0F);
	WriteMcp23018(IO_ADDRESS_KPD, GPINTENB, 0xFF);

	// Clear any interrupt generation
	ReadMcp23018(IO_ADDRESS_KPD, INTFA);
	ReadMcp23018(IO_ADDRESS_KPD, GPIOA);

	ReadMcp23018(IO_ADDRESS_KPD, INTFB);
	ReadMcp23018(IO_ADDRESS_KPD, GPIOB);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DisableMcp23018Interrupts(void)
{
	// Disable Interrupt Enable on Change
	WriteMcp23018(IO_ADDRESS_KPD, GPINTENA, 0x00);
	WriteMcp23018(IO_ADDRESS_KPD, GPINTENB, 0x00);

	// Clear any interrupt generation
	ReadMcp23018(IO_ADDRESS_KPD, INTFA);
	ReadMcp23018(IO_ADDRESS_KPD, GPIOA);

	ReadMcp23018(IO_ADDRESS_KPD, INTFB);
	ReadMcp23018(IO_ADDRESS_KPD, GPIOB);
}
