# Mainboard CV and pin map

Hardware mapping for **DCO4_Mainboard_Controller** (STM32). Signal routing comes from live `update_CV_outs()` / `mcpUpdate()` / `waveSelector` code and `Timers.h` pin macros.

---

## UART

| Port | Pins (RX / TX) | Baud | Peer |
|------|----------------|------|------|
| `Serial` | USB | 2 000 000 | Debug |
| `Serial1` | PA10 / PA9 | 2 500 000 | Screen |
| `Serial2` | PD6 / PD5 | 2 500 000 | DCO |
| `Serial8` | PE0 / PE1 | 2 500 000 | Input |

Defined in `Serial.h`; begun in `setup()`.

---

## Timer PWM → resonance / cutoff / VCA

Overflow / resolution uses `ADSR_1_DACSIZE` (see ADSR headers). Active writes are in `PWM.ino` `setPWMOuts()`.

### Resonance (same duty on all voices)

| Voice | Timer / channel | Pin macro | GPIO |
|-------|-----------------|-----------|------|
| 1 | TIM4 CH1 | `TIM4_CH1_PIN` | PD12 |
| 2 | TIM8 CH1 | `TIM8_CH1_PIN` | PC6 |
| 3 | TIM5 CH1 | `TIM5_CH1_PIN` | PA0 |
| 4 | TIM3 CH1 | `TIM3_CH1_PIN` | PB4 |

### Cutoff (`VCF_PWM[i]`)

| Voice | Timer / channel | Pin macro | GPIO |
|-------|-----------------|-----------|------|
| 0 | TIM12 CH1 | `TIM12_CH1_PIN` | PB14 |
| 1 | TIM4 CH3 | `TIM4_CH3_PIN` | PD14 |
| 2 | TIM15 CH2 | `TIM15_CH2_PIN` | PE6 |
| 3 | TIM5 CH3 | `TIM5_CH3_PIN` | PA2 |

### VCA (`VCA_PWM[i]`)

| Voice | Timer / channel | Pin macro | GPIO |
|-------|-----------------|-----------|------|
| 0 | TIM2 CH3 | `TIM2_CH3_PIN` | PB10 |
| 1 | TIM13 CH1 | `TIM13_CH1_PIN` | PA6 |
| 2 | TIM1 CH4 | `TIM1_CH4_PIN` | PE14 |
| 3 | TIM3 CH3 | `TIM3_CH3_PIN` | PB0 |

Other timer channels are configured in `init_timers()` for historical / alternate wiring; only the rows above are written every loop by `setPWMOuts()`.

## RP2350 candidate pin mapping

RP2350 supports the same GPIO/PWM structure as RP2040, so this is a direct logical mapping. Use any board-specific available GPIOs if needed, but this keeps the signals grouped clearly.

| RP2350 GPIO | Function |
|-------------|----------|
| `GP0` | VCA voice 2 |
| `GP1` | VCA voice 0 |
| `GP2` | VCA voice 3 |
| `GP3` | VCA voice 1 |
| `GP4` | resonance filter 1 |
| `GP5` | resonance filter 2 |
| `GP6` | resonance filter 3 |
| `GP7` | resonance filter 4 |
| `GP8` | VCF voice 0 |
| `GP9` | VCF voice 1 |
| `GP10` | VCF voice 2 |
| `GP11` | VCF voice 3 |
| `GP12` | 74HC595 DATA |
| `GP13` | 74HC595 LATCH |
| `GP14` | 74HC595 CLK |
| `GP18` | I2C SDA for MCP4728 |
| `GP19` | I2C SCL for MCP4728 |
| `GP20` | Serial1 RX to Screen |
| `GP21` | Serial1 TX to Screen |
| `GP22` | Serial2 RX to DCO |
| `GP23` | Serial2 TX to DCO |
| `GP24` | Serial8 RX to Input (PIO/UART) |
| `GP25` | Serial8 TX to Input (PIO/UART) |

> Note: RP2350 typically has two hardware UARTs. Use the two native UARTs for Screen and DCO, and implement Input on PIO or bit-banged UART if the board does not expose a third hardware UART.

## Full pin usage

| Pin | Function |
|-----|----------|
| `PE9` | TIM1 CH1 PWM output — configured, unused |
| `PE11` | TIM1 CH2 PWM output — configured, unused |
| `PE13` | TIM1 CH3 PWM output — configured, unused |
| `PE14` | TIM1 CH4 PWM output — VCA voice 2 |
| `PA15` | TIM2 CH1 PWM output — configured, unused |
| `PB3` | TIM2 CH2 PWM output — configured, unused |
| `PB10` | TIM2 CH3 PWM output — VCA voice 0 |
| `PB11` | TIM2 CH4 PWM output — configured, unused |
| `PB4` | TIM3 CH1 PWM output — resonance filter 4 |
| `PB5` | TIM3 CH2 PWM output — configured, unused |
| `PB0_ALT1` | TIM3 CH3 PWM output — VCA voice 3 |
| `PB1_ALT1` | TIM3 CH4 PWM output — configured, unused |
| `PD12` | TIM4 CH1 PWM output — resonance filter 1 |
| `PD13` | TIM4 CH2 PWM output — configured, unused |
| `PD14` | TIM4 CH3 PWM output — VCF voice 1 |
| `PD15` | TIM4 CH4 PWM output — configured, unused |
| `PA0_ALT1` | TIM5 CH1 PWM output — resonance filter 3 |
| `PA1_ALT1` | TIM5 CH2 PWM output — configured, unused |
| `PA2_ALT1` | TIM5 CH3 PWM output — VCF voice 3 |
| `PA3_ALT1` | TIM5 CH4 PWM output — configured, unused |
| `PC6_ALT1` | TIM8 CH1 PWM output — resonance filter 2 |
| `PC7_ALT1` | TIM8 CH2 PWM output — configured, unused |
| `PC8_ALT1` | TIM8 CH3 PWM output — configured, unused |
| `PC9_ALT1` | TIM8 CH4 PWM output — configured, unused |
| `PB14_ALT2` | TIM12 CH1 PWM output — VCF voice 0 |
| `PB15_ALT2` | TIM12 CH2 PWM output — configured, unused |
| `PA6_ALT1` | TIM13 CH1 PWM output — VCA voice 1 |
| `PE5` | TIM15 CH1 PWM output — configured, unused |
| `PE6` | TIM15 CH2 PWM output — VCF voice 2 |
| `PE2` | 74HC595 LATCH |
| `PE3` | 74HC595 CLK |
| `PE4` | 74HC595 DATA |
| `PB8` | I2C SCL for MCP4728 DACs |
| `PB9` | I2C SDA for MCP4728 DACs |
| `PA10` | Serial1 RX to Screen |
| `PA9` | Serial1 TX to Screen |
| `PD6` | Serial2 RX to DCO |
| `PD5` | Serial2 TX to DCO |
| `PE0` | Serial8 RX to Input |
| `PE1` | Serial8 TX to Input |

## Active PWM outputs only

| Pin | Function |
|-----|----------|
| `PE14` | TIM1 CH4 PWM output — VCA voice 2 |
| `PB10` | TIM2 CH3 PWM output — VCA voice 0 |
| `PB4` | TIM3 CH1 PWM output — resonance filter 4 |
| `PB0_ALT1` | TIM3 CH3 PWM output — VCA voice 3 |
| `PD12` | TIM4 CH1 PWM output — resonance filter 1 |
| `PD14` | TIM4 CH3 PWM output — VCF voice 1 |
| `PA0_ALT1` | TIM5 CH1 PWM output — resonance filter 3 |
| `PA2_ALT1` | TIM5 CH3 PWM output — VCF voice 3 |
| `PC6_ALT1` | TIM8 CH1 PWM output — resonance filter 2 |
| `PB14_ALT2` | TIM12 CH1 PWM output — VCF voice 0 |
| `PA6_ALT1` | TIM13 CH1 PWM output — VCA voice 1 |
| `PE6` | TIM15 CH2 PWM output — VCF voice 2 |

> Unused configured PWM channels: `PE9`, `PE11`, `PE13`, `PA15`, `PB3`, `PB11`, `PB5`, `PB1_ALT1`, `PD13`, `PD15`, `PA1_ALT1`, `PA3_ALT1`, `PC7_ALT1`, `PC8_ALT1`, `PB15_ALT2`, `PE5`.

**`ENABLE_SD`:** defined in the main sketch. TIM8 CH4 / some SDMMC pins (`PD2`, `PC8`–`PC12`) are reserved; see comments in `DCO4_Mainboard_Controller.ino` and gated paths in `Timers.ino`.

**`ENABLE_SPI`:** currently **off**. When off, TIM2 CH1/CH2 and TIM3 CH2 PWM modes are still enabled; SPI DAC path is inactive.

---

## I2C MCP4728 (square / sub levels)

| Item | Value |
|------|--------|
| SDA | PB9 |
| SCL | PB8 |
| Clock | 1 MHz |
| Chips | `mcp` @ `0x63`, `mcp2` @ `0x64`, `mcp3` @ `0x65` |

`mcpUpdate()` channel mapping (comments in `PWM.ino`):

| Chip | A | B | C | D |
|------|---|---|---|---|
| `mcp` | V1 OSC1 SQR | V2 OSC2 SQR | V2 OSC1 SQR | V3 OSC2 SQR |
| `mcp2` | V3 OSC1 SQR | V4 OSC2 SQR | V4 OSC1 SQR | SUB3 |
| `mcp3` | SUB4 | SUB1 | SUB2 | V1 OSC2 SQR |

Levels: globals `SQR1Level`, `SQR2Level`, `SubLevel` (updated from params).

---

## Wave selector (74HC595 × 2)

| Signal | Pin |
|--------|-----|
| DATA | PE4 |
| LATCH | PE2 |
| CLK | PE3 |

Mux bit arrays (crossed cabling — active values in `waveSelector.h`):

| Wave | Bits per voice 0..3 |
|------|---------------------|
| TRI | 14, 10, 6, 2 |
| SINE | 13, 9, 5, 1 |
| SAW2 | 12, 8, 4, 0 |
| SAW | 15, 11, 7, 3 |

`update_waveSelector(wave)` writes pins from `sawStatus` / `saw2Status` / `triStatus` / `sqr2Status` (inverted).

---

## Inactive hardware paths

| Path | Status |
|------|--------|
| Local SD / EEPROM presets | Deleted (`flashData.*`); the DCO owns the 256-slot store and this board relays for it |
| SPI BU2505FV | `ENABLE_SPI` off; functions commented |
| On-board Screen module | `ENABLE_SCREEN` / `Screen.h` commented out |
| Autotune include | Commented |

See [`MODULATION_PIPELINE.md`](MODULATION_PIPELINE.md) for how CVs are computed before these pins are written.
