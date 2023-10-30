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
#include "Common.h"
#include "Uart.h"
#include "SysEvents.h"
#include "RemoteHandler.h"
#include "Menu.h"
#include "SoftTimer.h"
#include "Common.h"
#include "TextTypes.h"

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
static uint8 s_msgReadIndex;
static uint8 s_msgWriteIndex;
static const COMMAND_MESSAGE_STRUCT s_cmdMessageTable[ TOTAL_COMMAND_MESSAGES ] = {
	{ 'A', 'A', 'A', HandleAAA },		// Dummy function call.
	{ 'M', 'R', 'S', HandleMRS },		// Modem reset.

#if 0	
	{ 'M', 'V', 'S', HandleMVS },		// Modem view settings.
	{ 'M', 'P', 'O', HandleMPO },		// Toggle modem on/off.
	{ 'M', 'M', 'O', HandleMMO },		// Toggle modem mode transmit/receive.
	{ 'M', 'N', 'O', HandleMNO },		// Toggle modem phone number A/B/C.
	{ 'M', 'T', 'O', HandleMTO },		// Toggle modem log on/off.
	{ 'M', 'S', 'D', HandleMSD },		// Modem set default initialization string.
	{ 'M', 'S', 'R', HandleMSR },		// Modem set receive initialization string.
	{ 'M', 'S', 'T', HandleMST },		// Modem set transmit initialization string.
	{ 'M', 'S', 'A', HandleMSA },		// Modem set phone number A.
	{ 'M', 'S', 'B', HandleMSB },		// Modem set phone number B.
	{ 'M', 'S', 'C', HandleMSC },		// Modem set phone number C.
	{ 'M', 'V', 'I', HandleMVI },		// Modem view last call in detail.
	{ 'M', 'V', 'O', HandleMVO },		// Modem view last call out detail.
	{ 'V', 'T', 'I', HandleVTI },		// View time.
	{ 'S', 'T', 'I', HandleSTI },		// Set time.
	{ 'V', 'D', 'A', HandleVDA },		// View date.
	{ 'S', 'D', 'A', HandleSDA },		// Set date.
#endif

	// Immediate commands
	{ 'U', 'N', 'L', HandleUNL },		// Unlock unit.
	{ 'R', 'S', 'T', handleRST },		// Reset the unit.
	{ 'D', 'D', 'P', HandleDDP },		// Disable Debug printing.
	{ 'D', 'A', 'I', HandleDAI },		// Download App Image.

#if 0
	{ 'Z', 'R', 'O', HandleZRO },		// Zero sensors.
	{ 'T', 'T', 'O', handleTTO },		// Toggle test mode on/off.
#endif
	{ 'C', 'A', 'L', HandleCAL },		// Calibrate sensors with cal pulse.
#if 0
	{ 'V', 'O', 'L', HandleVOL },		// View on/off log.
	{ 'V', 'C', 'G', HandleVCG },		// View command log.
	{ 'V', 'S', 'L', HandleVSL },		// View summary log.
	{ 'V', 'E', 'L', HandleVEL },		// View event log.
	{ 'E', 'S', 'M', handleESM },		// Erase summary memory.
	{ 'E', 'C', 'M', handleECM },		// Erase configuration memory.
	{ 'T', 'R', 'G', handleTRG },		// Trigger an event.
#endif

	{ 'V', 'M', 'L', HandleVML },		// View Monitor log
	{ 'D', 'Q', 'M', HandleDQM },		// Download summary memory.
	{ 'D', 'S', 'M', HandleDSM },		// Download summary memory.
	{ 'D', 'E', 'M', HandleDEM },		// Download event memory.
	{ 'D', 'E', 'T', HandleDET },		// Download event CSV text
#if 0 /* Command not complete */
	{ 'D', 'E', 'R', HandleDER },		// Download event resume.
#endif
	{ 'E', 'E', 'M', handleEEM },		// Erase event memory.
	{ 'D', 'C', 'M', HandleDCM },		// Download configuration memory.
	{ 'U', 'C', 'M', HandleUCM },		// Upload configuration memory.
	{ 'D', 'M', 'M', HandleDMM },		// Download modem configuration memory.
	{ 'U', 'M', 'M', HandleUMM },		// Upload modem configuration memory.
	{ 'G', 'M', 'N', handleGMN },		// Start Monitoring waveform/bargraph/combo.
	{ 'H', 'L', 'T', handleHLT },		// Halt Monitoring waveform/bargraph/combo.
	{ 'G', 'L', 'M', HandleGLM },		// Get/Start Bar Live Monitoring
	{ 'H', 'L', 'M', HandleHLM },		// Halt Bar Live Monitoring
	{ 'D', 'L', 'M', HandleDLM },		// Download Bar Live Monitoring pending event record
	{ 'U', 'D', 'E', HandleUDE },		// Update last Downloaded Event number
	{ 'G', 'A', 'D', handleGAD },		// Get Auto-Dialout/Download information
	{ 'G', 'F', 'S', handleGFS },		// Get Flash Stats
	{ 'V', 'F', 'V', HandleVFV },		// Get Flash Stats

	{ 'A', 'C', 'K', HandleACK },		// Acknowledge
	{ 'N', 'A', 'K', HandleNAK },		// Nack
	{ 'C', 'A', 'N', HandleCAN },		// Cancel

	{ 'Z', 'Z', 'Z', HandleAAA }		// Help on menus.
};

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitCraftInterruptBuffers(void)
{
	memset(g_isrMessageBufferPtr, 0, sizeof(CMD_BUFFER_STRUCT));
	g_isrMessageBufferPtr->status = CMD_MSG_NO_ERR;
	g_isrMessageBufferPtr->writePtr = g_isrMessageBufferPtr->readPtr = g_isrMessageBufferPtr->msg;
#if 0 /* No longer needed */
	g_isrMessageBufferPtr->overRunCheck = 0xBADD;
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 RemoteCmdMessageHandler(CMD_BUFFER_STRUCT* cmdMsg)
{
	uint8 cmdIndex = 0;
		
	if (g_modemStatus.testingFlag == YES) g_disableDebugPrinting = NO;

	// Commented out since one command (UCM) can blow the buffer and lockup the unit
	//debug("\nCMH:<%s>\r\n", cmdMsg->msg);

	// Fill in the data
	CHAR_UPPER_CASE(cmdMsg->msg[0]);
	CHAR_UPPER_CASE(cmdMsg->msg[1]);
	CHAR_UPPER_CASE(cmdMsg->msg[2]);		
		
	if ((cmdMsg->msg[0] == 'U') &&
		(cmdMsg->msg[1] == 'N') &&
		(cmdMsg->msg[2] == 'L'))
	{
		HandleUNL(cmdMsg);
		
		// If the system is now locked and there is a xfer in progress, 
		// halt the xfer. This is used as a break command.
		if (	(YES == g_modemStatus.systemIsLockedFlag) &&
			(YES == g_modemStatus.xferMutex))
		{
			g_modemStatus.xferState = NOP_CMD;
			g_modemStatus.xferMutex = NO;
		}
	}

	else 
	{ 
		if (NO == g_modemStatus.systemIsLockedFlag)
		{
			debug("System NOT Locked\r\n");
		
			// If the system is unlocked and a xfer command is not in progress 
			// look for the next command to complet. Else, toss the message.
			if (NO == g_modemStatus.xferMutex)
			{
				for (cmdIndex = 0; cmdIndex < TOTAL_COMMAND_MESSAGES; cmdIndex++)
				{
					if ((cmdMsg->msg[0] == s_cmdMessageTable[ cmdIndex ].cmdChar1) &&
						(cmdMsg->msg[1] == s_cmdMessageTable[ cmdIndex ].cmdChar2) &&
						(cmdMsg->msg[2] == s_cmdMessageTable[ cmdIndex ].cmdChar3))
					{
						// Command successfully decoded, signal that data has been transfered
						g_modemDataTransfered = YES;

						WaitForBargraphLiveMonitoringDataToFinishSendingWithTimeout();

						s_cmdMessageTable[ cmdIndex ].cmdFunction(cmdMsg);
						break;
					}
				}
			}
			else // Check for ACK, NAK, CAN while mutex is active
			{
				for (cmdIndex = ACK; cmdIndex < CAN; cmdIndex++)
				{
					if ((cmdMsg->msg[0] == s_cmdMessageTable[ cmdIndex ].cmdChar1) &&
					(cmdMsg->msg[1] == s_cmdMessageTable[ cmdIndex ].cmdChar2) &&
					(cmdMsg->msg[2] == s_cmdMessageTable[ cmdIndex ].cmdChar3))
					{
						// Command successfully decoded, signal that data has been transfered
						g_modemDataTransfered = YES;

						s_cmdMessageTable[ cmdIndex ].cmdFunction(cmdMsg);
						break;
					}
				}
			}	
		}	
		else
		{
			debug("System IS Locked\r\n");
		}
	}

		
	if ((g_modemStatus.testingFlag == YES) && (g_modemStatus.testingPrintFlag == YES))
	{
		g_disableDebugPrinting = YES;
	}

	return (0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void RemoteCmdMessageProcessing()
{
#if 0 /* No longer needed */
	// Check if there is a potentially fatal error.
	if (0xBADD != g_msgPool[s_msgReadIndex].overRunCheck)
	{
		g_msgPool[s_msgReadIndex].overRunCheck = 0xBADD;
		OverlayMessage(getLangText(STATUS_TEXT), getLangText(CRAFT_SERIAL_ERROR_TEXT), 0);
	}
#endif

	// NOTE: Need a different message if the command comming across contains
	// data or additional info other then the 3 char cmd field. Currently
	// assuming that only 3 char cmds are being sent. If data or a field is
	// sent with the incomming command we need to deal with it differently.

	RemoteCmdMessageHandler(&(g_msgPool[s_msgReadIndex]));

	memset(g_msgPool[s_msgReadIndex].msg, 0, CMD_BUFFER_SIZE);
	g_msgPool[s_msgReadIndex].size = 0;	
	g_msgPool[s_msgReadIndex].readPtr = g_msgPool[s_msgReadIndex].msg;	
	g_msgPool[s_msgReadIndex].writePtr = g_msgPool[s_msgReadIndex].msg;
#if 0 /* No longer needed */
	g_msgPool[s_msgReadIndex].overRunCheck = 0xBADD;
#endif
	
	s_msgReadIndex++;
	if (s_msgReadIndex >= CMD_MSG_POOL_SIZE)
	{
		s_msgReadIndex = 0;
	}

	// Are any more buffers filled?
	if (s_msgReadIndex != s_msgWriteIndex)
	{
		// Flag to indicate complete message to process
		raiseSystemEventFlag(CRAFT_PORT_EVENT);
	}

	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ProcessCraftData()
{
	uint8 newPoolBuffer = NO;

	// Halt all debugging message when recieving data.	
	if (g_modemStatus.testingFlag == YES) g_disableDebugPrinting = NO;

#if 0 /* Original */
	// Check status and then reset it to no error.
	if (CMD_MSG_OVERFLOW_ERR == g_isrMessageBufferPtr->status)
	{
		g_isrMessageBufferPtr->status = CMD_MSG_NO_ERR;
		OverlayMessage(getLangText(STATUS_TEXT), getLangText(MODEM_SYNC_FAILED_TEXT), 0);
#if 1 /* New */
		return;
#endif
	}
#endif

#if 0 /* No longer needed */
	// Check status and then reset it to no error.
	if (0xBADD != g_isrMessageBufferPtr->overRunCheck)
	{
		g_isrMessageBufferPtr->overRunCheck = 0xBADD;
		OverlayMessage(getLangText(STATUS_TEXT), getLangText(CRAFT_SERIAL_ERROR_TEXT), 0);
#if 1 /* New */
		return;
#endif
	}
#endif

	while (g_isrMessageBufferPtr->readPtr != g_isrMessageBufferPtr->writePtr)
	{
		debugRaw("<%c>",*g_isrMessageBufferPtr->readPtr);

		if ((*g_isrMessageBufferPtr->readPtr != 0x0A) &&
			(*g_isrMessageBufferPtr->readPtr != 0x0D))
		{
			*(g_msgPool[s_msgWriteIndex].writePtr) = *g_isrMessageBufferPtr->readPtr;
			g_msgPool[s_msgWriteIndex].writePtr++;
			g_msgPool[s_msgWriteIndex].size++;

#if 0 /* Original */
			// The buffer is full, go to the next buffer pool.
			if (g_msgPool[s_msgWriteIndex].size >= (CMD_BUFFER_SIZE-2))
			{
#if 1 /* New addition to add a null to the end of the message for those that get processed as a string */
				*(g_msgPool[s_msgWriteIndex].writePtr) = '\0';
				g_msgPool[s_msgWriteIndex].writePtr++;
				g_msgPool[s_msgWriteIndex].size++;
#endif
				newPoolBuffer = YES;
			}
#else /* Clear buffer if full and no terminating CR or LF, or if there was a serial overrun */
			// Check if the buffer is full with no terminating CR or LF
			if ((g_msgPool[s_msgWriteIndex].size >= (CMD_BUFFER_SIZE-2)) || (g_isrMessageBufferPtr->status == CMD_MSG_OVERFLOW_ERR))
			{
				// Clear buffer
				memset(g_msgPool[s_msgWriteIndex].msg, 0, CMD_BUFFER_SIZE);
				g_msgPool[s_msgWriteIndex].size = 0;
				g_msgPool[s_msgWriteIndex].writePtr = g_msgPool[s_msgWriteIndex].msg;

				if(g_isrMessageBufferPtr->status == CMD_MSG_OVERFLOW_ERR)
				{
					OverlayMessage(getLangText(STATUS_TEXT), getLangText(MODEM_SYNC_FAILED_TEXT), 0);
					g_isrMessageBufferPtr->status = CMD_MSG_NO_ERR;
				}
			}
#endif
		}
		else
		{
			if (g_msgPool[s_msgWriteIndex].size > 0)
			{
#if 1 /* New addition to add a null to the end of the message for those that get processed as a string */
				*(g_msgPool[s_msgWriteIndex].writePtr) = '\0';
				g_msgPool[s_msgWriteIndex].writePtr++;
				g_msgPool[s_msgWriteIndex].size++;
#endif
				newPoolBuffer = YES;
			}	
		}


		if (YES == newPoolBuffer)
		{
			// The message is now complete so go to the next message pool buffer.
			s_msgWriteIndex++;
			if (s_msgWriteIndex >= CMD_MSG_POOL_SIZE)
			{
				s_msgWriteIndex = 0;
			}
			
			// Flag to indicate complete message to process
			raiseSystemEventFlag(CRAFT_PORT_EVENT);
		}
		
		*g_isrMessageBufferPtr->readPtr = 0x00;
		g_isrMessageBufferPtr->readPtr++;
		if (g_isrMessageBufferPtr->readPtr >= (g_isrMessageBufferPtr->msg + CMD_BUFFER_SIZE))
		{			
			g_isrMessageBufferPtr->readPtr = g_isrMessageBufferPtr->msg;
		}
	}

	if ((g_modemStatus.testingFlag == YES) && (g_modemStatus.testingPrintFlag == YES))
	{
		g_disableDebugPrinting = YES;
	}
	
	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void RemoteCmdMessageHandlerInit()
{	
	// Clear and set up the addresses for the ptrs from the buffer array.
	memset(g_msgPool, 0, sizeof(g_msgPool));

	for (s_msgWriteIndex = 0; s_msgWriteIndex < CMD_MSG_POOL_SIZE; s_msgWriteIndex++)
	{	
		g_msgPool[s_msgWriteIndex].readPtr = g_msgPool[s_msgWriteIndex].msg;
		g_msgPool[s_msgWriteIndex].writePtr = g_msgPool[s_msgWriteIndex].msg;
#if 0 /* No longer needed */
		g_msgPool[s_msgWriteIndex].overRunCheck = 0xBADD;
#endif
	}
	
	// Initialize index to the start.
	s_msgWriteIndex = s_msgReadIndex = 0;

	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void CraftInitStatusFlags(void)
{
	memset(&g_modemStatus, 0, sizeof(g_modemStatus));

	// Modem and craft port specific flags.
	g_modemStatus.connectionState = NOP_CMD;	// State flag to indicate which modem command to handle.

	// Check if the Modem setup record is valid and Modem status is yes
	if ((!g_modemSetupRecord.invalid) && (g_modemSetupRecord.modemStatus == YES))
	{
		// Signal that the modem is available
		g_modemStatus.modemAvailable = YES;

		AssignSoftTimer(MODEM_DELAY_TIMER_NUM, (MODEM_ATZ_DELAY), ModemDelayTimerCallback);
	}
	else
	{
		// Signal that the modem is not available
		g_modemStatus.modemAvailable = NO;
	}

	g_modemStatus.craftPortRcvFlag = NO;	// Flag to indicate that incomming data has been received.
	g_modemStatus.xferState = NOP_CMD;		// Flag for xmitting data to the craft.
	g_modemStatus.xferMutex = NO;			// Flag to stop other message command from executing.
	g_modemStatus.systemIsLockedFlag = YES;

	g_modemStatus.ringIndicator = 0;
	g_modemStatus.xferPrintState = NO;
	
	// Modem is being tested/debugged set debug to true.
	g_modemStatus.testingPrintFlag = g_disableDebugPrinting;		
	// Modem is being tested/debugged, set to print to the PC
	g_modemStatus.testingFlag = OFF;
}

