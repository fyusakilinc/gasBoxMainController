#include <stdint.h>

uint8_t gasbox_build_frame_test(uint8_t cmd, uint16_t param, uint8_t *out, uint8_t outcap);
uint8_t parse_binary_gasbox_test(const uint8_t *msg, uint8_t nzeichen,
        uint8_t *out_cmd, uint8_t *out_status, uint16_t *out_val);
#define RMT_MAX_PAKET_LENGTH  20
#define GB_DLE               0x3D   // '='
#define GB_SOT               0x53   // 'S'
#define GB_EOT               0x45   // 'E'

// parser states
#define RMT_WAIT_FOR_PAKET_START  1
#define RMT_READ_PAKET            2
#define RMT_PARSE_PAKET           3
