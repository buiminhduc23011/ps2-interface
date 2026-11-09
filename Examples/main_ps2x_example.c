// main_ps2x_example.c — Vidu hoan chinh: doc tay PS2 truc tiep (DAT/CMD/CLK/ATT)
// roi "ban lai qua serial" (USART1 -> PC). Tuong duong PS2X_Example.ino cua Arduino.
//
// Dau noi de xuat (BluePill STM32F103, sua lai neu ban dung chan khac):
//   Receiver PS2 (board thu tay PS2)    STM32
//   VCC (3.3V!)                      -> 3V3  (uu tien 3.3V cho an toan; chi len 5V neu song yeu)
//   GND                              -> GND (bat buoc chung mass)
//   DAT                              -> PB15 (Input PullUp)
//   CMD                              -> PB14 (Output PP High)
//   ATT (SEL/CS)                     -> PB12 (Output PP High)
//   CLK                              -> PB13 (Output PP High)
//
// Serial ra PC: PA9 (TX USART1) -> RX mach USB-TTL -> PC 115200 8N1. PA10/GND tuong ung.
// (Cong Type-C tren BluePill muon thay COM phai bat USB CDC; khong thi dung USB-TTL ngoai.)
//
// CubeMX:
//  1. USART1 Async 115200. USART2 khong can (gi lai cho viec khac).
//  2. PB12/PB13/PB14: GPIO_Output PP, Pull No, Speed High. Muc khoi tao: SET (idle HIGH).
//     PB15: GPIO_Input, PullUp.
//  3. Copy noi dung main() + PS2X_Handle global nay vao main.c cua ban.

#include "main.h"
#include "ps2x_lib.h"
#include <stdio.h>

// Chon chan tai day (khop CubeMX):
#define PS2_CLK_PORT GPIOB
#define PS2_CLK_PIN  GPIO_PIN_13
#define PS2_CMD_PORT GPIOB
#define PS2_CMD_PIN  GPIO_PIN_14
#define PS2_ATT_PORT GPIOB
#define PS2_ATT_PIN  GPIO_PIN_12
#define PS2_DAT_PORT GPIOB
#define PS2_DAT_PIN  GPIO_PIN_15

// pressures=false, rumble=false nhu PS2X_Example.ino mac dinh
#define PS2_PRESSURES false
#define PS2_RUMBLE    false

PS2X_Handle ps2;
extern UART_HandleTypeDef huart1;

// printf() -> USART1 (ban lai qua serial)
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif
PUTCHAR_PROTOTYPE
{
  uint8_t c = (uint8_t)ch;
  HAL_UART_Transmit(&huart1, &c, 1, 10);
  return ch;
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART1_UART_Init();

  HAL_Delay(300); // cho module wireless on dinh nhu ban Arduino

  PS2X_Init(&ps2,
            PS2_CLK_PORT, PS2_CLK_PIN,
            PS2_CMD_PORT, PS2_CMD_PIN,
            PS2_ATT_PORT, PS2_ATT_PIN,
            PS2_DAT_PORT, PS2_DAT_PIN);

  uint8_t err = PS2X_ConfigGamepad(&ps2, PS2_PRESSURES, PS2_RUMBLE);
  uint8_t type = 0;
  if (err == 0) {
    printf("Found Controller, configured successful\r\n");
    type = PS2X_ReadType(&ps2);
    printf("type=%u (1=DualShock 2=Guitar 3=Wireless)\r\n", type);
    printf("Bam nut, gia tri se ban lien tuc qua serial:\r\n");
  } else if (err == 1) {
    printf("No controller found, check wiring\r\n");
  } else if (err == 2) {
    printf("Controller found but not accepting commands\r\n");
  } else if (err == 3) {
    printf("Controller refusing pressures mode\r\n");
  }

  uint8_t vibrate = 0;

  while (1)
  {
    if (err != 0) { HAL_Delay(500); continue; } // khong co tay cam thi doi

    PS2X_ReadGamepad(&ps2, false, vibrate);

    // --- Ban raw + nut + joystick qua serial (may tinh doc de dieu khien tiep) ---
    PS2X_DumpState(&ps2, &huart1);

    // --- Vi du tuong duong Arduino: ---
    if (PS2X_Button(&ps2, PSB_START))     printf("Start is being held\r\n");
    if (PS2X_Button(&ps2, PSB_SELECT))    printf("Select is being held\r\n");
    if (PS2X_ButtonPressed(&ps2, PSB_CIRCLE))  printf("Circle just pressed\r\n");
    if (PS2X_ButtonReleased(&ps2, PSB_SQUARE)) printf("Square just released\r\n");
    if (PS2X_NewButtonStateOf(&ps2, PSB_CROSS)) printf("X just changed\r\n");
    if (PS2X_Button(&ps2, PSB_L1) || PS2X_Button(&ps2, PSB_R1)) {
      printf("Stick: LY=%u LX=%u RY=%u RX=%u\r\n",
             PS2X_Analog(&ps2, PSS_LY), PS2X_Analog(&ps2, PSS_LX),
             PS2X_Analog(&ps2, PSS_RY), PS2X_Analog(&ps2, PSS_RX));
    }
    vibrate = PS2X_Analog(&ps2, PSAB_CROSS); // nhan X cang manh rung cang manh (neu bat rumble)

    HAL_Delay(50); // nhu delay(50) ban Arduino
  }
}
