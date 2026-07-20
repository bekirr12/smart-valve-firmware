/*
 * drivers/sensors.c — Analog measurements in real units.
 *
 * Each function reads its ADC channel, converts the raw 12-bit code to the
 * voltage at the ADC pin, then applies the channel's scaling formula.
 *
 * Formulas (from CLAUDE.md §2.1), where Vadc is the voltage at the pin:
 *   Panel V   = Vadc * (75k + 10k) / 10k          = Vadc * 8.5
 *   Battery V = Vadc * (120k + 10k) / 10k         = Vadc * 13
 *   Panel I   = (Vadc / 21k - 7uA) * 1000 / 0.012
 *   Battery I = Vadc / 0.405
 *   Motor I   = Vadc / (0.005 * 20)               = Vadc / 0.1
 */

#include <msp430.h>
#include "driverlib/MSP430FR5xx_6xx/driverlib.h"
#include "config.h"
#include "bsp/adc.h"
#include "drivers/sensors.h"

/* Convert a raw ADC code to the voltage present at the ADC pin. */
static float adc_to_pin_volts(uint16_t raw)
{
    return ((float)raw * ADC_VREF_VOLTS) / ADC_FULL_SCALE;
}

float sensor_panel_voltage(void)
{
    float vadc = adc_to_pin_volts(adc_read_raw(ADC_PANEL_V_CH));
    return vadc * ((SENSOR_PANEL_V_RTOP + SENSOR_PANEL_V_RBOT)
                   / SENSOR_PANEL_V_RBOT);
}

float sensor_battery_voltage(void)
{
    float vadc = adc_to_pin_volts(adc_read_raw(ADC_BATT_V_CH));
    return vadc * ((SENSOR_BATT_V_RTOP + SENSOR_BATT_V_RBOT)
                   / SENSOR_BATT_V_RBOT);
}

float sensor_panel_current(void)
{
    float vadc = adc_to_pin_volts(adc_read_raw(ADC_PANEL_I_CH));
    return (vadc / SENSOR_PANEL_I_RIMON - SENSOR_PANEL_I_IBIAS)
           * (1000.0f / SENSOR_PANEL_I_RSENSE);
}

float sensor_battery_current(void)
{
    float vadc = adc_to_pin_volts(adc_read_raw(ADC_BATT_I_CH));
    return vadc / SENSOR_BATT_I_DIV;
}

float sensor_motor_current(void)
{
    float vadc = adc_to_pin_volts(adc_read_raw(ADC_MOTOR_I_CH));
    return vadc / (SENSOR_MOTOR_I_RSHUNT * SENSOR_MOTOR_I_GAIN);
}
