#include "include_all.h"

// --- Transmit to DCO Engine (Serial2) ---
void serialSendParamToDCO(uint8_t id, int16_t value) {
  transmit_param16(DcoDma, id, value);
}

void serialSendParam32ToDCO(uint8_t id, uint32_t value) {
  transmit_param32(DcoDma, id, value);
}

// --- Transmit to Input Controller (Serial8) ---
void serialSendParam16ToInput(uint8_t id, int16_t value) {
  transmit_param16(InputDma, id, value);
}

void serialSendParam32ToInput(uint8_t id, uint32_t value) {
  transmit_param32(InputDma, id, value);
}

// --- Transmit to Screen Controller (Serial1) ---
void serialSendParam16ToScreen(uint8_t id, int16_t value) {
  transmit_param16(ScreenDma, id, value);
}

void serialSendParam32ToScreen(uint8_t id, uint32_t value) {
  transmit_param32(ScreenDma, id, value);
}