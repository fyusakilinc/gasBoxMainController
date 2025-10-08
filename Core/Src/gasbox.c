#include <string.h>
#include <stdio.h>
#include "remote.h"
#include "uart4.h"
#include "usart.h"
#include "protocoll.h"
#include "stacks.h"
#include "resultqueue.h"
#include "zentrale.h"
#include "cmdlist.h"
#include "SG_global.h"
#include "gasbox.h"

// ---- framing ----
#define RMT_MAX_PAKET_LENGTH  14
#define GB_DLE               0x3D   // '='
#define GB_SOT               0x53   // 'S'
#define GB_EOT               0x45   // 'E'

// parser states
#define RMT_WAIT_FOR_PAKET_START  1
#define RMT_READ_PAKET            2
#define RMT_PARSE_PAKET           3

//----- PRIVATE DEFINES -------------------------------------------------------
// die Zustandsdefinitionen fuer die Zustandsautomaten im ASCII-PROTOKOLL
#define get_cmd 1
#define get_sign 2
#define get_val 3
#define proc_cmd 4


//Definition der Einstellungsparameter für die Kommunikation mit ASCII-PROTOKOLL
volatile static uint8_t verbose=0;
volatile static uint8_t crlf=0;
volatile static uint8_t echo=0;
volatile static uint8_t sloppy=0;

// ---- internal parser storage ----
static volatile uint8_t msg[RMT_MAX_PAKET_LENGTH + 1];
static volatile uint8_t nzeichen = 0;       // bytes buffered from UART ring
static uint8_t state = RMT_WAIT_FOR_PAKET_START;

static uint8_t bufferRx[RMT_MAX_PAKET_LENGTH + 1];
static uint8_t lengthRx = 0;
static uint8_t dleFlag = 0;
static uint8_t checksum = 0;

static void parse_binary_gasbox(void);
void gb_on_frame(uint8_t cmd, uint8_t status, uint16_t value);

//----- GASBOX CONTROLLER -------------------------------------------------

// Gasbox (UART4, binary)
void gb_sero_get(void)
{
    nzeichen = 0;
    while ((rb_rx_used(&uart5_rb) > 0) && (nzeichen < RMT_MAX_PAKET_LENGTH)) {
        msg[nzeichen++] = (uint8_t)uartRB_Getc(&uart5_rb);
    }
    if (nzeichen) parse_binary_gasbox();
}


// ---- parser  ----
static void parse_binary_gasbox(void) {
	uint8_t data;
	uint8_t ptr = 0;

	do {
		switch (state) {
		case RMT_WAIT_FOR_PAKET_START: {
			// scan for DLE 'S'
			while (ptr < nzeichen) {
				data = msg[ptr++];
				if (dleFlag) {
					// second control char after DLE
					if (data == GB_DLE) {
						// interpret as literal DLE
						dleFlag = 0;
						// (no payload yet in WAIT state)
					} else if (data == GB_SOT) {
						// start of frame
						lengthRx = 0;
						checksum = 0;
						dleFlag = 0;
						state = RMT_READ_PAKET;
						break;
					} else {
						// other control -> ignore, keep scanning
						dleFlag = 0;
					}
				} else {
					if (data == GB_DLE)
						dleFlag = 1;
				}
			}
		}
			break;

		case RMT_READ_PAKET: {
			while (ptr < nzeichen) {
				data = msg[ptr++];

				// avoid runaway frames
				if (lengthRx > RMT_MAX_PAKET_LENGTH) {
					dleFlag = 0;
					state = RMT_WAIT_FOR_PAKET_START;
					break;
				}

				if (dleFlag) {
					if (data == GB_DLE) {
						// stuffed DLE as data
						dleFlag = 0;
						bufferRx[lengthRx++] = GB_DLE;
						checksum += GB_DLE;
					} else if (data == GB_SOT) {
						// unexpected new start → restart frame
						lengthRx = 0;
						checksum = 0;
						dleFlag = 0;
						state = RMT_READ_PAKET;
						break;
					} else if (data == GB_EOT) {
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
						bufferRx[lengthRx++] = data;
						checksum += data;
					}
				}
			}
		}
			break;

		case RMT_PARSE_PAKET: {
		    // Expect 4 payload bytes + 1 checksum (net length 5)
		    if (lengthRx == 5) {
		        uint8_t cmd    = bufferRx[0];
		        uint8_t status = bufferRx[1];
		        uint8_t pH     = bufferRx[2];
		        uint8_t pL     = bufferRx[3];
		        uint8_t cks    = bufferRx[4];

		        // checksum over the 4 payload bytes
		        uint8_t sum = (uint8_t)(cmd + status + pH + pL);

		        if (sum == cks) {
		            uint16_t val = ((uint16_t)pH << 8) | pL;
		            // Publish to mailbox: if someone is waiting for this cmd, wake them.
		            gb_on_frame(cmd, status, val);
		        }
		        // else: bad checksum -> drop silently (or raise an error flag if you want)
		    }
		    // reset for next frame
		    state     = RMT_WAIT_FOR_PAKET_START;
		    lengthRx  = 0;
		    checksum  = 0;
		    dleFlag   = 0;
		} break;
		}
	} while (ptr < nzeichen);
}


static inline uint8_t gb_sum8(const uint8_t *p, int n){
    uint32_t s = 0;
    for (int i=0; i<n; ++i) s += p[i];
    return (uint8_t)s;
}

static inline void gb_push_escaped(uint8_t **wp, uint8_t b){
    *(*wp)++ = b;
    if (b == GB_DLE) *(*wp)++ = b;  // double any DLE in-band
}

/**
 * Build + queue one framed command to the gasbox on UART4.
 * payload = [ cmd, 0x00, param_H, param_L ] ; cks = sum(payload)
 * Returns 1 if queued, 0 if TX ring had no room.
 */
uint8_t gasbox_send(uint8_t cmd, uint16_t param)
{
    uint8_t payload[4] = {
        cmd,
        0x00,                               // reserved/status=0 in requests
        (uint8_t)(param >> 8),
        (uint8_t)(param & 0xFF)
    };
    uint8_t cks = gb_sum8(payload, 4);

    // Worst case: 2 (DLE,S) + each of 5 bytes doubled + 2 (DLE,E) = 14
    uint8_t frame[16], *w = frame;

    *w++ = GB_DLE; *w++ = GB_SOT;
    for (int i=0; i<4; ++i) gb_push_escaped(&w, payload[i]);
    gb_push_escaped(&w, cks);
    *w++ = GB_DLE; *w++ = GB_EOT;

    uint8_t len = (uint8_t)(w - frame);

    // queue to UART4 ring
    if (!uartRB_Put(&uart5_rb, (char*)frame, len)) return 0;
    uartRB_KickTx(&uart5_rb);
    return 1;
}

// ---- sync mailbox ----
static struct {
    volatile uint8_t waiting;
    volatile uint8_t expect_cmd;
    volatile uint8_t have;
    GbReply          r;
} gb_sync = {0};

void gb_on_frame(uint8_t cmd, uint8_t status, uint16_t value)
{
    // deliver to a waiting xfer if it matches the command we sent
    if (gb_sync.waiting && gb_sync.expect_cmd == cmd) {
        gb_sync.r.cmd = cmd; gb_sync.r.status = status; gb_sync.r.value = value;
        gb_sync.have = 1;
        gb_sync.waiting = 0;
        return;
    }
    // else: unsolicited → raise events / z_set_error(...) as you like
}

/**
 * Send one request and wait for its echo parsed by the always-on gb_sero_get().
 * Returns 1 on success (out filled), 0 on timeout or queue failure.
 */
uint8_t gasbox_xfer(uint8_t cmd, uint16_t param, GbReply *out, uint32_t timeout_ms)
{
    // only one outstanding transaction
    if (gb_sync.waiting) return 0;

    gb_sync.expect_cmd = cmd;
    gb_sync.have = 0;
    gb_sync.waiting = 1;

    if (!gasbox_send(cmd, param)) { gb_sync.waiting = 0; return 0; }

    uint32_t t0 = HAL_GetTick();
    while (!gb_sync.have) {
        if ((HAL_GetTick() - t0) > timeout_ms) {
            gb_sync.waiting = 0;
            return 0;
        }
        // do NOT call gb_sero_get() here; main loop owns it
    }
    *out = gb_sync.r;
    return 1;
}
