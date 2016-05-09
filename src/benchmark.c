#include <assert.h>
#include <stdio.h>

#include <xmc_ccu4.h>
#include <xmc_scu.h>

#include "uart.h"
#include "8b10b.h"

#define MODULE_PTR        CCU42
#define MODULE_NUMBER     (2U)
#define CAPCOM_MASK       (SCU_GENERAL_CCUCON_GSC42_Msk) /**< Only CCU42 */
#define STATUS_LED P1_0

static void initTimer () {
	/* Ensure fCCU reaches CCU42 */
	XMC_CCU4_SetModuleClock(MODULE_PTR, XMC_CCU4_CLOCK_SCU);

	XMC_CCU4_Init(MODULE_PTR, XMC_CCU4_SLICE_MCMS_ACTION_TRANSFER_PR_CR);

	/* Get the slice out of idle mode */
	XMC_CCU4_EnableClock(MODULE_PTR, 2);
	XMC_CCU4_EnableClock(MODULE_PTR, 3);

	/* Start the prescaler and restore clocks to slices */
	XMC_CCU4_StartPrescaler(MODULE_PTR);

	XMC_CCU4_SLICE_CAPTURE_CONFIG_t config = {
		.fifo_enable = false,
		.timer_clear_mode = XMC_CCU4_SLICE_TIMER_CLEAR_MODE_NEVER,
		.same_event = true,
		.ignore_full_flag = false, /* ? */
		.prescaler_mode = XMC_CCU4_SLICE_PRESCALER_MODE_NORMAL,
		.prescaler_initval = 3, /* prescaler is 8 */
		.float_limit = 0,
		.timer_concatenation = false,
		};
	XMC_CCU4_SLICE_CaptureInit (CCU42_CC42, &config);
	config.timer_concatenation = true;
	XMC_CCU4_SLICE_CaptureInit (CCU42_CC43, &config);

	/* Enable shadow transfer */
	/* XXX: do we need this? */
	XMC_CCU4_EnableShadowTransfer(MODULE_PTR, XMC_CCU4_SHADOW_TRANSFER_SLICE_2);
	XMC_CCU4_EnableShadowTransfer(MODULE_PTR, XMC_CCU4_SHADOW_TRANSFER_SLICE_3);

	/* Local variable which holds configuration of Event-1 */
	XMC_CCU4_SLICE_EVENT_CONFIG_t evcfg = {
		.duration = XMC_CCU4_SLICE_EVENT_FILTER_DISABLED,
		.edge     = XMC_CCU4_SLICE_EVENT_EDGE_SENSITIVITY_RISING_EDGE,
		.level    = XMC_CCU4_SLICE_EVENT_LEVEL_SENSITIVITY_ACTIVE_HIGH, /* Not needed */
		.mapped_input = XMC_CCU4_SLICE_INPUT_I,
		};

	/* Configure Event-1 and map it to Input-I. Only the first slice needs to
	 * be set up, all concatenated slices inherit this configuration */
	XMC_CCU4_SLICE_ConfigureEvent(CCU42_CC42, XMC_CCU4_SLICE_EVENT_1, &evcfg);
	XMC_CCU4_SLICE_ConfigureEvent(CCU42_CC43, XMC_CCU4_SLICE_EVENT_1, &evcfg);
	/* map timer capture to event 1 */
	XMC_CCU4_SLICE_Capture1Config(CCU42_CC42, XMC_CCU4_SLICE_EVENT_1);
	XMC_CCU4_SLICE_Capture1Config(CCU42_CC43, XMC_CCU4_SLICE_EVENT_1);

	XMC_CCU4_SLICE_StartTimer (CCU42_CC42);
	XMC_CCU4_SLICE_StartTimer (CCU42_CC43);
}

void SystemCoreClockSetup () {
	/* setup high precision PLL using onboard 12 MHz crystal */
	static const XMC_SCU_CLOCK_CONFIG_t clkcfg = {
		.syspll_config = {
			/* set to 80 MHz, see manual p. 11-39 */
			.p_div = 3,
			.n_div = 80,
			.k_div = 4,
			.mode = XMC_SCU_CLOCK_SYSPLL_MODE_NORMAL,
			.clksrc = XMC_SCU_CLOCK_SYSPLLCLKSRC_OSCHP,
			},
		.enable_oschp = true,
		.enable_osculp = false,
		.calibration_mode = XMC_SCU_CLOCK_FOFI_CALIBRATION_MODE_FACTORY,
		.fstdby_clksrc = XMC_SCU_HIB_STDBYCLKSRC_OSI,
		.fsys_clksrc = XMC_SCU_CLOCK_SYSCLKSRC_PLL,
		.fsys_clkdiv = 1,
		.fcpu_clkdiv = 1,
		.fccu_clkdiv = 1,
		.fperipheral_clkdiv = 1,
		};
	XMC_SCU_CLOCK_Init (&clkcfg);
}

int main() {
	static uint8_t src[4096];
	static uint8_t dest[sizeof (src)*10/8+1];

	/* uart (for messages) */
	uartInit ();
	initTimer ();

	/* init data */
	for (size_t i = 0; i < sizeof (src); i++) {
		/* XXX: should use random data */
		src[i] = i;
	}

	for (size_t size = 1; size < sizeof (src); size++) {
		uint32_t encode, decode;
		/* bare-metal is deterministic enough, one loop */
		//for (size_t i = 0; i < 100; i++) {
			XMC_CCU4_SLICE_ClearTimer (CCU42_CC43);
			XMC_CCU4_SLICE_ClearTimer (CCU42_CC42);

			eightbtenbCtx linecode;
			eightbtenbInit (&linecode);
			eightbtenbSetDest (&linecode, dest);
			eightbtenbEncode (&linecode, src, size);

			/* capture and read timer */
			XMC_SCU_SetCcuTriggerHigh (CAPCOM_MASK);

			uint32_t lower, upper;
			if (XMC_CCU4_SLICE_GetLastCapturedTimerValue (CCU42_CC43,
					XMC_CCU4_SLICE_CAP_REG_SET_HIGH, &upper) != XMC_CCU4_STATUS_OK) {
				assert (0);
			}
			if (XMC_CCU4_SLICE_GetLastCapturedTimerValue (CCU42_CC42,
					XMC_CCU4_SLICE_CAP_REG_SET_HIGH, &lower) != XMC_CCU4_STATUS_OK) {
				assert (0);
			}
			encode = ((upper & 0xffff) << 16) | (lower & 0xffff);

			XMC_SCU_SetCcuTriggerLow(CAPCOM_MASK);

			XMC_CCU4_SLICE_ClearTimer (CCU42_CC43);
			XMC_CCU4_SLICE_ClearTimer (CCU42_CC42);

			eightbtenbInit (&linecode);
			eightbtenbSetDest (&linecode, src);
			eightbtenbDecode (&linecode, dest, size*10);

			/* capture and read timer */
			XMC_SCU_SetCcuTriggerHigh (CAPCOM_MASK);

			if (XMC_CCU4_SLICE_GetLastCapturedTimerValue (CCU42_CC43,
					XMC_CCU4_SLICE_CAP_REG_SET_HIGH, &upper) != XMC_CCU4_STATUS_OK) {
				assert (0);
			}
			if (XMC_CCU4_SLICE_GetLastCapturedTimerValue (CCU42_CC42,
					XMC_CCU4_SLICE_CAP_REG_SET_HIGH, &lower) != XMC_CCU4_STATUS_OK) {
				assert (0);
			}
			decode = ((upper & 0xffff) << 16) | (lower & 0xffff);

			XMC_SCU_SetCcuTriggerLow(CAPCOM_MASK);

			printf ("%u\t%lu\t%lu\n", size, encode, decode);
		//}
	}

	while (1);
}

