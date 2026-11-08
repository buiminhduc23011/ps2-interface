# STM32F1 đọc tay PS2 → bắn Serial ra PC

Port thư viện **Arduino-PS2X (Bill Porter v1.8, madsci1016)** sang **STM32F1 BluePill (HAL + CubeMX)**.
Đọc trực tiếp receiver PS2 qua 4 dây `DAT / CMD / CLK / ATT`, rồi bắn trạng thái
nút + joystick qua `USART1` ra PC để điều khiển tiếp.

Gốc Arduino: https://github.com/madsci1016/Arduino-PS2X

## 1. Phần cứng

| Receiver PS2 | BluePill STM32F103 |
|---|---|
| VCC | 3V3 (ưu tiên — an toàn cho STM32; chỉ lên 5V nếu sóng yếu) |
| GND | GND (bắt buộc chung mass) |
| DAT | PB15 (Input PullUp) |
| CMD | PB14 (Output PP, High speed, idle HIGH) |
| ATT (SEL) | PB12 (Output PP, High speed, idle HIGH) |
| CLK | PB13 (Output PP, High speed, idle HIGH) |

Serial ra PC:

| BluePill | Mạch USB-TTL (CH340/CP2102) |
|---|---|
| PA9 (TX USART1) | RX |
| GND | GND |

Mở terminal **115200 8N1** trên COM của mạch USB-TTL.

> Type-C trên BluePill đa số là USB native (PA11/PA12) — cắm vào PC sẽ **không hiện COM**
> trừ khi bật `USB_DEVICE CDC`. Dùng USART1 + USB-TTL ngoài là đường dễ nhất.

## 2. CubeMX

1. `USART1` → Asynchronous, **115200 8N1**.
2. `PB12 / PB13 / PB14` → `GPIO_Output`, Push-Pull, No pull, Speed High, initial SET.
   `PB15` → `GPIO_Input`, PullUp.
3. Generate code, chép vào project:
   - `Core/Inc/ps2x_lib.h`
   - `Core/Src/ps2x_lib.c`
4. Mở `Core/Src/main_ps2x_example.c`, chép `PS2X_Handle`, `__io_putchar`, `main()`
   vào `main.c`. Đổi chân trong `#define PS2_*` nếu bạn đi dây khác.

## 3. Dùng (API giữ nguyên như Arduino)

```c
PS2X_Handle ps2;
PS2X_Init(&ps2, GPIOB,GPIO_PIN_13, GPIOB,GPIO_PIN_14, GPIOB,GPIO_PIN_12, GPIOB,GPIO_PIN_15);
uint8_t err = PS2X_ConfigGamepad(&ps2, false, false); // pressures, rumble
// err: 0=OK, 1=ko thấy tay, 2=ko nhận lệnh, 3=ko vào pressures mode

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

Hàm nào Arduino có thì STM32 có tương đương:
`Button / ButtonPressed / ButtonReleased / NewButtonState / Analog / ReadType / EnableRumble / EnablePressures / Reconfig`.
Xem `main_ps2x_example.c` — đã dịch `PS2X_Example.ino` sang HAL.

## 4. Lỗi hay gặp

| Hiện tượng | Xử lý |
|---|---|
| `err=1` (No controller) | Sai chân DAT/CMD/CLK/ATT, chưa chung GND, chưa chờ 300ms sau power-on, receiver chưa pair với tay (nhấn nút pair), thử VCC 5V |
| `err=2` (not accepting) | Delay quá ngắn → lib tự tăng `read_delay`; kiểm tra CLK/CMD tiếp xúc, dây dài quá |
| Đọc chập chờn | Nguồn yếu (tay + receiver ăn dòng) → nguồn 5V/3.3V rời, chung GND; dây < 20cm |
| Timing | `PS2X_CLK_US=4`, `PS2X_BYTE_DELAY_US=10` (gốc Arduino 4/3; STM32 nới byte-delay cho ổn). Sửa trong `ps2x_lib.h` nếu cần |

## 5. Cấu trúc repo

```
Core/Inc/ps2x_lib.h        — API (port 1:1 Arduino-PS2X)
Core/Src/ps2x_lib.c        — bit-bang PS2 qua HAL_GPIO + DWT delay_us (đã test logic Button/Analog khớp bản gốc)
Core/Src/main_ps2x_example.c — main() mẫu + printf qua USART1 (dịch từ PS2X_Example.ino)
Core/Inc/ps2_ttl_bridge.h / Core/Src/ps2_ttl_bridge.c / Core/Src/main_example.c
  — phương án cũ cho module PS2-to-UART TTL (không dùng khi đã đọc trực tiếp DAT/CMD/CLK/ATT)
```

## 6. Đã verify

- `ps2x_lib.c` compile sạch `-std=gnu99` (stub HAL trên host).
- Test host: `Button / ButtonPressed / ButtonReleased / Analog` PASS, khớp ngữ nghĩa Arduino
  (nút active-low, `~buttons & mask`).
- Chưa test trên board thật (không có toolchain ARM ở máy này) — nạp vào BluePill,
  mở terminal 115200, bấm nút xem dòng `BTN=...` là đạt.
