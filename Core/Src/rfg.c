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
#define TOKEN_LENGTH_MAX  32

typedef enum {
    S_WAIT_CMD = 0,
    S_GET_CMD,
    S_GET_SIGN_OR_DIGIT,
    S_GET_VAL,
    S_PROC_CMD
} ap_state_t;

// parser states

//--- PRIVATE VARIABLES------------------------------------------------------------------------------------------------------
//Definition der Struktur des Elementes der Ascii-Tabelle
//Definition der Einstellungsparameter für die Kommunikation mit ASCII-PROTOKOLL

// ---- internal parser storage ----
static uint8_t msg[CMD_LENGTH_MAX + 1];
static uint8_t nzeichen = 0;       // bytes buffered from UART ring
void parse_ascii_rfg(void);
static inline void ap_reset_state(char *cmd, uint16_t *cmd_len,
                                  char *vbuf, uint8_t *vlen,
                                  uint8_t *pflag, uint8_t *eflag,
                                  ap_state_t *st);
static void arrived(const char* line, const double *dv, uint8_t had_error);

// A small parser callback: returns 1 on success and writes the parsed value(s)
typedef int (*rfg_reply_parser_t)(const char *line, double *out_val);

// ---- public API ----
void rfg_init(void) {
	nzeichen = 0;
	memset((void*) msg, 0, sizeof(msg));
}

// Pull bytes from UART4 RX ring into msg[] and feed parser
void rfg_sero_get(void) {
	nzeichen = 0;
	while ((rb_rx_used(&uart4_rb) > 0) && (nzeichen < CMD_LENGTH_MAX)) {
		msg[nzeichen++] = (uint8_t) uartRB_Getc(&uart4_rb);
	}
	if (nzeichen)
		parse_ascii_rfg();
}

void parse_ascii_rfg(void) {

	uint8_t ptr = 0;
	static uint16_t cmd_len = 0;
	static char cmd[CMD_LENGTH_MAX + 1] = "\0";
	static ap_state_t a_state = S_WAIT_CMD;
	static uint8_t nc = 0;
	static uint8_t pflag = 0;
	static uint8_t eflag = 0;

	static char vbuf[TOKEN_LENGTH_MAX + 1] = "\0"; // raw value text
	static uint8_t vlen = 0;
	//stack_item stack_data;

	do {
		// wenn es ein Zeichen in UART1 Buffer gibt und die Automate nicht im Bearbeitungszustand ist
		if ((nzeichen > 0) && (a_state != S_PROC_CMD)) {
			nc = msg[ptr++];				// hole ein Zeichen aus msg-buffer
		} else
			nc = 0;

		switch (a_state) {
		case S_WAIT_CMD:
			// skip leading spaces, CR/LF
			if (nc == '\r' || nc == '\n' || nc == ' ' || nc == '\t')
				break;
			// start command token
			cmd_len = 0;
			if (cmd_len < TOKEN_LENGTH_MAX - 1) {
				cmd[cmd_len++] = (char) nc;
				cmd[cmd_len] = '\0';
			}
			a_state = S_GET_CMD;
			break;

		case S_GET_CMD:
			if (nc == ';' || nc == '\r' || nc == '\n') {
				a_state = S_PROC_CMD;
				break;
			}
			if (nc == ' ' || nc == '\t') {
				a_state = S_GET_SIGN_OR_DIGIT;
				break;
			}
			if (cmd_len < TOKEN_LENGTH_MAX - 1) {
				cmd[cmd_len++] = (char) nc;
				cmd[cmd_len] = '\0';
			} else {
				eflag = 1;
				a_state = S_PROC_CMD;
			}
			break;

		case S_GET_SIGN_OR_DIGIT:
			if (nc == ';' || nc == '\r' || nc == '\n') {
				a_state = S_PROC_CMD;
				break;
			}
			if (nc == ' ' || nc == '\t')
				break; // still skipping whitespace
			// start value token (accept -, +, digit, '.' to allow .5)
			if ((nc == '-') || (nc == '+') || (nc == '.')
					|| (nc >= '0' && nc <= '9')) {
				vlen = 0;
				if (vlen < TOKEN_LENGTH_MAX - 1) {
					vbuf[vlen++] = (char) nc;
					vbuf[vlen] = '\0';
				}
				pflag = 1;
				a_state = S_GET_VAL;
			} else {
				eflag = 1;
				a_state = S_PROC_CMD;
			}
			break;

		case S_GET_VAL:
			if (nc == ';' || nc == '\r' || nc == '\n') {
				a_state = S_PROC_CMD;
				break;
			}
			// Accept digits, dot, exponent, signs inside exponent form
			if ((nc >= '0' && nc <= '9') || nc == '.' || nc == 'e' || nc == 'E'
					|| nc == '+' || nc == '-') {
				if (vlen < TOKEN_LENGTH_MAX - 1) {
					vbuf[vlen++] = (char) nc;
					vbuf[vlen] = '\0';
				} else {
					eflag = 1;
					a_state = S_PROC_CMD;
				} // overflow
			} else if (nc == ' ' || nc == '\t') {
				// allow trailing spaces before terminator
			} else {
				eflag = 1;
				a_state = S_PROC_CMD;
			}
			break;

		case S_PROC_CMD: {
			char line[CMD_LENGTH_MAX + 1];
			snprintf(line, sizeof line, "%s", cmd);

			// Parsed numeric if present and valid
			double dv = 0.0;
			if (pflag) {
				char *endp = NULL;
				dv = strtod(vbuf, &endp);
				// (endp==vbuf) means not a number; leave dv=0 but still deliver raw
			}

			arrived(line, pflag ? &dv : NULL, eflag);

			// reset tokenizer for next command in the burst
			ap_reset_state(cmd, &cmd_len, vbuf, &vlen, &pflag, &eflag,
					&a_state);
			;
		}
			break;

		default:
			a_state = S_GET_CMD;
			break;

		};

	} while (ptr < nzeichen);
}

typedef enum { RFG_IDLE=0, RFG_WAIT_RX } RfgState;

typedef struct {
    volatile RfgState state;
    volatile uint8_t  have;
    uint32_t          deadline_ms;
    char              reply[CMD_LENGTH_MAX];   // raw reply line
    double            value;                 // parsed numeric (if any)
} RfgSync;

static RfgSync rfg_sync = {0};

static void arrived(const char* line, const double *dv, uint8_t had_error)
{
    // If you want to latch explicit "ERR:NNN" replies too, detect here:
    if (strncasecmp(line, "ERR:", 4) == 0) {
        z_set_error(SG_ERR_RFG);// you can store this somewhere if needed
    }

    if (rfg_sync.state == RFG_WAIT_RX && !rfg_sync.have) {
        strncpy(rfg_sync.reply, line, sizeof rfg_sync.reply - 1);
        rfg_sync.reply[sizeof rfg_sync.reply - 1] = 0;
        rfg_sync.value = (dv ? *dv : 0.0);
        rfg_sync.have = 1;
    }

    (void)had_error; // if you want to act on tokenizer errors, you can
}


// small reset for the tokenizer state machine
static inline void ap_reset_state(char *cmd, uint16_t *cmd_len,
                                  char *vbuf, uint8_t *vlen,
                                  uint8_t *pflag, uint8_t *eflag,
                                  ap_state_t *st)
{
    (void)cmd; (void)vbuf;
    *cmd_len = 0; *vlen = 0;
    *pflag = 0; *eflag = 0;
    *st = S_WAIT_CMD;
}

// Send exactly one ASCII command line with CRLF
static uint8_t rfg_send_line(const char *s)
{
    size_t n = strlen(s);
    // don't double-append CR/LF
    while (n && (s[n-1]=='\r' || s[n-1]=='\n')) --n;

    const uint8_t *p = (const uint8_t*)s;
    size_t left = n;
    while (left) {
        uint8_t chunk = (left > 255) ? 255 : (uint8_t)left;
        if (!uartRB_Put(&uart4_rb, p, chunk)) return 0;
        p += chunk; left -= chunk;
    }
    static const char crlf[] = "\r\n";
    if (!uartRB_Put(&uart4_rb, (const uint8_t*)crlf, 2)) return 0;
    uartRB_KickTx(&uart4_rb);
    return 1;
}



uint8_t rfg_xfer(const char *s, float param, bool wr, uint32_t timeout_ms, float* out) {
	char buf[64];
	if (wr) snprintf(buf, sizeof buf, "%s %u", s, (unsigned)param);
	else    snprintf(buf, sizeof buf, "%s", s);

	// Arm mailbox
	rfg_sync.state = RFG_WAIT_RX;
	rfg_sync.have = 0;
	rfg_sync.value = 0.0;
	rfg_sync.deadline_ms = HAL_GetTick() + timeout_ms;

	// Send
	if (!rfg_send_line(buf)) {
		rfg_sync.state = RFG_IDLE;
		return 0;
	}

	for(;;){
		rfg_sero_get();
		// Check if reply arrived
		if (rfg_sync.have) {
			if (out)
				*out = rfg_sync.value;
			rfg_sync.state = RFG_IDLE;
			rfg_sync.have = 0;
			return 1;   // success
		}

		// Timeout check
		if ((int32_t) (HAL_GetTick() - rfg_sync.deadline_ms) >= 0) {
			rfg_sync.state = RFG_IDLE;
			return 0;   // timeout
		}

	}
	return 0;
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

uint8_t rf_cmd_is_on(const char *cmd, const char *vbuf, uint8_t pflag) {
	if (strcasecmp(cmd, "RF ON") == 0)
		return 1;
	if (pflag && strncasecmp(cmd, "RF", 2) == 0) {
		char *ep = NULL;
		double v = strtod(vbuf, &ep);
		if (ep != vbuf && v > 0.5)
			return 1;
	}
	return 0;
}

int rfg_readline(char *dst, int maxlen, uint32_t timeout_ms) {
	uint32_t t0 = HAL_GetTick();
	int len = 0, seen_eol = 0;

	while ((HAL_GetTick() - t0) < timeout_ms) {
		while (rb_rx_used(&uart4_rb) > 0) {
			uint8_t c = uartRB_Getc(&uart4_rb);
			if (c == '\r' || c == '\n') {
				if (seen_eol) {
					if (len) {
						dst[len] = 0;
						return len;
					}
				}
				seen_eol = 1;
				continue;
			}
			seen_eol = 0;
			if (len < (maxlen - 1))
				dst[len++] = (char) c;
		}
		HAL_Delay(1);  // cooperative wait
	}
	if (len) {
		dst[len] = 0;
		return len;
	}   // last line without CRLF
	return -1;                            // timeout
}
