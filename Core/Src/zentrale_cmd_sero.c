#include <stdint.h>
#include "SG_global.h"
#include "protocoll.h"
#include "cmdlist.h"
#include "stacks.h"
#include "prioritylist.h"
#include "priority_pushpop.h"
#include "resultqueue.h"
#include "zentrale_cmd_sero.h"
#include "uart4.h"
#include "func.h"
#include "zentrale.h"
#include "hardware.h"
#include "gasbox.h"
#include "ad5592.h"
#include "apc.h"
#include "iso.h"
#include "rfg.h"

//--- RFG HELPER FUNCTIONS --------------------------------------------------------------------------------------------------
// Helper function for RFG commands
static void handle_rfg_command(stack_item* cmd, const char* cmd_str, float param, bool is_write, unsigned long timeout_ms) {
	float out = 0.0f;
	if (rfg_xfer(cmd_str, param, is_write, timeout_ms, &out)) {
		// Check response type
		if (out < 0) {
			cmd->cmd_ack = CMR_COMMANDDENIED;  // Unknown response
		} else if (out > 1000) {
			cmd->cmd_ack = (uint8_t)out;  // RFG error code (Exxx;)
		} else {
			cmd->cmd_ack = CMR_SUCCESSFULL;  // Success (; or >OK; or numeric value)
		}
	} else {
		cmd->cmd_ack = CMR_COMMANDDENIED;  // Timeout or communication error
	}
	cmd->par0 = out;
}

// Convenience macros for common RFG command patterns
#define RFG_READ(cmd_str) handle_rfg_command(&cmd, cmd_str, 0, false, 1000)
#define RFG_WRITE(cmd_str) handle_rfg_command(&cmd, cmd_str, cmd.par0, true, 1000)

//--- PRIVATE DEFINES -------------------------------------------------------------------------------------------------------
#define Z_MAX_HIGHPRIO_NUM			5                     //max.Durchdatz der Befehlsverarbeitung für die Befehlen mit der höchsten Priorität
#define Z_MAXCMD                    10                     //max. Durchsatz einer Befehlsverarbeitung

//--- PRIVATE VARIABLES -----------------------------------------------------------------------------------------------------

//--- PRIVATE FUNKTIONSDEKLARATION ----------------------------------------------------------------------------------------
void z_cmd_sero(stack_item cmd);           //den einzelnen Befehl zu verarbeiten

//--- FUNKTIONSDEKLARATIONS -------------------------------------------------------------------------------------------------
//die Mechanimus zur Verarbeitung der Befehle mit den unterschiedlichen Prioritäten

//--- FUNKTIONSDEFINITIONS --------------------------------------------------------------------------------------------------
void z_cmd_scheduler(void)
{
	uint8_t priolevel0_null_flg = 0;
	uint8_t priolevel1_null_flg = 0;
	uint8_t priolevel2_null_flg = 0;

	uint8_t cmdcount = 0;             //Zähler für die zu verarbeitenden Befehle
	uint8_t cmd_flg = 0; //Falls cmd_flg = 1 ist, d.h. keinen Befehl zu verarbeiten; cmd_flg = 0, d.h. noch Befehl zu verarbeiten
	stack_item cmd;

	uint8_t resultflg = get_anzFrei_resultQueue();

	if (resultflg > 1)          //prüft, ob es noch freien Platz in resultqueue.
	{
		do
		{
			while ((cmdcount < Z_MAX_HIGHPRIO_NUM) && (priolevel0_null_flg == 0))
			{
				if (z_priolevel_header[PRIO_LEVEL0] != NONEXT)
				{
					zstack_pop(&cmd, PRIO_LEVEL0);
					z_cmd_sero(cmd);
					cmdcount++;

				}
				else
				{
					priolevel0_null_flg = 1; //es gibt keinen Befehl in der Prioritätliste mit Level 0
				};
			};

			if (z_priolevel_header[PRIO_LEVEL1] != NONEXT) {
				if (cmdcount < Z_MAXCMD) {
					zstack_pop(&cmd, PRIO_LEVEL1);
					z_cmd_sero(cmd);
					cmdcount++;

				};
			} else {
				priolevel1_null_flg = 1; //es gibt keinen Befehl in der Prioritätliste mit Level 1
			};

			if (z_priolevel_header[PRIO_LEVEL2] != NONEXT) {
				if (cmdcount < Z_MAXCMD) {
					zstack_pop(&cmd, PRIO_LEVEL2);
					z_cmd_sero(cmd);
					cmdcount++;
				};
			} else {
				priolevel2_null_flg = 1; //es gibt keinen Befehl in der Prioritätliste mit Level 2
			};

			//prüft, ob die drei Prioritätslisten alle leer sind.
			cmd_flg = priolevel0_null_flg & priolevel1_null_flg;
			cmd_flg &= priolevel2_null_flg;

		} while ((cmdcount < Z_MAXCMD) && (cmd_flg == 0));
	};

}

void z_cmd_sero(stack_item cmd) {

	switch (cmd.cmd_index) {


	// -----------
	// MFC1..MFC4 SET
	case CMD_SET_GAS_PDE: {
		uint16_t p = clamp16((uint16_t) cmd.par0);
		if (!atm_sensor_get())
			cmd.cmd_ack = z_mfc_set(0, p) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		else
			cmd.cmd_ack = CMR_COMMANDDENIED; // these can be changed, we can add cmr_safety_denied or something like that
		break;
	}
	case CMD_SET_GAS_AIR: {
		uint16_t p = clamp16((uint16_t) cmd.par0);
		if (!atm_sensor_get())
			cmd.cmd_ack = z_mfc_set(1, p) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		else
			cmd.cmd_ack = CMR_COMMANDDENIED;
		break;
	}
	case CMD_SET_GAS_O2: {
		uint16_t p = clamp16((uint16_t) cmd.par0);
		if (!atm_sensor_get())
			cmd.cmd_ack = z_mfc_set(2, p) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		else
			cmd.cmd_ack = CMR_COMMANDDENIED;
		break;
	}
	case CMD_SET_GAS_4: {
		uint16_t p = clamp16((uint16_t) cmd.par0);
		if (!atm_sensor_get())
			cmd.cmd_ack = z_mfc_set(3, p) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		else
			cmd.cmd_ack = CMR_COMMANDDENIED;
		break;
	}

		// MFC1..MFC4 GET
	case CMD_GET_GAS_PDE: {
		uint16_t v;
		cmd.cmd_ack = z_mfc_get(0, &v) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		if (cmd.cmd_ack == CMR_SUCCESSFULL)
			cmd.par0 = (double) v;
		break;
	}
	case CMD_GET_GAS_AIR: {
		uint16_t v;
		cmd.cmd_ack = z_mfc_get(1, &v) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		if (cmd.cmd_ack == CMR_SUCCESSFULL)
			cmd.par0 = (double) v;
		break;
	}
	case CMD_GET_GAS_O2: {
		uint16_t v;
		cmd.cmd_ack = z_mfc_get(2, &v) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		if (cmd.cmd_ack == CMR_SUCCESSFULL)
			cmd.par0 = (double) v;
		break;
	}
	case CMD_GET_GAS_4: {
		uint16_t v;
		cmd.cmd_ack = z_mfc_get(3, &v) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		if (cmd.cmd_ack == CMR_SUCCESSFULL)
			cmd.par0 = (double) v;
		break;
	}

		// MFC1..MFC4 CLOSE
	case CMD_CLOSE_GAS_PDE: {
		cmd.cmd_ack = z_mfc_close(0) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		break;
	}
	case CMD_CLOSE_GAS_AIR: {
		cmd.cmd_ack = z_mfc_close(1) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		break;
	}
	case CMD_CLOSE_GAS_O2: {
		cmd.cmd_ack = z_mfc_close(2) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		break;
	}
	case CMD_CLOSE_GAS_4: {
		cmd.cmd_ack = z_mfc_close(3) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		break;
	}

		// Valves OPEN-CLOSE-READ
	case CMD_V3_SET: {
		uint16_t p = (uint16_t) cmd.par0;
		uint8_t ret = 0;
		if (p) {
			if (!(iso_valve_get() || door_switch_get()))
				ret = z_valve_open(3);
		} else {
			ret = z_valve_close(3);
		}
		cmd.cmd_ack = ret ? CMR_SUCCESSFULL : CMR_UNITBUSY;
		break;
	}

	case CMD_V3_GET: {
		uint16_t st;
		if (z_valve_get(3, &st)) {
			cmd.par0 = (double) st;
			cmd.cmd_ack = CMR_SUCCESSFULL;
		} else {
			cmd.cmd_ack = CMR_COMMANDDENIED;
		}
		break;
	}

	case CMD_V4_SET: {
		uint16_t p = (uint16_t) cmd.par0;
		uint8_t ret = 0;
		uint16_t st; // v4 cant be opened when atm, door switch active, and v3 is open
		z_valve_get(3, &st);

		if (p) {
			if (!(st || atm_sensor_get() || door_switch_get()))
				ret = z_valve_open(4);
		} else {
			ret = z_valve_close(4);
		}
		cmd.cmd_ack = ret ? CMR_SUCCESSFULL : CMR_UNITBUSY;
		break;
	}

	case CMD_V4_GET: {
		uint16_t st;
		if (z_valve_get(4, &st)) {
			cmd.par0 = (double) st;
			cmd.cmd_ack = CMR_SUCCESSFULL;
		} else {
			cmd.cmd_ack = CMR_COMMANDDENIED;
		}
		break;
	}

	// -----------

	case CMD_PUMP_SET: {
		uint16_t p = cmd.par0;
		if (p) {
			setStartPump();
		} else {
			setStopPump();
		}
		cmd.cmd_ack = CMR_SUCCESSFULL;
		break;
	}

	case CMD_PUMP_GET /*| CMD_PUMP_GET_STA*/: {                 // "PUM?"
		// return 0/1 = running
		//uint8_t v = pumpRunning();
		//cmd_reply_uint(v);              // <-- use your normal “? ” reply routine
		//cmd.cmd_ack = CMR_SUCCESSFULL;
		break;
	}

	case CMD_PUMP_GET_WAR: {             // "PUM:WAR?"
		uint8_t v = readPumpWarning();         // collective warning bit
		cmd.par0 = (double) v;
		cmd.cmd_ack = CMR_SUCCESSFULL;
		break;
	}

	case CMD_PUMP_GET_ALA: {
		uint8_t v = readPumpAlarm();
		cmd.par0 = (double) v;
		cmd.cmd_ack = CMR_SUCCESSFULL;
		break;
	}

	case CMD_PUMP_GET_RMT: {
		uint8_t v = readPumpRemote();
		cmd.par0 = (double) v;
		cmd.cmd_ack = CMR_SUCCESSFULL;
		break;
	}

	// -----------

	case CMD_SET_T: {
		float v = cmd.par0;
		if (v > 120.0f)
			cmd.cmd_ack = CMR_COMMANDDENIED;
		else {
			set_TC_STP(v);
			cmd.cmd_ack = CMR_SUCCESSFULL;
		}
		break;
	}

	case CMD_GET_T: {
		float v = get_TIST();
		cmd.par0 = (float) v;
		cmd.cmd_ack = CMR_SUCCESSFULL;
		break;
	}

	// -----------

	case CMD_APC_CTL: {
		cmd.cmd_ack = apc_set_acc_mode(cmd.par0) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		break;
	}

	case CMD_APC_AMD_RD: {
		double v;                                      // local storage
		if (apc_get_ctl_mode(&v)) {
			cmd.par0 = (float) v;                   // always an int here
			cmd.cmd_ack = CMR_SUCCESSFULL;
		} else {
			cmd.cmd_ack = CMR_COMMANDDENIED;
		}
		break;
	}

	case CMD_APC_AMD: {
		cmd.cmd_ack = apc_set_ctl_mode(cmd.par0) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		break;
	}
	case CMD_APC_CTL_SEL_RD: {
		double v;
		if (apc_get_ctlr_selector(&v)) {
			cmd.par0 = (float) v;   // safe cast if range fits
			cmd.cmd_ack = CMR_SUCCESSFULL;
		} else {
			cmd.cmd_ack = CMR_COMMANDDENIED;
		}
		break;
	}

	case CMD_APC_CTL_SEL: {
		cmd.cmd_ack = apc_set_ctlr_selector(cmd.par0) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		;
		break;
	}
	case CMD_APC_ERN_RD: {
		double v;                                      // local storage
		if (apc_get_err_num(&v)) {
			cmd.par0 = (float) v;                   // always an int here
			cmd.cmd_ack = CMR_SUCCESSFULL;
		} else {
			cmd.cmd_ack = CMR_COMMANDDENIED;
		}
		break;
	}

	case CMD_APC_ERC_RD: {
		double v;                                      // local storage
		if (apc_get_err_code(&v)) {
			cmd.par0 = (float) v;                   // always an int here
			cmd.cmd_ack = CMR_SUCCESSFULL;
		} else {
			cmd.cmd_ack = CMR_COMMANDDENIED;
		}
		break;
	}
	case CMD_APC_VAL: {
		// 1 -> on, 0 -> off
		uint8_t ok = cmd.par0 ? apc_cmd_open() : apc_cmd_close();
		cmd.cmd_ack = ok ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		break;
	}


	case CMD_APC_VAL_RD: {
		double v;                                      // local storage
		if (apc_get_valve_state(&v)) {
			cmd.par0 = (float) v;                   // always an int here
			cmd.cmd_ack = CMR_SUCCESSFULL;
		} else {
			cmd.cmd_ack = CMR_COMMANDDENIED;
		}
		break;
	}

	case CMD_APC_POS: {
		cmd.cmd_ack = apc_set_pos(cmd.par0) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		break;
	}

	case CMD_APC_POS_RD: {
		double v;                                      // local storage
		if (apc_get_pos(&v)) {
			cmd.par0 = (float) v;
			cmd.cmd_ack = CMR_SUCCESSFULL;
		} else {
			cmd.cmd_ack = CMR_COMMANDDENIED;
		}
		break;
	}

	case CMD_APC_POS_SPD: {
		cmd.cmd_ack = apc_set_pos_ctl_spd(cmd.par0) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		break;
	}

	case CMD_APC_POS_SPD_RD: {
		double v;                                      // local storage
		if (apc_get_pos_ctl_spd(&v)) {
			cmd.par0 = (float) v;                           // always an int here
			cmd.cmd_ack = CMR_SUCCESSFULL;
		} else {
			cmd.cmd_ack = CMR_COMMANDDENIED;
		}
		break;
	}

	case CMD_APC_POS_RAM: { // here we might need to call both ramps
		cmd.cmd_ack = apc_set_pos_ramp_en(cmd.par0) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		break;
	}

	case CMD_APC_POS_RAM_RD: {
		double v;                                      // local storage
		if (apc_get_pos_ramp_en(&v)) {
			cmd.par0 = (float) v;                           // always an int here
			cmd.cmd_ack = CMR_SUCCESSFULL;
		} else {
			cmd.cmd_ack = CMR_COMMANDDENIED;
		}
		break;
	}
	case CMD_APC_POS_TI: {
		cmd.cmd_ack = apc_set_pos_ramp_time(cmd.par0) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		break;
	}

	case CMD_APC_POS_TI_RD: {
		double v;                                      // local storage
		if (apc_get_pos_ramp_time(&v)) {
			cmd.par0 = (float) v;                           // always an int here
			cmd.cmd_ack = CMR_SUCCESSFULL;
		} else {
			cmd.cmd_ack = CMR_COMMANDDENIED;
		}
		break;
	}
	case CMD_APC_POS_SLP: {
		cmd.cmd_ack = apc_set_pos_ramp_slope(cmd.par0) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		break;
	}

	case CMD_APC_POS_SLP_RD: {
		double v;                                      // local storage
		if (apc_get_pos_ramp_slope(&v)) {
			cmd.par0 = (float) v;                           // always an int here
			cmd.cmd_ack = CMR_SUCCESSFULL;
		} else {
			cmd.cmd_ack = CMR_COMMANDDENIED;
		}
		break;
	}
	case CMD_APC_POS_MD: {
		cmd.cmd_ack = apc_set_pos_ramp_mode(cmd.par0) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		break;
	}

	case CMD_APC_POS_MD_RD: {
		double v;                                      // local storage
		if (apc_get_pos_ramp_mode(&v)) {
			cmd.par0 = (float) v;                           // always an int here
			cmd.cmd_ack = CMR_SUCCESSFULL;
		} else {
			cmd.cmd_ack = CMR_COMMANDDENIED;
		}
		break;
	}

	case CMD_APC_PRE: {
		cmd.cmd_ack = apc_set_pre(cmd.par0) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		break;
	}

	case CMD_APC_PRE_RD: {
		double v;                                      // local storage
		if (apc_get_pre(&v)) {
			cmd.par0 = (float) v;                           // always an int here
			cmd.cmd_ack = CMR_SUCCESSFULL;
		} else {
			cmd.cmd_ack = CMR_COMMANDDENIED;
		}
		break;
	}
	case CMD_APC_PRE_SPD: {
		cmd.cmd_ack = apc_set_pre_speed(cmd.par0) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		break;
	}

	case CMD_APC_PRE_SPD_RD: {
		double v;                                      // local storage
		if (apc_get_pre_speed(&v)) {
			cmd.par0 = (float) v;                           // always an int here
			cmd.cmd_ack = CMR_SUCCESSFULL;
		} else {
			cmd.cmd_ack = CMR_COMMANDDENIED;
		}
		break;
	}
	case CMD_APC_PRE_UNT: {
		cmd.cmd_ack = apc_set_pre_unit(cmd.par0) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		break;
	}

	case CMD_APC_PRE_UNT_RD: {
		double v;                                      // local storage
		if (apc_get_pre_unit(&v)) {
			cmd.par0 = (float) v;                           // always an int here
			cmd.cmd_ack = CMR_SUCCESSFULL;
		} else {
			cmd.cmd_ack = CMR_COMMANDDENIED;
		}
		break;
	}

	case CMD_POS_STA_RD: { // TODO: this is same with read valve status, ask this!!
		double v;                                      // local storage
		if (apc_get_valve_state(&v)) {
			cmd.par0 = (float) v;                   // always an int here
			cmd.cmd_ack = CMR_SUCCESSFULL;
		} else {
			cmd.cmd_ack = CMR_COMMANDDENIED;
		}
		break;

	}

// -----------

	case CMD_ISO_V1: {
		uint16_t want = (uint16_t)cmd.par0;  // 0=close, 1=open
	    uint8_t  ok = 0;
		if (want) {
			uint16_t v3;	double apc_pos;
			z_valve_get(3, &v3);             // V3 state
			apc_get_pos(&apc_pos);           // APC position in %
			// Block opening if any safety is active:
			if (!(v3 || door_switch_get() || (apc_pos > 20.0))) {
				iso_valve_set(1);
				ok = 1;
			}
		} else {
			iso_valve_set(0);                 // closing is always allowed
			ok = 1;
		}
		cmd.cmd_ack = ok ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		break;
	}

	case CMD_ISO_V1_RD: {
		cmd.par0 = (float) iso_valve_get();
		cmd.cmd_ack = CMR_SUCCESSFULL;
		break;
	}

	case CMD_REL: {
		relais_set((uint8_t) cmd.par0);
		cmd.cmd_ack = CMR_SUCCESSFULL;
		break;
	}

	case CMD_REL_RD: {
		cmd.par0 = (float) relais_get();
		cmd.cmd_ack = CMR_SUCCESSFULL;
		break;
	}

	case CMD_BUZ: {
		buzzer_set((uint8_t) cmd.par0);
		cmd.cmd_ack = CMR_SUCCESSFULL;
		break;
	}

	case CMD_BUZ_RD: {
		cmd.par0 = (float) buzzer_get();
		cmd.cmd_ack = CMR_SUCCESSFULL;
		break;
	}

	case CMD_SYS_LED: {
		ledbereit_set((uint8_t) cmd.par0);
		cmd.cmd_ack = CMR_SUCCESSFULL;
		break;
	}

	case CMD_SYS_LED_RD: {
		cmd.par0 = (float) ledbereit_get();
		cmd.cmd_ack = CMR_SUCCESSFULL;
		break;
	}

	case CMD_PUM_LED: {
		ledpumpe_set((uint8_t) cmd.par0);
		cmd.cmd_ack = CMR_SUCCESSFULL;
		break;
	}

	case CMD_PUM_LED_RD: {
		cmd.par0 = (float) ledpumpe_get();
		cmd.cmd_ack = CMR_SUCCESSFULL;
		break;
	}

	case CMD_ATM_RD: {
		cmd.par0 = (float) atm_sensor_get();
		cmd.cmd_ack = CMR_SUCCESSFULL;
		break;
	}

	case CMD_DOOR_SWM_RD: {
		cmd.par0 = (float) door_switch_get();
		cmd.cmd_ack = CMR_SUCCESSFULL;
		break;
	}

	case CMD_AIR_RD: {
		cmd.par0 = (float) air_sensor_get();
		cmd.cmd_ack = CMR_SUCCESSFULL;
		break;
	}

	case CMD_STP_RD: {
		cmd.par0 = (float) stop_button_get();
		cmd.cmd_ack = CMR_SUCCESSFULL;
		break;
	}

// -----------

	case CMD_RF: {
		RFG_WRITE("RF");
		break;
	}

	case CMD_RF_RD: {
		RFG_READ("RF?");
		break;
	}

	case CMD_SPC_CTL: {
		RFG_WRITE("SPC:CTL");
		break;
	}

	case CMD_SPC_CTL_RD: {
		RFG_READ("SPC:CTL?");
		break;
	}

	case CMD_SPC_T_RD: {
		RFG_READ("SPC:T?");
		break;
	}

	case CMD_SPC_UDC_RD: {
		RFG_READ("SPC:UDC?");
		break;
	}

	// Power commands
	case CMD_PWR_PF_RD: {
		RFG_READ("PWR:PF?");
		break;
	}

	case CMD_PWR_PF: {
		RFG_WRITE("PWR:PF");
		break;
	}

	case CMD_PWR_PFS_RD: {
		RFG_READ("PWR:PFS?");
		break;
	}

	case CMD_PWR_PMD: {
		RFG_WRITE("PWR:PMD");
		break;
	}

	case CMD_PWR_PMD_RD: {
		RFG_READ("PWR:PMD?");
		break;
	}

	case CMD_PWR_PMDS_RD: {
		RFG_READ("PWR:PMDS?");
		break;
	}

	case CMD_PWR_PREA: {
		RFG_WRITE("PWR:PREA");
		break;
	}

	case CMD_PWR_PREA_RD: {
		RFG_READ("PWR:PREA?");
		break;
	}

	case CMD_PWR_PREAS_RD: {
		RFG_READ("PWR:PREAS?");
		break;
	}

	case CMD_PWR_DCB: {
		RFG_WRITE("PWR:DCB");
		break;
	}

	case CMD_PWR_DCB_RD: {
		RFG_READ("PWR:DCB?");
		break;
	}

	case CMD_PWR_DCBS_RD: {
		RFG_READ("PWR:DCBS?");
		break;
	}

	// Pulse commands
	case CMD_PLS_P_RD: {
		RFG_READ("PLS:P?");
		break;
	}

	case CMD_PLS_P: {
		RFG_WRITE("PLS:P");
		break;
	}

	case CMD_PLS_LEN_RD: {
		RFG_READ("PLS:LEN?");
		break;
	}

	case CMD_PLS_LEN: {
		RFG_WRITE("PLS:LEN");
		break;
	}

	case CMD_PLS_PER_RD: {
		RFG_READ("PLS:PER?");
		break;
	}

	case CMD_PLS_PER: {
		RFG_WRITE("PLS:PER");
		break;
	}

	// Ignition commands
	case CMD_IGN_CL_RD: {
		RFG_READ("IGN:CL?");
		break;
	}

	case CMD_IGN_CL: {
		RFG_WRITE("IGN:CL");
		break;
	}

	case CMD_IGN_CT_RD: {
		RFG_READ("IGN:CT?");
		break;
	}

	case CMD_IGN_CT: {
		RFG_WRITE("IGN:CT");
		break;
	}

	case CMD_IGN_I_RD: {
		RFG_READ("IGN:I?");
		break;
	}

	case CMD_IGN_I: {
		RFG_WRITE("IGN:I");
		break;
	}

	case CMD_IGN_PFI_RD: {
		RFG_READ("IGN:PFI?");
		break;
	}

	case CMD_IGN_PFI: {
		RFG_WRITE("IGN:PFI");
		break;
	}

	case CMD_IGN_TI_RD: {
		RFG_READ("IGN:TI?");
		break;
	}

	case CMD_IGN_TI: {
		RFG_WRITE("IGN:TI");
		break;
	}

	case CMD_IGN_TS_RD: {
		RFG_READ("IGN:TS?");
		break;
	}

	case CMD_IGN_TS: {
		RFG_WRITE("IGN:TS");
		break;
	}

	// Timer commands
	case CMD_TI_RST_RD: {
		RFG_READ("TI:RST?");
		break;
	}

	case CMD_TI_RST: {
		RFG_WRITE("TI:RST");
		break;
	}

	// Matching commands
	case CMD_MAT_CT_RD: {
		RFG_READ("MAT:CT?");
		break;
	}

	case CMD_MAT_CT: {
		RFG_WRITE("MAT:CT");
		break;
	}

	case CMD_MAT_CTS_RD: {
		RFG_READ("MAT:CTS?");
		break;
	}

	case CMD_MAT_CL: {
		RFG_WRITE("MAT:CL");
		break;
	}

	case CMD_MAT_CL_RD: {
		RFG_READ("MAT:CL?");
		break;
	}

	case CMD_MAT_CLS_RD: {
		RFG_READ("MAT:CLS?");
		break;
	}

	case CMD_MAT_MMD_RD: {
		RFG_READ("MAT:MMD?");
		break;
	}

	case CMD_MAT_MMD: {
		RFG_WRITE("MAT:MMD");
		break;
	}

	// EEPROM commands
	case CMD_EE_PRF_RD: {
		RFG_READ("EE:PRF?");
		break;
	}

	case CMD_EE_LDPRF: {
		RFG_WRITE("EE:LDPRF");
		break;
	}

	case CMD_EE_DPRF_RD: {
		RFG_READ("EE:DPRF?");
		break;
	}

	case CMD_EE_DPRF: {
		RFG_WRITE("EE:DPRF");
		break;
	}

	case CMD_MAT_T_RD: {
		RFG_READ("MAT:T?");
		break;
	}

	case CMD_EE_STPRF: {
		RFG_WRITE("EE:STPRF");
		break;
	}

// -----------

		// GET SET ERR GASBOX
	case CMD_GET_ERR_GB: {
		uint16_t e;
		cmd.cmd_ack = z_gb_err_get(&e);
		if (cmd.cmd_ack == CMR_SUCCESSFULL)
			cmd.par0 = e;
		break;
	}
	case CMD_RESET_ERR_GB: {
		cmd.cmd_ack = z_gb_err_clr();
		break;
	}

	default:
		cmd.cmd_ack = CMR_UNKNOWNCOMMAND;
		break;
	};
	resultQueue_push(cmd);

}

