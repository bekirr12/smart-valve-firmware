/*
 * hal_motor.c
 * Implementation of Motor Driver with PWM & Encoder
 */

#include <msp430.h>
#include <driverlib.h>
#include "hal_board.h"
#include "hal_motor.h"

// PWM Settings (20 kHz for Standard DC Motor)
// 8 MHz / 20 kHz = 400 ticks
#define MOTOR_PWM_PERIOD 400

static volatile uint32_t encoder_counter = 0;

void HAL_Motor_Init(void) {
    // 1. Configure GPIO Outputs
    GPIO_setAsOutputPin(MOTOR_POWER_EN_PORT, MOTOR_POWER_EN_PIN); // L_EN
    GPIO_setAsOutputPin(MOTOR_LOGIC_EN_PORT, MOTOR_LOGIC_EN_PIN); // ENABLE
    GPIO_setAsOutputPin(MOTOR_DIR_PORT, MOTOR_DIR_PIN);           // DIR
    GPIO_setAsOutputPin(MOTOR_BRAKE_PORT, MOTOR_BRAKE_PIN);       // BRAKE
    
    // PWM Pin
    GPIO_setAsPeripheralModuleFunctionOutputPin(
        MOTOR_PWM_PORT, MOTOR_PWM_PIN, GPIO_PRIMARY_MODULE_FUNCTION
    );

    // 2. Configure Inputs (Fault & Encoder)
    GPIO_setAsInputPinWithPullUpResistor(MOTOR_FAULT_PORT, MOTOR_FAULT_PIN);
    
    // Encoder Interrupt Setup (P1.2)
    GPIO_setAsInputPinWithPullUpResistor(MOTOR_ENC_PORT, MOTOR_ENC_PIN);
    GPIO_selectInterruptEdge(MOTOR_ENC_PORT, MOTOR_ENC_PIN, GPIO_HIGH_TO_LOW_TRANSITION);
    GPIO_clearInterrupt(MOTOR_ENC_PORT, MOTOR_ENC_PIN);
    GPIO_enableInterrupt(MOTOR_ENC_PORT, MOTOR_ENC_PIN);

    // 3. Initialize Timer for PWM (Using Timer_B3 as example for P9.1)
    // NOTE: Check datasheet if P9.1 is TB3. If not, adjust Timer Base.
    Timer_B_initUpModeParam initParam = {0};
    initParam.clockSource = TIMER_B_CLOCKSOURCE_SMCLK;
    initParam.clockSourceDivider = TIMER_B_CLOCKSOURCE_DIVIDER_1;
    initParam.timerPeriod = MOTOR_PWM_PERIOD;
    initParam.timerInterruptEnable_TBIE = TIMER_B_TBIE_INTERRUPT_DISABLE;
    initParam.captureCompareInterruptEnable_CCR0_CCIE = TIMER_B_CCIE_CCR0_INTERRUPT_DISABLE;
    initParam.timerClear = TIMER_B_DO_CLEAR;
    initParam.startTimer = false;
    Timer_B_initUpMode(TIMER_B3_BASE, &initParam); // Assuming Timer B3

    // Configure Compare Mode (PWM Channel)
    Timer_B_initCompareModeParam cmpParam = {0};
    cmpParam.compareRegister = TIMER_B_CAPTURECOMPARE_REGISTER_2; // TB3.2 -> P9.1 (Check Datasheet!)
    cmpParam.compareOutputMode = TIMER_B_OUTPUTMODE_RESET_SET;
    cmpParam.compareValue = 0;
    Timer_B_initCompareMode(TIMER_B3_BASE, &cmpParam);

    Timer_B_startCounter(TIMER_B3_BASE, TIMER_B_UP_MODE);

    // 4. Initial Safe State
    HAL_Motor_SetMainPower(false); // Power OFF
    HAL_Motor_Move(MOTOR_STOP, 0);
    HAL_Motor_Brake(true); // Parked
}

void HAL_Motor_SetMainPower(bool enable) {
    if (enable) {
        GPIO_setOutputHighOnPin(MOTOR_POWER_EN_PORT, MOTOR_POWER_EN_PIN);
    } else {
        GPIO_setOutputLowOnPin(MOTOR_POWER_EN_PORT, MOTOR_POWER_EN_PIN);
    }
}

void HAL_Motor_Move(MotorDir_t dir, uint8_t speed_percent) {
    if (speed_percent > 100) speed_percent = 100;
    
    // Logic Enable ON
    GPIO_setOutputHighOnPin(MOTOR_LOGIC_EN_PORT, MOTOR_LOGIC_EN_PIN);
    
    // Set Direction
    if (dir == MOTOR_OPENING) {
        GPIO_setOutputHighOnPin(MOTOR_DIR_PORT, MOTOR_DIR_PIN);
        HAL_Motor_Brake(false);
    } else if (dir == MOTOR_CLOSING) {
        GPIO_setOutputLowOnPin(MOTOR_DIR_PORT, MOTOR_DIR_PIN);
        HAL_Motor_Brake(false);
    } else {
        speed_percent = 0;
        // Logic Enable OFF when stopped
        GPIO_setOutputLowOnPin(MOTOR_LOGIC_EN_PORT, MOTOR_LOGIC_EN_PIN);
    }

    // Set PWM
    uint16_t duty = (uint16_t)(((uint32_t)MOTOR_PWM_PERIOD * speed_percent) / 100);
    Timer_B_setCompareValue(TIMER_B3_BASE, TIMER_B_CAPTURECOMPARE_REGISTER_2, duty);
}

void HAL_Motor_Brake(bool enable) {
    if (enable) {
        HAL_Motor_Move(MOTOR_STOP, 0); // PWM 0
        GPIO_setOutputHighOnPin(MOTOR_BRAKE_PORT, MOTOR_BRAKE_PIN);
    } else {
        GPIO_setOutputLowOnPin(MOTOR_BRAKE_PORT, MOTOR_BRAKE_PIN);
    }
}

uint32_t HAL_Motor_GetEncoderCount(void) {
    return encoder_counter;
}

void HAL_Motor_ResetEncoder(void) {
    encoder_counter = 0;
}

bool HAL_Motor_IsFault(void) {
    // Active Low or High? Assuming Active High fault.
    return (GPIO_getInputPinValue(MOTOR_FAULT_PORT, MOTOR_FAULT_PIN) == GPIO_INPUT_PIN_HIGH);
}

// ISR for Encoder Counting
#pragma vector=MOTOR_ENC_VECTOR
__interrupt void Port1_ISR(void) {
    if (GPIO_getInterruptStatus(MOTOR_ENC_PORT, MOTOR_ENC_PIN)) {
        encoder_counter++;
        GPIO_clearInterrupt(MOTOR_ENC_PORT, MOTOR_ENC_PIN);
    }
}