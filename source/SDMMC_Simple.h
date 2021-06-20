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
#ifndef SDMMC_SIMPLE_H_
#define SDMMC_SIMPLE_H_

#include "sdmmc_config.h"
#include "ff.h"

#define SDMMC_ENTRIES_MAX		(1000)

class SDMMC_Simple {
private:
	FATFS fileSystem; /* File system object */
	FIL fileRWObject; /* File object */
	bool dataFileExists = true;
public:
	SDMMC_Simple();
	virtual ~SDMMC_Simple();

	status_t sdcardWaitCardInsert(void);
	status_t open(void);
	status_t mount(void);
	status_t close(void);
	status_t read(uint32_t, uint8_t*, uint32_t);
	status_t write(uint32_t, uint8_t*, uint32_t);
	bool isDataFileExists(void);
	void setDataFileExists(bool);
};

#endif /* SDMMC_SIMPLE_H_ */
