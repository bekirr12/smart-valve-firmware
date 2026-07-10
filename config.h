/*
 * config.h — Central configuration for the Smart Valve firmware.
 *
 * Single home for tunable constants: clock frequencies, pin assignments,
 * timing intervals, and thresholds. This file grows one section at a time
 * as each development phase is implemented (see CLAUDE.md §9 roadmap).
 *
 * Currently implemented: Phase 1 (clock), Phase 2 (LEDs + buttons).
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

/* =====================================================================
 * LEDs & BUTTONS (Phase 2)
 * ---------------------------------------------------------------------
 * Each signal is a (PORT, PIN) pair using driverlib constants, so code
 * reads GPIO_setOutputHighOnPin(LED1_PORT, LED1_PIN), etc.
 *
 * Polarities (confirmed against the hardware):
 *   - LEDs    active-high (pin HIGH = LED on).
 *   - Buttons active-low  (wired to GND, internal pull-up, pressed = LOW).
 *
 * Other pins (motor, USS, RS485, ±15V, LT8490, HMI) are added in their
 * own phases, not here.
 * ===================================================================== */

/* --- LEDs (outputs, active-high) --- */
#define LED1_PORT       GPIO_PORT_P4
#define LED1_PIN        GPIO_PIN2
#define LED2_PORT       GPIO_PORT_P4
#define LED2_PIN        GPIO_PIN1
#define LED3_PORT       GPIO_PORT_P4
#define LED3_PIN        GPIO_PIN0

/* --- Buttons (inputs, pull-up, active-low) --- */
#define BTN_DOWN_PORT   GPIO_PORT_P5
#define BTN_DOWN_PIN    GPIO_PIN0
#define BTN_SELECT_PORT GPIO_PORT_P5
#define BTN_SELECT_PIN  GPIO_PIN1
#define BTN_LEFT_PORT   GPIO_PORT_P5
#define BTN_LEFT_PIN    GPIO_PIN2
#define BTN_UP_PORT     GPIO_PORT_P5
#define BTN_UP_PIN      GPIO_PIN3
#define BTN_RIGHT_PORT  GPIO_PORT_P5
#define BTN_RIGHT_PIN   GPIO_PIN4

#endif /* CONFIG_H_ */
