/**
 * @file mppt_manager.c
 * @brief MPPT Solar Charge Controller Module
 * @details Perturb & Observe algorithm for maximum power point tracking
 * @author Nuvo Tech Team - SmartValve Project
 * @version 1.0
 */

#include "mppt_manager.h"
#include "hal_board.h"
#include "hal_adc.h"
#include "hal_pwm.h"


// Private Variables
static MPPT_Status_t mppt_status;
static int8_t perturb_direction = 1;        // +1 or -1
static uint32_t last_power_mw = 0;
static uint8_t hysteresis_counter = 0;      // Prevents oscillation
static uint32_t state_entry_time_ms = 0;
static uint32_t last_process_time_ms = 0;

// External system time reference
extern volatile uint32_t system_millis;

// private functions
static void update_measurements(void);
static void check_faults(void);
static void perturb_and_observe(void);
static void constant_voltage_control(uint16_t target_mv);
static void auto_restart_check(void);

// -----------------------------------------------------------------------------
// PUBLIC FUNCTIONS
// -----------------------------------------------------------------------------

void MPPT_Init(void) {
    // Clear status structure
    memset(&mppt_status, 0, sizeof(MPPT_Status_t));
    
    mppt_status.state = MPPT_STATE_IDLE;
    mppt_status.fault = MPPT_FAULT_NONE;
    mppt_status.charging_enabled = false;
    mppt_status.duty_ticks = 0;
    
    // NOT: HAL_ADC_Init() ve HAL_PWM_Init() fonksiyonlarının 
    // main.c dosyasında çağrıldığı varsayılmıştır.
    
    // Disable drivers initially
    HAL_PWM_EnableDrivers(false, false);
    
    perturb_direction = 1;
    last_power_mw = 0;
    hysteresis_counter = 0;
    state_entry_time_ms = system_millis;
    last_process_time_ms = system_millis;
}

void MPPT_Process(void)
{
    uint32_t current_time_ms = system_millis;
    
    // Enforce 100ms update interval
    if((current_time_ms - last_process_time_ms) < MPPT_UPDATE_INTERVAL_MS)
        return;
    
    last_process_time_ms = current_time_ms;
    
    // Update measurements
    update_measurements();
    
    // Check faults
    check_faults();
    
    // Update runtime
    mppt_status.runtime_seconds = current_time_ms / 1000;
    
    // State machine
    switch(mppt_status.state)
    {
        case MPPT_STATE_IDLE:
            HAL_PWM_Stop();
            HAL_PWM_EnableDrivers(false, false);
            mppt_status.duty_ticks = 0;
            
            if(mppt_status.charging_enabled)
            {
                auto_restart_check();
            }
            break;
            
        case MPPT_STATE_STARTUP:
            HAL_PWM_EnableDrivers(true, true);
            
            if((current_time_ms - state_entry_time_ms) > MPPT_STARTUP_DELAY_MS)
            {
                // Start with 30% duty (12 ticks)
                mppt_status.duty_ticks = STARTUP_DUTY_TICKS;
                HAL_PWM_SetDuty(PWM_BOTH_PHASES, mppt_status.duty_ticks);
                HAL_PWM_Start();
                
                last_power_mw = mppt_status.pv_power_mw;
                hysteresis_counter = 0;
                
                mppt_status.state = MPPT_STATE_BULK;
                state_entry_time_ms = current_time_ms;
            }
            break;
            
        case MPPT_STATE_BULK:
            // Maximum Power Point Tracking
            perturb_and_observe();
            
            // Check for absorption transition
            if(mppt_status.battery_voltage_mv >= BATTERY_ABSORB_MV)
            {
                mppt_status.state = MPPT_STATE_ABSORB;
                state_entry_time_ms = current_time_ms;
            }
            break;
            
        case MPPT_STATE_ABSORB:
            // Constant voltage mode at 28.8V
            constant_voltage_control(BATTERY_ABSORB_MV);
            
            // Transition to float when current drops (e.g., < 5A)
            if(mppt_status.battery_current_ma < 5000)
            {
                mppt_status.state = MPPT_STATE_FLOAT;
                state_entry_time_ms = current_time_ms;
            }
            break;
            
        case MPPT_STATE_FLOAT:
            // Maintain float voltage (27.4V)
            constant_voltage_control(BATTERY_FLOAT_MV);
            
            // Return to bulk if voltage drops
            if(mppt_status.battery_voltage_mv < (BATTERY_FLOAT_MV - 500))
            {
                mppt_status.state = MPPT_STATE_BULK;
                state_entry_time_ms = current_time_ms;
            }
            break;
            
        case MPPT_STATE_NIGHT:
            HAL_PWM_Stop();
            mppt_status.duty_ticks = 0;
            
            // Check for sunrise
            if(mppt_status.pv_voltage_mv > 
               (mppt_status.battery_voltage_mv + PANEL_START_DELTA_MV))
            {
                mppt_status.state = MPPT_STATE_IDLE;
            }
            break;
            
        case MPPT_STATE_FAULT:
            HAL_PWM_Stop();
            HAL_PWM_EnableDrivers(false, false);
            mppt_status.duty_ticks = 0;
            break;
    }
    
    // Update energy counter (integer math)
    // Energy_increment = Power_mW × Time_s / 3600 / 1000
    // For 100ms interval: increment = (Power_mW × 100) / 3600000
    uint32_t energy_increment_uwh = (mppt_status.battery_power_mw * 100) / 3600;
    mppt_status.energy_today_wh += energy_increment_uwh / 1000;
}

void MPPT_GetStatus(MPPT_Status_t *status)
{
    if(status != NULL)
    {
        memcpy(status, &mppt_status, sizeof(MPPT_Status_t));
    }
}

void MPPT_EnableCharging(void)
{
    mppt_status.charging_enabled = true;
}

void MPPT_DisableCharging(void)
{
    mppt_status.charging_enabled = false;
    mppt_status.state = MPPT_STATE_IDLE;
    HAL_PWM_Stop();
    HAL_PWM_EnableDrivers(false, false);
}

void MPPT_EmergencyStop(void)
{
    mppt_status.state = MPPT_STATE_FAULT;
    mppt_status.fault = MPPT_FAULT_OVERCURRENT;
    HAL_PWM_Stop();
    HAL_PWM_EnableDrivers(false, false);
}

void MPPT_ClearFaults(void)
{
    if(mppt_status.state == MPPT_STATE_FAULT)
    {
        mppt_status.fault = MPPT_FAULT_NONE;
        mppt_status.state = MPPT_STATE_IDLE;
    }
}

void MPPT_ResetDailyEnergy(void)
{
    mppt_status.energy_today_wh = 0;
}

bool MPPT_IsCharging(void)
{
    return (mppt_status.state == MPPT_STATE_BULK ||
            mppt_status.state == MPPT_STATE_ABSORB ||
            mppt_status.state == MPPT_STATE_FLOAT);
}

// private functions 


static void update_measurements(void)
{
    HAL_ADC_Read();
    
    mppt_status.pv_voltage_mv      = HAL_ADC_GetPV_Voltage_mV();
    mppt_status.pv_current_ma      = HAL_ADC_GetPV_Current_mA();
    mppt_status.battery_voltage_mv = HAL_ADC_GetBatt_Voltage_mV();
    mppt_status.battery_current_ma = HAL_ADC_GetBatt_Current_mA();

    // Power calculation (integer math)
    // P(mW) = V(mV) × I(mA) / 1000
    mppt_status.pv_power_mw = ((uint32_t)mppt_status.pv_voltage_mv * (uint32_t)mppt_status.pv_current_ma) / 1000;
    mppt_status.battery_power_mw = ((uint32_t)mppt_status.battery_voltage_mv * (uint32_t)mppt_status.battery_current_ma) / 1000;

    // Update connection status
    mppt_status.panel_connected = (mppt_status.pv_voltage_mv > 5000);
    mppt_status.battery_connected = (mppt_status.battery_voltage_mv > 10000);
}

static void check_faults(void)
{
    /*
     * FAULT DETECTION (Integer Comparisons)
     * =====================================
     * All thresholds are in millivolts (integer constants)
     */
    
    // Over-voltage protection (28.8V + 0.5V hysteresis)
    if(mppt_status.battery_voltage_mv > (BATTERY_ABSORB_MV + 500))
    {
        mppt_status.fault = MPPT_FAULT_OVERVOLTAGE;
        mppt_status.state = MPPT_STATE_FAULT;
        HAL_PWM_Stop();
        return;
    }
    
    // Under-voltage (disconnect load only)
    if(mppt_status.battery_voltage_mv < BATTERY_LOW_MV &&
       mppt_status.battery_connected)
    {
        mppt_status.fault = MPPT_FAULT_UNDERVOLTAGE;
        HAL_PWM_EnableDrivers(true, false);  // Keep panel, disconnect load
    }
    else if(mppt_status.battery_voltage_mv > BATTERY_RECONNECT_MV)
    {
        if(mppt_status.fault == MPPT_FAULT_UNDERVOLTAGE)
        {
            mppt_status.fault = MPPT_FAULT_NONE;
            HAL_PWM_EnableDrivers(true, true);
        }
    }
    
    // PV short circuit
    if(mppt_status.pv_current_ma > 15000 && mppt_status.pv_voltage_mv < 5000)
    {
        mppt_status.fault = MPPT_FAULT_PV_SHORT;
        mppt_status.state = MPPT_STATE_FAULT;
        HAL_PWM_Stop();
    }
}

static void perturb_and_observe(void)
{
    // P&O Algorithm with Hysteresis

    uint32_t current_power_mw = mppt_status.pv_power_mw;
    int32_t delta_power = (int32_t)current_power_mw - (int32_t)last_power_mw;
    
    /*
     * P&O DECISION LOGIC
     * ==================
     * Only act if power change exceeds threshold (50mW)
     */
    if(delta_power > (int32_t)MPPT_POWER_THRESHOLD_MW)
    {
        // Power increased - good direction
        hysteresis_counter++;
        
        if(hysteresis_counter >= MPPT_HYSTERESIS_COUNT)
        {
            // Confirmed improvement - take step
            mppt_status.duty_ticks += (perturb_direction * MPPT_PERTURB_STEP_TICKS);
            hysteresis_counter = 0;
        }
    }
    else if(delta_power < -(int32_t)MPPT_POWER_THRESHOLD_MW)
    {
        // Power decreased - reverse direction
        perturb_direction = -perturb_direction;
        mppt_status.duty_ticks += (perturb_direction * MPPT_PERTURB_STEP_TICKS);
        hysteresis_counter = 0;
    }
    else
    {
        // Negligible change - reset counter
        hysteresis_counter = 0;
    }
    
    // Clamp to safe range (2-36 ticks)
    if(mppt_status.duty_ticks > MAX_DUTY_TICKS)
        mppt_status.duty_ticks = MAX_DUTY_TICKS;
    else if(mppt_status.duty_ticks < MIN_DUTY_TICKS)
        mppt_status.duty_ticks = MIN_DUTY_TICKS;
    
    // Apply to PWM hardware
    HAL_PWM_SetDuty(PWM_BOTH_PHASES, mppt_status.duty_ticks);
    
    last_power_mw = current_power_mw;
}

static void constant_voltage_control(uint16_t target_mv)
{
    
    int32_t voltage_error_mv = (int32_t)target_mv - 
                               (int32_t)mppt_status.battery_voltage_mv;
    
    // Proportional control (gain = 1/100)
    int16_t duty_adjustment = voltage_error_mv / 100;
    
    // Apply adjustment
    int16_t new_duty = (int16_t)mppt_status.duty_ticks + duty_adjustment;
    
    // Clamp
    if(new_duty > MAX_DUTY_TICKS)
        new_duty = MAX_DUTY_TICKS;
    else if(new_duty < MIN_DUTY_TICKS)
        new_duty = MIN_DUTY_TICKS;
    
    mppt_status.duty_ticks = (uint16_t)new_duty;
    HAL_PWM_SetDuty(PWM_BOTH_PHASES, mppt_status.duty_ticks);
}

static void auto_restart_check(void)
{
    // Check PV voltage sufficient
    if(mppt_status.pv_voltage_mv < PANEL_MIN_MV)
    {
        mppt_status.state = MPPT_STATE_NIGHT;
        return;
    }
    
    // Check PV > Battery + 1V
    if(mppt_status.pv_voltage_mv < 
       (mppt_status.battery_voltage_mv + PANEL_START_DELTA_MV))
    {
        return;
    }
    
    // Check battery connected
    if(!mppt_status.battery_connected)
    {
        return;
    }
    
    // All conditions met - start
    mppt_status.state = MPPT_STATE_STARTUP;
    state_entry_time_ms = system_millis;
}
