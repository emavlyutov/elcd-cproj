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

COMMON_SOURCES=
COMMON_INCFLAGS=

ifneq ($(CONFIG_PERIPH_CM_CONFIG_EN)$(CONFIG_PERIPH_CM_DATA_EN),)
    COMMON_INCFLAGS+= -I"$(SOURCE_PATH)common/periph/cm"
endif
ifeq ($(CONFIG_PERIPH_CM_CONFIG_EN),y)
    COMMON_SOURCES+= $(SOURCE_PATH)common/periph/cm/cm_config.c
endif
ifeq ($(CONFIG_PERIPH_CM_DATA_EN),y)
    COMMON_SOURCES+= $(SOURCE_PATH)common/periph/cm/cm_data.c
endif

ifeq ($(CONFIG_PERIPH_TRNG_EN),y)
    COMMON_SOURCES+= $(SOURCE_PATH)common/periph/trng/trng.c
    COMMON_INCFLAGS+= -I"$(SOURCE_PATH)common/periph/trng"
endif

ifeq ($(CONFIG_PERIPH_SFP_EN),y)
    COMMON_SOURCES+= $(SOURCE_PATH)common/periph/sfp/sfp.c
    COMMON_INCFLAGS+= -I"$(SOURCE_PATH)common/periph/sfp"
endif
