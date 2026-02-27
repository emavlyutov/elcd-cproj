#==============================================================================
# toolchain.mk - Toolchain selection (ARM vs MicroBlaze)
#==============================================================================
# Includes gcc_arm.mk or gcc_mb.mk based on TARGET_CORE. Sets GCC_CC, GCC_LD,
# GCC_SIZE with platform-specific flags (e.g. cortex-a9 for Zynq, v10.0 for MicroBlaze).
#==============================================================================

XILINX_PATH:=C:/Xilinx/SDK/2018.2/gnu/

$(eval TARGET_GCC=$(call REMOVE_NUMBERS,'$(TARGET_CORE)'))
-include gcc_$(TARGET_GCC).mk
$(eval GCC_PLATFORM=$(call UPPERCASE,'$(TARGET_GCC)'))

ifneq (GCC_$(GCC_PLATFORM)_LD,)
    # Common GCC flags
    GCC_WFLAGS= \
        -std=gnu11 \
        -Werror \
        -pedantic-errors \
        -Wall \
        -Wextra \
        -Wpedantic

    # GCC flags concat
    GCC_FLAGS=$(GCC_TARGET_FLAGS) $(GCC_WFLAGS)
    GCC_CC_FLAGS=$(GCC_INCFLAGS) $(GCC_DEFFLAGS)
    GCC_LD_FLAGS=$(GCC_LIBFLAGS)

    # GCC toolchain CLI call commands
    GCC_LD=$(TOOLCHAIN_PATH)$(TOOLCHAIN)-gcc $(GCC_FLAGS) $(GCC_LD_FLAGS)
    GCC_CC=$(TOOLCHAIN_PATH)$(TOOLCHAIN)-gcc $(GCC_FLAGS) $(GCC_CC_FLAGS)
    GCC_SIZE=$(TOOLCHAIN_PATH)$(TOOLCHAIN)-size

    # GCC toolchain CLI call commands
    GCC_$(GCC_PLATFORM)_LD:=$(GCC_LD)
    GCC_$(GCC_PLATFORM)_CC:=$(GCC_CC)
    GCC_$(GCC_PLATFORM)_SIZE:=$(GCC_SIZE)
endif
