#pragma once
#include <stdint.h>
#include <stdbool.h>

void apc_init(void);
void apc_sero_get(void);
void apc_sero_set(void);

#include "SG_global.h"

uint8_t apc_cmd_open (void);
uint8_t apc_cmd_close (void);


uint8_t apc_set_acc_mode(double v);
uint8_t apc_set_ctl_mode(double v);
uint8_t apc_get_ctl_mode(double *out);
uint8_t apc_get_err_num(double *out);
uint8_t apc_get_err_code(double *out);

uint8_t apc_set_valve_state(double v);
uint8_t apc_get_valve_state(double *out);


uint8_t apc_set_pos(double v);
uint8_t apc_get_pos(double *out);
uint8_t apc_set_pos_ctl_spd(double v);
uint8_t apc_get_pos_ctl_spd(double *v);
uint8_t apc_set_pos_ramp_en(double en);
uint8_t apc_get_pos_ramp_en(double *v);
uint8_t apc_set_pos_ramp_time(double v);
uint8_t apc_get_pos_ramp_time(double *v);
uint8_t apc_set_pos_ramp_slope(double v);
uint8_t apc_get_pos_ramp_slope(double *v);
uint8_t apc_set_pos_ramp_mode(double v);
uint8_t apc_get_pos_ramp_mode(double *v);


uint8_t apc_set_pre(double v);
uint8_t apc_get_pre(double *out);
uint8_t apc_set_pre_speed(double v);
uint8_t apc_get_pre_speed(double *out);
uint8_t apc_set_pre_unit(double v);
uint8_t apc_get_pre_unit(double *out);
uint8_t apc_set_ctlr_selector(double v);
uint8_t apc_get_ctlr_selector(double *out);
