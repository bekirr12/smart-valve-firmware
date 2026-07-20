/*
 * app/state_machine.h — top-level control flow (app layer).
 *
 * Ties the pieces together into the device's main behavior (CLAUDE.md §5):
 *   INIT -> IDLE (sleep) --RTC--> MEASURE -> TRANSMIT -> IDLE
 *                                                  \--cmd--> CMD_PROCESS
 *                                                            -> MOTOR_CTRL
 *
 * Current status: the MEASURE ADC reads, TRANSMIT telemetry frame, sleep/
 * wake, and command dispatch are real. USS flow, motor control, and LT8490
 * status are stubs, filled in by Phases 11 / 7 / 8. RS485 receive (to wake
 * on a command) is added with the physical link.
 */

#ifndef APP_STATE_MACHINE_H_
#define APP_STATE_MACHINE_H_

/*
 * state_machine_run() — initialize all peripherals and run the control loop
 * forever. Does not return. Call from main() after stopping the watchdog.
 */
void state_machine_run(void);

#endif /* APP_STATE_MACHINE_H_ */
