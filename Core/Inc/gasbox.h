void gb_sero_get(void);

// one parsed reply
typedef struct {
    uint8_t  cmd;
    uint8_t  status;    // 0x80 = OK
    uint16_t value;     // paramH:paramL
} GbReply;


uint8_t gasbox_send(uint8_t cmd, uint16_t param);
uint8_t gasbox_xfer(uint8_t cmd, uint16_t param, GbReply *out, uint32_t timeout_ms);
