#ifndef __WAVESELECTOR_H__
#define __WAVESELECTOR_H__

#include "_build_libs/RoxMux_fela/src/RoxMux_fela.h"

Rox74HC595<2> waveSelectorMux;

// pins for 74HC595
// #define PIN_DATA    PE_4 // pin 14 on 74HC595 (DATA)   / DS
// #define PIN_LATCH   PE_2  // pin 12 on 74HC595 (LATCH) / ST
// #define PIN_CLK     PE_3  // pin 11 on 74HC595 (CLK)   / SH
#define PIN_DATA PE4   // pin 14 on 74HC595 (DATA)   / DS
#define PIN_LATCH PE2  // pin 12 on 74HC595 (LATCH) / ST
#define PIN_CLK PE3    // pin 11 on 74HC595 (CLK)   / SH
#define PIN_PWM -1

// Crossed cables. Active-low (write 0 = switch closed). Voices 0..3.
// There is no OSC2 triangle bit; OSC1 pulse has no switch either — it rides its
// oscillator's level channel, so only the DCO's PW CV for that voice can silence it.
uint8_t osc1TriPins[4] = { 14, 10, 6, 2 };
uint8_t osc2PulsePins[4] = { 13, 9, 5, 1 };
uint8_t osc2SawPins[4] = { 12, 8, 4, 0 };
uint8_t osc1SawPins[4] = { 15, 11, 7, 3 };

#endif
