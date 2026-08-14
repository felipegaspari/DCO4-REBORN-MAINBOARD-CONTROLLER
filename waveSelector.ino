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
        waveSelectorMux.writePin(osc1SawPins[i], !osc1SawEnable);
      }
      break;
    case 1:
      for (int i = 0; i < 4; i++) {
        waveSelectorMux.writePin(osc2SawPins[i], !osc2SawEnable);
      }
      break;
    case 2:
      for (int i = 0; i < 4; i++) {
        waveSelectorMux.writePin(osc1TriPins[i], !osc1TriEnable);
      }
      break;
    case 3:
      for (int i = 0; i < 4; i++) {
        waveSelectorMux.writePin(osc2PulsePins[i], !osc2PulseEnable);
      }
      break;
    case 4:
      for (int i = 0; i < 4; i++) {
        waveSelectorMux.writePin(osc1SawPins[i], !osc1SawEnable);
        waveSelectorMux.writePin(osc2SawPins[i], !osc2SawEnable);
        waveSelectorMux.writePin(osc1TriPins[i], !osc1TriEnable);
        waveSelectorMux.writePin(osc2PulsePins[i], !osc2PulseEnable);
      }
      break;
    default:
      break;
  }
  waveSelectorMux.update();
}
