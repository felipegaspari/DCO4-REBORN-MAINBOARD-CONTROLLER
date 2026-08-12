#ifndef __AUX_H__
#define __AUX_H__

// Bézier VCA linearize lives in cv_bezier.h.

// Match Input/DCO wire curve: linearToExponential(v, 50, 25000). Used to invert
// exp-mapped ADSR times before they go to the Screen (bars expect 0..4095).
#define LIN_TO_EXP_TABLE_SIZE 4096
extern uint16_t linToExpLookup[LIN_TO_EXP_TABLE_SIZE];
static constexpr uint16_t maxADSRControlValue = 25000;

void init_aux();

// Lin→exp mapping for ADSR fader lookup table (filled in init_aux).
static inline uint16_t linearToExponential(uint16_t linearValue, float base, uint16_t maxValue) {

  if (linearValue < 0) linearValue = 0;
  if (linearValue > 4095) linearValue = 4095;

  float normalizedValue = (float)linearValue / 4095.0;
  float expValue = pow(base, normalizedValue) - 1;
  float maxExpValue = pow(base, 1.0) - 1;
  uint16_t scaledExpValue = (uint16_t)(expValue * (maxValue / maxExpValue));

  return scaledExpValue;
}

// Inverse of linToExpLookup[] (monotonic binary search).
static inline uint16_t exp_to_lin_index(uint16_t expValue) {
  uint16_t lo = 0;
  uint16_t hi = LIN_TO_EXP_TABLE_SIZE - 1;

  if (expValue <= linToExpLookup[lo]) return lo;
  if (expValue >= linToExpLookup[hi]) return hi;

  while ((uint16_t)(hi - lo) > 1) {
    uint16_t mid = (uint16_t)((lo + hi) >> 1);
    if (linToExpLookup[mid] <= expValue) {
      lo = mid;
    } else {
      hi = mid;
    }
  }

  uint16_t belowGap = (uint16_t)(expValue - linToExpLookup[lo]);
  uint16_t aboveGap = (uint16_t)(linToExpLookup[hi] - expValue);
  return (aboveGap < belowGap) ? hi : lo;
}

#endif
