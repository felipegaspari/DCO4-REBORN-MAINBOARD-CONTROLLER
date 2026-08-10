# System overview (Mainboard pointer)

This board is the **DCO4 mainboard** (STM32): analog / modulation hub. Q15 ADSRs and LFOs, filter/VCA/resonance CVs (timer PWM + MCP4728), 74HC595 waves, slim LE serial to DCO / Input / Screen.

Canonical four-board overview: **[`../../DCO/docs/SYSTEM_OVERVIEW.md`](../../DCO/docs/SYSTEM_OVERVIEW.md)**. Reintegration contract: [`MAINBOARD_REINTEGRATION.md`](MAINBOARD_REINTEGRATION.md).

### This board’s UARTs (verified in firmware)

| Port | Pins | Baud | Peer |
|------|------|------|------|
| `Serial` | USB | 2 000 000 | Debug |
| `Serial1` | PA10 / PA9 | 2 500 000 | Screen (optional) |
| `Serial2` | PD6 / PD5 | 2 500 000 | DCO |
| `Serial8` | PE0 / PE1 | 2 500 000 | Input |

Board-specific detail: [`CV_AND_PINS.md`](CV_AND_PINS.md), [`MODULATION_PIPELINE.md`](MODULATION_PIPELINE.md), [`REFERENCE_AI.md`](REFERENCE_AI.md).
