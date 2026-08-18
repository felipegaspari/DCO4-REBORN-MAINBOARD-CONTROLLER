uint16_t linToExpLookup[LIN_TO_EXP_TABLE_SIZE];

// Boot: fill lin→exp table so Screen ADSR mirrors can invert wire times.
void init_aux() {
  for (int i = 0; i < LIN_TO_EXP_TABLE_SIZE; i++) {
    linToExpLookup[i] = linearToExponential((uint16_t)i, 50.0f, maxADSRControlValue);
  }
}
