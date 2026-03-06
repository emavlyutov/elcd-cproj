#==============================================================================
# toolchain.mk - Toolchain selection (ARM vs MicroBlaze)
#==============================================================================
# Includes gcc_arm.mk or gcc_mb.mk based on TARGET_CORE. Sets GCC_CC, GCC_LD,
# GCC_SIZE with platform-specific flags (e.g. cortex-a9 for Zynq, v10.0 for MicroBlaze).
#==============================================================================

ifeq ($(TARGET_TOOLCHAIN),)
$(error TARGET_TOOLCHAIN is not set)
endif

-include $(TARGET_TOOLCHAIN)

# Common GCC flags
GCC_WFLAGS= \
        -std=gnu11 \
        -Werror \
        -pedantic-errors \
        -Wall \
        -Wextra \
        -Wpedantic -std=c11

ifdef (__DEBUG_EN)
    GCC_CFLAGS=-O0 -g3
    GCC_LFLAGS=
else
    GCC_CFLAGS=-ffunction-sections -fdata-sections -O2
    GCC_LFLAGS=-Wl,--gc-sections
endif

# GCC flags concat
GCC_FLAGS=$(GCC_TARGET_FLAGS) $(GCC_WFLAGS)
GCC_CC_FLAGS=$(GCC_CFLAGS) $(GCC_INCFLAGS) $(GCC_DEFFLAGS) -c
GCC_LD_FLAGS=$(GCC_LFLAGS) $(GCC_LIBFLAGS)

# GCC toolchain CLI call commands
GCC_LD=$(GCC_PATH)$(GCC_PREFIX)-gcc $(GCC_FLAGS) $(GCC_LD_FLAGS)
GCC_CC=$(GCC_PATH)$(GCC_PREFIX)-gcc $(GCC_FLAGS) $(GCC_CC_FLAGS)
GCC_AR=$(GCC_PATH)$(GCC_PREFIX)-ar
GCC_SIZE=$(GCC_PATH)$(GCC_PREFIX)-size
