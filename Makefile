MAKESYS := makesys
MAKE := make

-include sources/Makefile

.PHONY: all clean build_%

PROJECT_NAME ?=
all: release

build_%:
	$(MAKE) -C $(MAKESYS) TARGET_PROJECT=$* PROJECT_NAME=$(PROJECT_NAME)

clean:
	$(MAKE) -C $(MAKESYS) clean
