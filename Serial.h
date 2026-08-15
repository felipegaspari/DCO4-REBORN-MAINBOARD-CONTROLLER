#ifndef __SERIAL_H__
#define __SERIAL_H__

#define ENABLE_SERIAL1
#define ENABLE_SERIAL2
#define ENABLE_SERIAL8

#ifndef SERIAL_INNER_MAX_PAYLOAD
#define SERIAL_INNER_MAX_PAYLOAD 36
#endif

// Shared Protocol Includes
#include "_build_libs/DCO-PROTOCOL/serial_param_protocol.h"
#include "_build_libs/DCO-PROTOCOL/serial_input_protocol.h"
#include "_build_libs/DCO-PROTOCOL/serial_frame.h"
#include "_build_libs/DCO-PROTOCOL/serial_parser.h"
#include "_build_libs/DCO-PROTOCOL/serial_dma_tx.h" 

#define MB_SERIAL1_RX PA_10_ALT1  // Screen RX  (USART1_RX)
#define MB_SERIAL1_TX PA_9_ALT1   // Screen TX  (USART1_TX)
#define MB_SERIAL2_RX PD_6        // DCO
#define MB_SERIAL2_TX PD_5
#define MB_SERIAL8_RX PE_0        // Input RX  (UART8_RX)
#define MB_SERIAL8_TX PE_1        // Input TX  (UART8_TX)

#ifdef ENABLE_SERIAL1
inline Uart Serial1(MB_SERIAL1_RX, MB_SERIAL1_TX);
#endif
#ifdef ENABLE_SERIAL2
inline Uart Serial2(MB_SERIAL2_RX, MB_SERIAL2_TX);
#endif
#ifdef ENABLE_SERIAL8
inline Uart Serial8(MB_SERIAL8_RX, MB_SERIAL8_TX);
#endif

extern UartDmaTx ScreenDma;
extern UartDmaTx DcoDma;
extern UartDmaTx InputDma;

void init_serial_parsers();
void read_serial_1();
void read_serial_2();
void read_serial_8();
void sendSerial();

void serialSendParamToDCO(uint8_t id, int16_t value);
void serialSendParam32ToDCO(uint8_t id, uint32_t value);
void serialSendParam32ToInput(uint8_t id, uint32_t value);
void serialSendParam16ToInput(uint8_t id, int16_t value);
void serialSendParam16ToScreen(uint8_t id, int16_t value);
void serialSendParam32ToScreen(uint8_t id, uint32_t value);
void serial_send_bench_text_chunk(const uint8_t* data, uint8_t n);

#ifdef MB_UART_PROBE
void mb_uart_probe_poll();
#else
static inline void mb_uart_probe_poll() {}
#endif

#endif