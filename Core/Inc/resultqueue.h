/*-------------------------------------------------------------------------------------------------------------------------
Datei:				resultqueue.h
Version:			A00
Plattform:			Pv
Baugruppe:
Autor:				jl
Letzte Änderung:
CodeFreeze:			nein

[Dokumentation]
resultqueue.c bereitet eine Wartschlange resultqueue vor, welche die Ergebnispakete vom z_cmdswitch und mc_cmdswitch aufnehmen.
result_get_servo() liefert dem entsprechenden Anfrager das ankommenden Ergebnispaket zurück.
-------------------------------------------------------------------------------------------------------------------------*/
#ifndef RESULTQUEUE_H
#define RESULTQUEUE_H
#include "SG_global.h"
//--- PUBLIC FUNKTIONSDEKLARATION -----------------------------------------------------------------------------------------
//Initialisierung der Warteschlange
void resultQueue_init(void);

//gibt die Anzahl der freien Plaetzen aus
uint8_t get_anzFrei_resultQueue(void);

//gibt die Anzahl der besetzten Plaetzen aus
uint8_t get_anzBes_resultQueue(void);

//Hinfuegen eines Kommandos in die Warteschlange der Kommandos
uint8_t resultQueue_push(stack_item);

//gibt ein Ergebnispaket aus
void resultQueue_pop(stack_item *);

//liefert dem entsprechenden Anfrager das Ergebnispaket zurück
void result_get_sero(void);
#endif
