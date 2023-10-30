///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

#ifndef _REMOTE_HANDLER_H_
#define _REMOTE_HANDLER_H_

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include "RemoteCommon.h"
#include "RemoteOperation.h"
#include "RemoteImmediate.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------
//	Procedure: RemoteCmdMessageHandler() - Start determining the type of command.
uint8 RemoteCmdMessageHandler(CMD_BUFFER_STRUCT* inCmd);


//	Procedure: RemoteCmdMessageHandlerInit() - Initialize the buffer variables
void RemoteCmdMessageHandlerInit(void);

//	Procedure: RemoteCmdMessageProcessing() Processing the incomming data
//	string and determine if a valid cmd has been received. 
void RemoteCmdMessageProcessing(void);
void ProcessCraftData(void);
void CraftInitStatusFlags(void);
void InitCraftInterruptBuffers(void);

#endif // _REMOTE_COMMON_H_

