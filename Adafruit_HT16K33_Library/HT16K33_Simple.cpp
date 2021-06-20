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
#include "HT16K33_Simple.h"
#include <climits>

HT16K33_Simple::HT16K33_Simple() {

}

HT16K33_Simple::~HT16K33_Simple() {

}

/* Init LED interface */
void HT16K33_Simple::initHT16K33(void) {
	uint8_t ledData = 0;

	/* Reset */
	BOARD_LPI2C_Send(BOARD_CODEC_I2C_BASEADDR, HT16K33_DAYLIGHT_LED_I2C_ADDR,
	HT16K33_SYSTEM_SETUP_REG, 1, &ledData, 1);
	BOARD_LPI2C_Send(BOARD_CODEC_I2C_BASEADDR, HT16K33_RLIC_LED_I2C_ADDR,
	HT16K33_SYSTEM_SETUP_REG, 1, &ledData, 1);
	BOARD_LPI2C_Send(BOARD_CODEC_I2C_BASEADDR, HT16K33_DAYLIGHT_LED_I2C_ADDR,
	HT16K33_DISPLAY_SETUP_REG, 1, &ledData, 1);
	BOARD_LPI2C_Send(BOARD_CODEC_I2C_BASEADDR, HT16K33_RLIC_LED_I2C_ADDR,
	HT16K33_DISPLAY_SETUP_REG, 1, &ledData, 1);
	/* Reset Done */

	/* OSC ON */
	BOARD_LPI2C_Send(BOARD_CODEC_I2C_BASEADDR, HT16K33_DAYLIGHT_LED_I2C_ADDR,
	HT16K33_SYSTEM_SETUP_REG | HT16K33_SYSTEM_SETUP_S_BIT_POS, 1, &ledData, 1);
	BOARD_LPI2C_Send(BOARD_CODEC_I2C_BASEADDR, HT16K33_RLIC_LED_I2C_ADDR,
	HT16K33_SYSTEM_SETUP_REG | HT16K33_SYSTEM_SETUP_S_BIT_POS, 1, &ledData, 1);

	/* Reset RAM */
	for (int i = 0; i < HT16K33_COL_MAX; i++) {
		ledMatrix[i] = 0;
		BOARD_LPI2C_Send(BOARD_CODEC_I2C_BASEADDR,
				HT16K33_DAYLIGHT_LED_I2C_ADDR, i, 1, &ledData, 1);
		BOARD_LPI2C_Send(BOARD_CODEC_I2C_BASEADDR, HT16K33_RLIC_LED_I2C_ADDR, i,
				1, &ledData, 1);
	}

	/* Lowest Dimming */
	BOARD_LPI2C_Send(BOARD_CODEC_I2C_BASEADDR, HT16K33_DAYLIGHT_LED_I2C_ADDR,
			HT16K33_DIMMING_REG, 1, &ledData, 1);
	BOARD_LPI2C_Send(BOARD_CODEC_I2C_BASEADDR, HT16K33_RLIC_LED_I2C_ADDR,
			HT16K33_DIMMING_REG, 1, &ledData, 1);

	/* Display ON */
	BOARD_LPI2C_Send(BOARD_CODEC_I2C_BASEADDR, HT16K33_DAYLIGHT_LED_I2C_ADDR,
	HT16K33_DISPLAY_SETUP_REG | HT16K33_DISPLAY_SETUP_D_BIT_POS, 1, &ledData,
			1);
	BOARD_LPI2C_Send(BOARD_CODEC_I2C_BASEADDR, HT16K33_RLIC_LED_I2C_ADDR,
	HT16K33_DISPLAY_SETUP_REG | HT16K33_DISPLAY_SETUP_D_BIT_POS, 1, &ledData,
			1);
}

/* cycle day light logic */
bool HT16K33_Simple::cycleDayLight(void) {
	uint8_t ledData = 0;
	bool dayReset = false;

	if (sunRise) {
		if (HT16K33_COL_MAX <= col) {
			if (saturationCtr) {
				saturationCtr--;
				dayReset = false;
				goto exit;
			}
			sunRise = 0;
			col = 0;
			row = 0;
			dimCtr--;
			saturationCtr = HT16K33_SAT_CTR_MAX;
		} else {
			ledMatrix[col] ^= uint8_t(1 << row);
			if ((HT16K33_MID_DIMMING_ROW == row) || !row) {
				BOARD_LPI2C_Send(BOARD_CODEC_I2C_BASEADDR,
						HT16K33_DAYLIGHT_LED_I2C_ADDR,
						HT16K33_DIMMING_REG | dimCtr, 1, &ledData, 1);
				dimCtr++;
			}
			BOARD_LPI2C_Send(BOARD_CODEC_I2C_BASEADDR,
					HT16K33_DAYLIGHT_LED_I2C_ADDR, col, 1, &ledMatrix[col], 1);
			row++;
			if (HT16K33_ROW_MAX <= row) {
				col += 2;
				row = 0;
			}
		}
	} else {
		if (HT16K33_COL_MAX <= col) {
			if (saturationCtr) {
				saturationCtr--;
				dayReset = false;
				goto exit;
			}
			sunRise = 1;
			col = 0;
			row = 0;
			dimCtr++;
			saturationCtr = HT16K33_SAT_CTR_MAX;
			dayReset = true;
			goto exit;
		} else {
			ledMatrix[col] ^= uint8_t(1 << row);
			if ((HT16K33_MID_DIMMING_ROW == row) || !row) {
				BOARD_LPI2C_Send(BOARD_CODEC_I2C_BASEADDR,
						HT16K33_DAYLIGHT_LED_I2C_ADDR,
						HT16K33_DIMMING_REG | dimCtr, 1, &ledData, 1);
				dimCtr--;
			}
			BOARD_LPI2C_Send(BOARD_CODEC_I2C_BASEADDR,
					HT16K33_DAYLIGHT_LED_I2C_ADDR, col, 1, &ledMatrix[col], 1);
			row++;
			if (HT16K33_ROW_MAX <= row) {
				col += 2;
				row = 0;
			}
		}
	}
	exit: return dayReset;
}

/* set specific brightness */
void HT16K33_Simple::setLedBrightness(uint8_t numOnLed, uint8_t duty) {
	uint8_t ledMatrixLocal[HT16K33_COL_MAX];

	bzero(ledMatrixLocal, sizeof(ledMatrixLocal[0]) * HT16K33_COL_MAX);

	if (numOnLed > HT16K33_ONLED_MAX)
		numOnLed = HT16K33_ONLED_MAX;
	if (duty > HT16K33_DIMCTR_MAX)
		duty = HT16K33_DIMCTR_MAX;

	if (numOnLed) {
		uint8_t colSet = numOnLed / CHAR_BIT;
		uint8_t bitSet = numOnLed % CHAR_BIT;
		int colCtr = 0;
		int colValidCtr = 0;

		for (colCtr = 0, colValidCtr = 0; colCtr < colSet; colCtr++) {
			ledMatrixLocal[colValidCtr++] = 0xFF;
			ledMatrixLocal[colValidCtr++] = 0x00;
		}
		if (bitSet)
			ledMatrixLocal[colValidCtr] = uint8_t((1 << bitSet) - 1);
	} else {
		duty = 0;
	}

	uint32_t primask = DisableGlobalIRQ();

	BOARD_LPI2C_Send(BOARD_CODEC_I2C_BASEADDR, HT16K33_RLIC_LED_I2C_ADDR, 0, 1,
			ledMatrixLocal, HT16K33_COL_MAX);
	BOARD_LPI2C_Send(BOARD_CODEC_I2C_BASEADDR, HT16K33_RLIC_LED_I2C_ADDR,
	HT16K33_DIMMING_REG | duty, 1, ledMatrixLocal, 1);

	EnableGlobalIRQ(primask);
}
