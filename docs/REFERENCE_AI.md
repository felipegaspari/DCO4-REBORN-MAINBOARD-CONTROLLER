# DCO4_Mainboard_Controller — Reference (AI / developers)

Deep semantic map of the STM32 mainboard firmware. Prefer this for “how does X work?”; use [`FILE_INDEX.md`](FILE_INDEX.md) for “where is function Y / who calls it?”.

- Contract: [`MAINBOARD_REINTEGRATION.md`](MAINBOARD_REINTEGRATION.md)
- Preset relay: [`PRESET_RELAY.md`](PRESET_RELAY.md)
- System: [`SYSTEM_OVERVIEW.md`](SYSTEM_OVERVIEW.md) → [`../../DCO/docs/SYSTEM_OVERVIEW.md`](../../DCO/docs/SYSTEM_OVERVIEW.md)
- Pipeline: [`MODULATION_PIPELINE.md`](MODULATION_PIPELINE.md)
- Pins: [`CV_AND_PINS.md`](CV_AND_PINS.md)
- Serial how-to: [`README_serial_and_params.md`](README_serial_and_params.md)

---

## What this board owns

| Owns | Does not own |
|------|----------------|
| EnvVCA / EnvVCF / EnvDCO ×4 (Q15) | Voice allocation / PIO DCO pitch (DCO board) |
| LFO1/2 + VCF drift + mod matrix | Front-panel scanning (Input); preset store (DCO LittleFS) |
| Filter/VCA/resonance PWM CVs | TFT UI (Screen) |
| SQR/Sub DAC levels, wave mux enables | Amp/PW calibration measurement (DCO) |
| Param routing hub (forwards DCO-owned IDs) | Preset records / slot names (DCO / Input cache) |
| Preset relay Input ↔ DCO (`'q'`/`'N'`/`'O'`/`'L'`, blocks, ids 170–173) | |

`params_def.h` and `serial_input_protocol.h` are shared supersets, byte-identical on every board in both projects (master: `DCO3-MONOSYNTH/DCO/`); this board implements a subset. OSC3 level dest is a no-op on 4×2 analog.

---

## Runtime model

Single Arduino core. Soft timers in `millisTimer()` gate Serial1+8 (~223 µs) and 1 ms drift + `'m'` TX.

Every `loop` always: Serial2 → LFO1/2 → ADSR_update → `update_CV_outs` (or manual-cal).

---

## Modules (edit carefully)

### Serial / params

- **RX DCO** (`read_serial_2`): `'n'`/`'o'`/`'e'`/`'p'`/`'x'` + USB/MIDI `'a'`/`'b'`/`'d'` analog mirror → notes / expression / `update_parameters` / filter+ADSR times; `'O'`/`'L'` relayed on to Input.
- **RX Input** (`read_serial_8`): slim `'a'`–`'d'`/`'p'`/`'q'`/`'N'`. The `input8_*` wrappers apply `'a'`–`'d'` locally **and** forward them to the DCO so the DCO's preset records are not built from stale envelope/filter values; `'q'` is stored and relayed, `'N'` is pure relay.
- **RX Screen** (`read_serial_1`): **stub**.
- **TX DCO**: `'m'` @ 1 ms (LFO + EnvDCO + matrix pitch Q24); immediate slim `'p'` forwards, including preset / cal ids 170–173; relayed `'q'`/`'N'` and panel blocks.
- **TX Input:** gap/cal `'x'` 154/155 + persistable `'p'` mirror + `'d'` filter mirror (ring-buffered when Serial8 is full); relayed `'O'`/`'L'`.

Relaying is per command byte — there is no generic pass-through, so an unregistered byte dies here. Contract and rationale: [`PRESET_RELAY.md`](PRESET_RELAY.md).

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
| `formulas.*` float path | Deleted; depths bake in `cv_out.ino` / `params.ino` |
| `flashData.*` local preset store | Deleted; presets live on the DCO |
| `BU2505FV.ino` / `ENABLE_SPI` | Off |
| `ENABLE_SCREEN` / `Screen.h` | Off; `Screen.h` deleted |
| `autotune.h` | Deleted |

---

## Data paths worth memorizing

1. **Note → VCA/VCF CV:** DCO `'n'`/`'o'` → EnvVCA/VCF Q15 → `update_CV_outs` → timers.
2. **Panel filter block → cutoff:** Input `'d'` (Serial8) or DCO USB/MIDI `'d'` (Serial2) → CUTOFF/RESONANCE/depths → scale bake → next CV tick.
3. **EnvDCO / LFO → DCO pitch:** local clocks → `'m'` → DCO depth bakes + Character + pitch drift.
4. **Wave / SQR enable:** Param apply → `update_waveSelector` / MCP; digital square enables also forward to DCO.
5. **Manual cal:** Param flags → `update_CV_outs_manual_calibration` + DCO forwards; gap 154 DCO→MB→Input→Screen.
6. **Preset save:** Input `'q'` (stored + relayed) → Input `'p'` 170 → `apply_param_preset_save` → `forward_dco` → DCO writes the record from its own shadow, which the relayed `'a'`–`'d'` blocks keep current.
7. **Preset load / browse:** Input `'p'` 171 or `'N'` relayed to the DCO; the DCO answers with `'p'`/`'a'`–`'d'` (applied here, mirrored on) plus `'O'`/`'L'` (relayed straight to Input).

---

## What to edit carefully

- **ParamId numbers** — `params_def.h` is a shared superset (master `DCO3-MONOSYNTH/DCO/params_def.h`); never renumber, and copy the master out to every board.
- **Relay tables** — `inputSerial8Commands[]` / `mainSerial2Commands[]` in `Serial.ino` and `paramTable[]` in `params.ino`. A preset/DCO-bound byte missing from them is dropped silently between Input and the DCO, with no build error.
- **`serial_protocol.h` / `serial_input_protocol.h` payload sizes** — must match peer parsers. `'O'` is 17 B and `'m'`/`'t'` are 16 B, so `Serial.h` sets `SERIAL_INNER_MAX_PAYLOAD` to 17 before `serial_frame.h` locks its default.
- **Timer pin ↔ voice mapping** — hardware; document in `CV_AND_PINS.md` if changed.
- **`update_CV_outs` polarity** — VCF is inverted (`4095 - …`); VCA uses AS2164 table.
- **`'m'` rate** — keep ~1 kHz; do not stream 8× Q24 pitch at voice-task rate.
