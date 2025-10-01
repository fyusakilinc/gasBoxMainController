// Status - Definitionen
#include <stdint.h>
// Status - Definitionen
#define Z_INVALID     	0
#define Z_ERROR         1
#define Z_STARTINIT     3
#define Z_INACTIVE      9
#define Z_START         10
#define Z_ACTIVE        11
#define Z_COOLDOWN		12
#define Z_NOP           255

#define z_rmt_off      	0
#define z_rmt_rs232		1
#define z_rmt_unknown   255

#define z_rmt_delay     500

//#define PARAMETERSETSUCCESSFULL     0x00
//#define PARAMETERINVALID            0x01
//#define PARAMETERCLIPEDMIN          0x02
//#define PARAMETERCLIPEDMAX          0x03
//#define UNKNOWNCOMMAND              0x06
//#define COMMANDDENIED               0x07
//#define PARAMETERADJUSTED           0x0A
//#define WRONGPARAMETERFORMAT        0x0B

#define Z_FLT0  0
#define Z_FLT1  1
#define Z_FLT2  2
#define Z_FLT3  3
#define Z_FLT4  4
#define Z_FLT5  5


// Zentrale initialisieren.
void zentrale_init(void);

// Zentrale denken und entscheiden.
void zentrale(void);

// Statuswunsch setzen
void z_set_status_tend(uint8_t);
uint8_t z_get_status(void);

void z_set_error(uint8_t errnr);
void z_clear_errors(void);
uint64_t z_get_errors(void);
uint8_t z_err_reset(void);

void z_set_channel_error(uint8_t, uint8_t);
void z_clear_channel_errors(uint8_t);
uint16_t z_get_channel_errors(uint8_t);

uint8_t z_get_opmode(void);
uint8_t z_set_opmode(uint8_t);

uint8_t z_set_rf(uint8_t);
uint8_t z_get_rf(void);

//uint16_t z_get_pf_act_remote(uint8_t);
//uint16_t z_get_pr_act_remote(uint8_t);
//uint16_t z_get_u_act_lcd(void);
//uint16_t z_get_t_act_lcd(uint8_t);

uint8_t z_set_peak_filter_delay(uint8_t);
uint16_t z_get_peak_filter_delay(void);
uint8_t z_peak_filter_rst(void);

uint8_t z_set_rmt(uint8_t);
uint8_t z_get_rmt(void);

uint16_t z_get_lcdVal_filt(uint8_t);
uint16_t z_get_lcdVal_filt_chn(uint8_t, uint8_t, uint8_t);

uint8_t z_set_adm(uint16_t);
uint8_t z_get_adm(void);
