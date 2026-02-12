/**
 * @file mppt_manager.h
 * @brief MPPT Solar Charge Controller Module
 * @details Perturb & Observe algorithm for maximum power point tracking
 * @author Nuvo Tech Team - SmartValve Project
 * @version 1.1 (Sanitized)
 */

#ifndef MPPT_MANAGER_H_
#define MPPT_MANAGER_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

// --- Configuration Constants ---
#define BATTERY_ABSORB_MV           28800   // 28.8V - Maximum charging voltage
#define BATTERY_FLOAT_MV            27400   // 27.4V - Float voltage
#define BATTERY_LOW_MV              21000   // 21.0V - Low battery cutoff
#define BATTERY_RECONNECT_MV        24000   // 24.0V - Reconnect threshold

#define PANEL_MIN_MV                25000   // 25.0V - Minimum PV to start
#define PANEL_START_DELTA_MV        1000    // 1.0V - PV > Battery + 1V to start

#define MAX_DUTY_TICKS              36      // 90% of 40 = 36 ticks
#define MIN_DUTY_TICKS              2       // 5% of 40 = 2 ticks
#define STARTUP_DUTY_TICKS          12      // 30% of 40 = 12 ticks

#define MPPT_PERTURB_STEP_TICKS     1       // ±1 tick per iteration
#define MPPT_UPDATE_INTERVAL_MS     100     // 100ms algorithm cycle
#define MPPT_STARTUP_DELAY_MS       1000    // 1 second startup delay

#define MPPT_POWER_THRESHOLD_MW     50      // 50mW minimum change to act
#define MPPT_HYSTERESIS_COUNT       3       // Must see improvement 3x before stepping


// --- Enums ---
typedef enum {
    MPPT_STATE_IDLE = 0,
    MPPT_STATE_STARTUP,
    MPPT_STATE_BULK,
    MPPT_STATE_ABSORB,
    MPPT_STATE_FLOAT,
    MPPT_STATE_FAULT,
    MPPT_STATE_NIGHT
} MPPT_State_t;

typedef enum {
    MPPT_FAULT_NONE = 0,
    MPPT_FAULT_OVERVOLTAGE,
    MPPT_FAULT_UNDERVOLTAGE,
    MPPT_FAULT_OVERCURRENT,
    MPPT_FAULT_PV_SHORT
} MPPT_Fault_t;

// --- Structures ---
typedef struct {
    // State Machine
    MPPT_State_t state;
    MPPT_Fault_t fault;

    // Measurements (all integers)
    uint16_t pv_voltage_mv;         // Panel voltage (mV)
    uint16_t pv_current_ma;         // Panel current (mA)
    uint32_t pv_power_mw;           // Panel power (mW)
    
    uint16_t battery_voltage_mv;    // Battery voltage (mV)
    uint16_t battery_current_ma;    // Battery current (mA)
    uint32_t battery_power_mw;      // Battery power (mW)

    uint16_t duty_ticks;            // Current PWM duty (0-40 ticks)

    // Statistics
    uint32_t energy_today_wh;       // Energy harvested (Wh)
    uint32_t runtime_seconds;       // Total runtime

    // Control Flags
    bool charging_enabled;
    bool panel_connected;
    bool battery_connected;
} MPPT_Status_t;

// --- Public Functions ---

/**
 * @brief Initialize MPPT controller (integer-only)
 */
void MPPT_Init(void);

/**
 * @brief Execute MPPT algorithm (call every 100ms)
 * @details Integer-only P&O with hysteresis for low resolution
 */
void MPPT_Process(void);

/**
 * @brief Get current MPPT status
 * @param status Pointer to status structure
 */
void MPPT_GetStatus(MPPT_Status_t *status);

/**
 * @brief Enable charging
 */
void MPPT_EnableCharging(void);

/**
 * @brief Disable charging
 */
void MPPT_DisableCharging(void);

/**
 * @brief Emergency stop
 */
void MPPT_EmergencyStop(void);

/**
 * @brief Clear faults
 */
void MPPT_ClearFaults(void);

/**
 * @brief Reset daily energy counter
 */
void MPPT_ResetDailyEnergy(void);

/**
 * @brief Check if actively charging
 * @return true if charging, false otherwise
 */
bool MPPT_IsCharging(void);

#endif /* MPPT_MANAGER_H_ */