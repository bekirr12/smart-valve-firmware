/*
 * app/comm_protocol.c — register map + command handling implementation.
 *
 * See app/comm_protocol.h. Pure logic on top of drivers/rs485, so it is
 * fully testable without the physical RS485 link.
 */

#include "config.h"
#include "drivers/rs485.h"
#include "app/comm_protocol.h"

/* The read (holding) registers, indexed by register address 0..8. */
static uint16_t s_regs[COMM_NUM_READ_REGS];

/* Last valve command received, VALVE_CMD_* (cleared when read). */
static uint8_t s_valve_cmd = VALVE_CMD_NONE;

void comm_protocol_init(void)
{
    uint8_t i;
    for (i = 0; i < COMM_NUM_READ_REGS; i++)
        s_regs[i] = 0;
    s_valve_cmd = VALVE_CMD_NONE;
}

void comm_protocol_update_telemetry(const telemetry_t *t)
{
    s_regs[REG_FLOW]           = t->flow;
    s_regs[REG_BATT_VOLTAGE]   = t->batt_voltage;
    s_regs[REG_BATT_CURRENT]   = t->batt_current;
    s_regs[REG_PANEL_VOLTAGE]  = t->panel_voltage;
    s_regs[REG_PANEL_CURRENT]  = t->panel_current;
    s_regs[REG_MOTOR_CURRENT]  = t->motor_current;
    s_regs[REG_MOTOR_SPEED]    = t->motor_speed;
    s_regs[REG_VALVE_POSITION] = t->valve_position;
    s_regs[REG_LT8490_STATUS]  = t->lt8490_status;
}

uint8_t comm_protocol_get_valve_command(void)
{
    uint8_t cmd = s_valve_cmd;
    s_valve_cmd = VALVE_CMD_NONE;   /* consume it */
    return cmd;
}

/* Handle function 0x03 (read holding registers). Returns response length. */
static uint8_t handle_read(const uint8_t *req, uint8_t *resp)
{
    uint16_t start = ((uint16_t)req[2] << 8) | req[3];
    uint16_t count = ((uint16_t)req[4] << 8) | req[5];

    /* Range check against the read-register block. */
    if (count == 0 || (start + count) > COMM_NUM_READ_REGS)
        return 0;

    /* Build the response data: [byte_count][reg_hi reg_lo]... */
    uint8_t data[1 + COMM_NUM_READ_REGS * 2];
    data[0] = (uint8_t)(count * 2);

    uint16_t i;
    for (i = 0; i < count; i++)
    {
        uint16_t v = s_regs[start + i];
        data[1 + i * 2]     = (uint8_t)(v >> 8);    /* Modbus: hi byte first */
        data[1 + i * 2 + 1] = (uint8_t)(v & 0xFF);
    }

    return rs485_build_frame(resp, RS485_DEVICE_ADDRESS, MODBUS_FUNC_READ_HOLDING,
                             data, (uint8_t)(1 + count * 2));
}

/* Handle function 0x06 (write single register). Returns response length. */
static uint8_t handle_write(const uint8_t *req, uint8_t *resp)
{
    uint16_t reg   = ((uint16_t)req[2] << 8) | req[3];
    uint16_t value = ((uint16_t)req[4] << 8) | req[5];

    if (reg == REG_VALVE_COMMAND)
    {
        /* value 0 = open, 1 = close */
        s_valve_cmd = (value == 0) ? VALVE_CMD_OPEN : VALVE_CMD_CLOSE;
    }
    else
    {
        return 0;   /* unknown write register */
    }

    /* Modbus write-single-register echoes the request's data field back. */
    return rs485_build_frame(resp, RS485_DEVICE_ADDRESS, MODBUS_FUNC_WRITE_SINGLE,
                             &req[2], 4);
}

uint8_t comm_protocol_process(const uint8_t *req, uint8_t req_len,
                              uint8_t *resp)
{
    /* Must at least hold address + function + CRC. */
    if (req_len < 4)
        return 0;

    /* Is it addressed to us? */
    if (req[0] != RS485_DEVICE_ADDRESS)
        return 0;

    /* Is it intact? */
    if (!rs485_check_frame(req, req_len))
        return 0;

    switch (req[1])   /* function code */
    {
        case MODBUS_FUNC_READ_HOLDING:
            if (req_len != 8)   /* addr+func+start(2)+count(2)+CRC(2) */
                return 0;
            return handle_read(req, resp);

        case MODBUS_FUNC_WRITE_SINGLE:
            if (req_len != 8)   /* addr+func+reg(2)+value(2)+CRC(2) */
                return 0;
            return handle_write(req, resp);

        default:
            return 0;           /* unsupported function */
    }
}
