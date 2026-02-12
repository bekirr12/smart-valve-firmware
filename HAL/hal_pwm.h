/**
 * @file hal_pwm.H
 * @brief Hardware Abstraction Layer - PWM
 * @details TIMER_B peripheral implementation using TI DriverLib
 * @author Nuvo Tech Team - SmartValve Project
 * @version 1.0
 */

#ifndef HAL_PWM_H_
#define HAL_PWM_H_

#include <stdint.h>
#include <stdbool.h>

// pwm timing contants
#define PWM_PERIOD_TICKS    20  // Up-down mode: 0→20→0 = 40 total ticks
#define PWM_DEADTIME_TICKS  2   // 250ns @ 8MHz = 2 ticks
#define PWM_MAX_DUTY_TICKS  36  // 90% of 40 = 36 ticks (bootstrap limit)
#define PWM_MIN_DUTY_TICKS  2   // 5% of 40 = 2 ticks (minimum operation)
#define PWM_PHASE_OFFSET_TICKS  20  // 180° = 20 ticks

// pwm phase selection
typedef enum {
    PWM_PHASE_1 = 0,    // Phase 1 (CCR1, CCR2)
    PWM_PHASE_2 = 1,    // Phase 2 (CCR4, CCR6)
    PWM_BOTH_PHASES = 2 // Both phases
} PWM_Phase_t;

// Configures Timer_B0 in up-down mode for 200kHz interleaved buck
void HAL_PWM_Init(void);

// Set PWM duty cycle for specified phase
void HAL_PWM_SetDuty(PWM_Phase_t phase, uint16_t duty_ticks);

// starts pwm generation
void HAL_PWM_Start(void);

// Stop PWM generation
void HAL_PWM_Stop(void);

// Get current duty cycle
uint16_t HAL_PWM_GetDuty(PWM_Phase_t phase);

// Controls the Gate Driver Enable Pins
void HAL_PWM_EnableDrivers(bool panel_en, bool load_en);

// check if PWM is running
bool HAL_PWM_IsRunning(void);

#endif /* HAL_PWM_H_ */