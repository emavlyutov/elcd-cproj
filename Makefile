MAKE := make
MAKESYS := sources/Makefile	

ifneq ($(WS_NAME),)
$(error WS_NAME is not set)
endif
ifneq ($(PRJ_NAME),)
$(error PRJ_NAME is not set)
endif
ifneq ($(BSP_NAME),)
$(error BSP_NAME is not set)
endif

.PHONY: debug release clean

debug:
	$(MAKE) -C $(MAKESYS) TARGET_WS=$(WS_NAME) TARGET_PROJECT=$(PRJ_NAME) TARGET_BSP=$(BSP_NAME) __DEBUG_EN=y

release:
	$(MAKE) -C $(MAKESYS) TARGET_WS=$(WS_NAME) TARGET_PROJECT=$(PRJ_NAME) TARGET_BSP=$(BSP_NAME)

clean:
	$(MAKE) -C $(MAKESYS) TARGET_WS=$(WS_NAME) TARGET_PROJECT=$(PRJ_NAME) TARGET_BSP=$(BSP_NAME) clean
