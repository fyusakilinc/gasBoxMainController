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

//----- PRIVATE DEFINES -------------------------------------------------------
// die Zustandsdefinitionen fuer die Zustandsautomaten im ASCII-PROTOKOLL

#define apc_wait_ms 50


//Definition der Einstellungsparameter für die Kommunikation mit ASCII-PROTOKOLL

// in all commands assumed controller 1 is active
#define P_ID_CONTROL_MODE        0x0F020000  // Control Mode (2=Pos, 4=Open, 5=Pressure)
#define P_ID_CTLR_SELECTOR       0x07100000  // Control Mode (2=Pos, 4=Open, 5=Pressure)
#define P_ID_POS_SPEED           0x11030000  // TODO confirm
#define P_ID_PRE_SPEED           0x07050000  // TODO confirm
#define P_ID_RAMP_ENABLE_PRE(xx)(0x07000000u | ((xx)<<16) | 0x0301u)
#define P_ID_RAMP_TIME_PRE(xx)  (0x07000000u | ((xx)<<16) | 0x0302u)
#define P_ID_RAMP_SLOPE_PRE(xx) (0x07000000u | ((xx)<<16) | 0x0303u)
#define P_ID_RAMP_MODE_PRE(xx)  (0x07000000u | ((xx)<<16) | 0x0304u)
#define P_ID_RAMP_TYPE_PRE(xx)  (0x07000000u | ((xx)<<16) | 0x0306u)
#define P_ID_PRESS_UNIT          0x12010301  // TODO confirm (enum unit) for sensor 1
#define P_ID_RAMP_ENABLE_POS     0x11620100  // TODO confirm (bool)
#define P_ID_RAMP_TIME_POS       0x11620200  // TODO confirm (float / seconds)
#define P_ID_RAMP_SLOPE_POS      0x11620300  // TODO confirm (float / mbar per s)
#define P_ID_RAMP_MODE_POS       0x11620400  // TODO confirm (uint_8)
#define P_ID_RAMP_TYPE_POS       0x11620500  // TODO confirm (uint_8)

#define P_ID_OP_MODE			0xA1010000		// for choosing rs232 or rs485, set to 0
#define P_ID_BAUD				0xA1110100		// for choosing baud, set to 4 for 19200
#define P_ID_DB_LENGTH			0xA1110200		// for choosing data bit length, set to 1 for 8
#define P_ID_STOP_BIT			0xA1110300		// for choosing stop bit; 1 or 2
#define P_ID_PARITY_BIT			0xA1110400		// for choosing parity bit; 0, 1 or 2
#define P_ID_COMMAND_SET_SLCT	0xA1110500		// for selecting the command set like IC or PM, we use IC so 0
#define P_ID_COMMAND_TERM		0xA1110B00		// for selecting the command termination, set to 0 for CR + LF


static uint8_t apc_p_set_u32(uint32_t param, uint32_t val);
uint8_t apc_cmd_remote(void);

// forward
void apc_on_frame(uint8_t cmd, uint8_t status, uint16_t value);
// ---- public API ----
void apc_init(void) {
	apc_p_set_u32(P_ID_OP_MODE, 0);
	apc_p_set_u32(P_ID_BAUD, 4);
	apc_p_set_u32(P_ID_DB_LENGTH, 1);
	apc_p_set_u32(P_ID_STOP_BIT, 0); // stop bit 1
	apc_p_set_u32(P_ID_PARITY_BIT, 0); // 0 parity
	apc_p_set_u32(P_ID_COMMAND_SET_SLCT, 0); // ic command set
	apc_p_set_u32(P_ID_COMMAND_TERM, 0); // CR + LF command termination

	// Ensure controller listens to commands from RS‑232 port
	apc_cmd_remote();

	// Configure homing behaviour: perform homing automatically at startup
	apc_p_set_u32(0x10200100, 3); // Start condition: At startup
	apc_p_set_u32(0x10200300, 5); // End control mode: Pressure control

}

// Pull bytes from UART4 RX ring into msg[] and feed parser
void apc_sero_get(void) {
	/*nzeichen = 0;
	while ((rb_rx_used(&usart1_rb) > 0) && (nzeichen < RMT_MAX_PAKET_LENGTH)) {
		msg[nzeichen++] = (uint8_t) uartRB_Getc(&usart1_rb);   // legacy getc()
	}
	if (nzeichen)
		parse_binary_apc();*/
}

void apc_sero_set(void) {
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

uint8_t apc_wait_ack(const char *expect) {
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
uint8_t apc_get_lorr_mode(uint32_t *out) {
	char b[] = "I:" APC_EOL;
	apc_flush_rx();
	if (!apc_puts(b, sizeof b - 1))
		return 0;
	char line[64];
	if (apc_readline(line, sizeof line) < 0)
		return 0; // "I: LOCAL"
	char *p = strchr(line, ':');
	if (!p)
		return 0;
	for (++p; *p == ' ' || *p == '\t'; ++p) {
	}
	if (strncasecmp(p, "_LOCAL", 5) == 0)
		*out = 0;
	else if (strncasecmp(p, "REMOTE", 6) == 0)
		*out = 1;
	else if (strncasecmp(p, "LOCKED", 6) == 0)
		*out = 2;
	else
		return 0;
	return 1;
}

// set pressure or position control
uint8_t apc_set_pp_ctl(uint32_t v) {
	char buf[24];
	int n = snprintf(buf, sizeof(buf), "M:%06lu" APC_EOL, (unsigned long) v); // not true
	if (n <= 0)
		return 0;
	if (!apc_puts(buf, (uint8_t) n))
		return 0;
	return apc_wait_ack("M:");

}

uint8_t apc_cmd_open(void)  {
    char b[] = "O:" APC_EOL;
    apc_flush_rx();
    if (!apc_puts(b, sizeof(b)-1)) return 0;
    return apc_wait_ack("O:");
}

uint8_t apc_cmd_close(void) {
    char b[] = "C:" APC_EOL;
    apc_flush_rx();
    if (!apc_puts(b, sizeof(b)-1)) return 0;
    return apc_wait_ack("C:");
}

uint8_t apc_cmd_hold(void)  {
    char b[] = "H:" APC_EOL;
    apc_flush_rx();
    if (!apc_puts(b, sizeof(b)-1)) return 0;
    return apc_wait_ack("H:");
}

uint8_t apc_cmd_remote(void){
    char b[] = "U:02" APC_EOL;
    apc_flush_rx();
    if (!apc_puts(b, sizeof(b)-1)) return 0;
    return apc_wait_ack("U:");
}

uint8_t apc_cmd_local(void) {
    char b[] = "U:01" APC_EOL;
    apc_flush_rx();
    if (!apc_puts(b, sizeof(b)-1)) return 0;
    return apc_wait_ack("U:");
}

// set position / pressure (value already scaled per CPA)
uint8_t apc_set_pos(uint32_t v) {
	char buf[24];
	int n = snprintf(buf, sizeof(buf), "R:%06lu" APC_EOL, (unsigned long) v);
	if (n <= 0)
		return 0;
	if (!apc_puts(buf, (uint8_t) n))
		return 0;
	return apc_wait_ack("R:");

}

uint8_t apc_set_pres(uint32_t v) {
	char buf[24];
	int n = snprintf(buf, sizeof(buf), "S:%06lu" APC_EOL, (unsigned long) v);
	if (n <= 0)
		return 0;
	if (!apc_puts(buf, (uint8_t) n))
		return 0;
	return apc_wait_ack("S:");
}

// queries return 1 on success and write the parsed integer into *out
uint8_t apc_get_pres(uint32_t *out){
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

uint8_t apc_get_pos(uint32_t *out){
    char b[] = "A:" APC_EOL;  // lowercase per PM
    if (!apc_puts(b, sizeof(b)-1)) return 0;
    char line[64];
    if (apc_readline(line, sizeof(line)) < 0) return 0;
    char *p = line;
    while (*p && (*p < '0' || *p > '9')) ++p;
    *out = (uint32_t)strtoul(p, NULL, 10);
    return 1;
}

uint8_t apc_info_blk(char *dst, int maxlen){
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
/*
static uint8_t apc_p_set_f(float val, uint32_t param){
    char buf[64];
    int n = snprintf(buf,sizeof(buf),"p:01%08lX%02X%.6g" APC_EOL,(unsigned long)param,0,val);
    return (n>0)?apc_puts(buf,(uint8_t)n):0;
}
*/
// Read back single value (returns the value string in dst)
static int apc_p_get(uint32_t param, char *dst, int maxlen, uint32_t to_ms){
    char cmd[32];
    int n = snprintf(cmd,sizeof(cmd),"p:0B%08lX%02X" APC_EOL,(unsigned long)param,0);
    if(n<=0 || !apc_puts(cmd,(uint8_t)n)) return 0;
    return apc_readline(dst,maxlen) >= 0;
}

// commands

uint8_t apc_ram_set_pre(uint32_t en){ return apc_p_set_u32(P_ID_RAMP_ENABLE_PRE(11), !!en) ? 1 : 0; }
uint8_t apc_ram_set_pos(uint32_t en){ return apc_p_set_u32(P_ID_RAMP_ENABLE_POS, !!en) ? 1 : 0; }

uint8_t apc_ram_get_pre(uint32_t *out){
    char line[64]; if(!apc_p_get(P_ID_RAMP_ENABLE_PRE(11),line,sizeof(line),apc_wait_ms)) return 0;
    *out = (uint32_t)strtoul(strrchr(line,' ')+1,NULL,10); return 1;
}

uint8_t apc_ram_get_pos(uint32_t *out){
    char line[64]; if(!apc_p_get(P_ID_RAMP_ENABLE_POS,line,sizeof(line),apc_wait_ms)) return 0;
    *out = (uint32_t)strtoul(strrchr(line,' ')+1,NULL,10); return 1;
}

// RAM:TI / RAM:TI?
uint8_t apc_ramti_set_pre(uint32_t v){ return apc_p_set_u32(P_ID_RAMP_TIME_PRE(11), v) ? 1 : 0; } // units per CPA
uint8_t apc_ramti_get_pre(uint32_t *out){
    char line[64]; if(!apc_p_get(P_ID_RAMP_TIME_PRE(11),line,sizeof(line),apc_wait_ms)) return 0;
    *out = (uint32_t)strtoul(strrchr(line,' ')+1,NULL,10); return 1;
}

uint8_t apc_ramti_set_pos(uint32_t v){ return apc_p_set_u32(P_ID_RAMP_TIME_POS, v) ? 1 : 0; } // units per CPA
uint8_t apc_ramti_get_pos(uint32_t *out){
    char line[64]; if(!apc_p_get(P_ID_RAMP_TIME_POS,line,sizeof(line),apc_wait_ms)) return 0;
    *out = (uint32_t)strtoul(strrchr(line,' ')+1,NULL,10); return 1;
}

// RAM:SLP / RAM:SLP?
uint8_t apc_ramslp_set_pre(uint32_t v){ return apc_p_set_u32(P_ID_RAMP_SLOPE_PRE(11), v) ? 1 : 0; }
uint8_t apc_ramslp_get_pre(uint32_t *out){
    char line[64]; if(!apc_p_get(P_ID_RAMP_SLOPE_PRE(11),line,sizeof(line),apc_wait_ms)) return 0;
    *out = (uint32_t)strtoul(strrchr(line,' ')+1,NULL,10); return 1;
}

uint8_t apc_ramslp_set_pos(uint32_t v){ return apc_p_set_u32(P_ID_RAMP_SLOPE_POS, v) ? 1 : 0; }
uint8_t apc_ramslp(uint32_t *out){
    char line[64]; if(!apc_p_get(P_ID_RAMP_SLOPE_POS,line,sizeof(line),apc_wait_ms)) return 0;
    *out = (uint32_t)strtoul(strrchr(line,' ')+1,NULL,10); return 1;
}

// RAM:MD / RAM:MD?
uint8_t apc_rammd_set_pre(uint32_t v){ return apc_p_set_u32(P_ID_RAMP_MODE_PRE(11), v) ? 1 : 0; }
uint8_t apc_rammd_get_pre(uint32_t *out){
    char line[64]; if(!apc_p_get(P_ID_RAMP_MODE_PRE(11),line,sizeof(line),apc_wait_ms)) return 0;
    *out = (uint32_t)strtoul(strrchr(line,' ')+1,NULL,10); return 1;
}

uint8_t apc_rammd_set_pos(uint32_t v){ return apc_p_set_u32(P_ID_RAMP_MODE_POS, v) ? 1 : 0; }
uint8_t apc_rammd_get_pos(uint32_t *out){
    char line[64]; if(!apc_p_get(P_ID_RAMP_MODE_POS,line,sizeof(line),apc_wait_ms)) return 0;
    *out = (uint32_t)strtoul(strrchr(line,' ')+1,NULL,10); return 1;
}

// PRE:UNT / PRE:UNT?
uint8_t apc_preunt_set(uint32_t unit_enum){ return apc_p_set_u32(P_ID_PRESS_UNIT, unit_enum) ? 1 : 0; }
uint8_t apc_preunt_get(uint32_t *out){
    char line[64]; if(!apc_p_get(P_ID_PRESS_UNIT,line,sizeof(line),apc_wait_ms)) return 0;
    *out = (uint32_t)strtoul(strrchr(line,' ')+1,NULL,10); return 1;
}

// POS:SPD / POS:SPD?
uint8_t apc_posspd_set(uint32_t v){    // v scaled per scheme
    return apc_p_set_u32(P_ID_POS_SPEED, v) ? 1 : 0; // TODO param id
}
uint8_t apc_posspd_get(uint32_t *out){
    char line[64]; if(!apc_p_get(P_ID_POS_SPEED,line,sizeof(line),apc_wait_ms)) return 0;
    *out = (uint32_t)strtoul(strrchr(line,' ')+1,NULL,10);
    return 1;
}

// PRE:SPD / PRE:SPD?
uint8_t apc_prespd_set(uint32_t v){    // v scaled per scheme
    return apc_p_set_u32(P_ID_PRE_SPEED, v) ? 1 : 0; // TODO param id
}
uint8_t apc_prespd_get(uint32_t *out){
    char line[64]; if(!apc_p_get(P_ID_PRE_SPEED,line,sizeof(line),apc_wait_ms)) return 0;
    *out = (uint32_t)strtoul(strrchr(line,' ')+1,NULL,10);
    return 1;
}

// APC:CTL:SEL / APC:CTL:SEL?
uint8_t apc_ctlr_selector_set(uint32_t v){    // v scaled per scheme
    return apc_p_set_u32(P_ID_CTLR_SELECTOR, v) ? 1 : 0; // TODO param id
}
uint8_t apc_ctlr_selector_get(uint32_t *out){
    char line[64]; if(!apc_p_get(P_ID_CTLR_SELECTOR,line,sizeof(line),apc_wait_ms)) return 0;
    *out = (uint32_t)strtoul(strrchr(line,' ')+1,NULL,10);
    return 1;
}
