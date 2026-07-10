/*
 * bsp/uart.c — RS485 UART (eUSCI_A0) implementation, transmit path.
 *
 * See bsp/uart.h. Baud generator values live in config.h.
 */

#include <msp430.h>
#include "driverlib/MSP430FR5xx_6xx/driverlib.h"
#include "config.h"
#include "bsp/uart.h"

void uart_init(void)
{
    /* --- Direction-enable pin: output, start in RX (listen) mode ---- */
    GPIO_setAsOutputPin(RS485_EN_PORT, RS485_EN_PIN);
    GPIO_setOutputLowOnPin(RS485_EN_PORT, RS485_EN_PIN);   /* RX */

    /* --- Route P4.3/P4.4 to the eUSCI_A0 UART function --------------
     * UCA0TXD / UCA0RXD are the primary module function on these pins.
     */
    GPIO_setAsPeripheralModuleFunctionOutputPin(
        RS485_TX_PORT, RS485_TX_PIN, GPIO_PRIMARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionInputPin(
        RS485_RX_PORT, RS485_RX_PIN, GPIO_PRIMARY_MODULE_FUNCTION);

    /* --- Configure the UART: 9600 baud, 8 data bits, no parity, 1 stop
     * clocked from SMCLK (8 MHz), using oversampling for a cleaner baud.
     */
    EUSCI_A_UART_initParam param = {0};
    param.selectClockSource   = EUSCI_A_UART_CLOCKSOURCE_SMCLK;
    param.clockPrescalar      = RS485_BR_PRESCALAR;   /* UCBRx  */
    param.firstModReg         = RS485_BR_FIRSTMOD;    /* UCBRFx */
    param.secondModReg        = RS485_BR_SECONDMOD;   /* UCBRSx */
    param.parity              = EUSCI_A_UART_NO_PARITY;
    param.msborLsbFirst       = EUSCI_A_UART_LSB_FIRST;
    param.numberofStopBits    = EUSCI_A_UART_ONE_STOP_BIT;
    param.uartMode            = EUSCI_A_UART_MODE;
    param.overSampling        = EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION;

    EUSCI_A_UART_init(EUSCI_A0_BASE, &param);
    EUSCI_A_UART_enable(EUSCI_A0_BASE);
}

/* Send one byte, waiting until the TX buffer can accept it. */
static void uart_send_byte(uint8_t b)
{
    /* Wait until the transmit buffer is empty (TXIFG set). */
    while (!EUSCI_A_UART_getInterruptStatus(
                EUSCI_A0_BASE, EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG))
        ;
    EUSCI_A_UART_transmitData(EUSCI_A0_BASE, b);
}

void uart_send_string(const char *str)
{
    /* Switch the transceiver to TX (drive the bus). */
    GPIO_setOutputHighOnPin(RS485_EN_PORT, RS485_EN_PIN);

    while (*str != '\0')
        uart_send_byte((uint8_t)*str++);

    /* Wait until the last byte has completely left the shift register
     * before releasing the bus — dropping EN too early truncates it.
     */
    while (EUSCI_A_UART_queryStatusFlags(EUSCI_A0_BASE, EUSCI_A_UART_BUSY))
        ;

    /* Back to RX (listen). */
    GPIO_setOutputLowOnPin(RS485_EN_PORT, RS485_EN_PIN);
}
