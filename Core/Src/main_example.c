// main_example.c — chep cac doan nay vao main.c do CubeMX sinh ra (HAL, STM32F103 BluePill)
//
// CubeMX (nho lam du 3 buoc nay):
//  1. USART1: Mode Asynchronous, 115200 8N1 -> chan PA9(TX) PA10(RX). Day la cong DEBUG ra PC
//     (qua mach USB-TTL CH340/CP2102: TX_STM32 -> RX_USB, RX_STM32 -> TX_USB, GND chung).
//  2. USART2: Mode Asynchronous, 9600 8N1 (baud module PS2, neu sai se doi sau) -> PA2(TX) PA3(RX).
//     NVIC: bat ngat USART2 global interrupt (de nhan IT tung byte).
//  3. (Tuy chon, neu muon ra thang cong Type-C native): Middleware USB_DEVICE Class CDC.
//     Khong bat thi cu dung USART1 + USB-TTL ngoai, on dinh hon.
//
// Dau noi (quan trong!):
//   Module PS2-TTL (5V)      STM32 BluePill (3.3V)
//   VCC (5V)              -> 5V (khong cap 3.3V, module doi 5V)
//   GND                   -> GND (bat buoc chung mass)
//   TX (module)           -> PA3 (RX USART2)  [chan STM32 chiu 5V: PA3 la FT, doc truc tiep OK]
//   RX (module, neu co)   -> PA2 (TX USART2) qua R 1k (phong khi module 5V-only)
//   DEBUG ra PC:
//   PA9 (TX USART1)       -> RX mach USB-TTL -> cam USB vao PC, mo Hercules/PuTTY 115200
//   GND                   -> GND mach USB-TTL
//
// Type-C tren BluePill: neu board cua ban Type-C noi thang PA11/PA12 (native USB)
// thi phai bat USB CDC moi thay COM. Neu Type-C tren board chi de cap nguon + nap
// (qua ST-Link) thi bat buoc dung USB-TTL ngoai nhu tren.

#include "main.h"
#include "ps2_ttl_bridge.h"
#include <string.h>
#include <stdio.h>

/* --- Ghi de PS2_OnFrame neu muon decode thay vi in HEX ---
 * Cac module PS2-to-UART tren thi truong thuong co 1 trong 2 dang:
 *  A) Moi nut/joystick la 1 frame ngan, VD bat dau bang 0xAA 0x55 ...
 *  B) Stream lien tuc ~8-9 byte @50-100Hz chua bitmask nut + XY joystick.
 * Ban cu doc HEX 1 luc (bam tung nut, keo joystick) roi dien if ben duoi.
 */
void PS2_OnFrame(const uint8_t *data, uint16_t len)
{
  // Mac dinh giu HEX dump de do frame (copy tu file goc, tranh trung dinh nghia weak):
  // [len] AA 55 ... |ascii|
  char tmp[16];
  int n = snprintf(tmp, sizeof(tmp), "[%u] ", (unsigned)len);
  HAL_UART_Transmit(&huart1, (uint8_t *)tmp, n, 50);
  for (uint16_t i = 0; i < len; i++) {
    n = snprintf(tmp, sizeof(tmp), "%02X ", data[i]);
    HAL_UART_Transmit(&huart1, (uint8_t *)tmp, n, 50);
  }
  HAL_UART_Transmit(&huart1, (uint8_t *)"\r\n", 2, 50);

  // ---- Mau decode (sua lai theo HEX ban do duoc) ----
  // VD gia su frame 6 byte: [0xAA][0x55][BTN_H][BTN_L][LX][LY]
  // if (len == 6 && data[0] == 0xAA && data[1] == 0x55) {
  //   uint16_t btn = (data[2] << 8) | data[3];
  //   char s[64]; int m = snprintf(s, sizeof(s), "BTN=%04X LX=%u LY=%u\r\n", btn, data[4], data[5]);
  //   HAL_UART_Transmit(&huart1, (uint8_t*)s, m, 50);
  //   // TODO: dieu khien thu gi do o day (motor/relay/...) dua tren btn/LX/LY
  // }
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART1_UART_Init(); // 115200 ra PC
  MX_USART2_UART_Init(); // 9600 (tam) doc module PS2
  // MX_USB_DEVICE_Init(); // chi neu bat USB CDC

  PS2_Bridge_Init();

  // Tu test baud neu chua biet baud module: thu 9600 roi doi dan.
  // Mo terminal 115200, bam nut tay cam. Thay rac/khong thay -> thu baud ke tiep:
  // const uint32_t bauds[] = {9600, 19200, 38400, 57600, 115200};
  // -> goi PS2_Bridge_SetBaud(bauds[i]) hoac sua trong CubeMX roi nap lai.

  while (1)
  {
    PS2_Bridge_Task(); // non-blocking, gom frame theo timeout 10ms
    HAL_Delay(1);
  }
}

// --- Bat buoc: noi callback HAL sang bridge (them vao main.c, ngoai main) ---
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  PS2_Bridge_RxCpltCallback(huart);
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  PS2_Bridge_ErrorCallback(huart);
}
