/* main.c — BluePill STM32F103C8 đọc tay PS2 trực tiếp, bắn serial ra PC.
 * Dây (sửa trong #define nếu đi dây khác):
 *   PS2 VCC -> 3V3 (thử 5V nếu sóng yếu), GND -> GND
 *   DAT -> PB15 (Input PullUp), CMD -> PB14, ATT -> PB12, CLK -> PB13 (Output PP, idle HIGH)
 *   Serial PC: PA9 TX USART1 115200 -> RX USB-TTL -> PC, GND chung.
 */
#include "main.h"
#include "ps2x_lib.h"
#include <stdio.h>

/* Chọn chân tại đây */
#define PS2_CLK_PORT GPIOB
#define PS2_CLK_PIN  GPIO_PIN_13
#define PS2_CMD_PORT GPIOB
#define PS2_CMD_PIN  GPIO_PIN_14
#define PS2_ATT_PORT GPIOB
#define PS2_ATT_PIN  GPIO_PIN_12
#define PS2_DAT_PORT GPIOB
#define PS2_DAT_PIN  GPIO_PIN_15

#define PS2_PRESSURES false
#define PS2_RUMBLE    false

UART_HandleTypeDef huart1;
PS2X_Handle ps2;

/* printf() -> USART1 */
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

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /* HSE 8MHz -> PLL x9 = 72MHz */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;  /* PCLK1 max 36MHz */
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) Error_Handler();
}

void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK) Error_Handler();
}

void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_AFIO_CLK_ENABLE();

  /* PC13 LED onboard (đảo mức sáng) */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

  /* PS2: PB12 ATT, PB13 CLK, PB14 CMD — Output PP High, idle HIGH */
  GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14, GPIO_PIN_SET);

  /* PS2 DAT PB15 — Input PullUp */
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) {
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    HAL_Delay(200);
  }
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART1_UART_Init();

  HAL_Delay(300); /* chờ module wireless ổn định */

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
  uint32_t tick = 0;

  while (1)
  {
    if (err != 0) {           /* không có tay cầm thì chờ + nháy LED */
      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
      HAL_Delay(500);
      continue;
    }

    PS2X_ReadGamepad(&ps2, false, vibrate);

    PS2X_DumpState(&ps2, &huart1);

    if (PS2X_Button(&ps2, PSB_START))  printf("Start is being held\r\n");
    if (PS2X_Button(&ps2, PSB_SELECT)) printf("Select is being held\r\n");
    if (PS2X_ButtonPressed(&ps2, PSB_CIRCLE))   printf("Circle just pressed\r\n");
    if (PS2X_ButtonReleased(&ps2, PSB_SQUARE))  printf("Square just released\r\n");
    if (PS2X_NewButtonStateOf(&ps2, PSB_CROSS)) printf("X just changed\r\n");
    if (PS2X_Button(&ps2, PSB_L1) || PS2X_Button(&ps2, PSB_R1)) {
      printf("Stick: LY=%u LX=%u RY=%u RX=%u\r\n",
             PS2X_Analog(&ps2, PSS_LY), PS2X_Analog(&ps2, PSS_LX),
             PS2X_Analog(&ps2, PSS_RY), PS2X_Analog(&ps2, PSS_RX));
    }
    vibrate = PS2X_Analog(&ps2, PSAB_CROSS);

    if (++tick % 10 == 0) HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    HAL_Delay(50);
  }
}
