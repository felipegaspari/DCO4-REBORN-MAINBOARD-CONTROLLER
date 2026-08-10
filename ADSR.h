#ifndef __ADSR_DCO_H__
#define __ADSR_DCO_H__

#define ADSR_1_DACSIZE 4096   // DAC/PWM full scale; ctor / levelDac() export
#define ARRAY_SIZE 512        // Bézier curve LUT length; RAM vs resolution

#ifndef ADSR_BEZIER_PHASE_SHIFT
#define ADSR_BEZIER_PHASE_SHIFT 24  // uint64 phase (smoother long A/D/R; 22 = faster uint32)
#endif
#ifndef ADSR_BEZIER_USE_FLOAT
#define ADSR_BEZIER_USE_FLOAT 0     // 0 = fixed Q24/Q16 (STM32); 1 = float time index / FPU
#endif
#ifndef ADSR_BEZIER_USE_MICROS
#define ADSR_BEZIER_USE_MICROS 1    // 1 = micros timebase; 0 = millis
#endif
#ifndef ADSR_BEZIER_NATIVE_Q15
#define ADSR_BEZIER_NATIVE_Q15 1    // envelope output Q15 (not DAC counts)
#endif
#ifndef ADSR_BEZIER_Q15_DYADIC
#define ADSR_BEZIER_Q15_DYADIC 1    // Q15 peak 32768 (<<1 scales; 0 = peak 32767)
#endif

// Ignored when NATIVE_Q15=1 (primary output is already Q15).
#ifndef ADSR_BEZIER_UPDATE_Q15_CACHE
#define ADSR_BEZIER_UPDATE_Q15_CACHE 1
#endif

#include "_build_libs/ADSR_Bezier/ADSR_Bezier.h"

volatile byte noteStart[NUM_VOICES];
volatile byte noteEnd[NUM_VOICES];

int16_t ADSR1Level_q15[NUM_VOICES];       // EnvDCO
int16_t ADSR_VCA_Level_q15[NUM_VOICES];
int16_t ADSR_VCF_Level_q15[NUM_VOICES];

static constexpr uint16_t ADSR_1_CC = 4095;
static constexpr uint16_t ADSR_CV_CC = 4095;
static constexpr uint16_t ADSR_CV_SCALE = 4096;

int8_t ADSR3ToOscSelect = 2;
uint8_t env_dco_pitch_centered = 0;
static constexpr int16_t ENV_DCO_PITCH_CENTER_Q15 = 16384;

static inline int16_t env_dco_pitch_wave_q15(int16_t env_q15) {
  if (!env_dco_pitch_centered)
    return env_q15;
  return (int16_t)(((int32_t)env_q15 - ENV_DCO_PITCH_CENTER_Q15) << 1);
}

uint16_t ADSR1_attack = 0;
uint16_t ADSR1_decay = 0;
uint16_t ADSR1_sustain = 4095;
uint16_t ADSR1_release = 0;

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

volatile uint16_t adsr_params_dirty = 0;

static inline void mark_adsr_params_dirty(uint16_t mask) {
  adsr_params_dirty |= mask;
}

float ADSR1_curve1 = 0.999f;
float ADSR1_curve2 = 0.997f;
float ADSR_VCA_curve1 = 0.9995f;
float ADSR_VCA_curve2 = 0.9995f;
float ADSR_VCF_curve1 = 0.997f;
float ADSR_VCF_curve2 = 0.997f;

bool ADSRRestart = true;

int16_t ADSR1toDETUNE1;
int32_t ADSR1toDETUNE1_scale_q24;
int16_t ADSR1toPWM;
int32_t ADSR1toPWM_scale = 0;

adsr adsr1_voice_0(ADSR_CV_CC, ADSR1_curve1, ADSR1_curve2, false, 7, 7, 7);
adsr adsr1_voice_1(ADSR_CV_CC, ADSR1_curve1, ADSR1_curve2, false, 7, 7, 7);
adsr adsr1_voice_2(ADSR_CV_CC, ADSR1_curve1, ADSR1_curve2, false, 7, 7, 7);
adsr adsr1_voice_3(ADSR_CV_CC, ADSR1_curve1, ADSR1_curve2, false, 7, 7, 7);

adsr adsr_vca_voice_0(ADSR_CV_CC, ADSR_VCA_curve1, ADSR_VCA_curve2, false, 1, 2, 1);
adsr adsr_vca_voice_1(ADSR_CV_CC, ADSR_VCA_curve1, ADSR_VCA_curve2, false, 1, 2, 1);
adsr adsr_vca_voice_2(ADSR_CV_CC, ADSR_VCA_curve1, ADSR_VCA_curve2, false, 1, 2, 1);
adsr adsr_vca_voice_3(ADSR_CV_CC, ADSR_VCA_curve1, ADSR_VCA_curve2, false, 1, 2, 1);

adsr adsr_vcf_voice_0(ADSR_CV_CC, ADSR_VCF_curve1, ADSR_VCF_curve2, false, 4, 6, 1);
adsr adsr_vcf_voice_1(ADSR_CV_CC, ADSR_VCF_curve1, ADSR_VCF_curve2, false, 4, 6, 1);
adsr adsr_vcf_voice_2(ADSR_CV_CC, ADSR_VCF_curve1, ADSR_VCF_curve2, false, 4, 6, 1);
adsr adsr_vcf_voice_3(ADSR_CV_CC, ADSR_VCF_curve1, ADSR_VCF_curve2, false, 4, 6, 1);

struct ADSRStruct {
  adsr adsr1_voice;
  adsr adsr_vca_voice;
  adsr adsr_vcf_voice;
};

ADSRStruct ADSRVoices[] = {
  { adsr1_voice_0, adsr_vca_voice_0, adsr_vcf_voice_0 },
  { adsr1_voice_1, adsr_vca_voice_1, adsr_vcf_voice_1 },
  { adsr1_voice_2, adsr_vca_voice_2, adsr_vcf_voice_2 },
  { adsr1_voice_3, adsr_vca_voice_3, adsr_vcf_voice_3 },
};

void init_ADSR();
void ADSR_update();
void ADSR_set_parameters();
void ADSR1_set_restart();
void ADSR_VCA_set_restart();
void ADSR_VCF_set_restart();
void ADSR_VCA_change_attack_curve(uint8_t adsrCurveAttack);
void ADSR_VCA_change_decay_curve(uint8_t adsrCurveDecay);
void ADSR_VCF_change_attack_curve(uint8_t adsrCurveAttack);
void ADSR_VCF_change_decay_curve(uint8_t adsrCurveDecay);

#endif
