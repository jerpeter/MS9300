///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

#ifndef _COMMON_H_
#define _COMMON_H_

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include "Typedefs.h"
#include "board.h"

#include "i2c.h"
#include "gpio.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define SOFT_VERSION	"1.11"
#define SOFT_DATE		"6-22-2004"
#define SOFT_TIME		"08:35pm"

#if 0 /* old FS */
#define SYSTEM_PATH		"A:\\System\\"
#define EVENTS_PATH		"A:\\Events\\"
#define ER_DATA_PATH	"A:\\ERData\\"
#define LANGUAGE_PATH	"A:\\Language\\"
#define LOGS_PATH		"A:\\Logs\\"
#else /* new FS */
#define SYSTEM_PATH		"0:System/"
#define EVENTS_PATH		"0:Events/"
#define ER_DATA_PATH	"0:ERData/"
#define LANGUAGE_PATH	"0:Language/"
#define LOGS_PATH		"0:Logs/"
#endif

#define EVT_FILE		"Evt"
#define EVTS_SUB_DIR	"Evts"

#define SUMMARY_LIST_FILE			"SummaryList.bin"
#define MONITOR_LOG_BIN_FILE		"MonitorLog.ns8"
#define MONITOR_LOG_READABLE_FILE	"MonitorLogReadable.txt"
#define ON_OFF_READABLE_FILE		"OnOffLogReadable.txt"
#define EXCEPTION_REPORT_FILE		"ExceptionReport.txt"

#define MAX_FILE_NAME_CHARS		255
#define MAX_TEXT_LINE_CHARS		255
#define MAX_BASE_PATH_CHARS		20

enum {
	EVENT_FILE_TYPE = 1,
	ER_DATA_FILE_TYPE
};

// Define core clock rate
#define SYS_CLK	120000000 // 120 MHz
#define P_CLK 	(SYS_CLK / 2) // 60 MHz

#define INTERNAL_SAMPLING_SOURCE	NO
// Make sure choice is mutually exclusive
#if (!INTERNAL_SAMPLING_SOURCE)
#define EXTERNAL_SAMPLING_SOURCE	YES
#else
#define EXTERNAL_SAMPLING_SOURCE	NO
#endif

#define VT_FEATURE_DISABLED			NO

#define FLASH_USER_PAGE_BASE_ADDRESS	(0x0000)
#define GET_HARDWARE_ID					(g_shadowFactorySetupRecord.hardwareID) // Factory setup location of Hardware ID
#define GET_BUILD_ID					(g_shadowFactorySetupRecord.buildID) // Factory setup location of Build ID

enum {
	HARDWARE_ID_REV_8_NORMAL = 0x08,
	HARDWARE_ID_REV_8_WITH_GPS_MOD = 0x28,
	HARDWARE_ID_REV_8_WITH_USART = 0x18,
};

enum {
	ACTIVE_MODE = 0,
	SLEEP_MODE,
	BACKGROUND_MODE,
	DEEPSLEEP_MODE,
	BACKUP_MODE
};

#define ENDIAN_CONVERSION	1

typedef struct
{
	uint8 year;
	uint8 month;
	uint8 day;
	uint8 weekday;
	uint8 unused0;
	uint8 hour;
	uint8 min;
	uint8 sec;
	uint8 hundredths;
	// Additional fields added to make the structure even and long bounded (even div by 4)
	uint8 unused1;
	uint8 unused2;
	uint8 valid;
} DATE_TIME_STRUCT;

typedef struct {
	uint16 year;
	uint8 month;
	uint8 day;
} CALIBRATION_DATE_STRUCT;

typedef union {
	uint32 epochDate;
	CALIBRATION_DATE_STRUCT normalDate;
	uint8 rawDate[4];
} CALIBRATION_DATE_UNIVERSAL_STRUCT;

typedef struct
{
	CALIBRATION_DATE_STRUCT date;
	uint8 source;
	uint8 unused;
} WORKING_CAL_DATE_STRUCT;

typedef struct
{
	uint32 cmd;
	uint32 length;
	uint32 data[6];
} INPUT_MSG_STRUCT;

typedef struct
{
	uint8 hour;
	uint8 min;
	uint8 sec;
} TM_TIME_STRUCT;

typedef struct
{
	uint8 day;
	uint8 month;
	uint8 year;
} TM_DATE_STRUCT;

#define TEN_MSEC				1					// # of counts per msec
#define SECND 					(10 * TEN_MSEC)		// # of ten_msec per second

#define SOFT_MSECS				1000 				// Scale usecs to msecs
#define SOFT_SECS				(1000 * 1000) 		// Scale usecs to secs

#define EXCEPTION_HANDLING_USE_SOFT_DELAY_KEY	(0xFFFFFF00)
#define EXCEPTION_MSG_DISPLAY_TIME				(8)	// Seconds

#define SECS_PER_DAY			86400
#define SECS_PER_HOUR			3600
#define SECS_PER_MIN			60

#define PI	3.14159

enum {
	JUMP_TO_BOOT = 2,
	OFF_EXCEPTION,
	FORCED_OFF
};

#define LANGUAGE_TABLE_MAX_SIZE		16384

// To eliminate C warning when the variable is not used.
#define UNUSED(p) ((void)p)

#define	DB_CONVERSION_VALUE			5000000
#define MB_CONVERSION_VALUE			400
#define ADJUSTED_MB_TO_HEX_VALUE		1.5625 // Was 25 @ 12-bit, new value @ 16-bit
#define ADJUSTED_MB_IN_PSI_TO_HEX_VALUE	105.20565

enum {
	INPUT_BUFFER_EMPTY = 0,
	INPUT_BUFFER_NOT_EMPTY
};

#define INPUT_BUFFER_SIZE	3

enum {
	DATA_NORMALIZED = 0,
	DATA_NOT_NORMALIZED
};

enum {
	SAVE_EXTRA_FILE_COMPRESSED_DATA = 1,
	DO_NOT_SAVE_EXTRA_FILE_COMPRESSED_DATA
};

enum {
	QUICK_BOOT_ENTRY_FROM_MENU = 1,
	QUICK_BOOT_ENTRY_FROM_SERIAL
};

// Channel type definitions
#define RADIAL_CHANNEL_TYPE 	10
#define VERTICAL_CHANNEL_TYPE 	11
#define TRANSVERSE_CHANNEL_TYPE 12
#define ACOUSTIC_CHANNEL_TYPE 	13

#define CAL_PULSE_FIXED_SAMPLE_RATE		SAMPLE_RATE_1K
#define CALIBRATION_FIXED_SAMPLE_RATE	SAMPLE_RATE_1K

typedef enum {
	MILLISECOND_TIMER = 1,
	TYPEMATIC_TIMER = 2
} PIT_TIMER_NUM;

#define INTERNAL_SAMPLING_TIMER_NUM		MXC_TMR1
#define CYCLIC_HALF_SEC_TIMER_NUM		MXC_TMR2
#define MILLISECOND_TIMER_NUM			MXC_TMR3
#define TYPEMATIC_TIMER_NUM				MXC_TMR4

enum {
	SEISMIC_GROUP_1 = 1,
	SEISMIC_GROUP_2
};

enum {
	SYSTEM_EVENT = 0,
	TIMER_EVENT,
	MENU_EVENT,
	TOTAL_EVENTS
};

enum {
	RAISE_FLAG = 1,
	CLEAR_FLAG
};

#define USBM_RI_8507_DRYWALL_STANDARD	1
#define USBM_RI_8507_PLASTER_STANDARD	2
#define OSM_REGULATIONS_STANDARD		3
#define END_OF_VIBRATION_STANDARDS_LIST	4
#define START_OF_CUSTOM_CURVES_LIST		9
#define CUSTOM_STEP_THRESHOLD			10
#define CUSTOM_STEP_LIMITING			11
#define END_OF_VIBRATION_CURVES_LIST	12

enum {
	EXTERNAL_TRIGGER_EVENT = 1,
	VARIABLE_TRIGGER_EVENT
};

enum {
	EXT_CHARGE_VOLTAGE,
	BATTERY_VOLTAGE
};

enum {
	USB_SYNC_NORMAL = 1,
	USB_SYNC_FROM_SHELL
};

// I2C0 @ 1.8V
#define I2C_ADDR_ACCELEROMETER			0x3C
#define I2C_ADDR_1_WIRE					0x30
#define I2C_ADDR_EEPROM					0xA0
#define I2C_ADDR_EEPROM_ID				0xB0
#define I2C_ADDR_BATT_CHARGER			0x5C
#define I2C_ADDR_USBC_PORT_CONTROLLER	0x42
// I2C1 @ 3.3V
#define I2C_ADDR_EXTERNAL_RTC			0xA2
#define I2C_ADDR_FUEL_GUAGE				0xC8
#define I2C_ADDR_EXPANSION				0x9A


#define VIN_CHANNEL		2
#define VBAT_CHANNEL	3

#define LOW_VOLTAGE_THRESHOLD		4.0 // Showing range 4V-7.3V
#define EXTERNAL_VOLTAGE_PRESENT	4.4 // USB minimum 3.0 is 4.5V, legacy is 4.4V

#define CYCLIC_EVENT_TIME_THRESHOLD		(4 * 2)
#define UPDATE_TIME_EVENT_THRESHOLD		(60 * 2)

// LTE serial port - UART 0
#define LTE_BAUDRATE	115200
#define LTE_COM_PORT	0

// BLE serial port - UART 1
#define BLE_BAUDRATE	115200
#define BLE_COM_PORT	1

// Debug serial port - UART 2
#define DEBUG_BAUDRATE	115200
#define DEBUG_COM_PORT	2

// Craft/Remote Comms serial port - USB CDC/ACM
#define CRAFT_COM_PORT	3 // Actual number insignificant, distinguish from the other UART ports

// Define Project Debug Port
#define GLOBAL_DEBUG_PRINT_PORT	DEBUG_COM_PORT //CRAFT_COM_PORT

#define TOTAL_UNIQUE_EVENT_NUMBERS		(65535)
#define EVENT_NUMBER_CACHE_MAX_ENTRIES	(TOTAL_UNIQUE_EVENT_NUMBERS + 1)
#define DER_CACHE_SIZE					(65536)

#define SPARE_BUFFER_SIZE				8192

/* Battery Level defines */
#define BATT_MIN_VOLTS 			4.0

#if NS8100_ORIGINAL_PROTOTYPE
#define REFERENCE_VOLTAGE		(float)3.3
#else /* (NS8100_ALPHA_PROTOTYPE || NS8100_BETA_PROTOTYPE) */
#define REFERENCE_VOLTAGE		(float)2.5
#endif
#define BATT_RESOLUTION			(float)1024 // 10-bit resolution

#define VOLTAGE_RATIO_BATT			(float)3
#define VOLTAGE_RATIO_EXT_CHARGE	(float)16.05

enum {
	KEYPAD_LED_STATE_UNKNOWN = 0,
	KEYPAD_LED_STATE_BOTH_OFF,
	KEYPAD_LED_STATE_IDLE_GREEN_ON,
	KEYPAD_LED_STATE_CHARGE_RED_ON,
	KEYPAD_LED_STATE_ACTIVE_GREEN_ON,
	KEYPAD_LED_STATE_ACTIVE_GREEN_OFF,
	KEYPAD_LED_STATE_ACTIVE_CHARGE_GREEN_ON,
	KEYPAD_LED_STATE_ACTIVE_CHARGE_RED_ON
};

enum {
	POWER_SAVINGS_NONE = 0,
	POWER_SAVINGS_MINIMUM,
	POWER_SAVINGS_NORMAL,
	POWER_SAVINGS_HIGH,
	POWER_SAVINGS_MAX
};

enum USB_STATES {
	USB_INIT_DRIVER,
	USB_NOT_CONNECTED,
	USB_READY,
	USB_CONNECTED_AND_PROCESSING,
	USB_HOST_MODE_WAITING_FOR_DEVICE,
	USB_DISABLED_FOR_OTHER_PROCESSING,
	USB_DEVICE_MODE_SELECTED,
	USB_HOST_MODE_SELECTED
};

enum USB_SYNC_FILE_EXISTS_ACTIONS {
	PROMPT_OPTION = 0,
	SKIP_OPTION,
	REPLACE_OPTION,
	DUPLICATE_OPTION,
	SKIP_ALL_OPTION,
	REPLACE_ALL_OPTION,
	DUPLICATE_ALL_OPTION
};

#if NS8100_BETA_PROTOTYPE
enum SDMMC_CARD_DETECT_STATE {
	SDMMC_CARD_DETECTED = 0,
	SDMMC_CARD_NOT_PRESENT
};
#elif NS8100_ALPHA_PROTOTYPE
enum SDMMC_CARD_DETECT_STATE {
	SDMMC_CARD_NOT_PRESENT = 0,
	SDMMC_CARD_DETECTED
};
#endif

enum {
	BP_UNHANDLED_INT = 1,
	BP_SOFT_LOOP,
	BP_MB_LOOP,
	BP_INT_MEM_CORRUPTED,
	BP_AD_CHAN_SYNC_ERR,
	BP_END
};

typedef enum {
	AVAILABLE = 1,
	EVENT_LOCK,
	CAL_PULSE_LOCK,
	SDMMC_LOCK,
	RTC_TIME_LOCK,
	EEPROM_LOCK
} SPI1_LOCK_TYPE;

typedef enum {
	BLOCKING = 1,
	ASYNC_ISR
} HANDLING_METHOD;

#define SPI_8_BIT_DATA_SIZE	8

typedef struct
{
	uint16 freq_count;
	uint16 peak;
	uint16* peakSamplePtr;
} VARIABLE_TRIGGER_FREQ_CHANNEL_BUFFER;

typedef struct
{
	VARIABLE_TRIGGER_FREQ_CHANNEL_BUFFER r[2];
	VARIABLE_TRIGGER_FREQ_CHANNEL_BUFFER v[2];
	VARIABLE_TRIGGER_FREQ_CHANNEL_BUFFER t[2];
	uint16 r_sign;
	uint16 v_sign;
	uint16 t_sign;
} VARIABLE_TRIGGER_FREQ_CALC_BUFFER;

enum {
	GPS_MSG_START = 1,
	GPS_MSG_BODY,
	GPS_MSG_CHECKSUM_FIRST_NIBBLE,
	GPS_MSG_CHECKSUM_SECOND_NIBBLE,
	GPS_MSG_END,
	GPS_BINARY_MSG_START,
	GPS_BINARY_MSG_START_END,
	GPS_BINARY_MSG_PAYLOAD_START,
	GPS_BINARY_MSG_PAYLOAD,
	GPS_BINARY_MSG_BODY,
	GPS_BINARY_MSG_CHECKSUM,
	GPS_BINARY_MSG_END
};

#define GPS_SERIAL_BUFFER_SIZE		500
#define TOTAL_GPS_MESSAGES	5
#define TOTAL_GPS_BINARY_MESSAGES	10
#define GPS_MESSAGE_SIZE	100
#define GPS_THRESHOLD_TOTAL_FIXES_FOR_BEST_LOCATION		(2 * 30) // 30 seconds of GGA and GLL location fixes
#define GPS_ACTIVE_LOCATION_SEARCH_TIME		10
#define GPS_REACTIVATION_TIME_SEARCH_FAIL	50
#define GPS_REACTIVATION_TIME_NORMAL		60

#define GPS_POWER_NORMAL_SAVE_POWER		0
#define GPS_POWER_ALWAYS_ON_ACQUIRING	1

typedef struct {
	uint8* readPtr;
	uint8* writePtr;
	uint8* endPtr;
	uint8 buffer[GPS_SERIAL_BUFFER_SIZE];
	uint8 ready;
	uint8 state;
	uint8 binaryState;
} GPS_SERIAL_DATA;

typedef struct {
	uint8 data[GPS_MESSAGE_SIZE];
	uint8 checksum;
} GPS_MESSAGE;

typedef struct {
	uint8 binMsgValid;
	uint8 binMsgSize;
	uint8 data[GPS_MESSAGE_SIZE];
} GPS_BINARY_MESSAGE;

typedef struct {
	uint8 readIndex;
	uint8 writeIndex;
	uint8 endIndex;
	uint8 messageReady;
	GPS_MESSAGE message[TOTAL_GPS_MESSAGES];
} GPS_QUEUE;

typedef struct {
	uint8 readIndex;
	uint8 writeIndex;
	uint8 endIndex;
	uint8 binaryMessageReady;
	GPS_BINARY_MESSAGE message[TOTAL_GPS_BINARY_MESSAGES];
} GPS_BINARY_QUEUE;

typedef struct {
	uint32 acquisitionStartTicks;
	uint32 acqTickAccumulation;
	uint16 totalAcquisitions;
	uint16 failedAcquisitions;
	uint16 retryForPositionTimeoutInSeconds;
	uint8 positionFound;
} GPS_INFO;

enum {
	GGA = 0,
	GLL,
	ZDA,
	TOTAL_GPS_COMMANDS
};

enum {
	GPS_BIN_MSG_ACK = 0,
	GPS_BIN_MSG_NACK,
	VERSION_QUERY,
	SOFT_CRC_QUERY,
	NMEA_MSG_INTERVAL_QUERY,
	GPS_TIME_QUERY,
	TOTAL_GPS_BINARY_COMMANDS
};

typedef struct
{
	int     year;       /**< Years since 1900 */
	int     mon;        /**< Months since January - [0,11] */
	int     day;        /**< Day of the month - [1,31] */
	int     hour;       /**< Hours since midnight - [0,23] */
	int     min;        /**< Minutes after the hour - [0,59] */
	int     sec;        /**< Seconds after the minute - [0,59] */
	int     hsec;       /**< Hundredth part of second - [0,99] */
} nmeaTIME;

typedef struct
{
	uint8 latDegrees;
	uint8 latMinutes;
	uint16 latSeconds;
	uint8 longDegrees;
	uint8 longMinutes;
	uint16 longSeconds;
	char northSouth;
	char eastWest;
	uint8 utcHour;
	uint8 utcMin;
	uint8 utcSec;
	uint8 validLocationCount;
	uint8 locationFoundWhileMonitoring;
	uint8 positionFix;
	int16 altitude;
	uint16 utcYear;
	uint8 utcMonth;
	uint8 utcDay;
} GPS_POSITION;

//--------------------------------------------------------------------------------
// GPIO 0 Port defines
//--------------------------------------------------------------------------------
#define GPIO_MCU_POWER_LATCH_PORT				MXC_GPIO0
#define GPIO_EXPANDED_BATTERY_PORT				MXC_GPIO0
#define GPIO_LED_1_PORT							MXC_GPIO0
#define GPIO_GAUGE_ALERT_PORT					MXC_GPIO0
#define GPIO_BATTERY_CHARGER_IRQ_PORT			MXC_GPIO0
#define GPIO_ENABLE_12V_PORT					MXC_GPIO0
#define GPIO_ENABLE_5V_PORT						MXC_GPIO0
#define GPIO_EXPANSION_IRQ_PORT					MXC_GPIO0
#define GPIO_USB_SOURCE_ENABLE_PORT				MXC_GPIO0
#define GPIO_USB_AUX_POWER_ENABLE_PORT			MXC_GPIO0
#define GPIO_POWER_GOOD_5V_PORT					MXC_GPIO0
#define GPIO_POWER_GOOD_BATTERY_CHARGE_PORT		MXC_GPIO0
#define GPIO_SMART_SENSOR_SLEEP_PORT			MXC_GPIO0
#define GPIO_SMART_SENSOR_MUX_ENABLE_PORT		MXC_GPIO0
#define GPIO_ADC_RESET_PORT						MXC_GPIO0
#define GPIO_ADC_BUSY_ALT_GP0_PORT				MXC_GPIO0
#define GPIO_ADC_CONVERSION_PORT				MXC_GPIO0
#define GPIO_ADC_SPI_3_SS_0_PORT				MXC_GPIO0
#define GPIO_CAL_MUX_PRE_AD_ENABLE_PORT			MXC_GPIO0
#define GPIO_CAL_MUX_PRE_AD_SELECT_PORT			MXC_GPIO0
#define GPIO_ALERT_1_PORT						MXC_GPIO0
#define GPIO_ALERT_2_PORT						MXC_GPIO0
#define GPIO_LTE_OTA_PORT						MXC_GPIO0
#define GPIO_EMMC_RESET_PORT					MXC_GPIO0

//--------------------------------------------------------------------------------
// GPIO1 Port defines
//--------------------------------------------------------------------------------
#define GPIO_SDHC_PORT							MXC_GPIO1
#define GPIO_EMMC_DATA_STROBE_PORT				MXC_GPIO1
#define GPIO_EXPANSION_ENABLE_PORT				MXC_GPIO1
#define GPIO_EXPANSION_RESET_PORT				MXC_GPIO1
#define GPIO_USBC_PORT_CONTROLLER_I2C_IRQ_PORT	MXC_GPIO1
#define GPIO_ACCEL_INT_1_PORT					MXC_GPIO1
#define GPIO_ACCEL_INT_2_PORT					MXC_GPIO1
#define GPIO_ACCEL_TRIG_PORT					MXC_GPIO1
#define GPIO_POWER_BUTTON_IRQ_PORT				MXC_GPIO1
#define GPIO_BUTTON_1_PORT						MXC_GPIO1
#define GPIO_BUTTON_2_PORT						MXC_GPIO1
#define GPIO_BUTTON_3_PORT						MXC_GPIO1
#define GPIO_BUTTON_4_PORT						MXC_GPIO1
#define GPIO_BUTTON_5_PORT						MXC_GPIO1
#define GPIO_BUTTON_6_PORT						MXC_GPIO1
#define GPIO_BUTTON_7_PORT						MXC_GPIO1
#define GPIO_BUTTON_8_PORT						MXC_GPIO1
#define GPIO_BUTTON_9_PORT						MXC_GPIO1
#define GPIO_LED_2_PORT							MXC_GPIO1
#define GPIO_LED_3_PORT							MXC_GPIO1
#define GPIO_LED_4_PORT							MXC_GPIO1
#define GPIO_EXT_RTC_INTA_PORT					MXC_GPIO1
#define GPIO_BLE_OTA_PORT						MXC_GPIO1
#define GPIO_EXTERNAL_TRIGGER_OUT_PORT			MXC_GPIO1
#define GPIO_EXTERNAL_TRIGGER_IN_PORT			MXC_GPIO1

//--------------------------------------------------------------------------------
// GPIO2 Port defines
//--------------------------------------------------------------------------------
#define GPIO_LCD_POWER_ENABLE_PORT				MXC_GPIO2
#define GPIO_LCD_POWER_DISPLAY_PORT				MXC_GPIO2
#define GPIO_SPI_2_SS_0_LCD_PORT				MXC_GPIO2
#define GPIO_LCD_INT_PORT						MXC_GPIO2
#define GPIO_SENSOR_CHECK_ENABLE_PORT			MXC_GPIO2
#define GPIO_SENSOR_CHECK_PORT					MXC_GPIO2
#define GPIO_LTE_RESET_PORT						MXC_GPIO2
#define GPIO_BLE_RESET_PORT						MXC_GPIO2
#define GPIO_SMART_SENSOR_MUX_A0_PORT			MXC_GPIO2
#define GPIO_SMART_SENSOR_MUX_A1_PORT			MXC_GPIO2
#define GPIO_NYQUIST_0_A0_PORT					MXC_GPIO2
#define GPIO_NYQUIST_1_A1_PORT					MXC_GPIO2
#define GPIO_NYQUIST_2_ENABLE_PORT				MXC_GPIO2

//--------------------------------------------------------------------------------
// GPIO3 Port defines
//--------------------------------------------------------------------------------
#define GPIO_CELL_ENABLE_PORT					MXC_GPIO3
#define GPIO_SENSOR_ENABLE_GEO1_PORT			MXC_GPIO3
#define GPIO_SENSOR_ENABLE_AOP1_PORT			MXC_GPIO3
#define GPIO_SENSOR_ENABLE_GEO2_PORT			MXC_GPIO3
#define GPIO_SENSOR_ENABLE_AOP2_PORT			MXC_GPIO3
#define GPIO_GAIN_SELECT_GEO1_PORT				MXC_GPIO3
#define GPIO_PATH_SELECT_AOP1_PORT				MXC_GPIO3
#define GPIO_GAIN_SELECT_GEO2_PORT				MXC_GPIO3
#define GPIO_PATH_SELECT_AOP2_PORT				MXC_GPIO3
#define GPIO_RTC_CLOCK_PORT						MXC_GPIO3

//--------------------------------------------------------------------------------
// GPIO0 Port, Pin defines
//--------------------------------------------------------------------------------
#define GPIO_MCU_POWER_LATCH_PIN				MXC_GPIO_PIN_1
#define GPIO_EXPANDED_BATTERY_PIN				MXC_GPIO_PIN_2
#define GPIO_LED_1_PIN							MXC_GPIO_PIN_3
#define GPIO_GAUGE_ALERT_PIN					MXC_GPIO_PIN_4
#define GPIO_BATTERY_CHARGER_IRQ_PIN			MXC_GPIO_PIN_5
#define GPIO_ENABLE_12V_PIN						MXC_GPIO_PIN_6
#define GPIO_ENABLE_5V_PIN						MXC_GPIO_PIN_7
#define GPIO_EXPANSION_IRQ_PIN					MXC_GPIO_PIN_8
#define GPIO_USB_SOURCE_ENABLE_PIN				MXC_GPIO_PIN_9
#define GPIO_USB_AUX_POWER_ENABLE_PIN			MXC_GPIO_PIN_10
#define GPIO_POWER_GOOD_5V_PIN					MXC_GPIO_PIN_11
#define GPIO_POWER_GOOD_BATTERY_CHARGE_PIN		MXC_GPIO_PIN_12
#define GPIO_SMART_SENSOR_SLEEP_PIN				MXC_GPIO_PIN_13
#define GPIO_SMART_SENSOR_MUX_ENABLE_PIN		MXC_GPIO_PIN_14
#define GPIO_ADC_RESET_PIN						MXC_GPIO_PIN_15
// GPIO_ADC_SPI_SCK								MXC_GPIO_PIN_16
#define GPIO_ADC_BUSY_ALT_GP0_PIN				MXC_GPIO_PIN_17
#define GPIO_ADC_CONVERSION_PIN					MXC_GPIO_PIN_18
#define GPIO_ADC_SPI_3_SS_0_PIN					MXC_GPIO_PIN_19
// GPIO_ADC_SPI_3_SDO1							MXC_GPIO_PIN_20
// GPIO_ADC_SPI_3_SDI							MXC_GPIO_PIN_21
#define GPIO_CAL_MUX_PRE_AD_ENABLE_PIN			MXC_GPIO_PIN_22
#define GPIO_CAL_MUX_PRE_AD_SELECT_PIN			MXC_GPIO_PIN_23
#define GPIO_ALERT_1_PIN						MXC_GPIO_PIN_24
#define GPIO_ALERT_2_PIN						MXC_GPIO_PIN_25
// GPIO_TDI										MXC_GPIO_PIN_26
// GPIO_TDO										MXC_GPIO_PIN_27
// GPIO_TMS										MXC_GPIO_PIN_28
// GPIO_TCK										MXC_GPIO_PIN_29
#define GPIO_LTE_OTA_PIN						MXC_GPIO_PIN_30
#define GPIO_EMMC_RESET_PIN						MXC_GPIO_PIN_31

//--------------------------------------------------------------------------------
// GPIO1 Port, Pin defines
//--------------------------------------------------------------------------------
#define GPIO_SDHC_CMD_PIN						MXC_GPIO_PIN_0
#define GPIO_SDHC_DAT2_PIN						MXC_GPIO_PIN_1
#define GPIO_EMMC_DATA_STROBE_PIN				MXC_GPIO_PIN_2
#define GPIO_SDHC_DAT3_PIN						MXC_GPIO_PIN_3
#define GPIO_SDHC_DAT0_PIN						MXC_GPIO_PIN_4
#define GPIO_SDHC_CLK_PIN						MXC_GPIO_PIN_5
#define GPIO_SDHC_DAT1_PIN						MXC_GPIO_PIN_6
#define GPIO_EXPANSION_ENABLE_PIN				MXC_GPIO_PIN_7
#define GPIO_EXPANSION_RESET_PIN				MXC_GPIO_PIN_8
// GPIO_MCU_UART2_RX							MXC_GPIO_PIN_9
// GPIO_MCU_UART2_TX							MXC_GPIO_PIN_10
#define GPIO_USBC_PORT_CONTROLLER_I2C_IRQ_PIN	MXC_GPIO_PIN_11
#define GPIO_ACCEL_INT_1_PIN					MXC_GPIO_PIN_12
#define GPIO_ACCEL_INT_2_PIN					MXC_GPIO_PIN_13
#define GPIO_ACCEL_TRIG_PIN						MXC_GPIO_PIN_14
#define GPIO_POWER_BUTTON_IRQ_PIN				MXC_GPIO_PIN_15
#define GPIO_BUTTON_1_PIN						MXC_GPIO_PIN_16
#define GPIO_BUTTON_2_PIN						MXC_GPIO_PIN_17
#define GPIO_BUTTON_3_PIN						MXC_GPIO_PIN_18
#define GPIO_BUTTON_4_PIN						MXC_GPIO_PIN_19
#define GPIO_BUTTON_5_PIN						MXC_GPIO_PIN_20
#define GPIO_BUTTON_6_PIN						MXC_GPIO_PIN_21
#define GPIO_BUTTON_7_PIN						MXC_GPIO_PIN_22
#define GPIO_BUTTON_8_PIN						MXC_GPIO_PIN_23
#define GPIO_BUTTON_9_PIN						MXC_GPIO_PIN_24
#define GPIO_LED_2_PIN							MXC_GPIO_PIN_25
#define GPIO_LED_3_PIN							MXC_GPIO_PIN_26
#define GPIO_LED_4_PIN							MXC_GPIO_PIN_27
#define GPIO_EXT_RTC_INTA_PIN					MXC_GPIO_PIN_28
#define GPIO_BLE_OTA_PIN						MXC_GPIO_PIN_29
#define GPIO_EXTERNAL_TRIGGER_OUT_PIN			MXC_GPIO_PIN_30
#define GPIO_EXTERNAL_TRIGGER_IN_PIN			MXC_GPIO_PIN_31

//--------------------------------------------------------------------------------
// GPIO2 Port, Pin defines
//--------------------------------------------------------------------------------
#define GPIO_LCD_POWER_ENABLE_PIN				MXC_GPIO_PIN_0
#define GPIO_LCD_POWER_DISPLAY_PIN				MXC_GPIO_PIN_1
// GPIO_SPI2_SCK_LCD							MXC_GPIO_PIN_2
// GPIO_SPI2_MISO_LCD							MXC_GPIO_PIN_3
// GPIO_SPI2_MOSI_LCD							MXC_GPIO_PIN_4
#define GPIO_SPI_2_SS_0_LCD_PIN					MXC_GPIO_PIN_5
#define GPIO_LCD_INT_PIN						MXC_GPIO_PIN_6
// GPIO_I2C0_SDA								MXC_GPIO_PIN_7
// GPIO_I2C0_SCL								MXC_GPIO_PIN_8
#define GPIO_SENSOR_CHECK_ENABLE_PIN			MXC_GPIO_PIN_9
#define GPIO_SENSOR_CHECK_PIN					MXC_GPIO_PIN_10
// GPIO_LTE_UART_TXD							MXC_GPIO_PIN_11
// GPIO_LTE_UART_RXD							MXC_GPIO_PIN_12
#define GPIO_LTE_RESET_PIN						MXC_GPIO_PIN_13
// GPIO_BLE_UART_TXD							MXC_GPIO_PIN_14
#define GPIO_BLE_RESET_PIN						MXC_GPIO_PIN_15
// GPIO_BLE_UART_RXD							MXC_GPIO_PIN_16
// GPIO_I2C1_SDA								MXC_GPIO_PIN_17
// GPIO_I2C1_SCL								MXC_GPIO_PIN_18
// GAP											MXC_GPIO_PIN_19
// GAP											MXC_GPIO_PIN_20
// GAP											MXC_GPIO_PIN_21
// GAP											MXC_GPIO_PIN_22
#define GPIO_SMART_SENSOR_MUX_A0_PIN			MXC_GPIO_PIN_23
// GAP											MXC_GPIO_PIN_24
#define GPIO_SMART_SENSOR_MUX_A1_PIN			MXC_GPIO_PIN_25
#define GPIO_NYQUIST_0_A0_PIN					MXC_GPIO_PIN_26
// GAP											MXC_GPIO_PIN_27
#define GPIO_NYQUIST_1_A1_PIN					MXC_GPIO_PIN_28
// GAP											MXC_GPIO_PIN_29
#define GPIO_NYQUIST_2_ENABLE_PIN				MXC_GPIO_PIN_30

//--------------------------------------------------------------------------------
// GPIO3 Port, Pin defines
//--------------------------------------------------------------------------------
#define GPIO_CELL_ENABLE_PIN					MXC_GPIO_PIN_0
#define GPIO_SENSOR_ENABLE_GEO1_PIN				MXC_GPIO_PIN_1
#define GPIO_SENSOR_ENABLE_AOP1_PIN				MXC_GPIO_PIN_2
#define GPIO_SENSOR_ENABLE_GEO2_PIN				MXC_GPIO_PIN_3
#define GPIO_SENSOR_ENABLE_AOP2_PIN				MXC_GPIO_PIN_4
#define GPIO_GAIN_SELECT_GEO1_PIN				MXC_GPIO_PIN_5
#define GPIO_PATH_SELECT_AOP1_PIN				MXC_GPIO_PIN_6
#define GPIO_GAIN_SELECT_GEO2_PIN				MXC_GPIO_PIN_7
#define GPIO_PATH_SELECT_AOP2_PIN				MXC_GPIO_PIN_8
#define GPIO_RTC_CLOCK_PIN						MXC_GPIO_PIN_9

//--------------------------------------------------------------------------------
// GPIO Buttons group defines
//--------------------------------------------------------------------------------
#define REGULAR_BUTTONS_GPIO_PORT	MXC_GPIO1
#define REGULAR_BUTTONS_GPIO_MASK	0x1FF0000

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------
// Gps routines
void InitGpsBuffers(void);
void EnableGps(void);
void DisableGps(void);
uint8 GpsChecksum(uint8* message);
void ProcessGpsSerialData(void);
void ProcessGpsMessage(void);
void ProcessGpsBinaryMessage(void);
void GpsQueryVersion(void);
void GpsQueryTime(void);
//void GpsQueryUTCDate(void);
void GpsQuerySoftCrc(void);
void GpsQueryNmeaMsgInterval(void);
void GpsSendBinaryMessage(uint8* binaryMessage, uint16 messageLength);
uint8 GpsCalcBinaryChecksum(uint8* binaryPayload, uint16 payloadLength);
void HandleVersionQuery(void);
void HandleSoftCrcQuery(void);
void HandleNmeaMsgIntervalQuery(void);
void HandleGPSQueryTime(void);
void HandleUTCDateQuery(void);
void GpsChangeNmeaMsgInterval(uint8 GGAInt, uint8 GSAInt, uint8 GSVInt, uint8 GLLInt, uint8 RMCInt, uint8 VTGInt, uint8 ZDAInt);
void HandleBinaryMsgAck(void);
void HandleBinaryMsgNack(void);
void GpsChangeSerialBaud(void);

// Battery routines
float GetExternalVoltageLevelAveraged(uint8 type);
BOOLEAN CheckExternalChargeVoltagePresent(void);

// Power Savings
void AdjustPowerSavings(uint8_t powerSavingsLevel);

// Math routines
uint16 Isqrt (uint32 x);

// Input message routines
uint16 CheckInputMsg(INPUT_MSG_STRUCT *);
void ProcessInputMsg (INPUT_MSG_STRUCT);
uint16 SendInputMsg(INPUT_MSG_STRUCT *);

// Delay timing
void SoftUsecWait(uint32 usecs);
void SpinBar(void);

// Conversion routines
uint16 SwapInt(uint16);
float HexToDB(uint16, uint8, uint16, uint8);
float HexToMB(uint16, uint8, uint16, uint8);
float HexToPSI(uint16, uint8, uint16, uint8);
uint16 DbToHex(uint32, uint8);
uint16 MbToHex(uint32, uint8);
uint16 PsiToHex(uint32, uint8);

// PIT timers
void SetupInteralPITTimer(uint8_t channel, uint16_t freq);
void StartInteralPITTimer(PIT_TIMER_NUM channel);
void StopInteralPITTimer(PIT_TIMER_NUM channel);

// Language translation
void BuildLanguageLinkTable(uint8 languageSelection);

// Version routine
//void initVersionStrings(void);
void InitVersionMsg(void);

// Bootloader Function
void CheckBootloaderAppPresent(void);

// Main menu prototype extensions
void HandleSystemEvents(void);
void BootLoadManager(void);
void SystemEventManager(void);
void MenuEventManager(void);
void CraftManager(void);
void GpsManager(void);
void MessageManager(void);
void FactorySetupManager(void);
void UsbDeviceManager(void);
void UsbDisableIfActive(void);
void CheckExceptionReportLogExists(void);

// GPIO Status extensions
uint8_t GetExpandedBatteryPresenceState(void);
uint8_t GetPowerGood5vState(void);
uint8_t GetPowerGoodBatteryChargerState(void);
uint8_t GetPowerOnButtonState(void);
uint8_t GetLteOtaState(void);
uint8_t GetBleOtaState(void);
uint8_t GetSmartSensorMuxEnableState(void);
uint8_t GetCalMuxPreADSelectState(void);

// GPIO Control extensions
void SetSmartSensorSleepState(uint8_t state);
void SetSmartSensorMuxEnableState(uint8_t state);
void SetAdcConversionState(uint8_t state);
void SetCalMuxPreADEnableState(uint8_t state);
void SetCalMuxPreADSelectState(uint8_t state);
void SetAccelerometerTriggerState(uint8_t state);
void SetSensorCheckState(uint8_t state);
void SetSmartSensorMuxA0State(uint8_t state);
void SetSmartSensorMuxA1State(uint8_t state);
void SetNyquist0State(uint8_t state);
void SetNyquist1State(uint8_t state);
void SetNyquist2EnableState(uint8_t state);
void SetSensorGeo1EnableState(uint8_t state);
void SetSensorAop1EnableState(uint8_t state);
void SetSensorGeo2EnableState(uint8_t state);
void SetSensorAop2EnableState(uint8_t state);
void SetGainGeo1State(uint8_t state);
void SetPathSelectAop1State(uint8_t state);
void SetGainGeo2State(uint8_t state);
void SetPathSelectAop2State(uint8_t state);

// Init Hardware prototype extensions
void InitSystemHardware_MS9300(void);

// Init Interrupts prototype extensions
void InitInterrupts_MS9300(void);
void SetupInteralSampleTimer(uint16_t sampleRate);
void StartInteralSampleTimer(void);
void StopInteralSampleTimer(void);

// Init Software prototype extensions
void InitSoftwareSettings_MS9300(void);

// Init devices prototype extensions
void AccelerometerInit(void);
void ExpansionBridgeInit(void);
void USBCPortControllerInit(void);
void FuelGaugeInit(void);
void BatteryChargerInit(void);

// Test device functions
void TestAccelerometer(void);
void TestBatteryCharger(void);
void TestFuelGauge(void);
void TestEEPROM(void);
void TestExpansionI2CBridge(void);
void TestEMMCFatFilesystem(void);
void TestExternalRTC(void);
void TestUSBCPortController(void);
void Test1Wire(void);
void TestExternalADC(void);
void TestLCD(void);

// ISRs prototype extensions
void DataIsrInit(uint16 sampleRate);
void Keypad_irq(void);
void System_power_button_irq(void);
void External_rtc_irq(void);
void Gps_status_irq(void);
void Sample_irq(void);
void Internal_rtc_alarms(void);
void Soft_timer_tick_irq(void);
void External_trigger_irq(void);
void HandleActiveAlarmExtension(void);
void SensorCalibrationDataInit(void);
void ProcessSensorCalibrationData(void);
void Fuel_gauge_alert_irq(void);
void Battery_charger_irq(void);
void Expansion_irq(void);
void Usbc_port_controller_i2c_irq(void);
void Accelerometer_irq_1(void);
void Accelerometer_irq_2(void);
void Lcd_irq(void);
void Eic_low_battery_irq(void);
void Tc_typematic_irq(void);
void StartInteralPITTimer(PIT_TIMER_NUM);
void StopInteralPITTimer(PIT_TIMER_NUM);

#if EXTERNAL_SAMPLING_SOURCE
void Tc_ms_timer_irq(void);
#endif

// Process Handler prototype extensions
void StartDataCollection(uint32 sampleRate);
void GetManualCalibration(void);
void HandleCycleChangeEvent(void);

// Keypad prototype extensions
void InitKeypad(void);

// Time routines
uint8 GetDayOfWeek(uint8 year, uint8 month, uint8 day);
uint16 GetTotalDaysFromReference(TM_DATE_STRUCT date);
void GetDateString(char*, uint8, uint8);
uint8 GetDaysPerMonth(uint8, uint16);
void InitTimeMsg(void);
void CheckForCycleChange(void);

// Cycle Count
uint32_t CycleCountToMicroseconds(uint32_t cycleCount, uint32_t mpuCoreFreq);

// Error routines
void ReportFileSystemAccessProblem(char*);
void ReportFileAccessProblem(char* attemptedFile);

// CalDate/DateTime conversions
void ConvertDateTimeToCalDate(CALIBRATION_DATE_STRUCT* calDate, DATE_TIME_STRUCT* dateTime);
void ConvertCalDatetoDateTime(DATE_TIME_STRUCT* dateTime, CALIBRATION_DATE_STRUCT* calDate);

// Spi 1 Mutex Access
void GetSpi1MutexLock(SPI1_LOCK_TYPE spi1LockType);
void ReleaseSpi1MutexLock(void);

// Validate trigger source
uint8 CheckTriggerSourceExists(void);

// Process the USB Core routines
void ProcessUsbCoreHandling(void);

// Wrapper to help I2C slave device comms
int WriteI2CDevice(mxc_i2c_regs_t* i2cChannel, uint8_t slaveAddr, uint8_t* writeData, uint32_t writeSize, uint8_t* readData, uint32_t readSize);

// Wrapper to help SPI transactions
void SpiTransaction(mxc_spi_regs_t* spiPort, uint8_t dataBits, uint8_t ssDeassert, uint8_t* writeData, uint32_t writeSize, uint8_t* readData, uint32_t readSize, uint8_t method);

#endif // _COMMON_H_
