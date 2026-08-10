//#include <Adafruit_MCP4728.h>
#include "MCP4728_multiaddress.h"
#include <Wire.h>

// Adafruit_MCP4728 mcp;
// Adafruit_MCP4728 mcp2;
// Adafruit_MCP4728 mcp3;

MCP4728 mcp;
MCP4728 mcp2;
MCP4728 mcp3;

bool mcp_present[3];

static constexpr uint8_t MCP_FAST_WRITE_BYTES = 8;
static constexpr uint32_t MCP_IDLE_WAIT_US = 2000;
static uint8_t mcp_tx_buf[MCP_FAST_WRITE_BYTES];

// MCP4728_multiaddress analogWrite(a,b,c,d) → fastWrite: (FAST_WRITE|high), low × 4.
static void mcp_fill_tx(uint16_t a, uint16_t b, uint16_t c, uint16_t d) {
  const uint16_t ch[4] = { a, b, c, d };
  for (uint8_t i = 0; i < 4; i++) {
    const uint16_t v = (uint16_t)(ch[i] & 0x0FFFu);
    mcp_tx_buf[i * 2u]     = (uint8_t)(v >> 8);
    mcp_tx_buf[i * 2u + 1u] = (uint8_t)v;
  }
}

bool mcp_i2c_idle() {
  return HAL_I2C_GetState(Wire.getHandle()) == HAL_I2C_STATE_READY;
}

void mcp_i2c_wait_idle() {
  const uint32_t t0 = micros();
  while (!mcp_i2c_idle()) {
    if ((uint32_t)(micros() - t0) >= MCP_IDLE_WAIT_US) return;
  }
}

bool mcp_async_write(uint8_t addr7, uint16_t a, uint16_t b, uint16_t c, uint16_t d) {
  if (!mcp_i2c_idle()) return false;
  mcp_fill_tx(a, b, c, d);
  return HAL_I2C_Master_Transmit_IT(Wire.getHandle(), (uint16_t)addr7 << 1, mcp_tx_buf,
                                    MCP_FAST_WRITE_BYTES) == HAL_OK;
}

// Size > 0 so stm32duino uses i2c_master_write (same path as analogWrite), not IsDeviceReady.
static bool mcp_probe_addr(uint8_t addr) {
  Wire.beginTransmission(addr);
  Wire.write((uint8_t)0);
  return Wire.endTransmission() == 0;
}

// Boot: I2C @ 1 MHz, attach three MCP4728s, write mid/high idle levels.
void init_MCP4728() {

  Wire.setSDA(PB_9);
  Wire.setSCL(PB_8);
  Wire.setClock(1000000);
  Wire.begin();
  Wire.setClock(1000000);

  mcp.attach(Wire, 255, 0x63);
  mcp2.attach(Wire, 255, 0x64);
  mcp3.attach(Wire, 255, 0x65);

  mcp_present[0] = mcp_probe_addr(0x63);
  mcp_present[1] = mcp_probe_addr(0x64);
  mcp_present[2] = mcp_probe_addr(0x65);

  if (mcp_present[0]) mcp.analogWrite(4095, 4095, 4095, 4095);
  if (mcp_present[1]) mcp2.analogWrite(4095, 4095, 4095, 4095);
  if (mcp_present[2]) mcp3.analogWrite(4095, 4095, 4095, 4095);
}
