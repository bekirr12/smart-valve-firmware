/*
 * drivers/mcp4706.c — MCP4706 8-bit I2C DAC implementation.
 *
 * "Write Volatile DAC Register" command (MCP47X6 datasheet Table 6-2):
 * after the slave address, two bytes are sent:
 *   byte 1 = command 0x00 : C2C1C0 = 000 (write volatile DAC),
 *                           PD1:PD0 = 00 (normal operation)
 *   byte 2 = the 8-bit DAC value (D7..D0)
 * The POR-default config (VRL = VDD, gain 1x) already gives
 *   Vout = 3.3 V * value / 256, so no configuration write is needed.
 */

#include "config.h"
#include "bsp/i2c.h"
#include "drivers/mcp4706.h"

void mcp4706_init(void)
{
    /* "Write Volatile Configuration Bits" command (C2C1C0 = 100):
     *   byte = [1 0 0][VREF1 VREF0][PD1 PD0][G]
     *        = [1 0 0][0 0      ][0 0    ][0]  = 0x80
     *   -> VREF = VDD, normal power, gain 1x.
     */
    uint8_t cfg = 0x80;
    i2c_write(MCP4706_I2C_ADDR, &cfg, 1);
}

void mcp4706_set_value(uint8_t value)
{
    uint8_t frame[2];
    frame[0] = 0x00;      /* write volatile DAC register, normal power */
    frame[1] = value;     /* 8-bit output value                        */

    i2c_write(MCP4706_I2C_ADDR, frame, 2);
}

void mcp4706_set_percent(uint8_t percent)
{
    if (percent > 100)
        percent = 100;

    /* Map 0-100 % to 0-255. */
    mcp4706_set_value((uint8_t)(((uint16_t)percent * 255u) / 100u));
}
