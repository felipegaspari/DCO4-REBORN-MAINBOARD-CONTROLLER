#include "include_all.h"

void serialSendParamToDCO(uint8_t id, int16_t value) {
  uint8_t payload[SERIAL_LEN_PARAM_16];
  encode_param_p(payload, id, value);
  serial_frame_write(DcoDma, CMD_PARAM_16, payload, SERIAL_LEN_PARAM_16);
}

void serialSendParam32ToDCO(uint8_t id, uint32_t value) {
  uint8_t payload[SERIAL_LEN_PARAM_32];
  encode_param32(payload, id, value);
  serial_frame_write(DcoDma, CMD_PARAM_32, payload, SERIAL_LEN_PARAM_32);
}

void serialSendParam32ToInput(uint8_t id, uint32_t value) {
  uint8_t payload[SERIAL_LEN_PARAM_32];
  encode_param32(payload, id, value);
  serial_frame_write(InputDma, CMD_PARAM_32, payload, SERIAL_LEN_PARAM_32);
}

void serialSendParam16ToInput(uint8_t id, int16_t value) {
  uint8_t payload[SERIAL_LEN_PARAM_16];
  encode_param_p(payload, id, value);
  serial_frame_write(InputDma, CMD_PARAM_16, payload, SERIAL_LEN_PARAM_16);
}

void serialSendParam16ToScreen(uint8_t id, int16_t value) {
  uint8_t payload[SERIAL_LEN_PARAM_16];
  encode_param_p(payload, id, value);
  serial_frame_write(ScreenDma, CMD_PARAM_16, payload, SERIAL_LEN_PARAM_16);
}

void serialSendParam32ToScreen(uint8_t id, uint32_t value) {
  uint8_t payload[SERIAL_LEN_PARAM_32];
  encode_param32(payload, id, value);
  serial_frame_write(ScreenDma, CMD_PARAM_32, payload, SERIAL_LEN_PARAM_32);
}