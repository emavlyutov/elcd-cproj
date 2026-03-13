MICROBLAZE_TOOLCHAIN_PATH:=C:/Xilinx/SDK/2018.2/gnu/microblaze/nt/

# Toolchain PATH
GCC_PATH:=$(MICROBLAZE_TOOLCHAIN_PATH)microblaze-xilinx-elf/bin/
GCC_LIBS_PATH:=$(MICROBLAZE_TOOLCHAIN_PATH)lib/gcc/microblaze-xilinx-elf/7.2.0/

# Target compiler configuration	
GCC_PREFIX=mb
GCC_TARGET_FLAGS= -mlittle-endian -mcpu=v10.0 -mxl-soft-mul -Wl,--no-relax

# Target specific compiler flags (MicroBlaze)
GCC_INCFLAGS = \
		-I"$(GCC_PATH)include" \
		-I"$(GCC_LIBS_PATH)include" \
		-I"$(GCC_LIBS_PATH)include-fixed"

GCC_DEFFLAGS=
GCC_LIBFLAGS=
