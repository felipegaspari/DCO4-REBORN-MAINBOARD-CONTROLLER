// Compile mo-lfo into the sketch TU (Arduino IDE does not link _build_libs/*.cpp).
#include "_build_libs/mo-lfo/mo-lfo.cpp"

void init_LFOs() {
  LFO1_class.setWaveForm(LFO1Waveform);
  LFO1_class.setAmplQ15(MO_LFO_Q15_ONE);
  LFO1_class.setMode(false);
  LFO1_class.setMode0Freq(0.5f);

  LFO2_class.setWaveForm(LFO2Waveform);
  LFO2_class.setAmplQ15(MO_LFO_Q15_ONE);
  LFO2_class.setMode(false);
  LFO2_class.setMode0Freq(5.0f);
}

void init_DRIFT_LFOs() {
  for (int i = 0; i < NUM_VOICES; i++) {
    LFO_DRIFT_SPEED_OFFSET[i] =
      (float)(1.00f - (float)((float)analogDriftSpread * 0.005f) +
              (float)((float)analogDriftSpread * 0.00125f * (float)i)) *
      (float)expConverterFloat((uint16_t)analogDriftSpeed, 5000);
    LFO_DRIFT_CLASS[i].setWaveForm(LFO_DRIFT_WAVEFORM);
    LFO_DRIFT_CLASS[i].setAmplQ15(MO_LFO_Q15_ONE);
    LFO_DRIFT_CLASS[i].setMode(false);
    LFO_DRIFT_CLASS[i].setMode0Freq(LFO_DRIFT_SPEED_OFFSET[i], micros());
  }
}

inline void LFO1() {
  LFO1Level = LFO1_class.getWaveQ15(micros());
}

inline void LFO2() {
  LFO2Level = LFO2_class.getWaveQ15(micros());
}

inline void DRIFT_LFOs() {
  unsigned long currentMicros = micros();
  for (int i = 0; i < NUM_VOICES; i++) {
    LFO_DRIFT_LEVEL[i] = (int16_t)(-LFO_DRIFT_CLASS[i].getWaveQ15(currentMicros));
  }
}
