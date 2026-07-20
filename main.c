#include <msp430.h>
#include "driverlib/MSP430FR5xx_6xx/driverlib.h"
#include "app/state_machine.h"

int main(void)
{
    WDT_A_hold(WDT_A_BASE);   /* stop the watchdog before the long crystal
                               * startup in clock_init() */

    state_machine_run();      /* init + control loop; never returns */

    return 0;
}
