/*
 * main.h
 * Smart Valve Project
 * Phase: 5 (System Integration)
 * Purpose: Global definitions, module includes, and system settings.
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <msp430.h>
#include <driverlib.h>

// --- 1. Hardware Abstraction Layers (HAL) ---
#include "hal_board.h"        // Pin Definitions
#include "hal_uart.h"         // RS485 Communication Driver
#include "hal_adc.h"          // Analog Sensor Driver
#include "hal_motor.h"        // Motor Driver (PWM + GPIO)
#include "hal_booster.h"      // TIDA-01486 Booster Driver

// --- 2. Application Modules ---
#include "mppt_manager.h"     // Solar MPPT Logic
#include "valve_controller.h" // Valve Logic (Open/Close/Stall)
#include "telemetry.h"        // Modbus Protocol Handler
#include "flow_meter.h"       // Ultrasonic Flow Meter Logic

// --- 3. System Clock Settings ---
#define MCLK_FREQ_HZ    8000000 // 8 MHz Master Clock
#define SMCLK_FREQ_HZ   8000000 // 8 MHz Sub-Master Clock

// --- 4. Global System Ticker ---
// Incremented every 1ms by Timer_A0 ISR
extern volatile uint32_t system_millis;

#endif /* MAIN_H_ */
