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
  const AdsrBlock* in = (const AdsrBlock*)payload;
  AdsrBlock screen_payload = {
    exp_to_lin_index(in->attack),
    exp_to_lin_index(in->decay),
    in->sustain,
    exp_to_lin_index(in->release)
  };
  relay_to_screen(cmd, (const uint8_t*)&screen_payload, sizeof(AdsrBlock));
#endif
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
  
  // =============================================================================
  // 2. Local Hardware Apply Helpers
  // =============================================================================
  
  static void apply_local_adsr1(const uint8_t* payload) {
    const AdsrBlock* blk = (const AdsrBlock*)payload;
    uint16_t dirty = 0;
    if (blk->attack  != ADSR_VCA_attack)  { ADSR_VCA_attack  = blk->attack;  dirty |= ADSR_DIRTY_VCA_A; }
    if (blk->decay   != ADSR_VCA_decay)   { ADSR_VCA_decay   = blk->decay;   dirty |= ADSR_DIRTY_VCA_D; }
    if (blk->sustain != ADSR_VCA_sustain) { ADSR_VCA_sustain = blk->sustain; dirty |= ADSR_DIRTY_VCA_S; }
    if (blk->release != ADSR_VCA_release) { ADSR_VCA_release = blk->release; dirty |= ADSR_DIRTY_VCA_R; }
    if (dirty) mark_adsr_params_dirty(dirty);
  }
  
  static void apply_local_adsr2(const uint8_t* payload) {
    const AdsrBlock* blk = (const AdsrBlock*)payload;
    uint16_t dirty = 0;
    if (blk->attack  != ADSR_VCF_attack)  { ADSR_VCF_attack  = blk->attack;  dirty |= ADSR_DIRTY_VCF_A; }
    if (blk->decay   != ADSR_VCF_decay)   { ADSR_VCF_decay   = blk->decay;   dirty |= ADSR_DIRTY_VCF_D; }
    if (blk->sustain != ADSR_VCF_sustain) { ADSR_VCF_sustain = blk->sustain; dirty |= ADSR_DIRTY_VCF_S; }
    if (blk->release != ADSR_VCF_release) { ADSR_VCF_release = blk->release; dirty |= ADSR_DIRTY_VCF_R; }
    if (dirty) mark_adsr_params_dirty(dirty);
  }
  
  static void apply_local_adsr3(const uint8_t* payload) {
    const AdsrBlock* blk = (const AdsrBlock*)payload;
    uint16_t dirty = 0;
    if (blk->attack  != ADSR1_attack)  { ADSR1_attack  = blk->attack;  dirty |= ADSR_DIRTY_DCO_A; }
    if (blk->decay   != ADSR1_decay)   { ADSR1_decay   = blk->decay;   dirty |= ADSR_DIRTY_DCO_D; }
    if (blk->sustain != ADSR1_sustain) { ADSR1_sustain = blk->sustain; dirty |= ADSR_DIRTY_DCO_S; }
    if (blk->release != ADSR1_release) { ADSR1_release = blk->release; dirty |= ADSR_DIRTY_DCO_R; }
    if (dirty) mark_adsr_params_dirty(dirty);
  }
  
  static void apply_local_filter_block(const uint8_t* payload) {
    const FilterBlock* blk = (const FilterBlock*)payload;
    CUTOFF     = blk->cutoff;
    RESONANCE  = blk->resonance;
    ADSR2toVCF = blk->env2_to_vcf;
    LFO2toVCF  = blk->lfo2_to_vcf;
    cv_bake_adsr2_to_vcf_scale();
    cv_bake_lfo2_to_vcf_scale();
  }
// =============================================================================
// 3. Serial8 Ingress Handlers (From Input Controller)
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

static inline bool is_live_analog_stream(uint8_t id) {
  return (id == (uint8_t)PARAM_PW_VALUE || 
          id == (uint8_t)PARAM_ADSR1_TO_VCA || 
          id == (uint8_t)PARAM_VCA_LEVEL);
}

static void input8_handle_param16(char, const uint8_t* payload, uint8_t len) {
  ParamFrame frame;
  decode_param_p(payload, frame);

  // 1. Update local Mainboard analog hardware
  update_parameters((uint8_t)frame.id, (int16_t)frame.value);

  // 2. Forward to DCO
  relay_to_dco(CMD_PARAM_16, payload, len);

  // 3. Relay discrete params to Screen
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
  relay_to_screen(CMD_FILTER_BLOCK, payload, len);
}

// =============================================================================
// 4. Serial2 Ingress Handlers (From DCO Engine / Preset Recall / MIDI CC)
// =============================================================================

// 1. Oscillator & Voice Configuration Block ('v')
static void main_handle_patch_osc_block(char, const uint8_t* payload, uint8_t len) {
  const PatchOscBlock* blk = (const PatchOscBlock*)payload;

  // Relay immediately to downstream controllers
  relay_to_input(CMD_BLOCK_OSC, payload, len);
  relay_to_screen(CMD_BLOCK_OSC, payload, len);

  // --- Wave enables (DG411 / 74HC595) ---
  apply_param_osc1_saw_enable((blk->wave_enables & (1u << 0)) != 0);
  apply_param_osc1_pulse_enable((blk->wave_enables & (1u << 1)) != 0);
  apply_param_osc1_tri_enable((blk->wave_enables & (1u << 2)) != 0);
  apply_param_osc2_saw_enable((blk->wave_enables & (1u << 3)) != 0);
  apply_param_osc2_pulse_enable((blk->wave_enables & (1u << 4)) != 0);
  apply_param_osc2_tri_enable((blk->wave_enables & (1u << 5)) != 0);
  // (blk->wave_enables & (1u << 6)) // OSC3 Saw (DCO3 monosynth only)
  // (blk->wave_enables & (1u << 7)) // OSC3 Pulse (DCO3 monosynth only)
  // (blk->wave_enables & (1u << 8)) // OSC3 Tri (DCO3 monosynth only)
  update_waveSelector(4); // Latch entire 74HC595 register once

  // --- Pitch, Intervals & Voice ---
  apply_param_osc1_interval(blk->osc1_interval);
  apply_param_osc2_interval(blk->osc2_interval);
  // apply_param_osc3_interval(blk->osc3_interval); // DCO3 monosynth only (apply_noop on DCO4 MB)
  apply_param_osc2_detune(blk->osc2_detune);
  apply_param_unison_detune(blk->unison_detune);
  apply_param_voice_mode(blk->voice_mode);
  // apply_param_voice_alloc_mode(blk->voice_alloc_mode); // DCO-only voice allocation policy
  // apply_param_phase_align(blk->phase_align); // TODO: Uncomment this when phase align is implemented
  // apply_param_soft_sync(blk->soft_sync); // DCO-only PIO sync
  // apply_param_subosc_divide(blk->subosc_divide); // DCO-only PIO suboscillator

  // --- Analog Drift & Portamento ---
  apply_param_analog_drift_amount(blk->analog_drift);
  apply_param_analog_drift_speed(blk->analog_drift_speed);
  apply_param_analog_drift_spread(blk->analog_drift_spread);
  apply_param_portamento_time(blk->portamento_time);
  apply_param_portamento_mode(blk->portamento_mode);
  // apply_param_character(blk->character); // DCO-only character engine
}

// 2. LFO & Modulation Block ('l')
static void main_handle_patch_lfo_block(char, const uint8_t* payload, uint8_t len) {
  const PatchLfoBlock* blk = (const PatchLfoBlock*)payload;

  relay_to_input(CMD_BLOCK_LFO, payload, len);
  relay_to_screen(CMD_BLOCK_LFO, payload, len);

  // --- Speeds first, then Waveforms (ensures valid frequency calculation) ---
  apply_param_lfo1_speed(blk->lfo1_speed);
  apply_param_lfo1_waveform(blk->lfo1_waveform);
  apply_param_lfo2_speed(blk->lfo2_speed);
  apply_param_lfo2_waveform(blk->lfo2_waveform);

  // --- LFO Routings ---
  apply_param_lfo1_to_dco(blk->lfo1_to_dco);
  // apply_param_lfo1_to_osc1(blk->lfo1_to_osc1); // DCO-only additive pitch
  // apply_param_lfo1_to_osc2(blk->lfo1_to_osc2); // DCO-only additive pitch
  // apply_param_lfo1_to_osc3(blk->lfo1_to_osc3); // DCO-only additive pitch
  // apply_param_lfo2_to_osc2(blk->lfo2_to_osc2); // DCO-only pitch bus
  // apply_param_lfo2_to_osc3(blk->lfo2_to_osc3); // DCO-only pitch bus
  // apply_param_lfo2_to_osc2_coarse(blk->lfo2_to_osc2_coarse); // DCO-only coarse pitch
  // apply_param_lfo2_to_osc3_coarse(blk->lfo2_to_osc3_coarse); // DCO-only coarse pitch
  apply_param_lfo2_to_pw(blk->lfo2_to_pw);
  apply_param_lfo1_to_vca(blk->lfo1_to_vca);

  // --- Pulse Width & Envelopes ---
  apply_param_pw_value(blk->pw_value);
  apply_param_adsr1_to_vca(blk->adsr1_to_vca);
  apply_param_adsr3_to_pwm(blk->adsr3_to_pwm);
  apply_param_adsr3_to_detune1(blk->adsr3_to_detune1);
  apply_param_adsr3_pitch_mode(blk->adsr3_pitch_mode);
  apply_param_adsr3_to_osc_select(blk->adsr3_to_osc_select);
}

// 3. Mod Matrix Block ('M')
static void main_handle_patch_mod_block(char, const uint8_t* payload, uint8_t len) {
  const PatchModBlock* blk = (const PatchModBlock*)payload;

  relay_to_input(CMD_BLOCK_MOD, payload, len);
  relay_to_screen(CMD_BLOCK_MOD, payload, len);

  // Apply all 8 slots directly to Mainboard mod matrix engine
  for (uint8_t i = 0; i < 8; i++) {
    mod_matrix_set_source(i, blk->slots[i].src);
    mod_matrix_set_dest(i,   blk->slots[i].dest);
    mod_matrix_set_depth(i,  blk->slots[i].depth);
  }
}

static void main_handle_patch_mix_block(char, const uint8_t* payload, uint8_t len) {
  const PatchMixBlock* blk = (const PatchMixBlock*)payload;

  relay_to_input(CMD_BLOCK_MIX, payload, len);
  relay_to_screen(CMD_BLOCK_MIX, payload, len);

  // --- Mixer Levels ---
  apply_param_osc1_level(blk->osc1_level);
  apply_param_osc2_level(blk->osc2_level);
  apply_param_sub_level(blk->sub_level);
  apply_param_vca_level(blk->vca_level);
  apply_param_filter_mode(blk->filter_mode);

  // --- Dynamics & Keytrack ---
  apply_param_velocity_to_vcf(blk->velocity_to_vcf);
  apply_param_velocity_to_vca(blk->velocity_to_vca);
  apply_param_vcf_keytrack(blk->vcf_keytrack);
  apply_param_adsr1_to_vca(blk->adsr1_to_vca);
  apply_param_dist_drive(blk->dist_drive);
  apply_param_dist_mix(blk->dist_mix);

  // --- Envelope Curve Shaping (Apply All Curves) ---
  apply_param_adsr1_attack_curve(blk->adsr1_attack_curve);
  apply_param_adsr1_decay_curve(blk->adsr1_decay_curve);
  apply_param_adsr1_release_curve(blk->adsr1_release_curve); 
  apply_param_adsr2_attack_curve(blk->adsr2_attack_curve);
  apply_param_adsr2_decay_curve(blk->adsr2_decay_curve);
  apply_param_adsr2_release_curve(blk->adsr2_release_curve); 
  apply_param_adsr3_attack_curve(blk->adsr3_attack_curve);   
  apply_param_adsr3_decay_curve(blk->adsr3_decay_curve);     
  apply_param_adsr3_release_curve(blk->adsr3_release_curve); 

  apply_param_vcf_trigger_mode(blk->vcf_trigger_mode);       

  // --- Boolean Switches & Restarts ---
  apply_param_resonance_comp((blk->misc_flags & (1 << 0)) != 0);
  apply_param_vca_adsr_restart((blk->misc_flags & (1 << 1)) != 0);
  apply_param_vcf_adsr_restart((blk->misc_flags & (1 << 2)) != 0);
}

static void main_handle_param16(char, const uint8_t* payload, uint8_t len) {
  ParamFrame frame;
  decode_param_p(payload, frame);

  update_parameters((uint8_t)frame.id, (int16_t)frame.value);

  if (frame.id != PARAM_DEBUG_COMMAND) {
    relay_to_input(CMD_PARAM_16, payload, len);
    relay_to_screen(CMD_PARAM_16, payload, len);
  }
}

static void main_handle_param32(char, const uint8_t* payload, uint8_t len) {
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

  //////////// PRESET DEBUG PRINT ALL ////////////
  // if (payload[0] == 1) {
  //   mb_debug_print_all();
  // }
  //////////// PRESET DEBUG PRINT ALL ////////////

  relay_to_screen(CMD_SCREEN_SIGNAL, payload, len);
  relay_to_input(CMD_SCREEN_SIGNAL, payload, len);
}

static void main_handle_preset_scroll(char, const uint8_t* payload, uint8_t len) {
  if (len >= 17) {
    for (uint8_t i = 0; i < 16; i++) presetName[i] = payload[1 + i];
  } else if (len == 16) {
    for (uint8_t i = 0; i < 16; i++) presetName[i] = payload[i];
  }

  relay_to_screen(CMD_PRESET_NAME, payload, len);
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
#ifdef __HAL_RCC_DMA2_CLK_ENABLE
  __HAL_RCC_DMA2_CLK_ENABLE();
#endif

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
  serial_parser_drain(mainSerial2Parser, mainSerial2Lut, Serial2, 255);
#endif
}

inline void read_serial_8() {
#ifdef ENABLE_SERIAL8
  serial_parser_drain(inputSerial8Parser, inputSerial8Lut, Serial8, 255);
#endif
}


/// PRESET DEBUG PRINT ALL ////////////
// PRESET DEBUG PRINT ALL ////////////
// PRESET DEBUG PRINT ALL ////////////
// void mb_debug_print_all() {
//   Serial.println(F("\n================================================================="));
//   Serial.println(F(" MAINBOARD HARDWARE DUMP (AFTER PRESET RECALL)                   "));
//   Serial.println(F("================================================================="));

//   char nameBuf[17];
//   memcpy(nameBuf, presetName, 16);
//   nameBuf[16] = '\0';
//   Serial.printf(" Name (Relayed): \"%s\"\n", nameBuf);

//   // =========================================================================
//   // 1. Envelopes & Filter
//   // =========================================================================
//   Serial.println(F("\n--- [ ENVELOPES & FILTER (LOCAL STM32 STATE) ] ---"));
//   Serial.printf(" EnvVCA (ADSR): A=%-5u D=%-5u S=%-5u R=%-5u\n",
//                 ADSR_VCA_attack, ADSR_VCA_decay, ADSR_VCA_sustain, ADSR_VCA_release);
//   Serial.printf(" EnvVCF (ADSR): A=%-5u D=%-5u S=%-5u R=%-5u\n",
//                 ADSR_VCF_attack, ADSR_VCF_decay, ADSR_VCF_sustain, ADSR_VCF_release);
//   Serial.printf(" EnvDCO (ADSR): A=%-5u D=%-5u S=%-5u R=%-5u\n",
//                 ADSR1_attack, ADSR1_decay, ADSR1_sustain, ADSR1_release);
//   Serial.printf(" Filter:        Cutoff=%-5u Reso=%-5u Env2toVCF=%-5d LFO2toVCF=%-5u\n",
//                 CUTOFF, RESONANCE, (int16_t)ADSR2toVCF, LFO2toVCF);

//   // =========================================================================
//   // 2. Oscillators & Voice Modes
//   // =========================================================================
//   Serial.println(F("\n--- [ OSCILLATORS & VOICE ] ---"));
//   Serial.printf(" OSC1: Saw=%d Pulse=%d Tri=%d | Interval=%u\n",
//                 (int)osc1SawEnable, (int)osc1PulseEnable, (int)osc1TriEnable, (unsigned)OSC1Interval);
//   Serial.printf(" OSC2: Saw=%d Pulse=%d Tri=%d | Interval=%u Detune=%u\n",
//                 (int)osc2SawEnable, (int)osc2PulseEnable, (int)osc2TriEnable, (unsigned)OSC2Interval, (unsigned)OSC2Detune);
//   Serial.printf(" Voice: Mode=%u UnisonDetune=%d SyncMode=%u\n",
//                 (unsigned)voiceMode, unisonDetune, (unsigned)oscPhaseSync);
//   Serial.printf(" Portamento: Time=%u Mode=%u | PW=%u\n",
//                 (unsigned)portamentoTime, (unsigned)portamentoMode, (unsigned)PW);
//   Serial.printf(" Drift: Amount=%d Speed=%d Spread=%d\n",
//                 analogDrift, analogDriftSpeed, analogDriftSpread);

//   // =========================================================================
//   // 3. LFOs & Routing
//   // =========================================================================
//   Serial.println(F("\n--- [ LFOS ] ---"));
//   Serial.printf(" LFO1: Wave=%u SpeedVal=%-5u Speed=%-5d LFO1toDCO=%-5u LFO1toVCA=%-5u\n",
//                 (unsigned)LFO1Waveform, (unsigned)LFO1SpeedVal, (int)LFO1Speed, (unsigned)LFO1toDCOVal, (unsigned)LFO1toVCA);
//   Serial.printf(" LFO2: Wave=%u SpeedVal=%-5u Speed=%-5d LFO2toPW=%-5u\n",
//                 (unsigned)LFO2Waveform, (unsigned)LFO2SpeedVal, (int)LFO2Speed, (unsigned)LFO2toPW);
//   Serial.printf(" Env Mod: ADSR3toPWM=%-5d ADSR3toDETUNE1=%-5d PitchCentered=%d ADSR3ToOscSelect=%d\n",
//                 ADSR1toPWM, ADSR1toDETUNE1, (int)env_dco_pitch_centered, (int)ADSR3ToOscSelect);

//   // =========================================================================
//   // 4. Mixer Levels & Analog Hardware CVs
//   // =========================================================================
//   Serial.println(F("\n--- [ MIXER LEVELS & ANALOG CV ] ---"));
//   Serial.printf(" Levels (Raw): OSC1=%-3u OSC2=%-3u SUB=%-3u VCA_Level=%-5u\n",
//                 OSC1LevelVal, OSC2LevelVal, SubLevelVal, VCALevel);
//   Serial.printf(" Levels (DAC): OSC1=%-4u OSC2=%-4u SUB=%-4u\n",
//                 OSC1Level, OSC2Level, SubLevel);
//   Serial.printf(" Dynamics: Keytrack=%-5d VelToVCF=%-3d VelToVCA=%-3d EnvToVCA=%-5d\n",
//                 VCFKeytrack, (int)velocityToVCFVal, (int)velocityToVCAVal, ADSR1toVCA);
//   Serial.printf(" Curves: VCA_Atk=%u VCA_Dec=%u VCF_Atk=%u VCF_Dec=%u\n",
//                 (unsigned)ADSR1AttackCurveVal, (unsigned)ADSR1DecayCurveVal, (unsigned)ADSR2AttackCurveVal, (unsigned)ADSR2DecayCurveVal);
//   Serial.printf(" Distortion: Drive=%-5u Mix=%-5u | Switches: ResComp=%d VCA_Rst=%d VCF_Rst=%d\n",
//                 DIST_DRIVE, DIST_MIX, (int)RESONANCEAmpCompensation, (int)VCAADSRRestart, (int)VCFADSRRestart);

//   // =========================================================================
//   // 5. Modulation Matrix
//   // =========================================================================
//   Serial.println(F("\n--- [ MODULATION MATRIX (SLOTS 0..7) ] ---"));
//   for (uint8_t i = 0; i < 8; ++i) {
//     // If you have direct getters or slot structs:
//     // extern int16_t mod_matrix_get_depth(uint8_t slot);
//     // extern uint8_t mod_matrix_get_source(uint8_t slot);
//     // extern uint8_t mod_matrix_get_dest(uint8_t slot);
    
//     // uint8_t src = mod_matrix_get_source(i);
//     // uint8_t dst = mod_matrix_get_dest(i);
//     // int16_t dep = mod_matrix_get_depth(i);
//     // if (src != 0xFF && src != 0 && dep != 0) {
//     //   Serial.printf("  Slot %u: Source=%-3u -> Dest=%-3u [Depth=%-5d]\n", i, src, dst, dep);
//     // } else {
//     //   Serial.printf("  Slot %u: [OFF] (Src=%u Dest=%u Depth=%d)\n", i, src, dst, dep);
//     // }
//   }

//   Serial.println(F("=================================================================\n"));
// }