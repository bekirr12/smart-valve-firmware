#include <msp430.h>
#include "driverlib/MSP430FR5xx_6xx/driverlib.h"
#include "config.h"
#include "bsp/clock.h"
#include "bsp/gpio_init.h"
#include "bsp/uart.h"

int main(void)
{
    WDT_A_hold(WDT_A_BASE);

    clock_init();   /* Phase 1: MCLK/SMCLK = 8 MHz, ACLK = 32.768 kHz */
    gpio_init();    /* Phase 2: LEDs off, buttons as pull-up inputs   */
    uart_init();    /* Phase 3: RS485 UART, 9600 8N1                  */

    /* Phase 3 verification:
     * Once per second, send a line over RS485 and toggle LED1 so we can
     * see the board is alive. Connect an RS485-to-USB adapter and open a
     * serial terminal at 9600 baud, 8N1 to view the output.
     */
    while (1)
    {
        uart_send_string("Smart Valve: RS485 UART OK\r\n");
        GPIO_toggleOutputOnPin(LED1_PORT, LED1_PIN);
        __delay_cycles(8000000);   /* ~1 s at 8 MHz MCLK */
    }
}
