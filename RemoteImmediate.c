///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "RemoteCommon.h"
#include "RemoteImmediate.h"
#include "RemoteOperation.h"
#include "OldUart.h"
#include "Menu.h"
#include "EventProcessing.h"
#include "PowerManagement.h"
#include "Menu.h"
#include "SysEvents.h"
#include "Crc.h"
#include "Minilzo.h"
#include "TextTypes.h"

#include "fastmath.h"
#include "ff.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Externs
///----------------------------------------------------------------------------
#include "Globals.h"

///----------------------------------------------------------------------------
///	Local Scope Globals
///----------------------------------------------------------------------------
static VMLx_XFER_STRUCT s_vmlXferStruct;

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleUNL(CMD_BUFFER_STRUCT* inCmd)
{
	uint8 matchFlag = NO;
	uint8* dataStart = inCmd->msg + 3;
	char tempStr[UNLOCK_STR_SIZE];
	char unlStr[UNLOCK_STR_SIZE];
	char sendStr[UNLOCK_STR_SIZE*2];

	debug("HandleUNL-user unlock code=<%s>\r\n", dataStart);

	memset(&tempStr[0], 0, sizeof(tempStr));

	sprintf(unlStr,"%04d", g_modemSetupRecord.unlockCode);

	if (0 == strncmp((const char*)(unlStr), (const char*)dataStart, UNLOCK_CODE_SIZE))
	{
		matchFlag = YES;
		strcpy(tempStr, (char*)UNL_USER_RESP_STRING);
	}
	else if (0 == strncmp((const char*)UNLOCK_CODE_DEFAULT, (const char*)dataStart, UNLOCK_CODE_SIZE))
	{
		matchFlag = YES;
		strcpy(tempStr, (char*)UNL_USER_RESP_STRING);
	}
	else if (0 == strncmp((const char*)UNLOCK_CODE_ADMIN, (const char*)dataStart, UNLOCK_CODE_SIZE))
	{
		matchFlag = YES;
		strcpy(tempStr, (char*)UNL_ADMIN_RESP_STRING);
	}
	else if (0 == strncmp((const char*)UNLOCK_CODE_PROG, (const char*)dataStart, UNLOCK_CODE_SIZE))
	{
		matchFlag = YES;
		strcpy(tempStr, (char*)UNL_PROG_RESP_STRING);
	}

	if (YES == matchFlag)
	{
		debug("HandleUNL-MatchCode=<%s>\r\n", dataStart);

		memset(&sendStr[0], 0, sizeof(sendStr));
		if (YES == g_modemStatus.systemIsLockedFlag)
		{
			g_modemStatus.systemIsLockedFlag = NO;
			sprintf(sendStr,"%s0", tempStr);

			// Check to see if there is a binary flag set.
			// The default is to convert he data to ascii and transmit.
			if ('B' == inCmd->msg[UNLOCK_FLAG_LOC])
			{
				g_binaryXferFlag = NO_CONVERSION;
			}
			else
			{
				g_binaryXferFlag = CONVERT_DATA_TO_ASCII;
			}

			// Receiving successful unlock, update last successful connect time
			__autoDialoutTbl.lastConnectTime = GetCurrentTime();
			__autoDialoutTbl.lastConnectTime.valid = YES;
			__autoDialoutTbl.currentCycleConnects++;
		}
		else
		{
			g_modemStatus.systemIsLockedFlag = YES;
			g_binaryXferFlag = CONVERT_DATA_TO_ASCII;
			sprintf(sendStr,"%s1", tempStr);
		}

		ModemPuts((uint8*)(sendStr), strlen(sendStr), CONVERT_DATA_TO_ASCII);
		ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);

		if (g_modemStatus.systemIsLockedFlag == NO)
		{
			if (g_sampleProcessing == ACTIVE_STATE)
			{
				SendLMA();
			}
		}

		g_modemStatus.xferState = NOP_CMD;
	}

	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void handleRST(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);

	// Check if the unit is in monitor mode
	if (g_sampleProcessing == ACTIVE_STATE)
	{
		// Turn printing off
		g_unitConfig.autoPrint = NO;

		StopMonitoring(g_triggerRecord.opMode, FINISH_PROCESSING);
	}

	if (g_unitConfig.timerMode == ENABLED)
	{
		// Disable Timer mode since restarting would force a prompt for user action
		g_unitConfig.timerMode = DISABLED;
		SaveRecordData(&g_unitConfig, DEFAULT_RECORD, REC_UNIT_CONFIG_TYPE);

		OverlayMessage(getLangText(WARNING_TEXT), getLangText(TIMER_MODE_DISABLED_TEXT), (2 * SOFT_SECS));
	}

	PowerUnitOff(RESET_UNIT);

	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleDDP(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);

	if (	g_disableDebugPrinting == YES)
	{
		g_disableDebugPrinting = NO;
	}
	else
	{
		g_disableDebugPrinting = YES;
	}

	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleDAI(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);

	debug("HandleDAI:Here\r\n");

	// If we jump to boot this call will never return, otherwise proceed as if we can't jump
	g_quickBootEntryJump = QUICK_BOOT_ENTRY_FROM_SERIAL;
	BootLoadManager();

	// Issue something to the user to alert them that the DAI command is not functional with this unit
	ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);
	ModemPuts((uint8*)DAI_ERR_RESP_STRING, sizeof(DAI_ERR_RESP_STRING), CONVERT_DATA_TO_ASCII);
	ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);

	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleGLM(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);

	g_modemStatus.barLiveMonitorOverride = YES;

	HandleBargraphLiveMonitoringStartMsg();
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleHLM(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);

	g_modemStatus.barLiveMonitorOverride = BAR_LIVE_MONITORING_OVERRIDE_STOP;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SendLMA(void)
{
	uint16 length;

	// Setup LMA message
	if (g_triggerRecord.opMode == WAVEFORM_MODE)
	{
		// Send LMA for Waveform, Last stored event number and starting Waveform session time
		length = sprintf((char*)g_spareBuffer, "LMA,%d,%d,%lu", g_triggerRecord.opMode, GetLastStoredEventNumber(), ConvertDateTimeToEpochTime(g_pendingEventRecord.summary.captured.endTime));
	}
	else // ((g_triggerRecord.opMode == BARGRAPH_MODE) || (g_triggerRecord.opMode == COMBO_MODE))
	{
		// Send LMA for BG/Combo, BG event number and BG event start time (aka monitor session start time)
		length = sprintf((char*)g_spareBuffer, "LMA,%d,%d,%lu", g_triggerRecord.opMode, g_pendingBargraphRecord.summary.eventNumber, ConvertDateTimeToEpochTime(g_pendingBargraphRecord.summary.captured.eventTime));
	}

	ModemPuts((uint8*)&g_spareBuffer, length, NO_CONVERSION);
	ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleDLM(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);

	uint32 dataLength = 0;						// Will hold the new data length of the message
	uint8 flagData = 0;

	// If the process is busy sending data, return;
	if (YES == g_modemStatus.xferMutex)
	{
		return;
	}

	memset((uint8*)g_dsmXferStructPtr, 0, sizeof(DSMx_XFER_STRUCT));

	// Check if there is an active pending Bargraph event record
	if ((g_sampleProcessing == ACTIVE_STATE) && ((g_triggerRecord.opMode == BARGRAPH_MODE) || (g_triggerRecord.opMode == COMBO_MODE)))
	{
		g_dsmXferStructPtr->numOfSummaries = 1;
		dataLength = sizeof(EVENT_RECORD_DOWNLOAD_STRUCT);
	}
	else // No active pending Bargraph event record to send
	{
		g_dsmXferStructPtr->numOfSummaries = 0;
		dataLength = 0;
	}

	// Now start building the outgoing header. Clear the outgoing header data.
	memset((uint8*)g_outCmdHeaderPtr, 0, sizeof(COMMAND_MESSAGE_HEADER));

	// Copy the existing header data into the outgoing buffer.
	strcpy((char*)g_outCmdHeaderPtr->cmd, "DLMx");

	// Start Building the outgoing header. Set the msg type to a one for a response message.
	sprintf((char*)g_outCmdHeaderPtr->type, "%02d", MSGTYPE_RESPONSE);

	BuildIntDataField((char*)g_outCmdHeaderPtr->dataLength, (dataLength + MESSAGE_HEADER_LENGTH + MESSAGE_FOOTER_LENGTH + (DATA_FIELD_LEN)), FIELD_LEN_08);

	flagData = CRC_32BIT;
	flagData = (uint8)(flagData << 4);
	flagData = (uint8)((uint8)(flagData) | ((uint8)COMPRESS_NONE));
	sprintf((char*)g_outCmdHeaderPtr->compressCrcFlags,"%02x", flagData);

	// Create the message buffer from the outgoing header data.
	BuildOutgoingHeaderBuffer(g_outCmdHeaderPtr, g_dsmXferStructPtr->msgHdr);

	// Fill in the number of records
	BuildIntDataField((char*)g_dsmXferStructPtr->numOfRecStr, g_dsmXferStructPtr->numOfSummaries, FIELD_LEN_06);

	//-----------------------------------------------------------

	g_transferCount = 0;

	// If the beginning of sending data, send the crlf.
	if (MODEM_SEND_FAILED == ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION))
	{
		g_dsmXferStructPtr->xferStateFlag = NOP_XFER_STATE;
		g_modemStatus.xferMutex = NO;
		g_transferCount = 0;
	}

	// g_transmitCRC will be the seed value for the rest of the CRC calculations.
	g_transmitCRC = CalcCCITT32((uint8*)g_dsmXferStructPtr->msgHdr, MESSAGE_HEADER_LENGTH, SEED_32);

	if (MODEM_SEND_FAILED == ModemPuts((uint8*)g_dsmXferStructPtr->msgHdr, MESSAGE_HEADER_LENGTH, g_binaryXferFlag))
	{
		g_dsmXferStructPtr->xferStateFlag = NOP_XFER_STATE;
		g_modemStatus.xferMutex = NO;
		g_transferCount = 0;
	}

	g_transmitCRC = CalcCCITT32((uint8*)g_dsmXferStructPtr->numOfRecStr, FIELD_LEN_06, g_transmitCRC);

	if (MODEM_SEND_FAILED == ModemPuts((uint8*)g_dsmXferStructPtr->numOfRecStr, FIELD_LEN_06, g_binaryXferFlag))
	{
		g_dsmXferStructPtr->xferStateFlag = NOP_XFER_STATE;
		g_modemStatus.xferMutex = NO;
		g_transferCount = 0;
	}

	g_transferCount = MESSAGE_HEADER_LENGTH + FIELD_LEN_06 + 2;

	// Check if there is an active pending Bargraph event record
	if (g_dsmXferStructPtr->numOfSummaries)
	{
		// Stage the data
		g_dsmXferStructPtr->dloadEventRec.structureFlag = START_DLOAD_FLAG;
		g_dsmXferStructPtr->dloadEventRec.downloadDate = GetCurrentTime();
		memcpy(&g_dsmXferStructPtr->dloadEventRec.eventRecord, &g_pendingBargraphRecord, sizeof(EVT_RECORD));
		g_dsmXferStructPtr->dloadEventRec.endFlag = END_DLOAD_FLAG;

		// Setup the transfer structure pointers
		g_dsmXferStructPtr->startDloadPtr = (uint8*)&(g_dsmXferStructPtr->dloadEventRec);
		g_dsmXferStructPtr->dloadPtr = g_dsmXferStructPtr->startDloadPtr;
		g_dsmXferStructPtr->endDloadPtr = ((uint8*)g_dsmXferStructPtr->startDloadPtr + sizeof(EVENT_RECORD_DOWNLOAD_STRUCT));

		// Send the data
		g_transmitCRC = CalcCCITT32((uint8*)g_dsmXferStructPtr->startDloadPtr, sizeof(EVENT_RECORD_DOWNLOAD_STRUCT), g_transmitCRC);

		if (MODEM_SEND_FAILED == ModemPuts((uint8*)g_dsmXferStructPtr->startDloadPtr, sizeof(EVENT_RECORD_DOWNLOAD_STRUCT), g_binaryXferFlag))
		{
			g_dsmXferStructPtr->xferStateFlag = NOP_XFER_STATE;
			g_modemStatus.xferMutex = NO;
			g_transferCount = 0;
		}

		g_transferCount += 	sizeof(EVENT_RECORD_DOWNLOAD_STRUCT);
	}

	// Send the footer
#if ENDIAN_CONVERSION
	g_transmitCRC = __builtin_bswap32(g_transmitCRC);
#endif
	ModemPuts((uint8*)&g_transmitCRC, 4, NO_CONVERSION);
	ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);
	g_transferCount = 0;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void handleESM(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void handleEEM(CMD_BUFFER_STRUCT* inCmd)
{
	uint8 eemHdr[MESSAGE_HEADER_SIMPLE_LENGTH];
	uint8 msgTypeStr[HDR_TYPE_LEN+1];
	uint8 returnCode = MSGTYPE_RESPONSE;
	UNUSED(inCmd);

	if (ACTIVE_STATE == g_sampleProcessing)
	{
		// Error in message length
		returnCode = CFG_ERR_MONITORING_STATE;
	}
	else
	{
		// Disable USB if there is an active connection
		UsbDisableIfActive();

		// Delete events, recalculate space and reinitialize tables
		DeleteEventFileRecords();
		GetSDCardUsageStats();
		InitEventNumberCache();
	}

	sprintf((char*)msgTypeStr, "%02d", returnCode);
	BuildOutgoingSimpleHeaderBuffer((uint8*)eemHdr, (uint8*)"EEMx",
		(uint8*)msgTypeStr, MESSAGE_SIMPLE_TOTAL_LENGTH, COMPRESS_NONE, CRC_NONE);

	// Calculate the CRC on the header
	g_transmitCRC = CalcCCITT32((uint8*)&eemHdr, MESSAGE_HEADER_SIMPLE_LENGTH, SEED_32);

	// Send Starting CRLF
	ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);
	// Send Simple header
	ModemPuts((uint8*)eemHdr, MESSAGE_HEADER_SIMPLE_LENGTH, CONVERT_DATA_TO_ASCII);
	// Send Ending Footer
#if ENDIAN_CONVERSION
	g_transmitCRC = __builtin_bswap32(g_transmitCRC);
#endif
	ModemPuts((uint8*)&g_transmitCRC, 4, NO_CONVERSION);
	ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);

	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void handleECM(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void handleTRG(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleVML(CMD_BUFFER_STRUCT* inCmd)
{
	// Set the data pointer to start after the VML character data bytes
	uint16* dataPtr = (uint16*)(inCmd->msg + MESSAGE_HEADER_SIMPLE_LENGTH);

	// Save the last downloaded unique entry ID
	s_vmlXferStruct.lastDlUniqueEntryId = *dataPtr;

	// Init the start and temp monitor log table indices
	s_vmlXferStruct.startMonitorLogTableIndex = GetStartingMonitorLogTableIndex();
	s_vmlXferStruct.tempMonitorLogTableIndex = TOTAL_MONITOR_LOG_ENTRIES;

	// Set the transfer state command to the VML command
	g_modemStatus.xferState = VMLx_CMD;

	// Set the transfer state flag to the start with the header
	s_vmlXferStruct.xferStateFlag = HEADER_XFER_STATE;

	// Set the transfer mutex since the response will be handled on multiple passes
	g_modemStatus.xferMutex = YES;

	// Save off the printer state to allow the state to be reset when done with the command
	g_modemStatus.xferPrintState = g_unitConfig.autoPrint;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void sendVMLData(void)
{
	uint32 dataLength;
	uint16 i = 0;
	uint8 msgTypeStr[HDR_TYPE_LEN+2];
	uint8 status;

	// Check if handling the header
	if (s_vmlXferStruct.xferStateFlag == HEADER_XFER_STATE)
	{
		// Transmit a carriage return line feed
		if (ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION) == MODEM_SEND_FAILED)
		{
			// There was an error, so reset all global transfer and status fields
			s_vmlXferStruct.xferStateFlag = NOP_XFER_STATE;
			g_modemStatus.xferMutex = NO;
			g_modemStatus.xferState = NOP_CMD;
			g_transferCount = 0;

			return;
		}

		// Get the data length, which is the total number of new entries plus 1 (non-valid end 0xCC entry) times the log entry size
		dataLength = ((NumOfNewMonitorLogEntries(s_vmlXferStruct.lastDlUniqueEntryId) + 1) * sizeof(MONITOR_LOG_ENTRY_STRUCT));

		// Signal a message response in the message type string
		sprintf((char*)msgTypeStr, "%02d", MSGTYPE_RESPONSE_REV1);

		// Build the outgoing message header
		BuildOutgoingSimpleHeaderBuffer((uint8*)&(s_vmlXferStruct.vmlHdr), (uint8*)"VMLx", (uint8*)msgTypeStr,
										(MESSAGE_SIMPLE_TOTAL_LENGTH + dataLength), COMPRESS_NONE, CRC_32BIT);

		// Calculate the CRC on the header
		g_transmitCRC = CalcCCITT32((uint8*)&(s_vmlXferStruct.vmlHdr), MESSAGE_HEADER_SIMPLE_LENGTH, SEED_32);

		// Send the header out the modem
		if (ModemPuts((uint8*)&(s_vmlXferStruct.vmlHdr), MESSAGE_HEADER_SIMPLE_LENGTH, g_binaryXferFlag) == MODEM_SEND_FAILED)
		{
			// There was an error, so reset all global transfer and status fields
			s_vmlXferStruct.xferStateFlag = NOP_XFER_STATE;
			g_modemStatus.xferMutex = NO;
			g_modemStatus.xferState = NOP_CMD;
			g_transferCount = 0;

			return;
		}

		// Done with the header, move to data transfer
		s_vmlXferStruct.xferStateFlag = DATA_XFER_STATE;
	}
	else if (s_vmlXferStruct.xferStateFlag == DATA_XFER_STATE)
	{
		// Loop up to the total number of vml data log entries
		for (i = 0; i < VML_DATA_LOG_ENTRIES; i++)
		{
			// Get the next valid monitor log entry (routine will store it into the data buffer)
			status = GetNextMonitorLogEntry(s_vmlXferStruct.lastDlUniqueEntryId, s_vmlXferStruct.startMonitorLogTableIndex,
											&(s_vmlXferStruct.tempMonitorLogTableIndex), &(s_vmlXferStruct.vmlData[i]));

			// Check if the status indicates that no more valid entries were found
			if (status == NO)
			{
				// Write all 0xCC's to a log entry to mark the end of the data (following a convention used in the DQM command)
				memset(&(s_vmlXferStruct.vmlData[i]), 0xCC, sizeof(MONITOR_LOG_ENTRY_STRUCT));

				// Reached the end of the data, set state to handle footer next
				s_vmlXferStruct.xferStateFlag = FOOTER_XFER_STATE;

				// Since we're done, add 1 to the total count of entries in the buffer
				i++;

				// Break out of the for loop since there are no more entries
				break;
			}

#if ENDIAN_CONVERSION
			EndianSwapMonitorLogStruct(&(s_vmlXferStruct.vmlData[i]));
#endif
		}

		// Calculate the CRC on the data
		g_transmitCRC = CalcCCITT32((uint8*)&(s_vmlXferStruct.vmlData[0]), (i * sizeof(MONITOR_LOG_ENTRY_STRUCT)), g_transmitCRC);

		// Send the data out the modem
		if (ModemPuts((uint8*)&(s_vmlXferStruct.vmlData[0]), (i * sizeof(MONITOR_LOG_ENTRY_STRUCT)), g_binaryXferFlag) == MODEM_SEND_FAILED)
		{
			// There was an error, so reset all global transfer and status fields
			s_vmlXferStruct.xferStateFlag = NOP_XFER_STATE;
			g_modemStatus.xferMutex = NO;
			g_modemStatus.xferState = NOP_CMD;
			g_transferCount = 0;

			return;
		}
	}
	else if (s_vmlXferStruct.xferStateFlag == FOOTER_XFER_STATE)
	{
#if ENDIAN_CONVERSION
		g_transmitCRC = __builtin_bswap32(g_transmitCRC);
#endif
		ModemPuts((uint8*)&g_transmitCRC, 4, NO_CONVERSION);
		ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);

		// Done with the command, reset all global transfer and status fields
		s_vmlXferStruct.xferStateFlag = NOP_XFER_STATE;
		g_modemStatus.xferState = NOP_CMD;
		g_modemStatus.xferMutex = NO;
		g_transferCount = 0;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleUDE(CMD_BUFFER_STRUCT* inCmd)
{
	// Set the data pointer to start after the UDE character data bytes
	uint16* dataPtr = (uint16*)(inCmd->msg + MESSAGE_HEADER_SIMPLE_LENGTH);

	if (*dataPtr < g_nextEventNumberToUse)
		__autoDialoutTbl.lastDownloadedEvent = *dataPtr;

	// Done with the command, reset all global transfer and status fields
	s_vmlXferStruct.xferStateFlag = NOP_XFER_STATE;
	g_modemStatus.xferState = NOP_CMD;
	g_modemStatus.xferMutex = NO;
	g_transferCount = 0;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void handleGAD(CMD_BUFFER_STRUCT* inCmd)
{
	uint32 dataLength;
	uint8 msgTypeStr[HDR_TYPE_LEN+2];
	uint8 gadHdr[MESSAGE_HEADER_SIMPLE_LENGTH];
	uint8 serialNumber[SERIAL_NUMBER_STRING_SIZE];
	AUTODIALOUT_STRUCT tempAutoDialout = __autoDialoutTbl;

	UNUSED(inCmd);

	memset(&serialNumber[0], 0, sizeof(serialNumber));
	strcpy((char*)&serialNumber[0], g_factorySetupRecord.unitSerialNumber);

	// Transmit a carrige return line feed
	if (ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION) == MODEM_SEND_FAILED)
	{
		// There was an error, so reset all global transfer and status fields
		g_modemStatus.xferMutex = NO;
		g_modemStatus.xferState = NOP_CMD;
		g_transferCount = 0;

		return;
	}

	// Get the data length, which is the total number of new entries plus 1 (non-valid end 0xCC entry) times the log entry size
	dataLength = (SERIAL_NUMBER_STRING_SIZE + sizeof(AUTODIALOUT_STRUCT));

	// Signal a message response in the message type string
	sprintf((char*)msgTypeStr, "%02d", MSGTYPE_RESPONSE);

	// Build the outgoing message header
	BuildOutgoingSimpleHeaderBuffer((uint8*)&(gadHdr), (uint8*)"GADx", (uint8*)msgTypeStr,
									(MESSAGE_SIMPLE_TOTAL_LENGTH + dataLength), COMPRESS_NONE, CRC_32BIT);

	// Calculate the CRC on the header
	g_transmitCRC = CalcCCITT32((uint8*)&(gadHdr), MESSAGE_HEADER_SIMPLE_LENGTH, SEED_32);

	// Send the header out the modem
	if (ModemPuts((uint8*)&(gadHdr), MESSAGE_HEADER_SIMPLE_LENGTH, g_binaryXferFlag) == MODEM_SEND_FAILED)
	{
		// There was an error, so reset all global transfer and status fields
		g_modemStatus.xferMutex = NO;
		g_modemStatus.xferState = NOP_CMD;
		g_transferCount = 0;

		return;
	}

	// Calculate the CRC on the data
	g_transmitCRC = CalcCCITT32((uint8*)&serialNumber[0], sizeof(serialNumber), g_transmitCRC);

	// Send the data out the modem
	if (ModemPuts((uint8*)&serialNumber[0], sizeof(serialNumber), g_binaryXferFlag) == MODEM_SEND_FAILED)
	{
		// There was an error, so reset all global transfer and status fields
		g_modemStatus.xferMutex = NO;
		g_modemStatus.xferState = NOP_CMD;
		g_transferCount = 0;

		return;
	}

	// Calculate the CRC on the data
	g_transmitCRC = CalcCCITT32((uint8*)&tempAutoDialout, sizeof(AUTODIALOUT_STRUCT), g_transmitCRC);

	// Send the data out the modem
	if (ModemPuts((uint8*)&tempAutoDialout, sizeof(AUTODIALOUT_STRUCT), g_binaryXferFlag) == MODEM_SEND_FAILED)
	{
		// There was an error, so reset all global transfer and status fields
		g_modemStatus.xferMutex = NO;
		g_modemStatus.xferState = NOP_CMD;
		g_transferCount = 0;

		return;
	}

#if ENDIAN_CONVERSION
	g_transmitCRC = __builtin_bswap32(g_transmitCRC);
#endif
	ModemPuts((uint8*)&g_transmitCRC, 4, NO_CONVERSION);
	ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);

	// Done with the command, reset all global transfer and status fields
	g_modemStatus.xferState = NOP_CMD;
	g_modemStatus.xferMutex = NO;
	g_transferCount = 0;

	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void handleGFS(CMD_BUFFER_STRUCT* inCmd)
{
	uint32 dataLength;
	uint8 msgTypeStr[HDR_TYPE_LEN+2];
	uint8 gfsHdr[MESSAGE_HEADER_SIMPLE_LENGTH];
	
	UNUSED(inCmd);

	// Transmit a carrige return line feed
	if (ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION) == MODEM_SEND_FAILED)
	{
		// There was an error, so reset all global transfer and status fields
		g_modemStatus.xferMutex = NO;
		g_modemStatus.xferState = NOP_CMD;
		g_transferCount = 0;

		return;
	}

	// Get the data length, which is the total number of new entries plus 1 (non-valid end 0xCC entry) times the log entry size
	dataLength = sizeof(FLASH_USAGE_STRUCT);

	// Signal a message response in the message type string
	sprintf((char*)msgTypeStr, "%02d", MSGTYPE_RESPONSE);

	// Build the outgoing message header
	BuildOutgoingSimpleHeaderBuffer((uint8*)&(gfsHdr), (uint8*)"GFSx", (uint8*)msgTypeStr,
									(MESSAGE_SIMPLE_TOTAL_LENGTH + dataLength), COMPRESS_NONE, CRC_32BIT);

	// Calculate the CRC on the header
	g_transmitCRC = CalcCCITT32((uint8*)&(gfsHdr), MESSAGE_HEADER_SIMPLE_LENGTH, SEED_32);

	// Send the header out the modem
	if (ModemPuts((uint8*)&(gfsHdr), MESSAGE_HEADER_SIMPLE_LENGTH, g_binaryXferFlag) == MODEM_SEND_FAILED)
	{
		// There was an error, so reset all global transfer and status fields
		g_modemStatus.xferMutex = NO;
		g_modemStatus.xferState = NOP_CMD;
		g_transferCount = 0;

		return;
	}

	// Calculate the CRC on the data
	g_transmitCRC = CalcCCITT32((uint8*)&g_sdCardUsageStats, sizeof(FLASH_USAGE_STRUCT), g_transmitCRC);

	// Send the data out the modem
	if (ModemPuts((uint8*)&g_sdCardUsageStats, sizeof(FLASH_USAGE_STRUCT), g_binaryXferFlag) == MODEM_SEND_FAILED)
	{
		// There was an error, so reset all global transfer and status fields
		g_modemStatus.xferMutex = NO;
		g_modemStatus.xferState = NOP_CMD;
		g_transferCount = 0;

		return;
	}

#if ENDIAN_CONVERSION
	g_transmitCRC = __builtin_bswap32(g_transmitCRC);
#endif
	ModemPuts((uint8*)&g_transmitCRC, 4, NO_CONVERSION);
	ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);

	// Done with the command, reset all global transfer and status fields
	g_modemStatus.xferState = NOP_CMD;
	g_modemStatus.xferMutex = NO;
	g_transferCount = 0;

	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleDQM(CMD_BUFFER_STRUCT* inCmd)
{

	// Download summary memory...
	uint32 dataLength;						// Will hold the new data length of the message
	uint8 msgTypeStr[HDR_TYPE_LEN+2];
	uint8* dqmPtr;
	uint16 i;
	uint16 validEventCount = 0;

	UNUSED(inCmd);

	// If the process is busy sending data, return;
	if (YES == g_modemStatus.xferMutex)
	{
		return;
	}

	// Clear the transfer structure
	memset((uint8*)g_dqmXferStructPtr, 0, sizeof(DQMx_XFER_STRUCT));

	// Set the initial table index to the first element of the table
	g_dqmXferStructPtr->ramTableIndex = 0;

	// Set the number of records to the number of valid entries in the event cache
	g_dqmXferStructPtr->numOfRecs = g_eventNumberCacheValidEntries;

#if 1 /* New option to prevent responses greater than 800 entries (legacy) */
	if ((g_unitConfig.legacyDqmLimit == ENABLED) && (g_dqmXferStructPtr->numOfRecs > DQM_LEGACY_EVENT_LIMIT))
	{
		// Cap the number at DQM_LEGACY_EVENT_LIMIT (800)
		g_dqmXferStructPtr->numOfRecs = DQM_LEGACY_EVENT_LIMIT;

		// Set index to the upper limit (last event number stored)
		i = g_nextEventNumberToUse - 1;

		// Find the starting index of the last 800 events by working backwards
		while (validEventCount < DQM_LEGACY_EVENT_LIMIT)
		{
			if (g_eventNumberCache[i--] == EVENT_REFERENCE_VALID)
			{
				validEventCount++;
			}
		}

		// Set the new table index to the last cache reference (incremented by 1 to account for the last loop decrement)
		g_dqmXferStructPtr->ramTableIndex = (i + 1);
	}
#endif

	// Add 1 additional record as an end marker (which filled with 0xCC's)
	g_dqmXferStructPtr->numOfRecs++;

	// 4 is for the numOfRecs field.
	dataLength = 4 + (g_dqmXferStructPtr->numOfRecs * sizeof(DQMx_DATA_STRUCT));

	sprintf((char*)msgTypeStr, "%02d", MSGTYPE_RESPONSE);

	BuildOutgoingSimpleHeaderBuffer((uint8*)g_dqmXferStructPtr->dqmHdr, (uint8*)"DQSx",
		(uint8*)msgTypeStr, (uint32)(MESSAGE_SIMPLE_TOTAL_LENGTH + dataLength),
		COMPRESS_NONE, CRC_32BIT);

	// Go to the start of the record length value.
	dqmPtr = (uint8*)g_dqmXferStructPtr->dqmHdr + MESSAGE_HEADER_SIMPLE_LENGTH;
	sprintf((char*)dqmPtr, "%04d", g_dqmXferStructPtr->numOfRecs);

	//-----------------------------------------------------------
	g_dqmXferStructPtr->xferStateFlag = HEADER_XFER_STATE;		// This is the initial xfer state to start.
	g_modemStatus.xferState = DQMx_CMD;							// This is the xfer command state.
	g_modemStatus.xferMutex = YES;								// Mutex to prevent other commands.
	g_modemStatus.xferPrintState = g_unitConfig.autoPrint;
	g_transferCount = 0;

#if 0 /* Old */
	if (g_sampleProcessing == IDLE_STATE)
	{
		DumpSummaryListFileToEventBuffer();
	}
#endif

	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 sendDQMData(void)
{
	uint8 idex;
	uint8 xferState = DQMx_CMD;
	EVENT_LIST_ENTRY_STRUCT* eventListCache = (EVENT_LIST_ENTRY_STRUCT*)&g_eventDataBuffer[EVENT_LIST_CACHE_OFFSET];

	// If the beginning of sending data, send the crlf.
	if (HEADER_XFER_STATE == g_dqmXferStructPtr->xferStateFlag)
	{
		if (MODEM_SEND_FAILED == ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION))
		{
			g_dqmXferStructPtr->xferStateFlag = NOP_XFER_STATE;
			g_modemStatus.xferMutex = NO;
			g_transferCount = 0;
			return (NOP_CMD);
		}

		// g_transmitCRC will be the seed value for the rest of the CRC calculations.
		g_transmitCRC = CalcCCITT32((uint8*)g_dqmXferStructPtr->dqmHdr,
			(MESSAGE_HEADER_SIMPLE_LENGTH + 4), SEED_32);

		// Copy the hdr length plus the 4 of the record length.
		if (MODEM_SEND_FAILED == ModemPuts((uint8*)g_dqmXferStructPtr->dqmHdr,
			(MESSAGE_HEADER_SIMPLE_LENGTH + 4), g_binaryXferFlag))
		{
			g_dqmXferStructPtr->xferStateFlag = NOP_XFER_STATE;
			g_modemStatus.xferMutex = NO;
			g_transferCount = 0;
			return (NOP_CMD);
		}

		g_dqmXferStructPtr->xferStateFlag = SUMMARY_TABLE_SEARCH_STATE;
		g_transferCount = MESSAGE_HEADER_LENGTH + 4 + 2;
	}


	// xfer the event record structure.
	else if (SUMMARY_TABLE_SEARCH_STATE == g_dqmXferStructPtr->xferStateFlag)
	{
		// Check if down to the last record (the end marker record)
		if (g_dqmXferStructPtr->numOfRecs == 1)
		{
			memset((uint8*)g_dqmXferStructPtr->dqmData, 0xCC, sizeof(DQMx_DATA_STRUCT));
			g_dqmXferStructPtr->dqmData[0].endFlag = 0xEE;

			g_transmitCRC = CalcCCITT32((uint8*)g_dqmXferStructPtr->dqmData,
				sizeof(DQMx_DATA_STRUCT), g_transmitCRC);

			if (MODEM_SEND_FAILED == ModemPuts((uint8*)g_dqmXferStructPtr->dqmData,
				sizeof(DQMx_DATA_STRUCT), g_binaryXferFlag))
			{
				g_dqmXferStructPtr->xferStateFlag = NOP_XFER_STATE;
				g_modemStatus.xferMutex = NO;
				g_transferCount = 0;
				return (NOP_CMD);
			}
			g_dqmXferStructPtr->xferStateFlag = FOOTER_XFER_STATE;
		}
		else
		{
			idex = 0;
			while ((idex < DQM_XFER_SIZE) && (g_dqmXferStructPtr->numOfRecs > 1))
			{
				// Check if the current index is a valid event entry
				if (g_eventNumberCache[g_dqmXferStructPtr->ramTableIndex] == EVENT_REFERENCE_VALID)
				{
					g_dqmXferStructPtr->dqmData[idex].dqmxFlag = 0xCC;
					g_dqmXferStructPtr->dqmData[idex].mode = eventListCache[g_dqmXferStructPtr->ramTableIndex].mode;
					g_dqmXferStructPtr->dqmData[idex].eventNumber = g_dqmXferStructPtr->ramTableIndex;
					memcpy(g_dqmXferStructPtr->dqmData[idex].serialNumber, &g_serialNumberCache[eventListCache[g_dqmXferStructPtr->ramTableIndex].serialNumberCacheIndex], FACTORY_SERIAL_NUMBER_SIZE);
					g_dqmXferStructPtr->dqmData[idex].eventTime = ConvertEpochTimeToDateTime(eventListCache[g_dqmXferStructPtr->ramTableIndex].epochEventTime);
					g_dqmXferStructPtr->dqmData[idex].subMode = eventListCache[g_dqmXferStructPtr->ramTableIndex].subMode;
					g_dqmXferStructPtr->dqmData[idex].endFlag = 0xEE;
#if ENDIAN_CONVERSION
					g_dqmXferStructPtr->dqmData[idex].eventNumber = __builtin_bswap16(g_dqmXferStructPtr->dqmData[idex].eventNumber);
#endif
					idex++;
					g_dqmXferStructPtr->numOfRecs--;
				}

				g_dqmXferStructPtr->ramTableIndex++;
			}

			g_transmitCRC = CalcCCITT32((uint8*)g_dqmXferStructPtr->dqmData,
				(idex * sizeof(DQMx_DATA_STRUCT)), g_transmitCRC);

			if (MODEM_SEND_FAILED == ModemPuts((uint8*)g_dqmXferStructPtr->dqmData,
				(idex * sizeof(DQMx_DATA_STRUCT)), g_binaryXferFlag))
			{
				g_dqmXferStructPtr->xferStateFlag = NOP_XFER_STATE;
				g_modemStatus.xferMutex = NO;
				g_transferCount = 0;
				return (NOP_CMD);
			}
		}
	}

	else if (FOOTER_XFER_STATE == g_dqmXferStructPtr->xferStateFlag)
	{
#if ENDIAN_CONVERSION
		g_transmitCRC = __builtin_bswap32(g_transmitCRC);
#endif
		ModemPuts((uint8*)&g_transmitCRC, 4, NO_CONVERSION);
		ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);
		g_dqmXferStructPtr->xferStateFlag = NOP_XFER_STATE;
		xferState = NOP_CMD;
		g_modemStatus.xferMutex = NO;
		g_transferCount = 0;
	}

	return (xferState);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleDSM(CMD_BUFFER_STRUCT* inCmd)
{
	uint32 dataLength = 0;						// Will hold the new data length of the message
	uint8 flagData = 0;
	uint8 dsmHdr[MESSAGE_HEADER_SIMPLE_LENGTH];
	uint8 msgTypeStr[HDR_TYPE_LEN+2];
	uint16 startingEventNumber;
	uint32 i;

	// If the process is busy sending data, return;
	if (YES == g_modemStatus.xferMutex)
	{
		return;
	}

	if (YES == ParseIncommingMsgHeader(inCmd, g_inCmdHeaderPtr))
	{
		return;
	}

	memset((uint8*)g_dsmXferStructPtr, 0, sizeof(DSMx_XFER_STRUCT));

	// New method - Subrange, data should be represented as number of event summaries, followed by the starting event number
	// Check if the spare field has been filled in requesting a sub-range of summaries
	if ((g_inCmdHeaderPtr->spare[0] == '0') && ((g_inCmdHeaderPtr->spare[1] == '2') || (g_inCmdHeaderPtr->spare[1] == '3')))
	{
		// Set the index to the start of the message data
		i = MESSAGE_HEADER_LENGTH;

		// Check if data is represented as ASCII hex (Option 2)
		if (g_inCmdHeaderPtr->spare[1] == '2')
		{
			g_dsmXferStructPtr->numOfSummaries = (ConvertAscii2Binary(inCmd->msg[i + 0], inCmd->msg[i + 1]) << 8);
			g_dsmXferStructPtr->numOfSummaries |= (ConvertAscii2Binary(inCmd->msg[i + 2], inCmd->msg[i + 3]) & 0x00FF);
			startingEventNumber = (ConvertAscii2Binary(inCmd->msg[i + 4], inCmd->msg[i + 5]) << 8);
			startingEventNumber |= (ConvertAscii2Binary(inCmd->msg[i + 6], inCmd->msg[i + 7]) & 0x00FF);
		}
		else // Data represented as ASCII decimal (Option 3)
		{
			g_dsmXferStructPtr->numOfSummaries = ((inCmd->msg[i + 0] - '0') * 10000) + ((inCmd->msg[i + 1] - '0') * 1000) +	((inCmd->msg[i + 2] - '0') * 100) +
													((inCmd->msg[i + 3] - '0') * 10) + (inCmd->msg[i + 4] - '0');
			startingEventNumber = ((inCmd->msg[i + 5] - '0') * 10000) + ((inCmd->msg[i + 6] - '0') * 1000) +	((inCmd->msg[i + 7] - '0') * 100) +
									((inCmd->msg[i + 8] - '0') * 10) + (inCmd->msg[i + 9] - '0');
		}

		// Check if the starting event number is valid
		if (g_eventNumberCache[startingEventNumber] == EVENT_REFERENCE_VALID)
		{
			// Set the starting event point
			g_dsmXferStructPtr->currentEventNumber = startingEventNumber;
		}

		// Sanity check for correct size, valid number of summaries, valid event number, range doesn't exceed end event number, and starting event found
		if ((inCmd->size < (MESSAGE_HEADER_LENGTH + 8)) || (g_dsmXferStructPtr->numOfSummaries == 0) || (startingEventNumber == 0) ||
			((g_nextEventNumberToUse - g_dsmXferStructPtr->numOfSummaries - startingEventNumber) < 0) || (g_dsmXferStructPtr->currentEventNumber == 0))
		{
			if (((g_inCmdHeaderPtr->spare[1] == '2') && (inCmd->size < (MESSAGE_HEADER_LENGTH + 8))) || ((g_inCmdHeaderPtr->spare[1] == '3') && (inCmd->size < (MESSAGE_HEADER_LENGTH + 10))))
			{
				i = CFG_ERR_MSG_LENGTH;
			}
			else // Something wrong with the subset parameters
			{
				i = CFG_ERR_BAD_SUBSET;
			}

			// Signal a bad message payload length
			sprintf((char*)msgTypeStr, "%02lu", i);
			BuildOutgoingSimpleHeaderBuffer((uint8*)dsmHdr, (uint8*)"DSMx",	(uint8*)msgTypeStr, MESSAGE_SIMPLE_TOTAL_LENGTH, COMPRESS_NONE, CRC_NONE);

			// Send Starting CRLF
			ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);

			// Calculate the CRC on the header
			g_transmitCRC = CalcCCITT32((uint8*)&dsmHdr, MESSAGE_HEADER_SIMPLE_LENGTH, SEED_32);

			// Send Simple header
			ModemPuts((uint8*)dsmHdr, MESSAGE_HEADER_SIMPLE_LENGTH, CONVERT_DATA_TO_ASCII);

			// Send Ending Footer
#if ENDIAN_CONVERSION
			g_transmitCRC = __builtin_bswap32(g_transmitCRC);
#endif
			ModemPuts((uint8*)&g_transmitCRC, 4, NO_CONVERSION);
			ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);

			return;
		}
	}
	else // Spare field does not specify a sub-range, use the total entires of the ram summary list
	{
		g_dsmXferStructPtr->numOfSummaries = g_eventNumberCacheValidEntries;

		// Init index to the start of the table
		g_dsmXferStructPtr->currentEventNumber = 1;
	}

	// Data length based on the total number of the requested summaries
	dataLength = g_dsmXferStructPtr->numOfSummaries * sizeof(EVENT_RECORD_DOWNLOAD_STRUCT);

	// Now start building the outgoing header. Clear the outgoing header data.
	memset((uint8*)g_outCmdHeaderPtr, 0, sizeof(COMMAND_MESSAGE_HEADER));

	// Copy the existing header data into the outgoing buffer.
	memcpy(g_outCmdHeaderPtr, g_inCmdHeaderPtr, sizeof(COMMAND_MESSAGE_HEADER));

	// Start Building the outgoing header. Set the msg type to a one for a response message.
	sprintf((char*)g_outCmdHeaderPtr->type, "%02d", MSGTYPE_RESPONSE);

	BuildIntDataField((char*)g_outCmdHeaderPtr->dataLength,
		(dataLength + MESSAGE_HEADER_LENGTH + MESSAGE_FOOTER_LENGTH + (DATA_FIELD_LEN)), FIELD_LEN_08);

	flagData = CRC_32BIT;
	flagData = (uint8)(flagData << 4);
	flagData = (uint8)((uint8)(flagData) | ((uint8)COMPRESS_NONE));
	sprintf((char*)g_outCmdHeaderPtr->compressCrcFlags,"%02x", flagData);

	// Create the message buffer from the outgoing header data.
	BuildOutgoingHeaderBuffer(g_outCmdHeaderPtr, g_dsmXferStructPtr->msgHdr);

	// Fill in the number of records
	BuildIntDataField((char*)g_dsmXferStructPtr->numOfRecStr, g_dsmXferStructPtr->numOfSummaries, FIELD_LEN_06);

	//-----------------------------------------------------------
	g_dsmXferStructPtr->xferStateFlag = HEADER_XFER_STATE;	// This is the initail xfer state to start.
	g_modemStatus.xferState = DSMx_CMD;								// This is the xfer command state.
	g_modemStatus.xferMutex = YES;									// Mutex to prevent other commands.

	g_modemStatus.xferPrintState = g_unitConfig.autoPrint;

	g_transferCount = 0;

	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 sendDSMData(void)
{
	uint8 xferState = DSMx_CMD;

	// If the beginning of sending data, send the crlf.
	if (HEADER_XFER_STATE == g_dsmXferStructPtr->xferStateFlag)
	{
		if (MODEM_SEND_FAILED == ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION))
		{
			g_dsmXferStructPtr->xferStateFlag = NOP_XFER_STATE;
			g_modemStatus.xferMutex = NO;
			g_transferCount = 0;

			return (NOP_CMD);
		}

		// g_transmitCRC will be the seed value for the rest of the CRC calculations.
		g_transmitCRC = CalcCCITT32((uint8*)g_dsmXferStructPtr->msgHdr, MESSAGE_HEADER_LENGTH, SEED_32);

		if (MODEM_SEND_FAILED == ModemPuts((uint8*)g_dsmXferStructPtr->msgHdr,
			MESSAGE_HEADER_LENGTH, g_binaryXferFlag))
		{
			g_dsmXferStructPtr->xferStateFlag = NOP_XFER_STATE;
			g_modemStatus.xferMutex = NO;
			g_transferCount = 0;

			return (NOP_CMD);
		}

		g_transmitCRC = CalcCCITT32((uint8*)g_dsmXferStructPtr->numOfRecStr, FIELD_LEN_06, g_transmitCRC);

		if (MODEM_SEND_FAILED == ModemPuts((uint8*)g_dsmXferStructPtr->numOfRecStr,
			FIELD_LEN_06, g_binaryXferFlag))
		{
			g_dsmXferStructPtr->xferStateFlag = NOP_XFER_STATE;
			g_modemStatus.xferMutex = NO;
			g_transferCount = 0;

			return (NOP_CMD);
		}

		g_dsmXferStructPtr->xferStateFlag = SUMMARY_TABLE_SEARCH_STATE;
		g_transferCount = MESSAGE_HEADER_LENGTH + FIELD_LEN_06 + 2;
	}

	// xfer the event record structure.
	else if (SUMMARY_TABLE_SEARCH_STATE == g_dsmXferStructPtr->xferStateFlag)
	{
		// Check if there are still summaries to send
		if (g_dsmXferStructPtr->numOfSummaries)
		{
			// Loop to make sure the current table index does not point to an empty entry
			while (g_eventNumberCache[g_dsmXferStructPtr->currentEventNumber] != EVENT_REFERENCE_VALID)
			{
				g_dsmXferStructPtr->currentEventNumber++;
				if (g_dsmXferStructPtr->currentEventNumber == TOTAL_UNIQUE_EVENT_NUMBERS) { g_dsmXferStructPtr->currentEventNumber = 1; }
			}

			GetEventFileRecord(g_dsmXferStructPtr->currentEventNumber, &g_dsmXferStructPtr->dloadEventRec.eventRecord);

			g_dsmXferStructPtr->dloadEventRec.structureFlag = START_DLOAD_FLAG;
			g_dsmXferStructPtr->dloadEventRec.downloadDate = GetCurrentTime();
			g_dsmXferStructPtr->dloadEventRec.endFlag = END_DLOAD_FLAG;
#if ENDIAN_CONVERISON
			g_dsmXferStructPtr->dloadEventRec.structureFlag = __builtin_bswap32(g_dsmXferStructPtr->dloadEventRec.structureFlag);
			g_dsmXferStructPtr->dloadEventRec.endFlag = __builtin_bswap32(g_dsmXferStructPtr->dloadEventRec.endFlag);
			EndianSwapEventRecord(&g_dsmXferStructPtr->dloadEventRec.eventRecord);
#endif
			// Setup the transfer structure pointers
			g_dsmXferStructPtr->startDloadPtr = (uint8*)&(g_dsmXferStructPtr->dloadEventRec);
			g_dsmXferStructPtr->dloadPtr = g_dsmXferStructPtr->startDloadPtr;
			g_dsmXferStructPtr->endDloadPtr = ((uint8*)g_dsmXferStructPtr->startDloadPtr + sizeof(EVENT_RECORD_DOWNLOAD_STRUCT));

			g_dsmXferStructPtr->currentEventNumber++;							// Increment for next time
			if (g_dsmXferStructPtr->currentEventNumber == TOTAL_UNIQUE_EVENT_NUMBERS) { g_dsmXferStructPtr->currentEventNumber = 1; }

			g_dsmXferStructPtr->numOfSummaries--;						// Decrement total summaries to send
			g_dsmXferStructPtr->xferStateFlag = EVENTREC_XFER_STATE;	// Change state to transfer the record
		}
		else
		{
			g_dsmXferStructPtr->xferStateFlag = FOOTER_XFER_STATE;
		}
	}

	// xfer the event record structure.
	else if (EVENTREC_XFER_STATE == g_dsmXferStructPtr->xferStateFlag)
	{
		g_dsmXferStructPtr->dloadPtr = sendDataNoFlashWrapCheck(g_dsmXferStructPtr->dloadPtr,
			g_dsmXferStructPtr->endDloadPtr);

		if (NULL == g_dsmXferStructPtr->dloadPtr)
		{
			g_dsmXferStructPtr->xferStateFlag = NOP_XFER_STATE;
			g_modemStatus.xferMutex = NO;
			g_transferCount = 0;

			return (NOP_CMD);
		}

		if (g_dsmXferStructPtr->dloadPtr >= g_dsmXferStructPtr->endDloadPtr)
		{
			g_dsmXferStructPtr->xferStateFlag = SUMMARY_TABLE_SEARCH_STATE;
		}
	}

	else if (FOOTER_XFER_STATE == g_dsmXferStructPtr->xferStateFlag)
	{
#if ENDIAN_CONVERSION
		g_transmitCRC = __builtin_bswap32(g_transmitCRC);
#endif
		ModemPuts((uint8*)&g_transmitCRC, 4, NO_CONVERSION);
		ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);
		g_dsmXferStructPtr->xferStateFlag = NOP_XFER_STATE;
		xferState = NOP_CMD;
		g_modemStatus.xferMutex = NO;
		g_transferCount = 0;
	}

	return (xferState);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleACK(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);

#if 0
	uint16 len;
	len = sprintf((char*)g_spareBuffer, "\r\n\r\n<--Received Ack-->\r\n\r\n");
	ModemPuts((uint8*)g_spareBuffer, len, NO_CONVERSION);
#endif

	g_modemStatus.remoteResponse = ACKNOWLEDGE_PACKET;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleNAK(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);

#if 0
	uint16 len;
	len = sprintf((char*)g_spareBuffer, "\r\n\r\n!--Received Nak--!\r\n\r\n");
	ModemPuts((uint8*)g_spareBuffer, len, NO_CONVERSION);
#endif

	g_modemStatus.remoteResponse = NACK_PACKET;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleCAN(CMD_BUFFER_STRUCT* inCmd)
{
	UNUSED(inCmd);

#if 0
	uint16 len;
	len = sprintf((char*)g_spareBuffer, "\r\n\r\nX--Received Can--X\r\n\r\n");
	ModemPuts((uint8*)g_spareBuffer, len, NO_CONVERSION);
#endif

	g_modemStatus.remoteResponse = CANCEL_COMMAND;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleDER(CMD_BUFFER_STRUCT* inCmd)
{
	uint8 derHdr[MESSAGE_HEADER_SIMPLE_LENGTH];
	//DER_REQUEST derRequest;
	char msgTypeStr[8];
	uint8 derError = NO;

	FIL file;
	uint32_t bytesMoved;

	debug("HandleDER: Entry\r\n");

	// Check if the modem processing is busy
	if (g_modemStatus.xferMutex == YES)
	{
		// Ignore request
		return;
	}

	// Clear out the DER working structure
	memset(&g_derXferStruct, 0, sizeof(DERx_XFER_STRUCT));

	// Check to make sure the initial parse is correct
	if (ParseIncommingDERRequestMsg(inCmd, &g_derXferStruct.derRequest) == FAILED) { derError = YES; }

	// Check to make sure event number isn't larger than the max event number
	if (g_derXferStruct.derRequest.eventNumber >= g_nextEventNumberToUse) { derError = YES; }
	// Check to make sure the delay before sending isn't greater than 60 seconds
	if (g_derXferStruct.derRequest.delayBeforeSend > 60000) { derError = YES; }
	// Check to make sure the enable packets option is either 0 or 1
	if (g_derXferStruct.derRequest.enablePackets > 1) { derError = YES; }
	// Check to make sure the enable acknowledge option is either 0 or 1
	if (g_derXferStruct.derRequest.enableAck > 1) { derError = YES; }
	// Check to make sure the packet size is even
	if (g_derXferStruct.derRequest.packetSize & 0x0001) { derError = YES; }

	if (derError == YES)
	{
		debug("HandleDER: Request error\r\n");

		sprintf((char*)msgTypeStr, "%02d", CFG_ERR_BAD_DER_REQUEST);
		BuildOutgoingSimpleHeaderBuffer((uint8*)derHdr, (uint8*)"DERx",	(uint8*)msgTypeStr, MESSAGE_SIMPLE_TOTAL_LENGTH, COMPRESS_NONE, CRC_NONE);

		// Calculate the CRC on the header
		g_transmitCRC = CalcCCITT32((uint8*)&derHdr, MESSAGE_HEADER_SIMPLE_LENGTH, SEED_32);

		// Send Starting CRLF
		ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);
		// Send Simple header
		ModemPuts((uint8*)derHdr, MESSAGE_HEADER_SIMPLE_LENGTH, CONVERT_DATA_TO_ASCII);
		// Send Ending Footer
#if ENDIAN_CONVERSION
		g_transmitCRC = __builtin_bswap32(g_transmitCRC);
#endif
		ModemPuts((uint8*)&g_transmitCRC, 4, NO_CONVERSION);
		ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);

		return;
	}

	// Further validation
/*
	uint16 eventNumber;
	uint32 delayBeforeSend;
	uint16 enablePackets;
	uint16 packetSize;
	uint16 startPacket;
	uint16 numberOfPackets;
	uint16 enableAck;
	uint32 responseTimeout;
*/
/*
	if delayBeforeSend < 3000 then soft delay
	else setup soft timer for callback

	if enablePackets = NO then run DEM
	else run DER

	Want to mimic DEM output (MESSAGE_HEADER_LENGTH with Msg header+footer_event size and flags)
	Cache event summary to get compressed size

*/

	//debug("eventNumToSend = %d \r\n",eventNumToSend);

	// Initialize the flag and time fields.
	g_derXferStruct.xferStateFlag = NOP_XFER_STATE;
	g_derXferStruct.dloadEventRec.structureFlag = START_DLOAD_FLAG;
	g_derXferStruct.dloadEventRec.downloadDate = GetCurrentTime();
	g_derXferStruct.dloadEventRec.endFlag = END_DLOAD_FLAG;
	g_derXferStruct.errorStatus = MODEM_SEND_SUCCESS;

	// Reset flag to always attempt compression
	g_derXferStruct.downloadMethod = COMPRESS_MINILZO;
	g_derXferStruct.compressedEventDataFilePresent = CheckCompressedEventDataFileExists(g_derXferStruct.derRequest.eventNumber);

	// Get the event file record
	GetEventFileRecord(g_derXferStruct.derRequest.eventNumber, &g_derXferStruct.dloadEventRec.eventRecord);

	// Need total uncompressed data size
	g_derXferStruct.uncompressedEventSizePlusMessage = (uint16)(MESSAGE_HEADER_LENGTH + MESSAGE_FOOTER_LENGTH + g_derXferStruct.dloadEventRec.eventRecord.header.headerLength +
															g_derXferStruct.dloadEventRec.eventRecord.header.summaryLength + g_derXferStruct.dloadEventRec.eventRecord.header.dataLength);

	// Make sure compressed data pointer is null (actually done with the memset initially)
	g_derXferStruct.compressedDataPtr = NULL;

	// Determine the necessary download method and setup/cache based on that decision
	if (g_derXferStruct.compressedEventDataFilePresent == YES)
	{
		g_derXferStruct.compressedEventDataSize = GetERDataSize(g_derXferStruct.derRequest.eventNumber);
	}
	// No compressed data file exists but the system is idle so the event data buffer can be used to cache the event
	else if (g_sampleProcessing == IDLE_STATE)
	{
		CacheEventDataToRam(g_derXferStruct.derRequest.eventNumber, (uint8*)&g_eventDataBuffer[0], 0, g_derXferStruct.dloadEventRec.eventRecord.header.dataLength);

		// Need to compress the data portion
		// Check if the data length is less than 1/2 the buffer size (safe guesstimate for data + compressed data size given compression is typically 3:1)
		if (g_derXferStruct.dloadEventRec.eventRecord.header.dataLength < (EVENT_BUFF_SIZE_IN_BYTES / 2))
		{
			g_compressedDataOutletPtr = (uint8*)&g_eventDataBuffer[((g_derXferStruct.dloadEventRec.eventRecord.header.dataLength + 2) / 2)];
			g_derXferStruct.compressedEventDataSize = lzo1x_1_compress((void*)&g_eventDataBuffer[0], g_derXferStruct.dloadEventRec.eventRecord.header.dataLength, OUT_BUFFER);

			g_derXferStruct.compressedDataPtr = (uint8*)&g_eventDataBuffer[((g_derXferStruct.dloadEventRec.eventRecord.header.dataLength + 2) / 2)];
		}
		else // No memory space to work, must save compressed data as a file
		{
			// Get new compressed event filename and path
			GetERDataFilename(g_derXferStruct.dloadEventRec.eventRecord.summary.eventNumber);

			if ((f_open(&file, (const TCHAR*)g_spareFileName, FA_CREATE_ALWAYS | FA_WRITE)) != FR_OK)
			{
				debugErr("Unable to create ERdata event file: %s\r\n", g_spareFileName);
			}
			else // File created, write out the event
			{
				g_globalFileHandle = &file;
				g_spareBufferIndex = 0;
				g_derXferStruct.compressedEventDataSize = lzo1x_1_compress((void*)&g_eventDataBuffer[0], g_derXferStruct.dloadEventRec.eventRecord.header.dataLength, OUT_FILE);

				// Check if any remaining compressed data is queued
				if (g_spareBufferIndex)
				{
					// Finish writing the remaining compressed data
					f_write(&file, g_spareBuffer, g_spareBufferIndex, (UINT*)&bytesMoved);
					g_spareBufferIndex = 0;
				}

				// Done writing the event file, close the file handle
				g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;

				f_close(&file);
				SetFileTimestamp(g_spareFileName);

				g_derXferStruct.compressedEventDataFilePresent = YES;
			}
		}
	}
	else // Actively monitoring, no compressed event data file and can't use event data buffer to cache the event
	{
		// Signal to send the event raw/uncompressed
		g_derXferStruct.downloadMethod = COMPRESS_NONE;

		// Possible to create temp file with compressed event data?
	}

	// Check if the download method is still compress
	if (g_derXferStruct.downloadMethod == COMPRESS_MINILZO)
	{
		// Compress the event record
		g_compressedDataOutletPtr = (uint8*)&g_derXferStruct.compressedEventRecord;
		g_derXferStruct.compressedEventRecordSize = lzo1x_1_compress((void*)&g_derXferStruct.dloadEventRec.eventRecord, sizeof(EVT_RECORD), OUT_BUFFER);

		g_derXferStruct.totalPackageSize = (MESSAGE_HEADER_LENGTH + g_derXferStruct.compressedEventRecordSize + g_derXferStruct.compressedEventDataSize + MESSAGE_FOOTER_LENGTH);
		g_derXferStruct.headerPlusEventRecordBoundary = (MESSAGE_HEADER_LENGTH + g_derXferStruct.compressedEventRecordSize);
	}
	else // (g_derXferStruct.downloadMethod == COMPRESS_NONE)
	{
		g_derXferStruct.totalPackageSize = g_derXferStruct.uncompressedEventSizePlusMessage;
		g_derXferStruct.headerPlusEventRecordBoundary = (MESSAGE_HEADER_LENGTH + sizeof(EVT_RECORD));
	}

	// Tally the total number of packets
	g_derXferStruct.totalPackets = ((g_derXferStruct.totalPackageSize / g_derXferStruct.derRequest.packetSize) + 1);
	g_derXferStruct.currentPacket = g_derXferStruct.derRequest.startPacket;

	if ((g_derXferStruct.derRequest.numberOfPackets) && (g_derXferStruct.totalPackets > (g_derXferStruct.derRequest.startPacket + g_derXferStruct.derRequest.numberOfPackets - 1)))
	{
		g_derXferStruct.endPacket = (g_derXferStruct.derRequest.startPacket + g_derXferStruct.derRequest.numberOfPackets - 1);
	}
	else // Grab all packets from specified start to the end
	{
		g_derXferStruct.endPacket = g_derXferStruct.totalPackets;
	}

	g_dsmXferStructPtr->xferStateFlag = CACHE_PACKETS_STATE;
	g_modemStatus.xferState = DERx_CMD;
	g_modemStatus.xferMutex = YES;
	g_modemStatus.remoteResponse = WAITING_FOR_STATUS;

	// Main - Craft Manager will pick up from here without directly calling
	//ManageDER();
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 ManageDER(void)
{
	uint32 startLocation;
	uint32 cacheSize;

	/*
	==========
	DER Packet
	----------
	<CRLF>
	DER Command
	Event Number
	Packet Number
	Packet Size
	<Packet Data>
	Packet CRC
	<CRLF>

	DEMx0100028142000000000000000032000000 (38)
	*/

	//==========================================================================
	// DER State: Wait for Remote Status
	//--------------------------------------------------------------------------
	if (g_derXferStruct.xferStateFlag == WAIT_REMOTE_STATUS_STATE)
	{
		// Check if the still waiting for a response
		if (g_modemStatus.remoteResponse == WAITING_FOR_STATUS)
		{
			// Check response timeout status (use half second tick)
			// Fill in

			return (DERx_CMD);
		}
		else if (g_modemStatus.remoteResponse == ACKNOWLEDGE_PACKET)
		{
			// Increment packet number
			g_derXferStruct.currentPacket++;

			// Check if finished sending
			if (g_derXferStruct.currentPacket == g_derXferStruct.endPacket)
			{
				// Check if successfully completed the last packet of the event
				if (g_derXferStruct.currentPacket == g_derXferStruct.totalPackets)
				{
					// Update last downloaded event
					if (g_derXferStruct.derRequest.eventNumber > __autoDialoutTbl.lastDownloadedEvent) { __autoDialoutTbl.lastDownloadedEvent = g_derXferStruct.derRequest.eventNumber; }
				}

				// Done with the DER command, clear states and return
				g_derXferStruct.xferStateFlag = NOP_XFER_STATE; g_modemStatus.xferMutex = NO; g_transferCount = 0; return (NOP_CMD);
			}
			// Check if the next packet to send is not cached yet
			else if (g_derXferStruct.currentPacket > g_derXferStruct.cacheEndPacket)
			{
				g_derXferStruct.xferStateFlag = CACHE_PACKETS_STATE;
			}
			else // Next packed to send is already cached
			{
				g_derXferStruct.xferStateFlag = SEND_PACKETS_STATE;
			}
		}
		else if (g_modemStatus.remoteResponse == NACK_PACKET)
		{
			// Resend same packet number
			g_derXferStruct.retransmitPacket = YES;
			g_derXferStruct.xferStateFlag = SEND_PACKETS_STATE;
		}
		else // (g_modemStatus.remoteResponse == CANCEL_COMMAND)
		{
			g_derXferStruct.xferStateFlag = NOP_XFER_STATE; g_modemStatus.xferMutex = NO; g_transferCount = 0; return (NOP_CMD);
		}
	}

	//==========================================================================
	// DER State: Cache Packets
	//--------------------------------------------------------------------------
	if (g_derXferStruct.xferStateFlag == CACHE_PACKETS_STATE)
	{
		startLocation = ((g_derXferStruct.currentPacket - 1) * g_derXferStruct.derRequest.packetSize);
		//endLocation = g_derXferStruct.currentPacket * g_derXferStruct.derRequest.packetSize

		g_derCacheIndex = 0;

		// Check if caching the header and event summary
		if (startLocation < g_derXferStruct.headerPlusEventRecordBoundary)
		{
			// Cache message header
			memcpy(&g_derCache[g_derCacheIndex], &g_derXferStruct.msgHdr[0], MESSAGE_HEADER_LENGTH);
			g_derCacheIndex += MESSAGE_HEADER_LENGTH;

			// Cache compressed event record
			if (g_derXferStruct.downloadMethod == COMPRESS_MINILZO)
			{
				memcpy(&g_derCache[g_derCacheIndex], &g_derXferStruct.compressedEventRecord, g_derXferStruct.compressedEventRecordSize);
				g_derCacheIndex += g_derXferStruct.compressedEventRecordSize;
			}
			else // Cache uncompressed event record
			{
				memcpy(&g_derCache[g_derCacheIndex], &g_derXferStruct.dloadEventRec.eventRecord, sizeof(EVT_RECORD));
				g_derCacheIndex += sizeof(EVT_RECORD);
			}

			// Cache compressed event data
			if (g_derXferStruct.downloadMethod == COMPRESS_MINILZO)
			{
				// Check if the compressed data size is greater than the remaining cache size
				if (g_derXferStruct.compressedEventDataSize > (uint32)(DER_CACHE_SIZE - g_derCacheIndex))
				{
					// Set cache size to remaining cache available
					cacheSize = (DER_CACHE_SIZE - g_derCacheIndex);
				}
				else // Less compressed data than cache space
				{
					// Set cache size to the remaining compressed data available
					cacheSize = g_derXferStruct.compressedEventDataSize;
				}

				// Check if compressed data is in the event data buffer
				if (g_derXferStruct.compressedDataPtr)
				{
					memcpy(&g_derCache[g_derCacheIndex], g_derXferStruct.compressedDataPtr, cacheSize);
				}
				else // Compressed data is in a file
				{
					CacheERDataToBuffer(g_derXferStruct.derRequest.eventNumber, &g_derCache[g_derCacheIndex], 0, cacheSize);
				}
			}
			else // Cache uncompressed event data
			{
				// Check if the uncompressed data size is greater than the remaining cache size
				if (g_derXferStruct.dloadEventRec.eventRecord.header.dataLength > (uint32)(DER_CACHE_SIZE - g_derCacheIndex))
				{
					// Set cache size to remaining cache available
					cacheSize = (DER_CACHE_SIZE - g_derCacheIndex);
				}
				else // Less compressed data than cache space
				{
					// Set cache size to the remaining compressed data available
					cacheSize = g_derXferStruct.dloadEventRec.eventRecord.header.dataLength;
				}

				CacheEventDataToRam(g_derXferStruct.derRequest.eventNumber, &g_derCache[g_derCacheIndex], 0, cacheSize);
			}

			// Find the end packet cached
			g_derXferStruct.cacheEndPacket = (g_derXferStruct.currentPacket + ((DER_CACHE_SIZE / g_derXferStruct.derRequest.packetSize) * g_derXferStruct.derRequest.packetSize) - 1);
		}
		else // Caching only event data
		{
			// Augment the data start location by the original header and event record
			startLocation -= g_derXferStruct.headerPlusEventRecordBoundary;

			// Cache compressed event data
			if (g_derXferStruct.downloadMethod == COMPRESS_MINILZO)
			{
				// Check if the compressed data size is greater than the remaining cache size
				if ((g_derXferStruct.compressedEventDataSize - startLocation) > DER_CACHE_SIZE)
				{
					// Set cache size to remaining cache available
					cacheSize = DER_CACHE_SIZE;
				}
				else // Less compressed data than cache space
				{
					// Set cache size to the remaining compressed data available
					cacheSize = (g_derXferStruct.compressedEventDataSize - startLocation);
				}

				// Check if compressed data is in the event data buffer
				if (g_derXferStruct.compressedDataPtr)
				{
					memcpy(&g_derCache[g_derCacheIndex], &g_derXferStruct.compressedDataPtr[startLocation], cacheSize);
				}
				else // Compressed data is in a file
				{
					CacheERDataToBuffer(g_derXferStruct.derRequest.eventNumber, &g_derCache[0], startLocation, cacheSize);
				}
			}
			else // Cache uncompressed event data
			{
				// Check if the uncompressed data size is greater than the remaining cache size
				if ((g_derXferStruct.dloadEventRec.eventRecord.header.dataLength - startLocation) > DER_CACHE_SIZE)
				{
					// Set cache size to remaining cache available
					cacheSize = DER_CACHE_SIZE;
				}
				else // Less compressed data than cache space
				{
					// Set cache size to the remaining compressed data available
					cacheSize = (g_derXferStruct.dloadEventRec.eventRecord.header.dataLength - startLocation);
				}

				CacheEventDataToRam(g_derXferStruct.derRequest.eventNumber, &g_derCache[0], startLocation, cacheSize);
			}

			// Find the end packet cached
			g_derXferStruct.cacheStartPacket = g_derXferStruct.currentPacket;
			g_derXferStruct.cacheEndPacket = (g_derXferStruct.currentPacket + ((DER_CACHE_SIZE / g_derXferStruct.derRequest.packetSize) * g_derXferStruct.derRequest.packetSize) - 1);
		}

		g_derXferStruct.xferStateFlag = SEND_PACKETS_STATE;
	}

	//==========================================================================
	// DER State: Send Packets
	//--------------------------------------------------------------------------
	if (g_derXferStruct.xferStateFlag == SEND_PACKETS_STATE)
	{
		// Send starting <CRLF>
		// Send the DER packet header (DER Command, Event Number, Packet Number, Packet Size)
		// Send the DER packet data
		// Send the DER CRC
		// Send ending <CRLF>

		// Set response timeout (in half second value rounded up from response timeout value)

		// Fill in packet header
		//strncpy(g_derXferStruct.derPacketHeader.command, "DERx", 4);
		memcpy(&g_derXferStruct.derPacketHeader.command[0], "DERx", 4);
		g_derXferStruct.derPacketHeader.eventNumber = g_derXferStruct.derRequest.eventNumber;
		g_derXferStruct.derPacketHeader.packetNumber = g_derXferStruct.currentPacket;

		// Check if last packet and sending a partial packet size
		if (g_derXferStruct.currentPacket == g_derXferStruct.totalPackets)
		{
			g_derXferStruct.derPacketHeader.packetSize = (sizeof(DER_PACKET_HEADER) + (g_derXferStruct.totalPackageSize % g_derXferStruct.derRequest.packetSize) + (4 * sizeof(uint32)));

			if (!g_derXferStruct.retransmitPacket)
			{
				g_derXferStruct.packetDataXmitSize += g_derXferStruct.derPacketHeader.packetSize;
			}
			// Need to add compressed event header size
			// Need to add compressed event data size
			// Need to add data length including self and CRC
			// Need to add event CRC
		}
		else // Normal packet size
		{
			g_derXferStruct.derPacketHeader.packetSize = (sizeof(DER_PACKET_HEADER) + g_derXferStruct.derRequest.packetSize + sizeof(g_transmitCRC));
			if (!g_derXferStruct.retransmitPacket)
			{
				g_derXferStruct.packetDataXmitSize += g_derXferStruct.derPacketHeader.packetSize;
			}
		}

		// Send starting CRLF
		if (ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION) == MODEM_SEND_FAILED)
		{
			g_derXferStruct.xferStateFlag = NOP_XFER_STATE; g_modemStatus.xferMutex = NO; g_transferCount = 0; return (NOP_CMD);
		}

		// Calculate DER Packet CRC on DER Packet Header
		g_transmitCRC = CalcCCITT32((uint8*)&g_derXferStruct.derPacketHeader, sizeof(DER_PACKET_HEADER), SEED_32);

		// Send DER Packet Header
		if (ModemPuts((uint8*)&g_derXferStruct.derPacketHeader, sizeof(DER_PACKET_HEADER), NO_CONVERSION) == MODEM_SEND_FAILED)
		{
			g_derXferStruct.xferStateFlag = NOP_XFER_STATE; g_modemStatus.xferMutex = NO; g_transferCount = 0; return (NOP_CMD);
		}

		// Find the start of the cached packet data
		startLocation = ((g_derXferStruct.currentPacket - g_derXferStruct.cacheStartPacket) * g_derXferStruct.derRequest.packetSize);

		// Calculate DER Packet CRC on the DER Packet Data
		g_transmitCRC = CalcCCITT32((uint8*)&g_derCache[startLocation], g_derXferStruct.derRequest.packetSize, g_transmitCRC);

		if (!g_derXferStruct.retransmitPacket)
		{
			// Check if the first packet data is being sent
			if (g_derXferStruct.derRequest.startPacket == g_derXferStruct.currentPacket)
			{
				// Start the packet data CRC calculation
				g_derXferStruct.packetDataCRC = CalcCCITT32((uint8*)&g_derCache[startLocation], g_derXferStruct.derRequest.packetSize, SEED_32);
			}
			else // Not the first packet
			{
				// Continue the packet data CRC calculation
				g_derXferStruct.packetDataCRC = CalcCCITT32((uint8*)&g_derCache[startLocation], g_derXferStruct.derRequest.packetSize, g_derXferStruct.packetDataCRC);
			}
		}

		// Send DER Packet Data
		if (ModemPuts((uint8*)&g_derCache[startLocation], g_derXferStruct.derRequest.packetSize, NO_CONVERSION) == MODEM_SEND_FAILED)
		{
			g_derXferStruct.xferStateFlag = NOP_XFER_STATE; g_modemStatus.xferMutex = NO; g_transferCount = 0; return (NOP_CMD);
		}

		// Check if last packet and sending a partial packet size
		if (g_derXferStruct.currentPacket == g_derXferStruct.totalPackets)
		{
			// Fill in handling for uncompressed transmission

			g_transmitCRC = CalcCCITT32((uint8*)&g_derXferStruct.compressedEventRecordSize, sizeof(uint32), g_transmitCRC);
			g_derXferStruct.packetDataCRC = CalcCCITT32((uint8*)&g_derXferStruct.compressedEventRecordSize, sizeof(uint32), g_derXferStruct.packetDataCRC);

			if (ModemPuts((uint8*)&g_derXferStruct.compressedEventRecordSize, sizeof(uint32), NO_CONVERSION) == MODEM_SEND_FAILED)
			{
				g_derXferStruct.xferStateFlag = NOP_XFER_STATE; g_modemStatus.xferMutex = NO; g_transferCount = 0; return (NOP_CMD);
			}

			g_transmitCRC = CalcCCITT32((uint8*)&g_derXferStruct.compressedEventDataSize, sizeof(uint32), g_transmitCRC);
			g_derXferStruct.packetDataCRC = CalcCCITT32((uint8*)&g_derXferStruct.compressedEventDataSize, sizeof(uint32), g_derXferStruct.packetDataCRC);

			if (ModemPuts((uint8*)&g_derXferStruct.compressedEventDataSize, sizeof(uint32), NO_CONVERSION) == MODEM_SEND_FAILED)
			{
				g_derXferStruct.xferStateFlag = NOP_XFER_STATE; g_modemStatus.xferMutex = NO; g_transferCount = 0; return (NOP_CMD);
			}

			g_transmitCRC = CalcCCITT32((uint8*)&g_derXferStruct.packetDataXmitSize, sizeof(uint32), g_transmitCRC);
			g_derXferStruct.packetDataCRC = CalcCCITT32((uint8*)&g_derXferStruct.packetDataXmitSize, sizeof(uint32), g_derXferStruct.packetDataCRC);

			if (ModemPuts((uint8*)&g_derXferStruct.packetDataXmitSize, sizeof(uint32), NO_CONVERSION) == MODEM_SEND_FAILED)
			{
				g_derXferStruct.xferStateFlag = NOP_XFER_STATE; g_modemStatus.xferMutex = NO; g_transferCount = 0; return (NOP_CMD);
			}

			g_transmitCRC = CalcCCITT32((uint8*)&(g_derXferStruct.packetDataCRC), sizeof(uint32), g_transmitCRC);

			if (ModemPuts((uint8*)&g_derXferStruct.packetDataCRC, sizeof(uint32), NO_CONVERSION) == MODEM_SEND_FAILED)
			{
				g_derXferStruct.xferStateFlag = NOP_XFER_STATE; g_modemStatus.xferMutex = NO; g_transferCount = 0; return (NOP_CMD);
			}
		}

		// CRC xmit
#if ENDIAN_CONVERSION
		g_transmitCRC = __builtin_bswap32(g_transmitCRC);
#endif
		if (ModemPuts((uint8*)&g_transmitCRC, sizeof(g_transmitCRC), NO_CONVERSION) == MODEM_SEND_FAILED)
		{
			g_derXferStruct.xferStateFlag = NOP_XFER_STATE; g_modemStatus.xferMutex = NO; g_transferCount = 0; return (NOP_CMD);
		}

		// crlf xmit
		if (ModemPuts((uint8*)&g_CRLF, sizeof(uint16), NO_CONVERSION) == MODEM_SEND_FAILED)
		{
			g_derXferStruct.xferStateFlag = NOP_XFER_STATE; g_modemStatus.xferMutex = NO; g_transferCount = 0; return (NOP_CMD);
		}

		// Reset flag (in case it was set)
		g_derXferStruct.retransmitPacket = NO;

		if (g_derXferStruct.derRequest.enableAck)
		{
			g_derXferStruct.xferStateFlag = WAIT_REMOTE_STATUS_STATE;
			g_modemStatus.remoteResponse = WAITING_FOR_STATUS;

			// Set response timeout (use half second tick)
			// Fill in
		}
		else
		{
			g_derXferStruct.currentPacket++;

			// Check if finished sending all the requested packets
			if (g_derXferStruct.currentPacket == g_derXferStruct.endPacket)
			{
				// Check if successfully completed the last packet of the event
				if (g_derXferStruct.currentPacket == g_derXferStruct.totalPackets)
				{
					// Update last downloaded event
					if (g_derXferStruct.derRequest.eventNumber > __autoDialoutTbl.lastDownloadedEvent) { __autoDialoutTbl.lastDownloadedEvent = g_derXferStruct.derRequest.eventNumber; }
				}

				// Done with the DER command, clear states and return
				g_derXferStruct.xferStateFlag = NOP_XFER_STATE; g_modemStatus.xferMutex = NO; g_transferCount = 0; return (NOP_CMD);
			}
			// Check if the next packet to send is not cached yet
			else if (g_derXferStruct.currentPacket > g_derXferStruct.cacheEndPacket)
			{
				g_derXferStruct.xferStateFlag = CACHE_PACKETS_STATE;
			}
			else // Next packed to send is already cached
			{
				g_derXferStruct.xferStateFlag = SEND_PACKETS_STATE;
			}
		}
	}

	return (DERx_CMD);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SendEventCSVFormat(uint16 eventNumberToSend, uint8 csvOption)
{
	/* Sample Output from Supergraphics
	SIMPLE MATERIALS GROUP
	WATER TANK MONITOR
	DF 12908SM
	33 51 47.6 / 117 31 07.1
	Graph: 12908
	Event #: 3048
	Date: 8/13/2018
	Time: 13:01:41
	Air: 116.1dBL @ 4.9 Hz
	Radial: 2.413 (mm/s) @ 46.5 Hz (Disp. 0.0083 mm) (Accel. 0.072 g's)
	Transverse: 2.540 (mm/s) @ 34.1 Hz (Disp. 0.0119 mm) (Accel. 0.055 g's)
	Vertical: 2.413 (mm/s) @ 42.6 Hz (Disp. 0.0090 mm) (Accel. 0.066 g's)
	Vector Sum: 3.16 (mm/s)
	Sample Rate:1024/sec
	Record Duration: 5.25 seconds
	Air Trigger: 125.0dBL
	Seismic Trigger: 1.016 (mm/s)
	Battery Level: 6.61

	sample	Air	Air	Radial	Transverse	Vertical	Air	Radial	Transverse	Vertical
	Time	dBL	millibars	 (mm/s)	 (mm/s)	 (mm/s)	 Hz	 Hz	 Hz	 Hz
	*/

	EVT_RECORD* eventRecord = &(g_derXferStruct.dloadEventRec.eventRecord);
	uint32 dataSizeRemaining;
	uint32 dataOffset = sizeof(EVT_RECORD);
	uint32 barDataSize;
	uint16 xmitLength = 0;
	uint8 gainFactor;
	uint8 barType;
	uint16 bitAccuracyScale;
	uint16 pullCount;
	uint16 perfectLoops = 1; // Initialize for compiler warning
	uint16 barIntervalCount;
	float airPeak;
	float gUnits;
	char sUnits[8];
	char dUnits[4];
	char aUnits[8];
	SAMPLE_DATA_STRUCT* currentSample;
	uint16* currentBI;
	CALCULATED_DATA_STRUCT* cSum;
	uint32 pullSize;
	uint32 totalXmitLength = 0;
	int32 sampleCount = 0;
	uint16 i;
	float div, airdB, airmb, rUnits, tUnits = 0, vUnits = 0, VS, aFreq = 0, rFreq = 0, tFreq = 0, vFreq = 0;
	uint8 airSensorType;
	uint8 biHour, biMin, biSec;

	//===================================================================================================================================
	// Check that the event is valid by event number cache
	//===================================================================================================================================
	if (g_eventNumberCache[eventNumberToSend] != EVENT_REFERENCE_VALID)
	{
		xmitLength = sprintf((char*)&g_derCache[0], "DET: Event not found for CSV output\r\n");
		ModemPuts((uint8*)&g_derCache[0], xmitLength, NO_CONVERSION);
		return; // Nothing else to be done, time to bail
	}

	//===================================================================================================================================
	// Common factors needed for all sections
	//===================================================================================================================================
	GetEventFileRecord(eventNumberToSend, eventRecord);

	// Set the gain factor that was used to record the event (sensitivity)
	if ((eventRecord->summary.parameters.channel[0].options & 0x01) == GAIN_SELECT_x2) { gainFactor = 2; }
	else { gainFactor = 4; }

	// Set the scale based on the stored bit accuracy the event was recorded with
	switch (eventRecord->summary.parameters.bitAccuracy)
	{
		case ACCURACY_10_BIT: { bitAccuracyScale = ACCURACY_10_BIT_MIDPOINT; } break;
		case ACCURACY_12_BIT: {	bitAccuracyScale = ACCURACY_12_BIT_MIDPOINT; } break;
		case ACCURACY_14_BIT: { bitAccuracyScale = ACCURACY_14_BIT_MIDPOINT; } break;
		default: { bitAccuracyScale = ACCURACY_16_BIT_MIDPOINT; } break; // ACCURACY_16_BIT
	}

	// Calculate the divider used for converting stored A/D peak counts to units of measure
	if ((eventRecord->summary.parameters.seismicSensorType == SENSOR_ACC_832M1_0200) || (eventRecord->summary.parameters.seismicSensorType == SENSOR_ACC_832M1_0500))
	{ div = (float)(bitAccuracyScale * SENSOR_ACCURACY_100X_SHIFT * gainFactor) / (float)(eventRecord->summary.parameters.seismicSensorType * ACC_832M1_SCALER); }
	else { div = (float)(bitAccuracyScale * SENSOR_ACCURACY_100X_SHIFT * gainFactor) / (float)(eventRecord->summary.parameters.seismicSensorType); }

	if (eventRecord->summary.parameters.seismicUnitsOfMeasure == METRIC_TYPE)
	{
		airPeak = HexToMB(eventRecord->summary.calculated.a.peak, DATA_NORMALIZED, bitAccuracyScale, eventRecord->summary.parameters.airSensorType);
		div /= METRIC;
		gUnits = ONE_GRAVITY_IN_MM;
		strcpy(sUnits, "mm/s");
		strcpy(dUnits, "mm");
		strcpy(aUnits, "mb");
	}
	else
	{
		airPeak = HexToDB(eventRecord->summary.calculated.a.peak, DATA_NORMALIZED, bitAccuracyScale, eventRecord->summary.parameters.airSensorType);
		gUnits = ONE_GRAVITY_IN_INCHES;
		strcpy(sUnits, "in/s");
		strcpy(dUnits, "in");
		strcpy(aUnits, "dB");
	}

	airSensorType = (uint8)eventRecord->summary.parameters.airSensorType;

	//===================================================================================================================================
	if ((csvOption == CSV_FULL) || (csvOption == CSV_SUMMARY) || (csvOption == CSV_BARS))
	//===================================================================================================================================
	{
		xmitLength += sprintf((char*)&g_derCache[xmitLength], "Company: %s\r\n", (char*)&(eventRecord->summary.parameters.companyName[0]));
		xmitLength += sprintf((char*)&g_derCache[xmitLength], "Location: %s\r\n", (char*)&(eventRecord->summary.parameters.sessionLocation[0]));
		xmitLength += sprintf((char*)&g_derCache[xmitLength], "Operator: %s\r\n", (char*)&(eventRecord->summary.parameters.seismicOperator[0]));
		xmitLength += sprintf((char*)&g_derCache[xmitLength], "Comments: %s\r\n", (char*)&(eventRecord->summary.parameters.sessionComments[0]));
		xmitLength += sprintf((char*)&g_derCache[xmitLength], "Graph: %s\r\n", (char*)&(eventRecord->summary.version.serialNumber[0]));
		xmitLength += sprintf((char*)&g_derCache[xmitLength], "Event#: %d\r\n", eventRecord->summary.eventNumber);
		if (eventRecord->summary.mode == MANUAL_CAL_MODE) { xmitLength += sprintf((char*)&g_derCache[xmitLength], "Mode: Calibration\r\n"); }
		else if ((eventRecord->summary.mode == WAVEFORM_MODE) || ((eventRecord->summary.mode == COMBO_MODE) && (eventRecord->summary.subMode == WAVEFORM_MODE)))
		{ xmitLength += sprintf((char*)&g_derCache[xmitLength], "Mode: %sWaveform\r\n", (eventRecord->summary.mode == COMBO_MODE ? "Combo-" : "")); }
		else { xmitLength += sprintf((char*)&g_derCache[xmitLength], "Mode: %sBargraph\r\n", (eventRecord->summary.mode == COMBO_MODE ? "Combo-" : "")); }
		if ((eventRecord->summary.mode == BARGRAPH_MODE) || (eventRecord->summary.subMode == BARGRAPH_MODE))
		{
			xmitLength += sprintf((char*)&g_derCache[xmitLength], "Start Time: %d/%d/%d @ %02d:%02d:%02d\r\n", eventRecord->summary.captured.eventTime.month, eventRecord->summary.captured.eventTime.day, eventRecord->summary.captured.eventTime.year,
									eventRecord->summary.captured.eventTime.hour, eventRecord->summary.captured.eventTime.min, eventRecord->summary.captured.eventTime.sec);
			xmitLength += sprintf((char*)&g_derCache[xmitLength], "End Time: %d/%d/%d @ %02d:%02d:%02d\r\n", eventRecord->summary.captured.endTime.month, eventRecord->summary.captured.endTime.day, eventRecord->summary.captured.endTime.year,
									eventRecord->summary.captured.endTime.hour, eventRecord->summary.captured.endTime.min, eventRecord->summary.captured.endTime.sec);
			xmitLength += sprintf((char*)&g_derCache[xmitLength], "Bar Interval: %d secs\r\n", eventRecord->summary.parameters.barInterval);
			xmitLength += sprintf((char*)&g_derCache[xmitLength], "Summary Interval: %d mins\r\n", (eventRecord->summary.parameters.summaryInterval / 60));
		} else /* All non-Bargraph */ {
			xmitLength += sprintf((char*)&g_derCache[xmitLength], "Date: %d/%d/%d\r\n", eventRecord->summary.captured.eventTime.month, eventRecord->summary.captured.eventTime.day, eventRecord->summary.captured.eventTime.year);
			xmitLength += sprintf((char*)&g_derCache[xmitLength], "Time: %02d:%02d:%02d\r\n", eventRecord->summary.captured.eventTime.hour, eventRecord->summary.captured.eventTime.min, eventRecord->summary.captured.eventTime.sec);
		}
		xmitLength += sprintf((char*)&g_derCache[xmitLength], "Air: %.1f %s @ %.1f Hz\r\n", (double)airPeak, aUnits, (double)((float)eventRecord->summary.calculated.a.frequency / 10));
		xmitLength += sprintf((char*)&g_derCache[xmitLength], "Radial: %.4f %s @ %.1f Hz (Disp %0.4f %s) (Accel %0.3f g's)\r\n",
								(double)((float)eventRecord->summary.calculated.r.peak / div), sUnits, (double)((float)eventRecord->summary.calculated.r.frequency / 10), (double)((float)eventRecord->summary.calculated.r.displacement / (float)1000000 / div), dUnits,
								(double)(((float)eventRecord->summary.calculated.r.acceleration / gUnits) / (float)1000 / div));
		xmitLength += sprintf((char*)&g_derCache[xmitLength], "Transverse: %.4f %s @ %.1f Hz (Disp %0.4f %s) (Accel %0.3f g's)\r\n",
								(double)((float)eventRecord->summary.calculated.t.peak / div), sUnits, (double)((float)eventRecord->summary.calculated.t.frequency / 10), (double)((float)eventRecord->summary.calculated.t.displacement / (float)1000000 / div), dUnits,
								(double)(((float)eventRecord->summary.calculated.t.acceleration / gUnits) / (float)1000 / div));
		xmitLength += sprintf((char*)&g_derCache[xmitLength], "Vertical: %.4f %s @ %.1f Hz (Disp %0.4f %s) (Accel %0.3f g's)\r\n",
								(double)((float)eventRecord->summary.calculated.v.peak / div), sUnits, (double)((float)eventRecord->summary.calculated.v.frequency / 10), (double)((float)eventRecord->summary.calculated.v.displacement / (float)1000000 / div), dUnits,
								(double)(((float)eventRecord->summary.calculated.v.acceleration / gUnits) / (float)1000 / div));
		xmitLength += sprintf((char*)&g_derCache[xmitLength], "Vector Sum: %0.2f %s\r\n", (double)(sqrtf((float)eventRecord->summary.calculated.vectorSumPeak) / div), sUnits);
		if (eventRecord->summary.mode == MANUAL_CAL_MODE) { xmitLength += sprintf((char*)&g_derCache[xmitLength], "Sample Rate: 1024/sec\r\n"); }
		else { xmitLength += sprintf((char*)&g_derCache[xmitLength], "Sample Rate: %d/sec\r\n", eventRecord->summary.parameters.sampleRate); }
		xmitLength += sprintf((char*)&g_derCache[xmitLength], "Bit Accuracy: %d-bit\r\n", eventRecord->summary.parameters.bitAccuracy);
		if ((eventRecord->summary.mode != BARGRAPH_MODE) && (eventRecord->summary.subMode != BARGRAPH_MODE))
		{
			if (eventRecord->summary.mode == MANUAL_CAL_MODE) { xmitLength += sprintf((char*)&g_derCache[xmitLength], "Record Time: 100ms\r\n"); }
			else { xmitLength += sprintf((char*)&g_derCache[xmitLength], "Record Time: %lu sec + %0.2f sec pre-trigger\r\n", eventRecord->summary.parameters.recordTime, (double)((float)1 / (float)eventRecord->summary.parameters.pretrigBufferDivider)); }
			if (eventRecord->summary.parameters.airTriggerLevel == NO_TRIGGER_CHAR) { xmitLength += sprintf((char*)&g_derCache[xmitLength], "Air Trigger: No Trigger\r\n"); }
			else { xmitLength += sprintf((char*)&g_derCache[xmitLength], "Air Trigger: %f %s\r\n", (double)((float)eventRecord->summary.parameters.airTriggerLevel / div), aUnits); }
			if (eventRecord->summary.parameters.seismicTriggerLevel == NO_TRIGGER_CHAR) { xmitLength += sprintf((char*)&g_derCache[xmitLength], "Seismic Trigger: No Trigger\r\n"); }
			else if (eventRecord->summary.parameters.seismicTriggerLevel >= VARIABLE_TRIGGER_CHAR_BASE) { xmitLength += sprintf((char*)&g_derCache[xmitLength], "Seismic Trigger: Variable Trigger (Vibration Standard: %lu)\r\n", (eventRecord->summary.parameters.seismicTriggerLevel - VARIABLE_TRIGGER_CHAR_BASE)); }
			else { xmitLength += sprintf((char*)&g_derCache[xmitLength], "Seismic Trigger: %f %s\r\n", (double)((float)eventRecord->summary.parameters.seismicTriggerLevel / div), sUnits); }
		}
		xmitLength += sprintf((char*)&g_derCache[xmitLength], "Battery Level: %0.2f\r\n", (double)((float)eventRecord->summary.captured.batteryLevel / (float)100));

		if (ModemPuts((uint8*)&g_derCache[0], xmitLength, NO_CONVERSION) == MODEM_SEND_FAILED) { /* Failed send */ return; }
	}

	//===================================================================================================================================
	if ((csvOption == CSV_FULL) || (csvOption == CSV_DATA) || (csvOption == CSV_BARS))
	//===================================================================================================================================
	{
		//===================================================================================================================================
		if ((eventRecord->summary.mode != BARGRAPH_MODE) && (eventRecord->summary.subMode != BARGRAPH_MODE)) // Any mode that isn't Bargraph
		//===================================================================================================================================
		{
			xmitLength = sprintf((char*)&g_derCache[0], "Sample Sample Air Air Radial Transverse Vertical Vector\r\nCount Time dBL millibars (%s) (%s) (%s) Sum\r\n", sUnits, sUnits, sUnits);
			if (ModemPuts((uint8*)&g_derCache[0], xmitLength, NO_CONVERSION) == MODEM_SEND_FAILED) { /* Failed send */ return; }

			if (eventRecord->summary.mode == MANUAL_CAL_MODE)
			{
				dataSizeRemaining = (CALIBRATION_NUMBER_OF_SAMPLES * 8);
				sampleCount = 0;
			}
			else // ((eventRecord->summary.mode == WAVEFORM_MODE) || ((eventRecord->summary.mode == COMBO_MODE) && (eventRecord->summary.subMode == WAVEFORM_MODE)))
			{
				// Data size remaining equals pretrigger number of samples plus event number of samples (times 8 bytes for each sample)
				dataSizeRemaining = (((eventRecord->summary.parameters.recordTime * eventRecord->summary.parameters.sampleRate) + (eventRecord->summary.parameters.sampleRate / eventRecord->summary.parameters.pretrigBufferDivider)) * 8);

				// Set the sample count negative based on the pretrigger number of samples, so that the event trigger shows at 0 in time
				sampleCount = (eventRecord->summary.parameters.sampleRate / eventRecord->summary.parameters.pretrigBufferDivider) * -1;
			}

			//-------------------------------------------------------------
			while (dataSizeRemaining)
			{
				if (dataSizeRemaining > CMD_BUFFER_SIZE) { pullSize = CMD_BUFFER_SIZE; }
				else { pullSize = dataSizeRemaining; }

				CacheEventDataToBuffer(eventRecord->summary.eventNumber, (uint8*)&(g_derXferStruct.xmitBuffer[0]), dataOffset, pullSize);
				currentSample = (SAMPLE_DATA_STRUCT*)&(g_derXferStruct.xmitBuffer[0]);

				i = 0;
				while (i < (pullSize / 8))
				{
					// For seismic samples
					//	(float)(sample - bitAccuracyScale) / (float)div
					// For Acoustic sample
					//	sprintf(buff,"%0.3f mb", HexToMB(g_summaryList.cachedEntry.channelSummary.a.peak, DATA_NORMALIZED, bitAccuracyScale, acousticSensorType));
					//	sprintf(buff,"%0.1f dB", HexToDB(g_summaryList.cachedEntry.channelSummary.a.peak, DATA_NORMALIZED, bitAccuracyScale, acousticSensorType));
					airdB = HexToDB(currentSample->a, DATA_NOT_NORMALIZED, bitAccuracyScale, airSensorType);
					airmb = HexToMB(currentSample->a, DATA_NOT_NORMALIZED, bitAccuracyScale, airSensorType);
					rUnits = (float)(currentSample->r - bitAccuracyScale) / (float)div;
					tUnits = (float)(currentSample->t - bitAccuracyScale) / (float)div;
					vUnits = (float)(currentSample->v - bitAccuracyScale) / (float)div;
					VS = sqrt((rUnits * rUnits) + (tUnits * tUnits) + (vUnits * vUnits));

					//------------------------------------------------T,Ad,Am,Ru,Tu,Vu,VS
					xmitLength = sprintf((char*)&g_derCache[0], "%ld %f %f %f %f %f %f %f\r\n", sampleCount, (double)(float)((float)sampleCount/(float)eventRecord->summary.parameters.sampleRate), (double)airdB, (double)airmb, (double)rUnits, (double)tUnits, (double)vUnits, (double)VS);
					if (ModemPuts((uint8*)&g_derCache[0], xmitLength, NO_CONVERSION) == MODEM_SEND_FAILED) { /* Failed send */ return; }

					sampleCount++;
					currentSample++;
					totalXmitLength += xmitLength;
					i++;
				}

				dataOffset += pullSize;
				dataSizeRemaining -= pullSize;
			}
		}
		//===================================================================================================================================
		else // Bargraph and Combo-Bargraph
		//===================================================================================================================================
		{
			barType = eventRecord->summary.parameters.barIntervalDataType;
			barDataSize = ((eventRecord->summary.parameters.summaryInterval / eventRecord->summary.parameters.barInterval) * barType);
			dataSizeRemaining = (eventRecord->summary.calculated.barIntervalsCaptured * barType);

			if (csvOption == CSV_BARS)
			{
				// Bar Intervals only
				if (barType == BAR_INTERVAL_ORIGINAL_DATA_TYPE_SIZE) { xmitLength = sprintf((char*)&g_derCache[0], "Bar Bar Air Air Seismic Vector\r\nInterval Time dBL mb (%s) Sum\r\n", sUnits); }
				else if (barType == BAR_INTERVAL_A_R_V_T_DATA_TYPE_SIZE) { xmitLength = sprintf((char*)&g_derCache[0], "Bar Bar Air Air Radial Transverse Vertical Vector\r\nInterval Time dBL mb (%s) (%s) (%s) Sum\r\n", sUnits, sUnits, sUnits); }
				else { xmitLength = sprintf((char*)&g_derCache[0], "Bar Bar Air Air Air Radial Radial Transverse Transverse Vertical Vertical Vector\r\nInterval Time dBL mb Hz (%s) Hz (%s) Hz (%s) Hz Sum\r\n", sUnits, sUnits, sUnits); }
				if (ModemPuts((uint8*)&g_derCache[0], xmitLength, NO_CONVERSION) == MODEM_SEND_FAILED) { /* Failed send */ return; }

				for (i = barType; i < CMD_BUFFER_SIZE; i += barType)
				{
					if ((barDataSize % i) == 0) { perfectLoops = (barDataSize / i);	}
				}

				if (dataSizeRemaining < (barDataSize / perfectLoops)) { pullSize = dataSizeRemaining; }
				else { pullSize = (barDataSize / perfectLoops); }

				pullCount = 0;
				barIntervalCount = 1;
				biHour = eventRecord->summary.captured.eventTime.hour;
				biMin = eventRecord->summary.captured.eventTime.min;
				biSec = eventRecord->summary.captured.eventTime.sec;

				while (barIntervalCount <= eventRecord->summary.calculated.barIntervalsCaptured)
				{
					CacheEventDataToBuffer(eventRecord->summary.eventNumber, (uint8*)&(g_derXferStruct.xmitBuffer[0]), dataOffset, pullSize);

					currentBI = (uint16*)&(g_derXferStruct.xmitBuffer[0]);

					// Loop through all of the BI's pulled out of the Bargraph event data
					for (i=0; i<(pullSize / barType); i++)
					{
						airdB = HexToDB(currentBI[0], DATA_NORMALIZED, bitAccuracyScale, airSensorType);
						airmb = HexToMB(currentBI[0], DATA_NORMALIZED, bitAccuracyScale, airSensorType);
						rUnits = (float)(currentBI[1]) / (float)div;

						if (barType == BAR_INTERVAL_A_R_V_T_DATA_TYPE_SIZE)
						{
							vUnits = (float)(currentBI[2]) / (float)div;
							tUnits = (float)(currentBI[3]) / (float)div;
							VS = sqrt(*(uint32*)&(currentBI[4]) / div);
						}
						else if (barType == BAR_INTERVAL_A_R_V_T_WITH_FREQ_DATA_TYPE_SIZE)
						{
							vUnits = (float)(currentBI[2]) / (float)div;
							tUnits = (float)(currentBI[3]) / (float)div;
							aFreq = (float)(eventRecord->summary.parameters.sampleRate / (float)currentBI[4]);
							rFreq = (float)(eventRecord->summary.parameters.sampleRate / (float)currentBI[5]);
							vFreq = (float)(eventRecord->summary.parameters.sampleRate / (float)currentBI[6]);
							tFreq = (float)(eventRecord->summary.parameters.sampleRate / (float)currentBI[7]);
							VS = sqrt(*(uint32*)&(currentBI[8]) / div);
						}
						else // barType == BAR_INTERVAL_ORIGINAL_DATA_TYPE_SIZE
						{
							VS = sqrt(*(uint32*)&(currentBI[2]) / div);
						}

						if (barType == BAR_INTERVAL_ORIGINAL_DATA_TYPE_SIZE)
						{
							//-------------------------------------------BI,AdB,Amb,RVTpeak,VS
							xmitLength = sprintf((char*)&g_derCache[0], "%d %02d:%02d:%02d %f %f %f %f\r\n", barIntervalCount++, biHour, biMin, biSec, (double)airdB, (double)airmb, (double)rUnits, (double)VS);
						}
						else if (barType == BAR_INTERVAL_A_R_V_T_DATA_TYPE_SIZE)
						{
							//-------------------------------------------BI,AdB,Amb,Rpeak,Tpeak,Vpeak,VS
							xmitLength = sprintf((char*)&g_derCache[0], "%d %02d:%02d:%02d %f %f %f %f %f %f\r\n", barIntervalCount++, biHour, biMin, biSec, (double)airdB, (double)airmb, (double)rUnits, (double)tUnits, (double)vUnits, (double)VS);
						}
						else // (barType == BAR_INTERVAL_A_R_V_T_WITH_FREQ_DATA_TYPE_SIZE)
						{
							//-------------------------------------------BI,AdB,Amb,Ahz,Rpeak,Rhz,Tpeak,Thz,Vpeak,Vhz,VS
							xmitLength = sprintf((char*)&g_derCache[0], "%d %02d:%02d:%02d %f %f %f %f %f %f %f %f %f %f\r\n", barIntervalCount++, biHour, biMin, biSec, (double)airdB, (double)airmb, (double)aFreq, (double)rUnits, (double)rFreq, (double)tUnits, (double)tFreq, (double)vUnits, (double)vFreq, (double)VS);
						}

						if (ModemPuts((uint8*)&g_derCache[0], xmitLength, NO_CONVERSION) == MODEM_SEND_FAILED) { /* Failed send */ return; }

						// Advance to the next BI in the buffer
						currentBI += (barType / 2);

						// Advance BI time
						biSec += eventRecord->summary.parameters.barInterval;
						if (biSec > 59) { biSec -= 60; biMin++; }
						if (biMin > 59) { biMin -= 60; biHour++; }
						if (biHour > 23) { biHour = 0; }
					}

					dataOffset += pullSize;

					pullCount++;
					if (pullCount % perfectLoops == 0)
					{
						dataOffset += sizeof(CALCULATED_DATA_STRUCT);
					}

					if (((eventRecord->summary.calculated.barIntervalsCaptured - (barIntervalCount - 1)) * barType) < pullSize)
					{
						pullSize = ((eventRecord->summary.calculated.barIntervalsCaptured - (barIntervalCount - 1)) * barType);
					}
				}
			}
			else // (csvOption != CSV_BARS)
			{
				// Summary Intervals only
				xmitLength = sprintf((char*)&g_derCache[0], "Summary Start Air Air Air Air R R R T T T V V V Vector\r\nInterval Time dBL mb Hz Time (%s) Hz Time (%s) Hz Time (%s) Sum\r\n", sUnits, sUnits, sUnits);
				if (ModemPuts((uint8*)&g_derCache[0], xmitLength, NO_CONVERSION) == MODEM_SEND_FAILED) { /* Failed send */ return; }

				pullSize = sizeof(CALCULATED_DATA_STRUCT);
				sampleCount = 1;
				cSum = (CALCULATED_DATA_STRUCT*)&(g_derXferStruct.xmitBuffer[0]);

				if (eventRecord->summary.calculated.summariesCaptured == 1)
				{
					dataOffset += (eventRecord->summary.calculated.barIntervalsCaptured * barType);
				}
				else { dataOffset += barDataSize; }

				while (sampleCount <= eventRecord->summary.calculated.summariesCaptured)
				{
					CacheEventDataToBuffer(eventRecord->summary.eventNumber, (uint8*)&(g_derXferStruct.xmitBuffer[0]), dataOffset, pullSize);

					airdB = HexToDB(cSum->a.peak, DATA_NORMALIZED, bitAccuracyScale, airSensorType);
					airmb = HexToMB(cSum->a.peak, DATA_NORMALIZED, bitAccuracyScale, airSensorType);
					rUnits = (float)(cSum->r.peak) / (float)div;
					tUnits = (float)(cSum->t.peak) / (float)div;
					vUnits = (float)(cSum->v.peak) / (float)div;
					aFreq = (float)((float)eventRecord->summary.parameters.sampleRate / (float)((cSum->a.frequency * 2) - 1));
					rFreq = (float)((float)eventRecord->summary.parameters.sampleRate / (float)((cSum->r.frequency * 2) - 1));
					vFreq = (float)((float)eventRecord->summary.parameters.sampleRate / (float)((cSum->v.frequency * 2) - 1));
					tFreq = (float)((float)eventRecord->summary.parameters.sampleRate / (float)((cSum->t.frequency * 2) - 1));
					VS = (sqrt(cSum->vectorSumPeak) / (double)div);

					//-------------------------------------------SI,Start time,AdB,Amb,Ahz,Atime,Runits,Rhz,Rtime,Tunits,Thz,Ttime,Vunits,Vhz,Vtime,VS
					xmitLength = sprintf((char*)&g_derCache[0], "%lu %02d:%02d:%02d %f %f %f %02d:%02d:%02d %f %f %02d:%02d:%02d %f %f %02d:%02d:%02d %f %f %02d:%02d:%02d %f\r\n",
					sampleCount, cSum->intervalEnd_Time.hour, cSum->intervalEnd_Time.min, cSum->intervalEnd_Time.sec,
					(double)airdB, (double)airmb, (double)aFreq, cSum->a_Time.hour, cSum->a_Time.min, cSum->a_Time.sec,
					(double)rUnits, (double)rFreq, cSum->r_Time.hour, cSum->r_Time.min, cSum->r_Time.sec,
					(double)tUnits, (double)tFreq, cSum->t_Time.hour, cSum->t_Time.min, cSum->t_Time.sec,
					(double)vUnits, (double)vFreq, cSum->v_Time.hour, cSum->v_Time.min, cSum->v_Time.sec, (double)VS);

					if (ModemPuts((uint8*)&g_derCache[0], xmitLength, NO_CONVERSION) == MODEM_SEND_FAILED) { /* Failed send */ return; }

					// Summaries
					sampleCount++;

					if (sampleCount == eventRecord->summary.calculated.summariesCaptured)
					{
						dataOffset += (pullSize + ((eventRecord->summary.calculated.barIntervalsCaptured % (eventRecord->summary.parameters.summaryInterval / eventRecord->summary.parameters.barInterval)) * barType));
					}
					else { dataOffset += (pullSize + barDataSize); }
				}
			}
		}
	}

	ModemPuts((uint8*)&g_CRLF, sizeof(uint16), NO_CONVERSION);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleDEM(CMD_BUFFER_STRUCT* inCmd)
{
	uint16 eventNumToSend;					// In case there is a specific record to print.
	uint32 dataLength;						// Will hold the new data length of the message
	uint16 i = 0, j = 0;
	uint32 msgCRC;
	uint32 inCRC;
	char msgTypeStr[8];
	uint8 spareOption;
	uint8 rawData[5];
	uint8* rawDataPtr = &rawData[0];

	debug("HandleDEM:Entry\r\n");

	// If the process is busy sending data, return;
	if (YES == g_modemStatus.xferMutex)
	{
		return;
	}

	if (YES == ParseIncommingMsgHeader(inCmd, g_inCmdHeaderPtr))
	{
		debug("HandleDEM RTN Error.\r\n");
		return;
	}

	// Check if the CRC32 flag is set (2nd byte of Compress/CRC flags 2 byte field, stored as an ascii number)
	if ((g_inCmdHeaderPtr->compressCrcFlags[1] - 0x30) == CRC_32BIT)
	{
		//Move the string data into the configuration structure. String is (2 * cfgSize)
		i = MESSAGE_HEADER_LENGTH;
		while((i < inCmd->size) && (i < (MESSAGE_HEADER_LENGTH + (sizeof(rawData) * 2))) && (i < CMD_BUFFER_SIZE))
		{
			*rawDataPtr++ = ConvertAscii2Binary(inCmd->msg[i], inCmd->msg[i + 1]);
			i += 2;
		}

		// Set i to the start of the Unit Model 8 byte field (which now contains the ascii equiv. of the CRC32 value)
		i = HDR_CMD_LEN + HDR_TYPE_LEN + HDR_DATALENGTH_LEN;

		// Get the CRC value from incoming command
		while((i < inCmd->size) && (i < CMD_BUFFER_SIZE) && (j < 4))
		{
			((uint8*)&inCRC)[j++] = ConvertAscii2Binary(inCmd->msg[i], inCmd->msg[i + 1]);

			// Set the byte fields to ASCII zero's
			inCmd->msg[i] = 0x30;
			inCmd->msg[i + 1] = 0x30;

			// Advance the pointer
			i += 2;
		}

		// Validate the CRC
		msgCRC = CalcCCITT32((uint8*)&(inCmd->msg[0]), MESSAGE_HEADER_LENGTH, SEED_32);
		msgCRC = CalcCCITT32((uint8*)&(rawData[0]), sizeof(rawData), msgCRC);

		// The CRC's don't match
		if (inCRC != msgCRC)
		{
			// Signal a bad CRC value
			sprintf((char*)msgTypeStr, "%02d", CFG_ERR_BAD_CRC);
			memcpy(g_inCmdHeaderPtr->type, msgTypeStr, HDR_TYPE_LEN);

			// Send Starting CRLF
			ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);

			// Calculate the CRC
			g_transmitCRC = CalcCCITT32((uint8*)&(g_inCmdHeaderPtr->cmd[0]), (uint32)(inCmd->size - 4), SEED_32);

			// Send Simple header
			ModemPuts((uint8*)&(g_inCmdHeaderPtr->cmd[0]), (uint32)(inCmd->size - 4), NO_CONVERSION);

			// Send Ending Footer
#if ENDIAN_CONVERSION
			g_transmitCRC = __builtin_bswap32(g_transmitCRC);
#endif
			ModemPuts((uint8*)&g_transmitCRC, 4, NO_CONVERSION);
			ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);
			return;
		}

	}

#if EXTENDED_DEBUG // Test code (Display command components)
	debugRaw("Recieved DEM command: \r\n");
	for (i = 0; i < inCmd->size; i++)
	{
		debugRaw("(%d)%x ", i+1, inCmd->msg[i]);
	}
	debugRaw("\r\n");

	debugRaw("Command: <%s>, Len: %d\r\n", (char*)(g_inCmdHeaderPtr->cmd), HDR_CMD_LEN);
	debugRaw("Message Type: 0x%x 0x%x, Len: %d\r\n", g_inCmdHeaderPtr->type[0], g_inCmdHeaderPtr->type[1], HDR_TYPE_LEN);
	debugRaw("Data Length: <%s>, Len: %d\r\n", (char*)(g_inCmdHeaderPtr->dataLength), HDR_DATALENGTH_LEN);
	debugRaw("Unit Model: <%s>, Len: %d\r\n", (char*)(g_inCmdHeaderPtr->unitModel), HDR_UNITMODEL_LEN);
	debugRaw("Unit Serial #: <%s>, Len: %d\r\n", (char*)(g_inCmdHeaderPtr->unitSn), HDR_SERIALNUMBER_LEN);
	debugRaw("Compress/CRC Flags: 0x%x 0x%x, Len: %d\r\n", g_inCmdHeaderPtr->compressCrcFlags[0], g_inCmdHeaderPtr->compressCrcFlags[1], HDR_COMPRESSCRC_LEN);
	debugRaw("Software Version: 0x%x 0x%x, Len: %d\r\n", g_inCmdHeaderPtr->softwareVersion[0], g_inCmdHeaderPtr->softwareVersion[1], HDR_SOFTWAREVERSION_LEN);
	debugRaw("Data Version: 0x%x 0x%x, Len: %d\r\n", g_inCmdHeaderPtr->dataVersion[0], g_inCmdHeaderPtr->dataVersion[1], HDR_DATAVERSION_LEN);
	debugRaw("Spare: 0x%x 0x%x, Len: %d\r\n", g_inCmdHeaderPtr->spare[0], g_inCmdHeaderPtr->spare[1], HDR_SPARE_LEN);
#endif

	//-----------------------------------------------------------
	// We need to get the data length. This tells us if we have fields in the command
	// message to parse. The data length portion is length, minus the hdr and footer length.
	dataLength = DataLengthStrToUint32(g_inCmdHeaderPtr->dataLength);
 	dataLength = (uint32)(dataLength - MESSAGE_HEADER_LENGTH - MESSAGE_FOOTER_LENGTH);

	// Verify that this is a valid field and find the number and send it.
	if (dataLength == 6)
	{
		// Expecting a single field, so move to that location.
		eventNumToSend = GetInt16Field(inCmd->msg + MESSAGE_HEADER_LENGTH);
		debug("eventNumToSend = %d \r\n",eventNumToSend);

		spareOption = ConvertAscii2Binary(g_inCmdHeaderPtr->spare[0], g_inCmdHeaderPtr->spare[1]);

		// Check if the download option for CSV format was selected
		if (spareOption == 1)
		{
			SendEventCSVFormat(eventNumToSend, CSV_FULL);

			// Done processing
			return;
		}

		// Clear out the xmit structures and initialize the flag and time fields.
		memset(&(g_demXferStructPtr->dloadEventRec), 0, sizeof(EVENT_RECORD_DOWNLOAD_STRUCT));
		g_demXferStructPtr->xferStateFlag = NOP_XFER_STATE;
		g_demXferStructPtr->dloadEventRec.structureFlag = START_DLOAD_FLAG;
		g_demXferStructPtr->dloadEventRec.downloadDate = GetCurrentTime();
		g_demXferStructPtr->dloadEventRec.endFlag = END_DLOAD_FLAG;
		g_demXferStructPtr->errorStatus = MODEM_SEND_SUCCESS;

		// Reset flag to always attempt compression
#if 0 /* Normal operation */
		g_demXferStructPtr->downloadMethod = COMPRESS_MINILZO;
#else /* Temp disable compression */
		// Todo: Update MiniLZO compression to work directly from stored event before reversing logic
		g_demXferStructPtr->downloadMethod = COMPRESS_NONE;
#endif
		g_demXferStructPtr->compressedEventDataFilePresent = CheckCompressedEventDataFileExists(eventNumToSend);

		// Determine the necessary download method and setup/cache based on that decision
		if (g_demXferStructPtr->compressedEventDataFilePresent == YES)
		{
			GetEventFileRecord(eventNumToSend, &g_demXferStructPtr->dloadEventRec.eventRecord);
		}
		// Check if idle (not actively monitoring) and not in the Summary Menu (which utilizes the event data buffer)
		else if ((g_sampleProcessing == IDLE_STATE) && (g_activeMenu != SUMMARY_MENU))
		{
			if (CacheEventToRam(eventNumToSend, &g_demXferStructPtr->dloadEventRec.eventRecord) == EVENT_CACHE_FAILURE)
			{
				SendErrorMsg((uint8*)"DEMe", (uint8*)MSGTYPE_ERROR_NO_EVENT);
				return;
			}

			g_demXferStructPtr->dataPtr = g_demXferStructPtr->startDataPtr = (uint8*)&g_eventDataBuffer[0];
		}
		else // Actively monitoring, no compressed event data file and can't use event data buffer to cache the event
		{
			// Signal to send the event raw/uncompressed
			g_demXferStructPtr->downloadMethod = COMPRESS_NONE;

			GetEventFileRecord(eventNumToSend, &g_demXferStructPtr->dloadEventRec.eventRecord);

			if (g_activeMenu == SUMMARY_MENU)
			{
				OverlayMessage(getLangText(STATUS_TEXT), "REMOTE EVENT DOWNLOAD REQUEST. PLEASE BE PATIENT", 0);
			}
		}

		// Call to actually send the event
		prepareDEMDataToSend(g_inCmdHeaderPtr);

		if (g_demXferStructPtr->errorStatus == MODEM_SEND_SUCCESS)
		{
			// Update last downloaded event
			if (eventNumToSend > __autoDialoutTbl.lastDownloadedEvent)
			__autoDialoutTbl.lastDownloadedEvent = eventNumToSend;
		}
	}

	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void prepareDEMDataToSend(COMMAND_MESSAGE_HEADER* inCmdHeaderPtr)
{
	uint32 dataLength;						// Will hold the new data length of the message
	uint32 eventRecordXferLength = 0;		// Will hold the length of the compressed event record data.
	uint32 eventDataXferLength = 0;		// Will hold the length of the compressed event data.
	uint8 flagData = 0;
	uint32 dataSizeRemaining;
	uint32 dataOffset;

	// Now start building the outgoing header. Get the initial values from
	// the incoming header. Clear the outgoing header data.
	memset(g_outCmdHeaderPtr, 0, sizeof(COMMAND_MESSAGE_HEADER));
	memcpy(g_outCmdHeaderPtr, inCmdHeaderPtr, sizeof(COMMAND_MESSAGE_HEADER));

	// Start Building the outgoing message header. Set the type to a one for a response message.
	sprintf((char*)g_outCmdHeaderPtr->type, "%02d", MSGTYPE_RESPONSE);

	// Data length is total number of bytes of the uncompressed data.
	dataLength = (uint16)(MESSAGE_HEADER_LENGTH + MESSAGE_FOOTER_LENGTH + g_demXferStructPtr->dloadEventRec.eventRecord.header.headerLength +
					g_demXferStructPtr->dloadEventRec.eventRecord.header.summaryLength + g_demXferStructPtr->dloadEventRec.eventRecord.header.dataLength);
	BuildIntDataField((char*)g_outCmdHeaderPtr->dataLength, dataLength, FIELD_LEN_08);

	//-----------------------------------------------------------
	// Setup the xfer structure ptrs.
	g_demXferStructPtr->startDloadPtr = (uint8*)&(g_demXferStructPtr->dloadEventRec);
	g_demXferStructPtr->dloadPtr = g_demXferStructPtr->startDloadPtr;
	g_demXferStructPtr->endDloadPtr = ((uint8*)g_demXferStructPtr->startDloadPtr + sizeof(EVENT_RECORD_DOWNLOAD_STRUCT));

	dataLength = g_demXferStructPtr->dloadEventRec.eventRecord.header.headerLength + g_demXferStructPtr->dloadEventRec.eventRecord.header.summaryLength +
					g_demXferStructPtr->dloadEventRec.eventRecord.header.dataLength;

	// Build the CRC and compressed fields.
	flagData = CRC_32BIT;
	flagData = (uint8)(flagData << 4);

	//----------------------------------------------------
	// Updated method to set flag for compression or none
	if (g_demXferStructPtr->downloadMethod == COMPRESS_MINILZO)
	{
		flagData = (uint8)((uint8)(flagData) | ((uint8)COMPRESS_MINILZO));
	}
	else
	{
		flagData = (uint8)((uint8)(flagData) | ((uint8)COMPRESS_NONE));
	}

	sprintf((char*)g_outCmdHeaderPtr->compressCrcFlags,"%02x", flagData);
	// Create the message buffer from the outgoing header data.
	BuildOutgoingHeaderBuffer(g_outCmdHeaderPtr, g_demXferStructPtr->msgHdr);

	//----------------------------------------------------
	// Send CRLF start of msg.
	//----------------------------------------------------
	dataLength = 2;
	if (ModemPuts((uint8*)&g_CRLF, dataLength, NO_CONVERSION) == MODEM_SEND_FAILED)
	{
		g_demXferStructPtr->errorStatus = MODEM_SEND_FAILED;
	}
	g_transferCount = 0;						// Do not count the first CRLF

	//----------------------------------------------------
	// Send the communications header
	//----------------------------------------------------
	dataLength = MESSAGE_HEADER_LENGTH;
	g_transmitCRC = CalcCCITT32((uint8*)g_demXferStructPtr->msgHdr, dataLength, SEED_32);
	if (ModemPuts((uint8*)g_demXferStructPtr->msgHdr, dataLength, NO_CONVERSION) == MODEM_SEND_FAILED)
	{
		g_demXferStructPtr->errorStatus = MODEM_SEND_FAILED;
	}
	g_transferCount += dataLength;

	//----------------------------------------------------
	// Compress and download the event record.
	//----------------------------------------------------
	g_demXferStructPtr->xmitSize = 0;

	//----------------------------------------------------
	// Send event record either compressed or not
	//----------------------------------------------------
	if (g_demXferStructPtr->downloadMethod == COMPRESS_MINILZO)
	{
		eventRecordXferLength = lzo1x_1_compress((void*)g_demXferStructPtr->startDloadPtr, sizeof(EVENT_RECORD_DOWNLOAD_STRUCT), OUT_SERIAL);

		if (g_demXferStructPtr->xmitSize > 0)
		{
			g_transmitCRC = CalcCCITT32((uint8*)g_demXferStructPtr->xmitBuffer, g_demXferStructPtr->xmitSize, g_transmitCRC);

			if (ModemPuts((uint8*)g_demXferStructPtr->xmitBuffer, g_demXferStructPtr->xmitSize, NO_CONVERSION) == MODEM_SEND_FAILED)
			{
				g_demXferStructPtr->errorStatus = MODEM_SEND_FAILED;
			}

			g_transferCount += g_demXferStructPtr->xmitSize;
			g_demXferStructPtr->xmitSize = 0;
		}
	}
	else // (g_demXferStructPtr->downloadMethod == COMPRESS_NONE)
	{
		g_transmitCRC = CalcCCITT32((void*)g_demXferStructPtr->startDloadPtr, sizeof(EVENT_RECORD_DOWNLOAD_STRUCT), g_transmitCRC);

		if (ModemPuts((void*)g_demXferStructPtr->startDloadPtr, sizeof(EVENT_RECORD_DOWNLOAD_STRUCT), NO_CONVERSION) == MODEM_SEND_FAILED)
		{
			g_demXferStructPtr->errorStatus = MODEM_SEND_FAILED;
		}

		g_transferCount += sizeof(EVENT_RECORD_DOWNLOAD_STRUCT);
	}

	//----------------------------------------------------
	// Send event data either compressed or not
	//----------------------------------------------------
	if ((g_demXferStructPtr->downloadMethod == COMPRESS_MINILZO) && (g_demXferStructPtr->compressedEventDataFilePresent == YES))
	{
		dataSizeRemaining = GetERDataSize(g_demXferStructPtr->dloadEventRec.eventRecord.summary.eventNumber);
		dataOffset = 0;

		eventDataXferLength = dataSizeRemaining;
		g_transferCount += dataSizeRemaining;
		debug("Compressed Data length (file): %d\r\n", dataSizeRemaining);

		if (dataSizeRemaining > CMD_BUFFER_SIZE)
		{
			while (dataSizeRemaining > CMD_BUFFER_SIZE)
			{
				CacheERDataToBuffer(g_demXferStructPtr->dloadEventRec.eventRecord.summary.eventNumber, (uint8*)g_demXferStructPtr->xmitBuffer, dataOffset, CMD_BUFFER_SIZE);
				g_transmitCRC = CalcCCITT32((uint8*)g_demXferStructPtr->xmitBuffer, CMD_BUFFER_SIZE, g_transmitCRC);

				if (ModemPuts((uint8*)g_demXferStructPtr->xmitBuffer, CMD_BUFFER_SIZE, NO_CONVERSION) == MODEM_SEND_FAILED)
				{
					g_demXferStructPtr->errorStatus = MODEM_SEND_FAILED;
				}

				dataOffset += CMD_BUFFER_SIZE;
				dataSizeRemaining -= CMD_BUFFER_SIZE;

				// Now breaking periodically to handle system events, mostly for Bargraph processing to get it's processing time so that Summary Intervals aren't delayed
				HandleSystemEvents();
			}
		}

		if (dataSizeRemaining)
		{
			CacheERDataToBuffer(g_demXferStructPtr->dloadEventRec.eventRecord.summary.eventNumber, (uint8*)g_demXferStructPtr->xmitBuffer, dataOffset, dataSizeRemaining);
			g_transmitCRC = CalcCCITT32((uint8*)g_demXferStructPtr->xmitBuffer, dataSizeRemaining, g_transmitCRC);

			if (ModemPuts((uint8*)g_demXferStructPtr->xmitBuffer, dataSizeRemaining, NO_CONVERSION) == MODEM_SEND_FAILED)
			{
				g_demXferStructPtr->errorStatus = MODEM_SEND_FAILED;
			}
		}
	}
	// Check if method is compressed but no compressed event data file is present
	else if (g_demXferStructPtr->downloadMethod == COMPRESS_MINILZO)
	{
		g_demXferStructPtr->xmitSize = 0;
		eventDataXferLength = lzo1x_1_compress((void*)(g_demXferStructPtr->startDataPtr), g_demXferStructPtr->dloadEventRec.eventRecord.header.dataLength, OUT_SERIAL);
		debug("Compressed Data length (serial): %d\r\n", eventDataXferLength);

		if (g_demXferStructPtr->xmitSize > 0)
		{
			g_transmitCRC = CalcCCITT32((uint8*)g_demXferStructPtr->xmitBuffer, g_demXferStructPtr->xmitSize, g_transmitCRC);

			if (ModemPuts((uint8*)g_demXferStructPtr->xmitBuffer, g_demXferStructPtr->xmitSize, NO_CONVERSION) == MODEM_SEND_FAILED)
			{
				g_demXferStructPtr->errorStatus = MODEM_SEND_FAILED;
			}

			g_transferCount += g_demXferStructPtr->xmitSize;
			g_demXferStructPtr->xmitSize = 0;
		}
	}
	else // (g_demXferStructPtr->downloadMethod == COMPRESS_NONE)
	{
		// Actively monitoring, can't use event data buffer to cache the event
		dataSizeRemaining = g_demXferStructPtr->dloadEventRec.eventRecord.header.dataLength;
		dataOffset = sizeof(EVT_RECORD);

		eventDataXferLength = dataSizeRemaining;
		g_transferCount += dataSizeRemaining;

		if (dataSizeRemaining > CMD_BUFFER_SIZE)
		{
			while (dataSizeRemaining > CMD_BUFFER_SIZE)
			{
				CacheEventDataToBuffer(g_demXferStructPtr->dloadEventRec.eventRecord.summary.eventNumber, (uint8*)g_demXferStructPtr->xmitBuffer, dataOffset, CMD_BUFFER_SIZE);

				g_transmitCRC = CalcCCITT32((uint8*)g_demXferStructPtr->xmitBuffer, CMD_BUFFER_SIZE, g_transmitCRC);

				if (ModemPuts((uint8*)g_demXferStructPtr->xmitBuffer, CMD_BUFFER_SIZE, NO_CONVERSION) == MODEM_SEND_FAILED)
				{
					g_demXferStructPtr->errorStatus = MODEM_SEND_FAILED;
				}

				dataOffset += CMD_BUFFER_SIZE;
				dataSizeRemaining -= CMD_BUFFER_SIZE;
			}
		}

		if (dataSizeRemaining)
		{
			CacheEventDataToBuffer(g_demXferStructPtr->dloadEventRec.eventRecord.summary.eventNumber, (uint8*)g_demXferStructPtr->xmitBuffer, dataOffset, dataSizeRemaining);

			g_transmitCRC = CalcCCITT32((uint8*)g_demXferStructPtr->xmitBuffer, dataSizeRemaining, g_transmitCRC);

			if (ModemPuts((uint8*)g_demXferStructPtr->xmitBuffer, dataSizeRemaining, NO_CONVERSION) == MODEM_SEND_FAILED)
			{
				g_demXferStructPtr->errorStatus = MODEM_SEND_FAILED;
			}
		}
	}

	//----------------------------------------------------
	// Send data lengths, CRC and end of transmission
	//----------------------------------------------------
	dataLength = sizeof(uint32);
	// send the compressed size of the event record; sizeof(uint32) = 4.
	g_transmitCRC = CalcCCITT32((uint8*)&(eventRecordXferLength), dataLength, g_transmitCRC);
	if (ModemPuts((uint8*)&(eventRecordXferLength), dataLength, NO_CONVERSION) == MODEM_SEND_FAILED)
	{
		g_demXferStructPtr->errorStatus = MODEM_SEND_FAILED;
	}
	g_transferCount += dataLength;

	// send the compressed size of the event data; sizeof(uint32) = 4.
	g_transmitCRC = CalcCCITT32((uint8*)&(eventDataXferLength), dataLength, g_transmitCRC);
	if (ModemPuts((uint8*)&(eventDataXferLength), dataLength, NO_CONVERSION) == MODEM_SEND_FAILED)
	{
		g_demXferStructPtr->errorStatus = MODEM_SEND_FAILED;
	}
	g_transferCount += dataLength;

	// Total xmitted DataLength + size of the count the crc and the crlf.
	//g_transferCount += (dataLength + dataLength + dataLength + 2);	// Not counting last crlf

	g_transferCount += (dataLength + dataLength);
	g_transmitCRC = CalcCCITT32((uint8*)&(g_transferCount), dataLength, g_transmitCRC);
	if (ModemPuts((uint8*)&(g_transferCount), dataLength, NO_CONVERSION) == MODEM_SEND_FAILED)
	{
		g_demXferStructPtr->errorStatus = MODEM_SEND_FAILED;
	}

	// CRC xmit
#if ENDIAN_CONVERSION
	g_transmitCRC = __builtin_bswap32(g_transmitCRC);
#endif
	if (ModemPuts((uint8*)&g_transmitCRC, dataLength, NO_CONVERSION) == MODEM_SEND_FAILED)
	{
		g_demXferStructPtr->errorStatus = MODEM_SEND_FAILED;
	}

	// crlf xmit
	if (ModemPuts((uint8*)&g_CRLF, sizeof(uint16), NO_CONVERSION) == MODEM_SEND_FAILED)
	{
		g_demXferStructPtr->errorStatus = MODEM_SEND_FAILED;
	}

	debug("CRC=%d g_transferCount=%d errorStatus=%s\r\n", g_transmitCRC, g_transferCount, (g_demXferStructPtr->errorStatus == MODEM_SEND_FAILED) ? "Failed" : "Passed");

	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 sendDEMData(void)
{
	uint8 xferState = DEMx_CMD;

	// If the beginning of sending data, send the crlf.
	if (HEADER_XFER_STATE == g_demXferStructPtr->xferStateFlag)
	{
		if (MODEM_SEND_FAILED == ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION))
		{
			g_demXferStructPtr->xferStateFlag = NOP_XFER_STATE;
			g_modemStatus.xferMutex = NO;
			g_transferCount = 0;
			return (NOP_CMD);
		}

		// g_transmitCRC will be the seed value for the rest of the CRC calculations.
		g_transmitCRC = CalcCCITT32((uint8*)g_demXferStructPtr->msgHdr, MESSAGE_HEADER_LENGTH, SEED_32);

		if (MODEM_SEND_FAILED == ModemPuts((uint8*)g_demXferStructPtr->msgHdr,
			MESSAGE_HEADER_LENGTH, g_binaryXferFlag))
		{
			g_demXferStructPtr->xferStateFlag = NOP_XFER_STATE;
			g_modemStatus.xferMutex = NO;
			g_transferCount = 0;
			return (NOP_CMD);
		}

		g_demXferStructPtr->xferStateFlag = EVENTREC_XFER_STATE;
		g_transferCount = MESSAGE_HEADER_LENGTH + 2;
	}

	// xfer the event record structure.
	else if (EVENTREC_XFER_STATE == g_demXferStructPtr->xferStateFlag)
	{
		g_demXferStructPtr->dloadPtr = sendDataNoFlashWrapCheck(g_demXferStructPtr->dloadPtr, g_demXferStructPtr->endDloadPtr);

		if (NULL == g_demXferStructPtr->dloadPtr)
		{
			g_demXferStructPtr->xferStateFlag = NOP_XFER_STATE;
			g_modemStatus.xferMutex = NO;
			g_transferCount = 0;
			return (NOP_CMD);
		}

		if (g_demXferStructPtr->dloadPtr >= g_demXferStructPtr->endDloadPtr)
		{
			g_demXferStructPtr->xferStateFlag = DATA_XFER_STATE;
		}
	}

	// xfer the event data.
	else if (DATA_XFER_STATE == g_demXferStructPtr->xferStateFlag)
	{
		// Does the end ptr wrap in flash? if not then continue;
		if (g_demXferStructPtr->dataPtr < g_demXferStructPtr->endDataPtr)
		{
			g_demXferStructPtr->dataPtr = sendDataNoFlashWrapCheck(g_demXferStructPtr->dataPtr, g_demXferStructPtr->endDataPtr);

			if (NULL == g_demXferStructPtr->dataPtr)
			{
				g_demXferStructPtr->xferStateFlag = NOP_XFER_STATE;
				g_modemStatus.xferMutex = NO;
				g_transferCount = 0;
				return (NOP_CMD);
			}

			if (g_demXferStructPtr->dataPtr >= g_demXferStructPtr->endDataPtr)
			{
				g_demXferStructPtr->xferStateFlag = FOOTER_XFER_STATE;
			}
		}
		else	// The ptr does wrap in flash so the limit is the end of flash.
		{
			g_demXferStructPtr->dataPtr = sendDataNoFlashWrapCheck(g_demXferStructPtr->dataPtr, g_demXferStructPtr->endDataPtr);

			if (NULL == g_demXferStructPtr->dataPtr)
			{
				g_demXferStructPtr->xferStateFlag = NOP_XFER_STATE;
				g_modemStatus.xferMutex = NO;
				g_transferCount = 0;
				return (NOP_CMD);
			}
		}
	}

	else if (FOOTER_XFER_STATE == g_demXferStructPtr->xferStateFlag)
	{
		debug("CRC=%d g_transferCount=%d \r\n", g_transmitCRC, g_transferCount+2);
#if ENDIAN_CONVERSION
		g_transmitCRC = __builtin_bswap32(g_transmitCRC);
#endif
		ModemPuts((uint8*)&g_transmitCRC, 4, NO_CONVERSION);
		ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);
		g_demXferStructPtr->xferStateFlag = NOP_XFER_STATE;
		xferState = NOP_CMD;
		g_modemStatus.xferMutex = NO;
		g_transferCount = 0;
	}

	return (xferState);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8* sendDataNoFlashWrapCheck(uint8* xferPtr, uint8* endPtr)
{
	uint32 xmitSize = XMIT_SIZE_MONITORING;

	if ((xferPtr + xmitSize) < endPtr)
	{
		g_transmitCRC = CalcCCITT32((uint8*)xferPtr, xmitSize, g_transmitCRC);

		if (MODEM_SEND_FAILED == ModemPuts(
			(uint8*)xferPtr, xmitSize, g_binaryXferFlag))
		{
			return (NULL);
		}
	}
	else
	{
		xmitSize = (uint8)(endPtr - xferPtr);

		g_transmitCRC = CalcCCITT32((uint8*)xferPtr, xmitSize, g_transmitCRC);

		if (MODEM_SEND_FAILED == ModemPuts(
			(uint8*)xferPtr, xmitSize, g_binaryXferFlag))
		{
			return (NULL);
		}
	}

	g_transferCount += xmitSize;
	xferPtr += xmitSize;

	return (xferPtr);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void HandleDET(CMD_BUFFER_STRUCT* inCmd)
{
	uint8 csvOption = CSV_FULL;
	char* subString;
	uint32 eventNumberToSend;

	debug("HandleDET:Entry\r\n");

	// If the process is busy sending data, return;
	if (YES == g_modemStatus.xferMutex)
	{
		return;
	}

	subString = strtok ((char*)&(inCmd->msg[0]), ",\r\n");
	subString = strtok (NULL, ",\r\n");

	// Parse event number first
	eventNumberToSend = strtol(subString, NULL, 10);

	subString = strtok (NULL, ",\r\n");

	// Parse CVS format options second
	if ((subString[0] == 'S') || (subString[0] == 's')) { csvOption = CSV_SUMMARY; }
	else if ((subString[0] == 'D') || (subString[0] == 'd')) { csvOption = CSV_DATA; }
	else if ((subString[0] == 'B') || (subString[0] == 'b')) { csvOption = CSV_BARS; }

	SendEventCSVFormat((uint16)eventNumberToSend, csvOption);

	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void handleGMN(CMD_BUFFER_STRUCT* inCmd)
{
	INPUT_MSG_STRUCT mn_msg;
	uint8 gmnHdr[MESSAGE_HEADER_SIMPLE_LENGTH];

	uint8 nibble = 0;
	uint8 tempBuff[4];
	uint16 readDex;
	uint16 buffDex;
	uint32 returnCode = CFG_ERR_NONE;

	if (ACTIVE_STATE == g_sampleProcessing)
	{
		// Error in message length
		returnCode = CFG_ERR_MONITORING_STATE;
	}
	else if (inCmd->size < 18)
	{
		// Error in message length
		returnCode = CFG_ERR_MSG_LENGTH;
	}
	else
	{
		// Not larger then 1 but give some exta
		memset(tempBuff, 0, 4);

		readDex = 16;
		while ((readDex < inCmd->size) && (readDex < 18))
		{
			if ((inCmd->msg[readDex] >= 0x30) && (inCmd->msg[readDex] <= 0x39))
				nibble = (uint8)(inCmd->msg[readDex] - 0x30);
			else if ((inCmd->msg[readDex] >= 0x41) && (inCmd->msg[readDex] <= 0x46))
				nibble = (uint8)(inCmd->msg[readDex] - 0x37);

			buffDex = (uint16)((readDex-16)/2);
			tempBuff[buffDex] = (uint8)((uint8)(tempBuff[buffDex] << 4) + (uint8)nibble);

			readDex++;
		}

		switch (tempBuff[0])
		{
			case WAVEFORM_MODE:
			case BARGRAPH_MODE:
			case COMBO_MODE:
				// Good data
				g_triggerRecord.opMode = tempBuff[0];
				SETUP_MENU_WITH_DATA_MSG(MONITOR_MENU, (uint32)g_triggerRecord.opMode);
				JUMP_TO_ACTIVE_MENU();
				returnCode = CFG_ERR_NONE;
				break;

			default:
				// Invalid trigger mode.
				returnCode = CFG_ERR_TRIGGER_MODE;
				break;
		}
	}

	sprintf((char*)tempBuff, "%02lu", returnCode);
	BuildOutgoingSimpleHeaderBuffer((uint8*)gmnHdr, (uint8*)"GMNx",
		tempBuff, MESSAGE_SIMPLE_TOTAL_LENGTH, COMPRESS_NONE, CRC_NONE);

	// Calculate the CRC on the header
	g_transmitCRC = CalcCCITT32((uint8*)&gmnHdr, MESSAGE_HEADER_SIMPLE_LENGTH, SEED_32);

	// Send Starting CRLF
	ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);
	// Send Simple header
	ModemPuts((uint8*)gmnHdr, MESSAGE_HEADER_SIMPLE_LENGTH, CONVERT_DATA_TO_ASCII);
	// Send Ending Footer
#if ENDIAN_CONVERSION
	g_transmitCRC = __builtin_bswap32(g_transmitCRC);
#endif
	ModemPuts((uint8*)&g_transmitCRC, 4, NO_CONVERSION);
	ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void handleHLT(CMD_BUFFER_STRUCT* inCmd)
{
	INPUT_MSG_STRUCT mn_msg;
	uint8 hltHdr[MESSAGE_HEADER_SIMPLE_LENGTH];
	uint8 msgTypeStr[HDR_TYPE_LEN+1];
	uint8 resultCode = MSGTYPE_RESPONSE;

	UNUSED(inCmd);

	if (ACTIVE_STATE == g_sampleProcessing)
	{
		// Stop 430 data transfers for the current mode and let the event processing handle the rest
		StopMonitoring(g_triggerRecord.opMode, EVENT_PROCESSING);

		// Jump to the main menu
		SETUP_MENU_MSG(MAIN_MENU);
		JUMP_TO_ACTIVE_MENU();
		resultCode = MSGTYPE_RESPONSE;
	}

	sprintf((char*)msgTypeStr, "%02d", resultCode);
	BuildOutgoingSimpleHeaderBuffer((uint8*)hltHdr, (uint8*)"HLTx",
		(uint8*)msgTypeStr, MESSAGE_SIMPLE_TOTAL_LENGTH, COMPRESS_NONE, CRC_NONE);

	// Calculate the CRC on the header
	g_transmitCRC = CalcCCITT32((uint8*)&hltHdr, MESSAGE_HEADER_SIMPLE_LENGTH, SEED_32);

	// Send Starting CRLF
	ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);
	// Send Simple header
	ModemPuts((uint8*)hltHdr, MESSAGE_HEADER_SIMPLE_LENGTH, CONVERT_DATA_TO_ASCII);
	// Send Ending Footer
#if ENDIAN_CONVERSION
	g_transmitCRC = __builtin_bswap32(g_transmitCRC);
#endif
	ModemPuts((uint8*)&g_transmitCRC, 4, NO_CONVERSION);
	ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);

	// Stop the processing.
}
