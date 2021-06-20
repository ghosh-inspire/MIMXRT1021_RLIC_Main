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
#include <QLearning.h>
#include <stdlib.h>
#include <strings.h>
#include "fsl_debug_console.h"
#include "fsl_trng.h"

#define QLEARN_EXPLORE_MIN	(0) /* percent explore */
#define QLEARN_EXPLORE_MAX	(100)
#define QLEARN_LUX_MAX		(4500.0f) /* target illumination */
#define QTABLE_ONLED_MIN	(0)
#define QTABLE_ONLED_MAX	(64)
#define QTABLE_DIMM_MIN		(0)
#define QTABLE_DIMM_MAX		(15)
#define QLEARN_REWARD_MIN	(8) /* min acceptable reward */
#define QTABLE_TABLE_SZ		((QTABLE_ONLED_MAX + 1) * (QTABLE_DIMM_MAX + 1))

#define QLEARN_FAST_LEARN	(false)
#define QLEARN_PRUNECTR_MAX	(3)

/* main and pruned Q table */
enum qtable_idx_t {
	QTABLE_IDX = 0, QTABLE_PRUNED_IDX,
};

SDK_ALIGN(static uint8_t qtable[2][QTABLE_ONLED_MAX + 1][QTABLE_DIMM_MAX + 1],
		BOARD_SDMMC_DATA_BUFFER_ALIGN_SIZE);

QLearning::QLearning(void) {
	status_t status;
	uint32_t randdata = 1234;
	trng_config_t trngConfig;

	/* setup TRNG for seed */
	TRNG_GetDefaultConfig(&trngConfig);
	trngConfig.sampleMode = kTRNG_SampleModeVonNeumann;
	status = TRNG_Init(TRNG, &trngConfig);
	if (kStatus_Success == status) {
		status = TRNG_GetRandomData(TRNG, &randdata, sizeof(randdata));
		if (status == kStatus_Success) {
			srand(randdata);
			TRNG_Deinit(TRNG);
			return;
		}
	}

	/* TRNG failed, so default to something */
	srand(randdata);
}

QLearning::~QLearning() {

}

/* convert time MS to Qtable Index */
uint32_t QLearning::timeToQTableEntry(uint32_t dayTimeMS) {
	float seconds = float(dayTimeMS) / 1000.0f;

	uint32_t idx = (uint32_t) (seconds * 2.0f);

	if (idx > QTABLE_ENTRIES_MAX)
		idx = QTABLE_ENTRIES_MAX;
	return idx;
}

/* update QTable with latest data */
bool QLearning::updateQTable(Brightness brightness, uint8_t reward,
		uint32_t idx) {

	uint8_t exp_reward =
			qtable[QTABLE_IDX][brightness.numOnLeds][brightness.duty];
	qtable[QTABLE_IDX][brightness.numOnLeds][brightness.duty] = (exp_reward
			+ reward + 1) / 2;

	if (reward < QLEARN_REWARD_MIN) {
		if (qtable[QTABLE_PRUNED_IDX][brightness.numOnLeds][brightness.duty]
				< QLEARN_PRUNECTR_MAX) {
			qtable[QTABLE_PRUNED_IDX][brightness.numOnLeds][brightness.duty] +=
					1;
		}
	} else {
		qtable[QTABLE_PRUNED_IDX][brightness.numOnLeds][brightness.duty] = 0;
	}

	if (sdcard.write(sizeof(qtable), (uint8_t*) qtable, idx)
			!= kStatus_Success) {
		return false;
	}

	return true;
}

/* Calculate reward */
uint8_t QLearning::getReward(uint32_t luxT) {

	uint32_t gap = abs(luxT - QLEARN_LUX_MAX);
	float reward = 0.0;

	if (gap > QLEARN_LUX_MAX)
		return 0;

	reward = 1 - (gap / QLEARN_LUX_MAX);

	return uint8_t(reward * 10);
}

/* Get Brightness */
uint32_t QLearning::getQBrightness(Brightness &brightness, uint32_t dayTimeMS,
		bool &random, bool readqtable) {

	uint32_t idx = timeToQTableEntry(dayTimeMS);

	if(readqtable) {
		if (sdcard.read(sizeof(qtable), (uint8_t*) qtable, idx)
				!= kStatus_Success) {
			return QTABLE_ENTRIES_MAX + 1;
		}
	}

	brightness.duty = 0;
	brightness.numOnLeds = 0;

	if (random) { /* Retrieve uniform random */
		uint32_t ctr = QTABLE_TABLE_SZ;
		do {
			brightness.numOnLeds = uint8_t(rand() % (QTABLE_ONLED_MAX + 1));
			if (brightness.numOnLeds)
				brightness.duty = uint8_t(rand() % (QTABLE_DIMM_MAX + 1));

			if (qtable[QTABLE_PRUNED_IDX][brightness.numOnLeds][brightness.duty]
					< QLEARN_PRUNECTR_MAX)
				break;
		} while (ctr--);
	} else { /* Retrieve Expected */
		uint32_t maxidx[2] = { 0, 0 };
		for (int i = 0; i < (QTABLE_ONLED_MAX + 1); i++) {
			for (int j = 0; j < (QTABLE_DIMM_MAX + 1); j++) {
				if (qtable[QTABLE_IDX][i][j]
						> qtable[QTABLE_IDX][maxidx[0]][maxidx[1]]) {
					maxidx[0] = i;
					maxidx[1] = j;
				}
			}
		}
		brightness.numOnLeds = maxidx[0];
		brightness.duty = maxidx[1];
		if (qtable[QTABLE_PRUNED_IDX][brightness.numOnLeds][brightness.duty] >=
		QLEARN_PRUNECTR_MAX) {
			qtable[QTABLE_IDX][brightness.numOnLeds][brightness.duty] = 0;
			random = true;
			getQBrightness(brightness, dayTimeMS, random, false);
		}
	}

	return idx;
}

/* Decide Explore or Exploit */
bool QLearning::runExploreExploit(void) {

#if QLEARN_FAST_LEARN
	return true;
#endif

	if ((rand() % QLEARN_EXPLORE_MAX) >= QLEARN_EXPLORE_MIN) {
		return false;
	} else {
		return true;
	}

	return false;
}

/* mount sdcard */
bool QLearning::initQStorage(void) {

	if (sdcard.sdcardWaitCardInsert() != kStatus_Success) {
		return false;
	}

	if (sdcard.mount() != kStatus_Success) {
		return false;
	}

	if (sdcard.open() != kStatus_Success) {
		return false;
	}

	return true;
}

/* sync and save the data file */
void QLearning::closeQStorage(void) {
	sdcard.close();
}

/* print Q Table, reads into qtable */
void QLearning::__printQTable(uint32_t idx) {

	sdcard.read(sizeof(qtable), (uint8_t*) qtable, idx);

	for (int i = 0; i < (QTABLE_ONLED_MAX + 1); i++) {
		PRINTF("\nR-%d:\t", i);
		for (int j = 0; j < (QTABLE_DIMM_MAX + 1); j++) {
			PRINTF("%d [%d]\t", qtable[QTABLE_IDX][i][j],
					qtable[QTABLE_PRUNED_IDX][i][j]);
		}
	}
	PRINTF("\n");
}
