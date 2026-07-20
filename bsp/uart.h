/*
 * bsp/uart.h — RS485 UART (eUSCI_A0), BSP layer.
 *
 * Phase 3 scope: transmit only, used as the firmware's debug output.
 * Receive and the modbus-like protocol come later (Phase 9).
 *
 * The RS485 transceiver is half-duplex, so uart_send_string() handles the
 * direction-enable pin: it drives EN high (TX) before sending and back low
 * (RX) after the final byte has fully shifted out.
 */

#ifndef BSP_UART_H_
#define BSP_UART_H_

#include <stdint.h>

/*
 * uart_init() — configure eUSCI_A0 for 9600 baud 8N1 on P4.3/P4.4, and set
 * the RS485 EN pin as an output in receive mode. Call after clock_init()
 * (needs SMCLK running).
 */
void uart_init(void);

/*
 * uart_send_buffer() — transmit `len` raw bytes over RS485 (binary-safe,
 * so it can carry frames containing zero bytes), managing the direction-
 * enable pin around the transfer.
 */
void uart_send_buffer(const uint8_t *data, uint16_t len);

/*
 * uart_send_string() — transmit a NUL-terminated string over RS485,
 * managing the direction-enable pin around the transfer.
 */
void uart_send_string(const char *str);

#endif /* BSP_UART_H_ */
