/**
 * @file hal_board.h
 * @brief Hardware Abstraction Layer - Board Pin Definitions
 * @details Global pin mapping for SmartValve
 * @author Bekir Sami Karatas - SmartValve Project
 * @version 1.0
 */

#ifndef HAL_BOARD_H_
#define HAL_BOARD_H_

#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>

// =============================================================
// 1. POWER SYSTEM (MPPT & ADC INPUTS)
// =============================================================
// PV Voltage (P+) -> P1.5 (A3)
#define PV_V_PORT           GPIO_PORT_P1
#define PV_V_PIN            GPIO_PIN5
#define PV_V_CHANNEL        ADC12_B_INPUT_A3

// Battery Voltage (B+) -> P1.6 (A4)
#define BATT_V_PORT         GPIO_PORT_P1
#define BATT_V_PIN          GPIO_PIN6
#define BATT_V_CHANNEL      ADC12_B_INPUT_A4

// PV Current (P_I) -> P1.7 (A5)
#define PV_I_PORT           GPIO_PORT_P1
#define PV_I_PIN            GPIO_PIN7
#define PV_I_CHANNEL        ADC12_B_INPUT_A5

// Battery Current (B_I) -> P2.0 (A6)
#define BATT_I_PORT         GPIO_PORT_P2
#define BATT_I_PIN          GPIO_PIN0
#define BATT_I_CHANNEL      ADC12_B_INPUT_A6

// Load/Motor Current (L_I) -> P2.1 (A7)
#define LOAD_I_PORT         GPIO_PORT_P2
#define LOAD_I_PIN          GPIO_PIN1
#define LOAD_I_CHANNEL      ADC12_B_INPUT_A7

// =============================================================
// 2. MPPT PWM CONTROL (TIMER_B0)
// =============================================================
// PW_L1 -> P3.1 (TB0.1)
#define PW_L1_PORT     GPIO_PORT_P3
#define PW_L1_PIN      GPIO_PIN1

// PW_H1 -> P3.2 (TB0.2)
#define PW_H1_PORT     GPIO_PORT_P3
#define PW_H1_PIN      GPIO_PIN2

// PW_L2 -> P3.5 (TB0.4)
#define PW_L2_PORT     GPIO_PORT_P3
#define PW_L2_PIN      GPIO_PIN5

// PW_H2 -> P3.7 (TB0.6)
#define PW_H2_PORT     GPIO_PORT_P3
#define PW_H2_PIN      GPIO_PIN7


// =============================================================
// 3. POWER SWITCHES (GPIO ENABLE)
// =============================================================
// P_EN (Panel MOSFET Gate) -> P3.4 (TB0OUTH - PWM Capable)
#define P_EN_PORT      GPIO_PORT_P3
#define P_EN_PIN       GPIO_PIN4

// L_EN (Load/Motor Enable) -> P3.3
#define L_EN_PORT      GPIO_PORT_P3
#define L_EN_PIN       GPIO_PIN3


// =============================================================
// 3. MOTOR & VALVE CONTROL PINS
// =============================================================
// Speed Control (PWM) -> P3.5 (TB0.4)
#define MOTOR_PWM_PORT              GPIO_PORT_P3
#define MOTOR_PWM_PIN               GPIO_PIN5

// Direction Control -> P9.0
#define MOTOR_DIR_PORT              GPIO_PORT_P9
#define MOTOR_DIR_PIN               GPIO_PIN0

// Motor Enable -> P9.1
#define MOTOR_ENABLE_PORT           GPIO_PORT_P9
#define MOTOR_ENABLE_PIN            GPIO_PIN1

// Brake Control -> P9.2
#define MOTOR_BRAKE_PORT            GPIO_PORT_P9
#define MOTOR_BRAKE_PIN             GPIO_PIN2

// Encoder Input -> P3.6
#define MOTOR_ENCODER_PORT          GPIO_PORT_P3
#define MOTOR_ENCODER_PIN           GPIO_PIN6

// Fault Input -> P3.7
#define MOTOR_FAULT_PORT            GPIO_PORT_P2
#define MOTOR_FAULT_PIN             GPIO_PIN7


// =============================================================
// 4. COMMUNICATION (RS485 - Low Power UART)
// =============================================================
// Mapping based on "Pin 47/48" typically being P4.x or P5.x on 100-pin.
// Assuming Port 4 for RS485 Transceiver.
#define RS485_TX_PORT               GPIO_PORT_P4
#define RS485_TX_PIN                GPIO_PIN2
#define RS485_RX_PORT               GPIO_PORT_P4
#define RS485_RX_PIN                GPIO_PIN3

// RS485 Enable (RE/DE) -> Assumed P4.4
#define RS485_EN_PORT               GPIO_PORT_P4
#define RS485_EN_PIN                GPIO_PIN4


// =============================================================
// 5. BOOSTER & MUX PINS (TIDA-01486 Implementation)
// =============================================================
// S1 (Mux) -> P6.2
#define MUX_S1_PORT             GPIO_PORT_P6
#define MUX_S1_PIN              GPIO_PIN2

// S2 (Mux) -> P6.3
#define MUX_S2_PORT             GPIO_PORT_P6
#define MUX_S2_PIN              GPIO_PIN3

// S3 (Mux) -> P8.0
#define MUX_S3_PORT             GPIO_PORT_P8
#define MUX_S3_PIN              GPIO_PIN0

// S4 (Mux) -> P8.1
#define MUX_S4_PORT             GPIO_PORT_P8
#define MUX_S4_PIN              GPIO_PIN1

// PD1 (OpAmp Power Down 1) -> P8.2
#define PD1_PORT         GPIO_PORT_P8
#define PD1_PIN          GPIO_PIN2

// PD2 (OpAmp Power Down 2) -> P8.3
#define PD2_PORT         GPIO_PORT_P8
#define PD2_PIN          GPIO_PIN3


#endif /* HAL_BOARD_H_ */