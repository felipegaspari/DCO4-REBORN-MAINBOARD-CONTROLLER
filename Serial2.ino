static void serial_send_mod_stream() {
#if defined(ENABLE_SERIAL2) && defined(ENABLE_MB_MOD_STREAM)
  uint8_t payload[SERIAL_PAYLOAD_LEN_MOD_STREAM];
  encode_u16_le(payload + 0, (uint16_t)LFO1Level);
  encode_u16_le(payload + 2, (uint16_t)LFO2Level);
  for (uint8_t i = 0; i < 4; i++) {
    encode_u16_le(payload + 4 + i * 2, (uint16_t)ADSR1Level_q15[i]);
  }
  encode_u32_le(payload + 12, (uint32_t)matrix_pitch_mod_q24);
  serial_frame_write(Serial2, SERIAL_CMD_MOD_STREAM, payload, SERIAL_PAYLOAD_LEN_MOD_STREAM);
#endif
}

inline void sendSerial() {
  serial_send_mod_stream();
}

void serialSendParamToDCO(uint8_t id, int16_t value) {
#ifdef ENABLE_SERIAL2
  uint8_t payload[INPUT_SERIAL_LEN_PARAM_16];
  encode_param_p(payload, id, value);
  serial_frame_write(Serial2, SERIAL_CMD_PARAM_16, payload, INPUT_SERIAL_LEN_PARAM_16);
#endif
}

void serialSendParam32ToDCO(uint8_t id, uint32_t value) {
#ifdef ENABLE_SERIAL2
  uint8_t payload[INPUT_SERIAL_LEN_PARAM_32];
  encode_param32(payload, id, value);
  serial_frame_write(Serial2, SERIAL_CMD_PARAM_32, payload, INPUT_SERIAL_LEN_PARAM_32);
#endif
}

void serialSendParam32ToInput(uint8_t id, uint32_t value) {
#ifdef ENABLE_SERIAL8
  uint8_t payload[INPUT_SERIAL_LEN_PARAM_32];
  encode_param32(payload, id, value);
  serial_frame_write(Serial8, INPUT_CMD_PARAM_32, payload, INPUT_SERIAL_LEN_PARAM_32);
#endif
}

void serialSendParam16ToInput(uint8_t id, int16_t value) {
#ifdef ENABLE_SERIAL8
  uint8_t payload[INPUT_SERIAL_LEN_PARAM_16];
  encode_param_p(payload, id, value);
  serial_frame_write(Serial8, INPUT_CMD_PARAM_16, payload, INPUT_SERIAL_LEN_PARAM_16);
#endif
}

void serialSendParam32ToScreen(uint8_t id, uint32_t value) {
#ifdef ENABLE_SERIAL1
  uint8_t payload[INPUT_SERIAL_LEN_PARAM_32];
  encode_param32(payload, id, value);
  serial_frame_write(Serial1, INPUT_CMD_PARAM_32, payload, INPUT_SERIAL_LEN_PARAM_32);
#endif
}

void serial_send_bench_text_on(Stream& port, const uint8_t* data, uint8_t n) {
  if (n > SERIAL_BENCH_TEXT_DATA_MAX) n = SERIAL_BENCH_TEXT_DATA_MAX;
  uint8_t payload[SERIAL_PAYLOAD_LEN_BENCH_TEXT] = {};
  payload[0] = n;
  for (uint8_t i = 0; i < n; i++) {
    payload[1 + i] = data[i];
  }
  serial_frame_write(port, SERIAL_CMD_BENCH_TEXT, payload, SERIAL_PAYLOAD_LEN_BENCH_TEXT);
}

void serial_send_bench_text_chunk(const uint8_t* data, uint8_t n) {
#ifdef ENABLE_SERIAL2
  serial_send_bench_text_on(Serial2, data, n);
#else
  (void)data;
  (void)n;
#endif
}

#ifdef MB_UART_PROBE
void mb_uart_probe_poll() {
  static uint16_t ticks = 0;
  if (++ticks < 1000u) return;
  ticks = 0;
#ifdef ENABLE_SERIAL
  Serial.print("mb usb\n");
#endif
#ifdef ENABLE_SERIAL1
  serial_send_bench_text_on(Serial1, reinterpret_cast<const uint8_t*>("mb s1\n"), 6);
#endif
#ifdef ENABLE_SERIAL2
  serial_send_bench_text_on(Serial2, reinterpret_cast<const uint8_t*>("mb s2\n"), 6);
#endif
#ifdef ENABLE_SERIAL8
  serial_send_bench_text_on(Serial8, reinterpret_cast<const uint8_t*>("mb s8\n"), 6);
#endif
}
#endif
