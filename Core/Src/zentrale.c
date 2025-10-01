#include <stdio.h>
#include <string.h>

#include "zentrale.h"
#include "main.h"
#include "uart4.h"
#include "timer0.h"
#include "hardware.h"
#include "protocoll.h"
#include "SG_global.h"
#include "gasbox.h"

//-----------------PRIVATE-BEREICH---------------------------------------------

static uint8_t z_status;
static uint8_t z_status_h;
static uint8_t z_status_tend;
static uint16_t z_error_akt;                        // aktuelle Fehler
static uint16_t z_error_kum;                        // kumulierte Fehler
static uint8_t z_init_done = 0;
static uint8_t z_rf_state = 0;


//static uint8_t current_profile = 0;

//static uint8_t apply_mode = 0;

const int32_t min_amplitude = 0;
const int32_t max_amplitude = 1023;
const int32_t min_phase = 0;
const int32_t max_phase = 3600;


//-----------------FUNKTIONSDEFINITIONEN---------------------------------------

// Zentrale initialisieren.
void zentrale_init(void) {
	z_status = POWERON;
	z_status_h = NOP;
	z_status_tend = NOP;
	z_error_akt = 0;
	z_error_kum = 0;

}

static void z_update_power_errors(void) {
	// If you only have update_uok():
	if (!update_uok()) {
		// If you want individual bits, read pins directly (or add helpers):
		uint8_t ok5 = u_ok(UC__5VOK_GPIO_Port, UC__5VOK_Pin);
		uint8_t ok12 = u_ok(UC__12VOK_GPIO_Port, UC__12VOK_Pin);
		if (!ok5)
			z_set_error(SG_ERR_U5V);
		if (!ok12)
			z_set_error(SG_ERR_U12V);
	}
}

// Zentrale denken und entscheiden.
void zentrale(void) {	// ----- DENKEN -----

	z_update_power_errors();

	if (readPumpAlarm()) {
		z_set_error(SG_ERR_PUMP_ALARM);
	}
	if (readPumpWarning()) {
		//z_set_error(SG_ERR_PUMP_WARNING);
	}

	// Fehlerüberprüfung
	if (z_error_akt != 0)
		z_set_status_tend(ZERROR);

	switch (z_status)// Je nach aktuellem Systemzustand auf Anfragen zur Zustands�nderung
	{// reagieren
	case ZERROR:
		if (z_status_h != ZERROR) {				// add what to do when error
			z_status_h = ZERROR;
			z_rf_state = 0;
		}

		if ((z_error_akt) > 0) // Wenn aktuell noch ein Fehler mit Ausnahme des ERREXT vorliegt
				{
			z_error_kum |= z_error_akt; // Wir aktualisieren hier schon mal den Kummulierten Fehler
			z_set_status_tend(ZERROR); // kann als Folgezustand nur der Zustand ERROR eingenommen werden!
		} else {       // Falls kein Fehler im Modul vorliegt schließen wir hier
		}

		switch (z_status_tend) // Wünsche bezüglich einer Zustandsänderung bearbeiten
		{
		case POWERON:
			z_status = POWERON;
			break;
		case INACTIVE:
			z_status = INACTIVE;                // Zustand INACTIVE vorbereiten
			break;
		default:
			break;
		}
		z_status_tend = NOP;                         // Status_tend zurücksetzen
		break;

		// Prüfen ob die Wartezeit für einen DTC/Energy - Fehler abgelaufen ist
//			if(((z_error_kum && ((1<<SG_ERREN) + (1<<SG_ERRDTC))) != 0) && (adc7927_chk_en() != 1))
//				z_set_status_tend(Z_ERROR);


	case POWERON:// Startzustand nach Einschalten des Ger�tes. Keine Fehlerbehandlung!
		if (z_status_h != POWERON) {
			z_init_done = 0;
			z_rf_state = 0;
			z_status_h = POWERON;
		}

		if (ct_init_null()) {
			z_init_done = 1;
			z_status = INACTIVE;
		}

		z_status_tend = NOP;					// z_status_tend zur�cksetzen
		break;

	case INACTIVE:						//----- System im INACTIVE-Zustand

		if (z_status_h != INACTIVE) {
			z_error_kum = 0;
			z_rf_state = 0;
			z_status_h = INACTIVE;
		}

		if (z_error_akt != 0)
			z_set_status_tend(ZERROR);

		switch (z_status_tend) {
		case ZERROR:
			z_status = ZERROR;
			break;
		case ACTIVE:
			z_status = ACTIVE;
		default:
			break;
		}
		z_status_tend = NOP;
		break;

	case ACTIVE:
		if (z_status_h != ACTIVE) {
			z_rf_state = 1;
			z_status_h = ACTIVE;
		}

		if (z_error_akt != 0)
			z_set_status_tend(ZERROR);

		switch (z_status_tend) {
		case ZERROR:
			z_status = ZERROR;
			break;
		case INACTIVE:
			z_status = INACTIVE;
		default:
			break;
		}
		z_status_tend = NOP;
		break;

	default:
		z_status_tend = NOP;
		z_status_h = NOP;
		break;
	}
	z_error_kum |= z_error_akt;	// Eventuell aufgetretenen neuen Fehler zur kummulierten
	z_error_akt = 0;// Fehleranzeige für aktuell vorliegende Fehler zurücksetzen

}


// Statuswunsch setzen
// Priorität:   error, inactive, start, active
void z_set_status_tend(uint8_t statnew) {
	if (z_status_tend > statnew)
		z_status_tend = statnew;    // nur Status höherer Priorität übernehmen

	return;
}

uint16_t z_get_error(void) {
	return z_error_kum;
}

// Fehler an Zentrale melden
void z_set_error(uint8_t errnr) {
	if (errnr <= 15)
		z_error_akt |= (1 << errnr);
}

uint8_t z_get_status(void) {
	return z_status;
}

uint8_t z_reset(void) {
	z_error_kum = 0;
	if (z_status == ERROR) {
		z_set_status_tend(INACTIVE);
	}
	return CMR_SUCCESSFULL;
}


uint8_t z_mfc_set(uint8_t idx, uint16_t val) {
    GbReply r;
    return gasbox_xfer(kMfc[idx].cmd_set, val, &r, GB_TIMEOUT_MS) && (r.status == GB_STATUS_OK);
}
uint8_t z_mfc_get(uint8_t idx, uint16_t *out) {
    GbReply r;
    if (!gasbox_xfer(kMfc[idx].cmd_get, 0, &r, GB_TIMEOUT_MS) || r.status != GB_STATUS_OK) return 0;
    *out = r.value; return 1;
}
uint8_t z_mfc_close(uint8_t idx) {
    GbReply r;
    return gasbox_xfer(kMfc[idx].cmd_close, 0, &r, GB_TIMEOUT_MS) && (r.status == GB_STATUS_OK);
}

static inline uint8_t gb_do(uint8_t cmd, uint16_t param) {
    GbReply r;
    return gasbox_xfer(cmd, param, &r, GB_TIMEOUT_MS) && (r.status == GB_STATUS_OK);
}

static inline uint8_t gb_get16(uint8_t cmd, uint16_t *out) {
    GbReply r;
    if (!gasbox_xfer(cmd, 0, &r, GB_TIMEOUT_MS) || r.status != GB_STATUS_OK) return 0;
    *out = r.value; return 1;
}

uint8_t z_valve_open(uint8_t idx)  {
	return gb_do(idx==3 ? GB_CMD_VALVE3_OPEN  : GB_CMD_VALVE4_OPEN, 0);
}
uint8_t z_valve_close(uint8_t idx) {
	return gb_do(idx==3 ? GB_CMD_VALVE3_CLOSE : GB_CMD_VALVE4_CLOSE, 0);
}
uint8_t z_valve_get(uint8_t idx, uint16_t *state) {
    return gb_get16(idx==3 ? GB_CMD_VALVE3_GET : GB_CMD_VALVE4_GET, state);
}
uint8_t z_gb_err_clr(){
	return gb_do(GB_CMD_CLR_ERR, 0) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
}
uint8_t z_gb_err_get(uint16_t *out_err) {
    return gb_get16(GB_CMD_GET_ERR, out_err) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
}

