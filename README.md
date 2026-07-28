## DCO4 – Mainboard Controller (STM32 modulation brain)

Firmware for the **DCO4 mainboard**: an STM32 Arduino sketch that owns per-voice **ADSRs**, **LFOs**, **filter/VCA/resonance CVs** (hardware timer PWM), **square/sub levels** (MCP4728 I2C DACs), **analog wave select** (74HC595), and **parameter routing** between the DCO voice board, input controller, and screen.

Four voices (`NUM_VOICES 4`). Single-threaded `setup()` / `loop()` (not dual-core).

How this board fits the instrument: [`docs/SYSTEM_OVERVIEW.md`](docs/SYSTEM_OVERVIEW.md) (stub) → canonical overview in sibling **DCO4_DCO**.

All detailed documentation lives under **[`docs/`](docs/)**. This README is the entry point.

---

## Documentation

| Doc | Status | Contents |
|-----|--------|----------|
| [`docs/SYSTEM_OVERVIEW.md`](docs/SYSTEM_OVERVIEW.md) | Current | Stub → canonical four-board overview + local UART table |
| [`docs/MODULATION_PIPELINE.md`](docs/MODULATION_PIPELINE.md) | Current | Notes / params → ADSR/LFO → PWM/DAC → DCO serial |
| [`docs/CV_AND_PINS.md`](docs/CV_AND_PINS.md) | Current | Timer PWM, MCP4728, 74HC595, UART pins |
| [`docs/FILE_INDEX.md`](docs/FILE_INDEX.md) | Current | Every file + functions + call sites |
| [`docs/REFERENCE_AI.md`](docs/REFERENCE_AI.md) | Current | Deep semantic map for developers / AI |
| [`docs/README_serial_and_params.md`](docs/README_serial_and_params.md) | Current | Shared serial / ParamId how-to |

**Suggested reading order:** this README → system stub → modulation pipeline / CV pins → FILE_INDEX or REFERENCE_AI → serial how-to as needed.

---

## Features

- **Modulation:** 3 Bézier ADSRs per voice (VCA, VCF, ADSR3 for DCO); LFO1 / LFO2; per-voice VCF drift LFOs.
- **CV outs:** Resonance, cutoff, VCA via STM32 timer PWM; SQR1/SQR2/Sub via three MCP4728 DACs.
- **Wave select:** Dual 74HC595 mux (saw / saw2 / tri / sine enables).
- **Serial:** 2.5 Mbaud links to DCO (Serial2), Input (Serial8), Screen (Serial1); USB Serial @ 2 Mbaud for debug.
- **Params:** Table-driven `ParamId` router; many IDs applied locally and forwarded to the DCO.
- **Manual calibration:** Special PWM/mux path when `manualCalibrationFlag` is set.

**Not active in this firmware today:** local SD/EEPROM preset store (`flashData.ino` commented — presets owned by Input), SPI BU2505FV (`ENABLE_SPI` off), on-board Screen module, autotune include.

---

## High-level architecture

| Subsystem | Files | Role |
|-----------|-------|------|
| Entry / loop | `DCO4_Mainboard_Controller.ino` | Init + soft-timer schedule |
| Serial RX | `Serial.ino`, `serial_*.h` | DCO notes/params; Input blocks/params; Screen RX stub |
| Serial TX | `Serial2.ino` | ADSR3/`PW`/param forwards; Screen/Input helpers |
| Params | `params.ino`, `params_def.h`, `param_router.h` | Apply + forward |
| ADSR / LFO | `ADSR.*`, `LFO.*` | Envelope and LFO levels |
| Formulas | `formulas.*` | Depth/speed scalars |
| CV write | `PWM.ino`, `Timers.*`, `MCP4728.ino` | Timer PWM + I2C DACs |
| Waves | `waveSelector.*` | 74HC595 |

Hot path every `loop`: Serial2 → LFO1/2 → ADSR_update → setPWMOuts (or manual-cal). Details: [`docs/MODULATION_PIPELINE.md`](docs/MODULATION_PIPELINE.md).

---

## Hardware / UART summary

| Port | Pins | Baud | Peer |
|------|------|------|------|
| Serial | USB | 2 000 000 | Debug |
| Serial1 | PA10 / PA9 | 2 500 000 | Screen |
| Serial2 | PD6 / PD5 | 2 500 000 | DCO |
| Serial8 | PE0 / PE1 | 2 500 000 | Input |

I2C MCP4728: SDA **PB9**, SCL **PB8**, 1 MHz. Full CV pin table: [`docs/CV_AND_PINS.md`](docs/CV_AND_PINS.md).

---

## Building

- **Toolchain:** Arduino IDE / CLI with STM32 Arduino core.
- **Sketch:** open `DCO4_Mainboard_Controller.ino`.
- **Libraries:** `ADSR_Bezier`, `mo-lfo`, `MCP4728_multiaddress`, `RoxMux`, `Wire`; optional `RunningAverage` if benchmarking; `STM32SD` only if re-enabling SD presets.

### Feature flags

| Flag | Default | Effect |
|------|---------|--------|
| `ENABLE_SD` | **on** (main sketch) | SDMMC pin reservation / TIM8 CH4 gating; preset code still commented |
| `ENABLE_SPI` | **off** | SPI / BU2505 path |
| `ENABLE_SERIAL*` | **on** (`Serial.h`) | Per-UART compile-in |
| `ENABLE_SCREEN` | **off** | Would call `initScreen()` |
| `RUNNING_AVERAGE` | **off** | Loop micro-benchmarks |
| `NUM_VOICES` | **4** | Voice array sizes |
| `build_opt.h` | always | Larger Serial RX/TX buffers |

---

## Contributing / hacking

- Start with [`docs/REFERENCE_AI.md`](docs/REFERENCE_AI.md) and [`docs/FILE_INDEX.md`](docs/FILE_INDEX.md).
- Keep `ParamId` numbers stable across boards (`params_def.h`).
- Prefer routing new controls through `params.ino` + serial protocol headers rather than ad-hoc UART bytes.
- When changing CV math, keep [`docs/CV_AND_PINS.md`](docs/CV_AND_PINS.md) and [`docs/MODULATION_PIPELINE.md`](docs/MODULATION_PIPELINE.md) in sync.
