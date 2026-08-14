#include "include_all.h"
#include <string.h>

// Ping-pong so a 1 ms ADSR/filter burst can queue while the previous transfer
// drains. 256 B holds several stuffed frames (max inner payload is 17).
static constexpr uint16_t SERIAL_DMA_BUF_SIZE = 256;
static constexpr uint8_t SERIAL_DMA_ENGINES = 3;

struct SerialDmaEngine {
  USART_TypeDef *uart;
  DMA_HandleTypeDef hdma;
  alignas(32) uint8_t buf[2][SERIAL_DMA_BUF_SIZE];
  uint16_t len[2];
  uint8_t fill;
  bool sending;
};

static SerialDmaEngine serial_dma_eng[SERIAL_DMA_ENGINES];

UartDmaTx ScreenDma = { 0 };
UartDmaTx DcoDma = { 1 };
UartDmaTx InputDma = { 2 };

static void serial_dma_clean(uint8_t *p, uint16_t n) {
  if (n == 0) {
    return;
  }
  const uint32_t n_clean = ((uint32_t)n + 31u) & ~31u;
  SCB_CleanDCache_by_Addr(reinterpret_cast<uint32_t *>(p), (int32_t)n_clean);
  __DSB();
}

static void serial_dma_poll_one(uint8_t i) {
  SerialDmaEngine &e = serial_dma_eng[i];
  if (e.uart == nullptr) {
    return;
  }
  if (e.sending) {
    DMA_Stream_TypeDef *stream = reinterpret_cast<DMA_Stream_TypeDef *>(e.hdma.Instance);
    if ((stream->CR & DMA_SxCR_EN) != 0U) {
      return;
    }
    if (HAL_DMA_PollForTransfer(&e.hdma, HAL_DMA_FULL_TRANSFER, 0) != HAL_OK) {
      (void)HAL_DMA_Abort(&e.hdma);
    }
    e.sending = false;
  }
  const uint16_t count = e.len[e.fill];
  if (count == 0) {
    return;
  }
  const uint8_t send = e.fill;
  e.len[send] = 0;
  e.fill ^= 1u;
  serial_dma_clean(e.buf[send], count);
  if (HAL_DMA_Start(&e.hdma, reinterpret_cast<uint32_t>(e.buf[send]),
                    reinterpret_cast<uint32_t>(&e.uart->TDR), count) != HAL_OK) {
    e.len[send] = count;
    e.fill = send;
    return;
  }
  e.sending = true;
}

static void serial_dma_disable_uart_tx_irq(USART_TypeDef *uart) {
  CLEAR_BIT(uart->CR1, USART_CR1_TCIE);
#ifdef USART_CR1_TXEIE_TXFNFIE
  CLEAR_BIT(uart->CR1, USART_CR1_TXEIE_TXFNFIE);
#elif defined(USART_CR1_TXEIE)
  CLEAR_BIT(uart->CR1, USART_CR1_TXEIE);
#endif
#ifdef USART_CR1_TXFTIE
  CLEAR_BIT(uart->CR1, USART_CR1_TXFTIE);
#endif
}

static void serial_dma_init_one(uint8_t i, USART_TypeDef *uart, DMA_Stream_TypeDef *stream,
                                uint32_t request) {
  SerialDmaEngine &e = serial_dma_eng[i];
  e.uart = nullptr;
  e.len[0] = 0;
  e.len[1] = 0;
  e.fill = 0;
  e.sending = false;
  if (uart == nullptr || stream == nullptr) {
    return;
  }

  memset(&e.hdma, 0, sizeof(e.hdma));
  e.hdma.Instance = stream;
  e.hdma.Init.Request = request;
  e.hdma.Init.Direction = DMA_MEMORY_TO_PERIPH;
  e.hdma.Init.PeriphInc = DMA_PINC_DISABLE;
  e.hdma.Init.MemInc = DMA_MINC_ENABLE;
  e.hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  e.hdma.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  e.hdma.Init.Mode = DMA_NORMAL;
  e.hdma.Init.Priority = DMA_PRIORITY_LOW;
  e.hdma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  e.hdma.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
  e.hdma.Init.MemBurst = DMA_MBURST_SINGLE;
  e.hdma.Init.PeriphBurst = DMA_PBURST_SINGLE;
  if (HAL_DMA_Init(&e.hdma) != HAL_OK) {
    return;
  }

  serial_dma_disable_uart_tx_irq(uart);
  SET_BIT(uart->CR3, USART_CR3_DMAT);
  e.uart = uart;
}

static size_t serial_dma_write_one(uint8_t i, const uint8_t *p, size_t n) {
  SerialDmaEngine &e = serial_dma_eng[i];
  if (e.uart == nullptr || p == nullptr || n == 0) {
    return 0;
  }
  if (n > SERIAL_DMA_BUF_SIZE) {
    return 0;
  }
  serial_dma_poll_one(i);
  if ((size_t)e.len[e.fill] + n > SERIAL_DMA_BUF_SIZE) {
    if (!e.sending && e.len[e.fill] > 0) {
      serial_dma_poll_one(i);
    }
    if ((size_t)e.len[e.fill] + n > SERIAL_DMA_BUF_SIZE) {
      return 0;
    }
  }
  memcpy(e.buf[e.fill] + e.len[e.fill], p, n);
  e.len[e.fill] = (uint16_t)(e.len[e.fill] + n);
  serial_dma_poll_one(i);
  return n;
}

static size_t serial_dma_available_for_write(uint8_t i) {
  SerialDmaEngine &e = serial_dma_eng[i];
  if (e.uart == nullptr) {
    return 0;
  }
  serial_dma_poll_one(i);
  return (size_t)(SERIAL_DMA_BUF_SIZE - e.len[e.fill]);
}

void serial_dma_init() {
  __HAL_RCC_DMA1_CLK_ENABLE();
#ifdef ENABLE_SERIAL1
  serial_dma_init_one(0, Serial1.getHandle()->Instance, DMA1_Stream0, DMA_REQUEST_USART1_TX);
#endif
#ifdef ENABLE_SERIAL2
  serial_dma_init_one(1, Serial2.getHandle()->Instance, DMA1_Stream1, DMA_REQUEST_USART2_TX);
#endif
#ifdef ENABLE_SERIAL8
  serial_dma_init_one(2, Serial8.getHandle()->Instance, DMA1_Stream2, DMA_REQUEST_UART8_TX);
#endif
}

void serial_dma_poll() {
  for (uint8_t i = 0; i < SERIAL_DMA_ENGINES; ++i) {
    serial_dma_poll_one(i);
  }
}

size_t UartDmaTx::write(const uint8_t *p, size_t n) {
  if (id >= SERIAL_DMA_ENGINES) {
    return 0;
  }
  return serial_dma_write_one(id, p, n);
}

size_t UartDmaTx::write_blocking(const uint8_t *p, size_t n) {
  if (id >= SERIAL_DMA_ENGINES || n > SERIAL_DMA_BUF_SIZE || p == nullptr || n == 0) {
    return 0;
  }
  for (;;) {
    const size_t w = serial_dma_write_one(id, p, n);
    if (w == n) {
      return n;
    }
    if (serial_dma_eng[id].uart == nullptr) {
      return 0;
    }
  }
}

size_t UartDmaTx::availableForWrite() {
  if (id >= SERIAL_DMA_ENGINES) {
    return 0;
  }
  return serial_dma_available_for_write(id);
}
