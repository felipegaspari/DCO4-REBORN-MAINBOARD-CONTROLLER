uint16_t linToExpLookup[LIN_TO_EXP_TABLE_SIZE];

// Boot: fill lin→exp table so Screen ADSR mirrors can invert wire times.
void init_aux() {
  for (int i = 0; i < LIN_TO_EXP_TABLE_SIZE; i++) {
    linToExpLookup[i] = linearToExponential((uint16_t)i, 50.0f, maxADSRControlValue);
  }
}

// Squared float curve used by LFO speed/depth control formulas.
float expConverterFloat(uint16_t readingValue, uint16_t curve) {
  uint16_t pow3Calc = readingValue;
  float expValOut = (float)pow3Calc * pow3Calc / curve;
  if (expValOut < 0.005) {
    expValOut = 0;
  }
  return expValOut;
}
