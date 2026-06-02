#include <msp430.h>
#include "driverlib/MSP430FR5xx_6xx/driverlib.h"

int main(void)
{
    WDT_A_hold(WDT_A_BASE);

    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);

    PMM_unlockLPM5();

    while (1)
    {
        GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);

        __delay_cycles(500000);
    }
}
