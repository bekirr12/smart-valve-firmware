/**
 * @file hal_booster.h
 * @brief Hardware Abstraction Layer - TIDA-01486 Booster Control
 * @details Control interface for ultrasonic analog front-end
 * @author Nuvo Tech Team - SmartValve Project
 * @version 1.0
 */

#ifndef HAL_BOOSTER_H_
#define HAL_BOOSTER_H_

#include <msp430.h>
#include <driverlib.h>
#include <stdint.h>
#include <stdbool.h>

// Booster Durumları
typedef enum {
    BOOSTER_SLEEP = 0,          // All channels off, op-amps powered down
    BOOSTER_PAIR1_ACTIVE = 1,   // Transducer pair 1 active (S1=1, S3=1, PD1=1)
    BOOSTER_PAIR2_ACTIVE = 2,   // Transducer pair 2 active (S2=1, S4=1, PD2=1)
    BOOSTER_TEST_MODE = 3       // Test configuration (S1=1, S2=1, PD3=1, S4=1, PD1=1, PD2=1)
} Booster_State_t;

void HAL_Booster_Init(void);
void HAL_Booster_Control(Booster_State_t state);
void HAL_Booster_Sleep(void);
bool HAL_Booster_IsActive(void);

#endif /* HAL_BOOSTER_H_ */