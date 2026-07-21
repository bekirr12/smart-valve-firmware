/*
 * bsp/uart.h — the board's two UART links (BSP layer).
 *
 *   RS485  : eUSCI_A0 on P4.3/P4.4, 9600 8N1, half-duplex (DIR pin on P4.5)
 *            -> to the LoRaWAN gateway / center.
 *   HMI    : eUSCI_A2 on P7.0/P7.1, 115200 8N1, full-duplex 3.3 V TTL
 *            -> to the TY040HDL04NF screen module (see CLAUDE.md §2.2).
 *
 * RS485 send handles the transceiver direction pin; the HMI link is a plain
 * point-to-point TTL line and needs no direction control.
 */

#ifndef BSP_UART_H_
#define BSP_UART_H_

#include <stdint.h>

/* --- RS485 (eUSCI_A0) ------------------------------------------------ */

/* Configure eUSCI_A0 for 9600 8N1 and put the transceiver in receive mode.
 * Call after clock_init() (needs SMCLK). */
void uart_rs485_init(void);

/* Send raw bytes over RS485 (binary-safe), driving the direction pin high
 * for the transfer and back low when the last byte has shifted out. */
void uart_rs485_send(const uint8_t *data, uint16_t len);

/* Send a NUL-terminated string over RS485 (debug convenience). */
void uart_rs485_send_string(const char *str);

/* --- HMI screen (eUSCI_A2) ------------------------------------------- */

/* Configure eUSCI_A2 for 115200 8N1 on P7.0/P7.1. Call after clock_init(). */
void uart_hmi_init(void);

/* Send raw bytes to the screen module (binary-safe, no direction pin). */
void uart_hmi_send(const uint8_t *data, uint16_t len);

#endif /* BSP_UART_H_ */
