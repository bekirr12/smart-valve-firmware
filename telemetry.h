/*
 * telemetry.h — the device's measurement data model (shared).
 *
 * Deliberately layer-neutral: both app/comm_protocol (which serves it over
 * RS485) and drivers/hmi (which shows it on the screen) need this type, so
 * it must not live in either layer — a driver including an app header would
 * be an upward dependency (see CLAUDE.md §3.2).
 *
 * Values are already scaled to their register encoding: voltages and
 * currents are x100 (0.01 V / 0.01 A), motor speed is 0-100 %, valve
 * position and LT8490 status are small enums.
 */

#ifndef TELEMETRY_H_
#define TELEMETRY_H_

#include <stdint.h>

typedef struct {
    uint16_t flow;            /* flow rate, x100                       */
    uint16_t batt_voltage;    /* battery voltage, x100 V               */
    uint16_t batt_current;    /* battery current, x100 A               */
    uint16_t panel_voltage;   /* panel voltage,   x100 V               */
    uint16_t panel_current;   /* panel current,   x100 A               */
    uint16_t motor_current;   /* motor current,   x100 A               */
    uint16_t motor_speed;     /* motor speed, 0-100 %                  */
    uint16_t valve_position;  /* 0 = closed, 1 = open, 2 = moving      */
    uint16_t lt8490_status;   /* charger stage / fault code            */
} telemetry_t;

#endif /* TELEMETRY_H_ */
