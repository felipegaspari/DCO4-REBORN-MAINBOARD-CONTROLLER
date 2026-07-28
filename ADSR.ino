// Boot: init Bézier tables and default A/D/S/R + restart for all voices' ADSR1/2/3.
void init_ADSR() {

  // Initialize ADSR Bézier lookup tables via library helper
  adsrBezierInitTables(ADSR_1_CC, ARRAY_SIZE, _curve_tables);


  for (int i = 0; i < NUM_VOICES; i++) {
    ADSRVoices[i].adsr1_voice.setAttack(ADSR1_attack);    // initialize attack
    ADSRVoices[i].adsr1_voice.setDecay(ADSR1_decay);      // initialize decay
    ADSRVoices[i].adsr1_voice.setSustain(ADSR1_sustain);  // initialize sustain
    ADSRVoices[i].adsr1_voice.setRelease(ADSR1_release);
    ADSRVoices[i].adsr1_voice.setResetAttack(VCAADSRRestart);

    ADSRVoices[i].adsr2_voice.setAttack(ADSR2_attack);    // initialize attack
    ADSRVoices[i].adsr2_voice.setDecay(ADSR2_decay);      // initialize decay
    ADSRVoices[i].adsr2_voice.setSustain(ADSR2_sustain);  // initialize sustain
    ADSRVoices[i].adsr2_voice.setRelease(ADSR2_release);
    ADSRVoices[i].adsr2_voice.setResetAttack(VCFADSRRestart);

    ADSRVoices[i].adsr3_voice.setAttack(ADSR3_attack);    // initialize attack
    ADSRVoices[i].adsr3_voice.setDecay(ADSR3_decay);      // initialize decay
    ADSRVoices[i].adsr3_voice.setSustain(ADSR3_sustain);  // initialize sustain
    ADSRVoices[i].adsr3_voice.setRelease(ADSR3_release);
    ADSRVoices[i].adsr3_voice.setResetAttack(true);
  }
}

// Hot path: apply noteStart/noteEnd edges, sample ADSR1/2/3 levels, refresh sustain params.
inline void ADSR_update() {
  for (int i = 0; i < NUM_VOICES; i++) {
    if (noteEnd[i] == 1) {
      ADSRVoices[i].adsr1_voice.noteOff();
      ADSRVoices[i].adsr2_voice.noteOff();
      ADSRVoices[i].adsr3_voice.noteOff();
    } else if (noteStart[i] == 1) {
      ADSRVoices[i].adsr1_voice.noteOff();
      ADSRVoices[i].adsr1_voice.setAttack(ADSR1_attack);
      ADSRVoices[i].adsr1_voice.setDecay(ADSR1_decay);
      ADSRVoices[i].adsr1_voice.setRelease(ADSR1_release);
      ADSRVoices[i].adsr1_voice.noteOn();

      ADSRVoices[i].adsr2_voice.noteOff();
      ADSRVoices[i].adsr2_voice.setAttack(ADSR2_attack);
      ADSRVoices[i].adsr2_voice.setDecay(ADSR2_decay);
      ADSRVoices[i].adsr2_voice.setRelease(ADSR2_release);
      ADSRVoices[i].adsr2_voice.noteOn();

      ADSRVoices[i].adsr3_voice.noteOff();
      ADSRVoices[i].adsr3_voice.setAttack(ADSR3_attack);
      ADSRVoices[i].adsr3_voice.setDecay(ADSR3_decay);
      ADSRVoices[i].adsr3_voice.setRelease(ADSR3_release);
      ADSRVoices[i].adsr3_voice.noteOn();
    }
    ADSR1Level[i] = ADSRVoices[i].adsr1_voice.getWave();
    ADSR2Level[i] = ADSRVoices[i].adsr2_voice.getWave();
    ADSR3Level[i] = ADSRVoices[i].adsr3_voice.getWave();
  }
  ADSR_set_parameters();
}

// Throttle (~5 ms): push current sustain values to all voice ADSRs.
inline void ADSR_set_parameters() {
      tADSR = millis();
  if ((tADSR - tADSR_params) > 5) {
    for (int i = 0; i < NUM_VOICES; i++) {
      ADSRVoices[i].adsr1_voice.setSustain(ADSR1_sustain);
      ADSRVoices[i].adsr2_voice.setSustain(ADSR2_sustain);
      ADSRVoices[i].adsr3_voice.setSustain(ADSR3_sustain);
    }
    tADSR_params = tADSR;
  }
}

// Apply VCAADSRRestart to all ADSR1 instances (param table).
void ADSR1_set_restart() {
  for (int i = 0; i < NUM_VOICES; i++) {
    ADSRVoices[i].adsr1_voice.setResetAttack(VCAADSRRestart);
  }
}

// Apply VCFADSRRestart to all ADSR2 instances (param table).
void ADSR2_set_restart() {
  for (int i = 0; i < NUM_VOICES; i++) {
    ADSRVoices[i].adsr2_voice.setResetAttack(VCFADSRRestart);
  }
}

// Set ADSR1 attack curve and re-apply timing params (param table).
void ADSR1_change_attack_curve(uint8_t adsrCurveAttack) {
  for (int i = 0; i < NUM_VOICES; i++) {
    ADSRVoices[i].adsr1_voice.adsrCurveAttack(adsrCurveAttack);
    ADSRVoices[i].adsr1_voice.setAttack(ADSR1_attack);    // initialize attack
    ADSRVoices[i].adsr1_voice.setDecay(ADSR1_decay);      // initialize decay
    ADSRVoices[i].adsr1_voice.setSustain(ADSR1_sustain);  // initialize sustain
    ADSRVoices[i].adsr1_voice.setRelease(ADSR1_release);
    ADSRVoices[i].adsr1_voice.setResetAttack(VCAADSRRestart);
  }
}

// Set ADSR1 decay curve and re-apply timing params (param table).
void ADSR1_change_decay_curve(uint8_t adsrCurveDecay) {
  for (int i = 0; i < NUM_VOICES; i++) {
    //ADSRVoices[i].adsr1_voice.changeCurves(ADSR_1_DACSIZE, ADSR1_curve1, ADSR1_curve2);
    ADSRVoices[i].adsr1_voice.adsrCurveDecay(adsrCurveDecay);
    ADSRVoices[i].adsr1_voice.setAttack(ADSR1_attack);    // initialize attack
    ADSRVoices[i].adsr1_voice.setDecay(ADSR1_decay);      // initialize decay
    ADSRVoices[i].adsr1_voice.setSustain(ADSR1_sustain);  // initialize sustain
    ADSRVoices[i].adsr1_voice.setRelease(ADSR1_release);
    ADSRVoices[i].adsr1_voice.setResetAttack(VCAADSRRestart);
  }
}

// Set ADSR1 release curve. Currently unused (no live callers).
void ADSR1_change_release_curve(uint8_t adsrCurveRelease) {
  for (int i = 0; i < NUM_VOICES; i++) {
    //ADSRVoices[i].adsr1_voice.changeCurves(ADSR_1_DACSIZE, ADSR1_curve1, ADSR1_curve2);
    ADSRVoices[i].adsr1_voice.adsrCurveRelease(adsrCurveRelease);
    ADSRVoices[i].adsr1_voice.setAttack(ADSR1_attack);    // initialize attack
    ADSRVoices[i].adsr1_voice.setDecay(ADSR1_decay);      // initialize decay
    ADSRVoices[i].adsr1_voice.setSustain(ADSR1_sustain);  // initialize sustain
    ADSRVoices[i].adsr1_voice.setRelease(ADSR1_release);
    ADSRVoices[i].adsr1_voice.setResetAttack(VCAADSRRestart);
  }
}

// Set ADSR2 attack curve and re-apply timing params (param table).
void ADSR2_change_attack_curve(uint8_t adsrCurveAttack) {
  for (int i = 0; i < NUM_VOICES; i++) {
    ADSRVoices[i].adsr2_voice.adsrCurveAttack(adsrCurveAttack);
    ADSRVoices[i].adsr2_voice.setAttack(ADSR2_attack);    // initialize attack
    ADSRVoices[i].adsr2_voice.setDecay(ADSR2_decay);      // initialize decay
    ADSRVoices[i].adsr2_voice.setSustain(ADSR2_sustain);  // initialize sustain
    ADSRVoices[i].adsr2_voice.setRelease(ADSR2_release);
    ADSRVoices[i].adsr2_voice.setResetAttack(VCFADSRRestart);
  }
}

// Set ADSR2 decay curve and re-apply timing params (param table).
void ADSR2_change_decay_curve(uint8_t adsrCurveDecay) {
  for (int i = 0; i < NUM_VOICES; i++) {
    ADSRVoices[i].adsr2_voice.adsrCurveDecay(adsrCurveDecay);
    ADSRVoices[i].adsr2_voice.setAttack(ADSR2_attack);    // initialize attack
    ADSRVoices[i].adsr2_voice.setDecay(ADSR2_decay);      // initialize decay
    ADSRVoices[i].adsr2_voice.setSustain(ADSR2_sustain);  // initialize sustain
    ADSRVoices[i].adsr2_voice.setRelease(ADSR2_release);
    ADSRVoices[i].adsr2_voice.setResetAttack(VCFADSRRestart);
  }
}

// Set ADSR2 release curve. Currently unused (no live callers).
void ADSR2_change_release_curve(uint8_t adsrCurveRelease) {
  for (int i = 0; i < NUM_VOICES; i++) {
    ADSRVoices[i].adsr2_voice.adsrCurveRelease(adsrCurveRelease);
    ADSRVoices[i].adsr2_voice.setAttack(ADSR2_attack);    // initialize attack
    ADSRVoices[i].adsr2_voice.setDecay(ADSR2_decay);      // initialize decay
    ADSRVoices[i].adsr2_voice.setSustain(ADSR2_sustain);  // initialize sustain
    ADSRVoices[i].adsr2_voice.setRelease(ADSR2_release);
    ADSRVoices[i].adsr2_voice.setResetAttack(VCFADSRRestart);
  }
}