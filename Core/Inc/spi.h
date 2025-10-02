/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    spi.h
  * @brief   This file contains all the function prototypes for
  *          the spi.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SPI_H__
#define __SPI_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#define spi_deselect_device	0
#define spi_select_device	1

#define spi_dds			0
#define spi_dds_cs		0

#define spi_mio			1
#define spi_mio_cs		1

#define spi_sps_out			2
#define spi_sps_out_cs		2

#define spi_sps_in			3
#define spi_sps_in_cs		3

#define spi_mcp			4
#define spi_mcp_cs		4

#define spi_cs_all_off	255
/* USER CODE END Includes */

extern SPI_HandleTypeDef hspi1;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_SPI1_Init(void);

/* USER CODE BEGIN Prototypes */
void spiSendByte(uint8_t);
uint8_t spiTransferByte(uint8_t);
uint16_t spiTransferWord(uint16_t);
void spi_access_device(uint8_t);
void spi_release_device(uint8_t);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __SPI_H__ */

