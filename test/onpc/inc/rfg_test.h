#pragma once

#include <stdint.h>
#include "cmdlist.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RFG_TEST_IDLE = 0,
    RFG_TEST_WAIT_RX
} rfg_test_state_t;

typedef struct {
    rfg_test_state_t state;
    uint8_t have;
    uint8_t ok;
    double value;
    int32_t err_code;
    char reply[CMD_LENGTH_MAX];
} rfg_test_sync_t;

void rfg_test_reset(void);
void rfg_test_prepare_wait(void);
void rfg_test_arrived(const char *line, const double *dv, uint8_t had_error);
const rfg_test_sync_t *rfg_test_get_sync(void);

void parse_ascii_rfg_test_reset(void);
void parse_ascii_rfg_test_feed(const char *burst);

uint32_t rfg_test_error_count(void);
uint8_t rfg_test_last_error(void);

uint8_t rf_cmd_is_on_test(const char *cmd, const char *vbuf, uint8_t pflag);

#ifdef __cplusplus
}
#endif
