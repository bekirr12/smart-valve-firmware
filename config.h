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

/* --- Sensor scaling (calibration) --------------------------------------
 * From the board's divider / shunt / gain values. drivers/sensors.c applies
 * these to Vadc (the voltage at the ADC pin). Change these if the board's
 * component values change.
 *
 *   Voltage:      V = Vadc * (Rtop + Rbot) / Rbot
 *   Panel I:      I = (Vadc / RIMON - IBIAS) * 1000 / RSENSE
 *   Battery I:    I = Vadc / BATT_I_DIV
 *   Motor I:      I = Vadc / (RSHUNT * GAIN)
 */
#define SENSOR_PANEL_V_RTOP     75000.0f    /* panel divider top    */
#define SENSOR_PANEL_V_RBOT     10000.0f    /* panel divider bottom */
#define SENSOR_BATT_V_RTOP      120000.0f   /* battery divider top    */
#define SENSOR_BATT_V_RBOT      10000.0f    /* battery divider bottom */
#define SENSOR_PANEL_I_RIMON    21000.0f    /* panel IMON resistor    */
#define SENSOR_PANEL_I_IBIAS    7.0e-6f     /* panel IMON bias current*/
#define SENSOR_PANEL_I_RSENSE   0.012f      /* panel sense resistor   */
#define SENSOR_BATT_I_DIV       0.405f      /* battery I divisor      */
#define SENSOR_MOTOR_I_RSHUNT   0.005f      /* motor shunt resistor   */
#define SENSOR_MOTOR_I_GAIN     20.0f       /* motor current-sense gain */

/* =====================================================================
 * POWER MANAGEMENT + RTC (Phase 5)
 * ---------------------------------------------------------------------
 * The RTC (clocked from the LF crystal / ACLK) produces a 1 Hz tick; the
 * ISR counts ticks and raises a "measurement due" flag every
 * MEASURE_INTERVAL_S seconds, which wakes the CPU from low-power sleep.
 * ===================================================================== */

#define MEASURE_INTERVAL_S   3    /* seconds between wake-ups.
                                    * Use a small value (e.g. 3) to test
                                    * the wake cycle without waiting a minute. */

/* =====================================================================
 * I2C + MCP4706 DAC (Phase 6)   -- eUSCI_B0, motor speed reference
 * ---------------------------------------------------------------------
 * The MCP4706 is an 8-bit I2C DAC. Its 0-3.3 V output is scaled to
 * 0-10 V by a hardware opamp (no code). Formula:
 *   Vout = MCP4706_VREF_VOLTS * value / 256   (value 0-255)
 *
 * MUX: UCB0SDA (P1.6) and UCB0SCL (P1.7) are the SECONDARY module function
 * (P1SEL1=1, P1SEL0=0), confirmed from the datasheet Port P1 table.
 * ===================================================================== */

#define I2C_SDA_PORT     GPIO_PORT_P1     /* P1.6 = UCB0SDA */
#define I2C_SDA_PIN      GPIO_PIN6
#define I2C_SCL_PORT     GPIO_PORT_P1     /* P1.7 = UCB0SCL */
#define I2C_SCL_PIN      GPIO_PIN7
#define I2C_PIN_MUX      GPIO_SECONDARY_MODULE_FUNCTION
#define I2C_DATARATE     EUSCI_B_I2C_SET_DATA_RATE_100KBPS

#define MCP4706_I2C_ADDR    0x60          /* A0 variant, 7-bit address    */
#define MCP4706_VREF_VOLTS  3.3f          /* VDD = VRL reference           */

/* =====================================================================
 * RS485 PROTOCOL (Phase 9)   -- modbus-like framing over the RS485 UART
 * ---------------------------------------------------------------------
 * Frame: [address][function][data...][CRC16_lo][CRC16_hi]
 * CRC16 uses the Modbus polynomial (0xA001, init 0xFFFF) and is appended
 * low byte first (Modbus convention). This device answers to one address.
 * ===================================================================== */

#define RS485_DEVICE_ADDRESS   0x01       /* this device's bus address     */
#define RS485_MAX_FRAME        64         /* max frame length, bytes       */

/* =====================================================================
 * HMI SCREEN (Phase 12)   -- eUSCI_A2, TY040HDL04NF "Giraffe" protocol
 * ---------------------------------------------------------------------
 * 3.3 V TTL UART, 8N1, 115200 baud -> wired straight to the module, no
 * level shifter. UCA2TXD/UCA2RXD are the PRIMARY module function on
 * P7.0/P7.1 (datasheet Table 9-35).
 *
 * Baud generator is a MATCHED SET for 115200 from an 8 MHz SMCLK with
 * oversampling; recalculate all three if the baud or SMCLK changes.
 *
 * Screen control lines: AUDIO-PA-EN is active-LOW for audio, so we hold it
 * HIGH to keep the audio amplifier off.
 * ===================================================================== */

#define HMI_TX_PORT     GPIO_PORT_P7      /* P7.0 = UCA2TXD */
#define HMI_TX_PIN      GPIO_PIN0
#define HMI_RX_PORT     GPIO_PORT_P7      /* P7.1 = UCA2RXD */
#define HMI_RX_PIN      GPIO_PIN1
#define HMI_PIN_MUX     GPIO_PRIMARY_MODULE_FUNCTION

#define HMI_BAUD             115200UL
#define HMI_BR_PRESCALAR     4            /* UCBRx  for 115200 @ 8 MHz   */
#define HMI_BR_FIRSTMOD      5            /* UCBRFx (oversampling)       */
#define HMI_BR_SECONDMOD     0x55         /* UCBRSx (fractional mod.)    */

/* Screen connector control lines (plain GPIO). */
#define HMI_PE9_PORT        GPIO_PORT_P7  /* P7.3 - generic GPIO         */
#define HMI_PE9_PIN         GPIO_PIN3
#define HMI_PE8_PORT        GPIO_PORT_P7  /* P7.2 - 485-DIR              */
#define HMI_PE8_PIN         GPIO_PIN2
#define HMI_AUDIO_EN_PORT   GPIO_PORT_P6  /* P6.7 - AUDIO-PA-EN          */
#define HMI_AUDIO_EN_PIN    GPIO_PIN7     /*        HIGH = audio OFF     */
#define HMI_SPK_PORT        GPIO_PORT_P6  /* P6.6 - SPK to audio amp     */
#define HMI_SPK_PIN         GPIO_PIN6

#define HMI_MAX_FRAME        64           /* max Giraffe frame, bytes    */
#define HMI_DEFAULT_BRIGHTNESS  60        /* 0-100, startup backlight    */

/* CRC16-CCITT on the HMI link. MUST match "CRC Enable" in the Giraffe IDE
 * project; the vendor examples run with it off, so we start disabled. */
#define HMI_CRC_ENABLED      0

/* Screen layout IDs — FILL IN after the GUI is built in the Giraffe IDE.
 * Every label/widget there gets a page ID and a control ID; put them here
 * and then enable the writes in hmi_update(). */
#define HMI_PAGE_MAIN        0            /* TODO: main page view ID     */
#define HMI_ID_FLOW          0            /* TODO: flow label            */
#define HMI_ID_BATT_V        0            /* TODO: battery voltage label */
#define HMI_ID_BATT_I        0            /* TODO: battery current label */
#define HMI_ID_PANEL_V       0            /* TODO: panel voltage label   */
#define HMI_ID_PANEL_I       0            /* TODO: panel current label   */
#define HMI_ID_VALVE_POS     0            /* TODO: valve position label  */

#endif /* CONFIG_H_ */
