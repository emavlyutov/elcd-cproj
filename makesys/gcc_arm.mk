# Toolchain PATH
TOOLCHAIN_PATH=$(XILINX_PATH)aarch32/nt/gcc-arm-none-eabi/
TOOLCHAIN_GCC_PATH=$(TOOLCHAIN_PATH)arm-none-eabi/
TOOLCHAIN_LIB_PATH=$(TOOLCHAIN_PATH)lib/gcc/arm-none-eabi/7.2.1/

# Target compiler configuration
TOOLCHAIN=bin/arm-none-eabi
GCC_TARGET_FLAGS=-mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -Wl,-build-id=none -specs=$(BSP_BASE_PATH)Xilinx.spec -std=c11

# Target specific compiler flags (ARM: ps7_cortexa9_0 or ps7_cortexa9_1)
TARGET_BSP_PATH:=$(BSP_BASE_PATH)bsp_$(TARGET_PROJECT)/ps7_cortexa9_$(TARGET_CORE:arm%=%)/
GCC_LIBFLAGS:=\
	-L"$(TARGET_BSP_PATH)lib"

GCC_INCFLAGS= \
		-I"$(TOOLCHAIN_GCC_PATH)include" \
		-I"$(TOOLCHAIN_GCC_PATH)libc/usr/include" \
		-I"$(TOOLCHAIN_LIB_PATH)include" \
		-I"$(TOOLCHAIN_LIB_PATH)include-fixed" \
		-I"$(TARGET_BSP_PATH)include"

GCC_DEFFLAGS=