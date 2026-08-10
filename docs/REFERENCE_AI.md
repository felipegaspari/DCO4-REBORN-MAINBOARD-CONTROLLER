# DCO4_Mainboard_Controller — Reference (AI / developers)

Deep semantic map of the STM32 mainboard firmware. Prefer this for “how does X work?”; use [`FILE_INDEX.md`](FILE_INDEX.md) for “where is function Y / who calls it?”.

- Contract: [`MAINBOARD_REINTEGRATION.md`](MAINBOARD_REINTEGRATION.md)
- System: [`SYSTEM_OVERVIEW.md`](SYSTEM_OVERVIEW.md) → [`../../DCO/docs/SYSTEM_OVERVIEW.md`](../../DCO/docs/SYSTEM_OVERVIEW.md)
- Pipeline: [`MODULATION_PIPELINE.md`](MODULATION_PIPELINE.md)
- Pins: [`CV_AND_PINS.md`](CV_AND_PINS.md)
- Serial how-to: [`README_serial_and_params.md`](README_serial_and_params.md)

---

## What this board owns

| Owns | Does not own |
|------|----------------|
| EnvVCA / EnvVCF / EnvDCO ×4 (Q15) | Voice allocation / PIO DCO pitch (DCO board) |
| LFO1/2 + VCF drift + mod matrix | Front-panel scanning / preset files (Input) |
| Filter/VCA/resonance PWM CVs | TFT UI (Screen) |
| SQR/Sub DAC levels, wave mux enables | Amp/PW calibration measurement (DCO) |
| Param routing hub (forwards DCO-owned IDs) | |

Canonical ParamIds: DCO `params_def.h` (copied here). OSC3 level dest is a no-op on 4×2 analog.

---

## Runtime model

Single Arduino core. Soft timers in `millisTimer()` gate Serial1+8 (~223 µs) and 1 ms drift + `'m'` TX.

Every `loop` always: Serial2 → LFO1/2 → ADSR_update → `update_CV_outs` (or manual-cal).

---

## Modules (edit carefully)

### Serial / params

- **RX DCO** (`read_serial_2`): `'n'`/`'o'`/`'e'`/`'p'`/`'x'` + USB/MIDI `'a'`/`'b'`/`'d'` analog mirror → notes / expression / `update_parameters` / filter+ADSR times.
- **RX Input** (`read_serial_8`): slim `'a'`–`'d'`/`'p'`/`'q'`.
- **RX Screen** (`read_serial_1`): **stub**.
- **TX DCO**: `'m'` @ 1 ms (LFO + EnvDCO + matrix pitch Q24); immediate slim `'p'` forwards.
- **TX Input:** gap/cal `'x'` 154/155 + persistable `'p'` mirror.

Do **not** restore BE `'p'` or Input `'e'`/`'f'`/`'s'` streams.

### Modulation

- ADSR Bézier Q15 (`ADSR_BEZIER_NATIVE_Q15`) per voice × 3.
- LFO `getWaveQ15` full-scale; depths bake `/512` ADSR vs `/1024` LFO + negative LFO polarity.
- Drift LFOs only affect `VCF_DRIFT[]` when `analogDrift ≠ 0`.
- Matrix: dests 0–1 + sub → MCP4728; cutoff/reso → PWM; dest 9 → `'m'` Q24.

### CV path

- Hot math in `update_CV_outs()` — do not casually reorder timer writes without checking analog settling.
- `mcpUpdate()` on param/cal edges (and 1 ms matrix apply), not every CV tick unless levels change.
- Manual cal replaces the entire PWM path while the flag is set (`update_CV_outs_manual_calibration`).

### Dead / inactive

| Item | Status |
|------|--------|
| `formulas.ino` float path | Retired (stub) |
| `flashData.ino` | Fully commented |
| `BU2505FV.ino` / `ENABLE_SPI` | Off |
| `ENABLE_SCREEN` / `Screen.h` | Off |
| `autotune.h` | Include commented |

---

## Data paths worth memorizing

1. **Note → VCA/VCF CV:** DCO `'n'`/`'o'` → EnvVCA/VCF Q15 → `update_CV_outs` → timers.
2. **Panel filter block → cutoff:** Input `'d'` (Serial8) or DCO USB/MIDI `'d'` (Serial2) → CUTOFF/RESONANCE/depths → scale bake → next CV tick.
3. **EnvDCO / LFO → DCO pitch:** local clocks → `'m'` → DCO depth bakes + Character + pitch drift.
4. **Wave / SQR enable:** Param apply → `update_waveSelector` / MCP; digital square enables also forward to DCO.
5. **Manual cal:** Param flags → `update_CV_outs_manual_calibration` + DCO forwards; gap 154 DCO→MB→Input→Screen.

---

## What to edit carefully

- **ParamId numbers** — cross-repo; never renumber without updating all boards.
- **`serial_protocol.h` / `serial_input_protocol.h` payload sizes** — must match peer parsers. `'m'` is 16 B (`SERIAL_INNER_MAX_PAYLOAD` ≥ 16).
- **Timer pin ↔ voice mapping** — hardware; document in `CV_AND_PINS.md` if changed.
- **`update_CV_outs` polarity** — VCF is inverted (`4095 - …`); VCA uses AS2164 table.
- **`'m'` rate** — keep ~1 kHz; do not stream 8× Q24 pitch at voice-task rate.
