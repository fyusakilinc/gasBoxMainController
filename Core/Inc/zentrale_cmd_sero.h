/*-------------------------------------------------------------------------------------------------------------------------
Datei:				z_cmdswitch.h
Version:			A00
Plattform:			Pv
Baugruppe:
Autor:				jl
Letzte Änderung:
CodeFreeze:			nein

[Dokumentation]
zentrale_cmd_sero.c verarbeitet die Anfrage nach die Informationen der Zentrale.
Die Strategie ist:
das Programm holt jedes Mal max.5 Befehle aus dem Zentrale_Stack, die max.3 Befehle mit höchster Priorität und
min. 1 Befehle mit mittlerer oder niedriger Priorität sind. Danach werden die Befehlen in z_cmd_servo() verarbeitet. Das Ergebnis
mit dem Acknowledge werden wieder in die Result-Wartschlange eingefügt.
-------------------------------------------------------------------------------------------------------------------------*/
#ifndef ZENTRALE_CMD_SERO_H
#define ZENTRALE_CMD_SERO_H

//--- PUBLIC FUNKTIONSDEKLARATION -----------------------------------------------------------------------------------------
//Verarbeitung der Befehle für die Zentrale
void z_cmd_scheduler(void);
#endif
