#include <string.h>
#include <stdio.h>
#include <stdlib.h>
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
void rfg_forward_line(const uint8_t *buf, int len) {
	// raw forward; do NOT touch CR/LF
	const uint8_t *p = buf;
	while (len) {
		uint8_t chunk = (len > 255) ? 255 : (uint8_t) len;
		(void) uartRB_Put(&uart4_rb, (const char*) p, chunk);
		p += chunk;
		len -= chunk;
	}
	uartRB_KickTx(&uart4_rb);
}
#endif

uint8_t rf_cmd_is_on(const char *cmd, const char *vbuf, uint8_t pflag){
    if (strcasecmp(cmd, "RF ON") == 0) return 1;
    if (pflag && strncasecmp(cmd, "RF", 2) == 0) {
        char *ep = NULL; double v = strtod(vbuf, &ep);
        if (ep != vbuf && v > 0.5) return 1;
    }
    return 0;
}

