/*-------------------------------------------------------------------------------------------------------------------------
Datei:				prioritylist.c
Version:			A00
Plattform:			Pv
Baugruppe:
Autor:				jl
Letzte Änderung:
CodeFreeze:			nein

[Dokumentation]
prioritylist.c definiert die Strukturen und die Elemente für die Verwaltung der Prioritäten der Anfragen. Es gibt
zwei Prioritätsliste, eine z_priolist ist für die PV-Zentrale, der andere ist mc_priolist für das MatchingCube. Für
diese zwei Prioritätsliste gibt es noch zwei entsprechende Liste, die den Index des freigegebenen Platz verwalten.
Das Element der Prioritätsliste enthältet zwei Informationen:
- den Stackindex des ersten kommenden Anfragepaketes und
- den Stackindex des nächsten kommenden Anfragepaketes mit der gleichen Priorität
Damit werden die in Stack eingefügten Anfragepakete mit der gleichen Priorität verkettet.
Es werden drei Prioritätsstufe von 1 bis 3 definiert(drei Prioritätsliste).
-------------------------------------------------------------------------------------------------------------------------*/
#ifndef PRIORITYLIST_H
#define PRIORITYLIST_H

//--- PUBLIC DEFINES --------------------------------------------------------------------------------------------------------
#define Z_STACK_SIZE      30  //20                               //die Laenge der Warteschlange für die Befehle an die PV-Zentrale
#define MC_STACK_SIZE     20  //10							 //die Laenge der Warteschlange für die Befehle an das MatchingCube

//--- PUBLIC VARIABLES ------------------------------------------------------------------------------------------------------
//Deklaration der Variablen der zwei Prioritätsliste und die zwei Priorität-Level-Liste als extern(bzw. globalen Variablen)
//damit die Variablen für die anderen C-Module auch sichbar sind.
extern priority_item z_priolist[Z_STACK_SIZE];
extern priority_item mc_priolist[MC_STACK_SIZE];
extern uint8_t z_priolevel_header[PRIO_LEVELS];
extern uint8_t mc_priolevel_header[PRIO_LEVELS];
extern uint8_t zpriolist_firstunused_index;
extern uint8_t mcpriolist_firstunused_index;

//--- PUBLIC FUNKTIONSDEKLARATION -----------------------------------------------------------------------------------------
//Initialisierung der Prioritätsliste
void priolist_init(void);

#endif
