#ifndef __PARAMS_H__
#define __PARAMS_H__

#ifndef NUM_VOICES
#define NUM_VOICES 4
#endif
#ifndef NUM_VOICES_TOTAL
#define NUM_VOICES_TOTAL NUM_VOICES
#endif
#ifndef NUM_FILTERS
#define NUM_FILTERS NUM_VOICES
#endif

byte note[NUM_VOICES];
byte velocity[NUM_VOICES];
byte midi_velocity[NUM_VOICES];
#define VOICE_NOTES note

uint8_t note_flags[NUM_VOICES];

byte currentVoice = 0;

bool osc1SawEnable = false;
bool osc1PulseEnable = false;
bool osc1TriEnable = false;
bool osc2SawEnable = false;
bool osc2PulseEnable = false;
bool osc2TriEnable = false;
bool sineStatus = false;

byte PWMPots = 0;
bool PWMPotsControlManual = false;

int16_t unisonDetune;
int8_t analogDrift;
int16_t analogDriftSpeed;
int8_t analogDriftSpread;

bool calibrationFlag = false;
bool manualCalibrationFlag = false;
int8_t manualCalibrationStage = 0;

uint16_t aftertouch = 0;
uint8_t mod_wheel_in = 0;
uint16_t midi_pitch_bend = 8192;
byte portamentoTime = 0;
byte portamentoMode = 0;
uint16_t oscSyncMode = 0;
byte voiceMode = 0;

byte OSC1Interval = 24;
byte OSC2Interval = 24;
uint16_t OSC2Detune = 255;
uint16_t PW = 0;

byte presetName[12] = { 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32 };

void update_parameters(uint16_t paramNumber, int16_t paramValue);
void init_param_router();

#endif
