#include <string.h>
#include <stdio.h>
#include "rfg.h"
#include "uart4.h"
#include "usart.h"
#include "protocoll.h"
#include "stacks.h"
#include "resultqueue.h"
#include "zentrale.h"
#include "cmdlist.h"
#include "SG_global.h"

// ---- framing ----
#define RMT_MAX_PAKET_LENGTH  14

// parser states

//--- PRIVATE VARIABLES------------------------------------------------------------------------------------------------------
//Definition der Struktur des Elementes der Ascii-Tabelle
//Definition der Einstellungsparameter für die Kommunikation mit ASCII-PROTOKOLL

// ---- internal parser storage ----
static uint8_t msg[RMT_MAX_PAKET_LENGTH + 1];
static uint8_t nzeichen = 0;       // bytes buffered from UART ring


// ---- public API ----
void rfg_init(void) {
	nzeichen = 0;
	memset((void*) msg, 0, sizeof(msg));
}

// Pull bytes from UART4 RX ring into msg[] and feed parser
void rfg_sero_get(void) {
	nzeichen = 0;
	while ((rb_rx_used(&uart4_rb) > 0) && (nzeichen < RMT_MAX_PAKET_LENGTH)) {
		msg[nzeichen++] = (uint8_t) uartRB_Getc(&uart4_rb);   // legacy getc() // not using the msg, we dont parse it here, just send the data back
	}
	if (nzeichen) {

		uartRB_Put(&usart2_rb, msg, (uint8_t) nzeichen);
		uartRB_KickTx(&usart2_rb);
		//parse_ascii_rfg();  // just send back the rx to pc
	}

}

#ifdef RFG_PASSTHRU
void rfg_forward_line(volatile uint8_t *line, int len) {
	if (len != 0) {
		uartRB_Put(&uart4_rb, (const char*) line, len); // is this assignment correct? TODO!
		uartRB_KickTx(&uart4_rb);
	}
}
#endif

