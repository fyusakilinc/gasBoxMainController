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
#include "gasbox.h"


//--- PRIVATE DEFINES -------------------------------------------------------------------------------------------------------
#define Z_MAX_HIGHPRIO_NUM			5                     //max.Durchdatz der Befehlsverarbeitung für die Befehlen mit der höchsten Priorität
#define Z_MAXCMD                    10                     //max. Durchsatz einer Befehlsverarbeitung

//--- PRIVATE VARIABLES -----------------------------------------------------------------------------------------------------

//--- PRIVATE FUNKTIONSDEKLARATION ----------------------------------------------------------------------------------------
void z_cmd_sero(stack_item cmd);                   //den einzelnen Befehl zu verarbeiten

//--- FUNKTIONSDEKLARATIONS -------------------------------------------------------------------------------------------------
//die Mechanimus zur Verarbeitung der Befehle mit den unterschiedlichen Prioritäten
void z_cmd_scheduler(void);

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

void z_cmd_sero(stack_item cmd)
{

	switch (cmd.cmd_index)
	{
		case CMD_SET_REM_CTL:
			cmd.cmd_ack = z_set_remote_mode(cmd.parameter);
			break;
		case CMD_GET_RF:
			cmd.parameter = z_get_rf();
			cmd.cmd_ack = CMR_SUCCESSFULL;
			break;
		case CMD_SET_RF:
			cmd.cmd_ack = z_set_rf(cmd.parameter);
			break;
		case CMD_GET_REM_CTL:
			cmd.parameter = z_get_remote_mode ();
			cmd.cmd_ack = CMR_SUCCESSFULL;
			break;
		case CMD_RESET_ERROR:
			cmd.cmd_ack = z_reset();
			break;
		case CMD_GET_STATUS:
			cmd.parameter = z_get_status();
			cmd.cmd_ack = CMR_SUCCESSFULL;
			break;
		case CMD_GET_ERR:
			cmd.parameter = z_get_error();
			cmd.cmd_ack = CMR_SUCCESSFULL;
			break;
		case CMD_GET_A_PF:
			cmd.parameter = z_get_pf_a();
			cmd.cmd_ack = CMR_SUCCESSFULL;
			break;
		case CMD_GET_A_PR:
			cmd.parameter = z_get_pr_a();
			cmd.cmd_ack = CMR_SUCCESSFULL;
			break;
		case CMD_SET_A_AMPLITUDE:
			cmd.cmd_ack = z_set_amp_a(cmd.parameter);
			break;
		case CMD_GET_A_AMPLITUDE:
			cmd.parameter = z_get_amp_a();
			cmd.cmd_ack = CMR_SUCCESSFULL;
			break;
		case CMD_SET_A_PHASE:
			cmd.cmd_ack = z_set_phase_a(cmd.parameter);
			break;
		case CMD_GET_A_PHASE:
			cmd.parameter = z_get_phase_a();
			cmd.cmd_ack = CMR_SUCCESSFULL;
			break;
		case CMD_SET_A_AMPPHASE:
			cmd.cmd_ack = z_set_a_ampphase(cmd.parameter);
			break;
		case CMD_GET_B_PF:
			cmd.parameter = z_get_pf_b();
			cmd.cmd_ack = CMR_SUCCESSFULL;
			break;
		case CMD_GET_B_PR:
			cmd.parameter = z_get_pr_b();
			cmd.cmd_ack = CMR_SUCCESSFULL;
			break;
		case CMD_SET_B_AMPLITUDE:
			cmd.cmd_ack = z_set_amp_b(cmd.parameter);
			break;
		case CMD_GET_B_AMPLITUDE:
			cmd.parameter = z_get_amp_b();
			cmd.cmd_ack = CMR_SUCCESSFULL;
			break;
		case CMD_SET_B_PHASE:
			cmd.cmd_ack = z_set_phase_b(cmd.parameter);
			break;
		case CMD_GET_B_PHASE:
			cmd.parameter = z_get_phase_b();
			cmd.cmd_ack = CMR_SUCCESSFULL;
			break;
		case CMD_SET_B_AMPPHASE:
			cmd.cmd_ack = z_set_b_ampphase(cmd.parameter);
			break;
		case CMD_GET_C_PF:
			cmd.parameter = z_get_pf_c();
			cmd.cmd_ack = CMR_SUCCESSFULL;
			break;
		case CMD_GET_C_PR:
			cmd.parameter = z_get_pr_c();
			cmd.cmd_ack = CMR_SUCCESSFULL;
			break;
		case CMD_SET_C_AMPLITUDE:
			cmd.cmd_ack = z_set_amp_c(cmd.parameter);
			break;
		case CMD_GET_C_AMPLITUDE:
			cmd.parameter = z_get_amp_c();
			cmd.cmd_ack = CMR_SUCCESSFULL;
			break;
		case CMD_SET_C_PHASE:
			cmd.cmd_ack = z_set_phase_c(cmd.parameter);
			break;
		case CMD_GET_C_PHASE:
			cmd.parameter = z_get_phase_c();
			cmd.cmd_ack = CMR_SUCCESSFULL;
			break;
		case CMD_SET_C_AMPPHASE:
			cmd.cmd_ack = z_set_c_ampphase(cmd.parameter);
			break;
		case CMD_GET_D_PF:
			cmd.parameter = z_get_pf_d();
			cmd.cmd_ack = CMR_SUCCESSFULL;
			break;
		case CMD_GET_D_PR:
			cmd.parameter = z_get_pr_d();
			cmd.cmd_ack = CMR_SUCCESSFULL;
			break;
		case CMD_SET_D_AMPLITUDE:
			cmd.cmd_ack = z_set_amp_d(cmd.parameter);
			break;
		case CMD_GET_D_AMPLITUDE:
			cmd.parameter = z_get_amp_d();
			cmd.cmd_ack = CMR_SUCCESSFULL;
			break;
		case CMD_SET_D_PHASE:
			cmd.cmd_ack = z_set_phase_d(cmd.parameter);
			break;
		case CMD_GET_D_PHASE:
			cmd.parameter = z_get_phase_d();
			cmd.cmd_ack = CMR_SUCCESSFULL;
			break;
		case CMD_SET_D_AMPPHASE:
			cmd.cmd_ack = z_set_d_ampphase(cmd.parameter);
			break;
		case CMD_GET_MON_UDC:
			cmd.parameter = 0;//z_get_u_act_lcd();
			cmd.cmd_ack = CMR_SUCCESSFULL;
			break;
		case CMD_SET_APPLY:
			cmd.cmd_ack = z_set_apply();
			break;
		case CMD_GET_MON_T1:
			cmd.parameter = 1;//z_get_t1_act_lcd();
			cmd.cmd_ack = CMR_SUCCESSFULL;
			break;
		case CMD_GET_MON_T2:
			cmd.parameter = 1;//z_get_t2_act_lcd();
			cmd.cmd_ack = CMR_SUCCESSFULL;
			break;

		case CMD_FLOW1_SET:
		    //if (!has_param) return CMR_PARAMETERINVALID;
		    //uint16_t code = (val < 0) ? 0 : (val > 65535 ? 65535 : (uint16_t)val);
		    //GbReply r;
		    //if (gasbox_xfer(GB_CMD_SET_DAC0, code, &r, 200) && r.status == 0x80) {
		        // tell PC it's OK (use your existing OK path)
		        //output_ascii_cmdack(CMR_SUCCESSFULL);         // or queue an OK
		        //return CMR_SUCCESSFULL;
		    //}
			cmd.cmd_ack = CMR_SUCCESSFULL; // or a better error code you use for comms/timeout
		    break;

		case CMD_FLOW1_GET:
		    //GbReply r;
		    //if (gasbox_xfer(GB_CMD_READ_ADC0, 0, &r, 200) && r.status == 0x80) {
		        // return the number to PC using your usual result mechanism
		        //output_ascii_result_number(r.value);          // or enqueue into resultqueue
		       // return CMR_SUCCESSFULL;
		   // }
			cmd.cmd_ack = CMR_SUCCESSFULL;
		    break;


        default:
            cmd.cmd_ack = CMR_UNKNOWNCOMMAND;
            break;
	};
	resultQueue_push(cmd);

}

