/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    spi.c
  * @brief   This file provides code for the configuration
  *          of the SPI instances.
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
/* Includes ------------------------------------------------------------------*/
#include "spi.h"

/* USER CODE BEGIN 0 */
void spi_set_cs(uint8_t, uint8_t);
void _spi_access_device(uint8_t, uint8_t);
#include "ad5592.h"
/* USER CODE END 0 */

SPI_HandleTypeDef hspi1;

/* SPI1 init function */
void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

void HAL_SPI_MspInit(SPI_HandleTypeDef* spiHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(spiHandle->Instance==SPI1)
  {
  /* USER CODE BEGIN SPI1_MspInit 0 */

  /* USER CODE END SPI1_MspInit 0 */
    /* SPI1 clock enable */
    __HAL_RCC_SPI1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**SPI1 GPIO Configuration
    PA5     ------> SPI1_SCK
    PA6     ------> SPI1_MISO
    PA7     ------> SPI1_MOSI
    */
    GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN SPI1_MspInit 1 */

  /* USER CODE END SPI1_MspInit 1 */
  }
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef* spiHandle)
{

  if(spiHandle->Instance==SPI1)
  {
  /* USER CODE BEGIN SPI1_MspDeInit 0 */

  /* USER CODE END SPI1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SPI1_CLK_DISABLE();

    /**SPI1 GPIO Configuration
    PA5     ------> SPI1_SCK
    PA6     ------> SPI1_MISO
    PA7     ------> SPI1_MOSI
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7);

  /* USER CODE BEGIN SPI1_MspDeInit 1 */

  /* USER CODE END SPI1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
void spiSendByte(uint8_t data) {
    // send a byte over SPI and ignore reply (blocking)
    HAL_SPI_Transmit(&hspi1, &data, 1, 100);
}

uint8_t spiTransferByte(uint8_t txData) {
    uint8_t rxData = 0;

    // blocking tx/rx over SPI
    HAL_SPI_TransmitReceive(&hspi1, &txData, &rxData, 1, 100);

    // return the received data
    return rxData;
}

uint16_t spiTransferWord(uint16_t data) {
    uint16_t rxData = 0;

    HAL_SPI_TransmitReceive(&hspi1, (uint8_t *)&data, (uint8_t *)&rxData, 2, 100);

    // return the received data
    return rxData;
}

void spi_access_device(uint8_t device)
{
	 _spi_access_device(device, spi_select_device);
}

void spi_release_device(uint8_t device)
{
	 _spi_access_device(device, spi_deselect_device);
}

void _spi_access_device(uint8_t device, uint8_t status) {
	SPI_HandleTypeDef *spiHandle;

	if (status == spi_select_device) {
		spiHandle = &hspi1;
		/*Prior to changing the CPOL/CPHA bits the SPI must be disabled by resetting the SPE bit*/
		if (HAL_SPI_DeInit(spiHandle) != HAL_OK) {
			Error_Handler();
		}

		spiHandle->Instance = SPI1;
		spiHandle->Init.Mode = SPI_MODE_MASTER;
		spiHandle->Init.Direction = SPI_DIRECTION_2LINES;
		spiHandle->Init.DataSize = SPI_DATASIZE_8BIT;
		spiHandle->Init.CLKPolarity = SPI_POLARITY_LOW;
		spiHandle->Init.CLKPhase = SPI_PHASE_1EDGE;
		spiHandle->Init.NSS = SPI_NSS_SOFT;
		spiHandle->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
		spiHandle->Init.FirstBit = SPI_FIRSTBIT_MSB;
		spiHandle->Init.TIMode = SPI_TIMODE_DISABLE;
		spiHandle->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
		spiHandle->Init.CRCPolynomial = 7;
		spiHandle->Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
		spiHandle->Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
		if (HAL_SPI_Init(spiHandle) != HAL_OK) {
			Error_Handler();
		}

		switch (device) {
		case spi_mio:
			spi_set_cs(spi_mio_cs, spi_select_device);
			break;
		case spi_sps_in:
			spi_set_cs(spi_sps_in_cs, spi_select_device);
			break;
		case spi_sps_out:
			spi_set_cs(spi_sps_out_cs, spi_select_device);
			break;
		case spi_mcp:
			// High-Speed SPI Interface (MCP23S17): - 10 MHz (maximum) baud pre-scaler might needs to be changed
			spi_set_cs(spi_mcp_cs, spi_select_device);
			break;

		default:
			break;
		}
	} else {
		spi_set_cs(spi_mio_cs, spi_deselect_device);
		spi_set_cs(spi_sps_out_cs, spi_deselect_device);
		spi_set_cs(spi_sps_in_cs, spi_deselect_device);
		spi_set_cs(spi_mcp_cs, spi_deselect_device);
	}
}

void spi_set_cs(uint8_t cs, uint8_t state)
{
	if(state != spi_cs_all_off)
	{
		// Chipselect aktivieren
		switch(cs)
		{
		case spi_mio_cs:
			if (state == spi_select_device)
				HAL_GPIO_WritePin(UC_CS_AUX0_GPIO_Port, UC_CS_AUX0_Pin,	GPIO_PIN_RESET);
			else
				HAL_GPIO_WritePin(UC_CS_AUX0_GPIO_Port, UC_CS_AUX0_Pin,	GPIO_PIN_SET);
			break;

		case spi_sps_out_cs:
			if (state == spi_select_device)
				HAL_GPIO_WritePin(UC_CS_SPS_OUT_GPIO_Port, UC_CS_SPS_OUT_Pin,	GPIO_PIN_RESET);
			else
				HAL_GPIO_WritePin(UC_CS_SPS_OUT_GPIO_Port, UC_CS_SPS_OUT_Pin,	GPIO_PIN_SET);
			break;

		case spi_sps_in_cs:
			if (state == spi_select_device)
				HAL_GPIO_WritePin(UC_CS_SPS_IN_GPIO_Port, UC_CS_SPS_IN_Pin,	GPIO_PIN_RESET);
			else
				HAL_GPIO_WritePin(UC_CS_SPS_IN_GPIO_Port, UC_CS_SPS_IN_Pin,	GPIO_PIN_SET);
			break;

		case spi_mcp_cs:
			if (state == spi_select_device)
				HAL_GPIO_WritePin(UC_CS_AUX1_GPIO_Port, UC_CS_AUX1_Pin,	GPIO_PIN_RESET);
			else
				HAL_GPIO_WritePin(UC_CS_AUX1_GPIO_Port, UC_CS_AUX1_Pin,	GPIO_PIN_SET);
			break;

			default:
				break;
		}
	}
	else
	{
		// Chip selects off
		HAL_GPIO_WritePin(UC_CS_AUX0_GPIO_Port, UC_CS_AUX0_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(UC_CS_SPS_OUT_GPIO_Port, UC_CS_SPS_OUT_Pin,	GPIO_PIN_SET);
		HAL_GPIO_WritePin(UC_CS_SPS_IN_GPIO_Port, UC_CS_SPS_IN_Pin,	GPIO_PIN_SET);
		HAL_GPIO_WritePin(UC_CS_AUX1_GPIO_Port, UC_CS_AUX1_Pin,	GPIO_PIN_SET);
	}
}
/* USER CODE END 1 */
