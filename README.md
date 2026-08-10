## DCO4 – Mainboard Controller (STM32 analog / modulation hub)

Firmware for the **classic DCO4 mainboard**: STM32 Arduino sketch that owns **Q15 envelopes + LFOs + mod matrix**, **filter/VCA/resonance CVs** (hardware timer PWM), **square/sub levels** (MCP4728), **analog wave select** (74HC595), and **slim LE serial** between DCO, Input, and Screen.

Four voices (`NUM_VOICES 4`), one VCF per voice. Single-threaded `setup()` / `loop()`.

Contract: [`docs/MAINBOARD_REINTEGRATION.md`](docs/MAINBOARD_REINTEGRATION.md). System: [`docs/SYSTEM_OVERVIEW.md`](docs/SYSTEM_OVERVIEW.md).

All detailed documentation lives under **[`docs/`](docs/)**. This README is the entry point.

---

## Documentation

| Doc | Contents |
|-----|----------|
| [`docs/MAINBOARD_REINTEGRATION.md`](docs/MAINBOARD_REINTEGRATION.md) | UART topology, ownership, `'n'`/`'o'`/`'m'` layouts |
| [`docs/SYSTEM_OVERVIEW.md`](docs/SYSTEM_OVERVIEW.md) | Local UART table + pointer to DCO overview |
| [`docs/MODULATION_PIPELINE.md`](docs/MODULATION_PIPELINE.md) | Notes / params → Q15 ADSR/LFO → PWM/DAC → `'m'` |
| [`docs/CV_AND_PINS.md`](docs/CV_AND_PINS.md) | Timer PWM, MCP4728, 74HC595, UART pins |
| [`docs/FILE_INDEX.md`](docs/FILE_INDEX.md) | Files + functions |
| [`docs/REFERENCE_AI.md`](docs/REFERENCE_AI.md) | Semantic map |
| [`docs/README_serial_and_params.md`](docs/README_serial_and_params.md) | Shared serial / ParamId how-to |

**Suggested reading:** this README → reintegration → modulation / CV pins → FILE_INDEX or REFERENCE_AI.

---

## Features

- **Modulation:** EnvVCA / EnvVCF / EnvDCO ×4 (Q15 Bézier); LFO1 / LFO2 (`getWaveQ15`); VCF drift LFOs; 8-slot mod matrix.
- **CV outs:** Resonance, cutoff, VCA via STM32 timer PWM (`update_CV_outs`); SQR1/SQR2/Sub via three MCP4728 DACs. ADSR depths `/512`, LFO `/1024` + negative LFO polarity; AS2164 Bézier + reso→VCA lerp.
- **Wave select:** Dual 74HC595 (OSC1 saw/tri, OSC2 saw/pulse on sinePins cabling).
- **Serial:** 2.5 Mbaud slim LE — DCO Serial2 (`'n'`/`'o'`/`'e'`/`'m'`/`'p'`/`'x'`/`'t'`), Input Serial8 (`'a'`–`'d'`/`'p'`/`'q'`), Screen Serial1 optional; USB debug @ 2 Mbaud.
- **Params:** 256-entry jump table, `int16_t` apply; DCO-owned IDs forwarded.
- **Manual calibration:** `update_CV_outs_manual_calibration` mutes mux / parks VCA.

**Not active:** local SD/EEPROM preset store (`flashData.ino` commented — presets on Input), SPI BU2505FV, on-board Screen module, autotune include.

---

## High-level architecture

| Subsystem | Files | Role |
|-----------|-------|------|
| Entry / loop | `DCO4_Mainboard_Controller.ino` | Init + soft-timer schedule |
| Serial RX | `Serial.ino`, `serial_*.h` | DCO notes/expression/params; Input blocks/params |
| Serial TX | `Serial2.ino` | `'m'` @ 1 ms; `'t'` bench ASCII; `'p'`/`'x'` forwards |
| Params | `params.ino`, `params_def.h`, `param_router.h` | Jump table apply + DCO forward |
| ADSR / LFO / matrix | `ADSR.*`, `LFO.*`, `mod_matrix.*` | Q15 sources |
| CV write | `cv_out.*`, `PWM.ino`, `Timers.*`, `MCP4728.ino` | Bake + timer PWM + I2C DACs |
| Waves | `waveSelector.*` | 74HC595 |

Hot path every `loop`: Serial2 → LFO1/2 → ADSR_update → `update_CV_outs` (or manual-cal). `'m'` on 1 ms. Details: [`docs/MODULATION_PIPELINE.md`](docs/MODULATION_PIPELINE.md).

---

## Hardware / UART summary

| Port | Pins | Baud | Peer |
|------|------|------|------|
| Serial | USB | 2 000 000 | Debug |
| Serial1 | PA10 / PA9 | 2 500 000 | Screen (optional) |
| Serial2 | PD6 / PD5 | 2 500 000 | DCO |
| Serial8 | PE0 / PE1 | 2 500 000 | Input |

I2C MCP4728: SDA **PB9**, SCL **PB8**, 1 MHz. Full CV pin table: [`docs/CV_AND_PINS.md`](docs/CV_AND_PINS.md).

---

## Building

```bash
arduino-cli compile --libraries ./_build_libs .
```

Main sketch: `MAINBOARD-CONTROLLER.ino` (or `DCO4_Mainboard_Controller.ino`).

### Libraries (`_build_libs`)

| Library | Path | Notes |
|---------|------|--------|
| `ADSR_Bezier` | `_build_libs/ADSR_Bezier` → `../../ADSR_Bezier` | Symlink to monorepo root; track **`Q15`**. |
| `mo-lfo` | `_build_libs/mo-lfo` → `../../mo-lfo` | Symlink to monorepo root; track **`q15`**. `LFO.ino` `#include`s `mo-lfo.cpp` so Arduino IDE links it. |

Sketchbook / core libs (not under `_build_libs`): `MCP4728_multiaddress`, `RoxMux`, `Wire`.

### Feature flags

| Flag | Default | Effect |
|------|---------|--------|
| `ENABLE_SD` | **on** (main sketch) | SDMMC pin reservation; preset code still commented |
| `ENABLE_SPI` | **off** | SPI / BU2505 path |
| `ENABLE_SERIAL*` | **on** (`Serial.h`) | Per-UART compile-in |
| `ENABLE_SCREEN` | **off** | Would call `initScreen()` |
| `RUNNING_AVERAGE` | **on** (main sketch) | Slim `bench.h` profiler; dco_control opcodes 40/41/42 |
| `MB_UART_PROBE` | **off** (main sketch) | 1 Hz `'t'` labels on Serial/1/2/8; dco_control Board shows `mb s2` if DCO is on USART2 PD5. Desyncs Input/Screen if plugged in. |
| `MB_UART_RX_LOG` | **on** (main sketch) | USB CDC one line per UART cmd byte on Serial2/8 (`mb s2 rx 'n'`, unknown `'m'` included); `'p'` also prints id/value. Serial1: `mb s1 rx N`. Watch STM32 USB @ 2 Mbaud, not dco_control. |
| `ENABLE_MB_MOD_STREAM` | **off** (main sketch) | MB→DCO `'m'` @ 1 kHz. Also define on DCO to consume it. Leave off until pitch cutover. |
| `NUM_VOICES` | **4** | Voice array sizes |
| `SERIAL_INNER_MAX_PAYLOAD` | **16** | `'m'` / `'t'` stream |

---

## Contributing / hacking

- Start with [`docs/MAINBOARD_REINTEGRATION.md`](docs/MAINBOARD_REINTEGRATION.md) and [`docs/REFERENCE_AI.md`](docs/REFERENCE_AI.md).
- Keep `ParamId` numbers stable (canonical: DCO `params_def.h`).
- Route new controls through `params.ino` + slim LE headers — no BE `'e'`/`'f'`/`'s'` streams.
- When changing CV math, keep [`docs/CV_AND_PINS.md`](docs/CV_AND_PINS.md) and [`docs/MODULATION_PIPELINE.md`](docs/MODULATION_PIPELINE.md) in sync.
