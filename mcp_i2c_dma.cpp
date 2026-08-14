// IRQ handler lives here so Arduino does not auto-prototype it as C++.
// The vector table expects C linkage (DMA1_Stream3_IRQHandler).
#include "Arduino.h"

extern DMA_HandleTypeDef mcp_i2c_hdma;

extern "C" void DMA1_Stream3_IRQHandler(void) {
  HAL_DMA_IRQHandler(&mcp_i2c_hdma);
}
