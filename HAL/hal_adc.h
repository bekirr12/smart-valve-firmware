/**
 * @file hal_adc.h
 * @brief Hardware Abstraction Layer - ADC Implementation
 * @details ADC12_B peripheral implementation using TI DriverLib
 * @author Nuvo Tech Team - SmartValve Project
 * @version 1.0
 */

#ifndef HAL_ADC_H_
#define HAL_ADC_H_

#include <msp430.h>
#include <driverlib.h>
#include <stdint.h>
#include <stdbool.h>

#define ADC_RESOLUTION      4096 // 12 bit adc
#define ADC_VREF_MV         2500 // ref 2500mV
#define VOLTAGE_RATIO_X10   188  // (178k + 10k) / 10k
#define CURRENT_FACTOR_X10  10

// main function prototoype
void HAL_ADC_Init(void);
void HAL_ADC_Read(void);

// helper function
uint16_t HAL_ADC_GetPV_Voltage_mV(void);
uint16_t HAL_ADC_GetBatt_Voltage_mV(void);
uint16_t HAL_ADC_GetPV_Current_mA(void);
uint16_t HAL_ADC_GetBatt_Current_mA(void);
uint16_t HAL_ADC_GetLoad_Current_mA(void);

// for debug purpose
uint16_t HAL_ADC_GetRawValue(uint8_t index);

#endif /* HAL_ADC_H_ */
