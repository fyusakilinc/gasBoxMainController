#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "remote.h"
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
#define TOKEN_LENGTH_MAX  32
#define RMT_DLE               0x3D   // '='
#define RMT_SOT               0x53   // 'S'
#define RMT_EOT               0x45   // 'E'


typedef enum {
    S_WAIT_CMD = 0,
    S_GET_CMD,
    S_GET_SIGN_OR_DIGIT,
    S_GET_VAL,
    S_PROC_CMD
} ap_state_t;

//----- PRIVATE DEFINES -------------------------------------------------------
// die Zustandsdefinitionen fuer die Zustandsautomaten im ASCII-PROTOKOLL
#define get_cmd 1
#define get_sign 2
#define get_val 3
#define proc_cmd 4

//die max.Größe der ASCII-Tabelle
#define ASCII_CMD_MAX			180
#define BINARY_INDEX_MAX		256
#define CMD_LENGTH_MAX			32

#define CMD_MIN_RFG  89
#define CMD_MAX_RFG  149

//--- PRIVATE VARIABLES------------------------------------------------------------------------------------------------------
//Definition der Struktur des Elementes der Ascii-Tabelle

// ---- internal parser storage ----
static volatile uint8_t msg[CMD_LENGTH_MAX  + 1];
static volatile uint8_t nzeichen = 0;       // bytes buffered from UART ring
static uint8_t state = S_WAIT_CMD;

static uint8_t bufferRx[RMT_MAX_PAKET_LENGTH + 1];
static uint8_t lengthRx = 0;
static uint8_t dleFlag = 0;
static uint8_t checksum = 0;

// forward
void parse_ascii(void);
void output_ascii(int32_t);
void Binary_Search(uint8_t ncmd, char *key, uint8_t *cmdindex);
void output_ascii_cmdack(uint8_t cmd_ack);

// passthrough to RFG
//#define RFG_PASSTHRU
#ifdef RFG_PASSTHRU
static void forward_cmd(const char *cmdtxt, const char *valtxt) {
    char out[2*TOKEN_LENGTH_MAX + 16];
    int n = (valtxt && valtxt[0])
          ? snprintf(out, sizeof(out), "%s %s;\r\n", cmdtxt, valtxt)
          : snprintf(out, sizeof(out), "%s;\r\n",     cmdtxt);
    if (n > 0) { uart_puts_rb(&uart4_rb, out); uartRB_KickTx(&uart4_rb); }
}
#endif

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
void remote_init(void) {
	nzeichen = 0;
	state = S_WAIT_CMD;
	lengthRx = 0;
	dleFlag = 0;
	checksum = 0;
	memset((void*) msg, 0, sizeof(msg));
	memset(bufferRx, 0, sizeof(bufferRx));

}

// Pull bytes from UART4 RX ring into msg[] and feed parser
void remote_sero_get(void) {
	nzeichen = 0;
	while ((rb_rx_used(&usart2_rb) > 0) && (nzeichen < CMD_LENGTH_MAX)) {
		msg[nzeichen++] = (uint8_t) uartRB_Getc(&usart2_rb);   // legacy getc()
	}
	if (nzeichen)
		parse_ascii();
}

// das Paket in ASCII-Format analysieren und das Paket in den Stack einfügen.
void parse_ascii(void) {

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
		if ((nzeichen > 0) && (a_state != proc_cmd)) {
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
			uint8_t cmd_id = ASCII_CMD_MAX;
			Binary_Search(ASCII_CMD_MAX, cmd, &cmd_id);

			// 2) build stack item (READ if no value, WRITE if value present & valid)
			stack_item si = { 0 };
			si.cmd_index = cmd_id;
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

			// 4) optional passthrough
#ifdef RFG_PASSTHRU
		        forward_cmd(cmd, (si.rwflg==WRITE) ? vbuf : NULL);
		        #endif

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

void serialSendAnswer(uint8_t *message) {
	uint8_t i;
	uint8_t n = 0;
	uint8_t checksum = 0;
	char buffer[RMT_MAX_PAKET_LENGTH + 1];

	if ((message[2] == 0x00) || (message[2] == 0x02) || (message[2] == 0x03)
			|| (message[2] == 0x0A)) //für die Kompabilität vom altem MatchingCube-Programm. später zu löschen
			{
		message[2] |= 0x80;
	}

	buffer[n++] = RMT_DLE;
	buffer[n++] = RMT_SOT;
	for (i = 0; i < (CMR_DATAPAKET_LENGTH - 1); i++) {
		buffer[n++] = message[i];
		if (message[i] == RMT_DLE) {
			buffer[n++] = message[i];
			// Die Prüfsumme erstreckt sich nur noch über die NETTO-Payload!
			//checksum += message[i];
		}
		checksum += message[i];
	}
	buffer[n++] = checksum;
	if (checksum == RMT_DLE) {
		buffer[n++] = checksum;
	}
	buffer[n++] = RMT_DLE;
	buffer[n++] = RMT_EOT;

	uartRB_Put(&usart2_rb, buffer, n);
	uartRB_KickTx(&usart2_rb);
}

void output_ascii_cmdack(uint8_t cmd_ack) {
	char tmp[35];
	char tmp2[40];
	tmp2[0] = '\0';   // init before strcat

	switch(cmd_ack & 0xFF)  //(cmd_ack & 0x7F)
	{
		case CMR_COMMANDONDEMAND:
			strcpy(tmp, "No Answer!");
			break;

		case CMR_PARAMETERINVALID:
			strcpy(tmp, "Parameter Invalid!");
			break;

		case CMR_PARAMETERCLIPEDMIN:
			strcpy(tmp, "Parameter Clipped to Minimum!");
			break;

		case CMR_PARAMETERCLIPEDMAX:
			strcpy(tmp, "Parameter Clipped to Maximum!");
			break;

		case CMR_PARAMETERADJUSTED:
			strcpy(tmp, "Parameter Adjusted!");
			break;

		case CMR_WRONGPARAMETERFORMAT:
			strcpy(tmp, "Wrong Parameter Format!");
			break;

		case CMR_UNKNOWNCOMMAND:
			strcpy(tmp, "Unknown Command!");
			break;

		case CMR_COMMANDDENIED:
			strcpy(tmp, "Command Denied!");
			break;

		case CMR_COMMANDNOTSUPPORTED:
			strcpy(tmp, "Command Not Supported!");
			break;

		case CMR_EEPROMERROR:
			strcpy(tmp, "EEPROM Error!");
			break;

		case CMR_EEPWRLOCKED:
			strcpy(tmp, "EEPROM Write Lock!");
			break;

		case CMR_WRONGOPMODE:
			strcpy(tmp, "Wrong Operation Mode!");
			break;

		case CMR_UNITBUSY:
			strcpy(tmp, "Unit Busy!");
			break;

		case CMR_MISSINGPARAMETER:
			strcpy(tmp, "Missing Parameter!");
			break;

		case CMR_OPTIONNOTINSTALLED:
			strcpy(tmp, "Required Option Not Installed!");
			break;

		case CMR_MALFORMATTEDCOMMAND:
			strcpy(tmp, "Malformatted Command!");
			break;

		case STACK_CMDINSTACK:
			strcpy(tmp, "Ins Stack!");
			break;

		default:
			sprintf(tmp,"%3.3u",(cmd_ack & 0x7F));
			break;
	}

	strcat(tmp2,tmp);
	strcat(tmp2,";");
	uartRB_Put(&usart2_rb,tmp2, strlen(tmp2));
	uartRB_KickTx(&usart2_rb);       // ensure TX starts
}

void output_ascii_ui(uint32_t val) {
	char tmp[34];
	sprintf(tmp, "%-lu", val);
	uartRB_Put(&usart2_rb, tmp, strlen(tmp));
	uartRB_KickTx(&usart2_rb);
}

void output_ascii_si(int32_t val) {
	char tmp[34];
	sprintf(tmp, "%-ld", val);
	uartRB_Put(&usart2_rb, tmp, strlen(tmp));
	uartRB_KickTx(&usart2_rb);
}

void output_ascii_fl(float val) {
	char tmp[34]; // TODO reicht hier die laenge mit timestamp?
	sprintf(tmp, "%.2f", val); // @suppress("Float formatting support")
	uartRB_Put(&usart2_rb, tmp, strlen(tmp));
	uartRB_KickTx(&usart2_rb);
}


