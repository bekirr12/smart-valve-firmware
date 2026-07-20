# CLAUDE.md — Smart Valve Firmware Constitution

> This file is the single source of truth for the firmware architecture and rules.
> Any code change MUST conform to the decisions recorded here. If a change requires
> deviating from this document, update this document first, then implement.

---

## 1. Project Overview

Battery-powered **smart water valve** built around the **MSP430FR6047** ultrasonic
flow-metering MCU. The device:

- Measures water flow rate with ultrasonic transducers (TI USS module + USSLib).
- Reports telemetry to a central server over **RS485 → LoRaWAN gateway**.
- Receives commands from the center to **open/close a motorized valve**.
- Runs on a **24 V Li-ion battery** charged from a solar panel via an **LT8490 MPPT**
  charger (on a separate board, connected board-to-board).
- Is optimized for **ultra-low power** (mostly asleep, wakes to measure or on command).

**Leak detection is decided by the remote center, not on-device.** The firmware only
executes open/close commands and streams telemetry.

---

## 2. Hardware Summary

- **MCU:** MSP430FR6047, 100-pin, 256 KB FRAM.
- **Boards:** (1) MPPT board (LT8490, fully hardware-driven — no code needed to charge),
  (2) processor board (MSP430). Connected via board-to-board connector.
- **Ultrasonic:** 2 possible transducer pairs; **only 1 pair populated**. Of 8 possible
  opamps (THS3095), **only 4 soldered**. Opamps require **±15 V** generated from 24 V by
  an **LT8471**, gated by an enable pin (P3.1). ±15 V has a **~1–10 ms settling time**
  after enable — code must wait before measuring.
- **HMI screen:** own driver board, driven over UART. **Lowest priority — stubbed.**

### 2.1 Pin Map

#### ADC (all use internal 2.5 V VREF)
| Signal | Pin | Ch | Conversion Formula |
|---|---|---|---|
| Panel Voltage | P1.4 | A2 | `V = Vadc × (75k+10k)/10k` |
| Battery Voltage | P1.5 | A3 | `V = Vadc × (120k+10k)/10k` |
| Panel Current | P8.5 | A11 | `I = (Vadc/21k − 7µA) × 1000/0.012` |
| Battery Current | P8.6 | A12 | `I = Vadc / 0.405` |
| Motor Current | P2.3 | A15 | `I = Vadc / (0.005 × 20)` |

#### UART
| Function | TX | RX | Baud | Peripheral |
|---|---|---|---|---|
| RS485 (to LoRaWAN gateway) | P4.3 | P4.4 | TBD | eUSCI_A0 (hardware) |
| RS485 DIR/EN | P4.5 | — | — | GPIO (high=TX, low=RX) |
| HMI Screen | P7.0 | P7.1 | TBD | eUSCI_A2 (hardware) |
| LT8490 STATUS | — | P2.1 | 2400 | **bit-bang** (GPIO int + Timer_A) |

#### I2C — MCP4706 DAC (motor speed reference)
| SDA | SCL | Peripheral | Vref | Formula |
|---|---|---|---|---|
| P1.6 | P1.7 | eUSCI_B0 | 3.3 V | `Vout = Vref × (value/256)`, value 0–255 |
DAC 0–3.3 V output is scaled to **0–10 V** by a non-inverting opamp (hardware, no code).
- UCB0SDA/UCB0SCL on P1.6/P1.7 are the **SECONDARY** module function (P1SEL1=1,
  P1SEL0=0), confirmed from the datasheet Port P1 table.
- `mcp4706_init()` must write a config byte (`0x80` = VREF=VDD, normal power, gain 1x)
  at startup: the shipped/EEPROM config does **not** reference VDD, so without this the
  output stays 0 V regardless of the DAC value.
- Write Volatile DAC Register frame (MCP47X6 Fig 6-1): `[0x00][value]`.
- **driverlib gotcha:** `EUSCI_B_I2C_masterSendMultiByteFinish` leaves `UCTXIFG` set;
  `i2c_write()` clears `EUSCI_B_I2C_TRANSMIT_INTERRUPT0` after each transfer, otherwise
  only the first multi-byte transaction after init works (the next `...Start` skips its
  TXIFG wait and writes data before the address).

#### Motor Control (6 pins)
| Signal | Pin | Notes |
|---|---|---|
| BRAKE | P9.0 | GPIO out |
| ENABLE | P9.1 | GPIO out |
| FAULT (input) | P9.2 | GPIO in (from motor driver) |
| DIRECTION | P9.3 | GPIO out |
| ENCODER | P7.6 / TA4.1 | timer capture / pulse count |
| SPEED | via MCP4706 I2C | 0–100 % → DAC → 0–10 V |

**Confirmed signal polarities:**
- LEDs: active-high (HIGH = on). Buttons: active-low (pull-up, pressed = LOW).
- Motor ENABLE: active-low (LOW = enabled, HIGH = disabled/safe).
- Motor BRAKE: active-low (LOW = brake engaged/safe, HIGH = released).
- ±15 V EN (P3.1, LT8471): active-low (LOW = supply ON, HIGH = OFF/safe).
- RS485 EN (P4.5): HIGH = TX (drive bus), LOW = RX (listen).

#### USS (only Pair 1 active)
| Signal | Pin | Role |
|---|---|---|
| PD1 | P6.4 | Pair1 Upstream (TX0+RX1) |
| PD2 | P5.7 | Pair1 Downstream (TX1+RX0) |
| PD3 | P6.5 | Pair2 Upstream — **not populated** |
| PD4 | P6.0 | Pair2 Downstream — **not populated** |
| S1 | P6.3 | analog switch (TS5A3357) |
| S2 | P6.2 | analog switch |
| S3 | P5.6 | analog switch |
| S4 | P5.5 | analog switch |
| ±15 V EN | P3.1 | LT8471 enable (opamp supply) |

Only PD1/PD2 and S1/S3 are used (Pair 1). PD/S truth tables from TIDA-01486.

#### LT8490 MPPT Status
| Signal | Pin | Type |
|---|---|---|
| STATUS | P2.1 | bit-bang UART RX, 2400 baud, 8N1, 2 bytes every ~3.5 s |
| FAULT | P3.2 | GPIO interrupt, active-high |

#### HMI extra GPIO
| Signal | Pin | | Signal | Pin |
|---|---|---|---|---|
| PE9 | P7.3 | | PE15 | P6.7 |
| PE8 | P7.2 | | PE11 | P6.6 |

#### LEDs & Buttons
| LED | Pin | | Button | Pin |
|---|---|---|---|---|
| LED1 | P4.2 | | BUT_DOWN | P5.0 |
| LED2 | P4.1 | | BUT_SELECT | P5.1 |
| LED3 | P4.0 | | BUT_LEFT | P5.2 |
| | | | BUT_UP | P5.3 |
| | | | BUT_RIGHT | P5.4 |

#### Crystals
| Crystal | Frequency | Usage |
|---|---|---|
| HF | 8 MHz | MCLK source |
| LF | 32.768 kHz | ACLK / RTC / LPM wakeup |
| USS | 8 MHz | USS module PLL |

---

## 3. Firmware Architecture

### 3.1 Implementation Approach — HYBRID
- **~95 % driverlib** (the `bsp/` layer): all init/config, clock, ADC, UART, I2C, RTC,
  PMM, FRAM, motor GPIO, RS485. Chosen for readability, development speed, and safety.
- **Raw registers** only in **cycle-critical ISRs**: the bit-bang UART (416 µs sampling)
  and the encoder counting ISR. These carry teaching comments explaining each register.
- **USSLib** (separate TI library) handles ultrasonic flow measurement.
- `bsp/` = Board Support Package (driverlib wrappers), **not** a register abstraction.

### 3.2 Directory / File Structure
```
main.c              → INIT + main state-machine loop
config.h            → all #defines: pins, intervals, thresholds

bsp/                → Board Support Package (driverlib wrappers)
  clock.c/.h        → HF/LF/USS crystals, MCLK/SMCLK/ACLK
  gpio_init.c/.h    → all pin directions/mux, LCD module disable
  adc.c/.h          → ADC12_B init + channel reads
  uart.c/.h         → eUSCI_A0 (RS485), eUSCI_A2 (HMI)
  i2c.c/.h          → eUSCI_B0 (DAC)
  timer.c/.h        → Timer_A (bit-bang timebase + encoder), RTC
  power.c/.h        → LPM entry/exit, ±15 V enable

drivers/            → peripheral drivers (meaningful functions)
  uss_flow.c/.h     → USSLib wrapper, ΔToF → flow calculation
  motor.c/.h        → motor sub-state machine, stall, encoder, homing
  mcp4706.c/.h      → DAC speed control over I2C
  lt8490.c/.h       → bit-bang STATUS decode + FAULT handling
  sensors.c/.h      → ADC raw → real units (V, A) via formulas above
  rs485.c/.h        → modbus-like frame build/parse + CRC16
  hmi.c/.h          → screen UART stub

app/                → application logic
  state_machine.c/.h → top-level state transitions
  comm_protocol.c/.h → register map, command handling
  valve_control.c/.h → high-level valve logic (position, calibration)
```
**Layering rule:** each layer calls only the layer directly below it
(`app → drivers → bsp`). No layer-skipping, no upward calls.

---

## 4. Power Management

- Target sleep: code requests **LPM3**. Because eUSCI_A0 (RS485) requests SMCLK via the
  clock-request mechanism, the **actual achieved mode is LPM1** (SMCLK stays on so UART
  can listen). This is correct and expected.
- Write `LPM3_bits` (not LPM0) so intent = "deepest possible sleep"; if UART is later
  disabled, the device genuinely reaches LPM3.
- **LPM3.5 / LPM4.5 are NOT used** — they wipe RAM/registers, blind the UART, and add
  re-init overhead not worth the savings.
- eUSCI_A **receiver start-edge detection** wakes the CPU from LPMx on incoming RS485.
- **Wake sources:** RTC interval interrupt (ACLK), eUSCI_A0 RX interrupt (RS485),
  P3.2 FAULT GPIO interrupt.
- The RTC runs independently and fires every `MEASURE_INTERVAL_S` regardless of the
  previous cycle's duration (no self-reset). Its ISR only sets a `measurement_due` flag;
  the main loop acts on it.

### Approximate current budget
| State | Actual mode | Current |
|---|---|---|
| Sleep (RS485 listening) | LPM1 | ~50–100 µA |
| Measure + transmit | Active | ~5–10 mA (brief) |
| Motor running | Active | ~1–2 A |

---

## 5. State Machine

```
INIT → IDLE ⇄ { MEASURE → TRANSMIT , CMD_PROCESS → MOTOR_CTRL }

Background ISRs (any state):
  P2.1 falling edge → bit-bang decode → lt8490_status
  P3.2 rising edge  → FAULT_HANDLE
```

- **INIT** — clocks (HF/LF/USS), GPIO, ADC, UART, I2C, timers; disable LCD module;
  unlock LPM5; read valve position from FRAM (see §6 homing); start RTC → IDLE.
- **IDLE** — enter LPM3 (→LPM1). Wake on RTC / RS485 / FAULT.
- **MEASURE** — (1) enable ±15 V (P3.1 high), (2) **wait ~10 ms** for supply settle,
  (3) USS measure Pair 1 upstream + downstream → ΔToF → flow, (4) read ADC (panel V/I,
  battery V/I, motor I), (5) disable ±15 V → TRANSMIT.
- **TRANSMIT** — RS485 EN high, send telemetry packet, EN low. If a command is pending
  → CMD_PROCESS, else → IDLE.
- **CMD_PROCESS** — parse packet; VALVE_OPEN / VALVE_CLOSE → MOTOR_CTRL; others → IDLE.
- **Conflict rule** — if a command arrives mid-MEASURE, **finish the measurement first**
  (never interrupt a USS acquisition), then handle the command.

### 5.1 Telemetry Packet (device → center)
Flow rate, battery voltage, battery current, panel voltage, panel current, motor current,
motor speed (% of DAC full scale), valve position (open/closed/moving), LT8490 charge
stage (0–3 / done / fault).

### 5.2 Downlink Commands (center → device)
VALVE_OPEN, VALVE_CLOSE. (Others TBD.)

---

## 6. Motor Control & Valve Position

### 6.1 MOTOR_CTRL sub-state machine
```
BRAKE_FIRST(50 ms) → SET_DIRECTION → ACCELERATE(DAC ramp) → RUNNING
                                                              ↓
                                     BRAKE_STOP → VERIFY → DONE
```
- **RUNNING** exit conditions (polled every loop):
  1. `motor_current > STALL_THRESHOLD` → reached mechanical end-stop ✓
  2. `encoder_count` reached target (intermediate position) ✓
  3. new RS485 command → BRAKE_STOP, then re-enter with the new direction
  4. timeout → fault safety-stop ✗
- **Direction change:** always BRAKE_STOP before reversing (protects the motor driver).
  Never switch DIR while the motor is energized.
- **VERIFY:** if stall fired but `encoder_count` disagrees with expectation → report a
  position-inconsistency fault to the center.

### 6.2 Position Detection (no limit switch)
Two complementary layers:
1. **Stall detection (primary end-stop):** motor current (A15) spikes when the valve
   reaches its mechanical limit → fully open / fully closed.
2. **Encoder (P7.6/TA4.1):** pulse counting for intermediate positions and as a
   cross-check against stall (mismatch = jam/fault).

### 6.3 Homing (HYBRID + self-calibration)
- On boot, read the last position from **FRAM**:
  - Valid record → accept it; **do not move the valve** (avoids cutting water on every
    power blip).
  - Invalid / first run → run homing: drive CLOSE until stall, set `encoder_count = 0`.
- Every CLOSE command naturally hits the stall, so the system **auto-recalibrates
  `encoder_count = 0` on each full close** — no dedicated periodic homing needed.
- FRAM persistence + stall cross-check keeps position correct without needless motion.

---

## 7. Communication Protocol (RS485, MODBUS-LIKE)

- Frame: `[address][function][data...][CRC16]`, using the Modbus CRC16 polynomial.
- Register-style model:
  - **Read registers:** flow, voltages, currents, valve position, motor speed,
    LT8490 status.
  - **Write registers/commands:** valve open/close.
- Custom function codes are allowed as needed; **not required to be fully
  Modbus-compliant.**

---

## 8. LT8490 STATUS — Bit-Bang UART

- LT8490 STATUS pin (P2.1) transmits 2 UART bytes (8N1, 2400 baud) every ~3.5 s.
- P2.1 maps to eUSCI_A0, which is reserved for RS485 — so STATUS is read by **bit-bang**:
  1. P2.1 configured as GPIO input with falling-edge interrupt (start bit).
  2. On the edge, start Timer_A; sample P2.1 every **416 µs** (1/2400).
  3. Collect 8 bits, validate the stop bit, store the byte; repeat for byte 2.
  4. Store the decoded status in a global consumed by TRANSMIT.
- FAULT pin (P3.2) is a plain active-high GPIO interrupt → FAULT_HANDLE.
- STATUS/FAULT LED-pulse meanings follow LT8490 datasheet Table 6.

---

## 9. Development Roadmap (learning-oriented, hardware bring-up order)

Implemented **one phase at a time**, each verified on hardware before the next.
Not all at once.

| Phase | Module(s) | On-device test |
|---|---|---|
| 0 ✅ | skeleton, `config.h`, folders | LED on P1.0 blinks |
| 1 ✅ | `bsp/clock` | measure clock / blink rate |
| 2 ✅ | `bsp/gpio_init` | LED1-3, read 5 buttons |
| 3 ✅ | `bsp/uart` (RS485 TX) | RS485 sends "hello" |
| 4 ✅ | `bsp/adc` + `drivers/sensors` | print V/I readings over UART |
| 5 ✅ | `bsp/timer` (RTC) + `bsp/power` | low current, 60 s periodic wake |
| 6 ✅ | `bsp/i2c` + `drivers/mcp4706` | measure DAC output voltage |
| 7 | `drivers/motor` | valve open/close, position tracking |
| 8 | `drivers/lt8490` | read charger status bytes |
| 9 ✅(SW) | `drivers/rs485` + `app/comm_protocol` | full command/response exchange |
| 10 | `app/state_machine` | full 60 s measure/report loop |
| 11 | `drivers/uss_flow` + USSLib | real flow reading |
| 12 | `drivers/hmi` | (deferred) |

### 9.1 Pending Hardware Verification (parts not yet available)
Some phases are written and software-tested but await hardware to verify on-device.
Come back and run these checks when the parts arrive:

| Needed hardware | Blocks / to verify |
|---|---|
| 24 V supply + motor | Phase 7 real motion, stall detection, encoder counting under load |
| LT8490 chip | Phase 8 bit-bang STATUS decode + FAULT |
| LT8471 chip (±15 V) | USS opamp supply enable (P3.1) + Phase 11 measurement |
| RS485↔USB converter | Phase 3 TX and Phase 9 physical frame exchange |
| USSLib + transducers | Phase 11 ultrasonic flow measurement |
| HMI cable + screen data | Phase 12 screen |

Software-only work (CRC, frame build/parse, register map, state-machine logic) is
fully developed and debugger-tested without these; only the physical I/O is deferred.
Bench tricks used meanwhile: multimeter for GPIO/DAC levels, jumper pulses on the
encoder pin to test the counting ISR.

Raw-register code appears only in Phase 7 (encoder ISR) and Phase 8 (bit-bang).

---

## 10. Git Commit Convention (Conventional Commits)

```
<type>: <short summary>

<body: what changed and why>
```

| Type | Use for |
|---|---|
| `feat` | new feature (new driver, new state) |
| `fix` | bug fix |
| `refactor` | code restructuring, no behavior change |
| `docs` | documentation (CLAUDE.md, README) |
| `chore` | build/config (gitignore, project settings) |
| `test` | adding tests |
| `perf` | performance / power optimization |

Example: `feat: add bit-bang UART driver for LT8490 status`

---

## 11. Working Rules for Claude

1. **This document governs.** Conform to it; if a change needs to deviate, update this
   file first, then code.
2. **Incremental only.** Implement one roadmap phase at a time. Do not jump ahead or
   write multiple phases at once unless explicitly asked.
3. **Explain while building.** The developer is learning; annotate non-obvious code,
   especially raw-register sections, and be ready to pause and discuss.
4. **Hybrid discipline.** Use driverlib by default; drop to raw registers only in the
   cycle-critical ISRs named in §3.1 / §9.
5. **Respect the layering** (`app → drivers → bsp`), the pin map, and the power model.
6. Keep `config.h` the single home for tunable constants (pins, intervals, thresholds).
