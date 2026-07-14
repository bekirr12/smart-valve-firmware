/*
 * bsp/adc.c — ADC12_B setup and single-channel reads.
 *
 * See bsp/adc.h. All conversions use the internal 2.5 V reference and the
 * 12-bit resolution. Each read reconfigures memory buffer 0 for the
 * requested channel, triggers one conversion, and returns the raw code.
 */

#include <msp430.h>
#include "driverlib/MSP430FR5xx_6xx/driverlib.h"
#include "config.h"
#include "bsp/adc.h"

void adc_init(void)
{
    /* --- Switch the five analog input pins to analog mode -----------
     * Setting both SELx bits (ternary function) disconnects the digital
     * buffer and routes the pad to the ADC.
     */
    GPIO_setAsPeripheralModuleFunctionInputPin(
        ADC_PANEL_V_PORT, ADC_PANEL_V_PIN, GPIO_TERNARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionInputPin(
        ADC_BATT_V_PORT, ADC_BATT_V_PIN, GPIO_TERNARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionInputPin(
        ADC_PANEL_I_PORT, ADC_PANEL_I_PIN, GPIO_TERNARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionInputPin(
        ADC_BATT_I_PORT, ADC_BATT_I_PIN, GPIO_TERNARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionInputPin(
        ADC_MOTOR_I_PORT, ADC_MOTOR_I_PIN, GPIO_TERNARY_MODULE_FUNCTION);

    /* --- Enable the internal 2.5 V reference ------------------------ */
    while (Ref_A_isRefGenBusy(REF_A_BASE))
        ;
    Ref_A_setReferenceVoltage(REF_A_BASE, REF_A_VREF2_5V);
    Ref_A_enableReferenceVoltage(REF_A_BASE);
    __delay_cycles(400);   /* ~50 µs settle time for the reference */

    /* --- Configure the ADC12_B core --------------------------------
     * Sample trigger = software (SC bit), clock = internal ADC oscillator,
     * no dividers, no internal channels mapped.
     */
    ADC12_B_initParam initParam = {0};
    initParam.sampleHoldSignalSourceSelect = ADC12_B_SAMPLEHOLDSOURCE_SC;
    initParam.clockSourceSelect            = ADC12_B_CLOCKSOURCE_ADC12OSC;
    initParam.clockSourceDivider           = ADC12_B_CLOCKDIVIDER_1;
    initParam.clockSourcePredivider        = ADC12_B_CLOCKPREDIVIDER__1;
    initParam.internalChannelMap           = ADC12_B_NOINTCH;
    ADC12_B_init(ADC12_B_BASE, &initParam);

    /* Sample-and-hold time: 16 ADC clocks is plenty for our high-impedance
     * dividers to charge the sampling capacitor.
     */
    ADC12_B_setupSamplingTimer(ADC12_B_BASE,
                               ADC12_B_CYCLEHOLD_16_CYCLES,
                               ADC12_B_CYCLEHOLD_4_CYCLES,
                               ADC12_B_MULTIPLESAMPLESDISABLE);

    ADC12_B_setResolution(ADC12_B_BASE, ADC12_B_RESOLUTION_12BIT);
    ADC12_B_enable(ADC12_B_BASE);
}

uint16_t adc_read_raw(uint8_t input_channel)
{
    /* Point memory buffer 0 at the requested channel, referenced to the
     * internal reference (+) and VSS (-).
     */
    ADC12_B_configureMemoryParam memParam = {0};
    memParam.memoryBufferControlIndex = ADC12_B_MEMORY_0;
    memParam.inputSourceSelect        = input_channel;
    memParam.refVoltageSourceSelect   = ADC12_B_VREFPOS_INTBUF_VREFNEG_VSS;
    memParam.endOfSequence            = ADC12_B_ENDOFSEQUENCE;
    memParam.windowComparatorSelect   = ADC12_B_WINDOW_COMPARATOR_DISABLE;
    memParam.differentialModeSelect   = ADC12_B_DIFFERENTIAL_MODE_DISABLE;
    ADC12_B_configureMemory(ADC12_B_BASE, &memParam);

    /* One conversion on memory buffer 0, then wait for the result. */
    ADC12_B_startConversion(ADC12_B_BASE,
                            ADC12_B_MEMORY_0,
                            ADC12_B_SINGLECHANNEL);

    while (ADC12_B_isBusy(ADC12_B_BASE))
        ;

    return ADC12_B_getResults(ADC12_B_BASE, ADC12_B_MEMORY_0);
}
