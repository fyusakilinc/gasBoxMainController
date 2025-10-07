#include "iso.h"
#include "gpio.h"
#include "main.h"
#include "spi.h"

static volatile uint8_t iso_out_shadow = 0x00;   // set to your power-up state
HAL_StatusTypeDef isoWrite(uint8_t pattern);

static inline void iso_write_shadow(uint8_t v) {
	iso_out_shadow = v;
	(void) isoWrite(v);
}

void iso_init(void) {
	HAL_NVIC_DisableIRQ(SPI1_IRQn);
	spi_access_device(spi_sps_out);

	iso_out_shadow = 0x00;
	isoWrite(0x00);

	spi_release_device(spi_sps_out);
	HAL_NVIC_EnableIRQ(SPI1_IRQn);

}

static void iso_write_masked(uint8_t mask, uint8_t value)  // change only bits in mask
{
	HAL_NVIC_DisableIRQ(SPI1_IRQn);
	spi_access_device(spi_sps_out);

	uint8_t v = (iso_out_shadow & ~mask) | (value & mask); // zero the position and set the value
	iso_write_shadow(v);

	spi_release_device(spi_sps_out);
	HAL_NVIC_EnableIRQ(SPI1_IRQn);
}


HAL_StatusTypeDef isoWrite(uint8_t pattern)
{
	HAL_NVIC_DisableIRQ(SPI1_IRQn);
    spi_access_device(spi_sps_out);

    HAL_StatusTypeDef st = HAL_SPI_Transmit(&hspi1, &pattern, 1, 10);

    spi_release_device(spi_sps_out);
    HAL_NVIC_EnableIRQ(SPI1_IRQn);

    return st;
}

uint8_t isoRead(void) { // TODO this function might not be correct, have searched the internet for fw example but none, from datasheet this is plausible
	uint8_t tx = 0x00;
	uint8_t rx = 0x00;

	HAL_NVIC_DisableIRQ(SPI1_IRQn);
	spi_access_device(spi_sps_in);

	HAL_SPI_TransmitReceive(&hspi1, &tx, &rx, 1, 10);

	spi_release_device(spi_sps_in);
	HAL_NVIC_EnableIRQ(SPI1_IRQn);

	return rx;     // status + inputs
}

void iso_set_bit(uint8_t bit, uint8_t value) // Set a specific bit (0..7) to 0 or 1
{
    uint8_t mask = (1u << bit);
    uint8_t val = value ? mask : 0u;
    iso_write_masked(mask, val);
}

uint8_t iso_get_bit(uint8_t bit) // Get a specific bit (0..7)
{
    uint8_t v = isoRead();
    return (v >> bit) & 1u;
}

// Get any output bit from the shadow
uint8_t iso_get_output_bit(uint8_t bit)
{
    return (iso_out_shadow >> bit) & 1u;
}

void iso_valve_set(uint8_t value)     { iso_set_bit(0, value); }
void relais_set(uint8_t value)        { iso_set_bit(4, value); }
void buzzer_set(uint8_t value)        { iso_set_bit(5, value); }
void ledbereit_set(uint8_t value)     { iso_set_bit(6, value); }
void ledpumpe_set(uint8_t value)      { iso_set_bit(7, value); }

uint8_t relais_get(void)      { return iso_get_output_bit(4); }
uint8_t buzzer_get(void)      { return iso_get_output_bit(5); }
uint8_t ledbereit_get(void)   { return iso_get_output_bit(6); }
uint8_t ledpumpe_get(void)    { return iso_get_output_bit(7); }

uint8_t iso_valve_get(void)            { return iso_get_bit(0); }
uint8_t atm_sensor_get(void)           { return iso_get_bit(1); }
uint8_t door_switch_get(void)          { return iso_get_bit(2); }
uint8_t air_sensor_get(void)            { return iso_get_bit(6); }
uint8_t stop_button_get(void)           { return iso_get_bit(7); }

// TODO do we need to do these with interrupts?
