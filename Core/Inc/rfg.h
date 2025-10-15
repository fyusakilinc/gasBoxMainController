#pragma once
#include <stdint.h>
#include <stdbool.h>

//--- PUBLIC FUNKTIONSDEKLARATION -------------------------------------------------------------------------------------------
// erhält und verarbeitet den Befehl(in binäry- oder ASCII-Format) per UART4
void rfg_init(void);
void rfg_sero_get(void);

// passthrough to RFG
#define RFG_PASSTHRU
#ifdef RFG_PASSTHRU
void rfg_forward_line(const uint8_t *line, int len);
#endif
uint8_t rf_cmd_is_on(const char *cmd, const char *vbuf, uint8_t pflag);
