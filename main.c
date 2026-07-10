#include <msp430.h>
#include "driverlib/MSP430FR5xx_6xx/driverlib.h"
#include "config.h"
#include "bsp/clock.h"

int main(void)
{
    /* Stop the watchdog first — otherwise it resets us after ~32 ms. */
    WDT_A_hold(WDT_A_BASE);

    /* Phase 1: bring up the crystals and route the system clocks.
     * After this call MCLK = SMCLK = 8 MHz, ACLK = 32.768 kHz.
     * clock_init() also calls PMM_unlockLPM5() to unlock the I/O.
     */
    clock_init();

    /* Simple visual verification of the clock:
     * P1.0 LED toggles every 0.5 s -> a 1 Hz blink.
     *
     * __delay_cycles() counts MCLK cycles. With MCLK = 8 MHz,
     * 4,000,000 cycles = 4e6 / 8e6 s = 0.5 s.
     * If the HF crystal is running correctly, the blink is exactly 1 Hz.
     * (A wrong/absent crystal would blink at a visibly different rate.)
     */
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);

    while (1)
    {
        GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
        __delay_cycles(4000000);   /* 0.5 s at 8 MHz MCLK */
    }
}
