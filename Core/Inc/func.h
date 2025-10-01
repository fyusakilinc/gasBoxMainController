#include <stdint.h> // otherwise the compiler does not know about uint16 etc for some reason


// Verzoegerung um n * 1us
void delay_us(uint16_t n);

//Verzoegerung um n * 1ms
void delay_ms(uint16_t n);

// Mutiplikation mit einer reellen Zahl
uint32_t rmult(uint16_t, uint16_t, uint8_t);

// Pruefsummentest
uint8_t chk_crc(uint8_t, uint8_t);
