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
#include <math.h>
#include "float.h"
#include <errno.h>

//----- PRIVATE DEFINES -------------------------------------------------------

#define apc_wait_ms 50

// interface rs232
#define P_ID_OP_MODE			0xA1010000		// for choosing rs232 or rs485, set to 0
#define P_ID_BAUD				0xA1110100		// for choosing baud, set to 4 for 19200
#define P_ID_DB_LENGTH			0xA1110200		// for choosing data bit length, set to 1 for 8
#define P_ID_STOP_BIT			0xA1110300		// for choosing stop bit; 1 or 2
#define P_ID_PARITY_BIT			0xA1110400		// for choosing parity bit; 0, 1 or 2
#define P_ID_COMMAND_SET_SLCT	0xA1110500		// for selecting the command set like IC or PM, we use IC so 0
#define P_ID_COMMAND_TERM		0xA1110B00		// for selecting the command termination, set to 0 for CR + LF
#define P_ID_POS_UNIT			0xA1120101 		// set 0 to 7
#define P_ID_PRESS_UNIT			0xA1120201		// set 0 to 7

// system
#define P_ID_ACC_MODE			0x0F0B0000		// for choosing local or remote, 0 local, 1 remote, 2 remote locked
#define P_ID_CONTROL_MODE		0x0F020000  	// Control Mode (2=Pos, 4=Open, 5=Pressure)
#define P_ID_ERR_RECOVER		0x0F506600 		// bool, attempts to fix the error without restart
#define P_ID_RES_CTLR			0x0F500100		// bool, Emulates a power cycle
#define P_ID_ERR_NUM			0x0F300600		// UINT16, get error number
#define P_ID_ERR_CODE			0x0F300700		// UINT16, get error code

// valve
#define P_ID_VALVE_POS			0x10010000 		// float, read the valve actual position
#define P_ID_VALVE_POS_STATE	0x10100000		// 0 int, 1 closed, 2 open
#define P_ID_HOMING_START_COND	0x10200100 		// 3 for at startup
#define P_ID_HOMING_END_MOD		0x10200300 		// 5 for setting mode pressure after homing
#define P_ID_HOMING_STATUS		0x10201100		// 0 not started, 1 in prog, 2 ok, 3 error

// pos control
#define P_ID_ACT_POS			0x11010000 		// float, read the act pos , we set them int now, decimals are ignored TODO
#define P_ID_TARGET_POS			0x11020000 		// float, set the target pos
#define P_ID_POS_SPEED			0x11030000
#define P_ID_RAMP_ENABLE_POS	0x11620100 		// (bool)
#define P_ID_RAMP_TIME_POS		0x11620200  	// (float / seconds)
#define P_ID_RAMP_SLOPE_POS		0x11620300  	//(float / mbar per s)
#define P_ID_RAMP_MODE_POS		0x11620400  	// (uint_8)
#define P_ID_RAMP_TYPE_POS		0x11620500  	// (uint_8)

// pres control
#define P_ID_TARGET_P_USED			0x07030000 		//  This value is set as p ctlr input. It differs to the Target Pressure if a pressure ramp is used.
#define P_ID_PRE_SPEED				0x07050000
#define P_ID_CTLR_SELECTOR 			0x07100000 		// active controller in control mode = pressure
#define P_ID_RAMP_ENABLE_PRE(xx)	(0x07000000u | ((xx)<<16) | 0x0301u)
#define P_ID_RAMP_TIME_PRE(xx)  	(0x07000000u | ((xx)<<16) | 0x0302u)
#define P_ID_RAMP_SLOPE_PRE(xx) 	(0x07000000u | ((xx)<<16) | 0x0303u)
#define P_ID_RAMP_MODE_PRE(xx)  	(0x07000000u | ((xx)<<16) | 0x0304u)
#define P_ID_RAMP_TYPE_PRE(xx)  	(0x07000000u | ((xx)<<16) | 0x0306u)

// pres sensor
#define P_ID_ACT_PRE			0x12100000
#define P_ID_SENS_SLCT			0x12040100		// 0 to 3
#define P_ID_TARGET_PRE			0x12040300
#define P_ID_EXEC				0x12040400		// 1 exec zero adjust, 2 clear ofs value


static uint8_t apc_p_set_u32(uint32_t param, uint32_t val);
uint8_t apc_set_acc_mode(double v);

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
	apc_set_acc_mode(1);

	// Configure homing behaviour: perform homing automatically at startup
	apc_p_set_u32(P_ID_HOMING_START_COND, 3); // Start condition: At startup
	apc_p_set_u32(P_ID_HOMING_END_MOD, 5); // End control mode: Pressure control

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

void apc_sero_set(void) { }

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
		//HAL_Delay(1);
	}
	if (len) {
		dst[len] = 0;
		return len;
	}
	return -1;  // timeout
}


// IC helpers:
// String out (SET), index always 0
static uint8_t apc_p_set_u32(uint32_t param, uint32_t val){
    char buf[64];
    int n = snprintf(buf,sizeof(buf),"p:01%08lX%02X%lu" APC_EOL,(unsigned long)param,0,(unsigned long)val);
    return (n>0)?apc_puts(buf,(uint8_t)n):0;
}

static uint8_t apc_p_set_f(uint32_t param, double val){
    char buf[64];
    /* Use %.6g unless your device requires fixed decimals like %.3f */
    int n = snprintf(buf, sizeof(buf),
                     "p:01%08lX%02X%.6g" APC_EOL,
                     (unsigned long)param, 0, val);
    return (n > 0) ? apc_puts(buf, (uint8_t)n) : 0;
}

static int apc_p_set_u32_from_double(uint32_t param, double dv){
    if (!isfinite(dv)) return 0;
    double r = nearbyint(dv);
    /* Tolerance so 1.0000001 doesn’t fail due to FP noise */
    if (fabs(dv - r) > 1e-6) return 0;
    if (r < 0.0 || r > (double)UINT32_MAX) return 0;
    return apc_p_set_u32(param, (uint32_t)r);
}

// Read back single value (returns the value string in dst)
static int apc_p_get(uint32_t param, char *dst, int maxlen){
    char cmd[32];
    int n = snprintf(cmd,sizeof(cmd),"p:0B%08lX%02X" APC_EOL,(unsigned long)param,0);
    if(n<=0 || !apc_puts(cmd,(uint8_t)n)) return 0;
    return apc_readline(dst, maxlen) >= 0;
}

// returns 1 on success, 0 on failure; fills out fields and points *val_str to start of value
static int apc_parse_p_header(const char *line, unsigned *err, unsigned *func,
		unsigned *pid, unsigned *index, const char **val_str) {
	int pos_after_hdr = 0;
	// p: EE FF PPPPPPPP II <value...>
	if (sscanf(line, "p:%2x%2x%8x%2x%n", err, func, pid, index, &pos_after_hdr)
			!= 4)
		return 0;
	*val_str = line + pos_after_hdr;
	return 1;
}

static int apc_parse_p_value_double(const char *line,
                                       uint32_t expect_param,
                                       double *out)
{
    unsigned err=0, func=0, pid=0, idx=0;
    const char *vstr = NULL;

    if (!apc_parse_p_header(line, &err, &func, &pid, &idx, &vstr)) return 0;
	if (err != 0) {
		z_set_error(err); // TODO: implement get error structure here. error code and error number
		return 0;
	}  // device reported an error
    if (func != 0x0B) return 0;             // expect GET echo
    if (pid  != expect_param) return 0;     // sanity check: same param
    if (idx  != 0x00) return 0;             // ionly use index 0

    if (*vstr == '\0') return 0;            // missing value

    char *endp = NULL;
    double val = strtod(vstr, &endp);       // supports 23.1, -5.2, 1.23e-3, etc.
    if (endp == vstr) return 0;             // no digits parsed

    *out = val;
    return 1;
} // we are getting all the params in double because cmd.param is double

static int apc_parse_p_value_u16(const char *line, uint32_t expect_param,
		uint16_t *out) {
	unsigned err = 0, func = 0, pid = 0, idx = 0;
	const char *vstr = NULL;
	if (!apc_parse_p_header(line, &err, &func, &pid, &idx, &vstr))
		return 0;
	if (err != 0 || func != 0x0B || pid != expect_param || idx != 0x00)
		return 0;
	if (*vstr == '\0')
		return 0;
	errno = 0;
	char *endp = NULL;
	unsigned long u = strtoul(vstr, &endp, 10);
	if (endp == vstr || errno == ERANGE || u > UINT16_MAX)
		return 0;
	*out = (uint16_t) u;
	return 1;
}


//  ----- commands -----

//  ----- SYSTEM -----

uint8_t apc_set_acc_mode(double v) {
	return apc_p_set_u32_from_double(P_ID_ACC_MODE, v) ? 1 : 0;
}

uint8_t apc_set_ctl_mode(double v) {
	return apc_p_set_u32_from_double(P_ID_CONTROL_MODE, v) ? 1 : 0;
}

uint8_t apc_get_ctl_mode(double *out) {
	char line[64];
	uint16_t u = 0;
	if (!apc_p_get(P_ID_CONTROL_MODE, line, sizeof(line)))
		return 0;
	if (!apc_parse_p_value_u16(line, P_ID_CONTROL_MODE, &u))
		return 0;
	*out = (double) u;
	return 1;
}

uint8_t apc_cmd_open(void)  {
	return apc_p_set_u32(P_ID_CONTROL_MODE, 4) ? 1 : 0;
}

uint8_t apc_cmd_close(void) {
	return apc_p_set_u32(P_ID_CONTROL_MODE, 3) ? 1 : 0;
}

uint8_t apc_get_err_num(double *out) {
	char line[64];
	uint16_t u = 0;
	if (!apc_p_get(P_ID_ERR_NUM, line, sizeof(line)))
		return 0;
	if (!apc_parse_p_value_u16(line, P_ID_ERR_NUM, &u))
		return 0;
	*out = (double) u;              // cmd.param API stays double
	return 1;
}

uint8_t apc_get_err_code(double *out) {
	char line[64];
	uint16_t u = 0;
	if (!apc_p_get(P_ID_ERR_CODE, line, sizeof(line)))
		return 0;
	if (!apc_parse_p_value_u16(line, P_ID_ERR_CODE, &u))
		return 0;
	*out = (double) u;              // cmd.param API stays double
	return 1;
}


//  ----- VALVE -----

uint8_t apc_get_valve_state(double *out) {
	char line[64];
	uint16_t u = 0;
	if (!apc_p_get(P_ID_VALVE_POS_STATE, line, sizeof line))
		return 0;
	if (!apc_parse_p_value_u16(line, P_ID_VALVE_POS_STATE, &u))
		return 0;
	*out = (double) u;
	return 1;
}

//  ----- POSITION CONTROL -----

uint8_t apc_set_pos(double v) {
	return apc_p_set_f(P_ID_TARGET_POS, v) ? 1 : 0;
}
uint8_t apc_get_pos(double *out) {
	char line[64];
	if (!apc_p_get(P_ID_ACT_POS, line, sizeof(line)))
		return 0;
	return apc_parse_p_value_double(line, P_ID_ACT_POS, out) ? 1 : 0;
}


uint8_t apc_set_pos_ctl_spd(double v) {
	return apc_p_set_f(P_ID_POS_SPEED, v) ? 1 : 0;
}

uint8_t apc_get_pos_ctl_spd(double *out) {
	char line[64];
	if (!apc_p_get(P_ID_POS_SPEED, line, sizeof(line)))
		return 0;
	return apc_parse_p_value_double(line, P_ID_POS_SPEED, out) ? 1 : 0;
}

uint8_t apc_set_pos_ramp_en(double en) {
	return apc_p_set_u32_from_double(P_ID_RAMP_ENABLE_POS, !!en) ? 1 : 0;
}

uint8_t apc_get_pos_ramp_en(double *out) {
	char line[64];
	uint16_t u = 0;
	if (!apc_p_get(P_ID_RAMP_ENABLE_POS, line, sizeof(line)))
		return 0;
	if (!apc_parse_p_value_u16(line, P_ID_RAMP_ENABLE_POS, &u))
		return 0;
	*out = (double) u;              // cmd.param API stays double
	return 1;
}

uint8_t apc_set_pos_ramp_time(double v) {
	return apc_p_set_f(P_ID_RAMP_TIME_POS, v) ? 1 : 0;
} // units per CPA
uint8_t apc_get_pos_ramp_time(double *out) {
	char line[64];
	if (!apc_p_get(P_ID_RAMP_TIME_POS, line, sizeof(line)))
		return 0;
	return apc_parse_p_value_double(line, P_ID_RAMP_TIME_POS, out) ? 1 : 0;
}
// RAM:SLP / RAM:SLP?
uint8_t apc_set_pos_ramp_slope(double v) {
	return apc_p_set_f(P_ID_RAMP_SLOPE_POS, v) ? 1 : 0;
}
uint8_t apc_get_pos_ramp_slope(double *out) {
	char line[64];
	if (!apc_p_get(P_ID_RAMP_SLOPE_POS, line, sizeof(line)))
		return 0;
	return apc_parse_p_value_double(line, P_ID_RAMP_SLOPE_POS, out) ? 1 : 0;
}

uint8_t apc_set_pos_ramp_mode(double v) {
	return apc_p_set_u32_from_double(P_ID_RAMP_MODE_POS, v) ? 1 : 0;
}
uint8_t apc_get_pos_ramp_mode(double *out) {
	char line[64];
	uint16_t u = 0;
	if (!apc_p_get(P_ID_RAMP_MODE_POS, line, sizeof(line)))
		return 0;
	if (!apc_parse_p_value_u16(line, P_ID_RAMP_MODE_POS, &u))
		return 0;
	*out = (double) u;              // cmd.param API stays double
	return 1;
}


//  ----- PRESSURE CONTROL -----

uint8_t apc_set_pre(double v) {
	return apc_p_set_f(P_ID_TARGET_PRE, v) ? 1 : 0;
}
uint8_t apc_get_pre(double *out) {
	char line[64];
	if (!apc_p_get(P_ID_ACT_PRE, line, sizeof(line)))
		return 0;
	return apc_parse_p_value_double(line, P_ID_ACT_PRE, out) ? 1 : 0;
}

uint8_t apc_set_pre_speed(double v) {
	return apc_p_set_f(P_ID_PRE_SPEED, v) ? 1 : 0;
}
uint8_t apc_get_pre_speed(double *out) {
	char line[64];
	if (!apc_p_get(P_ID_PRE_SPEED, line, sizeof(line)))
		return 0;
	return apc_parse_p_value_double(line, P_ID_PRE_SPEED, out) ? 1 : 0;
}

uint8_t apc_set_pre_unit(double v) {
	return apc_p_set_u32_from_double(P_ID_PRESS_UNIT, v) ? 1 : 0;
}
uint8_t apc_get_pre_unit(double *out) {
	char line[64];
	uint16_t u = 0;
	if (!apc_p_get(P_ID_PRESS_UNIT, line, sizeof(line)))
		return 0;
	if (!apc_parse_p_value_u16(line, P_ID_PRESS_UNIT, &u))
		return 0;
	*out = (double) u;              // cmd.param API stays double
	return 1;
}

// APC:CTL:SEL / APC:CTL:SEL?
uint8_t apc_set_ctlr_selector(double v){    // v scaled per scheme
    return apc_p_set_u32_from_double(P_ID_CTLR_SELECTOR, v) ? 1 : 0;
}
uint8_t apc_get_ctlr_selector(double *out) {
	char line[64];
	uint16_t u = 0;
	if (!apc_p_get(P_ID_CTLR_SELECTOR, line, sizeof(line)))
		return 0;
	if (!apc_parse_p_value_u16(line, P_ID_CTLR_SELECTOR, &u))
		return 0;
	*out = (double) u;              // cmd.param API stays double
	return 1;
}
