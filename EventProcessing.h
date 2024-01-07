///----------------------------------------------------------------------------
///	Nomis Seismograph, Inc.
///	Copyright 2003-2014, All Rights Reserved
///
///	Author: Jeremy Peterson
///----------------------------------------------------------------------------

#ifndef _FLASHEVTS_H_
#define _FLASHEVTS_H_

///----------------------------------------------------------------------------
///	Includes
///----------------------------------------------------------------------------
#include "Summary.h"

///----------------------------------------------------------------------------
///	Defines
///----------------------------------------------------------------------------
#define FLASH_FULL_FLAG				0x0000
#define EVENT_RECORD_START_FLAG		0xA55A
#define EVENT_RECORD_VERSION		0x0101
#define EVENT_MAJOR_VERSION_MASK	0xFF00

#define FIRST_FLASH_EVENT_DATA_SECTOR	0
#define TOTAL_FLASH_EVENT_DATA_SECTORS	(TOTAL_FLASH_DATA_SECTORS - FIRST_FLASH_EVENT_DATA_SECTOR)

// Defines
#define FLASH_EVENT_START	(FLASH_BASE_ADDR + FLASH_BOOT_SIZE_x8 + (FIRST_FLASH_EVENT_DATA_SECTOR * FLASH_SECTOR_SIZE_x8))
#define FLASH_EVENT_END		(FLASH_EVENT_START + (TOTAL_FLASH_EVENT_DATA_SECTORS * FLASH_SECTOR_SIZE_x8))

#define FLASH_EVENT_START_BOUNDRY_UPDATE(flashPtr)		\
	if (flashPtr < (uint16*)FLASH_EVENT_START)			\
		flashPtr = (uint16*)((uint32)FLASH_EVENT_END - 	\
			(uint32)FLASH_EVENT_START - (uint32)flashPtr)

#define FLASH_EVENT_END_BOUNDRY_UPDATE(flashPtr) 							\
	if (flashPtr >= (uint16*)FLASH_EVENT_END)								\
	{	flashPtr = (uint16*)((uint32)flashPtr - (uint32)FLASH_EVENT_END); 	\
		flashPtr = (uint16*)((uint32)FLASH_EVENT_START + (uint32)flashPtr); }

#define RESERVED_FILESYSTEM_SIZE_IN_BYTES	16777216 // 2^24 aka 16 MB, can be evenly divisible by any fat sector size
#define ONE_MEGABYTE_SIZE		1048576
#define SECTOR_SIZE_IN_BYTES	FS_SIZE_OF_SECTOR

enum {
	EVENT_CACHE_FAILURE = 0,
	EVENT_CACHE_SUCCESS,
};

typedef struct
{
	uint32 sizeUsed;
	uint32 sizeFree;
	uint16 waveEventsLeft;
	uint16 barHoursLeft;
	uint16 manualCalsLeft;
	uint8 percentUsed;
	uint8 percentFree;
	uint16 clusterSizeInBytes;
} FLASH_USAGE_STRUCT;

typedef enum {
	CREATE_EVENT_FILE,
	READ_EVENT_FILE,
	APPEND_EVENT_FILE,
	OVERWRITE_EVENT_FILE
} EVENT_FILE_OPTION;

enum {
	ORIGINAL_EVENT_NAME_FORMAT,
	LOOSE_EVENT_NAME_FORMAT
};

enum {
	INIT_LIST,
	NEW_ENTRY
};

typedef struct
{
	int16 file;
	uint16 totalEntries;
	uint16 validEntries;
	uint16 deletedEntries;
	uint16 currentEntryIndex;
	uint32 unused;
	SUMMARY_LIST_ENTRY_STRUCT cachedEntry;
} SUMMARY_LIST_FILE_DETAILS;

///----------------------------------------------------------------------------
///	Prototypes
///----------------------------------------------------------------------------
void InitEventNumberCache(void);
void InitEventRecord(uint8 opMode);
void InitCurrentEventNumber(void);
uint16 GetLastStoredEventNumber(void);
void AddEventNumberToCache(uint16 eventNumber);
void StoreCurrentEventNumber(void);
void CompleteRamEventSummary(void);
void StoreData(uint16* dataPtr, uint16 dataWords);
void InitDriveUsageStats(void);
void GetSDCardUsageStats(void);
void UpdateSDCardUsageStats(uint32 removeSize);
void GetEventFileInfo(uint16 eventNumber, EVENT_HEADER_STRUCT* eventHeaderPtr, EVENT_SUMMARY_STRUCT* eventSummaryPtr, BOOLEAN cacheDataToRamBuffer);
void GetEventFileRecord(uint16 eventNumber, EVT_RECORD* tempEventRecord);
void CacheEventDataToBuffer(uint16 eventNumber, uint8* dataBuffer, uint32 dataOffset, uint32 dataSize);
uint32 GetEventSize(uint16 eventNumber);
uint32 GetERDataSize(uint16 eventNumber);
void CacheERDataToBuffer(uint16 eventNumber, uint8* dataBuffer, uint32 dataOffset, uint32 dataSize);
void CacheEventDataToRam(uint16 eventNumber, uint8* dataBuffer, uint32 dataOffset, uint32 dataSize);
uint8 CacheEventToRam(uint16 eventNumber, EVT_RECORD* eventRecordPtr);
BOOLEAN CheckValidEventFile(uint16 eventNumber);
void DeleteEventFileRecord(uint16 eventNumber);
void DeleteEventFileRecords(void);
void DeleteNonEssentialFiles(void);
void RemoveExcessEventsAboveCap(void);
void AdjustSampleForBitAccuracy(void);
void PowerDownSDCard(void);
uint32 SeismicTriggerConvertBitAccuracy(uint32 seismicTriggerToConvert);
uint16 AirTriggerConvert(uint32 airTriggerToConvert);
uint32 AirTriggerConvertToUnits(uint32 airTriggerToConvert);

void ManageEventsDirectory(void);
void ValidateSummaryListFileWithEventCache(void);
char* GetEventFilenameAndPath(uint16 eventNumber, uint8 eventType);

void DisplayFileNotFound(char* filename);
void DisplayFileCorrupt(char* filename);

uint8 CheckCompressedEventDataFileExists(uint16 eventNumber);

void SetFileDateTimestamp(uint8 option);
int ReadWithSizeFix(int file, void* bufferPtr, uint32 length);
int WriteWithSizeFix(int file, void* bufferPtr, uint32 length);

void CheckStoredEventsCapEventsLimit(void);
void GetEventFilename(uint16 newFileEventNumber);
void GetERDataFilename(uint16 newFileEventNumber);

void CacheResultsEventInfo(EVT_RECORD* eventRecordToCache);

void DumpSummaryListFileToEventBuffer(void);
SUMMARY_LIST_ENTRY_STRUCT* GetSummaryFromSummaryList(uint16 eventNumber);
void ParseAndCountSummaryListEntriesWithRewrite(void);
void AddEventToSummaryList(EVT_RECORD* event);
void InitSummaryListFile(void);

void SaveRemoteEventDownloadStreamToFile(uint16 eventNumber);

uint8 CacheSerialNumberAndReturnIndex(char* serialNumberString);
void CacheSummaryListEntryToEventList(uint8 entryType);
void ClearEventListCache(void);

// Custom function to set the file timestamp
uint32_t SetFileTimestamp(char* filename);

// Endian swap conversions
void EndianSwapDataX16(uint16_t* data, uint32_t dataLength);
void EndianSwapEventRecord(EVT_RECORD* evtRec);

#endif // _FLASHEVTS_H_
