/*
 * drivers/rs485.c — Modbus-like framing + CRC16 implementation.
 *
 * See drivers/rs485.h. Pure logic, no hardware access.
 */

#include "config.h"
#include "drivers/rs485.h"

uint16_t rs485_crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    uint16_t i;

    for (i = 0; i < len; i++)
    {
        crc ^= (uint16_t)data[i];        /* fold in the next byte      */

        uint8_t bit;
        for (bit = 0; bit < 8; bit++)
        {
            if (crc & 0x0001)            /* LSB set -> shift and XOR   */
            {
                crc >>= 1;
                crc ^= 0xA001;           /* Modbus reversed polynomial */
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}

uint8_t rs485_build_frame(uint8_t *out,
                          uint8_t address,
                          uint8_t function,
                          const uint8_t *data,
                          uint8_t data_len)
{
    /* total = address + function + data + 2 CRC bytes */
    uint8_t total = (uint8_t)(2 + data_len + 2);
    if (total > RS485_MAX_FRAME)
        return 0;

    out[0] = address;
    out[1] = function;

    uint8_t i;
    for (i = 0; i < data_len; i++)
        out[2 + i] = data[i];

    /* CRC over everything before the CRC field. */
    uint16_t crc = rs485_crc16(out, (uint16_t)(2 + data_len));
    out[2 + data_len]     = (uint8_t)(crc & 0xFF);        /* low byte first */
    out[2 + data_len + 1] = (uint8_t)((crc >> 8) & 0xFF); /* then high byte */

    return total;
}

uint8_t rs485_check_frame(const uint8_t *frame, uint8_t len)
{
    if (len < 4)                 /* need at least addr+func+CRC(2) */
        return 0;

    /* CRC of the payload (everything except the trailing 2 CRC bytes). */
    uint16_t calc = rs485_crc16(frame, (uint16_t)(len - 2));

    /* CRC carried in the frame, low byte first. */
    uint16_t recv = (uint16_t)frame[len - 2] |
                    ((uint16_t)frame[len - 1] << 8);

    return (calc == recv) ? 1u : 0u;
}
