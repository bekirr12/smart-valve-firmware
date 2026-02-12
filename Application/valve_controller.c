/*
 * valve_controller.c
 * Robust Valve Control Implementation
 */

#include "valve_controller.h"
#include "hal_motor.h"
#include "hal_adc.h"
#include "hal_board.h"

static ValveState_t state = VALVE_IDLE_CLOSED; // Default assumption
static uint32_t process_timer = 0;
static uint32_t last_encoder_val = 0;
static uint8_t current_speed = 0;

// Helper to check stall
bool check_stall(void) {
    // 1. Current Check
    uint16_t raw = HAL_ADC_GetRawValue(VAL_LOAD_CURRENT_ADC_CH);
    // Convert to mA: (Raw * 5000) / 4095
    uint32_t current_ma = ((uint32_t)raw * 5000) / 4095;
    
    if (current_ma > VALVE_STALL_CURRENT_MA) return true;

    // 2. Encoder Check (Is it moving?)
    // Only check if speed is high enough (>50%)
    if (current_speed > 50) {
        uint32_t now_enc = HAL_Motor_GetEncoderCount();
        uint32_t diff = now_enc - last_encoder_val;
        last_encoder_val = now_enc;
        
        if (diff < VALVE_MIN_ENCODER_DIFF) return true; // Stuck!
    }
    
    return false;
}

void Valve_Init(void) {
    HAL_Motor_Init();
    state = VALVE_IDLE_CLOSED;
}

void Valve_Open(void) {
    if (state == VALVE_IDLE_CLOSED || state == VALVE_IDLE_OPEN) {
        state = VALVE_PRE_OPENING;
        process_timer = 0;
    }
}

void Valve_Close(void) {
    if (state == VALVE_IDLE_CLOSED || state == VALVE_IDLE_OPEN) {
        state = VALVE_PRE_CLOSING;
        process_timer = 0;
    }
}

void Valve_Process(void) {
    // Assuming 50ms interval call
    process_timer += 50; 

    // Safety: Check Driver Fault Pin
    if (HAL_Motor_IsFault()) {
        HAL_Motor_SetMainPower(false);
        state = VALVE_FAULT;
        return;
    }

    switch (state) {
        case VALVE_PRE_OPENING:
            // 1. Power ON Driver
            HAL_Motor_SetMainPower(true);
            // 2. Reset Encoder
            HAL_Motor_ResetEncoder();
            last_encoder_val = 0;
            current_speed = 20;
            // 3. Wait 100ms for power to stabilize
            if (process_timer > 100) {
                state = VALVE_OPENING;
                process_timer = 0; // Reset timer for timeout check
            }
            break;

        case VALVE_OPENING:
            // 1. Soft Start (Ramp Up)
            if (current_speed < 100) {
                current_speed += 5; // Ramp up
                HAL_Motor_Move(MOTOR_OPENING, current_speed);
            }

            // 2. Check Timeout
            if (process_timer > VALVE_TIMEOUT_MS) {
                state = VALVE_FAULT;
                HAL_Motor_Move(MOTOR_STOP, 0);
                HAL_Motor_SetMainPower(false);
            }

            // 3. Check Stall (End of Travel)
            // Ignore first 500ms (Inrush current)
            if (process_timer > 500) {
                if (check_stall()) {
                    // REACHED OPEN POSITION
                    HAL_Motor_Brake(true); // Lock it
                    HAL_Motor_SetMainPower(false); // Cut Power
                    state = VALVE_IDLE_OPEN;
                }
            }
            break;

        case VALVE_PRE_CLOSING:
            HAL_Motor_SetMainPower(true);
            HAL_Motor_ResetEncoder();
            last_encoder_val = 0;
            current_speed = 20;
            if (process_timer > 100) {
                state = VALVE_CLOSING;
                process_timer = 0;
            }
            break;

        case VALVE_CLOSING:
            if (current_speed < 100) {
                current_speed += 5;
                HAL_Motor_Move(MOTOR_CLOSING, current_speed);
            }

            if (process_timer > VALVE_TIMEOUT_MS) {
                state = VALVE_FAULT;
                HAL_Motor_Move(MOTOR_STOP, 0);
                HAL_Motor_SetMainPower(false);
            }

            if (process_timer > 500) {
                if (check_stall()) {
                    // REACHED CLOSED POSITION
                    HAL_Motor_Brake(true);
                    HAL_Motor_SetMainPower(false);
                    state = VALVE_IDLE_CLOSED;
                }
            }
            break;

        default:
            break;
    }
}

ValveState_t Valve_GetState(void) {
    return state;
}