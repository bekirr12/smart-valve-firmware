/*
 * drivers/sensors.h — Analog measurements in real units (driver layer).
 *
 * Wraps bsp/adc: reads each ADC channel and applies its board-specific
 * scaling formula (divider ratios, shunt/gain values) to return volts or
 * amps. Formulas are documented in CLAUDE.md §2.1.
 */

#ifndef DRIVERS_SENSORS_H_
#define DRIVERS_SENSORS_H_

float sensor_panel_voltage(void);    /* solar panel voltage,   volts */
float sensor_battery_voltage(void);  /* battery voltage,       volts */
float sensor_panel_current(void);    /* panel current,         amps  */
float sensor_battery_current(void);  /* battery current,       amps  */
float sensor_motor_current(void);    /* motor current,         amps  */

#endif /* DRIVERS_SENSORS_H_ */
