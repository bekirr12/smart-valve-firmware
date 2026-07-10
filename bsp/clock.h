/*
 * bsp/clock.h — Clock system setup (BSP layer, driverlib wrapper).
 *
 * Configures the MSP430FR6047 clock tree from the external crystals:
 *   MCLK  = 8 MHz      (from HF crystal)  -> CPU
 *   SMCLK = 8 MHz      (from HF crystal)  -> UART / I2C / ADC / timers
 *   ACLK  = 32.768 kHz (from LF crystal)  -> RTC / low-power wakeup
 *
 * The USS module's dedicated 8 MHz crystal is NOT handled here; it is set
 * up later by USSLib (Phase 11).
 */

#ifndef BSP_CLOCK_H_
#define BSP_CLOCK_H_

/*
 * clock_init() — bring up the LF and HF crystals and route them to the
 * system clocks (MCLK, SMCLK, ACLK). Blocks until both crystals are stable.
 *
 * Must be called early in startup, after the watchdog is stopped. It calls
 * PMM_unlockLPM5() internally, which unlocks the GPIO/crystal pins that the
 * FRAM device holds locked after power-up.
 */
void clock_init(void);

#endif /* BSP_CLOCK_H_ */
