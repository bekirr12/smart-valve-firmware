/*
 * bsp/timer.c — RTC periodic wake-up implementation.
 *
 * The RTC_C runs in calendar mode, clocked from the 32.768 kHz LF crystal.
 * In calendar mode the internal prescalers auto-divide to 1 Hz, and the
 * "clock read ready" (RTCRDY) interrupt fires once per second. The ISR
 * counts those seconds and raises a flag every MEASURE_INTERVAL_S.
 *
 * (Counter mode + a manual prescale event would need the RT0PS/RT1PS clock
 * sources configured explicitly; calendar mode gives a clean 1 Hz for free.)
 */

#include <msp430.h>
#include "driverlib/MSP430FR5xx_6xx/driverlib.h"
#include "config.h"
#include "bsp/timer.h"

/* Shared with the ISR. volatile: the ISR writes these behind the main
 * loop's back, so the compiler must not cache them. */
static volatile uint16_t s_seconds         = 0;
static volatile uint8_t  s_measurement_due = 0;

void rtc_init(void)
{
    /* Calendar mode needs a starting time; the exact value is irrelevant
     * here — we only use the 1 Hz "seconds updated" (RTCRDY) interrupt, not
     * the wall-clock time.
     */
    Calendar startTime;
    startTime.Seconds    = 0;
    startTime.Minutes    = 0;
    startTime.Hours      = 0;
    startTime.DayOfWeek  = 0;
    startTime.DayOfMonth = 1;
    startTime.Month      = 1;
    startTime.Year       = 2025;

    RTC_C_initCalendar(RTC_C_BASE, &startTime, RTC_C_FORMAT_BINARY);

    /* RTCRDY asserts once per second when the seconds register updates. */
    RTC_C_clearInterrupt(RTC_C_BASE, RTC_C_CLOCK_READ_READY_INTERRUPT);
    RTC_C_enableInterrupt(RTC_C_BASE, RTC_C_CLOCK_READ_READY_INTERRUPT);

    RTC_C_startClock(RTC_C_BASE);
}

uint8_t rtc_measurement_due(void)
{
    return s_measurement_due;
}

void rtc_clear_measurement_due(void)
{
    s_measurement_due = 0;
}

/* RTC interrupt — fires once per second (RTCRDY, seconds updated).
 * Reading RTCIV clears the pending flag. Keep this short: just count and,
 * on the Nth second, raise the flag and wake the main loop.
 */
#pragma vector = RTC_C_VECTOR
__interrupt void rtc_c_isr(void)
{
    switch (__even_in_range(RTCIV, RTCIV__RTCRDYIFG))
    {
        case RTCIV__RTCRDYIFG:                    /* 1 Hz tick */
            if (++s_seconds >= MEASURE_INTERVAL_S)
            {
                s_seconds = 0;
                s_measurement_due = 1;
                /* Clear the LPM bits saved on the stack so the CPU stays
                 * awake after this ISR returns (lets the main loop run). */
                __bic_SR_register_on_exit(LPM3_bits);
            }
            break;
        default:
            break;
    }
}
