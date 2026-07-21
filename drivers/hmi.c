/*
 * drivers/hmi.c — TY040HDL04NF "Giraffe" screen driver implementation.
 *
 * See drivers/hmi.h and CLAUDE.md §2.2 / §2.3.
 */

#include "config.h"
#include "bsp/uart.h"
#include "drivers/hmi.h"

/* System (0xB0) function commands */
#define HMI_SYS_VERSION      0x00
#define HMI_SYS_BRIGHTNESS   0x02
#define HMI_SYS_TONE         0x03
#define HMI_SYS_STANDBY      0x06
#define HMI_SYS_STANDBY_SUB  0x01   /* standby status is command "06 01" */
#define HMI_VER_DRIVER       0x02   /* version type: 0=APP, 1=UI, 2=driver */

/* View (0xB1) function commands */
#define HMI_VIEW_DISPLAY     0x00

/* Label (0x00) function commands */
#define HMI_LABEL_TEXT       0x00

uint16_t hmi_crc16_ccitt(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0x0000;          /* CCITT/XMODEM init */
    uint16_t i;

    for (i = 0; i < len; i++)
    {
        crc ^= (uint16_t)((uint16_t)data[i] << 8);   /* fold byte into MSB */

        uint8_t bit;
        for (bit = 0; bit < 8; bit++)
        {
            if (crc & 0x8000)       /* MSB-first, unlike the Modbus CRC */
                crc = (uint16_t)((crc << 1) ^ 0x1021);
            else
                crc = (uint16_t)(crc << 1);
        }
    }

    return crc;
}

uint8_t hmi_build_frame(uint8_t *out, uint8_t op, uint8_t type,
                        const uint8_t *payload, uint8_t payload_len)
{
    /* LEN counts op + type + payload (CRC excluded). */
    uint8_t len = (uint8_t)(2 + payload_len);
    uint8_t total = (uint8_t)(3 + len);          /* header(2) + LEN(1) + len */

#if HMI_CRC_ENABLED
    total = (uint8_t)(total + 2);
#endif

    /* We only emit the 1-byte length form, so LEN must stay below 0x80. */
    if (len >= 0x80 || total > HMI_MAX_FRAME)
        return 0;

    out[0] = HMI_HDR0;
    out[1] = HMI_HDR1;
    out[2] = len;
    out[3] = op;
    out[4] = type;

    uint8_t i;
    for (i = 0; i < payload_len; i++)
        out[5 + i] = payload[i];

#if HMI_CRC_ENABLED
    /* CRC covers everything from LEN through the payload. */
    uint16_t crc = hmi_crc16_ccitt(&out[2], (uint16_t)(1 + len));
    out[5 + payload_len]     = (uint8_t)(crc >> 8);   /* MSB first */
    out[5 + payload_len + 1] = (uint8_t)(crc & 0xFF);
#endif

    return total;
}

/* Send one built frame. */
static void hmi_send(uint8_t op, uint8_t type,
                     const uint8_t *payload, uint8_t payload_len)
{
    uint8_t frame[HMI_MAX_FRAME];
    uint8_t n = hmi_build_frame(frame, op, type, payload, payload_len);
    if (n)
        uart_hmi_send(frame, n);
}

/* ------------------------- System (0xB0) ---------------------------- */

void hmi_request_version(void)
{
    uint8_t p[2] = { HMI_SYS_VERSION, HMI_VER_DRIVER };
    hmi_send(HMI_OP_READ, HMI_TYPE_SYSTEM, p, sizeof(p));
}

void hmi_set_brightness(uint8_t level)
{
    if (level > 100)
        level = 100;

    uint8_t p[2] = { HMI_SYS_BRIGHTNESS, level };
    hmi_send(HMI_OP_WRITE, HMI_TYPE_SYSTEM, p, sizeof(p));
}

void hmi_set_standby(uint8_t enter)
{
    /* Command "06 01", then 1 = enter standby, 0 = exit. */
    uint8_t p[3] = { HMI_SYS_STANDBY, HMI_SYS_STANDBY_SUB, enter ? 1u : 0u };
    hmi_send(HMI_OP_WRITE, HMI_TYPE_SYSTEM, p, sizeof(p));
}

void hmi_beep(uint8_t count, uint16_t interval_ms, uint16_t duration_ms)
{
    /* u16 parameters go MSB first. */
    uint8_t p[6];
    p[0] = HMI_SYS_TONE;
    p[1] = count;
    p[2] = (uint8_t)(interval_ms >> 8);
    p[3] = (uint8_t)(interval_ms & 0xFF);
    p[4] = (uint8_t)(duration_ms >> 8);
    p[5] = (uint8_t)(duration_ms & 0xFF);
    hmi_send(HMI_OP_WRITE, HMI_TYPE_SYSTEM, p, sizeof(p));
}

/* -------------------------- View (0xB1) ----------------------------- */

void hmi_show_page(uint16_t view_id)
{
    uint8_t p[3];
    p[0] = HMI_VIEW_DISPLAY;
    p[1] = (uint8_t)(view_id >> 8);
    p[2] = (uint8_t)(view_id & 0xFF);
    hmi_send(HMI_OP_WRITE, HMI_TYPE_VIEW, p, sizeof(p));
}

/* -------------------------- Ctrl (0xB2) ----------------------------- */

void hmi_set_label_text(uint16_t page_id, uint16_t ctrl_id, const char *text)
{
    /* control type + function cmd + page ID + control ID + string + NUL */
    uint8_t p[HMI_MAX_FRAME];
    uint8_t n = 0;

    p[n++] = HMI_CTRL_LABEL;
    p[n++] = HMI_LABEL_TEXT;
    p[n++] = (uint8_t)(page_id >> 8);
    p[n++] = (uint8_t)(page_id & 0xFF);
    p[n++] = (uint8_t)(ctrl_id >> 8);
    p[n++] = (uint8_t)(ctrl_id & 0xFF);

    /* Copy the text, leaving room for the NUL terminator and the frame
     * overhead (header + LEN + op + type, plus CRC when enabled). */
    while (*text && n < (uint8_t)(HMI_MAX_FRAME - 10))
        p[n++] = (uint8_t)*text++;
    p[n++] = 0x00;                       /* strings are NUL-terminated */

    hmi_send(HMI_OP_WRITE, HMI_TYPE_CTRL, p, n);
}

/* --------------------------- High level ----------------------------- */

void hmi_init(void)
{
    hmi_set_brightness(HMI_DEFAULT_BRIGHTNESS);
}

/* Format a x100-scaled value as "123.45" into buf (needs >= 10 bytes). */
static void format_x100(uint16_t scaled, char *buf)
{
    uint16_t whole = scaled / 100u;
    uint16_t frac  = scaled % 100u;

    uint8_t i = 0;
    if (whole >= 100) buf[i++] = (char)('0' + (whole / 100) % 10);
    if (whole >= 10)  buf[i++] = (char)('0' + (whole / 10) % 10);
    buf[i++] = (char)('0' + whole % 10);
    buf[i++] = '.';
    buf[i++] = (char)('0' + frac / 10);
    buf[i++] = (char)('0' + frac % 10);
    buf[i]   = '\0';
}

void hmi_update(const telemetry_t *t)
{
    /* TODO Phase 12: fill in HMI_ID_* (page and control IDs) in config.h
     * once the screen layout is built in the Giraffe IDE, then enable the
     * writes below. The formatting is ready; only the IDs are missing.
     *
     *   char buf[12];
     *   format_x100(t->flow, buf);
     *   hmi_set_label_text(HMI_PAGE_MAIN, HMI_ID_FLOW, buf);
     *   format_x100(t->batt_voltage, buf);
     *   hmi_set_label_text(HMI_PAGE_MAIN, HMI_ID_BATT_V, buf);
     *   ...
     */
    (void)t;
    (void)format_x100;
}
