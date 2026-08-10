static constexpr uint32_t MCP_RING_US = 200;
static constexpr uint8_t MCP_ADDR7[3] = { 0x63, 0x64, 0x65 };

inline void mcpUpdate(uint16_t l1, uint16_t l2, uint16_t ls, bool stagger) {
  if (!stagger) {
    mcp_i2c_wait_idle();
    if (mcp_present[0]) mcp.analogWrite(l1, l2, l1, l2);
    if (mcp_present[1]) mcp2.analogWrite(l1, l2, l1, ls);
    if (mcp_present[2]) mcp3.analogWrite(ls, ls, ls, l2);
    return;
  }

  if (bench_out_active) return;

  static uint8_t ring = 0;
  static uint32_t last_kick_us = 0;
  const uint32_t now = micros();
  if ((uint32_t)(now - last_kick_us) < MCP_RING_US) return;
  if (!mcp_i2c_idle()) return;

  for (uint8_t tries = 0; tries < 3; tries++) {
    const uint8_t i = ring;
    if (!mcp_present[i]) {
      ring = (uint8_t)((ring + 1u) % 3u);
      continue;
    }

    uint16_t a, b, c, d;
    if (i == 0) {
      a = l1;
      b = l2;
      c = l1;
      d = l2;
    } else if (i == 1) {
      a = l1;
      b = l2;
      c = l1;
      d = ls;
    } else {
      a = ls;
      b = ls;
      c = ls;
      d = l2;
    }

    if (mcp_async_write(MCP_ADDR7[i], a, b, c, d)) {
      last_kick_us = now;
      ring = (uint8_t)((ring + 1u) % 3u);
    }
    return;
  }
}

inline void mcpUpdate() {
  mcpUpdate(OSC1Level, OSC2Level, SubLevel, false);
}
