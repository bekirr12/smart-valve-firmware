/*
 * flow_meter.h
 * Smart Valve Project
 * Phase: 1.5 (Ultrasonic Flow Meter & Booster Wrapper)
 * Purpose: Interface for USS Library and TIDA-01486 Hardware.
 */

#ifndef FLOW_METER_H_
#define FLOW_METER_H_

#include <stdint.h>
#include <stdbool.h>

// --- Configuration Thresholds ---
// Leak Limit: 10 Liters/Hour (If valve is closed and flow > 10, it's a leak)
#define FLOW_LEAK_LIMIT_LPH    10.0f  

// Burst Limit: 500 Liters/Hour (If flow > 500, pipe might be broken)
#define FLOW_BURST_LIMIT_LPH   500.0f 

// --- Public API ---

/*
 * Initializes the USS Software Library and TIDA-01486 Booster Pins.
 * Must be called once at startup.
 */
void Flow_Init(void);

/*
 * Performs a single ultrasonic measurement cycle.
 * 1. Wakes up Booster.
 * 2. Fires Ultrasonic Pulse (via TI Lib).
 * 3. Sleeps Booster.
 * 4. Calculates Flow Rate based on Time of Flight (ToF).
 * * @return: Flow Rate in Liters Per Hour (LPH). Returns -1.0f on error.
 */
float Flow_Measure_LPH(void);

/*
 * Returns the last valid flow rate measurement.
 */
float Flow_GetLastRate(void);

/*
 * Returns the accumulated total volume (m^3 or Liters depending on logic).
 */
float Flow_GetTotalVolume(void);

#endif /* FLOW_METER_H_ */