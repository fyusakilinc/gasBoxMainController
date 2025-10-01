#pragma once
#include <stdint.h>
#include <stdbool.h>

// one parsed reply
typedef struct {
    uint8_t  cmd;
    uint8_t  status;    // 0x80 = OK
    uint16_t value;     // paramH:paramL
} GbReply;

void remote_init(void);

#include "SG_global.h"

//--- PUBLIC FUNKTIONSDEKLARATION -------------------------------------------------------------------------------------------
// erhält und verarbeitet den Befehl(in binäry- oder ASCII-Format) per UART1
void remote_sero_get(void);
void gb_sero_get(void);

// prüft, ob die Steuerung über den Serial-Port eingeschaltet ist
uint8_t ser_check_remote(uint8_t, uint16_t);

// packt die Binär-Befehlnummer, den Acknowledge und den Parameter als ein Paket in Format Binäre-Protokoll ein und sendet es zurück
void ser_send_response(uint8_t, uint8_t, uint16_t);

// Gibt die Zahl nach dem Format des ASCII-Protokolls zurück
void output_ascii(int32_t);

//Gibt die Bestätigungsmeldung in dem vorgestellten Kommunikationsformat für den ASCII-Protokoll zurück
void output_ascii_cmdack(uint8_t, uint8_t, uint8_t);

//Gibt das Ergebnis bzw. den Parameter und die Bestätigungsmeldung für den ASCII-Protokoll zurück.
void output_ascii_result(uint8_t, uint8_t, stack_item *);

//Gibt das Ergebnis für den Binary-Protokoll zurück.
void output_binary_result(stack_item *cmd);

uint8_t remote_ascii_verbose(void);

uint8_t remote_ascii_crlf(void);

