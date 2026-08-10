void init_waveSelector() {
  pinMode(PIN_LATCH, OUTPUT);
  pinMode(PIN_DATA, OUTPUT);
  pinMode(PIN_CLK, OUTPUT);
  waveSelectorMux.begin(PIN_DATA, PIN_LATCH, PIN_CLK, PIN_PWM);
  waveSelectorMux.setBrightness(255);
  waveSelectorMux.allOff();
}

void update_waveSelector(byte wave) {
  switch (wave) {
    case 0:
      for (int i = 0; i < 4; i++) {
        waveSelectorMux.writePin(sawPins[i], !osc1SawEnable);
        waveSelectorMux.writePin(saw2Pins[i], !osc2SawEnable);
      }
      break;
    case 1:
      for (int i = 0; i < 4; i++) {
        waveSelectorMux.writePin(saw2Pins[i], !osc2SawEnable);
      }
      break;
    case 2:
      for (int i = 0; i < 4; i++) {
        waveSelectorMux.writePin(triPins[i], !osc1TriEnable);
      }
      break;
    case 3:
      for (int i = 0; i < 4; i++) {
        waveSelectorMux.writePin(sinePins[i], !osc2PulseEnable);
      }
      break;
    case 4:
      for (int i = 0; i < 4; i++) {
        waveSelectorMux.writePin(sawPins[i], !osc1SawEnable);
        waveSelectorMux.writePin(saw2Pins[i], !osc2SawEnable);
        waveSelectorMux.writePin(triPins[i], !osc1TriEnable);
        waveSelectorMux.writePin(sinePins[i], !osc2PulseEnable);
      }
      break;
    default:
      break;
  }
  waveSelectorMux.update();
}
