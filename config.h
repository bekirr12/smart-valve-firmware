/*
 * config.h — Central configuration for the Smart Valve firmware.
 *
 * Single home for tunable constants: clock frequencies, pin assignments,
 * timing intervals, and thresholds. This file grows one section at a time
 * as each development phase is implemented (see CLAUDE.md §9 roadmap).
 *
 * Currently implemented: Phase 1 (clock), Phase 2 (LEDs + buttons).
 */

#ifndef CONFIG_H_
#define CONFIG_H_

/* =====================================================================
 * CLOCK SYSTEM (Phase 1)
 * ---------------------------------------------------------------------
 * The board has three external crystals:
 *   - HF  crystal: 8 MHz     -> drives MCLK (CPU) and SMCLK (fast periph.)
 *   - LF  crystal: 32.768 kHz-> drives ACLK (RTC, low-power wakeup)
 *   - USS crystal: 8 MHz     -> used ONLY by the USS module (Phase 11),
 *                               initialized by USSLib, NOT here.
 * ===================================================================== */

/* External crystal frequencies, in Hz. Must match the physical crystals. */
#define CONFIG_HFXT_FREQ_HZ     8000000UL   /* 8 MHz  high-frequency crystal */
#define CONFIG_LFXT_FREQ_HZ     32768UL     /* 32.768 kHz low-freq crystal   */

/* Resulting system clock frequencies after clock_init().
 * MCLK  = HFXT      = 8 MHz  (CPU core)
 * SMCLK = HFXT      = 8 MHz  (UART, I2C, ADC, timers)
 * ACLK  = LFXT      = 32.768 kHz (RTC, low-power timing)                 */
#define CONFIG_MCLK_FREQ_HZ     CONFIG_HFXT_FREQ_HZ
#define CONFIG_SMCLK_FREQ_HZ    CONFIG_HFXT_FREQ_HZ
#define CONFIG_ACLK_FREQ_HZ     CONFIG_LFXT_FREQ_HZ

/* =====================================================================
 * LEDs & BUTTONS (Phase 2)
 * ---------------------------------------------------------------------
 * Each signal is a (PORT, PIN) pair using driverlib constants, so code
 * reads GPIO_setOutputHighOnPin(LED1_PORT, LED1_PIN), etc.
 *
 * Polarities (confirmed against the hardware):
 *   - LEDs    active-high (pin HIGH = LED on).
 *   - Buttons active-low  (wired to GND, internal pull-up, pressed = LOW).
 *
 * Other pins (motor, USS, RS485, ±15V, LT8490, HMI) are added in their
 * own phases, not here.
 * ===================================================================== */

/* --- LEDs (outputs, active-high) --- */
#define LED1_PORT       GPIO_PORT_P4
#define LED1_PIN        GPIO_PIN2
#define LED2_PORT       GPIO_PORT_P4
#define LED2_PIN        GPIO_PIN1
#define LED3_PORT       GPIO_PORT_P4
#define LED3_PIN        GPIO_PIN0

/* --- Buttons (inputs, pull-up, active-low) --- */
#define BTN_DOWN_PORT   GPIO_PORT_P5
#define BTN_DOWN_PIN    GPIO_PIN0
#define BTN_SELECT_PORT GPIO_PORT_P5
#define BTN_SELECT_PIN  GPIO_PIN1
#define BTN_LEFT_PORT   GPIO_PORT_P5
#define BTN_LEFT_PIN    GPIO_PIN2
#define BTN_UP_PORT     GPIO_PORT_P5
#define BTN_UP_PIN      GPIO_PIN3
#define BTN_RIGHT_PORT  GPIO_PORT_P5
#define BTN_RIGHT_PIN   GPIO_PIN4

/* =====================================================================
 * RS485 UART (Phase 3)   -- eUSCI_A0, our debug output link
 * ---------------------------------------------------------------------
 * Half-duplex: the transceiver's EN pin selects direction.
 *   RS485_EN HIGH = TX (drive the bus), LOW = RX (listen).
 * We raise EN before sending and lower it once the last byte has fully
 * shifted out.
 *
 * Baud-rate generator values below are a MATCHED SET for 9600 baud from
 * an 8 MHz SMCLK using oversampling. If you change the baud or SMCLK, all
 * three (UCBRx / UCBRFx / UCBRSx) must be recalculated (see the family
 * user guide baud-rate table).
 * ===================================================================== */

#define RS485_TX_PORT   GPIO_PORT_P4      /* P4.3 = UCA0TXD */
#define RS485_TX_PIN    GPIO_PIN3
#define RS485_RX_PORT   GPIO_PORT_P4      /* P4.4 = UCA0RXD (used from Ph.9) */
#define RS485_RX_PIN    GPIO_PIN4
#define RS485_EN_PORT   GPIO_PORT_P4
#define RS485_EN_PIN    GPIO_PIN5

#define RS485_BAUD          9600UL        /* target baud rate            */
#define RS485_BR_PRESCALAR  52            /* UCBRx  for 9600 @ 8 MHz     */
#define RS485_BR_FIRSTMOD   1             /* UCBRFx (oversampling)       */
#define RS485_BR_SECONDMOD  0x49          /* UCBRSx (fractional modulation) */

/* =====================================================================
 * ADC & SENSORS (Phase 4)   -- ADC12_B, internal 2.5 V reference
 * ---------------------------------------------------------------------
 * Five analog measurements, all single-ended against the internal 2.5 V
 * reference. Raw 12-bit code -> volts:  Vadc = raw / 4096 * 2.5.
 * drivers/sensors.c then applies each channel's scaling formula.
 *
 * Analog pins must be switched to their analog function (both SELx bits
 * set = GPIO_TERNARY_MODULE_FUNCTION).
 * ===================================================================== */

#define ADC_VREF_VOLTS      2.5f          /* internal reference voltage   */
#define ADC_FULL_SCALE      4096.0f       /* 12-bit ADC (2^12)            */

/* ADC channel inputs (driverlib ADC12_B_INPUT_Ax constants). */
#define ADC_PANEL_V_CH      ADC12_B_INPUT_A2    /* P1.4 */
#define ADC_BATT_V_CH       ADC12_B_INPUT_A3    /* P1.5 */
#define ADC_PANEL_I_CH      ADC12_B_INPUT_A11   /* P8.5 */
#define ADC_BATT_I_CH       ADC12_B_INPUT_A12   /* P8.6 */
#define ADC_MOTOR_I_CH      ADC12_B_INPUT_A15   /* P2.3 */

/* Analog input pins (for switching the pad to analog mode). */
#define ADC_PANEL_V_PORT    GPIO_PORT_P1
#define ADC_PANEL_V_PIN     GPIO_PIN4
#define ADC_BATT_V_PORT     GPIO_PORT_P1
#define ADC_BATT_V_PIN      GPIO_PIN5
#define ADC_PANEL_I_PORT    GPIO_PORT_P8
#define ADC_PANEL_I_PIN     GPIO_PIN5
#define ADC_BATT_I_PORT     GPIO_PORT_P8
#define ADC_BATT_I_PIN      GPIO_PIN6
#define ADC_MOTOR_I_PORT    GPIO_PORT_P2
#define ADC_MOTOR_I_PIN     GPIO_PIN3

/* =====================================================================
 * POWER MANAGEMENT + RTC (Phase 5)
 * ---------------------------------------------------------------------
 * The RTC (clocked from the LF crystal / ACLK) produces a 1 Hz tick; the
 * ISR counts ticks and raises a "measurement due" flag every
 * MEASURE_INTERVAL_S seconds, which wakes the CPU from low-power sleep.
 * ===================================================================== */

#define MEASURE_INTERVAL_S   10    /* seconds between wake-ups.
                                    * Use a small value (e.g. 3) to test
                                    * the wake cycle without waiting a minute. */

#endif /* CONFIG_H_ */
