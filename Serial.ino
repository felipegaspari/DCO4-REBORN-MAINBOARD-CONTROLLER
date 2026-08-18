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
// 3. Serial8 Ingress Handlers (From Input Controller)
// =============================================================================

// Live exponential ADSR faders from panel -> apply locally and forward to DCO only.
// (Input Controller already sends linear bar graph frames directly to Screen).
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

static inline bool is_live_analog_stream(uint8_t id) {
  return (id == (uint8_t)PARAM_PW_VALUE || 
          id == (uint8_t)PARAM_ADSR1_TO_VCA || 
          id == (uint8_t)PARAM_VCA_LEVEL);
}

static void input8_handle_param16(char, const uint8_t* payload, uint8_t len) {
  ParamFrame frame;
  decode_param_p(payload, frame);

  // 1. Always update local Mainboard analog hardware
  update_parameters((uint8_t)frame.id, (int16_t)frame.value);

  // 2. Always forward to DCO audio engine
  relay_to_dco(CMD_PARAM_16, payload, len);

  // 3. Relay to Screen ONLY if it is a discrete parameter (never spam live pots)
  if (!is_live_analog_stream((uint8_t)frame.id)) {
    relay_to_screen(CMD_PARAM_16, payload, len);
  }
}

static void input8_handle_preset_name(char, const uint8_t* payload, uint8_t len) {
  for (uint8_t i = 0; i < len && i < 16; i++) presetName[i] = payload[i];
  relay_to_dco(CMD_PRESET_NAME, payload, len);
}

static void input8_handle_preset_dir_request(char, const uint8_t* payload, uint8_t len) {
  relay_to_dco(CMD_PRESET_DIR_REQUEST, payload, len);
}

// =============================================================================
// 4. Serial2 Ingress Handlers (From DCO Engine / Preset Recall / MIDI CC)
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

// =============================================================================
// Domain Block Ingress from DCO (All Routed via update_parameters)
// =============================================================================

// 1. Oscillator & Voice Configuration Block ('v')
static void main_handle_patch_osc_block(char, const uint8_t* payload, uint8_t len) {
  const PatchOscBlock* blk = (const PatchOscBlock*)payload;

  // Relay to Input Controller and Screen
  relay_to_input(CMD_BLOCK_OSC, payload, len);
  relay_to_screen(CMD_BLOCK_OSC, payload, len);

  // Wave switches (Bitmask unpacking)
  update_parameters(PARAM_OSC1_SAW_ENABLE,   (blk->wave_enables & (1u << 0)) != 0);
  update_parameters(PARAM_OSC1_PULSE_ENABLE, (blk->wave_enables & (1u << 1)) != 0);
  update_parameters(PARAM_OSC1_TRI_ENABLE,   (blk->wave_enables & (1u << 2)) != 0);
  update_parameters(PARAM_OSC2_SAW_ENABLE,   (blk->wave_enables & (1u << 3)) != 0);
  update_parameters(PARAM_OSC2_PULSE_ENABLE, (blk->wave_enables & (1u << 4)) != 0);
  update_parameters(PARAM_OSC2_TRI_ENABLE,   (blk->wave_enables & (1u << 5)) != 0);
  update_parameters(PARAM_OSC3_SAW_ENABLE,   (blk->wave_enables & (1u << 6)) != 0);
  update_parameters(PARAM_OSC3_PULSE_ENABLE, (blk->wave_enables & (1u << 7)) != 0);
  update_parameters(PARAM_OSC3_TRI_ENABLE,   (blk->wave_enables & (1u << 8)) != 0);

  // Intervals, Detunes & Modes
  update_parameters(PARAM_OSC1_INTERVAL,       blk->osc1_interval);
  update_parameters(PARAM_OSC2_INTERVAL,       blk->osc2_interval);
  update_parameters(PARAM_OSC3_INTERVAL,       blk->osc3_interval);
  update_parameters(PARAM_OSC2_DETUNE_VAL,     blk->osc2_detune);
  update_parameters(PARAM_UNISON_DETUNE,       blk->unison_detune);
  update_parameters(PARAM_VOICE_MODE,          blk->voice_mode);
  update_parameters(PARAM_VOICE_ALLOC_MODE,    blk->voice_alloc_mode);
  update_parameters(PARAM_SYNC_MODE,           blk->sync_mode);
  update_parameters(PARAM_SOFT_SYNC,           blk->soft_sync);
  update_parameters(PARAM_SUBOSC_DIVIDE,       blk->subosc_divide);
  update_parameters(PARAM_ANALOG_DRIFT_AMOUNT, blk->analog_drift);
  update_parameters(PARAM_ANALOG_DRIFT_SPEED,  blk->analog_drift_speed);
  update_parameters(PARAM_ANALOG_DRIFT_SPREAD, blk->analog_drift_spread);
  update_parameters(PARAM_PORTAMENTO_TIME,     blk->portamento_time);
  update_parameters(PARAM_PORTAMENTO_MODE,     blk->portamento_mode);
  update_parameters(PARAM_CHARACTER,           blk->character);

  // Ensure full hardware analog switch refresh
  update_waveSelector(4);
}

// 2. LFO & Modulation Block ('l')
static void main_handle_patch_lfo_block(char, const uint8_t* payload, uint8_t len) {
  const PatchLfoBlock* blk = (const PatchLfoBlock*)payload;

  // Relay to Input Controller and Screen
  relay_to_input(CMD_BLOCK_LFO, payload, len);
  relay_to_screen(CMD_BLOCK_LFO, payload, len);

  update_parameters(PARAM_LFO1_WAVEFORM,        blk->lfo1_waveform);
  update_parameters(PARAM_LFO2_WAVEFORM,        blk->lfo2_waveform);
  update_parameters(PARAM_LFO1_SPEED,           blk->lfo1_speed);
  update_parameters(PARAM_LFO2_SPEED,           blk->lfo2_speed);
  update_parameters(PARAM_LFO1_TO_DCO,          blk->lfo1_to_dco);
  update_parameters(PARAM_LFO1_TO_OSC1,         blk->lfo1_to_osc1);
  update_parameters(PARAM_LFO1_TO_OSC2,         blk->lfo1_to_osc2);
  update_parameters(PARAM_LFO1_TO_OSC3,         blk->lfo1_to_osc3);
  update_parameters(PARAM_LFO2_TO_OSC2,         blk->lfo2_to_osc2);
  update_parameters(PARAM_LFO2_TO_OSC3,         blk->lfo2_to_osc3);
  update_parameters(PARAM_LFO2_TO_OSC2_COARSE,  blk->lfo2_to_osc2_coarse);
  update_parameters(PARAM_LFO2_TO_OSC3_COARSE,  blk->lfo2_to_osc3_coarse);
  update_parameters(PARAM_LFO2_TO_PW,           blk->lfo2_to_pw);
  update_parameters(PARAM_LFO1_TO_VCA,          blk->lfo1_to_vca);
  update_parameters(PARAM_PW_VALUE,             blk->pw_value);
  update_parameters(PARAM_ADSR1_TO_VCA,         blk->adsr1_to_vca);
  update_parameters(PARAM_ADSR3_TO_PWM,         blk->adsr3_to_pwm); // Math handled in apply_param
  update_parameters(PARAM_ADSR3_TO_DETUNE1,     blk->adsr3_to_detune1);
  update_parameters(PARAM_ADSR3_PITCH_MODE,     blk->adsr3_pitch_mode);
  update_parameters(PARAM_ADSR3_TO_OSC_SELECT,  blk->adsr3_to_osc_select);
}

// 3. Mod Matrix Block ('M')
static void main_handle_patch_mod_block(char, const uint8_t* payload, uint8_t len) {
  const PatchModBlock* blk = (const PatchModBlock*)payload;

  // Relay to Input Controller and Screen
  relay_to_input(CMD_BLOCK_MOD, payload, len);
  relay_to_screen(CMD_BLOCK_MOD, payload, len);

  for (uint8_t i = 0; i < 8; i++) {
    update_parameters(PARAM_MOD_SLOT0_SOURCE + i * 3, blk->slots[i].src);
    update_parameters(PARAM_MOD_SLOT0_DEST   + i * 3, blk->slots[i].dest);
    update_parameters(PARAM_MOD_SLOT0_DEPTH  + i * 3, blk->slots[i].depth);
  }
}

// 4. Mixer, Curves, VCA & Filter Modes Block ('X')
static void main_handle_patch_mix_block(char, const uint8_t* payload, uint8_t len) {
  const PatchMixBlock* blk = (const PatchMixBlock*)payload;

  // Relay to Input Controller and Screen
  relay_to_input(CMD_BLOCK_MIX, payload, len);
  relay_to_screen(CMD_BLOCK_MIX, payload, len);

  update_parameters(PARAM_OSC1_LEVEL,          blk->osc1_level);
  update_parameters(PARAM_OSC2_LEVEL,          blk->osc2_level);
  update_parameters(PARAM_OSC3_LEVEL,          blk->osc3_level);
  update_parameters(PARAM_SUB_LEVEL,           blk->sub_level);
  update_parameters(PARAM_VCA_LEVEL,           blk->vca_level);
  update_parameters(PARAM_FILTER_MODE,         blk->filter_mode);
  update_parameters(PARAM_VELOCITY_TO_VCF,     blk->velocity_to_vcf);
  update_parameters(PARAM_VELOCITY_TO_VCA,     blk->velocity_to_vca);
  update_parameters(PARAM_VCF_KEYTRACK,        blk->vcf_keytrack);
  update_parameters(PARAM_ADSR1_TO_VCA,        blk->adsr1_to_vca);
  update_parameters(PARAM_DIST_DRIVE,          blk->dist_drive);
  update_parameters(PARAM_DIST_MIX,            blk->dist_mix);
  update_parameters(PARAM_ADSR1_ATTACK_CURVE,  blk->adsr1_attack_curve);
  update_parameters(PARAM_ADSR1_DECAY_CURVE,   blk->adsr1_decay_curve);
  update_parameters(PARAM_ADSR2_ATTACK_CURVE,  blk->adsr2_attack_curve);
  update_parameters(PARAM_ADSR2_DECAY_CURVE,   blk->adsr2_decay_curve);

  // Boolean flags
  update_parameters(PARAM_RESONANCE_COMPENSATION, (blk->misc_flags & (1 << 0)) != 0);
  update_parameters(PARAM_VCA_ADSR_RESTART,       (blk->misc_flags & (1 << 1)) != 0);
  update_parameters(PARAM_VCF_ADSR_RESTART,       (blk->misc_flags & (1 << 2)) != 0);
}

static void main_handle_param16(char, const uint8_t* payload, uint8_t len) {
  ParamFrame frame;
  decode_param_p(payload, frame);

  // 1. Update local Mainboard analog state (WITHOUT echoing back to DCO)
  update_parameters((uint8_t)frame.id, (int16_t)frame.value);

  // 2. Relay to Input and Screen
  if (frame.id != PARAM_DEBUG_COMMAND) {
    relay_to_input(CMD_PARAM_16, payload, len);
    relay_to_screen(CMD_PARAM_16, payload, len);
  }
}

static void main_handle_param32(char, const uint8_t* payload, uint8_t len) {
  // Relay all 32-bit calibration, gap, and offset frames from DCO to Input and Screen
  relay_to_input(CMD_PARAM_32, payload, len);
  relay_to_screen(CMD_PARAM_32, payload, len);
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

// Preset Scroll Ingress (17 bytes: [slot:u8][name:16 ASCII])
static void main_handle_preset_scroll(char, const uint8_t* payload, uint8_t len) {
  if (len >= 17) {
    for (uint8_t i = 0; i < 16; i++) presetName[i] = payload[1 + i];
  } else if (len == 16) {
    for (uint8_t i = 0; i < 16; i++) presetName[i] = payload[i];
  }

  // Relay to Screen for instant display
  relay_to_screen(CMD_PRESET_NAME, payload, len);

  // Relay to Input Controller so in-RAM presetName is 100% in sync!
  relay_to_input(CMD_PRESET_NAME, payload, len);
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
  { CMD_BLOCK_OSC,        SERIAL_LEN_BLOCK_OSC,            main_handle_patch_osc_block },
  { CMD_BLOCK_LFO,        SERIAL_LEN_BLOCK_LFO,            main_handle_patch_lfo_block },
  { CMD_BLOCK_MOD,        SERIAL_LEN_BLOCK_MOD,            main_handle_patch_mod_block },
  { CMD_PRESET_DIR_ENTRY, SERIAL_LEN_PRESET_DIR_ENTRY,     main_handle_preset_dir_entry },
  { CMD_PRESET_LOADED,    SERIAL_LEN_PRESET_LOADED,        main_handle_preset_loaded },
  { CMD_SCREEN_SIGNAL,    SERIAL_LEN_SCREEN_SIGNAL,        main_handle_screen_signal },
  { CMD_PRESET_NAME,      SERIAL_LEN_SCREEN_PRESET_SCROLL, main_handle_preset_scroll },
  { CMD_BLOCK_MIX,        SERIAL_LEN_BLOCK_MIX,            main_handle_patch_mix_block },
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