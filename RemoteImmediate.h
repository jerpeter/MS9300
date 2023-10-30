///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

#ifndef _REMOTE_IMMEDIATE_H_
#define _REMOTE_IMMEDIATE_H_

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include "RemoteCommon.h"
#include "Summary.h"

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------
// Function: HandleUNL - Unlock unit.
void HandleUNL(CMD_BUFFER_STRUCT* inCmd);

// Function: handleRST - Reset the unit.
void handleRST(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleDDP - Toggle debug printing.
void HandleDDP(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleDAI - Download Application Image.
void HandleDAI(CMD_BUFFER_STRUCT* inCmd);


// Function: handleESM - Erase summary memory.
void handleESM(CMD_BUFFER_STRUCT* inCmd);

// Function: handleEEM - Erase event memory.
void handleEEM(CMD_BUFFER_STRUCT* inCmd);

// Function: handleECM - Erase configuration memory.
void handleECM(CMD_BUFFER_STRUCT* inCmd);

// Function: handleECM - Start a Trigger.
void handleTRG(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleVML - View Monitor Log
void HandleVML(CMD_BUFFER_STRUCT* inCmd);
void sendVMLData(void);

// Function: HandleUDE - Update last Downloaded Event number
void HandleUDE(CMD_BUFFER_STRUCT* inCmd);

// Function: handleGAD - Get Auto-dialout/Download information
void handleGAD(CMD_BUFFER_STRUCT* inCmd);

// Function: handleGFS - Get flash stats
void handleGFS(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleDQS - Download Quick Summary Memory.
void HandleDQM(CMD_BUFFER_STRUCT* inCmd);
uint8 sendDQMData(void);

// Function: HandleDSM - Download summary memory.
void HandleDSM(CMD_BUFFER_STRUCT* inCmd);
uint8 sendDSMData(void);

// Function: HandleDEM - Download event memory.
void HandleDEM(CMD_BUFFER_STRUCT* inCmd);
void prepareDEMDataToSend(COMMAND_MESSAGE_HEADER*);
uint8 sendDEMData(void);
uint8* sendDataNoFlashWrapCheck(uint8*, uint8*);

// Function: HandleDET - Download event CSV
void HandleDET(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleDER - Download event resume.
void HandleDER(CMD_BUFFER_STRUCT* inCmd);
uint8 ManageDER(void);
void HandleACK(CMD_BUFFER_STRUCT* inCmd);
void HandleNAK(CMD_BUFFER_STRUCT* inCmd);
void HandleCAN(CMD_BUFFER_STRUCT* inCmd);

// Function: handleGMN - Start Monitoring waveform/bargraph/combo.
void handleGMN(CMD_BUFFER_STRUCT* inCmd);
// Function: handleHLP - Halt Monitoring waveform/bargraph/combo.
void handleHLT(CMD_BUFFER_STRUCT* inCmd);

// Function: handleGLM - Start Bar Live Monitoring
void HandleGLM(CMD_BUFFER_STRUCT* inCmd);
// Function: handleHLM - Halt Bar Live Monitoring
void HandleHLM(CMD_BUFFER_STRUCT* inCmd);
// Function: handleDLM - Download Bar Live Monitoring Pending Event Record
void HandleDLM(CMD_BUFFER_STRUCT* inCmd);
// Function: SendLMA - Live Monitoring Available status
void SendLMA(void);

#endif // _REMOTE_IMMEDIATE_H_

