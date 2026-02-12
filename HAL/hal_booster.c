/**
 * @file hal_booster.h
 * @brief Hardware Abstraction Layer - TIDA-01486 Booster Control
 * @details Control interface for ultrasonic analog front-end
 * @author Nuvo Tech Team - SmartValve Project
 * @version 1.0
 */

#include "hal_board.h"
#include "hal_booster.h"

static Booster_State_t current_state = BOOSTER_SLEEP;

void HAL_Booster_Init(void) {
    // Configure MUX control pins as outputs
    GPIO_setAsOutputPin(MUX_S1_PORT, MUX_S1_PIN);
    GPIO_setAsOutputPin(MUX_S2_PORT, MUX_S2_PIN);
    GPIO_setAsOutputPin(MUX_S3_PORT, MUX_S3_PIN);
    GPIO_setAsOutputPin(MUX_S4_PORT, MUX_S4_PIN);

    // Configure power-down pins as outputs
    GPIO_setAsOutputPin(PD1_PORT, PD1_PIN);
    GPIO_setAsOutputPin(PD2_PORT, PD2_PIN);

    // Initialize to sleep mode
    HAL_Booster_Control(BOOSTER_SLEEP);
}

void HAL_Booster_Control(Booster_State_t state) {

    current_state = state;

    switch(state)
    {
        case BOOSTER_PAIR1_ACTIVE:
            // Configure MUX for transducer pair 1
            // S1=1, S2=0, S3=1, S4=0, PD1=1, PD2 = 0
            GPIO_setOutputHighOnPin(MUX_S1_PORT, MUX_S1_PIN);
            GPIO_setOutputLowOnPin(MUX_S2_PORT, MUX_S2_PIN);
            GPIO_setOutputHighOnPin(MUX_S3_PORT, MUX_S3_PIN);
            GPIO_setOutputLowOnPin(MUX_S4_PORT, MUX_S4_PIN);
            GPIO_setOutputHighOnPin(PD1_PORT, PD1_PIN);
            GPIO_setOutputLowOnPin(PD2_PORT, PD2_PIN);
            break;

        case BOOSTER_PAIR2_ACTIVE:
            // Configure MUX for transducer pair 2
            // S1=0, S2=1, S3=0, S4=1, PD1=0, PD2 = 1
            GPIO_setOutputLowOnPin(MUX_S1_PORT, MUX_S1_PIN);
            GPIO_setOutputHighOnPin(MUX_S2_PORT, MUX_S2_PIN);
            GPIO_setOutputLowOnPin(MUX_S3_PORT, MUX_S3_PIN);
            GPIO_setOutputHighOnPin(MUX_S4_PORT, MUX_S4_PIN);
            GPIO_setOutputLowOnPin(PD1_PORT, PD1_PIN);
            GPIO_setOutputHighOnPin(PD2_PORT, PD2_PIN);
            break;

        case BOOSTER_TEST_MODE:
            // Configure MUX for test mode
            // S1=1, S2=1, S3=1, S4=1, PD1=1, PD2 = 1
            GPIO_setOutputHighOnPin(MUX_S1_PORT, MUX_S1_PIN);
            GPIO_setOutputHighOnPin(MUX_S2_PORT, MUX_S2_PIN);
            GPIO_setOutputHighOnPin(MUX_S3_PORT, MUX_S3_PIN);
            GPIO_setOutputHighOnPin(MUX_S4_PORT, MUX_S4_PIN);
            GPIO_setOutputHighOnPin(PD1_PORT, PD1_PIN);
            GPIO_setOutputHighOnPin(PD2_PORT, PD2_PIN);
            break;

        case BOOSTER_SLEEP:
            // Configure MUX for sleep
            // S1=0, S2=0, S3=0, S4=0, PD1=0, PD2 = 0
            GPIO_setOutputLowOnPin(MUX_S1_PORT, MUX_S1_PIN);
            GPIO_setOutputLowOnPin(MUX_S2_PORT, MUX_S2_PIN);
            GPIO_setOutputLowOnPin(MUX_S3_PORT, MUX_S3_PIN);
            GPIO_setOutputLowOnPin(MUX_S4_PORT, MUX_S4_PIN);
            GPIO_setOutputLowOnPin(PD1_PORT, PD1_PIN);
            GPIO_setOutputLowOnPin(PD2_PORT, PD2_PIN);
            break;
    }
}

// for state information request
bool HAL_Booster_IsActive(void)
{
    return (current_state != BOOSTER_SLEEP);
}