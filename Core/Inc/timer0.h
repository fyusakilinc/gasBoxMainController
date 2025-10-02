void timer0_incTick(void);

// Timer 0 Initialisierung
void timer2_init(void);

// START Stoppuhr abfragen
uint8_t ct_start_null(void);

// START Stoppuhr setzen
void set_ct_start(uint16_t);

// HEARTBEAT Stoppuhr abfragen
uint8_t ct_hbeat_null(void);

// HEARTBEAT Stoppuhr setzen
void set_ct_hbeat(uint16_t);

// HEARTBEAT Stoppuhr abfragen
uint8_t ct_mio_hbeat_null(void);

// HEARTBEAT Stoppuhr setzen
void set_ct_mio_hbeat(uint16_t);

// HEARTBEAT Stoppuhr abfragen
uint8_t ct_mcp_hbeat_null(void);

// HEARTBEAT Stoppuhr setzen
void set_ct_mcp_hbeat(uint16_t);

// INIT Stoppuhr abfragen
uint8_t ct_init_null(void);

// INIT Stoppuhr setzen
void set_ct_init(uint16_t);

// Stoppuhr slave setzen
void set_ct_ctl(uint16_t);

// Stoppuhr slave abfragen
uint8_t ct_ctl_null(void);

// Stoppuhr slave setzen
void set_ct_cmd(uint16_t);
uint8_t ct_cmd_null(void);

// Stoppuhr Reset Errors setzen
void set_ct_rst_err(uint16_t);
// Stoppuhr Reset Errors abfragen
uint8_t ct_rst_err_null(void);
// Stoppuhr Reset Device setzen
void set_ct_rst_dev(uint16_t);
// Stoppuhr Reset Device abfragen
uint8_t ct_rst_dev_null(void);


uint8_t ct_zreset_null(void);
void set_ct_zreset(uint16_t);


