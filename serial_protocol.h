#ifndef SERIAL_PROTOCOL_H
#define SERIAL_PROTOCOL_H

#include <stdint.h>
#include "_build_libs/DCO-PROTOCOL/serial_input_protocol.h"

// -----------------------------------------------------------------------------
// Mainboard ↔ DCO slim LE command set (Serial2 @ 2.5 M).
// See docs/MAINBOARD_REINTEGRATION.md.
// -----------------------------------------------------------------------------

enum SerialCmd : uint8_t {
  SERIAL_CMD_NOTE_ON     = 'n',  // DCO → MB
  SERIAL_CMD_NOTE_OFF    = 'o',  // DCO → MB
  SERIAL_CMD_EXPRESSION  = 'e',  // DCO → MB  AT / MW / pitch bend
  SERIAL_CMD_MOD_STREAM  = 'm',  // MB → DCO  LFO + EnvDCO + matrix pitch
  SERIAL_CMD_BENCH_TEXT  = 't',  // MB → DCO  dump ASCII chunk
  SERIAL_CMD_PARAM_16    = 'p',
  SERIAL_CMD_PARAM_32    = 'x',
  SERIAL_CMD_SCREEN_SIGNAL = 's',  // DCO → MB → Screen  ScreenMode byte, relayed verbatim
};

// 'n': [voice][vel][note][flags]  flags bit0=retrigger ADSR, bit1=porta-only
static constexpr uint8_t SERIAL_PAYLOAD_LEN_NOTE_ON    = 4;
static constexpr uint8_t NOTE_FLAG_RETRIGGER           = (1u << 0);
static constexpr uint8_t NOTE_FLAG_PORTA_ONLY          = (1u << 1);

// 'o': [voice]
static constexpr uint8_t SERIAL_PAYLOAD_LEN_NOTE_OFF   = 1;

// 'e': [aftertouch u8][modwheel u8][pitch_bend u16 LE]
static constexpr uint8_t SERIAL_PAYLOAD_LEN_EXPRESSION = 4;

// 'm': LFO1 i16, LFO2 i16, EnvDCO_q15[4] i16, matrix_pitch_q24 i32  (all LE)
static constexpr uint8_t SERIAL_PAYLOAD_LEN_MOD_STREAM = 16;

// 't': [n:u8][n bytes ASCII][pad to 16]; n <= 15
static constexpr uint8_t SERIAL_PAYLOAD_LEN_BENCH_TEXT = 16;
static constexpr uint8_t SERIAL_BENCH_TEXT_DATA_MAX    = 15;

// 's': [ScreenMode] — the raw byte the Screen puts in serialSignal.
static constexpr uint8_t SERIAL_PAYLOAD_LEN_SCREEN_SIGNAL = 1;

static inline uint8_t serial_protocol_payload_len(uint8_t cmd) {
  switch (cmd) {
    case SERIAL_CMD_NOTE_ON:     return SERIAL_PAYLOAD_LEN_NOTE_ON;
    case SERIAL_CMD_NOTE_OFF:    return SERIAL_PAYLOAD_LEN_NOTE_OFF;
    case SERIAL_CMD_EXPRESSION:  return SERIAL_PAYLOAD_LEN_EXPRESSION;
    case SERIAL_CMD_MOD_STREAM:  return SERIAL_PAYLOAD_LEN_MOD_STREAM;
    case SERIAL_CMD_BENCH_TEXT:  return SERIAL_PAYLOAD_LEN_BENCH_TEXT;
    case SERIAL_CMD_PARAM_16:    return INPUT_SERIAL_LEN_PARAM_16;
    case SERIAL_CMD_PARAM_32:    return INPUT_SERIAL_LEN_PARAM_32;
    case SERIAL_CMD_SCREEN_SIGNAL: return SERIAL_PAYLOAD_LEN_SCREEN_SIGNAL;
    // USB/MIDI/preset mirror (Input cmds on DCO→MB Serial2). The EnvDCO engine
    // is DCO-local, but 'c' still crosses so it can be relayed to the panel.
    case INPUT_CMD_ADSR1_BLOCK:
    case INPUT_CMD_ADSR2_BLOCK:
    case INPUT_CMD_ADSR3_BLOCK:  return INPUT_SERIAL_LEN_ADSR_BLOCK;
    case INPUT_CMD_FILTER_BLOCK: return INPUT_SERIAL_LEN_FILTER_BLOCK;
    default:                     return 0;
  }
}

#endif // SERIAL_PROTOCOL_H
