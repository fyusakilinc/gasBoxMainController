#pragma once
#include <stdint.h>
#include <stdbool.h>


void apc_sero_get(void);

#include "SG_global.h"

static uint8_t apc_cmd_open (void);
static uint8_t apc_cmd_close (void);
static uint8_t apc_cmd_hold (void);
static uint8_t apc_cmd_remote (void);
static uint8_t apc_cmd_local (void);

static uint8_t apc_set_pos(uint32_t v);
static uint8_t apc_set_pres(uint32_t v);
static uint8_t apc_get_pres(uint32_t *out);
static uint8_t apc_get_pos(uint32_t *out);
static uint8_t apc_info_blk(char *dst, int maxlen);
