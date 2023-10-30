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
#include "Uart.h"
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
#include "Sensor.h"
#include "NomisLogo.h"
//#include "navigation.h"
#include "Analog.h"
#include "cs8900.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#if 0 /* old hw */

#define AVR32_IDLE_MODE		AVR32_PM_SMODE_IDLE
#else
#define AVR32_IDLE_MODE		0
#endif
#define PBA_HZ				FOSC0
#define ONE_MS_RESOLUTION	1000

#define AVR32_WDT_KEY_VALUE_ASSERT		0x55000000
#define AVR32_WDT_KEY_VALUE_DEASSERT	0xAA000000
#define AVR32_WDT_DISABLE_VALUE			0x00000000

#define EEPROM_SPI_CS_NUM			0
#define RTC_SPI_CS_NUM				1
#define SDMMC_SPI_CS_NUM			2
#define AD_CTL_SPI_CS_NUM			3
#define EEPROM_SPI_MAX_SPEED		500000 //2100000 // Speed should be safe up to 2.1 MHz, needs to be tested
#define RTC_SPI_MAX_SPEED			1000000 // 1000000 // Speed should be safe up to 3.5 MHz, needs to be tested
#define SDMMC_SPI_MAX_SPEED			12000000 // Speed should be safe up to 12 MHz (150 KHz * 80x)
#define AD_CTL_SPI_MAX_SPEED		4000000 // Speed should be safe up to 10 MHz, needs to be tested

// Chip select defines
#define NRD_SETUP	30 //10
#define NRD_PULSE	30 //135
#define NRD_CYCLE	75 //180

#define NCS_RD_SETUP	0 //10
#define NCS_RD_PULSE	55 //250

#define NWE_SETUP	0 //20
#define NWE_PULSE	40 //110
#define NWE_CYCLE	55 //150

#define NCS_WR_SETUP	0 //20
#define NCS_WR_PULSE	45 //230

#define EXT_SM_SIZE				16
#define NCS_CONTROLLED_READ		FALSE
#define NCS_CONTROLLED_WRITE	FALSE
#define NWAIT_MODE				0
#define PAGE_MODE				0
#define PAGE_SIZE				0
#define SMC_8_BIT_CHIPS			FALSE
#define SMC_DBW					16
#define TDF_CYCLES				0
#define TDF_OPTIM				0

// Configure the SM Controller with SM setup and timing information for all chip select
#if 0 /* old hw */

#define SMC_CS_SETUP(ncs) { \
	U32 nwe_setup = ((NWE_SETUP * hsb_mhz_up + 999) / 1000); \
	U32 ncs_wr_setup = ((NCS_WR_SETUP * hsb_mhz_up + 999) / 1000); \
	U32 nrd_setup = ((NRD_SETUP * hsb_mhz_up + 999) / 1000); \
	U32 ncs_rd_setup = ((NCS_RD_SETUP * hsb_mhz_up + 999) / 1000); \
	U32 nwe_pulse = ((NWE_PULSE * hsb_mhz_up + 999) / 1000); \
	U32 ncs_wr_pulse = ((NCS_WR_PULSE * hsb_mhz_up + 999) / 1000); \
	U32 nrd_pulse = ((NRD_PULSE * hsb_mhz_up + 999) / 1000); \
	U32 ncs_rd_pulse = ((NCS_RD_PULSE * hsb_mhz_up + 999) / 1000); \
	U32 nwe_cycle = ((NWE_CYCLE * hsb_mhz_up + 999) / 1000); \
	U32 nrd_cycle = ((NRD_CYCLE * hsb_mhz_up + 999) / 1000); \
	\
	/* Some coherence checks... */ \
	/* Ensures CS is active during Rd or Wr */ \
	if (ncs_rd_setup + ncs_rd_pulse < nrd_setup + nrd_pulse) \
	ncs_rd_pulse = nrd_setup + nrd_pulse - ncs_rd_setup; \
	if (ncs_wr_setup + ncs_wr_pulse < nwe_setup + nwe_pulse) \
	ncs_wr_pulse = nwe_setup + nwe_pulse - ncs_wr_setup; \
	\
	/* ncs_hold = n_cycle - ncs_setup - ncs_pulse */ \
	/* n_hold = n_cycle - n_setup - n_pulse */ \
	/* */ \
	/* All holds parameters must be positive or null, so: */ \
	/* nwe_cycle shall be >= ncs_wr_setup + ncs_wr_pulse */ \
	if (nwe_cycle < ncs_wr_setup + ncs_wr_pulse) \
	nwe_cycle = ncs_wr_setup + ncs_wr_pulse; \
	\
	/* nwe_cycle shall be >= nwe_setup + nwe_pulse */ \
	if (nwe_cycle < nwe_setup + nwe_pulse) \
	nwe_cycle = nwe_setup + nwe_pulse; \
	\
	/* nrd_cycle shall be >= ncs_rd_setup + ncs_rd_pulse */ \
	if (nrd_cycle < ncs_rd_setup + ncs_rd_pulse) \
	nrd_cycle = ncs_rd_setup + ncs_rd_pulse; \
	\
	/* nrd_cycle shall be >= nrd_setup + nrd_pulse */ \
	if (nrd_cycle < nrd_setup + nrd_pulse) \
	nrd_cycle = nrd_setup + nrd_pulse; \
	\
	AVR32_SMC.cs[ncs].setup = (nwe_setup << AVR32_SMC_SETUP0_NWE_SETUP_OFFSET) | \
	(ncs_wr_setup << AVR32_SMC_SETUP0_NCS_WR_SETUP_OFFSET) | \
	(nrd_setup << AVR32_SMC_SETUP0_NRD_SETUP_OFFSET) | \
	(ncs_rd_setup << AVR32_SMC_SETUP0_NCS_RD_SETUP_OFFSET); \
	AVR32_SMC.cs[ncs].pulse = (nwe_pulse << AVR32_SMC_PULSE0_NWE_PULSE_OFFSET) | \
	(ncs_wr_pulse << AVR32_SMC_PULSE0_NCS_WR_PULSE_OFFSET) | \
	(nrd_pulse << AVR32_SMC_PULSE0_NRD_PULSE_OFFSET) | \
	(ncs_rd_pulse << AVR32_SMC_PULSE0_NCS_RD_PULSE_OFFSET); \
	AVR32_SMC.cs[ncs].cycle = (nwe_cycle << AVR32_SMC_CYCLE0_NWE_CYCLE_OFFSET) | \
	(nrd_cycle << AVR32_SMC_CYCLE0_NRD_CYCLE_OFFSET); \
	AVR32_SMC.cs[ncs].mode = (((NCS_CONTROLLED_READ) ? AVR32_SMC_MODE0_READ_MODE_NCS_CONTROLLED : \
	AVR32_SMC_MODE0_READ_MODE_NRD_CONTROLLED) << AVR32_SMC_MODE0_READ_MODE_OFFSET) | \
	(((NCS_CONTROLLED_WRITE) ? AVR32_SMC_MODE0_WRITE_MODE_NCS_CONTROLLED : \
	AVR32_SMC_MODE0_WRITE_MODE_NWE_CONTROLLED) << AVR32_SMC_MODE0_WRITE_MODE_OFFSET) | \
	(NWAIT_MODE << AVR32_SMC_MODE0_EXNW_MODE_OFFSET) | \
	(((SMC_8_BIT_CHIPS) ? AVR32_SMC_MODE0_BAT_BYTE_WRITE : \
	AVR32_SMC_MODE0_BAT_BYTE_SELECT) << AVR32_SMC_MODE0_BAT_OFFSET) | \
	(((SMC_DBW <= 8) ? AVR32_SMC_MODE0_DBW_8_BITS : \
	(SMC_DBW <= 16) ? AVR32_SMC_MODE0_DBW_16_BITS : \
	AVR32_SMC_MODE0_DBW_32_BITS) << AVR32_SMC_MODE0_DBW_OFFSET) | \
	(TDF_CYCLES << AVR32_SMC_MODE0_TDF_CYCLES_OFFSET) | \
	(TDF_OPTIM << AVR32_SMC_MODE0_TDF_MODE_OFFSET) | \
	(PAGE_MODE << AVR32_SMC_MODE0_PMEN_OFFSET) | \
	(PAGE_SIZE << AVR32_SMC_MODE0_PS_OFFSET); \
	g_smc_tab_cs_size[ncs] = EXT_SM_SIZE; \
}
#else
#define SMC_CS_SETUP(ncs) {}
#endif

#define NRD_SETUP_SPECIAL	10 //10
#define NRD_PULSE_SPECIAL	250 //135 //135
#define NRD_CYCLE_SPECIAL	180 //180

#define NCS_RD_SETUP_SPECIAL	35 //10
#define NCS_RD_PULSE_SPECIAL	250 //150 //250

#define NWE_SETUP_SPECIAL	20 //20
#define NWE_PULSE_SPECIAL	110 //110
#define NWE_CYCLE_SPECIAL	165 //150

#define NCS_WR_SETUP_SPECIAL	35 //20
#define NCS_WR_PULSE_SPECIAL	230 //150 //230

#if 0 /* old hw */

#define SMC_CS_SETUP_SPECIAL(ncs) { \
	U32 nwe_setup = ((NWE_SETUP_SPECIAL * hsb_mhz_up + 999) / 1000); \
	U32 ncs_wr_setup = ((NCS_WR_SETUP_SPECIAL * hsb_mhz_up + 999) / 1000); \
	U32 nrd_setup = ((NRD_SETUP_SPECIAL * hsb_mhz_up + 999) / 1000); \
	U32 ncs_rd_setup = ((NCS_RD_SETUP_SPECIAL * hsb_mhz_up + 999) / 1000); \
	U32 nwe_pulse = ((NWE_PULSE_SPECIAL * hsb_mhz_up + 999) / 1000); \
	U32 ncs_wr_pulse = ((NCS_WR_PULSE_SPECIAL * hsb_mhz_up + 999) / 1000); \
	U32 nrd_pulse = ((NRD_PULSE_SPECIAL * hsb_mhz_up + 999) / 1000); \
	U32 ncs_rd_pulse = ((NCS_RD_PULSE_SPECIAL * hsb_mhz_up + 999) / 1000); \
	U32 nwe_cycle = ((NWE_CYCLE_SPECIAL * hsb_mhz_up + 999) / 1000); \
	U32 nrd_cycle = ((NRD_CYCLE_SPECIAL * hsb_mhz_up + 999) / 1000); \
	\
	/* Some coherence checks... */ \
	/* Ensures CS is active during Rd or Wr */ \
	if (ncs_rd_setup + ncs_rd_pulse < nrd_setup + nrd_pulse) \
	ncs_rd_pulse = nrd_setup + nrd_pulse - ncs_rd_setup; \
	if (ncs_wr_setup + ncs_wr_pulse < nwe_setup + nwe_pulse) \
	ncs_wr_pulse = nwe_setup + nwe_pulse - ncs_wr_setup; \
	\
	/* ncs_hold = n_cycle - ncs_setup - ncs_pulse */ \
	/* n_hold = n_cycle - n_setup - n_pulse */ \
	/* */ \
	/* All holds parameters must be positive or null, so: */ \
	/* nwe_cycle shall be >= ncs_wr_setup + ncs_wr_pulse */ \
	if (nwe_cycle < ncs_wr_setup + ncs_wr_pulse) \
	nwe_cycle = ncs_wr_setup + ncs_wr_pulse; \
	\
	/* nwe_cycle shall be >= nwe_setup + nwe_pulse */ \
	if (nwe_cycle < nwe_setup + nwe_pulse) \
	nwe_cycle = nwe_setup + nwe_pulse; \
	\
	/* nrd_cycle shall be >= ncs_rd_setup + ncs_rd_pulse */ \
	if (nrd_cycle < ncs_rd_setup + ncs_rd_pulse) \
	nrd_cycle = ncs_rd_setup + ncs_rd_pulse; \
	\
	/* nrd_cycle shall be >= nrd_setup + nrd_pulse */ \
	if (nrd_cycle < nrd_setup + nrd_pulse) \
	nrd_cycle = nrd_setup + nrd_pulse; \
	\
	AVR32_SMC.cs[ncs].setup = (nwe_setup << AVR32_SMC_SETUP0_NWE_SETUP_OFFSET) | \
	(ncs_wr_setup << AVR32_SMC_SETUP0_NCS_WR_SETUP_OFFSET) | \
	(nrd_setup << AVR32_SMC_SETUP0_NRD_SETUP_OFFSET) | \
	(ncs_rd_setup << AVR32_SMC_SETUP0_NCS_RD_SETUP_OFFSET); \
	AVR32_SMC.cs[ncs].pulse = (nwe_pulse << AVR32_SMC_PULSE0_NWE_PULSE_OFFSET) | \
	(ncs_wr_pulse << AVR32_SMC_PULSE0_NCS_WR_PULSE_OFFSET) | \
	(nrd_pulse << AVR32_SMC_PULSE0_NRD_PULSE_OFFSET) | \
	(ncs_rd_pulse << AVR32_SMC_PULSE0_NCS_RD_PULSE_OFFSET); \
	AVR32_SMC.cs[ncs].cycle = (nwe_cycle << AVR32_SMC_CYCLE0_NWE_CYCLE_OFFSET) | \
	(nrd_cycle << AVR32_SMC_CYCLE0_NRD_CYCLE_OFFSET); \
	AVR32_SMC.cs[ncs].mode = (((NCS_CONTROLLED_READ) ? AVR32_SMC_MODE0_READ_MODE_NCS_CONTROLLED : \
	AVR32_SMC_MODE0_READ_MODE_NRD_CONTROLLED) << AVR32_SMC_MODE0_READ_MODE_OFFSET) | \
	(((NCS_CONTROLLED_WRITE) ? AVR32_SMC_MODE0_WRITE_MODE_NCS_CONTROLLED : \
	AVR32_SMC_MODE0_WRITE_MODE_NWE_CONTROLLED) << AVR32_SMC_MODE0_WRITE_MODE_OFFSET) | \
	(NWAIT_MODE << AVR32_SMC_MODE0_EXNW_MODE_OFFSET) | \
	(((SMC_8_BIT_CHIPS) ? AVR32_SMC_MODE0_BAT_BYTE_WRITE : \
	AVR32_SMC_MODE0_BAT_BYTE_SELECT) << AVR32_SMC_MODE0_BAT_OFFSET) | \
	(((SMC_DBW <= 8) ? AVR32_SMC_MODE0_DBW_8_BITS : \
	(SMC_DBW <= 16) ? AVR32_SMC_MODE0_DBW_16_BITS : \
	AVR32_SMC_MODE0_DBW_32_BITS) << AVR32_SMC_MODE0_DBW_OFFSET) | \
	(TDF_CYCLES << AVR32_SMC_MODE0_TDF_CYCLES_OFFSET) | \
	(TDF_OPTIM << AVR32_SMC_MODE0_TDF_MODE_OFFSET) | \
	(PAGE_MODE << AVR32_SMC_MODE0_PMEN_OFFSET) | \
	(PAGE_SIZE << AVR32_SMC_MODE0_PS_OFFSET); \
	g_smc_tab_cs_size[ncs] = EXT_SM_SIZE; \
}
#else
#define SMC_CS_SETUP_SPECIAL(ncs) {}
#endif

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
#include "Globals.h"
#if 0 /* old hw */
extern int rtc_init(volatile avr32_rtc_t *rtc, unsigned char osc_type, unsigned char psel);
extern void rtc_set_top_value(volatile avr32_rtc_t *rtc, unsigned long top);
extern void rtc_enable(volatile avr32_rtc_t *rtc);
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
void SPI_0_Init(void)
{
#if 0 /* old hw */
	const gpio_map_t spi0Map =
	{
		{AVR32_SPI0_SCK_0_0_PIN, AVR32_SPI0_SCK_0_0_FUNCTION }, // SPI Clock.
		{AVR32_SPI0_MISO_0_0_PIN, AVR32_SPI0_MISO_0_0_FUNCTION}, // MISO.
		{AVR32_SPI0_MOSI_0_0_PIN, AVR32_SPI0_MOSI_0_0_FUNCTION}, // MOSI.
		{AVR32_SPI0_NPCS_0_0_PIN, AVR32_SPI0_NPCS_0_0_FUNCTION} // Chip Select NPCS.
	};
	
	// SPI options.
	spi_options_t spiOptions =
	{
		.reg = 0,
		.baudrate = 33000000, //36000000, //33000000, // 33 MHz
		.bits = 16,
		.spck_delay	= 0,
		.trans_delay = 0,
		.stay_act = 1,
		.spi_mode = 0,
		.modfdis = 1
	};

	// Assign I/Os to SPI.
	gpio_enable_module(spi0Map, sizeof(spi0Map) / sizeof(spi0Map[0]));

	// Initialize as master.
	spi_initMaster(&AVR32_SPI0, &spiOptions);

	// Set SPI selection mode: variable_ps, pcs_decode, delay.
	spi_selectionMode(&AVR32_SPI0, 0, 0, 0);

	// Enable SPI module.
	spi_enable(&AVR32_SPI0);

	// Initialize AD driver with SPI clock (PBA).
	// Setup SPI registers according to spiOptions.
	spi_setupChipReg(&AVR32_SPI0, &spiOptions, FOSC0);

	// Make sure SPI0 input isn't floating
	gpio_enable_pin_pull_up(AVR32_SPI0_MISO_0_0_PIN);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SPI_1_Init(void)
{
#if 0 /* old hw */	
	// SPI 1 MAP Pin select
	const gpio_map_t spi1Map =
	{
		{AVR32_SPI1_SCK_0_0_PIN, AVR32_SPI1_SCK_0_0_FUNCTION },		// SPI Clock.
		{AVR32_SPI1_MISO_0_0_PIN, AVR32_SPI1_MISO_0_0_FUNCTION},	// MISO.
		{AVR32_SPI1_MOSI_0_0_PIN, AVR32_SPI1_MOSI_0_0_FUNCTION},	// MOSI.
		{AVR32_SPI1_NPCS_3_PIN,	AVR32_SPI1_NPCS_3_FUNCTION},		// AD Control Chip Select NPCS.
		{AVR32_SPI1_NPCS_0_0_PIN, AVR32_SPI1_NPCS_0_0_FUNCTION},	// EEprom Chip Select NPCS.
		{AVR32_SPI1_NPCS_1_0_PIN, AVR32_SPI1_NPCS_1_0_FUNCTION},	// RTC Chip Select NPCS.
		{AVR32_SPI1_NPCS_2_0_PIN, AVR32_SPI1_NPCS_2_0_FUNCTION},	// SDMMC Chip Select NPCS.
	};

	// Generic SPI options
	spi_options_t spiOptions =
	{
		.bits = 8,
		.spck_delay = 0,
		.trans_delay = 0,
		.stay_act = 1,
		.spi_mode = 0,
		.modfdis = 1
	};

	// Assign I/Os to SPI.
	gpio_enable_module(spi1Map, sizeof(spi1Map) / sizeof(spi1Map[0]));

	// Initialize as master.
	spi_initMaster(&AVR32_SPI1, &spiOptions);

	// Set SPI selection mode: variable_ps, pcs_decode, delay.
	spi_selectionMode(&AVR32_SPI1, 0, 0, 0);

	// Enable SPI module.
	spi_enable(&AVR32_SPI1);

	spiOptions.reg = AD_CTL_SPI_CS_NUM; // 3
	spiOptions.baudrate = AD_CTL_SPI_MAX_SPEED; // 4 MHz
	spi_setupChipReg(&AVR32_SPI1, &spiOptions, FOSC0);

	spiOptions.reg = EEPROM_SPI_CS_NUM; // 0
	spiOptions.baudrate = EEPROM_SPI_MAX_SPEED; // 2.1 MHz
	spi_setupChipReg(&AVR32_SPI1, &spiOptions, FOSC0);

	spiOptions.reg = RTC_SPI_CS_NUM; // 1
	spiOptions.baudrate = RTC_SPI_MAX_SPEED; // 1 MHz
	spi_setupChipReg(&AVR32_SPI1, &spiOptions, FOSC0);

	spiOptions.reg = SDMMC_SPI_CS_NUM; // 2
	spiOptions.baudrate = SDMMC_SPI_MAX_SPEED; // 12 MHz
	spi_setupChipReg(&AVR32_SPI1, &spiOptions, FOSC0);

	// Make sure SPI1 input isn't floating
	gpio_enable_pin_pull_up(AVR32_SPI1_MISO_0_0_PIN);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Avr32_enable_muxed_pins(void)
{
#if 0 /* old hw */
	static const gpio_map_t SMC_EBI_GPIO_MAP =
	{
		//=====================================================
		// EBI - Data
		{AVR32_EBI_DATA_0_PIN, AVR32_EBI_DATA_0_FUNCTION},
		{AVR32_EBI_DATA_1_PIN, AVR32_EBI_DATA_1_FUNCTION},
		{AVR32_EBI_DATA_2_PIN, AVR32_EBI_DATA_2_FUNCTION},
		{AVR32_EBI_DATA_3_PIN, AVR32_EBI_DATA_3_FUNCTION},
		{AVR32_EBI_DATA_4_PIN, AVR32_EBI_DATA_4_FUNCTION},
		{AVR32_EBI_DATA_5_PIN, AVR32_EBI_DATA_5_FUNCTION},
		{AVR32_EBI_DATA_6_PIN, AVR32_EBI_DATA_6_FUNCTION},
		{AVR32_EBI_DATA_7_PIN, AVR32_EBI_DATA_7_FUNCTION},
		{AVR32_EBI_DATA_8_PIN, AVR32_EBI_DATA_8_FUNCTION},
		{AVR32_EBI_DATA_9_PIN ,AVR32_EBI_DATA_9_FUNCTION},
		{AVR32_EBI_DATA_10_PIN, AVR32_EBI_DATA_10_FUNCTION},
		{AVR32_EBI_DATA_11_PIN, AVR32_EBI_DATA_11_FUNCTION},
		{AVR32_EBI_DATA_12_PIN, AVR32_EBI_DATA_12_FUNCTION},
		{AVR32_EBI_DATA_13_PIN, AVR32_EBI_DATA_13_FUNCTION},
		{AVR32_EBI_DATA_14_PIN, AVR32_EBI_DATA_14_FUNCTION},
		{AVR32_EBI_DATA_15_PIN, AVR32_EBI_DATA_15_FUNCTION},

		//=====================================================
		// EBI - Address
		{AVR32_EBI_ADDR_0_PIN, AVR32_EBI_ADDR_0_FUNCTION},
		{AVR32_EBI_ADDR_1_PIN, AVR32_EBI_ADDR_1_FUNCTION},
		{AVR32_EBI_ADDR_2_PIN, AVR32_EBI_ADDR_2_FUNCTION},
		{AVR32_EBI_ADDR_3_PIN, AVR32_EBI_ADDR_3_FUNCTION},
		{AVR32_EBI_ADDR_4_PIN, AVR32_EBI_ADDR_4_FUNCTION},
		{AVR32_EBI_ADDR_5_PIN, AVR32_EBI_ADDR_5_FUNCTION},
		{AVR32_EBI_ADDR_6_PIN, AVR32_EBI_ADDR_6_FUNCTION},
		{AVR32_EBI_ADDR_7_PIN, AVR32_EBI_ADDR_7_FUNCTION},
		{AVR32_EBI_ADDR_8_PIN, AVR32_EBI_ADDR_8_FUNCTION},
		{AVR32_EBI_ADDR_9_PIN, AVR32_EBI_ADDR_9_FUNCTION},
		{AVR32_EBI_ADDR_10_PIN, AVR32_EBI_ADDR_10_FUNCTION},
		{AVR32_EBI_ADDR_11_PIN, AVR32_EBI_ADDR_11_FUNCTION},
		{AVR32_EBI_ADDR_12_PIN, AVR32_EBI_ADDR_12_FUNCTION},
		{AVR32_EBI_ADDR_13_PIN, AVR32_EBI_ADDR_13_FUNCTION},
		{AVR32_EBI_ADDR_14_PIN, AVR32_EBI_ADDR_14_FUNCTION},
		{AVR32_EBI_ADDR_15_PIN, AVR32_EBI_ADDR_15_FUNCTION},
		{AVR32_EBI_ADDR_16_PIN, AVR32_EBI_ADDR_16_FUNCTION},
		{AVR32_EBI_ADDR_17_PIN, AVR32_EBI_ADDR_17_FUNCTION},
		{AVR32_EBI_ADDR_18_PIN, AVR32_EBI_ADDR_18_FUNCTION},
		{AVR32_EBI_ADDR_19_PIN, AVR32_EBI_ADDR_19_FUNCTION},
		{AVR32_EBI_ADDR_20_1_PIN, AVR32_EBI_ADDR_20_1_FUNCTION},
		{AVR32_EBI_ADDR_21_1_PIN, AVR32_EBI_ADDR_21_1_FUNCTION},
		{AVR32_EBI_ADDR_22_1_PIN, AVR32_EBI_ADDR_22_1_FUNCTION},
#if NS8100_ORIGINAL_PROTOTYPE
		{AVR32_EBI_ADDR_23_PIN, AVR32_EBI_ADDR_23_FUNCTION},
#endif

		//=====================================================
		// EBI - Other
		{AVR32_EBI_NWE0_0_PIN, AVR32_EBI_NWE0_0_FUNCTION},
		{AVR32_EBI_NWE1_0_PIN, AVR32_EBI_NWE1_0_FUNCTION},
		{AVR32_EBI_NRD_0_PIN, AVR32_EBI_NRD_0_FUNCTION},
		{AVR32_EBI_NCS_0_1_PIN, AVR32_EBI_NCS_0_1_FUNCTION},
		{AVR32_EBI_NCS_1_PIN, AVR32_EBI_NCS_1_FUNCTION},
		{AVR32_EBI_NCS_2_PIN, AVR32_EBI_NCS_2_FUNCTION},
		{AVR32_EBI_NCS_3_PIN, AVR32_EBI_NCS_3_FUNCTION},
		
		//=====================================================
		// EIC
		{AVR32_EIC_EXTINT_4_PIN, AVR32_EIC_EXTINT_4_FUNCTION},
		{AVR32_EIC_EXTINT_5_PIN, AVR32_EIC_EXTINT_5_FUNCTION},

		// External RTC sampling interrupt
#if (EXTERNAL_SAMPLING_SOURCE || NS8100_ALPHA_PROTOTYPE || NS8100_BETA_PROTOTYPE)
		{AVR32_EIC_EXTINT_1_PIN, AVR32_EIC_EXTINT_1_FUNCTION},
#endif

		// Low battery interrupt
#if (NS8100_ALPHA_PROTOTYPE || NS8100_BETA_PROTOTYPE)
		{AVR32_EIC_EXTINT_0_PIN, AVR32_EIC_EXTINT_0_FUNCTION},
#endif

		//=====================================================
		// TWI
		{AVR32_TWI_SDA_0_0_PIN, AVR32_TWI_SDA_0_0_FUNCTION},
		{AVR32_TWI_SCL_0_0_PIN, AVR32_TWI_SCL_0_0_FUNCTION},

		//=====================================================
		// USB
#if (NS8100_ALPHA_PROTOTYPE || NS8100_BETA_PROTOTYPE)
		{AVR32_USBB_USB_VBOF_0_1_PIN, AVR32_USBB_USB_VBOF_0_1_FUNCTION},
		{AVR32_USBB_USB_ID_0_1_PIN, AVR32_USBB_USB_ID_0_1_FUNCTION},
#endif

		//=====================================================
		// Usart 0 - RS232 Debug
#if (NS8100_ALPHA_PROTOTYPE || NS8100_BETA_PROTOTYPE)
		{AVR32_USART0_RXD_0_0_PIN, AVR32_USART0_RXD_0_0_FUNCTION},
		{AVR32_USART0_TXD_0_0_PIN, AVR32_USART0_TXD_0_0_FUNCTION},
#endif

		//=====================================================
		// Usart 1 - RS232
		{AVR32_USART1_RXD_0_0_PIN, AVR32_USART1_RXD_0_0_FUNCTION}, // PA05
		{AVR32_USART1_TXD_0_0_PIN, AVR32_USART1_TXD_0_0_FUNCTION}, // PA06
		{AVR32_USART1_RI_0_PIN, AVR32_USART1_RI_0_FUNCTION}, // PB26
		{AVR32_USART1_DTR_0_PIN, AVR32_USART1_DTR_0_FUNCTION}, // PB25
		{AVR32_USART1_DSR_0_PIN, AVR32_USART1_DSR_0_FUNCTION}, // PB24
		{AVR32_USART1_DCD_0_PIN, AVR32_USART1_DCD_0_FUNCTION}, // PB23
		{AVR32_USART1_RTS_0_0_PIN, AVR32_USART1_RTS_0_0_FUNCTION}, // PA08
		{AVR32_USART1_CTS_0_0_PIN, AVR32_USART1_CTS_0_0_FUNCTION}, // PA09

		//=====================================================
		// Usart 3 - RS485
		{AVR32_USART3_RXD_0_0_PIN, AVR32_USART3_RXD_0_0_FUNCTION},
		{AVR32_USART3_TXD_0_0_PIN, AVR32_USART3_TXD_0_0_FUNCTION},
		{AVR32_USART3_RTS_0_1_PIN, AVR32_USART3_RTS_0_1_FUNCTION},

		//=====================================================
		// Voltage monitor pins
		{AVR32_ADC_AD_2_PIN, AVR32_ADC_AD_2_FUNCTION},
		{AVR32_ADC_AD_3_PIN, AVR32_ADC_AD_3_FUNCTION}
	};

	gpio_enable_module(SMC_EBI_GPIO_MAP, sizeof(SMC_EBI_GPIO_MAP) / sizeof(SMC_EBI_GPIO_MAP[0]));
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitProcessorNoConnectPins(void)
{
#if 0 /* old hw */

#if NS8100_ORIGINAL_PROTOTYPE
	gpio_clr_gpio_pin(AVR32_PIN_PA00); // USART0_RXD
	gpio_clr_gpio_pin(AVR32_PIN_PA01); // USART0_TXD
	gpio_clr_gpio_pin(AVR32_EBI_SDA10_0_PIN);
	gpio_clr_gpio_pin(AVR32_EBI_RAS_0_PIN);
	gpio_clr_gpio_pin(AVR32_EBI_NWAIT_0_PIN);

	//gpio_clr_gpio_pin(AVR32_PIN_PB19); // GPIO 51 (Pin 143)
	//gpio_clr_gpio_pin(AVR32_EBI_CAS_0_PIN); // GPIO 45 (Pin 126)
	//gpio_clr_gpio_pin(AVR32_EBI_SDWE_0_PIN); // GPIO 46 (Pin 127)
	//gpio_clr_gpio_pin(AVR32_EBI_SDCS_0_PIN); // GPIO 62 (Pin 21)
#else /* (NS8100_ALPHA_PROTOTYPE || NS8100_BETA_PROTOTYPE) (Pins brought to connector so can be an input or output) */
	if (GET_HARDWARE_ID == HARDWARE_ID_REV_8_WITH_GPS_MOD) // Unit with GPS Module
	{
		gpio_clr_gpio_pin(AVR32_PIN_PB13); // GPIO 45 (Pin 126 / EBI - CAS) // Set GPS UART Baud config low to prevent back powering the GPS module (Pin 7 Expansion header P2)
		gpio_set_gpio_pin(AVR32_PIN_PB14); // GPIO 46 (Pin 127 / EBI - SDWE) // Disable power for Gps module (Pin 6 Expansion header P2)
		gpio_clr_gpio_pin(AVR32_PIN_PB19); // GPIO 51 (Pin 143 / U28-143) // Gps eeprom data line (Pin 13 Expansion header P2)
		gpio_enable_gpio_pin(AVR32_PIN_PB30); // GPIO 62 (Pin 21 / EBI - SDCS) // Gps module input (Pin 1 Expansion header P2)
		gpio_enable_pin_pull_up(AVR32_PIN_PB30); // Enable pull-up on pin
	}
	else // Normal unit
	{
		gpio_clr_gpio_pin(AVR32_PIN_PB30); // GPIO 62 (Pin 21)
		gpio_clr_gpio_pin(AVR32_PIN_PB19); // GPIO 51 (Pin 143)
		gpio_clr_gpio_pin(AVR32_PIN_PB13); // GPIO 45 (Pin 126) // Set line low to prevent back powering the GPS module if attached
		gpio_set_gpio_pin(AVR32_PIN_PB14); // GPIO 46 (Pin 127) // Set line high to disable power for GPS module if attached
	}
#endif

#if INTERNAL_SAMPLING_SOURCE
	// This pin is unused if the internal sampling source is configured. Set pin as an output to prevent it from floating
	gpio_clr_gpio_pin(AVR32_PIN_PA22); // USB_VBOF
#endif
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void Avr32_chip_select_init(unsigned long hsb_hz)
{
#if 0 /* old hw */

	unsigned long int hsb_mhz_up = (hsb_hz + 999999) / 1000000;

	// Setup all 4 chip selects
	SMC_CS_SETUP(0)			// LCD
	SMC_CS_SETUP(1)			// External RAM
	SMC_CS_SETUP_SPECIAL(2)	// Network/LAN
	SMC_CS_SETUP(3)			// Network/LAN Memory

	// Put the multiplexed MCU pins used for the SM under control of the SMC.
	Avr32_enable_muxed_pins();
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#define TEST_DISABLE_32K_CRYSTAL	0
void _init_startup(void)
{
#if 0 /* old hw */
	// Disable watchdog if reset from boot loader
	AVR32_WDT.ctrl = (AVR32_WDT_KEY_VALUE_ASSERT | AVR32_WDT_DISABLE_VALUE);
	AVR32_WDT.ctrl = (AVR32_WDT_KEY_VALUE_DEASSERT | AVR32_WDT_DISABLE_VALUE);
	
	//-----------------------------------------------------------------
	// Enable External 12 MHz oscillator clock
	//-----------------------------------------------------------------
	pm_enable_osc0_ext_clock(&AVR32_PM);

	//-----------------------------------------------------------------
	// Switch the main clock to the external oscillator 0 (12 MHz)
	//-----------------------------------------------------------------
	// Using shorter delay to lock for start/restart of clock (instead of OSC0_STARTUP)
	pm_switch_to_osc0(&AVR32_PM, FOSC0, AVR32_PM_OSCCTRL0_STARTUP_0_RCOSC);

	//-----------------------------------------------------------------
	// Set clock to 66 MHz
	//-----------------------------------------------------------------
	// Logic to change the clock source to PLL 0
	// PLL = 0, Multiplier = 10 (actual 11), Divider = 1 (actually 1), OSC = 0, 16 clocks to stabilize
	pm_pll_setup(&AVR32_PM, 0, 10, 1, 0, 16);
	pm_pll_set_option(&AVR32_PM, 0, 1, 1, 0); // PLL = 0, Freq = 1, Div2 = 1

	// Enable and lock
	pm_pll_enable(&AVR32_PM, 0);
	pm_wait_for_pll0_locked(&AVR32_PM);

	pm_cksel(&AVR32_PM, 0, 0, 0, 0, 0, 0);
	flashc_set_wait_state(1);
	pm_switch_to_clock(&AVR32_PM, AVR32_PM_MCSEL_PLL0);

	// Chip Select Initialization
	Avr32_chip_select_init(FOSC0);
	
	// Disable the unused and non connected clock 1
	pm_disable_clk1(&AVR32_PM);

	// With clock 1 disabled, configure GPIO lines to be outputs and low
	gpio_clr_gpio_pin(AVR32_PM_XIN1_0_PIN);
	gpio_clr_gpio_pin(AVR32_PM_XOUT1_0_PIN);

#if TEST_DISABLE_32K_CRYSTAL
	// Disable the 32KHz crystal
	pm_disable_clk32(&AVR32_PM);
	
	// With the 32KHz crystal disabled, configure GPIO lines to be outputs and low
	gpio_clr_gpio_pin(AVR32_PM_XIN32_0_PIN);
	gpio_clr_gpio_pin(AVR32_PM_XOUT32_0_PIN);
#endif

	SoftUsecWait(1000);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitPullupsOnFloatingDataLines(void)
{
#if 0 /* old hw */

	gpio_enable_pin_pull_up(AVR32_PIN_PX00);
	gpio_enable_pin_pull_up(AVR32_PIN_PX01);
	gpio_enable_pin_pull_up(AVR32_PIN_PX02);
	gpio_enable_pin_pull_up(AVR32_PIN_PX03);
	gpio_enable_pin_pull_up(AVR32_PIN_PX04);
	gpio_enable_pin_pull_up(AVR32_PIN_PX05);
	gpio_enable_pin_pull_up(AVR32_PIN_PX06);
	gpio_enable_pin_pull_up(AVR32_PIN_PX07);
	gpio_enable_pin_pull_up(AVR32_PIN_PX08);
	gpio_enable_pin_pull_up(AVR32_PIN_PX09);
	gpio_enable_pin_pull_up(AVR32_PIN_PX10);
	gpio_enable_pin_pull_up(AVR32_PIN_PX35);
	gpio_enable_pin_pull_up(AVR32_PIN_PX36);
	gpio_enable_pin_pull_up(AVR32_PIN_PX37);
	gpio_enable_pin_pull_up(AVR32_PIN_PX38);
	gpio_enable_pin_pull_up(AVR32_PIN_PX39);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitSerial485(void)
{
#if 0 /* old hw */

	PowerControl(SERIAL_485_DRIVER_ENABLE, ON);
#if (NS8100_ALPHA_PROTOTYPE || NS8100_BETA_PROTOTYPE)
	PowerControl(SERIAL_485_RECEIVER_ENABLE, ON);
#endif

	// Options for debug USART.
	usart_options_t usart_3_rs485_usart_options =
	{
		.baudrate = 38400,
		.charlength = 8,
		.paritytype = USART_NO_PARITY,
		.stopbits = USART_1_STOPBIT,
		.channelmode = USART_NORMAL_CHMODE
	};

	// Initialize it in RS485 mode.
	usart_init_rs485(&AVR32_USART3, &usart_3_rs485_usart_options, FOSC0);

	PowerControl(SERIAL_485_DRIVER_ENABLE, OFF);
#if (NS8100_ALPHA_PROTOTYPE || NS8100_BETA_PROTOTYPE)
	PowerControl(SERIAL_485_RECEIVER_ENABLE, OFF);
#endif

	// Make sure 485 lines aren't floating
	gpio_enable_pin_pull_up(AVR32_USART3_RXD_0_0_PIN);
	gpio_enable_pin_pull_up(AVR32_USART3_TXD_0_0_PIN);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitSerial232(void)
{
#if 0 /* old hw */
	// Setup debug serial port
	usart_options_t usart_1_rs232_options =
	{
		.baudrate = 115200,
		.charlength = 8,
		.paritytype = USART_NO_PARITY,
		.stopbits = USART_1_STOPBIT,
		.channelmode = USART_NORMAL_CHMODE
	};

	// Load the Unit Config to get the stored Baud rate. Only dependency should be SPI
	GetRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

	// Check if the Unit Config is valid
	if (g_unitConfig.validationKey == 0xA5A5)
	{
		// Set the baud rate to the user stored baud rate setting (initialized to 115200)
		switch (g_unitConfig.baudRate)
		{
			case BAUD_RATE_57600: usart_1_rs232_options.baudrate = 57600; break;
			case BAUD_RATE_38400: usart_1_rs232_options.baudrate = 38400; break;
			case BAUD_RATE_19200: usart_1_rs232_options.baudrate = 19200; break;
			case BAUD_RATE_9600: usart_1_rs232_options.baudrate = 9600; break;
			default: usart_1_rs232_options.baudrate = 115200; break;
		}
	}

	// Initialize it in RS232 mode.
	usart_init_modem(&AVR32_USART1, &usart_1_rs232_options, FOSC0);

	// Enable external driver and receiver
	PowerControl(SERIAL_232_DRIVER_ENABLE, ON);
	PowerControl(SERIAL_232_RECEIVER_ENABLE, ON);

	// Enable internal pullups on input lines since external pullups aren't present
	gpio_enable_pin_pull_up(AVR32_USART1_RXD_0_0_PIN);
	gpio_enable_pin_pull_up(AVR32_USART1_DCD_0_PIN);
	gpio_enable_pin_pull_up(AVR32_USART1_DSR_0_PIN);
	gpio_enable_pin_pull_up(AVR32_USART1_CTS_0_0_PIN);
	gpio_enable_pin_pull_up(AVR32_USART1_RI_0_PIN);

	/* To prevent the TXD line from falling when the USART is disabled, the use of an internal pull up
	is mandatory. If the hardware handshaking feature or Modem mode is used, the internal pull up
	on TXD must also be enabled. */
	gpio_enable_pin_pull_up(AVR32_USART1_TXD_0_0_PIN);

	sprintf((char*)g_spareBuffer, "-----     NS8100 Fresh boot, App version: %s (Date: %s)     -----\r\n", (char*)g_buildVersion, (char*)g_buildDate);
	usart_write_line((&AVR32_USART1), "\r\n\n");
	usart_write_line((&AVR32_USART1), "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\r\n");
	usart_write_line((&AVR32_USART1), "---------------------------------------------------------------------------------------\r\n");
	usart_write_line((&AVR32_USART1), (char*)g_spareBuffer);
	usart_write_line((&AVR32_USART1), "---------------------------------------------------------------------------------------\r\n");
	usart_write_line((&AVR32_USART1), "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\r\n");
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitDebug232(void)
{
#if 0 /* old hw */

#if (NS8100_ALPHA_PROTOTYPE || NS8100_BETA_PROTOTYPE)
	// Setup debug serial port
	usart_options_t usart_0_rs232_options =
	{
		.baudrate = 115200,
		.charlength = 8,
		.paritytype = USART_NO_PARITY,
		.stopbits = USART_1_STOPBIT,
		.channelmode = USART_NORMAL_CHMODE
	};

	// Initialize it in RS232 mode.
	usart_init_rs232(&AVR32_USART0, &usart_0_rs232_options, FOSC0);

	sprintf((char*)g_spareBuffer, "-----     NS8100 Debug port, App version: %s (Date: %s)     -----\r\n", (char*)g_buildVersion, (char*)g_buildDate);
	usart_write_line((&AVR32_USART0), "\r\n\n");
	usart_write_line((&AVR32_USART0), "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\r\n");
	usart_write_line((&AVR32_USART0), "---------------------------------------------------------------------------------------\r\n");
	usart_write_line((&AVR32_USART0), (char*)g_spareBuffer);
	usart_write_line((&AVR32_USART0), "---------------------------------------------------------------------------------------\r\n");
	usart_write_line((&AVR32_USART0), "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\r\n\r\n");
#endif
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitGps232(void)
{
#if 0 /* old hw */
	// Setup Gps serial port
	usart_options_t usart_0_rs232_options =
	{
		.baudrate = 9600,
		.charlength = 8,
		.paritytype = USART_NO_PARITY,
		.stopbits = USART_1_STOPBIT,
		.channelmode = USART_NORMAL_CHMODE
	};

	// Initialize RS232 mode.
	usart_init_rs232(&AVR32_USART0, &usart_0_rs232_options, FOSC0);

	gpio_enable_pin_pull_up(AVR32_USART0_RXD_0_0_PIN);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitLANToSleep(void)
{
#if 0 /* old hw */
#if 1 /* Normal */
	// Enable the LAN Sleep
	PowerControl(LAN_SLEEP_ENABLE, ON);
	SoftUsecWait(10 * SOFT_MSECS);

	//Sleep8900();

	ToggleLedOn8900();
	SoftUsecWait(250 * SOFT_MSECS);
	ToggleLedOff8900();
	SoftUsecWait(250 * SOFT_MSECS);

	ToggleLedOn8900();
	SoftUsecWait(250 * SOFT_MSECS);
	ToggleLedOff8900();
	SoftUsecWait(250 * SOFT_MSECS);

	//Sleep8900_LedOn();
	Sleep8900();
#endif
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitExternalKeypad(void)
{
#if 0 /* old hw */

	InitTWI();
	InitMcp23018();

	// Primer read
	uint8 keyScan = ReadMcp23018(IO_ADDRESS_KPD, GPIOB);
	if (keyScan)
	{
		debugWarn("Keypad key being pressed, likely a bug. Key: %x", keyScan);
	}

#if NS8100_ORIGINAL_PROTOTYPE
	// Turn on the red keypad LED while loading
	WriteMcp23018(IO_ADDRESS_KPD, GPIOA, ((ReadMcp23018(IO_ADDRESS_KPD, GPIOA) & 0xCF) | RED_LED_PIN));
#endif
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitInternalRTC(void)
{
#if 0 /* old hw */

	rtc_init(&AVR32_RTC, 1, 0);

	// Set top value to generate an interrupt every 1/2 second
	// Data sheet: When enabled, the RTC will increment until it reaches TOP, and then wrap to 0x0. The status bit TOPI in ISR is set when this occurs
	// From 0x0 the counter will count TOP+1 cycles of the source clock before it wraps back to 0x0
	rtc_set_top_value(&AVR32_RTC, 8191);

	// Enable the Internal RTC
	rtc_enable(&AVR32_RTC);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitInternalAD(void)
{
#if 0 /* old hw */

	adc_configure(&AVR32_ADC);

	// Enable the A/D channels; Warning: Can't use the driver call 'adc_enable' because it's a single channel enable only (write only register)
	AVR32_ADC.cher = 0x0C; // Directly enable
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitSDAndFileSystem(void)
{
#if 0 /* old hw */
	// Necessary ? (No, setting an output will configure it)
	// Set SD Power pin as GPIO
	gpio_enable_gpio_pin(AVR32_PIN_PB15);

	// Necessary ? (No, pin is configured as an input on power up)
	// Set SD Write Protect pin as GPIO (Active low control)
	gpio_enable_gpio_pin(AVR32_PIN_PA07);
	
#if 1 /* NS8100_BETA_PROTOTYPE */
	gpio_enable_pin_pull_up(AVR32_PIN_PA07);
#endif

	// Necessary ? (No, pin is configured as an input on power up)
	// Set SD Detect pin as GPIO
	gpio_enable_gpio_pin(AVR32_PIN_PA02);
	
#if 1 /* NS8100_BETA_PROTOTYPE */
	gpio_enable_pin_pull_up(AVR32_PIN_PA02);
#endif

	// Enable Power to SD
	PowerControl(SD_POWER, ON);

	// Wait for power to propagate
	SoftUsecWait(10 * SOFT_MSECS);

	// Check if SD Detect pin
	if (gpio_get_pin_value(AVR32_PIN_PA02) == SDMMC_CARD_DETECTED)
	{
		GetSpi1MutexLock(SDMMC_LOCK);

		spi_selectChip(&AVR32_SPI1, SD_MMC_SPI_NPCS);
		if (sd_mmc_spi_internal_init() != OK)
		{
			debugErr("SD MMC Internal Init failed\r\n");
			OverlayMessage(getLangText(ERROR_TEXT), getLangText(FAILED_TO_INIT_SD_CARD_TEXT), (3 * SOFT_SECS));
		}
		spi_unselectChip(&AVR32_SPI1, SD_MMC_SPI_NPCS);

		// Init the NAV and select the SD MMC Card
		nav_reset();
		nav_select(FS_NAV_ID_DEFAULT);

		// Check if the drive select was successful
		if (nav_drive_set(0) == TRUE)
		{
			// Check if the partition mount was unsuccessful (otherwise passes through without an error case)
			if (nav_partition_mount() == FALSE)
			{
				// Error case
				debugErr("FAT32 SD Card mount failed\r\n");
				OverlayMessage(getLangText(ERROR_TEXT), getLangText(FAILED_TO_MOUNT_SD_CARD_TEXT), (3 * SOFT_SECS));
			}
		}
		else // Error case
		{
			debugErr("FAT32 SD Card drive select failed\r\n");
			OverlayMessage(getLangText(ERROR_TEXT), getLangText(FAILED_TO_SELECT_SD_CARD_DRIVE_TEXT), (3 * SOFT_SECS));
		}

		ReleaseSpi1MutexLock();
	}
	else
	{
		debugErr("SD Card not detected\r\n");
		OverlayMessage(getLangText(ERROR_TEXT), getLangText(SD_CARD_IS_NOT_PRESENT_TEXT), (10 * SOFT_SECS));
	}
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitExternalAD(void)
{
#if 0 /* old hw */
	// Enable the A/D
#if EXTENDED_DEBUG
	debug("Enable the A/D\r\n");
#endif

	PowerControl(ANALOG_SLEEP_ENABLE, OFF);

	// Delay to allow AD to power up/stabilize
	SoftUsecWait(50 * SOFT_MSECS);

	// Setup the A/D Channel configuration
#if EXTENDED_DEBUG
	debug("Setup A/D config and channels (External Ref, Temp On)\r\n");
#endif
	SetupADChannelConfig(SAMPLE_RATE_DEFAULT, UNIT_CONFIG_CHANNEL_VERIFICATION);

	// Read a few test samples
	GetChannelOffsets(SAMPLE_RATE_DEFAULT);

#if EXTENDED_DEBUG
	debug("Disable the A/D\r\n");
#endif
	PowerControl(ANALOG_SLEEP_ENABLE, ON);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestPowerDownAndStop(void)
{
#if 0 /* old hw */
	// Turn off the keypad LED
	WriteMcp23018(IO_ADDRESS_KPD, GPIOA, ((ReadMcp23018(IO_ADDRESS_KPD, GPIOA) & 0xCF) | NO_LED_PINS));

	spi_reset(&AVR32_SPI1);
	gpio_clr_gpio_pin(AVR32_SPI1_MISO_0_0_PIN);
	gpio_clr_gpio_pin(AVR32_SPI1_MOSI_0_0_PIN);
	gpio_clr_gpio_pin(AVR32_SPI1_NPCS_3_PIN);

	debug("\nClosing up shop.\n\r\n");

	// Disable rs232 driver and receiver (Active low control)
	PowerControl(SERIAL_232_DRIVER_ENABLE, OFF);
	PowerControl(SERIAL_232_RECEIVER_ENABLE, OFF);

	//DisplayTimerCallBack();
	SetLcdBacklightState(BACKLIGHT_OFF);

	//LcdPwTimerCallBack();
	PowerControl(LCD_CONTRAST_ENABLE, OFF);
	ClearLcdDisplay();
	ClearControlLinesLcdDisplay();
	LcdClearPortReg();
	PowerControl(LCD_POWER_ENABLE, OFF);

	// Drive the unused pin
	gpio_clr_gpio_pin(AVR32_PIN_PB20);

	//SLEEP(AVR32_PM_SMODE_IDLE);
	SLEEP(AVR32_PM_SMODE_STOP);
	//SLEEP(AVR32_PM_SMODE_STANDBY);
	while (1) {}
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void KillClocksToModules(void)
{
#if 0 /* old hw */
	// Leave active: SYSTIMER; Disable: OCD
	AVR32_PM.cpumask = 0x0100;
	
	// Leave active: EBI, PBA & PBB BRIDGE, FLASHC; Disable: PDCA, MACB, USBB
	AVR32_PM.hsbmask = 0x0047;
	
	// Leave active: TC, TWI, SPI0, SPI1, ADC, PM/RTC/EIC, GPIO, INTC; Disable: ABDAC, SSC, PWM, USART 0 & 1 & 2 & 3, PDCA
	AVR32_PM.pbamask = 0x40FB;
	
	// Leave active: SMC, FLASHC, HMATRIX; Disable: SDRAMC, MACB, USBB
	AVR32_PM.pbbmask = 0x0015;
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitGplpRegisters(void)
{
#if 0 /* old hw */

	AVR32_PM.gplp[0] = 0x12345678;
	AVR32_PM.gplp[1] = 0x90ABCDEF;
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitUSBClockAndIOLines(void)
{
#if 0 /* old hw */

	pm_configure_usb_clock();

#if 0 /* Moved to USB device manager */
	// Init USB and Mass Storage drivers
	usb_task_init();
	device_mass_storage_task_init();
#endif

#if (NS8100_ALPHA_PROTOTYPE || NS8100_BETA_PROTOTYPE)
	// Disable USB LED
	PowerControl(USB_LED, OFF);
#endif

#if NS8100_ORIGINAL_PROTOTYPE
	// Enable internal pullup for USB ID since external pullup isn't present
	gpio_enable_pin_pull_up(AVR32_PIN_PA21);
#elif (NS8100_ALPHA_PROTOTYPE || NS8100_BETA_PROTOTYPE)
	// Enable internal pullup for USB ID since external pullup isn't present
	gpio_enable_pin_pull_up(AVR32_USBB_USB_ID_0_1_PIN);
	// Enable internal pullup for USB OC (Active low) since external pullup isn't present
	gpio_enable_pin_pull_up(AVR32_PIN_PB12);
#endif
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitLCD(void)
{
#if 0 /* old hw */

	PowerControl(LCD_CONTRAST_ENABLE, ON);
	PowerControl(LCD_POWER_ENABLE, ON);
	SoftUsecWait(LCD_ACCESS_DELAY);
	Backlight_On();
	Backlight_High();
	Set_Contrast(DEFUALT_CONTRAST);
	InitDisplay();

#if (NS8100_ALPHA_PROTOTYPE || NS8100_BETA_PROTOTYPE)
	memcpy(g_mmap, sign_on_logo, (8*128));
	WriteMapToLcd(g_mmap);
#endif
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitExternalRTC(void)
{
#if 0 /* old hw */

	ExternalRtcInit();

	// Set RTC Timestamp pin high (Active low control)
	PowerControl(RTC_TIMESTAMP, OFF);

#if (NS8100_ALPHA_PROTOTYPE || NS8100_BETA_PROTOTYPE)
	// Enable internal pullup on RTC PFO (Active low) since external pullups aren't present
	gpio_enable_pin_pull_up(AVR32_PIN_PA21);
#endif

#if (EXTERNAL_SAMPLING_SOURCE || NS8100_ALPHA_PROTOTYPE || NS8100_BETA_PROTOTYPE)
	// The internal pull up for this pin needs to be enables to bring the line high, otherwise the clock out will only reach half level
	gpio_enable_pin_pull_up(AVR32_EIC_EXTINT_1_PIN);
#endif
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void TestExternalRAM(void)
{
	uint32 i, j;
	uint32 index;
	uint32 printErrors = 0;
	uint32 testSize = 5120; // Was (EVENT_BUFF_SIZE_IN_WORDS) - 614400

#if EXTENDED_DEBUG
	debug("External RAM Test: Incrementing index with rolling increment...\r\n");
#endif

	for (i = 0, j = 0, index = 0; index < testSize; index++)
	{
		g_eventDataBuffer[index] = (uint16)(i + j); i++;
		if ((i & 0xFFFF) == 0) { j++; }
	}

	for (i = 0, j = 0, index = 0; index < testSize; index++)
	{
		if (g_eventDataBuffer[index] != (uint16)(i + j))
		{
			debugErr("Test of External RAM: failed (Index: %d, Address: 0x%x, Expected: 0x%x, Got: 0x%x)\r\n",
			index, &g_eventDataBuffer[index], (uint16)(i + j), g_eventDataBuffer[index]);
			printErrors++; if (printErrors > 5000) { debugErr("Too many errors, bailing on memory test\r\n"); return; }
		}
		i++;
		if ((i & 0xFFFF) == 0) { j++; }
	}

	if (printErrors) { debug("External RAM: Total errors: %d\r\n", printErrors); }
#if EXTENDED_DEBUG
	else { debug("Test of External RAM: passed\r\n"); }
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void PowerDownAndHalt(void)
{
#if 0 /* old hw */
	// Enable the A/D
	debug("Enable the A/D\r\n");
	PowerControl(ANALOG_SLEEP_ENABLE, OFF);

	// Delay to allow AD to power up/stabilize
	SoftUsecWait(50 * SOFT_MSECS);

	debug("Setup A/D config and channels\r\n");
	// Setup the A/D Channel configuration
	SetupADChannelConfig(1024, UNIT_CONFIG_CHANNEL_VERIFICATION);

	debug("Disable the A/D\r\n");
	PowerControl(ANALOG_SLEEP_ENABLE, OFF);

	spi_reset(&AVR32_SPI1);

	gpio_clr_gpio_pin(AVR32_SPI1_MISO_0_0_PIN);
	gpio_clr_gpio_pin(AVR32_SPI1_MOSI_0_0_PIN);
	gpio_clr_gpio_pin(AVR32_SPI1_NPCS_3_PIN);

	debug("\nClosing up shop.\n\r\n");

	// Disable rs232 driver and receiver (Active low control)
	PowerControl(SERIAL_232_DRIVER_ENABLE, OFF);
	PowerControl(SERIAL_232_RECEIVER_ENABLE, OFF);

	//DisplayTimerCallBack();
	SetLcdBacklightState(BACKLIGHT_OFF);

	//LcdPwTimerCallBack();
	PowerControl(LCD_CONTRAST_ENABLE, OFF);
	ClearLcdDisplay();
	ClearControlLinesLcdDisplay();
	LcdClearPortReg();
	PowerControl(LCD_POWER_ENABLE, OFF);

	SLEEP(AVR32_PM_SMODE_STOP);

	while (1) {}
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitSystemHardware_NS8100(void)
{
	//-------------------------------------------------------------------------
	// Set General Purpose Low-Power registers
	//-------------------------------------------------------------------------
	InitGplpRegisters();

	//-------------------------------------------------------------------------
	// Disable all interrupts and clear all interrupt vectors 
	//-------------------------------------------------------------------------
#if 0 /* old hw */
	Disable_global_interrupt();
	INTC_init_interrupts();
#endif

	//-------------------------------------------------------------------------
	// Enable internal pull ups on the floating data lines
	//-------------------------------------------------------------------------
	InitPullupsOnFloatingDataLines();

	//-------------------------------------------------------------------------
	// Init unused pins to low outputs
	//-------------------------------------------------------------------------
	InitProcessorNoConnectPins();
	
	//-------------------------------------------------------------------------
	// Set Alarm 1 and Alarm 2 low (Active high control)
	//-------------------------------------------------------------------------
	PowerControl(ALARM_1_ENABLE, OFF);
	PowerControl(ALARM_2_ENABLE, OFF);

	//-------------------------------------------------------------------------
	// Set Trigger Out low (Active high control)
	//-------------------------------------------------------------------------
	PowerControl(TRIGGER_OUT, OFF);

	//-------------------------------------------------------------------------
	// Set USB LED Output low (Active high control)
	//-------------------------------------------------------------------------
	PowerControl(USB_LED, OFF);

	//-------------------------------------------------------------------------
	// Configure Debug rs232
	//-------------------------------------------------------------------------
	if (GET_HARDWARE_ID != HARDWARE_ID_REV_8_WITH_GPS_MOD)
	{
		InitDebug232();	debug("Debug Port enabled\r\n");
	}

	//-------------------------------------------------------------------------
	// Smart Sensor data/control init (Hardware pull up on signal)
	//-------------------------------------------------------------------------
	OneWireInit(); debug("One Wire init complete\r\n");

	//-------------------------------------------------------------------------
	// Init the SPI interfaces
	//-------------------------------------------------------------------------
	SPI_0_Init(); debug("SPI0 init complete\r\n");
	SPI_1_Init(); debug("SPI1 init complete\r\n");
	
	//-------------------------------------------------------------------------
	// Turn on rs232 driver and receiver (Active low control)
	//-------------------------------------------------------------------------
	InitSerial232(); debug("Craft RS232 init complete\r\n");

	//-------------------------------------------------------------------------
	// Turn on rs485 driver and receiver
	//-------------------------------------------------------------------------
	InitSerial485(); debug("RS485 init complete\r\n");

	//-------------------------------------------------------------------------
	// Initialize the external RTC
	//-------------------------------------------------------------------------
	InitExternalRTC(); debug("External RTC init complete\r\n");

	//-------------------------------------------------------------------------
	// Set LAN to Sleep
	//-------------------------------------------------------------------------
	InitLANToSleep(); debug("LAN init complete\r\n");

	//-------------------------------------------------------------------------
	// Initialize the AD Control
	//-------------------------------------------------------------------------
	InitAnalogControl(); debug("Analog Control init complete\r\n");

	//-------------------------------------------------------------------------
	// Init the LCD display
	//-------------------------------------------------------------------------
	InitLCD(); debug("LCD Display init complete\r\n");

	//-------------------------------------------------------------------------
	// Init the Internal RTC for half second tick used for state processing
	//-------------------------------------------------------------------------
	InitInternalRTC(); debug("Internal RTC init complete\r\n");

	//-------------------------------------------------------------------------
	// Enable Processor A/D
	//-------------------------------------------------------------------------
	InitInternalAD(); debug("Internal A/D init complete\r\n");

	//-------------------------------------------------------------------------
	// Power on the SD Card and init the file system
	//-------------------------------------------------------------------------
	InitSDAndFileSystem(); debug("SD Card and filesystem init complete\r\n");

	//-------------------------------------------------------------------------
	// Initialize USB clock.
	//-------------------------------------------------------------------------
	InitUSBClockAndIOLines(); debug("USB Clock and I/O lines init complete\r\n");

	//-------------------------------------------------------------------------
	// Init Keypad
	//-------------------------------------------------------------------------
	InitExternalKeypad(); debug("Keyboard init complete\r\n");

	//-------------------------------------------------------------------------
	// Init and configure the A/D to prevent the unit from burning current charging internal reference (default config)
	//-------------------------------------------------------------------------
	InitExternalAD(); debug("External A/D init complete\r\n");

	//-------------------------------------------------------------------------
	// Set the power savings mode based on the saved setting
	//-------------------------------------------------------------------------
	AdjustPowerSavings(); debug("Power Savings init complete\r\n");

	//-------------------------------------------------------------------------
	// Test the External RAM Event buffer to make sure it's valid
	//-------------------------------------------------------------------------
	TestExternalRAM(); debug("External RAM Test init complete\r\n");

	//-------------------------------------------------------------------------
	// Read and cache Smart Sensor data
	//-------------------------------------------------------------------------
	SmartSensorReadRomAndMemory(SEISMIC_SENSOR); debug("Smart Sensor check for Seismic sensor\r\n");
	SmartSensorReadRomAndMemory(ACOUSTIC_SENSOR); debug("Smart Sensor check for Acoustic sensor\r\n");

	//-------------------------------------------------------------------------
	// Enable Power off protection
	//-------------------------------------------------------------------------
	PowerControl(POWER_OFF_PROTECTION_ENABLE, ON); debug("Enabling Power Off protection\r\n");

	//-------------------------------------------------------------------------
	// Hardware initialization complete
	//-------------------------------------------------------------------------
	debug("Hardware Init complete\r\n");
}
