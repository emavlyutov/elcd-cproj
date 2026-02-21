# Toolchain PATH
TOOLCHAIN_PATH:=$(XILINX_PATH)microblaze/nt/
TOOLCHAIN_GCC_PATH:=$(TOOLCHAIN_PATH)microblaze-xilinx-elf/
TOOLCHAIN_LIB_PATH:=$(TOOLCHAIN_PATH)lib/gcc/microblaze-xilinx-elf/7.2.0/

# Target compiler configuration	
TOOLCHAIN=bin/mb
GCC_TARGET_FLAGS= -mlittle-endian -mcpu=v10.0 -mxl-soft-mul -Wl,--no-relax -std=c11

# Target specific compiler flags (MicroBlaze)
TARGET_BSP_PATH:=$(BSP_BASE_PATH)bsp_$(TARGET_PROJECT)/mb_cpu_mb_subsystem_microblaze_inst/
GCC_LIBFLAGS:=\
	-L"$(TARGET_BSP_PATH)lib"
GCC_INCFLAGS = \
		-I"$(TOOLCHAIN_GCC_PATH)include" \
		-I"$(TOOLCHAIN_LIB_PATH)include" \
		-I"$(TOOLCHAIN_LIB_PATH)include-fixed" \
		-I"$(TARGET_BSP_PATH)include"
GCC_DEFFLAGS=
