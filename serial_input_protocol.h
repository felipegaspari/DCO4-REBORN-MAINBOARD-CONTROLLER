#ifndef SERIAL_INPUT_PROTOCOL_H
#define SERIAL_INPUT_PROTOCOL_H

#include <stdint.h>

// -----------------------------------------------------------------------------
// Slim LE inner protocol (Input ↔ Mainboard, and shared 'p'/'q'/'x').
//
//   [1 byte] command
//   [N bytes] payload (little-endian multi-byte fields)
//
// On-wire default: identical to the inner frame (SERIAL_FRAMING_RAW).
// 0x00 is reserved as the COBS delimiter and is never a command.
// -----------------------------------------------------------------------------

enum InputSerialCmd : uint8_t {
  INPUT_CMD_ADSR1_BLOCK  = 'a',  // EnvVCA times
  INPUT_CMD_ADSR2_BLOCK  = 'b',  // EnvVCF times
  INPUT_CMD_ADSR3_BLOCK  = 'c',  // EnvDCO times
  INPUT_CMD_FILTER_BLOCK = 'd',
  INPUT_CMD_PARAM_16     = 'p',  // id + int16 LE
  INPUT_CMD_PRESET_NAME  = 'q',  // 8 ASCII chars
  INPUT_CMD_PARAM_32     = 'x',  // id + u32 LE (gap 154 / cal 155)
};

static constexpr uint8_t INPUT_SERIAL_LEN_ADSR_BLOCK   = 8;
static constexpr uint8_t INPUT_SERIAL_LEN_FILTER_BLOCK = 8;
static constexpr uint8_t INPUT_SERIAL_LEN_PARAM_16     = 3;
static constexpr uint8_t INPUT_SERIAL_LEN_PRESET_NAME  = 8;
static constexpr uint8_t INPUT_SERIAL_LEN_PARAM_32     = 5;

static inline uint8_t serial_input_payload_len(uint8_t cmd) {
  switch (cmd) {
    case INPUT_CMD_ADSR1_BLOCK:
    case INPUT_CMD_ADSR2_BLOCK:
    case INPUT_CMD_ADSR3_BLOCK:  return INPUT_SERIAL_LEN_ADSR_BLOCK;
    case INPUT_CMD_FILTER_BLOCK: return INPUT_SERIAL_LEN_FILTER_BLOCK;
    case INPUT_CMD_PARAM_16:     return INPUT_SERIAL_LEN_PARAM_16;
    case INPUT_CMD_PRESET_NAME:  return INPUT_SERIAL_LEN_PRESET_NAME;
    case INPUT_CMD_PARAM_32:     return INPUT_SERIAL_LEN_PARAM_32;
    default:                     return 0;
  }
}

#endif // SERIAL_INPUT_PROTOCOL_H
