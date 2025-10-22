#include "unity.h"
#include "functions.h"
#include "rfg_test.h"
#include <string.h>
#include <SG_global.h>

/* ------------------ Unity hooks ------------------ */
void setUp(void) {
}
void tearDown(void) {
}

static void dump_hex(const char *label, const uint8_t *b, size_t n) {
	printf("%s (%zu): ", label, n);
	for (size_t i = 0; i < n; ++i)
		printf("%02X ", b[i]);
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

void test_gb_parser(void){
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

void test_binsearch_found(void) {
	uint16_t out = 0;
	Binary_Search(190, "GAS:V3", &out);
	TEST_ASSERT_EQUAL_UINT16(1, out);
}

void test_parse_ascii_write_ok(void) {
	ascii_parse_result_t r;
	parse_ascii_test_feed("\n\rGAS:V3 12.5;\n\r");

	TEST_ASSERT_TRUE(parse_ascii_test_get(&r));
	TEST_ASSERT_EQUAL_STRING("GAS:V3", r.cmd);
	TEST_ASSERT_EQUAL_UINT8(1, r.has_value);
	TEST_ASSERT_FLOAT_WITHIN(1e-3f, 12.5f, r.value);
	TEST_ASSERT_EQUAL_UINT16(1, r.cmd_index);   // via test_resolver
	TEST_ASSERT_EQUAL_UINT8(WRITE, r.rwflg);     // whatever your enum is
}

void test_rfg_arrived_ack(void) {
    rfg_test_reset();
    rfg_test_prepare_wait();

    rfg_test_arrived(";", NULL, 0);

    const rfg_test_sync_t *sync = rfg_test_get_sync();
    TEST_ASSERT_TRUE(sync->have);
    TEST_ASSERT_TRUE(sync->ok);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, (float)sync->value);
    TEST_ASSERT_EQUAL_STRING(";", sync->reply);
}


void test_rfg_arrived_numeric(void) {
    rfg_test_reset();
    rfg_test_prepare_wait();

    double value = 42.5;
    rfg_test_arrived("RF", &value, 0);

    const rfg_test_sync_t *sync = rfg_test_get_sync();
    TEST_ASSERT_TRUE(sync->have);
    TEST_ASSERT_TRUE(sync->ok);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, (float)value, (float)sync->value);
    TEST_ASSERT_EQUAL_STRING("RF", sync->reply);
}

void test_rfg_arrived_error_code(void) {
    rfg_test_reset();
    rfg_test_prepare_wait();

    rfg_test_arrived("E123;", NULL, 0);

    const rfg_test_sync_t *sync = rfg_test_get_sync();
    TEST_ASSERT_TRUE(sync->have);
    TEST_ASSERT_FALSE(sync->ok);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, -1.0f, (float)sync->value);
    TEST_ASSERT_EQUAL_INT32(123, sync->err_code);
}

void test_rfg_arrived_warning_sets_error(void) {
    rfg_test_reset();
    rfg_test_prepare_wait();

    rfg_test_arrived("W123", NULL, 0);

    const rfg_test_sync_t *sync = rfg_test_get_sync();
    TEST_ASSERT_TRUE(sync->have);
    TEST_ASSERT_FALSE(sync->ok);
    TEST_ASSERT_EQUAL_UINT32(1, rfg_test_error_count());
    TEST_ASSERT_EQUAL_UINT8(SG_ERR_RFG, rfg_test_last_error());
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, -1.0f, (float)sync->value);
}

void test_rfg_arrived_ignore_when_idle(void) {
    rfg_test_reset();
    // do not call prepare_wait -> remains IDLE

    rfg_test_arrived(";", NULL, 0);

    const rfg_test_sync_t *sync = rfg_test_get_sync();
    TEST_ASSERT_FALSE(sync->have);
    TEST_ASSERT_FALSE(sync->ok);
}

void test_parse_ascii_rfg_single_query(void) {
    rfg_test_reset();
    rfg_test_prepare_wait();
    parse_ascii_rfg_test_reset();

    parse_ascii_rfg_test_feed("RF? 55.5;\r\n");

    const rfg_test_sync_t *sync = rfg_test_get_sync();
    TEST_ASSERT_TRUE(sync->have);
    TEST_ASSERT_TRUE(sync->ok);
    TEST_ASSERT_EQUAL_STRING("RF?", sync->reply);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 55.5f, (float)sync->value);
}

void test_rf_cmd_is_on_variants(void) {
    TEST_ASSERT_EQUAL_UINT8(1, rf_cmd_is_on_test("RF ON", NULL, 0));
    TEST_ASSERT_EQUAL_UINT8(1, rf_cmd_is_on_test("RF", "1.0", 1));
    TEST_ASSERT_EQUAL_UINT8(0, rf_cmd_is_on_test("RF", "0.2", 1));
    TEST_ASSERT_EQUAL_UINT8(0, rf_cmd_is_on_test("OTHER", "10", 1));
}
int main(void) {

	UNITY_BEGIN();
	RUN_TEST(test_gasbox_frame_simple);
	RUN_TEST(test_gb_parser);
	RUN_TEST(test_binsearch_found);
	RUN_TEST(test_parse_ascii_write_ok);
	RUN_TEST(test_rfg_arrived_ack);
	RUN_TEST(test_rfg_arrived_numeric);
	RUN_TEST(test_rfg_arrived_error_code);
	RUN_TEST(test_rfg_arrived_warning_sets_error);
	RUN_TEST(test_rfg_arrived_ignore_when_idle);
	RUN_TEST(test_parse_ascii_rfg_single_query);
	RUN_TEST(test_rf_cmd_is_on_variants);
	return UNITY_END();
}
