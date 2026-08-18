static inline int adsr_sustain_for_set(uint16_t panel, uint16_t panel_full) {
#if ADSR_BEZIER_NATIVE_Q15
  if (panel_full == 0) return 0;
  if (panel_full == ADSR_CV_SCALE)
    return (int)(((uint32_t)panel * (uint32_t)ADSR_Q15_PEAK) >> 12);
  return (int)(((uint32_t)panel * (uint32_t)ADSR_Q15_PEAK) / (uint32_t)panel_full);
#else
  (void)panel_full;
  return (int)panel;
#endif
}

void init_ADSR() {
#if ADSR_BEZIER_NATIVE_Q15
  adsrBezierInitTables((float)ADSR_Q15_PEAK, ARRAY_SIZE, _curve_tables);
#else
  adsrBezierInitTables(ADSR_1_CC, ARRAY_SIZE, _curve_tables);
#endif

// Boot: fill lin→exp table so Screen ADSR mirrors can invert wire times.
for (int i = 0; i < LIN_TO_EXP_TABLE_SIZE; i++) {
  linToExpLookup[i] = linearToExponential((uint16_t)i, 50.0f, maxADSRControlValue);
}

  for (int i = 0; i < NUM_VOICES; i++) {
    ADSRVoices[i].adsr1_voice.setAttack(ADSR1_attack);
    ADSRVoices[i].adsr1_voice.setDecay(ADSR1_decay);
    ADSRVoices[i].adsr1_voice.setSustain(adsr_sustain_for_set(ADSR1_sustain, ADSR_CV_SCALE));
    ADSRVoices[i].adsr1_voice.setRelease(ADSR1_release);
    ADSRVoices[i].adsr1_voice.setResetAttack(ADSRRestart);

    ADSRVoices[i].adsr_vca_voice.setAttack(ADSR_VCA_attack);
    ADSRVoices[i].adsr_vca_voice.setDecay(ADSR_VCA_decay);
    ADSRVoices[i].adsr_vca_voice.setSustain(adsr_sustain_for_set(ADSR_VCA_sustain, ADSR_CV_SCALE));
    ADSRVoices[i].adsr_vca_voice.setRelease(ADSR_VCA_release);
    ADSRVoices[i].adsr_vca_voice.setResetAttack(VCAADSRRestart);

    ADSRVoices[i].adsr_vcf_voice.setAttack(ADSR_VCF_attack);
    ADSRVoices[i].adsr_vcf_voice.setDecay(ADSR_VCF_decay);
    ADSRVoices[i].adsr_vcf_voice.setSustain(adsr_sustain_for_set(ADSR_VCF_sustain, ADSR_CV_SCALE));
    ADSRVoices[i].adsr_vcf_voice.setRelease(ADSR_VCF_release);
    ADSRVoices[i].adsr_vcf_voice.setResetAttack(VCFADSRRestart);
  }
}

inline void ADSR_update() {
  for (int i = 0; i < NUM_VOICES; i++) {
    if (noteEnd[i] == 1) {
      ADSRVoices[i].adsr1_voice.noteOff();
      ADSRVoices[i].adsr_vca_voice.noteOff();
      ADSRVoices[i].adsr_vcf_voice.noteOff();
      noteEnd[i] = 0;
    } else if (noteStart[i] == 1) {
      if ((note_flags[i] & NOTE_FLAG_PORTA_ONLY) == 0) {
        ADSRVoices[i].adsr1_voice.noteOn();
        ADSRVoices[i].adsr_vca_voice.noteOn();
        ADSRVoices[i].adsr_vcf_voice.noteOn();
      }
      noteStart[i] = 0;
      note_flags[i] = 0;
    }
    ADSR1Level_q15[i] = (int16_t)ADSRVoices[i].adsr1_voice.getWave();
    ADSR_VCA_Level_q15[i] = (int16_t)ADSRVoices[i].adsr_vca_voice.getWave();
    ADSR_VCF_Level_q15[i] = (int16_t)ADSRVoices[i].adsr_vcf_voice.getWave();
  }
  ADSR_set_parameters();
}

inline void ADSR_set_parameters() {
  static uint8_t tick = 0;
  if (++tick < 50) return;
  tick = 0;

  uint16_t ch = adsr_params_dirty;
  if (!ch) return;
  adsr_params_dirty = 0;

  if (ch & ADSR_DIRTY_DCO_A) {
    for (int i = 0; i < NUM_VOICES; i++) ADSRVoices[i].adsr1_voice.setAttack(ADSR1_attack);
  }
  if (ch & ADSR_DIRTY_DCO_D) {
    for (int i = 0; i < NUM_VOICES; i++) ADSRVoices[i].adsr1_voice.setDecay(ADSR1_decay);
  }
  if (ch & ADSR_DIRTY_DCO_S) {
    const int s = adsr_sustain_for_set(ADSR1_sustain, ADSR_CV_SCALE);
    for (int i = 0; i < NUM_VOICES; i++) ADSRVoices[i].adsr1_voice.setSustain(s);
  }
  if (ch & ADSR_DIRTY_DCO_R) {
    for (int i = 0; i < NUM_VOICES; i++) ADSRVoices[i].adsr1_voice.setRelease(ADSR1_release);
  }
  if (ch & ADSR_DIRTY_VCA_A) {
    for (int i = 0; i < NUM_VOICES; i++) ADSRVoices[i].adsr_vca_voice.setAttack(ADSR_VCA_attack);
  }
  if (ch & ADSR_DIRTY_VCA_D) {
    for (int i = 0; i < NUM_VOICES; i++) ADSRVoices[i].adsr_vca_voice.setDecay(ADSR_VCA_decay);
  }
  if (ch & ADSR_DIRTY_VCA_S) {
    const int s = adsr_sustain_for_set(ADSR_VCA_sustain, ADSR_CV_SCALE);
    for (int i = 0; i < NUM_VOICES; i++) ADSRVoices[i].adsr_vca_voice.setSustain(s);
  }
  if (ch & ADSR_DIRTY_VCA_R) {
    for (int i = 0; i < NUM_VOICES; i++) ADSRVoices[i].adsr_vca_voice.setRelease(ADSR_VCA_release);
  }
  if (ch & ADSR_DIRTY_VCF_A) {
    for (int i = 0; i < NUM_VOICES; i++) ADSRVoices[i].adsr_vcf_voice.setAttack(ADSR_VCF_attack);
  }
  if (ch & ADSR_DIRTY_VCF_D) {
    for (int i = 0; i < NUM_VOICES; i++) ADSRVoices[i].adsr_vcf_voice.setDecay(ADSR_VCF_decay);
  }
  if (ch & ADSR_DIRTY_VCF_S) {
    const int s = adsr_sustain_for_set(ADSR_VCF_sustain, ADSR_CV_SCALE);
    for (int i = 0; i < NUM_VOICES; i++) ADSRVoices[i].adsr_vcf_voice.setSustain(s);
  }
  if (ch & ADSR_DIRTY_VCF_R) {
    for (int i = 0; i < NUM_VOICES; i++) ADSRVoices[i].adsr_vcf_voice.setRelease(ADSR_VCF_release);
  }
}

void ADSR1_set_restart() {
  for (int i = 0; i < NUM_VOICES; i++) ADSRVoices[i].adsr1_voice.setResetAttack(ADSRRestart);
}

void ADSR_VCA_set_restart() {
  for (int i = 0; i < NUM_VOICES; i++) ADSRVoices[i].adsr_vca_voice.setResetAttack(VCAADSRRestart);
}

void ADSR_VCF_set_restart() {
  for (int i = 0; i < NUM_VOICES; i++) ADSRVoices[i].adsr_vcf_voice.setResetAttack(VCFADSRRestart);
}

void ADSR_VCA_change_attack_curve(uint8_t adsrCurveAttack) {
  const int s = adsr_sustain_for_set(ADSR_VCA_sustain, ADSR_CV_SCALE);
  for (int i = 0; i < NUM_VOICES; i++) {
    ADSRVoices[i].adsr_vca_voice.adsrCurveAttack(adsrCurveAttack);
    ADSRVoices[i].adsr_vca_voice.setAttack(ADSR_VCA_attack);
    ADSRVoices[i].adsr_vca_voice.setDecay(ADSR_VCA_decay);
    ADSRVoices[i].adsr_vca_voice.setSustain(s);
    ADSRVoices[i].adsr_vca_voice.setRelease(ADSR_VCA_release);
    ADSRVoices[i].adsr_vca_voice.setResetAttack(VCAADSRRestart);
  }
}

void ADSR_VCA_change_decay_curve(uint8_t adsrCurveDecay) {
  const int s = adsr_sustain_for_set(ADSR_VCA_sustain, ADSR_CV_SCALE);
  for (int i = 0; i < NUM_VOICES; i++) {
    ADSRVoices[i].adsr_vca_voice.adsrCurveDecay(adsrCurveDecay);
    ADSRVoices[i].adsr_vca_voice.setAttack(ADSR_VCA_attack);
    ADSRVoices[i].adsr_vca_voice.setDecay(ADSR_VCA_decay);
    ADSRVoices[i].adsr_vca_voice.setSustain(s);
    ADSRVoices[i].adsr_vca_voice.setRelease(ADSR_VCA_release);
    ADSRVoices[i].adsr_vca_voice.setResetAttack(VCAADSRRestart);
  }
}

void ADSR_VCF_change_attack_curve(uint8_t adsrCurveAttack) {
  const int s = adsr_sustain_for_set(ADSR_VCF_sustain, ADSR_CV_SCALE);
  for (int i = 0; i < NUM_VOICES; i++) {
    ADSRVoices[i].adsr_vcf_voice.adsrCurveAttack(adsrCurveAttack);
    ADSRVoices[i].adsr_vcf_voice.setAttack(ADSR_VCF_attack);
    ADSRVoices[i].adsr_vcf_voice.setDecay(ADSR_VCF_decay);
    ADSRVoices[i].adsr_vcf_voice.setSustain(s);
    ADSRVoices[i].adsr_vcf_voice.setRelease(ADSR_VCF_release);
    ADSRVoices[i].adsr_vcf_voice.setResetAttack(VCFADSRRestart);
  }
}

void ADSR_VCF_change_decay_curve(uint8_t adsrCurveDecay) {
  const int s = adsr_sustain_for_set(ADSR_VCF_sustain, ADSR_CV_SCALE);
  for (int i = 0; i < NUM_VOICES; i++) {
    ADSRVoices[i].adsr_vcf_voice.adsrCurveDecay(adsrCurveDecay);
    ADSRVoices[i].adsr_vcf_voice.setAttack(ADSR_VCF_attack);
    ADSRVoices[i].adsr_vcf_voice.setDecay(ADSR_VCF_decay);
    ADSRVoices[i].adsr_vcf_voice.setSustain(s);
    ADSRVoices[i].adsr_vcf_voice.setRelease(ADSR_VCF_release);
    ADSRVoices[i].adsr_vcf_voice.setResetAttack(VCFADSRRestart);
  }
}
