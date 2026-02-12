/**
 * @file hal_pwm.c
 * @brief Hardware Abstraction Layer - PWM
 * @details TIMER_B peripheral implementation using TI DriverLib
 * @author Nuvo Tech Team - SmartValve Project
 * @version 1.0
 */


#include "hal_board.h"
#include "hal_pwm.h"
#include <msp430.h>
#include <driverlib.h>

// private variables
static uint16_t current_duty_phase1 = 0;
static uint16_t current_duty_phase2 = 0;
static bool pwm_running = false;

// private function prototypes
static inline uint16_t clamp_duty_ticks(uint16_t duty_ticks);
static inline uint16_t calculate_low_side_ticks(uint16_t high_side_ticks);


void HAL_PWM_Init(void) {

    // Configure Enable Pins (GPIO)
    GPIO_setAsOutputPin(P_EN_PORT, P_EN_PIN);
    GPIO_setAsOutputPin(L_EN_PORT, L_EN_PIN);

    // Initially disable drivers for safety
    HAL_PWM_EnableDrivers(false, false);

    // Configure PWM Pins (Peripheral Mode)
    GPIO_setAsPeripheralModuleFunctionOutputPin(PW_L1_PORT, PW_L1_PIN, GPIO_SECONDARY_MODULE_FUNCTION); // TB0.1 -> PW_L1
    GPIO_setAsPeripheralModuleFunctionOutputPin(PW_H1_PORT, PW_H1_PIN, GPIO_SECONDARY_MODULE_FUNCTION); // TB0.2 -> PW_H1
    GPIO_setAsPeripheralModuleFunctionOutputPin(PW_L2_PORT, PW_L2_PIN, GPIO_SECONDARY_MODULE_FUNCTION); // TB0.4 -> PW_L2
    GPIO_setAsPeripheralModuleFunctionOutputPin(PW_H2_PORT, PW_H2_PIN, GPIO_SECONDARY_MODULE_FUNCTION); // TB0.6 -> PW_H2

    // Initialize Timer_B0 (Up Down Mode)
    Timer_B_initUpDownModeParam timerParams = {0};
    timerParams.clockSource = TIMER_B_CLOCKSOURCE_SMCLK; // 8 MHz
    timerParams.clockSourceDivider = TIMER_B_CLOCKSOURCE_DIVIDER_1;
    timerParams.timerPeriod = PWM_PERIOD_TICKS; // Set Frequency to 200 kHz
    timerParams.timerInterruptEnable_TBIE = TIMER_B_TBIE_INTERRUPT_DISABLE;
    timerParams.captureCompareInterruptEnable_CCR0_CCIE = TIMER_B_CCIE_CCR0_INTERRUPT_DISABLE;
    timerParams.timerClear = TIMER_B_DO_CLEAR;
    timerParams.startTimer = false; // Wait until Compare registers are set
    
    Timer_B_initUpDownMode(TIMER_B0_BASE, &timerParams);

    // Phase 1 High-side config (PW_H1) - CCR2
    Timer_B_initCompareModeParam compareH1 = {0};
    compareH1.compareRegister = TIMER_B_CAPTURECOMPARE_REGISTER_2;
    compareH1.compareInterruptEnable = TIMER_B_CAPTURECOMPARE_INTERRUPT_DISABLE;
    compareH1.compareOutputMode = TIMER_B_OUTPUTMODE_SET_RESET;  // Mode 3
    compareH1.compareValue = 0;  // Start at 0% duty
    Timer_B_initCompareMode(TIMER_B0_BASE, &compareH1);

    // Phase 1 Low-side (PW_L1) config - CCR1 (Complementary)
    Timer_B_initCompareModeParam compareL1 = {0};
    compareL1.compareRegister = TIMER_B_CAPTURECOMPARE_REGISTER_1;
    compareL1.compareInterruptEnable = TIMER_B_CAPTURECOMPARE_INTERRUPT_DISABLE;
    compareL1.compareOutputMode = TIMER_B_OUTPUTMODE_RESET_SET;  // Mode 7 (Inverted)
    compareL1.compareValue = PWM_DEADTIME_TICKS;  // Offset for dead-time
    Timer_B_initCompareMode(TIMER_B0_BASE, &compareL1);

    // Phase 2 High-side (PW_H2) config - CCR6
    Timer_B_initCompareModeParam compareH2 = {0};
    compareH2.compareRegister = TIMER_B_CAPTURECOMPARE_REGISTER_6;
    compareH2.compareInterruptEnable = TIMER_B_CAPTURECOMPARE_INTERRUPT_DISABLE;
    compareH2.compareOutputMode = TIMER_B_OUTPUTMODE_SET_RESET;  // Mode 3
    compareH2.compareValue = 0;  // Start at 0% duty
    Timer_B_initCompareMode(TIMER_B0_BASE, &compareH2);

    // Phase 2 Low-side (PW_L2) config - CCR4 (Complementary)
    Timer_B_initCompareModeParam compareL2 = {0};
    compareL2.compareRegister = TIMER_B_CAPTURECOMPARE_REGISTER_4;
    compareL2.compareInterruptEnable = TIMER_B_CAPTURECOMPARE_INTERRUPT_DISABLE;
    compareL2.compareOutputMode = TIMER_B_OUTPUTMODE_RESET_SET;  // Mode 7 (Inverted)
    compareL2.compareValue = PWM_DEADTIME_TICKS;  // Offset for dead-time
    Timer_B_initCompareMode(TIMER_B0_BASE, &compareL2);

    // Initialize state
    current_duty_phase1 = 0;
    current_duty_phase2 = 0;
    pwm_running = false;
}

void HAL_PWM_SetDuty(PWM_Phase_t phase, uint16_t duty_ticks)
{
    // Clamp to safe operating range
    duty_ticks = clamp_duty_ticks(duty_ticks);

    // Calculate low-side value with dead-time
    uint16_t low_side_ticks = calculate_low_side_ticks(duty_ticks);

    switch(phase)
    {
        case PWM_PHASE_1:
            Timer_B_setCompareValue(TIMER_B0_BASE, TIMER_B_CAPTURECOMPARE_REGISTER_2, duty_ticks);
            Timer_B_setCompareValue(TIMER_B0_BASE, TIMER_B_CAPTURECOMPARE_REGISTER_1, low_side_ticks);
            current_duty_phase1 = duty_ticks;
            break;
            
        case PWM_PHASE_2:
            Timer_B_setCompareValue(TIMER_B0_BASE, TIMER_B_CAPTURECOMPARE_REGISTER_6, duty_ticks);
            Timer_B_setCompareValue(TIMER_B0_BASE, TIMER_B_CAPTURECOMPARE_REGISTER_4, low_side_ticks);
            current_duty_phase2 = duty_ticks;
            break;
            
        case PWM_BOTH_PHASES:
            Timer_B_setCompareValue(TIMER_B0_BASE, TIMER_B_CAPTURECOMPARE_REGISTER_2, duty_ticks);
            Timer_B_setCompareValue(TIMER_B0_BASE, TIMER_B_CAPTURECOMPARE_REGISTER_1, low_side_ticks);
            Timer_B_setCompareValue(TIMER_B0_BASE, TIMER_B_CAPTURECOMPARE_REGISTER_6, duty_ticks);
            Timer_B_setCompareValue(TIMER_B0_BASE, TIMER_B_CAPTURECOMPARE_REGISTER_4, low_side_ticks);
            current_duty_phase1 = duty_ticks;
            current_duty_phase2 = duty_ticks;
            break;
    }

}

void HAL_PWM_Start(void)
{
    if(!pwm_running)
    {
        Timer_B_startCounter(TIMER_B0_BASE, TIMER_B_UPDOWN_MODE);
        pwm_running = true;
    }
}

void HAL_PWM_Stop(void)
{
    if(pwm_running)
    {
        // Safe shutdown: Set duty to 0 before stopping
        HAL_PWM_SetDuty(PWM_BOTH_PHASES, 0);
        
        // Delay for final PWM cycle (~5us)
        __delay_cycles(40);  // 5us at 8MHz
        
        // Stop timer
        Timer_B_stop(TIMER_B0_BASE);
        
        // Force all outputs low
        GPIO_setAsOutputPin(PW_H1_PORT, PW_H1_PIN);
        GPIO_setOutputLowOnPin(PW_H1_PORT, PW_H1_PIN);
        GPIO_setAsOutputPin(PW_L1_PORT, PW_L1_PIN);
        GPIO_setOutputLowOnPin(PW_L1_PORT, PW_L1_PIN);
        GPIO_setAsOutputPin(PW_H2_PORT, PW_H2_PIN);
        GPIO_setOutputLowOnPin(PW_H2_PORT, PW_H2_PIN);
        GPIO_setAsOutputPin(PW_L2_PORT, PW_L2_PIN);
        GPIO_setOutputLowOnPin(PW_L2_PORT, PW_L2_PIN);
        
        pwm_running = false;
    }
}

void HAL_PWM_EnableDrivers(bool panel_en, bool load_en) {
    if (panel_en) {
        GPIO_setOutputHighOnPin(P_EN_PORT, P_EN_PIN);
    } else {
        GPIO_setOutputLowOnPin(P_EN_PORT, P_EN_PIN);
    }

    if (load_en) {
        GPIO_setOutputHighOnPin(L_EN_PORT, L_EN_PIN);
    } else {
        GPIO_setOutputLowOnPin(L_EN_PORT, L_EN_PIN);
    }
}

uint16_t HAL_PWM_GetDuty(PWM_Phase_t phase)
{
    if(phase == PWM_PHASE_1)
        return current_duty_phase1;
    else if(phase == PWM_PHASE_2)
        return current_duty_phase2;
    else
        return (current_duty_phase1 + current_duty_phase2) >> 1;  // Average (div by 2)
}

bool HAL_PWM_IsRunning(void)
{
    return pwm_running;
}

// clamp duty cycle to safe operating range
static inline uint16_t clamp_duty_ticks(uint16_t duty_ticks)
{
    if(duty_ticks > PWM_MAX_DUTY_TICKS)
        return PWM_MAX_DUTY_TICKS;  // 36 ticks (90%)
    else if(duty_ticks < PWM_MIN_DUTY_TICKS)
        return PWM_MIN_DUTY_TICKS;  // 2 ticks (5%)
    else
        return duty_ticks;
}

// calculate low-side CCR value with dead-time
static inline uint16_t calculate_low_side_ticks(uint16_t high_side_ticks)
{
    uint16_t low_side = high_side_ticks + PWM_DEADTIME_TICKS;

    if(low_side > PWM_PERIOD_TICKS)
        low_side = PWM_PERIOD_TICKS;

    return low_side;
}
