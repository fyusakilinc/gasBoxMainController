#pragma once
#include <stdint.h>
#include <stdbool.h>


void remote_init(void);

#include "SG_global.h"

//--- PUBLIC FUNKTIONSDEKLARATION -------------------------------------------------------------------------------------------
// erhält und verarbeitet den Befehl(in binäry- oder ASCII-Format) per UART1
void remote_sero_get(void);

// Gibt die Zahl nach dem Format des ASCII-Protokolls zurück
void output_ascii_remote(int32_t);

//Gibt die Bestätigungsmeldung in dem vorgestellten Kommunikationsformat für den ASCII-Protokoll zurück
void output_ascii_cmdack(uint8_t, uint8_t, uint8_t);

//Gibt das Ergebnis bzw. den Parameter und die Bestätigungsmeldung für den ASCII-Protokoll zurück.
void output_ascii_result(uint8_t, uint8_t, stack_item *);

//Gibt das Ergebnis für den Binary-Protokoll zurück.
void output_binary_result(stack_item *cmd);

uint8_t remote_ascii_verbose(void);

uint8_t remote_ascii_crlf(void);
