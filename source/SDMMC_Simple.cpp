/*
 * MIT License
 *
 * Copyright (c) 2021 Subhasish Ghosh
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/*! @file */
#include <SDMMC_Simple.h>
#include "fsl_sd_disk.h"
#include "fsl_debug_console.h"

#define SDMMC_FILEPATH_LEN_MAX	20
#define SDMMC_ENTRIES_SZ		(4 * 1024)
#define SDMMC_ENTRIES_OFFSET(x)	(x * SDMMC_ENTRIES_SZ)
#define SDMMC_INIT_CHUNK_SZ		(1024)
#define SDMMC_FILE_SZ			(SDMMC_ENTRIES_MAX * SDMMC_ENTRIES_SZ)

SDMMC_Simple::SDMMC_Simple() {

}

SDMMC_Simple::~SDMMC_Simple() {

}

status_t SDMMC_Simple::sdcardWaitCardInsert(void) {
	BOARD_SD_Config(&g_sd, NULL, BOARD_SDMMC_SD_HOST_IRQ_PRIORITY, NULL);

	/* SD host init function */
	if (SD_HostInit(&g_sd) != kStatus_Success) {
		PRINTF("\r\nSD host init fail\r\n");
		return kStatus_Fail;
	}

	/* wait card insert */
	if (SD_PollingCardInsert(&g_sd, kSD_Inserted) == kStatus_Success) {
		PRINTF("\r\nCard inserted.\r\n");
		/* power off card */
		SD_SetCardPower(&g_sd, false);
		/* power on the card */
		SD_SetCardPower(&g_sd, true);
	} else {
		PRINTF("\r\nCard detect fail.\r\n");
		return kStatus_Fail;
	}

	return kStatus_Success;
}

/* Close Sdcard file */
status_t SDMMC_Simple::close(void) {

	if (f_close(&fileRWObject) != FR_OK) {
		PRINTF("failed to close file: RLIC.dat\n");
		return kStatus_Fail;
	}

	return kStatus_Success;
}

/* Open SDCard File */
status_t SDMMC_Simple::open(void) {

	FRESULT error;
	UINT bytesWritten;

	if (f_open(&fileRWObject, _T("/dir_1/RLIC.dat"),
			(FA_WRITE | FA_READ | FA_OPEN_EXISTING)) != FR_OK) {
		setDataFileExists(false);
		PRINTF("Open existing file failed, Initialising RLIC.dat..\r\n");
		if (f_open(&fileRWObject, _T("/dir_1/RLIC.dat"),
				(FA_WRITE | FA_READ | FA_OPEN_ALWAYS)) != FR_OK) {
			PRINTF("Failed to open RLIC.dat!\r\n");
			return kStatus_Fail;
		} else {
			uint8_t data[SDMMC_INIT_CHUNK_SZ];
			bzero(data, sizeof(data));

			/* if new file, then initialise */
			for (int i = 0; i < SDMMC_FILE_SZ / SDMMC_INIT_CHUNK_SZ; i++) {
				error = f_write(&fileRWObject, data, sizeof(data),
						&bytesWritten);
				if ((error) || (bytesWritten != sizeof(data))) {
					PRINTF("Write file failed. \r\n");
					return kStatus_Fail;
				}
			}
		}
	}

	if (f_sync(&fileRWObject) != FR_OK) {
		PRINTF("Sync file failed. \r\n");
		return kStatus_Fail;
	}

	return kStatus_Success;
}

/* mount sdcard */
status_t SDMMC_Simple::mount(void) {

	FRESULT error;
	const TCHAR driverNumberBuffer[3U] = { SDDISK + '0', ':', '/' };

	if ((f_mount(&fileSystem, driverNumberBuffer, 0U) != FR_OK)) {
		PRINTF("Mount volume failed.\r\n");
		return kStatus_Fail;
	}

	if ((f_chdrive((char const*) &driverNumberBuffer[0U]) != FR_OK)) {
		PRINTF("Change drive failed.\r\n");
		return kStatus_Fail;
	}

	error = f_mkdir(_T("/dir_1"));
	if ((error != FR_OK) && (error != FR_EXIST)) {
#if FF_USE_MKFS
		BYTE work[FF_MAX_SS];
		PRINTF("\r\nMake file system......The time may be long if the card capacity is big.\r\n");
		if (f_mkfs(driverNumberBuffer, 0, work, sizeof work))
		{
			PRINTF("Make file system failed.\r\n");
			return kStatus_Fail;
		}
#endif /* FF_USE_MKFS */
    	error = f_mkdir(_T("/dir_1"));
    	if ((error != FR_OK) && (error != FR_EXIST)) {
    		PRINTF("Make directory failed.\r\n");
    		return kStatus_Fail;
    	}
	}

	return kStatus_Success;
}

/* read data from sdcard */
status_t SDMMC_Simple::read(uint32_t numbytes, uint8_t *data,
		uint32_t fileidx) {

	FRESULT error;
	UINT bytesRead;

	if (f_lseek(&fileRWObject, SDMMC_ENTRIES_OFFSET(fileidx)) != FR_OK) {
		PRINTF("Read lseek file failed. \r\n");
		return kStatus_Fail;
	}

	error = f_read(&fileRWObject, data, numbytes, &bytesRead);
	if ((error) || (bytesRead != numbytes)) {
		PRINTF("Read file failed. \r\n");
		return kStatus_Fail;
	}

	return kStatus_Success;
}

/* update sdcard data */
status_t SDMMC_Simple::write(uint32_t numbytes, uint8_t *data,
		uint32_t fileidx) {

	FRESULT error;
	UINT bytesWritten;

	if (f_lseek(&fileRWObject, SDMMC_ENTRIES_OFFSET(fileidx)) != FR_OK) {
		PRINTF("Write lseek file failed. \r\n");
		return kStatus_Fail;
	}

	error = f_write(&fileRWObject, data, numbytes, &bytesWritten);
	if ((error) || (bytesWritten != numbytes)) {
		PRINTF("Write file failed. \r\n");
		return kStatus_Fail;
	}

	if (f_sync(&fileRWObject) != FR_OK) {
		PRINTF("Sync file failed. \r\n");
		return kStatus_Fail;
	}

	return kStatus_Success;
}

bool SDMMC_Simple::isDataFileExists(void) {
	return dataFileExists;
}

void SDMMC_Simple::setDataFileExists(bool dataFileExists) {
	this->dataFileExists = dataFileExists;
}
