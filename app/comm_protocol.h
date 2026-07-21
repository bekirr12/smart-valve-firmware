/*
 * app/comm_protocol.h — register map + command handling (app layer).
 *
 * Sits on top of drivers/rs485 (framing + CRC). Interprets a validated
 * request frame as a modbus-like register access:
 *   function 0x03 = read holding registers (telemetry)
 *   function 0x06 = write single register  (valve command)
 * and builds the response frame.
 *
 * Data flow:
 *   state machine --update_telemetry--> [read registers] --0x03--> center
 *   center --0x06 write valve cmd--> [command] --get_valve_command--> motor
 */

#ifndef APP_COMM_PROTOCOL_H_
#define APP_COMM_PROTOCOL_H_

#include <stdint.h>
#include "telemetry.h"

/* Modbus-like function codes. */
#define MODBUS_FUNC_READ_HOLDING   0x03   /* read holding registers        */
#define MODBUS_FUNC_WRITE_SINGLE   0x06   /* write single register         */
#define MODBUS_FUNC_REPORT         0x41   /* custom: unsolicited telemetry */

/* Read-register (holding register) addresses. */
#define REG_FLOW            0x0000
#define REG_BATT_VOLTAGE    0x0001
#define REG_BATT_CURRENT    0x0002
#define REG_PANEL_VOLTAGE   0x0003
#define REG_PANEL_CURRENT   0x0004
#define REG_MOTOR_CURRENT   0x0005
#define REG_MOTOR_SPEED     0x0006
#define REG_VALVE_POSITION  0x0007
#define REG_LT8490_STATUS   0x0008
#define COMM_NUM_READ_REGS  9        /* 0x0000 .. 0x0008 */

/* Write-register (command) address. */
#define REG_VALVE_COMMAND   0x0010   /* value 0 = open, 1 = close */

/* Valve command returned by comm_protocol_get_valve_command(). */
#define VALVE_CMD_NONE      0
#define VALVE_CMD_OPEN      1
#define VALVE_CMD_CLOSE     2

/* telemetry_t lives in the shared telemetry.h so drivers/hmi can use it too
 * without depending on this app-layer header. */

/* comm_protocol_init() — clear registers and pending command. */
void comm_protocol_init(void);

/* comm_protocol_update_telemetry() — copy fresh telemetry into the read
 * registers, so the next 0x03 request returns current values. */
void comm_protocol_update_telemetry(const telemetry_t *t);

/*
 * comm_protocol_process() — handle one received frame.
 * Validates address + CRC, dispatches by function code, and builds the
 * response into `resp` (must hold RS485_MAX_FRAME bytes). Returns the
 * response length, or 0 if the frame is ignored (not our address, bad CRC,
 * or unsupported function/register).
 */
uint8_t comm_protocol_process(const uint8_t *req, uint8_t req_len,
                              uint8_t *resp);

/*
 * comm_protocol_build_report() — build an unsolicited telemetry frame
 * carrying all read registers (function MODBUS_FUNC_REPORT), for the
 * periodic device->center push. Writes into `out` (>= RS485_MAX_FRAME) and
 * returns the frame length.
 */
uint8_t comm_protocol_build_report(uint8_t *out);

/*
 * comm_protocol_get_valve_command() — return the last valve command
 * (VALVE_CMD_OPEN / VALVE_CMD_CLOSE) and clear it, or VALVE_CMD_NONE if no
 * command is pending. The state machine polls this.
 */
uint8_t comm_protocol_get_valve_command(void);

#endif /* APP_COMM_PROTOCOL_H_ */
