#==============================================================================
# makesys/prj_init.mk - Project init and source aggregation
#==============================================================================
# Builds single project (TARGET_PROJECT from root). Requires SOURCE_PATH from
# Makefile. Includes platform.config, aggregates SRCS from ElAPI/thirdparty/project.
#==============================================================================

SRCS:=
OBJS:=
LIBS:=
INCFLAGS=-I"$(SOURCE_PATH)"
DEFFLAGS=
LIBFLAGS=

# PROJECT_NAME (from mktarg prj_init): sources/<project>/<target>/, else sources/<target>/
ifeq ($(PROJECT_NAME),)
PROJECT_PATH:=$(SOURCE_PATH)$(TARGET_PROJECT)/
MAIN_SRC:=$(SOURCE_PATH)main.c
else
PROJECT_PATH:=$(SOURCE_PATH)$(PROJECT_NAME)/$(TARGET_PROJECT)/
MAIN_SRC:=$(SOURCE_PATH)$(PROJECT_NAME)/main.c
endif

-include $(PROJECT_PATH)platform.config

# Source aggregation
SRCS=$(MAIN_SRC)
ifeq ($(CONFIG_AMP_EN),y)
    DEFFLAGS+= -DUSE_AMP
endif
ifeq ($(CONFIG_ELAPI_ELRTOS_EN),y)
    LIBS+=-lfreertos,
endif

COMMON_DIR=$(SOURCE_PATH)shared/
-include $(COMMON_DIR)Makefile
SRCS+= $(COMMON_SOURCES)
INCFLAGS+= $(COMMON_INCFLAGS)

ELAPIDIR=$(COMMON_DIR)elAPI/
-include $(ELAPIDIR)Makefile
SRCS+= $(ELAPISOURCES)
INCFLAGS+= $(ELAPIINCFLAGS)
DEFFLAGS+= $(ELAPIDEFFLAGS)

THIRDPARTY_DIR=$(COMMON_DIR)thirdparty/
ifeq ($(CONFIG_THIRDPARTY_EN),y)
    -include $(THIRDPARTY_DIR)Makefile
    SRCS+= $(THIRDPARTY_SOURCES)
    INCFLAGS+= $(THIRDPARTY_INCFLAGS)
endif

-include $(PROJECT_PATH)Makefile

# Build object list
OBJS := $(OBJS_COMMON) $(OBJS) $(patsubst %.c, $(BUILD_PATH)$(TARGET_PROJECT)/%.o, $(SRCS_COMMON) $(SRCS:$(SOURCE_PATH)%.c=%.c))
LIBS := $(strip $(LIBS_COMMON)$(LIBS))

-include toolchain.mk
GCC_LD := $(GCC_$(GCC_PLATFORM)_LD) $(GCC_$(GCC_PLATFORM)_LD_FLAGS) $(LIBFLAGS)
GCC_CC := $(GCC_$(GCC_PLATFORM)_CC) $(GCC_$(GCC_PLATFORM)_CC_FLAGS) $(INCFLAGS) $(DEFFLAGS)
GCC_SIZE := $(GCC_$(GCC_PLATFORM)_SIZE)

$(BUILD_PATH)$(TARGET_PROJECT)/%.o: $(SOURCE_PATH)%.c
	@echo "Compiling $(TARGET_PROJECT), source $<"
	@$(call MKDIR,$(dir $@))
	@$(GCC_CC) -O0 -g3 -c -ffunction-sections -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo "Done"

$(ELF_PATH)$(TARGET_PROJECT).elf: $(OBJS) $(PROJECT_PATH)ldscript.ld
	@echo "Linking $(TARGET_PROJECT).elf"
	@$(call MKDIR,$(dir $@))
	@$(GCC_LD) -Wl,-T -Wl,"$(PROJECT_PATH)ldscript.ld" -Wl,--gc-sections -o "$@" $(OBJS) -Wl,--start-group,-lxil,-lgcc,-lc,$(LIBS)--end-group
	@echo "Done"

$(ELF_PATH)$(TARGET_PROJECT).elf.size: $(ELF_PATH)$(TARGET_PROJECT).elf
	@echo "Size $(TARGET_PROJECT).elf"
	@$(GCC_SIZE) "$<" > "$@"
	@echo "Done"
