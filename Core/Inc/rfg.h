#pragma once
#include <stdint.h>
#include <stdbool.h>

//--- PUBLIC FUNKTIONSDEKLARATION -------------------------------------------------------------------------------------------
// erhält und verarbeitet den Befehl(in binäry- oder ASCII-Format) per UART4
void rfg_init(void);
void rfg_sero_get(void);

