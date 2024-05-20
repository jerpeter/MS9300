/******************************************************************************
 * Copyright (C) 2023 Maxim Integrated Products, Inc., All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Maxim Integrated
 * Products, Inc. shall not be used except as stated in the Maxim Integrated
 * Products, Inc. Branding Policy.
 *
 * The mere transfer of this software does not imply any licenses
 * of trade secrets, proprietary technology, copyrights, patents,
 * trademarks, maskwork rights, or any other form of intellectual
 * property whatsoever. Maxim Integrated Products, Inc. retains all
 * ownership rights.
 *
 ******************************************************************************/

/**
 * @file    mscmem.h
 * @brief   Memory routines used by the USB Mass Storage Class example.
 *          See the msc_mem_t structure in msc.h for function details.
 * @details Functions are provided for using the external SPI flash memory.
 */

#include "mscmem.h"
#include <string.h>
#include <stdio.h>
#include "Ext_Flash.h"
#include "spixf.h"

#include "sdhc_lib.h"

/***** Definitions *****/

#define SPIXF_DISK 0

#define LBA_SIZE 512 /* Size of "logical blocks" in bytes */
#define LBA_SIZE_SHIFT 9 /* The shift value used to convert between addresses and block numbers */

/***** Global Data *****/

/***** File Scope Variables *****/

//static int initialized = 0;
//static int running = 0;

#if SPIXF_DISK
//==============================================================================
// SPIXF Disk reference MSC interface functions
//==============================================================================

#undef EXT_FLASH_BAUD
#define EXT_FLASH_BAUD 5000000 /* SPI clock rate to communicate with the external flash */

#define EXT_FLASH_SECTOR_SIZE 4096 /* Number of bytes in one sector of the external flash */
#define EXT_FLASH_SECTOR_SIZE_SHIFT \
    12 /* The shift value used to convert between addresses and block numbers */
#define EXT_FLASH_NUM_SECTORS 2048 /* Total number of sectors in the external flash */

#define MXC_SPIXF_WIDTH Ext_Flash_DataLine_Single /*Number of data lines*/

#define LBA_PER_SECTOR (EXT_FLASH_SECTOR_SIZE >> LBA_SIZE_SHIFT)
#define INVALID_SECTOR \
    EXT_FLASH_NUM_SECTORS /* Use a sector number past the end of memory to indicate invalid */

/***** File Scope Variables *****/
static uint32_t sectorNum = INVALID_SECTOR;
static uint8_t sector[EXT_FLASH_SECTOR_SIZE];
static int sectorDirty = 0;

/***** Function Prototypes *****/
static uint32_t getSectorNum(uint32_t lba);
static uint32_t getSectorAddr(uint32_t lba);
static uint32_t getSector(uint32_t num);

/******************************************************************************/
static uint32_t getSectorNum(uint32_t lba)
{
    /* Absolute_address = lba * LBA_SIZE                    */
    /* Sector_num = Absolute_address / EXT_FLASH_SECTOR_SIZE     */
    /* Sector_num = lba * 512 / 4096                        */
    return lba >> (EXT_FLASH_SECTOR_SIZE_SHIFT - LBA_SIZE_SHIFT);
}

/******************************************************************************/
static uint32_t getSectorAddr(uint32_t lba)
{
    /* eight 512 byte blocks in each sector */
    return (lba & (LBA_PER_SECTOR - 1)) << LBA_SIZE_SHIFT;
}

/******************************************************************************/
static uint32_t getSector(uint32_t num)
{
    /* New sector requested? */
    if (sectorNum != num) {
        /* Is the current sector real? */
        if (sectorNum != INVALID_SECTOR) {
            /* Was it written to after it was read from memory? */
            if (sectorDirty) {
                /* Erase the old data. */
                Ext_Flash_Erase(sectorNum << EXT_FLASH_SECTOR_SIZE_SHIFT, Ext_Flash_Erase_4K);
                /* Write the new */
                Ext_Flash_Program_Page(sectorNum << EXT_FLASH_SECTOR_SIZE_SHIFT, sector,
                                       EXT_FLASH_SECTOR_SIZE, MXC_SPIXF_WIDTH);
                /* Mark data as clean */
                sectorDirty = 0;
            }
        }

        /* Requesting a new valid sector? */
        if (num != INVALID_SECTOR) {
            Ext_Flash_Read(num << EXT_FLASH_SECTOR_SIZE_SHIFT, sector, EXT_FLASH_SECTOR_SIZE,
                           MXC_SPIXF_WIDTH);
            sectorDirty = 0;
            sectorNum = num;
        }
    }

    return 0;
}

/******************************************************************************/
int mscmem_Init()
{
    if (!initialized) {
        MXC_SPIXF_SetSPIFrequency(EXT_FLASH_BAUD);
        Ext_Flash_Init();
        Ext_Flash_Reset();

        if (MXC_SPIXF_WIDTH == Ext_Flash_DataLine_Quad) {
            Ext_Flash_Quad(1);
        } else {
            Ext_Flash_Quad(0);
        }

        initialized = 1;
    }

    return 0;
}

/******************************************************************************/
uint32_t mscmem_Size(void)
{
    /* Get number of 512 byte chunks the external flash contains. */
    return (EXT_FLASH_SECTOR_SIZE >> LBA_SIZE_SHIFT) * EXT_FLASH_NUM_SECTORS;
}

/******************************************************************************/
int mscmem_Read(uint32_t lba, uint8_t *buffer)
{
    uint32_t addr;

    /* Convert to external flash sector number. */
    uint32_t sNum = getSectorNum(lba);

    if (getSector(sNum)) {
        /* Failed to write/read from external flash */
        return 1;
    }

    /* Get the offset into the current sector */
    addr = getSectorAddr(lba);

    memcpy(buffer, sector + addr, LBA_SIZE);

    return 0;
}

/******************************************************************************/
int mscmem_Write(uint32_t lba, uint8_t *buffer)
{
    uint32_t addr;

    /* Convert to external flash sector number. */
    uint32_t sNum = getSectorNum(lba);

    if (getSector(sNum)) {
        /* Failed to write/read from external flash */
        return 1;
    }

    /* Get the offset into the current sector */
    addr = getSectorAddr(lba);

    memcpy(sector + addr, buffer, LBA_SIZE);
    sectorDirty = 1;

    return 0;
}

/******************************************************************************/
int mscmem_Start()
{
    /* Turn on the external flash if it is not already. */
    if (!initialized) {
        mscmem_Init();
    }

    /* Check if the initialization succeeded. If it has, start running. */
    if (initialized) {
        running = 1;
    }

    /* Start should return fail (non-zero) if the memory cannot be initialized. */
    return !initialized;
}

/******************************************************************************/
int mscmem_Stop()
{
    /* Could shut down XIPF interface here. */

    /* Flush the currently cached sector if necessary. */
    if (getSector(INVALID_SECTOR)) {
        return 1;
    }

    running = 0;
    return 0;
}

/******************************************************************************/
int mscmem_Ready()
{
    return running;
}

#else // Native SDHC disk
//==============================================================================
// Native SDHC/eMMC MSC interface functions
//==============================================================================

#define E_MMC_SECTOR_SIZE   512
#define E_MMC_SECTOR_COUNT  15269888
#define E_MMC_INVALID_SECTOR    (E_MMC_SECTOR_COUNT + 1)

static uint8_t msc_emmc_initialized = 0;
static uint8_t msc_emmc_running = 0;

#define USE_CACHED_SECTOR   0

#if USE_CACHED_SECTOR
static uint32_t cachedSectorNum = E_MMC_INVALID_SECTOR;
static uint8_t cachedSector[E_MMC_SECTOR_SIZE];
static int cachedSectorDirty = 0;
#endif

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
extern uint8_t SetupSDHCeMMC(void);
int mscmem_Init(void)
{
    if (MXC_SDHC_Lib_Get_Card_Type() == CARD_MMC)
    {
        msc_emmc_initialized = 1;
    }
    else // Setup the SDHC eMMC (should have been done in InitHardware)
    {
        SetupSDHCeMMC();
        if (MXC_SDHC_Lib_Get_Card_Type() == CARD_MMC) { msc_emmc_initialized = 1; }
    }

    return (!msc_emmc_initialized);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int mscmem_Start(void)
{
    // Initialize the SDHC if not already, but should always be up
    if (!msc_emmc_initialized) { mscmem_Init(); }

    if (msc_emmc_initialized) { msc_emmc_running = 1; }

    return (!msc_emmc_running);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int mscmem_Stop(void)
{
#if USE_CACHED_SECTOR
    // Flush the currently cached sector if necessary
    if (cachedSectorDirty)
    {
        MXC_SDHC_Lib_Write(cachedSectorNum, (void *)cachedSector, LBA_SIZE, MXC_SDHC_LIB_QUAD_DATA);
        cachedSectorDirty = 0;
    }
#endif

    // Reset/clear any active/running flags
    msc_emmc_running = 0;

    return (0);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int mscmem_Ready(void)
{
    // Return active/running flag state
    return (msc_emmc_running);
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
uint32_t mscmem_Size(void)
{
#if 1 /* Use datasheet sector count */
    // Return the number of 512 byte chunks within the embedded MMC flash
    return (E_MMC_SECTOR_COUNT);
#else /* Read device method via SDHC library produces a value larger than the return value would hold */
    mxc_sdhc_csd_regs_t csd;
    MXC_SDHC_Lib_GetCSD(&csd);
    uint32_t size = MXC_SDHC_Lib_GetCapacity(&csd);
    return (size);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int mscmem_Read(uint32_t lba, uint8_t *buffer)
{
#if USE_CACHED_SECTOR /* Using a cached sector */
    int status = 0;

    // Check if request is a different sector than the prior request
    if (lba != cachedSectorNum)
    {
        if (cachedSectorDirty)
        {
            MXC_SDHC_Lib_Write(cachedSectorNum, (void *)cachedSector, LBA_SIZE, MXC_SDHC_LIB_QUAD_DATA);
            cachedSectorDirty = 0;
        }

        status = MXC_SDHC_Lib_Read(cachedSector, lba, LBA_SIZE, MXC_SDHC_LIB_QUAD_DATA);
        cachedSectorNum = lba;
    }

    memcpy(buffer, cachedSector, LBA_SIZE);

    return (status);
#else /* Raw access */
    // Read sector, lba directly translates to sector number, assume that also equals sector address for SDHC lib read (raw addres wouldn't fit in uint32)
    int status = MXC_SDHC_Lib_Read(buffer, lba, LBA_SIZE, MXC_SDHC_LIB_QUAD_DATA);

    return (status);
#endif
}

///----------------------------------------------------------------------------
///	Function Break
///----------------------------------------------------------------------------
int mscmem_Write(uint32_t lba, uint8_t *buffer)
{
#if USE_CACHED_SECTOR /* Using a cached sector */
    // Check if request is a different sector than the prior request
    if (lba != cachedSectorNum)
    {
        if (cachedSectorDirty)
        {
            MXC_SDHC_Lib_Write(cachedSectorNum, (void *)cachedSector, LBA_SIZE, MXC_SDHC_LIB_QUAD_DATA);
            cachedSectorDirty = 0;
        }

        cachedSectorNum = lba;
    }

    memcpy(cachedSector, buffer, LBA_SIZE);
    cachedSectorDirty = 1;

    return (0);
#else /* Raw access */
    // Write sector, lba directly translates to sector number, assume that also equals sector address for SDHC lib write (raw addres wouldn't fit in uint32)
    int status = MXC_SDHC_Lib_Write(lba, (void *)buffer, LBA_SIZE, MXC_SDHC_LIB_QUAD_DATA);

    return (status);
#endif
}

#endif
