#include "unity.h"
#include "functions.h"
#include <string.h>


/* ------------------ Unity hooks ------------------ */
void setUp(void) {}
void tearDown(void) {}

static void dump_hex(const char *label, const uint8_t *b, size_t n){
    printf("%s (%zu): ", label, n);
    for (size_t i=0;i<n;++i) printf("%02X ", b[i]);
    printf("\n");
}

void test_gasbox_frame_simple(void)
{
    uint8_t cmd = 0x12;
    uint16_t param = 0x3456;

    uint8_t out[32] = {0};
    uint8_t len = gasbox_build_frame_test(cmd, param, out, sizeof out);

    uint8_t exp[] = { GB_DLE, GB_SOT, 0x12, 0x00, 0x34, 0x56, 0x9C, GB_DLE, GB_EOT };
    TEST_ASSERT_EQUAL_UINT8(sizeof exp, len);
    if (len != sizeof exp || memcmp(exp, out, len) != 0) {
        dump_hex("EXP", exp, sizeof exp);
        dump_hex("GOT", out, len);
    }
    TEST_ASSERT_EQUAL_UINT8_ARRAY(exp, out, len);
}

void test_parser(void){
    // Payload: cmd, status, value_hi, value_lo
    uint8_t cmd = 0x22, st = 0x60;
    uint16_t val = 0x1234;
    uint8_t payload[] = { cmd, st, (uint8_t)(val >> 8), (uint8_t)val };
    uint8_t cks = (uint8_t)(payload[0] + payload[1] + payload[2] + payload[3]);

    // Frame with correct delimiters for THIS repo
    uint8_t frame[] = {
        GB_DLE, GB_SOT,
        payload[0], payload[1], payload[2], payload[3],
        cks,
        GB_DLE, GB_EOT
    };

    uint8_t len = (uint8_t)sizeof(frame);
    printf("len is %d\n", len);

    uint8_t out_cmd=0, out_st=0; uint16_t out_val=0;
    TEST_ASSERT_EQUAL_UINT8(1, parse_binary_gasbox_test(frame, len, &out_cmd, &out_st, &out_val));
    TEST_ASSERT_EQUAL_UINT8(cmd, out_cmd);
    TEST_ASSERT_EQUAL_UINT8(st, out_st);
    TEST_ASSERT_EQUAL_UINT16(val, out_val);

}

int main(void) {
	UNITY_BEGIN();
	RUN_TEST(test_gasbox_frame_simple);
	RUN_TEST(test_parser);
	return UNITY_END();
}
