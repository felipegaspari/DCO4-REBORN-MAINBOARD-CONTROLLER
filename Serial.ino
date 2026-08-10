static SerialCommandTable mainSerial2Lut;
static SerialParserContext mainSerial2Parser;
static SerialCommandTable inputSerial8Lut;
static SerialParserContext inputSerial8Parser;

#ifdef MB_UART_RX_LOG
const char* g_mb_uart_rx_port = "?";
#endif

static void main_handle_note_on(char, const uint8_t* payload, uint8_t len) {
  if (len != SERIAL_PAYLOAD_LEN_NOTE_ON) return;
  uint8_t voice_n = payload[0];
  if (voice_n >= NUM_VOICES) return;
  velocity[voice_n] = payload[1];
  midi_velocity[voice_n] = payload[1];
  note[voice_n] = payload[2];
  note_flags[voice_n] = payload[3];
  noteEnd[voice_n] = 0;
  if (payload[3] & NOTE_FLAG_PORTA_ONLY) {
    noteStart[voice_n] = 0;
    return;
  }
  noteStart[voice_n] = 1;
  if (payload[3] & NOTE_FLAG_RETRIGGER) {
    mod_matrix_on_note_on();
  }
}

static void main_handle_note_off(char, const uint8_t* payload, uint8_t len) {
  if (len != SERIAL_PAYLOAD_LEN_NOTE_OFF) return;
  uint8_t voice_n = payload[0];
  if (voice_n >= NUM_VOICES) return;
  noteEnd[voice_n] = 1;
  noteStart[voice_n] = 0;
}

static void main_handle_expression(char, const uint8_t* payload, uint8_t len) {
  if (len != SERIAL_PAYLOAD_LEN_EXPRESSION) return;
  aftertouch = payload[0];
  mod_wheel_in = payload[1];
  midi_pitch_bend = decode_u16_le(payload + 2);
  mod_matrix_set_aftertouch(payload[0]);
  mod_matrix_set_mod_wheel(payload[1]);
}

static void main_handle_param16(char, const uint8_t* payload, uint8_t len) {
  if (len != INPUT_SERIAL_LEN_PARAM_16) return;
  ParamFrame frame;
  decode_param_p(payload, frame);
  update_parameters(frame.id, (int16_t)frame.value);
  if (frame.id != PARAM_DEBUG_COMMAND) {
    serialSendParam16ToInput(frame.id, (int16_t)frame.value);
  }
}

static void main_handle_param32(char, const uint8_t* payload, uint8_t len) {
  if (len != INPUT_SERIAL_LEN_PARAM_32) return;
  ParamFrame frame;
  decode_param_x(payload, frame);
  if (frame.id == PARAM_GAP_FROM_DCO ||
      frame.id == PARAM_MANUAL_CALIBRATION_OFFSET_FROM_DCO) {
    update_parameters(frame.id, (int16_t)frame.value);
    if (frame.id == PARAM_GAP_FROM_DCO) {
      serialSendParam32ToInput(PARAM_GAP_FROM_DCO, (uint32_t)frame.value);
      serialSendParam32ToScreen(PARAM_GAP_FROM_DCO, (uint32_t)frame.value);
    } else {
      serialSendParam32ToInput(PARAM_MANUAL_CALIBRATION_OFFSET_FROM_DCO, (uint32_t)frame.value);
    }
  }
}

static void input_handle_adsr1(char, const uint8_t* payload, uint8_t len) {
  if (len != INPUT_SERIAL_LEN_ADSR_BLOCK) return;
  uint16_t dirty = 0, v;
  v = decode_u16_le(payload + 0);
  if (v != ADSR_VCA_attack)  { ADSR_VCA_attack = v; dirty |= ADSR_DIRTY_VCA_A; }
  v = decode_u16_le(payload + 2);
  if (v != ADSR_VCA_decay)   { ADSR_VCA_decay = v; dirty |= ADSR_DIRTY_VCA_D; }
  v = decode_u16_le(payload + 4);
  if (v != ADSR_VCA_sustain) { ADSR_VCA_sustain = v; dirty |= ADSR_DIRTY_VCA_S; }
  v = decode_u16_le(payload + 6);
  if (v != ADSR_VCA_release) { ADSR_VCA_release = v; dirty |= ADSR_DIRTY_VCA_R; }
  if (dirty) mark_adsr_params_dirty(dirty);
}

static void input_handle_adsr2(char, const uint8_t* payload, uint8_t len) {
  if (len != INPUT_SERIAL_LEN_ADSR_BLOCK) return;
  uint16_t dirty = 0, v;
  v = decode_u16_le(payload + 0);
  if (v != ADSR_VCF_attack)  { ADSR_VCF_attack = v; dirty |= ADSR_DIRTY_VCF_A; }
  v = decode_u16_le(payload + 2);
  if (v != ADSR_VCF_decay)   { ADSR_VCF_decay = v; dirty |= ADSR_DIRTY_VCF_D; }
  v = decode_u16_le(payload + 4);
  if (v != ADSR_VCF_sustain) { ADSR_VCF_sustain = v; dirty |= ADSR_DIRTY_VCF_S; }
  v = decode_u16_le(payload + 6);
  if (v != ADSR_VCF_release) { ADSR_VCF_release = v; dirty |= ADSR_DIRTY_VCF_R; }
  if (dirty) mark_adsr_params_dirty(dirty);
}

static void input_handle_adsr3(char, const uint8_t* payload, uint8_t len) {
  if (len != INPUT_SERIAL_LEN_ADSR_BLOCK) return;
  uint16_t dirty = 0, v;
  v = decode_u16_le(payload + 0);
  if (v != ADSR1_attack)  { ADSR1_attack = v; dirty |= ADSR_DIRTY_DCO_A; }
  v = decode_u16_le(payload + 2);
  if (v != ADSR1_decay)   { ADSR1_decay = v; dirty |= ADSR_DIRTY_DCO_D; }
  v = decode_u16_le(payload + 4);
  if (v != ADSR1_sustain) { ADSR1_sustain = v; dirty |= ADSR_DIRTY_DCO_S; }
  v = decode_u16_le(payload + 6);
  if (v != ADSR1_release) { ADSR1_release = v; dirty |= ADSR_DIRTY_DCO_R; }
  if (dirty) mark_adsr_params_dirty(dirty);
}

static void input_handle_filter_block(char, const uint8_t* payload, uint8_t len) {
  if (len != INPUT_SERIAL_LEN_FILTER_BLOCK) return;
  CUTOFF     = decode_u16_le(payload + 0);
  RESONANCE  = decode_u16_le(payload + 2);
  ADSR2toVCF = decode_i16_le(payload + 4);
  LFO2toVCF  = decode_u16_le(payload + 6);
  cv_bake_adsr2_to_vcf_scale();
  cv_bake_lfo2_to_vcf_scale();
}

static void input_handle_param16(char, const uint8_t* payload, uint8_t len) {
  if (len != INPUT_SERIAL_LEN_PARAM_16) return;
  ParamFrame frame;
  decode_param_p(payload, frame);
  update_parameters(frame.id, (int16_t)frame.value);
}

static void input_handle_preset_name(char, const uint8_t* payload, uint8_t len) {
  if (len != INPUT_SERIAL_LEN_PRESET_NAME) return;
  for (uint8_t i = 0; i < 8; i++) presetName[i] = payload[i];
}

static const SerialCommandDef mainSerial2Commands[] = {
  { SERIAL_CMD_NOTE_ON,     SERIAL_PAYLOAD_LEN_NOTE_ON,     main_handle_note_on },
  { SERIAL_CMD_NOTE_OFF,    SERIAL_PAYLOAD_LEN_NOTE_OFF,    main_handle_note_off },
  { SERIAL_CMD_EXPRESSION,  SERIAL_PAYLOAD_LEN_EXPRESSION,  main_handle_expression },
  { SERIAL_CMD_PARAM_16,    INPUT_SERIAL_LEN_PARAM_16,      main_handle_param16 },
  { SERIAL_CMD_PARAM_32,    INPUT_SERIAL_LEN_PARAM_32,      main_handle_param32 },
  // USB/MIDI analog mirror from DCO (same handlers as Input Serial8).
  { INPUT_CMD_ADSR1_BLOCK,  INPUT_SERIAL_LEN_ADSR_BLOCK,    input_handle_adsr1 },
  { INPUT_CMD_ADSR2_BLOCK,  INPUT_SERIAL_LEN_ADSR_BLOCK,    input_handle_adsr2 },
  { INPUT_CMD_FILTER_BLOCK, INPUT_SERIAL_LEN_FILTER_BLOCK,  input_handle_filter_block },
};

static const SerialCommandDef inputSerial8Commands[] = {
  { INPUT_CMD_ADSR1_BLOCK,  INPUT_SERIAL_LEN_ADSR_BLOCK,   input_handle_adsr1 },
  { INPUT_CMD_ADSR2_BLOCK,  INPUT_SERIAL_LEN_ADSR_BLOCK,   input_handle_adsr2 },
  { INPUT_CMD_ADSR3_BLOCK,  INPUT_SERIAL_LEN_ADSR_BLOCK,   input_handle_adsr3 },
  { INPUT_CMD_FILTER_BLOCK, INPUT_SERIAL_LEN_FILTER_BLOCK, input_handle_filter_block },
  { INPUT_CMD_PARAM_16,     INPUT_SERIAL_LEN_PARAM_16,     input_handle_param16 },
  { INPUT_CMD_PRESET_NAME,  INPUT_SERIAL_LEN_PRESET_NAME,  input_handle_preset_name },
};

void init_serial_parsers() {
  serial_parser_reset(mainSerial2Parser);
  serial_parser_reset(inputSerial8Parser);
  serial_command_table_init(mainSerial2Lut, mainSerial2Commands,
                            sizeof(mainSerial2Commands) / sizeof(mainSerial2Commands[0]));
  serial_command_table_init(inputSerial8Lut, inputSerial8Commands,
                            sizeof(inputSerial8Commands) / sizeof(inputSerial8Commands[0]));
}

inline void read_serial_1() {
#ifdef ENABLE_SERIAL1
#ifdef MB_UART_RX_LOG
  int n = Serial1.available();
  if (n > 0) {
#ifdef ENABLE_SERIAL
    Serial.print("mb s1 rx ");
    Serial.println(n);
#endif
    while (Serial1.available() > 0) (void)Serial1.read();
  }
#else
  while (Serial1.available() > 0) (void)Serial1.read();
#endif
#endif
}

inline void read_serial_2() {
#ifdef ENABLE_SERIAL2
#ifdef MB_UART_RX_LOG
  g_mb_uart_rx_port = "s2";
#endif
  serial_parser_drain(mainSerial2Parser, mainSerial2Lut, Serial2, SERIAL_DRAIN_BYTE_BUDGET);
#endif
}

inline void read_serial_8() {
#ifdef ENABLE_SERIAL8
#ifdef MB_UART_RX_LOG
  g_mb_uart_rx_port = "s8";
#endif
  serial_parser_drain(inputSerial8Parser, inputSerial8Lut, Serial8, SERIAL_DRAIN_BYTE_BUDGET);
#endif
}
