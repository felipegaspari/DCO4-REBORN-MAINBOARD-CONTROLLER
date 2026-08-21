#ifndef __MAINBOARD_ADSR_H__
#define __MAINBOARD_ADSR_H__

/**
 * @file MAINBOARD-ADSR.h
 * @brief 4-Voice Polyphonic Envelope Subsystem (VCA, VCF, EnvDCO).
 */

#define ADSR_1_DACSIZE 4096   // DAC/PWM full scale; ctor / levelDac() export
#define ARRAY_SIZE 512        // Bézier curve LUT length (defined BEFORE library include)

// =============================================================================
// MATH BACKEND & PRECISION CONFIGURATION (STM32)
// =============================================================================
#ifndef ADSR_BEZIER_PHASE_SHIFT
#define ADSR_BEZIER_PHASE_SHIFT 24  // uint64 phase (smoother long A/D/R on STM32)
#endif
#ifndef ADSR_BEZIER_USE_FLOAT
#define ADSR_BEZIER_USE_FLOAT 0     // 0 = fixed Q24/Q16; 1 = float time index / FPU
#endif
#ifndef ADSR_BEZIER_USE_MICROS
#define ADSR_BEZIER_USE_MICROS 1    // 1 = micros timebase; 0 = millis
#endif
#ifndef ADSR_BEZIER_NATIVE_Q15
#define ADSR_BEZIER_NATIVE_Q15 1    // envelope output Q15 (0..32767)
#endif
#ifndef ADSR_BEZIER_Q15_DYADIC
#define ADSR_BEZIER_Q15_DYADIC 1    // Q15 peak 32768 (dyadic scales)
#endif
#ifndef ADSR_BEZIER_UPDATE_Q15_CACHE
#define ADSR_BEZIER_UPDATE_Q15_CACHE 1
#endif

#include "_build_libs/ADSR_Bezier/ADSR_Bezier.h"

// =============================================================================
// EXTERNAL VARIABLES (from cv_state.h)
// =============================================================================
extern uint16_t ADSR_VCA_attack;
extern uint16_t ADSR_VCA_decay;
extern uint16_t ADSR_VCA_sustain;
extern uint16_t ADSR_VCA_release;

extern uint16_t ADSR_VCF_attack;
extern uint16_t ADSR_VCF_decay;
extern uint16_t ADSR_VCF_sustain;
extern uint16_t ADSR_VCF_release;

extern bool VCAADSRRestart;
extern bool VCFADSRRestart;

// =============================================================================
// TRIGGER & MODULATION BUFFERS (PER-VOICE)
// =============================================================================
extern volatile byte noteStart[NUM_VOICES];
extern volatile byte noteEnd[NUM_VOICES];
extern volatile uint8_t note_flags[NUM_VOICES];

/** @brief Primary Q15 modulation output tap for EnvDCO / ADSR1 (per voice). */
extern int16_t ADSR1Level_q15[NUM_VOICES];

/** @brief Primary Q15 modulation output tap for EnvVCA (per voice). */
extern int16_t ADSR_VCA_Level_q15[NUM_VOICES];

/** @brief Primary Q15 modulation output tap for EnvVCF (per voice). */
extern int16_t ADSR_VCF_Level_q15[NUM_VOICES];

// =============================================================================
// SCALES & CV CONSTANTS
// =============================================================================
static constexpr uint16_t ADSR_1_CC     = 4095;
static constexpr uint16_t ADSR_CV_CC    = 4095;
static constexpr uint16_t ADSR_CV_SCALE = 4096;

extern int8_t ADSR3ToOscSelect;
extern uint8_t env_dco_pitch_centered;
static constexpr int16_t ENV_DCO_PITCH_CENTER_Q15 = 16384;

/** @brief Shifts unipolar EnvDCO tap to centered pitch domain if enabled. */
static inline int16_t env_dco_pitch_wave_q15(int16_t env_q15) {
  if (!env_dco_pitch_centered) return env_q15;
  return (int16_t)(((int32_t)env_q15 - ENV_DCO_PITCH_CENTER_Q15) << 1);
}

// EnvDCO / ADSR1 timing staging
extern uint16_t ADSR1_attack;
extern uint16_t ADSR1_decay;
extern uint16_t ADSR1_sustain;
extern uint16_t ADSR1_release;
extern bool ADSRRestart;

extern int16_t ADSR1toDETUNE1;
extern int32_t ADSR1toDETUNE1_scale_q24;
extern int16_t ADSR1toPWM;
extern int32_t ADSR1toPWM_scale;

// =============================================================================
// DIRTY FLAGS (Parameter Thread -> Audio Thread)
// =============================================================================
#define ADSR_DIRTY_DCO_A   (1u << 0)
#define ADSR_DIRTY_DCO_D   (1u << 1)
#define ADSR_DIRTY_DCO_S   (1u << 2)
#define ADSR_DIRTY_DCO_R   (1u << 3)
#define ADSR_DIRTY_VCA_A   (1u << 4)
#define ADSR_DIRTY_VCA_D   (1u << 5)
#define ADSR_DIRTY_VCA_S   (1u << 6)
#define ADSR_DIRTY_VCA_R   (1u << 7)
#define ADSR_DIRTY_VCF_A   (1u << 8)
#define ADSR_DIRTY_VCF_D   (1u << 9)
#define ADSR_DIRTY_VCF_S   (1u << 10)
#define ADSR_DIRTY_VCF_R   (1u << 11)

#define ADSR_DIRTY_DCO_ALL (ADSR_DIRTY_DCO_A | ADSR_DIRTY_DCO_D | ADSR_DIRTY_DCO_S | ADSR_DIRTY_DCO_R)
#define ADSR_DIRTY_VCA_ALL (ADSR_DIRTY_VCA_A | ADSR_DIRTY_VCA_D | ADSR_DIRTY_VCA_S | ADSR_DIRTY_VCA_R)
#define ADSR_DIRTY_VCF_ALL (ADSR_DIRTY_VCF_A | ADSR_DIRTY_VCF_D | ADSR_DIRTY_VCF_S | ADSR_DIRTY_VCF_R)

extern volatile uint16_t adsr_params_dirty;

/** @brief Marks parameters dirty for deferred application. */
static inline void mark_adsr_params_dirty(uint16_t mask) {
  adsr_params_dirty |= mask;
}

// =============================================================================
// ENVELOPE INSTANCE DECLARATIONS (12 INSTANCES: 4 VOICES x 3 ENVELOPES)
// =============================================================================

// EnvDCO / ADSR1 (Pitch / PWM Modulation)
extern adsr adsr1_voice_0;
extern adsr adsr1_voice_1;
extern adsr adsr1_voice_2;
extern adsr adsr1_voice_3;

// EnvVCA (Volume Amplitude)
extern adsr adsr_vca_voice_0;
extern adsr adsr_vca_voice_1;
extern adsr adsr_vca_voice_2;
extern adsr adsr_vca_voice_3;

// EnvVCF (Filter Cutoff)
extern adsr adsr_vcf_voice_0;
extern adsr adsr_vcf_voice_1;
extern adsr adsr_vcf_voice_2;
extern adsr adsr_vcf_voice_3;

/** @brief Struct binding all 3 envelopes belonging to a single voice channel. */
struct ADSRStruct {
  adsr& adsr1_voice;   // EnvDCO
  adsr& adsr_vca_voice;// EnvVCA
  adsr& adsr_vcf_voice;// EnvVCF
};

extern ADSRStruct ADSRVoices[];

// =============================================================================
// FUNCTION PROTOTYPES
// =============================================================================

void init_ADSR();
void ADSR_update();
void ADSR_set_parameters();

void ADSR1_set_restart();
void ADSR_VCA_set_restart();
void ADSR_VCF_set_restart();

// VCA Envelope Curve Switchers
void ADSR_VCA_change_attack_curve(uint8_t adsrCurveAttack);
void ADSR_VCA_change_decay_curve(uint8_t adsrCurveDecay);
void ADSR_VCA_change_release_curve(uint8_t adsrCurveRelease);

// VCF Envelope Curve Switchers
void ADSR_VCF_change_attack_curve(uint8_t adsrCurveAttack);
void ADSR_VCF_change_decay_curve(uint8_t adsrCurveDecay);
void ADSR_VCF_change_release_curve(uint8_t adsrCurveRelease);

// DCO Envelope Curve Switchers
void ADSR1_change_attack_curve(uint8_t adsrCurveAttack);
void ADSR1_change_decay_curve(uint8_t adsrCurveDecay);
void ADSR1_change_release_curve(uint8_t adsrCurveRelease);
void ADSR1_change_curves(uint8_t adsrCurveAttack, uint8_t adsrCurveDecay, uint8_t adsrCurveRelease);

#endif // __MAINBOARD_ADSR_H__