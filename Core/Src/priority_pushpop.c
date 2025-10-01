//#include <stdio.h>
//#include <string.h>

#ifndef _STDINT_H
#include <stdint.h>
#endif

#include "SG_global.h"
#include "prioritylist.h"
#include "priority_pushpop.h"

//--- PRIVATE DEFINES -------------------------------------------------------------------------------------------------------

//--- PRIVATE VARIABLES -----------------------------------------------------------------------------------------------------

//--- PRIVATE FUNKTIONESDEKLARATIONS ----------------------------------------------------------------------------------------
//Fügt einen Stackindex mit der angegebenen Priorität in die entsprechendene Prioritätsliste ein.
void priolist_node_insert(priority_item prio_list[], uint8_t priolist_length,  uint8_t priolevel_header[], uint8_t *unused_list, uint8_t stackindex, uint8_t priolevel);

//--- FUNKTIONSDEKLARATIONS -------------------------------------------------------------------------------------------------
uint8_t priolist_push (priority_item prio_list[], uint8_t priolist_length,  uint8_t priolevel_header[], uint8_t *firstunused, uint8_t stack_index, uint8_t priolevel_data);
uint8_t priolist_pop (priority_item prio_list[], uint8_t priolevel_header[], uint8_t *firstunused, uint8_t priolevel_data);


//--------------FUNKTIONSDEFINITIONS----------------------------
uint8_t priolist_push (priority_item prio_list[], uint8_t priolist_length,  uint8_t priolevel_header[], uint8_t *firstunused, uint8_t stack_index, uint8_t priolevel_data)
{

	uint8_t flag = 0;

	if (*firstunused == NONEXT )            //wenn es keinen freien Platz in der prio_list gibt,
	{
		flag = 0;
	}
	else
	{
		switch (priolevel_data)
		{
			case  PRIO_LEVEL0:
			priolist_node_insert(prio_list, priolist_length, priolevel_header, firstunused, stack_index, PRIO_LEVEL0);
			flag = 1;
			break;
			case  PRIO_LEVEL1:
			priolist_node_insert(prio_list, priolist_length, priolevel_header, firstunused, stack_index, PRIO_LEVEL1);
			flag = 1;
			break;
			case  PRIO_LEVEL2:
			priolist_node_insert(prio_list, priolist_length, priolevel_header, firstunused, stack_index, PRIO_LEVEL2);
			flag = 1;
			break;
		};
	};
	return flag;
}

void priolist_node_insert(priority_item prio_list[], uint8_t priolist_length,  uint8_t priolevel_header[], uint8_t *unused_list, uint8_t stackindex, uint8_t priolevel)
{
	uint8_t act_index ;
	uint8_t last;

	act_index = *unused_list;
	*unused_list = prio_list[act_index].next;
	if (priolevel_header[priolevel] == NONEXT)                //Wenn die Prioritätsliste mit dem Level0 noch leer ist,
	{

		prio_list[act_index].stackindex = stackindex;
		priolevel_header[priolevel] = act_index;
		prio_list[act_index].next = NONEXT;
	};
	if (priolevel_header[priolevel] < priolist_length)    //Wenn die Prioritätsliste schon existiert,
	{
		prio_list[act_index].stackindex = stackindex;
		last =  priolevel_header[priolevel];                  //das erste Element der Prioritätsliste
		while (prio_list[last].next != NONEXT)               //Suche den Ende der Prioritätsliste, um das neue Element hineinzufügen
		{
			last = prio_list[last].next;
		};
		prio_list[last].next = act_index;
		prio_list[act_index].next = NONEXT;
	};
}

uint8_t priolist_pop (priority_item prio_list[], uint8_t priolevel_header[], uint8_t *firstunused, uint8_t priolevel_data)
{
	uint8_t stack_index = NONEXT ;  //ungültige Wert, wenn die Prioritätsliste noch leer oder ein Fehler

	if (priolevel_header[priolevel_data] != NONEXT)
	{
		uint8_t act_index = priolevel_header[priolevel_data];
		stack_index = prio_list[act_index].stackindex;
		priolevel_header[priolevel_data] = prio_list[act_index].next;

		prio_list[act_index].next = *firstunused;
		*firstunused = act_index;
	};

	return stack_index;
}

