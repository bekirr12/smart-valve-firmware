/**
 * @file hal_adc.c
 * @brief Hardware Abstraction Layer - ADC Implementation
 * @details ADC12_B peripheral implementation using TI DriverLib
 * @author Nuvo Tech Team - SmartValve Project
 * @version 1.0
 */

#include "hal_adc.h"
#include "hal_board.h" // Access to Pin and Channel definitions


// Static buffer to hold the latest results for 5 variables
static volatile uint16_t adc_results[5];

void HAL_ADC_Init(void) {

    // Configure GPIO Pins as Analog Inputs
    
    // Set P1.5, P1.6, P1.7, P2.0, P2.1 to Ternary Function (Analog)
    GPIO_setAsPeripheralModuleFunctionInputPin(PV_V_PORT, PV_V_PIN, GPIO_TERNARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionInputPin(BATT_V_PORT, BATT_V_PIN, GPIO_TERNARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionInputPin(PV_I_PORT, PV_I_PIN, GPIO_TERNARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionInputPin(BATT_I_PORT, BATT_I_PIN, GPIO_TERNARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionInputPin(LOAD_I_PORT, LOAD_I_PIN, GPIO_TERNARY_MODULE_FUNCTION);

    // Configure internal reference to 2.5V
    PMM_unlockLPM5();
    
    while(REF_A_isRefGenBusy(REF_A_BASE));
    REF_A_setReferenceVoltage(REF_A_BASE, REF_A_VREF2_5V);
    REF_A_enableReferenceVoltage(REF_A_BASE);
    
    // Delay for reference to settle (~75us at 8MHz)
    __delay_cycles(600);

    // Initialize ADC12_B Module
    ADC12_B_initParam initParam = {0};
    initParam.sampleHoldSignalSourceSelect = ADC12_B_SAMPLEHOLDSOURCE_SC; // Software Trigger
    initParam.clockSourceSelect = ADC12_B_CLOCKSOURCE_ADC12OSC;           // Internal Oscillator
    initParam.clockSourceDivider = ADC12_B_CLOCKDIVIDER_1;
    initParam.internalChannelMap = ADC12_B_NOINTCH;
    
    // Initialize ADC
    ADC12_B_init(ADC12_B_BASE, &initParam);

    // Enable the ADC
    ADC12_B_enable(ADC12_B_BASE);
 
    // Configure Memory Buffers (Sequence Setup)
    // We map 5 physical channels to MEM0 - MEM4. Order: PV_V -> BATT_V -> PV_I -> BATT_I -> LOAD_I
    
    // common config
    ADC12_B_configureMemoryParam configureParam = {0};
    configureParam.refVoltageSourceSelect = ADC12_B_VREFPOS_INTBUF_VREFNEG_VSS; // ref 2.5V
    configureParam.windowComparatorSelect = ADC12_B_WINDOW_COMPARATOR_DISABLE;
    configureParam.differentialModeSelect = ADC12_B_DIFFERENTIAL_MODE_DISABLE;

    // MEM0: PV Voltage (A3)
    configureParam.memoryBufferControlIndex = ADC12_B_MEMORY_0;
    configureParam.inputSourceSelect = PV_V_CHANNEL; // ADC12 channel 3
    configureParam.endOfSequence = ADC12_B_NOTENDOFSEQUENCE;
    ADC12_B_configureMemory(ADC12_B_BASE, &configureParam);

    // MEM1: Battery Voltage (A4)
    configureParam.memoryBufferControlIndex = ADC12_B_MEMORY_1;
    configureParam.inputSourceSelect = BATT_V_CHANNEL; // ADC12 channel 4
    ADC12_B_configureMemory(ADC12_B_BASE, &configureParam);

    // MEM2: PV Current (A5)
    configureParam.memoryBufferControlIndex = ADC12_B_MEMORY_2;
    configureParam.inputSourceSelect = PV_I_CHANNEL; // ADC12 channel 5
    ADC12_B_configureMemory(ADC12_B_BASE, &configureParam);

    // MEM3: Battery Current (A6)
    configureParam.memoryBufferControlIndex = ADC12_B_MEMORY_3;
    configureParam.inputSourceSelect = BATT_I_CHANNEL; // ADC12 channel 6
    ADC12_B_configureMemory(ADC12_B_BASE, &configureParam);

    // MEM4: Load Current (A7) (End of Sequence)
    configureParam.memoryBufferControlIndex = ADC12_B_MEMORY_4;
    configureParam.inputSourceSelect = LOAD_I_CHANNEL; // ADC12 channel 7
    configureParam.endOfSequence = ADC12_B_ENDOFSEQUENCE; // Stop here!
    ADC12_B_configureMemory(ADC12_B_BASE, &configureParam);

    // Clear any previous interrupts
    ADC12_B_clearInterrupt(ADC12_B_BASE, 0, ADC12_B_IFG0);
}

void HAL_ADC_Read(void) {
    // Start Conversion (Sequence of Channels)
    ADC12_B_startConversion(ADC12_B_BASE, ADC12_B_MEMORY_0, ADC12_B_SEQOFCHANNELS);

    // Wait for completion (Blocking)
    // We poll the busy bit. This is acceptable for MPPT as we need data before processing.
    while (ADC12_B_isBusy(ADC12_B_BASE) == ADC12_B_BUSY);

    // 3. Store Results into the static array
    adc_results[0] = ADC12_B_getResults(ADC12_B_BASE, ADC12_B_MEMORY_0); // A3 (PV)
    adc_results[1] = ADC12_B_getResults(ADC12_B_BASE, ADC12_B_MEMORY_1); // A4 (BV)
    adc_results[2] = ADC12_B_getResults(ADC12_B_BASE, ADC12_B_MEMORY_2); // A5 (PI)
    adc_results[3] = ADC12_B_getResults(ADC12_B_BASE, ADC12_B_MEMORY_3); // A6 (BI) 
    adc_results[4] = ADC12_B_getResults(ADC12_B_BASE, ADC12_B_MEMORY_4); // A7 (LI)
}

// Convert value from raw to voltage
static uint16_t Calculate_Voltage(uint16_t raw_val, uint32_t ratio_x10) {
    uint32_t calc = (uint32_t)raw_val * ADC_VREF_MV; 
    calc = calc * ratio_x10;      
    calc = calc / ADC_RESOLUTION; 
    calc = calc / 10;             
    return (uint16_t)calc;
}

// Convert value from raw to current
static uint16_t Calculate_Current(uint16_t raw_val, uint32_t factor_x10) {
    uint32_t calc = (uint32_t)raw_val * ADC_VREF_MV;
    calc = calc / ADC_RESOLUTION;
    calc = calc * factor_x10;     
    calc = calc / 10;
    return (uint16_t)calc;
}

uint16_t HAL_ADC_GetPV_Voltage_mV(void) {
    return Calculate_Voltage(adc_results[0], VOLTAGE_RATIO_X10);
}

uint16_t HAL_ADC_GetBatt_Voltage_mV(void) {
    return Calculate_Voltage(adc_results[1], VOLTAGE_RATIO_X10);
}

uint16_t HAL_ADC_GetPV_Current_mA(void) {
    return Calculate_Current(adc_results[2], CURRENT_FACTOR_X10);
}

uint16_t HAL_ADC_GetBatt_Current_mA(void) {
    return Calculate_Current(adc_results[3], CURRENT_FACTOR_X10);
}

uint16_t HAL_ADC_GetLoad_Current_mA(void) {
    return Calculate_Current(adc_results[4], CURRENT_FACTOR_X10);
}

// for debug
uint16_t HAL_ADC_GetRawValue(uint8_t index) {
    if (index < 5) {
        return adc_results[index];
    }
    return 0;
}