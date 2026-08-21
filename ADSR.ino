/**
 * @file MAINBOARD-ADSR.ino
 * @brief 4-Voice Polyphonic Envelope Generator & Voice Trigger Engine (STM32).
 */

 #include "include_all.h"

 // =============================================================================
 // GLOBAL VARIABLE DEFINITIONS
 // =============================================================================
 
 volatile byte noteStart[NUM_VOICES];
 volatile byte noteEnd[NUM_VOICES];
 
 int16_t ADSR1Level_q15[NUM_VOICES];
 int16_t ADSR_VCA_Level_q15[NUM_VOICES];
 int16_t ADSR_VCF_Level_q15[NUM_VOICES];
 
 int8_t ADSR3ToOscSelect = 2;
 uint8_t env_dco_pitch_centered = 0;
 
 uint16_t ADSR1_attack  = 0;
 uint16_t ADSR1_decay   = 0;
 uint16_t ADSR1_sustain = 4095;
 uint16_t ADSR1_release = 0;
 bool ADSRRestart = true;
 
 int16_t ADSR1toDETUNE1;
 int32_t ADSR1toDETUNE1_scale_q24;
 int16_t ADSR1toPWM;
 int32_t ADSR1toPWM_scale = 0;
 
 volatile uint16_t adsr_params_dirty = 0;
 
 // =============================================================================
 // ENVELOPE INSTANCE INITIALIZATION (Clean constructors using named presets)
 // =============================================================================
 
 // EnvDCO / ADSR1 (Pitch / PWM Modulation)
 adsr adsr1_voice_0(ADSR_CV_CC, ADSR_CURVE_LINEAR, ADSR_CURVE_LINEAR, ADSR_CURVE_LINEAR);
 adsr adsr1_voice_1(ADSR_CV_CC, ADSR_CURVE_LINEAR, ADSR_CURVE_LINEAR, ADSR_CURVE_LINEAR);
 adsr adsr1_voice_2(ADSR_CV_CC, ADSR_CURVE_LINEAR, ADSR_CURVE_LINEAR, ADSR_CURVE_LINEAR);
 adsr adsr1_voice_3(ADSR_CV_CC, ADSR_CURVE_LINEAR, ADSR_CURVE_LINEAR, ADSR_CURVE_LINEAR);
 
 // EnvVCA (Volume Amplitude)
 adsr adsr_vca_voice_0(ADSR_CV_CC, ADSR_CURVE_EXP_SMOOTH, ADSR_CURVE_PERCUSSIVE, ADSR_CURVE_EXP_SMOOTH);
 adsr adsr_vca_voice_1(ADSR_CV_CC, ADSR_CURVE_EXP_SMOOTH, ADSR_CURVE_PERCUSSIVE, ADSR_CURVE_EXP_SMOOTH);
 adsr adsr_vca_voice_2(ADSR_CV_CC, ADSR_CURVE_EXP_SMOOTH, ADSR_CURVE_PERCUSSIVE, ADSR_CURVE_EXP_SMOOTH);
 adsr adsr_vca_voice_3(ADSR_CV_CC, ADSR_CURVE_EXP_SMOOTH, ADSR_CURVE_PERCUSSIVE, ADSR_CURVE_EXP_SMOOTH);
 
 // EnvVCF (Filter Cutoff)
 adsr adsr_vcf_voice_0(ADSR_CV_CC, ADSR_CURVE_S_CURVE_SOFT, ADSR_CURVE_ROUNDED, ADSR_CURVE_EXP_SMOOTH);
 adsr adsr_vcf_voice_1(ADSR_CV_CC, ADSR_CURVE_S_CURVE_SOFT, ADSR_CURVE_ROUNDED, ADSR_CURVE_EXP_SMOOTH);
 adsr adsr_vcf_voice_2(ADSR_CV_CC, ADSR_CURVE_S_CURVE_SOFT, ADSR_CURVE_ROUNDED, ADSR_CURVE_EXP_SMOOTH);
 adsr adsr_vcf_voice_3(ADSR_CV_CC, ADSR_CURVE_S_CURVE_SOFT, ADSR_CURVE_ROUNDED, ADSR_CURVE_EXP_SMOOTH);
 
 ADSRStruct ADSRVoices[] = {
   { adsr1_voice_0, adsr_vca_voice_0, adsr_vcf_voice_0 },
   { adsr1_voice_1, adsr_vca_voice_1, adsr_vcf_voice_1 },
   { adsr1_voice_2, adsr_vca_voice_2, adsr_vcf_voice_2 },
   { adsr1_voice_3, adsr_vca_voice_3, adsr_vcf_voice_3 },
 };
 
 // =============================================================================
 // IMPLEMENTATION
 // =============================================================================
 
 /**
  * @brief Converts MIDI panel sustain values into native library sustain limits.
  */
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
 
 /**
  * @brief Boot Initialization. Builds Bézier LUTs and applies initial envelope parameters.
  */
 void init_ADSR() {
 #if ADSR_BEZIER_NATIVE_Q15
   adsrBezierInitTables((float)ADSR_Q15_PEAK, ARRAY_SIZE);
 #else
   adsrBezierInitTables((float)ADSR_1_CC, ARRAY_SIZE);
 #endif
 
   // Fill lin->exp lookup table for UI time inverses
   for (int i = 0; i < LIN_TO_EXP_TABLE_SIZE; i++) {
     linToExpLookup[i] = linearToExponential((uint16_t)i, 50.0f, maxADSRControlValue);
   }
 
   // Apply initial times and restart settings across all 4 voices
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
 
 /**
  * @brief High-speed audio loop (~10 kHz). Evaluates all 12 envelopes and updates Q15 buffers.
  */
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
 
     ADSR1Level_q15[i]     = (int16_t)ADSRVoices[i].adsr1_voice.getWave();
     ADSR_VCA_Level_q15[i] = (int16_t)ADSRVoices[i].adsr_vca_voice.getWave();
     ADSR_VCF_Level_q15[i] = (int16_t)ADSRVoices[i].adsr_vcf_voice.getWave();
   }
   ADSR_set_parameters();
 }
 
 /**
  * @brief Periodic parameter flush (~200 Hz).
  */
 inline void ADSR_set_parameters() {
   static uint8_t tick = 0;
   if (++tick < 50) return;
   tick = 0;
 
   uint16_t ch = adsr_params_dirty;
   if (!ch) return;
   adsr_params_dirty = 0;
 
   // DCO Envelope Updates
   if (ch & ADSR_DIRTY_DCO_ALL) {
     const int s = adsr_sustain_for_set(ADSR1_sustain, ADSR_CV_SCALE);
     for (int i = 0; i < NUM_VOICES; i++) {
       if (ch & ADSR_DIRTY_DCO_A) ADSRVoices[i].adsr1_voice.setAttack(ADSR1_attack);
       if (ch & ADSR_DIRTY_DCO_D) ADSRVoices[i].adsr1_voice.setDecay(ADSR1_decay);
       if (ch & ADSR_DIRTY_DCO_S) ADSRVoices[i].adsr1_voice.setSustain(s);
       if (ch & ADSR_DIRTY_DCO_R) ADSRVoices[i].adsr1_voice.setRelease(ADSR1_release);
     }
   }
 
   // VCA Envelope Updates
   if (ch & ADSR_DIRTY_VCA_ALL) {
     const int s = adsr_sustain_for_set(ADSR_VCA_sustain, ADSR_CV_SCALE);
     for (int i = 0; i < NUM_VOICES; i++) {
       if (ch & ADSR_DIRTY_VCA_A) ADSRVoices[i].adsr_vca_voice.setAttack(ADSR_VCA_attack);
       if (ch & ADSR_DIRTY_VCA_D) ADSRVoices[i].adsr_vca_voice.setDecay(ADSR_VCA_decay);
       if (ch & ADSR_DIRTY_VCA_S) ADSRVoices[i].adsr_vca_voice.setSustain(s);
       if (ch & ADSR_DIRTY_VCA_R) ADSRVoices[i].adsr_vca_voice.setRelease(ADSR_VCA_release);
     }
   }
 
   // VCF Envelope Updates
   if (ch & ADSR_DIRTY_VCF_ALL) {
     const int s = adsr_sustain_for_set(ADSR_VCF_sustain, ADSR_CV_SCALE);
     for (int i = 0; i < NUM_VOICES; i++) {
       if (ch & ADSR_DIRTY_VCF_A) ADSRVoices[i].adsr_vcf_voice.setAttack(ADSR_VCF_attack);
       if (ch & ADSR_DIRTY_VCF_D) ADSRVoices[i].adsr_vcf_voice.setDecay(ADSR_VCF_decay);
       if (ch & ADSR_DIRTY_VCF_S) ADSRVoices[i].adsr_vcf_voice.setSustain(s);
       if (ch & ADSR_DIRTY_VCF_R) ADSRVoices[i].adsr_vcf_voice.setRelease(ADSR_VCF_release);
     }
   }
 }
 
 // =============================================================================
 // RETRIGGER BEHAVIOR SETTERS
 // =============================================================================
 
 void ADSR1_set_restart() {
   for (int i = 0; i < NUM_VOICES; i++) ADSRVoices[i].adsr1_voice.setResetAttack(ADSRRestart);
 }
 
 void ADSR_VCA_set_restart() {
   for (int i = 0; i < NUM_VOICES; i++) ADSRVoices[i].adsr_vca_voice.setResetAttack(VCAADSRRestart);
 }
 
 void ADSR_VCF_set_restart() {
   for (int i = 0; i < NUM_VOICES; i++) ADSRVoices[i].adsr_vcf_voice.setResetAttack(VCFADSRRestart);
 }
 
 // =============================================================================
 // CURVE SWITCHERS (Direct pointer swaps across all 4 voices)
 // =============================================================================
 
 // --- VCA Envelope Curves ---
 void ADSR_VCA_change_attack_curve(uint8_t adsrCurveAttack) {
   for (int i = 0; i < NUM_VOICES; i++) ADSRVoices[i].adsr_vca_voice.adsrCurveAttack(adsrCurveAttack);
 }
 void ADSR_VCA_change_decay_curve(uint8_t adsrCurveDecay) {
   for (int i = 0; i < NUM_VOICES; i++) ADSRVoices[i].adsr_vca_voice.adsrCurveDecay(adsrCurveDecay);
 }
 void ADSR_VCA_change_release_curve(uint8_t adsrCurveRelease) {
   for (int i = 0; i < NUM_VOICES; i++) ADSRVoices[i].adsr_vca_voice.adsrCurveRelease(adsrCurveRelease);
 }
 
 // --- VCF Envelope Curves ---
 void ADSR_VCF_change_attack_curve(uint8_t adsrCurveAttack) {
   for (int i = 0; i < NUM_VOICES; i++) ADSRVoices[i].adsr_vcf_voice.adsrCurveAttack(adsrCurveAttack);
 }
 void ADSR_VCF_change_decay_curve(uint8_t adsrCurveDecay) {
   for (int i = 0; i < NUM_VOICES; i++) ADSRVoices[i].adsr_vcf_voice.adsrCurveDecay(adsrCurveDecay);
 }
 void ADSR_VCF_change_release_curve(uint8_t adsrCurveRelease) {
   for (int i = 0; i < NUM_VOICES; i++) ADSRVoices[i].adsr_vcf_voice.adsrCurveRelease(adsrCurveRelease);
 }
 
 // --- DCO Envelope Curves (ADSR1) ---
 void ADSR1_change_attack_curve(uint8_t adsrCurveAttack) {
   for (int i = 0; i < NUM_VOICES; i++) ADSRVoices[i].adsr1_voice.adsrCurveAttack(adsrCurveAttack);
 }
 void ADSR1_change_decay_curve(uint8_t adsrCurveDecay) {
   for (int i = 0; i < NUM_VOICES; i++) ADSRVoices[i].adsr1_voice.adsrCurveDecay(adsrCurveDecay);
 }
 void ADSR1_change_release_curve(uint8_t adsrCurveRelease) {
   for (int i = 0; i < NUM_VOICES; i++) ADSRVoices[i].adsr1_voice.adsrCurveRelease(adsrCurveRelease);
 }
 
 void ADSR1_change_curves(uint8_t adsrCurveAttack, uint8_t adsrCurveDecay, uint8_t adsrCurveRelease) {
   ADSR1_change_attack_curve(adsrCurveAttack);
   ADSR1_change_decay_curve(adsrCurveDecay);
   ADSR1_change_release_curve(adsrCurveRelease);
 }