/*
 * bsp/i2c.c — I2C master (eUSCI_B0) implementation.
 *
 * See bsp/i2c.h. Transmit-only master at 100 kHz. The transaction helpers
 * build one START..STOP frame per call.
 */

#include <msp430.h>
#include "driverlib/MSP430FR5xx_6xx/driverlib.h"
#include "config.h"
#include "bsp/i2c.h"

void i2c_init(void)
{
    /* Route P1.6/P1.7 to the eUSCI_B0 I2C function. SDA/SCL are open-drain
     * signals pulled up by external resistors (4.7k on the board).
     */
    GPIO_setAsPeripheralModuleFunctionInputPin(
        I2C_SDA_PORT, I2C_SDA_PIN, I2C_PIN_MUX);
    GPIO_setAsPeripheralModuleFunctionInputPin(
        I2C_SCL_PORT, I2C_SCL_PIN, I2C_PIN_MUX);

    EUSCI_B_I2C_initMasterParam param = {0};
    param.selectClockSource   = EUSCI_B_I2C_CLOCKSOURCE_SMCLK;  /* 8 MHz */
    param.i2cClk              = CONFIG_SMCLK_FREQ_HZ;
    param.dataRate            = I2C_DATARATE;                    /* 100 kHz */
    param.byteCounterThreshold = 0;
    param.autoSTOPGeneration  = EUSCI_B_I2C_NO_AUTO_STOP;
    EUSCI_B_I2C_initMaster(EUSCI_B0_BASE, &param);

    EUSCI_B_I2C_enable(EUSCI_B0_BASE);
}

void i2c_write(uint8_t address, const uint8_t *data, uint8_t len)
{
    if (len == 0)
        return;

    EUSCI_B_I2C_setSlaveAddress(EUSCI_B0_BASE, address);
    EUSCI_B_I2C_setMode(EUSCI_B0_BASE, EUSCI_B_I2C_TRANSMIT_MODE);

    /* Wait for the bus to be free before starting. */
    while (EUSCI_B_I2C_isBusBusy(EUSCI_B0_BASE))
        ;

    if (len == 1)
    {
        /* START + address + one byte + STOP. */
        EUSCI_B_I2C_masterSendSingleByte(EUSCI_B0_BASE, data[0]);
        return;
    }

    /* START + address + first byte. */
    EUSCI_B_I2C_masterSendMultiByteStart(EUSCI_B0_BASE, data[0]);

    /* Middle bytes (if any). */
    uint8_t i;
    for (i = 1; i < len - 1; i++)
        EUSCI_B_I2C_masterSendMultiByteNext(EUSCI_B0_BASE, data[i]);

    /* Last byte + STOP. */
    EUSCI_B_I2C_masterSendMultiByteFinish(EUSCI_B0_BASE, data[len - 1]);

    /* MultiByteFinish leaves UCTXIFG set. If we don't clear it, the next
     * MultiByteStart's "wait for TXIFG" returns immediately and writes the
     * first data byte before the address is sent, corrupting the transfer
     * (so only the very first transaction after init would work). Clear it
     * here, mirroring what masterSendSingleByte does internally.
     */
    EUSCI_B_I2C_clearInterrupt(EUSCI_B0_BASE, EUSCI_B_I2C_TRANSMIT_INTERRUPT0);
}
