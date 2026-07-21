/*
 * drivers/hmi.h — TY040HDL04NF screen, "Giraffe" protocol (driver layer).
 *
 * Frame (CLAUDE.md §2.2 / §2.3):
 *   5A A5 | LEN | R/W | TYPE | <instruction data> | [CRC16]
 * LEN counts from R/W through the instruction data (CRC excluded).
 * R/W is 0x11 (read) or 0x22 (write).
 *
 * CRC is CRC-16/CCITT (poly 0x1021, init 0x0000) — NOT the Modbus CRC used
 * on RS485. It is appended only when HMI_CRC_ENABLED is 1, which must match
 * the "CRC Enable" setting in the Giraffe IDE project.
 */

#ifndef DRIVERS_HMI_H_
#define DRIVERS_HMI_H_

#include <stdint.h>
#include "telemetry.h"   /* shared data model — not an app-layer header */

/* --- Protocol constants ---------------------------------------------- */
#define HMI_HDR0            0x5A
#define HMI_HDR1            0xA5
#define HMI_OP_READ         0x11
#define HMI_OP_WRITE        0x22

/* Instruction types */
#define HMI_TYPE_SYSTEM     0xB0
#define HMI_TYPE_VIEW       0xB1
#define HMI_TYPE_CTRL       0xB2

/* Control types (used inside a 0xB2 ctrl instruction) */
#define HMI_CTRL_LABEL      0x00
#define HMI_CTRL_BUTTON     0x01
#define HMI_CTRL_ARC        0x05
#define HMI_CTRL_BAR        0x06
#define HMI_CTRL_COMMON     0xF0

/* --- API -------------------------------------------------------------- */

/* CRC-16/CCITT (poly 0x1021, init 0x0000, MSB-first) as used by the screen. */
uint16_t hmi_crc16_ccitt(const uint8_t *data, uint16_t len);

/*
 * hmi_build_frame() — assemble a frame into `out`:
 *   5A A5 | LEN | op | type | payload... | [CRC16]
 * `payload` is the whole instruction data (function command, IDs, params).
 * Returns the total frame length, or 0 if it would not fit HMI_MAX_FRAME.
 */
uint8_t hmi_build_frame(uint8_t *out, uint8_t op, uint8_t type,
                        const uint8_t *payload, uint8_t payload_len);

/* Bring the link up: set the startup backlight level. Call after uart_hmi_init(). */
void hmi_init(void);

/* System (0xB0) */
void hmi_request_version(void);                 /* link test */
void hmi_set_brightness(uint8_t level);         /* 0-100 */
void hmi_set_standby(uint8_t enter);            /* 1 = standby, 0 = wake */
void hmi_beep(uint8_t count, uint16_t interval_ms, uint16_t duration_ms);

/* View (0xB1) */
void hmi_show_page(uint16_t view_id);

/* Ctrl (0xB2) */
void hmi_set_label_text(uint16_t page_id, uint16_t ctrl_id, const char *text);

/*
 * hmi_update() — push the current telemetry to the screen.
 * Partially implemented: the formatting is here, but the page/control IDs
 * come from the GUI built in the Giraffe IDE, so the HMI_ID_* defines in
 * config.h must be filled in once that screen design exists.
 */
void hmi_update(const telemetry_t *t);

#endif /* DRIVERS_HMI_H_ */
