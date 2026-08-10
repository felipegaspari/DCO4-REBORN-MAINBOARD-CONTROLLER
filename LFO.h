#ifndef __LFO_H__
#define __LFO_H__

#ifndef MO_LFO_USE_Q15
#define MO_LFO_USE_Q15 1
#endif

#include "_build_libs/mo-lfo/mo-lfo.h"

static constexpr int32_t LFO1_PITCH_DEPTH_SCALE = 1700;
static constexpr int32_t LFO2_PITCH_DEPTH_SCALE = 512;
static constexpr float ADSR_PITCH_MAX_OCTAVES = 2.0f;
static constexpr uint16_t ADSR_PITCH_DEPTH_PANEL_FULL = 511;
static constexpr int32_t DRIFT_PITCH_DEPTH_SCALE = 1000;
static constexpr int32_t DRIFT_PITCH_UNIT_Q24 =
  (int32_t)(0.0000005f * (float)(1 << 24) + 0.5f);

static inline int32_t lfo_pitch_depth_q24(float amt, int32_t depth_scale) {
  return (int32_t)(amt * (float)depth_scale * (float)(1 << 24) + 0.5f);
}

static inline int32_t applyDepthQ24(int16_t wave_q15, int32_t depth_q24) {
  const int32_t w = (int32_t)wave_q15;
  const int32_t hi = depth_q24 >> 15;
  const int32_t lo = depth_q24 - (hi << 15);
  return w * hi + ((w * lo) >> 15);
}

static constexpr int LFO_DAC_SIZE_UNUSED = 1;

int32_t drift_pitch_scale_q24 = 0;

lfo LFO1_class(LFO_DAC_SIZE_UNUSED);
lfo LFO2_class(LFO_DAC_SIZE_UNUSED);

lfo LFO_DRIFT_CLASS[NUM_VOICES] = {
  lfo(LFO_DAC_SIZE_UNUSED),
  lfo(LFO_DAC_SIZE_UNUSED),
  lfo(LFO_DAC_SIZE_UNUSED),
  lfo(LFO_DAC_SIZE_UNUSED)
};

byte LFO_DRIFT_WAVEFORM = 2;
float LFO_DRIFT_SPEED_OFFSET[NUM_VOICES];
volatile int16_t LFO_DRIFT_LEVEL[NUM_VOICES];

volatile int16_t LFO1Level;
byte LFO1Waveform = 2;
float LFO1Speed = 0.5f;
int32_t LFO1toDCO_q24 = 0;

volatile int16_t LFO2Level;
byte LFO2Waveform = 2;
float LFO2Speed = 5.0f;
volatile uint16_t LFO2toPW;

uint16_t LFO1SpeedVal;
uint16_t LFO2SpeedVal;
uint16_t LFO1toDCOVal;

void init_LFOs();
void init_DRIFT_LFOs();
void LFO1();
void LFO2();
void DRIFT_LFOs();

#endif
