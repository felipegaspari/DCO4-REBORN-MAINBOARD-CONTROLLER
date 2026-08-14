#ifndef __CV_STATE_H__
#define __CV_STATE_H__

#ifndef NUM_FILTERS
#define NUM_FILTERS NUM_VOICES
#endif

// EnvVCA / EnvVCF times
uint16_t ADSR_VCA_attack = 0;
uint16_t ADSR_VCA_decay = 0;
uint16_t ADSR_VCA_sustain = 0;
uint16_t ADSR_VCA_release = 0;

uint16_t ADSR_VCF_attack = 0;
uint16_t ADSR_VCF_decay = 0;
uint16_t ADSR_VCF_sustain = 0;
uint16_t ADSR_VCF_release = 0;

bool VCAADSRRestart = true;
bool VCFADSRRestart = true;

uint8_t ADSR1AttackCurveVal = 0;
uint8_t ADSR1DecayCurveVal = 0;
uint8_t ADSR2AttackCurveVal = 0;
uint8_t ADSR2DecayCurveVal = 0;

uint16_t CUTOFF = 1024;
uint16_t RESONANCE = 0;
int16_t ADSR2toVCF = 0;
uint16_t LFO2toVCF = 0;
int16_t ADSR1toVCA = 0;
uint16_t VCALevel = 0;
uint16_t LFO1toVCA = 0;

uint16_t DIST_DRIVE = 0;
uint16_t DIST_MIX = 0;
uint8_t FILTER_MODE = 0;

int32_t ADSR2toVCF_scale_q15 = 0;
int32_t LFO2toVCF_scale_q15 = 0;
int32_t LFO1toVCA_scale_q15 = 0;
int32_t VCFKeytrackModifier_q15 = 32768;
int32_t VCFKeytrackPerVoice_q15[NUM_VOICES];  // 32768 in init_cv_out (BSS, not FLASH .data)
int32_t velocityToVCF_q15 = 0;
int32_t velocityToVCA_q15 = 0;
int32_t vcf_drift_scale_q15 = 0;
volatile int16_t VCF_DRIFT[NUM_VOICES];

bool RESONANCEAmpCompensation = true;
int16_t VCAResonanceCompensation = 100;
int16_t VCFKeytrack = 0;
int8_t velocityToVCFVal = 0;
int8_t velocityToVCAVal = 0;

uint16_t VCA_PWM[NUM_VOICES];
uint16_t VCF_PWM[NUM_VOICES];
uint16_t RESONANCE_PWM[NUM_FILTERS];

uint16_t AS2164_VCA_linearize_table[4096];

const uint16_t lin_to_log_128[129] = { 4095, 2040, 1702, 1593, 1504, 1428, 1363, 1305, 1254, 1207, 1164, 1125, 1088, 1054, 1023, 993, 965, 938, 913, 889, 866, 844, 823, 803, 784, 765, 748, 730, 714, 698, 682, 667, 652, 638, 624, 610, 597, 585, 572, 560, 548, 537, 525, 514, 503, 493, 482, 472, 462, 453, 443, 434, 424, 415, 407, 398, 389, 381, 373, 365, 357, 349, 341, 333, 326, 318, 311, 304, 297, 290, 283, 276, 269, 263, 256, 250, 244, 237, 231, 225, 219, 213, 207, 201, 195, 190, 184, 179, 173, 168, 162, 157, 152, 146, 141, 136, 131, 126, 121, 116, 112, 107, 102, 97, 93, 88, 83, 79, 74, 70, 65, 61, 57, 52, 48, 44, 40, 36, 32, 27, 23, 19, 15, 11, 8, 6, 4, 2, 0 };

int16_t OSC1LevelVal = 0;
int16_t OSC2LevelVal = 0;
int16_t OSC3LevelVal = 0;
int16_t SubLevelVal = 0;
uint16_t OSC1Level = 0;
uint16_t OSC2Level = 0;
uint16_t OSC3Level = 0;
uint16_t SubLevel = 0;

// A level is the whole oscillator, every wave of it at once; the old SQR1Level /
// SQR2Level names for these MCP4728 channels described a square-only level that
// the wiring never had, so they are gone.

bool ADSR3Enabled = false;

#endif
