#include <msp430.h>
#include "driverlib/MSP430FR5xx_6xx/driverlib.h"
#include "config.h"
#include "bsp/clock.h"
#include "bsp/gpio_init.h"

/* Helper: return 1 if an active-low button is pressed, else 0. */
static uint8_t button_pressed(uint8_t port, uint16_t pin)
{
    /* Pull-up input idles HIGH; pressed pulls it LOW. */
    return (GPIO_getInputPinValue(port, pin) == GPIO_INPUT_PIN_LOW) ? 1u : 0u;
}

/* Helper: drive an active-high LED on/off. */
static void led_set(uint8_t port, uint16_t pin, uint8_t on)
{
    if (on)
        GPIO_setOutputHighOnPin(port, pin);
    else
        GPIO_setOutputLowOnPin(port, pin);
}

int main(void)
{
    WDT_A_hold(WDT_A_BASE);

    clock_init();   /* Phase 1: MCLK/SMCLK = 8 MHz, ACLK = 32.768 kHz */
    gpio_init();    /* Phase 2: all pins to safe states               */

    /* Phase 2 verification:
     * Each of three buttons lights one LED while held down.
     *   BTN_UP     -> LED1
     *   BTN_SELECT -> LED2
     *   BTN_DOWN   -> LED3
     * This exercises both button reading (pull-up inputs) and LED driving.
     * (LEFT/RIGHT can be watched in the debugger via their port input
     *  registers, or remapped here if you want to test them on the LEDs.)
     */
    while (1)
    {
        led_set(LED1_PORT, LED1_PIN, button_pressed(BTN_UP_PORT,     BTN_UP_PIN));
        led_set(LED2_PORT, LED2_PIN, button_pressed(BTN_SELECT_PORT, BTN_SELECT_PIN));
        led_set(LED3_PORT, LED3_PIN, button_pressed(BTN_DOWN_PORT,   BTN_DOWN_PIN));
    }
}
