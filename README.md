# Smart Valve Firmware

Firmware for a solar-powered, battery-backed **smart water valve** based on the
**Texas Instruments MSP430FR6047** ultrasonic flow-metering microcontroller.

The device measures water flow with ultrasonic transducers, streams telemetry to a
central server over RS485 → LoRaWAN, and drives a motorized valve open/closed on command
— all while spending most of its life in a low-power sleep mode to preserve battery.

---

## Features

- 🌊 **Ultrasonic flow measurement** using the MSP430 USS module + TI USSLib.
- 🔋 **Solar + battery powered** — 24 V Li-ion pack charged by an LT8490 MPPT charger.
- 📡 **Remote telemetry & control** over an RS485 link to a LoRaWAN gateway.
- ⚙️ **Motorized valve control** with encoder feedback and current-based stall detection
  (no limit switches required).
- 💤 **Ultra-low-power** design — sleeps in LPM1/LPM3, wakes on a timer, an incoming
  command, or a charger fault.
- 🖥️ **HMI screen** support over UART (planned).

---

## Hardware

- **MCU:** MSP430FR6047 (100-pin, 256 KB FRAM)
- **Charger:** LT8490 MPPT (hardware-driven, on a separate board)
- **±15 V supply:** LT8471 (powers the ultrasonic signal-chain opamps)
- **DAC:** MCP4706 (I2C) for the motor 0–10 V speed reference
- **Comms:** RS485 transceiver → LoRaWAN gateway
- **Sensing:** panel & battery voltage/current, motor current (ADC, internal 2.5 V ref)

Full pin assignments and conversion formulas live in [CLAUDE.md](CLAUDE.md).

---

## Architecture

Layered design:

```
app/       high-level logic (state machine, protocol, valve control)
drivers/   peripheral drivers (USS, motor, DAC, LT8490, sensors, RS485, HMI)
bsp/       board support (clock, GPIO, ADC, UART, I2C, timers, power)
```

Rule: each layer calls only the layer below it (`app → drivers → bsp`).

**Implementation approach:** hybrid — mostly TI **driverlib** for readability and safety,
with **raw-register** code only in the cycle-critical interrupt handlers (bit-bang UART,
encoder). Ultrasonic flow math is handled by TI's separate **USSLib**.

---

## Build & Flash

- **Toolchain:** TI Code Composer Studio (CCS) with the MSP430 compiler.
- **Target:** MSP430FR6047 (`targetConfigs/MSP430FR6047.ccxml`)
- **Driver library:** bundled under `driverlib/MSP430FR5xx_6xx/`

1. Import the project into CCS (*Project → Import CCS Projects*).
2. Build (*Project → Build Project*).
3. Flash/debug with an MSP-FET or on-board debugger (*Run → Debug*).

---

## Development Roadmap

The firmware is built up incrementally, one hardware-testable milestone at a time:

| Phase | Focus | Status |
|---|---|---|
| 0 | Project skeleton + LED blink | ✅ Done |
| 1 | Clock system (HF/LF crystals) | ✅ Done |
| 2 | GPIO init, LEDs, buttons | ✅ Done |
| 3 | RS485 UART (debug output) | ✅ Done |
| 4 | ADC + sensor readings | ✅ Done |
| 5 | Low-power modes + RTC wake | ✅ Done |
| 6 | I2C + MCP4706 DAC | ✅ Done |
| 7 | Motor control + position | ⬜ Planned |
| 8 | LT8490 status (bit-bang) | ⬜ Planned |
| 9 | RS485 protocol (Modbus-like) | ⬜ Planned |
| 10 | State machine integration | ⬜ Planned |
| 11 | Ultrasonic flow (USSLib) | ⬜ Planned |
| 12 | HMI screen | ⬜ Deferred |

---

## Repository Layout

```
main.c                       application entry point
config.h                     tunable constants (pins, intervals, thresholds)
CLAUDE.md                    full technical specification ("firmware constitution")
driverlib/                   TI MSP430 driver library
targetConfigs/               CCS target configuration
lnk_msp430fr6047.cmd         linker command file
```

---

## License

Proprietary — internal project. All rights reserved.
