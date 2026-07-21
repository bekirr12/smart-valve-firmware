/*
 * bsp/uart.c — RS485 (eUSCI_A0) and HMI (eUSCI_A2) UART links.
 *
 * See bsp/uart.h. Baud generator values live in config.h; each is a matched
 * set (UCBRx / UCBRFx / UCBRSx) for its baud rate from the 8 MHz SMCLK.
 */

#include <msp430.h>
#include "driverlib/MSP430FR5xx_6xx/driverlib.h"
#include "config.h"
#include "bsp/uart.h"

/* Shared helper: block until the given eUSCI_A can accept a byte, then
 * write it. */
static void uart_put(uint16_t base, uint8_t b)
{
    while (!EUSCI_A_UART_getInterruptStatus(
                base, EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG))
        ;
    EUSCI_A_UART_transmitData(base, b);
}

/* Shared helper: wait until the last byte has fully left the shift register. */
static void uart_wait_idle(uint16_t base)
{
    while (EUSCI_A_UART_queryStatusFlags(base, EUSCI_A_UART_BUSY))
        ;
}

/* ===================== RS485 — eUSCI_A0 ============================== */

void uart_rs485_init(void)
{
    /* Direction-enable pin: output, start in RX (listen) mode. */
    GPIO_setAsOutputPin(RS485_EN_PORT, RS485_EN_PIN);
    GPIO_setOutputLowOnPin(RS485_EN_PORT, RS485_EN_PIN);

    /* UCA0TXD / UCA0RXD are the primary module function on P4.3/P4.4. */
    GPIO_setAsPeripheralModuleFunctionOutputPin(
        RS485_TX_PORT, RS485_TX_PIN, GPIO_PRIMARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionInputPin(
        RS485_RX_PORT, RS485_RX_PIN, GPIO_PRIMARY_MODULE_FUNCTION);

    EUSCI_A_UART_initParam param = {0};
    param.selectClockSource   = EUSCI_A_UART_CLOCKSOURCE_SMCLK;
    param.clockPrescalar      = RS485_BR_PRESCALAR;
    param.firstModReg         = RS485_BR_FIRSTMOD;
    param.secondModReg        = RS485_BR_SECONDMOD;
    param.parity              = EUSCI_A_UART_NO_PARITY;
    param.msborLsbFirst       = EUSCI_A_UART_LSB_FIRST;
    param.numberofStopBits    = EUSCI_A_UART_ONE_STOP_BIT;
    param.uartMode            = EUSCI_A_UART_MODE;
    param.overSampling        = EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION;

    EUSCI_A_UART_init(EUSCI_A0_BASE, &param);
    EUSCI_A_UART_enable(EUSCI_A0_BASE);
}

void uart_rs485_send(const uint8_t *data, uint16_t len)
{
    /* Drive the bus. */
    GPIO_setOutputHighOnPin(RS485_EN_PORT, RS485_EN_PIN);

    uint16_t i;
    for (i = 0; i < len; i++)
        uart_put(EUSCI_A0_BASE, data[i]);

    /* Releasing the bus before the last byte is out would truncate it. */
    uart_wait_idle(EUSCI_A0_BASE);

    /* Back to listening. */
    GPIO_setOutputLowOnPin(RS485_EN_PORT, RS485_EN_PIN);
}

void uart_rs485_send_string(const char *str)
{
    uint16_t len = 0;
    while (str[len] != '\0')
        len++;
    uart_rs485_send((const uint8_t *)str, len);
}

/* ===================== HMI screen — eUSCI_A2 ========================= */

void uart_hmi_init(void)
{
    /* UCA2TXD / UCA2RXD are the primary module function on P7.0/P7.1
     * (datasheet Table 9-35). */
    GPIO_setAsPeripheralModuleFunctionOutputPin(
        HMI_TX_PORT, HMI_TX_PIN, HMI_PIN_MUX);
    GPIO_setAsPeripheralModuleFunctionInputPin(
        HMI_RX_PORT, HMI_RX_PIN, HMI_PIN_MUX);

    EUSCI_A_UART_initParam param = {0};
    param.selectClockSource   = EUSCI_A_UART_CLOCKSOURCE_SMCLK;
    param.clockPrescalar      = HMI_BR_PRESCALAR;
    param.firstModReg         = HMI_BR_FIRSTMOD;
    param.secondModReg        = HMI_BR_SECONDMOD;
    param.parity              = EUSCI_A_UART_NO_PARITY;
    param.msborLsbFirst       = EUSCI_A_UART_LSB_FIRST;
    param.numberofStopBits    = EUSCI_A_UART_ONE_STOP_BIT;
    param.uartMode            = EUSCI_A_UART_MODE;
    param.overSampling        = EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION;

    EUSCI_A_UART_init(EUSCI_A2_BASE, &param);
    EUSCI_A_UART_enable(EUSCI_A2_BASE);
}

void uart_hmi_send(const uint8_t *data, uint16_t len)
{
    uint16_t i;
    for (i = 0; i < len; i++)
        uart_put(EUSCI_A2_BASE, data[i]);

    uart_wait_idle(EUSCI_A2_BASE);
}
