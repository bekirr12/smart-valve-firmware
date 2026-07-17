/*
 * drivers/mcp4706.h — MCP4706 8-bit I2C DAC (driver layer).
 *
 * Sets the motor speed reference. The DAC's 0-3.3 V output is scaled to
 * 0-10 V by a hardware opamp. Value 0-255:  Vout = 3.3 * value / 256.
 */

#ifndef DRIVERS_MCP4706_H_
#define DRIVERS_MCP4706_H_

#include <stdint.h>

/*
 * mcp4706_init() — force the volatile configuration to a known-good state:
 * VREF = VDD, normal power (not powered down), gain 1x. This overrides
 * whatever was loaded from EEPROM at power-up, so the DAC output is
 * referenced to VDD (= 3.3 V) regardless of the shipped defaults.
 * Call after i2c_init().
 */
void mcp4706_init(void);

/*
 * mcp4706_set_value() — write the raw 8-bit DAC value (0-255) to the
 * volatile DAC register (normal power, gain 1x).
 */
void mcp4706_set_value(uint8_t value);

/*
 * mcp4706_set_percent() — convenience wrapper: 0-100 % mapped to 0-255.
 * Used for motor speed (percent of full scale). Values above 100 are
 * clamped.
 */
void mcp4706_set_percent(uint8_t percent);

#endif /* DRIVERS_MCP4706_H_ */
