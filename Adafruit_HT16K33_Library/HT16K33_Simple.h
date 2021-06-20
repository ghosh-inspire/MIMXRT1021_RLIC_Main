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
#include <stdlib.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MIMXRT1021.h"
#include "fsl_debug_console.h"

#ifndef HT16K33_SIMPLE_H_
#define HT16K33_SIMPLE_H_

#define HT16K33_COL_MAX		16
#define HT16K33_ROW_MAX		8
#define HT16K33_DIMCTR_MAX	15
#define HT16K33_ONLED_MAX	64
#define HT16K33_DAYLIGHT_LED_I2C_ADDR	0x72
#define HT16K33_RLIC_LED_I2C_ADDR		0x70

#define HT16K33_SYSTEM_SETUP_REG		0x20
#define HT16K33_DISPLAY_SETUP_REG		0x80
#define HT16K33_DIMMING_REG				0xE0

#define HT16K33_SYSTEM_SETUP_S_BIT_POS	1
#define HT16K33_DISPLAY_SETUP_D_BIT_POS	1

#define HT16K33_MID_DIMMING_ROW			4
#define HT16K33_SAT_CTR_MAX				56U

class HT16K33_Simple {
private:
	uint8_t col = 0, row = 0, sunRise = 1, dimCtr = 0;
	uint32_t saturationCtr = HT16K33_SAT_CTR_MAX;
	uint8_t ledMatrix[HT16K33_COL_MAX];
public:
	HT16K33_Simple();
	virtual ~HT16K33_Simple();
	void initHT16K33(void);
	bool cycleDayLight(void);
	void setLedBrightness(uint8_t, uint8_t);
};

#endif /* HT16K33_SIMPLE_H_ */
