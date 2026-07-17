/*
 * bsp/power.c — Low-power mode entry implementation.
 *
 * See bsp/power.h. Writing LPM3_bits expresses "deepest possible sleep";
 * the clock-request system decides the mode actually reached.
 */

#include <msp430.h>
#include "bsp/power.h"

void power_enter_sleep(void)
{
    /* Set the LPM3 mode bits and the global interrupt enable in one step,
     * then halt. The CPU resumes right here when an interrupt wakes it.
     */
    __bis_SR_register(LPM3_bits | GIE);
}
