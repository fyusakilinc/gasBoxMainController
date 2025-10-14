#include "gasbox_tests.h"
#include "gasbox.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "remote.h"
#include "uart4.h"
#include "usart.h"
#include "protocoll.h"
#include "stacks.h"
#include "resultqueue.h"
#include "zentrale.h"
#include "cmdlist.h"
#include "SG_global.h"
#include "unity.h"

#define GB_TOLERANCE_BITS 5

#define ASSERT_GB_XFER_OK(cmd, param, reply) \
    do { \
        uint8_t _ok = gasbox_xfer(cmd, param, reply, GB_TIMEOUT_MS); \
        TEST_ASSERT_TRUE_MESSAGE(_ok, #cmd " xfer failed"); \
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(GB_STATUS_OK, (reply)->status, #cmd " status not OK"); \
    } while(0)

void test_gasbox_set_mfc1_0x000(void) {
	GbReply r1, r2;
	ASSERT_GB_XFER_OK(GB_CMD_SET_MFC1, 0x0000, &r1);
	ASSERT_GB_XFER_OK(GB_CMD_GET_MFC1, 0x0000, &r2);
	TEST_ASSERT_UINT16_WITHIN(GB_TOLERANCE_BITS, 0x0000, r2.value);
}

void test_gasbox_set_mfc1_0x800(void) {
	GbReply r1, r2;
	ASSERT_GB_XFER_OK(GB_CMD_SET_MFC1, 0x0800, &r1);
	ASSERT_GB_XFER_OK(GB_CMD_GET_MFC1, 0x0000, &r2);
	TEST_ASSERT_UINT16_WITHIN(GB_TOLERANCE_BITS, 0x0800, r2.value);
}

void test_gasbox_set_mfc1_0xFFF(void) {
	GbReply r1, r2;
	ASSERT_GB_XFER_OK(GB_CMD_SET_MFC1, 0x0FFF, &r1);
	ASSERT_GB_XFER_OK(GB_CMD_GET_MFC1, 0x0000, &r2);
	TEST_ASSERT_UINT16_WITHIN(GB_TOLERANCE_BITS, 0x0FFF, r2.value);
}

void test_gasbox_set_mfc2_0x000(void) {
	GbReply r1, r2;
	ASSERT_GB_XFER_OK(GB_CMD_SET_MFC1, 0x0000, &r1);
	ASSERT_GB_XFER_OK(GB_CMD_GET_MFC1, 0x0000, &r2);
	TEST_ASSERT_UINT16_WITHIN(GB_TOLERANCE_BITS, 0x0000, r2.value);
}

void test_gasbox_set_mfc2_0x800(void) {
	GbReply r1, r2;
	ASSERT_GB_XFER_OK(GB_CMD_SET_MFC1, 0x0800, &r1);
	ASSERT_GB_XFER_OK(GB_CMD_GET_MFC1, 0x0000, &r2);
	TEST_ASSERT_UINT16_WITHIN(GB_TOLERANCE_BITS, 0x0800, r2.value);
}

void test_gasbox_set_mfc2_0xFFF(void) {
	GbReply r1, r2;
	ASSERT_GB_XFER_OK(GB_CMD_SET_MFC1, 0x0FFF, &r1);
	ASSERT_GB_XFER_OK(GB_CMD_GET_MFC1, 0x0000, &r2);
	TEST_ASSERT_UINT16_WITHIN(GB_TOLERANCE_BITS, 0x0FFF, r2.value);
}

void test_gasbox_set_mfc3_0x000(void) {
	GbReply r1, r2;
	ASSERT_GB_XFER_OK(GB_CMD_SET_MFC1, 0x0000, &r1);
	ASSERT_GB_XFER_OK(GB_CMD_GET_MFC1, 0x0000, &r2);
	TEST_ASSERT_UINT16_WITHIN(GB_TOLERANCE_BITS, 0x0000, r2.value);
}

void test_gasbox_set_mfc3_0x800(void) {
	GbReply r1, r2;
	ASSERT_GB_XFER_OK(GB_CMD_SET_MFC1, 0x0800, &r1);
	ASSERT_GB_XFER_OK(GB_CMD_GET_MFC1, 0x0000, &r2);
	TEST_ASSERT_UINT16_WITHIN(GB_TOLERANCE_BITS, 0x0800, r2.value);
}

void test_gasbox_set_mfc3_0xFFF(void) {
	GbReply r1, r2;
	ASSERT_GB_XFER_OK(GB_CMD_SET_MFC1, 0x0FFF, &r1);
	ASSERT_GB_XFER_OK(GB_CMD_GET_MFC1, 0x0000, &r2);
	TEST_ASSERT_UINT16_WITHIN(GB_TOLERANCE_BITS, 0x0FFF, r2.value);
}

void test_gasbox_open_valve3(void) {
    GbReply r1, r2;

    // Send "open" command (assuming 1 = open, 0 = closed)
    ASSERT_GB_XFER_OK(GB_CMD_VALVE3_OPEN, 1, &r1);
    ASSERT_GB_XFER_OK(GB_CMD_VALVE3_GET, 0x0000, &r2);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(1, r2.value, "Valve1 failed to open");
}

void test_gasbox_close_valve3(void) {
    GbReply r1, r2;

    // Send "close" command
    ASSERT_GB_XFER_OK(GB_CMD_VALVE3_CLOSE, 0, &r1);
    ASSERT_GB_XFER_OK(GB_CMD_VALVE3_GET, 0x0000, &r2);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, r2.value, "Valve1 failed to close");
}


