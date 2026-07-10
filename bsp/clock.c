/*
 * bsp/clock.c — Clock system setup implementation.
 *
 * See bsp/clock.h for the resulting clock configuration.
 *
 * Bring-up order matters on FRAM devices:
 *   1. Point the crystal pins (Port J) at their crystal function.
 *   2. Unlock the I/O (PMM_unlockLPM5) — after power-up the FRAM device
 *      holds all pins in a locked high-impedance state; crystals cannot
 *      start until this lock is cleared.
 *   3. Tell driverlib the crystal frequencies.
 *   4. Start the crystals (blocks until each is stable).
 *   5. Route MCLK/SMCLK to HFXT and ACLK to LFXT.
 */

#include <msp430.h>
#include "driverlib/MSP430FR5xx_6xx/driverlib.h"
#include "config.h"
#include "bsp/clock.h"

void clock_init(void)
{
    /* --- 1. Configure the crystal pins on Port J ---------------------
     * LFXIN / LFXOUT = PJ.4 / PJ.5   (32.768 kHz crystal)
     * HFXIN / HFXOUT = PJ.6 / PJ.7   (8 MHz crystal)
     *
     * Selecting the "primary module function" on these pins connects them
     * to the oscillator instead of using them as plain GPIO.
     */
    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_PJ,
        GPIO_PIN4 | GPIO_PIN5,          /* LFXIN, LFXOUT */
        GPIO_PRIMARY_MODULE_FUNCTION);

    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_PJ,
        GPIO_PIN6 | GPIO_PIN7,          /* HFXIN, HFXOUT */
        GPIO_PRIMARY_MODULE_FUNCTION);

    /* --- 2. Unlock the I/O held locked since power-up ----------------
     * Required before the crystals can drive their pins. Also clears the
     * LOCKLPM5 bit so all our later GPIO settings take effect.
     */
    PMM_unlockLPM5();

    /* --- 3. Tell driverlib the external crystal frequencies ----------
     * driverlib needs these to configure drive strength correctly and to
     * report values from CS_getMCLK / CS_getSMCLK / CS_getACLK later.
     */
    CS_setExternalClockSource(CONFIG_LFXT_FREQ_HZ, CONFIG_HFXT_FREQ_HZ);

    /* --- 4. Start the crystals --------------------------------------
     * These blocking calls turn on each oscillator and wait, clearing the
     * oscillator-fault flags, until the crystal is stable.
     *
     * LF crystal: 32.768 kHz watch crystal -> lowest drive strength.
     * HF crystal: 8 MHz -> the 4-8 MHz drive setting.
     */
    CS_turnOnLFXT(CS_LFXT_DRIVE_0);
    CS_turnOnHFXT(CS_HFXT_DRIVE_4MHZ_8MHZ);

    /* --- 5. Route the system clocks ---------------------------------
     * MCLK  <- HFXT (8 MHz)      : CPU
     * SMCLK <- HFXT (8 MHz)      : UART / I2C / ADC / timers
     * ACLK  <- LFXT (32.768 kHz) : RTC / low-power wakeup
     * Divider /1 = use the source frequency directly.
     */
    CS_initClockSignal(CS_MCLK,  CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_SMCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_ACLK,  CS_LFXTCLK_SELECT, CS_CLOCK_DIVIDER_1);
}
