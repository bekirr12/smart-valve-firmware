/*
 * valve_controller.h
 * Phase: 3 - High Level Logic
 * Features: Soft-Start, Dual Stall Detection (Current + Encoder), Power Mgmt
 */

#ifndef VALVE_CONTROLLER_H_
#define VALVE_CONTROLLER_H_

#include <stdint.h>
#include <stdbool.h>

// --- Config ---
#define VALVE_TIMEOUT_MS        15000 // 15 Sec Max
#define VALVE_STALL_CURRENT_MA  1200  // 1.2A Stall Limit
#define VALVE_MIN_ENCODER_DIFF  5     // Minimum pulses expected per check interval

typedef enum {
    VALVE_IDLE_CLOSED,
    VALVE_IDLE_OPEN,
    VALVE_PRE_OPENING, // Power Up & Brake Release
    VALVE_OPENING,     // Moving
    VALVE_PRE_CLOSING,
    VALVE_CLOSING,
    VALVE_FAULT
} ValveState_t;

void Valve_Init(void);
void Valve_Open(void);
void Valve_Close(void);

// Call this function periodically (e.g., every 50ms)
void Valve_Process(void);

ValveState_t Valve_GetState(void);

#endif