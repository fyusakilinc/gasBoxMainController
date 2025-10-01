#include <stdio.h>
#include <string.h>

#include "zentrale.h"
#include "main.h"
#include "uart4.h"
#include "timer0.h"
#include "hardware.h"
#include "protocoll.h"
#include "SG_global.h"

//-----------------PRIVATE-BEREICH---------------------------------------------

static uint8_t z_status;
static uint8_t z_status_h;
static uint8_t z_status_tend;
static uint16_t z_error_akt;                        // aktuelle Fehler
static uint16_t z_error_kum;                        // kumulierte Fehler
static uint8_t z_init_done = 0;
static uint8_t z_rf_state = 0;

static uint8_t z_remote_mode = 1;

//static uint8_t current_profile = 0;

//static uint8_t apply_mode = 0;

static int32_t chan1_phase = 0, chan2_phase = 0, chan3_phase = 0, chan4_phase =
		0;
static int32_t chan1_amplitude = 0, chan2_amplitude = 0, chan3_amplitude = 0,
		chan4_amplitude = 0;

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

// z_status zurückgeben.
uint8_t z_get_initok(void) {
	return z_init_done;
}

uint8_t z_get_opm(void) {
	return z_status;
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

int32_t z_get_remote_mode(void) {
	return z_remote_mode;
}

uint8_t z_chk_remote_mode(uint8_t md) {
	if (md == z_remote_mode)
		return 1;
	else
		return 0;
}

uint8_t z_set_remote_mode(uint8_t opmode) {
	switch (opmode) {
	case z_rmt_off:
		z_remote_mode = z_rmt_off;
		return CMR_SUCCESSFULL;
		break;

	case z_rmt_rs232:
		z_remote_mode = z_rmt_rs232;
		return CMR_SUCCESSFULL;
		break;

	default:
		return CMR_PARAMETERINVALID;
		break;
	}
}

uint8_t z_set_rf(uint8_t x) {
	uint8_t retVal = CMR_SUCCESSFULL;
	switch (x) {
	case 0:
		z_set_status_tend(INACTIVE);
//            z_rf_state = 0;
		break;
	case 1:
		z_set_status_tend(ACTIVE);
//            z_rf_state = 1;
		break;
	default:
		retVal = CMR_PARAMETERINVALID;
	}

	return retVal;
}

uint8_t z_get_rf(void) {
	return z_rf_state;
}

int32_t z_get_pf_a(void) {
	if (z_rf_state == 0)
		return 0;
	else
		return 1;//get_lcd1234Val_filt(PF1);
}

int32_t z_get_pr_a(void) {
	if (z_rf_state == 0)
		return 0;
	else
		return 1;//get_lcd1234Val_filt(PR1);
}

int32_t z_get_pf_b(void) {
	if (z_rf_state == 0)
		return 0;
	else
		return 1;//get_lcd1234Val_filt(PF2);
}

int32_t z_get_pr_b(void) {
	if (z_rf_state == 0)
		return 0;
	else
		return 1;//get_lcd1234Val_filt(PR2);
}

int32_t z_get_pf_c(void) {
	if (z_rf_state == 0)
		return 0;
	else
		return 1;//get_lcd1234Val_filt(PF3);
}

int32_t z_get_pr_c(void) {
	if (z_rf_state == 0)
		return 0;
	else
		return 1;//get_lcd1234Val_filt(PR3);
}

int32_t z_get_pf_d(void) {
	if (z_rf_state == 0)
		return 0;
	else
		return 1;//get_lcd1234Val_filt(PF4);
}

int32_t z_get_pr_d(void) {
	if (z_rf_state == 0)
		return 0;
	else
		return 1;//get_lcd1234Val_filt(PR4);
}

uint8_t z_set_a_ampphase(uint32_t val) {
	int32_t tmp_phase = val & 0x0000FFFF;
	int32_t tmp_amplitude = (val & 0xFFFF0000) >> 16;

	uint8_t retVal = CMR_SUCCESSFULL;

	if (tmp_phase < min_phase) {
		retVal = CMR_PARAMETERADJUSTED;
		chan1_phase = min_phase;
	} else if (tmp_phase > max_phase) {
		retVal = CMR_PARAMETERADJUSTED;
		chan1_phase = max_phase;
	} else {
		chan1_phase = tmp_phase;
	}

	if (tmp_amplitude < min_amplitude) {
		retVal = CMR_PARAMETERADJUSTED;
		chan1_amplitude = min_amplitude;
	} else if (tmp_amplitude > max_amplitude) {
		retVal = CMR_PARAMETERADJUSTED;
		chan1_amplitude = max_amplitude;
	} else {
		chan1_amplitude = tmp_amplitude;
	}

	//dds_set_cpow_x(chan1_phase, 1);
	//dds_set_amp_x(chan1_amplitude, 1);
	//dds_apply();

	return retVal;
}

uint8_t z_set_b_ampphase(uint32_t val) {
	int32_t tmp_phase = val & 0x0000FFFF;
	int32_t tmp_amplitude = (val & 0xFFFF0000) >> 16;

	uint8_t retVal = CMR_SUCCESSFULL;

	if (tmp_phase < min_phase) {
		retVal = CMR_PARAMETERADJUSTED;
		chan2_phase = min_phase;
	} else if (tmp_phase > max_phase) {
		retVal = CMR_PARAMETERADJUSTED;
		chan2_phase = max_phase;
	} else {
		chan2_phase = tmp_phase;
	}

	if (tmp_amplitude < min_amplitude) {
		retVal = CMR_PARAMETERADJUSTED;
		chan2_amplitude = min_amplitude;
	} else if (tmp_amplitude > max_amplitude) {
		retVal = CMR_PARAMETERADJUSTED;
		chan2_amplitude = max_amplitude;
	} else {
		chan2_amplitude = tmp_amplitude;
	}

	//dds_set_cpow_x(chan2_phase, 2);
	//dds_set_amp_x(chan2_amplitude, 2);
	//dds_apply();

	return retVal;
}

uint8_t z_set_c_ampphase(uint32_t val) {
	int32_t tmp_phase = val & 0x0000FFFF;
	int32_t tmp_amplitude = (val & 0xFFFF0000) >> 16;

	uint8_t retVal = CMR_SUCCESSFULL;

	if (tmp_phase < min_phase) {
		retVal = CMR_PARAMETERADJUSTED;
		chan3_phase = min_phase;
	} else if (tmp_phase > max_phase) {
		retVal = CMR_PARAMETERADJUSTED;
		chan3_phase = max_phase;
	} else {
		chan3_phase = tmp_phase;
	}

	if (tmp_amplitude < min_amplitude) {
		retVal = CMR_PARAMETERADJUSTED;
		chan3_amplitude = min_amplitude;
	} else if (tmp_amplitude > max_amplitude) {
		retVal = CMR_PARAMETERADJUSTED;
		chan3_amplitude = max_amplitude;
	} else {
		chan3_amplitude = tmp_amplitude;
	}

	//dds_set_cpow_x(chan3_phase, 3);
	//dds_set_amp_x(chan3_amplitude, 3);
	//dds_apply();

	return retVal;
}

uint8_t z_set_d_ampphase(uint32_t val) {
	int32_t tmp_phase = val & 0x0000FFFF;
	int32_t tmp_amplitude = (val & 0xFFFF0000) >> 16;

	uint8_t retVal = CMR_SUCCESSFULL;

	if (tmp_phase < min_phase) {
		retVal = CMR_PARAMETERADJUSTED;
		chan4_phase = min_phase;
	} else if (tmp_phase > max_phase) {
		retVal = CMR_PARAMETERADJUSTED;
		chan4_phase = max_phase;
	} else {
		chan4_phase = tmp_phase;
	}

	if (tmp_amplitude < min_amplitude) {
		retVal = CMR_PARAMETERADJUSTED;
		chan4_amplitude = min_amplitude;
	} else if (tmp_amplitude > max_amplitude) {
		retVal = CMR_PARAMETERADJUSTED;
		chan4_amplitude = max_amplitude;
	} else {
		chan4_amplitude = tmp_amplitude;
	}

	//dds_set_cpow_x(chan4_phase, 4);
	//dds_set_amp_x(chan4_amplitude, 4);
	//dds_apply();

	return retVal;
}

uint8_t z_set_amp_a(int32_t val) {
	uint8_t retVal = CMR_SUCCESSFULL;

	if (val < min_amplitude) {
		retVal = CMR_PARAMETERCLIPEDMIN;
		chan1_amplitude = min_amplitude;
	} else if (val > max_amplitude) {
		retVal = CMR_PARAMETERCLIPEDMAX;
		chan1_amplitude = max_amplitude;
	} else {
		chan1_amplitude = val;
	}
	//dds_set_amp_x(chan1_amplitude, 1);

	return retVal;
}

int32_t z_get_amp_a(void) {
	return chan1_amplitude;
}

uint8_t z_set_amp_b(int32_t val) {
	uint8_t retVal = CMR_SUCCESSFULL;

	if (val < min_amplitude) {
		retVal = CMR_PARAMETERCLIPEDMIN;
		chan2_amplitude = min_amplitude;
	} else if (val > max_amplitude) {
		retVal = CMR_PARAMETERCLIPEDMAX;
		chan2_amplitude = max_amplitude;
	} else {
		chan2_amplitude = val;
	}
	//dds_set_amp_x(chan2_amplitude, 2);

	return retVal;
}

int32_t z_get_amp_b(void) {
	return chan2_amplitude;
}

uint8_t z_set_amp_c(int32_t val) {
	uint8_t retVal = CMR_SUCCESSFULL;

	if (val < min_amplitude) {
		retVal = CMR_PARAMETERCLIPEDMIN;
		chan3_amplitude = min_amplitude;
	} else if (val > max_amplitude) {
		retVal = CMR_PARAMETERCLIPEDMAX;
		chan3_amplitude = max_amplitude;
	} else {
		chan3_amplitude = val;
	}
	//dds_set_amp_x(chan3_amplitude, 3);

	return retVal;
}

int32_t z_get_amp_c(void) {
	return chan3_amplitude;
}

uint8_t z_set_amp_d(int32_t val) {
	uint8_t retVal = CMR_SUCCESSFULL;

	if (val < min_amplitude) {
		retVal = CMR_PARAMETERCLIPEDMIN;
		chan4_amplitude = min_amplitude;
	} else if (val > max_amplitude) {
		retVal = CMR_PARAMETERCLIPEDMAX;
		chan4_amplitude = max_amplitude;
	} else {
		chan4_amplitude = val;
	}
	//dds_set_amp_x(chan4_amplitude, 4);

	return retVal;
}

int32_t z_get_amp_d(void) {
	return chan4_amplitude;
}

uint8_t z_set_phase_a(int32_t val) {
	uint8_t retVal = CMR_SUCCESSFULL;

	if (val < min_phase) {
		retVal = CMR_PARAMETERCLIPEDMIN;
		chan1_phase = min_phase;
	} else if (val > max_phase) {
		retVal = CMR_PARAMETERCLIPEDMAX;
		chan1_phase = max_phase;
	} else {
		chan1_phase = val;
	}

	//dds_set_cpow_x(chan1_phase, 1);
	return retVal;
}

int32_t z_get_phase_a(void) {
	return chan1_phase;
}

uint8_t z_set_phase_b(int32_t val) {
	uint8_t retVal = CMR_SUCCESSFULL;

	if (val < min_phase) {
		retVal = CMR_PARAMETERCLIPEDMIN;
		chan2_phase = min_phase;
	} else if (val > max_phase) {
		retVal = CMR_PARAMETERCLIPEDMAX;
		chan2_phase = max_phase;
	} else {
		chan2_phase = val;
	}

	//dds_set_cpow_x(chan2_phase, 2);
	return retVal;
}

int32_t z_get_phase_b(void) {
	return chan2_phase;
}

uint8_t z_set_phase_c(int32_t val) {
	uint8_t retVal = CMR_SUCCESSFULL;

	if (val < min_phase) {
		retVal = CMR_PARAMETERCLIPEDMIN;
		chan3_phase = min_phase;
	} else if (val > max_phase) {
		retVal = CMR_PARAMETERCLIPEDMAX;
		chan3_phase = max_phase;
	} else {
		chan3_phase = val;
	}

	//dds_set_cpow_x(chan3_phase, 3);
	return retVal;
}

int32_t z_get_phase_c(void) {
	return chan3_phase;
}

uint8_t z_set_phase_d(int32_t val) {
	uint8_t retVal = CMR_SUCCESSFULL;

	if (val < min_phase) {
		retVal = CMR_PARAMETERCLIPEDMIN;
		chan4_phase = min_phase;
	} else if (val > max_phase) {
		retVal = CMR_PARAMETERCLIPEDMAX;
		chan4_phase = max_phase;
	} else {
		chan4_phase = val;
	}

	//dds_set_cpow_x(chan4_phase, 4);
	return retVal;
}

int32_t z_get_phase_d(void) {
	return chan4_phase;
}

uint8_t z_set_apply(void) {

	uint8_t retVal = CMR_SUCCESSFULL;

	//retVal = dds_apply();
	return retVal;
}

