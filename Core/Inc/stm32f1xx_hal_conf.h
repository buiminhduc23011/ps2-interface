/* stm32f1xx_hal_conf.h — chỉ bật module dùng: CORTEX/GPIO/RCC/PWR/FLASH/UART/DMA/EXTI */
#ifndef __STM32F1xx_HAL_CONF_H
#define __STM32F1xx_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/* Exported types */
#include "stm32f1xx_hal_rcc.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_exti.h"
#include "stm32f1xx_hal_dma.h"
#include "stm32f1xx_hal_cortex.h"
#include "stm32f1xx_hal_flash.h"
#include "stm32f1xx_hal_pwr.h"
#include "stm32f1xx_hal_uart.h"

/* Oscillator values */
#if !defined (HSE_VALUE)
#define HSE_VALUE    ((uint32_t)8000000U)   /* BluePill: thạch anh 8MHz */
#endif
#if !defined (HSI_VALUE)
#define HSI_VALUE    ((uint32_t)8000000U)
#endif
#if !defined (LSE_VALUE)
#define LSE_VALUE    ((uint32_t)32768U)
#endif
#if !defined (LSI_VALUE)
#define LSI_VALUE    ((uint32_t)40000U)
#endif

/* Module enabled */
#define HAL_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_EXTI_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED

/* System configuration */
#define HSE_STARTUP_TIMEOUT          100U
#define LSE_STARTUP_TIMEOUT          5000U
#define VDD_VALUE                    ((uint32_t)3300U)
#define TICK_INT_PRIORITY            ((uint32_t)0U)
#define USE_RTOS                     0U
#define PREFETCH_ENABLE              1U

/* Assert */
#ifdef USE_FULL_ASSERT
#include "stm32_assert.h"
#define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
void assert_failed(uint8_t *file, uint32_t line);
#else
#define assert_param(expr) ((void)0U)
#endif

#ifdef __cplusplus
}
#endif

#endif
