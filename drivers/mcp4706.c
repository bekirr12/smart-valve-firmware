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

/* MCP47X6 command bytes (see the datasheet command figures):
 *   WRITE_DAC  = [00 (cmd)][00 (PD normal)][0000] -> write volatile DAC reg
 *   CFG_VDD_1X = [100 (cmd)][00 VREF=VDD][00 PD][0 gain 1x] -> config bits
 */
#define MCP4706_CMD_WRITE_DAC   0x00
#define MCP4706_CFG_VDD_1X      0x80

void mcp4706_init(void)
{
    uint8_t cfg = MCP4706_CFG_VDD_1X;      /* VREF=VDD, normal power, gain 1x */
    i2c_write(MCP4706_I2C_ADDR, &cfg, 1);
}

void mcp4706_set_value(uint8_t value)
{
    uint8_t frame[2];
    frame[0] = MCP4706_CMD_WRITE_DAC;      /* write volatile DAC, normal power */
    frame[1] = value;                      /* 8-bit output value               */

    i2c_write(MCP4706_I2C_ADDR, frame, 2);
}

void mcp4706_set_percent(uint8_t percent)
{
    if (percent > 100)
        percent = 100;

    /* Map 0-100 % to 0-255. */
    mcp4706_set_value((uint8_t)(((uint16_t)percent * 255u) / 100u));
}
