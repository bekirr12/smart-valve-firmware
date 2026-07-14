#include <msp430.h>
#include "driverlib/MSP430FR5xx_6xx/driverlib.h"
#include "config.h"
#include "bsp/clock.h"
#include "bsp/gpio_init.h"
#include "bsp/adc.h"
#include "drivers/sensors.h"

/*
 * Debug-view globals (temporary bring-up experiment).
 * Marked volatile so the compiler keeps them in memory and the CCS
 * Expressions/Variables view can always read the latest value.
 *   g_batt_raw      - battery raw 12-bit ADC code (0..4095)
 *   g_batt_voltage  - converted battery voltage, volts
 *   g_panel_raw     - panel raw 12-bit ADC code (0..4095)
 *   g_panel_voltage - converted panel voltage, volts
 */
volatile uint16_t g_batt_raw;
volatile float    g_batt_voltage;
volatile uint16_t g_panel_raw;
volatile float    g_panel_voltage;

int main(void)
{
    WDT_A_hold(WDT_A_BASE);

    clock_init();   /* Phase 1: clocks          */
    gpio_init();    /* Phase 2: LEDs, buttons   */
    adc_init();     /* Phase 4: ADC + reference */

    /* Read the battery ADC into globals so it can be watched in the CCS
     * debugger. Add g_batt_raw and g_batt_voltage to the Expressions view;
     * either breakpoint on the delay line, or enable Continuous Refresh and
     * just Run. LED1 toggles as a sign of life.
     */
    while (1)
    {
        g_batt_raw      = adc_read_raw(ADC_BATT_V_CH);
        g_batt_voltage  = sensor_battery_voltage();
        g_panel_raw     = adc_read_raw(ADC_PANEL_V_CH);
        g_panel_voltage = sensor_panel_voltage();

        GPIO_toggleOutputOnPin(LED1_PORT, LED1_PIN);
        __delay_cycles(4000000);   /* ~0.5 s at 8 MHz MCLK */
    }
}
