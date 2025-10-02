//#include <stdio.h>
//#include <string.h>

#include "SG_global.h"
#include "protocoll.h"
#include "cmdlist.h"
#include "stacks.h"
#include "prioritylist.h"
#include "priority_pushpop.h"
#include "resultqueue.h"
#include "zentrale.h"
#include "zentrale_cmd_sero.h"
#include "uart4.h"
#include "func.h"
#include "hardware.h"
#include "gasbox.h"


//--- PRIVATE DEFINES -------------------------------------------------------------------------------------------------------
#define Z_MAX_HIGHPRIO_NUM			5                     //max.Durchdatz der Befehlsverarbeitung für die Befehlen mit der höchsten Priorität
#define Z_MAXCMD                    10                     //max. Durchsatz einer Befehlsverarbeitung

//--- PRIVATE VARIABLES -----------------------------------------------------------------------------------------------------

//--- PRIVATE FUNKTIONSDEKLARATION ----------------------------------------------------------------------------------------
void z_cmd_sero(stack_item cmd);                   //den einzelnen Befehl zu verarbeiten

//--- FUNKTIONSDEKLARATIONS -------------------------------------------------------------------------------------------------
//die Mechanimus zur Verarbeitung der Befehle mit den unterschiedlichen Prioritäten
//--- FUNKTIONSDEFINITIONS --------------------------------------------------------------------------------------------------
void z_cmd_scheduler(void)
{
	uint8_t priolevel0_null_flg = 0;
	uint8_t priolevel1_null_flg = 0;
	uint8_t priolevel2_null_flg = 0;

	uint8_t cmdcount = 0;                       //Zähler für die zu verarbeitenden Befehle
	uint8_t cmd_flg = 0;                       //Falls cmd_flg = 1 ist, d.h. keinen Befehl zu verarbeiten; cmd_flg = 0, d.h. noch Befehl zu verarbeiten
	stack_item cmd;

	uint8_t resultflg = get_anzFrei_resultQueue();

	if (resultflg > 1 )            //prüft, ob es noch freien Platz in resultqueue.
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
					priolevel0_null_flg = 1;      //es gibt keinen Befehl in der Prioritätliste mit Level 0
				};
			};

			if (z_priolevel_header[PRIO_LEVEL1] != NONEXT)
			{
				if (cmdcount < Z_MAXCMD)
				{
					zstack_pop(&cmd, PRIO_LEVEL1);
					z_cmd_sero(cmd);
					cmdcount++;

				};
			}
			else
			{
				priolevel1_null_flg = 1;   //es gibt keinen Befehl in der Prioritätliste mit Level 1
			};

			if (z_priolevel_header[PRIO_LEVEL2] != NONEXT)
			{
				if (cmdcount < Z_MAXCMD)
				{
					zstack_pop(&cmd, PRIO_LEVEL2);
					z_cmd_sero(cmd);
					cmdcount++;
				};
			}
			else
			{
				priolevel2_null_flg = 1;   //es gibt keinen Befehl in der Prioritätliste mit Level 2
			};

			//prüft, ob die drei Prioritätslisten alle leer sind.
			cmd_flg = priolevel0_null_flg & priolevel1_null_flg;
			cmd_flg &= priolevel2_null_flg;

		}while ( (cmdcount < Z_MAXCMD) && (cmd_flg == 0));
	};

}

void z_cmd_sero(stack_item cmd) {

	switch (cmd.cmd_index) {

	// MFC1..MFC4 SET
	case CMD_SET_GAS_PDE: {
		uint16_t p = clamp16(cmd.parameter);
		cmd.cmd_ack = z_mfc_set(0, p) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		break;
	}
	case CMD_SET_GAS_AIR: {
		uint16_t p = clamp16(cmd.parameter);
		cmd.cmd_ack = z_mfc_set(1, p) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		break;
	}
	case CMD_SET_GAS_O2: {
		uint16_t p = clamp16(cmd.parameter);
		cmd.cmd_ack = z_mfc_set(2, p) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		break;
	}
	case CMD_SET_GAS_4: {
		uint16_t p = clamp16(cmd.parameter);
		cmd.cmd_ack = z_mfc_set(3, p) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		break;
	}

	// MFC1..MFC4 GET
	case CMD_GET_GAS_PDE: {
		uint16_t v;
		cmd.cmd_ack = z_mfc_get(0, &v) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		if (cmd.cmd_ack == CMR_SUCCESSFULL)
			cmd.parameter = v;
		break;
	}
	case CMD_GET_GAS_AIR: {
		uint16_t v;
		cmd.cmd_ack = z_mfc_get(1, &v) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		if (cmd.cmd_ack == CMR_SUCCESSFULL)
			cmd.parameter = v;
		break;
	}
	case CMD_GET_GAS_O2: {
		uint16_t v;
		cmd.cmd_ack = z_mfc_get(2, &v) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		if (cmd.cmd_ack == CMR_SUCCESSFULL)
			cmd.parameter = v;
		break;
	}
	case CMD_GET_GAS_4: {
		uint16_t v;
		cmd.cmd_ack = z_mfc_get(3, &v) ? CMR_SUCCESSFULL : CMR_COMMANDDENIED;
		if (cmd.cmd_ack == CMR_SUCCESSFULL)
			cmd.parameter = v;
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
		uint16_t p = cmd.parameter;
		uint8_t ret;
		if (p) {ret = z_valve_open(3); } else { ret = z_valve_close(3); }
	    cmd.cmd_ack = ret ? CMR_SUCCESSFULL : CMR_UNITBUSY;
	    break;
	}

	case CMD_V3_GET: {
	    uint16_t st;
	    if (z_valve_get(3, &st)) { cmd.parameter = st; cmd.cmd_ack = CMR_SUCCESSFULL; }
	    else                    { cmd.cmd_ack = CMR_COMMANDDENIED; }
	    break;
	}

	case CMD_V4_SET: {
		uint16_t p = cmd.parameter;
		uint8_t ret;
		if (p) {ret = z_valve_open(4); } else { ret = z_valve_close(4); }
	    cmd.cmd_ack = ret ? CMR_SUCCESSFULL : CMR_UNITBUSY;
	    break;
	}

	case CMD_V4_GET: {
	    uint16_t st;
	    if (z_valve_get(4, &st)) { cmd.parameter = st; cmd.cmd_ack = CMR_SUCCESSFULL; }
	    else                    { cmd.cmd_ack = CMR_COMMANDDENIED; }
	    break;
	}

	case CMD_PUMP_SET: {
		uint16_t p = cmd.parameter;
	    if (p) { setStartPump(); } else { setStopPump(); }
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
		cmd.parameter = v;
		cmd.cmd_ack = CMR_SUCCESSFULL;
		break;
	}

	case CMD_PUMP_GET_ALA: {             // "PUM:WAR?"
		uint8_t v = readPumpAlarm();         // collective warning bit
		cmd.parameter = v;
		cmd.cmd_ack = CMR_SUCCESSFULL;
		break;
	}

	case CMD_PUMP_GET_RMT: {             // "PUM:WAR?"
		uint8_t v = readPumpRemote();         // collective warning bit
		cmd.parameter = v;
		cmd.cmd_ack = CMR_SUCCESSFULL;
		break;
	}

	// GET SET ERR GASBOX
	case CMD_GET_ERR_GB: {
	    uint16_t e;
	    cmd.cmd_ack = z_gb_err_get(&e);
	    if (cmd.cmd_ack == CMR_SUCCESSFULL) cmd.parameter = e;
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

