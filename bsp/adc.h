/*
 * bsp/adc.h — ADC12_B setup and single-channel reads (BSP layer).
 *
 * Phase 4 scope: configure the ADC12_B with the internal 2.5 V reference
 * and provide a blocking single-channel read that returns the raw 12-bit
 * code. Conversion to real units (volts, amps) is done one layer up, in
 * drivers/sensors.c.
 */

#ifndef BSP_ADC_H_
#define BSP_ADC_H_

#include <stdint.h>

/*
 * adc_init() — enable the internal 2.5 V reference, configure ADC12_B
 * (12-bit, single conversion), and switch the five analog input pins to
 * analog mode. Call after clock_init().
 */
void adc_init(void);

/*
 * adc_read_raw() — perform one conversion on the given ADC input channel
 * (an ADC12_B_INPUT_Ax constant) and return the raw 12-bit result (0..4095).
 * Blocking.
 */
uint16_t adc_read_raw(uint8_t input_channel);

#endif /* BSP_ADC_H_ */
