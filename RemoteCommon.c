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
#include "Common.h"
#include "OldUart.h"
#include "Record.h"
#include "RemoteHandler.h"
#include "RemoteCommon.h"
#include "Crc.h"
#include "EventProcessing.h"
#include "SysEvents.h"
#include "SoftTimer.h"

#include "ff.h"
#include "PowerManagement.h"
#include "mxc_errors.h"
#include "uart.h"

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

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 ParseIncommingMsgHeader(CMD_BUFFER_STRUCT* inCmd, COMMAND_MESSAGE_HEADER* incommingHdr)
{
	uint8 errCode = NO;
	char* msgPtr = (char*)inCmd->msg;			// A tempPtr into the message buffer.

	// clear the incomming header data.
	memset((uint8*)incommingHdr, 0, sizeof(COMMAND_MESSAGE_HEADER));

	if (strlen(msgPtr) >= HDR_CMD_LEN)
	{
		// Parse the string into a header data struct.
		memcpy(incommingHdr->cmd, msgPtr, HDR_CMD_LEN);
		msgPtr += HDR_CMD_LEN;
	}

	if (strlen(msgPtr) >= HDR_TYPE_LEN)
	{
		memcpy(incommingHdr->type, msgPtr, HDR_TYPE_LEN);
		msgPtr += HDR_TYPE_LEN;
	}

	if (strlen((char*)inCmd->msg) >= MESSAGE_HEADER_LENGTH)
	{
		memcpy(incommingHdr->dataLength, msgPtr, HDR_DATALENGTH_LEN);
		msgPtr += HDR_DATALENGTH_LEN;

		memcpy(incommingHdr->unitModel, msgPtr, HDR_UNITMODEL_LEN);
		msgPtr += HDR_UNITMODEL_LEN;

		memcpy(incommingHdr->unitSn, msgPtr, HDR_SERIALNUMBER_LEN);
		msgPtr += HDR_SERIALNUMBER_LEN;

		memcpy(incommingHdr->compressCrcFlags, msgPtr, HDR_COMPRESSCRC_LEN);
		msgPtr += HDR_COMPRESSCRC_LEN;

		memcpy(incommingHdr->softwareVersion, msgPtr, HDR_SOFTWAREVERSION_LEN);
		msgPtr += HDR_SOFTWAREVERSION_LEN;

		memcpy(incommingHdr->dataVersion, msgPtr, HDR_DATAVERSION_LEN);
		msgPtr += HDR_DATAVERSION_LEN;

		memcpy(incommingHdr->spare, msgPtr, HDR_SPARE_LEN);
		msgPtr += HDR_SPARE_LEN;
	}
	else
	{
		errCode = YES;
	}

	return (errCode);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 ParseIncommingMsgCmd(CMD_BUFFER_STRUCT* inCmd, COMMAND_MESSAGE_HEADER* incommingHdr)
{
	uint8 errCode = NO;
	char* msgPtr = (char*)inCmd->msg;			// A tempPtr into the message buffer.

	// clear the incomming header data.
	memset((uint8*)incommingHdr, 0, sizeof(COMMAND_MESSAGE_HEADER));

#if 0 /* Original */
	if (strlen(msgPtr) >= HDR_CMD_LEN)
#else /* Size known */
	if (inCmd->size >= HDR_CMD_LEN)
#endif
	{
		// Parse the string into a header data struct.
		memcpy(incommingHdr->cmd, msgPtr, HDR_CMD_LEN);
#if 0 /* Useless */
		msgPtr += HDR_CMD_LEN;
#endif
	}
	else
	{
		errCode = YES;
	}

	return (errCode);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void BuildOutgoingHeaderBuffer(COMMAND_MESSAGE_HEADER* msgHdrData, uint8* msgHdrBuf)
{
	uint8* bufPtr = msgHdrBuf;

	memset(bufPtr, 0, MESSAGE_HEADER_LENGTH+1);

	// Parse the string into a header data struct.
	memcpy(bufPtr, msgHdrData->cmd, HDR_CMD_LEN);
	bufPtr += HDR_CMD_LEN;

	memcpy(bufPtr, msgHdrData->type, HDR_TYPE_LEN);
	bufPtr += HDR_TYPE_LEN;

	memcpy(bufPtr, msgHdrData->dataLength, HDR_DATALENGTH_LEN);
	bufPtr += HDR_DATALENGTH_LEN;

	memcpy(bufPtr, msgHdrData->unitModel, HDR_UNITMODEL_LEN);
	bufPtr += HDR_UNITMODEL_LEN;

	memcpy(bufPtr, msgHdrData->unitSn, HDR_SERIALNUMBER_LEN);
	bufPtr += HDR_SERIALNUMBER_LEN;

	memcpy(bufPtr, msgHdrData->compressCrcFlags, HDR_COMPRESSCRC_LEN);
	bufPtr += HDR_COMPRESSCRC_LEN;

	memcpy(bufPtr, msgHdrData->softwareVersion, HDR_SOFTWAREVERSION_LEN);
	bufPtr += HDR_SOFTWAREVERSION_LEN;

	memcpy(bufPtr, msgHdrData->dataVersion, HDR_DATAVERSION_LEN);
	bufPtr += HDR_DATAVERSION_LEN;

	memcpy(bufPtr, msgHdrData->spare, HDR_SPARE_LEN);
	bufPtr += HDR_SPARE_LEN;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void BuildOutgoingSimpleHeaderBuffer(uint8* msgHdrBuf,
	uint8* msgCmd, uint8* msgType, uint32 dataLength,
	uint8 verFlag, uint8 crcFlag)
{
	uint8* bufPtr = msgHdrBuf;
	//uint8 verData = 0;

	UNUSED(verFlag);
	UNUSED(crcFlag);

	memset(bufPtr, 0, MESSAGE_HEADER_SIMPLE_LENGTH);

	// Parse the string into a header data struct.
	memcpy(bufPtr, msgCmd, HDR_CMD_LEN);
	bufPtr += HDR_CMD_LEN;

	memcpy(bufPtr, msgType, HDR_TYPE_LEN);
	bufPtr += HDR_TYPE_LEN;

	BuildIntDataField((char*)bufPtr, dataLength, FIELD_LEN_08);
	bufPtr += HDR_DATALENGTH_LEN;

	// Put in the version number and if the data message has a crcFlag.
	//*bufPtr++ = verFlag;
	//*bufPtr = crcFlag;
	*bufPtr++ = 0x46;
	*bufPtr = 0x46;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SendErrorMsg(uint8* msgCmd, uint8* msgType)
{
	uint8 errHdr[MESSAGE_HEADER_SIMPLE_LENGTH];

	BuildOutgoingSimpleHeaderBuffer(errHdr, msgCmd, msgType,
		MESSAGE_SIMPLE_TOTAL_LENGTH, COMPRESS_NONE, CRC_NONE);

	// Calculate the CRC on the header
	g_transmitCRC = CalcCCITT32((uint8*)&errHdr, MESSAGE_HEADER_SIMPLE_LENGTH, SEED_32);

	// Send Starting CRLF
	ModemPuts((uint8*)&g_CRLF, 2, NO_CONVERSION);
	// Send Simple header
	ModemPuts((uint8*)errHdr, MESSAGE_HEADER_SIMPLE_LENGTH, CONVERT_DATA_TO_ASCII);
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
uint8 FirstPassValidateCommandString(char* command)
{
	uint16 i;
	uint16 length = strlen(command);

	for (i = 3; i < length; i++)
	{
		// Check if at the trailing CRLF
		if ((command[i] == '\r') || (command[i] == '\n'))
		{
			break;
		}

		// Check to make sure input is either a number or a delimiter
		if (((command[i] < '0') || (command[i] > '9')) && (command[i] != ','))
		{
			return (FAILED);
		}

		// Check for back to back delimiters
		if ((command[i] == ',') && (command[i + 1] == ','))
		{
			return (FAILED);
		}
	}

	return (PASSED);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16 GetInt16Field(uint8* dataPtr)
{
	uint16 int16Data = 0;
	uint8 dataStr[DATA_FIELD_LEN+1];

	memset(dataStr, 0, sizeof(dataStr));
	memcpy(dataStr, dataPtr, DATA_FIELD_LEN);
	int16Data = (uint16)DataLengthStrToUint32(dataStr);

	return (int16Data);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void BuildIntDataField(char* strBuffer, uint32 data, uint8 fieldLen)
{
	if (fieldLen == FIELD_LEN_08)
	{
		sprintf((char*)strBuffer,"%08lu", data);
	}
	else if (fieldLen == FIELD_LEN_06)
	{
		sprintf((char*)strBuffer,"%06lu", data);
	}
	else if (fieldLen == FIELD_LEN_04)
	{
		sprintf((char*)strBuffer,"%04lu", data);
	}
	else if (fieldLen == FIELD_LEN_02)
	{
		sprintf((char*)strBuffer,"%02lu", data);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint32 DataLengthStrToUint32(uint8* dataLengthStr)
{
	uint32 dataLength = 0;
	uint8 strDex = 0;
	uint8 dataLenBuf[HDR_DATALENGTH_LEN+1];

	uint8* dataStr = dataLengthStr;
	uint8* dataBuf = dataLenBuf;


	memset(&dataLenBuf[0], 0, sizeof(dataLenBuf));

	// Look and clear all leading zeros and non digits.
	while (((*dataStr <= '0') 	||
			(*dataStr > '9')) 	&&
		 	(*dataStr != 0x00) 	&&
		 	(strDex < HDR_DATALENGTH_LEN))
	{
		strDex++;
		dataStr++;
	}

	// Copy over all valid digits to the buffer.
	while ((*dataStr != 0x00) && (strDex < HDR_DATALENGTH_LEN))
	{
		// Is it a valid digit, if so copy it over.
		if ((*dataStr >= '0') && (*dataStr <= '9'))
		{
			*dataBuf = *dataStr;
			dataBuf++;
		}
		strDex++;
		dataStr++;
	}

	dataLength = (uint32)atoi((char*)dataLenBuf);

	return (dataLength);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void WriteCompressedData(uint8 compressedData, uint8 outMode)
{
	uint32_t bytesWritten;

	if (outMode == OUT_SERIAL)
	{
		g_demXferStructPtr->xmitBuffer[g_demXferStructPtr->xmitSize] = compressedData;
		g_demXferStructPtr->xmitSize++;

		if (g_demXferStructPtr->xmitSize >= XMIT_SIZE_MONITORING)
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
	else if (outMode == OUT_FILE)
	{
		g_spareBuffer[g_spareBufferIndex] = compressedData;
		g_spareBufferIndex++;

		if (g_spareBufferIndex == SPARE_BUFFER_SIZE)
		{
#if ENDIAN_CONVERSION
			// No conversion for compressed data
#endif
			f_write(g_globalFileHandle, g_spareBuffer, SPARE_BUFFER_SIZE, (UINT*)&bytesWritten);
			g_spareBufferIndex = 0;
		}
	}
	else if (outMode == OUT_BUFFER)
	{
		*g_compressedDataOutletPtr++ = compressedData;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ShutdownPdnAndCellModem(void)
{
	int strLen;

	debug("Cell module: Attempting graceful shutdown...\r\n");

#if 0 /* Useful? */
	sprintf((char*)g_spareBuffer, "AT+CGACT=0,0\r\n"); strLen = (int)strlen((char*)g_spareBuffer); MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen);
	{ SoftUsecWait(500 * SOFT_MSECS); ProcessCraftData(); if (getSystemEventState(CRAFT_PORT_EVENT)) { clearSystemEventFlag(CRAFT_PORT_EVENT); RemoteCmdMessageProcessing(); } }
#endif
	sprintf((char*)g_spareBuffer, "AT+CGATT=0\r\n"); strLen = (int)strlen((char*)g_spareBuffer); MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen);
	{ SoftUsecWait(500 * SOFT_MSECS); ProcessCraftData(); if (getSystemEventState(CRAFT_PORT_EVENT)) { clearSystemEventFlag(CRAFT_PORT_EVENT); RemoteCmdMessageProcessing(); } }
	sprintf((char*)g_spareBuffer, "AT+CFUN=0\r\n"); strLen = (int)strlen((char*)g_spareBuffer); MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen);
	{ SoftUsecWait(500 * SOFT_MSECS); ProcessCraftData(); if (getSystemEventState(CRAFT_PORT_EVENT)) { clearSystemEventFlag(CRAFT_PORT_EVENT); RemoteCmdMessageProcessing(); } }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
char* AdoStateGetDebugString(void)
{
	char* debugString = NULL;

	switch (g_autoDialoutState)
	{
		case AUTO_DIAL_IDLE: debugString = ADO_IDLE; break;
		case AUTO_DIAL_INIT: debugString = ADO_INIT; break;
		case AUTO_DIAL_CONNECTING: debugString = ADO_CONNECTING; break;
		case AUTO_DIAL_CONNECTED: debugString = ADO_CONNECTED; break;
		case AUTO_DIAL_RESPONSE: debugString = ADO_RESPONSE; break;
		case AUTO_DIAL_WAIT: debugString = ADO_WAIT; break;
		case AUTO_DIAL_RESET: debugString = ADO_RESET; break;
		case AUTO_DIAL_RESET_WAIT: debugString = ADO_RESET_WAIT; break;
		case AUTO_DIAL_RETRY: debugString = ADO_RETRY; break;
		case AUTO_DIAL_SLEEP: debugString = ADO_SLEEP; break;
		case AUTO_DIAL_ACTIVE: debugString = ADO_ACTIVE; break;
		case AUTO_DIAL_FINISH: debugString = ADO_FINISH; break;
	}

	return (debugString);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* Not needed yet, unfinished */
char* TcpServerDebugString(void)
{
	char* debugString = NULL;

	switch (g_tcpServerStartStage)
	{
		case 0: debugString = ; break;
		case 1: debugString = ; break;
		case 2: debugString = ; break;
		case 3: debugString = ; break;
		case 4: debugString = ; break;
		case 5: debugString = ; break;
		case 6: debugString = ; break;
		case 7: debugString = ; break;
		case 8: debugString = ; break;
		case 9: debugString = ; break;
	}

	return (debugString);
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetStartCellConnectTime(void)
{
	g_cellConnectStats[0] = g_lifetimeHalfSecondTickCount;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void CellConnectStatsUpdate(void)
{
	g_cellConnectStats[1] = ((g_lifetimeHalfSecondTickCount - g_cellConnectStats[0]) / 2);
	g_cellConnectStats[2] += g_cellConnectStats[1];
	g_cellConnectStats[3]++;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint32 GetCurrentCellConnectTime(void)
{
	return ((g_lifetimeHalfSecondTickCount - g_cellConnectStats[0]) / 2);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint32 GetCellConnectStatsLastConenct(void)
{
	return(g_cellConnectStats[1]);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint32 GetCellConnectStatsAverage(void)
{
	uint32 avg;

	// Saftey check to make sure the divisor is never 0
	if (g_cellConnectStats[3] == 0) { avg = 0; }
	else { avg = (g_cellConnectStats[2] / g_cellConnectStats[3]); }

	return(avg);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitTcpListenServer(void)
{
	if ((!g_cellModemSetupRecord.invalid) && (g_cellModemSetupRecord.tcpServer == YES))
	{
		//OverlayMessage(getLangText(STATUS_TEXT), "CELL MODEM STARTNG LISTEN SERVER. PLEASE WAIT A MOMENT", (0 * SOFT_SECS));

		debug("TCP Listen Server: Selected, setting delayed start timer (15 seconds)\r\n");
		g_tcpServerStartStage = 1;
		AssignSoftTimer(TCP_SERVER_START_NUM, (15 * TICKS_PER_SEC), TcpServerStartCallback);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitAutoDialout(void)
{
	// Check if the table key is not valid
	if (__autoDialoutTblKey != VALID_AUTODIALOUT_TABLE_KEY)
	{
		// Clear AutoDialout Log table
		memset(&__autoDialoutTbl, 0, sizeof(__autoDialoutTbl));

		// No need to set the following since the memset takes care of this
		//__autoDialoutTbl.lastDownloadedEvent = 0;
		//__autoDialoutTbl.lastConnectTime.valid = FALSE;

		__autoDialoutTblKey = VALID_AUTODIALOUT_TABLE_KEY;
	}

	// Update the last stored event
	__autoDialoutTbl.lastStoredEvent = GetLastStoredEventNumber();

	if (g_modemSetupRecord.dialOutType == AUTODIALOUT_EVENTS_CONFIG_STATUS)
	{
		debug("Auto Dialout: Events/Config/Status selected, starting timer (set for %d minutes)\r\n", g_modemSetupRecord.dialOutCycleTime);
		AssignSoftTimer(AUTO_DIAL_OUT_CYCLE_TIMER_NUM, (uint32)(g_modemSetupRecord.dialOutCycleTime * TICKS_PER_MIN), AutoDialOutCycleTimerCallBack);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 CheckAutoDialoutStatusAndFlagIfAvailable(void)
{
	// Check that Dial Out state is currently idle, there is no active modem connection, a modem reset is not in progress, Modem Setup is enabled and the Modem Setup Dial string is not empty
#if 0 /* Original */
	if ((g_autoDialoutState == AUTO_DIAL_IDLE) && (READ_DCD == NO_CONNECTION) && (g_modemResetStage == 0) &&
		(g_modemSetupRecord.modemStatus == YES) && strlen((char*)&(g_modemSetupRecord.dial[0])) != 0)
#elif 0 /* External modem, No modem controls, utilize system lock flag */
	if ((g_autoDialoutState == AUTO_DIAL_IDLE) && (g_modemStatus.systemIsLockedFlag == YES) && (g_modemStatus.modemAvailable == YES) &&
		(g_modemResetStage == 0) && (g_modemSetupRecord.modemStatus == YES) && strlen((char*)&(g_modemSetupRecord.dial[0])) != 0)
#else
	if ((g_autoDialoutState == AUTO_DIAL_IDLE) && (g_modemStatus.systemIsLockedFlag == YES) && (g_modemStatus.modemAvailable == YES) &&
		(g_modemResetStage == 0) && (g_modemSetupRecord.modemStatus == YES) && strlen((char*)&(g_cellModemSetupRecord.server[0])) != 0)
#endif
	{
		raiseSystemEventFlag(AUTO_DIALOUT_EVENT);
		return (YES);
	}

	return (NO);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void StartAutoDialoutProcess(void)
{
	int strLen, status;

#if 0 /* Original */
	if (READ_DCD == NO_CONNECTION)
#else /* No modem controls, utilize system lock flag */
	if (g_modemStatus.systemIsLockedFlag == YES)
#endif
	{
		// Start ADO state machine
		g_autoRetries = g_modemSetupRecord.retries;
		g_autoDialoutState = AUTO_DIAL_INIT;

#if 1 /* New Cell module */
		if (g_cellModemSetupRecord.tcpServer == YES)
		{
			// Check if the TCP Server is running
			if (g_tcpServerStartStage == 0)
			{
				// Escape out of data mode and stop the TCP Server
				debug("+++...\r\n"); sprintf((char*)g_spareBuffer, "+++\r\n"); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
				{ SoftUsecWait(500 * SOFT_MSECS); ProcessCraftData(); if (getSystemEventState(CRAFT_PORT_EVENT)) { clearSystemEventFlag(CRAFT_PORT_EVENT); RemoteCmdMessageProcessing(); } }

				SoftUsecWait(1 * SOFT_SECS);

				// Kill the TCP Server
				debug("AT#XTCPSVR=0...\r\n"); sprintf((char*)g_spareBuffer, "AT#XTCPSVR=0\r\n"); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
			}
			else // the TCP Server is in the processing of starting up
			{
				// Need to postpone and reset cell in case of an error
				debug("Cell/LTE: Postponing TCP Server startup for ADO\r\n");
				g_tcpServerStartStage = 9;
				ClearSoftTimer(TCP_SERVER_START_NUM);

				ShutdownPdnAndCellModem();
				PowerControl(LTE_RESET, ON);
				PowerControl(CELL_ENABLE, OFF);

				// Can't power right back on, need small delay
				SoftUsecWait(5 * SOFT_SECS);
			}
		}

		if(GetPowerControlState(CELL_ENABLE) == OFF)
		{
			debug("Cell/LTE: Powering section...\r\n");
			PowerControl(CELL_ENABLE, ON);
			SoftUsecWait(1 * SOFT_SECS); // Small charge up delay
			PowerControl(LTE_RESET, OFF);

			CheckforModemReady(6);
#if 0 /* Test */
			// System mode setting
			//debug("AT%%XSYSTEMMODE...\r\n"); sprintf((char*)g_spareBuffer, "AT%%XSYSTEMMODE=0,1,0,0\r\n"); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
			debug("AT%%XSYSTEMMODE?...\r\n"); sprintf((char*)g_spareBuffer, "AT%%XSYSTEMMODE?\r\n"); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
			{ ProcessCraftData(); if (getSystemEventState(CRAFT_PORT_EVENT)) { clearSystemEventFlag(CRAFT_PORT_EVENT); RemoteCmdMessageProcessing(); } }
#endif
			if (strlen(g_cellModemSetupRecord.pdnApn))
			{
				// PDN/APN setting, AT+CGDCONT=0,"IP","psmtneofin"
				debug("AT+CGDCONT=0,\"IP\",\"%s\"...\r\n", g_cellModemSetupRecord.pdnApn); sprintf((char*)g_spareBuffer, "AT+CGDCONT=0,\"IP\",\"%s\"\r\n", g_cellModemSetupRecord.pdnApn); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
				{ SoftUsecWait(500 * SOFT_MSECS); ProcessCraftData(); if (getSystemEventState(CRAFT_PORT_EVENT)) { clearSystemEventFlag(CRAFT_PORT_EVENT); RemoteCmdMessageProcessing(); } }
			}

			if (g_cellModemSetupRecord.pdnAuthProtocol != AUTH_NONE)
			{
				debug("AT+CGAUTH=0,%d,\"%s\",\"%s\"...\r\n", g_cellModemSetupRecord.pdnAuthProtocol, g_cellModemSetupRecord.pdnUsername, g_cellModemSetupRecord.pdnPassword);
				sprintf((char*)g_spareBuffer, "AT+CGAUTH=0,%d,\"%s\",\"%s\"\r\n", g_cellModemSetupRecord.pdnAuthProtocol, g_cellModemSetupRecord.pdnUsername, g_cellModemSetupRecord.pdnPassword);
				strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
				{ SoftUsecWait(500 * SOFT_MSECS); ProcessCraftData(); if (getSystemEventState(CRAFT_PORT_EVENT)) { clearSystemEventFlag(CRAFT_PORT_EVENT); RemoteCmdMessageProcessing(); } }
			}

			debug("AT+CFUN=1...\r\n"); sprintf((char*)g_spareBuffer, "AT+CFUN=1\r\n"); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
			{ SoftUsecWait(500 * SOFT_MSECS); ProcessCraftData(); if (getSystemEventState(CRAFT_PORT_EVENT)) { clearSystemEventFlag(CRAFT_PORT_EVENT); RemoteCmdMessageProcessing(); } }
		}

		debug("AT+CEREG?...\r\n"); sprintf((char*)g_spareBuffer, "AT+CEREG?\r\n"); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
#if 1 /* Test */
		SetStartCellConnectTime();
#endif
#endif
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AutoDialoutStateMachine(void)
{
	static uint32 timer = 0;
	CMD_BUFFER_STRUCT msg;

	switch (g_autoDialoutState)
	{
		//----------------------------------------------------------------
		// Send Dial string to connect to remote server
		//----------------------------------------------------------------
		case AUTO_DIAL_INIT:
#if 0 /* Original */
			// Issue dial command and dial string
			if ((g_modemSetupRecord.dial[0] >= '0') && (g_modemSetupRecord.dial[0] <= '9'))
			{
				UartPuts((char *)"ATDT", CRAFT_COM_PORT);
			}
			UartPuts((char *)(g_modemSetupRecord.dial), CRAFT_COM_PORT);
			UartPuts((char *)&g_CRLF, CRAFT_COM_PORT);
#else /* New Cell module */
			;int strLen, status;

			if (g_modemStatus.remoteResponse == MODEM_CELL_NETWORK_REGISTERED)
			{
#if 1 /* Test */
				CellConnectStatsUpdate();
				debug("ADO: Cell network connect time was %d seconds (Avg: %d)\r\n", GetCellConnectStatsLastConenct(), GetCellConnectStatsAverage());
#endif
				debug("AT#XTCPCLI=1,\"%s\",%d...\r\n", g_cellModemSetupRecord.server, g_cellModemSetupRecord.serverPort);
				sprintf((char*)g_spareBuffer, "AT#XTCPCLI=1,\"%s\",%d\r\n", g_cellModemSetupRecord.server, g_cellModemSetupRecord.serverPort); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }

				// Update timer to current tick count
				timer = g_lifetimeHalfSecondTickCount;

				// Advance to Connecting state
				g_autoDialoutState = AUTO_DIAL_CONNECTING;
			}
			else if (GetCurrentCellConnectTime() > CELL_NETWORK_CONNECT_TIMEOUT)
			{
				// Couldn't find a cell network to attach/register, give up and retry later
				debugWarn("ADO: Failed to find cell network\r\n");

				// Update timer to current tick count
				timer = g_lifetimeHalfSecondTickCount;

				// Advance to Retry state
				g_autoDialoutState = AUTO_DIAL_RETRY;
			}
			else if (g_lifetimeHalfSecondTickCount == timer)
			{
				//debug("AT+CEREG?...\r\n");
				sprintf((char*)g_spareBuffer, "AT+CEREG?\r\n"); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }

				// Update timer to current tick count
				timer = g_lifetimeHalfSecondTickCount + 2;

			}
			else if (timer < g_lifetimeHalfSecondTickCount)
			{
				// Initialize (utlize) timer as a control switch for issuing cell registration check
				timer = g_lifetimeHalfSecondTickCount + 2;
			}
#endif
		break;

		//----------------------------------------------------------------
		// Look for remote server conneciton
		//----------------------------------------------------------------
		case AUTO_DIAL_CONNECTING:
			// Check if a remote connection has been established
#if 0 /* Original */
			if (READ_DCD == CONNECTION_ESTABLISHED)
#else /* No modem controls, wait for CONNECT response  */
			if (g_modemStatus.remoteResponse == CONNECT_RESPONSE)
#endif
			{
#if 1 /* New for cell modem */
				debug("AT#XTCPSEND...\r\n"); sprintf((char*)g_spareBuffer, "AT#XTCPSEND\r\n"); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
				//{ SoftUsecWait(500 * SOFT_MSECS); ProcessCraftData(); if (getSystemEventState(CRAFT_PORT_EVENT)) { clearSystemEventFlag(CRAFT_PORT_EVENT); RemoteCmdMessageProcessing(); } }
#else /* Test verify */
				debug("AT+CGDCONT?...\r\n"); sprintf((char*)g_spareBuffer, "AT+CGDCONT?\r\n"); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
				{ SoftUsecWait(500 * SOFT_MSECS); ProcessCraftData(); if (getSystemEventState(CRAFT_PORT_EVENT)) { clearSystemEventFlag(CRAFT_PORT_EVENT); RemoteCmdMessageProcessing(); } }

				debug("AT#XTCPSEND"); sprintf((char*)g_spareBuffer, "AT#XTCPSEND\r\n"); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
				{ SoftUsecWait(500 * SOFT_MSECS); ProcessCraftData(); if (getSystemEventState(CRAFT_PORT_EVENT)) { clearSystemEventFlag(CRAFT_PORT_EVENT); RemoteCmdMessageProcessing(); } }
#endif
				// Clear remote response
				g_modemStatus.remoteResponse = NO_RESPONSE;

				// Check if TCP Server didn't start (due to not connecting), but clear if ADO found conneciton
				if (g_tcpServerStartStage == 9) { g_tcpServerStartStage = 0; }

				// Update timer to current tick count
				timer = g_lifetimeHalfSecondTickCount;

				// Advance to Connected state
				g_autoDialoutState = AUTO_DIAL_CONNECTED;
			}
			// Check if the timer has surpassed 1 minute
			else if ((g_lifetimeHalfSecondTickCount - timer) > (1 * TICKS_PER_MIN))
			{
				// Couldn't establish a connection, give up and retry later
				debugWarn("ADO: Failed to find connection\r\n");

				// Update timer to current tick count
				timer = g_lifetimeHalfSecondTickCount;

#if 1 /* Original */
				// Advance to Retry state
				g_autoDialoutState = AUTO_DIAL_RETRY;
#else
				// Advance to Modem Reset state
				g_autoDialoutState = AUTO_DIAL_RESET;
#endif
			}
			// Check if the TCP Client failed to connect to the server
			else if (g_modemStatus.remoteResponse == TCP_CLIENT_NOT_CONNECTED)
			{
				g_modemStatus.remoteResponse = NO_RESPONSE;

				// Retry server connection
				debug("AT#XTCPCLI=1,\"%s\",%d...\r\n", g_cellModemSetupRecord.server, g_cellModemSetupRecord.serverPort);
				sprintf((char*)g_spareBuffer, "AT#XTCPCLI=1,\"%s\",%d\r\n", g_cellModemSetupRecord.server, g_cellModemSetupRecord.serverPort); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
			}
		break;

		//----------------------------------------------------------------
		// Send out GAD command
		//----------------------------------------------------------------
		case AUTO_DIAL_CONNECTED:
			// Check if the current connection has been established for a second
			if ((g_lifetimeHalfSecondTickCount - timer) > (1 * TICKS_PER_SEC))
			{
				// Make sure transfer flag is set to ascii
				g_binaryXferFlag = NO_CONVERSION;

#if 0 /* Test forcing data mode */
				if (g_modemSetupRecord.init[59] == ENABLED)
				{
					SoftUsecWait((1 * SOFT_SECS));
					UartPuts((char*)(CMDMODE_CMD_STRING), CRAFT_COM_PORT);
					SoftUsecWait((1 * SOFT_SECS));
					UartPuts((char*)"ATO", CRAFT_COM_PORT);
					UartPuts((char*)&g_CRLF, CRAFT_COM_PORT);
					SoftUsecWait((1 * SOFT_SECS));
				}
#endif

				// Send out GAD command (includes serial number and auto dialout parameters)
				debug("ADO: Sending GAD...");
				handleGAD(&msg);

				// Update timer to current tick count
				timer = g_lifetimeHalfSecondTickCount;

				// Advance to Response state
				g_autoDialoutState = AUTO_DIAL_RESPONSE;
			}
#if 0 /* Orignal */
			// Check if we lose the remote connection
			else if (READ_DCD == NO_CONNECTION)
			{
				// Advance to Retry state
				g_autoDialoutState = AUTO_DIAL_RETRY;
			}
#endif
		break;

		//----------------------------------------------------------------
		// Look for system to be unlocked
		//----------------------------------------------------------------
		case AUTO_DIAL_RESPONSE:
			// Check if the system has been unlocked (thus successful receipt of an unlock command)
			if (g_modemStatus.systemIsLockedFlag == NO)
			{
				// Update timer to current tick count
				timer = g_lifetimeHalfSecondTickCount;

				// Advance to Active state
				g_autoDialoutState = AUTO_DIAL_ACTIVE;
			}
			// Check if more than 30 seconds have elapsed without a successful unlock command
			else if ((g_lifetimeHalfSecondTickCount - timer) > (30 * TICKS_PER_SEC))
			{
				// Send out GAD command again
				debug("ADO: Sending GAD...");
				handleGAD(&msg);

				// Update timer to current tick count
				timer = g_lifetimeHalfSecondTickCount;

				// Advance to Wait state
				g_autoDialoutState = AUTO_DIAL_WAIT;
			}
#if 0 /* Orignal */
			// Check if we lose the remote connection
			else if (READ_DCD == NO_CONNECTION)
			{
				// Advance to Retry state
				g_autoDialoutState = AUTO_DIAL_RETRY;
			}
#endif
		break;

		//----------------------------------------------------------------
		// Wait for system to be unlocked (2nd attempt)
		//----------------------------------------------------------------
		case AUTO_DIAL_WAIT:
			// Check if the system has been unlocked (thus successful receipt of an unlock command)
			if (g_modemStatus.systemIsLockedFlag == NO)
			{
				// Update timer to current tick count
				timer = g_lifetimeHalfSecondTickCount;

				// Advance to Active state
				g_autoDialoutState = AUTO_DIAL_ACTIVE;
			}
			// Check if more than 30 seconds have elapsed without a successful unlock command (again, 2nd attempt)
			else if ((g_lifetimeHalfSecondTickCount - timer) > (30 * TICKS_PER_SEC))
			{
				// Update timer to current tick count
				timer = g_lifetimeHalfSecondTickCount;

#if 1 /* Original */
				// Advance to Retry state
				g_autoDialoutState = AUTO_DIAL_RETRY;
#else
				// Advance to Modem Reset state
				g_autoDialoutState = AUTO_DIAL_RESET;
#endif
			}
#if 0 /* Orignal */
			// Check if we lose the remote connection
			else if (READ_DCD == NO_CONNECTION)
			{
				// Advance to Retry state
				g_autoDialoutState = AUTO_DIAL_RETRY;
			}
#endif
		break;

		//----------------------------------------------------------------
		// Issue modem reset handling
		//----------------------------------------------------------------
		case AUTO_DIAL_RESET:
#if 0 /* Original */
			SoftUsecWait((1 * SOFT_SECS));
			UartPuts((char*)(CMDMODE_CMD_STRING), CRAFT_COM_PORT);
			SoftUsecWait((1 * SOFT_SECS));
			UartPuts((char*)(ATZ_CMD_STRING), CRAFT_COM_PORT);
			UartPuts((char*)&g_CRLF, CRAFT_COM_PORT);
#else
			// ADO Reset called if failed 2x on server connect or 2x failed response from server after 2x GAD, in case server conneciton made need to close the conneciton
			SoftUsecWait(1 * SOFT_SECS);
			debug("+++...\r\n"); sprintf((char*)g_spareBuffer, "+++\r\n"); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
			SoftUsecWait(1 * SOFT_SECS);
			debug("AT#XTCPCLI=0...\r\n"); sprintf((char*)g_spareBuffer, "AT#XTCPCLI=0\r\n"); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
#endif
			// Update timer to current tick count
			timer = g_lifetimeHalfSecondTickCount;

			g_autoDialoutState = AUTO_DIAL_RESET_WAIT;
		break;

		//----------------------------------------------------------------
		// Wait for Modem Reset complete
		//----------------------------------------------------------------
		case AUTO_DIAL_RESET_WAIT:
			// Check if the retry time has expired
			if ((g_lifetimeHalfSecondTickCount - timer) > (g_modemSetupRecord.retryTime * TICKS_PER_MIN))
			{
				// Advance to Retry state
				g_autoDialoutState = AUTO_DIAL_RETRY;
			}
		break;

		//----------------------------------------------------------------
		// Start retry handling
		//----------------------------------------------------------------
		case AUTO_DIAL_RETRY:
			// Check if retries have been exhausted
			if (g_autoRetries == 0)
			{
				// Advance to Finish state
				g_autoDialoutState = AUTO_DIAL_FINISH;

				debug("ADO: Out of retries, finished for now\r\n");
			}
			else // Keep trying
			{
				// Decrement retry count
				g_autoRetries--;

#if 0 /* Original */
				// Unable to successfully connect to remote end, start retry with modem reset
				ModemResetProcess();
#else
				debug("ADO: Retry %d\r\n", (g_modemSetupRecord.retries - g_autoRetries));
				ShutdownPdnAndCellModem();
				PowerControl(LTE_RESET, ON);
				PowerControl(CELL_ENABLE, OFF);
#endif
				// Update timer to current tick count
				timer = g_lifetimeHalfSecondTickCount;

				// Advance to Sleep state
				g_autoDialoutState = AUTO_DIAL_SLEEP;
			}
		break;

		//----------------------------------------------------------------
		// Sleep for variable retry time
		//----------------------------------------------------------------
		case AUTO_DIAL_SLEEP:
			// Check if the retry time has expired
			if ((g_lifetimeHalfSecondTickCount - timer) > (g_modemSetupRecord.retryTime * TICKS_PER_MIN))
			{
				// Update timer to current tick count
				timer = g_lifetimeHalfSecondTickCount;

#if 1 /* New Cell module */
				if(GetPowerControlState(CELL_ENABLE) == OFF)
				{
					debug("Cell/LTE: Powering section...\r\n");
					PowerControl(CELL_ENABLE, ON);
					SoftUsecWait(1 * SOFT_SECS); // Small charge up delay
					PowerControl(LTE_RESET, OFF);

					CheckforModemReady(6);

					int strLen, status;
					if (strlen(g_cellModemSetupRecord.pdnApn))
					{
						// PDN/APN setting, AT+CGDCONT=0,"IP","psmtneofin"
						debug("AT+CGDCONT=0,\"IP\",\"%s\"...\r\n", g_cellModemSetupRecord.pdnApn); sprintf((char*)g_spareBuffer, "AT+CGDCONT=0,\"IP\",\"%s\"\r\n", g_cellModemSetupRecord.pdnApn); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
						{ SoftUsecWait(500 * SOFT_MSECS); ProcessCraftData(); if (getSystemEventState(CRAFT_PORT_EVENT)) { clearSystemEventFlag(CRAFT_PORT_EVENT); RemoteCmdMessageProcessing(); } }
					}

					if (g_cellModemSetupRecord.pdnAuthProtocol != AUTH_NONE)
					{
						debug("AT+CGAUTH=0,%d,\"%s\",\"%s\"...\r\n", g_cellModemSetupRecord.pdnAuthProtocol, g_cellModemSetupRecord.pdnUsername, g_cellModemSetupRecord.pdnPassword);
						sprintf((char*)g_spareBuffer, "AT+CGAUTH=0,%d,\"%s\",\"%s\"\r\n", g_cellModemSetupRecord.pdnAuthProtocol, g_cellModemSetupRecord.pdnUsername, g_cellModemSetupRecord.pdnPassword);
						strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
						{ SoftUsecWait(500 * SOFT_MSECS); ProcessCraftData(); if (getSystemEventState(CRAFT_PORT_EVENT)) { clearSystemEventFlag(CRAFT_PORT_EVENT); RemoteCmdMessageProcessing(); } }
					}

					debug("AT+CFUN=1...\r\n"); sprintf((char*)g_spareBuffer, "AT+CFUN=1\r\n"); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
					{ SoftUsecWait(500 * SOFT_MSECS); ProcessCraftData(); if (getSystemEventState(CRAFT_PORT_EVENT)) { clearSystemEventFlag(CRAFT_PORT_EVENT); RemoteCmdMessageProcessing(); } }
				}
				debug("AT+CEREG?...\r\n"); sprintf((char*)g_spareBuffer, "AT+CEREG?\r\n"); strLen = (int)strlen((char*)g_spareBuffer); status = MXC_UART_Write(MXC_UART1, g_spareBuffer, &strLen); if (status != E_SUCCESS) { debugErr("Cell/LTE Uart write failure (%d)\r\n", status); }
#if 1 /* Test */
				SetStartCellConnectTime();
#endif
#endif
				// Start back at Init state
				g_autoDialoutState = AUTO_DIAL_INIT;
			}
		break;

		//----------------------------------------------------------------
		// Active connection
		//----------------------------------------------------------------
		case AUTO_DIAL_ACTIVE:
			// Check if modem data has been transfered (either sent or successful receipt of a message)
			if (g_modemDataTransfered == YES)
			{
				// Toggle the flag off
				g_modemDataTransfered = NO;

				// Update timer to current tick count
				timer = g_lifetimeHalfSecondTickCount;
			}
			// Check if data has not been transmitted in the last 5 minutes
			else if (((g_lifetimeHalfSecondTickCount - timer) > (5 * TICKS_PER_MIN)) || (g_modemStatus.systemIsLockedFlag == YES))
			{
				// No data has been transfered in 5 minutes, tear down connection

				// Advance to Finish state
				g_autoDialoutState = AUTO_DIAL_FINISH;
			}
#if 0 /* Orignal */
			// Check if we lose the remote connection
			else if (READ_DCD == NO_CONNECTION)
			{
				// Advance to Finish state
				g_autoDialoutState = AUTO_DIAL_FINISH;
			}
#endif
		break;

		//----------------------------------------------------------------
		// Finished with Auto Dialout (either successful connection of failed retries)
		//----------------------------------------------------------------
		case AUTO_DIAL_FINISH:
#if 1 // Handle BLM feature
			if (g_bargraphLiveMonitoringBISendActive == YES) { g_bargraphLiveMonitoringBISendActive = NO; } // Kill the Bar live data transfer

			// Stop sending BLM data unless remote side requests
			g_modemStatus.barLiveMonitorOverride = BAR_LIVE_MONITORING_OVERRIDE_STOP;
#endif
			// Done with Auto Dialout processing, issue a modem reset
			ModemResetProcess();

			if (g_modemSetupRecord.dialOutType == AUTODIALOUT_EVENTS_CONFIG_STATUS)
			{
				debug("ADO: restart timer\r\n");
				AssignSoftTimer(AUTO_DIAL_OUT_CYCLE_TIMER_NUM, (uint32)(g_modemSetupRecord.dialOutCycleTime * TICKS_PER_MIN), AutoDialOutCycleTimerCallBack);
			}

			// Check for a special case where ADO Events only no active connections called out but failed to connect
			if (__autoDialoutTbl.currentCycleConnects == 0xFFFF)
			{
				// Reset the current cycle counts for the special case where ADO Events only call out counted itself
				__autoDialoutTbl.currentCycleConnects = 0;
			}

			// Place in Idle state
			g_autoDialoutState = AUTO_DIAL_IDLE;
		break;

		//----------------------------------------------------------------
		// Idle
		//----------------------------------------------------------------
		case AUTO_DIAL_IDLE:
			// Do nothing
		break;
	}
}
