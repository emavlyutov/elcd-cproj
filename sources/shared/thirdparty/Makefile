THIRDPARTY_INCFLAGS=
THIRDPARTY_SOURCES=

# lwip library: thirdparty/lwip/ (src, include)
LWIPLIB_DIR=$(THIRDPARTY_DIR)lwip
# lwip port: thirdparty/port/lwip/ (port/elrtos.c, port/arch_*)
LWIPPORT_DIR=$(THIRDPARTY_DIR)port/lwip

-include $(LWIPPORT_DIR)/Makefile

THIRDPARTY_INCFLAGS+= $(LWIP_INCFLAGS)
THIRDPARTY_SOURCES+= $(LWIP_SOURCES)
