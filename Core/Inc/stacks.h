/*-------------------------------------------------------------------------------------------------------------------------
Datei:				stack.c
Version:			A00
Plattform:			Pv
Baugruppe:
Autor:				jl
Letzte Änderung:
CodeFreeze:			nein

[Dokumentation]
Es dient zum Hinfügen des Befehls aus dem Anschluss RS232 und der Zentrale in einen entsprechenden (Zentrale-/MatchingCube)Stack
-------------------------------------------------------------------------------------------------------------------------*/

//--- PUBLIC FUNKTIONSDEKLARATION -----------------------------------------------------------------------------------------
#ifndef STACKS_H
#define STACKS_H
#include "SG_global.h"
//Initialisierung des Zentrale-Stackes und des MatchingCube-Stackes
void stacks_init(void);

//Hinfügen des Befehls in den entsprechenden Stack, anhand des Attributes(READ/WRITE) des Befehls und der Operationsmode des PV.
uint8_t stack_insert_sero(stack_item stack_data); // uint8_t attribut);

//Gibt ein Anfragepaket vom MatchingCube-Stack aus
uint8_t mcstack_pop(stack_item *sitem, uint8_t prio_data);

//Gibt ein Anfragepaket vom Zentrale-Stack aus
uint8_t zstack_pop(stack_item *sitem, uint8_t prio_data);

//Gibt den aktuellen Anzahl der freien Plätze vom Zentrale-Stack aus
uint8_t zstackindex_list_act_get(void);

//Gibt den aktuellen Anzahl der freien Plätze vom MatchingCube-Stack aus
uint8_t mcstackindex_list_act_get(void);
#endif



