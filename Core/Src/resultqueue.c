#include "SG_global.h"
#include "uart4.h"
//#include "uart1.h"
#include "protocoll.h"
#include "stacks.h"
#include "func.h"
#include "prioritylist.h"
#include "zentrale.h"
#include "remote.h"

//-------------------------PRIVATE DEFINES-----------------------------------------------------------------------------------
#define RESULT_QUEUE_SIZE  60 //30                  //max. Länge der Warteschlange

//-------------PRIVATE VARIABLES---------------------------------------------------------------------------------------------
static stack_item resultQueue[RESULT_QUEUE_SIZE];
static uint8_t  resultQueue_in;                                 //der Zeiger fuer den letzten besetzten Platz der Warteschlange
static uint8_t  resultQueue_out;							   //der Zeiger fuer den letzten freien Platz der Warteschlange

//--------------FUNKTIONSDEKLARATIONS----------------------------------------------------------------------------------------
void resultQueue_init(void);
uint8_t resultQueue_push(stack_item sitem);
void resultQueue_pop(stack_item *sitem);
uint8_t get_anzFrei_resultQueue(void);
uint8_t get_anzBes_resultQueue(void);

void result_get_sero(void);

//fuer den Touchpanel
//void output_touch_result(stack_item *cmd);
//void spi_SendAnswer(uint8_t *message);

//--------------FUNKTIONSDEFINITIONS-----------------------------------------------------------------------------------------
void resultQueue_init(void)
{
	resultQueue_in = 0;
	resultQueue_out = 0;
}

uint8_t get_anzFrei_resultQueue(void)
{
	int8_t x;
	//ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		x = resultQueue_in - resultQueue_out;
		if (x < 0) x = x + RESULT_QUEUE_SIZE;
		x = RESULT_QUEUE_SIZE - x;
	}
	return x;
}

uint8_t get_anzBes_resultQueue(void)
{	int8_t x;
	//ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		x = resultQueue_in - resultQueue_out;
		if (x < 0) x = x + RESULT_QUEUE_SIZE;
	}
	return x;
}

uint8_t resultQueue_push (stack_item  sitem)
{
	uint8_t tmp = get_anzFrei_resultQueue();

	uint8_t flag =0;  // der Flag bedeutet, ob das Hinfuegen des Commando in Queue erfolgreich ist: 0= Nicht erfolgreich, 1= Erfolgreich

	if (tmp > 1)
	{
		resultQueue[resultQueue_in] = sitem;
		resultQueue_in ++;
		if (resultQueue_in >= RESULT_QUEUE_SIZE) resultQueue_in =0;
		flag = 1;
	}
	else
	{
		flag = 0;
	};
	return flag;
}

void resultQueue_pop(stack_item *sitem)
{
	uint8_t tmp =get_anzFrei_resultQueue();

	if (tmp < RESULT_QUEUE_SIZE )
	{
		*sitem = resultQueue[resultQueue_out];
		resultQueue_out ++;
		if (resultQueue_out >= RESULT_QUEUE_SIZE) resultQueue_out = 0;
	};
	return;
}

void result_get_sero(void)
{
	stack_item cmd_tmp;
	uint8_t verbose_tmp = remote_ascii_verbose();
	uint8_t crlf_tmp = remote_ascii_crlf();

	while (get_anzBes_resultQueue() > 0 )
	{
		resultQueue_pop(&cmd_tmp);
		//uart0_puts("result");
		//uart0_puti(cmd_tmp.cmd_index);
		//uart0_puti(cmd_tmp.cmd_ack);
		//uart0_puti(cmd_tmp.parameter);
		switch (cmd_tmp.cmd_sender)
		{
			//case Q_TOUCHPANEL:
				//output_touch_result(&cmd_tmp);
			//	break;
			case Q_RS232_ASCII:
				//uart0_puts("ASC");
				output_ascii_result(verbose_tmp, crlf_tmp, &cmd_tmp);
				break;
			case Q_RS232_BINARY:
				output_binary_result(&cmd_tmp);
				break;
//			case Q_ANALOG:
//				output_userport_result(&cmd_tmp);
//			case Q_ZENTRALE:
//				zentrale_match_get_sero(&cmd_tmp);
//				break;
			default:
				break;
		};
	};

	return;
}

