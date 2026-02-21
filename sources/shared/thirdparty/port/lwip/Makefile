# lwip library src (thirdparty/lwip/src)
LWIPDIR:=$(LWIPLIB_DIR)/src

# Reset on each inclusion (multi-target build)
LWIP_INCFLAGS:=
LWIP_SOURCES:=

-include $(LWIPDIR)/Filelists.mk

# LWIP core
LWIP_INCFLAGS += -I"$(LWIPDIR)/include"
LWIP_SOURCES += $(LWIPNOAPPSFILES)

# Port layer (thirdparty/port/lwip/port/)
LWIPPORTDIR:=$(LWIPPORT_DIR)/port/
LWIPARCHPORTDIR:=$(LWIPPORT_DIR)/port/arch_$(TARGET_PROJECT)/

LWIP_INCFLAGS += -I"$(LWIPPORTDIR)" -I"$(LWIPARCHPORTDIR)"
LWIP_SOURCES += $(LWIPPORTDIR)elrtos.c