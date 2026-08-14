#include "include_all.h"
#include "MCP4728_multiaddress.h"
#include <Wire.h>
#include <string.h>

// STM32 bus backend for _shared/mcp4728_impl.h. DMA IRQ stays in mcp_i2c_dma.cpp
// (C linkage). Probe / reattach / Fast Write live in the shared impl.

DMA_HandleTypeDef mcp_i2c_hdma;

#if MCP_I2C_DMA
static void mcp_i2c_clean_tx() {
  SCB_CleanDCache_by_Addr(reinterpret_cast<uint32_t *>(mcp_tx_buf), 32);
  __DSB();
}

static void mcp_i2c_dma_init() {
  I2C_HandleTypeDef *hi2c = Wire.getHandle();
  if (hi2c == nullptr) {
    return;
  }

  __HAL_RCC_DMA1_CLK_ENABLE();
  memset(&mcp_i2c_hdma, 0, sizeof(mcp_i2c_hdma));
  mcp_i2c_hdma.Instance = DMA1_Stream3;
  mcp_i2c_hdma.Init.Request = DMA_REQUEST_I2C1_TX;
  mcp_i2c_hdma.Init.Direction = DMA_MEMORY_TO_PERIPH;
  mcp_i2c_hdma.Init.PeriphInc = DMA_PINC_DISABLE;
  mcp_i2c_hdma.Init.MemInc = DMA_MINC_ENABLE;
  mcp_i2c_hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  mcp_i2c_hdma.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  mcp_i2c_hdma.Init.Mode = DMA_NORMAL;
  mcp_i2c_hdma.Init.Priority = DMA_PRIORITY_LOW;
  mcp_i2c_hdma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  mcp_i2c_hdma.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
  mcp_i2c_hdma.Init.MemBurst = DMA_MBURST_SINGLE;
  mcp_i2c_hdma.Init.PeriphBurst = DMA_PBURST_SINGLE;
  if (HAL_DMA_Init(&mcp_i2c_hdma) != HAL_OK) {
    return;
  }
  __HAL_LINKDMA(hi2c, hdmatx, mcp_i2c_hdma);
  HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
}
#endif

void mcp_i2c_bus_begin() {
  Wire.setSDA(PB_9);
  Wire.setSCL(PB_8);
  Wire.setClock(1000000);
  Wire.begin();
  Wire.setClock(1000000);
#if MCP_I2C_DMA
  mcp_i2c_dma_init();
#endif
}

bool mcp_i2c_idle() {
  I2C_HandleTypeDef *hi2c = Wire.getHandle();
  return hi2c != nullptr && HAL_I2C_GetState(hi2c) == HAL_I2C_STATE_READY;
}

static uint32_t mcp_i2c_error_latch = 0;

// Error bits only. State and Mode belong to the ISR while a transfer is in
// flight; forging them to READY is what let a blocking transfer start on an
// armed peripheral, and the resulting IRQ storm is what wedged the board.
void mcp_i2c_clear_error() {
  mcp_i2c_error_latch = 0;
  I2C_HandleTypeDef *hi2c = Wire.getHandle();
  if (hi2c == nullptr) return;
  hi2c->ErrorCode = HAL_I2C_ERROR_NONE;
}

uint32_t mcp_i2c_last_error() {
  return mcp_i2c_error_latch;
}

// End the in-flight CV write for real, so the handle reaches READY on its own.
void mcp_i2c_abort() {
  I2C_HandleTypeDef *hi2c = Wire.getHandle();
  if (hi2c == nullptr || HAL_I2C_GetState(hi2c) == HAL_I2C_STATE_READY) return;
  (void)HAL_I2C_Master_Abort_IT(hi2c, (uint16_t)hi2c->Devaddress);
  const uint32_t t0 = micros();
  while (HAL_I2C_GetState(hi2c) != HAL_I2C_STATE_READY) {
    if ((uint32_t)(micros() - t0) >= MCP_IDLE_WAIT_US) return;
  }
}

// Abort, then cycle PE through Wire.end() / begin(): HAL_I2C_DeInit drops the
// peripheral and begin() bit-bangs SCL free before re-init. Rewriting the
// handle fields is bookkeeping and leaves a slave holding SDA exactly as stuck.
void mcp_i2c_recover() {
  mcp_i2c_abort();
  mcp_i2c_error_latch = 0;

  Wire.end();
#if MCP_I2C_DMA
  HAL_NVIC_DisableIRQ(DMA1_Stream3_IRQn);
  (void)HAL_DMA_DeInit(&mcp_i2c_hdma);
#endif
  mcp_i2c_bus_begin();
}

bool mcp_i2c_tx(uint8_t addr7, const uint8_t* data, uint8_t n) {
  (void)data;
#if MCP_I2C_DMA
  mcp_i2c_clean_tx();
  return HAL_I2C_Master_Transmit_DMA(Wire.getHandle(), (uint16_t)addr7 << 1, mcp_tx_buf,
                                     n) == HAL_OK;
#else
  return HAL_I2C_Master_Transmit_IT(Wire.getHandle(), (uint16_t)addr7 << 1, mcp_tx_buf,
                                    n) == HAL_OK;
#endif
}

// Full Fast Write ACK for the probe, on a bus the caller already quiesced. HAL
// puts the handle back to READY itself on both paths, so only the error bits
// need clearing — a latched NACK would otherwise follow the live CV writes.
// AF (0x4) = no slave ACK (wiring, address straps, power); BERR/ARLO = bus fight.
bool mcp_i2c_tx_blocking(uint8_t addr7, const uint8_t* data, uint8_t n) {
  I2C_HandleTypeDef *hi2c = Wire.getHandle();
  if (hi2c == nullptr || data == nullptr || n == 0) return false;
#if MCP_I2C_DMA
  if (data == mcp_tx_buf) {
    mcp_i2c_clean_tx();
  }
#endif
  const HAL_StatusTypeDef st = HAL_I2C_Master_Transmit(
      hi2c, (uint16_t)addr7 << 1, const_cast<uint8_t *>(data), n, 5);
  if (st != HAL_OK) {
    mcp_i2c_error_latch |= HAL_I2C_GetError(hi2c);
    hi2c->ErrorCode = HAL_I2C_ERROR_NONE;
    return false;
  }
  return true;
}

// Board pane via DCO USB: slim 't' chunks, no RUNNING_AVERAGE buffer needed.
void mcp_diag_print(const char* s) {
  if (s == nullptr) return;
  const uint8_t* p = reinterpret_cast<const uint8_t*>(s);
  size_t n = strlen(s);
  while (n > 0) {
    uint8_t chunk = (n > SERIAL_BENCH_TEXT_DATA_MAX) ? SERIAL_BENCH_TEXT_DATA_MAX : (uint8_t)n;
    serial_send_bench_text_chunk(p, chunk);
    p += chunk;
    n -= chunk;
  }
}

void mcp_after_reattach() {
  mcpUpdate();
}

#include "_shared/mcp4728_impl.h"
