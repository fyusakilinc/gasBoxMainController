#include "main.h"


#pragma once
#include "stm32g4xx_hal.h"
#include <stdint.h>
// Instances (defined in uart4.c)


#define ECHO				1												// 0: Echo aus 1: Echo ein
#define BUFLEN 				127												// USART RS232 RX/TX Pufferl�nge
#define CMDLEN				16

typedef struct {
    UART_HandleTypeDef *huart;
    IRQn_Type irqn;

    // rings
    volatile uint16_t tx_in, tx_out;
    volatile uint16_t rx_in, rx_out;
    uint8_t tx[127];
    uint8_t rx[127];

    // one-byte IT buffers
    uint8_t it_rx;
    uint8_t it_tx_scratch; // optional
    volatile uint8_t tx_hold[BUFLEN];
} UartRB;

void uartRB_Init(UartRB *p, UART_HandleTypeDef *huart, IRQn_Type irqn);
uint8_t uartRB_Put(UartRB *p, const void *buf, uint8_t n);
uint8_t uartRB_Puts(UartRB *p, const char *s);
void     uartRB_KickTx(UartRB *p);
uint8_t  uartRB_RxAvail(const UartRB *p);
uint8_t  uartRB_Getc(UartRB *p);
void uart_initAll(void);

// to be called from HAL callbacks:
void uartRB_OnRxCplt(UartRB *p, UART_HandleTypeDef *huart);
void uartRB_OnTxCplt(UartRB *p, UART_HandleTypeDef *huart);

int16_t rb_free_tx(UartRB *p);
int16_t rb_free_rx(UartRB *p);
int16_t rb_rx_used(const UartRB *p);

extern UartRB uart4_rb;
extern UartRB uart5_rb;
extern UartRB usart1_rb;
extern UartRB usart3_rb;
