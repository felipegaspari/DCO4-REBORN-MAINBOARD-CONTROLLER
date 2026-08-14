#ifndef __PWM_H__
#define __PWM_H__

#ifndef MCP_I2C_DMA
#define MCP_I2C_DMA 1
#endif

#ifndef ENABLE_MCP4728
#define ENABLE_MCP4728
#endif
#include "_shared/mcp4728.h"

void mcpUpdate(uint16_t l1, uint16_t l2, uint16_t ls, bool stagger);
void mcpUpdate();

#endif
