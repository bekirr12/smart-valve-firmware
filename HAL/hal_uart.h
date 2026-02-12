/*
 * hal_uart.h
 * Smart Valve Project
 * Phase: 4 (Communication Layer)
 * Purpose: Low-Level RS485 Driver using EUSCI_A1
 */

#ifndef HAL_HAL_UART_H_
#define HAL_HAL_UART_H_

#include <stdint.h>
#include <stdbool.h>

// Buffer size slightly larger than expected Modbus frame (8 bytes)
#define UART_RX_BUFFER_SIZE 16

/*
 * Initializes the UART module for RS485 communication.
 * Settings: 9600 Baud, 8N1.
 * Configures RS485 Enable pin (P4.4) and enables RX Interrupts.
 */
void HAL_UART_Init(void);

/*
 * Sends a raw byte array over RS485.
 * Automatically handles the DE/RE pin:
 * 1. Sets DE HIGH (Transmit Mode).
 * 2. Sends data.
 * 3. Waits for completion.
 * 4. Sets DE LOW (Receive Mode).
 */
void HAL_UART_TxBuffer(uint8_t* buffer, uint16_t length);

/*
 * Checks if a complete packet has been received.
 * @return: true if data is available in the internal buffer.
 */
bool HAL_UART_IsDataReady(void);

/*
 * Retrieves the received data from the internal buffer.
 * @param dest_buffer: Pointer to the destination array.
 * @return: Number of bytes copied.
 */
uint16_t HAL_UART_GetData(uint8_t* dest_buffer);

#endif /* HAL_HAL_UART_H_ */