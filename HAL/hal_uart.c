/*
 * hal_uart.c
 * Implementation of RS485 Driver
 * Hardware: MSP430FR6047 EUSCI_A1
 */

#include <msp430.h>
#include <driverlib.h>
#include "hal_board.h"
#include "hal_uart.h"

// Internal buffer variables
static volatile uint8_t rx_buffer[UART_RX_BUFFER_SIZE];
static volatile uint16_t rx_index = 0;
static volatile bool data_ready = false;

void HAL_UART_Init(void) {
    // 1. Configure GPIO for UART (TX, RX)
    GPIO_setAsPeripheralModuleFunctionInputPin(
        RS485_TX_PORT, RS485_TX_PIN, GPIO_PRIMARY_MODULE_FUNCTION
    );
    GPIO_setAsPeripheralModuleFunctionInputPin(
        RS485_RX_PORT, RS485_RX_PIN, GPIO_PRIMARY_MODULE_FUNCTION
    );

    // 2. Configure RS485 Enable Pin (DE/RE)
    GPIO_setAsOutputPin(RS485_EN_PORT, RS485_EN_PIN);
    GPIO_setOutputLowOnPin(RS485_EN_PORT, RS485_EN_PIN); // Default: Receive Mode

    // 3. Initialize EUSCI_A1 (9600 Baud @ 8MHz)
    // Calc: 8,000,000 / 9600 = 833.333
    // UCBRx = 52, UCBRFx = 1, UCBRSx = 0x49, OS16 = 1
    EUSCI_A_UART_initParam param = {0};
    param.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK;
    param.clockPrescalar = 52;
    param.firstModReg = 1;
    param.secondModReg = 0x49;
    param.parity = EUSCI_A_UART_NO_PARITY;
    param.msborLsbFirst = EUSCI_A_UART_LSB_FIRST;
    param.numberofStopBits = EUSCI_A_UART_ONE_STOP_BIT;
    param.uartMode = EUSCI_A_UART_MODE;
    param.overSampling = EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION;

    if (EUSCI_A_UART_init(EUSCI_A1_BASE, &param) == STATUS_FAIL) {
        return; // Initialization failed
    }

    EUSCI_A_UART_enable(EUSCI_A1_BASE);
    
    // Enable Receive Interrupt
    EUSCI_A_UART_clearInterrupt(EUSCI_A1_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
    EUSCI_A_UART_enableInterrupt(EUSCI_A1_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
}

void HAL_UART_TxBuffer(uint8_t* buffer, uint16_t length) {
    // 1. Enter Transmit Mode (DE = HIGH)
    GPIO_setOutputHighOnPin(RS485_EN_PORT, RS485_EN_PIN);
    
    // Short delay to ensure transceiver stability
    __delay_cycles(160); // approx 20us

    // 2. Transmit Data
    for (uint16_t i = 0; i < length; i++) {
        EUSCI_A_UART_transmitData(EUSCI_A1_BASE, buffer[i]);
        
        // Wait until buffer is ready for next byte
        while (!EUSCI_A_UART_getInterruptStatus(EUSCI_A1_BASE, EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG));
    }

    // 3. Wait for the last byte to fully leave the shift register
    while (EUSCI_A_UART_queryStatusFlags(EUSCI_A1_BASE, EUSCI_A_UART_BUSY));

    // 4. Return to Receive Mode (DE = LOW)
    GPIO_setOutputLowOnPin(RS485_EN_PORT, RS485_EN_PIN);
}

bool HAL_UART_IsDataReady(void) {
    return data_ready;
}

uint16_t HAL_UART_GetData(uint8_t* dest_buffer) {
    if (!data_ready) return 0;

    // Critical Section: Disable RX interrupt while reading buffer
    EUSCI_A_UART_disableInterrupt(EUSCI_A1_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);

    uint16_t count = rx_index;
    for (uint16_t i = 0; i < count; i++) {
        dest_buffer[i] = rx_buffer[i];
    }

    // Reset Buffer
    rx_index = 0;
    data_ready = false;

    EUSCI_A_UART_enableInterrupt(EUSCI_A1_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
    return count;
}

// --- Interrupt Service Routine ---
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void) {
    switch (__even_in_range(UCA1IV, USCI_UART_UCTXCPTIFG)) {
        case USCI_UART_UCRXIFG: // Receive Interrupt
        {
            uint8_t received_byte = EUSCI_A_UART_receiveData(EUSCI_A1_BASE);
            
            // Basic buffering logic
            if (rx_index < UART_RX_BUFFER_SIZE) {
                rx_buffer[rx_index++] = received_byte;
                
                // Simple Protocol: Expecting 8 bytes for Modbus frame
                // In a robust system, a timer timeout should also be used.
                if (rx_index >= 8) {
                    data_ready = true;
                }
            }
            break;
        }
        default: break;
    }
}