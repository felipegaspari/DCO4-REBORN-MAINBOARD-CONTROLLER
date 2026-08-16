// =============================================================================
// Mainboard Local Parameter Appliers (Pure Analog Hardware / Local State)
// =============================================================================

static void apply_param_osc1_saw_enable(int16_t v)   { osc1SawEnable = (v != 0); update_waveSelector(0); }
static void apply_param_osc1_pulse_enable(int16_t v) { osc1PulseEnable = (v != 0); }
static void apply_param_osc1_tri_enable(int16_t v)   { osc1TriEnable = (v != 0); update_waveSelector(2); }
static void apply_param_osc2_saw_enable(int16_t v)   { osc2SawEnable = (v != 0); update_waveSelector(1); }
static void apply_param_osc2_pulse_enable(int16_t v) { osc2PulseEnable = (v != 0); update_waveSelector(3); }
static void apply_param_osc2_tri_enable(int16_t v)   { osc2TriEnable = (v != 0); }
static void apply_param_sine_status(int16_t v)       { sineStatus = (v != 0); }

static void apply_param_resonance_comp(int16_t v)    { RESONANCEAmpCompensation = (v != 0); }
static void apply_param_vca_adsr_restart(int16_t v)  { VCAADSRRestart = (v != 0); ADSR_VCA_set_restart(); }
static void apply_param_vcf_adsr_restart(int16_t v)  { VCFADSRRestart = (v != 0); ADSR_VCF_set_restart(); }

static void apply_param_adsr3_to_osc_select(int16_t v) { ADSR3ToOscSelect = (int8_t)v; }

static void apply_param_lfo1_waveform(int16_t v) {
  LFO1Waveform = (uint8_t)v;
  LFO1_class.setWaveForm(LFO1Waveform);
  LFO1_class.setMode0Freq((float)LFO1Speed, micros());
}
static void apply_param_lfo2_waveform(int16_t v) {
  LFO2Waveform = (uint8_t)v;
  LFO2_class.setWaveForm(LFO2Waveform);
  LFO2_class.setMode0Freq((float)LFO2Speed, micros());
}

static void apply_param_osc1_interval(int16_t v)     { OSC1Interval = (uint8_t)v; }
static void apply_param_osc2_interval(int16_t v)     { OSC2Interval = (uint8_t)v; }
static void apply_param_osc2_detune(int16_t v)       { OSC2Detune = (uint16_t)v; }
static void apply_param_osc_sync_mode(int16_t v)     { oscSyncMode = (uint8_t)v; }
static void apply_param_portamento_time(int16_t v)   { portamentoTime = (uint8_t)v; }
static void apply_param_portamento_mode(int16_t v)   { portamentoMode = (uint8_t)v; }

static void apply_param_vcf_keytrack(int16_t v) {
  VCFKeytrack = v;
  VCFKeytrackModifier_q15 = (VCFKeytrack != 0) ? (((int32_t)VCFKeytrack * 32768) / 8000) : 32768;
}

static void apply_param_velocity_to_vcf(int16_t v) {
  velocityToVCFVal = (int8_t)v;
  velocityToVCF_q15 = ((int32_t)velocityToVCFVal * 825) >> 6;
}

static void apply_param_velocity_to_vca(int16_t v) {
  velocityToVCAVal = (int8_t)v;
  velocityToVCA_q15 = ((int32_t)velocityToVCAVal * 825) >> 6;
}

static void apply_param_osc1_level(int16_t v) {
  OSC1LevelVal = constrain(v, 0, 128);
  OSC1Level = lin_to_log_128[OSC1LevelVal];
  mcpUpdate();
}
static void apply_param_osc2_level(int16_t v) {
  OSC2LevelVal = constrain(v, 0, 128);
  OSC2Level = lin_to_log_128[OSC2LevelVal];
  mcpUpdate();
}
static void apply_param_sub_level(int16_t v) {
  SubLevelVal = v;
  SubLevel = (uint16_t)constrain((int)SubLevelVal * 32, 0, 4095);
  mcpUpdate();
}

static void apply_param_voice_mode(int16_t v)        { voiceMode = (uint8_t)v; }
static void apply_param_unison_detune(int16_t v)     { unisonDetune = v; }

static void apply_param_analog_drift_amount(int16_t v) {
  analogDrift = (int8_t)v;
  vcf_drift_scale_q15 = (int32_t)analogDrift;
}
static void apply_param_analog_drift_speed(int16_t v) {
  analogDriftSpeed = v;
  init_DRIFT_LFOs();
}
static void apply_param_analog_drift_spread(int16_t v) {
  analogDriftSpread = (int8_t)v;
  init_DRIFT_LFOs();
}

static void apply_param_lfo1_to_dco(int16_t v) {
  LFO1toDCOVal = (uint16_t)v;
  float amt = (float)expConverterFloat(LFO1toDCOVal, 500) / 275000.0f;
  LFO1toDCO_q24 = lfo_pitch_depth_q24(amt, LFO1_PITCH_DEPTH_SCALE);
}

static void apply_param_lfo1_speed(int16_t v) {
  LFO1SpeedVal = (uint16_t)v;
  LFO1Speed = expConverterFloat(LFO1SpeedVal, 5000);
  LFO1_class.setMode0Freq((float)LFO1Speed, micros());
}
static void apply_param_lfo2_speed(int16_t v) {
  LFO2SpeedVal = (uint16_t)v;
  LFO2Speed = expConverterFloat(LFO2SpeedVal, 5000);
  LFO2_class.setMode0Freq((float)LFO2Speed, micros());
}

static void apply_param_vca_level(int16_t v) {
  VCALevel = (uint16_t)constrain((int)v * 32, 0, 4095);
}
static void apply_param_dist_drive(int16_t v) { DIST_DRIVE = (uint16_t)constrain((int)v, 0, 4095); }
static void apply_param_dist_mix(int16_t v)   { DIST_MIX = (uint16_t)constrain((int)v, 0, 4095); }
static void apply_param_filter_mode(int16_t v){ FILTER_MODE = (uint8_t)constrain((int)v, 0, 255); }

static void apply_param_lfo1_to_vca(int16_t v) {
  LFO1toVCA = (uint16_t)constrain((int)v, 0, 4095);
  cv_bake_lfo1_to_vca_scale();
}
static void apply_param_lfo2_to_pw(int16_t v)         { LFO2toPW = (uint16_t)v; }
static void apply_param_adsr3_to_pwm(int16_t v)       { ADSR1toPWM = (int16_t)v - 512; }
static void apply_param_adsr3_to_detune1(int16_t v)   { ADSR1toDETUNE1 = v; }
static void apply_param_adsr3_pitch_mode(int16_t v)   { env_dco_pitch_centered = (v != 0) ? 1 : 0; }

static void apply_param_adsr1_attack_curve(int16_t v) {
  ADSR1AttackCurveVal = (uint8_t)v;
  ADSR_VCA_change_attack_curve(ADSR1AttackCurveVal);
}
static void apply_param_adsr1_decay_curve(int16_t v) {
  ADSR1DecayCurveVal = (uint8_t)v;
  ADSR_VCA_change_decay_curve(ADSR1DecayCurveVal);
}
static void apply_param_adsr2_attack_curve(int16_t v) {
  ADSR2AttackCurveVal = (uint8_t)v;
  ADSR_VCF_change_attack_curve(ADSR2AttackCurveVal);
}
static void apply_param_adsr2_decay_curve(int16_t v) {
  ADSR2DecayCurveVal = (uint8_t)v;
  ADSR_VCF_change_decay_curve(ADSR2DecayCurveVal);
}

static void apply_param_pw_value(int16_t v)          { PW = (uint16_t)v; }
static void apply_param_adsr1_to_vca(int16_t v)      { ADSR1toVCA = v; }
static void apply_param_pwm_pots_manual(int16_t v)   { PWMPotsControlManual = (v != 0); }
static void apply_param_adsr3_enabled(int16_t v)     { ADSR3Enabled = (v != 0); }

#define DECL_MOD_SLOT_APPLIERS(N) \
  static void apply_param_mod_slot##N##_source(int16_t v) { mod_matrix_set_source(N, v); } \
  static void apply_param_mod_slot##N##_dest(int16_t v)   { mod_matrix_set_dest(N, v); } \
  static void apply_param_mod_slot##N##_depth(int16_t v)  { mod_matrix_set_depth(N, v); }

DECL_MOD_SLOT_APPLIERS(0)
DECL_MOD_SLOT_APPLIERS(1)
DECL_MOD_SLOT_APPLIERS(2)
DECL_MOD_SLOT_APPLIERS(3)
DECL_MOD_SLOT_APPLIERS(4)
DECL_MOD_SLOT_APPLIERS(5)
DECL_MOD_SLOT_APPLIERS(6)
DECL_MOD_SLOT_APPLIERS(7)
#undef DECL_MOD_SLOT_APPLIERS

static void apply_param_calibration_flag(int16_t v) {
  calibrationFlag = (v != 0);
}
static void apply_param_manual_calibration_flag(int16_t v) {
  const bool wasOn = manualCalibrationFlag;
  manualCalibrationFlag = (v != 0);
  calibrationFlag = (v != 0);
  if (wasOn && v == 0) {
    OSC1Level = lin_to_log_128[OSC1LevelVal];
    OSC2Level = lin_to_log_128[OSC2LevelVal];
    SubLevel = (uint16_t)constrain((int)SubLevelVal * 32, 0, 4095);
    mcpUpdate();
    update_waveSelector(4);
  }
}
static void apply_param_manual_calibration_stage(int16_t v) {
  int16_t stage = constrain(v, 0, (int16_t)cal_stage_max_n(NUM_OSCILLATORS));
  manualCalibrationStage = (uint8_t)stage;
}

static void apply_noop(int16_t) {}

static void apply_param_debug_command(int16_t v) {
  switch (v) {
    case 40: bench_request_dump(); return;
    case 41: bench_reset_all(); return;
    case 42: bench_toggle_periodic(); return;
    case 43: mcp_dac_probe(); return;
    case 44: mcp_dac_reattach(); return;
    case 45: bench_request_dump(); return;
    default: return;
  }
}

static const ParamDescriptorT<int16_t> paramTable[] = {
  { PARAM_OSC1_SAW_ENABLE, apply_param_osc1_saw_enable },
  { PARAM_OSC1_PULSE_ENABLE, apply_param_osc1_pulse_enable },
  { PARAM_OSC1_TRI_ENABLE, apply_param_osc1_tri_enable },
  { PARAM_OSC2_SAW_ENABLE, apply_param_osc2_saw_enable },
  { PARAM_OSC2_PULSE_ENABLE, apply_param_osc2_pulse_enable },
  { PARAM_OSC2_TRI_ENABLE, apply_param_osc2_tri_enable },
  { PARAM_OSC3_SAW_ENABLE, apply_noop },
  { PARAM_OSC3_PULSE_ENABLE, apply_noop },
  { PARAM_OSC3_TRI_ENABLE, apply_noop },
  { PARAM_SINE_STATUS, apply_param_sine_status },
  { PARAM_RESONANCE_COMPENSATION, apply_param_resonance_comp },
  { PARAM_VCA_ADSR_RESTART, apply_param_vca_adsr_restart },
  { PARAM_VCF_ADSR_RESTART, apply_param_vcf_adsr_restart },
  { PARAM_ADSR3_TO_OSC_SELECT, apply_param_adsr3_to_osc_select },
  { PARAM_LFO1_WAVEFORM, apply_param_lfo1_waveform },
  { PARAM_LFO2_WAVEFORM, apply_param_lfo2_waveform },
  { PARAM_OSC1_INTERVAL, apply_param_osc1_interval },
  { PARAM_OSC2_INTERVAL, apply_param_osc2_interval },
  { PARAM_OSC3_INTERVAL, apply_noop },
  { PARAM_OSC2_DETUNE_VAL, apply_param_osc2_detune },
  { PARAM_OSC3_DETUNE_VAL, apply_noop },
  { PARAM_LFO2_TO_OSC2, apply_noop },
  { PARAM_LFO2_TO_OSC3, apply_noop },
  { PARAM_LFO2_TO_OSC2_COARSE, apply_noop },
  { PARAM_LFO2_TO_OSC3_COARSE, apply_noop },
  { PARAM_CHARACTER, apply_noop },
  { PARAM_OSC_SYNC_MODE, apply_param_osc_sync_mode },
  { PARAM_PORTAMENTO_TIME, apply_param_portamento_time },
  { PARAM_PORTAMENTO_MODE, apply_param_portamento_mode },
  { PARAM_VCF_KEYTRACK, apply_param_vcf_keytrack },
  { PARAM_VELOCITY_TO_VCF, apply_param_velocity_to_vcf },
  { PARAM_VELOCITY_TO_VCA, apply_param_velocity_to_vca },
  { PARAM_OSC1_LEVEL, apply_param_osc1_level },
  { PARAM_OSC2_LEVEL, apply_param_osc2_level },
  { PARAM_OSC3_LEVEL, apply_noop },
  { PARAM_SUB_LEVEL, apply_param_sub_level },
  { PARAM_CALIBRATION_VALUE, apply_noop },
  { PARAM_VOICE_MODE, apply_param_voice_mode },
  { PARAM_VOICE_ALLOC_MODE, apply_noop },
  { PARAM_UNISON_DETUNE, apply_param_unison_detune },
  { PARAM_ANALOG_DRIFT_AMOUNT, apply_param_analog_drift_amount },
  { PARAM_ANALOG_DRIFT_SPEED, apply_param_analog_drift_speed },
  { PARAM_ANALOG_DRIFT_SPREAD, apply_param_analog_drift_spread },
  { PARAM_SYNC_MODE, apply_noop },
  { PARAM_SOFT_SYNC, apply_noop },
  { PARAM_SUBOSC_DIVIDE, apply_noop },
  { PARAM_LFO1_TO_DCO, apply_param_lfo1_to_dco },
  { PARAM_LFO1_TO_OSC1, apply_noop },
  { PARAM_LFO1_TO_OSC2, apply_noop },
  { PARAM_LFO1_TO_OSC3, apply_noop },
  { PARAM_LFO1_SPEED, apply_param_lfo1_speed },
  { PARAM_LFO2_SPEED, apply_param_lfo2_speed },
  { PARAM_VCA_LEVEL, apply_param_vca_level },
  { PARAM_LFO1_TO_VCA, apply_param_lfo1_to_vca },
  { PARAM_LFO2_TO_PW, apply_param_lfo2_to_pw },
  { PARAM_ADSR3_TO_PWM, apply_param_adsr3_to_pwm },
  { PARAM_ADSR3_TO_DETUNE1, apply_param_adsr3_to_detune1 },
  { PARAM_ADSR3_PITCH_MODE, apply_param_adsr3_pitch_mode },
  { PARAM_ADSR1_ATTACK_CURVE, apply_param_adsr1_attack_curve },
  { PARAM_ADSR1_DECAY_CURVE, apply_param_adsr1_decay_curve },
  { PARAM_ADSR2_ATTACK_CURVE, apply_param_adsr2_attack_curve },
  { PARAM_ADSR2_DECAY_CURVE, apply_param_adsr2_decay_curve },
  { PARAM_DIST_DRIVE, apply_param_dist_drive },
  { PARAM_DIST_MIX, apply_param_dist_mix },
  { PARAM_FILTER_MODE, apply_param_filter_mode },
  { PARAM_MOD_SLOT0_SOURCE, apply_param_mod_slot0_source },
  { PARAM_MOD_SLOT0_DEST, apply_param_mod_slot0_dest },
  { PARAM_MOD_SLOT0_DEPTH, apply_param_mod_slot0_depth },
  { PARAM_MOD_SLOT1_SOURCE, apply_param_mod_slot1_source },
  { PARAM_MOD_SLOT1_DEST, apply_param_mod_slot1_dest },
  { PARAM_MOD_SLOT1_DEPTH, apply_param_mod_slot1_depth },
  { PARAM_MOD_SLOT2_SOURCE, apply_param_mod_slot2_source },
  { PARAM_MOD_SLOT2_DEST, apply_param_mod_slot2_dest },
  { PARAM_MOD_SLOT2_DEPTH, apply_param_mod_slot2_depth },
  { PARAM_MOD_SLOT3_SOURCE, apply_param_mod_slot3_source },
  { PARAM_MOD_SLOT3_DEST, apply_param_mod_slot3_dest },
  { PARAM_MOD_SLOT3_DEPTH, apply_param_mod_slot3_depth },
  { PARAM_MOD_SLOT4_SOURCE, apply_param_mod_slot4_source },
  { PARAM_MOD_SLOT4_DEST, apply_param_mod_slot4_dest },
  { PARAM_MOD_SLOT4_DEPTH, apply_param_mod_slot4_depth },
  { PARAM_MOD_SLOT5_SOURCE, apply_param_mod_slot5_source },
  { PARAM_MOD_SLOT5_DEST, apply_param_mod_slot5_dest },
  { PARAM_MOD_SLOT5_DEPTH, apply_param_mod_slot5_depth },
  { PARAM_MOD_SLOT6_SOURCE, apply_param_mod_slot6_source },
  { PARAM_MOD_SLOT6_DEST, apply_param_mod_slot6_dest },
  { PARAM_MOD_SLOT6_DEPTH, apply_param_mod_slot6_depth },
  { PARAM_MOD_SLOT7_SOURCE, apply_param_mod_slot7_source },
  { PARAM_MOD_SLOT7_DEST, apply_param_mod_slot7_dest },
  { PARAM_MOD_SLOT7_DEPTH, apply_param_mod_slot7_depth },
  { PARAM_PW_VALUE, apply_param_pw_value },
  { PARAM_ADSR1_TO_VCA, apply_param_adsr1_to_vca },
  { PARAM_PWM_POTS_CONTROL_MANUAL, apply_param_pwm_pots_manual },
  { PARAM_ADSR3_ENABLED, apply_param_adsr3_enabled },
  { PARAM_FUNCTION_KEY, apply_noop },
  { PARAM_CALIBRATION_FLAG, apply_param_calibration_flag },
  { PARAM_MANUAL_CALIBRATION_FLAG, apply_param_manual_calibration_flag },
  { PARAM_MANUAL_CALIBRATION_STAGE, apply_param_manual_calibration_stage },
  { PARAM_MANUAL_CALIBRATION_OFFSET, apply_noop },
  { PARAM_MANUAL_CALIBRATION_STEP, apply_noop },
  { PARAM_AMP_COMP_440, apply_noop },
  { PARAM_CAL_PW_CENTER, apply_noop },
  { PARAM_AMP_COMP_DUTY_OFFSET, apply_noop },
  { PARAM_GAP_FROM_DCO, apply_noop },
  { PARAM_MANUAL_CALIBRATION_OFFSET_FROM_DCO, apply_noop },
  { PARAM_MANUAL_CALIBRATION_STORE, apply_noop },
  { PARAM_PRESET_SAVE, apply_noop },
  { PARAM_PRESET_LOAD, apply_noop },
  { PARAM_PRESET_DUMP, apply_noop },
  { PARAM_CAL_DUMP, apply_noop },
  { PARAM_DEBUG_COMMAND, apply_param_debug_command },
};

static const size_t paramTableSize = sizeof(paramTable) / sizeof(paramTable[0]);
static void (*paramApplyJump[PARAM_ROUTER_JUMP_SIZE])(int16_t);

void init_param_router() {
  param_router_build_jump(paramApplyJump, paramTable, paramTableSize);
}

void update_parameters(uint8_t paramNumber, int16_t paramValue) {
  param_router_apply(paramApplyJump, paramNumber, paramValue);
}