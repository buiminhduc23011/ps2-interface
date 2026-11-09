/* main.h — STM32F103C8 BluePill + PS2 direct read (DAT/CMD/CLK/ATT) */
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

extern UART_HandleTypeDef huart1;

void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_USART1_UART_Init(void);
void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif
