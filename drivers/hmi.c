/*
 * drivers/hmi.c — TY040HDL04NF "Giraffe" screen driver implementation.
 *
 * See drivers/hmi.h. Only the framing layer and the two documented system
 * commands are implemented; widget/page commands await the vendor's full
 * instruction table.
 */

#include "config.h"
#include "bsp/uart.h"
#include "drivers/hmi.h"

/* System sub-commands (from the vendor's examples). */
#define HMI_SUB_VERSION_HI   0x00   /* read version: payload = 00 02 */
#define HMI_SUB_VERSION_LO   0x02
#define HMI_SUB_BRIGHTNESS   0x02   /* write brightness: payload = 02 <level> */

uint8_t hmi_build_frame(uint8_t *out, uint8_t op, uint8_t type,
                        const uint8_t *payload, uint8_t payload_len)
{
    /* LEN counts op + type + payload. Frame adds the 2 header bytes and the
     * 1-byte LEN field on top of that. */
    uint8_t len = (uint8_t)(2 + payload_len);

    /* A LEN of 0x80 or more would need the 2-byte length form, which we do
     * not use; and the whole frame must fit the buffer. */
    if (len >= 0x80 || (uint16_t)(3 + len) > HMI_MAX_FRAME)
        return 0;

    out[0] = HMI_HDR0;
    out[1] = HMI_HDR1;
    out[2] = len;
    out[3] = op;
    out[4] = type;

    uint8_t i;
    for (i = 0; i < payload_len; i++)
        out[5 + i] = payload[i];

    return (uint8_t)(5 + payload_len);
}

void hmi_set_brightness(uint8_t level)
{
    if (level > 100)
        level = 100;

    uint8_t payload[2] = { HMI_SUB_BRIGHTNESS, level };
    uint8_t frame[HMI_MAX_FRAME];

    uint8_t n = hmi_build_frame(frame, HMI_OP_WRITE, HMI_TYPE_SYSTEM,
                                payload, sizeof(payload));
    if (n)
        uart_hmi_send(frame, n);
}

void hmi_request_version(void)
{
    uint8_t payload[2] = { HMI_SUB_VERSION_HI, HMI_SUB_VERSION_LO };
    uint8_t frame[HMI_MAX_FRAME];

    uint8_t n = hmi_build_frame(frame, HMI_OP_READ, HMI_TYPE_SYSTEM,
                                payload, sizeof(payload));
    if (n)
        uart_hmi_send(frame, n);
}

void hmi_init(void)
{
    hmi_set_brightness(HMI_DEFAULT_BRIGHTNESS);
}

void hmi_update(const telemetry_t *t)
{
    /* TODO Phase 12: write the telemetry into the screen's widgets once the
     * vendor supplies the instruction table for text/number fields and page
     * selection. The values to show are already in `t` (flow, battery and
     * panel V/I, motor current/speed, valve position, LT8490 status).
     */
    (void)t;
}
