/**
 * @file hal_motor.h
 * @brief Hardware Abstraction Layer - Motor Control Interface
 * @details 0-10V Analog Drive via PWM for valve actuation
 * @author Nuvo Tech Team - SmartValve Project
 * @version 1.0
 */

#ifndef HAL_HAL_MOTOR_H_
#define HAL_HAL_MOTOR_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    MOTOR_STOP = 0,
    MOTOR_OPENING, // Forward
    MOTOR_CLOSING  // Reverse
} MotorDir_t;

void HAL_Motor_Init(void);  // Initializes Motor GPIOs, PWM Timer, and Encoder Interrupts.
void HAL_Motor_SetMainPower(bool enable);   // Enable motor power supply
void HAL_Motor_Move(MotorDir_t dir, uint8_t speed_percent); // Sets Logic Enable, Direction, and PWM Speed.
void HAL_Motor_Brake(bool enable);  // Engages the Electronic Brake
uint32_t HAL_Motor_GetEncoderCount(void);   // Returns the current Encoder Pulse Count.
void HAL_Motor_ResetEncoder(void);  // Resets the Encoder Counter to 0.
bool HAL_Motor_IsFault(void);   // Reads the Fault Pin (P1.3).

#endif /* HAL_HAL_MOTOR_H_ */