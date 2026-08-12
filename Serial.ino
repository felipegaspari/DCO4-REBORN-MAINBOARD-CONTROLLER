static SerialCommandTable mainSerial2Lut;
static SerialParserContext mainSerial2Parser;
static SerialCommandTable inputSerial8Lut;
static SerialParserContext inputSerial8Parser;

#ifdef MB_UART_RX_LOG
const char* g_mb_uart_rx_port = "?";
#endif

#define MB_FILTER_FORWARD_RING_CAP 8
static uint8_t mb_filter_forward_ring[MB_FILTER_FORWARD_RING_CAP][INPUT_SERIAL_LEN_FILTER_BLOCK];
static uint8_t mb_filter_forward_ring_head = 0;
static uint8_t mb_filter_forward_ring_tail = 0;
static uint8_t mb_filter_forward_ring_count = 0;

static void mb_filter_forward_ring_enqueue(const uint8_t* payload) {
  if (mb_filter_forward_ring_count == MB_FILTER_FORWARD_RING_CAP) {
    mb_filter_forward_ring_tail = (uint8_t)((mb_filter_forward_ring_tail + 1u) % MB_FILTER_FORWARD_RING_CAP);
    mb_filter_forward_ring_count--;
  }
  memcpy(mb_filter_forward_ring[mb_filter_forward_ring_head], payload, INPUT_SERIAL_LEN_FILTER_BLOCK);
  mb_filter_forward_ring_head = (uint8_t)((mb_filter_forward_ring_head + 1u) % MB_FILTER_FORWARD_RING_CAP);
  mb_filter_forward_ring_count++;
}

static void mb_filter_forward_ring_drain() {
#ifdef ENABLE_SERIAL8
  while (mb_filter_forward_ring_count > 0 && Serial8.availableForWrite() > 0) {
    serial_frame_write(
      Serial8,
      INPUT_CMD_FILTER_BLOCK,
      mb_filter_forward_ring[mb_filter_forward_ring_tail],
      INPUT_SERIAL_LEN_FILTER_BLOCK
    );
    mb_filter_forward_ring_tail = (uint8_t)((mb_filter_forward_ring_tail + 1u) % MB_FILTER_FORWARD_RING_CAP);
    mb_filter_forward_ring_count--;
  }
#else
  (void)mb_filter_forward_ring_tail;
  (void)mb_filter_forward_ring_head;
  (void)mb_filter_forward_ring_count;
#endif
}

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

// Serial2 ('a'-'d' from the DCO): apply locally, then mirror to Input so a
// preset recall or USB/MIDI edit moves the panel's faders and pots and reaches
// the Screen. Only the DCO-origin path mirrors — forwarding an Input-origin
// block back to Input would echo every panel edit straight to its sender.
static void main_handle_filter_block(char c, const uint8_t* payload, uint8_t len) {
  if (len != INPUT_SERIAL_LEN_FILTER_BLOCK) return;
  input_handle_filter_block(c, payload, len);
#ifdef ENABLE_SERIAL8
  if (Serial8.availableForWrite() > 0) {
    serial_frame_write(Serial8, INPUT_CMD_FILTER_BLOCK, payload, INPUT_SERIAL_LEN_FILTER_BLOCK);
  } else {
    mb_filter_forward_ring_enqueue(payload);
  }
#endif
}

// The envelope blocks take the plain drop-if-full path rather than the filter
// ring: they arrive only on a preset recall or a host edit, never per encoder
// tick, so there is nothing to smooth out.
static void mb_forward_block_to_input(uint8_t cmd, const uint8_t* payload, uint8_t len) {
#ifdef ENABLE_SERIAL8
  if (Serial8.availableForWrite() < 1) return;
  serial_frame_write(Serial8, cmd, payload, len);
#endif
}

static void main_handle_adsr1(char c, const uint8_t* payload, uint8_t len) {
  if (len != INPUT_SERIAL_LEN_ADSR_BLOCK) return;
  input_handle_adsr1(c, payload, len);
  mb_forward_block_to_input(INPUT_CMD_ADSR1_BLOCK, payload, len);
}

static void main_handle_adsr2(char c, const uint8_t* payload, uint8_t len) {
  if (len != INPUT_SERIAL_LEN_ADSR_BLOCK) return;
  input_handle_adsr2(c, payload, len);
  mb_forward_block_to_input(INPUT_CMD_ADSR2_BLOCK, payload, len);
}

// EnvDCO drives the DCO's own engine; the Mainboard only keeps a copy and
// passes it on so the panel faders follow.
static void main_handle_adsr3(char c, const uint8_t* payload, uint8_t len) {
  if (len != INPUT_SERIAL_LEN_ADSR_BLOCK) return;
  input_handle_adsr3(c, payload, len);
  mb_forward_block_to_input(INPUT_CMD_ADSR3_BLOCK, payload, len);
}

static void input_handle_param16(char, const uint8_t* payload, uint8_t len) {
  if (len != INPUT_SERIAL_LEN_PARAM_16) return;
  ParamFrame frame;
  decode_param_p(payload, frame);
  update_parameters(frame.id, (int16_t)frame.value);
}

// 'q': the 16-char name Input just edited. Keep a local display copy and pass it
// through to the DCO, which needs it staged before the PARAM_PRESET_SAVE that
// follows (the name is written verbatim into the preset record).
static void input_handle_preset_name(char, const uint8_t* payload, uint8_t len) {
  if (len != INPUT_SERIAL_LEN_PRESET_NAME) return;
  for (uint8_t i = 0; i < INPUT_SERIAL_LEN_PRESET_NAME; i++) presetName[i] = payload[i];
#ifdef ENABLE_SERIAL2
  serial_frame_write(Serial2, INPUT_CMD_PRESET_NAME, payload, INPUT_SERIAL_LEN_PRESET_NAME);
#endif
}

// --- preset directory relay ('N' / 'O' / 'L') ----------------------------------
//
// The DCO owns the 256-slot preset store; Input only caches slot names in RAM.
// Those three frames are pure pass-through, but nothing crosses this board
// implicitly — every relayed byte needs its own LUT entry and handler.
//
// Writes go straight out rather than through mb_filter_forward_ring: a directory
// push is 256 back-to-back 'O' frames and dropping any of them would leave a
// wrong name in Input's cache. Both links run at 2.5 Mbaud, so the Mainboard
// forwards at exactly the rate it receives and the 512-byte TX buffer absorbs
// the jitter; it is a one-shot on Input boot / browse-mode-enter, never a
// per-encoder-tick path.

// 'N' Input → DCO: "send me the whole directory".
static void input_handle_preset_dir_request(char, const uint8_t* payload, uint8_t len) {
  if (len != INPUT_SERIAL_LEN_PRESET_DIR_REQUEST) return;
#ifdef ENABLE_SERIAL2
  serial_frame_write(Serial2, INPUT_CMD_PRESET_DIR_REQUEST, payload, len);
#endif
}

// 'O' DCO → Input: one [slot][name:16] directory entry.
static void main_handle_preset_dir_entry(char, const uint8_t* payload, uint8_t len) {
  if (len != INPUT_SERIAL_LEN_PRESET_DIR_ENTRY) return;
#ifdef ENABLE_SERIAL8
  serial_frame_write(Serial8, INPUT_CMD_PRESET_DIR_ENTRY, payload, len);
#endif
}

// 'L' DCO → Input: the DCO just finished loading [slot] (boot recall, MIDI PC,
// USB/dco_control or Input itself), so Input's Screen display can follow.
static void main_handle_preset_loaded(char, const uint8_t* payload, uint8_t len) {
  if (len != INPUT_SERIAL_LEN_PRESET_LOADED) return;
#ifdef ENABLE_SERIAL8
  serial_frame_write(Serial8, INPUT_CMD_PRESET_LOADED, payload, len);
#endif
}

// --- panel block frames: apply here, and shadow them on the DCO -----------------
//
// The Mainboard owns the analog envelopes and filter, so it consumes 'a'-'d'
// itself. But the DCO owns the preset store, and it builds a record out of its
// own copies of those same values — and on this instrument the DCO has no direct
// link to the panel. So an Input-origin block is applied locally *and* passed on,
// otherwise a saved preset would capture stale envelope and filter settings.
//
// Only the Serial8 (Input) side forwards. The identical frames arriving on
// Serial2 come from the DCO itself (USB/MIDI edits and preset recalls) and must
// not be echoed back to it.
static void mb_forward_block_to_dco(uint8_t cmd, const uint8_t* payload, uint8_t len) {
#ifdef ENABLE_SERIAL2
  serial_frame_write(Serial2, cmd, payload, len);
#endif
}

static void input8_handle_adsr1(char c, const uint8_t* payload, uint8_t len) {
  input_handle_adsr1(c, payload, len);
  mb_forward_block_to_dco(INPUT_CMD_ADSR1_BLOCK, payload, len);
}

static void input8_handle_adsr2(char c, const uint8_t* payload, uint8_t len) {
  input_handle_adsr2(c, payload, len);
  mb_forward_block_to_dco(INPUT_CMD_ADSR2_BLOCK, payload, len);
}

static void input8_handle_adsr3(char c, const uint8_t* payload, uint8_t len) {
  input_handle_adsr3(c, payload, len);
  mb_forward_block_to_dco(INPUT_CMD_ADSR3_BLOCK, payload, len);
}

static void input8_handle_filter_block(char c, const uint8_t* payload, uint8_t len) {
  input_handle_filter_block(c, payload, len);
  mb_forward_block_to_dco(INPUT_CMD_FILTER_BLOCK, payload, len);
}

static const SerialCommandDef mainSerial2Commands[] = {
  { SERIAL_CMD_NOTE_ON,     SERIAL_PAYLOAD_LEN_NOTE_ON,     main_handle_note_on },
  { SERIAL_CMD_NOTE_OFF,    SERIAL_PAYLOAD_LEN_NOTE_OFF,    main_handle_note_off },
  { SERIAL_CMD_EXPRESSION,  SERIAL_PAYLOAD_LEN_EXPRESSION,  main_handle_expression },
  { SERIAL_CMD_PARAM_16,    INPUT_SERIAL_LEN_PARAM_16,      main_handle_param16 },
  { SERIAL_CMD_PARAM_32,    INPUT_SERIAL_LEN_PARAM_32,      main_handle_param32 },
  // USB/MIDI/preset mirror from DCO: applied here, then relayed to Input.
  { INPUT_CMD_ADSR1_BLOCK,  INPUT_SERIAL_LEN_ADSR_BLOCK,    main_handle_adsr1 },
  { INPUT_CMD_ADSR2_BLOCK,  INPUT_SERIAL_LEN_ADSR_BLOCK,    main_handle_adsr2 },
  { INPUT_CMD_ADSR3_BLOCK,  INPUT_SERIAL_LEN_ADSR_BLOCK,    main_handle_adsr3 },
  { INPUT_CMD_FILTER_BLOCK, INPUT_SERIAL_LEN_FILTER_BLOCK,  main_handle_filter_block },
  // Preset store answers, relayed on to Input.
  { INPUT_CMD_PRESET_DIR_ENTRY, INPUT_SERIAL_LEN_PRESET_DIR_ENTRY, main_handle_preset_dir_entry },
  { INPUT_CMD_PRESET_LOADED,    INPUT_SERIAL_LEN_PRESET_LOADED,    main_handle_preset_loaded },
};

static const SerialCommandDef inputSerial8Commands[] = {
  { INPUT_CMD_ADSR1_BLOCK,  INPUT_SERIAL_LEN_ADSR_BLOCK,   input8_handle_adsr1 },
  { INPUT_CMD_ADSR2_BLOCK,  INPUT_SERIAL_LEN_ADSR_BLOCK,   input8_handle_adsr2 },
  { INPUT_CMD_ADSR3_BLOCK,  INPUT_SERIAL_LEN_ADSR_BLOCK,   input8_handle_adsr3 },
  { INPUT_CMD_FILTER_BLOCK, INPUT_SERIAL_LEN_FILTER_BLOCK, input8_handle_filter_block },
  { INPUT_CMD_PARAM_16,     INPUT_SERIAL_LEN_PARAM_16,     input_handle_param16 },
  { INPUT_CMD_PRESET_NAME,  INPUT_SERIAL_LEN_PRESET_NAME,  input_handle_preset_name },
  { INPUT_CMD_PRESET_DIR_REQUEST, INPUT_SERIAL_LEN_PRESET_DIR_REQUEST, input_handle_preset_dir_request },
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
  mb_filter_forward_ring_drain();
  serial_parser_drain(mainSerial2Parser, mainSerial2Lut, Serial2, SERIAL_DRAIN_BYTE_BUDGET);
  mb_filter_forward_ring_drain();
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
