/*
 * telemetry.h
 * Smart Valve Project
 * Phase: 4 (Protocol Logic)
 * Purpose: Modbus RTU-like Protocol Handler
 */

#ifndef TELEMETRY_H_
#define TELEMETRY_H_

#include <stdint.h>
#include <stdbool.h>

// --- Device Configuration ---
#define DEVICE_ID           0x01  // Slave ID of this Valve

// --- Register Map (Address Book) ---
// READ ONLY Registers (Function 0x03)
#define REG_VALVE_STATE     0x0001
#define REG_BATT_VOLTAGE    0x0002
#define REG_PV_VOLTAGE      0x0003
#define REG_MOTOR_CURRENT   0x0004
#define REG_ENCODER_COUNT   0x0005

// WRITE ONLY Registers (Function 0x06)
#define REG_COMMAND_VALVE   0x0010 // 1: OPEN, 0: CLOSE

// --- Public API ---
void Telemetry_Init(void);

/*
 * Main process loop for communication.
 * Checks for incoming packets, validates CRC, and sends responses.
 * Should be called frequently in the main loop.
 */
void Telemetry_Process(void);

#endif /* TELEMETRY_H_ */