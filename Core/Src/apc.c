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
#include "apc.h"

// ---- framing ----
#define RMT_MAX_PAKET_LENGTH  14
#define RMT_DLE               0x3D   // '='
#define RMT_SOT               0x53   // 'S'
#define RMT_EOT               0x45   // 'E'

// parser states
#define RMT_WAIT_FOR_PAKET_START  1
#define RMT_READ_PAKET            2
#define RMT_PARSE_PAKET           3

//----- PRIVATE DEFINES -------------------------------------------------------
// die Zustandsdefinitionen fuer die Zustandsautomaten im ASCII-PROTOKOLL
#define get_cmd 1
#define get_sign 2
#define get_val 3
#define proc_cmd 4

#define apc_wait_ms 15

//die max.Größe der ASCII-Tabelle
#define ASCII_CMD_MAX			29
#define BINARY_INDEX_MAX		256
#define CMD_LENGTH				19

//--- PRIVATE VARIABLES------------------------------------------------------------------------------------------------------
//Definition der Struktur des Elementes der Ascii-Tabelle
typedef struct {
	char cmdline[CMD_LENGTH];           //für ASCII-Befehl
	uint16_t cmdindex;					//interne Befehlnummer
//uint8_t cmdattribut;				//gibt an, dass der Befehl READ/WRITE-Operation ist.
} Cmdline_Item;


//Definition der Einstellungsparameter für die Kommunikation mit ASCII-PROTOKOLL
volatile static uint8_t verbose = 0;
volatile static uint8_t crlf = 0;
volatile static uint8_t echo = 0;
volatile static uint8_t sloppy = 0;

// ---- internal parser storage ----
static volatile uint8_t msg[RMT_MAX_PAKET_LENGTH + 1];
static volatile uint8_t nzeichen = 0;       // bytes buffered from UART ring
static uint8_t state = RMT_WAIT_FOR_PAKET_START;

static uint8_t bufferRx[RMT_MAX_PAKET_LENGTH + 1];
static uint8_t lengthRx = 0;
static uint8_t dleFlag = 0;
static uint8_t checksum = 0;

// in all commands assumed controller 1 is active
#define P_ID_CONTROL_MODE        0x0F020000  // Control Mode (2=Pos, 4=Open, 5=Pressure)
#define P_ID_POS_SPEED           0x11040000  // TODO confirm
#define P_ID_PRESS_SPEED         0x07040000  // TODO confirm
#define P_ID_RAMP_ENABLE_PRE     0x07110301  // TODO confirm (bool)
#define P_ID_RAMP_TIME_PRE       0x07110302  // TODO confirm (float / seconds)
#define P_ID_RAMP_SLOPE_PRE      0x07110303  // TODO confirm (float / mbar per s)
#define P_ID_RAMP_MODE_PRE       0x07110304  // TODO confirm (uint_8)
#define P_ID_RAMP_STA_PRE        0x07110305  // TODO confirm (uint_8) -	start value
#define P_ID_RAMP_TYPE_PRE       0x07110306  // TODO confirm (uint_8)
#define P_ID_PRESS_UNIT          0x07010000  // TODO confirm (enum unit)
#define P_ID_RAMP_ENABLE_POS     0x11620100  // TODO confirm (bool)
#define P_ID_RAMP_TIME_POS       0x11620200  // TODO confirm (float / seconds)
#define P_ID_RAMP_SLOPE_POS      0x11620300  // TODO confirm (float / mbar per s)
#define P_ID_RAMP_MODE_POS       0x11620400  // TODO confirm (uint_8)
#define P_ID_RAMP_TYPE_POS       0x11620500  // TODO confirm (uint_8)

// forward
static void parse_binary_apc(void);
void apc_on_frame(uint8_t cmd, uint8_t status, uint16_t value);
// ---- public API ----
void apc_init(void) {
	nzeichen = 0;
	state = RMT_WAIT_FOR_PAKET_START;
	lengthRx = 0;
	dleFlag = 0;
	checksum = 0;
	memset((void*) msg, 0, sizeof(msg));
	memset(bufferRx, 0, sizeof(bufferRx));
}

// Pull bytes from UART4 RX ring into msg[] and feed parser
void apc_sero_get(void) {
	nzeichen = 0;
	while ((rb_rx_used(&usart1_rb) > 0) && (nzeichen < RMT_MAX_PAKET_LENGTH)) {
		msg[nzeichen++] = (uint8_t) uartRB_Getc(&usart1_rb);   // legacy getc()
	}
	if (nzeichen)
		parse_binary_apc();
}

#define APC_EOL "\r\n"     // match CPA setting (default CR+LF)
#define APC_EOL1 '\r'
#define APC_EOL2 '\n'

static uint8_t apc_puts(const char *s, uint8_t n)
{
    if (!uartRB_Put(&usart1_rb, s, n)) return 0;
    uartRB_KickTx(&usart1_rb);
    return 1;
}

static int apc_readline(char *dst, int maxlen) {
	uint32_t t0 = HAL_GetTick();
	int len = 0, seen_eol = 0;

	while ((HAL_GetTick() - t0) < apc_wait_ms) {
		while (rb_rx_used(&usart1_rb) > 0) {
			uint8_t c = uartRB_Getc(&usart1_rb);
			if (c == APC_EOL1 || c == APC_EOL2) {
				if (seen_eol) {
					if (len) {
						dst[len] = 0;
						return len;
					}
				}
				seen_eol = 1;    // allow CR+LF
				continue;
			}
			seen_eol = 0;
			if (len < (maxlen - 1))
				dst[len++] = (char) c;
		}
		HAL_Delay(1);
	}
	if (len) {
		dst[len] = 0;
		return len;
	}
	return -1;  // timeout
}

static void apc_flush_rx(void) {
    char dump[64];
    while (apc_readline(dump, sizeof dump) >= 0) { /* discard */ }
}

static uint8_t apc_wait_ack(const char *expect) {
    char line[64];
    if (apc_readline(line, sizeof line) < 0) return 0;
    if (line[0] == 'E' && line[1] == ':') return 0;      // error like "E:000080"
    // allow exact match or just compare leading token
    for (const char* a=expect, *b=line; *a && *b; ++a, ++b) {
        if (*a != *b) return 0;
    }
    return 1;
}

// fire-and-forget commands
static uint8_t apc_get_lorr_mode(uint32_t *out){
    char b[] = "I:" APC_EOL;
    if (!apc_puts(b, sizeof(b)-1)) return 0;
    char line[64];
    if (apc_readline(line, sizeof(line)) < 0) return 0; // e.g., "P:000423"
    // parse trailing integer
    char *p = line;
    while (*p && (*p < '0' || *p > '9')) ++p;
    *out = (uint32_t)strtoul(p, NULL, 10);
    return 1;
}

// set pressure or position control
static uint8_t apc_set_pp_ctl(uint32_t v) {
	char buf[24];
	int n = snprintf(buf, sizeof(buf), "M:%06lu" APC_EOL, (unsigned long) v); // not true
	if (n <= 0)
		return 0;
	if (!apc_puts(buf, (uint8_t) n))
		return 0;
	return apc_wait_ack("M:");

}

static uint8_t apc_cmd_open(void)  {
    char b[] = "O:" APC_EOL;
    apc_flush_rx();
    if (!apc_puts(b, sizeof(b)-1)) return 0;
    return apc_wait_ack("O:");
}

static uint8_t apc_cmd_close(void) {
    char b[] = "C:" APC_EOL;
    apc_flush_rx();
    if (!apc_puts(b, sizeof(b)-1)) return 0;
    return apc_wait_ack("C:");
}

static uint8_t apc_cmd_hold(void)  {
    char b[] = "H:" APC_EOL;
    apc_flush_rx();
    if (!apc_puts(b, sizeof(b)-1)) return 0;
    return apc_wait_ack("H:");
}

static uint8_t apc_cmd_remote(void){
    char b[] = "U:02" APC_EOL;
    apc_flush_rx();
    if (!apc_puts(b, sizeof(b)-1)) return 0;
    return apc_wait_ack("U:");
}

static uint8_t apc_cmd_local(void) {
    char b[] = "U:01" APC_EOL;
    apc_flush_rx();
    if (!apc_puts(b, sizeof(b)-1)) return 0;
    return apc_wait_ack("U:");
}

// set position / pressure (value already scaled per CPA)
static uint8_t apc_set_pos(uint32_t v) {
	char buf[24];
	int n = snprintf(buf, sizeof(buf), "R:%06lu" APC_EOL, (unsigned long) v);
	if (n <= 0)
		return 0;
	if (!apc_puts(buf, (uint8_t) n))
		return 0;
	return apc_wait_ack("R:");

}

static uint8_t apc_set_pres(uint32_t v) {
	char buf[24];
	int n = snprintf(buf, sizeof(buf), "S:%06lu" APC_EOL, (unsigned long) v);
	if (n <= 0)
		return 0;
	if (!apc_puts(buf, (uint8_t) n))
		return 0;
	return apc_wait_ack("S:");
}

// set position speed
static uint8_t apc_set_pos_spd(uint32_t v) {
	char buf[24];
	int n = snprintf(buf, sizeof(buf), "V:" APC_EOL, (unsigned long) v);
	if (n <= 0)
		return 0;
	if (!apc_puts(buf, (uint8_t) n))
		return 0;
	return apc_wait_ack("V:");

}

// set pressure speed
static uint8_t apc_set_pre_spd(uint32_t v) {
	char buf[24];
	int n = snprintf(buf, sizeof(buf), "V:" APC_EOL, (unsigned long) v);
	if (n <= 0)
		return 0;
	if (!apc_puts(buf, (uint8_t) n))
		return 0;
	return apc_wait_ack("V:");

}

// queries return 1 on success and write the parsed integer into *out
static uint8_t apc_get_pres(uint32_t *out){
    char b[] = "P:" APC_EOL;
    if (!apc_puts(b, sizeof(b)-1)) return 0;
    char line[64];
    if (apc_readline(line, sizeof(line)) < 0) return 0; // e.g., "P:000423"
    // parse trailing integer
    char *p = line;
    while (*p && (*p < '0' || *p > '9')) ++p;
    *out = (uint32_t)strtoul(p, NULL, 10);
    return 1;
}

static uint8_t apc_get_pos(uint32_t *out){
    char b[] = "r:" APC_EOL;  // lowercase per PM
    if (!apc_puts(b, sizeof(b)-1)) return 0;
    char line[64];
    if (apc_readline(line, sizeof(line)) < 0) return 0;
    char *p = line;
    while (*p && (*p < '0' || *p > '9')) ++p;
    *out = (uint32_t)strtoul(p, NULL, 10);
    return 1;
}

static uint8_t apc_info_blk(char *dst, int maxlen){
    char b[] = "i:76" APC_EOL;
    if (!apc_puts(b, sizeof(b)-1)) return 0;
    return (apc_readline(dst, maxlen) >= 0);
}

// IC helpers:
// String out (SET), index always 0
static uint8_t apc_p_set_u32(uint32_t param, uint32_t val){
    char buf[64];
    int n = snprintf(buf,sizeof(buf),"p:01%08lX%02X%lu" APC_EOL,(unsigned long)param,0,(unsigned long)val);
    return (n>0)?apc_puts(buf,(uint8_t)n):0;
}
static uint8_t apc_p_set_f(float val, uint32_t param){
    char buf[64];
    int n = snprintf(buf,sizeof(buf),"p:01%08lX%02X%.6g" APC_EOL,(unsigned long)param,0,val);
    return (n>0)?apc_puts(buf,(uint8_t)n):0;
}
// Read back single value (returns the value string in dst)
static int apc_p_get(uint32_t param, char *dst, int maxlen, uint32_t to_ms){
    char cmd[32];
    int n = snprintf(cmd,sizeof(cmd),"p:0B%08lX%02X" APC_EOL,(unsigned long)param,0);
    if(n<=0 || !apc_puts(cmd,(uint8_t)n)) return 0;
    return apc_readline(dst,maxlen) >= 0;
}

// commands

uint8_t apc_ram_set_pre(uint16_t en){ return apc_p_set_u32(P_ID_RAMP_ENABLE_PRE, !!en) ? 1 : 0; }
uint8_t apc_ram_set_pos(uint16_t en){ return apc_p_set_u32(P_ID_RAMP_ENABLE_POS, !!en) ? 1 : 0; }

uint8_t apc_ram_get_pre(uint16_t *out){
    char line[64]; if(!apc_p_get(P_ID_RAMP_ENABLE_PRE,line,sizeof(line),apc_wait_ms)) return 0;
    *out = (uint16_t)strtoul(strrchr(line,' ')+1,NULL,10); return 1;
}

uint8_t apc_ram_get_pos(uint16_t *out){
    char line[64]; if(!apc_p_get(P_ID_RAMP_ENABLE_POS,line,sizeof(line),apc_wait_ms)) return 0;
    *out = (uint16_t)strtoul(strrchr(line,' ')+1,NULL,10); return 1;
}
