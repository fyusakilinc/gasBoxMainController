#include "functions.h"
#include "unity.h"
#include <string.h>

static inline uint8_t gb_sum8_test(const uint8_t *p, int n){
    uint32_t s = 0;
    for (int i=0; i<n; ++i) s += p[i];
    return (uint8_t)s;
}

static inline void gb_push_escaped_test(uint8_t **wp, uint8_t b){
    *(*wp)++ = b;
    if (b == GB_DLE) *(*wp)++ = b;  // double any DLE in-band
}


uint8_t gasbox_build_frame_test(uint8_t cmd, uint16_t param, uint8_t *out, uint8_t outcap)
{
    if (!out || outcap < 16) return 0; // worst-case is 14; 16 keeps it simple

    uint8_t payload[4] = {
        cmd,
        0x00,                               // reserved/status
        (uint8_t)(param >> 8),
        (uint8_t)(param & 0xFF)
    };
    uint8_t cks = gb_sum8_test(payload, 4);

    uint8_t *w = out;

    *w++ = GB_DLE; *w++ = GB_SOT;
    for (int i = 0; i < 4; ++i) gb_push_escaped_test(&w, payload[i]);
    gb_push_escaped_test(&w, cks);
    *w++ = GB_DLE; *w++ = GB_EOT;

    return (uint8_t)(w - out);
}

static uint8_t lengthRx = 0;
static uint8_t dleFlag = 0;
static uint8_t checksum = 0;
static uint8_t bufferRx[RMT_MAX_PAKET_LENGTH + 1];

uint8_t parse_binary_gasbox_test(const uint8_t *msg, uint8_t nzeichen,
        uint8_t *out_cmd, uint8_t *out_status, uint16_t *out_val) {

	uint8_t data;
	uint8_t ptr = 0;
	uint8_t state = RMT_WAIT_FOR_PAKET_START;

	do {
		switch (state) {
		case RMT_WAIT_FOR_PAKET_START: {
			printf("pointer = %d\n",ptr);
			printf("arrived wait\n");
			// scan for DLE 'S'
			while (ptr < nzeichen) {
				data = msg[ptr++];
				printf("pointer = %d\n",ptr);
				if (dleFlag) {
					// second control char after DLE
					if (data == GB_DLE) {
						printf("data = gb_dle, pointer = %d\n",ptr);
						// interpret as literal DLE
						dleFlag = 0;
						// (no payload yet in WAIT state)
					} else if (data == GB_SOT) {
						printf("data = gb_sot, pointer = %d\n",ptr);
						printf("len=%u byte=%02X\n", lengthRx, data);
						// start of frame
						lengthRx = 0;
						checksum = 0;
						dleFlag = 0;
						state = RMT_READ_PAKET;
						break;
					} else {
						printf("data = else, pointer = %d\n",ptr);
						// other control -> ignore, keep scanning
						dleFlag = 0;
					}
				} else {
					if (data == GB_DLE){
						printf("data = gb_dle, pointer = %d\n",ptr);
						dleFlag = 1;}
				}
			}
		}
			break;

		case RMT_READ_PAKET: {
			printf("arrived read\n");
			while (ptr < nzeichen) {
				data = msg[ptr++];

				// avoid runaway frames
				if (lengthRx >= RMT_MAX_PAKET_LENGTH) {
					dleFlag = 0;
					state = RMT_WAIT_FOR_PAKET_START;
					break;
				}

				if (dleFlag) {
					if (data == GB_DLE) {
						printf("data = gb_dle, pointer = %d\n",ptr);
						printf("len=%u byte=%02X\n", lengthRx, data);
						// stuffed DLE as data
						dleFlag = 0;
						bufferRx[lengthRx++] = GB_DLE;
						checksum += GB_DLE;
					} else if (data == GB_SOT) {
						printf("data = gb_sot, pointer = %d\n",ptr);
						printf("len=%u byte=%02X\n", lengthRx, data);
						// unexpected new start → restart frame
						lengthRx = 0;
						checksum = 0;
						dleFlag = 0;
						state = RMT_READ_PAKET;
						break;
					} else if (data == GB_EOT) {
						printf("data = gb_eot, pointer = %d\n",ptr);
						printf("len=%u byte=%02X\n", lengthRx, data);
						// proper trailer -> parse
						state = RMT_PARSE_PAKET;
						dleFlag = 0;
						break;
					} else {
						dleFlag = 0; // unknown after DLE -> ignore
					}
				} else {
					if (data == GB_DLE) {
						dleFlag = 1;            // next is control
					} else {
						printf("len=%u byte=%02X, pointer = %d\n", lengthRx, data, ptr);
						bufferRx[lengthRx++] = data;
						checksum += data;
					}
				}
			}
		}
			break;

		case RMT_PARSE_PAKET: {
			printf("arrived parse, pointer = %d\n",ptr);
			// Expect 4 payload bytes + 1 checksum (net length 5)
			if (lengthRx == 5) {
				uint8_t cmd = bufferRx[0];
				uint8_t status = bufferRx[1];
				uint8_t pH = bufferRx[2];
				uint8_t pL = bufferRx[3];
				uint8_t cks = bufferRx[4];

				// checksum over the 4 payload bytes
				uint8_t sum = (uint8_t) (cmd + status + pH + pL);

				if (sum == cks) {
					if (out_cmd)
						*out_cmd = cmd;
					if (out_status)
						*out_status = status;
					if (out_val)
						*out_val = (uint16_t) ((pH << 8) | pL);
					// reset for next frame
					state = RMT_WAIT_FOR_PAKET_START;
					lengthRx = 0;
					cks = 0;
					dleFlag = 0;
					return 1;  // success
				}
				// else: bad checksum -> drop silently (or raise an error flag if you want)
			}
			// reset for next frame
			state = RMT_WAIT_FOR_PAKET_START;
			lengthRx = 0;
			checksum = 0;
			dleFlag = 0;
		}
			break;
		}
	} while (ptr <= nzeichen);
	return 0; // not enough bytes yet
}




