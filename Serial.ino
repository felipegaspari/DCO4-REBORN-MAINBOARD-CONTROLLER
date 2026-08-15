#include "include_all.h"

UartDmaTx ScreenDma = {0};
UartDmaTx DcoDma    = {1};
UartDmaTx InputDma  = {2};

static SerialCommandTable mainSerial2Lut;
static SerialParserContext mainSerial2Parser;
static SerialCommandTable inputSerial8Lut;
static SerialParserContext inputSerial8Parser;

// =============================================================================
// 1. Relay Helpers
// =============================================================================

static inline void relay_to_dco(uint8_t cmd, const uint8_t* payload, uint8_t len) {
#ifdef ENABLE_SERIAL2
  serial_frame_write(DcoDma, cmd, payload, len);
#endif
}

static inline void relay_to_input(uint8_t cmd, const uint8_t* payload, uint8_t len) {
#ifdef ENABLE_SERIAL8
  serial_frame_write(InputDma, cmd, payload, len);
#endif
}

static inline void relay_to_screen(uint8_t cmd, const uint8_t* payload, uint8_t len) {
#ifdef ENABLE_SERIAL1
  serial_frame_write(ScreenDma, cmd, payload, len);
#endif
}

static void forward_adsr_to_screen(uint8_t cmd, const uint8_t* payload) {
#ifdef ENABLE_SERIAL1
  uint8_t screen_payload[SERIAL_LEN_ADSR_BLOCK];
  encode_u16_le(screen_payload + 0, exp_to_lin_index(decode_u16_le(payload + 0)));
  encode_u16_le(screen_payload + 2, exp_to_lin_index(decode_u16_le(payload + 2)));
  encode_u16_le(screen_payload + 4, decode_u16_le(payload + 4));  // Sustain is linear
  encode_u16_le(screen_payload + 6, exp_to_lin_index(decode_u16_le(payload + 6)));
  relay_to_screen(cmd, screen_payload, SERIAL_LEN_ADSR_BLOCK);
#endif
}

static void forward_filter_ui_to_screen(const uint8_t* payload) {
  serialSendParam16ToScreen((uint8_t)PARAM_UI_CUTOFF, (int16_t)decode_u16_le(payload + 0));
  serialSendParam16ToScreen((uint8_t)PARAM_UI_RESONANCE, (int16_t)decode_u16_le(payload + 2));
  serialSendParam16ToScreen((uint8_t)PARAM_UI_ADSR2_TO_VCF, decode_i16_le(payload + 4));
  serialSendParam16ToScreen((uint8_t)PARAM_UI_LFO2_TO_VCF, (int16_t)decode_u16_le(payload + 6));
}

void serial_send_bench_text_chunk(const uint8_t* data, uint8_t n) {
  if (n == 0 || data == nullptr) return;
  uint8_t payload[SERIAL_LEN_BENCH_TEXT];
  payload[0] = n;
  for (uint8_t i = 0; i < n && i < SERIAL_BENCH_TEXT_DATA_MAX; i++) {
    payload[1 + i] = data[i];
  }
  serial_frame_write(DcoDma, CMD_BENCH_TEXT, payload, SERIAL_LEN_BENCH_TEXT);
}

void sendSerial() {}

// =============================================================================
// 2. Local Hardware Apply Helpers
// =============================================================================

static void apply_local_adsr1(const uint8_t* payload) {
  uint16_t dirty = 0, v;
  v = decode_u16_le(payload + 0); if (v != ADSR_VCA_attack)  { ADSR_VCA_attack = v; dirty |= ADSR_DIRTY_VCA_A; }
  v = decode_u16_le(payload + 2); if (v != ADSR_VCA_decay)   { ADSR_VCA_decay = v; dirty |= ADSR_DIRTY_VCA_D; }
  v = decode_u16_le(payload + 4); if (v != ADSR_VCA_sustain) { ADSR_VCA_sustain = v; dirty |= ADSR_DIRTY_VCA_S; }
  v = decode_u16_le(payload + 6); if (v != ADSR_VCA_release) { ADSR_VCA_release = v; dirty |= ADSR_DIRTY_VCA_R; }
  if (dirty) mark_adsr_params_dirty(dirty);
}

static void apply_local_adsr2(const uint8_t* payload) {
  uint16_t dirty = 0, v;
  v = decode_u16_le(payload + 0); if (v != ADSR_VCF_attack)  { ADSR_VCF_attack = v; dirty |= ADSR_DIRTY_VCF_A; }
  v = decode_u16_le(payload + 2); if (v != ADSR_VCF_decay)   { ADSR_VCF_decay = v; dirty |= ADSR_DIRTY_VCF_D; }
  v = decode_u16_le(payload + 4); if (v != ADSR_VCF_sustain) { ADSR_VCF_sustain = v; dirty |= ADSR_DIRTY_VCF_S; }
  v = decode_u16_le(payload + 6); if (v != ADSR_VCF_release) { ADSR_VCF_release = v; dirty |= ADSR_DIRTY_VCF_R; }
  if (dirty) mark_adsr_params_dirty(dirty);
}

static void apply_local_adsr3(const uint8_t* payload) {
  uint16_t dirty = 0, v;
  v = decode_u16_le(payload + 0); if (v != ADSR1_attack)  { ADSR1_attack = v; dirty |= ADSR_DIRTY_DCO_A; }
  v = decode_u16_le(payload + 2); if (v != ADSR1_decay)   { ADSR1_decay = v; dirty |= ADSR_DIRTY_DCO_D; }
  v = decode_u16_le(payload + 4); if (v != ADSR1_sustain) { ADSR1_sustain = v; dirty |= ADSR_DIRTY_DCO_S; }
  v = decode_u16_le(payload + 6); if (v != ADSR1_release) { ADSR1_release = v; dirty |= ADSR_DIRTY_DCO_R; }
  if (dirty) mark_adsr_params_dirty(dirty);
}

static void apply_local_filter_block(const uint8_t* payload) {
  CUTOFF     = decode_u16_le(payload + 0);
  RESONANCE  = decode_u16_le(payload + 2);
  ADSR2toVCF = decode_i16_le(payload + 4);
  LFO2toVCF  = decode_u16_le(payload + 6);
  cv_bake_adsr2_to_vcf_scale();
  cv_bake_lfo2_to_vcf_scale();
}

// =============================================================================
// 3. Serial8 Handlers (From Input Controller)
// =============================================================================

static void input8_handle_adsr1(char, const uint8_t* payload, uint8_t len) {
  apply_local_adsr1(payload);
  relay_to_dco(CMD_ADSR1_BLOCK, payload, len);
}

static void input8_handle_adsr2(char, const uint8_t* payload, uint8_t len) {
  apply_local_adsr2(payload);
  relay_to_dco(CMD_ADSR2_BLOCK, payload, len);
}

static void input8_handle_adsr3(char, const uint8_t* payload, uint8_t len) {
  apply_local_adsr3(payload);
  relay_to_dco(CMD_ADSR3_BLOCK, payload, len);
}

static void input8_handle_filter_block(char, const uint8_t* payload, uint8_t len) {
  apply_local_filter_block(payload);
  relay_to_dco(CMD_FILTER_BLOCK, payload, len);
}

static void input8_handle_param16(char, const uint8_t* payload, uint8_t) {
  ParamFrame frame;
  decode_param_p(payload, frame);
  update_parameters((uint8_t)frame.id, (int16_t)frame.value);
}

static void input8_handle_preset_name(char, const uint8_t* payload, uint8_t len) {
  for (uint8_t i = 0; i < len; i++) presetName[i] = payload[i];
  relay_to_dco(CMD_PRESET_NAME, payload, len);
}

static void input8_handle_preset_dir_request(char, const uint8_t* payload, uint8_t len) {
  relay_to_dco(CMD_PRESET_DIR_REQUEST, payload, len);
}

// =============================================================================
// 4. Serial2 Handlers (From DCO)
// =============================================================================

static void main_handle_note_on(char, const uint8_t* payload, uint8_t) {
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

static void main_handle_note_off(char, const uint8_t* payload, uint8_t) {
  uint8_t voice_n = payload[0];
  if (voice_n >= NUM_VOICES) return;
  noteEnd[voice_n] = 1;
  noteStart[voice_n] = 0;
}

static void main_handle_expression(char, const uint8_t* payload, uint8_t) {
  aftertouch = payload[0];
  mod_wheel_in = payload[1];
  midi_pitch_bend = decode_u16_le(payload + 2);
  mod_matrix_set_aftertouch(payload[0]);
  mod_matrix_set_mod_wheel(payload[1]);
}

static void main_handle_mod_stream(char, const uint8_t*, uint8_t) {}

static void main_handle_adsr1(char, const uint8_t* payload, uint8_t len) {
  apply_local_adsr1(payload);
  relay_to_input(CMD_ADSR1_BLOCK, payload, len);
  forward_adsr_to_screen(CMD_ADSR1_BLOCK, payload);
}

static void main_handle_adsr2(char, const uint8_t* payload, uint8_t len) {
  apply_local_adsr2(payload);
  relay_to_input(CMD_ADSR2_BLOCK, payload, len);
  forward_adsr_to_screen(CMD_ADSR2_BLOCK, payload);
}

static void main_handle_adsr3(char, const uint8_t* payload, uint8_t len) {
  apply_local_adsr3(payload);
  relay_to_input(CMD_ADSR3_BLOCK, payload, len);
}

static void main_handle_filter_block(char, const uint8_t* payload, uint8_t len) {
  apply_local_filter_block(payload);
  relay_to_input(CMD_FILTER_BLOCK, payload, len);
  forward_filter_ui_to_screen(payload);
}

// Domain Block Ingress Handlers
static void main_handle_patch_osc_block(char, const uint8_t* payload, uint8_t len) {
  const PatchOscBlock* blk = (const PatchOscBlock*)payload;
  OSC1Interval = blk->osc1_interval;
  OSC2Interval = blk->osc2_interval;
  OSC2Detune   = blk->osc2_detune;
  voiceMode    = blk->voice_mode;
  analogDrift  = blk->analog_drift;

  relay_to_input(CMD_BLOCK_OSC, payload, len);
  relay_to_screen(CMD_BLOCK_OSC, payload, len);
}

static void main_handle_patch_lfo_block(char, const uint8_t* payload, uint8_t len) {
  const PatchLfoBlock* blk = (const PatchLfoBlock*)payload;
  LFO1Waveform = blk->lfo1_waveform;
  LFO2Waveform = blk->lfo2_waveform;
  LFO1SpeedVal = blk->lfo1_speed;
  LFO2SpeedVal = blk->lfo2_speed;
  LFO1_class.setWaveForm(LFO1Waveform);
  LFO2_class.setWaveForm(LFO2Waveform);
  LFO1_class.setMode0Freq((float)expConverterFloat(LFO1SpeedVal, 5000), micros());
  LFO2_class.setMode0Freq((float)expConverterFloat(LFO2SpeedVal, 5000), micros());

  relay_to_input(CMD_BLOCK_LFO, payload, len);
  relay_to_screen(CMD_BLOCK_LFO, payload, len);
}

static void main_handle_patch_mod_block(char, const uint8_t* payload, uint8_t len) {
  const PatchModBlock* blk = (const PatchModBlock*)payload;
  for (uint8_t i = 0; i < 8; i++) {
    mod_matrix_set_source(i, blk->slots[i].src);
    mod_matrix_set_dest(i, blk->slots[i].dest);
    mod_matrix_set_depth(i, blk->slots[i].depth);
  }

  relay_to_input(CMD_BLOCK_MOD, payload, len);
  relay_to_screen(CMD_BLOCK_MOD, payload, len);
}

static void main_handle_param16(char, const uint8_t* payload, uint8_t) {
  ParamFrame frame;
  decode_param_p(payload, frame);
  update_parameters((uint8_t)frame.id, (int16_t)frame.value);
  if (frame.id != PARAM_DEBUG_COMMAND) {
    serialSendParam16ToInput(frame.id, (int16_t)frame.value);
    serialSendParam16ToScreen(frame.id, (int16_t)frame.value);
  }
}

static void main_handle_param32(char, const uint8_t* payload, uint8_t) {
  ParamFrame frame;
  decode_param_x(payload, frame);
  if (frame.id == PARAM_GAP_FROM_DCO) {
    serialSendParam32ToInput(PARAM_GAP_FROM_DCO, (uint32_t)frame.value);
    serialSendParam32ToScreen(PARAM_GAP_FROM_DCO, (uint32_t)frame.value);
  } else if (frame.id == PARAM_MANUAL_CALIBRATION_OFFSET_FROM_DCO) {
    serialSendParam32ToInput(PARAM_MANUAL_CALIBRATION_OFFSET_FROM_DCO, (uint32_t)frame.value);
  }
}

static void main_handle_preset_dir_entry(char, const uint8_t* payload, uint8_t len) {
  relay_to_input(CMD_PRESET_DIR_ENTRY, payload, len);
}

static void main_handle_preset_loaded(char, const uint8_t* payload, uint8_t len) {
  relay_to_input(CMD_PRESET_LOADED, payload, len);
}

static void main_handle_screen_signal(char, const uint8_t* payload, uint8_t len) {
  relay_to_screen(CMD_SCREEN_SIGNAL, payload, len);
}

static void main_handle_preset_scroll(char, const uint8_t* payload, uint8_t len) {
  relay_to_screen(CMD_PRESET_NAME, payload, len);
}

// =============================================================================
// 5. Command Tables & Initialization
// =============================================================================

static const SerialCommandDef mainSerial2Commands[] = {
  { CMD_NOTE_ON,          SERIAL_LEN_NOTE_ON,              main_handle_note_on },
  { CMD_NOTE_OFF,         SERIAL_LEN_NOTE_OFF,             main_handle_note_off },
  { CMD_EXPRESSION,       SERIAL_LEN_EXPRESSION,           main_handle_expression },
  { CMD_MOD_STREAM,       SERIAL_LEN_MOD_STREAM,           main_handle_mod_stream },
  { CMD_PARAM_16,         SERIAL_LEN_PARAM_16,             main_handle_param16 },
  { CMD_PARAM_32,         SERIAL_LEN_PARAM_32,             main_handle_param32 },
  { CMD_ADSR1_BLOCK,      SERIAL_LEN_ADSR_BLOCK,           main_handle_adsr1 },
  { CMD_ADSR2_BLOCK,      SERIAL_LEN_ADSR_BLOCK,           main_handle_adsr2 },
  { CMD_ADSR3_BLOCK,      SERIAL_LEN_ADSR_BLOCK,           main_handle_adsr3 },
  { CMD_FILTER_BLOCK,     SERIAL_LEN_FILTER_BLOCK,         main_handle_filter_block },
  { CMD_BLOCK_OSC,        SERIAL_LEN_BLOCK_OSC,            main_handle_patch_osc_block }, // <-- REGISTERED!
  { CMD_BLOCK_LFO,        SERIAL_LEN_BLOCK_LFO,            main_handle_patch_lfo_block }, // <-- REGISTERED!
  { CMD_BLOCK_MOD,        SERIAL_LEN_BLOCK_MOD,            main_handle_patch_mod_block }, // <-- REGISTERED!
  { CMD_PRESET_DIR_ENTRY, SERIAL_LEN_PRESET_DIR_ENTRY,     main_handle_preset_dir_entry },
  { CMD_PRESET_LOADED,    SERIAL_LEN_PRESET_LOADED,        main_handle_preset_loaded },
  { CMD_SCREEN_SIGNAL,    SERIAL_LEN_SCREEN_SIGNAL,        main_handle_screen_signal },
  { CMD_PRESET_NAME,      SERIAL_LEN_SCREEN_PRESET_SCROLL, main_handle_preset_scroll },
};

static const SerialCommandDef inputSerial8Commands[] = {
  { CMD_ADSR1_BLOCK,        SERIAL_LEN_ADSR_BLOCK,         input8_handle_adsr1 },
  { CMD_ADSR2_BLOCK,        SERIAL_LEN_ADSR_BLOCK,         input8_handle_adsr2 },
  { CMD_ADSR3_BLOCK,        SERIAL_LEN_ADSR_BLOCK,         input8_handle_adsr3 },
  { CMD_FILTER_BLOCK,       SERIAL_LEN_FILTER_BLOCK,       input8_handle_filter_block },
  { CMD_PARAM_16,           SERIAL_LEN_PARAM_16,           input8_handle_param16 },
  { CMD_PRESET_NAME,        SERIAL_LEN_PRESET_NAME,        input8_handle_preset_name },
  { CMD_PRESET_DIR_REQUEST, SERIAL_LEN_PRESET_DIR_REQUEST, input8_handle_preset_dir_request },
};

void init_serial_parsers() {
  serial_parser_reset(mainSerial2Parser);
  serial_parser_reset(inputSerial8Parser);
  serial_command_table_init(mainSerial2Lut, mainSerial2Commands, sizeof(mainSerial2Commands) / sizeof(mainSerial2Commands[0]));
  serial_command_table_init(inputSerial8Lut, inputSerial8Commands, sizeof(inputSerial8Commands) / sizeof(inputSerial8Commands[0]));
}

void serial_dma_init() {
  __HAL_RCC_DMA1_CLK_ENABLE();
#ifdef ENABLE_SERIAL1
  serial_dma_init_stm32(0, Serial1.getHandle()->Instance, DMA1_Stream0, DMA_REQUEST_USART1_TX);
#endif
#ifdef ENABLE_SERIAL2
  serial_dma_init_stm32(1, Serial2.getHandle()->Instance, DMA1_Stream1, DMA_REQUEST_USART2_TX);
#endif
#ifdef ENABLE_SERIAL8
  serial_dma_init_stm32(2, Serial8.getHandle()->Instance, DMA1_Stream2, DMA_REQUEST_UART8_TX);
#endif
}

void serial_dma_poll() {
  serial_dma_poll_one(0);
  serial_dma_poll_one(1);
  serial_dma_poll_one(2);
}

inline void read_serial_1() {
#ifdef ENABLE_SERIAL1
  while (Serial1.available() > 0) (void)Serial1.read();
#endif
}

inline void read_serial_2() {
#ifdef ENABLE_SERIAL2
  serial_parser_drain(mainSerial2Parser, mainSerial2Lut, Serial2, SERIAL_DRAIN_BYTE_BUDGET);
#endif
}

inline void read_serial_8() {
#ifdef ENABLE_SERIAL8
  serial_parser_drain(inputSerial8Parser, inputSerial8Lut, Serial8, SERIAL_DRAIN_BYTE_BUDGET);
#endif
}