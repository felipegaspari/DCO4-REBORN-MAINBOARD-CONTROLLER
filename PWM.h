#ifndef __PWM_H__
#define __PWM_H__

extern bool mcp_present[3];
bool mcp_i2c_idle();
void mcp_i2c_wait_idle();
bool mcp_async_write(uint8_t addr7, uint16_t a, uint16_t b, uint16_t c, uint16_t d);
void mcpUpdate(uint16_t l1, uint16_t l2, uint16_t ls, bool stagger);
void mcpUpdate();

#endif
