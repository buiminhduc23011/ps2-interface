// ps2_ttl_bridge.h
// Bridge: doc UART tu module PS2-to-TTL (VD: mach chuyen cho tay PS2/PS3 Arduino UNO)
// roi ban lai ra PC qua Serial (USART1) hoac USB CDC (PA11/PA12).
// Board: STM32F1 BluePill (STM32F103C8T6), HAL + CubeMX
//
// Luong: USART2 (PA2/PA3) <- module PS2 | USART1 (PA9/PA10) -> USB-TTL -> PC
// Neu muon ra thang cong Type-C native USB: bat USB_DEVICE CDC trong CubeMX,
// dinh nghia USE_USB_CDC = 1, goi PS2_Bridge_SetUsbTx().

#ifndef PS2_TTL_BRIDGE_H
#define PS2_TTL_BRIDGE_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

#ifndef PS2_RX_BUF_SIZE
#define PS2_RX_BUF_SIZE 512
#endif

// -- Cau hinh: sua 2 dong nay cho khop CubeMX cua ban --
extern UART_HandleTypeDef huart1; // DEBUG ra PC (115200 8N1)
extern UART_HandleTypeDef huart2; // PS2 module (mac dinh 9600 8N1)
#define PS2_UART   (&huart2)
#define DEBUG_UART (&huart1)

#ifdef __cplusplus
extern "C" {
#endif

void PS2_Bridge_Init(void);   // goi 1 lan sau MX_USARTx_Init()
void PS2_Bridge_Task(void);   // goi trong while(1), non-blocking

// Callback yeu: co frame moi (da gom theo timeout 10ms). Ghi de o main.c de decode.
__weak void PS2_OnFrame(const uint8_t *data, uint16_t len);

// Gui chuoi raw ra PC (dung cho USB-CDC hoac UART tuy cau hinh)
void DEBUG_Tx(const uint8_t *data, uint16_t len);

// Goi trong stm32f1xx_it.c / main.c khi co ngat UART (neu dung IT byte-by-byte)
void PS2_Bridge_RxCpltCallback(UART_HandleTypeDef *huart);
void PS2_Bridge_ErrorCallback(UART_HandleTypeDef *huart);

// Tien ich: doi baud USART2 luc chay de do frame module la (9600/19200/115200...)
HAL_StatusTypeDef PS2_Bridge_SetBaud(uint32_t baud);

#ifdef __cplusplus
}
#endif

#endif
