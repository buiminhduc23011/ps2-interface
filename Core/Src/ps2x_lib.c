// ps2x_lib.c — Port 1:1 tu Arduino-PS2X (PS2X_lib.cpp) sang STM32 HAL.
// Thay digitalWrite/digitalRead -> HAL_GPIO, delayMicroseconds -> DWT, millis -> HAL_GetTick.

#include "ps2x_lib.h"
#include <stdio.h>
#include <string.h>

// Lenh cau hinh goc (giua nguyen)
static const uint8_t CMD_ENTER_CONFIG[] = {0x01,0x43,0x00,0x01,0x00};
static const uint8_t CMD_SET_MODE[]     = {0x01,0x44,0x00,0x01,0x03,0x00,0x00,0x00,0x00};
static const uint8_t CMD_SET_LARGE[]    = {0x01,0x4F,0x00,0xFF,0xFF,0x03,0x00,0x00,0x00};
static const uint8_t CMD_EXIT_CONFIG[]  = {0x01,0x43,0x00,0x00,0x5A,0x5A,0x5A,0x5A,0x5A};
static const uint8_t CMD_ENABLE_RUMBLE[]= {0x01,0x4D,0x00,0x00,0x01};
static const uint8_t CMD_TYPE_READ[]    = {0x01,0x45,0x00,0x5A,0x5A,0x5A,0x5A,0x5A,0x5A};

static inline void delay_us(uint32_t us)
{
  // DWT CYCCNT — chay duoc tren Cortex-M3 (STM32F1)
  uint32_t start = DWT->CYCCNT;
  uint32_t ticks = us * (SystemCoreClock / 1000000UL);
  while ((DWT->CYCCNT - start) < ticks) { __NOP(); }
}

static inline void dwt_init(void)
{
  if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk))
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static inline void CLK_SET(PS2X_Handle *h) { HAL_GPIO_WritePin(h->clk_port, h->clk_pin, GPIO_PIN_SET); }
static inline void CLK_CLR(PS2X_Handle *h) { HAL_GPIO_WritePin(h->clk_port, h->clk_pin, GPIO_PIN_RESET); }
static inline void CMD_SET(PS2X_Handle *h) { HAL_GPIO_WritePin(h->cmd_port, h->cmd_pin, GPIO_PIN_SET); }
static inline void CMD_CLR(PS2X_Handle *h) { HAL_GPIO_WritePin(h->cmd_port, h->cmd_pin, GPIO_PIN_RESET); }
static inline void ATT_SET(PS2X_Handle *h) { HAL_GPIO_WritePin(h->att_port, h->att_pin, GPIO_PIN_SET); }
static inline void ATT_CLR(PS2X_Handle *h) { HAL_GPIO_WritePin(h->att_port, h->att_pin, GPIO_PIN_RESET); }
static inline bool DAT_CHK(PS2X_Handle *h)
{
  return HAL_GPIO_ReadPin(h->dat_port, h->dat_pin) == GPIO_PIN_SET;
}

static uint8_t shift_inout(PS2X_Handle *h, uint8_t out)
{
  uint8_t in = 0;
  for (uint8_t i = 0; i < 8; i++) {
    if (out & (1 << i)) CMD_SET(h); else CMD_CLR(h);
    CLK_CLR(h);
    delay_us(PS2X_CLK_US);
    if (DAT_CHK(h)) in |= (1 << i);
    CLK_SET(h);
    delay_us(PS2X_CLK_US);
  }
  CMD_SET(h);
  delay_us(PS2X_BYTE_DELAY_US);
  return in;
}

static void send_cmd(PS2X_Handle *h, const uint8_t *str, uint8_t len)
{
  ATT_CLR(h);
  delay_us(PS2X_BYTE_DELAY_US);
  for (uint8_t y = 0; y < len; y++)
    shift_inout(h, str[y]);
  ATT_SET(h);
  HAL_Delay(h->read_delay);
}

static uint8_t map_u8(uint8_t x, uint8_t in_min, uint8_t in_max, uint8_t out_min, uint8_t out_max)
{
  if (x <= in_min) return out_min;
  if (x >= in_max) return out_max;
  return (uint8_t)(((uint16_t)(x - in_min) * (out_max - out_min)) / (in_max - in_min) + out_min);
}

// ---------------- public ----------------

void PS2X_Init(PS2X_Handle *h,
               GPIO_TypeDef *clk_port, uint16_t clk_pin,
               GPIO_TypeDef *cmd_port, uint16_t cmd_pin,
               GPIO_TypeDef *att_port, uint16_t att_pin,
               GPIO_TypeDef *dat_port, uint16_t dat_pin)
{
  memset(h, 0, sizeof(*h));
  h->clk_port = clk_port; h->clk_pin = clk_pin;
  h->cmd_port = cmd_port; h->cmd_pin = cmd_pin;
  h->att_port = att_port; h->att_pin = att_pin;
  h->dat_port = dat_port; h->dat_pin = dat_pin;
  h->read_delay = 1;
  dwt_init();
  CMD_SET(h);
  CLK_SET(h);
  ATT_SET(h);
}

bool PS2X_ReadGamepad(PS2X_Handle *h, bool motor1, uint8_t motor2)
{
  uint32_t now = HAL_GetTick();
  uint32_t temp = now - h->last_read;

  if (temp > 1500) // lau qua khong poll -> cau hinh lai
    PS2X_Reconfig(h);

  if (temp < h->read_delay)
    HAL_Delay(h->read_delay - temp);

  if (motor2 != 0x00)
    motor2 = map_u8(motor2, 0, 255, 0x40, 0xFF); // duoi 0x40 motor khong quay

  uint8_t dword[9]  = {0x01,0x42,0x00,(uint8_t)motor1,motor2,0,0,0,0};
  uint8_t dword2[12] = {0,0,0,0,0,0,0,0,0,0,0,0};

  for (uint8_t retry = 0; retry < 5; retry++) {
    CMD_SET(h);
    CLK_SET(h);
    ATT_CLR(h); // chon tay cam
    delay_us(PS2X_BYTE_DELAY_US);

    for (int i = 0; i < 9; i++)
      h->data[i] = shift_inout(h, dword[i]);

    if (h->data[1] == 0x79) { // full data mode: doc them 12 byte pressures
      for (int i = 0; i < 12; i++)
        h->data[i + 9] = shift_inout(h, dword2[i]);
    }

    ATT_SET(h); // nha tay cam

    if ((h->data[1] & 0xF0) == 0x70) // analog mode 0x7x la hop le
      break;

    PS2X_Reconfig(h);
    HAL_Delay(h->read_delay);
  }

  if ((h->data[1] & 0xF0) != 0x70) {
    if (h->read_delay < 10)
      h->read_delay++; // tang delay nhu ban goc de cuu tuong thich
  }

  h->last_buttons = h->buttons;
  h->buttons = (uint16_t)(((uint16_t)h->data[4] << 8) | h->data[3]);
  h->last_read = HAL_GetTick();
  return ((h->data[1] & 0xF0) == 0x70);
}

uint8_t PS2X_ConfigGamepad(PS2X_Handle *h, bool pressures, bool rumble)
{
  uint8_t temp[9];

  CMD_SET(h);
  CLK_SET(h);

  PS2X_ReadGamepad(h, false, 0x00);
  PS2X_ReadGamepad(h, false, 0x00);

  // Phai tra ve 1 trong 0x41/0x42/0x73/0x79 neu co tay cam
  if (h->data[1] != 0x41 && h->data[1] != 0x42 &&
      h->data[1] != 0x73 && h->data[1] != 0x79)
    return 1;

  h->read_delay = 1;

  for (int y = 0; y <= 10; y++) {
    send_cmd(h, CMD_ENTER_CONFIG, sizeof(CMD_ENTER_CONFIG));

    delay_us(PS2X_BYTE_DELAY_US);
    CMD_SET(h);
    CLK_SET(h);
    ATT_CLR(h);
    delay_us(PS2X_BYTE_DELAY_US);

    for (int i = 0; i < 9; i++)
      temp[i] = shift_inout(h, CMD_TYPE_READ[i]);

    ATT_SET(h);

    h->controller_type = temp[3];

    send_cmd(h, CMD_SET_MODE, sizeof(CMD_SET_MODE));
    if (rumble) { send_cmd(h, CMD_ENABLE_RUMBLE, sizeof(CMD_ENABLE_RUMBLE)); h->en_rumble = true; }
    if (pressures) { send_cmd(h, CMD_SET_LARGE, sizeof(CMD_SET_LARGE)); h->en_pressures = true; }
    send_cmd(h, CMD_EXIT_CONFIG, sizeof(CMD_EXIT_CONFIG));

    PS2X_ReadGamepad(h, false, 0x00);

    if (pressures) {
      if (h->data[1] == 0x79) break;
      if (h->data[1] == 0x73) return 3;
    }
    if (h->data[1] == 0x73) break;

    if (y == 10) return 2;
    h->read_delay += 1;
  }
  return 0;
}

void PS2X_Reconfig(PS2X_Handle *h)
{
  send_cmd(h, CMD_ENTER_CONFIG, sizeof(CMD_ENTER_CONFIG));
  send_cmd(h, CMD_SET_MODE, sizeof(CMD_SET_MODE));
  if (h->en_rumble) send_cmd(h, CMD_ENABLE_RUMBLE, sizeof(CMD_ENABLE_RUMBLE));
  if (h->en_pressures) send_cmd(h, CMD_SET_LARGE, sizeof(CMD_SET_LARGE));
  send_cmd(h, CMD_EXIT_CONFIG, sizeof(CMD_EXIT_CONFIG));
}

void PS2X_EnableRumble(PS2X_Handle *h)
{
  send_cmd(h, CMD_ENTER_CONFIG, sizeof(CMD_ENTER_CONFIG));
  send_cmd(h, CMD_ENABLE_RUMBLE, sizeof(CMD_ENABLE_RUMBLE));
  send_cmd(h, CMD_EXIT_CONFIG, sizeof(CMD_EXIT_CONFIG));
  h->en_rumble = true;
}

bool PS2X_EnablePressures(PS2X_Handle *h)
{
  send_cmd(h, CMD_ENTER_CONFIG, sizeof(CMD_ENTER_CONFIG));
  send_cmd(h, CMD_SET_LARGE, sizeof(CMD_SET_LARGE));
  send_cmd(h, CMD_EXIT_CONFIG, sizeof(CMD_EXIT_CONFIG));
  PS2X_ReadGamepad(h, false, 0x00);
  PS2X_ReadGamepad(h, false, 0x00);
  if (h->data[1] != 0x79) return false;
  h->en_pressures = true;
  return true;
}

bool PS2X_Button(PS2X_Handle *h, uint16_t button)
{
  return ((~h->buttons & button) != 0);
}

uint16_t PS2X_ButtonDataByte(PS2X_Handle *h)
{
  return (uint16_t)(~h->buttons);
}

bool PS2X_NewButtonState(PS2X_Handle *h)
{
  return ((h->last_buttons ^ h->buttons) != 0);
}

bool PS2X_NewButtonStateOf(PS2X_Handle *h, uint16_t button)
{
  return (((h->last_buttons ^ h->buttons) & button) != 0);
}

bool PS2X_ButtonPressed(PS2X_Handle *h, uint16_t button)
{
  return (PS2X_NewButtonStateOf(h, button) && PS2X_Button(h, button));
}

bool PS2X_ButtonReleased(PS2X_Handle *h, uint16_t button)
{
  return (PS2X_NewButtonStateOf(h, button) && ((~h->last_buttons & button) != 0));
}

uint8_t PS2X_Analog(PS2X_Handle *h, uint8_t index)
{
  if (index >= sizeof(h->data)) return 0;
  return h->data[index];
}

uint8_t PS2X_ReadType(PS2X_Handle *h)
{
  if (h->controller_type == 0x03) return 1;                          // DualShock
  if (h->controller_type == 0x01 && h->data[1] == 0x42) return 4;    // (ban goc giu)
  if (h->controller_type == 0x01) return 2;                          // GuitarHero
  if (h->controller_type == 0x0C) return 3;                         // Wireless Sony
  return 0;
}

int PS2X_DumpState(PS2X_Handle *h, UART_HandleTypeDef *huart)
{
  // Dong gon de PC doc: BTN=hex LX=.. LY=.. RX=.. RY=.. | raw 9 byte
  char buf[160];
  int n = snprintf(buf, sizeof(buf),
    "BTN=%04X LX=%3u LY=%3u RX=%3u RY=%3u | %02X %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
    (unsigned)(~h->buttons & 0xFFFF),
    h->data[PSS_LX], h->data[PSS_LY], h->data[PSS_RX], h->data[PSS_RY],
    h->data[0], h->data[1], h->data[2], h->data[3], h->data[4],
    h->data[5], h->data[6], h->data[7], h->data[8]);
  if (n > 0) HAL_UART_Transmit(huart, (uint8_t *)buf, (uint16_t)n, 50);
  return n;
}
