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

/**
 * @file MIMXRT1021_RLIC_Main.cpp
 * \ref MIMXRT1021_RLIC_Main.cpp
 * @brief   Application entry point: Reinforcement Learning based Illumination Controller (RLIC)
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MIMXRT1021.h"
#include "fsl_debug_console.h"
#include "systick_delay.h"
#include "Adafruit_Sensor.h"
#include "Adafruit_TSL2591.h"
#include "HT16K33_Simple.h"
#include "QLearning.h"
#include "fsl_wdog.h"

#define RLIC_LED_GPIO			BOARD_USER_LED_GPIO
#define RLIC_LED_GPIO_PIN		BOARD_USER_LED_GPIO_PIN
#define RLIC_WDOG_BASE			WDOG1
#define RLIC_APP_EXIT_GPIO		BOARD_INITPINS_USER_BUTTON_GPIO
#define RLIC_APP_EXIT_GPIO_PIN	BOARD_INITPINS_USER_BUTTON_GPIO_PIN
#define RLIC_EXPLORE_STRING		"[EXPLORE]"
#define RLIC_EXPLOIT_STRING		"[EXPLOIT]"

#define QTMR_CLOCK_SOURCE_DIVIDER (128U)
/* The frequency of the source clock after divided. */
#define QTMR_SOURCE_CLOCK (CLOCK_GetFreq(kCLOCK_IpgClk) / QTMR_CLOCK_SOURCE_DIVIDER)

/* The PIN status */
static uint8_t g_pinSet = false;

/* Adafruit light sensor init */
static Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591);
/* Adafruit LEDs init */
static HT16K33_Simple ledControl;
static volatile bool dayReset = true;

#ifdef __cplusplus
extern "C" {
#endif
void TMR2_IRQHANDLER(void) {

	/* Clear interrupt flag.*/
	QTMR_ClearStatusFlags(TMR2_PERIPHERAL, TMR2_CHANNEL_1_CHANNEL,
			kQTMR_CompareFlag);

	if (!dayReset)
		dayReset = ledControl.cycleDayLight();

	SDK_ISR_EXIT_BARRIER;
}
#ifdef __cplusplus
}
#endif

/*
 * @brief   Application entry point.
 */
int main(void) {
	QLearning qlearn;
	Brightness brightness = { 0, 0 };
	uint32_t dayStartOffset = 0;
	uint32_t dayTimeMS = 0;
	uint16_t luxT = 0;
	uint32_t idx = 0;
	uint8_t reward = 0;

	/* Init board hardware. */
	BOARD_ConfigMPU();
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();

#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
	/* Init FSL debug console. */
	BOARD_InitDebugConsole();
#endif

	/*Clock setting for LPI2C*/
	CLOCK_SetMux(kCLOCK_Lpi2cMux, BOARD_ACCEL_I2C_CLOCK_SOURCE_SELECT);
	CLOCK_SetDiv(kCLOCK_Lpi2cDiv, BOARD_ACCEL_I2C_CLOCK_SOURCE_DIVIDER);

	SysTick_Init();

	PRINTF("Reinforcement Learning Based Illumination Controller\n");
	tsl.printSensorDetails();
	tsl.configureSensor();

	/* HT16K33 Init */
	ledControl.initHT16K33();

	/* mount SDCard */
	if (!qlearn.initQStorage())
		goto FAILED;

	/* Enable Day cycle Timer */
	EnableIRQ(TMR2_IRQN);

	while (1) {
		bool exep = false;
		const char *exepstr = RLIC_EXPLOIT_STRING;

		if (dayReset) {
			dayStartOffset = SysTick_UptimeMS();
			dayTimeMS = 0;
			dayReset = false;
		} else {
			dayTimeMS = SysTick_UptimeMS() - dayStartOffset;
		}

		/* Explore or Exploit */
		exep = qlearn.runExploreExploit();

		/* get LED and Dimm values */
		idx = qlearn.getQBrightness(brightness, dayTimeMS, exep, true);
		if (idx > QTABLE_ENTRIES_MAX)
			goto FAILED;

		/* set LEDs and Dimm */
		ledControl.setLedBrightness(brightness.numOnLeds, brightness.duty);

		/* Sence the brightness */
		luxT = tsl.getLuminosity(TSL2591_VISIBLE);
		/* twice gives better accuracy */
		luxT = tsl.getLuminosity(TSL2591_VISIBLE);

		/* Calculate reward */
		reward = qlearn.getReward(luxT);

		if (exep)
			exepstr = RLIC_EXPLORE_STRING;

		PRINTF("[%ld ms] [%ld] numOnLeds: %d duty; %d Lum: %d reward: %d %s\n",
				dayTimeMS, idx, brightness.numOnLeds, brightness.duty, luxT,
				reward, exepstr);

		/* update learned data */
		if (!qlearn.updateQTable(brightness, reward, idx))
			goto FAILED;

		//qlearn.__printQTable(idx);

		g_pinSet ^= 1;
		GPIO_PinWrite(RLIC_LED_GPIO, RLIC_LED_GPIO_PIN, g_pinSet);
		if (0 == GPIO_PinRead(RLIC_APP_EXIT_GPIO, RLIC_APP_EXIT_GPIO_PIN)) {
			goto APPEXIT;
		}
	}

	/* failed, so reset and try again */
FAILED:
	PRINTF("Board Runtime Failed. Resetting..\n");
	/* close storage to avoid corrupting the file */
	qlearn.closeQStorage();
	WDOG_TriggerSystemSoftwareReset(RLIC_WDOG_BASE);

	/* graceful exit */
APPEXIT:
	qlearn.closeQStorage();
	while (1) {
		g_pinSet ^= 1;
		GPIO_PinWrite(RLIC_LED_GPIO, RLIC_LED_GPIO_PIN, g_pinSet);
		SysTick_DelayTicksMS(50);
	}

	return 0;
}
