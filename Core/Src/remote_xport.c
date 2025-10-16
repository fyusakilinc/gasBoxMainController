#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "remote_xport.h"
#include "uart4.h"
#include "usart.h"
#include "protocoll.h"
#include "stacks.h"
#include "rfg.h"
#include "resultqueue.h"
#include "zentrale.h"
#include "cmdlist.h"
#include "SG_global.h"
#include "iso.h"

// ---- framing ----
#define TOKEN_LENGTH_MAX  32

typedef enum {
    S_WAIT_CMD = 0,
    S_GET_CMD,
    S_GET_SIGN_OR_DIGIT,
    S_GET_VAL,
    S_PROC_CMD
} ap_state_t;

//----- PRIVATE DEFINES -------------------------------------------------------
// die Zustandsdefinitionen fuer die Zustandsautomaten im ASCII-PROTOKOLL

//die max.Größe der ASCII-Tabelle

#define CMD_MIN_RFG  89
#define CMD_MAX_RFG  149

//--- PRIVATE VARIABLES------------------------------------------------------------------------------------------------------
//Definition der Struktur des Elementes der Ascii-Tabelle

// ---- internal parser storage ----
static volatile uint8_t msg[CMD_LENGTH_MAX  + 1];
static volatile uint8_t nzeichen = 0;       // bytes buffered from UART ring

// forward
void parse_ascii_xport(void);


static inline void ap_reset_state(char *cmd, uint8_t *cmd_len,
                                  char *vbuf, uint8_t *vlen,
                                  volatile uint8_t *pflag, volatile uint8_t *eflag,
                                  volatile uint8_t *a_state){
    *cmd_len = 0; cmd[0] = '\0';
    *vlen = 0; vbuf[0] = '\0';
    *pflag = 0; *eflag = 0;
    *a_state = S_WAIT_CMD;
}

// ---- public API ----
void remote_xport_init(void) {
	nzeichen = 0;
	memset((void*) msg, 0, sizeof(msg));

}

// Pull bytes from UART4 RX ring into msg[] and feed parser
void remote_xport_sero_get(void) {
	nzeichen = 0;
	while ((rb_rx_used(&usart3_rb) > 0) && (nzeichen < CMD_LENGTH_MAX)) {
		msg[nzeichen++] = (uint8_t) uartRB_Getc(&usart3_rb);   // legacy getc()
	}
	if (nzeichen)
		parse_ascii_xport();
}

// das Paket in ASCII-Format analysieren und das Paket in den Stack einfügen.
void parse_ascii_xport(void) {

	static char cmd[CMD_LENGTH_MAX+1] ="\0";
	static uint8_t cmd_len = 0;

	static char  vbuf[TOKEN_LENGTH_MAX+1] = "\0";   // raw value text (for forwarding / precise parse)
	static uint8_t vlen = 0;

	static volatile uint8_t nc=0;
	//volatile static int32_t val = 0;
	volatile static uint8_t a_state = S_WAIT_CMD;
	volatile static uint8_t pflag = 0;
	volatile static uint8_t eflag = 0;

	uint8_t ptr = 0;

	//stack_item stack_data;

	do {
		// wenn es ein Zeichen in UART1 Buffer gibt und die Automate nicht im Bearbeitungszustand ist
		if ((nzeichen > 0) && (a_state != S_PROC_CMD)) {
			nc = msg[ptr++];				// hole ein Zeichen aus msg-buffer
		} else
			nc = 0;

		//len = strlen(cmd);

		if (eflag == 1) {
			if ((nc == 32) || (nc == 13) || (nc == 10)) {
				a_state = S_PROC_CMD;
			}
		}

		switch (a_state) {
		case S_WAIT_CMD:
			// skip leading spaces, CR/LF
			if (nc == ' ' || nc == '\t' || nc == '\r' || nc == '\n')
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
				a_state = S_PROC_CMD;  // READ (no value)
				break;
			}
			if (nc == ' ' || nc == '\t') {
				a_state = S_GET_SIGN_OR_DIGIT; // space before value
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
			// 1) resolve command
			uint16_t cmd_id = ASCII_CMD_MAX;
			Binary_Search(ASCII_CMD_MAX, cmd, &cmd_id);

// RFG_PASSTHRU bridge removed - RFG commands now go through z_cmd_sero()

			// 2) build stack item (READ if no value, WRITE if value present & valid)
			stack_item si = { 0 };
			si.cmd_index = cmd_id;
			si.cmd_sender = Q_XPORT;
			si.prio = PRIO_LEVEL1;
			si.rwflg = (pflag && !eflag) ? WRITE : READ;

			// parse numeric only if WRITE
			if (si.rwflg == WRITE) {
				char *endp = NULL;
				double dv = strtod(vbuf, &endp);
				if (endp == vbuf) {
					eflag = 1;
				}
				si.par0 = (float) dv;  // carry float to worker
			} else {
				si.par0 = 0.0f;
			}

			// 3) enqueue or report error
			if (eflag || cmd_id >= ASCII_CMD_MAX) {
				// optional: your error reporting
				// output_ascii_error(CMR_BADCOMMAND);
			} else {
				(void) stack_insert_sero(si);
			}

			// 5) reset for next command
		    ap_reset_state(cmd, &cmd_len, vbuf, &vlen, &pflag, &eflag, &a_state);
		}
			break;

		default:
			a_state = S_GET_CMD;
			break;

		};

		//	char1[0]=nc;
		//  if(nzeichen >0)	uart1_put(char1, 1);
		//	versandstart1();

	} while (ptr < nzeichen);
}

void output_ascii_ui_xport(uint32_t val) {
	char tmp[34];
	sprintf(tmp, "%-lu", val);
	uartRB_Put(&usart3_rb, tmp, strlen(tmp));
	uartRB_KickTx(&usart3_rb);
}

void output_ascii_si_xport(int32_t val) {
	char tmp[34];
	sprintf(tmp, "%-ld", val);
	uartRB_Put(&usart3_rb, tmp, strlen(tmp));
	uartRB_KickTx(&usart3_rb);
}

void output_ascii_fl_xport(float val) {
	char tmp[34]; // TODO reicht hier die laenge mit timestamp?
	sprintf(tmp, "%.2f", val); // @suppress("Float formatting support")
	uartRB_Put(&usart3_rb, tmp, strlen(tmp));
	uartRB_KickTx(&usart3_rb);
}


