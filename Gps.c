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
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include "Common.h"
#include "Uart.h"
#include "Menu.h"
#include "SoftTimer.h"
#include "PowerManagement.h"
#include "SysEvents.h"
#include "TextTypes.h"
//#include "usart.h"
#include "ctype.h"
#include "Display.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define NMEA_TOKS_COMPARE   (1)
#define NMEA_TOKS_PERCENT   (2)
#define NMEA_TOKS_WIDTH     (3)
#define NMEA_TOKS_TYPE      (4)
#define NMEA_CONVSTR_BUF    (256)
#define NMEA_TIMEPARSE_BUF  (256)
#define NMEA_DEF_PARSEBUFF  (1024)
#define NMEA_MIN_PARSEBUFF  (256)

void nmea_error(const char *str);

typedef struct _nmeaGPGGA
{
	nmeaTIME utc;       /**< UTC of position (just time) */
	double  lat;        /**< Latitude in NDEG - [degree][min].[sec/60] */
	char    ns;         /**< [N]orth or [S]outh */
	double  lon;        /**< Longitude in NDEG - [degree][min].[sec/60] */
	char    ew;         /**< [E]ast or [W]est */
	int     sig;        /**< GPS quality indicator (0 = Invalid; 1 = Fix; 2 = Differential, 3 = Sensitive) */
	int     satinuse;   /**< Number of satellites in use (not those in view) */
	double  HDOP;       /**< Horizontal dilution of precision */
	double  elv;        /**< Antenna altitude above/below mean sea level (geoid) */
	char    elv_units;  /**< [M]eters (Antenna height unit) */
	double  diff;       /**< Geoidal separation (Diff. between WGS-84 earth ellipsoid and mean sea level. '-' = geoid is below WGS-84 ellipsoid) */
	char    diff_units; /**< [M]eters (Units of geoidal separation) */
	double  dgps_age;   /**< Time in seconds since last DGPS update */
	int     dgps_sid;   /**< DGPS station ID number */
} nmeaGPGGA;

typedef struct _nmeaGPGLL
{
	double  lat;        /**< Latitude in NDEG - [degree][min].[sec/60] */
	char    ns;         /**< [N]orth or [S]outh */
	double  lon;        /**< Longitude in NDEG - [degree][min].[sec/60] */
	char	ew;         /**< [E]ast or [W]est */
	nmeaTIME utc;       /**< UTC of position (just time) */
	char	sta;		/**< Status, A= data valid, V= Data not valid */
	char    sig;        /**< Signal integrity, A=autonomous, D=differential, E=Estimated, N=not valid, S=Simulator */
} nmeaGPGLL;

typedef struct _nmeaGPZDA
{
	nmeaTIME utc;       /**< UTC (full time) */
	int		lzHours;	/**< Local zone hours */
	int		lzMins;		/**< Local zone minutes */
} nmeaGPZDA;

typedef struct
{
	uint8 cmdChar1;
	uint8 cmdChar2;
	uint8 cmdChar3;
	void (*cmdFunction)(uint8*);
} GPS_MESSAGE_COMMAND_STRUCT;

typedef struct
{
	uint8 cmd;
	uint8 cmdID;
	uint8 cmdSubID;
	void (*cmdFunction)(void);
} GPS_BINARY_MESSAGE_COMMAND_STRUCT;

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
#include "Globals.h"

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------
int nmea_scanf(const char *buff, int buff_sz, const char *format, ...);
int _nmea_parse_time(const char *buff, int buff_sz, nmeaTIME *res);
void HandleGGA(uint8* message);
void HandleGLL(uint8* message);
void HandleZDA(uint8* message);

///----------------------------------------------------------------------------
///	Local Scope Globals
///----------------------------------------------------------------------------
static const GPS_MESSAGE_COMMAND_STRUCT s_gpsMessageTable[TOTAL_GPS_COMMANDS] = {
	{'G', 'G', 'A', HandleGGA},		// Global Positioning System Fix Data
	{'G', 'L', 'L', HandleGLL},		// Geographic Latitude and Longitude
	{'Z', 'D', 'A', HandleZDA}		// UTC Time with Local Zone
};

static const GPS_BINARY_MESSAGE_COMMAND_STRUCT s_gpsBinaryMessageTable[TOTAL_GPS_BINARY_COMMANDS] = {
	{GPS_BIN_MSG_ACK, 0x83, 0x00, HandleBinaryMsgAck},					// Version data for GPS
	{GPS_BIN_MSG_NACK, 0x84, 0x00, HandleBinaryMsgNack},				// Version data for GPS
	{VERSION_QUERY,	0x80, 0x00, HandleVersionQuery},					// Version data for GPS
	{SOFT_CRC_QUERY, 0x81, 0x00, HandleSoftCrcQuery},					// Crc for version data
	{NMEA_MSG_INTERVAL_QUERY, 0x64, 0x81, HandleNmeaMsgIntervalQuery},	// NMEA Message Interval
	{GPS_TIME_QUERY, 0x64, 0x8E, HandleGPSQueryTime}
};

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ConvertGpsPositionToDegreesMinsSecs(double position, uint8* degrees, uint8* minutes, uint16* seconds)
{
	char buf[20];
	uint16 degreesMinutes;

	sprintf(buf, "%0.4f", position);
	sscanf(buf, "%hu.%hu", &degreesMinutes, seconds);

	*degrees = (uint8)(degreesMinutes / 100);
	*minutes = (uint8)(degreesMinutes % 100);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 GpsChecksum(uint8* message)
{
	uint8 checksum = 0;

	while (*message)
	{
		checksum ^= *message++;
	}

	return checksum;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void EnableGps(void)
{
	// Check that the Hardware ID is set for GPS module
	if (GET_HARDWARE_ID == HARDWARE_ID_REV_8_WITH_GPS_MOD)
	{
		// Check if the GPS module is powered off (Active low signal)
#if 0 /* old hw */
		if (gpio_get_pin_value(AVR32_PIN_PB14) == 1)
		{
			// Enable power for Usart0
			// Enable Usart0 power (Includes: USART0, USART 1, USART 3, TC, TWI, SPI0, SPI1, ADC, PM/RTC/EIC, GPIO, INTC; Disable: ABDAC, SSC, PWM, USART 2, PDCA)
			AVR32_PM.pbamask = 0x00004BFB;

			// Note: Check if needed
			InitGps232();

			// Reset buffers, flags and re-setup interrupt handler
			InitGpsBuffers();
			Setup_8100_Usart0_RS232_ISR();

			if (g_gpsOutputToCraft)
			{
				uint16 length = sprintf((char*)g_spareBuffer, "<GPS: POWER ON>\r\n");
				ModemPuts(g_spareBuffer, length, NO_CONVERSION);
			}

			// Set Baud rate (115200)
			gpio_set_gpio_pin(AVR32_PIN_PB13); // GPIO 45 (Pin 126 / EBI - CAS)

			// Enable power for Gps module
			gpio_clr_gpio_pin(AVR32_PIN_PB14); // GPIO 46 (Pin 127 / EBI - SDWE)
#if 0 /* Original */
			// Add soft timer to power off
			AssignSoftTimer(GPS_POWER_OFF_TIMER_NUM, (GPS_ACTIVE_LOCATION_SEARCH_TIME * TICKS_PER_MIN), GpsPowerOffTimerCallBack);
#else /* New option to allow GPS to remain powered */
			if (g_unitConfig.gpsPowerMode != GPS_POWER_ALWAYS_ON_ACQUIRING)
			{
				// Add soft timer to power off
				AssignSoftTimer(GPS_POWER_OFF_TIMER_NUM, (GPS_ACTIVE_LOCATION_SEARCH_TIME * TICKS_PER_MIN), GpsPowerOffTimerCallBack);
			}
#endif
			// Short delay to wait for GPS module
			SoftUsecWait(1 * SOFT_SECS);

			// Query version
			GpsQueryVersion();

			// Short delay to wait for GPS module
			SoftUsecWait(5 * SOFT_MSECS);

			// Change message interval to only pump out GGA, GLL and ZDA messages
			GpsChangeNmeaMsgInterval(1, 0, 0, 1, 0, 0, 1);

			ActivateDisplayShortDuration(3);
			OverlayMessage(getLangText(GPS_LOCATION_TEXT), getLangText(ACQUIRING_SATELLITE_SIGNALS_TEXT), (3 * SOFT_SECS));
		}
		else // GPS is already active
		{
			ActivateDisplayShortDuration(3);
			OverlayMessage(getLangText(GPS_LOCATION_TEXT), getLangText(CONTINUING_ACQUISITION_TEXT), (3 * SOFT_SECS));
		}
#endif
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DisableGps(void)
{
	if (g_gpsOutputToCraft)
	{
		uint16 length = sprintf((char*)g_spareBuffer, "<GPS: POWER OFF>\r\n");
		ModemPuts(g_spareBuffer, length, NO_CONVERSION);
	}

	ClearSoftTimer(GPS_POWER_OFF_TIMER_NUM);

#if 0 /* old hw */

	usart_reset(&AVR32_USART0);

	// Set Baud rate config low to collapse line and prevent back powering the Gps module
	gpio_clr_gpio_pin(AVR32_PIN_PB13); // GPIO 45 (Pin 126 / EBI - CAS)

	// Disable power for Gps module
	gpio_set_gpio_pin(AVR32_PIN_PB14); // GPIO 46 (Pin 127 / EBI - SDWE)

	// Disable power for Usart0
	// Disable Usart0 power (Includes: USART 1, USART 3, TC, TWI, SPI0, SPI1, ADC, PM/RTC/EIC, GPIO, INTC; Disable: ABDAC, SSC, PWM, USART 2, PDCA)
	AVR32_PM.pbamask = 0x00004AFB;
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitGpsBuffers(void)
{
	g_gpsSerialData.readPtr = &g_gpsSerialData.buffer[0];
	g_gpsSerialData.writePtr = &g_gpsSerialData.buffer[0];
	g_gpsSerialData.endPtr = &g_gpsSerialData.buffer[GPS_SERIAL_BUFFER_SIZE];
	g_gpsSerialData.ready = NO;
	g_gpsSerialData.state = GPS_MSG_START;
	g_gpsSerialData.binaryState = GPS_BINARY_MSG_START;

	g_gpsQueue.readIndex = 0;
	g_gpsQueue.writeIndex = 0;
	g_gpsQueue.endIndex = TOTAL_GPS_MESSAGES;
	g_gpsQueue.messageReady = NO;
	memset(&g_gpsQueue.message[0], 0, (sizeof(GPS_MESSAGE) * TOTAL_GPS_MESSAGES));

	g_gpsBinaryQueue.readIndex = 0;
	g_gpsBinaryQueue.writeIndex = 0;
	g_gpsBinaryQueue.endIndex = TOTAL_GPS_BINARY_MESSAGES;
	g_gpsBinaryQueue.binaryMessageReady = NO;
	memset(&g_gpsBinaryQueue.message[0], 0, (sizeof(GPS_BINARY_MESSAGE) * TOTAL_GPS_BINARY_MESSAGES));

	// Rest the GPS Position fix counter
	g_gpsPosition.validLocationCount = 0;

	// Reset the GPS UTC Epoch time counter
	g_epochTimeGPS = 0;

	if (g_sampleProcessing != ACTIVE_STATE)
	{
		g_gpsPosition.locationFoundWhileMonitoring = NO;
		g_gpsPosition.positionFix = NO;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UpdateSystemEpochTimeGps(time_t epochTime)
{
	// Check if the GPS Epoch time is true, meaning that it's greater than the initial internal date & time that the GPS chip powers up with before reading time from a satellite
	if (epochTime > 1672531200) // Epoch date corresponding with the beginning of the year 1/1/2023
	{
		// Update the system GPS Epoch
		g_epochTimeGPS = epochTime;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#define BINARY_RAW_OUTPUT	0
void ProcessGpsSerialData(void)
{
	static uint8 s_pendingDataIndex = 0;
	static uint16 s_binaryPayloadLength = 0;
	static uint8 s_binaryMessageInProgress = NO;
	static uint16 s_binaryPayloadCount = 0;
	uint8 error = NO;
#if BINARY_RAW_OUTPUT
	uint16 messageLength;
#endif

	while (g_gpsSerialData.readPtr != g_gpsSerialData.writePtr)
	{
		//______________________________________________________________________________________________________________
		//
		// Binary message parse
		//______________________________________________________________________________________________________________
		if ((g_gpsSerialData.binaryState == GPS_BINARY_MSG_START) && (*g_gpsSerialData.readPtr == 0xA0))
		{
#if BINARY_RAW_OUTPUT
			messageLength = sprintf((char*)g_spareBuffer, "\r\n========\r\nBinary Message Start\r\n========\r\n");
			ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
#endif
			s_binaryMessageInProgress = YES;

#if BINARY_RAW_OUTPUT
			messageLength = sprintf((char*)g_spareBuffer, "<A0>");
			ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
#endif
			g_gpsSerialData.binaryState = GPS_BINARY_MSG_START_END;
		}
		else if ((g_gpsSerialData.binaryState == GPS_BINARY_MSG_START_END) && (*g_gpsSerialData.readPtr == 0xA1))
		{
#if BINARY_RAW_OUTPUT
			messageLength = sprintf((char*)g_spareBuffer, "<A1>");
			ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
#endif
			g_gpsSerialData.binaryState = GPS_BINARY_MSG_PAYLOAD_START;
		}
		else if (g_gpsSerialData.binaryState == GPS_BINARY_MSG_PAYLOAD_START)
		{
#if BINARY_RAW_OUTPUT
			messageLength = sprintf((char*)g_spareBuffer, "<%x>", *g_gpsSerialData.readPtr);
			ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
#endif

			s_binaryPayloadLength = ((*g_gpsSerialData.readPtr << 8) & 0xFF00);

			g_gpsSerialData.binaryState = GPS_BINARY_MSG_PAYLOAD;
		}
		else if (g_gpsSerialData.binaryState == GPS_BINARY_MSG_PAYLOAD)
		{
#if BINARY_RAW_OUTPUT
			messageLength = sprintf((char*)g_spareBuffer, "<%x>", *g_gpsSerialData.readPtr);
			ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
#endif

			s_binaryPayloadLength |= *g_gpsSerialData.readPtr;
			s_binaryPayloadCount = 0;
			g_gpsBinaryQueue.message[g_gpsBinaryQueue.writeIndex].binMsgSize = s_binaryPayloadLength;

			g_gpsSerialData.binaryState = GPS_BINARY_MSG_BODY;

#if BINARY_RAW_OUTPUT
			messageLength = sprintf((char*)g_spareBuffer, "(L:%d)", s_binaryPayloadLength);
			ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
#endif
		}
		else if (g_gpsSerialData.binaryState == GPS_BINARY_MSG_BODY)
		{
#if BINARY_RAW_OUTPUT
			messageLength = sprintf((char*)g_spareBuffer, "<%x>", *g_gpsSerialData.readPtr);
			ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
#endif
			g_gpsBinaryQueue.message[g_gpsBinaryQueue.writeIndex].data[s_binaryPayloadCount] = *g_gpsSerialData.readPtr;

			if (++s_binaryPayloadCount == s_binaryPayloadLength)
			{
				g_gpsSerialData.binaryState = GPS_BINARY_MSG_CHECKSUM;
			}

			if (s_binaryPayloadCount == GPS_MESSAGE_SIZE)
			{
#if BINARY_RAW_OUTPUT
				messageLength = sprintf((char*)g_spareBuffer, "Overflow <Reseting>\r\n");
				ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
#endif
				memset(&g_gpsBinaryQueue.message[g_gpsBinaryQueue.writeIndex], 0, sizeof(GPS_BINARY_MESSAGE));
				g_gpsSerialData.binaryState = GPS_BINARY_MSG_START;
				s_binaryMessageInProgress = NO;
				s_binaryPayloadCount = 0;
			}
			// Error check for buffer overflow
			// Todo
		}
		else if (g_gpsSerialData.binaryState == GPS_BINARY_MSG_CHECKSUM)
		{
#if BINARY_RAW_OUTPUT
			messageLength = sprintf((char*)g_spareBuffer, "<%x>", *g_gpsSerialData.readPtr);
			ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
#endif

			if (GpsCalcBinaryChecksum(g_gpsBinaryQueue.message[g_gpsBinaryQueue.writeIndex].data, s_binaryPayloadLength) == *g_gpsSerialData.readPtr)
			{
				g_gpsBinaryQueue.message[g_gpsBinaryQueue.writeIndex].binMsgValid = YES;

#if BINARY_RAW_OUTPUT
				messageLength = sprintf((char*)g_spareBuffer, "(Cksm:Pass)");
				ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
#endif
			}
			else
			{
				g_gpsBinaryQueue.message[g_gpsBinaryQueue.writeIndex].binMsgValid = NO;

#if BINARY_RAW_OUTPUT
				messageLength = sprintf((char*)g_spareBuffer, "(Cksm:Fail)");
				ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
#endif
			}

			g_gpsSerialData.binaryState = GPS_BINARY_MSG_END;
		}
		else if (g_gpsSerialData.binaryState == GPS_BINARY_MSG_END)
		{
			if ((*g_gpsSerialData.readPtr == '\r') || (*g_gpsSerialData.readPtr == '\n'))
			{
#if BINARY_RAW_OUTPUT
				messageLength = sprintf((char*)g_spareBuffer, "<End>");
				ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
#endif

				if (g_gpsBinaryQueue.message[g_gpsBinaryQueue.writeIndex].binMsgValid == YES)
				{
					g_gpsBinaryQueue.binaryMessageReady = YES;

					g_gpsBinaryQueue.writeIndex++;
					if (g_gpsBinaryQueue.writeIndex == g_gpsBinaryQueue.endIndex) { g_gpsBinaryQueue.writeIndex = 0; }
				}
				else
				{
					memset(&g_gpsBinaryQueue.message[g_gpsBinaryQueue.writeIndex], 0, sizeof(GPS_BINARY_MESSAGE));
				}

			}
			else
			{
#if BINARY_RAW_OUTPUT
				messageLength = sprintf((char*)g_spareBuffer, "<Err>");
				ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
#endif

				memset(&g_gpsBinaryQueue.message[g_gpsBinaryQueue.writeIndex], 0, sizeof(GPS_BINARY_MESSAGE));
			}

			s_binaryMessageInProgress = NO;

#if BINARY_RAW_OUTPUT
			ModemPutc(0x0A, NO_CONVERSION); ModemPutc(0x0D, NO_CONVERSION);
#endif

			g_gpsSerialData.binaryState = GPS_BINARY_MSG_START;
		}
		//______________________________________________________________________________________________________________
		//
		// NMEA message parse
		//______________________________________________________________________________________________________________
		else if (s_binaryMessageInProgress == NO)
		{
			if ((g_gpsSerialData.state == GPS_MSG_START) && (*g_gpsSerialData.readPtr == '$'))
			{
				g_gpsSerialData.state = GPS_MSG_BODY;
			}
			else if (g_gpsSerialData.state == GPS_MSG_BODY)
			{
				if (*g_gpsSerialData.readPtr != '*')
				{
					g_gpsQueue.message[g_gpsQueue.writeIndex].data[s_pendingDataIndex++] = *g_gpsSerialData.readPtr;
				}
				else
				{
					g_gpsSerialData.state = GPS_MSG_CHECKSUM_FIRST_NIBBLE;
				}
			}
			else if (g_gpsSerialData.state == GPS_MSG_CHECKSUM_FIRST_NIBBLE)
			{
				if (*g_gpsSerialData.readPtr <= '9')
				{
					g_gpsQueue.message[g_gpsQueue.writeIndex].checksum = ((*g_gpsSerialData.readPtr - '0') << 4);
				}
				else if (*g_gpsSerialData.readPtr <= 'F')
				{
					g_gpsQueue.message[g_gpsQueue.writeIndex].checksum = ((*g_gpsSerialData.readPtr - 'A' + 10) << 4);
				}
				else
				{
					g_gpsQueue.message[g_gpsQueue.writeIndex].checksum = ((*g_gpsSerialData.readPtr - 'a' + 10) << 4);
				}

				g_gpsSerialData.state = GPS_MSG_CHECKSUM_SECOND_NIBBLE;
			}
			else if (g_gpsSerialData.state == GPS_MSG_CHECKSUM_SECOND_NIBBLE)
			{
				if (*g_gpsSerialData.readPtr <= '9')
				{
					g_gpsQueue.message[g_gpsQueue.writeIndex].checksum |= ((*g_gpsSerialData.readPtr - '0') & 0x0F);
				}
				else if (*g_gpsSerialData.readPtr <= 'F')
				{
					g_gpsQueue.message[g_gpsQueue.writeIndex].checksum |= ((*g_gpsSerialData.readPtr - 'A' + 10) & 0x0F);
				}
				else
				{
					g_gpsQueue.message[g_gpsQueue.writeIndex].checksum |= ((*g_gpsSerialData.readPtr - 'a' + 10) & 0x0F);
				}

				g_gpsSerialData.state = GPS_MSG_END;
			}
			else if (g_gpsSerialData.state == GPS_MSG_END)
			{
				if ((*g_gpsSerialData.readPtr == '\r') || (*g_gpsSerialData.readPtr == '\n'))
				{
					// Done receiving message
					g_gpsQueue.writeIndex++;

					if (g_gpsQueue.writeIndex == g_gpsQueue.endIndex) { g_gpsQueue.writeIndex = 0; }

					g_gpsSerialData.state = GPS_MSG_START;

					s_pendingDataIndex = 0;

					// Flag for processing Gps message
					g_gpsQueue.messageReady = YES;
				}
				else
				{
					error = YES;
				}
			}
		}

		g_gpsSerialData.readPtr++;

		if (g_gpsSerialData.readPtr == g_gpsSerialData.endPtr) { g_gpsSerialData.readPtr = &g_gpsSerialData.buffer[0]; }

		if ((error == YES) || (s_pendingDataIndex == GPS_MESSAGE_SIZE))
		{
			// Reset
			g_gpsSerialData.state = GPS_MSG_START;
			s_pendingDataIndex = 0;
			memset(&g_gpsQueue.message[g_gpsQueue.writeIndex].data, 0, GPS_MESSAGE_SIZE);
			g_gpsQueue.message[g_gpsQueue.writeIndex].checksum = 0;
		}
	}

	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleGGA(uint8* message)
{
	uint16 messageLength;
	char timeBuff[NMEA_TIMEPARSE_BUF];
	nmeaGPGGA gga;

	memset(&gga, 0, sizeof(nmeaGPGGA));

#if 0
	messageLength = sprintf((char*)g_spareBuffer, "%s\r\n", (char*)message);
	ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
#endif

	//========================================================================================================
	// GGA - Global Positioning System Fix Data
	//========================================================================================================
	// Format: --GGA,hhmmss.sss,llll.llll,a,yyyyy.yyyy,a,x,uu,v.v,w.w,M,x.x,M,,zzzz
	//========================================================================================================
	//<|> Field      <|> Name               <|> Description
	//========================================================================================================
	//<|> hhmmss.sss <|> UTC Time           <|> UTC of position in hhmmss.sss format, (000000.000 ~ 235959.999)
	//<|> llll.llll  <|> Latitude           <|> Latitude in ddmm.mmmm format. Leading zeros are inserted.
	//<|> a          <|> N/S Indicator      <|> �N� = North, �S� = South
	//<|> yyyyy.yyyy <|> Longitude          <|> Longitude in dddmm.mmmm format. Leading zeros are inserted.
	//<|> a          <|> E/W Indicator      <|> 'E' = East, 'W' = West
	//<|> x          <|> GPS quality        <|> GPS quality, 0: Position unavailable, 1: Valid position - SPS mode, 2: Valid position - differential GPS mode
	//<|> uu         <|> Satellites Used    <|> Number of satellites in use, (00 ~ 24)
	//<|> v.v        <|> HDOP               <|> Horizontal dilution of precision, (0.0 ~ 99.9)
	//<|> w.w        <|> Altitude           <|> Mean sea level altitude (-9999.9 ~ 17999.9) in meter
	//<|> x.x        <|> Geoidal Separation <|> In meter
	//<|> zzzz       <|> DGPS Station ID    <|> Differential reference station ID, 0000 ~ 1023, NULL when DGPS not used
	//========================================================================================================

	if (14 != nmea_scanf((char*)message, (int)strlen((char*)message), "GPGGA,%s,%f,%C,%f,%C,%d,%d,%f,%f,%C,%f,%C,%f,%d",
						&timeBuff, &gga.lat, &gga.ns, &gga.lon, &gga.ew, &gga.sig, &gga.satinuse, &gga.HDOP,
						&gga.elv, &gga.elv_units, &gga.diff, &gga.diff_units, &gga.dgps_age, &gga.dgps_sid))
	{
		if (g_gpsOutputToCraft)
		{
			messageLength = sprintf((char*)g_spareBuffer, "<ERROR: GPGGA parse error>\r\n");
			ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
		}
		return;
	}

	if(0 != _nmea_parse_time(&timeBuff[0], (int)strlen(&timeBuff[0]), &gga.utc))
	{
		if (g_gpsOutputToCraft)
		{
			messageLength = sprintf((char*)g_spareBuffer, "<ERROR: GPGGA time parse error>\r\n");
			ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
		}
		return;
	}

#if 0
	messageLength = sprintf((char*)g_spareBuffer, "<SUCCESS: GPGGA parsed correctly>\r\n");
	ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
#endif

	if (g_gpsOutputToCraft)
	{
		messageLength = sprintf((char*)g_spareBuffer, "Lat: %.4f(%c), Lon:%.4f(%c), Time:%02d:%02d:%02d.%02d, GPS-Qual:%d, Sats-Used: %d, Alt:%fm\r\n",
								gga.lat, gga.ns, gga.lon, gga.ew, gga.utc.hour, gga.utc.min, gga.utc.sec, gga.utc.hsec, gga.sig, gga.satinuse, gga.elv);
		ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
	}

	// Check if status says data is valid
	if (gga.sig)
	{
		// Update current position
		ConvertGpsPositionToDegreesMinsSecs(gga.lat, &g_gpsPosition.latDegrees, &g_gpsPosition.latMinutes, &g_gpsPosition.latSeconds);
		ConvertGpsPositionToDegreesMinsSecs(gga.lon, &g_gpsPosition.longDegrees, &g_gpsPosition.longMinutes, &g_gpsPosition.longSeconds);
		g_gpsPosition.northSouth = gga.ns;
		g_gpsPosition.eastWest = gga.ew;
		g_gpsPosition.utcHour = gga.utc.hour;
		g_gpsPosition.utcMin = gga.utc.min;
		g_gpsPosition.utcSec = gga.utc.sec;
		g_gpsPosition.altitude = ((int)gga.elv);
		g_gpsPosition.validLocationCount++;

		if (g_epochTimeGPS == 0)
		{
			// Either call GpsQueryTime or set a flag to handle it outside of scope
			GpsQueryTime();
		}

		if (g_gpsPosition.validLocationCount == GPS_THRESHOLD_TOTAL_FIXES_FOR_BEST_LOCATION)
		{
			g_gpsPosition.positionFix = YES;

#if 0 /* Original */
			DisableGps();
#else /* New option to allow GPS to remain powered */
			if (g_unitConfig.gpsPowerMode != GPS_POWER_ALWAYS_ON_ACQUIRING)
			{
				DisableGps();
			}
#endif
			ActivateDisplayShortDuration(3);
			sprintf((char*)g_spareBuffer, "Lat %.4f(%c) Lon %.4f(%c)", gga.lat, gga.ns, gga.lon, gga.ew);
			OverlayMessage(getLangText(GPS_LOCATION_FIX_TEXT), (char*)g_spareBuffer, (3 * SOFT_SECS));
#if 0 /* Original */
			// Set timer to re-check GPS location in the future
			AssignSoftTimer(GPS_POWER_ON_TIMER_NUM, (GPS_REACTIVATION_TIME_NORMAL * TICKS_PER_MIN), GpsPowerOnTimerCallBack);
#else /* New option to allow GPS to remain powered */
			if (g_unitConfig.gpsPowerMode != GPS_POWER_ALWAYS_ON_ACQUIRING)
			{
				// Set timer to re-check GPS location in the future
				AssignSoftTimer(GPS_POWER_ON_TIMER_NUM, (GPS_REACTIVATION_TIME_NORMAL * TICKS_PER_MIN), GpsPowerOnTimerCallBack);
			}
#endif
		}

		if (g_sampleProcessing == ACTIVE_STATE)
		{
			g_gpsPosition.locationFoundWhileMonitoring = YES;
			g_pendingEventRecord.summary.calculated.gpsPosition = g_gpsPosition;
			g_pendingBargraphRecord.summary.calculated.gpsPosition = g_gpsPosition;
		}
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleGLL(uint8* message)
{
	uint16 messageLength;
	char timeBuff[NMEA_TIMEPARSE_BUF];
	nmeaGPGLL gll;

	memset(&gll, 0, sizeof(nmeaGPGLL));

#if 0
	messageLength = sprintf((char*)g_spareBuffer, "%s\r\n", (char*)message);
	ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
#endif

	//========================================================================================================
	// GLL � Geographic Position � Latitude/Longitude
	//========================================================================================================
	// Format: --GLL,llll.llll,a,yyyyy.yyyy,b,hhmmss.sss,A,a
	// Sample: GPGLL,3259.6000,N,09640.8761,W,084429.000,A,A
	//========================================================================================================
	//<|> Field      <|> Name             <|> Description
	//========================================================================================================
	//<|> llll.llll  <|> Latitude         <|> Latitude in ddmm.mmmm format. Leading zeros are inserted
	//<|> a          <|> N/S Indicator    <|> �N� = North, �S� = South
	//<|> yyyyy.yyyy <|> Longitude        <|> Longitude in dddmm.mmmm format. Leading zeros are inserted
	//<|> b          <|> E/W Indicator    <|> 'E' = East, 'W' = West
	//<|> hhmmss.sss <|> UTC Time         <|> UTC of position in hhmmss.sss format, (000000.000 ~ 235959.999)
	//<|> A          <|> Status           <|> A= data valid, V= Data not valid
	//<|> A          <|> Signal integrity <|> A=autonomous, D=differential, E=Estimated, N=not valid, S=Simulator
	//========================================================================================================

	if (7 != nmea_scanf((char*)message, (int)strlen((char*)message), "GPGLL,%f,%C,%f,%C,%s,%c,%c", &gll.lat, &gll.ns, &gll.lon, &gll.ew, &timeBuff, &gll.sta, &gll.sig))
	{
		if (g_gpsOutputToCraft)
		{
			messageLength = sprintf((char*)g_spareBuffer, "<ERROR: GPGLL parse error>\r\n");
			ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
		}
		return;
	}

	if(0 != _nmea_parse_time(&timeBuff[0], (int)strlen(&timeBuff[0]), &gll.utc))
	{
		if (g_gpsOutputToCraft)
		{
			messageLength = sprintf((char*)g_spareBuffer, "<ERROR: GPGLL time parse error>\r\n");
			ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
		}
		return;
	}

#if 0
	messageLength = sprintf((char*)g_spareBuffer, "<SUCCESS: GPGLL parsed correctly>\r\n");
	ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
#endif

	if (g_gpsOutputToCraft)
	{
		messageLength = sprintf((char*)g_spareBuffer, "Lat: %.4f(%c), Lon:%.4f(%c), Time:%02d:%02d:%02d.%02d, Stat: %c, SI: %c\r\n", gll.lat, gll.ns, gll.lon, gll.ew,
								gll.utc.hour, gll.utc.min, gll.utc.sec, gll.utc.hsec, gll.sta, gll.sig);
		ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
	}

	// Check if status says data is valid
	if (gll.sta == 'A')
	{
		// Update current position
		ConvertGpsPositionToDegreesMinsSecs(gll.lat, &g_gpsPosition.latDegrees, &g_gpsPosition.latMinutes, &g_gpsPosition.latSeconds);
		ConvertGpsPositionToDegreesMinsSecs(gll.lon, &g_gpsPosition.longDegrees, &g_gpsPosition.longMinutes, &g_gpsPosition.longSeconds);
		g_gpsPosition.northSouth = gll.ns;
		g_gpsPosition.eastWest = gll.ew;
		g_gpsPosition.utcHour = gll.utc.hour;
		g_gpsPosition.utcMin = gll.utc.min;
		g_gpsPosition.utcSec = gll.utc.sec;
		g_gpsPosition.validLocationCount++;

		if (g_epochTimeGPS == 0)
		{
			// Either call GpsQueryTime or set a flag to handle it outside of scope
			GpsQueryTime();
		}

		if (g_gpsPosition.validLocationCount == GPS_THRESHOLD_TOTAL_FIXES_FOR_BEST_LOCATION)
		{
			g_gpsPosition.positionFix = YES;

#if 0 /* Original */
			DisableGps();
#else /* New option to allow GPS to remain powered */
			if (g_unitConfig.gpsPowerMode != GPS_POWER_ALWAYS_ON_ACQUIRING)
			{
				DisableGps();
			}
#endif
			ActivateDisplayShortDuration(3);
			sprintf((char*)g_spareBuffer, "Lat %.4f(%c) Lon %.4f(%c)", gll.lat, gll.ns, gll.lon, gll.ew);
			OverlayMessage(getLangText(GPS_LOCATION_FIX_TEXT), (char*)g_spareBuffer, (3 * SOFT_SECS));
#if 0 /* Original */
			// Set timer to re-check GPS location in the future
			AssignSoftTimer(GPS_POWER_ON_TIMER_NUM, (GPS_REACTIVATION_TIME_NORMAL * TICKS_PER_MIN), GpsPowerOnTimerCallBack);
#else /* New option to allow GPS to remain powered */
			if (g_unitConfig.gpsPowerMode != GPS_POWER_ALWAYS_ON_ACQUIRING)
			{
				// Set timer to re-check GPS location in the future
				AssignSoftTimer(GPS_POWER_ON_TIMER_NUM, (GPS_REACTIVATION_TIME_NORMAL * TICKS_PER_MIN), GpsPowerOnTimerCallBack);
			}
#endif
		}

		if (g_sampleProcessing == ACTIVE_STATE)
		{
			g_gpsPosition.locationFoundWhileMonitoring = YES;
			g_pendingEventRecord.summary.calculated.gpsPosition = g_gpsPosition;
			g_pendingBargraphRecord.summary.calculated.gpsPosition = g_gpsPosition;
		}
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleZDA(uint8* message)
{
	uint16 messageLength;
	char timeBuff[NMEA_TIMEPARSE_BUF];
	nmeaGPZDA zda;
	struct tm currentTime;
	time_t epochTime;

	//========================================================================================================
	// ZDA - Time and Date (UTC, day, month, year and local time zone)
	//========================================================================================================
	// Format: --ZDA,hhmmss.sss,dd,mm,yyyy,xx,yy*hh<CR><LF>
	//========================================================================================================
	//<|> Field      <|> Name             <|> Description
	//========================================================================================================
	//<|> hhmmss.sss <|> UTC Time         <|> UTC of position in hhmmss.sss format, (000000.000 ~ 235959.999)
	//<|> dd		 <|> UTC day		  <|> 01 to 31
	//<|> mm		 <|> UTC month		  <|> 01 to 12
	//<|> yyyy		 <|> UTC year		  <|> Four-digit year number
	//<|> xx		 <|> Local zone hours <|> 00 to +-13
	//<|> yy		 <|> Local zone mins  <|> 00 to +59
	//========================================================================================================

	if (6 != nmea_scanf((char*)message, (int)strlen((char*)message), "GPZDA,%s,%d,%d,%d,%d,%d",
						&timeBuff, &zda.utc.day, &zda.utc.mon, &zda.utc.year, &zda.lzHours, &zda.lzMins))
	{
		if (g_gpsOutputToCraft)
		{
			messageLength = sprintf((char*)g_spareBuffer, "<ERROR: GPZDA parse error>\r\n");
			ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
		}
		return;
	}

	if(0 != _nmea_parse_time(&timeBuff[0], (int)strlen(&timeBuff[0]), &zda.utc))
	{
		if (g_gpsOutputToCraft)
		{
			messageLength = sprintf((char*)g_spareBuffer, "<ERROR: GPZDA time parse error>\r\n");
			ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
		}
		return;
	}

#if 0
	messageLength = sprintf((char*)g_spareBuffer, "<SUCCESS: GPZDA parsed correctly>\r\n");
	ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
#endif

	// Example --> GPS Date: 07-03-2023, Time:19:24:14
	currentTime.tm_year = (zda.utc.year - 1900); // From 1900;
	currentTime.tm_mon = (zda.utc.mon - 1); // Month, 0 - jan
	currentTime.tm_mday = zda.utc.day; // Day of the month
	currentTime.tm_hour = zda.utc.hour;
	currentTime.tm_min = zda.utc.min;
	currentTime.tm_sec = zda.utc.sec;
	currentTime.tm_isdst = -1; // Is DST on? 1 = yes, 0 = no, -1 = unknown

	// Convert current time into Epoch time
	epochTime = mktime(&currentTime);

#if 0 /* Removed updating GPS Epoch time since every 2m 11s (131s) the GPS chip lags for 4s and upon message resumption the first ZDA message reports a second old before catching up */
	UpdateSystemEpochTimeGps(epochTime);
#endif

	if (g_gpsOutputToCraft)
	{
		messageLength = sprintf((char*)g_spareBuffer, "GPS Date: %02d-%02d-%04d, Time:%02d:%02d:%02d, GPS Epoch: %lu\r\n", zda.utc.day, zda.utc.mon, zda.utc.year, zda.utc.hour, zda.utc.min, zda.utc.sec, epochTime);
		ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
	}

	g_gpsPosition.utcYear = zda.utc.year;
	g_gpsPosition.utcMonth = zda.utc.mon;
	g_gpsPosition.utcDay = zda.utc.day;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ProcessGpsMessage(void)
{
	uint8 messageValidated = NO;
	uint16 messageLength = 0;
	int checksum;
	uint8 gpsIndex;

	while (g_gpsQueue.readIndex != g_gpsQueue.writeIndex)
	{
		checksum = GpsChecksum(&g_gpsQueue.message[g_gpsQueue.readIndex].data[0]);

		// Check the message checksum
		if (g_gpsQueue.message[g_gpsQueue.readIndex].checksum == checksum)
		{
			messageValidated = YES;
		}

		if (messageValidated == YES)
		{
#if 0 /* Test */
			messageLength = sprintf((char*)g_spareBuffer, "%s\r\n", (char*)&g_gpsQueue.message[g_gpsQueue.readIndex].data[0]);
			ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
#endif
			CHAR_UPPER_CASE(g_gpsQueue.message[g_gpsQueue.readIndex].data[2]);
			CHAR_UPPER_CASE(g_gpsQueue.message[g_gpsQueue.readIndex].data[3]);
			CHAR_UPPER_CASE(g_gpsQueue.message[g_gpsQueue.readIndex].data[4]);

			for (gpsIndex = 0; gpsIndex < TOTAL_GPS_COMMANDS; gpsIndex++)
			{
				if ((g_gpsQueue.message[g_gpsQueue.readIndex].data[2] == s_gpsMessageTable[gpsIndex].cmdChar1) &&
					(g_gpsQueue.message[g_gpsQueue.readIndex].data[3] == s_gpsMessageTable[gpsIndex].cmdChar2) &&
					(g_gpsQueue.message[g_gpsQueue.readIndex].data[4] == s_gpsMessageTable[gpsIndex].cmdChar3))
				{
					s_gpsMessageTable[gpsIndex].cmdFunction(&g_gpsQueue.message[g_gpsQueue.readIndex].data[0]);
					break;
				}
			}

			if (gpsIndex == TOTAL_GPS_COMMANDS)
			{
				if (g_gpsOutputToCraft)
				{
					messageLength = sprintf((char*)g_spareBuffer, "[%s]\r\n", (char*)&g_gpsQueue.message[g_gpsQueue.readIndex].data[0]);
					ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
				}
			}
		}
		else
		{
			if (g_gpsOutputToCraft)
			{
				messageLength = sprintf((char*)g_spareBuffer, "%s (0x%x<>0x%x)\r\n", (char*)&g_gpsQueue.message[g_gpsQueue.readIndex].data[0], g_gpsQueue.message[g_gpsQueue.readIndex].checksum,checksum);
				ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
			}
		}

		// Clear message after use
		memset(&g_gpsQueue.message[g_gpsQueue.readIndex], 0, sizeof(GPS_MESSAGE));

		// Increment read index
		g_gpsQueue.readIndex++;

		if (g_gpsQueue.readIndex == g_gpsQueue.endIndex) { g_gpsQueue.readIndex = 0; }
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ProcessGpsBinaryMessage(void)
{
	uint8 gpsBinaryIndex = 0;
	uint16 messageLength;

	while (g_gpsBinaryQueue.readIndex != g_gpsBinaryQueue.writeIndex)
	{
		for (gpsBinaryIndex = 0; gpsBinaryIndex < TOTAL_GPS_BINARY_COMMANDS; gpsBinaryIndex++)
		{
			// Look for a match of the Command ID
			if (s_gpsBinaryMessageTable[gpsBinaryIndex].cmdID == g_gpsBinaryQueue.message[g_gpsBinaryQueue.readIndex].data[0])
			{
				// Check if the Sub ID is zero (meaning no Sub ID) or if the Sub ID matches the Command Sub ID
				if ((s_gpsBinaryMessageTable[gpsBinaryIndex].cmdSubID == 0) || (s_gpsBinaryMessageTable[gpsBinaryIndex].cmdSubID) == g_gpsBinaryQueue.message[g_gpsBinaryQueue.readIndex].data[1])
				{
					s_gpsBinaryMessageTable[gpsBinaryIndex].cmdFunction();
					break;
				}
			}
		}

		if (gpsBinaryIndex == TOTAL_GPS_BINARY_COMMANDS)
		{
			if (g_gpsOutputToCraft)
			{
				messageLength = sprintf((char*)g_spareBuffer, "[Binary Msg not recognized][%x,%x,%x,%x,%x,%x,%x,%x]\r\n",
										g_gpsBinaryQueue.message[g_gpsBinaryQueue.readIndex].data[0],
										g_gpsBinaryQueue.message[g_gpsBinaryQueue.readIndex].data[1],
										g_gpsBinaryQueue.message[g_gpsBinaryQueue.readIndex].data[2],
										g_gpsBinaryQueue.message[g_gpsBinaryQueue.readIndex].data[3],
										g_gpsBinaryQueue.message[g_gpsBinaryQueue.readIndex].data[4],
										g_gpsBinaryQueue.message[g_gpsBinaryQueue.readIndex].data[5],
										g_gpsBinaryQueue.message[g_gpsBinaryQueue.readIndex].data[6],
										g_gpsBinaryQueue.message[g_gpsBinaryQueue.readIndex].data[7]);
				ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
			}
		}

		// Clear message after use
		memset(&g_gpsBinaryQueue.message[g_gpsBinaryQueue.readIndex], 0, sizeof(GPS_BINARY_MESSAGE));

		// Increment read index
		g_gpsBinaryQueue.readIndex++;

		if (g_gpsBinaryQueue.readIndex == g_gpsBinaryQueue.endIndex) { g_gpsBinaryQueue.readIndex = 0; }
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 1
int nmea_atoi(const char *str, int str_sz, int radix)
{
	char *tmp_ptr;
	char buff[NMEA_CONVSTR_BUF];
	int res = 0;

	if(str_sz < NMEA_CONVSTR_BUF)
	{
		memcpy(&buff[0], str, str_sz);
		buff[str_sz] = '\0';
		res = strtol(&buff[0], &tmp_ptr, radix);
	}

	return res;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
double nmea_atof(const char *str, int str_sz)
{
	char *tmp_ptr;
	char buff[NMEA_CONVSTR_BUF];
	double res = 0;

	if(str_sz < NMEA_CONVSTR_BUF)
	{
		memcpy(&buff[0], str, str_sz);
		buff[str_sz] = '\0';
		res = strtod(&buff[0], &tmp_ptr);
	}

	return res;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void nmea_error(const char *str)
{
	UNUSED(str);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int nmea_scanf(const char *buff, int buff_sz, const char *format, ...)
{
	const char *beg_tok;
	const char *end_buf = buff + buff_sz;

	va_list arg_ptr;
	int tok_type = NMEA_TOKS_COMPARE;
	int width = 0;
	const char *beg_fmt = 0;
	int snum = 0, unum = 0;

	int tok_count = 0;
	void *parg_target;

	va_start(arg_ptr, format);

	for(; *format && buff < end_buf; ++format)
	{
		switch(tok_type)
		{
			case NMEA_TOKS_COMPARE:
			if('%' == *format)
			tok_type = NMEA_TOKS_PERCENT;
			else if(*buff++ != *format)
			goto fail;
			break;
			case NMEA_TOKS_PERCENT:
			width = 0;
			beg_fmt = format;
			tok_type = NMEA_TOKS_WIDTH;
			case NMEA_TOKS_WIDTH:
			if(isdigit(*format))
			break;
			{
				tok_type = NMEA_TOKS_TYPE;
				if(format > beg_fmt)
				width = nmea_atoi(beg_fmt, (int)(format - beg_fmt), 10);
			}
			case NMEA_TOKS_TYPE:
			beg_tok = buff;

			if(!width && ('c' == *format || 'C' == *format) && *buff != format[1])
			width = 1;

			if(width)
			{
				if(buff + width <= end_buf)
				buff += width;
				else
				goto fail;
			}
			else
			{
				if(!format[1] || (0 == (buff = (char *)memchr(buff, format[1], end_buf - buff))))
				buff = end_buf;
			}

			if(buff > end_buf)
			goto fail;

			tok_type = NMEA_TOKS_COMPARE;
			tok_count++;

			parg_target = 0; width = (int)(buff - beg_tok);

			switch(*format)
			{
				case 'c':
				case 'C':
				parg_target = (void *)va_arg(arg_ptr, char *);
				if(width && 0 != (parg_target))
				*((char *)parg_target) = *beg_tok;
				break;
				case 's':
				case 'S':
				parg_target = (void *)va_arg(arg_ptr, char *);
				if(width && 0 != (parg_target))
				{
					memcpy(parg_target, beg_tok, width);
					((char *)parg_target)[width] = '\0';
				}
				break;
				case 'f':
				case 'g':
				case 'G':
				case 'e':
				case 'E':
				parg_target = (void *)va_arg(arg_ptr, double *);
				if(width && 0 != (parg_target))
				*((double *)parg_target) = nmea_atof(beg_tok, width);
				break;
			};

			if(parg_target)
			break;
			if(0 == (parg_target = (void *)va_arg(arg_ptr, int *)))
			break;
			if(!width)
			break;

			switch(*format)
			{
				case 'd':
				case 'i':
				snum = nmea_atoi(beg_tok, width, 10);
				memcpy(parg_target, &snum, sizeof(int));
				break;
				case 'u':
				unum = nmea_atoi(beg_tok, width, 10);
				memcpy(parg_target, &unum, sizeof(unsigned int));
				break;
				case 'x':
				case 'X':
				unum = nmea_atoi(beg_tok, width, 16);
				memcpy(parg_target, &unum, sizeof(unsigned int));
				break;
				case 'o':
				unum = nmea_atoi(beg_tok, width, 8);
				memcpy(parg_target, &unum, sizeof(unsigned int));
				break;
				default:
				goto fail;
			};

			break;
		};
	}

	fail:

	va_end(arg_ptr);

	return tok_count;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int _nmea_parse_time(const char *buff, int buff_sz, nmeaTIME *res)
{
	int success = 0;

	switch(buff_sz)
	{
		case sizeof("hhmmss") - 1:
			success = (3 == nmea_scanf(buff, buff_sz, "%2d%2d%2d", &(res->hour), &(res->min), &(res->sec)));
		break;

		case sizeof("hhmmss.s") - 1:
		case sizeof("hhmmss.ss") - 1:
		case sizeof("hhmmss.sss") - 1:
			success = (4 == nmea_scanf(buff, buff_sz, "%2d%2d%2d.%d", &(res->hour), &(res->min), &(res->sec), &(res->hsec)));
		break;

		default:
			nmea_error("Parse of time error (format error)!");
			success = 0;
		break;
	}

	return (success ? 0 : -1);
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 GpsCalcBinaryChecksum(uint8* binaryPayload, uint16 payloadLength)
{
	uint8 checksum = 0;
	uint16 i = 0;

	for (; i < payloadLength; i++)
	{
		checksum ^= binaryPayload[i];
	}

	return (checksum);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GpsSendBinaryMessage(uint8* binaryMessage, uint16 messageLength)
{
	uint8 checksum = 0;
	uint16 i = 4;

	// Calculate checksum of the message payload which starts with the 5th byte
#if 0
	for (i = 4; i < messageLength; i++)
	{
		checksum ^= binaryMessage[i];
	}
#else
	checksum = GpsCalcBinaryChecksum(&binaryMessage[4], (messageLength - 4));
#endif

#if 0 /* old hw */

	for (i = 0; i < messageLength; i++)
	{
		while (usart_write_char(&AVR32_USART0, binaryMessage[i]) != USART_SUCCESS) {}
	}

	while (usart_write_char(&AVR32_USART0, checksum) != USART_SUCCESS) {}
	while (usart_write_char(&AVR32_USART0, 0x0D) != USART_SUCCESS) {}
	while (usart_write_char(&AVR32_USART0, 0x0A) != USART_SUCCESS) {}
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GpsQueryVersion(void)
{
/*
	=================================================
	Query Version
	-------------------------------------------------
	In : a0 a1 00 02 02 01 03 0d 0a
	Ack: a0 a1 00 02 83 02 81 0d 0a
	Out: a0 a1 00 0e 80 01 00 02 00 4a 00 01 07 1b 00 0e 07 15 c8 0d 0a
	Query Version Successful...
	Kernel Version 2.0.74
	Software Version 1.7.27
	Revision 2014.7.21
*/
	uint8 queryVersionMessage[] = {0xA0, 0xA1, 0x00, 0x02, 0x02, 0x01};
	uint8 messageSize = sizeof(queryVersionMessage);

	GpsSendBinaryMessage(queryVersionMessage, messageSize);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GpsQueryTime(void)
{
/*
	=================================================
	Query Time
	-------------------------------------------------
	In : A0 A1 00 02 64 20 44 0D 0A
*/
	uint8 queryTimeMessage[] = {0xA0, 0xA1, 0x00, 0x02, 0x64, 0x20};
	uint8 messageSize = sizeof(queryTimeMessage);

	if (g_gpsOutputToCraft)
	{
		uint16 messageLength = sprintf((char*)g_spareBuffer, "GPS Query Time...\r\n");
		ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
	}

	GpsSendBinaryMessage(queryTimeMessage, messageSize);
}

#if 0
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GpsQueryUTCDate(void)
{
/*
	=================================================
	Query Time
	-------------------------------------------------
	In : A0 A1 00 02 64 16 72 0D 0A
*/
	uint8 queryUTCDateMessage[] = {0xA0, 0xA1, 0x00, 0x02, 0x64, 0x16};
	uint8 messageSize = sizeof(queryUTCDateMessage);

	if (g_gpsOutputToCraft)
	{
		uint16 messageLength = sprintf((char*)g_spareBuffer, "GPS Query UTC Date...\r\n");
		ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
	}

	GpsSendBinaryMessage(queryUTCDateMessage, messageSize);
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GpsDumpBinaryMessage(void)
{
	uint16 messageLength;
	uint8 i = 0;

	for (; i < g_gpsBinaryQueue.message[g_gpsBinaryQueue.readIndex].binMsgSize; i++)
	{
		if (g_gpsOutputToCraft)
		{
			messageLength = sprintf((char*)g_spareBuffer, "<%x>", g_gpsBinaryQueue.message[g_gpsBinaryQueue.readIndex].data[i]);
			ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
		}
	}

	if (g_gpsOutputToCraft)
	{
		ModemPutc(0x0D, NO_CONVERSION);
		ModemPutc(0x0A, NO_CONVERSION);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleVersionQuery(void)
{
	uint16 messageLength;

	if (g_gpsBinaryQueue.message[g_gpsBinaryQueue.readIndex].binMsgValid == YES)
	{
		messageLength = sprintf((char*)g_spareBuffer, "Version Query received\r\n");
	}
	else
	{
		messageLength = sprintf((char*)g_spareBuffer, "Version Query not valid\r\n");
	}

	if (g_gpsOutputToCraft)
	{
		ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);

		GpsDumpBinaryMessage();
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleSoftCrcQuery(void)
{
	uint16 messageLength;

	if (g_gpsBinaryQueue.message[g_gpsBinaryQueue.readIndex].binMsgValid == YES)
	{
		messageLength = sprintf((char*)g_spareBuffer, "Soft CRC Query received\r\n");
	}
	else
	{
		messageLength = sprintf((char*)g_spareBuffer, "Soft CRC  Query not valid\r\n");
	}

	if (g_gpsOutputToCraft)
	{
		ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);

		GpsDumpBinaryMessage();
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleNmeaMsgIntervalQuery(void)
{
	uint16 messageLength;

	if (g_gpsBinaryQueue.message[g_gpsBinaryQueue.readIndex].binMsgValid == YES)
	{
		messageLength = sprintf((char*)g_spareBuffer, "NMEA Msg Interval Query received\r\n");
	}
	else
	{
		messageLength = sprintf((char*)g_spareBuffer, "NMEA Msg Interval Query not valid\r\n");
	}

	if (g_gpsOutputToCraft)
	{
		ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);

		GpsDumpBinaryMessage();
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleGPSQueryTime(void)
{
	uint16 messageLength;
	uint32 timeOfWeekMS;
	//uint32 subTimeOfWeekNS;
	uint16 weeks;
	uint8 leapSeconds;
	uint8* msgData = &g_gpsBinaryQueue.message[g_gpsBinaryQueue.readIndex].data[0];
	time_t epochTime;
	DATE_TIME_STRUCT tempTime;

	if (g_gpsBinaryQueue.message[g_gpsBinaryQueue.readIndex].binMsgValid == YES)
	{
		messageLength = sprintf((char*)g_spareBuffer, "GPS Time Query received\r\n");
	}
	else
	{
		messageLength = sprintf((char*)g_spareBuffer, "GPS Time Query not valid\r\n");
	}

	if (g_gpsOutputToCraft)
	{
		ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);

		GpsDumpBinaryMessage();
	}

	// Offset between 1.1.1970 UTC and 6.1.1980 UTC in seconds: 315964800

	//Example: GPS Time Query response
	//<64><8e><1f><56><bc><4f><0><1><14><40><8><27><10><12><7>
	timeOfWeekMS = ((msgData[2] << 24) + (msgData[3] << 16) + (msgData[4] << 8) + (msgData[5]));
	//subTimeOfWeekNS = ((msgData[6] << 24) + (msgData[7] << 16) + (msgData[8] << 8) + (msgData[9]));
	weeks = ((msgData[10] << 8) + (msgData[11]));
	leapSeconds = msgData[13]; // Better to use active Leap seconds than default Leap seconds
	//fractionSecs = (((timeOfWeekMS % 1000) * 1000 * 1000) + (subTimeOfWeekNS));

	// 315964800 is the correct offset between 1.1.1970 UTC and 6.1.1980 UTC where GPS Time began, verified the correct time correlates with subtracting leap seconds
	epochTime = (315964800 + ((weeks * 7) * 86400) + (timeOfWeekMS / 1000) - leapSeconds);

	UpdateSystemEpochTimeGps(epochTime);

	if (g_gpsOutputToCraft)
	{
		tempTime = ConvertEpochTimeToDateTime(epochTime);
		messageLength = sprintf((char*)g_spareBuffer, "GPS current Epoch Time: %ld seconds, (%d/%d/%d @ %02d:%02d:%02d GMT time zone)\r\n", epochTime, tempTime.month, tempTime.day, tempTime.year, tempTime.hour, tempTime.min, tempTime.sec);
		ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
	}
}

#if 0
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleUTCDateQuery(void)
{
	uint16 messageLength;
	uint16 utcYear;
	uint8 utcMonth;
	uint8 utcDay;
	uint8* msgData = &g_gpsBinaryQueue.message[g_gpsBinaryQueue.readIndex].data[0];

	if (g_gpsBinaryQueue.message[g_gpsBinaryQueue.readIndex].binMsgValid == YES)
	{
		messageLength = sprintf((char*)g_spareBuffer, "UTC Date Query received\r\n");
	}
	else
	{
		messageLength = sprintf((char*)g_spareBuffer, "UTC Date Query not valid\r\n");
}

	if (1) //(g_gpsOutputToCraft)
	{
		ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);

		GpsDumpBinaryMessage();
	}

	//Example: GPS Time Query response
	//<64><8a><01><07><dd><01><01>
	utcYear = ((msgData[3] << 8) + (msgData[4]));
	utcMonth = msgData[5];
	utcDay = msgData[6];

	messageLength = sprintf((char*)g_spareBuffer, "UTC Date: %s, %02d, %4d\r\n", g_monthTable[utcMonth].name, utcDay, utcYear);
	ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GpsQuerySoftCrc(void)
{
/*
	=================================================
	Query CRC
	-------------------------------------------------
	In : a0 a1 00 02 03 01 02 0d 0a
	Ack: a0 a1 00 02 83 03 80 0d 0a
	Out: a0 a1 00 04 81 01 ec c7 ab 0d 0a
	Query CRC Successful...
	System CRC: ecc7
*/
	uint8 querySoftCrcMessage[] = {0xA0, 0xA1, 0x00, 0x02, 0x03, 0x01};
	uint8 messageSize = sizeof(querySoftCrcMessage);

	GpsSendBinaryMessage(querySoftCrcMessage, messageSize);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GpsQueryNmeaMsgInterval(void)
{
/*
	=================================================
	Query NMEA Message Interval
	-------------------------------------------------
	In : a0 a1 00 02 64 03 67 0d 0a
	Ack: a0 a1 00 03 83 64 03 e4 0d 0a
	Out: a0 a1 00 0e 64 81 01 01 03 01 01 01 01 00 00 00 00 00 e6 0d 0a
	Query NMEA Message Interval Successful
	GGA Interval : 1 second(s)
	GSA Interval : 1 second(s)
	GSV Interval : 3 second(s)
	GLL Interval : 1 second(s)
	RMC Interval : 1 second(s)
	VTG Interval : 1 second(s)
	ZDA Interval : 1 second(s)
*/
	uint8 queryNmeaMsgIntervalMessage[] = {0xA0, 0xA1, 0x00, 0x02, 0x64, 0x03};
	uint8 messageSize = sizeof(queryNmeaMsgIntervalMessage);

	GpsSendBinaryMessage(queryNmeaMsgIntervalMessage, messageSize);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GpsChangeNmeaMsgInterval(uint8 GGAInt, uint8 GSAInt, uint8 GSVInt, uint8 GLLInt, uint8 RMCInt, uint8 VTGInt, uint8 ZDAInt)
{
/*
	=================================================
	Configure NMEA Message Interval
	-------------------------------------------------
	In : a0 a1 00 0f 64 02 01 00 00 01 00 00 00 00 00 00 00 00 00 66 0d 0a
	Ack: a0 a1 00 03 83 64 02 e5 0d 0a
	Configure NMEA Successful...
*/
	uint8 queryNmeaMsgIntervalMessage[] = {0xa0, 0xa1, 0x00, 0x0f, 0x64, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	uint8 messageSize = sizeof(queryNmeaMsgIntervalMessage);

	queryNmeaMsgIntervalMessage[6] = GGAInt;
	queryNmeaMsgIntervalMessage[7] = GSAInt;
	queryNmeaMsgIntervalMessage[8] = GSVInt;
	queryNmeaMsgIntervalMessage[9] = GLLInt;
	queryNmeaMsgIntervalMessage[10] = RMCInt;
	queryNmeaMsgIntervalMessage[11] = VTGInt;
	queryNmeaMsgIntervalMessage[12] = ZDAInt;

	GpsSendBinaryMessage(queryNmeaMsgIntervalMessage, messageSize);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleBinaryMsgAck(void)
{
	uint16 messageLength;

	if (g_gpsBinaryQueue.message[g_gpsBinaryQueue.readIndex].binMsgValid == YES)
	{
		messageLength = sprintf((char*)g_spareBuffer, "NMEA Ack received\r\n");
	}
	else
	{
		messageLength = sprintf((char*)g_spareBuffer, "NMEA Ack not valid\r\n");
	}

	if (g_gpsOutputToCraft)
	{
		ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);

		GpsDumpBinaryMessage();
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleBinaryMsgNack(void)
{
	uint16 messageLength;

	if (g_gpsBinaryQueue.message[g_gpsBinaryQueue.readIndex].binMsgValid == YES)
	{
		messageLength = sprintf((char*)g_spareBuffer, "NMEA Nack received\r\n");
	}
	else
	{
		messageLength = sprintf((char*)g_spareBuffer, "NMEA Nack not valid\r\n");
	}

	if (g_gpsOutputToCraft)
	{
		ModemPuts(g_spareBuffer, messageLength, NO_CONVERSION);

		GpsDumpBinaryMessage();
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GpsChangeSerialBaud(void)
{
/*
	=================================================
	Configure Serial Port (115200 SRAM only)
	-------------------------------------------------
	In : a0 a1 00 04 05 00 05 00 00 0d 0a
	Ack: a0 a1 00 02 83 05 86 0d 0a
	Configure Serial Port Successful...
*/
	uint8 querySoftCrcMessage[] = {0xA0, 0xA1, 0x00, 0x04, 0x05, 0x00, 0x05, 0x00};
	uint8 messageSize = sizeof(querySoftCrcMessage);

	GpsSendBinaryMessage(querySoftCrcMessage, messageSize);
}
