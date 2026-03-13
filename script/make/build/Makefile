#==============================================================================
# makesys/Makefile - Single-project build system
#==============================================================================
# Builds one ELF per invocation. Requires TARGET_PROJECT (fsbl|syscpu|home|world).
# Usage: make -C makesys TARGET_PROJECT=world
# Includes: prj_init.mk (platform.config, sources), toolchain.mk (gcc_arm|gcc_mb)
# Output: ../build/elf/$(TARGET_PROJECT).elf
#==============================================================================

.SUFFIXES:
.PHONY: all clean

-include shell.mk

# Common build list
INCF_COMMON=
LIBS_COMMON=
OBJS_COMMON=
SRCS_COMMON=

# Paths (makesys/ is cwd when invoked via make -C makesys)
SOURCE_PATH=../sources/
BUILD_PATH=../build/
ELF_PATH=$(BUILD_PATH)elf/
BSP_BASE_PATH=../hw_bsp/

# TARGET_PROJECT required for build (not for clean)
ifndef TARGET_PROJECT
ifneq ($(MAKECMDGOALS),clean)
$(error TARGET_PROJECT is required. Use: make -C makesys TARGET_PROJECT=fsbl)
endif
TARGET_PROJECT:=dummy
endif

ifneq ($(MAKECMDGOALS),clean)
-include prj_init.mk
endif

all: ecd_pre-build $(ELF_PATH)$(TARGET_PROJECT).elf.size ecd_post-build

clean:
	$(call RM,$(BUILD_PATH))

ecd_pre-build:
	$(call MKDIR,$(ELF_PATH))

ecd_post-build:
