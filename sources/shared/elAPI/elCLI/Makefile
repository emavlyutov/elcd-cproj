#==============================================================================
# elCLI/Makefile - ElCyberDev command-line interface
#==============================================================================
# Adds CLI sources when CONFIG_ELAPI_CLI_EN is set.
# When OS_ELRTOS_EN: uses elRTOS (queue, task, mutex). Else: bare-metal stubs.
#==============================================================================

ELCLISOURCES=
ELCLIINCFLAGS=
ELCLIDEFFLAGS=

ifeq ($(CONFIG_ELAPI_CLI_EN),y)
    ELCLISOURCES=							\
        $(ELCLIDIR)cli.c						\
        $(ELCLIDIR)cli_cfg.c					\
        $(ELCLIDIR)cli_auth.c					\
        $(ELCLIDIR)cli_charfunc.c				\
        $(ELCLIDIR)cli_kbdfunc.c				\
        $(ELCLIDIR)cli_cmdhandler.c				\
        $(ELCLIDIR)cli_apps.c
    ELCLIINCFLAGS= -I"$(ELCLIDIR)include" -I"$(ELCLIDIR)"
    ELCLIDEFFLAGS= -DCONFIG_ELAPI_CLI_EN
endif
