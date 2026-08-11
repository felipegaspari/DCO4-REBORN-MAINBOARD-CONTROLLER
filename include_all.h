#ifndef __INCLUDE_ALL_H__
#define __INCLUDE_ALL_H__

#include "Arduino.h"
// Relative _build_libs paths: Arduino IDE does not put sketch libraries/ on the
// STM32 include path reliably (same pattern as ADSR / RoxMux on this board).
#include "_build_libs/DCO-PROTOCOL/params_def.h"
#include "_build_libs/DCO-PROTOCOL/param_router.h"
#include "params.h"
#include "cv_bezier.h"
#include "cv_state.h"
#include "cv_out.h"
#include "mod_matrix.h"
#include "auxiliary.h"
#include "PWM.h"
#include "ADSR.h"
#include "LFO.h"
#include "Timers.h"
#include "Timers_millis.h"
#include "Serial.h"
#include "bench.h"
#include "waveSelector.h"

#endif
