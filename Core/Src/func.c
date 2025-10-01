#include "main.h"
#include "func.h"
#include "timer0.h"

// Verz�gerung um n * 1us
void delay_us(uint16_t n)
{	uint16_t m = n * 12; // Calibrated for 160 MHz, 5 us.
	for(volatile uint32_t i=0; i<m; i++);				// volatile, damit nicht vom Compiler wegoptimiert
}

//Verz�gerung um n * 1ms
void delay_ms(uint16_t n)
{	HAL_Delay(n);
}

// Multiplikation mit einer reellen Zahl als Faktor
uint32_t rmult(uint16_t x1, uint16_t x2, uint8_t n)
{ return (((uint32_t) x1 * x2) >> n);
}

// Pr�fsummencheck
uint8_t chk_crc(uint8_t val1, uint8_t val2)
{
	if (((val1 | val2) == 0xFF) && ((val1 & val2) == 0x00))
		return 1;
	else
		return 0;
}

uint16_t clamp16(int32_t v) {
    if (v < 0)      return 0u;
    if (v > 65535)  return 65535u;
    return (uint16_t)v;
}
