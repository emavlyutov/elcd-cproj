#==============================================================================
# sources/Makefile - Source directory configuration
#==============================================================================
# This file contains the configuration for the source directory.
#==============================================================================

CURRENT_DIR:=$(shell pwd)
MAKESCRIPT_PATH:=$(CURRENT_DIR)/../script/make/

ifeq (,$(TARGET_WS))
$(error TARGET_WS is not set)
endif
ifeq (,$(TARGET_PROJECT))
$(error TARGET_PROJECT is not set)
endif
ifeq (,$(TARGET_BSP))
$(error TARGET_BSP is not set)
endif

SOURCES_PATH:=$(CURRENT_DIR)/$(TARGET_WS)/$(TARGET_PROJECT)/
BUILD_PATH:=$(CURRENT_DIR)/../build/$(TARGET_PROJECT)/
ELF_PATH:=$(BUILD_PATH)elf/
BSP_PATH:=$(SOURCES_PATH)/bsp/$(TARGET_BSP)/
SHARED_PATH:=$(CURRENT_DIR)/shared/
COMMON_PATH:=$(CURRENT_DIR)/$(TARGET_WS)/common/

EXTRA_CLEAN:=

include $(MAKESCRIPT_PATH)prj_init.mk

.PHONY: all clean
all: $(ELF_PATH)$(TARGET_PROJECT).elf
clean: $(EXTRA_CLEAN)
	$(call RM,$(BUILD_PATH))
