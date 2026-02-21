#==============================================================================
# elAPI/Makefile - ElCyberDev API build configuration
#==============================================================================
# Aggregates HAL, elRTOS, elCrypto sources and defines. Conditionally includes
# sub-Makefiles based on CONFIG_ELAPI_HAL_EN, CONFIG_ELAPI_ELRTOS_EN, etc.
# Defines: OS_ELRTOS_EN, OS_ELRTOS_IRQ_MANAGED, CONFIG_ELAPI_HAL_*
#==============================================================================

ELAPISOURCES=
ELAPIDEFFLAGS=

ELAPIINCFLAGS=					\
        -I"$(ELAPIDIR)"			\
        -I"$(ELAPIDIR)elUtils"	\

ifeq ($(CONFIG_ELAPI_HAL_EN),y)
    ELHALDIR=$(ELAPIDIR)elHAL/
    -include $(ELHALDIR)Makefile
    ELAPISOURCES+= $(ELHALSOURCES)
    ELAPIINCFLAGS+= $(ELHALINCFLAGS)
    ELAPIDEFFLAGS+= $(ELHALDEFFLAGS)
endif

# elRTOS: core + optional modules per platform.config; stub when CLI needs it but ELRTOS disabled
ifeq ($(CONFIG_ELAPI_ELRTOS_EN),y)
    ELRTOSDIR=$(ELAPIDIR)elRTOS/
    -include $(ELRTOSDIR)Makefile
    ELAPISOURCES+= $(ELRTOSSOURCES)
    ELAPIINCFLAGS+= $(ELRTOSINCFLAGS)
    ELAPIDEFFLAGS+= -DOS_ELRTOS_EN
    ifeq ($(CONFIG_ELAPI_ELRTOS_QUEUE_EN),y)
        ELAPIDEFFLAGS+= -DOS_ELRTOS_QUEUE_EN
    endif
    ifeq ($(CONFIG_ELAPI_ELRTOS_SEMAPHORE_EN),y)
        ELAPIDEFFLAGS+= -DOS_ELRTOS_SEMAPHORE_EN
    endif
    ifeq ($(CONFIG_ELAPI_ELRTOS_MUTEX_EN),y)
        ELAPIDEFFLAGS+= -DOS_ELRTOS_MUTEX_EN
    endif
    ifeq ($(CONFIG_ELAPI_ELRTOS_EVENT_EN),y)
        ELAPIDEFFLAGS+= -DOS_ELRTOS_EVENT_EN
    endif
    ifeq ($(CONFIG_ELAPI_ELRTOS_IRQ_MANAGED),y)
        ELAPIDEFFLAGS+= -DOS_ELRTOS_IRQ_MANAGED
    endif
    ifeq ($(CONFIG_ELAPI_ELRTOS_DRIVER_EN),y)
        ELAPIDEFFLAGS+= -DOS_ELRTOS_DRIVER_EN
    endif
else ifeq ($(CONFIG_ELAPI_CLI_EN),y)
    ELRTOSDIR=$(ELAPIDIR)elRTOS/
    ELAPISOURCES+= $(ELRTOSDIR)port.c
    ELAPIINCFLAGS+= -I"$(ELRTOSDIR)"
endif

ifeq ($(CONFIG_ELAPI_ELCRYPTO_EN),y)
    ELCRYPTODIR=$(ELAPIDIR)elCrypto/
    -include $(ELCRYPTODIR)Makefile
    ELAPISOURCES+= $(ELCRYPTOSOURCES)
    ELAPIINCFLAGS+= $(ELCRYPTOINCFLAGS)
endif

ifeq ($(CONFIG_ELAPI_CLI_EN),y)
    ELCLIDIR=$(ELAPIDIR)elCLI/
    -include $(ELCLIDIR)Makefile
    ELAPISOURCES+= $(ELCLISOURCES)
    ELAPIINCFLAGS+= $(ELCLIINCFLAGS)
    ELAPIDEFFLAGS+= $(ELCLIDEFFLAGS)
endif
