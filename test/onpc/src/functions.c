#include "functions.h"
#include "unity.h"
#include "cmdlist.h"
#include <string.h>
#include "SG_global.h"
#include <stdio.h>
#include <stdlib.h>

#define TOKEN_LENGTH_MAX  32
typedef enum {
    S_WAIT_CMD = 0,
    S_GET_CMD,
    S_GET_SIGN_OR_DIGIT,
    S_GET_VAL,
    S_PROC_CMD
} ap_state_t;

static inline uint8_t gb_sum8_test(const uint8_t *p, int n){
    uint32_t s = 0;
    for (int i=0; i<n; ++i) s += p[i];
    return (uint8_t)s;
}

static inline void gb_push_escaped_test(uint8_t **wp, uint8_t b){
    *(*wp)++ = b;
    if (b == GB_DLE) *(*wp)++ = b;  // double any DLE in-band
}

static inline void ap_reset_state(char *cmd, uint8_t *cmd_len,
                                  char *vbuf, uint8_t *vlen,
                                  volatile uint8_t *pflag, volatile uint8_t *eflag,
                                  volatile uint8_t *a_state){
    *cmd_len = 0; cmd[0] = '\0';
    *vlen = 0; vbuf[0] = '\0';
    *pflag = 0; *eflag = 0;
    *a_state = S_WAIT_CMD;
}

uint8_t gasbox_build_frame_test(uint8_t cmd, uint16_t param, uint8_t *out, uint8_t outcap)
{
    if (!out || outcap < 16) return 0; // worst-case is 14; 16 keeps it simple

    uint8_t payload[4] = {
        cmd,
        0x00,                               // reserved/status
        (uint8_t)(param >> 8),
        (uint8_t)(param & 0xFF)
    };
    uint8_t cks = gb_sum8_test(payload, 4);

    uint8_t *w = out;

    *w++ = GB_DLE; *w++ = GB_SOT;
    for (int i = 0; i < 4; ++i) gb_push_escaped_test(&w, payload[i]);
    gb_push_escaped_test(&w, cks);
    *w++ = GB_DLE; *w++ = GB_EOT;

    return (uint8_t)(w - out);
}

static uint8_t lengthRx = 0;
static uint8_t dleFlag = 0;
static uint8_t checksum = 0;
static uint8_t bufferRx[RMT_MAX_PAKET_LENGTH + 1];

uint8_t parse_binary_gasbox_test(const uint8_t *msg, uint8_t nzeichen,
        uint8_t *out_cmd, uint8_t *out_status, uint16_t *out_val) {

	uint8_t data;
	uint8_t ptr = 0;
	uint8_t state = RMT_WAIT_FOR_PAKET_START;

	do {
		switch (state) {
		case RMT_WAIT_FOR_PAKET_START: {
			printf("pointer = %d\n",ptr);
			printf("arrived wait\n");
			// scan for DLE 'S'
			while (ptr < nzeichen) {
				data = msg[ptr++];
				printf("pointer = %d\n",ptr);
				if (dleFlag) {
					// second control char after DLE
					if (data == GB_DLE) {
						printf("data = gb_dle, pointer = %d\n",ptr);
						// interpret as literal DLE
						dleFlag = 0;
						// (no payload yet in WAIT state)
					} else if (data == GB_SOT) {
						printf("data = gb_sot, pointer = %d\n",ptr);
						printf("len=%u byte=%02X\n", lengthRx, data);
						// start of frame
						lengthRx = 0;
						checksum = 0;
						dleFlag = 0;
						state = RMT_READ_PAKET;
						break;
					} else {
						printf("data = else, pointer = %d\n",ptr);
						// other control -> ignore, keep scanning
						dleFlag = 0;
					}
				} else {
					if (data == GB_DLE){
						printf("data = gb_dle, pointer = %d\n",ptr);
						dleFlag = 1;}
				}
			}
		}
			break;

		case RMT_READ_PAKET: {
			printf("arrived read\n");
			while (ptr < nzeichen) {
				data = msg[ptr++];

				// avoid runaway frames
				if (lengthRx >= RMT_MAX_PAKET_LENGTH) {
					dleFlag = 0;
					state = RMT_WAIT_FOR_PAKET_START;
					break;
				}

				if (dleFlag) {
					if (data == GB_DLE) {
						printf("data = gb_dle, pointer = %d\n",ptr);
						printf("len=%u byte=%02X\n", lengthRx, data);
						// stuffed DLE as data
						dleFlag = 0;
						bufferRx[lengthRx++] = GB_DLE;
						checksum += GB_DLE;
					} else if (data == GB_SOT) {
						printf("data = gb_sot, pointer = %d\n",ptr);
						printf("len=%u byte=%02X\n", lengthRx, data);
						// unexpected new start → restart frame
						lengthRx = 0;
						checksum = 0;
						dleFlag = 0;
						state = RMT_READ_PAKET;
						break;
					} else if (data == GB_EOT) {
						printf("data = gb_eot, pointer = %d\n",ptr);
						printf("len=%u byte=%02X\n", lengthRx, data);
						// proper trailer -> parse
						state = RMT_PARSE_PAKET;
						dleFlag = 0;
						break;
					} else {
						dleFlag = 0; // unknown after DLE -> ignore
					}
				} else {
					if (data == GB_DLE) {
						dleFlag = 1;            // next is control
					} else {
						printf("len=%u byte=%02X, pointer = %d\n", lengthRx, data, ptr);
						bufferRx[lengthRx++] = data;
						checksum += data;
					}
				}
			}
		}
			break;

		case RMT_PARSE_PAKET: {
			printf("arrived parse, pointer = %d\n",ptr);
			// Expect 4 payload bytes + 1 checksum (net length 5)
			if (lengthRx == 5) {
				uint8_t cmd = bufferRx[0];
				uint8_t status = bufferRx[1];
				uint8_t pH = bufferRx[2];
				uint8_t pL = bufferRx[3];
				uint8_t cks = bufferRx[4];

				// checksum over the 4 payload bytes
				uint8_t sum = (uint8_t) (cmd + status + pH + pL);

				if (sum == cks) {
					if (out_cmd)
						*out_cmd = cmd;
					if (out_status)
						*out_status = status;
					if (out_val)
						*out_val = (uint16_t) ((pH << 8) | pL);
					// reset for next frame
					state = RMT_WAIT_FOR_PAKET_START;
					lengthRx = 0;
					cks = 0;
					dleFlag = 0;
					return 1;  // success
				}
				// else: bad checksum -> drop silently (or raise an error flag if you want)
			}
			// reset for next frame
			state = RMT_WAIT_FOR_PAKET_START;
			lengthRx = 0;
			checksum = 0;
			dleFlag = 0;
		}
			break;
		}
	} while (ptr <= nzeichen);
	return 0; // not enough bytes yet
}

static ascii_parse_result_t pa_last;   // last parse result snapshot

static volatile uint8_t msg[CMD_LENGTH_MAX  + 1];
static volatile uint8_t nzeichen = 0;
void parse_ascii_test(void);
// feed one ASCII line to the parser (no UART needed)
void parse_ascii_test_feed(const char *s) {
    size_t L = strlen(s);
    if (L > CMD_LENGTH_MAX) L = CMD_LENGTH_MAX;
    memcpy((void*)msg, s, L);
    nzeichen = (uint8_t)L;
    parse_ascii_test();
}

// Read back the parsed fields captured by the hook below
int parse_ascii_test_get(ascii_parse_result_t *out) {
    if (!out) return 0;
    *out = pa_last;
    return pa_last.ok;
}


void parse_ascii_test(void) {
	printf("arrived at parser\n");
	static char cmd[CMD_LENGTH_MAX+1] ="\0";
	static uint8_t cmd_len = 0;

	static char  vbuf[TOKEN_LENGTH_MAX+1] = "\0";   // raw value text (for forwarding / precise parse)
	static uint8_t vlen = 0;

	uint8_t nc=0;
	//volatile static int32_t val = 0;
	uint8_t a_state = S_WAIT_CMD;
	uint8_t pflag = 0;
	uint8_t eflag = 0;

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
			printf("arrived at wait\n");
			printf("nc value = %d\n", nc);
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
			printf("arrived at get\n");
			printf("nc value = %d\n", nc);
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
			printf("arrived at sign\n");
			printf("nc value = %d\n", nc);
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
			printf("arrived at get val\n");
			printf("nc value = %d\n", nc);
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
			printf("arrived at proc val\n");
			printf("nc value = %d\n", nc);
			uint16_t cmd_id = ASCII_CMD_MAX;
			Binary_Search(ASCII_CMD_MAX, cmd, &cmd_id);

			// 2) build stack item (READ if no value, WRITE if value present & valid)
			stack_item si = { 0 };
			si.cmd_index = cmd_id;
			si.cmd_sender = Q_RS232;
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
			printf("error flag = %d \n",eflag);
			printf("cmd id = %d \n",cmd_id);
			printf("[ascii] CMD token = %s, param value = %F\n", cmd, si.par0);
			// 3) enqueue or report error
			if (eflag || cmd_id >= ASCII_CMD_MAX) {
				// optional: your error reporting
				// output_ascii_error(CMR_BADCOMMAND);
			} else {

				// inside case S_PROC_CMD, right *before* the current `return;`
				pa_last.ok        = (eflag == 0) && (cmd_id < ASCII_CMD_MAX);
				strncpy(pa_last.cmd, cmd, sizeof(pa_last.cmd)-1);
				pa_last.cmd[sizeof(pa_last.cmd)-1] = '\0';
				pa_last.cmd_index = cmd_id;
				pa_last.rwflg     = si.rwflg;              // READ or WRITE
				pa_last.has_value = (si.rwflg == WRITE);
				pa_last.value     = si.par0;               // parsed float (or 0.0f for READ)
				// keep your early return
			}

			// 5) reset for next command
		    ap_reset_state(cmd, &cmd_len, vbuf, &vlen, &pflag, &eflag, &a_state);
		    return;
		}
			break;

		default:
			a_state = S_GET_CMD;
			break;

		};

		//	char1[0]=nc;
		//  if(nzeichen >0)	uart1_put(char1, 1);
		//	versandstart1();

	} while (ptr <= nzeichen);
}







