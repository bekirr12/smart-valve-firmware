#include <msp430.h>
#include "driverlib/MSP430FR5xx_6xx/driverlib.h"
#include "config.h"
#include "bsp/clock.h"
#include "bsp/gpio_init.h"
#include "drivers/rs485.h"
#include "app/comm_protocol.h"

/* Phase 9b debug-view globals (watch in the CCS Expressions view):
 *   g_read_len    - length of the 0x03 read response (expect 9)
 *   g_read_valid  - CRC of the read response is intact (expect 1)
 *   g_read_reg0   - first register value in the response (expect 0x1234)
 *   g_write_len   - length of the 0x06 write response (expect 8)
 *   g_valve_cmd   - valve command stored after the write (expect 2 = CLOSE)
 *   g_wrong_addr  - process() on a frame for another address (expect 0)
 *   g_bad_crc     - process() on a corrupted frame (expect 0)
 */
volatile uint8_t  g_read_len;
volatile uint8_t  g_read_valid;
volatile uint16_t g_read_reg0;
volatile uint8_t  g_write_len;
volatile uint8_t  g_valve_cmd;
volatile uint8_t  g_wrong_addr;
volatile uint8_t  g_bad_crc;

int main(void)
{
    WDT_A_hold(WDT_A_BASE);

    clock_init();
    gpio_init();
    comm_protocol_init();

    /* Load some telemetry so a read returns recognizable values. */
    telemetry_t t = {0};
    t.flow         = 0x1234;
    t.batt_voltage = 0x5678;
    comm_protocol_update_telemetry(&t);

    uint8_t req[RS485_MAX_FRAME];
    uint8_t resp[RS485_MAX_FRAME];
    uint8_t req_len;

    /* --- Test 1: read 2 registers from 0x0000 (function 0x03) --------- */
    {
        static const uint8_t body[] = {0x00, 0x00, 0x00, 0x02}; /* start, count */
        req_len = rs485_build_frame(req, RS485_DEVICE_ADDRESS,
                                    MODBUS_FUNC_READ_HOLDING, body, 4);
        g_read_len   = comm_protocol_process(req, req_len, resp);
        g_read_valid = rs485_check_frame(resp, g_read_len);
        g_read_reg0  = ((uint16_t)resp[3] << 8) | resp[4];      /* expect 0x1234 */
    }

    /* --- Test 2: write valve command = close (function 0x06) ---------- */
    {
        static const uint8_t body[] = {0x00, 0x10, 0x00, 0x01}; /* reg 0x10 = 1 */
        req_len = rs485_build_frame(req, RS485_DEVICE_ADDRESS,
                                    MODBUS_FUNC_WRITE_SINGLE, body, 4);
        g_write_len = comm_protocol_process(req, req_len, resp);
        g_valve_cmd = comm_protocol_get_valve_command();        /* expect 2 */
    }

    /* --- Test 3: frame for another address is ignored ----------------- */
    {
        static const uint8_t body[] = {0x00, 0x00, 0x00, 0x02};
        req_len = rs485_build_frame(req, 0x02,                  /* not our addr */
                                    MODBUS_FUNC_READ_HOLDING, body, 4);
        g_wrong_addr = comm_protocol_process(req, req_len, resp); /* expect 0 */
    }

    /* --- Test 4: corrupted frame is rejected -------------------------- */
    {
        static const uint8_t body[] = {0x00, 0x00, 0x00, 0x02};
        req_len = rs485_build_frame(req, RS485_DEVICE_ADDRESS,
                                    MODBUS_FUNC_READ_HOLDING, body, 4);
        req[3] ^= 0xFF;                                          /* corrupt */
        g_bad_crc = comm_protocol_process(req, req_len, resp);  /* expect 0 */
    }

    while (1)
    {
        GPIO_toggleOutputOnPin(LED1_PORT, LED1_PIN);
        __delay_cycles(4000000);
    }
}
