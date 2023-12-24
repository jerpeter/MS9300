///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include "board.h"
//#include "pm.h"
#include "gpio.h"
//#include "sdramc.h"
//#include "intc.h"
//#include "usart.h"
//#include "print_funcs.h"
#include "lcd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Typedefs.h"
#include "Common.h"
#include "Display.h"
#include "Menu.h"
#include "OldUart.h"
#include "spi.h"
#include "ProcessBargraph.h"
#include "SysEvents.h"
#include "Record.h"
#include "InitDataBuffers.h"
#include "PowerManagement.h"
#include "SoftTimer.h"
#include "Keypad.h"
#include "RealTimeClock.h"
#include "RemoteHandler.h"
#include "TextTypes.h"
#include "RemoteCommon.h"
//#include "twi.h"
#include "M23018.h"
//#include "sd_mmc_spi.h"
#include "adc.h"
//#include "usb_task.h"
//#include "device_mass_storage_task.h"
//#include "usb_drv.h"
//#include "flashc.h"
#include "rtc.h"
//#include "tc.h"

#include "tmr.h"
#include "nvic_table.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define ONE_MS_RESOLUTION	1000

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
#include "Globals.h"

///----------------------------------------------------------------------------
///	Local Scope Globals
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupInteralSampleTimer(uint16_t sampleRate)
{
	MXC_SYS_Reset_Periph(MXC_SYS_RESET_TIMER1);
	MXC_SYS_ClockEnable(MXC_SYS_PERIPH_CLOCK_TIMER1);

    // Clear interrupt flag
    MXC_TMR1->intr = MXC_F_TMR_INTR_IRQ;

    // Set the prescaler (TMR_PRES_4096)
	MXC_TMR1->cn |= (MXC_S_TMR_CN_PRES_DIV1);

    // Set the mode
	MXC_TMR1->cn |= TMR_MODE_CONTINUOUS << MXC_F_TMR_CN_TMODE_POS;

	// Set the polarity
    MXC_TMR1->cn |= (0) << MXC_F_TMR_CN_TPOL_POS; // Polarity (0 or 1) doesn't matter

	/*
		32768 = 1,831 compare, add 1 count every 18.2857 cycles (trim)
		16384 = 3,662 compare, add 1 count every 9.1428 cycles (trim)
		8192 = 7,324 compare, add 1 count every 4.5714 cycles (trim)
		4096 = 14,648 compare, add 1 count every 2.2857 cycles (trim)
		2048 = 29,297 compare, subtract 1 count every 8 cycles (trim)
		1024 = 58,594 compare, subtract 1 count every 4 cycles (trim)
	*/

	// Init the compare value
    MXC_TMR1->cmp = (60000000 / sampleRate);

	// Init the counter
    MXC_TMR1->cnt = 0x1;

	// Setup the Timer 0 interrupt
	NVIC_ClearPendingIRQ(TMR1_IRQn);
    NVIC_DisableIRQ(TMR1_IRQn);
    MXC_NVIC_SetVector(TMR1_IRQn, Sample_irq);
    NVIC_EnableIRQ(TMR1_IRQn);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void StartInteralSampleTimer(void)
{
	// Enable the timer
	MXC_TMR1->cn |= MXC_F_TMR_CN_TEN;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void StopInteralSampleTimer(void)
{
	// Disable the timer
	MXC_TMR1->cn &= ~MXC_F_TMR_CN_TEN;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitInterrupts_MS9300(void)
{
	// Any specific interrupt setup?

    __enable_irq();
}

