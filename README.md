# Smart Valve Project: Hardware Architecture & Firmware Development Guide

## 1. Project Overview
The Smart Valve Project aims to optimize field and irrigation systems by retrofitting valves with smart measurement and control capabilities. The system features a dual-board architecture:
*   **MPPT Board**: Manages energy harvesting from a solar panel to charge a battery, and regulates power to a valve-actuating motor using a full-bridge driver.
*   **MCU Board**: Features the Texas Instruments MSP430FR6047 microcontroller to calculate water flow rates using ultrasonic sensors, communicate via an HMI screen and a LoRaWAN RS485 transceiver, and execute system logic at ultra-low power. 

## 2. MCU Board Architecture & Firmware Mapping
The MCU board is the brain of the smart valve. The firmware must manage flow calculations, communications, and system timing.

### 2.1. Core Processor & Low-Power Operations
*   **Hardware**: The board uses the **MSP430FR6047**, a 16-bit Mixed-Signal Microcontroller featuring an integrated Ultrasonic Sensing Solution (USS) module and Low-Energy Accelerator (LEA). It is populated with two external crystals: a low-frequency 32.768 kHz (Y2) and a high-frequency 8 MHz (Y1/Y3). 
*   **Firmware Implementation**: 
    *   The 32.768 kHz crystal should be used to drive the RTC (Real-Time Clock) while the device sits in the Standby Mode (LPM3.5), allowing the system to consume roughly 450 nA during sleep intervals. 
    *   The 8 MHz crystal will be utilized by the High-Speed PLL (HSPLL) to generate the transmit frequency required for the ultrasonic transducers. 
    *   Use the **TI DriverLib** to quickly configure the Unified Clock System (UCS) and Power Management Module (PMM).

### 2.2. Ultrasonic Analog Front End (Booster Stage)
*   **Hardware**: Because pipe valves can attenuate ultrasonic signals, the board incorporates a custom analog front-end (AFE) "booster." High-voltage driving is achieved using **THS3095D** operational amplifiers powered by a ±15V DC-DC supply (TPS65131RGER). Path selection for the transmit/receive sequence is handled by **TS5A3357DCUR** SP3T analog switches.
*   **Firmware Implementation**:
    *   You will leverage the **Ultrasonic Software Library (USSLib)** to abstract the complexity of configuring the internal UUPS, PPG, PGA, and SDHS submodules. 
    *   Before invoking measurement APIs like `USS_startUltrasonicMeasurement`, the firmware must toggle the appropriate GPIO pins connected to the MUX/DEMUX ICs to align the TX and RX channels. The switch truth table requires precise sequential control of S1, S2, S3, S4, PD1, and PD2 signals to select Pair 1 or Pair 2 properly.
    *   The raw ADC capture waveforms can be accessed via `USS_getUPSPtr()` and `USS_getDNSPtr()` if your custom booster requires adjustments to the standard Time-of-Flight (TOF) calculations. 

### 2.3. Communications (LoRaWAN & HMI)
*   **Hardware**: An RS485 transceiver connects to a LoRaWAN module for long-range reporting, and an HMI screen provides a local interface. Both are controlled via UART.
*   **Firmware Implementation**:
    *   The MSP430FR6047 features enhanced Universal Serial Communication Interfaces (`eUSCI_A`) supporting UART. 
    *   You will implement the UART protocol via DriverLib APIs to push flow data strings periodically to the LoRaWAN node and receive real-time actuation commands.

## 3. MPPT Board Architecture & Firmware Mapping
The MPPT board ensures the system is self-sustaining and handles the physical opening and closing of the valve.

### 3.1. Solar MPPT and Battery Management
*   **Hardware**: Solar charging is facilitated via the **SM72295** Photovoltaic Full Bridge Driver. The board reads voltages and currents using ADC channels.
*   **Firmware Implementation**: 
    *   Configure the **ADC12_B** module to read the voltage and current feedback lines. 
    *   Implement an MPPT algorithm (such as the Perturb & Observe algorithm). The MCU will constantly read the power parameters and adjust the duty cycle of the PWM signals sent to the SM72295 driver.

### 3.2. Motor Control (Valve Actuation)
*   **Hardware**: The motor load is also connected through the MPPT board structure and enabled by control signals.
*   **Firmware Implementation**:
    *   Configure the **Timer_A / Timer_B** modules in Pulse Width Modulation (PWM) mode to safely control the motor speed and direction when a remote actuation command is received from the LoRaWAN interface.
    *   Ensure hardware protection bounds (overvoltage/overcurrent limit rules) are enforced in firmware using the SM72295’s built-in OVP (Over Voltage Protection) and `PGOOD` indicator interrupts.

## 4. Firmware Development Roadmap
To proceed to the coding phase systematically, follow these structured steps:

1.  **Phase 1: Board Bring-Up & Clocking** 
    *   Initialize the MSP430FR6047 using DriverLib. 
    *   Verify the 32.768kHz and 8MHz crystals are locked and stable. Ensure the Watchdog Timer is configured correctly.
2.  **Phase 2: Ultrasonic Subsystem & Booster Logic**
    *   Initialize the `USS_userConfig` structures (Frequency, Gap between pulses, Transducer parameters). 
    *   Write the GPIO control logic for the TS5A3357 switches and THS3095 op-amps. Test that the upstream and downstream signals are successfully penetrating the valve pipe using the Ultrasonic Sensing Design Center GUI.
3.  **Phase 3: Motor & MPPT Control Integration**
    *   Configure the 12-bit ADC and verify solar panel and battery telemetry.
    *   Map the Timer PWM outputs to the SM72295 (`HIA`, `HIB`, `LIA`, `LIB`) inputs. Build the MPPT tracking algorithm. 
4.  **Phase 4: Communication Layer**
    *   Initialize the `eUSCI_A` UART peripherals. 
    *   Set up RX interrupts to listen for incoming valve control commands from the LoRaWAN module, and TX routines to report the calculated flow rates.
5.  **Phase 5: Low-Power Optimization**
    *   Once functionality is verified, transition the system into an interrupt-driven state machine. 
    *   Sleep in `LPM3.5` with the RTC running, waking up periodically to measure flow, manage the MPPT duty cycle, and execute network transactions.