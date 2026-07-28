# System overview (Mainboard pointer)

This board is the **DCO4 mainboard** (STM32): the modulation brain that runs ADSRs and LFOs, drives filter/VCA/resonance CVs (timer PWM + MCP4728 DACs), selects analog waves (74HC595), and routes parameters between the DCO, input, and screen boards.

The **canonical four-board system overview** (ownership table, UART topology, ParamId note) lives in the sibling DCO repo:

**[`../../DCO4_DCO/docs/SYSTEM_OVERVIEW.md`](../../DCO4_DCO/docs/SYSTEM_OVERVIEW.md)**

Do not fork a second full system document here. If a UART pin, baud, or ownership fact in that file disagrees with this firmware, fix the **DCO** copy.

### This board’s UARTs (verified in firmware)

| Port | Pins | Baud | Peer |
|------|------|------|------|
| `Serial` | USB | 2 000 000 | Debug |
| `Serial1` | PA10 / PA9 | 2 500 000 | Screen |
| `Serial2` | PD6 / PD5 | 2 500 000 | DCO |
| `Serial8` | PE0 / PE1 | 2 500 000 | Input |

Board-specific detail: [`CV_AND_PINS.md`](CV_AND_PINS.md), [`MODULATION_PIPELINE.md`](MODULATION_PIPELINE.md), [`REFERENCE_AI.md`](REFERENCE_AI.md).
