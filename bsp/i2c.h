/*
 * bsp/i2c.h — I2C master (eUSCI_B0), BSP layer.
 *
 * Phase 6 scope: master transmit only, used to drive the MCP4706 DAC.
 * 100 kHz standard mode on P1.6 (SDA) / P1.7 (SCL).
 */

#ifndef BSP_I2C_H_
#define BSP_I2C_H_

#include <stdint.h>

/*
 * i2c_init() — configure eUSCI_B0 as a 100 kHz I2C master on P1.6/P1.7.
 * Call after clock_init() (uses SMCLK).
 */
void i2c_init(void);

/*
 * i2c_write() — send `len` bytes to the 7-bit slave `address` as a single
 * START..STOP transaction. Blocking. len must be >= 1.
 */
void i2c_write(uint8_t address, const uint8_t *data, uint8_t len);

#endif /* BSP_I2C_H_ */
