OS_NAME:=$(shell uname -s 2>/dev/null)

# Utilities
ifeq ($(OS_NAME),)
    # Windows
    RM=$(shell rd /s /q "$(1)")
    MKDIR=$(shell if not exist "$(1)" mkdir "$(1)")
    UPPERCASE=$(shell powershell.exe -Command "&{Write-Output $(1).ToUpper()}")
    REMOVE_NUMBERS = $(shell powershell.exe -Command "&{Write-Output ($(1) -replace '\d+$$$$', '')}")
else
    # Linux
    RM = $(shell rm -rf "$(1)")
    MKDIR = $(shell mkdir -p "$(1)")
    UPPERCASE = $(shell echo "$(1)" | tr '[:lower:]' '[:upper:]')
    REMOVE_NUMBERS = $(shell echo "$(1)" | sed 's/[0-9][0-9]*$$//g')
endif
