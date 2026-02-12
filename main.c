/*
 * main.c
 * Smart Valve Project - Master Firmware
 * Architecture: Non-blocking Super Loop with Time Slicing
 */

#include "main.h"

// --- Global System Timer ---
volatile uint32_t system_millis = 0;

// --- Task Timestamps (Scheduling) ---
static uint32_t last_mppt_time = 0;
static uint32_t last_valve_time = 0;
static uint32_t last_flow_time = 0;

// --- System Initialization ---
void System_Init(void) {
    // 1. Stop Watchdog Timer (Prevent reset during init)
    WDT_A_hold(WDT_A_BASE);

    // 2. Power Management (Unlock GPIOs after power-up)
    PMM_unlockLPM5();

    // 3. Clock Setup (Set DCO to 8 MHz)
    CS_setDCOFreq(CS_DCORSEL_0, CS_DCOFSEL_6); 
    CS_initClockSignal(CS_SMCLK, CS_DCOCLK, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_MCLK, CS_DCOCLK, CS_CLOCK_DIVIDER_1);

    // 4. Initialize Hardware Drivers
    HAL_ADC_Init();    // ADC for Sensors
    HAL_UART_Init();   // RS485 for Comms
    
    // 5. Initialize Application Logic
    MPPT_Init();       // Solar Charger
    Valve_Init();      // Valve Controller
    Telemetry_Init();  // Communication Protocol

    // 6. Initialize Flow Meter (Starts Booster & USS Lib)
    Flow_Init();       

    // 7. Setup System Tick Timer (Timer_A0 - 1ms Interrupt)
    // Clock: 8MHz / 8 = 1MHz. Period: 1000 ticks = 1ms.
    Timer_A_initUpModeParam param = {0};
    param.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    param.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_8;
    param.timerPeriod = 1000; 
    param.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
    param.captureCompareInterruptEnable_CCR0_CCIE = TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE;
    param.timerClear = TIMER_A_DO_CLEAR;
    param.startTimer = true;
    Timer_A_initUpMode(TIMER_A0_BASE, &param);

    // 8. Enable Global Interrupts
    __enable_interrupt();
}

// --- Main Application Loop ---
int main(void) {
    // Boot up the system
    System_Init();

    // Optional: Ensure valve is closed safely on startup
    // Valve_Close(); 

    while (1) {
        // =============================================================
        // TASK 1: COMMUNICATION (Priority: HIGH - Run Every Loop)
        // Must run frequently to catch RS485 bytes without delay.
        // =============================================================
        Telemetry_Process();

        // =============================================================
        // TASK 2: MPPT SOLAR CHARGER (Interval: 20ms)
        // Tracks Maximum Power Point for efficient charging.
        // =============================================================
        if ((system_millis - last_mppt_time) >= 20) {
            MPPT_Process();
            last_mppt_time = system_millis;
        }

        // =============================================================
        // TASK 3: VALVE CONTROL (Interval: 50ms)
        // Handles Soft-Start ramping and Stall Detection logic.
        // =============================================================
        if ((system_millis - last_valve_time) >= 50) {
            Valve_Process();
            last_valve_time = system_millis;
        }

        // =============================================================
        // TASK 4: FLOW METER & PROTECTION (Interval: 1000ms / 1 sec)
        // Measures water flow, checks for leaks and bursts.
        // =============================================================
        if ((system_millis - last_flow_time) >= 1000) {
            
            // 1. Measure Flow (Triggers Booster + USS)
            float flow_rate = Flow_Measure_LPH();

            // 2. Check if measurement was successful (Not -1.0)
            if (flow_rate >= 0.0f) {
                
                // A. Leak Detection: Valve is CLOSED but Flow exists
                if (Valve_GetState() == VALVE_IDLE_CLOSED) {
                    if (flow_rate > FLOW_LEAK_LIMIT_LPH) {
                        // LEAK DETECTED! 
                        // Action: Set a flag for Telemetry to report alarm.
                        // TODO: Global_Alarm_Flags |= ALARM_LEAK;
                    }
                }

                // B. Burst Protection: Flow is too high (Pipe Burst)
                // Even if valve is open, if flow exceeds physical limits, close it.
                if (flow_rate > FLOW_BURST_LIMIT_LPH) {
                    Valve_Close(); // EMERGENCY SHUTDOWN
                }
            }

            // Heartbeat: Toggle LED on P1.0 to show system is alive
            GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);

            last_flow_time = system_millis;
        }
    }
}

// --- System Tick Interrupt Service Routine (1ms) ---
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void) {
    system_millis++;
}