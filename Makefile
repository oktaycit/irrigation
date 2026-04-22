# STM32F407VET6 Makefile for Irrigation Project
# CMSIS-DAP (DAPLink) ile programlama desteği

# Target mikrodenetleyici
TARGET = irrigation
MCU = STM32F407VETx

# Cortex-M4 core ayarları
CPU = -mcpu=cortex-m4
FPU = -mfpu=fpv4-sp-d16
FLOAT-ABI = -mfloat-abi=hard
MCU_FLAGS = $(CPU) $(FPU) $(FLOAT-ABI)

# Build ayarları
BUILD_DIR = Debug
CFLAGS = $(MCU_FLAGS) -Wall -fdata-sections -ffunction-sections
CFLAGS += -g3 -gdwarf-2
CFLAGS += -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx
CFLAGS += --specs=nosys.specs

# Linker ayarları
LDSCRIPT = STM32F407VETX_FLASH.ld
LDFLAGS = $(CPU) $(FPU) $(FLOAT-ABI) -T$(LDSCRIPT)
LDFLAGS += -Wl,--gc-sections -Wl,--print-memory-usage
LDFLAGS += -specs=nano.specs -lc -lm -lnosys

# Compiler ve araçlar (STM32CubeIDE dahili toolchain)
TOOLCHAIN_PATH = /Applications/STM32CubeIDE.app/Contents/Eclipse/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.macosaarch64_1.0.0.202602081740/tools/bin/
PREFIX = $(TOOLCHAIN_PATH)arm-none-eabi-
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
RM = rm -rf

# Debug için OpenOCD ayarları
OPENOCD = openocd
OPENOCD_INTERFACE = interface/cmsis-dap.cfg
OPENOCD_TARGET = target/stm32f4x.cfg
OPENOCD_SCRIPTS = -c "program $(BUILD_DIR)/$(TARGET).elf verify reset exit"

# Include yolları
C_INCLUDES = -ICore/Inc
C_INCLUDES += -IDrivers/STM32F4xx_HAL_Driver/Inc
C_INCLUDES += -IDrivers/STM32F4xx_HAL_Driver/Inc/Legacy
C_INCLUDES += -IDrivers/CMSIS/Device/ST/STM32F4xx/Include
C_INCLUDES += -IDrivers/CMSIS/Include
C_INCLUDES += -IMiddlewares/ST/STM32_USB_Device_Library/Core/Inc
C_INCLUDES += -IMiddlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc

# Kaynak dosyalar
C_SOURCES = \
Core/Src/main.c \
Core/Src/system_config.c \
Core/Src/stm32f4xx_it.c \
Core/Src/stm32f4xx_hal_msp.c \
Core/Src/system_stm32f4xx.c \
Core/Src/lcd_ili9341.c \
Core/Src/touch_xpt2046.c \
Core/Src/fonts.c \
Core/Src/sensors.c \
Core/Src/valves.c \
Core/Src/gui.c \
Core/Src/eeprom.c \
Core/Src/irrigation_control.c \
Core/Src/usb_device.c \
Core/Src/usb_config.c \
Core/Src/usbd_conf.c \
Core/Src/usbd_desc.c \
Core/Src/usbd_cdc_interface.c \
Core/Src/buzzer.c \
Core/Src/hw_crc_stub.c \
Core/Src/rtc_driver_stub.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_nor.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd_ex.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_fsmc.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_usb.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc.c \
Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc_ex.c \
Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.c \
Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c \
Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c \
Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c

# Startup dosyası
ASM_SOURCES = Core/Startup/startup_stm32f407vetx.s

# Object dosyalarını oluştur
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

# Hedefler
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

# Build dizinini oluştur
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Link
$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) | $(BUILD_DIR)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

# Hex ve Binary oluştur
$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(CP) -O ihex $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(CP) -O binary -S $< $@

# C dosyalarını derle
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(C_INCLUDES) -c $< -o $@

# Assembly dosyalarını derle
$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	$(AS) $(CFLAGS) $(C_INCLUDES) -c $< -o $@

# Temizle
clean:
	$(RM) $(BUILD_DIR)/*

# CMSIS-DAP (DAPLink) ile programla
flash: $(BUILD_DIR)/$(TARGET).elf
	$(OPENOCD) -f $(OPENOCD_INTERFACE) -f $(OPENOCD_TARGET) $(OPENOCD_SCRIPTS)

# Debug bağlantısını test et
debug-test:
	$(OPENOCD) -f $(OPENOCD_INTERFACE) -f $(OPENOCD_TARGET) -c "init; reset; halt; info target; exit"

# GDB ile debug başlat
debug-gdb: $(BUILD_DIR)/$(TARGET).elf
	$(OPENOCD) -f $(OPENOCD_INTERFACE) -f $(OPENOCD_TARGET) &
	sleep 2
	$(PREFIX)gdb $(BUILD_DIR)/$(TARGET).elf -ex "target remote localhost:3333" -ex "monitor reset halt"

.PHONY: all clean flash debug-test debug-gdb
