#ifndef SERIAL_DMA_H
#define SERIAL_DMA_H

#include <stddef.h>
#include <stdint.h>

// UART TX DMA for Serial1 (Screen), Serial2 (DCO), and Serial8 (Input).
// DMA1 Stream0–2; Stream3 is I2C1 TX for MCP4728 (MCP4728.ino).
// stm32duino keeps RX IRQ on those UARTs. Do not Serial1/2/8.write() after
// serial_dma_init(). USB Serial is unchanged.

void serial_dma_init();
void serial_dma_poll();

struct UartDmaTx {
  uint8_t id;
  size_t write(const uint8_t *p, size_t n);
  size_t write_blocking(const uint8_t *p, size_t n);
  size_t availableForWrite();
};

extern UartDmaTx ScreenDma;
extern UartDmaTx DcoDma;
extern UartDmaTx InputDma;

#endif
