#include <msp430.h>
#include "driverlib/MSP430FR5xx_6xx/driverlib.h"
#include "config.h"
#include "bsp/clock.h"
#include "bsp/gpio_init.h"
#include "drivers/rs485.h"

/* Phase 9a debug-view globals (watch these in the CCS Expressions view):
 *   g_crc_check    - CRC16 of "123456789"; the CRC-16/MODBUS standard check
 *                    value is 0x4B37, so this must read 0x4B37.
 *   g_frame_len    - length returned by rs485_build_frame().
 *   g_frame_valid  - rs485_check_frame() on the intact frame  -> expect 1.
 *   g_frame_broken - rs485_check_frame() after flipping a byte -> expect 0.
 */
volatile uint16_t g_crc_check;
volatile uint8_t  g_frame_len;
volatile uint8_t  g_frame_valid;
volatile uint8_t  g_frame_broken;

int main(void)
{
    WDT_A_hold(WDT_A_BASE);

    clock_init();
    gpio_init();

    /* 1) CRC16 against the standard CRC-16/MODBUS check value. */
    static const uint8_t check[] = {'1','2','3','4','5','6','7','8','9'};
    g_crc_check = rs485_crc16(check, sizeof(check));   /* expect 0x4B37 */

    /* 2) Build a frame, then validate it (round-trip). */
    static const uint8_t payload[] = {0x00, 0x00, 0x00, 0x01};
    uint8_t frame[RS485_MAX_FRAME];
    g_frame_len = rs485_build_frame(frame, RS485_DEVICE_ADDRESS,
                                    0x03, payload, sizeof(payload));
    g_frame_valid = rs485_check_frame(frame, g_frame_len);   /* expect 1 */

    /* 3) Corrupt one byte and confirm the CRC check now fails. */
    frame[2] ^= 0xFF;
    g_frame_broken = rs485_check_frame(frame, g_frame_len);   /* expect 0 */

    /* Blink LED1 to show we reached the end. */
    while (1)
    {
        GPIO_toggleOutputOnPin(LED1_PORT, LED1_PIN);
        __delay_cycles(4000000);
    }
}
