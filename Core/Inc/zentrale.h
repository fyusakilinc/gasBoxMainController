// Status - Definitionen
#include <stdint.h>
#include "cmdlist.h"
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

typedef struct {
    uint8_t cmd_set;   // GB_CMD_SET_MFCn
    uint8_t cmd_get;   // GB_CMD_GET_MFCn
    uint8_t cmd_close; // GB_CMD_CLOSE_MFCn   (if applicable)
} MfcWire;

static const MfcWire kMfc[4] = {
    { GB_CMD_SET_MFC1, GB_CMD_GET_MFC1, GB_CMD_CLOSE_MFC1 },
    { GB_CMD_SET_MFC2, GB_CMD_GET_MFC2, GB_CMD_CLOSE_MFC2 },
    { GB_CMD_SET_MFC3, GB_CMD_GET_MFC3, GB_CMD_CLOSE_MFC3 },
    { GB_CMD_SET_MFC4, GB_CMD_GET_MFC4, GB_CMD_CLOSE_MFC4 },
};

#define GB_TIMEOUT_MS 20


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

uint8_t z_get_status(void);

uint8_t z_mfc_set(uint8_t, uint16_t);
uint8_t z_mfc_get(uint8_t idx, uint16_t *out);
uint8_t z_mfc_close(uint8_t);

uint8_t z_valve_open(uint8_t idx);
uint8_t z_valve_close(uint8_t idx);
uint8_t z_valve_get(uint8_t idx, uint16_t *state);
uint8_t z_gb_err_clr();
uint8_t z_gb_err_get(uint16_t *out_err);
