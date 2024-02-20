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
    INTERNAL_SAMPLING_TIMER_NUM->intr = MXC_F_TMR_INTR_IRQ;

    // Set the prescaler (TMR_PRES_4096)
	INTERNAL_SAMPLING_TIMER_NUM->cn |= (MXC_S_TMR_CN_PRES_DIV1);

    // Set the mode
	INTERNAL_SAMPLING_TIMER_NUM->cn |= TMR_MODE_CONTINUOUS << MXC_F_TMR_CN_TMODE_POS;

	// Set the polarity
    INTERNAL_SAMPLING_TIMER_NUM->cn |= (0) << MXC_F_TMR_CN_TPOL_POS; // Polarity (0 or 1) doesn't matter

	/*
		32768 = 1,831 compare, add 1 count every 18.2857 cycles (trim)
		16384 = 3,662 compare, add 1 count every 9.1428 cycles (trim)
		8192 = 7,324 compare, add 1 count every 4.5714 cycles (trim)
		4096 = 14,648 compare, add 1 count every 2.2857 cycles (trim)
		2048 = 29,297 compare, subtract 1 count every 8 cycles (trim)
		1024 = 58,594 compare, subtract 1 count every 4 cycles (trim)
	*/

	// Init the compare value
    INTERNAL_SAMPLING_TIMER_NUM->cmp = (60000000 / sampleRate);

	// Init the counter
    INTERNAL_SAMPLING_TIMER_NUM->cnt = 0x1;

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
	INTERNAL_SAMPLING_TIMER_NUM->cn |= MXC_F_TMR_CN_TEN;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void StopInteralSampleTimer(void)
{
	// Disable the timer
	INTERNAL_SAMPLING_TIMER_NUM->cn &= ~MXC_F_TMR_CN_TEN;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GPIO0_IRQHandler(void)
{
    //debug("GPIO Int Handler Port 0 processing...\n");
    MXC_GPIO_Handler(MXC_GPIO_GET_IDX(MXC_GPIO0));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GPIO1_IRQHandler(void)
{
    //debug("GPIO Int Handler Port 1 processing...\n");
    MXC_GPIO_Handler(MXC_GPIO_GET_IDX(MXC_GPIO1));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GPIO2_IRQHandler(void)
{
    //debug("GPIO Int Handler Port 2 processing...\n");
    MXC_GPIO_Handler(MXC_GPIO_GET_IDX(MXC_GPIO2));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GPIO3_IRQHandler(void)
{
    //debug("GPIO Int Handler Port 3 processing...\n");
    MXC_GPIO_Handler(MXC_GPIO_GET_IDX(MXC_GPIO3));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitInterrupts_MS9300(void)
{
	// Any specific interrupt setup?

#if 1 /* Moved from Gpio Init in Init Hardware section */
	//----------------------------------------------------------------------------------------------------------------------
	// Enable IRQ's for any of the appropritate GPIO input interrupts
	//----------------------------------------------------------------------------------------------------------------------
#if 0 /* Temp remove until init further along and exception fixed */
	NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(MXC_GPIO0)));
	NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(MXC_GPIO1)));
	NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(MXC_GPIO2)));
	NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(MXC_GPIO3)));
#endif
#endif

    __enable_irq();
}

