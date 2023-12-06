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
#include <math.h>
#include "Common.h"
#include "OldUart.h"
#include "Display.h"
#include "Menu.h"
#include "SoftTimer.h"
#include "PowerManagement.h"
#include "Keypad.h"
#include "SysEvents.h"
#include "TextTypes.h"
#include "adc.h"
#include "ctype.h"

#include "mxc_delay.h"
#include "ff.h"
//#include "usart.h"
//#include "usb_drv.h"
//#include "usb_task.h"
//#include "device_mass_storage_task.h"
//#include "host_mass_storage_task.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define N_BITS 32
#define MAX_BIT ((N_BITS + 1) / 2 - 1)

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
#include "Globals.h"
extern const char applicationVersion[];
extern const char applicationDate[];

///----------------------------------------------------------------------------
///	Local Scope Globals
///----------------------------------------------------------------------------
static INPUT_MSG_STRUCT* s_inputWritePtr = &(g_input_buffer[0]);
static INPUT_MSG_STRUCT* s_inputReadPtr = &(g_input_buffer[0]);

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#define AD_VOLTAGE_READ_LOOP_COUNT	15
float GetExternalVoltageLevelAveraged(uint8 type)
{
	uint32 adVoltageReadValue = 0;
	float adVoltageLevel = (float)0.0;

#if NS8100_ORIGINAL_PROTOTYPE
	uint32 adChannelValueLow = 0xFFFF;
	uint32 adChannelValueHigh = 0;
	uint32 adChannelSum = 0;
	uint32 i = 0;

	// Need to average some amount of reads
	for (i = 0; i < AD_VOLTAGE_READ_LOOP_COUNT; i++)
	{
		switch (type)
		{
			case EXT_CHARGE_VOLTAGE:
				{
#if 0 /* old hw */
					adc_start(&AVR32_ADC);
					adVoltageReadValue = adc_get_value(&AVR32_ADC, VIN_CHANNEL);
#endif					
					// Need delay to prevent lockup on spin, EOC check inside adc_get_value appears not working as intended
					SoftUsecWait(4);
					//debug("Ext Charge Voltage A/D Reading: 0x%x\r\n", adVoltageReadValue);
				}				
				break;

			case BATTERY_VOLTAGE:
				{
#if 0 /* old hw */
					adc_start(&AVR32_ADC);
					adVoltageReadValue = adc_get_value(&AVR32_ADC, VBAT_CHANNEL);
#endif
					// Need delay to prevent lockup on spin, EOC check inside adc_get_value appears not working as intended
					SoftUsecWait(4);
					//debug("Battery Voltage A/D Reading: 0x%x\r\n", adVoltageReadValue);
				}
				break;
		}
		
		if (adVoltageReadValue < adChannelValueLow)
			adChannelValueLow = adVoltageReadValue;

		if (adVoltageReadValue > adChannelValueHigh)
			adChannelValueHigh = adVoltageReadValue;
			
		adChannelSum += adVoltageReadValue;
	}

	// Remove the lowest and highest values read
	adVoltageLevel = adChannelSum - (adChannelValueLow + adChannelValueHigh);
	
	adVoltageLevel /= (AD_VOLTAGE_READ_LOOP_COUNT - 2);
		
	// Converted A/D value / 1024 * 3.3 * RATIO
	switch (type)
	{
		case EXT_CHARGE_VOLTAGE:
			adVoltageLevel *= (REFERENCE_VOLTAGE * VOLTAGE_RATIO_EXT_CHARGE);
			break;

		case BATTERY_VOLTAGE:
			adVoltageLevel *= (REFERENCE_VOLTAGE * VOLTAGE_RATIO_BATT);
			break;
	}
#else /* (NS8100_ALPHA_PROTOTYPE || NS8100_BETA_PROTOTYPE) */
	switch (type)
	{
		case EXT_CHARGE_VOLTAGE:
		{
#if 0 /* old hw */
			adc_start(&AVR32_ADC);
			adVoltageReadValue = adc_get_value(&AVR32_ADC, VIN_CHANNEL);
#endif
			adVoltageLevel = adVoltageReadValue * (REFERENCE_VOLTAGE * VOLTAGE_RATIO_EXT_CHARGE);

			// Need delay to prevent lockup on spin, EOC check inside adc_get_value appears not working as intended
			//SoftUsecWait(4);
			//debug("Ext Charge Voltage A/D Reading: 0x%x\r\n", adVoltageReadValue);
		}
		break;

		case BATTERY_VOLTAGE:
		{
#if 0 /* old hw */
			adc_start(&AVR32_ADC);
			adVoltageReadValue = adc_get_value(&AVR32_ADC, VBAT_CHANNEL);
#endif
			adVoltageLevel = adVoltageReadValue * (REFERENCE_VOLTAGE * VOLTAGE_RATIO_BATT);

			// Need delay to prevent lockup on spin, EOC check inside adc_get_value appears not working as intended
			//SoftUsecWait(4);
			//debug("Battery Voltage A/D Reading: 0x%x\r\n", adVoltageReadValue);
		}
		break;
	}
#endif
	
	adVoltageLevel /= BATT_RESOLUTION;

	return (adVoltageLevel);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
BOOLEAN CheckExternalChargeVoltagePresent(void)
{
	float adVoltageLevel = (float)0.0;
	BOOLEAN	externalChargePresent = NO;
	uint32 adVoltageReadValue = 0;

#if 0 /* old hw */
	adc_start(&AVR32_ADC);

#if 1 /* Normal operation */
	adVoltageReadValue = adc_get_value(&AVR32_ADC, VIN_CHANNEL);
#else /* Test with battery */
	adVoltageReadValue = adc_get_value(&AVR32_ADC, VBAT_CHANNEL);
#endif
#endif					
	adVoltageLevel = adVoltageReadValue * (REFERENCE_VOLTAGE * VOLTAGE_RATIO_EXT_CHARGE);
	adVoltageLevel /= BATT_RESOLUTION;

	if (adVoltageLevel > EXTERNAL_VOLTAGE_PRESENT)
		externalChargePresent = YES;
		
	return (externalChargePresent);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16 CheckInputMsg(INPUT_MSG_STRUCT *msg_ptr)
{
	uint16 data_index;

	/* If the pointers are not equal that means a msg has been
	put into the input buffer queue. */

	if (s_inputReadPtr != s_inputWritePtr)
	{
		msg_ptr->cmd = s_inputReadPtr->cmd;
		msg_ptr->length = s_inputReadPtr->length;

		for (data_index = 0; data_index < msg_ptr->length; data_index++)
		{
			msg_ptr->data[data_index] = s_inputReadPtr->data[data_index];
		}

		if (s_inputReadPtr == (g_input_buffer + (INPUT_BUFFER_SIZE - 1)))
		{
			s_inputReadPtr = g_input_buffer;
		}
		else
		{
			s_inputReadPtr++;
		}

		return (INPUT_BUFFER_NOT_EMPTY);
	}

	return (INPUT_BUFFER_EMPTY);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ProcessInputMsg(INPUT_MSG_STRUCT mn_msg)
{
	KeypressEventMgr();

	// Check if the LCD power is off
	if (getSystemEventState(UPDATE_MENU_EVENT))
	{
		debug("Keypress command absorbed to signal unit to power on LCD\r\n");

		// Clear out the message parameters
		mn_msg.cmd = 0; mn_msg.length = 0; mn_msg.data[0] = 0;

		// Clear the flag
		clearSystemEventFlag(UPDATE_MENU_EVENT);

		// Recall the current active menu
		JUMP_TO_ACTIVE_MENU();

		// Done processing, return
		return;
	}

	switch (mn_msg.cmd)
	{
		case CTRL_CMD:
			{
				debug("Handling Ctrl Sequence\r\n");
				HandleCtrlKeyCombination((char)mn_msg.data[0]);
			}
			break;

		case BACK_LIGHT_CMD:
			{
				debug("Handling Backlight Command\r\n");
				SetNextLcdBacklightState();
			}
			break;

		case KEYPRESS_MENU_CMD:
			{
				debug("Handling Keypress Command\r\n");
				JUMP_TO_ACTIVE_MENU();
			}				
			break;

		case POWER_OFF_CMD:
			{
				PowerUnitOff(SHUTDOWN_UNIT);
			}			
			break;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16 SendInputMsg(INPUT_MSG_STRUCT *msg_ptr)
{
	uint16 data_index;

	if (s_inputWritePtr != (g_input_buffer + (INPUT_BUFFER_SIZE - 1)))
	{
		if ((s_inputWritePtr + 1) != s_inputReadPtr)
		{
			s_inputWritePtr->cmd = msg_ptr->cmd;
			s_inputWritePtr->length = msg_ptr->length;

			for (data_index = 0; data_index < msg_ptr->length; data_index++)
			{
				s_inputWritePtr->data[data_index] = msg_ptr->data[data_index];
			}

			s_inputWritePtr++;
		}
		else
		{
			return (FAILED);
		}
	}
	else
	{
		if (s_inputReadPtr != g_input_buffer)
		{
			s_inputWritePtr->cmd = msg_ptr->cmd;
			s_inputWritePtr->length = msg_ptr->length;

			for (data_index = 0; data_index < msg_ptr->length; data_index++)
			{
				s_inputWritePtr->data[data_index] = msg_ptr->data[data_index];
			}

			s_inputWritePtr = g_input_buffer;
		}
		else
		{
			return (FAILED);
		}
	}

	return (PASSED);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SoftUsecWait(uint32 usecs)
{
#if 0 /* Old soft loop method */
	unsigned long int countdown = (usecs << 2) + usecs;
	
	for (; countdown > 0; )
	{
		countdown--;
	}
#else /* New hardware timer */
	MXC_Delay(usecs);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SpinBar(void)
{
	static uint8 BarPos = 0;

	switch (BarPos)
	{
		case 0 : case 4 : debugChar('|'); debugChar(0x8); BarPos++; return;
		case 1 : case 5 : debugChar('/'); debugChar(0x8); BarPos++; return;
		case 2 : case 6 : debugChar('-'); debugChar(0x8); BarPos++; return;
		case 3 : case 7 : debugChar('\\'); debugChar(0x8); (BarPos == 7) ? BarPos = 0 : BarPos++; return;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16 SwapInt(uint16 Scr)
{
	uint16 swap1;
	uint16 swap2;

	swap1 = (uint16)((Scr >> 8) & 0x00ff);
	swap2 = (uint16)((Scr << 8) & 0xff00);

	return ((uint16)(swap1 | swap2));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
float HexToDB(uint16 data, uint8 dataNormalizedFlag, uint16 bitAccuracyMidpoint, uint8 acousticSensorType)
{
	float tempValue;

	tempValue = HexToMB(data, dataNormalizedFlag, bitAccuracyMidpoint, acousticSensorType) * (float)DB_CONVERSION_VALUE;

	if (tempValue > 0)
	{
		tempValue = (float)log10f(tempValue) * (float)20.0;
	}

	return (tempValue);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
float HexToMB(uint16 data, uint8 dataNormalizedFlag, uint16 bitAccuracyMidpoint, uint8 acousticSensorType)
{
	float millibars;

	if (dataNormalizedFlag == DATA_NOT_NORMALIZED)
	{
		if (data >= bitAccuracyMidpoint)
		{
			data = (uint16)(data - bitAccuracyMidpoint);
		}
		else
		{
			data = (uint16)(bitAccuracyMidpoint - data);
		}
	}

#if 0 /* Normal (brought forward from 7K series) */
	millibars = (float)((float)(data * 25)/(float)10000.0);
	
	// Scale appropriate to bit accuracy based on the original calc for 12-bit
	millibars /= (float)((float)bitAccuracyMidpoint / (float)ACCURACY_12_BIT_MIDPOINT);

	if (acousticSensorType == SENSOR_MIC_160_DB)
	{
		millibars *= 4;
	}
#else /* Alternate */
	float micScale;

	if (acousticSensorType == SENSOR_MIC_160_DB) { micScale = SENSOR_MIC_160_DB_FULL_SCALE_MB; }
	else if (acousticSensorType == SENSOR_MIC_5_PSI) { micScale = SENSOR_MIC_5_PSI_FULL_SCALE_MB; }
	else if (acousticSensorType == SENSOR_MIC_10_PSI) { micScale = SENSOR_MIC_10_PSI_FULL_SCALE_MB; }
	else /* (acousticSensorType == SENSOR_MIC_148_DB) */ { micScale = SENSOR_MIC_148_DB_FULL_SCALE_MB; }

	millibars = (float)(((float)data * micScale)/(float)bitAccuracyMidpoint);
#endif

	return (millibars);
}

#if 1
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
float HexToPSI(uint16 data, uint8 dataNormalizedFlag, uint16 bitAccuracyMidpoint, uint8 acousticSensorType)
{
	float mbToPSI;

	mbToPSI = HexToMB(data, dataNormalizedFlag, bitAccuracyMidpoint, acousticSensorType);

	mbToPSI /= PSI_TO_MB_CONVERSION_RATIO;

	return (mbToPSI);
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16 DbToHex(uint32 db, uint8 acousticSensorType)
{
#if 0 /* Original */
	// Convert dB to an offset from 0 to 2048 and upscale to 16-bit

	// This is the inverse log of base 10.
	float dbValue = (float)pow((float)10, ((float)db/(float)20.0));

	// Do the conversion. 1/.0000002 = 5000000
	dbValue = (float)dbValue / (float)DB_CONVERSION_VALUE;

	// Do the conversion. millibar conversion 400 = 10000/25
	dbValue = (float)dbValue * (float)MB_CONVERSION_VALUE;

	// Upscale from 12-bit to 16-bit
	dbValue *= 16;

	if (acousticSensorType == SENSOR_MIC_160_DB)
	{
		dbValue /= 4;
	}
#else /* Added sensor types */
	// Convert dB to an offset from 0 to 32768

	// This is the inverse log of base 10.
	float dbValue = (float)pow((float)10, ((float)db/(float)20.0));
	float micScale;

	if (acousticSensorType == SENSOR_MIC_160_DB) { micScale = (781.25 * 4); }
	else if (acousticSensorType == SENSOR_MIC_5_PSI) { micScale = 52602.824; }
	else if (acousticSensorType == SENSOR_MIC_10_PSI) { micScale = (52602.824 * 2); }
	else /* (acousticSensorType == SENSOR_MIC_148_DB) */ { micScale = 781.25; }

	dbValue /= micScale;
#endif

	return (ceil(dbValue));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16 MbToHex(uint32 mb, uint8 acousticSensorType)
{
#if 0 /* Original */
	// Range is from 0 to 32768, incoming mb is adjusted up by 10,000
	float mbValue = (float)((float)mb / ADJUSTED_MB_TO_HEX_VALUE);

	if (acousticSensorType == SENSOR_MIC_160_DB)
	{
		mbValue /= 4;
	}
#else /* Added sensor types */
	float mbValue;

	// Incoming mb is adjusted up by 10000
	if (acousticSensorType == SENSOR_MIC_160_DB) { mbValue = (float)(((float)mb / 4) / ADJUSTED_MB_TO_HEX_VALUE); }
	else if (acousticSensorType == SENSOR_MIC_5_PSI) { mbValue = (float)((float)mb / ADJUSTED_MB_IN_PSI_TO_HEX_VALUE); }
	else if (acousticSensorType == SENSOR_MIC_10_PSI) { mbValue = (float)(((float)mb / 2) / ADJUSTED_MB_IN_PSI_TO_HEX_VALUE); }
	else /* (acousticSensorType == SENSOR_MIC_148_DB) */ { mbValue = (float)((float)mb / ADJUSTED_MB_TO_HEX_VALUE); }
#endif

	return (ceil(mbValue));
}

#if 1
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16 PsiToHex(uint32 psi, uint8 acousticSensorType)
{
	uint32 psiToMillibars = (uint32)((float)psi * PSI_TO_MB_CONVERSION_RATIO);

	return (MbToHex(psiToMillibars, acousticSensorType));
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16 Isqrt(uint32 x)
{
	register uint32 xroot, m2, x2;

	xroot = 0;
	m2 = 1 << MAX_BIT * 2;

	do
	{
		x2 = xroot + m2;
		xroot >>= 1;

		if (x2 <= x)
		{
			x -= x2;
			xroot += m2;
		}
	}
	while (m2 >>= 2);

	if (xroot < x)
		return ((uint16)(xroot + 1));

	return ((uint16)xroot);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitVersionMsg(void)
{
	debug("Software Version: %s (Build Date & Time: %s)\r\n", g_buildVersion, g_buildDate);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
//#include "fsaccess.h"
void BuildLanguageLinkTable(uint8 languageSelection)
{
	uint16 i, currIndex;
	char languageFilename[50];
	char promptTitle[25];
	uint16 sizeCheck;
	uint32 fileSize = 0;

	FIL file;
	FILINFO fno;
	uint32_t readSize;

	memset((char*)&languageFilename[0], 0, sizeof(languageFilename));
	strcpy((char*)&languageFilename[0], LANGUAGE_PATH);

	switch (languageSelection)
	{
		case FRENCH_LANG:	strcat(languageFilename, "French.tbl");		strcpy(promptTitle, "ATTENTION");		strcpy((char*)g_spareBuffer, "FICHIER DE LANGUE NON COURANT. VEUILLEZ METTRE A JOUR");	sizeCheck = 8400; break;
		case ITALIAN_LANG:	strcat(languageFilename, "Italian.tbl");	strcpy(promptTitle, "AVVERTIMENTO");	strcpy((char*)g_spareBuffer, "LINGUA FILE NON CORRENTE. PER FAVORE AGGIORNARE");		sizeCheck = 8200; break;
		case GERMAN_LANG:	strcat(languageFilename, "German.tbl");		strcpy(promptTitle, "WARNUNG");			strcpy((char*)g_spareBuffer, "LANGUAGE DATEI NICHT STROM. BITTE AKTUALISIEREN");		sizeCheck = 8000; break;
		case SPANISH_LANG:	strcat(languageFilename, "Spanish.tbl");	strcpy(promptTitle, "ADVERTENCIA");		strcpy((char*)g_spareBuffer, "ARCHIVO DE IDIOMA NO CORRIENTE. POR FAVOR ACTUALICE");	sizeCheck = 8800; break;

		case ENGLISH_LANG:
		default:			strcat(languageFilename, "English.tbl");	strcpy(promptTitle, "WARNING");			strcpy((char*)g_spareBuffer, "LANGUAGE FILE IS NOT CURRENT. PLEASE UPDATE");			sizeCheck = 7200;
			// Check if no match (default)
			if (languageSelection != ENGLISH_LANG)
			{
				g_unitConfig.languageMode = ENGLISH_LANG;
				SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);
			}
			break;
	}

#if 0 /* Old method no longer needed to copy */
	// Default to internal English language table as a backup (overwritten if a language file is found)
	memcpy(&g_languageTable[0], &englishLanguageTable[0], sizeof(g_languageTable));
#endif

#if 1 /* New method to link any missing language elements to the built in English table */
	// Set the first element of the link table to the start of the language table
	g_languageLinkTable[0] = &englishLanguageTable[0];

#if 0 /* Test debug output */
	uint16 length = sprintf((char*)g_spareBuffer, "Built in English Table Link\r\n(%d) %s\r\n", 1, (char*)g_languageLinkTable[0]);
	ModemPuts(g_spareBuffer, length, NO_CONVERSION);
#endif

	// Build the language link table by pointing to the start of every string following a Null
	for (i = 1, currIndex = 0; i < TOTAL_TEXT_STRINGS; i++)
	{
		while (englishLanguageTable[currIndex++] != '\0') { /* spin */ };

		g_languageLinkTable[i] = englishLanguageTable + currIndex;

#if 0 /* Test debug output */
		length = sprintf((char*)g_spareBuffer, "(%d) %s\r\n", (i + 1), (char*)g_languageLinkTable[i]);
		ModemPuts(g_spareBuffer, length, NO_CONVERSION);
#endif
	}
#endif

	// Attempt to find the file on the SD file system
    if ((f_stat((const TCHAR*)&languageFilename[0], &fno)) == FR_NO_FILE)
	{
		debugWarn("Language file not found: %s\r\n", &languageFilename[0]);
	}
	else // File exists
	{
		if ((f_open(&file, (const TCHAR*)&languageFilename[0], FA_READ)) != FR_OK) { debugErr("Unable to open file: %s\r\n", &languageFilename[0]); }
		debug("Loading language table from file: %s, Length: %d\r\n", (char*)&languageFilename[0], f_size(&file));

		memset(&g_languageTable[0], '\0', sizeof(g_languageTable));

		if (f_size(&file) > LANGUAGE_TABLE_MAX_SIZE)
		{
			// Error case - Just read the maximum buffer size and pray
			f_read(&file, (uint8*)&g_languageTable[0], LANGUAGE_TABLE_MAX_SIZE, (UINT*)&readSize);
		}
		else
		{
			// Check if the language file is slightly smaller than the default updated language file for this build (allowing some room for change/reduced text)
			if (f_size(&file) < sizeCheck)
			{
				// Prompt the user
				OverlayMessage(promptTitle, (char*)g_spareBuffer, (5 * SOFT_SECS));
			}

			f_read(&file, (uint8*)&g_languageTable[0], f_size(&file), (UINT*)&readSize);
		}

		g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
		f_close(&file);
	}

	// Loop and convert all line feeds and carriage returns to nulls, and leaving the last char element as a null
	for (i = 1; i < (LANGUAGE_TABLE_MAX_SIZE - 1); i++)
	{
		// Check if a CR or LF was used as an element separator
		if ((g_languageTable[i] == '\r') || (g_languageTable[i] == '\n'))
		{
			// Convert the CR of LF to a Null
			g_languageTable[i] = '\0';

			// Check if a CR/LF or LF/CR combo was used to as the element separator
			if ((g_languageTable[i + 1] == '\r') || (g_languageTable[i + 1] == '\n'))
			{
				// Skip the second character of the combo separator
				i++;
			}
		}
	}

	// Set the first element of the link table to the start of the language table
	g_languageLinkTable[0] = &g_languageTable[0];

#if 0 /* Test debug output */
	length = sprintf((char*)g_spareBuffer, "Language File Table Link\r\n(%d) %s\r\n", 1, (char*)g_languageLinkTable[0]);
	ModemPuts(g_spareBuffer, length, NO_CONVERSION);
#endif
	
	// Build the language link table by pointing to the start of every string following a Null
	for (i = 1, currIndex = 0; i < TOTAL_TEXT_STRINGS; i++)
	{
		while (g_languageTable[currIndex++] != '\0')
		{ /* spin */
			if (currIndex == fileSize) break;
		};

		if (currIndex < fileSize) {	g_languageLinkTable[i] = g_languageTable + currIndex; }

#if 0 /* Test debug output */
		length = sprintf((char*)g_spareBuffer, "(%d) %s\r\n", (i + 1), (char*)g_languageLinkTable[i]);
		ModemPuts(g_spareBuffer, length, NO_CONVERSION);
#endif
	}

	// Check if the null text is actually empty covering the case where a newer language table is loaded but using older firmware where the null entry is populated
	if (g_languageLinkTable[NULL_TEXT][0] != '\0')
	{
		// Set the null text to the last element of the language table, reserved for null
		g_languageLinkTable[NULL_TEXT] = &g_languageTable[(LANGUAGE_TABLE_MAX_SIZE - 1)];
	}

	debug("Language Link Table built.\r\n");
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
extern const char default_boot_name[];
void CheckBootloaderAppPresent(void)
{
	FILINFO fno;

	sprintf((char*)g_spareBuffer, "%s%s", SYSTEM_PATH, default_boot_name);

	if ((f_stat((const TCHAR*)g_spareBuffer, &fno)) == FR_NO_FILE)
	{
		debugWarn("Bootloader not found\r\n");
	}
	else
	{
		debug("Bootloader found and available\r\n");
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#define DISABLE_OCD_MODULE	0
void AdjustPowerSavings(void)
{
	// Todo: Determine sub systems that can be disabled

#if 0 /* temp remove while unused */
	uint32 usartRetries = 100; //USART_DEFAULT_TIMEOUT;
#endif
	uint8 powerSavingsLevel = POWER_SAVINGS_NORMAL;

#if (GLOBAL_DEBUG_PRINT_ENABLED)
	// With Debug enabled use Minimum setting to enable USART0 for serial output
	powerSavingsLevel = POWER_SAVINGS_MINIMUM;
#endif

	switch (powerSavingsLevel)
	{
		//----------------------------------------------------------------------------
		case POWER_SAVINGS_MINIMUM:
		//----------------------------------------------------------------------------
#if DISABLE_OCD_MODULE // Normal
			// Leave active: SYSTIMER; Disable: OCD
			AVR32_PM.cpumask = 0x00010000;
#else /* Exception test */
			// Do nothing
#endif
#if 0 /* old hw */
			// Leave active: EBI, PBA & PBB BRIDGE, FLASHC, USBB; Disable: PDCA, MACB
			AVR32_PM.hsbmask = 0x0000004F;

			// Leave active: USART0, USART 1, USART 3, TC, TWI, SPI0, SPI1, ADC, PM/RTC/EIC, GPIO, INTC; Disable: ABDAC, SSC, PWM, USART 2, PDCA
			AVR32_PM.pbamask = 0x00004BFB;

			// Leave active: SMC, FLASHC, HMATRIX, USBB; Disable: SDRAMC, MACB
			AVR32_PM.pbbmask = 0x00000017;

			// Check if the USB is disabled
			if (!Is_usb_enabled())
			{
				// Enable the USB
				Usb_enable();
			}
#endif
		break;

		//----------------------------------------------------------------------------
		case POWER_SAVINGS_NORMAL:
		//----------------------------------------------------------------------------
#if 0 /* old hw */
			// Wait for serial data to pushed out to prevent the driver from lagging the system
			while (((AVR32_USART0.csr & AVR32_USART_CSR_TXRDY_MASK) == 0) && usartRetries)
			{
				usartRetries--;
			}

#if DISABLE_OCD_MODULE // Normal
			// Leave active: SYSTIMER; Disable: OCD
			AVR32_PM.cpumask = 0x00010000;
#else /* Exception test */
			// Do nothing
#endif

			// Leave active: EBI, PBA & PBB BRIDGE, FLASHC, USBB; Disable: PDCA, MACB
			AVR32_PM.hsbmask = 0x0000004F;

			// Leave active: USART 1, USART 3, TC, TWI, SPI0, SPI1, ADC, PM/RTC/EIC, GPIO, INTC; Disable: ABDAC, SSC, PWM, USART 0 & 2, PDCA
			AVR32_PM.pbamask = 0x00004AFB;

			// Leave active: SMC, FLASHC, HMATRIX, USBB; Disable: SDRAMC, MACB
			AVR32_PM.pbbmask = 0x00000017;

			// Check if the USB is disabled
			if (!Is_usb_enabled())
			{
				// Enable the USB
				Usb_enable();
			}
#endif
		break;

		//----------------------------------------------------------------------------
		case POWER_SAVINGS_MOST:
		//----------------------------------------------------------------------------
#if 0 /* old hw */
			// Check if the USB is enabled
			if (Is_usb_enabled())
			{
				// Disable the USB
				Usb_disable();
			}

#if DISABLE_OCD_MODULE // Normal
			// Leave active: SYSTIMER; Disable: OCD
			AVR32_PM.cpumask = 0x00010000;
#else /* Exception test */
			// Do nothing
#endif

			// Leave active: EBI, PBA & PBB BRIDGE, FLASHC; Disable: PDCA, MACB, USBB
			AVR32_PM.hsbmask = 0x00000047;

			// Leave active: USART1, TC, TWI, SPI0, SPI1, ADC, PM/RTC/EIC, GPIO, INTC; Disable: ABDAC, SSC, PWM, USART 0 & 2 & 3, PDCA
			AVR32_PM.pbamask = 0x000042FB;

			// Leave active: SMC, FLASHC, HMATRIX; Disable: SDRAMC, MACB, USBB
			AVR32_PM.pbbmask = 0x00000015;
#endif
		break;

		//----------------------------------------------------------------------------
		case POWER_SAVINGS_MAX:
		//----------------------------------------------------------------------------
#if 0 /* old hw */
			// Wait for serial data to pushed out to prevent the driver from lagging the system
			while (((AVR32_USART1.csr & AVR32_USART_CSR_TXRDY_MASK) == 0) && usartRetries)
			{
				usartRetries--;
			}

			// Check if the USB is enabled
			if (Is_usb_enabled())
			{
				// Disable the USB
				Usb_disable();
			}

#if DISABLE_OCD_MODULE // Normal
			// Leave active: SYSTIMER; Disable: OCD
			AVR32_PM.cpumask = 0x00010000;
#else /* Exception test */
			// Do nothing
#endif

			// Leave active: EBI, PBA & PBB BRIDGE, FLASHC; Disable: PDCA, MACB, USBB
			AVR32_PM.hsbmask = 0x0000004F; //0x00000047;

			// Leave active: TC, TWI, SPI0, SPI1, ADC, PM/RTC/EIC, GPIO, INTC; Disable: ABDAC, SSC, PWM, USART 0 & 1 & 2 & 3, PDCA
			AVR32_PM.pbamask = 0x000040FB;

			// Leave active: SMC, FLASHC, HMATRIX; Disable: SDRAMC, MACB, USBB
			AVR32_PM.pbbmask = 0x00000017; //0x00000015;
#endif
		break;

		//----------------------------------------------------------------------------
		default: // POWER_SAVINGS_NONE
		//----------------------------------------------------------------------------
#if DISABLE_OCD_MODULE // Normal
			// Leave active: All; Disable: None
			AVR32_PM.cpumask = 0x00010002;
#else /* Exception test */
			// Do nothing
#endif
#if 0 /* old hw */
			// Leave active: All; Disable: None
			AVR32_PM.hsbmask = 0x0000007F;

			// Leave active: All; Disable: None
			AVR32_PM.pbamask = 0x0000FFFF;

			// Leave active: All; Disable: None
			AVR32_PM.pbbmask = 0x0000003F;

			// Check if the USB is disabled
			if (!Is_usb_enabled())
			{
				// Enable the USB
				Usb_enable();
			}
#endif
		break;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GetDateString(char* buff, uint8 monthNum, uint8 bufSize)
{
	if (bufSize > 3)
	{
		if ((monthNum < 1) || (monthNum > 12))
		{
			monthNum = 1;
		}

		memset(buff, 0, bufSize);
		strcpy((char*)buff, (char*)g_monthTable[monthNum].name);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 GetDaysPerMonth(uint8 month, uint16 year)
{
	uint8 daysInTheMonth = 0;

	daysInTheMonth = (uint8)g_monthTable[month].days;

	if (month == FEB)
	{
		if ((year % 4) == 0)
		{
			// Leap year
			daysInTheMonth++;
		}
	}

	return (daysInTheMonth);
}

///----------------------------------------------------------------------------
///	Function:	GetDayOfWeek
///	Purpose:
///----------------------------------------------------------------------------
uint8 GetDayOfWeek(uint8 year, uint8 month, uint8 day)
{
	uint16 numOfDaysPassed = 0;
	uint8 dayOfWeek = 0;

	// Reference Date: Saturday, Jan 1st, 2000
	// Dates before 01-01-2000 not meant to be calculated

	// Calc days in years passed
	numOfDaysPassed = (uint16)((year) * 365);

	// Calc leap days of years passed
	numOfDaysPassed += (year + 3) / 4;

	// Add in days passed of current month offset by 1 due to day 1 as a reference
	numOfDaysPassed += day - 1;

	// For each month passed in the current year, add in number of days passed for the specific month
	while (--month > 0)
	{
		switch (month)
		{
			case JAN: case MAR: case MAY: case JUL: case AUG: case OCT:
				numOfDaysPassed += 31;
				break;

			case APR: case JUN: case SEP: case NOV:
				numOfDaysPassed += 30;
				break;

			case FEB:
				// If this was a leap year, add a day
				if ((year % 4) == 0)
					numOfDaysPassed += 1;
				numOfDaysPassed += 28;
				break;
		}
	}

	// Mod to find day offset from reference
	dayOfWeek = (uint8)(numOfDaysPassed % 7);

	// If the day of the week is Saturday, set it to the day after Friday (based on RTC week starting Sunday)
	if (dayOfWeek == 0) 
		dayOfWeek = 7;

	// Days of week in RTC is zero based, with Sunday being 0. Adjust 1's based day of the week to zero's based
	dayOfWeek--;

	return (dayOfWeek);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16 GetTotalDaysFromReference(TM_DATE_STRUCT date)
{
	uint16 numOfDaysPassed = 0;

	// Reference Date: Saturday, Jan 1st, 2000
	// Dates before 01-01-2000 not meant to be calculated

	// Calculate days in years passed
	numOfDaysPassed = (uint16)((date.year) * 365);

	// Calculate leap days of years passed
	numOfDaysPassed += (date.year + 3) / 4;

	// Add in days passed of current month offset by 1 due to day 1 as a reference
	numOfDaysPassed += date.day - 1;

	// For each month passed in the current year, add in number of days passed for the specific month
	while (--date.month > 0)
	{
		switch (date.month)
		{
			case JAN: case MAR: case MAY: case JUL: case AUG: case OCT:
				numOfDaysPassed += 31;
				break;

			case APR: case JUN: case SEP: case NOV:
				numOfDaysPassed += 30;
				break;

			case FEB:
				// If this was a leap year, add a day
				if ((date.year % 4) == 0)
					numOfDaysPassed += 1;
				numOfDaysPassed += 28;
				break;
		}
	}

	return (numOfDaysPassed);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitTimeMsg(void)
{
#if (GLOBAL_DEBUG_PRINT_ENABLED == ALL_DEBUG)
	//DATE_TIME_STRUCT time = GetCurrentTime();
	DATE_TIME_STRUCT time = GetExternalRtcTime();
#endif

	debug("RTC: Current Date: %s %02d, %4d\r\n", g_monthTable[time.month].name,	time.day, (time.year + 2000));
	debug("RTC: Current Time: %02d:%02d:%02d %s\r\n", ((time.hour > 12) ? (time.hour - 12) : time.hour),
			time.min, time.sec, ((time.hour > 12) ? "PM" : "AM"));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ReportFileSystemAccessProblem(char* attemptedProcess)
{
	uint16 i = 0;

	debugErr("Unable to access file system during: %s\r\n", attemptedProcess);
	for (; i < strlen(attemptedProcess); i++) { attemptedProcess[i] = toupper(attemptedProcess[i]); }
	sprintf((char*)g_spareBuffer, "%s: %s", getLangText(FILE_SYSTEM_BUSY_DURING_TEXT), attemptedProcess);
	OverlayMessage(getLangText(ERROR_TEXT), (char*)g_spareBuffer, (2 * SOFT_SECS));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ReportFileAccessProblem(char* attemptedFile)
{
	uint16 i = 0;

	debugErr("Unable to access file: %s\r\n", attemptedFile);
	for (; i < strlen(attemptedFile); i++) { attemptedFile[i] = toupper(attemptedFile[i]); }
	sprintf((char*)g_spareBuffer, "%s: %s", getLangText(ERROR_TRYING_TO_ACCESS_TEXT), attemptedFile);
	OverlayMessage(getLangText(ERROR_TEXT), (char*)g_spareBuffer, (2 * SOFT_SECS));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ConvertDateTimeToCalDate(CALIBRATION_DATE_STRUCT* calDate, DATE_TIME_STRUCT* dateTime)
{
	calDate->day = dateTime->day;
	calDate->month = dateTime->month;
	calDate->year = (dateTime->year + 2000);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ConvertCalDatetoDateTime(DATE_TIME_STRUCT* dateTime, CALIBRATION_DATE_STRUCT* calDate)
{
	dateTime->day = calDate->day;
	dateTime->month = calDate->month;
	dateTime->year = (calDate->year - 2000);
	dateTime->weekday = GetDayOfWeek(dateTime->year, dateTime->month, dateTime->day);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void CheckForCycleChange(void)
{
	static uint8 processingCycleChange = NO;
	DATE_TIME_STRUCT currentTime = GetCurrentTime();

	// Check for cycle end time match and make sure that the current cycle end isn't already been processed
	if ((currentTime.hour == g_unitConfig.cycleEndTimeHour) && (currentTime.min == 0) && (processingCycleChange == NO))
	{
		// Check that the unit has been on for 1 minute before processing a cycle change event (especially in the case of powering up at cycle time)
		if (g_lifetimeHalfSecondTickCount > (1 * TICKS_PER_MIN))
		{
			processingCycleChange = YES;
			raiseSystemEventFlag(CYCLE_CHANGE_EVENT);
		}
	}
	else if (processingCycleChange == YES)
	{
		if ((currentTime.hour != g_unitConfig.cycleEndTimeHour) && (currentTime.min != 0))
		{
			processingCycleChange = NO;
		}
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GetSpi1MutexLock(SPI1_LOCK_TYPE spi1LockType)
{
	while (g_spi1AccessLock != AVAILABLE) { /* spin and wait */ }

	g_spi1AccessLock = spi1LockType;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ReleaseSpi1MutexLock(void)
{
	g_spi1AccessLock = AVAILABLE;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 CheckTriggerSourceExists(void)
{
	if ((g_unitConfig.externalTrigger == DISABLED) && (g_triggerRecord.trec.seismicTriggerLevel == NO_TRIGGER_CHAR) &&
		(g_triggerRecord.trec.airTriggerLevel == NO_TRIGGER_CHAR))
	{
		return (NO);
	}
	else
	{
		return (YES);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ProcessUsbCoreHandling(void)
{
#if 0 /* old hw */
	usb_task();
	device_mass_storage_task();
	host_mass_storage_task();
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t GetExpandedBatteryPresenceState(void)
{
	// Check if External Battery Presense is found, Active high (Port 0, Pin 2)
	if (MXC_GPIO0->in & MXC_GPIO_PIN_2) { return (YES); }
	else return (NO);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t GetPowerGood5vState(void)
{
	// Check Power good 5v state, Active high (Port 0, Pin 11)
	if (MXC_GPIO0->in & MXC_GPIO_PIN_11) { return (YES); }
	else return (NO);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t GetPowerGoodBatteryChargerState(void)
{
	// Check Power good from the Battery Charger state, Active high (Port 0, Pin 12)
	if (MXC_GPIO0->in & MXC_GPIO_PIN_12) { return (YES); }
	else return (NO);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t GetPowerOnButtonState(void)
{
	// Check Power On button state, Active high (Port 0, Pin 12)
	if (MXC_GPIO1->in & MXC_GPIO_PIN_15) { return (YES); }
	else return (NO);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t GetLteOtaState(void)
{
	// Check LTE OTA state, Active high (Port 0, Pin 30)
	if (MXC_GPIO0->in & MXC_GPIO_PIN_30) { return (ON); }
	else return (OFF);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t GetBleOtaState(void)
{
	// Check BLE OTA state, Active high (Port 1, Pin 29)
	if (MXC_GPIO0->in & MXC_GPIO_PIN_30) { return (ON); }
	else return (OFF);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8_t GetSmartSensorMuxEnableState(void)
{
	// Get Smart Sensor Mux Enable state, Active high (Port 0, Pin 14)
	if (MXC_GPIO_OutGet(MXC_GPIO0, MXC_GPIO_PIN_14)) { return (ON); }
	else return (OFF);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetSmartSensorSleepState(uint8_t state)
{
	// Set Smart Sensor Sleep state, Active low (Port 0, Pin 13)
	if (state == ON) { MXC_GPIO0->out_clr = MXC_GPIO_PIN_13; }
	else /* (state == OFF) */ { MXC_GPIO0->out_set = MXC_GPIO_PIN_13; }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetSmartSensorMuxEnableState(uint8_t state)
{
	// Set Smart Sensor Mux Enable state, Active high (Port 0, Pin 14)
	if (state == ON) { MXC_GPIO0->out_set = MXC_GPIO_PIN_14; }
	else /* (state == OFF) */ { MXC_GPIO0->out_clr = MXC_GPIO_PIN_14; }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetAdcConversionState(uint8_t state)
{
	// Set External ADC Conversion state, Active high (Port 0, Pin 18)
	if (state == ON) { MXC_GPIO0->out_set = MXC_GPIO_PIN_18; }
	else /* (state == OFF) */ { MXC_GPIO0->out_clr = MXC_GPIO_PIN_18; }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetCalMuxPreADEnableState(uint8_t state)
{
	// Set Cal Mux Pre A/D Enable state, Active low (Port 0, Pin 22)
	if (state == ON) { MXC_GPIO0->out_clr = MXC_GPIO_PIN_22; }
	else /* (state == OFF) */ { MXC_GPIO0->out_set = MXC_GPIO_PIN_22; }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetCalMuxPreADSelectState(uint8_t state)
{
	// Set Cal Mux Pre A/D Select state, Select (Port 0, Pin 23)
	if (state == ON) { MXC_GPIO0->out_set = MXC_GPIO_PIN_23; }
	else /* (state == OFF) */ { MXC_GPIO0->out_clr = MXC_GPIO_PIN_23; }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetAccelerometerTriggerState(uint8_t state)
{
	// Set Accelerometer Trigger state, Active high (Port 1, Pin 14)
	if (state == ON) { MXC_GPIO1->out_set = MXC_GPIO_PIN_14; }
	else /* (state == OFF) */ { MXC_GPIO1->out_clr = MXC_GPIO_PIN_14; }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetSensorCheckState(uint8_t state)
{
	// Set Sensor Check state, Active high (Port 2, Pin 10)
	if (state == ON) { MXC_GPIO2->out_set = MXC_GPIO_PIN_10; }
	else /* (state == OFF) */ { MXC_GPIO2->out_clr = MXC_GPIO_PIN_10; }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetSmartSensorMuxA0State(uint8_t state)
{
	// Set Smart Sensor Mux A0 state, Select (Port 2, Pin 23)
	if (state == ON) { MXC_GPIO2->out_set = MXC_GPIO_PIN_23; }
	else /* (state == OFF) */ { MXC_GPIO2->out_clr = MXC_GPIO_PIN_23; }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetSmartSensorMuxA1State(uint8_t state)
{
	// Set Smart Sensor Mux A0 state, Select (Port 2, Pin 25)
	if (state == ON) { MXC_GPIO2->out_set = MXC_GPIO_PIN_25; }
	else /* (state == OFF) */ { MXC_GPIO2->out_clr = MXC_GPIO_PIN_25; }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetNyquist0State(uint8_t state)
{
	// Set Nyquist 0 (Addr 0) state, Select (Port 2, Pin 26)
	if (state == ON) { MXC_GPIO2->out_set = MXC_GPIO_PIN_26; }
	else /* (state == OFF) */ { MXC_GPIO2->out_clr = MXC_GPIO_PIN_26; }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetNyquist1State(uint8_t state)
{
	// Set Nyquist 1 (Addr 1) state, Select (Port 2, Pin 28)
	if (state == ON) { MXC_GPIO2->out_set = MXC_GPIO_PIN_28; }
	else /* (state == OFF) */ { MXC_GPIO2->out_clr = MXC_GPIO_PIN_28; }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetNyquist2EnableState(uint8_t state)
{
	// Set Nyquist 2 (Enable) state, Active low (Port 2, Pin 30)
	if (state == ON) { MXC_GPIO2->out_clr = MXC_GPIO_PIN_30; }
	else /* (state == OFF) */ { MXC_GPIO2->out_set = MXC_GPIO_PIN_30; }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetSensorGeo1EnableState(uint8_t state)
{
	// Set Sensor GEO 1 enable state, Active high (Port 3, Pin 1)
	if (state == ON) { MXC_GPIO3->out_set = MXC_GPIO_PIN_1; }
	else /* (state == OFF) */ { MXC_GPIO3->out_clr = MXC_GPIO_PIN_1; }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetSensorAop1EnableState(uint8_t state)
{
	// Set Sensor AOP 1 enable state, Active high (Port 3, Pin 2)
	if (state == ON) { MXC_GPIO3->out_set = MXC_GPIO_PIN_2; }
	else /* (state == OFF) */ { MXC_GPIO3->out_clr = MXC_GPIO_PIN_2; }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetSensorGeo2EnableState(uint8_t state)
{
	// Set Sensor GEO 2 enable state, Active high (Port 3, Pin 3)
	if (state == ON) { MXC_GPIO3->out_set = MXC_GPIO_PIN_3; }
	else /* (state == OFF) */ { MXC_GPIO3->out_clr = MXC_GPIO_PIN_3; }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetSensorAop2EnableState(uint8_t state)
{
	// Set Sensor AOP 2 enable state, Active high (Port 3, Pin 4)
	if (state == ON) { MXC_GPIO3->out_set = MXC_GPIO_PIN_4; }
	else /* (state == OFF) */ { MXC_GPIO3->out_clr = MXC_GPIO_PIN_4; }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetGainGeo1State(uint8_t state)
{
	// Set Gain GEO 1 state, Active high (Port 3, Pin 5)
	if (state == ON) { MXC_GPIO3->out_set = MXC_GPIO_PIN_5; }
	else /* (state == OFF) */ { MXC_GPIO3->out_clr = MXC_GPIO_PIN_5; }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetPathSelectAop1State(uint8_t state)
{
	// Set Path Select AOP 1 state, Active high (Port 3, Pin 6)
	if (state == ON) { MXC_GPIO3->out_set = MXC_GPIO_PIN_6; }
	else /* (state == OFF) */ { MXC_GPIO3->out_clr = MXC_GPIO_PIN_6; }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetGainGeo2State(uint8_t state)
{
	// Set Gain GEO 2 state, Active high (Port 3, Pin 7)
	if (state == ON) { MXC_GPIO3->out_set = MXC_GPIO_PIN_7; }
	else /* (state == OFF) */ { MXC_GPIO3->out_clr = MXC_GPIO_PIN_7; }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetPathSelectAop2State(uint8_t state)
{
	// Set Path Select AOP 2 state, Active high (Port 3, Pin 8)
	if (state == ON) { MXC_GPIO3->out_set = MXC_GPIO_PIN_8; }
	else /* (state == OFF) */ { MXC_GPIO3->out_clr = MXC_GPIO_PIN_8; }
}
