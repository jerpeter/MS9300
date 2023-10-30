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
#include "EventProcessing.h"
#include "Common.h"
#include "Summary.h"
#include "Record.h"
#include "Uart.h"
#include "Menu.h"
#include "TextTypes.h"
//#include "sd_mmc_spi.h"
#include "PowerManagement.h"
#include "Sensor.h"
#include "lcd.h"
//#include "navigation.h"
//#include "fsaccess.h"
//#include "fat.h"

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
static char s_summaryListFileName[] = LOGS_PATH SUMMARY_LIST_FILE;
static uint32 s_addedSizeToSDCard = 0;

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitEventNumberCache(void)
{
	debug("Initializing event number cache...\r\n");

	memset(&g_eventNumberCache[0], NO_EVENT_FILE, EVENT_NUMBER_CACHE_MAX_ENTRIES);

	g_eventNumberCacheValidEntries = 0;
	g_eventNumberCacheOldestIndex = 0;
	g_eventNumberCacheMaxIndex = 1;
	g_eventNumberCacheValidKey = VALID_EVENT_NUMBER_CACHE_KEY;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AddEventNumberToCache(uint16 eventNumber)
{
	if (g_eventNumberCache[eventNumber] == NO_EVENT_FILE)
	{
		g_eventNumberCache[eventNumber] = EVENT_REFERENCE_VALID;
		g_eventNumberCacheValidEntries++;

		if (eventNumber > g_eventNumberCacheMaxIndex) { g_eventNumberCacheMaxIndex = eventNumber; }
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AddEventNumberFromFileToCache(uint16 eventNumber)
{
	if (g_eventNumberCache[eventNumber] == NO_EVENT_FILE)
	{
		g_eventNumberCache[eventNumber] = EVENT_FILE_FOUND;

		if (eventNumber > g_eventNumberCacheMaxIndex) { g_eventNumberCacheMaxIndex = eventNumber; }
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 ValidateEventNumber(uint16 eventNumber)
{
	if ((eventNumber) && (g_eventNumberCache[eventNumber] == EVENT_FILE_FOUND))
	{
		g_eventNumberCache[eventNumber] = EVENT_REFERENCE_VALID;
		g_eventNumberCacheValidEntries++;
		return (YES);
	}

	return (NO);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GetSubDirLowerAndUpperBounds(uint16 eventNumber, uint16* lowerBounds, uint16* upperBounds)
{
	if ((eventNumber / 100) == 0) { *lowerBounds = 1; *upperBounds = 99; }
	else { *lowerBounds = ((eventNumber / 100) * 100); *upperBounds = (*lowerBounds + 99); }
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16 GetEventNumberFromFilename(char* eventFilename, uint8 filter)
{
	char* eventNumberPrefix;
	uint8 charactersToSkip = 3;
	uint32 eventNumber = 0;

	// Check for the "Evt" prefix sub string
	eventNumberPrefix = strstr(eventFilename, "Evt");

	// Check if no sub string was found and loose format selected
	if ((eventNumberPrefix == NULL) && (filter == LOOSE_EVENT_NAME_FORMAT))
	{
		// Set skip to 5 characters
		charactersToSkip = 5;

		// Check for the "Event" sub string
		eventNumberPrefix = strstr(eventFilename, "Event");
	}

	// Check if prefix was found
	if (eventNumberPrefix != NULL)
	{
		// Check if the proceeding character past the prefix is not a number (allowing for a special character separator like #/_/-)
		if ((filter == LOOSE_EVENT_NAME_FORMAT) && ((eventNumberPrefix[charactersToSkip] < 0x30) || (eventNumberPrefix[charactersToSkip] > 0x39)))
		{
			// Scan the number after the prefix and special character
			sscanf((eventNumberPrefix + charactersToSkip + 1), "%lu", &eventNumber);
		}
		else // Event number mates up with prefix
		{
			// Scan the number after the prefix
			sscanf((eventNumberPrefix + charactersToSkip), "%lu", &eventNumber);
		}

		// Check if the event number scanned is not greater than allowed in the system
		if (eventNumber < 65535)
		{
			return ((uint16)eventNumber);
		}
	}

	// Event number not found
	return (0);
}

#if 0
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ushell_cmd_copy( void )
{
	Fs_index sav_index;
	uint8_t u8_status_copy;

	if( g_s_arg[0][0] == 0 )
	return;

	// Save the position
	sav_index = nav_getindex();

	// Select source file
	if( !nav_setcwd( (FS_STRING)g_s_arg[0], TRUE, FALSE ) )
	{
		fputs(MSG_ER_UNKNOWN_FILE, stdout);
		return;
	}
	// Get name of source to be used as same destination name
	nav_file_name( (FS_STRING)g_s_arg[0], MAX_FILE_PATH_LENGTH, FS_NAME_GET, TRUE );
	// Mark this selected file like source file
	if( !nav_file_copy())
	{
		fputs(MSG_KO, stdout);
		goto cp_end;
	}

	// Select destination
	if( g_s_arg[1][0]==0 )
	{
		// g_s_arg[1] is NULL, using mark
		if( !nav_gotoindex(&g_mark_index) )
		goto cp_end;
	}
	else
	{
		// g_s_arg[1] exists, then go to this destination
		if( !nav_setcwd( (FS_STRING)g_s_arg[1], TRUE, FALSE ) )
		{
			fputs(MSG_ER_UNKNOWN_FILE, stdout);
			goto cp_end;
		}
	}

	// Set the name destination and start paste
	if( !nav_file_paste_start((FS_STRING)g_s_arg[0]) )
	{
		fputs(MSG_ER_PASTE, stdout);
		goto cp_end;
	}

	// Performs copy
	do
	{
		u8_status_copy = nav_file_paste_state( FALSE );
	}while( u8_status_copy == COPY_BUSY );

	// Check status of copy action
	if( u8_status_copy == COPY_FAIL )
	{
		fputs(MSG_ER_PASTE, stdout);
		goto cp_end;
	}

	cp_end:
	// Restore the position
	nav_gotoindex(&sav_index);
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0
void SortRamSummaryTable2(void)
{
	uint32 i, j, temp, totalElements;
	uint16 length;
	uint32 totalPasses = 0;
	uint8 activeSwap;

	if (__ramFlashSummaryTbl[s_currFlashSummary].fileEventNum == 0xFFFFFFFF) { totalElements = s_currFlashSummary; } else { totalElements = ???; }

	for(i = 0; (i < totalElements); i++)
	{
		activeSwap = NO;

		for(j = 0; (j < (totalElements - i)); j++)
		{
			totalPasses++;

			if(__ramFlashSummaryTbl[j].fileEventNum > __ramFlashSummaryTbl[(j+1)].fileEventNum)
			{
				temp = __ramFlashSummaryTbl[j].fileEventNum;
				__ramFlashSummaryTbl[j].fileEventNum = __ramFlashSummaryTbl[(j+1)].fileEventNum;
				__ramFlashSummaryTbl[(j+1)].fileEventNum = temp;

				activeSwap = YES;
			}
		}

		if (activeSwap == NO)
		{
			break;
		}
	}

#if 0 /* Test */
	for (i = 0; i < totalElements; i++)
	{
		length = sprintf((char*)g_debugBuffer, "Sorted RST2 entry %04lu event #: %lu\r\n", (i + 1), __ramFlashSummaryTbl[i].fileEventNum);
		ModemPuts(g_debugBuffer, length, NO_CONVERSION);
	}

	length = sprintf((char*)g_debugBuffer, "Sorted RST2 Total passes: %lu\r\n", totalPasses);
	ModemPuts(g_debugBuffer, length, NO_CONVERSION);
#endif
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DeleteEmptyDirectories(uint8 dirType)
{
#if 0 /* old hw */
	uint16 navIndex;
	uint8 dirDeleted = NO;
	uint8 firstDirNotEmpty = NO;

	nav_select(FS_NAV_ID_DEFAULT);
	if (dirType == EVENT_FILE_TYPE) { nav_setcwd(EVENTS_PATH, TRUE, TRUE); }
	else { nav_setcwd(ER_DATA_PATH, TRUE, TRUE); }

	// Check all entries in the current directory
	while (nav_filelist_set(0, FS_FIND_NEXT))
	{
		navIndex = nav_filelist_get();

		// Check if the entry is a directory (sub-directory of the Events directory)
		if (nav_file_isdir())
		{
			nav_file_name((FS_STRING)g_spareBuffer, 80, FS_NAME_GET, FALSE);

			// Delete directory only if empty
			if (nav_file_del(TRUE) == TRUE)
			{
				sprintf((char*)g_debugBuffer, "%s %s (%s %s)", getLangText(REMOVED_TEXT), getLangText(EMPTY_TEXT), getLangText(DIR_TEXT), g_spareBuffer);
				OverlayMessage(getLangText(SUMMARY_LIST_TEXT), (char*)g_debugBuffer, 0);

				// Check if the first entry is a directory that was erased (special case)
				dirDeleted = YES;
				if (navIndex) { navIndex--; }
			}
			else // Directory is not empty
			{
				// Check if the first entry is a directory that is not empty (special case)
				if (navIndex == 0) { firstDirNotEmpty = YES; }
			}

			// Have to reset the current directory after a nav_file_del command
			if (dirType == EVENT_FILE_TYPE) { nav_setcwd(EVENTS_PATH, TRUE, TRUE); }
			else { nav_setcwd(ER_DATA_PATH, TRUE, TRUE); }
		}

		// Check if the nav index is non-zero or a special case
		if ((navIndex) || ((navIndex == 0) && ((dirDeleted == NO) || (firstDirNotEmpty == YES))))
		{
			// Set the current index
			nav_filelist_goto(navIndex);
		}

		// Reset flag
		dirDeleted = NO;
	}
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ManageEventsDirectory(void)
{
	uint16 navIndex = 0;
	uint16 eventNumber;
	uint8 eventsToMigrate = NO;
	uint8 emptyEventDirs = NO;

	debug("Managing Events directory files and caching event numbers...\r\n");

	sprintf((char*)g_debugBuffer, "%s %s", getLangText(EVENT_TEXT), getLangText(SYNC_IN_PROGRESS_TEXT));
	OverlayMessage(getLangText(SUMMARY_LIST_TEXT), (char*)g_debugBuffer, (1 * SOFT_SECS));

	if (g_fileAccessLock != AVAILABLE)
	{
		ReportFileSystemAccessProblem("Cache event numbers");
	}
	else // (g_fileAccessLock == AVAILABLE)
	{
		InitEventNumberCache();

		GetSpi1MutexLock(SDMMC_LOCK);

#if 0 /* old hw */
		nav_select(FS_NAV_ID_DEFAULT);
		nav_setcwd(EVENTS_PATH, TRUE, TRUE);

		// Check all entries in the Events directory
		while (nav_filelist_set(0, FS_FIND_NEXT))
		{
			// Check if the entry is a directory (sub-directory of the Events directory)
			if (nav_file_isdir())
			{
				nav_file_name((FS_STRING)g_spareBuffer, 80, FS_NAME_GET, FALSE);

				sprintf((char*)g_debugBuffer, "%s %s (%s %s)", getLangText(EVENT_TEXT), getLangText(SYNC_IN_PROGRESS_TEXT), getLangText(DIR_TEXT), g_spareBuffer);
				OverlayMessage(getLangText(SUMMARY_LIST_TEXT), (char*)g_debugBuffer, 0);

				sprintf((char*)g_spareFileName, "%s%s\\", EVENTS_PATH, (char*)g_spareBuffer);
				nav_setcwd((char*)g_spareFileName, TRUE, TRUE);

				if ((emptyEventDirs == NO) && (nav_filelist_set(0, FS_FIND_NEXT) == FALSE))
				{
					emptyEventDirs = YES;
				}

				// Reset the file position to unselected to prevent the empty check from starting at position 1 which causes the following logic to skip the 1st entry
				nav_filelist_reset();

				while (nav_filelist_set(0, FS_FIND_NEXT))
				{
					if (!nav_file_isdir())
					{
						// Check if the file has the correct event file extension
						if (nav_file_checkext("ns8"))
						{
							// Get the event number from the filename
							nav_file_name((FS_STRING)g_spareBuffer, 255, FS_NAME_GET, FALSE);
							eventNumber = GetEventNumberFromFilename((char*)g_spareBuffer, LOOSE_EVENT_NAME_FORMAT);

							// Check if the event number is valid
							if (eventNumber)
							{
								// Add event number to the cache
								AddEventNumberFromFileToCache(eventNumber);
							}

						}
						else
						{
							// Get the event number from the filename
							nav_file_name((FS_STRING)g_spareBuffer, 255, FS_NAME_GET, FALSE);
							eventNumber = GetEventNumberFromFilename((char*)g_spareBuffer, LOOSE_EVENT_NAME_FORMAT);
						}
					}
				}

				nav_setcwd(EVENTS_PATH, TRUE, TRUE);
				nav_filelist_set(navIndex, FS_FIND_NEXT);
			}
			else // (!nav_file_isdir()) // Entry is a file (in the root of the Events dir)
			{
				// Check if the file has the correct event file extension
				if (nav_file_checkext("ns8"))
				{

					// Get the event number from the filename
					nav_file_name((FS_STRING)g_spareBuffer, 255, FS_NAME_GET, FALSE);
					eventNumber = GetEventNumberFromFilename((char*)g_spareBuffer, LOOSE_EVENT_NAME_FORMAT);

					// Check if the event number is valid
					if (eventNumber)
					{
						// Add event number to the cache
						AddEventNumberFromFileToCache(eventNumber);

						// Signal event files need migration to the correct sub-directory
						eventsToMigrate = YES;
					}
				}
				else
				{
					// Get the event number from the filename
					nav_file_name((FS_STRING)g_spareBuffer, 255, FS_NAME_GET, FALSE);
					eventNumber = GetEventNumberFromFilename((char*)g_spareBuffer, LOOSE_EVENT_NAME_FORMAT);
				}
			}

			navIndex++;
		}

		if (emptyEventDirs == YES)
		{
			DeleteEmptyDirectories(EVENT_FILE_TYPE);
			DeleteEmptyDirectories(ER_DATA_FILE_TYPE);
		}
#endif
		ReleaseSpi1MutexLock();

		if (eventsToMigrate)
		{
			// Found loose events, set timer to handle the event file migration process from secondary to primary location (wait at least the LCD timeout to start)
			AssignSoftTimer(LOOSE_EVENT_MIGRATION_TIMER_NUM, (uint32)(g_unitConfig.lcdTimeout * TICKS_PER_MIN), LooseEventMigrationTimerCallBack);
		}
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* Poor performance 564 events (of 1100 objects) took ~3.5 seconds */
void CountExistingEvents(void)
{
	uint16 navIndex = 0;
	uint16 eventsFound = 0;
	uint32 msCountTime;

	if (g_fileAccessLock != AVAILABLE)
	{
		ReportFileSystemAccessProblem("Cache event numbers");
	}
	else // (g_fileAccessLock == AVAILABLE)
	{

		// Start the key timer
		Start_Data_Clock(TC_MILLISECOND_TIMER_CHANNEL);

		msCountTime = g_msTimerTicks;

		GetSpi1MutexLock(SDMMC_LOCK);
		nav_select(FS_NAV_ID_DEFAULT);
		nav_setcwd(EVENTS_PATH, TRUE, TRUE);

		// Check all entries in the Events directory
		while (nav_filelist_set(0 , FS_FIND_NEXT))
		{
			// Check if the entry is a directory (sub-directory of the Events directory)
			if (nav_file_isdir())
			{
				nav_file_name((FS_STRING)g_spareBuffer, 80, FS_NAME_GET, FALSE);

				sprintf((char*)g_spareFileName, "%s%s\\", EVENTS_PATH, (char*)g_spareBuffer);
				nav_setcwd((char*)g_spareFileName, TRUE, TRUE);

				while (nav_filelist_set(0 , FS_FIND_NEXT))
				{
					// Check if sub directory element is a file
					if (!nav_file_isdir())
					{
						// Check if the file has the correct event file extension
						if (nav_file_checkext("ns8"))
						{
							eventsFound++;
						}
					}
				}

				nav_setcwd(EVENTS_PATH, TRUE, TRUE);
				nav_filelist_set(navIndex, FS_FIND_NEXT);
			}
			else // (!nav_file_isdir()) // Entry is a file (in the root of the Events dir)
			{
				// Check if the file has the correct event file extension
				if (nav_file_checkext("ns8"))
				{
					eventsFound++;
				}
			}

			// Keep track of file list index for returning from a sub directory search
			navIndex++;
		}

		ReleaseSpi1MutexLock();

		msCountTime = (g_msTimerTicks - msCountTime);

		// Disable the key timer
		Stop_Data_Clock(TC_MILLISECOND_TIMER_CHANNEL);
	}

	uint16 length;
	length = sprintf((char*)g_spareBuffer, "CountExistingEvents Total: %d, Time: %lu ms", eventsFound, msCountTime);
	ModemPuts(g_spareBuffer, length, NO_CONVERSION);
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
static uint16 s_lastEventNumber;
static uint8 s_lastEventType;
static uint8 s_lastSearchValid = NO;
char* GetEventFilenameAndPath(uint16 eventNumber, uint8 eventType)
{
	uint16 fileEventNumber;
	uint16 lowerBounds, upperBounds;
	char directory[20];
	char fileExtension[4];
#if 0 /* Test */
	uint16 length;
#endif

	// Check if the last search matches the cached results
	if ((s_lastSearchValid) && (s_lastEventNumber == eventNumber) && (s_lastEventType == eventType))
	{
#if 0 /* Test */
		length = sprintf((char*)g_debugBuffer, "Returning Event file cached result\r\n");
		ModemPuts(g_debugBuffer, length, NO_CONVERSION);
#endif
		return (&g_eventPathAndFilename[0]);
	}

	// Cache the new event parameters
	s_lastSearchValid = NO;
	s_lastEventNumber = eventNumber;
	s_lastEventType = eventType;

#if 0 /* old hw */
	nav_select(FS_NAV_ID_DEFAULT);

	GetSubDirLowerAndUpperBounds(eventNumber, &lowerBounds, &upperBounds);

	if (eventType == EVENT_FILE_TYPE)
	{
		strcpy(directory, EVENTS_PATH);
		strcpy(fileExtension, "ns8");
	}
	else
	{
		strcpy(directory, ER_DATA_PATH);
		strcpy(fileExtension, "nsD");
	}

	// Create Primary location path and filename
	sprintf((char*)g_eventPathAndFilename, "%s%s %d-%d\\%s%d.%s", directory, EVTS_SUB_DIR, lowerBounds, upperBounds, EVT_FILE, eventNumber, fileExtension);

	// Check if Primary location is valid
	if (nav_setcwd((char*)g_eventPathAndFilename, TRUE, FALSE))
	{
#if 0 /* Test */
		length = sprintf((char*)g_debugBuffer, "Get Path and File: Primary location found for Event(%d): %s\r\n", eventNumber, g_eventPathAndFilename);
		ModemPuts(g_debugBuffer, length, NO_CONVERSION);
#endif
		// Found file
		s_lastSearchValid = YES;
		return (&g_eventPathAndFilename[0]);
	}

	// Create Primary location path and filename
	sprintf((char*)g_eventPathAndFilename, "%s%s%d.%s", directory, EVT_FILE, eventNumber, fileExtension);

	// Check if Secondary location is valid
	if (nav_setcwd((char*)g_eventPathAndFilename, TRUE, FALSE))
	{
#if 0 /* Test */
		length = sprintf((char*)g_debugBuffer, "Get Path and File: Secondary location found for Event(%d): %s\r\n", eventNumber, g_eventPathAndFilename);
		ModemPuts(g_debugBuffer, length, NO_CONVERSION);
#endif
		// Found file
		s_lastSearchValid = YES;
		return (&g_eventPathAndFilename[0]);
	}

	// Create Primary location directory
	sprintf((char*)g_eventPathAndFilename, "%s%s %d-%d\\\r\n", directory, EVTS_SUB_DIR, lowerBounds, upperBounds);

	// Change to Primary location directory (if it exists)
	if (nav_setcwd((char*)g_eventPathAndFilename, TRUE, TRUE))
	{
		// Search through all the directory elements
		while (nav_filelist_set(0, FS_FIND_NEXT))
		{
			// Check if the current element is a file
			if (!nav_file_isdir())
			{
				// Check if the file has the correct event file extension
				if (nav_file_checkext(fileExtension))
				{
					// Get the event number from the filename
					nav_file_name((FS_STRING)g_spareFileName, 255, FS_NAME_GET, FALSE);
					fileEventNumber = GetEventNumberFromFilename((char*)g_spareFileName, LOOSE_EVENT_NAME_FORMAT);

					// Check if the event number is valid
					if (fileEventNumber == eventNumber)
					{
						// Found file
						strcat(g_eventPathAndFilename, (char*)g_spareFileName);
#if 0 /* Test */
						length = sprintf((char*)g_debugBuffer, "Get Path and File: Primary directory found for Event(%d): %s\r\n", eventNumber, g_eventPathAndFilename);
						ModemPuts(g_debugBuffer, length, NO_CONVERSION);
#endif
						s_lastSearchValid = YES;
						return (&g_eventPathAndFilename[0]);
					}
				}
			}
		}
	}

	// Create Secondary location directory
	sprintf((char*)g_eventPathAndFilename, directory);

	// Change to Secondary location directory (if it exists)
	if (nav_setcwd((char*)g_eventPathAndFilename, TRUE, TRUE))
	{
		// Search through all the directory elements
		while (nav_filelist_set(0, FS_FIND_NEXT))
		{
			// Check if the current element is a file
			if (!nav_file_isdir())
			{
				// Check if the file has the correct event file extension
				if (nav_file_checkext(fileExtension))
				{
					// Get the event number from the filename
					nav_file_name((FS_STRING)g_spareFileName, 255, FS_NAME_GET, FALSE);
					fileEventNumber = GetEventNumberFromFilename((char*)g_spareFileName, LOOSE_EVENT_NAME_FORMAT);

					// Check if the event number is valid
					if (fileEventNumber == eventNumber)
					{
						// Found file
						strcat(g_eventPathAndFilename, (char*)g_spareFileName);
#if 0 /* Test */
						length = sprintf((char*)g_debugBuffer, "Get Path and File: Secondary directory found for Event(%d): %s\r\n", eventNumber, g_eventPathAndFilename);
						ModemPuts(g_debugBuffer, length, NO_CONVERSION);
#endif
						s_lastSearchValid = YES;
						return (&g_eventPathAndFilename[0]);
					}
				}
			}
		}
	}
#endif
	// If execution reaches here then the event file was not found
	g_eventPathAndFilename[0] = '\0';
	return (&g_eventPathAndFilename[0]);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DumpSummaryListFileToEventBuffer(void)
{
	SUMMARY_LIST_ENTRY_STRUCT* summaryListCache = (SUMMARY_LIST_ENTRY_STRUCT*)&g_eventDataBuffer[0];

	if (g_summaryList.totalEntries)
	{
		GetSpi1MutexLock(SDMMC_LOCK);

#if 0 /* old hw */
		nav_select(FS_NAV_ID_DEFAULT);

		if (nav_setcwd(s_summaryListFileName, TRUE, FALSE))
		{
			g_summaryList.file = open(s_summaryListFileName, O_RDONLY);

			debug("Dumping Summary list with file size: %d\r\n", nav_file_lgt());

			ReadWithSizeFix(g_summaryList.file, summaryListCache, nav_file_lgt());
			g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
			close(g_summaryList.file);
		}
		else
		{
			debugErr("File access problem: Dump Summary List\r\n");
		}
#endif
		ReleaseSpi1MutexLock();
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void AddEventToSummaryList(EVT_RECORD* event)
{
	memset(&g_summaryList.cachedEntry, 0, sizeof(g_summaryList.cachedEntry));

	g_summaryList.cachedEntry.eventNumber = event->summary.eventNumber;
	g_summaryList.cachedEntry.mode = event->summary.mode;
	g_summaryList.cachedEntry.subMode = event->summary.subMode;
	g_summaryList.cachedEntry.channelSummary.a = event->summary.calculated.a;
	g_summaryList.cachedEntry.channelSummary.r = event->summary.calculated.r;
	g_summaryList.cachedEntry.channelSummary.v = event->summary.calculated.v;
	g_summaryList.cachedEntry.channelSummary.t = event->summary.calculated.t;
	g_summaryList.cachedEntry.eventTime = event->summary.captured.eventTime;
	memcpy(g_summaryList.cachedEntry.serialNumber, event->summary.version.serialNumber, SERIAL_NUMBER_STRING_SIZE);
	g_summaryList.cachedEntry.seismicSensorType = event->summary.parameters.seismicSensorType;
	g_summaryList.cachedEntry.sampleRate = event->summary.parameters.sampleRate;
	g_summaryList.cachedEntry.unitsOfMeasure = event->summary.parameters.seismicUnitsOfMeasure;
	g_summaryList.cachedEntry.unitsOfAir = event->summary.parameters.airUnitsOfMeasure;
	g_summaryList.cachedEntry.gainSelect = event->summary.parameters.channel[0].options;
	g_summaryList.cachedEntry.bitAccuracy = event->summary.parameters.bitAccuracy;
	g_summaryList.cachedEntry.vectorSumPeak = event->summary.calculated.vectorSumPeak;

#if 1 /* New - Override A channel acceleration */
	g_summaryList.cachedEntry.channelSummary.a.acceleration = event->summary.parameters.airSensorType;
#endif

	// Parent handles GetSpi1MutexLock(SDMMC_LOCK)

#if 0 /* old hw */
	nav_select(FS_NAV_ID_DEFAULT);

	if (nav_setcwd(s_summaryListFileName, TRUE, TRUE))
	{
		g_summaryList.file = open(s_summaryListFileName, O_APPEND);
		//file_seek(0, FS_SEEK_END);
		write(g_summaryList.file, &g_summaryList.cachedEntry, sizeof(SUMMARY_LIST_ENTRY_STRUCT));
		g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
		close(g_summaryList.file);
	}
	else
	{
		debugErr("File access problem: Add Event to Summary list\r\n");
	}
#endif
	// Parent handles ReleaseSpi1MutexLock()

	debug("Added Event: %d to Summary List file\r\n", g_summaryList.cachedEntry.eventNumber);

	g_summaryList.totalEntries++;
	g_summaryList.validEntries++;

	CacheSummaryListEntryToEventList(NEW_ENTRY);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0
void CacheNextSummaryListEntry(void)
{
	SUMMARY_LIST_ENTRY_STRUCT* summaryListCache = (SUMMARY_LIST_ENTRY_STRUCT*)&g_eventDataBuffer[???];

	// Check if able to use the summary list cache generated by the dump summary file to cache call
	if ((g_summaryList.totalEntries < SUMMARY_LIST_CACHE_ENTRIES_LIMIT) &&
		((g_summaryListMenuActive == YES) || ((g_modemStatus.xferState == DQMx_CMD) && (g_sampleProcessing == IDLE_STATE))))
	{
		// Clear the cached entry
		memset(&g_summaryList.cachedEntry, 0, sizeof(g_summaryList.cachedEntry));

		while (++g_summaryList.currentEntryIndex < g_summaryList.totalEntries)
		{
			memcpy(&g_summaryList.cachedEntry, &summaryListCache[g_summaryList.currentEntryIndex], sizeof(SUMMARY_LIST_ENTRY_STRUCT));

			if (g_summaryList.cachedEntry.eventNumber)
			{
				break;
			}
		}

		if (g_summaryList.currentEntryIndex == g_summaryList.totalEntries)
		{
			debug("End of Summary list cache\r\n");
		}
	}
	else
	{
		// Clear the cached entry
		memset(&g_summaryList.cachedEntry, 0, sizeof(g_summaryList.cachedEntry));

		GetSpi1MutexLock(SDMMC_LOCK);

		nav_select(FS_NAV_ID_DEFAULT);

		if (nav_setcwd(s_summaryListFileName, TRUE, FALSE))
		{
			g_summaryList.file = open(s_summaryListFileName, O_RDONLY);

			while (file_seek(++g_summaryList.currentEntryIndex * sizeof(SUMMARY_LIST_ENTRY_STRUCT), FS_SEEK_SET) == TRUE)
			{
				ReadWithSizeFix(g_summaryList.file, &g_summaryList.cachedEntry, sizeof(SUMMARY_LIST_ENTRY_STRUCT));

				if (g_summaryList.cachedEntry.eventNumber)
				{
					break;
				}

				if (g_summaryList.currentEntryIndex == g_summaryList.totalEntries)
				{
					debug("End of Summary list entries\r\n");
					break;
				}
			}

			g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
			close(g_summaryList.file);
		}
		else
		{
			debugErr("File access problem: Cache Next Summary List\r\n");
		}

		ReleaseSpi1MutexLock();
	}
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0
void CachePreviousSummaryListEntry(void)
{
	SUMMARY_LIST_ENTRY_STRUCT* summaryListCache = (SUMMARY_LIST_ENTRY_STRUCT*)&g_eventDataBuffer[???];

	// Check if able to use the summary list cache generated by the dump summary file to cache call
	if ((g_summaryList.totalEntries < SUMMARY_LIST_CACHE_ENTRIES_LIMIT) &&
		((g_summaryListMenuActive == YES) || ((g_modemStatus.xferState == DQMx_CMD) && (g_sampleProcessing == IDLE_STATE))))
	{
		// Clear the cached entry
		memset(&g_summaryList.cachedEntry, 0, sizeof(g_summaryList.cachedEntry));

		if (g_summaryList.currentEntryIndex)
		{
			while (--g_summaryList.currentEntryIndex)
			{
				memcpy(&g_summaryList.cachedEntry, &summaryListCache[g_summaryList.currentEntryIndex], sizeof(SUMMARY_LIST_ENTRY_STRUCT));

				if (g_summaryList.cachedEntry.eventNumber)
				{
					break;
				}
			}
		}
		else
		{
			memcpy(&g_summaryList.cachedEntry, &summaryListCache[0], sizeof(SUMMARY_LIST_ENTRY_STRUCT));
		}

		if (g_summaryList.currentEntryIndex == 0)
		{
			debug("Start of Summary list cache\r\n");
		}
	}
	else
	{
		// Clear the cached entry
		memset(&g_summaryList.cachedEntry, 0, sizeof(g_summaryList.cachedEntry));

		GetSpi1MutexLock(SDMMC_LOCK);

		nav_select(FS_NAV_ID_DEFAULT);

		if (nav_setcwd(s_summaryListFileName, TRUE, FALSE))
		{
			g_summaryList.file = open(s_summaryListFileName, O_RDONLY);

			if (g_summaryList.currentEntryIndex)
			{
				while (file_seek(--g_summaryList.currentEntryIndex * sizeof(SUMMARY_LIST_ENTRY_STRUCT), FS_SEEK_SET) == TRUE)
				{
					ReadWithSizeFix(g_summaryList.file, &g_summaryList.cachedEntry, sizeof(SUMMARY_LIST_ENTRY_STRUCT));

					if (g_summaryList.cachedEntry.eventNumber)
					{
						break;
					}

					if (g_summaryList.currentEntryIndex == 0)
					{
						debug("Start of Summary list entries\r\n");
						break;
					}
				}
			}
			else
			{
				file_seek(0, FS_SEEK_SET);
				ReadWithSizeFix(g_summaryList.file, &g_summaryList.cachedEntry, sizeof(SUMMARY_LIST_ENTRY_STRUCT));
				debug("Start of Summary list entries\r\n");
			}

			g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
			close(g_summaryList.file);
		}
		else
		{
			debugErr("File access problem: Cache Previous Summary List\r\n");
		}

		ReleaseSpi1MutexLock();
	}
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0
void CacheSummaryEntryByIndex(uint16 index)
{
	SUMMARY_LIST_ENTRY_STRUCT* summaryListCache = (SUMMARY_LIST_ENTRY_STRUCT*)&g_eventDataBuffer[???];

	// Check if able to use the summary list cache generated by the dump summary file to cache call
	if ((g_summaryList.totalEntries < SUMMARY_LIST_CACHE_ENTRIES_LIMIT) &&
		((g_summaryListMenuActive == YES) || ((g_modemStatus.xferState == DQMx_CMD) && (g_sampleProcessing == IDLE_STATE))))
	{
		// Clear the cached entry
		memset(&g_summaryList.cachedEntry, 0, sizeof(g_summaryList.cachedEntry));

		while (index < g_summaryList.totalEntries)
		{
			memcpy(&g_summaryList.cachedEntry, &summaryListCache[index], sizeof(SUMMARY_LIST_ENTRY_STRUCT));

			// Check if a valid entry was found
			if (g_summaryList.cachedEntry.eventNumber)
			{
				g_summaryList.currentEntryIndex = index;
				break;
			}
			else // Deleted entry, move to next entry
			{
				//debug("Skipping deleted entry\r\n");
				index++;
			}
		}

		// Check if no entry was found
		if (index == g_summaryList.totalEntries)
		{
			debugErr("Cache Summary Entry reached end\r\n");
		}
	}
	else // Resort to file access
	{
		// Clear the cached entry
		memset(&g_summaryList.cachedEntry, 0, sizeof(g_summaryList.cachedEntry));

		GetSpi1MutexLock(SDMMC_LOCK);

		nav_select(FS_NAV_ID_DEFAULT);

		if (nav_setcwd(s_summaryListFileName, TRUE, FALSE))
		{
			g_summaryList.file = open(s_summaryListFileName, O_RDONLY);

			if (index < g_summaryList.totalEntries)
			{
				while (file_seek((index * sizeof(SUMMARY_LIST_ENTRY_STRUCT)), FS_SEEK_SET) == TRUE)
				{
					ReadWithSizeFix(g_summaryList.file, &g_summaryList.cachedEntry, sizeof(SUMMARY_LIST_ENTRY_STRUCT));

					// Check if no entry was found
					if (g_summaryList.cachedEntry.eventNumber == 0)
					{
						//debug("Skipping deleted entry\r\n");
						index++;
					}
					else
					{
						g_summaryList.currentEntryIndex = index;
						break;
					}
				}
			}

			// Check if no entry was found
			if (g_summaryList.cachedEntry.eventNumber == 0)
			{
				debugErr("Cache Summary Entry failed using index: %d\r\n", index);
			}
			else
			{
				//debug("Caching Summary File event: %d\r\n", g_summaryList.cachedEntry.eventNumber);
			}

			g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
			close(g_summaryList.file);
		}
		else
		{
			debugErr("File access problem: Cache Summary Entry by Index\r\n");
		}

		ReleaseSpi1MutexLock();
	}
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
SUMMARY_LIST_ENTRY_STRUCT* GetSummaryFromSummaryList(uint16 eventNumber)
{
	uint32 summaryListIndex = 0;
	uint16 summaryListIndexEventNumber;

	// Check if the current cached entry is not already loaded with the current event request
	if (eventNumber != g_summaryList.cachedEntry.eventNumber)
	{
		// Clear the cached entry
		memset(&g_summaryList.cachedEntry, 0, sizeof(g_summaryList.cachedEntry));

		GetSpi1MutexLock(SDMMC_LOCK);

#if 0 /* old hw */
		nav_select(FS_NAV_ID_DEFAULT);

		if (nav_setcwd(s_summaryListFileName, TRUE, FALSE))
		{
			g_summaryList.file = open(s_summaryListFileName, O_RDONLY);

			while (file_seek(summaryListIndex * sizeof(SUMMARY_LIST_ENTRY_STRUCT), FS_SEEK_SET) == TRUE)
			{
				ReadWithSizeFix(g_summaryList.file, &summaryListIndexEventNumber, 2);

				if (summaryListIndexEventNumber == eventNumber)
				{
					file_seek(summaryListIndex * sizeof(SUMMARY_LIST_ENTRY_STRUCT), FS_SEEK_SET);
					ReadWithSizeFix(g_summaryList.file, &g_summaryList.cachedEntry, sizeof(SUMMARY_LIST_ENTRY_STRUCT));

					break;
				}
				else
				{
					summaryListIndex++;
				}
			}

			// Check if no entry was found
			if (g_summaryList.cachedEntry.eventNumber == 0)
			{
				debugErr("No Summary List entry found for Event Number: %d\r\n", eventNumber);
			}

			g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
			close(g_summaryList.file);
		}
		else
		{
			debugErr("File access problem: Get Summary from Summary List\r\n");
		}
#endif
		ReleaseSpi1MutexLock();
	}

	return (&g_summaryList.cachedEntry);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 CacheSerialNumberAndReturnIndex(char* serialNumberString)
{
	uint8 index = 0;

	// Check if the serial number does not match the current unit serial number
	if (strncmp(serialNumberString, (char*)&g_serialNumberCache[index], 15) != 0)
	{
		while (++index != 255)
		{
			// Check if the next cache entry is valid
			if (strlen((char*)&g_serialNumberCache[index]))
			{
				// Check if there is a match to the cache entry
				if (strncmp(serialNumberString, (char*)&g_serialNumberCache[index], 15) == 0)
				{
					break;
				}
			}
			else // Have not found a match but found an empty entry
			{
				// Store the entry into the cache
				strncpy((char*)&g_serialNumberCache[index], serialNumberString, 16);
				break;
			}
		}
	}

	return (index);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void CacheSummaryListEntryToEventList(uint8 entryType)
{
	EVENT_LIST_ENTRY_STRUCT* eventListCache = (EVENT_LIST_ENTRY_STRUCT*)&g_eventDataBuffer[EVENT_LIST_CACHE_OFFSET];

	eventListCache[g_summaryList.cachedEntry.eventNumber].mode = g_summaryList.cachedEntry.mode;
	eventListCache[g_summaryList.cachedEntry.eventNumber].subMode = g_summaryList.cachedEntry.subMode;
	eventListCache[g_summaryList.cachedEntry.eventNumber].serialNumberCacheIndex = CacheSerialNumberAndReturnIndex((char*)&g_summaryList.cachedEntry.serialNumber[0]);
	eventListCache[g_summaryList.cachedEntry.eventNumber].epochEventTime = ConvertDateTimeToEpochTime(g_summaryList.cachedEntry.eventTime);

	if (entryType == INIT_LIST)
	{
		// Check if oldest epoch reference is valid (non-zero, stored in index 0 which is not a valid event number)
		if (eventListCache[0].epochEventTime)
		{
			// Check if an older event time was found
			if (eventListCache[g_summaryList.cachedEntry.eventNumber].epochEventTime < eventListCache[0].epochEventTime)
			{
				// Save the older time and index of the oldest event
				eventListCache[0].epochEventTime = eventListCache[g_summaryList.cachedEntry.eventNumber].epochEventTime;
				g_eventNumberCacheOldestIndex = g_summaryList.cachedEntry.eventNumber;
			}
		}
		else // Initial condition
		{
			eventListCache[0].epochEventTime = eventListCache[g_summaryList.cachedEntry.eventNumber].epochEventTime;
			g_eventNumberCacheOldestIndex = g_summaryList.cachedEntry.eventNumber;
		}
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ClearEventListCache(void)
{
	// Clear out the event list cache
	memset(&g_eventDataBuffer[EVENT_LIST_CACHE_OFFSET], 0, (EVENT_LIST_CACHE_ENTRIES_LIMIT * sizeof(EVENT_LIST_ENTRY_STRUCT)));

	// Clear out the serial number cache
	memset(&g_serialNumberCache[0], 0, (SERIAL_NUMBER_CACHE_ENTRIES * SERIAL_NUMBER_STRING_SIZE));

	// Copy the current serial number to the first entry
	memcpy(&g_serialNumberCache[0], g_factorySetupRecord.unitSerialNumber, FACTORY_SERIAL_NUMBER_SIZE);

	// Label the last entry as error
	strcpy((char*)&g_serialNumberCache[255], getLangText(ERROR_TEXT));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ParseAndCountSummaryListEntriesWithRewrite(void)
{
	uint32 startRewriteFileLocation = 0;
	SUMMARY_LIST_ENTRY_STRUCT* rewriteSummaryListEntryCachePtr = NULL;
	uint32 displacedEntries = 0;
	uint16 totalEntries = 0; //(nav_file_lgt() / sizeof(SUMMARY_LIST_ENTRY_STRUCT));
	uint32 halfSecondCompare = 0;

	ClearEventListCache();

	sprintf((char*)g_spareBuffer, "%s %s (%d of %d)", getLangText(EVENT_TEXT), getLangText(SYNC_IN_PROGRESS_TEXT), 1, totalEntries);
	OverlayMessage(getLangText(SUMMARY_LIST_TEXT), (char*)g_spareBuffer, 0);
	halfSecondCompare = g_lifetimeHalfSecondTickCount + 1;

#if 0 /* old hw */
	file_seek(0, FS_SEEK_SET);
#endif
	while (ReadWithSizeFix(g_summaryList.file, (uint8*)&g_summaryList.cachedEntry, sizeof(SUMMARY_LIST_ENTRY_STRUCT)) != -1)
	{
		// Check if a second has passed
		if (g_lifetimeHalfSecondTickCount > halfSecondCompare)
		{
			// Update progress to LCD
			sprintf((char*)g_spareBuffer, "%s %s (%d of %d)", getLangText(EVENT_TEXT), getLangText(SYNC_IN_PROGRESS_TEXT), (g_summaryList.validEntries + g_summaryList.deletedEntries), totalEntries);
			OverlayMessage(getLangText(SUMMARY_LIST_TEXT), (char*)g_spareBuffer, 0);
			halfSecondCompare = g_lifetimeHalfSecondTickCount + 1;
		}

		// Validate event file is present to make sure it hasn't been deleted under the covers
		if (ValidateEventNumber(g_summaryList.cachedEntry.eventNumber))
		{
			g_summaryList.validEntries++;

			CacheSummaryListEntryToEventList(INIT_LIST);

			// Check if the write pointer was initialized meaning a deleted entry was found
			if (rewriteSummaryListEntryCachePtr)
			{
				memcpy(rewriteSummaryListEntryCachePtr++, &g_summaryList.cachedEntry, sizeof(SUMMARY_LIST_ENTRY_STRUCT));
				displacedEntries++;
			}
			else
			{
				// Update the position of the last known good string of valid event summaries
#if 0 /* old hw */
				startRewriteFileLocation = file_getpos();
#endif
			}
		}
		else // Event file is missing or erased, need to clear summary list entry
		{
			if (rewriteSummaryListEntryCachePtr == NULL) { rewriteSummaryListEntryCachePtr = (SUMMARY_LIST_ENTRY_STRUCT*)&g_eventDataBuffer[0]; }

			// Entry removed this run
			g_summaryList.deletedEntries++;
		}
	}

	// Check if any entries were deleted
	if (g_summaryList.deletedEntries)
	{
		// Reset to the marked rewrite location of the summary list file
#if 0 /* old hw */
		file_seek(startRewriteFileLocation, FS_SEEK_SET);
#endif
		// Re-write the summary list minus the deleted entries
		WriteWithSizeFix(g_summaryList.file, &g_eventDataBuffer[0], (displacedEntries * sizeof(SUMMARY_LIST_ENTRY_STRUCT)));

		// Set the end of the file after the write to clip the blank space now at the end
#if 0 /* old hw */
		file_set_eof();
#endif
		// Reset the deleted entry count
		g_summaryList.deletedEntries = 0;
	}

	// Set the summary list total entries
	g_summaryList.totalEntries = g_summaryList.validEntries;

	if (g_summaryList.totalEntries != (g_summaryList.validEntries + g_summaryList.deletedEntries))
	{
		debugErr("Summary List Parse and Count showing incorrect amount of total entries\r\n");
	}
	else
	{
		debug("Summary List file: Valid Entires: %d, Deleted Entries: %d (Total: %d)\r\n", g_summaryList.validEntries, g_summaryList.deletedEntries, g_summaryList.totalEntries);
	}

	//nav_select(FS_NAV_ID_DEFAULT);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitSummaryListFile(void)
{
	memset(&g_summaryList, 0, sizeof(g_summaryList));

	GetSpi1MutexLock(SDMMC_LOCK);

#if 0 /* old hw */
	nav_select(FS_NAV_ID_DEFAULT);

	if (nav_setcwd(s_summaryListFileName, TRUE, TRUE))
	{
		g_summaryList.file = open(s_summaryListFileName, O_RDWR);

		if (nav_file_lgt() % sizeof(SUMMARY_LIST_ENTRY_STRUCT) != 0)
		{
			debugErr("Summary List file contains a corrupted entry\r\n");
		}

		ParseAndCountSummaryListEntriesWithRewrite();
	}
	else
	{
		ReportFileAccessProblem(s_summaryListFileName);
	}

	g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
	close(g_summaryList.file);
#endif
	ReleaseSpi1MutexLock();
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ClearAndFillInCommonRecordInfo(EVT_RECORD* eventRec)
{
	uint8 i;

	memset(eventRec, 0x00, sizeof(EVT_RECORD));
	memset(eventRec->summary.captured.unused, 0xBF, sizeof(eventRec->summary.captured.unused));
	memset(eventRec->summary.calculated.unused, 0xCF, sizeof(eventRec->summary.calculated.unused));

	//--------------------------------
	eventRec->header.startFlag = (uint16)EVENT_RECORD_START_FLAG;
	eventRec->header.recordVersion = (uint16)EVENT_RECORD_VERSION;
	eventRec->header.headerLength = (uint16)sizeof(EVENT_HEADER_STRUCT);
	eventRec->header.summaryLength = (uint16)sizeof(EVENT_SUMMARY_STRUCT);
	//--------------------------------
	eventRec->summary.parameters.sampleRate = (uint16)g_triggerRecord.trec.sample_rate;
	//--------------------------------
	eventRec->summary.captured.endTime = GetCurrentTime();
	eventRec->summary.captured.batteryLevel = (uint32)(100.0 * GetExternalVoltageLevelAveraged(BATTERY_VOLTAGE));
	eventRec->summary.captured.bargraphSessionComplete = NO;
	eventRec->summary.captured.externalTrigger = NO;
	eventRec->summary.captured.comboEventsRecordedDuringSession = 0;
	eventRec->summary.captured.comboEventsRecordedStartNumber = 0;
	eventRec->summary.captured.comboEventsRecordedEndNumber = 0;
	eventRec->summary.captured.comboBargraphEventNumberLink = 0;
	//--------------------------------
	if (g_triggerRecord.trec.variableTriggerEnable) { eventRec->summary.captured.variableTriggerPercentageLevel = g_triggerRecord.trec.variableTriggerPercentageLevel; }
	//--------------------------------
	eventRec->summary.parameters.calibrationDateSource = g_currentCalibration.source;
	memset(&(eventRec->summary.captured.calDateTime), 0, sizeof(eventRec->summary.captured.calDateTime));
	ConvertCalDatetoDateTime(&eventRec->summary.captured.calDateTime, &g_currentCalibration.date);
	eventRec->summary.parameters.seismicSensorCurrentCalDate = g_seismicSmartSensorMemory.currentCal.calDate;
	eventRec->summary.parameters.acousticSensorCurrentCalDate = g_acousticSmartSensorMemory.currentCal.calDate;
	// Store Unit cal date in smart sensor field if the selected source is a smart sensor cal date (to prevent duplication of data and losing the Unit cal date)
	if (g_currentCalibration.source == SEISMIC_SMART_SENSOR_CAL_DATE) { eventRec->summary.parameters.seismicSensorCurrentCalDate = g_factorySetupRecord.calDate; }
	else if (g_currentCalibration.source == ACOUSTIC_SMART_SENSOR_CAL_DATE) { eventRec->summary.parameters.acousticSensorCurrentCalDate = g_factorySetupRecord.calDate; }
	//-----------------------
	memset(&(eventRec->summary.parameters.seismicSensorSerialNumber[0]), 0, SENSOR_SERIAL_NUMBER_SIZE);
	memcpy(&(eventRec->summary.parameters.seismicSensorSerialNumber[0]), &(g_seismicSmartSensorMemory.serialNumber[0]), SENSOR_SERIAL_NUMBER_SIZE);
	eventRec->summary.parameters.seismicSensorFacility = g_seismicSmartSensorMemory.currentCal.calFacility;
	eventRec->summary.parameters.seismicSensorInstrument = g_seismicSmartSensorMemory.currentCal.calInstrument;
	memset(&(eventRec->summary.parameters.acousticSensorSerialNumber[0]), 0, SENSOR_SERIAL_NUMBER_SIZE);
	memcpy(&(eventRec->summary.parameters.acousticSensorSerialNumber[0]), &(g_acousticSmartSensorMemory.serialNumber[0]), SENSOR_SERIAL_NUMBER_SIZE);
	eventRec->summary.parameters.acousticSensorFacility = g_acousticSmartSensorMemory.currentCal.calFacility;
	eventRec->summary.parameters.acousticSensorInstrument = g_acousticSmartSensorMemory.currentCal.calInstrument;
	//-----------------------
	memset(&(eventRec->summary.parameters.companyName[0]), 0, COMPANY_NAME_STRING_SIZE);
	memcpy(&(eventRec->summary.parameters.companyName[0]), &(g_triggerRecord.trec.client[0]), COMPANY_NAME_STRING_SIZE - 1);
	memset(&(eventRec->summary.parameters.seismicOperator[0]), 0, SEISMIC_OPERATOR_STRING_SIZE);
	memcpy(&(eventRec->summary.parameters.seismicOperator[0]), &(g_triggerRecord.trec.oper[0]), SEISMIC_OPERATOR_STRING_SIZE - 1);
	memset(&(eventRec->summary.parameters.sessionLocation[0]), 0, SESSION_LOCATION_STRING_SIZE);
	memcpy(&(eventRec->summary.parameters.sessionLocation[0]), &(g_triggerRecord.trec.loc[0]), SESSION_LOCATION_STRING_SIZE - 1);
	memset(&(eventRec->summary.parameters.sessionComments[0]), 0, SESSION_COMMENTS_STRING_SIZE);
	memcpy(&(eventRec->summary.parameters.sessionComments[0]), &(g_triggerRecord.trec.comments[0]), sizeof(g_triggerRecord.trec.comments));
	//-----------------------
	memset(&(eventRec->summary.version.modelNumber[0]), 0, MODEL_STRING_SIZE);
	memcpy(&(eventRec->summary.version.modelNumber[0]), &(g_factorySetupRecord.unitSerialNumber[0]), 15);
	memset(&(eventRec->summary.version.serialNumber[0]), 0, SERIAL_NUMBER_STRING_SIZE);
	memcpy(&(eventRec->summary.version.serialNumber[0]), &(g_factorySetupRecord.unitSerialNumber[0]), 15);
	memset(&(eventRec->summary.version.softwareVersion[0]), 0, VERSION_STRING_SIZE);
	memcpy(&(eventRec->summary.version.softwareVersion[0]), (void*)&g_buildVersion[0], strlen(g_buildVersion));
	memset(&(eventRec->summary.version.seismicSensorRom), 0, sizeof(g_seismicSmartSensorRom));
	memcpy(&(eventRec->summary.version.seismicSensorRom), (void*)&g_seismicSmartSensorRom, sizeof(g_seismicSmartSensorRom));
	memset(&(eventRec->summary.version.acousticSensorRom), 0, sizeof(g_acousticSmartSensorRom));
	memcpy(&(eventRec->summary.version.acousticSensorRom), (void*)&g_acousticSmartSensorRom, sizeof(g_acousticSmartSensorRom));
	eventRec->summary.version.hardwareId = GET_HARDWARE_ID;

	//-----------------------
	eventRec->summary.parameters.bitAccuracy = ((g_triggerRecord.trec.bitAccuracy < ACCURACY_10_BIT) || (g_triggerRecord.trec.bitAccuracy > ACCURACY_16_BIT)) ? 
												ACCURACY_16_BIT : g_triggerRecord.trec.bitAccuracy;
	eventRec->summary.parameters.numOfChannels = NUMBER_OF_CHANNELS_DEFAULT;
	eventRec->summary.parameters.aWeighting = ((uint8)g_factorySetupRecord.aWeightOption & (uint8)g_unitConfig.airScale); // Equals 1 if enabled (1) and scale is A-weighting (1)
	eventRec->summary.parameters.seismicSensorType = g_factorySetupRecord.seismicSensorType;
	eventRec->summary.parameters.airSensorType = g_factorySetupRecord.acousticSensorType;
	eventRec->summary.parameters.adChannelVerification = g_unitConfig.adChannelVerification;
	eventRec->summary.parameters.adjustForTempDrift = g_triggerRecord.trec.adjustForTempDrift;
	eventRec->summary.parameters.pretrigBufferDivider = g_unitConfig.pretrigBufferDivider;
	eventRec->summary.parameters.seismicUnitsOfMeasure = g_unitConfig.unitsOfMeasure;
	eventRec->summary.parameters.airUnitsOfMeasure = g_unitConfig.unitsOfAir;
	eventRec->summary.parameters.distToSource = (uint32)(g_triggerRecord.trec.dist_to_source * 100.0);
	eventRec->summary.parameters.weightPerDelay = (uint32)(g_triggerRecord.trec.weight_per_delay * 100.0);
	//-----------------------
	eventRec->summary.parameters.channel[0].type = ACOUSTIC_CHANNEL_TYPE;
	eventRec->summary.parameters.channel[0].input = 4;
	eventRec->summary.parameters.channel[1].type = RADIAL_CHANNEL_TYPE;
	eventRec->summary.parameters.channel[1].input = 1;
	eventRec->summary.parameters.channel[2].type = VERTICAL_CHANNEL_TYPE;
	eventRec->summary.parameters.channel[2].input = 3;
	eventRec->summary.parameters.channel[3].type = TRANSVERSE_CHANNEL_TYPE;
	eventRec->summary.parameters.channel[3].input = 2;
	//-----------------------
	eventRec->summary.calculated.gpsPosition = g_gpsPosition;
	eventRec->summary.parameters.utcZoneOffset = g_unitConfig.utcZoneOffset;
	//-----------------------

	for (i = 0; i < 4; i++) // First seismic group
	{
		eventRec->summary.parameters.channel[i].group = SEISMIC_GROUP_1;
		eventRec->summary.parameters.channel[i].options = (g_triggerRecord.srec.sensitivity == LOW) ? GAIN_SELECT_x2 : GAIN_SELECT_x4;
	}

	for (i = 4; i < 8; i++) // Second seismic group
	{
		eventRec->summary.parameters.channel[i].group = SEISMIC_GROUP_2;
		eventRec->summary.parameters.channel[i].type = 0;
		eventRec->summary.parameters.channel[i].input = DISABLED;
		eventRec->summary.parameters.channel[i].options = 0;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void FillInTriggerLevelRecordInfo(EVT_RECORD* eventRec)
{
	float tempSesmicTriggerInUnits;
	float unitsDiv;

	eventRec->summary.parameters.recordTime = (uint32)g_triggerRecord.trec.record_time;

	if ((g_triggerRecord.trec.seismicTriggerLevel == NO_TRIGGER_CHAR) || (g_triggerRecord.trec.seismicTriggerLevel == EXTERNAL_TRIGGER_CHAR) ||
	(g_triggerRecord.trec.airTriggerLevel == MANUAL_TRIGGER_CHAR))
	{
		eventRec->summary.parameters.seismicTriggerLevel = g_triggerRecord.trec.seismicTriggerLevel;

		debug("Seismic trigger in units: No Trigger\r\n");
	}
	else if (g_triggerRecord.trec.variableTriggerEnable == YES)
	{
		eventRec->summary.parameters.seismicTriggerLevel = (VARIABLE_TRIGGER_CHAR_BASE + g_triggerRecord.trec.variableTriggerVibrationStandard);

		debug("Seismic trigger in units: Variable Trigger (Vibration Standard: %d)\r\n", g_triggerRecord.trec.variableTriggerVibrationStandard);
	}
	else // Seismic trigger is standard/valid
	{
		eventRec->summary.parameters.seismicTriggerLevel = g_triggerRecord.trec.seismicTriggerLevel / (ACCURACY_16_BIT_MIDPOINT / g_bitAccuracyMidpoint);

		// Calculate the divider used for converting stored A/D peak counts to units of measure
		if ((g_factorySetupRecord.seismicSensorType == SENSOR_ACC_832M1_0200) || (g_factorySetupRecord.seismicSensorType == SENSOR_ACC_832M1_0500))
		{
			unitsDiv = (float)(g_bitAccuracyMidpoint * SENSOR_ACCURACY_100X_SHIFT * ((g_triggerRecord.srec.sensitivity == LOW) ? 2 : 4)) / (float)(g_factorySetupRecord.seismicSensorType * ACC_832M1_SCALER);
		}
		else unitsDiv = (float)(g_bitAccuracyMidpoint * SENSOR_ACCURACY_100X_SHIFT * ((g_triggerRecord.srec.sensitivity == LOW) ? 2 : 4)) / (float)(g_factorySetupRecord.seismicSensorType);

		tempSesmicTriggerInUnits = (float)(g_triggerRecord.trec.seismicTriggerLevel >> g_bitShiftForAccuracy) / (float)unitsDiv;

		if ((g_factorySetupRecord.seismicSensorType < SENSOR_ACC_RANGE_DIVIDER) && (g_unitConfig.unitsOfMeasure == METRIC_TYPE))
		{
			tempSesmicTriggerInUnits *= (float)METRIC;
		}

		debug("Seismic trigger in units: %05.2f %s\r\n", tempSesmicTriggerInUnits, (g_unitConfig.unitsOfMeasure == METRIC_TYPE ? "mm" : "in"));
		eventRec->summary.parameters.seismicTriggerInUnits = (uint32)(tempSesmicTriggerInUnits * 100);
	}

	if ((g_triggerRecord.trec.airTriggerLevel == NO_TRIGGER_CHAR) || (g_triggerRecord.trec.airTriggerLevel == EXTERNAL_TRIGGER_CHAR) ||
	(g_triggerRecord.trec.airTriggerLevel == MANUAL_TRIGGER_CHAR))
	{
		eventRec->summary.parameters.airTriggerLevel = g_triggerRecord.trec.airTriggerLevel;

		debug("Air trigger in units: No Trigger\r\n");
	}
	else // Air trigger is valid
	{
		eventRec->summary.parameters.airTriggerLevel = g_triggerRecord.trec.airTriggerLevel / (ACCURACY_16_BIT_MIDPOINT / g_bitAccuracyMidpoint);

		eventRec->summary.parameters.airTriggerInUnits = AirTriggerConvertToUnits(g_triggerRecord.trec.airTriggerLevel);

		if (g_unitConfig.unitsOfAir == MILLIBAR_TYPE) { debug("Air trigger in units: %05.3f mB\r\n", (float)(eventRec->summary.parameters.airTriggerInUnits / 10000)); }
		else if (g_unitConfig.unitsOfAir == PSI_TYPE) { debug("Air trigger in units: %05.3f PSI\r\n", (float)(eventRec->summary.parameters.airTriggerInUnits / 10000)); }
		else /* (g_unitConfig.unitsOfAir == DECIBEL_TYPE) */ { debug("Air trigger in units: %d dB\r\n", eventRec->summary.parameters.airTriggerInUnits); }
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitEventRecord(uint8 opMode)
{
	EVT_RECORD* eventRec;
	uint8 idex;

	if ((opMode == WAVEFORM_MODE) || (opMode == MANUAL_CAL_MODE) || (opMode == COMBO_MODE))
	{
		eventRec = &g_pendingEventRecord;		
		ClearAndFillInCommonRecordInfo(eventRec);

		eventRec->summary.mode = opMode;
		if (opMode == COMBO_MODE) { eventRec->summary.subMode = WAVEFORM_MODE; }
		eventRec->summary.eventNumber = (uint16)g_nextEventNumberToUse;

		eventRec->summary.parameters.numOfSamples = (uint16)(g_triggerRecord.trec.sample_rate * g_triggerRecord.trec.record_time);
		eventRec->summary.parameters.preBuffNumOfSamples = (uint16)(g_triggerRecord.trec.sample_rate / g_unitConfig.pretrigBufferDivider);
		eventRec->summary.parameters.calDataNumOfSamples = (uint16)(CALIBRATION_NUMBER_OF_SAMPLES);

		// Reset parameters for the special calibration mode
		if (opMode == MANUAL_CAL_MODE)
		{
			eventRec->summary.parameters.sampleRate = MANUAL_CAL_DEFAULT_SAMPLE_RATE;
			eventRec->summary.parameters.bitAccuracy = ACCURACY_16_BIT;
			eventRec->summary.parameters.numOfSamples = 0;
			eventRec->summary.parameters.preBuffNumOfSamples = 0;
			eventRec->summary.parameters.seismicTriggerLevel = 0;
			eventRec->summary.parameters.airTriggerLevel = 0;
			eventRec->summary.parameters.recordTime = 0;
			for (idex = 0; idex < 8; idex++) { eventRec->summary.parameters.channel[idex].options = GAIN_SELECT_x2; }
		}
		else // ((opMode == WAVEFORM_MODE) || (opMode == COMBO_MODE))
		{
			eventRec->summary.parameters.recordTime = (uint32)g_triggerRecord.trec.record_time;
			FillInTriggerLevelRecordInfo(eventRec);
		}	
	}

	if ((opMode == BARGRAPH_MODE) || (opMode == COMBO_MODE))
	{
		eventRec = &g_pendingBargraphRecord;		
		ClearAndFillInCommonRecordInfo(eventRec);

		eventRec->summary.mode = opMode;
		if (opMode == COMBO_MODE)
		{
			eventRec->summary.subMode = BARGRAPH_MODE;
			FillInTriggerLevelRecordInfo(eventRec); // Set the trigger level in Combo - Bargraph to track trigger level in case no events were taken
		}

		eventRec->summary.eventNumber = (uint16)g_nextEventNumberToUse;
		eventRec->summary.captured.eventTime = GetCurrentTime();
		eventRec->summary.parameters.barInterval = (uint16)g_triggerRecord.bgrec.barInterval;
		eventRec->summary.parameters.summaryInterval = (uint16)g_triggerRecord.bgrec.summaryInterval;
		eventRec->summary.parameters.numOfSamples = 0;
		eventRec->summary.parameters.preBuffNumOfSamples = 0;
		eventRec->summary.parameters.calDataNumOfSamples = 0;
		eventRec->summary.parameters.activeChannels = NUMBER_OF_CHANNELS_DEFAULT;

		// Check if either of the new Bar Interval Data types have been selected
		if ((g_triggerRecord.berec.barIntervalDataType == BAR_INTERVAL_A_R_V_T_DATA_TYPE_SIZE) || (g_triggerRecord.berec.barIntervalDataType == BAR_INTERVAL_A_R_V_T_WITH_FREQ_DATA_TYPE_SIZE))
		{
			eventRec->summary.parameters.barIntervalDataType = g_triggerRecord.berec.barIntervalDataType;
		}
		else // Set to the original which also covers initial condition
		{
			eventRec->summary.parameters.barIntervalDataType = g_triggerRecord.berec.barIntervalDataType = BAR_INTERVAL_ORIGINAL_DATA_TYPE_SIZE;
		}
	}	
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitCurrentEventNumber(void)
{
	CURRENT_EVENT_NUMBER_STRUCT currentEventNumberRecord;

	GetRecordData(&currentEventNumberRecord, DEFAULT_RECORD, REC_UNIQUE_EVENT_ID_TYPE);

	if (!currentEventNumberRecord.invalid)
	{
		// Set the Current Event number to the last event number stored plus 1
		g_nextEventNumberToUse = currentEventNumberRecord.currentEventNumber + 1;
	}
	else // record is invalid
	{
		g_nextEventNumberToUse = 1;

		// Don't save as 1 since no event has been recorded; The eventual save will validate the event number
	}

	debug("Stored Event ID: %d, Next Event ID to use: %d\r\n", (g_nextEventNumberToUse - 1), g_nextEventNumberToUse);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint16 GetLastStoredEventNumber(void)
{
	return ((uint16)(g_nextEventNumberToUse - 1));
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void StoreCurrentEventNumber(void)
{
	CURRENT_EVENT_NUMBER_STRUCT currentEventNumberRecord;

#if 0 /* Removed to solve pending Bargraph DQM pull picking up the entry early */
	AddEventNumberToCache(g_nextEventNumberToUse);
#endif

	// Store as the last Event recorded in AutoDialout table
	__autoDialoutTbl.lastStoredEvent = g_nextEventNumberToUse;

	// Store the Current Event number as the newest Unique Event number
	currentEventNumberRecord.currentEventNumber = g_nextEventNumberToUse;
	SaveRecordData(&currentEventNumberRecord, DEFAULT_RECORD, REC_UNIQUE_EVENT_ID_TYPE);

	// Increment to a new Event number
	g_nextEventNumberToUse++;

	// Check if the next event number is at the max total unique events allowed
	if (g_nextEventNumberToUse == TOTAL_UNIQUE_EVENT_NUMBERS)
	{
		// Loop the event number back to the start (0 is invalid)
		g_nextEventNumberToUse = 1;
	}

	debug("Saved Event ID: %d, Next Event ID to use: %d\r\n", (g_nextEventNumberToUse - 1), g_nextEventNumberToUse);

	return;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GetEventFileInfo(uint16 eventNumber, EVENT_HEADER_STRUCT* eventHeaderPtr, EVENT_SUMMARY_STRUCT* eventSummaryPtr, BOOLEAN cacheDataToRamBuffer)
{
	EVENT_HEADER_STRUCT fileEventHeader;
	EVENT_SUMMARY_STRUCT fileSummary;
	char* pathAndFilename;
	int eventFile;

	if (g_fileAccessLock != AVAILABLE)
	{
		ReportFileSystemAccessProblem("Check event file");
	}
	else // (g_fileAccessLock == AVAILABLE)
	{
		GetSpi1MutexLock(SDMMC_LOCK);

#if 0 /* old hw */
		nav_select(FS_NAV_ID_DEFAULT);

#if 0 /* Original */
		sprintf(fileName, "%sEvt%d.ns8", EVENTS_PATH, eventNumber);
#else
		pathAndFilename = GetEventFilenameAndPath(eventNumber, EVENT_FILE_TYPE);
#endif
		eventFile = open(pathAndFilename, O_RDONLY);

		// Verify file ID
		if (eventFile == -1)
		{
			DisplayFileNotFound(pathAndFilename);
		}
		// Verify file is big enough
		else if (fsaccess_file_get_size(eventFile) < sizeof(EVENT_HEADER_STRUCT))
		{
			DisplayFileCorrupt(pathAndFilename);
		}
		else
		{
			ReadWithSizeFix(eventFile, (uint8*)&fileEventHeader, sizeof(EVENT_HEADER_STRUCT));

			// If we find the EVENT_RECORD_START_FLAG followed by the encodeFlag2, then assume this is the start of an event
			if ((fileEventHeader.startFlag == EVENT_RECORD_START_FLAG) &&
			((fileEventHeader.recordVersion & EVENT_MAJOR_VERSION_MASK) == (EVENT_RECORD_VERSION & EVENT_MAJOR_VERSION_MASK)) &&
			(fileEventHeader.headerLength == sizeof(EVENT_HEADER_STRUCT)))
			{
				debug("Found Valid Event File: %s\r\n", pathAndFilename);

				ReadWithSizeFix(eventFile, (uint8*)&fileSummary, sizeof(EVENT_SUMMARY_STRUCT));
			
				if (cacheDataToRamBuffer == YES)
				{
					ReadWithSizeFix(eventFile, (uint8*)&g_eventDataBuffer[0], (fsaccess_file_get_size(eventFile) - (sizeof(EVENT_HEADER_STRUCT) - sizeof(EVENT_SUMMARY_STRUCT))));
				}
			}

			g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
			close(eventFile);
		}
#endif
		ReleaseSpi1MutexLock();
	}

	if (eventHeaderPtr != NULL)
	{
		memcpy(eventHeaderPtr, &fileEventHeader, sizeof(EVENT_HEADER_STRUCT));
	}

	if (eventSummaryPtr != NULL)
	{
		memcpy(eventSummaryPtr, &fileSummary, sizeof(EVENT_SUMMARY_STRUCT));
	}	
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int OpenEventFile(uint16 eventNumber)
{
	char* pathAndFilename;
	int eventFile = 0;
	uint8 dummy;

	if (g_fileAccessLock != AVAILABLE)
	{
		ReportFileSystemAccessProblem("Get event file");
	}
	else // (g_fileAccessLock == AVAILABLE)
	{
		GetSpi1MutexLock(SDMMC_LOCK);

#if 0 /* old hw */
		nav_select(FS_NAV_ID_DEFAULT);

#if 0 /* Original */
		sprintf(fileName, "%sEvt%d.ns8", EVENTS_PATH, eventNumber);
#else
		pathAndFilename = GetEventFilenameAndPath(eventNumber, EVENT_FILE_TYPE);
#endif
		eventFile = open(pathAndFilename, O_RDONLY);

		// Dummy read to cache first sector and set up globals
		dummy = file_getc();
		file_seek(0, FS_SEEK_SET);
#endif
	}

	return (eventFile);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void CloseEventFile(int eventFile)
{
	g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
#if 0 /* old hw */
	close(eventFile);
#endif
	ReleaseSpi1MutexLock();
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 CheckCompressedEventDataFileExists(uint16 eventNumber)
{
	uint8 fileExistStatus = NO;
	uint16 lowerBounds, upperBounds;

	GetSpi1MutexLock(SDMMC_LOCK);

#if 0 /* old hw */
	nav_select(FS_NAV_ID_DEFAULT);

	GetSubDirLowerAndUpperBounds(eventNumber, &lowerBounds, &upperBounds);

	sprintf(g_spareFileName, "%s%s %d-%d\\%s%d.nsD", ER_DATA_PATH, EVTS_SUB_DIR, lowerBounds, upperBounds, EVT_FILE, eventNumber);

	// Check if the Compressed data file is present
	if (nav_setcwd(g_spareFileName, TRUE, FALSE) == TRUE)
	{
		fileExistStatus = YES;
	}
	else
	{
		sprintf(g_spareFileName, "%s%s%d.nsD", ER_DATA_PATH, EVT_FILE, eventNumber);

		if (nav_setcwd(g_spareFileName, TRUE, FALSE) == TRUE)
		{
			fileExistStatus = YES;
		}
	}
#endif
	ReleaseSpi1MutexLock();
	
	return (fileExistStatus);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GetEventFileRecord(uint16 eventNumber, EVT_RECORD* eventRecord)
{
	char* pathAndFilename;
	int eventFile;

	if (g_fileAccessLock != AVAILABLE)
	{
		ReportFileSystemAccessProblem("Get event file");
	}
	else // (g_fileAccessLock == AVAILABLE)
	{
		GetSpi1MutexLock(SDMMC_LOCK);

#if 0 /* old hw */
		nav_select(FS_NAV_ID_DEFAULT);

#if 0 /* Original */
		sprintf(fileName, "%sEvt%d.ns8", EVENTS_PATH, eventNumber);
#else
		pathAndFilename = GetEventFilenameAndPath(eventNumber, EVENT_FILE_TYPE);
#endif
		eventFile = open(pathAndFilename, O_RDONLY);

		// Verify file ID
		if (eventFile == -1)
		{
			DisplayFileNotFound(pathAndFilename);
		}
		// Verify file is big enough
		else if (fsaccess_file_get_size(eventFile) < sizeof(EVENT_HEADER_STRUCT))
		{
			DisplayFileCorrupt(pathAndFilename);
		}
		else
		{
			ReadWithSizeFix(eventFile, (uint8*)eventRecord, sizeof(EVT_RECORD));
			g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
			close(eventFile);
		}
#endif
		ReleaseSpi1MutexLock();
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DeleteEventFileRecord(uint16 eventNumber)
{
	uint16 lowerBounds, upperBounds;

	if (g_fileAccessLock != AVAILABLE)
	{
		ReportFileSystemAccessProblem("Delete event file");
	}
	else // (g_fileAccessLock == AVAILABLE)
	{
		GetSpi1MutexLock(SDMMC_LOCK);

#if 0 /* old hw */
		nav_select(FS_NAV_ID_DEFAULT);

		GetSubDirLowerAndUpperBounds(eventNumber, &lowerBounds, &upperBounds);

		sprintf(g_spareFileName, "%s%s %d-%d\\%s%d.ns8", EVENTS_PATH, EVTS_SUB_DIR, lowerBounds, upperBounds, EVT_FILE, eventNumber);

		if (nav_setcwd(g_spareFileName, TRUE, FALSE) == TRUE)
		{
			if (nav_file_del(TRUE) == FALSE)
			{
				sprintf((char*)g_spareBuffer, "%s %s", getLangText(UNABLE_TO_DELETE_TEXT), getLangText(EVENT_TEXT));
				OverlayMessage(g_spareFileName, (char*)g_spareBuffer, 3 * SOFT_SECS);
			}
		}
		else
		{
			sprintf(g_spareFileName, "%sEvt%d.ns8", EVENTS_PATH, eventNumber);

			if (nav_setcwd(g_spareFileName, TRUE, FALSE) == TRUE)
			{
				if (nav_file_del(TRUE) == FALSE)
				{
					sprintf((char*)g_spareBuffer, "%s %s", getLangText(UNABLE_TO_DELETE_TEXT), getLangText(EVENT_TEXT));
					OverlayMessage(g_spareFileName, (char*)g_spareBuffer, 3 * SOFT_SECS);
				}
			}
		}
#endif
		ReleaseSpi1MutexLock();
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DeleteEventFileRecords(void)
{
	debug("Deleting Events...\r\n");

	if (g_fileAccessLock != AVAILABLE)
	{
		ReportFileSystemAccessProblem("Delete multiple events");
	}
	else // (g_fileAccessLock == AVAILABLE)
	{
		GetSpi1MutexLock(SDMMC_LOCK);

#if 0 /* old hw */
		nav_select(FS_NAV_ID_DEFAULT);

		// Handle removing event files
		if (nav_setcwd(EVENTS_PATH, TRUE, FALSE) == TRUE)
		{
			while (nav_filelist_set(0 , FS_FIND_NEXT))
			{
				nav_file_getname(g_spareFileName, MAX_FILE_NAME_CHARS);
				sprintf((char*)g_spareBuffer, "%s %s", getLangText(REMOVING_TEXT), g_spareFileName);
				OverlayMessage(getLangText(STATUS_TEXT), (char*)g_spareBuffer, 0);

				// Delete file or directory
				if (nav_file_del(FALSE) == FALSE)
				{
					nav_file_getname(g_spareFileName, MAX_FILE_NAME_CHARS);
					sprintf((char*)g_spareBuffer, "%s %s", getLangText(UNABLE_TO_DELETE_TEXT), getLangText(EVENT_TEXT));
					OverlayMessage(g_spareFileName, (char*)g_spareBuffer, 3 * SOFT_SECS);
					//break;
				}
				else
				{
#if 0 /* Exception testing (Prevent non-ISR soft loop watchdog from triggering) */
					g_execCycles++;
#endif
				}
			}
		}

		// Handle removing compressed event data files
		if (nav_setcwd(ER_DATA_PATH, TRUE, FALSE) == TRUE)
		{
			while (nav_filelist_set(0 , FS_FIND_NEXT))
			{
				nav_file_getname(g_spareFileName, MAX_FILE_NAME_CHARS);
				sprintf((char*)g_spareBuffer, "%s %s", getLangText(REMOVING_TEXT), g_spareFileName);
				OverlayMessage(getLangText(STATUS_TEXT), (char*)g_spareBuffer, 0);

				// Delete file or directory
				if (nav_file_del(FALSE) == FALSE)
				{
					nav_file_getname(g_spareFileName, MAX_FILE_NAME_CHARS);
					sprintf((char*)g_spareBuffer, "%s %s", getLangText(UNABLE_TO_DELETE_TEXT), getLangText(EVENT_TEXT));
					OverlayMessage(g_spareFileName, (char*)g_spareBuffer, 3 * SOFT_SECS);
					//break;
				}
				else
				{
#if 0 /* Exception testing (Prevent non-ISR soft loop watchdog from triggering) */
					g_execCycles++;
#endif
				}
			}
		}

		// Re-create directory
		if (nav_setcwd(ER_DATA_PATH, TRUE, FALSE) == FALSE)
		{
			debugErr("Unable to access or create the Events directory\r\n");
			sprintf((char*)g_spareBuffer, "%s %s %s", getLangText(UNABLE_TO_ACCESS_TEXT), "ERDATA", getLangText(DIR_TEXT));
			OverlayMessage(getLangText(ERROR_TEXT), (char*)g_spareBuffer, 3 * SOFT_SECS);
		}

		// Re-create directory
		if (nav_setcwd(EVENTS_PATH, TRUE, FALSE) == FALSE)
		{
			debugErr("Unable to access or create the Events directory\r\n");
			sprintf((char*)g_spareBuffer, "%s %s %s", getLangText(UNABLE_TO_ACCESS_TEXT), "EVENTS", getLangText(DIR_TEXT));
			OverlayMessage(getLangText(ERROR_TEXT), (char*)g_spareBuffer, 3 * SOFT_SECS);
		}

		if (nav_setcwd(s_summaryListFileName, TRUE, FALSE) == TRUE)
		{
			if (nav_file_del(FALSE) == FALSE)
			{
				nav_file_getname(g_spareFileName, MAX_FILE_NAME_CHARS);
				sprintf((char*)g_spareBuffer, "%s %s", getLangText(UNABLE_TO_DELETE_TEXT), getLangText(SUMMARY_LIST_TEXT));
				OverlayMessage(g_spareFileName, (char*)g_spareBuffer, 3 * SOFT_SECS);
			}
		}
#endif
		ReleaseSpi1MutexLock();

		sprintf((char*)g_spareBuffer, "%s %d %s", getLangText(REMOVED_TEXT), g_eventNumberCacheValidEntries, getLangText(EVENTS_TEXT));
		OverlayMessage(getLangText(DELETE_EVENTS_TEXT), (char*)g_spareBuffer, 3 * SOFT_SECS);

		InitEventNumberCache();
		InitSummaryListFile();
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DeleteNonEssentialFiles(void)
{
	uint16 filesDeleted = 0;

	debug("Deleting Non-Essential Files...\r\n");

	if (g_fileAccessLock != AVAILABLE)
	{
		ReportFileSystemAccessProblem("Delete multiple non-essential files");
	}
	else // (g_fileAccessLock == AVAILABLE)
	{
		GetSpi1MutexLock(SDMMC_LOCK);

#if 0 /* old hw */
		nav_select(FS_NAV_ID_DEFAULT);

		//-----------------------------------------------------------------------------
		// Handle removing event files
		//-----------------------------------------------------------------------------
		if (nav_setcwd(EVENTS_PATH, TRUE, FALSE) == TRUE)
		{
			while (nav_filelist_set(0 , FS_FIND_NEXT))
			{
				nav_file_getname(g_spareFileName, MAX_FILE_NAME_CHARS);
				sprintf((char*)g_spareBuffer, "%s %s", getLangText(REMOVING_TEXT), g_spareFileName);
				OverlayMessage(getLangText(STATUS_TEXT), (char*)g_spareBuffer, 0);

				// Delete file or directory
				if (nav_file_del(FALSE) == FALSE)
				{
					nav_file_getname(g_spareFileName, MAX_FILE_NAME_CHARS);
					sprintf((char*)g_spareBuffer, "%s %s", getLangText(UNABLE_TO_DELETE_TEXT), getLangText(EVENT_TEXT));
					OverlayMessage(g_spareFileName, (char*)g_spareBuffer, 3 * SOFT_SECS);
					//break;
				}
				else
				{
					filesDeleted++;
				}
			}
		}

		//-----------------------------------------------------------------------------
		// Handle removing compressed event data files
		//-----------------------------------------------------------------------------
		if (nav_setcwd(ER_DATA_PATH, TRUE, FALSE) == TRUE)
		{
			while (nav_filelist_set(0 , FS_FIND_NEXT))
			{
				nav_file_getname(g_spareFileName, MAX_FILE_NAME_CHARS);
				sprintf((char*)g_spareBuffer, "%s %s", getLangText(REMOVING_TEXT), g_spareFileName);
				OverlayMessage(getLangText(STATUS_TEXT), (char*)g_spareBuffer, 0);

				// Delete file or directory
				if (nav_file_del(FALSE) == FALSE)
				{
					nav_file_getname(g_spareFileName, MAX_FILE_NAME_CHARS);
					sprintf((char*)g_spareBuffer, "%s %s", getLangText(UNABLE_TO_DELETE_TEXT), getLangText(EVENT_TEXT));
					OverlayMessage(g_spareFileName, (char*)g_spareBuffer, 3 * SOFT_SECS);
					//break;
				}
				else
				{
					filesDeleted++;
				}
			}
		}

		//-----------------------------------------------------------------------------
		// Handle removing Log files
		//-----------------------------------------------------------------------------
		if (nav_setcwd(LOGS_PATH, TRUE, FALSE) == TRUE)
		{
			while (nav_filelist_set(0 , FS_FIND_NEXT))
			{
				nav_file_getname(g_spareFileName, MAX_FILE_NAME_CHARS);
				sprintf((char*)g_spareBuffer, "%s %s", getLangText(REMOVING_TEXT), g_spareFileName);
				OverlayMessage(getLangText(STATUS_TEXT), (char*)g_spareBuffer, 0);

				// Delete file or directory
				if (nav_file_del(FALSE) == FALSE)
				{
					nav_file_getname(g_spareFileName, MAX_FILE_NAME_CHARS);
					sprintf((char*)g_spareBuffer, "%s %s", getLangText(UNABLE_TO_DELETE_TEXT), getLangText(EVENT_TEXT));
					OverlayMessage(g_spareFileName, (char*)g_spareBuffer, 3 * SOFT_SECS);
					//break;
				}
				else
				{
					filesDeleted++;
				}
			}
		}

		//-----------------------------------------------------------------------------
		// Handle removing non Language files except for .tbl extension
		//-----------------------------------------------------------------------------
		if (nav_setcwd(LANGUAGE_PATH, TRUE, FALSE) == TRUE)
		{
			while (nav_filelist_set(0 , FS_FIND_NEXT))
			{
				nav_file_getname(g_spareFileName, MAX_FILE_NAME_CHARS);

				if (nav_file_checkext("tbl") == FALSE)
				{
					sprintf((char*)g_spareBuffer, "%s %s", getLangText(REMOVING_TEXT), g_spareFileName);
					OverlayMessage(getLangText(STATUS_TEXT), (char*)g_spareBuffer, 0);

					// Delete file or directory
					if (nav_file_del(FALSE) == FALSE)
					{
						nav_file_getname(g_spareFileName, MAX_FILE_NAME_CHARS);
						sprintf((char*)g_spareBuffer, "%s %s", getLangText(UNABLE_TO_DELETE_TEXT), getLangText(EVENT_TEXT));
						OverlayMessage(g_spareFileName, (char*)g_spareBuffer, 3 * SOFT_SECS);
						//break;
					}
					else
					{
						filesDeleted++;
					}
				}
			}
		}

		//-----------------------------------------------------------------------------
		// Handle removing System files except for Boot/Apploader
		//-----------------------------------------------------------------------------
		if (nav_setcwd(SYSTEM_PATH, TRUE, FALSE) == TRUE)
		{
			while (nav_filelist_set(0 , FS_FIND_NEXT))
			{
				nav_file_getname(g_spareFileName, MAX_FILE_NAME_CHARS);

				if (strcasecmp(g_spareFileName, "Boot.s") != 0)
				{
					sprintf((char*)g_spareBuffer, "%s %s", getLangText(REMOVING_TEXT), g_spareFileName);
					OverlayMessage(getLangText(STATUS_TEXT), (char*)g_spareBuffer, 0);

					// Delete file or directory
					if (nav_file_del(FALSE) == FALSE)
					{
						nav_file_getname(g_spareFileName, MAX_FILE_NAME_CHARS);
						sprintf((char*)g_spareBuffer, "%s %s", getLangText(UNABLE_TO_DELETE_TEXT), getLangText(EVENT_TEXT));
						OverlayMessage(g_spareFileName, (char*)g_spareBuffer, 3 * SOFT_SECS);
						//break;
					}
					else
					{
						filesDeleted++;
					}
				}
			}
		}

		//-----------------------------------------------------------------------------
		// Re-create directory
		//-----------------------------------------------------------------------------
		if (nav_setcwd(ER_DATA_PATH, TRUE, FALSE) == FALSE)
		{
			debugErr("Unable to access or create the ERData directory\r\n");
			sprintf((char*)g_spareBuffer, "%s %s %s", getLangText(UNABLE_TO_ACCESS_TEXT), "ERDATA", getLangText(DIR_TEXT));
			OverlayMessage(getLangText(ERROR_TEXT), (char*)g_spareBuffer, 3 * SOFT_SECS);
		}

		//-----------------------------------------------------------------------------
		// Re-create directory
		//-----------------------------------------------------------------------------
		if (nav_setcwd(EVENTS_PATH, TRUE, FALSE) == FALSE)
		{
			debugErr("Unable to access or create the Events directory\r\n");
			sprintf((char*)g_spareBuffer, "%s %s %s", getLangText(UNABLE_TO_ACCESS_TEXT), "EVENTS", getLangText(DIR_TEXT));
			OverlayMessage(getLangText(ERROR_TEXT), (char*)g_spareBuffer, 3 * SOFT_SECS);
		}

		//-----------------------------------------------------------------------------
		// Re-create directory
		//-----------------------------------------------------------------------------
		if (nav_setcwd(LANGUAGE_PATH, TRUE, FALSE) == FALSE)
		{
			debugErr("Unable to access or create the Events directory\r\n");
			sprintf((char*)g_spareBuffer, "%s %s %s", getLangText(UNABLE_TO_ACCESS_TEXT), "LANGUAGE", getLangText(DIR_TEXT));
			OverlayMessage(getLangText(ERROR_TEXT), (char*)g_spareBuffer, 3 * SOFT_SECS);
		}

		//-----------------------------------------------------------------------------
		// Re-create directory
		//-----------------------------------------------------------------------------
		if (nav_setcwd(LOGS_PATH, TRUE, FALSE) == FALSE)
		{
			debugErr("Unable to access or create the Events directory\r\n");
			sprintf((char*)g_spareBuffer, "%s %s %s", getLangText(UNABLE_TO_ACCESS_TEXT), "LOGS", getLangText(DIR_TEXT));
			OverlayMessage(getLangText(ERROR_TEXT), (char*)g_spareBuffer, 3 * SOFT_SECS);
		}

		//-----------------------------------------------------------------------------
		// Re-create directory
		//-----------------------------------------------------------------------------
		if (nav_setcwd(SYSTEM_PATH, TRUE, FALSE) == FALSE)
		{
			debugErr("Unable to access or create the Events directory\r\n");
			sprintf((char*)g_spareBuffer, "%s %s %s", getLangText(UNABLE_TO_ACCESS_TEXT), "SYSTEM", getLangText(DIR_TEXT));
			OverlayMessage(getLangText(ERROR_TEXT), (char*)g_spareBuffer, 3 * SOFT_SECS);
		}

		//-----------------------------------------------------------------------------
		// Re-create Summary List
		//-----------------------------------------------------------------------------
		if (nav_setcwd(s_summaryListFileName, TRUE, FALSE) == TRUE)
		{
			if (nav_file_del(FALSE) == FALSE)
			{
				nav_file_getname(g_spareFileName, MAX_FILE_NAME_CHARS);
				sprintf((char*)g_spareBuffer, "%s %s", getLangText(UNABLE_TO_DELETE_TEXT), getLangText(SUMMARY_LIST_TEXT));
				OverlayMessage(g_spareFileName, (char*)g_spareBuffer, 3 * SOFT_SECS);
			}
		}
#endif
		ReleaseSpi1MutexLock();

		sprintf((char*)g_spareBuffer, "%s %d %s", getLangText(REMOVED_TEXT), filesDeleted, getLangText(TOTAL_FILES_TEXT));
		OverlayMessage(getLangText(STATUS_TEXT), (char*)g_spareBuffer, 3 * SOFT_SECS);

		InitEventNumberCache();
		InitSummaryListFile();
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void CacheEventDataToBuffer(uint16 eventNumber, uint8* dataBuffer, uint32 dataOffset, uint32 dataSize)
{
	char* pathAndFilename;
	int eventFile;

	GetSpi1MutexLock(SDMMC_LOCK);

#if 0 /* old hw */
	nav_select(FS_NAV_ID_DEFAULT);

#if 0 /* Original */
	sprintf(fileName, "%sEvt%d.ns8", EVENTS_PATH, eventNumber);
#else
	pathAndFilename = GetEventFilenameAndPath(eventNumber, EVENT_FILE_TYPE);
#endif
	eventFile = open(pathAndFilename, O_RDONLY);

	// Verify file ID
	if (eventFile == -1)
	{
		DisplayFileNotFound(pathAndFilename);
	}
	else
	{
		file_seek(dataOffset, FS_SEEK_SET);
		ReadWithSizeFix(eventFile, dataBuffer, dataSize);

		g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
		close(eventFile);
	}
#endif
	ReleaseSpi1MutexLock();
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint32 GetEventSize(uint16 eventNumber)
{
	uint32 size = 0;
	char* pathAndFilename;
	int eventFile;

	GetSpi1MutexLock(SDMMC_LOCK);

#if 0 /* old hw */
	nav_select(FS_NAV_ID_DEFAULT);

	pathAndFilename = GetEventFilenameAndPath(eventNumber, EVENT_FILE_TYPE);
	eventFile = open(pathAndFilename, O_RDONLY);

	// Verify file ID
	if (eventFile == -1)
	{
		DisplayFileNotFound(pathAndFilename);
	}
	else
	{
		size = nav_file_lgt();

		g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
		close(eventFile);
	}
#endif
	ReleaseSpi1MutexLock();

	return (size);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint32 GetERDataSize(uint16 eventNumber)
{
	uint32 size = 0;
	char* pathAndFilename;
	int eventFile;

	GetSpi1MutexLock(SDMMC_LOCK);

#if 0 /* old hw */
	nav_select(FS_NAV_ID_DEFAULT);

#if 0 /* Original */
	sprintf(fileName, "%sEvt%d.nsD", ER_DATA_PATH, eventNumber);
#else
	pathAndFilename = GetEventFilenameAndPath(eventNumber, ER_DATA_FILE_TYPE);
#endif
	eventFile = open(pathAndFilename, O_RDONLY);

	// Verify file ID
	if (eventFile == -1)
	{
		DisplayFileNotFound(pathAndFilename);
	}
	else
	{
		size = nav_file_lgt();

		g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
		close(eventFile);
	}
#endif
	ReleaseSpi1MutexLock();

	return (size);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void CacheERDataToBuffer(uint16 eventNumber, uint8* dataBuffer, uint32 dataOffset, uint32 dataSize)
{
	char* pathAndFilename;
	int eventFile;

	GetSpi1MutexLock(SDMMC_LOCK);

#if 0 /* old hw */
	nav_select(FS_NAV_ID_DEFAULT);

#if 0 /* Original */
	sprintf(fileName, "%sEvt%d.nsD", ER_DATA_PATH, eventNumber);
#else
	pathAndFilename = GetEventFilenameAndPath(eventNumber, ER_DATA_FILE_TYPE);
#endif
	eventFile = open(pathAndFilename, O_RDONLY);

	// Verify file ID
	if (eventFile == -1)
	{
		DisplayFileNotFound(pathAndFilename);
	}
	else
	{
		file_seek(dataOffset, FS_SEEK_SET);
		ReadWithSizeFix(eventFile, dataBuffer, dataSize);

		g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
		close(eventFile);
	}
#endif
	ReleaseSpi1MutexLock();
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void CacheEventDataToRam(uint16 eventNumber, uint8* dataBuffer, uint32 dataOffset, uint32 dataSize)
{
	char* pathAndFilename;
	int eventFile;

	if (g_fileAccessLock != AVAILABLE)
	{
		ReportFileSystemAccessProblem("Cache event data");
	}
	else // (g_fileAccessLock == AVAILABLE)
	{
		GetSpi1MutexLock(SDMMC_LOCK);

#if 0 /* old hw */
		nav_select(FS_NAV_ID_DEFAULT);

#if 0 /* Original */
		sprintf(fileName, "%sEvt%d.ns8", EVENTS_PATH, eventNumber);
#else
		pathAndFilename = GetEventFilenameAndPath(eventNumber, EVENT_FILE_TYPE);
#endif

		eventFile = open(pathAndFilename, O_RDONLY);

		// Verify file ID
		if (eventFile == -1)
		{
			DisplayFileNotFound(pathAndFilename);
		}
		// Verify file is big enough
		else if (fsaccess_file_get_size(eventFile) < sizeof(EVENT_HEADER_STRUCT))
		{
			DisplayFileCorrupt(pathAndFilename);
		}
		else
		{
			file_seek((sizeof(EVT_RECORD) + dataOffset), FS_SEEK_SET);
			ReadWithSizeFix(eventFile, dataBuffer, dataSize);

			g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
			close(eventFile);
		}
#endif
		ReleaseSpi1MutexLock();
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0
#define VERIFY_CHUNK_SIZE	1024
void VerifyCacheEventToRam(uint16 eventNumber, char* subMessage)
{
	uint16 i = 0;
	uint32 remainingData = 0;
	uint32 index = 0;
	uint32 readCount = 0;
	uint16 errorCount = 0;
	uint8* dataPtr = (uint8*)&g_eventDataBuffer[0];
	uint8* verifyPtr = (uint8*)&g_spareBuffer[(SPARE_BUFFER_SIZE - VERIFY_CHUNK_SIZE - 1)];
	int eventFile;
	char fileName[50];

	sprintf(fileName, "%sEvt%d.ns8", EVENTS_PATH, eventNumber);

	nav_select(FS_NAV_ID_DEFAULT);

	eventFile = open(fileName, O_RDONLY);

	file_seek(0, FS_SEEK_SET);
	remainingData = nav_file_lgt();

	debug("Verify Cache data bytes: %d\r\n", remainingData);

	while (remainingData)
	{
		if (remainingData > VERIFY_CHUNK_SIZE)
		{
			readCount = read(eventFile, verifyPtr, VERIFY_CHUNK_SIZE);

			if (readCount != VERIFY_CHUNK_SIZE)
			{
				debugErr("CacheEventToRam Read Data size incorrect (%d)\r\n", readCount);
				close (eventFile);
				return;
			}

			for (i = 0; i < VERIFY_CHUNK_SIZE; i++)
			{
				if (verifyPtr[i] != dataPtr[index])
				{
					debugErr("CacheEventToRam verification failed: %x != %x, index: %d\r\n", verifyPtr[i], dataPtr[index], index);

					if (errorCount++ > 1024) { debugErr("Too many errors, bailing\r\n\r\n"); close (eventFile); return; }
				}

				index++;
			}

			remainingData -= VERIFY_CHUNK_SIZE;
		}
		else // Remaining data size is less than the access limit
		{
			readCount = read(eventFile, &verifyPtr[0], remainingData);

			if (readCount != remainingData)
			{
				debugErr("CacheEventToRam Read Data size incorrect (%d)\r\n", readCount);
				close (eventFile);
				return;
			}

			for (i = 0; i < remainingData; i++)
			{
				if (verifyPtr[(index % VERIFY_CHUNK_SIZE)] != dataPtr[index])
				{
					debugErr("CacheEventToRam verification failed: %x != %x, index: %d\r\n", verifyPtr[(index % 8192)], dataPtr[index], index);

					if (errorCount++ > 1024) { debugErr("Too many errors, bailing\r\n\r\n"); close (eventFile); return; }
				}

				index++;
			}

			remainingData = 0;
		}
	}

	if (errorCount)
	{
		debugErr("Event %d cache verify failed (%s SEND)\r\n", eventNumber, subMessage);
	}

	close (eventFile);
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 CacheEventToRam(uint16 eventNumber, EVT_RECORD* eventRecordPtr)
{
	char* pathAndFilename;
	int eventFile;

	if (g_fileAccessLock != AVAILABLE)
	{
		ReportFileSystemAccessProblem("Cache event");
		return EVENT_CACHE_FAILURE;
	}
	else // (g_fileAccessLock == AVAILABLE)
	{
		GetSpi1MutexLock(SDMMC_LOCK);

#if 0 /* old hw */
		nav_select(FS_NAV_ID_DEFAULT);

#if 0 /* Original */
		sprintf(fileName, "%sEvt%d.ns8", EVENTS_PATH, eventNumber);
#else
		pathAndFilename = GetEventFilenameAndPath(eventNumber, EVENT_FILE_TYPE);
#endif

		eventFile = open(pathAndFilename, O_RDONLY);

		// Verify file ID
		if (eventFile == -1)
		{
			DisplayFileNotFound(pathAndFilename);

			ReleaseSpi1MutexLock();
			return EVENT_CACHE_FAILURE;
		}
		// Verify file is big enough
		else if (fsaccess_file_get_size(eventFile) < sizeof(EVT_RECORD))
		{
			DisplayFileCorrupt(pathAndFilename);

			ReleaseSpi1MutexLock();
			return EVENT_CACHE_FAILURE;
		}
		else
		{
			ReadWithSizeFix(eventFile, (uint8*)eventRecordPtr, sizeof(EVT_RECORD));
			ReadWithSizeFix(eventFile, (uint8*)&g_eventDataBuffer[0], (fsaccess_file_get_size(eventFile) - sizeof(EVT_RECORD)));
			g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
			close(eventFile);
		}
#endif
		ReleaseSpi1MutexLock();

		return EVENT_CACHE_SUCCESS;
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
BOOLEAN CheckValidEventFile(uint16 eventNumber)
{
	EVENT_HEADER_STRUCT fileEventHeader;
	BOOLEAN validFile = NO;
	char* pathAndFilename;
	int eventFile;

	if (g_fileAccessLock != AVAILABLE)
	{
		ReportFileSystemAccessProblem("Check valid event");
	}
	else // (g_fileAccessLock == AVAILABLE)
	{
		GetSpi1MutexLock(SDMMC_LOCK);

#if 0 /* old hw */
		nav_select(FS_NAV_ID_DEFAULT);

#if 0 /* Original */
		sprintf(fileName, "%sEvt%d.ns8", EVENTS_PATH, eventNumber);
#else
		pathAndFilename = GetEventFilenameAndPath(eventNumber, EVENT_FILE_TYPE);
#endif
		eventFile = open(pathAndFilename, O_RDONLY);

		// Verify file ID
		if (eventFile == -1)
		{
			DisplayFileNotFound(pathAndFilename);
		}
		// Verify file is big enough
		else if (fsaccess_file_get_size(eventFile) < sizeof(EVENT_HEADER_STRUCT))
		{
			DisplayFileCorrupt(pathAndFilename);
		}
		else
		{
			ReadWithSizeFix(eventFile, (uint8*)&fileEventHeader, sizeof(EVENT_HEADER_STRUCT));

			// If we find the EVENT_RECORD_START_FLAG followed by the encodeFlag2, then assume this is the start of an event
			if ((fileEventHeader.startFlag == EVENT_RECORD_START_FLAG) &&
				((fileEventHeader.recordVersion & EVENT_MAJOR_VERSION_MASK) == (EVENT_RECORD_VERSION & EVENT_MAJOR_VERSION_MASK)) &&
				(fileEventHeader.headerLength == sizeof(EVENT_HEADER_STRUCT)))
			{
				//debug("Found Valid Event File: %s", fileName);

				validFile = YES;
			}

			g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
			close(eventFile);
		}
#endif
		ReleaseSpi1MutexLock();
	}

	return(validFile);
}

#define SD_MMC_SPI_NPCS	2
///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ReInitSdCardAndFat32(void)
{
	// Power off the SD card
	PowerControl(SD_POWER, OFF);

	// Wait for power to propagate
	SoftUsecWait(10 * SOFT_MSECS);

	// Power on the SD Card
	PowerControl(SD_POWER, ON);

	// Wait for power to propagate
	SoftUsecWait(10 * SOFT_MSECS);

#if 0 /* old hw */
	// Check if SD Detect pin 
	if (gpio_get_pin_value(AVR32_PIN_PA02) == SDMMC_CARD_DETECTED)
	{
		GetSpi1MutexLock(SDMMC_LOCK);

		spi_selectChip(&AVR32_SPI1, SD_MMC_SPI_NPCS);
		sd_mmc_spi_internal_init();
		spi_unselectChip(&AVR32_SPI1, SD_MMC_SPI_NPCS);

		// Init the NAV and select the SD MMC Card
		nav_reset();
		nav_select(FS_NAV_ID_DEFAULT);

		// Check if the drive select was successful
		if (nav_drive_set(0) == TRUE)
		{
			// Check if the partition mount was unsuccessful (otherwise passes through without an error case)
			if (nav_partition_mount() == FALSE)
			{
				// Error case
				debugErr("FAT32 SD Card mount failed\r\n");
				OverlayMessage(getLangText(ERROR_TEXT), getLangText(FAILED_TO_MOUNT_SD_CARD_TEXT), 0);
			}
		}
		else // Error case
		{
			debugErr("FAT32 SD Card drive select failed\r\n");
			OverlayMessage(getLangText(ERROR_TEXT), getLangText(FAILED_TO_SELECT_SD_CARD_DRIVE_TEXT), 0);
		}

		ReleaseSpi1MutexLock();
	}
	else
	{
		debugErr("\n\nSD Card not detected\r\n");
	}
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void PowerDownSDCard(void)
{
	debugRaw("\n Powering down SD Card... ");

	// Power off the SD card
	PowerControl(SD_POWER, OFF);

	// Wait for power to propagate
	SoftUsecWait(10 * SOFT_MSECS);

	debugRaw("done.\r\n");
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void PowerUpSDCardAndInitFat32(void)
{
	debugRaw("\nPowering up SD Card and ReInit Fat32... \r\n");

	// Power on the SD Card
	PowerControl(SD_POWER, ON);

	// Wait for power to propagate
	SoftUsecWait(10 * SOFT_MSECS);

#if 0 /* old hw */
	// Check if SD Detect pin 
	if (gpio_get_pin_value(AVR32_PIN_PA02) == SDMMC_CARD_DETECTED)
	{
		GetSpi1MutexLock(SDMMC_LOCK);

		spi_selectChip(&AVR32_SPI1, SD_MMC_SPI_NPCS);
		sd_mmc_spi_internal_init();
		spi_unselectChip(&AVR32_SPI1, SD_MMC_SPI_NPCS);

		// Init the NAV and select the SD MMC Card
		nav_reset();
		nav_select(FS_NAV_ID_DEFAULT);

		// Check if the drive select was successful
		if (nav_drive_set(0) == TRUE)
		{
			// Check if the partition mount was unsuccessful (otherwise passes through without an error case)
			if (nav_partition_mount() == FALSE)
			{
				// Error case
				debugErr("FAT32 SD Card mount failed\r\n");
				OverlayMessage(getLangText(ERROR_TEXT), getLangText(FAILED_TO_MOUNT_SD_CARD_TEXT), 0);
			}
		}
		else // Error case
		{
			debugErr("FAT32 SD Card drive select failed\r\n");
			OverlayMessage(getLangText(ERROR_TEXT), getLangText(FAILED_TO_SELECT_SD_CARD_DRIVE_TEXT), 0);
		}

		ReleaseSpi1MutexLock();
	}
	else
	{
		debugErr("\n\nSD Card not detected\r\n");
	}
#endif
	debugRaw("done.\r\n");
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetFileDateTimestamp(uint8 option)
{
	char dateTimeBuffer[20];
	DATE_TIME_STRUCT dateTime;

	// Set the creation date and time
	dateTime = GetCurrentTime();

	// ASCII string date/time format to write: "YYYYMMDDHHMMSSMS" = year, month, day, hour, min, sec, ms
	sprintf((char*)&dateTimeBuffer[0], "%04d%02d%02d%02d%02d%02d%02d", (dateTime.year + 2000), dateTime.month, dateTime.day, dateTime.hour, dateTime.min, dateTime.sec, dateTime.hundredths);

#if 0 /* old hw */
	if (option == FS_DATE_CREATION)
	{
		nav_file_dateset((char*)&dateTimeBuffer[0], FS_DATE_CREATION);
	}
	else
	{
		nav_file_dateset((char*)&dateTimeBuffer[0], FS_DATE_LAST_WRITE);
	}
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void RemoveEventFile(uint16 eventNumber)
{
	uint16 lowerBounds, upperBounds;

	GetSubDirLowerAndUpperBounds(eventNumber, &lowerBounds, &upperBounds);

	sprintf((char*)g_spareBuffer, "%s %s %d (%s, %s)", getLangText(REMOVING_TEXT), EVT_FILE, eventNumber, getLangText(ABOVE_EVT_CAP_TEXT), getLangText(OLDEST_FIRST_TEXT));
	OverlayMessage(getLangText(STATUS_TEXT), (char*)g_spareBuffer, 0);

	//-------------------------------------------------------------------------
	// Remove Event data file (should exist)
	//-------------------------------------------------------------------------
	sprintf(g_spareFileName, "%s%s %d-%d\\%s%d.ns8", EVENTS_PATH, EVTS_SUB_DIR, lowerBounds, upperBounds, EVT_FILE, eventNumber);

	// Check if the Event data file is in the normal location
#if 0 /* old hw */
	if (nav_setcwd(g_spareFileName, TRUE, FALSE) == TRUE)
	{
		// Delete the file and check if successful
		if (nav_file_del(TRUE) == FALSE)
		{
			sprintf((char*)g_spareBuffer, "%s %s", getLangText(UNABLE_TO_DELETE_TEXT), getLangText(EVENT_TEXT));
			OverlayMessage(g_spareFileName, (char*)g_spareBuffer, 3 * SOFT_SECS);
		}
	}
	else // Check if the Event data file is in the legacy location
	{
		sprintf(g_spareFileName, "%s%s%d.ns8", EVENTS_PATH, EVT_FILE, eventNumber);

		if (nav_setcwd(g_spareFileName, TRUE, FALSE) == TRUE)
		{
			// Delete the file and check if successful
			if (nav_file_del(TRUE) == FALSE)
			{
				sprintf((char*)g_spareBuffer, "%s %s", getLangText(UNABLE_TO_DELETE_TEXT), getLangText(EVENT_TEXT));
				OverlayMessage(g_spareFileName, (char*)g_spareBuffer, 3 * SOFT_SECS);
			}
		}
	}
#endif
	//-------------------------------------------------------------------------
	// Remove compressed data file (if it exists)
	//-------------------------------------------------------------------------
	sprintf(g_spareFileName, "%s%s %d-%d\\%s%d.nsD", ER_DATA_PATH, EVTS_SUB_DIR, lowerBounds, upperBounds, EVT_FILE, eventNumber);

#if 0 /* old hw */
	// Check if the Compressed Event data file is in the normal location
	if (nav_setcwd(g_spareFileName, TRUE, FALSE) == TRUE)
	{
		// Delete the file and check if successful
		if (nav_file_del(TRUE) == FALSE)
		{
			sprintf((char*)g_spareBuffer, "%s %s", getLangText(UNABLE_TO_DELETE_TEXT), getLangText(EVENT_TEXT));
			OverlayMessage(g_spareFileName, (char*)g_spareBuffer, 3 * SOFT_SECS);
		}
	}
	else // Check if the Event data file is in the legacy location
	{
		sprintf(g_spareFileName, "%s%s%d.nsD", ER_DATA_PATH, EVT_FILE, eventNumber);

		if (nav_setcwd(g_spareFileName, TRUE, FALSE) == TRUE)
		{
			// Delete the file and check if successful
			if (nav_file_del(TRUE) == FALSE)
			{
				sprintf((char*)g_spareBuffer, "%s %s", getLangText(UNABLE_TO_DELETE_TEXT), getLangText(EVENT_TEXT));
				OverlayMessage(g_spareFileName, (char*)g_spareBuffer, 3 * SOFT_SECS);
			}
		}
	}
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void RemoveExcessEventsAboveCap(void)
{
	uint16 loopCount = 0;
	uint16 currentEventIndex = g_eventNumberCacheOldestIndex;

	while (g_eventNumberCacheValidEntries >= g_unitConfig.storedEventLimit)
	{
		if (g_eventNumberCache[currentEventIndex] == EVENT_REFERENCE_VALID)
		{
			// Remove event file
			RemoveEventFile(currentEventIndex);

			// Event number cache adjustments
			g_eventNumberCache[currentEventIndex] = NO_EVENT_FILE;
			g_eventNumberCacheOldestIndex++;
			g_eventNumberCacheValidEntries--;
		}

		currentEventIndex++;
		loopCount++;

		// Check if a full cycled around the event list has occurred
		if (loopCount == EVENT_LIST_CACHE_ENTRIES_LAST_INDEX)
		{
			break;
		}
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int GetEventFileHandle(uint16 newFileEventNumber, EVENT_FILE_OPTION option)
{
	int fileHandle;
	int fileOption;
	uint16 lowerBounds, upperBounds;

#if 1 /* New feature */
	if ((option == CREATE_EVENT_FILE) && (g_unitConfig.storedEventsCapMode == ENABLED) && (g_eventNumberCacheValidEntries >= g_unitConfig.storedEventLimit))
	{
		RemoveExcessEventsAboveCap();
	}
#endif

	GetSubDirLowerAndUpperBounds(newFileEventNumber, &lowerBounds, &upperBounds);

#if 0 /* Original */
	sprintf(g_spareFileName, "%sEvt%d.ns8", EVENTS_PATH, newFileEventNumber);
#else
	sprintf(g_spareFileName, "%s%s %d-%d\\%s%d.ns8", EVENTS_PATH, EVTS_SUB_DIR, lowerBounds, upperBounds, EVT_FILE, newFileEventNumber);
#endif

	// Set the file option flags for the file process
	switch (option)
	{
#if 0 /* old hw */
		case CREATE_EVENT_FILE: debug("File to create: %s\r\n", g_spareFileName);
			fileOption = (O_CREAT | O_WRONLY); break;
		case READ_EVENT_FILE: debug("File to read: %s\r\n", g_spareFileName);
			fileOption = O_RDONLY; break;
		case APPEND_EVENT_FILE: debug("File to append: %s\r\n", g_spareFileName);
			fileOption = O_APPEND; break;
		case OVERWRITE_EVENT_FILE: debug("File to overwrite: %s\r\n", g_spareFileName);
			fileOption = O_RDWR; break;
#endif
	}

#if 0 /* old hw */
	fileHandle = open(g_spareFileName, fileOption);

	// Check if trying to create a new event file and one already exists
	if ((fileHandle == -1) && (option == CREATE_EVENT_FILE))
	{
		// Delete the existing file and check if the operation was successful
		if (nav_file_del(TRUE) == TRUE)
		{
			fileHandle = open(g_spareFileName, fileOption);
		}
	}
#endif

	if (option == CREATE_EVENT_FILE)
	{
#if 0 /* old hw */
		SetFileDateTimestamp(FS_DATE_CREATION);
#endif
	}

	return (fileHandle);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int GetERDataFileHandle(uint16 newFileEventNumber, EVENT_FILE_OPTION option)
{
	int fileHandle;
	int fileOption;
	uint16 lowerBounds, upperBounds;

	GetSubDirLowerAndUpperBounds(newFileEventNumber, &lowerBounds, &upperBounds);

#if 0 /* Original */
	sprintf(g_spareFileName, "%sEvt%d.nsD", ER_DATA_PATH, newFileEventNumber);
#else
	sprintf(g_spareFileName, "%s%s %d-%d\\%s%d.nsD", ER_DATA_PATH, EVTS_SUB_DIR, lowerBounds, upperBounds, EVT_FILE, newFileEventNumber);
#endif

	// Set the file option flags for the file process
	switch (option)
	{
#if 0 /* old hw */
		case CREATE_EVENT_FILE: debug("File to create: %s\r\n", g_spareFileName);
			fileOption = (O_CREAT | O_WRONLY); break;
		case READ_EVENT_FILE: debug("File to read: %s\r\n", g_spareFileName);
			fileOption = O_RDONLY; break;
		case APPEND_EVENT_FILE: debug("File to append: %s\r\n", g_spareFileName);
			fileOption = O_APPEND; break;
		case OVERWRITE_EVENT_FILE: debug("File to overwrite: %s\r\n", g_spareFileName);
			fileOption = O_RDWR; break;
#endif
	}

#if 0 /* old hw */
	fileHandle = open(g_spareFileName, fileOption);

	// Check if trying to create a new event file and one already exists
	if ((fileHandle == -1) && (option == CREATE_EVENT_FILE))
	{
		// Delete the existing file and check if the operation was successful
		if (nav_file_del(TRUE) == TRUE)
		{
			fileHandle = open(g_spareFileName, fileOption);
		}
	}
#endif

	if (option == CREATE_EVENT_FILE)
	{
#if 0 /* old hw */
		SetFileDateTimestamp(FS_DATE_CREATION);
#endif
	}

	return (fileHandle);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0 /* old hw */
inline void AdjustSampleForBitAccuracy(void)
#else
void AdjustSampleForBitAccuracy(void)
#endif
{
	// Shift the sample data to adjust for the lower accuracy
	*(g_currentEventSamplePtr) >>= g_bitShiftForAccuracy;
	*(g_currentEventSamplePtr + 1) >>= g_bitShiftForAccuracy;
	*(g_currentEventSamplePtr + 2) >>= g_bitShiftForAccuracy;
	*(g_currentEventSamplePtr + 3) >>= g_bitShiftForAccuracy;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void CompleteRamEventSummary(void)
{
	//--------------------------------
	// Complete the Summary (used for Wave, Cal, Combo-Wave
	//--------------------------------

	if (g_pendingEventRecord.summary.mode == COMBO_MODE)
	{
		g_pendingEventRecord.summary.captured.comboEventsRecordedDuringSession = (g_pendingEventRecord.summary.eventNumber - g_pendingBargraphRecord.summary.eventNumber);
		g_pendingEventRecord.summary.captured.comboEventsRecordedStartNumber = (g_pendingBargraphRecord.summary.eventNumber + 1);
		g_pendingEventRecord.summary.captured.comboEventsRecordedEndNumber = g_pendingEventRecord.summary.eventNumber;
		g_pendingEventRecord.summary.captured.comboBargraphEventNumberLink = g_pendingBargraphRecord.summary.eventNumber;
	}

	debug("Newly stored peaks: a:%04x r:%04x v:%04x t:%04x\r\n", g_pendingEventRecord.summary.calculated.a.peak, g_pendingEventRecord.summary.calculated.r.peak,
			g_pendingEventRecord.summary.calculated.v.peak,	g_pendingEventRecord.summary.calculated.t.peak);

	debug("Newly stored freq: a:%d r:%d v:%d t:%d\r\n", g_pendingEventRecord.summary.calculated.a.frequency, g_pendingEventRecord.summary.calculated.r.frequency,
			g_pendingEventRecord.summary.calculated.v.frequency, g_pendingEventRecord.summary.calculated.t.frequency);

	// Calculate Displacement as PPV/(2 * PI * Freq) with 1000000 to shift to keep accuracy and the 10 to adjust the frequency
	// R Channel
	if (g_pendingEventRecord.summary.calculated.r.frequency != 0)
	{
		g_pendingEventRecord.summary.calculated.r.displacement = (uint32)(g_pendingEventRecord.summary.calculated.r.peak * 1000000 / 2 / PI / g_pendingEventRecord.summary.calculated.r.frequency * 10);
	}
	else { g_pendingEventRecord.summary.calculated.r.displacement = 0; }

	// V Channel
	if (g_pendingEventRecord.summary.calculated.v.frequency != 0)
	{
		g_pendingEventRecord.summary.calculated.v.displacement = (uint32)(g_pendingEventRecord.summary.calculated.v.peak * 1000000 / 2 / PI / g_pendingEventRecord.summary.calculated.v.frequency * 10);
	}
	else { g_pendingEventRecord.summary.calculated.v.displacement = 0; }

	// T Channel
	if (g_pendingEventRecord.summary.calculated.t.frequency != 0)
	{
		g_pendingEventRecord.summary.calculated.t.displacement = (uint32)(g_pendingEventRecord.summary.calculated.t.peak * 1000000 / 2 / PI / g_pendingEventRecord.summary.calculated.t.frequency * 10);
	}
	else { g_pendingEventRecord.summary.calculated.t.displacement = 0; }

	// A Channel (No Displacement)
	g_pendingEventRecord.summary.calculated.a.displacement = 0;

	// Calculate Peak Acceleration as (2 * PI * PPV * Freq) / 1G, where 1G = 386.4in/sec2 or 9814.6 mm/sec2, using 1000 to shift to keep accuracy
	// The divide by 10 at the end to adjust the frequency, since freq stored as freq * 10
	// Not dividing by 1G at this time. Before displaying Peak Acceleration, 1G will need to be divided out
	g_pendingEventRecord.summary.calculated.r.acceleration = (uint32)(g_pendingEventRecord.summary.calculated.r.peak * 1000 * 2 * PI * g_pendingEventRecord.summary.calculated.r.frequency / 10);
	g_pendingEventRecord.summary.calculated.v.acceleration = (uint32)(g_pendingEventRecord.summary.calculated.v.peak * 1000 * 2 * PI * g_pendingEventRecord.summary.calculated.v.frequency / 10);
	g_pendingEventRecord.summary.calculated.t.acceleration = (uint32)(g_pendingEventRecord.summary.calculated.t.peak * 1000 * 2 * PI * g_pendingEventRecord.summary.calculated.t.frequency / 10);

	// A Channel (No Acceleration)
	g_pendingEventRecord.summary.calculated.a.acceleration = 0;

	//--------------------------------
	g_pendingEventRecord.header.summaryChecksum = 0;
	g_pendingEventRecord.header.dataChecksum = 0;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void CalculateCurrentConfigEventSizesRemaining(void)
{
	uint32 waveSize;
	uint32 barSize;
	uint32 manualCalSize;

	// Waveform size based on current config settings
	waveSize = sizeof(EVT_RECORD) + (CALIBRATION_NUMBER_OF_SAMPLES * NUMBER_OF_CHANNELS_DEFAULT * BYTES_PER_CHANNEL) +
				(uint32)((g_triggerRecord.trec.sample_rate / g_unitConfig.pretrigBufferDivider) * NUMBER_OF_CHANNELS_DEFAULT * BYTES_PER_CHANNEL) +
				(uint32)(g_triggerRecord.trec.sample_rate * g_triggerRecord.trec.record_time * NUMBER_OF_CHANNELS_DEFAULT * BYTES_PER_CHANNEL);

	// Bargraph size per hour based on current config settings
	barSize = (uint32)(((3600 * NUMBER_OF_CHANNELS_DEFAULT * BYTES_PER_CHANNEL) / g_triggerRecord.bgrec.barInterval) +
				(sizeof(EVT_RECORD) / 24) + ((3600 * sizeof(CALCULATED_DATA_STRUCT)) / g_triggerRecord.bgrec.summaryInterval));

	// Manual cal size is event record plus 100 cal samples
	manualCalSize = sizeof(EVT_RECORD) + (CALIBRATION_NUMBER_OF_SAMPLES * NUMBER_OF_CHANNELS_DEFAULT * BYTES_PER_CHANNEL);

	// Check if optioned (the default) for saving the extra compressed data file
	if (g_unitConfig.saveCompressedData == SAVE_EXTRA_FILE_COMPRESSED_DATA)
	{
		// Account for the extra compressed data file, typically 3-to-1 compression but accounting for the lower end (but not worst case) is typically 2-to-1 (50% more)
		waveSize = ((waveSize * 3) / 2);
		barSize = ((barSize * 3) / 2);
		manualCalSize = ((manualCalSize * 3) / 2);
	}

	if (g_sdCardUsageStats.sizeFree > RESERVED_FILESYSTEM_SIZE_IN_BYTES)
	{
		g_sdCardUsageStats.waveEventsLeft = (uint16)((g_sdCardUsageStats.sizeFree - RESERVED_FILESYSTEM_SIZE_IN_BYTES) / waveSize);
		g_sdCardUsageStats.barHoursLeft = (uint16)((g_sdCardUsageStats.sizeFree - RESERVED_FILESYSTEM_SIZE_IN_BYTES) / barSize);
		g_sdCardUsageStats.manualCalsLeft = (uint16)((g_sdCardUsageStats.sizeFree - RESERVED_FILESYSTEM_SIZE_IN_BYTES) / manualCalSize);
	}
	else
	{
		g_sdCardUsageStats.waveEventsLeft = (uint16)(0);
		g_sdCardUsageStats.barHoursLeft = (uint16)(0);
		g_sdCardUsageStats.manualCalsLeft = (uint16)(0);
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void InitSDCardUsageStats(void)
{
#if 0 /* old hw */
#if NS8100_ORIGINAL_PROTOTYPE
	sd_mmc_spi_get_capacity();

	g_sdCardUsageStats.sizeUsed = 0;
	g_sdCardUsageStats.sizeFree = capacity - g_sdCardUsageStats.sizeUsed;
	g_sdCardUsageStats.percentUsed = (uint8)((g_sdCardUsageStats.sizeUsed * 100) / capacity);
	g_sdCardUsageStats.percentFree = (uint8)(100 - g_sdCardUsageStats.percentUsed);
#else /* (NS8100_ALPHA_PROTOTYPE || NS8100_BETA_PROTOTYPE) */
	nav_select(FS_NAV_ID_DEFAULT);
	nav_drive_set(0);

	if (!nav_partition_mount()) // Mount drive
	{
		debugErr("SD MMC Card: Unable to mount volume\r\n");
	}

	g_sdCardUsageStats.clusterSizeInBytes = (fs_g_nav.u8_BPB_SecPerClus * SECTOR_SIZE_IN_BYTES);
	g_sdCardUsageStats.sizeFree = (nav_partition_freespace() * SECTOR_SIZE_IN_BYTES);
	g_sdCardUsageStats.sizeUsed = (nav_partition_space() * SECTOR_SIZE_IN_BYTES) - g_sdCardUsageStats.sizeFree;
	g_sdCardUsageStats.percentFree = (uint8)(nav_partition_freespace_percent());
	g_sdCardUsageStats.percentUsed = (uint8)(100 - g_sdCardUsageStats.percentFree);
#endif
#endif
	CalculateCurrentConfigEventSizesRemaining();

	// Reset tracking size count
	s_addedSizeToSDCard = 0;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void GetSDCardUsageStats(void)
{
#if NS8100_ORIGINAL_PROTOTYPE
	sd_mmc_spi_get_capacity();

	g_sdCardUsageStats.sizeUsed = 0;
	g_sdCardUsageStats.sizeFree = capacity - g_sdCardUsageStats.sizeUsed;
	g_sdCardUsageStats.percentUsed = (uint8)((g_sdCardUsageStats.sizeUsed * 100) / capacity);
	g_sdCardUsageStats.percentFree = (uint8)(100 - g_sdCardUsageStats.percentUsed);
#else /* (NS8100_ALPHA_PROTOTYPE || NS8100_BETA_PROTOTYPE) */
#if 0 /* old hw */
	g_sdCardUsageStats.clusterSizeInBytes = (fs_g_nav.u8_BPB_SecPerClus * SECTOR_SIZE_IN_BYTES);
	g_sdCardUsageStats.sizeFree = (nav_partition_freespace() * SECTOR_SIZE_IN_BYTES);
	g_sdCardUsageStats.sizeUsed = (nav_partition_space() * SECTOR_SIZE_IN_BYTES) - g_sdCardUsageStats.sizeFree;
	g_sdCardUsageStats.percentFree = (uint8)(nav_partition_freespace_percent());
	g_sdCardUsageStats.percentUsed = (uint8)(100 - g_sdCardUsageStats.percentFree);
#endif
#endif

	CalculateCurrentConfigEventSizesRemaining();

	// Reset tracking size count
	s_addedSizeToSDCard = 0;
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void UpdateSDCardUsageStats(uint32 removeSize)
{
	uint32 removeSizeByAllocationUnit;

	// Adjust size down to a allocation unit (cluster) boundary
	removeSizeByAllocationUnit = ((removeSize / g_sdCardUsageStats.clusterSizeInBytes) * g_sdCardUsageStats.clusterSizeInBytes);

	// Check if remove size doesn't exactly match the allocation unit (cluster) size
	if (removeSize % g_sdCardUsageStats.clusterSizeInBytes)
	{
		// Remainder size fills up a partial allocation unit (cluster) but need to count the entire allocation unit (cluster)
		removeSizeByAllocationUnit += g_sdCardUsageStats.clusterSizeInBytes;
	}

	s_addedSizeToSDCard += removeSizeByAllocationUnit;

	// Check if the current running added size is above 100 MB or if the remaining size is less than the system reserved space
	if ((s_addedSizeToSDCard > (100 * ONE_MEGABYTE_SIZE)) || (g_sdCardUsageStats.sizeFree < RESERVED_FILESYSTEM_SIZE_IN_BYTES))
	{
		// Reset added size in either case
		s_addedSizeToSDCard = 0;

		// Do official recalculation
		GetSDCardUsageStats();
	}
	else
	{
		g_sdCardUsageStats.sizeFree -= removeSizeByAllocationUnit;
		g_sdCardUsageStats.sizeUsed += removeSizeByAllocationUnit;
		g_sdCardUsageStats.percentFree = (uint8)((float)((float)g_sdCardUsageStats.sizeFree / (float)(g_sdCardUsageStats.sizeFree + g_sdCardUsageStats.sizeUsed)) * 100);
		g_sdCardUsageStats.percentUsed = (uint8)(100 - g_sdCardUsageStats.percentFree);

		CalculateCurrentConfigEventSizesRemaining();
	}
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int ReadWithSizeFix(int file, void* bufferPtr, uint32 length)
{
	uint32 remainingByteLengthToRead = length;
	uint8* readLocationPtr = (uint8*)bufferPtr;
	int readCount = 0;

	while (remainingByteLengthToRead)
	{
		if (remainingByteLengthToRead > ATMEL_FILESYSTEM_ACCESS_LIMIT)
		{
#if 0 /* old hw */
			readCount = read(file, readLocationPtr, ATMEL_FILESYSTEM_ACCESS_LIMIT);
#endif
			if (readCount != ATMEL_FILESYSTEM_ACCESS_LIMIT)
			{
				if (readCount != -1) { debugErr("Atmel Read Data size incorrect (%d)\r\n", readCount); }
				return (-1);
			}

			remainingByteLengthToRead -= ATMEL_FILESYSTEM_ACCESS_LIMIT;
			readLocationPtr += ATMEL_FILESYSTEM_ACCESS_LIMIT;
		}
		else // Remaining data size is less than the access limit
		{
#if 0 /* old hw */
			readCount = read(file, readLocationPtr, remainingByteLengthToRead);
#endif
			if (readCount != (int)remainingByteLengthToRead)
			{
				if (readCount != -1) { debugErr("Atmel Read Data size incorrect (%d)\r\n", readCount); }
				return (-1);
			}

			remainingByteLengthToRead = 0;
		}
	}

	return (length);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int WriteWithSizeFix(int file, void* bufferPtr, uint32 length)
{
	uint32 remainingByteLengthToWrite = length;
	uint8* writeLocationPtr = (uint8*)bufferPtr;
	int writeCount = 0;

	while (remainingByteLengthToWrite)
	{
		if (remainingByteLengthToWrite > ATMEL_FILESYSTEM_ACCESS_LIMIT)
		{
#if 0 /* old hw */
			writeCount = write(file, writeLocationPtr, ATMEL_FILESYSTEM_ACCESS_LIMIT);
#endif
			if (writeCount != ATMEL_FILESYSTEM_ACCESS_LIMIT)
			{
				if (writeCount != -1) { debugErr("Atmel Read Data size incorrect (%d)\r\n", writeCount); }
				return (-1);
			}

			remainingByteLengthToWrite -= ATMEL_FILESYSTEM_ACCESS_LIMIT;
			writeLocationPtr += ATMEL_FILESYSTEM_ACCESS_LIMIT;
		}
		else // Remaining data size is less than the access limit
		{
#if 0 /* old hw */
			writeCount = write(file, writeLocationPtr, remainingByteLengthToWrite);
#endif
			if (writeCount != (int)remainingByteLengthToWrite)
			{
				if (writeCount != -1) { debugErr("Atmel Read Data size incorrect (%d)\r\n", writeCount); }
				return (-1);
			}

			remainingByteLengthToWrite = 0;
		}
	}

	return (length);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DisplayFileNotFound(char* filename)
{
	debugErr("File not found: %s\r\n", filename);
	OverlayMessage(getLangText(FILE_NOT_FOUND_TEXT), filename, 3 * SOFT_SECS);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void DisplayFileCorrupt(char* filename)
{
	debugErr("File corrupt: %s\r\n", filename);
	OverlayMessage(getLangText(FILE_CORRUPT_TEXT), filename, 3 * SOFT_SECS);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
#if 0
void SaveRemoteEventDownloadStreamToFile(uint16 eventNumber)
{
#if 1
	UNUSED(eventNumber);
#else /* Test */
	int fileHandle;
	uint32 remainingDataLength;
	uint32 bytesWritten = 0;

	// Split the buffer in half, and use the 2nd half for shadow copy
	uint8* dataPtr = (uint8*)&g_eventDataBuffer[(EVENT_BUFF_SIZE_IN_WORDS / 2)];

	// Get new event file handle
	fileHandle = GetEventFileHandle(eventNumber, CREATE_EVENT_FILE);

	if (fileHandle == -1)
	{
		debugErr("Failed to get a new file handle for the current %s event\r\n", (g_triggerRecord.opMode == WAVEFORM_MODE) ? "Waveform" : "Combo - Waveform");
	}
	else // Write the file event to the SD card
	{
		remainingDataLength = g_spareIndex;

		while (remainingDataLength)
		{
			if (remainingDataLength > WAVEFORM_FILE_WRITE_CHUNK_SIZE)
			{
				// Write the event data, containing the Pretrigger, event and cal
				bytesWritten = write(fileHandle, dataPtr, WAVEFORM_FILE_WRITE_CHUNK_SIZE);

				if (bytesWritten != WAVEFORM_FILE_WRITE_CHUNK_SIZE)
				{
					debugErr("Remote Event Download to file write size incorrect (%d)\r\n", bytesWritten);
				}

				remainingDataLength -= WAVEFORM_FILE_WRITE_CHUNK_SIZE;
				dataPtr += (WAVEFORM_FILE_WRITE_CHUNK_SIZE);
			}
			else // Remaining data size is less than the file write chunk size
			{
				// Write the event data, containing the Pretrigger, event and cal
				bytesWritten = write(fileHandle, dataPtr, remainingDataLength);

				if (bytesWritten != remainingDataLength)
				{
					debugErr("Remote Event Download to file write size incorrect (%d)\r\n", bytesWritten);
				}

				remainingDataLength = 0;
			}
		}

		SetFileDateTimestamp(FS_DATE_LAST_WRITE);

		// Done writing the event file, close the file handle
		g_testTimeSinceLastFSWrite = g_lifetimeHalfSecondTickCount;
		close(fileHandle);
		fat_cache_flush();
	}
#endif
}
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void SetNavDefault(void)
{
#if 0 /* old hw */
	nav_select(FS_NAV_ID_DEFAULT);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint8 MigrateLooseFiles(uint8 dirType)
{
	char baseDirPath[MAX_BASE_PATH_CHARS];
	char fileExtension[10];
	char* fileExtensionStartPtr;
	char originalCreationDateTimeBuffer[20];
	char originalLastWriteDateTimeBuffer[20];
	uint16 lowerBounds, upperBounds;
	uint16 duplicateCount = 0;
	uint16 eventNumber = 0;
	uint8 status = 0; //COPY_BUSY;
#if 0 /* Test */
	uint16 length;
#endif

	//=========================================================================
	// Setup based on type
	//-------------------------------------------------------------------------
	if (dirType == EVENT_FILE_TYPE)
	{
#if 0 /* Test */
		length = sprintf((char*)g_debugBuffer, "Loose Event Migration, Handling Event (Up: %lu s)\r\n", (g_lifetimeHalfSecondTickCount / 2));
		ModemPuts(g_debugBuffer, length, NO_CONVERSION);
#endif
		strcpy(baseDirPath, EVENTS_PATH);
		strcpy(fileExtension, "ns8");
	}
	else // ER_DATA_FILE_TYPE
	{
#if 0 /* Test */
		length = sprintf((char*)g_debugBuffer, "Loose Event Migration, Handling ERData (Up: %lu s)\r\n", (g_lifetimeHalfSecondTickCount / 2));
		ModemPuts(g_debugBuffer, length, NO_CONVERSION);
#endif
		strcpy(baseDirPath, ER_DATA_PATH);
		strcpy(fileExtension, "nsD");
	}

	// Select source directory in COPYFILE navigator handle
#if 0 /* old hw */
	nav_select(FS_NAV_ID_COPYFILE);
	nav_setcwd((FS_STRING)baseDirPath, TRUE, FALSE);
	nav_filelist_reset();

	//=========================================================================
	// Find loose event file to move
	//-------------------------------------------------------------------------
	while (nav_filelist_set(0, FS_FIND_NEXT))
	{
		// Check if entry is a file (in the root of the Events directory)
		if (!nav_file_isdir())
		{
			// Check if the file has the correct event file extension
			if (nav_file_checkext(fileExtension))
			{
				// Get the event number from the filename filtering for the original format
				nav_file_name((FS_STRING)g_spareFileName, 255, FS_NAME_GET, FALSE);
				eventNumber = GetEventNumberFromFilename((char*)g_spareFileName, ORIGINAL_EVENT_NAME_FORMAT);

				// Check if the event number is valid
				if (eventNumber)
				{
#if 0 /* Test */
					length = sprintf((char*)g_debugBuffer, "Loose Event Migration, Found loose event: %d (Up: %lu s)\r\n", eventNumber, (g_lifetimeHalfSecondTickCount / 2));
					ModemPuts(g_debugBuffer, length, NO_CONVERSION);
#endif
					// Get Primary location directory boundaries
					GetSubDirLowerAndUpperBounds(eventNumber, &lowerBounds, &upperBounds);

					// Get the original creation date
					nav_file_dateget((FS_STRING)originalCreationDateTimeBuffer, FS_DATE_CREATION);
					nav_file_dateget((FS_STRING)originalLastWriteDateTimeBuffer, FS_DATE_LAST_WRITE);

					// Got what we needed
					break;
				}
			}
		}
	}

	//=========================================================================
	// Move loose event file to Primary location
	//-------------------------------------------------------------------------
	if (eventNumber)
	{
#if 0 /* Test */
		length = sprintf((char*)g_debugBuffer, "Loose Event Migration, Handling %s #%d (Up: %lu s)\r\n", ((dirType == EVENT_FILE_TYPE) ? "Event" : "ERData"), eventNumber, (g_lifetimeHalfSecondTickCount / 2));
		ModemPuts(g_debugBuffer, length, NO_CONVERSION);
#endif
		// Select destination directory in USHELL navigator handle
		nav_select(FS_NAV_ID_DEFAULT);
		sprintf(g_spareFileName, "%s%s %d-%d\\", baseDirPath, EVTS_SUB_DIR, lowerBounds, upperBounds);

#if 0 /* Test */
		length = sprintf((char*)g_debugBuffer, "Loose Event Migration, Base dir: (%s) (Up: %lu s)\r\n", g_spareFileName, (g_lifetimeHalfSecondTickCount / 2));
		ModemPuts(g_debugBuffer, length, NO_CONVERSION);
#endif
		nav_setcwd((FS_STRING)g_spareFileName, TRUE, TRUE);

		sprintf(g_spareFileName, "%s%d.%s", EVT_FILE, eventNumber, fileExtension);
#if 0 /* Test */
		length = sprintf((char*)g_debugBuffer, "Loose Event Migration, Path and file: (%s) (Up: %lu s)\r\n", g_spareFileName, (g_lifetimeHalfSecondTickCount / 2));
		ModemPuts(g_debugBuffer, length, NO_CONVERSION);
#endif

		//=========================================================================
		// Start paste operation
		//-------------------------------------------------------------------------
		while ((!nav_file_paste_start((FS_STRING)g_spareFileName)) && (duplicateCount < 256))
		{
            // Check if the error code if anything but the file exists
            if (fs_g_status != FS_ERR_FILE_EXIST)
            {
	            // Error
				return (NO);
            }

			//=========================================================================
			// Duplicate file found
			//-------------------------------------------------------------------------
			// Check if this is the first duplicate match
			if (!duplicateCount)
			{
#if 0 /* Test */
				length = sprintf((char*)g_debugBuffer, "Loose Event Migration, Duplicate found for #%d (Up: %lu s)\r\n", eventNumber, (g_lifetimeHalfSecondTickCount / 2));
				ModemPuts(g_debugBuffer, length, NO_CONVERSION);
#endif
				fileExtensionStartPtr = strstr(g_spareFileName, ".");

				// Check if the '.' separator was found
				if (fileExtensionStartPtr)
				{
					// Terminate the string at the '.' separator
					*fileExtensionStartPtr = '\0';
				}

				// Copy the file name (either up to the '.' separator)
				strcpy((char*)g_spareBuffer, g_spareFileName);
			}

			sprintf((char*)g_spareFileName, "%s (%d).%s", g_spareBuffer, ++duplicateCount, fileExtension);
#if 0 /* Test */
			length = sprintf((char*)g_debugBuffer, "Loose Event Migration, New Path and file: (%s) (Up: %lu s)\r\n", g_spareFileName, (g_lifetimeHalfSecondTickCount / 2));
			ModemPuts(g_debugBuffer, length, NO_CONVERSION);
#endif
		}

		//=========================================================================
		// Continue paste operation
		//-------------------------------------------------------------------------
		if (fs_g_status != FS_ERR_FILE_EXIST)
		{
#if 0 /* Test */
			length = sprintf((char*)g_debugBuffer, "Loose Event Migration, Finish Paste: (%s) (Up: %lu s)\r\n", g_spareFileName, (g_lifetimeHalfSecondTickCount / 2));
			ModemPuts(g_debugBuffer, length, NO_CONVERSION);
#endif
			// Set file creation and last access for new copied file
			nav_file_dateset(originalCreationDateTimeBuffer, FS_DATE_CREATION);
			nav_file_dateset(originalLastWriteDateTimeBuffer, FS_DATE_LAST_WRITE);

			while (status == COPY_BUSY)
			{
				status = nav_file_paste_state(FALSE);
			}

			if (status == COPY_FINISH)
			{
				nav_select(FS_NAV_ID_COPYFILE);

				if (!nav_file_del(TRUE))
				{
					return (NO);
				}

				return (YES);
			}
		}
	}
#endif
	return (NO);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
void ValidateSummaryListFileWithEventCache(void)
{
	// Count g_eventNumberCache entires stored as EVENT_FILE_FOUND that weren't migrated to EVENT_REFERENCE_VALID
	uint16 validEventFileCount = 0;
	uint16 invalidEventFileCount = 0;
	uint16 i = 0;
	EVT_RECORD tempEventRecord;
	uint8 promptDelay = YES;

	for (i = 0; i < TOTAL_UNIQUE_EVENT_NUMBERS; i++)
	{
		if (g_eventNumberCache[i] == EVENT_FILE_FOUND)
		{
			if (promptDelay) { OverlayMessage(getLangText(STATUS_TEXT), getLangText(COMPARING_EVENTS_TO_SUMMARY_LIST_TEXT), 0); promptDelay = NO; }

			// Cache the event record
			GetEventFileRecord(i, &tempEventRecord);

			// String compare the current unit serial number against the stored event serial number up to 15 characters (which is all that can be set by the user through the edit menu)
			if (strncmp((char*)tempEventRecord.summary.version.serialNumber, (char*)g_factorySetupRecord.unitSerialNumber, 15) == 0)
			{
				// Count the valid event file
				validEventFileCount++;
			}
			else // Serial numbers do not match meaning this event was not created from this unit
			{
				// Change the event cache designation to invalid and count
				g_eventNumberCache[i] = INVALID_EVENT_FILE_FOUND;
				invalidEventFileCount++;
			}
		}
		else if (g_eventNumberCache[i] == INVALID_EVENT_FILE_FOUND)
		{
			invalidEventFileCount++;
		}
	}

	// Check if either valid or invalid event files exist
	if (validEventFileCount || invalidEventFileCount)
	{
		if (validEventFileCount)
		{
			sprintf((char*)g_spareBuffer, "%s %d %s (%s) %s", getLangText(FOUND_TEXT), validEventFileCount, getLangText(VALID_EVENTS_TEXT), getLangText(MATCH_SERIAL_TEXT), getLangText(NOT_IN_SUMMARY_TEXT));
			MessageBox(getLangText(STATUS_TEXT), (char*)g_spareBuffer, MB_OK);

			sprintf((char*)g_spareBuffer, "%s %s (%s) %s", getLangText(MERGE_TEXT), getLangText(VALID_EVENTS_TEXT), getLangText(MATCH_SERIAL_TEXT), getLangText(TO_SUMMARY_Q_TEXT));
			if (MessageBox(getLangText(STATUS_TEXT), (char*)g_spareBuffer, MB_YESNO) == MB_FIRST_CHOICE)
			{
				for (i = 0; i < TOTAL_UNIQUE_EVENT_NUMBERS; i++)
				{
					if (g_eventNumberCache[i] == EVENT_FILE_FOUND)
					{
						if (promptDelay) { OverlayMessage(getLangText(STATUS_TEXT), getLangText(COMPARING_EVENTS_TO_SUMMARY_LIST_TEXT), 0); promptDelay = NO; }

						GetEventFileRecord(i, &tempEventRecord);

						GetSpi1MutexLock(SDMMC_LOCK);
#if 0 /* old hw */
						nav_select(FS_NAV_ID_DEFAULT);
#endif
						AddEventToSummaryList(&tempEventRecord);
						ReleaseSpi1MutexLock();
						g_eventNumberCache[i] = EVENT_REFERENCE_VALID;
						validEventFileCount--;
					}
				}

				// Make sure contents are written to file
#if 0 /* old hw */
				fat_cache_flush();
#endif
			}
		}

		if (invalidEventFileCount)
		{
			sprintf((char*)g_spareBuffer, "%s %d %s (%s)", getLangText(FOUND_TEXT), invalidEventFileCount, getLangText(INVALID_EVENTS_TEXT), getLangText(WRONG_SERIAL_TEXT));
			MessageBox(getLangText(STATUS_TEXT), (char*)g_spareBuffer, MB_OK);

#if 0 /* Used for testing, want to prevent unauthorized event merging for events not associated with the unit serial number */
			sprintf((char*)g_spareBuffer, "%s %s (%s) %s", getLangText(MERGE_TEXT), getLangText(INVALID_EVENTS_TEXT), getLangText(WRONG_SERIAL_TEXT), getLangText(TO_SUMMARY_Q_TEXT));
			if (MessageBox(getLangText(STATUS_TEXT), (char*)g_spareBuffer, MB_YESNO) == MB_FIRST_CHOICE)
			{
				for (i = 0; i < TOTAL_UNIQUE_EVENT_NUMBERS; i++)
				{
					if (g_eventNumberCache[i] == INVALID_EVENT_FILE_FOUND)
					{
						GetEventFileRecord(i, &tempEventRecord);

						GetSpi1MutexLock(SDMMC_LOCK);
						nav_select(FS_NAV_ID_DEFAULT);
						AddEventToSummaryList(&tempEventRecord);
						ReleaseSpi1MutexLock();
						g_eventNumberCache[i] = EVENT_REFERENCE_VALID;
						invalidEventFileCount--;
					}
				}
			}
#endif
		}
	}
	else
	{
		sprintf((char*)g_spareBuffer, "%s (%s)", getLangText(SUMMARY_FILE_MATCHES_EVENT_STORAGE_TEXT), getLangText(NO_STRAY_EVENTS_TEXT));
		MessageBox(getLangText(STATUS_TEXT), (char*)g_spareBuffer, MB_OK);
	}
}
