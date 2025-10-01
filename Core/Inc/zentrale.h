// Status - Definitionen
#include <stdint.h>
// Status - Definitionen
#define ZERROR 			1
#define POWERON         2
#define STARTDELAY      7
#define RAMPUP          8
#define INACTIVE        9
//#define START           10
#define ACTIVE          11
#define NOP             255

#define z_rmt_off       0
#define z_rmt_rs232     1
#define z_rmt_ai        2

#define profile_min     0
#define profile_max     5

#define SANITYCHECK_MODE_OFF    0
#define SANITYCHECK_MODE_ON     1


// Zentrale initialisieren.
void zentrale_init(void);

// Zentrale denken und entscheiden.
void zentrale(void);

// Statuswunsch setzen
void z_set_status_tend(uint8_t);
uint8_t z_get_opm(void);
uint8_t z_reset(void);
uint8_t z_get_initok(void);

// Error zurückgeben.
void z_set_error(uint8_t errnr);
uint16_t z_get_error(void);

uint8_t z_set_remote_mode(uint8_t);
int32_t z_get_remote_mode(void);

uint8_t z_check_remote_mode(uint8_t);

int32_t z_get_u_act_lcd(void);
int32_t z_get_t1_act_lcd(void);
int32_t z_get_t2_act_lcd(void);

// EEPROM
uint8_t z_store_profile(uint8_t);
uint8_t z_load_profile(uint8_t);
uint8_t z_get_active_profile(void);
uint8_t z_set_default_profile(uint8_t);
uint8_t z_get_default_profile(void);

uint8_t z_set_rf(uint8_t);
uint8_t z_get_rf(void);
uint8_t z_get_status(void);


uint8_t z_set_a_ampphase(uint32_t);
uint8_t z_set_amp_a(int32_t);
int32_t z_get_amp_a(void);
int32_t z_get_pf_a(void);
uint8_t z_set_phase_a(int32_t);
int32_t z_get_phase_a(void);
int32_t z_get_pr_a(void);
uint8_t z_set_apply(void);
uint8_t z_set_freq_all(uint32_t);

uint8_t z_set_b_ampphase(uint32_t);
uint8_t z_set_amp_b(int32_t);
int32_t z_get_amp_b(void);
int32_t z_get_pf_b(void);
uint8_t z_set_phase_b(int32_t);
int32_t z_get_phase_b(void);
int32_t z_get_pr_b(void);

uint8_t z_set_c_ampphase(uint32_t);
uint8_t z_set_amp_c(int32_t);
int32_t z_get_amp_c(void);
int32_t z_get_pf_c(void);
uint8_t z_set_phase_c(int32_t);
int32_t z_get_phase_c(void);
int32_t z_get_pr_c(void);

uint8_t z_set_d_ampphase(uint32_t);
uint8_t z_set_amp_d(int32_t);
int32_t z_get_amp_d(void);
int32_t z_get_pf_d(void);
uint8_t z_set_phase_d(int32_t);
int32_t z_get_phase_d(void);
int32_t z_get_pr_d(void);
