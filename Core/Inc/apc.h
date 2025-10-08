#pragma once
#include <stdint.h>
#include <stdbool.h>

void apc_init(void);
void apc_sero_get(void);
void apc_sero_set(void);

#include "SG_global.h"

uint8_t apc_cmd_open (void);
uint8_t apc_cmd_close (void);


uint8_t apc_set_acc_mode(uint32_t v);
uint8_t apc_set_ctl_mode(uint32_t v);
uint8_t apc_get_ctl_mode(uint32_t *out);


uint8_t apc_set_valve_state(uint32_t v);
uint8_t apc_get_valve_state(uint32_t *out);


uint8_t apc_set_pos(uint32_t v);
uint8_t apc_get_pos(uint32_t *out);
uint8_t apc_set_pos_ctl_spd(uint32_t v);
uint8_t apc_get_pos_ctl_spd(uint32_t *v);
uint8_t apc_set_pos_ramp_en(uint32_t en);
uint8_t apc_get_pos_ramp_en(uint32_t *v);
uint8_t apc_set_pos_ramp_time(uint32_t v);
uint8_t apc_get_pos_ramp_time(uint32_t *v);
uint8_t apc_set_pos_ramp_slope(uint32_t v);
uint8_t apc_get_pos_ramp_slope(uint32_t *v);
uint8_t apc_set_pos_ramp_mode(uint32_t v);
uint8_t apc_get_pos_ramp_mode(uint32_t *v);


uint8_t apc_set_pre(uint32_t v);
uint8_t apc_get_pre(uint32_t *out);
uint8_t apc_set_pre_speed(uint32_t v);
uint8_t apc_get_pre_speed(uint32_t *out);
uint8_t apc_set_pre_unit(uint32_t v);
uint8_t apc_get_pre_unit(uint32_t *out);
uint8_t apc_set_ctlr_selector(uint32_t v);
uint8_t apc_get_ctlr_selector(uint32_t *out);
