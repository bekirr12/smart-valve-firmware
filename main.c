#include <msp430.h>
#include "driverlib/MSP430FR5xx_6xx/driverlib.h"
#include "config.h"
#include "bsp/clock.h"
#include "bsp/gpio_init.h"
#include "bsp/i2c.h"
#include "drivers/mcp4706.h"

/* Current DAC code, also visible in the CCS debugger. */
volatile uint8_t g_dac_value;

int main(void)
{
    WDT_A_hold(WDT_A_BASE);

    clock_init();   /* Phase 1: clocks        */
    gpio_init();    /* Phase 2: LEDs, buttons */
    i2c_init();     /* Phase 6: I2C master    */
    mcp4706_init(); /* force config: VREF=VDD, normal, 1x */

    /* Phase 6 verification:
     * Step the DAC through five codes, holding each for ~3 s. The cycle
     * starts at 0 (first 3 s read 0 V, expected), then climbs. Measure the
     * MCP4706 VOUT pin against Vout = 3.3 * value / 256:
     *   0   -> 0.00 V
     *   64  -> 0.83 V
     *   128 -> 1.65 V
     *   192 -> 2.48 V
     *   255 -> 3.29 V
     * Watch g_dac_value in the debugger to match each reading. LED1 toggles
     * at each step.
     */
    static const uint8_t steps[] = {0, 64, 128, 192, 255};
    uint8_t i = 0;

    while (1)
    {
        g_dac_value = steps[i];
        mcp4706_set_value(g_dac_value);

        GPIO_toggleOutputOnPin(LED1_PORT, LED1_PIN);
        __delay_cycles(24000000);   /* ~3 s at 8 MHz MCLK */

        i++;
        if (i >= sizeof(steps))
            i = 0;
    }
}
