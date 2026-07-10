/*
 * config.h — Central configuration for the Smart Valve firmware.
 *
 * Single home for tunable constants: clock frequencies, pin assignments,
 * timing intervals, and thresholds. This file grows one section at a time
 * as each development phase is implemented (see CLAUDE.md §9 roadmap).
 *
 * Phase 1 adds the clock section only.
 */

#ifndef CONFIG_H_
#define CONFIG_H_

/* =====================================================================
 * CLOCK SYSTEM (Phase 1)
 * ---------------------------------------------------------------------
 * The board has three external crystals:
 *   - HF  crystal: 8 MHz     -> drives MCLK (CPU) and SMCLK (fast periph.)
 *   - LF  crystal: 32.768 kHz-> drives ACLK (RTC, low-power wakeup)
 *   - USS crystal: 8 MHz     -> used ONLY by the USS module (Phase 11),
 *                               initialized by USSLib, NOT here.
 * ===================================================================== */

/* External crystal frequencies, in Hz. Must match the physical crystals. */
#define CONFIG_HFXT_FREQ_HZ     8000000UL   /* 8 MHz  high-frequency crystal */
#define CONFIG_LFXT_FREQ_HZ     32768UL     /* 32.768 kHz low-freq crystal   */

/* Resulting system clock frequencies after clock_init().
 * MCLK  = HFXT      = 8 MHz  (CPU core)
 * SMCLK = HFXT      = 8 MHz  (UART, I2C, ADC, timers)
 * ACLK  = LFXT      = 32.768 kHz (RTC, low-power timing)                 */
#define CONFIG_MCLK_FREQ_HZ     CONFIG_HFXT_FREQ_HZ
#define CONFIG_SMCLK_FREQ_HZ    CONFIG_HFXT_FREQ_HZ
#define CONFIG_ACLK_FREQ_HZ     CONFIG_LFXT_FREQ_HZ

#endif /* CONFIG_H_ */
