#include "rfg_test.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "SG_global.h"

#define TOKEN_LENGTH_MAX 32

typedef enum {
    S_WAIT_CMD = 0,
    S_GET_CMD,
    S_GET_SIGN_OR_DIGIT,
    S_GET_VAL,
    S_PROC_CMD
} ap_state_t;

static rfg_test_sync_t rfg_sync_test;

static struct {
    uint32_t count;
    uint8_t last;
} error_stats;

typedef struct {
    ap_state_t state;
    uint16_t cmd_len;
    char cmd[CMD_LENGTH_MAX + 1];
    uint8_t pflag;
    uint8_t eflag;
    char vbuf[TOKEN_LENGTH_MAX + 1];
    uint8_t vlen;
} rfg_parser_test_t;

static rfg_parser_test_t parser_state = {
    .state = S_WAIT_CMD,
    .cmd_len = 0,
    .cmd = "\0",
    .pflag = 0,
    .eflag = 0,
    .vbuf = "\0",
    .vlen = 0,
};

static void record_error(uint8_t errnr) {
    error_stats.count++;
    error_stats.last = errnr;
}

void z_set_error_test(uint8_t errnr) {
    record_error(errnr);
}

void rfg_test_reset(void) {
    memset(&rfg_sync_test, 0, sizeof(rfg_sync_test));
    rfg_sync_test.state = RFG_TEST_IDLE;
    rfg_sync_test.value = 0.0;
    rfg_sync_test.err_code = 0;
    memset(&error_stats, 0, sizeof(error_stats));
}

void rfg_test_prepare_wait(void) {
    rfg_sync_test.state = RFG_TEST_WAIT_RX;
    rfg_sync_test.have = 0;
    rfg_sync_test.ok = 0;
    rfg_sync_test.value = 0.0;
    rfg_sync_test.err_code = 0;
    memset(rfg_sync_test.reply, 0, sizeof(rfg_sync_test.reply));
}

static void parser_reset(void) {
    parser_state.state = S_WAIT_CMD;
    parser_state.cmd_len = 0;
    parser_state.cmd[0] = '\0';
    parser_state.pflag = 0;
    parser_state.eflag = 0;
    parser_state.vbuf[0] = '\0';
    parser_state.vlen = 0;
}

void parse_ascii_rfg_test_reset(void) {
    parser_reset();
}

void parse_ascii_rfg_test_feed(const char *burst) {
    if (!burst) {
        return;
    }

    size_t nzeichen = strlen(burst);
    size_t ptr = 0;

    do {
        uint8_t nc = 0;
        if ((ptr < nzeichen) && (parser_state.state != S_PROC_CMD)) {
            nc = (uint8_t)burst[ptr++];
        }

        switch (parser_state.state) {
        case S_WAIT_CMD:
            if (nc == '\r' || nc == '\n' || nc == ' ' || nc == '\t') {
                break;
            }
            parser_state.cmd_len = 0;
            if (parser_state.cmd_len < TOKEN_LENGTH_MAX - 1) {
                parser_state.cmd[parser_state.cmd_len++] = (char)nc;
                parser_state.cmd[parser_state.cmd_len] = '\0';
            }
            parser_state.state = S_GET_CMD;
            break;

        case S_GET_CMD:
            if (nc == ';' || nc == '\r' || nc == '\n') {
                parser_state.state = S_PROC_CMD;
                break;
            }
            if (nc == ' ' || nc == '\t') {
                parser_state.state = S_GET_SIGN_OR_DIGIT;
                break;
            }
            if (parser_state.cmd_len < TOKEN_LENGTH_MAX - 1) {
                parser_state.cmd[parser_state.cmd_len++] = (char)nc;
                parser_state.cmd[parser_state.cmd_len] = '\0';
            } else {
                parser_state.eflag = 1;
                parser_state.state = S_PROC_CMD;
            }
            break;

        case S_GET_SIGN_OR_DIGIT:
            if (nc == ';' || nc == '\r' || nc == '\n') {
                parser_state.state = S_PROC_CMD;
                break;
            }
            if (nc == ' ' || nc == '\t') {
                break;
            }
            if ((nc == '-') || (nc == '+') || (nc == '.') || (nc >= '0' && nc <= '9')) {
                parser_state.vlen = 0;
                if (parser_state.vlen < TOKEN_LENGTH_MAX - 1) {
                    parser_state.vbuf[parser_state.vlen++] = (char)nc;
                    parser_state.vbuf[parser_state.vlen] = '\0';
                }
                parser_state.pflag = 1;
                parser_state.state = S_GET_VAL;
            } else {
                parser_state.eflag = 1;
                parser_state.state = S_PROC_CMD;
            }
            break;

        case S_GET_VAL:
            if (nc == ';' || nc == '\r' || nc == '\n') {
                parser_state.state = S_PROC_CMD;
                break;
            }
            if ((nc >= '0' && nc <= '9') || nc == '.' || nc == 'e' || nc == 'E' ||
                nc == '+' || nc == '-') {
                if (parser_state.vlen < TOKEN_LENGTH_MAX - 1) {
                    parser_state.vbuf[parser_state.vlen++] = (char)nc;
                    parser_state.vbuf[parser_state.vlen] = '\0';
                } else {
                    parser_state.eflag = 1;
                    parser_state.state = S_PROC_CMD;
                }
            } else if (nc == ' ' || nc == '\t') {
                /* allow whitespace before terminator */
            } else {
                parser_state.eflag = 1;
                parser_state.state = S_PROC_CMD;
            }
            break;

        case S_PROC_CMD: {
            char line[CMD_LENGTH_MAX + 1];
            snprintf(line, sizeof line, "%s", parser_state.cmd);

            double dv = 0.0;
            if (parser_state.pflag) {
                char *endp = NULL;
                dv = strtod(parser_state.vbuf, &endp);
                (void)endp;
            }

            rfg_test_arrived(line, parser_state.pflag ? &dv : NULL, parser_state.eflag);
            parser_reset();
            break;
        }

        default:
            parser_state.state = S_GET_CMD;
            break;
        }

    } while (ptr < nzeichen || parser_state.state == S_PROC_CMD);
}

static int is_error_code(const char *line, int32_t *out_err) {
    if (!line || line[0] != 'E') {
        return 0;
    }
    size_t len = strlen(line);
    if (len <= 1) {
        return 0;
    }
    char *endp = NULL;
    long code = strtol(line + 1, &endp, 10);
    if (endp == line + 1 || code <= 0) {
        return 0;
    }
    if (out_err) {
        *out_err = (int32_t)code;
    }
    return 1;
}

void rfg_test_arrived(const char *line, const double *dv, uint8_t had_error) {
    (void)had_error;

    rfg_sync_test.ok = 0;
    rfg_sync_test.err_code = 0;

    int32_t err_code = 0;
    if (is_error_code(line, &err_code)) {
        rfg_sync_test.err_code = err_code;
        rfg_sync_test.value = -1.0;
        rfg_sync_test.have = 1;
        rfg_sync_test.ok = 0;
        return;
    }

    if (line && line[0] == 'W' && strlen(line) > 1) {
        record_error(SG_ERR_RFG);
    }

    if (line && strncasecmp(line, "ERR:", 4) == 0) {
        record_error(SG_ERR_RFG);
    }

    if (rfg_sync_test.state == RFG_TEST_WAIT_RX && !rfg_sync_test.have) {
        if (line) {
            strncpy(rfg_sync_test.reply, line, sizeof(rfg_sync_test.reply) - 1);
            rfg_sync_test.reply[sizeof(rfg_sync_test.reply) - 1] = '\0';
        } else {
            rfg_sync_test.reply[0] = '\0';
        }

        if (line && (strcmp(line, ";") == 0 || strcmp(line, ";;") == 0 || strncmp(line, ">OK;", 4) == 0)) {
            rfg_sync_test.value = dv ? *dv : 0.0;
            rfg_sync_test.ok = 1;
        } else if (dv != NULL) {
            rfg_sync_test.value = *dv;
            rfg_sync_test.ok = 1;
        } else {
            rfg_sync_test.value = -1.0;
            rfg_sync_test.ok = 0;
        }
        rfg_sync_test.have = 1;
    }
}

const rfg_test_sync_t *rfg_test_get_sync(void) {
    return &rfg_sync_test;
}

uint32_t rfg_test_error_count(void) {
    return error_stats.count;
}

uint8_t rfg_test_last_error(void) {
    return error_stats.last;
}

uint8_t rf_cmd_is_on_test(const char *cmd, const char *vbuf, uint8_t pflag) {
    if (!cmd) {
        return 0;
    }
    if (strcasecmp(cmd, "RF ON") == 0) {
        return 1;
    }
    if (pflag && cmd && strncasecmp(cmd, "RF", 2) == 0) {
        char *ep = NULL;
        double v = strtod(vbuf ? vbuf : "", &ep);
        if (ep != (vbuf ? vbuf : "") && v > 0.5) {
            return 1;
        }
    }
    return 0;
}
