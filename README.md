# STM32F1 Ä‘á»c tay PS2 â†’ báº¯n Serial ra PC

Port thÆ° viá»‡n **Arduino-PS2X (Bill Porter v1.8, madsci1016)** sang **STM32F1 BluePill (HAL + CubeMX)**.
Äá»c trá»±c tiáº¿p receiver PS2 qua 4 dÃ¢y `DAT / CMD / CLK / ATT`, rá»“i báº¯n tráº¡ng thÃ¡i
nÃºt + joystick qua `USART1` ra PC Ä‘á»ƒ Ä‘iá»u khiá»ƒn tiáº¿p.

Gá»‘c Arduino: https://github.com/madsci1016/Arduino-PS2X

## 1. Pháº§n cá»©ng

| Receiver PS2 | BluePill STM32F103 |
|---|---|
| VCC | 3V3 (Æ°u tiÃªn â€” an toÃ n cho STM32; chá»‰ lÃªn 5V náº¿u sÃ³ng yáº¿u) |
| GND | GND (báº¯t buá»™c chung mass) |
| DAT | PB15 (Input PullUp) |
| CMD | PB14 (Output PP, High speed, idle HIGH) |
| ATT (SEL) | PB12 (Output PP, High speed, idle HIGH) |
| CLK | PB13 (Output PP, High speed, idle HIGH) |

Serial ra PC:

| BluePill | Máº¡ch USB-TTL (CH340/CP2102) |
|---|---|
| PA9 (TX USART1) | RX |
| GND | GND |

Má»Ÿ terminal **115200 8N1** trÃªn COM cá»§a máº¡ch USB-TTL.

> Type-C trÃªn BluePill Ä‘a sá»‘ lÃ  USB native (PA11/PA12) â€” cáº¯m vÃ o PC sáº½ **khÃ´ng hiá»‡n COM**
> trá»« khi báº­t `USB_DEVICE CDC`. DÃ¹ng USART1 + USB-TTL ngoÃ i lÃ  Ä‘Æ°á»ng dá»… nháº¥t.

## 2. CubeMX

1. `USART1` â†’ Asynchronous, **115200 8N1**.
2. `PB12 / PB13 / PB14` â†’ `GPIO_Output`, Push-Pull, No pull, Speed High, initial SET.
   `PB15` â†’ `GPIO_Input`, PullUp.
3. Generate code, chÃ©p vÃ o project:
   - `Core/Inc/ps2x_lib.h`
   - `Core/Src/ps2x_lib.c`
4. Má»Ÿ `Examples/main_ps2x_example.c`, chÃ©p `PS2X_Handle`, `__io_putchar`, `main()`
   vÃ o `main.c`. Äá»•i chÃ¢n trong `#define PS2_*` náº¿u báº¡n Ä‘i dÃ¢y khÃ¡c.

## 3. DÃ¹ng (API giá»¯ nguyÃªn nhÆ° Arduino)

```c
PS2X_Handle ps2;
PS2X_Init(&ps2, GPIOB,GPIO_PIN_13, GPIOB,GPIO_PIN_14, GPIOB,GPIO_PIN_12, GPIOB,GPIO_PIN_15);
uint8_t err = PS2X_ConfigGamepad(&ps2, false, false); // pressures, rumble
// err: 0=OK, 1=ko tháº¥y tay, 2=ko nháº­n lá»‡nh, 3=ko vÃ o pressures mode

while (1) {
  PS2X_ReadGamepad(&ps2, false, 0);
  PS2X_DumpState(&ps2, &huart1); // "BTN=.. LX=.. ..." -> PC qua serial
  if (PS2X_Button(&ps2, PSB_START)) { ... }
  if (PS2X_ButtonPressed(&ps2, PSB_CIRCLE)) { ... }
  if (PS2X_ButtonReleased(&ps2, PSB_SQUARE)) { ... }
  uint8_t lx = PS2X_Analog(&ps2, PSS_LX);
  HAL_Delay(50);
}
```

HÃ m nÃ o Arduino cÃ³ thÃ¬ STM32 cÃ³ tÆ°Æ¡ng Ä‘Æ°Æ¡ng:
`Button / ButtonPressed / ButtonReleased / NewButtonState / Analog / ReadType / EnableRumble / EnablePressures / Reconfig`.
Xem `main_ps2x_example.c` â€” Ä‘Ã£ dá»‹ch `PS2X_Example.ino` sang HAL.

## 4. Lá»—i hay gáº·p

| Hiá»‡n tÆ°á»£ng | Xá»­ lÃ½ |
|---|---|
| `err=1` (No controller) | Sai chÃ¢n DAT/CMD/CLK/ATT, chÆ°a chung GND, chÆ°a chá» 300ms sau power-on, receiver chÆ°a pair vá»›i tay (nháº¥n nÃºt pair), thá»­ VCC 5V |
| `err=2` (not accepting) | Delay quÃ¡ ngáº¯n â†’ lib tá»± tÄƒng `read_delay`; kiá»ƒm tra CLK/CMD tiáº¿p xÃºc, dÃ¢y dÃ i quÃ¡ |
| Äá»c cháº­p chá»n | Nguá»“n yáº¿u (tay + receiver Äƒn dÃ²ng) â†’ nguá»“n 5V/3.3V rá»i, chung GND; dÃ¢y < 20cm |
| Timing | `PS2X_CLK_US=4`, `PS2X_BYTE_DELAY_US=10` (gá»‘c Arduino 4/3; STM32 ná»›i byte-delay cho á»•n). Sá»­a trong `ps2x_lib.h` náº¿u cáº§n |

## 5. Cáº¥u trÃºc repo

```
Core/Inc/ps2x_lib.h        â€” API (port 1:1 Arduino-PS2X)
Core/Src/ps2x_lib.c        â€” bit-bang PS2 qua HAL_GPIO + DWT delay_us (Ä‘Ã£ test logic Button/Analog khá»›p báº£n gá»‘c)
Examples/main_ps2x_example.c â€” main() máº«u + printf qua USART1 (dá»‹ch tá»« PS2X_Example.ino)
Core/Inc/ps2_ttl_bridge.h / Examples/ps2_ttl_bridge.c / Examples/main_example.c
  â€” phÆ°Æ¡ng Ã¡n cÅ© cho module PS2-to-UART TTL (khÃ´ng dÃ¹ng khi Ä‘Ã£ Ä‘á»c trá»±c tiáº¿p DAT/CMD/CLK/ATT)
```

## 6. ÄÃ£ verify

- `ps2x_lib.c` compile sáº¡ch `-std=gnu99` (stub HAL trÃªn host).
- Test host: `Button / ButtonPressed / ButtonReleased / Analog` PASS, khá»›p ngá»¯ nghÄ©a Arduino
  (nÃºt active-low, `~buttons & mask`).
- ChÆ°a test trÃªn board tháº­t (khÃ´ng cÃ³ toolchain ARM á»Ÿ mÃ¡y nÃ y) â€” náº¡p vÃ o BluePill,
  má»Ÿ terminal 115200, báº¥m nÃºt xem dÃ²ng `BTN=...` lÃ  Ä‘áº¡t.
