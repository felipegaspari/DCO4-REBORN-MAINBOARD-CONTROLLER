#ifndef __SERIAL_H__
#define __SERIAL_H__

// #define ENABLE_SERIAL
#define ENABLE_SERIAL1
#define ENABLE_SERIAL2
#define ENABLE_SERIAL8

// 17 = the 'O' preset-directory entry ([slot:u8][name:16]) the Mainboard relays
// DCO → Input; everything else it handles tops out at 16 ('m'/'t'). Must be set
// before serial_frame.h locks its default.
#ifndef SERIAL_INNER_MAX_PAYLOAD
#define SERIAL_INNER_MAX_PAYLOAD 17
#endif

#include "_build_libs/DCO-PROTOCOL/serial_param_protocol.h"
#include "serial_protocol.h"
#include "_build_libs/DCO-PROTOCOL/serial_input_protocol.h"
#include "_build_libs/DCO-PROTOCOL/serial_frame.h"
#include "_build_libs/DCO-PROTOCOL/serial_parser.h"

// STM32 core 3.x declares `extern Uart SerialN` but only *defines* them if
// ENABLE_HWSERIALn reaches core Serial.cpp (often cached, so build_opt.h is
// unreliable). Provide the objects here as Uart (not HardwareSerial).
#define MB_SERIAL1_RX PA10  // Screen
#define MB_SERIAL1_TX PA9
#define MB_SERIAL2_RX PD6   // DCO
#define MB_SERIAL2_TX PD5
#define MB_SERIAL8_RX PE0   // Input
#define MB_SERIAL8_TX PE1

#ifdef ENABLE_SERIAL1
Uart Serial1(MB_SERIAL1_RX, MB_SERIAL1_TX);
#endif
#ifdef ENABLE_SERIAL2
Uart Serial2(MB_SERIAL2_RX, MB_SERIAL2_TX);
#endif
#ifdef ENABLE_SERIAL8
Uart Serial8(MB_SERIAL8_RX, MB_SERIAL8_TX);
#endif

void init_serial_parsers();
void read_serial_1();
void read_serial_2();
void read_serial_8();
void sendSerial();

void serialSendParamToDCO(uint8_t id, int16_t value);
void serialSendParam32ToDCO(uint8_t id, uint32_t value);
void serialSendParam32ToInput(uint8_t id, uint32_t value);
void serialSendParam16ToInput(uint8_t id, int16_t value);
void serialSendParam32ToScreen(uint8_t id, uint32_t value);
void serial_send_bench_text_on(Stream& port, const uint8_t* data, uint8_t n);
void serial_send_bench_text_chunk(const uint8_t* data, uint8_t n);
#ifdef MB_UART_PROBE
void mb_uart_probe_poll();
#else
static inline void mb_uart_probe_poll() {}
#endif

#endif
