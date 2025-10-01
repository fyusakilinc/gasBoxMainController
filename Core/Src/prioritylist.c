//#include <stdio.h>
//#include <string.h>

#ifndef _STDINT_H
#include <stdint.h>
#endif

#include "SG_global.h"
#include "prioritylist.h"


//-------------------------PRIVATE DEFINES-----------------------------------------------------------------------------------

//-------------PRIVATE VARIABLES---------------------------------------------------------------------------------------------
//Definition der Prioritätsliste und der Priorität-Level-Liste für die PV-Zentrale und das MatchingCube
priority_item z_priolist[Z_STACK_SIZE];
priority_item mc_priolist[MC_STACK_SIZE];
uint8_t z_priolevel_header[PRIO_LEVELS];
uint8_t mc_priolevel_header[PRIO_LEVELS];        //den ersten Index der 3 Prioritätsliste z/mc_priolist.
uint8_t zpriolist_firstunused_index;           //der Zeiger auf den ersten Index der freigegebenen Plätzen in der Prioritätsliste für die PV-Zentrale
uint8_t mcpriolist_firstunused_index;		//der Zeiger auf den ersten freien Index der freigegebenen Plätzen in der Prioritätsliste für das MatchingCube

//--------------FUNKTIONSDEKLARATIONS----------------------------------------------------------------------------------------
void priolist_init(void);

//--------------FUNKTIONSDEFINITIONS-----------------------------------------------------------------------------------------
void priolist_init(void)
{
	uint8_t i = 0;

	zpriolist_firstunused_index = 0;         //der gültige Wert liegt zwischen 0 und (Z_STACK_SIZE-1); wenn = NONEXT, d.h. keinen freien Platz in der Prioliste.
	mcpriolist_firstunused_index = 0;		//der Wert liegt zwischen 0 und (MC_STACK_SIZE-1)

	for (i = 0;  i < PRIO_LEVELS; i++)
	{
		z_priolevel_header[i] = NONEXT;    //d.h. es gibt noch keinen Befehl in der i-te. Prioritätsliste.

		mc_priolevel_header[i] = NONEXT;

	}

	for ( i= 0; i < Z_STACK_SIZE; i++)
	{
		if (i == (Z_STACK_SIZE-1) )
		{
			z_priolist[i].next = NONEXT;
		}
		else
		{
			z_priolist[i].next = i+1;
		};
	};

	for ( i= 0; i < MC_STACK_SIZE; i++)
	{
		if (i == (MC_STACK_SIZE-1) )
		{
			mc_priolist[i].next = NONEXT;
		}
		else
		{
			mc_priolist[i].next = i+1;
		};
	};
}



