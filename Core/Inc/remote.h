#pragma once
#include <stdint.h>
#include <stdbool.h>


void remote_init(void);

#include "SG_global.h"

//--- PUBLIC FUNKTIONSDEKLARATION -------------------------------------------------------------------------------------------
// erhält und verarbeitet den Befehl(in binäry- oder ASCII-Format) per UART1
void remote_sero_get(void);
void output_ascii_ui(uint32_t val);
void output_ascii_si(int32_t val);
void output_ascii_fl(float val);


