#include <msp430.h>
#include "driverlib/MSP430FR5xx_6xx/driverlib.h"
#include "config.h"
#include "bsp/clock.h"
#include "bsp/gpio_init.h"
#include "bsp/timer.h"
#include "bsp/power.h"

int main(void)
{
    WDT_A_hold(WDT_A_BASE);

    clock_init();   /* Phase 1: clocks (incl. LF crystal -> ACLK for RTC) */
    gpio_init();    /* Phase 2: LEDs, buttons                             */
    rtc_init();     /* Phase 5: 1 Hz RTC tick -> wake every interval      */

    /* Phase 5 verification:
     * Sleep in LPM3 and let the RTC wake the CPU every MEASURE_INTERVAL_S
     * seconds; toggle LED2 on each wake. Between wakes the current draw
     * should drop to the LPM3 range.
     *
     * The check-then-sleep below runs with interrupts disabled to avoid the
     * classic LPM race: if the RTC tick landed between "is it due?" and "go
     * to sleep", a naive version could sleep through it. Because
     * power_enter_sleep() sets the LPM bits and GIE in a single instruction,
     * an interrupt pending during this window is serviced the instant we
     * sleep -- waking us right back up -- so no wake-up is ever lost.
     */
    while (1)
    {
        __disable_interrupt();
        if (rtc_measurement_due())
        {
            __enable_interrupt();
            rtc_clear_measurement_due();
            GPIO_toggleOutputOnPin(LED2_PORT, LED2_PIN);
        }
        else
        {
            power_enter_sleep();   /* LPM3 + GIE; wakes on the RTC interrupt */
        }
    }
}
