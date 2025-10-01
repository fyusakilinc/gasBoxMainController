/*-------------------------------------------------------------------------------------------------------------------------
Datei:				priority_pushpop.c
Version:			A00
Plattform:			Pv
Baugruppe:
Autor:				jl
Letzte Änderung:
CodeFreeze:			nein

[Dokumentation]
Die Aufgaben von priority_pushpop.c sind:
1. Hinfügen des Stackindex mit der angegebenen Priorität in die entsprechenden Prioritätsliste
2. Geben einen Stackindex des Anfragepaketes mit der angegebenen Priorität frei
-------------------------------------------------------------------------------------------------------------------------*/
#ifndef PRIORITY_PUSHPOP_H
#define PRIORITY_PUSHPOP_H
//--- PUBLIC FUNKTIONSDEKLARATION -----------------------------------------------------------------------------------------
//Fügt einen Stackindex des Anfragepaketes mit der angegebenen Priorität in die entsprechenden Prioritätsliste ein.
uint8_t priolist_push (priority_item prio_list[], uint8_t priolist_length, uint8_t priolevel_header[], uint8_t *firstunused, uint8_t stack_index, uint8_t priolevel_data);

//Gibt einen Stackindex des Anfragepaketes mit der angegebenen Priorität aus.
uint8_t priolist_pop (priority_item prio_list[], uint8_t priolevel_header[], uint8_t *firstunused, uint8_t priolevel_data);
#endif
