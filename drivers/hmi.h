/*
 * drivers/hmi.h — TY040HDL04NF screen, "Giraffe" protocol (driver layer).
 *
 * Frame (CLAUDE.md §2.2):
 *   5A A5 | LEN | R/W | TYPE | CMD... | DATA... | [CRC16]
 * where LEN counts from R/W through DATA (CRC excluded), R/W is 0x11 (read)
 * or 0x22 (write), and CRC is optional (the vendor examples omit it).
 *
 * Status: the framing layer and the known system commands (driver version,
 * backlight brightness) are implemented. Writing values into screen widgets
 * and switching pages still needs the vendor's full instruction table —
 * hmi_update() is a stub until then.
 */

#ifndef DRIVERS_HMI_H_
#define DRIVERS_HMI_H_

#include <stdint.h>
#include "telemetry.h"   /* shared data model — not an app-layer header */

/* Giraffe protocol constants. */
#define HMI_HDR0            0x5A
#define HMI_HDR1            0xA5
#define HMI_OP_READ         0x11
#define HMI_OP_WRITE        0x22
#define HMI_TYPE_SYSTEM     0xB0

/*
 * hmi_init() — bring the screen link up: set the startup backlight level.
 * Call after uart_hmi_init().
 */
void hmi_init(void);

/*
 * hmi_build_frame() — assemble a Giraffe frame into `out`:
 *   5A A5 | LEN | op | type | payload...
 * `payload` is the control command plus any data bytes. Returns the total
 * frame length, or 0 if it would not fit in HMI_MAX_FRAME.
 * (CRC is not appended; the module accepts frames without it.)
 */
uint8_t hmi_build_frame(uint8_t *out, uint8_t op, uint8_t type,
                        const uint8_t *payload, uint8_t payload_len);

/*
 * hmi_set_brightness() — set the backlight level, 0-100.
 * 0 turns the backlight off, which drops the module from ~480 mA to
 * ~190 mA (it still draws the rest — see CLAUDE.md §4 / §9.3).
 */
void hmi_set_brightness(uint8_t level);

/*
 * hmi_request_version() — send the "read driver version" command. Mainly a
 * link test: a reply proves wiring, baud and framing are correct.
 */
void hmi_request_version(void);

/*
 * hmi_update() — push the current telemetry to the screen widgets.
 * STUB: needs the vendor's widget/page instruction table.
 */
void hmi_update(const telemetry_t *t);

#endif /* DRIVERS_HMI_H_ */
