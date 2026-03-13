XILINX_TOOLCHAIN_PATH=C:/Xilinx/SDK/2018.2/gnu/aarch32/nt/gcc-arm-none-eabi/

ifeq ($(GCC_SPEC_PATH),)
$(error GCC_SPEC_PATH is not set)
endif

# Toolchain PATH
GCC_PATH=$(XILINX_TOOLCHAIN_PATH)arm-none-eabi/bin/
GCC_LIBS_PATH=$(XILINX_TOOLCHAIN_PATHTOOLCHAIN_PATH)lib/gcc/arm-none-eabi/7.2.1/

# Target compiler configuration
GCC_PREFIX=arm-none-eabi
GCC_TARGET_FLAGS=-mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -Wl,-build-id=none -specs=$(GCC_SPEC_PATH)

GCC_INCFLAGS= \
		-I"$(GCC_PATH)include" \
		-I"$(GCC_PATH)libc/usr/include" \
		-I"$(GCC_LIBS_PATH)include" \
		-I"$(GCC_LIBS_PATH)include-fixed"

GCC_DEFFLAGS=
GCC_LIBFLAGS=
