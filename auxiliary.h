#ifndef __AUX_H__
#define __AUX_H__

// Bézier VCA linearize lives in cv_bezier.h.

// Lin→exp mapping for ADSR fader lookup table (filled in setup).
static inline uint16_t linearToExponential(uint16_t linearValue, float base, uint16_t maxValue) {

  if (linearValue < 0) linearValue = 0;
  if (linearValue > 4095) linearValue = 4095;

  float normalizedValue = (float)linearValue / 4095.0;
  float expValue = pow(base, normalizedValue) - 1;
  float maxExpValue = pow(base, 1.0) - 1;
  uint16_t scaledExpValue = (uint16_t)(expValue * (maxValue / maxExpValue));

  return scaledExpValue;
}

#endif
