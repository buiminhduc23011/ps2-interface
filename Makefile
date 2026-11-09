# Makefile — ps2-interface / BluePill STM32F103C8
# Dùng toolchain đi kèm CubeIDE, không cần cài thêm.
# Build:  make          (hoặc bấm búa Build trong CubeIDE)
# Clean:  make clean
# Nạp:    make flash    (cần STM32_Programmer_CLI trong PATH, hoặc nạp file Debug/*.elf bằng CubeProgrammer)

TARGET = ps2-interface
BUILD_DIR = Debug

# Toolchain CubeIDE 2.2.0 (sửa nếu bạn cài chỗ khác)
CUBE_TOOLS ?= C:/ST/STM32CubeIDE_2.2.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740/tools/bin
PREFIX = arm-none-eabi-
CC = $(CUBE_TOOLS)/$(PREFIX)gcc
AS = $(CUBE_TOOLS)/$(PREFIX)gcc -x assembler-with-cpp
CP = $(CUBE_TOOLS)/$(PREFIX)objcopy
SZ = $(CUBE_TOOLS)/$(PREFIX)size
RM = del /Q /F

C_DEFS = -DSTM32F103xB -DUSE_HAL_DRIVER
C_INCLUDES = -ICore/Inc \
  -IDrivers/CMSIS/Include \
  -IDrivers/CMSIS/Device/ST/STM32F1xx/Include \
  -IDrivers/STM32F1xx_HAL_Driver/Inc \
  -IDrivers/STM32F1xx_HAL_Driver/Inc/Legacy

C_FLAGS = -mcpu=cortex-m3 -mthumb $(C_DEFS) $(C_INCLUDES) \
  -O2 -g -Wall -ffunction-sections -fdata-sections \
  -MMD -MP -MF"$(@:%.o=%.d)"

AS_FLAGS = -mcpu=cortex-m3 -mthumb -g $(C_DEFS) $(C_INCLUDES) \
  -ffunction-sections -fdata-sections

LDSCRIPT = STM32F103C8TX_FLASH.ld
LD_FLAGS = -mcpu=cortex-m3 -mthumb -T$(LDSCRIPT) \
  -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref \
  --specs=nano.specs -static -Wl,--start-group -lc -lm -Wl,--end-group \
  -Wl,--print-memory-usage

# Nguồn build thật. Hai file main_*_example.c là tài liệu tham khảo, KHÔNG build
# (chúng cũng định nghĩa main() nên build chung sẽ lỗi trùng main).
C_SOURCES = Core/Src/main.c \
  Core/Src/ps2x_lib.c \
  Core/Src/stm32f1xx_it.c \
  Core/Src/stm32f1xx_hal_msp.c \
  Core/Src/system_stm32f1xx.c \
  Core/Src/syscalls.c \
  Core/Src/sysmem.c \
  Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c \
  Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_cortex.c \
  Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_dma.c \
  Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_exti.c \
  Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash.c \
  Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash_ex.c \
  Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c \
  Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio_ex.c \
  Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pwr.c \
  Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc.c \
  Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc_ex.c \
  Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_uart.c

ASM_SOURCES = Core/Startup/startup_stm32f103xb.s

OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin
	@$(SZ) $(BUILD_DIR)/$(TARGET).elf

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	@$(CC) -c $(C_FLAGS) $< -o $@
	@echo CC $<

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	@$(AS) -c $(AS_FLAGS) $< -o $@
	@echo AS $<

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile $(LDSCRIPT)
	@$(CC) $(OBJECTS) $(LD_FLAGS) -o $@
	@echo LD $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	@$(CP) -O ihex $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	@$(CP) -O binary -S $< $@

$(BUILD_DIR):
	if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"

clean:
	if exist "$(BUILD_DIR)" rmdir /S /Q "$(BUILD_DIR)"

flash: $(BUILD_DIR)/$(TARGET).elf
	STM32_Programmer_CLI -c port=SWD -w $(BUILD_DIR)/$(TARGET).elf -v -rst

-include $(wildcard $(BUILD_DIR)/*.d)
