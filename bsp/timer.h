/*
 * bsp/timer.h — RTC periodic wake-up (BSP layer).
 *
 * Phase 5 scope: configure the RTC_C to fire a 1 Hz tick interrupt and,
 * every MEASURE_INTERVAL_S seconds, raise a "measurement due" flag that the
 * main loop consumes. The RTC is clocked from the LF crystal (ACLK domain),
 * so it keeps running in low-power sleep.
 *
 * (Timer_A for the bit-bang UART timebase and the encoder is added to this
 * file in later phases.)
 */

#ifndef BSP_TIMER_H_
#define BSP_TIMER_H_

#include <stdint.h>

/*
 * rtc_init() — start the RTC_C producing a 1 Hz tick interrupt. Call after
 * clock_init() (the RTC uses the 32.768 kHz LF crystal).
 */
void rtc_init(void);

/*
 * rtc_measurement_due() — returns non-zero once MEASURE_INTERVAL_S seconds
 * have elapsed since the last clear. Set by the RTC ISR, read by main.
 */
uint8_t rtc_measurement_due(void);

/* rtc_clear_measurement_due() — clear the flag after handling a wake-up. */
void rtc_clear_measurement_due(void);

#endif /* BSP_TIMER_H_ */
