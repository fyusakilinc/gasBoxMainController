#ifndef TEST_LOG_H
#define TEST_LOG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

static inline bool test_log_is_enabled(void) {
    static int cached = -1;
    if (cached < 0) {
        const char *env = getenv("ONPC_TEST_VERBOSE");
        cached = (env && *env && env[0] != '0') ? 1 : 0;
    }
    return cached == 1;
}

static inline void test_logf(const char *fmt, ...) {
    if (!test_log_is_enabled()) {
        return;
    }
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

static inline void test_log_hex(const char *label, const uint8_t *data, size_t len) {
    if (!test_log_is_enabled()) {
        return;
    }
    fprintf(stderr, "%s (%zu): ", label, len);
    for (size_t i = 0; i < len; ++i) {
        fprintf(stderr, "%02X ", data[i]);
    }
    fputc('\n', stderr);
}

#endif /* TEST_LOG_H */
