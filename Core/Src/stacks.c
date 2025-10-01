//#include <stdio.h>
//#include <string.h>

#include "protocoll.h"
#include "stacks.h"
#include "cmdlist.h"
#include "prioritylist.h"
#include "priority_pushpop.h"
#include "zentrale.h"

//--- PRIVATE DEFINES -------------------------------------------------------------------------------------------------------
#define STACKPUSH_ERFOLG    1
#define STACKINTERN_ERROR   0
#define MCSTACK_ZENTRALE_RESERVATION  2         //Für die Befehle der Zentrale an den MC werden 2 Plätze reserviert.

//--- PRIVATE VARIABLES ---------------------------------------------------------------------------------------------
static stack_item zentrale_stack[Z_STACK_SIZE];           //die Warteschlange fuer die Anfragepakete an die PV-Zentrale
static stack_item mc_stack[MC_STACK_SIZE];				 //die Warteschlange fuer die Anfragepakete an das MatchingCube
static uint8_t zstackindex_list[Z_STACK_SIZE];                  //das Element speichert den freien Stackindex
static uint8_t mcstackindex_list[MC_STACK_SIZE];
static uint8_t zstackindex_list_act;                           // der aktuelle Zeiger auf den ZStack_indexlist bzw. die Anzahl der freien Stackplätze
static uint8_t mcstackindex_list_act;						 // der aktuelle Zeiger auf den MCStack_indexlist

volatile static uint8_t mcstack_rest_length;

//--- FUNKTIONSDEKLARATIONS -------------------------------------------------------------------------------------------------

uint8_t stacks_insert_cmd(stack_item stack[], uint8_t stack_length, uint8_t stackindex_list[], stack_item stack_data, uint8_t *stackindexlist_act, uint8_t *stackindex);
uint8_t z_mc_stack_insert(stack_item stack_data, uint8_t mc_flg);
uint8_t mcstack_pop(stack_item *sitem, uint8_t prio_data);
uint8_t zstack_pop(stack_item *sitem, uint8_t prio_data);
uint8_t zstackindex_list_act_get(void);
uint8_t mcstackindex_list_act_get(void);

//--- FUNKTIONSDEFINITIONS --------------------------------------------------------------------------------------------------
void stacks_init(void)
{
	uint8_t i = 0;

	zstackindex_list_act = Z_STACK_SIZE -1;        //alle Stackindexe sind frei
	mcstackindex_list_act = MC_STACK_SIZE -1;
	mcstack_rest_length = MC_STACK_SIZE - MCSTACK_ZENTRALE_RESERVATION;


	for (i = 0; i < Z_STACK_SIZE; i++)
	{
		zentrale_stack[i].next = NONEXT;
		zstackindex_list[i] = zstackindex_list_act - i;   //alle freien Stackindexe sind abwärts in die Stackindex-Liste(aufwärts) gelegt.
	};

	for (i = 0; i < MC_STACK_SIZE; i++)
	{
		mc_stack[i].next = NONEXT;
		mcstackindex_list[i] = mcstackindex_list_act - i;
	};
}

uint8_t stacks_insert_cmd(stack_item stack[], uint8_t stack_length, uint8_t stackindex_list[], stack_item stack_data, uint8_t *stackindexlist_act, uint8_t *stackindex)
{
	uint8_t index_tmp;
	uint8_t flag;

	if ( (*stackindexlist_act >= 0)  && (*stackindexlist_act < stack_length) )          //wenn es einen freien Platz gibt,
	{
		index_tmp = *stackindexlist_act;
		*stackindex = stackindex_list[index_tmp];                    //fügt einen Element in den Stack ein
		stack[*stackindex] = stack_data;

		if (index_tmp == 0 )                                     //d.h. es keine leer Platz mehr im Stack nach dem Einfügen gibt.
		{
			*stackindexlist_act = NONEXT;           //NONEXT bezeichnet: der Stack ist voll, weil die negative Zahl nicht erlaubt ist.
		}
		else
		{
			index_tmp -=1;
			*stackindexlist_act = index_tmp;
		};
		flag = STACK_INTSERT_OK;
	}
	else //Falls der Stack voll ist,
	{
		flag = CMR_UNITBUSY;
	};

	return flag;
}

uint8_t z_mc_stack_insert(stack_item stack_data, uint8_t mc_flg)
{
	uint8_t flag = 0;
	uint8_t push_result = 0;
	uint8_t prio_pushflg = 0;
	uint8_t stack_index = NONEXT;
	uint8_t mc_push_enable = 0;  //=0: es ist nicht erlaubt, die Befehlen in den MC- Stack einlegen; > 1: es ist erlaubt; =1: die Befehlen aus der Zentrale;
	//=2: aus der anderen Quellen.

	stack_item stack_tmp;

	if (mc_flg)   // d.h. der Befehl ist für den MC
	{
		if (stack_data.cmd_sender == Q_ZENTRALE)
		{
			mc_push_enable = 1;
		}
		else
		{
			if (mcstack_rest_length  > 0)
			{
				mc_push_enable = 2;
			}
			else
			{
				mc_push_enable = 0;
				flag = CMR_UNITBUSY;
			};
		};

		if (mc_push_enable > 0)
		{
			push_result = stacks_insert_cmd(mc_stack, MC_STACK_SIZE, mcstackindex_list, stack_data, &mcstackindex_list_act, &stack_index);
			if (push_result == STACK_INTSERT_OK)
			{
				prio_pushflg = priolist_push(mc_priolist, MC_STACK_SIZE, mc_priolevel_header,  &mcpriolist_firstunused_index, stack_index, stack_data.prio);
				if (prio_pushflg )
				{
					flag = STACK_CMDINSTACK;
					if (mc_push_enable == 2)
					{
						if (mcstack_rest_length > 0)
						{
							mcstack_rest_length -=1;                   //die Plätze für die Befehle aus den anderen Quelle ausser der Zentrale wird einen weniger sein.
						}

					};
				}
				else
				{
					mcstack_pop(&stack_tmp, stack_data.prio);
					flag = STACK_PRIOLIST_ERROR;
				}
			}
			else
			{
				flag = CMR_UNITBUSY;
			};
		};
	}
	else //d.h. der Befehl wird an die Zentrale gesendet
	{

		push_result = stacks_insert_cmd(zentrale_stack, Z_STACK_SIZE, zstackindex_list, stack_data, &zstackindex_list_act, &stack_index);

		if (push_result == STACK_INTSERT_OK)
		{
			prio_pushflg = priolist_push(z_priolist, Z_STACK_SIZE, z_priolevel_header, &zpriolist_firstunused_index, stack_index, stack_data.prio);
			if (prio_pushflg )
			{
				flag = STACK_CMDINSTACK;
			}
			else
			{
				zstack_pop(&stack_tmp, stack_data.prio);
				flag = STACK_PRIOLIST_ERROR;
			} ;
		}
		else
		{

			flag = CMR_UNITBUSY;
		};
	};
	return flag;
}



uint8_t stack_insert_sero(stack_item stack_data)
{
	uint8_t flag = 0;
	uint8_t mc_flg = 0;                            //bezeichnet, dass der Befehl für die Zentrale ist, wenn es 0 ist; für den MC , wenn es 1 ist.

	//hier gibt es nur die Befehle für die Zentrale,

	mc_flg = 0;
	flag = z_mc_stack_insert(stack_data, mc_flg);
	return flag;
}


uint8_t mcstack_pop(stack_item *sitem, uint8_t priolevel)
{
	uint8_t flag = 0;
	uint8_t sindex;

	sindex = priolist_pop(mc_priolist, mc_priolevel_header, &mcpriolist_firstunused_index, priolevel);
	if (sindex == NONEXT)  // die zu der Priorität entsprechende MCStacklist ist leer
	{
		flag = 0;

	}
	else
	{

			*sitem = mc_stack[sindex];
			if (mcstackindex_list_act == NONEXT)
			{
				mcstackindex_list_act =0;

			}
			else
			{
				mcstackindex_list_act +=1;

			};

			mcstackindex_list[mcstackindex_list_act] = sindex;

			if (mc_stack[sindex].cmd_sender != Q_ZENTRALE)
			{
				mcstack_rest_length += 1;
			};

			flag = 1;

	};
	return flag;
}

uint8_t zstack_pop(stack_item *sitem, uint8_t priolevel)
{
	uint8_t flag = 0;
	uint8_t sindex;
	//uint8_t indexlist_tmp;

	sindex =  priolist_pop(z_priolist, z_priolevel_header, &zpriolist_firstunused_index, priolevel);

	if (sindex == NONEXT)  // die zu der Priorität entsprechende MCStacklist ist leer
	{
		flag = 0;

	}
	else
	{
		*sitem = zentrale_stack[sindex];

		if (zstackindex_list_act == NONEXT)
		{
			zstackindex_list_act = 0;
		}
		else
		{
			zstackindex_list_act +=1;
		};
		zstackindex_list[zstackindex_list_act] = sindex;

		flag = 1;

	};
	return flag;
}

uint8_t zstackindex_list_act_get(void)
{
	return zstackindex_list_act;
}

uint8_t mcstackindex_list_act_get(void)
{
	return mcstackindex_list_act;
}

