/*
 * bsp/power.h — Low-power mode entry (BSP layer).
 *
 * Phase 5 scope: enter sleep. Code requests LPM3; if a peripheral (e.g. the
 * RS485 UART, added later) is requesting SMCLK, the device actually settles
 * in LPM1 — this is expected (see CLAUDE.md §4).
 *
 * (The ±15 V opamp-supply enable helper is added here in the USS phase.)
 */

#ifndef BSP_POWER_H_
#define BSP_POWER_H_

/*
 * power_enter_sleep() — enter LPM3 with interrupts enabled (GIE). Returns
 * after an enabled interrupt (e.g. the RTC tick) wakes the CPU.
 */
void power_enter_sleep(void);

#endif /* BSP_POWER_H_ */
