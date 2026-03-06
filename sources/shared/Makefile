#==============================================================================
# common/Makefile - Peripherals (cm_config, cm_data, trng)
#==============================================================================
# Driven by platform.config: CONFIG_PERIPH_CM_CONFIG_EN, CONFIG_PERIPH_CM_DATA_EN,
# CONFIG_PERIPH_TRNG_EN, CONFIG_PERIPH_SFP_EN.
# CM config: key load (MMIO), syscpu.
# CM data: frame crypto (DMA), home/world; requires CONFIG_ELAPI_HAL_DMA_EN.
# TRNG: random bytes (MMIO), syscpu.
# SFP: transceiver (I2C, GPIO), home/world; requires GPIO, IIC.
#==============================================================================

SRCS+= $(SHARED_PATH)main.c

ELAPIDIR=$(SHARED_PATH)elAPI/
-include $(ELAPIDIR)Makefile
SRCS+= $(ELAPISRCS)
OBJS+= $(ELAPIOBJS)
LIBS+= $(ELAPILIBS)
INCFLAGS+= $(ELAPIINCFLAGS)
DEFFLAGS+= $(ELAPIDEFFLAGS)
LIBFLAGS+= $(ELAPILIBFLAGS)

THIRDPARTYDIR=$(SHARED_PATH)thirdparty/
ifeq ($(CONFIG_THIRDPARTY_EN),y)
    -include $(THIRDPARTYDIR)Makefile
    SRCS+= $(THIRDPARTYSRCS)
    OBJS+= $(THIRDPARTYOBJS)
    LIBS+= $(THIRDPARTYLIBS)
    INCFLAGS+= $(THIRDPARTYINCFLAGS)
    DEFFLAGS+= $(THIRDPARTYDEFFLAGS)
    LIBFLAGS+= $(THIRDPARTYLIBFLAGS)
endif
