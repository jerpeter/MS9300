///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

#ifndef _REMOTE_OPERATION_H_
#define _REMOTE_OPERATION_H_

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include "RemoteCommon.h"

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------
//==================================================
// Dummy commands
//==================================================

// Function: HandleAAA - Dummy function for testing purposes.
void HandleAAA(CMD_BUFFER_STRUCT* inCmd);

//==================================================
// Operating parameter commands
//==================================================

// Function: HandleDCM - Download configuation memory.
void HandleDCM(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleUCM - Upload configuation memory.
void HandleUCM(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleDMM - Download modem configuation memory.
void HandleDMM(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleUMM - Upload modem configuation memory.
void HandleUMM(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleVTI - View time.
void HandleVTI(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleSTI - Set time.
void HandleSTI(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleVDA - View date.
void HandleVDA(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleSDA - Set date.
void HandleSDA(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleZRO - Zero sensors.
void HandleZRO(CMD_BUFFER_STRUCT* inCmd);

// Function: handleTTO - Toggle test mode on/off.
void handleTTO(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleCAL - Calibrate sensors with cal pulse.
void HandleCAL(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleVOL - View on/off log.
void HandleVOL(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleVCG - View command log.
void HandleVCG(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleVSL - View summary log.
void HandleVSL(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleVEL - View event log.
void HandleVEL(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleVBD - View backlight delay.
void HandleVBD(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleSBD - Set backlight delay.
void HandleSBD(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleVDT - View display timeout.
void HandleVDT(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleSDT - Set display timeout.
void HandleSDT(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleVCL - View contrast level.
void HandleVCL(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleSCL - Set contrast level.
void HandleSCL(CMD_BUFFER_STRUCT* inCmd);

// Init process
void ModemInitProcess(void);

// Reset process, for the US Robotics
void ModemResetProcess(void);

// Function: HandleMRS - Modem reset.
void HandleMRS(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleMVS - Modem view settings.
void HandleMVS(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleMPO - Toggle modem on/off.
void HandleMPO(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleMMO - Toggle modem mode transmit/receive.
void HandleMMO(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleMNO - Toggle modem phone number A/B/C.
void HandleMNO(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleMTO - Toggle modem log on/off.
void HandleMTO(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleMSD - Modem set default initialization string.
void HandleMSD(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleMSR - Modem set receive initialization string.
void HandleMSR(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleMST - Modem set transmit initialization string.
void HandleMST(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleMSA - Modem set phone number A.
void HandleMSA(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleMSB - Modem set phone number B.
void HandleMSB(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleMSC - Modem set phone number C.
void HandleMSC(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleMVI - Modem view last call in detail.
void HandleMVI(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleMVO - Modem view last call out detail.
void HandleMVO(CMD_BUFFER_STRUCT* inCmd);

// Function: HandleVFV - Modem view firmware version.
void HandleVFV(CMD_BUFFER_STRUCT* inCmd);

uint8 ConvertAscii2Binary(uint8 firstByte, uint8 secondByte);

#endif // _REMOTE_OPERATION_H_

