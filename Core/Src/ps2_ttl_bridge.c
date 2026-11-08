// ps2_ttl_bridge.c
// STM32F1 + HAL. Doc module PS2-to-TTL qua UART, dump HEX+ASCII ra PC.
// Khong gia dinh format frame -> soi duoc bat ky module nao.

#include "ps2_ttl_bridge.h"
#include <stdio.h>
#include <string.h>

// Dinh nghia ham USB CDC neu ban bat USE_USB_CDC (CubeMX: USB_DEVICE Class CDC).
// Trong usbd_cdc_if.c co san: uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);
#ifdef USE_USB_CDC
extern uint8_t CDC_Transmit_FS(uint8_t *Buf, uint16_t Len);
#endif

static uint8_t  rx_byte;                        // byte IT tam
static uint8_t  ring[PS2_RX_BUF_SIZE];
static volatile uint16_t ring_head = 0;         // ISR ghi
static volatile uint16_t ring_tail = 0;         // main doc
static uint32_t last_rx_tick = 0;
static uint8_t  frame_buf[PS2_RX_BUF_SIZE];
static char     line_buf[128];

// printf() -> DEBUG_UART (USART1). Neu dung USB CDC thi printf van ra UART1,
// con du lieu bridge go ca 2 noi (xem DEBUG_Tx).
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif
PUTCHAR_PROTOTYPE
{
  uint8_t c = (uint8_t)ch;
  HAL_UART_Transmit(DEBUG_UART, &c, 1, 10);
  return ch;
}

static inline uint16_t ring_count(void)
{
  return (uint16_t)(ring_head - ring_tail);
}

static void ring_push(uint8_t b)
{
  uint16_t next = (uint16_t)(ring_head + 1);
  // ring dung % PS2_RX_BUF_SIZE nen size nao cung dung (512 mac dinh)
  ring[ring_head % PS2_RX_BUF_SIZE] = b;
  // tran bo nho: ghi de byte cu nhat
  if ((uint16_t)(next - ring_tail) > PS2_RX_BUF_SIZE) {
    ring_tail++;
  }
  ring_head = next;
}

static uint16_t ring_pop_all(uint8_t *dst, uint16_t max)
{
  uint16_t n = ring_count();
  if (n > max) n = max;
  for (uint16_t i = 0; i < n; i++) {
    dst[i] = ring[(ring_tail + i) % PS2_RX_BUF_SIZE];
  }
  ring_tail += n;
  return n;
}

void PS2_Bridge_Init(void)
{
  ring_head = ring_tail = 0;
  last_rx_tick = HAL_GetTick();
  memset(ring, 0, sizeof(ring));
  // Bat nhan ngat tung byte dau tien. Cac byte sau restart trong callback.
  HAL_UART_Receive_IT(PS2_UART, &rx_byte, 1);
  printf("\r\n=== STM32F1 PS2-TTL bridge ready ===\r\n");
  printf("PS2_UART baud=%lu. Go phim tay cam, xem HEX do ve.\r\n",
         (unsigned long)PS2_UART->Init.BaudRate);
  printf("Neu ra rac / khong ra: doi baud USART2 9600->19200->38400->57600->115200\r\n");
}

void PS2_Bridge_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart == PS2_UART) {
    ring_push(rx_byte);
    last_rx_tick = HAL_GetTick();
    HAL_UART_Receive_IT(PS2_UART, &rx_byte, 1);
  }
}

void PS2_Bridge_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart == PS2_UART) {
    // Loi frame/overrun rat hay gap khi sai baud -> clear flag roi nhan tiep
    __HAL_UART_CLEAR_OREFLAG(huart);
    __HAL_UART_CLEAR_FEFLAG(huart);
    __HAL_UART_CLEAR_NEFLAG(huart);
    HAL_UART_Receive_IT(PS2_UART, &rx_byte, 1);
  }
}

HAL_StatusTypeDef PS2_Bridge_SetBaud(uint32_t baud)
{
  HAL_UART_AbortReceive(PS2_UART);
  PS2_UART->Init.BaudRate = baud;
  // F1: phai DeInit/Init lai de tinh lai BRR
  if (HAL_UART_DeInit(PS2_UART) != HAL_OK) return HAL_ERROR;
  if (HAL_UART_Init(PS2_UART) != HAL_OK) return HAL_ERROR;
  ring_head = ring_tail = 0;
  return HAL_UART_Receive_IT(PS2_UART, &rx_byte, 1);
}

void DEBUG_Tx(const uint8_t *data, uint16_t len)
{
  if (len == 0) return;
#ifdef USE_USB_CDC
  CDC_Transmit_FS((uint8_t *)data, len); // ra thang Type-C native USB
#endif
  // Luon gui ca ra USART1 de soi bang USB-TTL ngoai (on dinh hon)
  HAL_UART_Transmit(DEBUG_UART, (uint8_t *)data, len, 100);
}

// Mac dinh: in HEX + ASCII. Ghi de ham nay o main.c neu muon decode nut/joystick.
__weak void PS2_OnFrame(const uint8_t *data, uint16_t len)
{
  // VD output: [12] AA 55 00 01 7F 7F ...  |ascii|
  int n = snprintf(line_buf, sizeof(line_buf), "[%u] ", (unsigned)len);
  DEBUG_Tx((uint8_t *)line_buf, (uint16_t)n);

  for (uint16_t i = 0; i < len; i++) {
    n = snprintf(line_buf, sizeof(line_buf), "%02X ", data[i]);
    DEBUG_Tx((uint8_t *)line_buf, (uint16_t)n);
  }
  n = snprintf(line_buf, sizeof(line_buf), " |");
  DEBUG_Tx((uint8_t *)line_buf, (uint16_t)n);
  for (uint16_t i = 0; i < len; i++) {
    char c = (data[i] >= 32 && data[i] < 127) ? (char)data[i] : '.';
    DEBUG_Tx((uint8_t *)&c, 1);
  }
  const char tail[] = "|\r\n";
  DEBUG_Tx((const uint8_t *)tail, sizeof(tail) - 1);
}

void PS2_Bridge_Task(void)
{
  // Gom frame theo timeout: co du lieu + im lang >10ms => cat 1 frame
  if (ring_count() == 0) return;
  if ((HAL_GetTick() - last_rx_tick) < 10) return;

  __disable_irq();
  uint16_t len = ring_pop_all(frame_buf, sizeof(frame_buf));
  __enable_irq();

  if (len) PS2_OnFrame(frame_buf, len);
}
