///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#ifndef _TYPEDEFS_H_
#define _TYPEDEFS_H_

#ifndef _PORTABLE_DATA_TYPES_
#define _PORTABLE_DATA_TYPES_
typedef unsigned char		uint8;	/* 8 bits */
typedef unsigned short int	uint16;	/* 16 bits */
typedef unsigned long int	uint32;	/* 32 bits */
typedef signed char			int8;	/* 8 bits */
typedef signed short int	int16;	/* 16 bits */
typedef signed long int		int32;	/* 32 bits */
#endif // _PORTABLE_DATA_TYPES_

typedef unsigned char	BOOLEAN;
typedef unsigned		bitfield;		/* variable # of bits */

// ========================================================================
// This section is devoted to handling debug printing to the craft com port
// ========================================================================

// Project Debug Modes
#define NO_DEBUG			0
#define ALL_DEBUG			1
#define WARNINGS_AND_ERRORS	2
#define ERRORS				3

// Extended Debug output enable
#define EXTENDED_DEBUG		0

// Debug levels
enum debugModes {RAW, NORM, WARN, ERR};

// Define Project Debug Mode
#define GLOBAL_DEBUG_PRINT_ENABLED	ALL_DEBUG

#if 1 /* Necessary with new comiler? Appears so */
extern short DebugPrint(unsigned char mode, char* fmt, ...);
#endif

// Print all debug statements
#if (GLOBAL_DEBUG_PRINT_ENABLED == ALL_DEBUG)
#define debugRaw(...) 	DebugPrint(RAW, __VA_ARGS__)
#define debug(...)		DebugPrint(NORM, __VA_ARGS__)
#define debugWarn(...) 	DebugPrint(WARN, __VA_ARGS__)
#define debugErr(...) 	DebugPrint(ERR, __VA_ARGS__)
#define debugChar(x) 	DebugPrintChar(x)

// Print just warning and error debug statements
#elif (GLOBAL_DEBUG_PRINT_ENABLED == WARNINGS_AND_ERRORS)
#define debugRaw(...) 	;
#define debug(...)		;
#define debugWarn(...) 	DebugPrint(WARN, __VA_ARGS__)
#define debugErr(...) 	DebugPrint(ERR, __VA_ARGS__)
#define debugChar(x) 	;

// Print just error debug statements
#elif (GLOBAL_DEBUG_PRINT_ENABLED == ERRORS)
#define debugRaw(...) 	;
#define debug(...)		;
#define debugWarn(...) 	;
#define debugErr(...) 	DebugPrint(ERR, __VA_ARGS__)
#define debugChar(x) 	;

// Print no debug statements
#elif (GLOBAL_DEBUG_PRINT_ENABLED == NO_DEBUG)
#define debugRaw(...) 	;
#define debug(...)		;
#define debugWarn(...) 	;
#define debugErr(...) 	;
#define debugChar(x) 	;

#endif // End of Global Debug subsection
// ========================================================================

#ifndef NULL
	#define NULL 0x0
#endif

#ifndef bool
	typedef unsigned int bool;
#endif

enum {SLOW = 0, FAST};
enum {INPUT = 0, OUTPUT};
enum {VSS = 0, VDD};
enum {FAILED = 0, PASSED};
enum {OUT_SERIAL = 0, OUT_FILE, OUT_BUFFER};

#if 0 /* old hw */
#define PASS      0
#define FAIL      1
// End of old references
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef NO
#define NO 0
#endif

#ifndef YES
#define YES 1
#endif

#ifndef OFF
#define OFF 0
#endif

#ifndef ON
#define ON 1
#endif

#ifndef CLEAR
#define CLEAR 0
#endif

#ifndef SET
#define SET 1
#endif

#ifndef LOW
#define LOW 0
#endif

#ifndef HIGH
#define HIGH 1
#endif

#ifndef DISABLED
#define DISABLED 0
#endif

#ifndef ENABLED
#define ENABLED 1
#endif

#ifndef FAILED
#define FAILED 0
#endif

#ifndef PASSED
#define PASSED 1
#endif

#define FOREVER while (TRUE)

#endif // _TYPEDEFS_H_
