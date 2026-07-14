#include <msp430.h>
#include <stdio.h>
#include "driverlib/MSP430FR5xx_6xx/driverlib.h"
#include "config.h"
#include "bsp/clock.h"
#include "bsp/gpio_init.h"
#include "bsp/uart.h"
#include "bsp/adc.h"
#include "drivers/sensors.h"

/*
 * Print "label: X.XXX unit" over UART. We format the float manually with
 * integer math (whole part + 3 decimals) so we don't depend on printf's
 * heavy floating-point %f support.
 */
static void send_value(const char *label, float v, const char *unit)
{
    char buf[48];
    int neg = (v < 0.0f);
    if (neg)
        v = -v;

    int whole = (int)v;
    int milli = (int)((v - (float)whole) * 1000.0f + 0.5f);
    if (milli >= 1000) {          /* rounding carried over */
        whole += 1;
        milli -= 1000;
    }

    sprintf(buf, "%s: %s%d.%03d %s\r\n",
            label, neg ? "-" : "", whole, milli, unit);
    uart_send_string(buf);
}

int main(void)
{
    WDT_A_hold(WDT_A_BASE);

    clock_init();   /* Phase 1: clocks           */
    gpio_init();    /* Phase 2: LEDs, buttons    */
    uart_init();    /* Phase 3: RS485 UART       */
    adc_init();     /* Phase 4: ADC + reference  */

    /* Phase 4 verification:
     * Once per second, read all five sensors and print them over RS485.
     * Open a serial terminal at 9600 8N1 to view. With nothing connected
     * to the ADC inputs the numbers will be noise/near-zero — the point is
     * to confirm the read+convert+print pipeline works. Real values appear
     * once the MPPT board is connected.
     */
    while (1)
    {
        uart_send_string("--- Sensors ---\r\n");
        send_value("Panel V", sensor_panel_voltage(),   "V");
        send_value("Batt  V", sensor_battery_voltage(), "V");
        send_value("Panel I", sensor_panel_current(),   "A");
        send_value("Batt  I", sensor_battery_current(), "A");
        send_value("Motor I", sensor_motor_current(),   "A");

        GPIO_toggleOutputOnPin(LED1_PORT, LED1_PIN);
        __delay_cycles(8000000);   /* ~1 s at 8 MHz MCLK */
    }
}
