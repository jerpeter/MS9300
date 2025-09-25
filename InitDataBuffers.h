///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

#ifndef _DATABUFFS_H_
#define _DATABUFFS_H_

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include "Summary.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define MIN_SAMPLE_RATE				1024
#define MAX_SAMPLE_RATE				16384

#define SAMPLE_RATE_1K		1024
#define SAMPLE_RATE_2K		2048
#define SAMPLE_RATE_4K		4096
#define SAMPLE_RATE_8K		8192
#define SAMPLE_RATE_16K		16384
#define SAMPLE_RATE_32K		32768
#define SAMPLE_RATE_64K		65535
#define SAMPLE_RATE_DEFAULT	1024

#if 0 /* Normal */
#define MAX_NUM_OF_CHANNELS			4
#else /* Add storage for Acc data testing */
#define MAX_NUM_OF_CHANNELS			(4 + 3)
#endif
#define CHANNEL_DATA_IN_BYTES		2
#define MAX_CAL_SAMPLES				100
#define START_CAL_SIGNAL			(100 + 1)
#if 0 /* Normal */
#define MAX_DATA_PER_SAMPLE			(MAX_NUM_OF_CHANNELS * CHANNEL_DATA_IN_BYTES) // 8
#else /* Use fixed number of channels due to testing storage for extra Acc data */
#define MAX_DATA_PER_SAMPLE			(4 * CHANNEL_DATA_IN_BYTES) // 4 channels * 2 bytes = 8
#endif
#define MAX_DATA_PER_SECOND			(MAX_SAMPLE_RATE * MAX_DATA_PER_SAMPLE) // 131K

#define LARGEST_PRETRIGGER_SIZE_IN_BYTES	(MAX_DATA_PER_SECOND)
#if VT_FEATURE_DISABLED /* Original */
//#define LARGEST_EVENT_SIZE_IN_BYTES			(MAX_DATA_PER_SECOND * 1) // Determined by max data and ram storage available
#else /* New VT feature */
//#define LARGEST_EVENT_SIZE_IN_BYTES			(MAX_DATA_PER_SECOND * 1) // Determined by max data and ram storage available (remove 1 second max rate for pretrigger increase due to variable trigger feature
#endif
//#define LARGEST_CAL_SIZE_IN_BYTES			(MAX_DATA_PER_SAMPLE * MAX_CAL_SAMPLES) // 800

#define PRE_TRIG_BUFF_SIZE_IN_BYTES 	(LARGEST_PRETRIGGER_SIZE_IN_BYTES + MAX_DATA_PER_SAMPLE) // Max 1 second + 1 sample
#define PRE_TRIG_BUFF_SIZE_IN_WORDS 	(PRE_TRIG_BUFF_SIZE_IN_BYTES / 2)

//#define EVENT_BUFF_SIZE_IN_BYTES	(LARGEST_EVENT_SIZE_IN_BYTES + LARGEST_PRETRIGGER_SIZE_IN_BYTES + LARGEST_CAL_SIZE_IN_BYTES)
//#define EVENT_BUFF_SIZE_IN_WORDS	(((EVENT_BUFF_SIZE_IN_BYTES / sizeof(SAMPLE_DATA_STRUCT)) * sizeof(SAMPLE_DATA_STRUCT)) / 2)
#define EVENT_BUFF_SIZE_IN_BYTES	((65536 * 9) + 8192)
#define EVENT_BUFF_SIZE_IN_WORDS	(EVENT_BUFF_SIZE_IN_BYTES / 2)

#define EVENT_BUFF_SIZE_IN_WORDS_PLUS_EVT_RECORD					(EVENT_BUFF_SIZE_IN_WORDS + (sizeof(EVT_RECORD) / 2))
#if 0 /* Old */
#define SUMMARY_LIST_CACHE_ENTRIES_LIMIT							5000 // 96 bytes each entry
#define EVENT_BUFF_SIZE_IN_WORDS_PLUS_EVT_RECORD_PLUS_SUMMARY_LIST	(EVENT_BUFF_SIZE_IN_WORDS_PLUS_EVT_RECORD + ((SUMMARY_LIST_CACHE_ENTRIES_LIMIT * sizeof(SUMMARY_LIST_ENTRY_STRUCT))/ 2))
#define SUMMARY_LIST_CACHE_OFFSET									(EVENT_BUFF_SIZE_IN_WORDS_PLUS_EVT_RECORD)
#else
#define EVENT_LIST_CACHE_ENTRIES_LAST_INDEX							4095
#define EVENT_LIST_CACHE_ENTRIES_LIMIT								4096 // 8 bytes each entry
//#define EVENT_BUFF_SIZE_IN_WORDS_PLUS_EVT_RECORD_PLUS_EVENT_LIST	(EVENT_BUFF_SIZE_IN_WORDS_PLUS_EVT_RECORD + ((EVENT_LIST_CACHE_ENTRIES_LIMIT * sizeof(EVENT_LIST_ENTRY_STRUCT))/ 2))
//#define EVENT_LIST_CACHE_OFFSET										(EVENT_BUFF_SIZE_IN_WORDS_PLUS_EVT_RECORD)
#endif

#define MAX_EVENT_TIMESTAMP_BUFFERS		((EVENT_BUFF_SIZE_IN_BYTES / (SAMPLE_RATE_1K * MAX_NUM_OF_CHANNELS * CHANNEL_DATA_IN_BYTES)) + 1)

typedef enum
{
#if 0 /* Original (Meant for 12-bit accuracy) */
	FREQ_VALID_PEAK 			= 4,
	FREQ_CROSSOVER_BACKWARD 	= 2,
	FREQ_CROSSOVER_FORWARD 		= 1
#else /* New (Adjusted for 16-bit at a 1/4 the equivalent of the 12-bit) */
	FREQ_VALID_PEAK_16_BIT			= 16,
	FREQ_CROSSOVER_BACKWARD_16_BIT 	= 8,
	FREQ_CROSSOVER_FORWARD_16_BIT	= 4
#endif
} CROSSOVER_LEVELS;

typedef enum
{
	MOV_BEGINNING = 1,
	MOV_ABORT,
	MOV_CHANNEL_ONE,
	MOV_CHANNEL_TWO,
	MOV_CHANNEL_THREE,
	MOV_CHANNEL_FOUR,
	MOV_CHANNEL_FIVE,
	MOV_CHANNEL_SIX,
	MOV_CHANNEL_SEVEN,
	MOV_CHANNEL_EIGHT
} MOV_STATE;

typedef enum
{
	WAVE_INIT,
	WAVE_PRETRIG,
	WAVE_BODY_INIT,
	WAVE_BODY,
	WAVE_CAL_PULSE,
	WAVE_CALCULATE,
	WAVE_STORE,
//	WAVE_STORE_SUMMARY,
//	WAVE_STORE_DATA,
	WAVE_COMPLETE
} WAVE_PROCESSING_STATE;

typedef union
{
	uint16 sampleWord[8];
	uint8 sampleByte[16];
} ISPI_PACKET;

enum
{
	A_CHAN_OFFSET = 0,
	R_CHAN_OFFSET = 1,
	V_CHAN_OFFSET = 2,
	T_CHAN_OFFSET = 3
};

enum
{
	ACCURACY_16_BIT = 16,
	ACCURACY_14_BIT = 14,
	ACCURACY_12_BIT = 12,
	ACCURACY_10_BIT = 10
};

#define AD_BIT_ACCURACY ACCURACY_16_BIT

#define ACCURACY_16_BIT_MIDPOINT 0x8000
#define ACCURACY_14_BIT_MIDPOINT 0x2000
#define ACCURACY_12_BIT_MIDPOINT 0x800
#define ACCURACY_10_BIT_MIDPOINT 0x200

#define WAVEFORM_FILE_WRITE_CHUNK_SIZE	512 // Flash sector size needed to get past 0x2008000 boundary that hangs the SDHC Fat driver (to eMMC Flash), any value above shows problems saving Waveform events while monitoring
#define ATMEL_FILESYSTEM_ACCESS_LIMIT	16384

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------
void InitDataBuffs(uint8 opMode);
uint16 CalcSumFreq(uint16* dataPtr, uint32 sampleRate, uint16* startAddrPtr, uint16* endAddrPtr);
uint16 FixDataToZero(uint16 data_);

void ProcessWaveformData(void);
void MoveWaveformEventToFile(void);

void MoveManualCalToFile(void);

#endif //_DATABUFFS_H_
