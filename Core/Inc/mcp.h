
#ifndef INC_MCP_H_
#define INC_MCP_H_

HAL_StatusTypeDef mcp_init(void);
void mcp_sero_get(void);
void mcp_sero_set(void);
void mcp_leds_write4(uint8_t pattern_4lsb);
HAL_StatusTypeDef mcp_set_sps_out_disable(uint8_t disable);
uint8_t mcp_diag_read_b_raw(void);

#endif
