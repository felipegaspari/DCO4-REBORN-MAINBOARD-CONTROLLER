## DCO4 – Mainboard Controller (STM32 analog / modulation hub)

Firmware for the **classic DCO4 mainboard**: STM32 Arduino sketch that owns **Q15 envelopes + LFOs + mod matrix**, **filter/VCA/resonance CVs** (hardware timer PWM), **square/sub levels** (MCP4728), **analog wave select** (74HC595), and **slim LE serial** between DCO, Input, and Screen.

Four voices (`NUM_VOICES 4`), one VCF per voice. Single-threaded `setup()` / `loop()`.

It is also the **relay** on the panel↔preset-store path: Input has no direct wire to the DCO, which owns the 256-slot preset store, so preset frames cross this board one registered command byte at a time. See [`docs/PRESET_RELAY.md`](docs/PRESET_RELAY.md).

Contract: [`docs/MAINBOARD_REINTEGRATION.md`](docs/MAINBOARD_REINTEGRATION.md). System: [`docs/SYSTEM_OVERVIEW.md`](docs/SYSTEM_OVERVIEW.md).

All detailed documentation lives under **[`docs/`](docs/)**. This README is the entry point.

---

## Documentation

| Doc | Contents |
|-----|----------|
| [`docs/MAINBOARD_REINTEGRATION.md`](docs/MAINBOARD_REINTEGRATION.md) | UART topology, ownership, `'n'`/`'o'`/`'m'` layouts |
| [`docs/PRESET_RELAY.md`](docs/PRESET_RELAY.md) | Preset traffic Input ↔ DCO across this board |
| [`docs/SYSTEM_OVERVIEW.md`](docs/SYSTEM_OVERVIEW.md) | Local UART table + pointer to DCO overview |
| [`docs/MODULATION_PIPELINE.md`](docs/MODULATION_PIPELINE.md) | Notes / params → Q15 ADSR/LFO → PWM/DAC → `'m'` |
| [`docs/CV_AND_PINS.md`](docs/CV_AND_PINS.md) | Timer PWM, MCP4728, 74HC595, UART pins |
| [`docs/FILE_INDEX.md`](docs/FILE_INDEX.md) | Files + functions |
| [`docs/REFERENCE_AI.md`](docs/REFERENCE_AI.md) | Semantic map |
| [`docs/README_serial_and_params.md`](docs/README_serial_and_params.md) | Shared serial / ParamId how-to |

**Suggested reading:** this README → reintegration → preset relay → modulation / CV pins → FILE_INDEX or REFERENCE_AI.

---

## Features

- **Modulation:** EnvVCA / EnvVCF / EnvDCO ×4 (Q15 Bézier); LFO1 / LFO2 (`getWaveQ15`); VCF drift LFOs; 8-slot mod matrix.
- **CV outs:** Resonance, cutoff, VCA via STM32 timer PWM (`update_CV_outs`); OSC A / OSC B / Sub levels via three MCP4728 DACs (one channel per voice per oscillator plus one per sub, each carrying that oscillator's whole wave mix). ADSR depths `/512`, LFO `/1024` + negative LFO polarity; AS2164 Bézier + reso→VCA lerp.
- **Wave select:** Dual 74HC595 (OSC1 saw/tri, OSC2 saw/pulse on `osc2PulsePins` cabling).
- **Serial:** 2.5 Mbaud slim LE — DCO Serial2 (`'n'`/`'o'`/`'e'`/`'m'`/`'p'`/`'x'`/`'t'` plus the `'a'`/`'b'`/`'d'` mirror and `'O'`/`'L'`), Input Serial8 (`'a'`–`'d'`/`'p'`/`'q'`/`'N'`), Screen Serial1 optional; USB debug @ 2 Mbaud.
- **Params:** 256-entry jump table, `int16_t` apply; DCO-owned IDs forwarded, including preset / cal commands 170–173.
- **Preset relay:** `'q'` / `'N'` and panel `'a'`–`'d'` on to the DCO, `'O'` / `'L'` back to Input — per command byte, no generic pass-through.
- **Manual calibration:** `update_CV_outs_manual_calibration` mutes mux / parks VCA.

**Not active:** SPI BU2505FV (`ENABLE_SPI` off). The on-board Screen module (`Screen.h`) and `autotune.h` are deleted, not just gated.

**No preset storage here.** The DCO owns the 256-slot LittleFS store for the whole instrument; Input keeps a RAM cache of slot names only. The old `flashData.*` SD/EEPROM path is deleted and `presetName[16]` on this board is a display/echo copy. Details: [`docs/PRESET_RELAY.md`](docs/PRESET_RELAY.md).

---

## High-level architecture

| Subsystem | Files | Role |
|-----------|-------|------|
| Entry / loop | `MAINBOARD-CONTROLLER.ino` | Init + soft-timer schedule |
| Serial RX | `Serial.ino`, `serial_*.h` | DCO notes/expression/params; Input blocks/params; preset relay both ways |
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

Main sketch: `MAINBOARD-CONTROLLER.ino`.

### Libraries (`_build_libs` and `libraries/`)

| Library | Path | Notes |
|---------|------|--------|
| `DCO-PROTOCOL` | `_build_libs/DCO-PROTOCOL` → `../../DCO-PROTOCOL` | Shared `params_def.h` / serial headers. Included by relative `_build_libs/DCO-PROTOCOL/...` paths so Arduino IDE finds them (same pattern as ADSR / RoxMux). `libraries/DCO-PROTOCOL` remains for tools that scan sketch libraries. |
| `ADSR_Bezier` | `_build_libs/ADSR_Bezier` → `../../ADSR_Bezier` | Symlink to monorepo root; track **`Q15`**. |
| `mo-lfo` | `_build_libs/mo-lfo` → `../../mo-lfo` | Symlink to monorepo root; track **`q15`**. `LFO.ino` `#include`s `mo-lfo.cpp` so Arduino IDE links it. |
| `RoxMux_fela` | `_build_libs/RoxMux_fela` → `../../RoxMux_FELA` | FELA fork. Included by relative path like ADSR/mo-lfo (`"_build_libs/RoxMux_fela/src/RoxMux_fela.h"`) so Arduino IDE finds it without library discovery. Nested headers use quotes. |

Sketchbook / core libs (not under `_build_libs`): `MCP4728_multiaddress`, `Wire`.

### Feature flags

| Flag | Default | Effect |
|------|---------|--------|
| `ENABLE_SD` | **on** (main sketch) | SDMMC pin reservation only; no preset code on this board |
| `ENABLE_SPI` | **off** | SPI / BU2505 path |
| `ENABLE_SERIAL*` | **on** (`Serial.h`) | Per-UART compile-in |
| `ENABLE_SCREEN` | **off** | Legacy; `Screen.h` no longer exists |
| `RUNNING_AVERAGE` | **on** (main sketch) | Slim `bench.h` profiler; dco_control 40/41/42 + dump-once 45 |
| `MB_UART_PROBE` | **off** (main sketch) | 1 Hz `'t'` labels on Serial/1/2/8; dco_control Board shows `mb s2` if DCO is on USART2 PD5. Desyncs Input/Screen if plugged in. |
| `MB_UART_RX_LOG` | **off** (main sketch) | USB CDC one line per UART cmd byte on Serial2/8 (`mb s2 rx 'n'`, unknown `'m'` included); `'p'` also prints id/value. Serial1: `mb s1 rx N`. Watch STM32 USB @ 2 Mbaud, not dco_control. |
| `ENABLE_MB_MOD_STREAM` | **off** (main sketch) | MB→DCO `'m'` @ 1 kHz. Also define on DCO to consume it. Leave off until pitch cutover. |
| `NUM_VOICES` | **4** | Voice array sizes |
| `SERIAL_INNER_MAX_PAYLOAD` | **17** | `'O'` preset-directory entry (`[slot][name:16]`); `'m'` / `'t'` need only 16 |

---

## Contributing / hacking

- Start with [`docs/MAINBOARD_REINTEGRATION.md`](docs/MAINBOARD_REINTEGRATION.md), [`docs/PRESET_RELAY.md`](docs/PRESET_RELAY.md) and [`docs/REFERENCE_AI.md`](docs/REFERENCE_AI.md).
- `params_def.h` and `serial_input_protocol.h` live in the shared [`DCO-PROTOCOL`](../DCO-PROTOCOL/README.md) library. Edit there once; never renumber an existing ID. Each board implements only the subset it cares about.
- Route new controls through `params.ino` + slim LE headers — no BE `'e'`/`'f'`/`'s'` streams.
- A new preset/DCO-bound command byte must be registered in this board's relay tables (`inputSerial8Commands[]` / `mainSerial2Commands[]`, or `paramTable[]` for a `'p'` id) or it is silently dropped between Input and the DCO. See [`docs/PRESET_RELAY.md`](docs/PRESET_RELAY.md).
- When changing CV math, keep [`docs/CV_AND_PINS.md`](docs/CV_AND_PINS.md) and [`docs/MODULATION_PIPELINE.md`](docs/MODULATION_PIPELINE.md) in sync.
