/*
 * app/state_machine.c — top-level control flow implementation.
 *
 * See app/state_machine.h. Stubs are marked "TODO Phase N" where a
 * not-yet-built module plugs in.
 */

#include <msp430.h>
#include "driverlib/MSP430FR5xx_6xx/driverlib.h"
#include "config.h"
#include "bsp/clock.h"
#include "bsp/gpio_init.h"
#include "bsp/adc.h"
#include "bsp/uart.h"
#include "bsp/i2c.h"
#include "bsp/timer.h"
#include "bsp/power.h"
#include "drivers/sensors.h"
#include "drivers/mcp4706.h"
#include "drivers/hmi.h"
#include "app/comm_protocol.h"
#include "app/state_machine.h"

typedef enum {
    ST_INIT,
    ST_IDLE,
    ST_MEASURE,
    ST_TRANSMIT,
    ST_CMD_PROCESS,
    ST_MOTOR_CTRL
} state_t;

/* Debug-view globals (watch in the CCS Expressions view):
 *   g_state          - current state (0=INIT..5=MOTOR_CTRL)
 *   g_cycle_count    - number of measure/transmit cycles completed
 *   g_telem          - the telemetry gathered this cycle
 *   g_last_frame_len - length of the last telemetry frame sent
 */
volatile uint8_t  g_state;
volatile uint32_t g_cycle_count;
telemetry_t       g_telem;
volatile uint8_t  g_last_frame_len;

static uint8_t s_frame[RS485_MAX_FRAME];
static uint8_t s_pending_cmd = VALVE_CMD_NONE;

/* Convert a real unit (V or A) to the register encoding (x100), clamped to
 * the unsigned 16-bit range. */
static uint16_t scale_x100(float v)
{
    if (v < 0.0f)
        v = 0.0f;
    float s = v * 100.0f;
    if (s > 65535.0f)
        s = 65535.0f;
    return (uint16_t)s;
}

/* INIT — bring up every peripheral, then go idle. */
static state_t do_init(void)
{
    clock_init();          /* Phase 1  */
    gpio_init();           /* Phase 2  */
    adc_init();            /* Phase 4  */
    i2c_init();            /* Phase 6  */
    uart_rs485_init();     /* Phase 3  */
    uart_hmi_init();       /* Phase 12 */
    comm_protocol_init();  /* Phase 9  */
    mcp4706_init();        /* Phase 6: DAC config (VREF=VDD)     */
    hmi_init();            /* Phase 12: startup backlight level  */
    rtc_init();            /* Phase 5: start the periodic wake   */

    /* TODO Phase 7: motor_init();  Phase 8: lt8490_init();
     * TODO Phase 11: uss_init();                                     */

    return ST_IDLE;
}

/* IDLE — sleep until the RTC signals a measurement is due.
 * Race-free check-then-sleep (see Phase 5). RS485-RX and FAULT wake sources
 * will branch here too once wired. */
static state_t do_idle(void)
{
    for (;;)
    {
        __disable_interrupt();
        if (rtc_measurement_due())
        {
            __enable_interrupt();
            break;
        }
        power_enter_sleep();   /* LPM (LPM1 with UART on); wakes on interrupt */
    }
    rtc_clear_measurement_due();
    return ST_MEASURE;
}

/* MEASURE — gather this cycle's telemetry. */
static state_t do_measure(void)
{
    /* TODO Phase 11: enable ±15V, wait ~10 ms, USS measure -> flow. */
    g_telem.flow = 0;

    g_telem.batt_voltage  = scale_x100(sensor_battery_voltage());
    g_telem.batt_current  = scale_x100(sensor_battery_current());
    g_telem.panel_voltage = scale_x100(sensor_panel_voltage());
    g_telem.panel_current = scale_x100(sensor_panel_current());
    g_telem.motor_current = scale_x100(sensor_motor_current());

    g_telem.motor_speed    = 0;   /* TODO Phase 7: current speed %      */
    g_telem.valve_position = 0;   /* TODO Phase 7: real valve position  */
    g_telem.lt8490_status  = 0;   /* TODO Phase 8: charger status        */

    return ST_TRANSMIT;
}

/* TRANSMIT — push the telemetry frame to the center. */
static state_t do_transmit(void)
{
    comm_protocol_update_telemetry(&g_telem);

    /* Telemetry goes to two sinks: the center over RS485, and the local
     * HMI screen over its own UART. */
    g_last_frame_len = comm_protocol_build_report(s_frame);
    uart_rs485_send(s_frame, g_last_frame_len);
    hmi_update(&g_telem);   /* stub until the widget command table arrives */

    g_cycle_count++;
    GPIO_toggleOutputOnPin(LED1_PORT, LED1_PIN);   /* sign of life */

    /* If a command arrived (RS485 RX, wired later), handle it next. */
    s_pending_cmd = comm_protocol_get_valve_command();
    return (s_pending_cmd != VALVE_CMD_NONE) ? ST_CMD_PROCESS : ST_IDLE;
}

/* CMD_PROCESS — a valve command is pending; route it to the motor. */
static state_t do_cmd_process(void)
{
    return ST_MOTOR_CTRL;
}

/* MOTOR_CTRL — drive the valve. */
static state_t do_motor_ctrl(void)
{
    /* TODO Phase 7: run the motor sub-state machine (brake, direction,
     * accelerate, run until stall/encoder/timeout, verify). */
    g_telem.valve_position =
        (s_pending_cmd == VALVE_CMD_OPEN) ? 1 : 0;   /* stub */
    s_pending_cmd = VALVE_CMD_NONE;
    return ST_IDLE;
}

void state_machine_run(void)
{
    state_t state = ST_INIT;

    for (;;)
    {
        g_state = (uint8_t)state;
        switch (state)
        {
            case ST_INIT:        state = do_init();        break;
            case ST_IDLE:        state = do_idle();        break;
            case ST_MEASURE:     state = do_measure();     break;
            case ST_TRANSMIT:    state = do_transmit();    break;
            case ST_CMD_PROCESS: state = do_cmd_process(); break;
            case ST_MOTOR_CTRL:  state = do_motor_ctrl();  break;
            default:             state = ST_IDLE;          break;
        }
    }
}
