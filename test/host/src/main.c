#include "main.h"
#include "adc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "uart4.h"
#include "Hardware.h"
#include "iso.h"
#include "remote.h"
#include "remote_xport.h"
#include "zentrale.h"
#include "timer0.h"
#include "func.h"
#include "prioritylist.h"
#include "priority_pushpop.h"
#include "stacks.h"
#include "resultqueue.h"
#include "zentrale_cmd_sero.h"
#include "gasbox.h"
#include "mcp.h"
#include "apc.h"
#include "rfg.h"
#include "ad5592.h"
#include "gasbox_tests.h"
#include "unity.h"

int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

void SystemClock_Config(void);

/* ------------------ Unity hooks ------------------ */
void setUp(void) {}
void tearDown(void) {}

int main(void) {

	HAL_Init();
	SystemClock_Config();
	MX_GPIO_Init();
	MX_SPI1_Init();
	MX_UART4_Init();
	MX_UART5_Init();
	MX_USART1_UART_Init();
	MX_USART2_UART_Init();
	MX_USART3_UART_Init();
	MX_TIM1_Init();
	MX_ADC5_Init();

	HAL_TIM_Base_Start_IT(&htim1);
	priolist_init();
	stacks_init();
	resultQueue_init();
	uart_initAll();
	remote_init();
	remote_xport_init();
	zentrale_init();
	mio_init();
	mcp_init();
	apc_init();
	iso_init();
	rfg_init();
	hw_xport_reset_disable(1);

	delay_ms(500);

	printf("\r\n=== Starting GasBox Tests ===\r\n");

	UNITY_BEGIN();

	RUN_TEST(test_gasbox_set_mfc1_0x000);
	RUN_TEST(test_gasbox_set_mfc1_0x800);
	RUN_TEST(test_gasbox_set_mfc1_0xFFF);

	RUN_TEST(test_gasbox_set_mfc2_0x000);
	RUN_TEST(test_gasbox_set_mfc2_0x800);
	RUN_TEST(test_gasbox_set_mfc2_0xFFF);

	RUN_TEST(test_gasbox_set_mfc3_0x000);
	RUN_TEST(test_gasbox_set_mfc3_0x800);
	RUN_TEST(test_gasbox_set_mfc3_0xFFF);

	RUN_TEST(test_gasbox_open_valve3);
	RUN_TEST(test_gasbox_close_valve3);

	UNITY_END();

	while (1) {
		hw_sero_get();
		remote_sero_get();
		remote_xport_sero_get();
		gb_sero_get();
		mio_sero_get();
		mcp_sero_get();
		apc_sero_get();
		rfg_sero_get();

		zentrale();

		hw_sero_set();
		mio_sero_set();
		mcp_sero_set();
		apc_sero_set();
		z_cmd_scheduler();
		result_get_sero();

	}

}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
