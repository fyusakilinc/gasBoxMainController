void gb_sero_get(void);

// one parsed reply
typedef struct {
    uint8_t  cmd;
    uint8_t  status;    // 0x80 = OK
    uint16_t value;     // paramH:paramL
} GbReply;

typedef void (*GasboxCallback)(GbReply *r, void *context);


typedef struct {
    uint8_t active;
    uint8_t cmd;
    uint32_t expires;
    GasboxCallback cb;
    void *ctx;
    GbReply reply;
} GbPending;

#define GB_MAX_PENDING 1
static GbPending gb_pending[GB_MAX_PENDING];



uint8_t gasbox_send(uint8_t cmd, uint16_t param);
uint8_t gasbox_xfer(uint8_t cmd, uint16_t param, GbReply *out, uint32_t timeout_ms);
uint8_t gasbox_request(uint8_t cmd, uint16_t param, uint32_t timeout_ms,
		GasboxCallback cb, void *ctx);
