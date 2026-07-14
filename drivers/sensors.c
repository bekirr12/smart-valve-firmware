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
    return vadc * ((75000.0f + 10000.0f) / 10000.0f);   /* x8.5 */
}

float sensor_battery_voltage(void)
{
    float vadc = adc_to_pin_volts(adc_read_raw(ADC_BATT_V_CH));
    return vadc * ((120000.0f + 10000.0f) / 10000.0f);  /* x13 */
}

float sensor_panel_current(void)
{
    float vadc = adc_to_pin_volts(adc_read_raw(ADC_PANEL_I_CH));
    return (vadc / 21000.0f - 7.0e-6f) * (1000.0f / 0.012f);
}

float sensor_battery_current(void)
{
    float vadc = adc_to_pin_volts(adc_read_raw(ADC_BATT_I_CH));
    return vadc / 0.405f;
}

float sensor_motor_current(void)
{
    float vadc = adc_to_pin_volts(adc_read_raw(ADC_MOTOR_I_CH));
    return vadc / (0.005f * 20.0f);   /* /0.1 */
}
