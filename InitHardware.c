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
#include "mxc_errors.h"
#include "uart.h"
#include "nvic_table.h"
#include "icc.h"
#include "i2c.h"
#include "gpio.h"
#include "wdt.h"
#include "spi.h"
//#include "spi_reva1.h" // try without

//#include "pm.h"
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

mxc_gpio_cfg_t g_MCUPowerLatch;
mxc_gpio_cfg_t g_Battery1VoltagePresence;
mxc_gpio_cfg_t g_Battery2VoltagePresence;
mxc_gpio_cfg_t g_GaugeAlert;
mxc_gpio_cfg_t g_BatteryChargerIRQ;
mxc_gpio_cfg_t g_Enable12V;
mxc_gpio_cfg_t g_Enable5V;
mxc_gpio_cfg_t g_Enable2_5_VRef;
mxc_gpio_cfg_t g_USBSourceEnable;
mxc_gpio_cfg_t g_USBAuxPowerEnable;
mxc_gpio_cfg_t g_PowerGood5v;
mxc_gpio_cfg_t g_PowerGoodBatteryCharge;
mxc_gpio_cfg_t g_SmartSensorSleep;
mxc_gpio_cfg_t g_SmartSensorMuxEnable;
mxc_gpio_cfg_t g_ADCReset;
mxc_gpio_cfg_t g_ADCConversion;
mxc_gpio_cfg_t g_CalMuxPreADEnable;
mxc_gpio_cfg_t g_CalMuxPreADSelect;
mxc_gpio_cfg_t g_Alert1;
mxc_gpio_cfg_t g_Alert2;
mxc_gpio_cfg_t g_LTEOTA;
mxc_gpio_cfg_t g_ExpansionEnable;
mxc_gpio_cfg_t g_ExpansionReset;
mxc_gpio_cfg_t g_ExpansionIRQ;
mxc_gpio_cfg_t g_USBCI2CIRQ;
mxc_gpio_cfg_t g_AccelInt1;
mxc_gpio_cfg_t g_AccelInt2;
mxc_gpio_cfg_t g_AccelTrig;
mxc_gpio_cfg_t g_PowerButtonIRQ;
mxc_gpio_cfg_t g_Button1;
mxc_gpio_cfg_t g_Button2;
mxc_gpio_cfg_t g_Button3;
mxc_gpio_cfg_t g_Button4;
mxc_gpio_cfg_t g_Button5;
mxc_gpio_cfg_t g_Button6;
mxc_gpio_cfg_t g_Button7;
mxc_gpio_cfg_t g_Button8;
mxc_gpio_cfg_t g_Button9;
mxc_gpio_cfg_t g_LED1;
mxc_gpio_cfg_t g_LED2;
mxc_gpio_cfg_t g_LED3;
mxc_gpio_cfg_t g_ExtRTCIntA;
mxc_gpio_cfg_t g_BLEOTA;
mxc_gpio_cfg_t g_ExternalTriggerOut;
mxc_gpio_cfg_t g_ExternalTriggerIn;
mxc_gpio_cfg_t g_LCDPowerEnable;
mxc_gpio_cfg_t g_LCDPowerDisplay;
mxc_gpio_cfg_t g_LCDInt;
mxc_gpio_cfg_t g_SensorCheckEnable;
mxc_gpio_cfg_t g_SensorCheck;
mxc_gpio_cfg_t g_LTEReset;
mxc_gpio_cfg_t g_BLEReset;
mxc_gpio_cfg_t g_SmartSensorMux_A0;
mxc_gpio_cfg_t g_SmartSensorMux_A1;
mxc_gpio_cfg_t g_Nyquist0_A0;
mxc_gpio_cfg_t g_Nyquist1_A1;
mxc_gpio_cfg_t g_Nyquist2_Enable;
mxc_gpio_cfg_t g_SensorEnable1_Geo1;
mxc_gpio_cfg_t g_SensorEnable2_Aop1;
mxc_gpio_cfg_t g_SensorEnable3_Geo2;
mxc_gpio_cfg_t g_SensorEnable4_Aop2;
mxc_gpio_cfg_t g_GainPathSelect1_Geo1;
mxc_gpio_cfg_t g_GainPathSelect2_Aop1;
mxc_gpio_cfg_t g_GainPathSelect3_Geo2;
mxc_gpio_cfg_t g_GainPathSelect4_Aop2;
mxc_gpio_cfg_t g_RTCClock;

void GaugeAlert_ISR(void *cbdata);
void BatteryCharger_ISR(void *cbdata);
void Expansion_ISR(void *cbdata);
void USBCI2C_ISR(void *cbdata);
void AccelInt1_ISR(void *cbdata);
void AccelInt2_ISR(void *cbdata);
void PowerButton_ISR(void *cbdata);
void Button1_ISR(void *cbdata);
void Button2_ISR(void *cbdata);
void Button3_ISR(void *cbdata);
void Button4_ISR(void *cbdata);
void Button5_ISR(void *cbdata);
void Button6_ISR(void *cbdata);
void Button7_ISR(void *cbdata);
void Button8_ISR(void *cbdata);
void Button9_ISR(void *cbdata);
void ExtRTCIntA_ISR(void *cbdata);
void ExternalTriggerIn_ISR(void *cbdata);
void LCD_ISR(void *cbdata);
void RTCClock_ISR(void *cbdata);

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupGPIO(void)
{
	//----------------------------------------------------------------------------------------------------------------------
	// MCU Power Latch: Port 0, Pin 1, Output, External pulldown, Active high, 3.3V
	//----------------------------------------------------------------------------------------------------------------------
	g_MCUPowerLatch.port = MXC_GPIO0;
	g_MCUPowerLatch.mask = MXC_GPIO_PIN_1;
	g_MCUPowerLatch.pad = MXC_GPIO_PAD_NONE;
	g_MCUPowerLatch.func = MXC_GPIO_FUNC_OUT;
	g_MCUPowerLatch.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_MCUPowerLatch);
	MXC_GPIO_OutClr(g_MCUPowerLatch.port, g_MCUPowerLatch.mask); // Start disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Battery 1 Voltage Presence: Port 0, Pin 2, Input, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_Battery1VoltagePresence.port = MXC_GPIO0;
	g_Battery1VoltagePresence.mask = MXC_GPIO_PIN_2;
	g_Battery1VoltagePresence.pad = MXC_GPIO_PAD_NONE;
	g_Battery1VoltagePresence.func = MXC_GPIO_FUNC_IN;
	g_Battery1VoltagePresence.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&g_Battery1VoltagePresence);

	//----------------------------------------------------------------------------------------------------------------------
	// Battery 2 Voltage Presence: Port 0, Pin 3, Input, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_Battery2VoltagePresence.port = MXC_GPIO0;
	g_Battery2VoltagePresence.mask = MXC_GPIO_PIN_3;
	g_Battery2VoltagePresence.pad = MXC_GPIO_PAD_NONE;
	g_Battery2VoltagePresence.func = MXC_GPIO_FUNC_IN;
	g_Battery2VoltagePresence.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&g_Battery2VoltagePresence);

	//----------------------------------------------------------------------------------------------------------------------
	// Gauge Alert: Port 0, Pin 4, Input, External pullup, Active low, 1.8V, Interrupt
	//----------------------------------------------------------------------------------------------------------------------
	g_GaugeAlert.port = MXC_GPIO0;
	g_GaugeAlert.mask = MXC_GPIO_PIN_4;
	g_GaugeAlert.pad = MXC_GPIO_PAD_NONE;
	g_GaugeAlert.func = MXC_GPIO_FUNC_IN;
	g_GaugeAlert.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_RegisterCallback(&g_GaugeAlert, GaugeAlert_ISR, NULL);
    MXC_GPIO_IntConfig(&g_GaugeAlert, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_GaugeAlert.port, g_GaugeAlert.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Battery Charger IRQ: Port 0, Pin 5, Input, Needs strong internal pullup, Active low, 1.8V, Interrupt
	//----------------------------------------------------------------------------------------------------------------------
	g_BatteryChargerIRQ.port = MXC_GPIO0;
	g_BatteryChargerIRQ.mask = MXC_GPIO_PIN_5;
	g_BatteryChargerIRQ.pad = MXC_GPIO_PAD_STRONG_PULL_UP;
	g_BatteryChargerIRQ.func = MXC_GPIO_FUNC_IN;
	g_BatteryChargerIRQ.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_RegisterCallback(&g_BatteryChargerIRQ, BatteryCharger_ISR, NULL);
    MXC_GPIO_IntConfig(&g_BatteryChargerIRQ, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_BatteryChargerIRQ.port, g_BatteryChargerIRQ.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Enable 12V = Port 0, Pin 6, Output, External pulldown, Active high, 1.8V (minimum 1.5V)
	//----------------------------------------------------------------------------------------------------------------------
	g_Enable12V.port = MXC_GPIO0;
	g_Enable12V.mask = MXC_GPIO_PIN_6;
	g_Enable12V.pad = MXC_GPIO_PAD_NONE;
	g_Enable12V.func = MXC_GPIO_FUNC_OUT;
	g_Enable12V.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&g_Enable12V);
	MXC_GPIO_OutSet(g_Enable12V.port, g_Enable12V.mask); // Start enabled

	//----------------------------------------------------------------------------------------------------------------------
	// Enable 5V: Port 0, Pin 7, Output, External pulldown, Active high, 1.8V (minimum 0.9V)
	//----------------------------------------------------------------------------------------------------------------------
	g_Enable5V.port = MXC_GPIO0;
	g_Enable5V.mask = MXC_GPIO_PIN_7;
	g_Enable5V.pad = MXC_GPIO_PAD_NONE;
	g_Enable5V.func = MXC_GPIO_FUNC_OUT;
	g_Enable5V.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&g_Enable5V);
	MXC_GPIO_OutSet(g_Enable5V.port, g_Enable5V.mask); // Start enabled

	//----------------------------------------------------------------------------------------------------------------------
	// Enable 2.5V Ref: Port 0, Pin 8, Output, External pulldown, Active high, probably 3.3V (minimum 0.7 * Vin)
	//----------------------------------------------------------------------------------------------------------------------
	g_Enable2_5_VRef.port = MXC_GPIO0;
	g_Enable2_5_VRef.mask = MXC_GPIO_PIN_8;
	g_Enable2_5_VRef.pad = MXC_GPIO_PAD_NONE;
	g_Enable2_5_VRef.func = MXC_GPIO_FUNC_OUT;
	g_Enable2_5_VRef.vssel = MXC_GPIO_VSSEL_VDDIOH; // Trying 3.3
    MXC_GPIO_Config(&g_Enable2_5_VRef);
	MXC_GPIO_OutSet(g_Enable2_5_VRef.port, g_Enable2_5_VRef.mask); // Enabling to start for testing, may delay power later until monitoring setup

	//----------------------------------------------------------------------------------------------------------------------
	// USB Source Enable: Port 0, Pin 9, Output, External pulldown, Active high, 1.8V (minimum 1.2V)
	//----------------------------------------------------------------------------------------------------------------------
	g_USBSourceEnable.port = MXC_GPIO0;
	g_USBSourceEnable.mask = MXC_GPIO_PIN_9;
	g_USBSourceEnable.pad = MXC_GPIO_PAD_NONE;
	g_USBSourceEnable.func = MXC_GPIO_FUNC_OUT;
	g_USBSourceEnable.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&g_USBSourceEnable);
	MXC_GPIO_OutClr(g_USBSourceEnable.port, g_USBSourceEnable.mask); // Start disabled

	//----------------------------------------------------------------------------------------------------------------------
	// USB Aux Power Enable: Port 0, Pin 10, Output, External pulldown, Active high, 1.8V (minimum 0.6V)
	//----------------------------------------------------------------------------------------------------------------------
	g_USBAuxPowerEnable.port = MXC_GPIO0;
	g_USBAuxPowerEnable.mask = MXC_GPIO_PIN_10;
	g_USBAuxPowerEnable.pad = MXC_GPIO_PAD_NONE;
	g_USBAuxPowerEnable.func = MXC_GPIO_FUNC_OUT;
	g_USBAuxPowerEnable.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&g_USBAuxPowerEnable);
	MXC_GPIO_OutClr(g_USBAuxPowerEnable.port, g_USBAuxPowerEnable.mask); // Start disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Power Good 5v: Port 0, Pin 11, Input, External pullup, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_PowerGood5v.port = MXC_GPIO0;
	g_PowerGood5v.mask = MXC_GPIO_PIN_11;
	g_PowerGood5v.pad = MXC_GPIO_PAD_NONE;
	g_PowerGood5v.func = MXC_GPIO_FUNC_IN;
	g_PowerGood5v.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&g_PowerGood5v);

	//----------------------------------------------------------------------------------------------------------------------
	// Power Good Battery Charge: Port 0, Pin 12, Input, External pullup, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_PowerGoodBatteryCharge.port = MXC_GPIO0;
	g_PowerGoodBatteryCharge.mask = MXC_GPIO_PIN_12;
	g_PowerGoodBatteryCharge.pad = MXC_GPIO_PAD_NONE;
	g_PowerGoodBatteryCharge.func = MXC_GPIO_FUNC_IN;
	g_PowerGoodBatteryCharge.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&g_PowerGoodBatteryCharge);

	//----------------------------------------------------------------------------------------------------------------------
	// Smart Sensor Sleep: Port 0, Pin 13, Output, No external pull, Active low, 1.8V (minimum 1.3V)
	//----------------------------------------------------------------------------------------------------------------------
	g_SmartSensorSleep.port = MXC_GPIO0;
	g_SmartSensorSleep.mask = MXC_GPIO_PIN_13;
	g_SmartSensorSleep.pad = MXC_GPIO_PAD_NONE;
	g_SmartSensorSleep.func = MXC_GPIO_FUNC_OUT;
	g_SmartSensorSleep.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&g_SmartSensorSleep);
	MXC_GPIO_OutClr(g_SmartSensorSleep.port, g_SmartSensorSleep.mask); // Start in sleep

	//----------------------------------------------------------------------------------------------------------------------
	// Smart Sensor Mux Enable: Port 0, Pin 14, Output, External pulldown, Active high, 3.3V (minimum 2.0V)
	//----------------------------------------------------------------------------------------------------------------------
	g_SmartSensorMuxEnable.port = MXC_GPIO0;
	g_SmartSensorMuxEnable.mask = MXC_GPIO_PIN_14;
	g_SmartSensorMuxEnable.pad = MXC_GPIO_PAD_NONE;
	g_SmartSensorMuxEnable.func = MXC_GPIO_FUNC_OUT;
	g_SmartSensorMuxEnable.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_SmartSensorMuxEnable);
	MXC_GPIO_OutClr(g_SmartSensorMuxEnable.port, g_SmartSensorMuxEnable.mask); // Start disabled

	//----------------------------------------------------------------------------------------------------------------------
	// ADC Reset: Port 0, Pin 15, Output, External pulldown, Active low, 1.8V (minimuim 1.26V)
	//----------------------------------------------------------------------------------------------------------------------
	g_ADCReset.port = MXC_GPIO0;
	g_ADCReset.mask = MXC_GPIO_PIN_15;
	g_ADCReset.pad = MXC_GPIO_PAD_NONE;
	g_ADCReset.func = MXC_GPIO_FUNC_OUT;
	g_ADCReset.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&g_ADCReset);
	MXC_GPIO_OutClr(g_ADCReset.port, g_ADCReset.mask); // Start in reset

	//----------------------------------------------------------------------------------------------------------------------
	// ADC Conversion: Port 0, Pin 18, Output, External pulldown, Active high, 1.8V (minimuim 1.26V)
	//----------------------------------------------------------------------------------------------------------------------
	g_ADCConversion.port = MXC_GPIO0;
	g_ADCConversion.mask = MXC_GPIO_PIN_18;
	g_ADCConversion.pad = MXC_GPIO_PAD_NONE;
	g_ADCConversion.func = MXC_GPIO_FUNC_OUT;
	g_ADCConversion.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&g_ADCConversion);
	MXC_GPIO_OutClr(g_ADCConversion.port, g_ADCConversion.mask); // Start as no conversion

	//----------------------------------------------------------------------------------------------------------------------
	// Cal Mux Pre-A/D Enable: Port 0, Pin 22, Output, External pulldown, Active low, 3.3V (minimum 2V)
	//----------------------------------------------------------------------------------------------------------------------
	g_CalMuxPreADEnable.port = MXC_GPIO0;
	g_CalMuxPreADEnable.mask = MXC_GPIO_PIN_22;
	g_CalMuxPreADEnable.pad = MXC_GPIO_PAD_NONE;
	g_CalMuxPreADEnable.func = MXC_GPIO_FUNC_OUT;
	g_CalMuxPreADEnable.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_CalMuxPreADEnable);
	MXC_GPIO_OutSet(g_CalMuxPreADEnable.port, g_CalMuxPreADEnable.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Cal Mux Pre-A/D Select: Port 0, Pin 23, Output, External pulldown, Active high, 3.3V (minimum 2V)
	//----------------------------------------------------------------------------------------------------------------------
	g_CalMuxPreADSelect.port = MXC_GPIO0;
	g_CalMuxPreADSelect.mask = MXC_GPIO_PIN_23;
	g_CalMuxPreADSelect.pad = MXC_GPIO_PAD_NONE;
	g_CalMuxPreADSelect.func = MXC_GPIO_FUNC_OUT;
	g_CalMuxPreADSelect.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_CalMuxPreADSelect);
	MXC_GPIO_OutClr(g_CalMuxPreADSelect.port, g_CalMuxPreADSelect.mask); // Start as 0 (Full sensor group A/1)

	//----------------------------------------------------------------------------------------------------------------------
	// Alert 1: Port 0, Pin 24, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	g_Alert1.port = MXC_GPIO0;
	g_Alert1.mask = MXC_GPIO_PIN_24;
	g_Alert1.pad = MXC_GPIO_PAD_NONE;
	g_Alert1.func = MXC_GPIO_FUNC_OUT;
	g_Alert1.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&g_Alert1);
	MXC_GPIO_OutClr(g_Alert1.port, g_Alert1.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Alert 2: Port 0, Pin 25, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	g_Alert2.port = MXC_GPIO0;
	g_Alert2.mask = MXC_GPIO_PIN_25;
	g_Alert2.pad = MXC_GPIO_PAD_NONE;
	g_Alert2.func = MXC_GPIO_FUNC_OUT;
	g_Alert2.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&g_Alert2);
	MXC_GPIO_OutClr(g_Alert2.port, g_Alert2.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// LTE OTA: Port 0, Pin 30, Input, No external pull, Active unknown, 3.3V (device runs 3.3V)
	//----------------------------------------------------------------------------------------------------------------------
	g_LTEOTA.port = MXC_GPIO0;
	g_LTEOTA.mask = MXC_GPIO_PIN_30;
	g_LTEOTA.pad = MXC_GPIO_PAD_NONE;
	g_LTEOTA.func = MXC_GPIO_FUNC_IN;
	g_LTEOTA.vssel = MXC_GPIO_VSSEL_VDDIOH;

	//----------------------------------------------------------------------------------------------------------------------
	// Expansion Enable: Port 1, Pin 7, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	g_ExpansionEnable.port = MXC_GPIO1;
	g_ExpansionEnable.mask = MXC_GPIO_PIN_7;
	g_ExpansionEnable.pad = MXC_GPIO_PAD_NONE;
	g_ExpansionEnable.func = MXC_GPIO_FUNC_OUT;
	g_ExpansionEnable.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&g_ExpansionEnable);
	MXC_GPIO_OutClr(g_ExpansionEnable.port, g_ExpansionEnable.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Expansion Reset: Port 1, Pin 8, Output, External pulldown, Active low, 3.3V (minimum 2V)
	//----------------------------------------------------------------------------------------------------------------------
	g_ExpansionReset.port = MXC_GPIO1;
	g_ExpansionReset.mask = MXC_GPIO_PIN_8;
	g_ExpansionReset.pad = MXC_GPIO_PAD_NONE;
	g_ExpansionReset.func = MXC_GPIO_FUNC_OUT;
	g_ExpansionReset.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_ExpansionReset);
	MXC_GPIO_OutClr(g_ExpansionReset.port, g_ExpansionReset.mask); // Start in reset

	//----------------------------------------------------------------------------------------------------------------------
	// Expansion IRQ: Port 1, Pin 9, Input, External pullup, Active low, 3.3V (minimum 2V)
	//----------------------------------------------------------------------------------------------------------------------
	g_ExpansionIRQ.port = MXC_GPIO1;
	g_ExpansionIRQ.mask = MXC_GPIO_PIN_9;
	g_ExpansionIRQ.pad = MXC_GPIO_PAD_NONE;
	g_ExpansionIRQ.func = MXC_GPIO_FUNC_IN;
	g_ExpansionIRQ.vssel = MXC_GPIO_VSSEL_VDDIOH;
	MXC_GPIO_RegisterCallback(&g_ExpansionIRQ, Expansion_ISR, NULL);
    MXC_GPIO_IntConfig(&g_ExpansionIRQ, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_ExpansionIRQ.port, g_ExpansionIRQ.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// USBC I2C IRQ: Port 1, Pin 11, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_USBCI2CIRQ.port = MXC_GPIO1;
	g_USBCI2CIRQ.mask = MXC_GPIO_PIN_11;
	g_USBCI2CIRQ.pad = MXC_GPIO_PAD_NONE;
	g_USBCI2CIRQ.func = MXC_GPIO_FUNC_IN;
	g_USBCI2CIRQ.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_RegisterCallback(&g_USBCI2CIRQ, USBCI2C_ISR, NULL);
    MXC_GPIO_IntConfig(&g_USBCI2CIRQ, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_USBCI2CIRQ.port, g_USBCI2CIRQ.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Accel Int 1: Port 1, Pin 12, Input, No external pull, Active high, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_AccelInt1.port = MXC_GPIO1;
	g_AccelInt1.mask = MXC_GPIO_PIN_12;
	g_AccelInt1.pad = MXC_GPIO_PAD_NONE;
	g_AccelInt1.func = MXC_GPIO_FUNC_IN;
	g_AccelInt1.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_RegisterCallback(&g_AccelInt1, AccelInt1_ISR, NULL);
    MXC_GPIO_IntConfig(&g_AccelInt1, MXC_GPIO_INT_RISING);
    MXC_GPIO_EnableInt(g_AccelInt1.port, g_AccelInt1.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Accel Int 2: Port 1, Pin 13, Input, No external pull, Active high, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_AccelInt2.port = MXC_GPIO1;
	g_AccelInt2.mask = MXC_GPIO_PIN_13;
	g_AccelInt2.pad = MXC_GPIO_PAD_NONE;
	g_AccelInt2.func = MXC_GPIO_FUNC_IN;
	g_AccelInt2.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_RegisterCallback(&g_AccelInt2, AccelInt2_ISR, NULL);
    MXC_GPIO_IntConfig(&g_AccelInt2, MXC_GPIO_INT_RISING);
    MXC_GPIO_EnableInt(g_AccelInt2.port, g_AccelInt2.mask);
	NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(MXC_GPIO1)));

	//----------------------------------------------------------------------------------------------------------------------
	// Accel Trig: Port 1, Pin 14, Output, No external pull, Active high, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_AccelTrig.port = MXC_GPIO1;
	g_AccelTrig.mask = MXC_GPIO_PIN_14;
	g_AccelTrig.pad = MXC_GPIO_PAD_NONE;
	g_AccelTrig.func = MXC_GPIO_FUNC_OUT;
	g_AccelTrig.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&g_AccelTrig);
	MXC_GPIO_OutClr(g_AccelTrig.port, g_AccelTrig.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Power Button Int: Port 1, Pin 15, Input, External pullup, Active high, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_PowerButtonIRQ.port = MXC_GPIO1;
	g_PowerButtonIRQ.mask = MXC_GPIO_PIN_15;
	g_PowerButtonIRQ.pad = MXC_GPIO_PAD_NONE;
	g_PowerButtonIRQ.func = MXC_GPIO_FUNC_IN;
	g_PowerButtonIRQ.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_RegisterCallback(&g_PowerButtonIRQ, PowerButton_ISR, NULL);
    MXC_GPIO_IntConfig(&g_PowerButtonIRQ, MXC_GPIO_INT_RISING);
    MXC_GPIO_EnableInt(g_PowerButtonIRQ.port, g_PowerButtonIRQ.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Button 1: Port 1, Pin 16, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_Button1.port = MXC_GPIO1;
	g_Button1.mask = MXC_GPIO_PIN_16;
	g_Button1.pad = MXC_GPIO_PAD_NONE;
	g_Button1.func = MXC_GPIO_FUNC_IN;
	g_Button1.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_RegisterCallback(&g_Button1, Button1_ISR, NULL);
    MXC_GPIO_IntConfig(&g_Button1, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_Button1.port, g_Button1.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Button 2: Port 1, Pin 17, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_Button2.port = MXC_GPIO1;
	g_Button2.mask = MXC_GPIO_PIN_17;
	g_Button2.pad = MXC_GPIO_PAD_NONE;
	g_Button2.func = MXC_GPIO_FUNC_IN;
	g_Button2.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_RegisterCallback(&g_Button2, Button2_ISR, NULL);
    MXC_GPIO_IntConfig(&g_Button2, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_Button2.port, g_Button2.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Button 3: Port 1, Pin 18, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_Button3.port = MXC_GPIO1;
	g_Button3.mask = MXC_GPIO_PIN_18;
	g_Button3.pad = MXC_GPIO_PAD_NONE;
	g_Button3.func = MXC_GPIO_FUNC_IN;
	g_Button3.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_RegisterCallback(&g_Button3, Button3_ISR, NULL);
    MXC_GPIO_IntConfig(&g_Button3, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_Button3.port, g_Button3.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Button 4: Port 1, Pin 19, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_Button4.port = MXC_GPIO1;
	g_Button4.mask = MXC_GPIO_PIN_19;
	g_Button4.pad = MXC_GPIO_PAD_NONE;
	g_Button4.func = MXC_GPIO_FUNC_IN;
	g_Button4.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_RegisterCallback(&g_Button4, Button4_ISR, NULL);
    MXC_GPIO_IntConfig(&g_Button4, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_Button4.port, g_Button4.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Button 5: Port 1, Pin 20, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_Button5.port = MXC_GPIO1;
	g_Button5.mask = MXC_GPIO_PIN_20;
	g_Button5.pad = MXC_GPIO_PAD_NONE;
	g_Button5.func = MXC_GPIO_FUNC_IN;
	g_Button5.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_RegisterCallback(&g_Button5, Button5_ISR, NULL);
    MXC_GPIO_IntConfig(&g_Button5, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_Button5.port, g_Button5.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Button 6: Port 1, Pin 21, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_Button6.port = MXC_GPIO1;
	g_Button6.mask = MXC_GPIO_PIN_21;
	g_Button6.pad = MXC_GPIO_PAD_NONE;
	g_Button6.func = MXC_GPIO_FUNC_IN;
	g_Button6.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_RegisterCallback(&g_Button6, Button6_ISR, NULL);
    MXC_GPIO_IntConfig(&g_Button6, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_Button6.port, g_Button6.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Button 7: Port 1, Pin 22, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_Button7.port = MXC_GPIO1;
	g_Button7.mask = MXC_GPIO_PIN_22;
	g_Button7.pad = MXC_GPIO_PAD_NONE;
	g_Button7.func = MXC_GPIO_FUNC_IN;
	g_Button7.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_RegisterCallback(&g_Button7, Button7_ISR, NULL);
    MXC_GPIO_IntConfig(&g_Button7, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_Button7.port, g_Button7.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Button 8: Port 1, Pin 23, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_Button8.port = MXC_GPIO1;
	g_Button8.mask = MXC_GPIO_PIN_23;
	g_Button8.pad = MXC_GPIO_PAD_NONE;
	g_Button8.func = MXC_GPIO_FUNC_IN;
	g_Button8.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_RegisterCallback(&g_Button8, Button8_ISR, NULL);
    MXC_GPIO_IntConfig(&g_Button8, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_Button8.port, g_Button8.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Button 9: Port 1, Pin 24, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_Button9.port = MXC_GPIO1;
	g_Button9.mask = MXC_GPIO_PIN_24;
	g_Button9.pad = MXC_GPIO_PAD_NONE;
	g_Button9.func = MXC_GPIO_FUNC_IN;
	g_Button9.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_RegisterCallback(&g_Button9, Button9_ISR, NULL);
    MXC_GPIO_IntConfig(&g_Button9, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_Button9.port, g_Button9.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// LED 1: Port 1, Pin 25, Output, No external pull, Active high, 3.3V
	//----------------------------------------------------------------------------------------------------------------------
	g_LED1.port = MXC_GPIO1;
	g_LED1.mask = MXC_GPIO_PIN_25;
	g_LED1.pad = MXC_GPIO_PAD_NONE;
	g_LED1.func = MXC_GPIO_FUNC_OUT;
	g_LED1.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_LED1);
	MXC_GPIO_OutClr(g_LED1.port, g_LED1.mask); // Start as off

	//----------------------------------------------------------------------------------------------------------------------
	// LED 2: Port 1, Pin 26, Output, No external pull, Active high, 3.3V
	//----------------------------------------------------------------------------------------------------------------------
	g_LED2.port = MXC_GPIO1;
	g_LED2.mask = MXC_GPIO_PIN_26;
	g_LED2.pad = MXC_GPIO_PAD_NONE;
	g_LED2.func = MXC_GPIO_FUNC_OUT;
	g_LED2.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_LED2);
	MXC_GPIO_OutClr(g_LED2.port, g_LED2.mask); // Start as off

	//----------------------------------------------------------------------------------------------------------------------
	// LED 3: Port 1, Pin 27, Output, No external pull, Active high, 3.3V
	//----------------------------------------------------------------------------------------------------------------------
	g_LED3.port = MXC_GPIO1;
	g_LED3.mask = MXC_GPIO_PIN_27;
	g_LED3.pad = MXC_GPIO_PAD_NONE;
	g_LED3.func = MXC_GPIO_FUNC_OUT;
	g_LED3.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_LED3);
	MXC_GPIO_OutClr(g_LED3.port, g_LED3.mask); // Start as off

	//----------------------------------------------------------------------------------------------------------------------
	// RTC Int A: Port 1, Pin 28, Input, External pullup, Active low, 1.8V (minimum 0.66V)
	//----------------------------------------------------------------------------------------------------------------------
	g_ExtRTCIntA.port = MXC_GPIO1;
	g_ExtRTCIntA.mask = MXC_GPIO_PIN_28;
	g_ExtRTCIntA.pad = MXC_GPIO_PAD_NONE;
	g_ExtRTCIntA.func = MXC_GPIO_FUNC_IN;
	g_ExtRTCIntA.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_RegisterCallback(&g_ExtRTCIntA, ExtRTCIntA_ISR, NULL);
    MXC_GPIO_IntConfig(&g_ExtRTCIntA, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_ExtRTCIntA.port, g_ExtRTCIntA.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// BLE OTA: Port 1, Pin 29, Input, No external pull, Active unknown, 3.3V (device runs 3.3V)
	//----------------------------------------------------------------------------------------------------------------------
	g_BLEOTA.port = MXC_GPIO1;
	g_BLEOTA.mask = MXC_GPIO_PIN_29;
	g_BLEOTA.pad = MXC_GPIO_PAD_NONE;
	g_BLEOTA.func = MXC_GPIO_FUNC_IN;
	g_BLEOTA.vssel = MXC_GPIO_VSSEL_VDDIOH;

	//----------------------------------------------------------------------------------------------------------------------
	// Trig Out: Port 1, Pin 30, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	g_ExternalTriggerOut.port = MXC_GPIO1;
	g_ExternalTriggerOut.mask = MXC_GPIO_PIN_30;
	g_ExternalTriggerOut.pad = MXC_GPIO_PAD_NONE;
	g_ExternalTriggerOut.func = MXC_GPIO_FUNC_OUT;
	g_ExternalTriggerOut.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&g_ExternalTriggerOut);
	MXC_GPIO_OutClr(g_ExternalTriggerOut.port, g_ExternalTriggerOut.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Trig In: Port 1, Pin 31, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_ExternalTriggerIn.port = MXC_GPIO1;
	g_ExternalTriggerIn.mask = MXC_GPIO_PIN_31;
	g_ExternalTriggerIn.pad = MXC_GPIO_PAD_NONE;
	g_ExternalTriggerIn.func = MXC_GPIO_FUNC_IN;
	g_ExternalTriggerIn.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
	MXC_GPIO_RegisterCallback(&g_ExternalTriggerIn, ExternalTriggerIn_ISR, NULL);
    MXC_GPIO_IntConfig(&g_ExternalTriggerIn, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_ExternalTriggerIn.port, g_ExternalTriggerIn.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// LCD Power Enable: Port 2, Pin 0, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	g_LCDPowerEnable.port = MXC_GPIO2;
	g_LCDPowerEnable.mask = MXC_GPIO_PIN_0;
	g_LCDPowerEnable.pad = MXC_GPIO_PAD_NONE;
	g_LCDPowerEnable.func = MXC_GPIO_FUNC_OUT;
	g_LCDPowerEnable.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&g_LCDPowerEnable);
	MXC_GPIO_OutClr(g_LCDPowerEnable.port, g_LCDPowerEnable.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// LCD Power Display: Port 2, Pin 1, Output, External pulldown, Active low, 1.8V (minimum 1.7V)
	//----------------------------------------------------------------------------------------------------------------------
	g_LCDPowerDisplay.port = MXC_GPIO2;
	g_LCDPowerDisplay.mask = MXC_GPIO_PIN_1;
	g_LCDPowerDisplay.pad = MXC_GPIO_PAD_NONE;
	g_LCDPowerDisplay.func = MXC_GPIO_FUNC_OUT;
	g_LCDPowerDisplay.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&g_LCDPowerDisplay);
	MXC_GPIO_OutSet(g_LCDPowerDisplay.port, g_LCDPowerDisplay.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// LCD Int: Port 2, Pin 6, Input, External pullup, Active low, 1.8V
	//----------------------------------------------------------------------------------------------------------------------
	g_LCDInt.port = MXC_GPIO2;
	g_LCDInt.mask = MXC_GPIO_PIN_6;
	g_LCDInt.pad = MXC_GPIO_PAD_NONE;
	g_LCDInt.func = MXC_GPIO_FUNC_IN;
	g_LCDInt.vssel = MXC_GPIO_VSSEL_VDDIO;
	MXC_GPIO_RegisterCallback(&g_LCDInt, LCD_ISR, NULL);
    MXC_GPIO_IntConfig(&g_LCDInt, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_LCDInt.port, g_LCDInt.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Sensor Check Enable: Port 2, Pin 9, Output, External pulldown, Active high, 1.8V (minimum 0.65 * Vin)
	//----------------------------------------------------------------------------------------------------------------------
	g_SensorCheckEnable.port = MXC_GPIO2;
	g_SensorCheckEnable.mask = MXC_GPIO_PIN_9;
	g_SensorCheckEnable.pad = MXC_GPIO_PAD_NONE;
	g_SensorCheckEnable.func = MXC_GPIO_FUNC_OUT;
	g_SensorCheckEnable.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&g_SensorCheckEnable);
	MXC_GPIO_OutClr(g_SensorCheckEnable.port, g_SensorCheckEnable.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Sensor Check: Port 2, Pin 10, Output, No external pull, Active high, 1.8V (must match Sensor Check enable)
	//----------------------------------------------------------------------------------------------------------------------
	g_SensorCheck.port = MXC_GPIO2;
	g_SensorCheck.mask = MXC_GPIO_PIN_10;
	g_SensorCheck.pad = MXC_GPIO_PAD_NONE; // Consider weak pulldown?
	g_SensorCheck.func = MXC_GPIO_FUNC_OUT;
	g_SensorCheck.vssel = MXC_GPIO_VSSEL_VDDIO;
    MXC_GPIO_Config(&g_SensorCheck);
	MXC_GPIO_OutClr(g_SensorCheck.port, g_SensorCheck.mask); // Start as disabled


	//----------------------------------------------------------------------------------------------------------------------
	// LTE Reset: Port 2, Pin 13, Input, External pull up, Active low, 3.3V
	//----------------------------------------------------------------------------------------------------------------------
	g_LTEReset.port = MXC_GPIO2;
	g_LTEReset.mask = MXC_GPIO_PIN_13;
	g_LTEReset.pad = MXC_GPIO_PAD_NONE;
	g_LTEReset.func = MXC_GPIO_FUNC_IN;
	g_LTEReset.vssel = MXC_GPIO_VSSEL_VDDIOH;

	//----------------------------------------------------------------------------------------------------------------------
	// BLE Reset: Port 2, 15, Input, External pull up, Active low, 3.3V
	//----------------------------------------------------------------------------------------------------------------------
	g_BLEReset.port = MXC_GPIO2;
	g_BLEReset.mask = MXC_GPIO_PIN_15;
	g_BLEReset.pad = MXC_GPIO_PAD_NONE;
	g_BLEReset.func = MXC_GPIO_FUNC_IN;
	g_BLEReset.vssel = MXC_GPIO_VSSEL_VDDIOH;

	//----------------------------------------------------------------------------------------------------------------------
	// Smart Sensor Mux A0: Port 2, Pin 23, Output, External pulldown, Active high, 3.3V (minimum 2.0V)
	//----------------------------------------------------------------------------------------------------------------------
	g_SmartSensorMux_A0.port = MXC_GPIO2;
	g_SmartSensorMux_A0.mask = MXC_GPIO_PIN_23;
	g_SmartSensorMux_A0.pad = MXC_GPIO_PAD_NONE;
	g_SmartSensorMux_A0.func = MXC_GPIO_FUNC_OUT;
	g_SmartSensorMux_A0.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_SmartSensorMux_A0);
	MXC_GPIO_OutClr(g_SmartSensorMux_A0.port, g_SmartSensorMux_A0.mask); // Start as low

	//----------------------------------------------------------------------------------------------------------------------
	// Smart Sensor Mux A1: Port 2, Pin 25, Output, External pulldown, Active high, 3.3V (minimum 2.0V)
	//----------------------------------------------------------------------------------------------------------------------
	g_SmartSensorMux_A1.port = MXC_GPIO2;
	g_SmartSensorMux_A1.mask = MXC_GPIO_PIN_25;
	g_SmartSensorMux_A1.pad = MXC_GPIO_PAD_NONE;
	g_SmartSensorMux_A1.func = MXC_GPIO_FUNC_OUT;
	g_SmartSensorMux_A1.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_SmartSensorMux_A1);
	MXC_GPIO_OutClr(g_SmartSensorMux_A1.port, g_SmartSensorMux_A1.mask); // Start as low

	//----------------------------------------------------------------------------------------------------------------------
	// Nyquist 0/A0:Port 2, Pin 26, Output, External pulldown, Active high, 3.3V (minimum 2.4V)
	//----------------------------------------------------------------------------------------------------------------------
	g_Nyquist0_A0.port = MXC_GPIO2;
	g_Nyquist0_A0.mask = MXC_GPIO_PIN_26;
	g_Nyquist0_A0.pad = MXC_GPIO_PAD_NONE;
	g_Nyquist0_A0.func = MXC_GPIO_FUNC_OUT;
	g_Nyquist0_A0.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_Nyquist0_A0);
	MXC_GPIO_OutClr(g_Nyquist0_A0.port, g_Nyquist0_A0.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Nyquist 1/A1: Port 2, Pin 28, Output, External pulldown, Active high, 3.3V (minimum 2.4V))
	//----------------------------------------------------------------------------------------------------------------------
	g_Nyquist1_A1.port = MXC_GPIO2;
	g_Nyquist1_A1.mask = MXC_GPIO_PIN_28;
	g_Nyquist1_A1.pad = MXC_GPIO_PAD_NONE;
	g_Nyquist1_A1.func = MXC_GPIO_FUNC_OUT;
	g_Nyquist1_A1.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_Nyquist1_A1);
	MXC_GPIO_OutClr(g_Nyquist1_A1.port, g_Nyquist1_A1.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Nyquist 2/Enable: Port 2, Pin 30, Output, External pulldown, Active low, 3.3V (minimum 2.4V))
	//----------------------------------------------------------------------------------------------------------------------
	g_Nyquist2_Enable.port = MXC_GPIO2;
	g_Nyquist2_Enable.mask = MXC_GPIO_PIN_30;
	g_Nyquist2_Enable.pad = MXC_GPIO_PAD_NONE;
	g_Nyquist2_Enable.func = MXC_GPIO_FUNC_OUT;
	g_Nyquist2_Enable.vssel = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&g_Nyquist2_Enable);
	MXC_GPIO_OutClr(g_Nyquist2_Enable.port, g_Nyquist2_Enable.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Sensor Enable 1(Geo1): Port 3, Pin 1, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	g_SensorEnable1_Geo1.port = MXC_GPIO3;
	g_SensorEnable1_Geo1.mask = MXC_GPIO_PIN_1;
	g_SensorEnable1_Geo1.pad = MXC_GPIO_PAD_NONE;
	g_SensorEnable1_Geo1.func = MXC_GPIO_FUNC_OUT;
	g_SensorEnable1_Geo1.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&g_SensorEnable1_Geo1);
	MXC_GPIO_OutClr(g_SensorEnable1_Geo1.port, g_SensorEnable1_Geo1.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Sensor Enable 2(Aop1): Port 3, Pin 2, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	g_SensorEnable2_Aop1.port = MXC_GPIO3;
	g_SensorEnable2_Aop1.mask = MXC_GPIO_PIN_2;
	g_SensorEnable2_Aop1.pad = MXC_GPIO_PAD_NONE;
	g_SensorEnable2_Aop1.func = MXC_GPIO_FUNC_OUT;
	g_SensorEnable2_Aop1.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&g_SensorEnable2_Aop1);
	MXC_GPIO_OutClr(g_SensorEnable2_Aop1.port, g_SensorEnable2_Aop1.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Sensor Enable 3(Geo2): Port 3, Pin 3, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	g_SensorEnable3_Geo2.port = MXC_GPIO3;
	g_SensorEnable3_Geo2.mask = MXC_GPIO_PIN_3;
	g_SensorEnable3_Geo2.pad = MXC_GPIO_PAD_NONE;
	g_SensorEnable3_Geo2.func = MXC_GPIO_FUNC_OUT;
	g_SensorEnable3_Geo2.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&g_SensorEnable3_Geo2);
	MXC_GPIO_OutClr(g_SensorEnable3_Geo2.port, g_SensorEnable3_Geo2.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Sensor Enable 4(Aop2): Port 3, Pin 4, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	g_SensorEnable4_Aop2.port = MXC_GPIO3;
	g_SensorEnable4_Aop2.mask = MXC_GPIO_PIN_4;
	g_SensorEnable4_Aop2.pad = MXC_GPIO_PAD_NONE;
	g_SensorEnable4_Aop2.func = MXC_GPIO_FUNC_OUT;
	g_SensorEnable4_Aop2.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&g_SensorEnable4_Aop2);
	MXC_GPIO_OutClr(g_SensorEnable4_Aop2.port, g_SensorEnable4_Aop2.mask); // Start as disabled

	//----------------------------------------------------------------------------------------------------------------------
	// Gain/Path Select 1(Geo1): Port 3, Pin 5, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	g_GainPathSelect1_Geo1.port = MXC_GPIO3;
	g_GainPathSelect1_Geo1.mask = MXC_GPIO_PIN_5;
	g_GainPathSelect1_Geo1.pad = MXC_GPIO_PAD_NONE;
	g_GainPathSelect1_Geo1.func = MXC_GPIO_FUNC_OUT;
	g_GainPathSelect1_Geo1.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&g_GainPathSelect1_Geo1);
	MXC_GPIO_OutClr(g_GainPathSelect1_Geo1.port, g_GainPathSelect1_Geo1.mask); // Start as low

	//----------------------------------------------------------------------------------------------------------------------
	// Gain/Path Select 2(Aop1): Port 3, Pin 6, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	g_GainPathSelect2_Aop1.port = MXC_GPIO3;
	g_GainPathSelect2_Aop1.mask = MXC_GPIO_PIN_6;
	g_GainPathSelect2_Aop1.pad = MXC_GPIO_PAD_NONE;
	g_GainPathSelect2_Aop1.func = MXC_GPIO_FUNC_OUT;
	g_GainPathSelect2_Aop1.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&g_GainPathSelect2_Aop1);
	MXC_GPIO_OutClr(g_GainPathSelect2_Aop1.port, g_GainPathSelect2_Aop1.mask); // Start as low

	//----------------------------------------------------------------------------------------------------------------------
	// Gain/Path Select 3(Geo2): Port 3, Pin 7,Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	g_GainPathSelect3_Geo2.port = MXC_GPIO3;
	g_GainPathSelect3_Geo2.mask = MXC_GPIO_PIN_7;
	g_GainPathSelect3_Geo2.pad = MXC_GPIO_PAD_NONE;
	g_GainPathSelect3_Geo2.func = MXC_GPIO_FUNC_OUT;
	g_GainPathSelect3_Geo2.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&g_GainPathSelect3_Geo2);
	MXC_GPIO_OutClr(g_GainPathSelect3_Geo2.port, g_GainPathSelect3_Geo2.mask); // Start as low

	//----------------------------------------------------------------------------------------------------------------------
	// Gain/Path Select 4(Aop2): Port 3, Pin 8, Output, External pulldown, Active high, 1.8V (minimum 0.5V)
	//----------------------------------------------------------------------------------------------------------------------
	g_GainPathSelect4_Aop2.port = MXC_GPIO3;
	g_GainPathSelect4_Aop2.mask = MXC_GPIO_PIN_8;
	g_GainPathSelect4_Aop2.pad = MXC_GPIO_PAD_NONE;
	g_GainPathSelect4_Aop2.func = MXC_GPIO_FUNC_OUT;
	g_GainPathSelect4_Aop2.vssel = MXC_GPIO_VSSEL_VDDIOH; // Schematic suggests 3.3V
    MXC_GPIO_Config(&g_GainPathSelect4_Aop2);
	MXC_GPIO_OutClr(g_GainPathSelect4_Aop2.port, g_GainPathSelect4_Aop2.mask); // Start as low

	//----------------------------------------------------------------------------------------------------------------------
	// RTC Clock: Port 3, Pin 9, Input, No external pull, Active high, 3.3V (minimum 2.64V)
	//----------------------------------------------------------------------------------------------------------------------
	g_RTCClock.port = MXC_GPIO3;
	g_RTCClock.mask = MXC_GPIO_PIN_9;
	g_RTCClock.pad = MXC_GPIO_PAD_NONE; // Consider a weak internal pull?
	g_RTCClock.func = MXC_GPIO_FUNC_IN;
	g_RTCClock.vssel = MXC_GPIO_VSSEL_VDDIOH;
	MXC_GPIO_RegisterCallback(&g_RTCClock, RTCClock_ISR, NULL);
    MXC_GPIO_IntConfig(&g_RTCClock, MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(g_RTCClock.port, g_RTCClock.mask);

	//----------------------------------------------------------------------------------------------------------------------
	// Enable IRQ's for any of the appropritate GPIO input interrupts
	//----------------------------------------------------------------------------------------------------------------------
	NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(MXC_GPIO0)));
	NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(MXC_GPIO1)));
	NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(MXC_GPIO2)));
	NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(MXC_GPIO3)));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UART0_Read_Callback(mxc_uart_req_t *req, int error)
{
    // UART0 receive processing
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UART1_Read_Callback(mxc_uart_req_t *req, int error)
{
    // UART1 receive processing
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UART2_Read_Callback(mxc_uart_req_t *req, int error)
{
    // UART2 receive processing
}

#define UART_BUFFER_SIZE 512
uint8_t g_Uart0_RxBuffer[UART_BUFFER_SIZE];
uint8_t g_Uart0_TxBuffer[UART_BUFFER_SIZE];
uint8_t g_Uart1_RxBuffer[UART_BUFFER_SIZE];
uint8_t g_Uart1_TxBuffer[UART_BUFFER_SIZE];
uint8_t g_Uart2_RxBuffer[UART_BUFFER_SIZE];
uint8_t g_Uart2_TxBuffer[UART_BUFFER_SIZE];

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UART0_Handler(void)
{
    MXC_UART_AsyncHandler(MXC_UART0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UART1_Handler(void)
{
    MXC_UART_AsyncHandler(MXC_UART1);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UART2_Handler(void)
{
    MXC_UART_AsyncHandler(MXC_UART2);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupUART(void)
{
	int status;

    status = MXC_UART_Init(MXC_UART0, UART_BAUD);
    if (status != E_SUCCESS) { printf("Error! UART0 failed init with code: %d\n", status); }

    // Move to Interrupt init
	NVIC_ClearPendingIRQ(UART0_IRQn);
    NVIC_DisableIRQ(UART0_IRQn);
    MXC_NVIC_SetVector(UART0_IRQn, UART0_Handler);
    NVIC_EnableIRQ(UART0_IRQn);

    status = MXC_UART_Init(MXC_UART1, UART_BAUD);
    if (status != E_SUCCESS) { printf("Error! UART1 failed init with code: %d\n", status); }

    // Move to Interrupt init
    NVIC_ClearPendingIRQ(UART1_IRQn);
    NVIC_DisableIRQ(UART1_IRQn);
    MXC_NVIC_SetVector(UART1_IRQn, UART1_Handler);
    NVIC_EnableIRQ(UART1_IRQn);

    // Setup the asynchronous request
    mxc_uart_req_t uart0ReadRequest;
    uart0ReadRequest.uart = MXC_UART0;
    uart0ReadRequest.rxData = g_Uart0_RxBuffer;
    uart0ReadRequest.rxLen = UART_BUFFER_SIZE;
    uart0ReadRequest.txLen = 0;
    uart0ReadRequest.callback = UART0_Read_Callback;

    // Setup the asynchronous request
    mxc_uart_req_t uart1ReadRequest;
    uart1ReadRequest.uart = MXC_UART1;
    uart1ReadRequest.rxData = g_Uart1_RxBuffer;
    uart1ReadRequest.rxLen = UART_BUFFER_SIZE;
    uart1ReadRequest.txLen = 0;
    uart1ReadRequest.callback = UART1_Read_Callback;

    status = MXC_UART_TransactionAsync(&uart0ReadRequest);
    if (status != E_SUCCESS) { printf("Error! Uart0 Read setup (async) failed with code: %d\n", status); }

    status = MXC_UART_TransactionAsync(&uart1ReadRequest);
    if (status != E_SUCCESS) { printf("Error! Uart1 Read setup (async) failed with code: %d\n", status); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupDebugUART(void)
{
	int status;

    status = MXC_UART_Init(MXC_UART2, UART_BAUD);
    if (status != E_SUCCESS) { printf("<Error> Debug UART2 failed init with code: %d\n", status); }

    // Move to Interrupt init
	NVIC_ClearPendingIRQ(UART2_IRQn);
    NVIC_DisableIRQ(UART2_IRQn);
    MXC_NVIC_SetVector(UART2_IRQn, UART2_Handler);
    NVIC_EnableIRQ(UART2_IRQn);

    // Setup the asynchronous request
    mxc_uart_req_t uart2ReadRequest;
    uart2ReadRequest.uart = MXC_UART0;
    uart2ReadRequest.rxData = g_Uart2_RxBuffer;
    uart2ReadRequest.rxLen = UART_BUFFER_SIZE;
    uart2ReadRequest.txLen = 0;
    uart2ReadRequest.callback = UART2_Read_Callback;

    status = MXC_UART_TransactionAsync(&uart2ReadRequest);
    if (status != E_NO_ERROR) { printf("<Error> Debug Uart2 Read setup (async) failed with code: %d\n", status); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
// <uart driver option> int MXC_UART_RevA_Write(mxc_uart_reva_regs_t *uart, uint8_t *byte, int *len)
void UART0_Write_Blocking(uint8_t* data, uint32_t size)
{
	int error;

    mxc_uart_req_t uart0WriteRequest;
    uart0WriteRequest.uart = MXC_UART0;
    uart0WriteRequest.txData = data;
    uart0WriteRequest.txLen = size;
    uart0WriteRequest.rxLen = 0;
    uart0WriteRequest.callback = NULL;

    error = MXC_UART_Transaction(&uart0WriteRequest);
    if (error != E_NO_ERROR) { printf("Error! Uart0 write failed with code: %d\n", error); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
// <uart driver option> int MXC_UART_RevA_Write(mxc_uart_reva_regs_t *uart, uint8_t *byte, int *len)
void UART1_Write_Blocking(uint8_t* data, uint32_t size)
{
	int error;

    mxc_uart_req_t uart1WriteRequest;
    uart1WriteRequest.uart = MXC_UART1;
    uart1WriteRequest.txData = data;
    uart1WriteRequest.txLen = size;
    uart1WriteRequest.rxLen = 0;
    uart1WriteRequest.callback = NULL;

    error = MXC_UART_Transaction(&uart1WriteRequest);
    if (error != E_NO_ERROR) { printf("Error! Uart0 write failed with code: %d\n", error); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UART0_Write_Async_ISR(uint8_t* data, uint32_t size)
{
	int error;

	memcpy(g_Uart0_TxBuffer, data, (size < UART_BUFFER_SIZE) ? size : UART_BUFFER_SIZE);

    mxc_uart_req_t uart0WriteRequest;
    uart0WriteRequest.uart = MXC_UART0;
    uart0WriteRequest.txData = g_Uart0_TxBuffer;
    uart0WriteRequest.txLen = size;
    uart0WriteRequest.rxLen = 0;
    uart0WriteRequest.callback = NULL;

    error = MXC_UART_TransactionAsync(&uart0WriteRequest);
    if (error != E_NO_ERROR) { printf("Error! Uart0 write setup (async) failed with code: %d\n", error); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UART1_Write_Async_ISR(uint8_t* data, uint32_t size)
{
	int error;

	memcpy(g_Uart1_TxBuffer, data, (size < UART_BUFFER_SIZE) ? size : UART_BUFFER_SIZE);

    mxc_uart_req_t uart1WriteRequest;
    uart1WriteRequest.uart = MXC_UART1;
    uart1WriteRequest.txData = g_Uart1_TxBuffer;
    uart1WriteRequest.txLen = size;
    uart1WriteRequest.rxLen = 0;
    uart1WriteRequest.callback = NULL;

    error = MXC_UART_TransactionAsync(&uart1WriteRequest);
    if (error != E_NO_ERROR) { printf("Error! Uart1 write setup (async) failed with code: %d\n", error); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void WriteI2CDevice(mxc_i2c_regs_t* i2cChannel, uint8_t slaveAddr, uint8_t* writeData, uint32_t writeSize, uint8_t* readData, uint32_t readSize)
{
	int status;

    mxc_i2c_req_t masterRequest;
    masterRequest.i2c = i2cChannel;
    masterRequest.addr = slaveAddr;
    masterRequest.tx_buf = writeData;
    masterRequest.tx_len = writeSize;
    masterRequest.rx_buf = readData;
    masterRequest.rx_len = readSize;
    masterRequest.restart = 0;
    masterRequest.callback = NULL;

    status = MXC_I2C_MasterTransaction(&masterRequest);
	if (status != E_SUCCESS) { printf("Error! I2C%d Master transaction to Slave (%02x) failed with code: %d\n", ((i2cChannel == MXC_I2C0) ? 0 : 1), slaveAddr, status); }
}

#if 0 /* Used if setting up the MXC I2C as a slave */
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void I2C0_IRQHandler(void)
{
    MXC_I2C_AsyncHandler(MXC_I2C0);
    return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void I2C1_IRQHandler(void)
{
    MXC_I2C_AsyncHandler(MXC_I2C1);
    return;
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupI2C(void)
{
	int error;

    // Setup I2C0 as Master (1.8V) 
    error = MXC_I2C_Init(MXC_I2C0, 1, 0);
    if (error != E_NO_ERROR) { printf("Error! I2C0 init (master) failed to initialize with code: %d\n", error); }

    // Setup I2C1 as Master (3.3V) 
    error = MXC_I2C_Init(MXC_I2C1, 1, 0);
    if (error != E_NO_ERROR) { printf("Error! I2C1 init (master) failed to initialize with code: %d\n", error); }

#if 0 /* Needed setting up the MXC I2C as a slave */
    MXC_NVIC_SetVector(I2C0_IRQn, I2C0_IRQHandler);
    NVIC_EnableIRQ(I2C0_IRQn);
    MXC_NVIC_SetVector(I2C1_IRQn, I2C1_IRQHandler);
    NVIC_EnableIRQ(I2C1_IRQn);
#endif

	// Set I2C speed, either Standard (MXC_I2C_STD_MODE = 100000) or Fast (MXC_I2C_FAST_SPEED = 400000)
    MXC_I2C_SetFrequency(MXC_I2C0, MXC_I2C_FAST_SPEED);
    MXC_I2C_SetFrequency(MXC_I2C1, MXC_I2C_FAST_SPEED);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void WDT0_IRQHandler(void)
{
    MXC_WDT_ClearIntFlag(MXC_WDT0);

    printf("(Error) Watchdog ISR triggered, attempting to gracefully close shop before reset...\n");

	// Shutdown/data handling before reset
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupWatchdog(void)
{
    // Check if watchdog caused reset
	if (MXC_WDT_GetResetFlag(MXC_WDT0))
	{
		MXC_WDT_ClearResetFlag(MXC_WDT0);
		MXC_WDT_DisableReset(MXC_WDT0);
		MXC_WDT_Disable(MXC_WDT0);
		printf("(Error) Watchdog reset the unit\n");
    }

	// Reset the Watchdog peripheral 
	MXC_WDT_Init(MXC_WDT0);

	MXC_WDT_SetResetPeriod(MXC_WDT0, MXC_WDT_PERIOD_2_31); // ~18 secs
	MXC_WDT_SetIntPeriod(MXC_WDT0, MXC_WDT_PERIOD_2_30); // ~9 secs
	MXC_WDT_EnableReset(MXC_WDT0);
	MXC_WDT_EnableInt(MXC_WDT0);
	NVIC_EnableIRQ(WDT0_IRQn);

	// Reset watchdog timer for enable sequence
	MXC_WDT_ResetTimer(MXC_WDT0);
    MXC_WDT_Enable(MXC_WDT0);
}

#define SPI_SPEED_ADC 10000000 // Bit Rate
#define SPI_SPEED_LCD 10000000 // Bit Rate
#define SPI_WIDTH_DUAL	2

<Add a master transasction>

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetupSPI(void)
{
	int status;

	status = MXC_SPI_Init(MXC_SPI3, 1, 0, 1, 0, SPI_SPEED_ADC);
	if (status != E_SUCCESS) { printf("<Error> SPI3 (ADC) Init failed with code: %d\n", status); }

	MXC_SPI_SetWidth(MXC_SPI3, SPI_WIDTH_DUAL);

	status = MXC_SPI_Init(MXC_SPI2, 1, 0, 1, 0, SPI_SPEED_LCD);
	if (status != E_SUCCESS) { printf("<Error> SPI2 (LCD) Init failed with code: %d\n", status); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitSystemHardware_NS9100(void)
{
	//-------------------------------------------------------------------------
	// Setup Watchdog
	//-------------------------------------------------------------------------
    SetupWatchdog();

	//-------------------------------------------------------------------------
	// Enable the instruction cache
	//-------------------------------------------------------------------------
    MXC_ICC_Enable();

	//-------------------------------------------------------------------------
	// Setup all GPIO
	//-------------------------------------------------------------------------
	SetupGPIO();

	//-------------------------------------------------------------------------
	// Setup UART0 (LTE) and UART1 (BLE)
	//-------------------------------------------------------------------------
	SetupUART();

	//-------------------------------------------------------------------------
	// Setup Debug Uart (UART2)
	//-------------------------------------------------------------------------
#if 0 /* Requires mod from original schematic */
	SetupDebugUART();
#warning Setup CONSOLE_UART 2 define (probably in board.h)
#endif

	//-------------------------------------------------------------------------
	// Setup I2C0 (1.8V devices) and I2C1 (3.3V devices)
	//-------------------------------------------------------------------------
	SetupI2C();

	//-------------------------------------------------------------------------
	// Setup SPI3 (ADC) and SPI2 (LCD)
	//-------------------------------------------------------------------------
	SetupSPI();

	//-------------------------------------------------------------------------
	// Set General Purpose Low-Power registers
	//-------------------------------------------------------------------------
#if 0 /* old hw */
	InitGplpRegisters();
#endif

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
#if 0 /* old hw */
	InitPullupsOnFloatingDataLines();
#endif

	//-------------------------------------------------------------------------
	// Init unused pins to low outputs
	//-------------------------------------------------------------------------
#if 0 /* old hw */
	InitProcessorNoConnectPins();
#endif

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
