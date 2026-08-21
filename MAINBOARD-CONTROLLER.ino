#define NUM_VOICES 4          // voice array sizes
#define NUM_VOICES_TOTAL 4
#ifndef NUM_FILTERS
#define NUM_FILTERS 4         // filter CV array size
#endif

//#define ENABLE_SD             // SDMMC pin reservation; preset store still commented

#define RUNNING_AVERAGE       // slim bench.h profiler; dco_control 40/41/42 + dump 45 + MCP 43/44
// #define MB_UART_PROBE      // 1 Hz 't' labels on Serial/1/2/8; Board shows mb s2 if DCO is on USART2 PD5; desyncs Input/Screen if plugged in
// #define MB_UART_RX_LOG        // USB CDC one line per UART cmd byte (s2/s8); s1 logs byte count (no parser)
// #define ENABLE_MB_MOD_STREAM  // MB→DCO 'm' @ 1 kHz; also define on DCO to consume it
#define ENABLE_MCP4728        // three I2C DACs on this board; dco_control opcodes 43/44
#define MCP_I2C_DMA 1  // 0 = I2C IT (pre-DMA), 1 = I2C TX DMA
#ifndef BENCH_STAGE_STRIDE
#define BENCH_STAGE_STRIDE 1  // profiler: sample every N loops
#endif
#ifndef BENCH_PERIOD_MAX_US
#define BENCH_PERIOD_MAX_US 20000000  // 20 s; keep I2C/DAC stalls in loop period
#endif

// DMA implementation -- uncomment to enable
#define DCO_PROTOCOL_IMPLEMENT_DMA
////////////////////////////////////////////////

#include "Arduino.h"
#include "include_all.h"

#ifdef ENABLE_SPI
#include "SPI_settings.h"
#endif

void setup() {
  init_aux();
  init_timers();
  init_LFOs();
  init_DRIFT_LFOs();
  init_ADSR();
  init_cv_out();
  init_param_router();
  init_serial_parsers();
  mod_matrix_init();
  init_waveSelector();
  init_MCP4728();
  bench_init();

#ifdef ENABLE_SERIAL
  Serial.begin(2000000);
#endif
#ifdef ENABLE_SERIAL1
  Serial1.setRx(MB_SERIAL1_RX);
  Serial1.setTx(MB_SERIAL1_TX);
  Serial1.begin(2500000);
#endif
#ifdef ENABLE_SERIAL2
  Serial2.setRx(MB_SERIAL2_RX);
  Serial2.setTx(MB_SERIAL2_TX);
  Serial2.begin(2500000);
#endif
#ifdef ENABLE_SERIAL8
  Serial8.setRx(MB_SERIAL8_RX);
  Serial8.setTx(MB_SERIAL8_TX);
  Serial8.begin(2500000);
#endif
  serial_dma_init();

  noteStart[0] = 0;
  noteEnd[0] = 1;
  VCFKeytrack = 0;
}

void loop() {
  BENCH_PERIOD(loop_period);
  BENCH_SAMPLE_TICK();

  {
    BENCH_BEGIN(millisTimer);
    millisTimer();
    BENCH_END(millisTimer);
  }

  if (timer223microsFlag) {
    BENCH_BEGIN(serial_1_8);
    read_serial_1();
    read_serial_8();
    BENCH_END(serial_1_8);
#ifdef ENABLE_SERIAL8
  } else if (Serial8.available() > 0) {
    BENCH_BEGIN(serial_1_8);
    read_serial_8();
    BENCH_END(serial_1_8);
#endif
  }


  // no-op
  // if (timer1msFlag) {
  //   BENCH_BEGIN(sendSerial);
  //   sendSerial();
  //   mb_uart_probe_poll();
  //   BENCH_END(sendSerial);
  // }

  if (Serial2.available() > 0) {
    BENCH_BEGIN(serial_2);
    read_serial_2();
    BENCH_END(serial_2);
  }


  {
    BENCH_BEGIN(lfos);
    LFO1();
    LFO2();
    DRIFT_LFOs();
    BENCH_END(lfos);
  }
 
 {
    BENCH_BEGIN(adsr);
    ADSR_update();
    BENCH_END(adsr);
 }
 
 {
    BENCH_BEGIN(cv_outs);
    if (!manualCalibrationFlag) {
      update_CV_outs();
    } else {
      update_CV_outs_manual_calibration();
    }
    BENCH_END(cv_outs);
  }

  bench_poll();
  serial_dma_poll();
}
