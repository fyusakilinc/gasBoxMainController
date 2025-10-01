#include <stdio.h>
#include <string.h>


#include "zentrale.h"

#include "uart4.h"
#include "timer0.h"
#include "hardware.h"
#include "protocoll.h"
#include "SG_global.h"

//-----------------PRIVATE-BEREICH---------------------------------------------

static uint8_t z_status;
static uint8_t z_status_h;
static uint8_t z_status_tend;
static uint64_t z_error_akt;						// aktuelle Fehler
static uint64_t z_error_kum;						// kumulierte Fehler
static uint16_t z_chn_error_akt[4]={0, 0, 0, 0};
static uint16_t z_chn_error_kum[4]={0, 0, 0, 0};

static uint8_t z_init_done = 0;

static uint8_t z_ctrl_src = 0;

static uint8_t z_adm_cnt = 0;
static uint8_t z_adm = 0;

//-----------------FUNKTIONSDEFINITIONEN---------------------------------------

// Zentrale initialisieren.
void zentrale_init(void)
{	z_status = Z_STARTINIT;
	z_init_done = 0;
	z_status_tend = Z_NOP;
	z_error_akt = 0;
	z_error_kum = 0;
	for(uint8_t i=0; i<4; i++)
	{
		z_chn_error_akt[i] = 0;
		z_chn_error_kum[i] = 0;
	}

	z_adm = 0;

}

static void z_update_power_errors(void)
{
    // If you only have update_uok():
    if (!update_uok()) {
        // If you want individual bits, read pins directly (or add helpers):
        uint8_t ok5  = u_ok(UC__5VOK_GPIO_Port,  UC__5VOK_Pin);
        uint8_t ok12 = u_ok(UC__12VOK_GPIO_Port, UC__12VOK_Pin);
        if (!ok5)  z_set_error(SG_ERR_U5V);
        if (!ok12) z_set_error(SG_ERR_U12V);
    }
}

// Zentrale denken und entscheiden.
void zentrale(void)
{	// ----- DENKEN -----

	z_update_power_errors();

	if (readPumpAlarm()) {
	    z_set_error(SG_ERR_PUMP_ALARM);
	}
	if (readPumpWarning()) {
	    //z_set_error(SG_ERR_PUMP_WARNING);
	}

	// Fehlerüberprüfung
	if (z_error_akt != 0)
		z_set_status_tend(Z_ERROR);

	switch(z_status)															// Je nach aktuellem Systemzustand auf Anfragen zur Zustands�nderung
	{																		// reagieren
		case Z_ERROR:
			if (z_status_h != Z_ERROR)
			{// add what to do when error
				z_status_h = Z_ERROR;
			}

			// Prüfen ob die Wartezeit für einen DTC/Energy - Fehler abgelaufen ist
//			if(((z_error_kum && ((1<<SG_ERREN) + (1<<SG_ERRDTC))) != 0) && (adc7927_chk_en() != 1))
//				z_set_status_tend(Z_ERROR);

			switch(z_status_tend)												// W�nsche bez�glich einer Zustands�nderung bearbeiten
			{
				case Z_STARTINIT:
				case Z_INACTIVE:
					{	if (((z_error_kum & SG_CRITICAL_ERRORS) == 0) && (z_init_done == 1))
						{
							z_status = Z_COOLDOWN;
						}
						else
						{	z_status = Z_STARTINIT;
						}
					}
					break;
				default:
					break;
			}
			z_status_tend = Z_NOP;											// z_status_tend zur�cksetzen
			break;

		case Z_STARTINIT:																// Startzustand nach Einschalten des Ger�tes. Keine Fehlerbehandlung!
				if(z_status_h != Z_STARTINIT)
				{
					z_init_done = 0;
					z_clear_errors();							// Messen der Spannung an der Sicherheitsschleif aktivieren
					z_status_h = Z_STARTINIT;
				}

				if (ct_init_null())
				{
					z_init_done = 1;
					z_status = Z_INACTIVE;
				}

				switch(z_status_tend)
				{	case Z_ERROR: z_status = Z_ERROR;
						break;
					default:
						break;
				}
				z_status_tend = Z_NOP;											// z_status_tend zur�cksetzen
				break;

		case Z_INACTIVE:														//----- System im INACTIVE-Zustand

			if(z_status_h != Z_INACTIVE)
			{
				//hw_set_rf(0);
				//smps_on(0);
			    z_error_kum = 0;
				for(uint8_t i=0; i<4; i++)
				{
					z_chn_error_kum[i] = 0;
				}
				//hw_set_nmop(1);
				//hw_set_ledspeed(500);
				z_status_h = Z_INACTIVE;
			}

			if (z_error_akt != 0)	z_set_status_tend(Z_ERROR);

			switch(z_status_tend)
			{	case Z_ERROR: z_status = Z_ERROR;
					break;
				case Z_ACTIVE:
				case Z_START: z_status = Z_START;
					break;
				default:
					break;
			}
			z_status_tend = Z_NOP;
			break;


		case Z_START:																//----- System im START-Zustand

			if (z_status_h != Z_START)
			{
				//set_ct_start(2500); // was 4500
			    //smps_on(1);
				z_status_h = Z_START;
			}

			// Hier sollte getcheckt werde, ob alle Slaves aktiv sind!
			if ((ct_start_null() == 1) || update_uok())
				z_status = Z_ACTIVE;

			if ((z_error_akt != 0) && ct_start_null()) z_set_status_tend(Z_ERROR);

			switch(z_status_tend)
			{	case Z_ERROR: z_status = Z_ERROR;
					break;
				case Z_INACTIVE: z_status = Z_INACTIVE;
					break;
				default:
					break;
			}
			z_status_tend = Z_NOP;
			break;


		case Z_ACTIVE:
			if (z_status_h != Z_ACTIVE)
			{
				//hw_set_rf(1);
				z_status_h = Z_ACTIVE;
			}

			if (z_error_akt != 0) z_set_status_tend(Z_ERROR);

			switch(z_status_tend)
			{	case Z_ERROR: z_status = Z_ERROR;
					break;
				case Z_INACTIVE:
					if(1)
						z_status = Z_INACTIVE;
					else
						z_status = Z_COOLDOWN;
					break;
				default:
					break;
			}
			z_status_tend = Z_NOP;
			break;

		case Z_COOLDOWN:
			if (z_status_h != Z_COOLDOWN)
			{
				//hw_set_rf(0);
			   // smps_on(0);
			    z_error_kum = 0;
				for(uint8_t i=0; i<4; i++)
				{
					z_chn_error_kum[i] = 0;
				}
				//hw_set_nmop(1);
				//hw_set_ledspeed(500);
				z_status_h = Z_COOLDOWN;
			}

			if (z_error_akt != 0) z_set_status_tend(Z_ERROR);


			switch(z_status_tend)
			{	case Z_ERROR: z_status = Z_ERROR;
					break;
				default:
					break;
			}
			z_status_tend = Z_NOP;
			break;

		default:
			z_status_tend = Z_NOP;
			z_status_h = Z_NOP;
			break;
	}
	z_error_kum |= z_error_akt;		// Eventuell aufgetretenen neuen Fehler zur kummulierten
	z_error_akt = 0;				// Fehleranzeige für aktuell vorliegende Fehler zurücksetzen

	for(uint8_t i=0; i<4; i++)
	{
		z_chn_error_kum[i] |= z_chn_error_akt[i];
		z_chn_error_akt[i] = 0;
	}

	z_error_kum &= 0xFFF0FFF;
	if(z_chn_error_kum[0] != 0)
		z_error_kum |= (1ul << SG_ERRCH1);
	if(z_chn_error_kum[1] != 0)
		z_error_kum |= (1ul << SG_ERRCH2);
	if(z_chn_error_kum[2] != 0)
		z_error_kum |= (1ul << SG_ERRCH3);
	if(z_chn_error_kum[3] != 0)
		z_error_kum |= (1ul << SG_ERRCH4);

}

// z_status zur�ckgeben.
uint8_t z_get_status(void)
{
	return z_status;
}

// Statuswunsch setzen
// Priorit�t:	error, inactive, start, active
void z_set_status_tend(uint8_t statnew)
{	if (z_status_tend > statnew) z_status_tend = statnew;	// nur z_status h�herer Priorit�t �bernehmen
	return;
}

// Fehler an Zentrale melden
void z_set_error(uint8_t errnr)
{	if (errnr <= 63) z_error_akt |= (1ull << errnr);
}

void z_clear_errors(void)
{
	z_error_kum = 0;
	for(uint8_t i=0; i<4; i++)
	{
		z_chn_error_kum[i] = 0;
	}
}

// Error zur�ckgeben.
uint64_t z_get_errors(void)
{
	if(z_status == Z_ERROR)
		return z_error_kum;
	else
		return 0;
}

void z_set_channel_error (uint8_t chn, uint8_t errnr)
{
	if (errnr <= 63) z_error_akt |= (1ull << errnr);

	if((chn >= 0) && (chn <= 3))
	{
		if (errnr <= 15) z_chn_error_akt[chn] |= (1ull << errnr);
	}
}

void z_clear_channel_errors(uint8_t chn)
{
	if((chn >= 0) && (chn <= 3))
	{
		z_chn_error_kum[chn] = 0;
	}
}

uint16_t z_get_channel_errors(uint8_t chn)
{
	if((chn >= 0) && (chn <= 3))
	{
		return z_chn_error_kum[chn];
	}
	else
	{
		return 0;
	}
}

// Funktionen f�r Remote
uint8_t z_set_rmt(uint8_t src)
{
	static uint8_t ctrl_src_h = 99;
	uint8_t ret_val = CMR_SUCCESSFULL;

	if(ct_ctl_null() == 1)
	{
		switch(src)
		{
			case z_rmt_off:
				z_ctrl_src = z_rmt_off;
				break;

			case z_rmt_rs232:
				z_ctrl_src = z_rmt_rs232;
				break;

			default:
				ret_val = CMR_PARAMETERINVALID;
				break;
		}
		if(z_ctrl_src != ctrl_src_h)
		{
			set_ct_ctl(z_rmt_delay);
			ctrl_src_h = z_ctrl_src;
		}
		return ret_val;
	}
	else
	{
		return CMR_UNITBUSY;
	}
}

uint8_t z_get_rmt(void)
{
	return z_ctrl_src;
}


uint8_t z_set_adm(uint16_t pin)
{
	if(z_adm_cnt < 3)
	{	if(pin == SG_ADM_PIN)
		{	z_adm = 1;
			z_adm_cnt = 0;
			return CMR_SUCCESSFULL;
		}
		else if (pin == 0)
		{	z_adm = 0;
			return CMR_SUCCESSFULL;
		}
		else
		{	z_adm = 0;
			if(z_adm_cnt < 3) z_adm_cnt++;
			return CMR_PARAMETERINVALID;
		}
	}
	else
	{
		return CMR_COMMANDDENIED;
	}
}

uint8_t z_get_adm(void)
{
	return z_adm;
}

uint16_t z_get_peak_filter_delay(void)
{
	return 100;
}

uint8_t z_set_peak_filter_delay(uint8_t flt)
{
	return CMR_COMMANDNOTSUPPORTED;
}

uint8_t z_peak_filter_rst(void)
{
	return CMR_COMMANDNOTSUPPORTED;
}



uint8_t z_err_reset(void)
{
	if(z_status == Z_ERROR)
	{
		z_set_status_tend(Z_INACTIVE);
	}
	return CMR_SUCCESSFULL;
}
