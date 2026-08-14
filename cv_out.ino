#include "include_all.h"
#include <string.h>

static constexpr int32_t CV_U12_MAX = 4095;
static constexpr int32_t CV_U12_SCALE = 4096;
static constexpr int32_t CV_PANEL_DEPTH_FULL = 512;
static constexpr int32_t CV_LFO_Q15_PEAK_DIV = CV_PANEL_DEPTH_FULL * 2;

static constexpr int CV_VCA_COMP_DEFAULT = 100;
static constexpr int CV_RESO_COMP_MAX_RESONANCE = 2300;
static constexpr int CV_RESO_COMP_MIN_RESONANCE = 50;
static constexpr int CV_RESO_COMP_MAX_VCA = 316;
static constexpr int CV_RESO_COMP_SLOPE_Q8 = 36;

static inline uint16_t lerp_0_4095(uint16_t value, uint16_t y0, uint16_t y1) {
  return (uint16_t)((int32_t)y0 + ((((int32_t)y1 - (int32_t)y0) * (int32_t)value) >> 12));
}

static inline uint16_t cv_clamp_u12(int32_t v) {
  if (v < 0) return 0;
  if (v > CV_U12_MAX) return (uint16_t)CV_U12_MAX;
  return (uint16_t)v;
}

static inline uint16_t cv_q15_to_u12(int16_t q15) {
  return cv_clamp_u12((int32_t)q15 >> 3);
}

void init_cv_out() {
  generateBezierArray({ 0, 4095 }, { 4095, 0 }, { 150, 1420 }, { -235, 815 }, 4096, AS2164_VCA_linearize_table);
  cv_update_mod_scales();
  vcf_drift_scale_q15 = (int32_t)analogDrift;
  for (byte i = 0; i < NUM_VOICES; i++) VCFKeytrackPerVoice_q15[i] = 32768;
}

void cv_bake_adsr2_to_vcf_scale() {
  ADSR2toVCF_scale_q15 = (int32_t)ADSR2toVCF << 3;
}

void cv_bake_lfo2_to_vcf_scale() {
  LFO2toVCF_scale_q15 = -((int32_t)LFO2toVCF << 2);
}

void cv_bake_lfo1_to_vca_scale() {
  LFO1toVCA_scale_q15 = -((int32_t)LFO1toVCA << 2);
}

void cv_update_mod_scales() {
  cv_bake_adsr2_to_vcf_scale();
  cv_bake_lfo2_to_vcf_scale();
  cv_bake_lfo1_to_vca_scale();
}

void write_cv_pwm() {
  htim4->setCaptureCompare(1, RESONANCE_PWM[0], TICK_COMPARE_FORMAT);
  htim8->setCaptureCompare(1, RESONANCE_PWM[1 < NUM_FILTERS ? 1 : 0], TICK_COMPARE_FORMAT);
  htim5->setCaptureCompare(1, RESONANCE_PWM[2 < NUM_FILTERS ? 2 : 0], TICK_COMPARE_FORMAT);
  htim3->setCaptureCompare(1, RESONANCE_PWM[3 < NUM_FILTERS ? 3 : 0], TICK_COMPARE_FORMAT);

  htim12->setCaptureCompare(1, VCF_PWM[0], TICK_COMPARE_FORMAT);
  htim4->setCaptureCompare(3, VCF_PWM[1], TICK_COMPARE_FORMAT);
  htim15->setCaptureCompare(2, VCF_PWM[2], TICK_COMPARE_FORMAT);
  htim5->setCaptureCompare(3, VCF_PWM[3], TICK_COMPARE_FORMAT);

  htim2->setCaptureCompare(3, VCA_PWM[0], TICK_COMPARE_FORMAT);
  htim13->setCaptureCompare(1, VCA_PWM[1], TICK_COMPARE_FORMAT);
  htim1->setCaptureCompare(4, VCA_PWM[2], TICK_COMPARE_FORMAT);
  htim3->setCaptureCompare(3, VCA_PWM[3], TICK_COMPARE_FORMAT);
}

void write_cv_pwm_manual_calibration(uint8_t voice) {
  // AS2164 / inverted VCF: PWM 0 = open, PWM 4095 = mute/closed. Play-path
  // full envelope lands around 100; cal opens the soloed voice fully.
  static constexpr uint16_t CAL_VCA_OPEN = 0;
  static constexpr uint16_t CAL_VCA_MUTED = 4095;
  static constexpr uint16_t CAL_VCF_OPEN = 0;
  static constexpr uint16_t CAL_VCF_CLOSED = 4095;

  htim4->setCaptureCompare(1, 0, TICK_COMPARE_FORMAT);
  htim8->setCaptureCompare(1, 0, TICK_COMPARE_FORMAT);
  htim5->setCaptureCompare(1, 0, TICK_COMPARE_FORMAT);
  htim3->setCaptureCompare(1, 0, TICK_COMPARE_FORMAT);

  uint16_t vcf_v[4] = { CAL_VCF_CLOSED, CAL_VCF_CLOSED, CAL_VCF_CLOSED, CAL_VCF_CLOSED };
  uint16_t vca_v[4] = { CAL_VCA_MUTED, CAL_VCA_MUTED, CAL_VCA_MUTED, CAL_VCA_MUTED };
  if (voice < 4) {
    vcf_v[voice] = CAL_VCF_OPEN;
    vca_v[voice] = CAL_VCA_OPEN;
  }

  htim12->setCaptureCompare(1, vcf_v[0], TICK_COMPARE_FORMAT);
  htim4->setCaptureCompare(3, vcf_v[1], TICK_COMPARE_FORMAT);
  htim15->setCaptureCompare(2, vcf_v[2], TICK_COMPARE_FORMAT);
  htim5->setCaptureCompare(3, vcf_v[3], TICK_COMPARE_FORMAT);

  htim2->setCaptureCompare(3, vca_v[0], TICK_COMPARE_FORMAT);
  htim13->setCaptureCompare(1, vca_v[1], TICK_COMPARE_FORMAT);
  htim1->setCaptureCompare(4, vca_v[2], TICK_COMPARE_FORMAT);
  htim3->setCaptureCompare(3, vca_v[3], TICK_COMPARE_FORMAT);
}

inline void update_CV_outs() {
  const int16_t local_LFO1Level = LFO1Level;
  const int16_t local_LFO2Level = LFO2Level;

  if (timer1msFlag) {
    if (RESONANCEAmpCompensation) {
      const int resonance_in = min((int)RESONANCE, CV_RESO_COMP_MAX_RESONANCE);
      const int32_t comp = CV_RESO_COMP_MAX_VCA -
        (((resonance_in - CV_RESO_COMP_MIN_RESONANCE) * CV_RESO_COMP_SLOPE_Q8) >> 8);
      VCAResonanceCompensation = (int16_t)(comp > 0 ? comp : 0);
    } else {
      VCAResonanceCompensation = CV_VCA_COMP_DEFAULT;
    }

    if (VCFKeytrack != 0) {
      for (byte i = 0; i < NUM_VOICES; i++) {
        const int32_t dn = (int)note[i] - 60;
        VCFKeytrackPerVoice_q15[i] = 32768 + (int32_t)(((int64_t)VCFKeytrackModifier_q15 * dn));
      }
    } else {
      for (byte i = 0; i < NUM_VOICES; i++) VCFKeytrackPerVoice_q15[i] = 32768;
    }

    if (analogDrift != 0) {
      for (byte i = 0; i < NUM_VOICES; i++) {
        VCF_DRIFT[i] = (int16_t)(((int64_t)LFO_DRIFT_LEVEL[i] * (int64_t)vcf_drift_scale_q15) >> 15);
      }
    } else {
      for (byte i = 0; i < NUM_VOICES; i++) VCF_DRIFT[i] = 0;
    }
  }

  int32_t mod_sums[MOD_DEST_COUNT];
  if (!manualCalibrationFlag) {
    mod_matrix_accumulate(mod_sums, local_LFO1Level, local_LFO2Level);
    matrix_pitch_mod_q24 = mod_matrix_pitch_to_q24(mod_sums[MOD_DEST_PITCH]);
  } else {
    memset(mod_sums, 0, sizeof(mod_sums));
    matrix_pitch_mod_q24 = 0;
  }
  const int32_t matrix_cutoff = mod_sums[MOD_DEST_VCF_CUTOFF];

  const int16_t LFO1toVCA_calc =
    (int16_t)(((int64_t)local_LFO1Level * (int64_t)LFO1toVCA_scale_q15) >> 15);
  const int32_t LFO2toVCF_mod =
    (int32_t)(((int64_t)local_LFO2Level * (int64_t)LFO2toVCF_scale_q15) >> 15);

  for (byte i = 0; i < NUM_VOICES; i++) {
    int32_t vca_q15 = 32768;
    if (velocityToVCAVal != 0) {
      vca_q15 = 32768 - velocityToVCA_q15 * (127 - (int32_t)midi_velocity[i]);
      if (vca_q15 < 0) vca_q15 = 0;
    }

    const uint16_t env_u12 = cv_q15_to_u12(ADSR_VCA_Level_q15[i]);
    const int16_t LFO1toVCA_current = (ADSR_VCA_Level_q15[i] == 0) ? 0 : LFO1toVCA_calc;
    const int32_t vca_pre = (int32_t)env_u12 + (int32_t)LFO1toVCA_current;
    const uint16_t VCA_Calculated = cv_clamp_u12((int32_t)(((int64_t)vca_pre * vca_q15) >> 15));
    VCA_PWM[i] = lerp_0_4095(AS2164_VCA_linearize_table[VCA_Calculated],
                             (uint16_t)VCAResonanceCompensation, (uint16_t)(4095 - VCALevel));

    int32_t vcf_vel_q15 = 32768;
    if (velocityToVCFVal != 0) {
      vcf_vel_q15 = 32768 - velocityToVCF_q15 * (127 - (int32_t)midi_velocity[i]);
      if (vcf_vel_q15 < 0) vcf_vel_q15 = 0;
    }

    const int32_t ADSR2toVCFcalculated =
      (int32_t)(((int64_t)ADSR_VCF_Level_q15[i] * (int64_t)ADSR2toVCF_scale_q15) >> 15);
    int32_t combined =
      ADSR2toVCFcalculated + LFO2toVCF_mod + (int32_t)CUTOFF + (int32_t)VCF_DRIFT[i] + matrix_cutoff;
    int32_t scaled = (int32_t)(((int64_t)combined * vcf_vel_q15) >> 15);
    scaled = (int32_t)(((int64_t)scaled * VCFKeytrackPerVoice_q15[i]) >> 15);
    VCF_PWM[i] = (uint16_t)(4095 - (int)cv_clamp_u12(scaled));
  }

  uint16_t dist_out = DIST_DRIVE;
  uint16_t dist_mix_out = DIST_MIX;
  uint16_t l1 = OSC1Level, l2 = OSC2Level, ls = SubLevel;
  if (!manualCalibrationFlag) {
    mod_matrix_apply_cv(mod_sums, &dist_out, &dist_mix_out, &l1, &l2, &ls);
  } else {
    for (byte i = 0; i < NUM_FILTERS; i++) RESONANCE_PWM[i] = RESONANCE;
  }
  (void)dist_out;
  (void)dist_mix_out;

  write_cv_pwm();
  mcpUpdate(l1, l2, ls, true);
}

void update_CV_outs_manual_calibration() {
  // One level per oscillator, carrying all of its waves: opening it for the
  // oscillator under trim also lets its pulse through (see waveSelector.h).
  static constexpr uint16_t CAL_OSC_ON = 50;
  static constexpr uint16_t CAL_OSC_MUTED = 4095;
  // The oscillator levels are inverted (lin_to_log_128[] — 4095 is silent), the
  // sub CV is direct (SubLevelVal * 32), so its mute is the other end of the
  // scale: at CAL_OSC_MUTED mcpUpdate() had every voice's sub DAC wide open.
  static constexpr uint16_t CAL_SUB_MUTED = 0;

  uint8_t osc = cal_stage_to_osc_n((uint8_t)manualCalibrationStage, NUM_OSCILLATORS);
  if (osc >= NUM_OSCILLATORS) osc = NUM_OSCILLATORS - 1;
  uint8_t voice = osc / 2;
  if (voice > 3) voice = 3;
  const uint8_t stage = (uint8_t)manualCalibrationStage;
  const bool oscA = ((osc % 2) == 0);
  const bool tri = cal_stage_is_tri_n(stage, NUM_OSCILLATORS);
  const bool square = cal_stage_is_square_n(stage, NUM_OSCILLATORS);

  for (int i = 0; i < 4; i++) {
    waveSelectorMux.writePin(osc1SawPins[i], 1);
    waveSelectorMux.writePin(osc2SawPins[i], 1);
    waveSelectorMux.writePin(osc1TriPins[i], 1);
    waveSelectorMux.writePin(osc2PulsePins[i], 1);
  }

  // A needs OSC1Level on every substage; B needs OSC2Level. Pulse has no
  // switch — it mutes when DCO PW is 0 (saw/tri), not via this DAC.
  if (oscA) {
    OSC1Level = CAL_OSC_ON;
    OSC2Level = CAL_OSC_MUTED;
    if (tri) {
      waveSelectorMux.writePin(osc1TriPins[voice], 0);
    } else if (!square) {
      waveSelectorMux.writePin(osc1SawPins[voice], 0);
    }
  } else {
    OSC1Level = CAL_OSC_MUTED;
    OSC2Level = CAL_OSC_ON;
    if (square) {
      waveSelectorMux.writePin(osc2PulsePins[voice], 0);
    } else {
      waveSelectorMux.writePin(osc2SawPins[voice], 0);
    }
  }
  SubLevel = CAL_SUB_MUTED;
  waveSelectorMux.update();
  mcpUpdate();
  write_cv_pwm_manual_calibration(voice);
}
