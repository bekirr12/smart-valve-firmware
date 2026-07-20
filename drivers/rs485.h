/*
 * drivers/rs485.h — Modbus-like framing + CRC16 (driver layer).
 *
 * Phase 9a scope: the pure framing/checksum logic, independent of the
 * physical UART. Frame layout:
 *   [address][function][data...][CRC16_lo][CRC16_hi]
 * The CRC covers address + function + data and is appended low byte first
 * (Modbus convention). This layer is fully unit-testable without hardware.
 *
 * The actual send/receive over the RS485 UART (bsp/uart) and the register
 * map / command handling (app/comm_protocol) sit above this.
 */

#ifndef DRIVERS_RS485_H_
#define DRIVERS_RS485_H_

#include <stdint.h>

/*
 * rs485_crc16() — Modbus CRC16 (poly 0xA001, init 0xFFFF) over `len` bytes.
 */
uint16_t rs485_crc16(const uint8_t *data, uint16_t len);

/*
 * rs485_build_frame() — assemble a frame into `out`:
 *   out = [address][function][data...][CRC_lo][CRC_hi]
 * Returns the total frame length (2 + data_len + 2), or 0 if it would
 * exceed RS485_MAX_FRAME. `out` must hold at least RS485_MAX_FRAME bytes.
 */
uint8_t rs485_build_frame(uint8_t *out,
                          uint8_t address,
                          uint8_t function,
                          const uint8_t *data,
                          uint8_t data_len);

/*
 * rs485_check_frame() — verify a received frame's trailing CRC16.
 * Returns 1 if the CRC matches (frame intact), 0 otherwise. `len` is the
 * full frame length including the 2 CRC bytes.
 */
uint8_t rs485_check_frame(const uint8_t *frame, uint8_t len);

#endif /* DRIVERS_RS485_H_ */
