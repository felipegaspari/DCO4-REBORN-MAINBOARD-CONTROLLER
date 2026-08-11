# Mainboard reintegration (DCO3 math on DCO4 wiring)

STM32 Mainboard is the analog / modulation hub again. DCO keeps MIDI + PIO pitch **and the preset store**. Boards speak **slim little-endian** frames (no finish byte). `params_def.h` and `serial_input_protocol.h` are shared supersets, byte-identical on every board of both projects (master copies: `DCO3-MONOSYNTH/DCO/`); this board implements a subset and must never renumber an ID.

See also: [`SYSTEM_OVERVIEW.md`](SYSTEM_OVERVIEW.md), [`PRESET_RELAY.md`](PRESET_RELAY.md), [`MODULATION_PIPELINE.md`](MODULATION_PIPELINE.md), DCO [`docs/MAINBOARD_REINTEGRATION.md`](../../DCO/docs/MAINBOARD_REINTEGRATION.md).

---

## Topology (classic DCO4 PCB)

```mermaid
flowchart LR
  MIDI["MIDI USB+DIN"] --> DCO["DCO RP2040\n4x2 PIO + RANGE + PW\nLittleFS preset store"]
  DCO -->|"Serial2 GP20/21 2.5M"| MB["STM32 Mainboard"]
  Input["Input RP2040"] -->|"Serial2 GP4/5"| MB
  Input -->|"Serial1 GP0/1"| Screen["Screen RP2040"]
  MB -->|"Serial1 PA9/PA10"| Screen
  MB -->|"Serial8 PE0/PE1"| Input
  MB --> Analog["4x VCA + 4x VCF + reso PWM\nMCP4728 SQR/Sub\n74HC595 waves"]
```

| Port | Pins | Peer |
|------|------|------|
| Mainboard `Serial2` | PD6 / PD5 | DCO GP21 / GP20 |
| Mainboard `Serial8` | PE0 / PE1 | Input Serial2 GP4 / GP5 |
| Mainboard `Serial1` | PA10 / PA9 | Screen (optional second feed) |
| Input `Serial1` | GP0 / GP1 | Screen GP13 |
| DCO `Serial1` | GP0 / GP1 | DIN MIDI @ 31250 |

---

## Ownership

| Owns | Board |
|------|--------|
| MIDI, voice alloc, PIO pitch, RANGE/PW PWM, amp-comp, autotune, Character pitch/PW jitter, per-osc **pitch** drift, **256-slot LittleFS preset store** | DCO |
| EnvVCA ×4, EnvVCF ×4, EnvDCO ×4, LFO1/LFO2, VCF drift, mod matrix, VCA/VCF/reso PWM, MCP4728, 74HC595, Input RX, DCO peer, **preset relay** | Mainboard |
| Panel, Screen UI frames, RAM cache of the 256 slot **names** | Input |
| LVGL UI | Screen |

OSC3 ParamIds (33–35, 38, 87–89) stay in the enum. Analog 4×2 has SQR1 / SQR2 / Sub only — OSC3 level dest is a no-op. Dist 52–53 is stored/stubbed.

---

## Slim LE protocol

Inner frame: `[cmd:1][payload:N]`. Little-endian. No finish byte. Optional COBS via `SERIAL_FRAMING_COBS`. `SERIAL_INNER_MAX_PAYLOAD` is **17** here (`Serial.h`), sized by the relayed `'O'` entry.

Frames marked **relay** are not consumed by this board's DSP; they exist only because Input and the DCO have no direct wire. Registration rules: [`PRESET_RELAY.md`](PRESET_RELAY.md).

### Input → Mainboard (Serial8)

| Cmd | Payload | Meaning |
|-----|---------|---------|
| `'a'` | 8 B LE | EnvVCA A/D/S/R — applied **and** relayed to DCO |
| `'b'` | 8 B | EnvVCF — applied **and** relayed |
| `'c'` | 8 B | EnvDCO — applied **and** relayed |
| `'d'` | 8 B | CUTOFF, RESONANCE, ADSR2→VCF, LFO2→VCF — applied **and** relayed |
| `'p'` | id + i16 LE | ParamId (PW=210, ADSR1→VCA=222; ids 170–173 relay to DCO) |
| `'q'` | 16 ASCII | Preset name — stored locally **and** relayed |
| `'N'` | 1 B pad | Preset directory request — **relay** only |

### DCO → Mainboard (Serial2)

| Cmd | Payload | Meaning |
|-----|---------|---------|
| `'n'` | voice, vel, note, flags | Note on. flags: bit0 retrigger ADSR, bit1 porta-only |
| `'o'` | voice | Note off (gate off only) |
| `'e'` | AT, MW, PB u16 LE | Aftertouch, mod wheel, pitch bend (matrix sources) |
| `'x'` | id + u32 LE | Gap 154 / cal 155 |
| `'p'` | id + i16 LE | Persistable / AT helpers |
| `'a'` | 8 B LE | USB/MIDI EnvVCA times (same as Input `'a'`) |
| `'b'` | 8 B | USB/MIDI EnvVCF times (same as Input `'b'`) |
| `'d'` | 8 B | USB/MIDI CUTOFF, RESONANCE, ADSR2→VCF, LFO2→VCF — also **mirrored** to Input so recalls move the pots |
| `'O'` | 17 B | Preset directory entry `[slot][name:16]` — **relay** to Input |
| `'L'` | 1 B | Preset `[slot]` finished loading — **relay** to Input |

DCO-origin `'a'`/`'b'`/`'d'` are **not** echoed back to the DCO, and only `'d'` is mirrored on to Input (from `main_handle_filter_block()`, the Serial2-only wrapper). `'c'` is not registered on Serial2: EnvDCO from the DCO side stays DCO-local.

### Mainboard → DCO (Serial2)

| Cmd | Payload | Meaning |
|-----|---------|---------|
| `'m'` | 16 B LE | LFO1 Q15, LFO2 Q15, EnvDCO Q15×4, matrix pitch Q24 |
| `'t'` | 16 B | Bench ASCII: `[n:u8][n bytes]`, `n ≤ 15`, pad to 16 |
| `'p'` | id + i16 LE | DCO-owned ParamIds forwarded, including preset/cal 170–173 |
| `'a'` / `'b'` / `'c'` / `'d'` | 8 B LE | Panel-origin blocks shadowed on the DCO so its preset records are current |
| `'q'` | 16 ASCII | Preset name staged before a save |
| `'N'` | 1 B pad | Preset directory request |

`'m'` layout (16 bytes after cmd):

```
[0..1]   LFO1Level i16 LE (Q15)
[2..3]   LFO2Level i16 LE (Q15)
[4..11]  EnvDCO_q15[0..3] i16 LE
[12..15] matrix_pitch_mod_q24 i32 LE
```

Send ~1 kHz. DCO default ignores `'m'` and runs LFO/Env/matrix locally (`ENABLE_MB_MOD_STREAM` off). Opt-in: DCO consumes `'m'` and applies local depth bakes (`LFO1_TO_DCO`, per-osc 216–218, `ADSR3_TO_DETUNE1`, `LFO2_TO_PW`, Character). Pitch drift LFOs stay on DCO either way.

### Mainboard → Input (Serial8)

| Cmd | Payload | Meaning |
|-----|---------|---------|
| `'x'` | id + u32 LE | Gap 154 (Input relays to Screen), cal 155 |
| `'p'` | id + i16 LE | Persistable mirror |
| `'d'` | 8 B LE | Filter block mirror so Input's pots follow a recall; parked in an 8-deep ring if Serial8 is full |
| `'O'` | 17 B | Relayed directory entry |
| `'L'` | 1 B | Relayed preset-loaded notice |

---

## Numeric domains

| Bus | Domain | Use |
|-----|--------|-----|
| Env / LFO / matrix src | Q15 | Hot `(src * scale) >> 15` |
| Pitch modifiers | Q24 | Octave-fraction; `applyDepthQ24` |
| Timer PWM / MCP | u12 0..4095 | Analog edge |

CV bakers: ADSR depths `/512`, LFO depths `/1024` + negative polarity. See DCO [`CV_MOD_SCALES.md`](../../DCO/docs/CV_MOD_SCALES.md).
