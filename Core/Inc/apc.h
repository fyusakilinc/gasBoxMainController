#pragma once
#include <stdint.h>
#include <stdbool.h>

void apc_init(void);
void apc_sero_get(void);
void apc_sero_set(void);

#include "SG_global.h"

uint8_t apc_cmd_open (void);
uint8_t apc_cmd_close (void);
uint8_t apc_cmd_hold (void);
uint8_t apc_cmd_remote (void);
uint8_t apc_cmd_local (void);

uint8_t apc_set_pos(uint32_t v);
uint8_t apc_set_pres(uint32_t v);
uint8_t apc_get_pres(uint32_t *out);
uint8_t apc_get_pos(uint32_t *out);
uint8_t apc_info_blk(char *dst, int maxlen);

uint8_t apc_ctlr_selector_get(uint32_t *out);
uint8_t apc_ctlr_selector_set(uint32_t v);
uint8_t apc_prespd_get(uint32_t *out);
uint8_t apc_prespd_set(uint32_t v);
uint8_t apc_posspd_get(uint32_t *out);
uint8_t apc_posspd_set(uint32_t v);
uint8_t apc_preunt_get(uint32_t *out);
uint8_t apc_preunt_set(uint32_t unit_enum);
uint8_t apc_rammd_get_pos(uint32_t *out);
uint8_t apc_rammd_set_pos(uint32_t v);
uint8_t apc_rammd_get_pre(uint32_t *out);
uint8_t apc_rammd_set_pre(uint32_t v);
uint8_t apc_ramslp(uint32_t *out);
uint8_t apc_ramslp_set_pos(uint32_t v);
uint8_t apc_ramslp_set_pre(uint32_t v);
uint8_t apc_ramslp_get_pre(uint32_t *out);
uint8_t apc_ramti_set_pre(uint32_t v);
uint8_t apc_ramti_get_pre(uint32_t *out);
uint8_t apc_ramti_set_pos(uint32_t v);
uint8_t apc_ramti_get_pos(uint32_t *out);
uint8_t apc_ram_set_pre(uint32_t en);
uint8_t apc_ram_set_pos(uint32_t en);
uint8_t apc_ram_get_pre(uint32_t *out);
uint8_t apc_ram_get_pos(uint32_t *out);
uint8_t apc_set_pp_ctl(uint32_t v);
