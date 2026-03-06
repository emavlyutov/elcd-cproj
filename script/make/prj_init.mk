#==============================================================================
# makesys/prj_init.mk - Project init and source aggregation
#==============================================================================
# Builds single project (TARGET_PROJECT from root). Requires SOURCE_PATH from
# Makefile. Includes platform.config, aggregates SRCS from ElAPI/thirdparty/project.
#==============================================================================
ifeq (,$(TARGET_WS))
$(error TARGET_WS is not set)
endif
ifeq (,$(TARGET_PROJECT))
$(error TARGET_PROJECT is not set)
endif
ifeq (,$(TARGET_BSP))
$(error TARGET_BSP is not set)
endif
ifeq (,$(MAKESCRIPT_PATH))
$(error TARGET_WS is not set)
endif
ifeq (,$(SOURCES_PATH))
$(error SOURCES_PATH is not set)
endif
ifeq (,$(BUILD_PATH))
$(error BUILD_PATH is not set)
endif
ifeq (,$(ELF_PATH))
$(error ELF_PATH is not set)
endif
ifeq (,$(LIBS_PATH))
$(error LIBS_PATH is not set)
endif
ifeq (,$(BSP_PATH))
$(error BSP_PATH is not set)
endif
ifeq (,$(SHARED_PATH))
$(error SHARED_PATH is not set)
endif
ifeq (,$(COMMON_PATH))
$(error COMMON_PATH is not set)
endif

include $(MAKESCRIPT_PATH)shell.mk

# Read project configuration
include $(SOURCES_PATH)platform.config

# Source aggregation
SRCS:=
OBJS:=
LIBS:=
INCFLAGS:=
DEFFLAGS:=
LIBFLAGS:=
include $(BSP_PATH)Makefile
include $(SOURCES_PATH)Makefile
include $(SHARED_PATH)Makefile
include $(COMMON_PATH)Makefile

# Initialize toolchain
include $(MAKESCRIPT_PATH)toolchain.mk

# Build object list
OBJS := $(OBJS) $(patsubst %.c, $(BUILD_PATH)/%.o, $(SRCS_COMMON) $(SRCS:$(SOURCE_PATH)%.c=%.c))
LIBSLIST := $(lastword $(patsubst %.a, $(lastword $(subst /, ,$(%))), $(LIBS)))
LDFLAGS := -Wl,-T -Wl,"$(GCC_LDSCRIPT_PATH)" $(LIBFLAGS) $(LIBSLIST:%=-l%)

$(BUILD_PATH)/%.o: $(SOURCE_PATH)%.c
	@echo "Compiling source $<"
	@$(call MKDIR,$(dir $@))
	@$(GCC_CC) $(INCFLAGS) $(DEFFLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo "Done"

$(ELF_PATH)$(TARGET_PROJECT).elf: $(OBJS) $(LIBS)
	@echo "Linking elf file $<"
	@$(call MKDIR,$(dir $@))
	@$(GCC_LD) $(LDFLAGS) $(OBJS) -o "$@" 
	@echo "Done"

$(ELF_PATH)$(TARGET_PROJECT).elf.size: $(ELF_PATH)$(TARGET_PROJECT).elf
	@echo "Size $(TARGET_PROJECT).elf"
	@$(GCC_SIZE) "$<" > "$@"
	@echo "Done"
