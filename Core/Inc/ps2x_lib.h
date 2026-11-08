// ps2x_lib.h — Port cua Arduino-PS2X (Bill Porter v1.8, madsci1016) sang STM32F1 HAL.
// Giao thuc: bit-bang SPI kieu PS2 (CLK/CMD/DAT/ATT), khong phai UART.
// Original: https://github.com/madsci1016/Arduino-PS2X (GPL)

#ifndef PS2X_LIB_H
#define PS2X_LIB_H

#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// --- Button constants (giua nguyen nhu Arduino-PS2X) ---
#define PSB_SELECT      0x0001
#define PSB_L3          0x0002
#define PSB_R3          0x0004
#define PSB_START       0x0008
#define PSB_PAD_UP      0x0010
#define PSB_PAD_RIGHT   0x0020
#define PSB_PAD_DOWN    0x0040
#define PSB_PAD_LEFT    0x0080
#define PSB_L2          0x0100
#define PSB_R2          0x0200
#define PSB_L1          0x0400
#define PSB_R1          0x0800
#define PSB_GREEN       0x1000
#define PSB_RED         0x2000
#define PSB_BLUE        0x4000
#define PSB_PINK        0x8000
#define PSB_TRIANGLE    0x1000
#define PSB_CIRCLE      0x2000
#define PSB_CROSS       0x4000
#define PSB_SQUARE      0x8000

// Stick values (index trong PS2data)
#define PSS_RX 5
#define PSS_RY 6
#define PSS_LX 7
#define PSS_LY 8

// Analog buttons (index trong PS2data khi enable pressures)
#define PSAB_PAD_RIGHT    9
#define PSAB_PAD_UP      11
#define PSAB_PAD_DOWN    12
#define PSAB_PAD_LEFT    10
#define PSAB_L2          19
#define PSAB_R2          20
#define PSAB_L1          17
#define PSAB_R1          18
#define PSAB_GREEN       13
#define PSAB_RED         14
#define PSAB_BLUE        15
#define PSAB_PINK        16
#define PSAB_TRIANGLE    13
#define PSAB_CIRCLE      14
#define PSAB_CROSS       15
#define PSAB_SQUARE      16

// Timing goc Arduino-PS2X: CTRL_CLK=4us, BYTE_DELAY=3us. STM32 giu nguyen.
#ifndef PS2X_CLK_US
#define PS2X_CLK_US 4
#endif
#ifndef PS2X_BYTE_DELAY_US
#define PS2X_BYTE_DELAY_US 10  // nhích lên 10us cho on dinh tren HAL (goc 3us)
#endif

typedef struct {
  GPIO_TypeDef *clk_port; uint16_t clk_pin;
  GPIO_TypeDef *cmd_port; uint16_t cmd_pin;
  GPIO_TypeDef *att_port; uint16_t att_pin;
  GPIO_TypeDef *dat_port; uint16_t dat_pin;

  uint8_t  data[21];
  uint16_t last_buttons;
  uint16_t buttons;
  uint32_t last_read;      // HAL_GetTick
  uint8_t  read_delay;     // ms, tu tang 1..10 nhu ban goc
  uint8_t  controller_type;
  bool     en_rumble;
  bool     en_pressures;
} PS2X_Handle;

// Goi 1 lan sau MX_GPIO_Init. Chan CLK/CMD/ATT: Output PP, DAT: Input PullUp.
void    PS2X_Init(PS2X_Handle *h,
                  GPIO_TypeDef *clk_port, uint16_t clk_pin,
                  GPIO_TypeDef *cmd_port, uint16_t cmd_pin,
                  GPIO_TypeDef *att_port, uint16_t att_pin,
                  GPIO_TypeDef *dat_port, uint16_t dat_pin);

// Tra ve 0 = OK, 1 = khong thay tay cam, 2 = khong nhan lenh, 3 = khong vao pressures mode
uint8_t PS2X_ConfigGamepad(PS2X_Handle *h, bool pressures, bool rumble);

// Doc tay cam. motor1: rung nho on/off, motor2: 0-255 toc do rung lon.
// Tra ve true neu o analog mode (0x7x) = du lieu hop le.
bool    PS2X_ReadGamepad(PS2X_Handle *h, bool motor1, uint8_t motor2);

bool     PS2X_Button(PS2X_Handle *h, uint16_t button);
uint16_t PS2X_ButtonDataByte(PS2X_Handle *h);
bool     PS2X_NewButtonState(PS2X_Handle *h);
bool     PS2X_NewButtonStateOf(PS2X_Handle *h, uint16_t button);
bool     PS2X_ButtonPressed(PS2X_Handle *h, uint16_t button);
bool     PS2X_ButtonReleased(PS2X_Handle *h, uint16_t button);
uint8_t  PS2X_Analog(PS2X_Handle *h, uint8_t index);
uint8_t  PS2X_ReadType(PS2X_Handle *h);
void     PS2X_Reconfig(PS2X_Handle *h);
void     PS2X_EnableRumble(PS2X_Handle *h);
bool     PS2X_EnablePressures(PS2X_Handle *h);

// Tien ich: in trang thai ra UART (de "ban lai qua serial"); tra ve so ky tu.
int PS2X_DumpState(PS2X_Handle *h, UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif

#endif
