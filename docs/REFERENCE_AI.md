# DCO4_Mainboard_Controller — Reference (AI / developers)

Deep semantic map of the STM32 mainboard firmware. Prefer this for “how does X work?”; use [`FILE_INDEX.md`](FILE_INDEX.md) for “where is function Y / who calls it?”.

- System: [`SYSTEM_OVERVIEW.md`](SYSTEM_OVERVIEW.md) → canonical in DCO4_DCO
- Pipeline: [`MODULATION_PIPELINE.md`](MODULATION_PIPELINE.md)
- Pins: [`CV_AND_PINS.md`](CV_AND_PINS.md)
- Serial how-to: [`README_serial_and_params.md`](README_serial_and_params.md)
- Entry: [`../README.md`](../README.md)

---

## What this board owns

| Owns | Does not own |
|------|----------------|
| ADSR1/2 (VCA/VCF), local ADSR3 state | Voice allocation / PIO DCO pitch (DCO board) |
| LFO1/2 + VCF drift | Front-panel scanning / preset files (Input) |
| Filter/VCA/resonance PWM CVs | TFT UI (Screen) |
| SQR/Sub DAC levels, wave mux enables | Amp/PW calibration measurement (DCO) |
| Param routing hub (many forwards to DCO) | |

`params_def.h` on this repo is the coordination copy for shared ParamIds.

---

## Runtime model

Single Arduino core. Soft timers in `millisTimer()` gate Serial1+8 (~223 µs), 1 ms drift/flags, and ~99 µs `sendSerial`.

Every `loop` always: clear note edge buffers → Serial2 → LFO1/2 → ADSR_update → PWM outs → (optional) sendSerial.

Typical loop budget (Nov 2025 benchmark comment in main sketch): ~7.5 µs average when profiling was enabled — ADSR + PWM dominate.

---

## Modules (edit carefully)

### Serial / params

- **RX DCO** (`read_serial_2`): notes `'n'`/`'o'`; params `'p'`/`'w'`/`'x'` → `update_parameters`.
- **RX Input** (`read_serial_8`): dense ADSR/filter/PW blocks + params.
- **RX Screen** (`read_serial_1`): **stub** (no live cases).
- **TX DCO** (`sendSerial` + immediate helpers): live `'s'` (ADSR3), `'f'` (PW), and per-apply `'w'`/`'p'`. Queued `serialSendParam*Buf` arrays are unused today.
- **TX Screen/Input:** used for calibration gap/offset reporting (`'x'`), not a full UI mirror.

### Modulation

- ADSR Bézier library instances per voice × 3.
- LFO levels are bipolar around mid-scale (`*_CC_HALF - getWave`).
- Drift LFOs only affect `VCF_DRIFT[]` when `analogDrift ≠ 0`.

### CV path

- Hot math in `setPWMOuts()` — do not casually reorder timer writes without checking analog settling.
- `mcpUpdate()` is not every loop; only when SQR/Sub params change or during manual cal.
- Manual cal replaces the entire PWM path while the flag is set.

### Dead / inactive

| Item | Status |
|------|--------|
| `flashData.ino` | Fully commented |
| `BU2505FV.ino` / `ENABLE_SPI` | Off |
| `ENABLE_SCREEN` / `Screen.h` | Off |
| `autotune.h` | Include commented |
| `LFO3()` | Declared, not defined |
| Several `serial_send_*` / curve helpers | Dead (no callers) |

---

## Data paths worth memorizing

1. **Note → VCA/VCF CV:** DCO `'n'`/`'o'` → ADSR1/2 levels → `setPWMOuts` → timers.
2. **Panel filter block → cutoff:** Input `'d'` → CUTOFF/RESONANCE/depths → formulas → next `setPWMOuts`.
3. **ADSR3 → DCO pitch/PWM:** Input `'c'` or 1 ms refresh → `'s'` on Serial2; related ParamIds also forward.
4. **Wave / SQR enable:** Param apply → `update_waveSelector` / DCO forward for digital square enables.
5. **Manual cal:** Param flags → `setPWMOutsManualCalibration` + DCO/Input/Screen param forwards.

---

## What to edit carefully

- **ParamId numbers** — cross-repo; never renumber without updating all boards.
- **`serial_protocol.h` / `serial_input_protocol.h` payload sizes** — must match peer parsers.
- **Timer pin ↔ voice mapping** — hardware; document in `CV_AND_PINS.md` if changed.
- **`setPWMOuts` polarity** — VCF is inverted (`4095 - …`); VCA uses AS2164 table.
- **Soft timer periods** — Serial drain vs send rates affect UI latency and DCO ADSR3 freshness.

---

## Libraries (external)

`ADSR_Bezier`, `mo-lfo`, `MCP4728_multiaddress`, `RoxMux`, `Wire`, optional `RunningAverage`.
