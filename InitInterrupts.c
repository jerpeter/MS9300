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
#include "srec.h"
//#include "flashc.h"
#include "rtc.h"
//#include "tc.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define ONE_MS_RESOLUTION	1000

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
#include "Globals.h"
#if 0 /* old hw */
extern void rtc_enable_interrupt(volatile avr32_rtc_t *rtc);
#endif

///----------------------------------------------------------------------------
///	Local Scope Globals
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GaugeAlert_ISR(void *cbdata)
{

}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void BatteryCharger_ISR(void *cbdata)
{

}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Expansion_ISR(void *cbdata)
{

}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void USBCI2C_ISR(void *cbdata)
{

}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AccelInt1_ISR(void *cbdata)
{

}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AccelInt2_ISR(void *cbdata)
{

}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void PowerButton_ISR(void *cbdata)
{

}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Button1_ISR(void *cbdata)
{

}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Button2_ISR(void *cbdata)
{

}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Button3_ISR(void *cbdata)
{

}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Button4_ISR(void *cbdata)
{

}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Button5_ISR(void *cbdata)
{

}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Button6_ISR(void *cbdata)
{

}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Button7_ISR(void *cbdata)
{

}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Button8_ISR(void *cbdata)
{

}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Button9_ISR(void *cbdata)
{

}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ExtRTCIntA_ISR(void *cbdata)
{

}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ExternalTriggerIn_ISR(void *cbdata)
{

}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void LCD_ISR(void *cbdata)
{

}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void RTCClock_ISR(void *cbdata)
{

}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Setup_8100_EIC_Low_Battery_ISR(void)
{
#if 0 /* old hw */
	// External Interrupt Controller setup
	AVR32_EIC.IER.int0 = 1;
	AVR32_EIC.MODE.int0 = 0;
	AVR32_EIC.EDGE.int0 = 0;
	AVR32_EIC.LEVEL.int0 = 0;
	AVR32_EIC.FILTER.int0 = 1;
	AVR32_EIC.ASYNC.int0 = 1;
	AVR32_EIC.EN.int0 = 1;

	INTC_register_interrupt(&Eic_low_battery_irq, AVR32_EIC_IRQ_0, 0);

#if 0
	// Test for int enable
	if (AVR32_EIC.IMR.int0 == 0x01) debug("\nLow Battery Interrupt Enabled\r\n");
	else debug("\nLow Battery Interrupt Not Enabled\r\n");
#endif
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Setup_8100_EIC_Keypad_ISR(void)
{
#if 0 /* old hw */
	// External Interrupt Controller setup
	AVR32_EIC.IER.int5 = 1;
	AVR32_EIC.MODE.int5 = 0;
	AVR32_EIC.EDGE.int5 = 0;
	AVR32_EIC.LEVEL.int5 = 0;
	AVR32_EIC.FILTER.int5 = 0;
	AVR32_EIC.ASYNC.int5 = 1;
	AVR32_EIC.EN.int5 = 1;

	INTC_register_interrupt(&Eic_keypad_irq, AVR32_EIC_IRQ_5, 0);

#if 0 /* Some residual wrongly executed code */
	// Enable the interrupt
	rtc_enable_interrupt(&AVR32_RTC);
#endif

#if 0
	// Test for int enable
	if (AVR32_EIC.IMR.int5 == 0x01) debug("\nKeypad Interrupt Enabled\r\n");
	else debug("\nKeypad Interrupt Not Enabled\r\n");
#endif
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Setup_8100_EIC_System_ISR(void)
{
#if 0 /* old hw */
	// External Interrupt Controller setup
	AVR32_EIC.IER.int4 = 1;
	AVR32_EIC.MODE.int4 = 0;
	AVR32_EIC.EDGE.int4 = 0;
	AVR32_EIC.LEVEL.int4 = 0;
	AVR32_EIC.FILTER.int4 = 0;
	AVR32_EIC.ASYNC.int4 = 1;
	AVR32_EIC.EN.int4 = 1;

	INTC_register_interrupt(&Eic_system_irq, AVR32_EIC_IRQ_4, 0);

#if 0 /* Some residual wrongly executed code */
	// Enable the interrupt
	rtc_enable_interrupt(&AVR32_RTC);
#endif

#if 0
	// Test for int enable
	if (AVR32_EIC.IMR.int4 == 0x01) debug("\nSystem Interrupt Enabled\r\n");
	else debug("\nSystem Interrupt Not Enabled\r\n");
#endif
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Setup_8100_EIC_External_RTC_ISR(void)
{
#if 0 /* old hw */
	// External Interrupt Controller setup
	AVR32_EIC.IER.int1 = 1;
	AVR32_EIC.MODE.int1 = 0;
	AVR32_EIC.EDGE.int1 = 0;
	AVR32_EIC.LEVEL.int1 = 0;
	AVR32_EIC.FILTER.int1 = 0;
	AVR32_EIC.ASYNC.int1 = 1;
	AVR32_EIC.EN.int1 = 1;

#if 0 /* Test */
	INTC_register_interrupt(&Eic_external_rtc_irq, AVR32_EIC_IRQ_1, 0);
#else /* Hook in the External RTC interrupt to the actual sample processing interrupt handler */
	INTC_register_interrupt(&Tc_sample_irq, AVR32_EIC_IRQ_1, 0);
#endif

#if 0
	// Test for int enable
	if (AVR32_EIC.IMR.int1 == 0x01) { debug("External RTC Interrupt Enabled\r\n"); }
	else { debug("External RTC Interrupt Not Enabled\r\n"); }
#endif
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Setup_8100_Usart1_RS232_ISR(void)
{
#if 0 /* old hw */

	INTC_register_interrupt(&Usart_1_rs232_irq, AVR32_USART1_IRQ, 1);

	// Enable Receive Ready, Overrun, Parity and Framing error interrupts
	AVR32_USART1.ier = (AVR32_USART_IER_RXRDY_MASK | AVR32_USART_IER_OVRE_MASK | AVR32_USART_IER_PARE_MASK | AVR32_USART_IER_FRAME_MASK);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Setup_8100_Usart0_RS232_ISR(void)
{
#if 0 /* old hw */

	INTC_register_interrupt(&Usart_0_rs232_irq, AVR32_USART0_IRQ, 1);

	// Enable Receive Ready, Overrun, Parity and Framing error interrupts
	AVR32_USART0.ier = (AVR32_USART_IER_RXRDY_MASK | AVR32_USART_IER_OVRE_MASK | AVR32_USART_IER_PARE_MASK | AVR32_USART_IER_FRAME_MASK);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Setup_8100_Soft_Timer_Tick_ISR(void)
{
#if 0 /* old hw */

	INTC_register_interrupt(&Soft_timer_tick_irq, AVR32_RTC_IRQ, 0);
	
	// Enable half second tick
	rtc_enable_interrupt(&AVR32_RTC);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Setup_8100_Gps_Status_ISR(void)
{
#if 0 /* old hw */

	INTC_register_interrupt(&Gps_status_irq, AVR32_GPIO_IRQ_7, 2);

	// Enable the gpio pin interrupt
	gpio_enable_pin_interrupt(AVR32_PIN_PB30, GPIO_RISING_EDGE);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Setup_8100_TC_Clock_ISR(uint32 sampleRate, TC_CHANNEL_NUM channel)
{
#if 0 /* old hw */

	volatile avr32_tc_t *tc = &AVR32_TC;

	// Options for waveform generation.
	tc_waveform_opt_t WAVEFORM_OPT =
	{
		.channel	= channel,								// Channel selection.
		.bswtrg		= TC_EVT_EFFECT_NOOP,					// Software trigger effect on TIOB.
		.beevt		= TC_EVT_EFFECT_NOOP,					// External event effect on TIOB.
		.bcpc		= TC_EVT_EFFECT_NOOP,					// RC compare effect on TIOB.
		.bcpb		= TC_EVT_EFFECT_NOOP,					// RB compare effect on TIOB.
		.aswtrg		= TC_EVT_EFFECT_NOOP,					// Software trigger effect on TIOA.
		.aeevt		= TC_EVT_EFFECT_NOOP,					// External event effect on TIOA.
		.acpc		= TC_EVT_EFFECT_NOOP,					// RC compare effect on TIOA: toggle.
		.acpa		= TC_EVT_EFFECT_NOOP,					// RA compare effect on TIOA: toggle (other possibilities are none, set and clear).
		.wavsel		= TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER,	// Waveform selection: Up mode with automatic trigger(reset) on RC compare.
		.enetrg		= FALSE,								// External event trigger enable.
		.eevt		= 0,									// External event selection.
		.eevtedg	= TC_SEL_NO_EDGE,						// External event edge selection.
		.cpcdis		= FALSE,								// Counter disable when RC compare.
		.cpcstop	= FALSE,								// Counter clock stopped with RC compare.
		.burst		= FALSE,								// Burst signal selection.
		.clki		= FALSE,								// Clock inversion.
		.tcclks		= TC_CLOCK_SOURCE_TC2					// Internal source clock 2 - connected to PBA/2
	};

	// Options for interrupt
	tc_interrupt_t TC_INTERRUPT =
	{
		.etrgs = 0,
		.ldrbs = 0,
		.ldras = 0,
		.cpcs = 1,
		.cpbs = 0,
		.cpas = 0,
		.lovrs = 0,
		.covfs = 0
	};

	switch (channel)
	{
		case TC_SAMPLE_TIMER_CHANNEL:
		INTC_register_interrupt(&Tc_sample_irq, AVR32_TC_IRQ0, 3);
		break;
		
#if INTERNAL_SAMPLING_SOURCE
		case TC_CALIBRATION_TIMER_CHANNEL:
		INTC_register_interrupt(&Tc_sample_irq, AVR32_TC_IRQ1, 3);
		break;
#else /* EXTERNAL_SAMPLING_SOURCE */
		case TC_MILLISECOND_TIMER_CHANNEL:
		INTC_register_interrupt(&Tc_ms_timer_irq, AVR32_TC_IRQ1, 0);
		break;
#endif
		
		case TC_TYPEMATIC_TIMER_CHANNEL:
		INTC_register_interrupt(&Tc_typematic_irq, AVR32_TC_IRQ2, 0);
		break;
	}

	// Initialize the timer/counter.
	tc_init_waveform(tc, &WAVEFORM_OPT); // Initialize the timer/counter waveform.

	// Set the compare triggers.
	// Remember TC counter is 16-bits, so counting second is not possible.
	// We configure it to count ms.
	// We want: (1/(FOSC0/4)) * RC = 1000 Hz => RC = (FOSC0/4) / 1000 = 3000 to get an interrupt every 1ms
	//tc_write_rc(tc, TC_CHANNEL_0, (FOSC0/2)/1000); // Set RC value.
	//tc_write_rc(tc, channel, (FOSC0 / (sampleRate * 2)));
	tc_write_rc(tc, channel, (32900000 / sampleRate));
	
	tc_configure_interrupts(tc, channel, &TC_INTERRUPT);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitInterrupts_NS9100(void)
{
#if 0 /* old hw */
	// Disable all interrupts (Actually done at the start of InitHardware but calling here again just in case some local code enables)
	Disable_global_interrupt();

#if 0 /* Moved to the start of InitHardware to prevent wiping out the TWI interrupt handler */
	// Assign all interrupt vectors an un-handled
	INTC_init_interrupts();
#endif

	// Hook in and enable half second tick
	Setup_8100_Soft_Timer_Tick_ISR();
	
	// Setup typematic timer for repeat key interrupt
	Setup_8100_TC_Clock_ISR(ONE_MS_RESOLUTION, TC_TYPEMATIC_TIMER_CHANNEL);

#if EXTERNAL_SAMPLING_SOURCE
	// Setup millisecond timer
	Setup_8100_TC_Clock_ISR(ONE_MS_RESOLUTION, TC_MILLISECOND_TIMER_CHANNEL);
#endif

#if 0 /* Removed for now because interrupt doesn't provide any benefit over just reading the pin as a GPIO input */
	// Setup Low Battery interrupt from RTC when PFO triggers
	Setup_8100_EIC_Low_Battery_ISR();
#endif

	// Setup keypad for interrupts
	Setup_8100_EIC_Keypad_ISR();
	Setup_8100_EIC_System_ISR();

#if 0 /* Moved interrupt setup to the end of the BootloaderManager to allow for searching for Ctrl-B */
	InitCraftInterruptBuffers();
	Setup_8100_Usart1_RS232_ISR();
#endif

	if (GET_HARDWARE_ID == HARDWARE_ID_REV_8_WITH_GPS_MOD)
	{
		Setup_8100_Gps_Status_ISR();
	}
	
	Enable_global_interrupt();
#endif
}

