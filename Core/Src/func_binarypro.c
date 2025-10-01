/*
 * binary_protokol_func.c
 *
 * Created: 08.12.2016 11:14:15
 *  Author: Jianmin
 */

//#include <stdio.h>
//#include <string.h>

#ifndef _STDINT_H
#include <stdint.h>
#endif

#include "SG_global.h"
#include "func.h"
#include "protocoll.h"

//--- FUNKTIONSDEFINITION ---------------------------------------------------------------------------------------------------

//hole die Byte-Werte aus dem Binary_Paket und setze 32-bit-Wert zusammen.
int32_t get_ParameterValue_32(uint8_t buffer[])
{	int32_t param;
	uint8_t i;

	param = buffer[PARAMETER_START];

	for (i=PARAMETER_START+1; i<=PARAMETER_END; i++)
	{
		param = param << 8;
		param |= buffer[i];
	}

	return param;
}
