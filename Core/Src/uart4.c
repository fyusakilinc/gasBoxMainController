#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "uart4.h"
#include "usart.h"

//----- PRIVATE BEREICH -------------------------------------------------------
											// Max. Befehlsl�nge

UartRB uart4_rb;
UartRB uart5_rb;
UartRB usart1_rb;
UartRB usart2_rb;
UartRB usart3_rb;

volatile uint8_t TxBuffer[BUFLEN];

int16_t rb_free_rx(UartRB *p);
int16_t rb_free_tx(UartRB *p);
//----- INTERRUPT ROUTINEN ----------------------------------------------------

// Reception interrupt
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	uartRB_OnRxCplt(&uart4_rb, huart);
	uartRB_OnRxCplt(&uart5_rb, huart);
	uartRB_OnRxCplt(&usart1_rb, huart);
	uartRB_OnRxCplt(&usart2_rb, huart);
	uartRB_OnRxCplt(&usart3_rb, huart);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	uartRB_OnTxCplt(&uart4_rb, huart);
	uartRB_OnTxCplt(&uart5_rb, huart);
	uartRB_OnTxCplt(&usart1_rb, huart);
	uartRB_OnTxCplt(&usart2_rb, huart);
	uartRB_OnTxCplt(&usart3_rb, huart);
}

void uartRB_OnRxCplt(UartRB *p, UART_HandleTypeDef *huart) {
	if (huart != p->huart)
		return;

	if (1 < rb_free_rx(p))                        // noch Platz f�r ein Zeichen?
			{
		p->rx[p->rx_in] = p->it_rx;                      // Zeichen �bernehmen
		p->rx_in++;                                       // Zeiger weiterr�cken
		if (p->rx_in >= BUFLEN)
			p->rx_in = 0;
	}

	while (HAL_UART_Receive_IT(p->huart, &p->it_rx, 1) != HAL_OK) {
	}
}

void uartRB_OnTxCplt(UartRB *p, UART_HandleTypeDef *huart) {
	if (huart != p->huart)
		return;
	// batch send remaining bytes
	HAL_NVIC_DisableIRQ(p->irqn);
	int16_t x;
	x = BUFLEN - rb_free_tx(p);
	if (x > 0) {
		// Copy the characters to transmit out of the ring buffer
		for (uint16_t i = 0; i < x; i++) {
			p->tx_hold[i] = p->tx[p->tx_out];
			p->tx_out++;
			if (p->tx_out >= BUFLEN)
				p->tx_out = 0;
		}
		HAL_UART_Transmit_IT(p->huart, (uint8_t *) &p->tx_hold, x);
	}
	HAL_NVIC_EnableIRQ(p->irqn);
}

//----- FUNKTIONSDEFINITIONEN -------------------------------------------------

/******************** RS232 ***********************/

void uart_initAll(void) {
	uartRB_Init(&uart4_rb, &huart4, UART4_IRQn);
	uartRB_Init(&uart5_rb, &huart5, UART5_IRQn);
	uartRB_Init(&usart1_rb, &huart1, USART1_IRQn);
	uartRB_Init(&usart2_rb, &huart2, USART2_IRQn);
	uartRB_Init(&usart3_rb, &huart3, USART3_IRQn);
}

void uartRB_Init(UartRB *p, UART_HandleTypeDef *huart, IRQn_Type irqn) {
	p->huart = huart;
	p->irqn = irqn;
	p->tx_in = p->tx_out = p->rx_in = p->rx_out = 0;
	HAL_UART_Receive_IT(p->huart, &p->it_rx, 1);
}

int16_t rb_free_tx(UartRB *p) {
	int16_t x;
	HAL_NVIC_DisableIRQ(p->irqn);
	x = (int16_t) p->tx_in - (int16_t) p->tx_out;
	if (x < 0)
		x += BUFLEN;
	x = BUFLEN - x;
	HAL_NVIC_EnableIRQ(p->irqn);
	return (uint8_t) x;
}

int16_t rb_free_rx(UartRB *p) {
	int16_t x;
	HAL_NVIC_DisableIRQ(p->irqn);
	x = p->rx_in - p->rx_out;
	if (x < 0)
		x += BUFLEN;
	x = BUFLEN - x;
	HAL_NVIC_EnableIRQ(p->irqn);
	return x;
}

int16_t rb_rx_used(const UartRB *p)
{
	int16_t used;
    HAL_NVIC_DisableIRQ(p->irqn);
    used = (int16_t)p->rx_in - (int32_t)p->rx_out;
    if (used < 0) used += BUFLEN;
    HAL_NVIC_EnableIRQ(p->irqn);
    return used;
}

// is this correct
void uartRB_KickTx(UartRB *p) {

	if (__HAL_UART_GET_FLAG(p->huart, UART_FLAG_TC) == 0)
		return;
	HAL_NVIC_DisableIRQ(p->irqn);
// No, we still have something to transmit?
	int16_t free = rb_free_tx(p);          // how many free slots in TX ring
	int16_t tmpBuf_out = p->tx_out;
	if (free < BUFLEN) {               // => there is at least 1 byte pending
		p->tx_out++;
		if (p->tx_out >= BUFLEN) p->tx_out = 0;
		HAL_UART_Transmit_IT(p->huart, &p->tx[tmpBuf_out], 1);
	}
	HAL_NVIC_EnableIRQ(p->irqn);
}

uint8_t uartRB_Put(UartRB *p, const void *buf, uint8_t n) {
	HAL_NVIC_DisableIRQ(p->irqn);
	const uint8_t *src = (const uint8_t *)buf;
	if (n > rb_free_tx(p)){
		HAL_NVIC_EnableIRQ(p->irqn);
		return 0;
	}
	for (uint8_t i = 1; i <= n; i++) {
		p->tx[p->tx_in] = src[i-1];
		p->tx_in = (uint16_t) ((p->tx_in + 1) % BUFLEN);
	}
	HAL_NVIC_EnableIRQ(p->irqn);
	return 1;
}


uint8_t uartRB_Puts(UartRB *rb, const char *s) {
	HAL_NVIC_DisableIRQ(rb->irqn);
	uint8_t n;
	n = strlen(s);
	if (n < rb_free_tx(rb)) {
		for (uint8_t i = 1; i <= n; i++) {
			rb->tx[rb->tx_in] = s[i - 1];
			rb->tx_in = (uint16_t) ((rb->tx_in + 1) % BUFLEN);
		}
		HAL_NVIC_EnableIRQ(rb->irqn);
		return 1;
	}
	HAL_NVIC_EnableIRQ(rb->irqn);
	return 0;
}

uint8_t uartRB_Puti(UartRB *rb, uint32_t val) {
	HAL_NVIC_DisableIRQ(rb->irqn);
	char txt[20];
	uint8_t tmp = 0;
	sprintf(txt, "%8li", val);
	tmp = uartRB_Puts(rb, txt);
	HAL_NVIC_EnableIRQ(rb->irqn);
	uartRB_KickTx(rb);
	return tmp;
}

uint8_t uartRB_Getc(UartRB *p) {
	uint8_t c;
	HAL_NVIC_DisableIRQ(p->irqn);
	c = p->rx[p->rx_out];
	p->rx_out++;
	if (p->rx_out >= BUFLEN) p->rx_out = 0;
	HAL_NVIC_EnableIRQ(p->irqn);
	return c;
}
