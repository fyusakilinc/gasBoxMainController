#include "iso.h"
#include "gpio.h"
#include "main.h"
#include "spi.h"


HAL_StatusTypeDef isoWrite(uint8_t pattern)
{
	HAL_NVIC_DisableIRQ(SPI1_IRQn);
    spi_access_device(spi_sps_out);
    HAL_StatusTypeDef st = HAL_SPI_Transmit(&hspi1, &pattern, 1, 10);
    spi_release_device(spi_sps_out);
    HAL_NVIC_EnableIRQ(SPI1_IRQn);
    return st;
}

uint16_t iso_read_in_raw(void) {
	HAL_NVIC_DisableIRQ(SPI1_IRQn);
    uint8_t tx[2] = {0x00, 0x00}, rx[2] = {0,0};
    spi_access_device(spi_sps_in);
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 2, 10);
    spi_release_device(spi_sps_in);
    HAL_NVIC_EnableIRQ(SPI1_IRQn);
    return ((uint16_t)rx[0] << 8) | rx[1];     // status + inputs
}

uint8_t isoRead(void) {
    uint16_t w = iso_read_in_raw();
    return (uint8_t)(w & 0xFF);
}

// do we need to do this with interrupts?
